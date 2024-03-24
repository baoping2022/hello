// SPDX-License-Identifier: GPL-2.0-only
/*
 * Thermal monitoring tool based on the thermal netlink events.
 *
 * Copyright (C) 2022 Linaro Ltd.
 *
 * Author: Daniel Lezcano <daniel.lezcano@kernel.org>
 */
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <syslog.h>

#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "./include/thermal.h"
#include "mainloop.h"

#ifndef __maybe_unused
#define __maybe_unused          __attribute__((__unused__))
#endif

struct options {
	int loglevel;
	int logopt;
	int interactive;
	int daemonize;
};

struct thermal_data {
	struct thermal_zone *tz;
	struct thermal_handler *th;
};

struct hd_thermal_info {
	char log[1024];
};

enum {
	THERMAL_ENGINE_SUCCESS = 0,
	THERMAL_ENGINE_OPTION_ERROR,
	THERMAL_ENGINE_DAEMON_ERROR,
	THERMAL_ENGINE_LOG_ERROR,
	THERMAL_ENGINE_THERMAL_ERROR,
	THERMAL_ENGINE_MAINLOOP_ERROR,
};

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
#define ADDR_BASE (0x200000000 + 0x1D400000)

static int h_thermal_info_memcpy(char *msg)
{
	int fd;
	void *map_base;
	struct hd_thermal_info *info_ptr, *info_ptr2;

	info_ptr2 = malloc(sizeof(struct hd_thermal_info));
	if (!info_ptr2) {
		fprintf(stderr, "Failed to allocate memory\n");
		return -1;
	}
	memset(info_ptr2, 0, sizeof(struct hd_thermal_info));

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (-1 == fd) {
		fprintf(stderr, "open /dev/mem  fail!\n");
		free(info_ptr2);
		return -1;
	}

	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, ADDR_BASE & ~MAP_MASK);
	if (map_base == MAP_FAILED) {
		fprintf(stderr, "mmap  fail! %d(%s)\n", errno, strerror(errno));
		close(fd);
		free(info_ptr2);
		return -1;
	}
	info_ptr = (struct hd_thermal_info *)map_base;

	memcpy(info_ptr2, info_ptr, sizeof(struct hd_thermal_info));
	strcpy(info_ptr2->log, msg);
	memcpy(info_ptr, info_ptr2, sizeof(struct hd_thermal_info));

	munmap(map_base, MAP_SIZE);
	close(fd);
	free(info_ptr2);

	return 0;
}

static int show_trip(struct thermal_trip *tt, __maybe_unused void *arg)
{
	fprintf(stdout, "trip id=%d, type=%d, temp=%d, hyst=%d\n",
	     tt->id, tt->type, tt->temp, tt->hyst);

	return 0;
}

static int show_governor(struct thermal_zone *tz, __maybe_unused void *arg)
{
	thermal_cmd_get_governor(arg, tz);

	fprintf(stdout, "governor: '%s'\n", tz->governor);

	return 0;
}

static int show_temp(struct thermal_zone *tz, __maybe_unused void *arg)
{
	thermal_cmd_get_temp(arg, tz);

	fprintf(stdout, "temperature: %d\n", tz->temp);

	return 0;
}

static int show_tz(struct thermal_zone *tz, __maybe_unused void *arg)
{
	fprintf(stdout, "thermal zone '%s', id=%d\n", tz->name, tz->id);

	for_each_thermal_trip(tz->trip, show_trip, NULL);

	show_temp(tz, arg);

	show_governor(tz, arg);

	return 0;
}

static int tz_create(const char *name, int tz_id, __maybe_unused void *arg)
{
	fprintf(stdout, "Thermal zone '%s'/%d created\n", name, tz_id);

	return 0;
}

static int tz_delete(int tz_id, __maybe_unused void *arg)
{
	fprintf(stdout, "Thermal zone %d deleted\n", tz_id);

	return 0;
}

static int tz_disable(int tz_id, void *arg)
{
	struct thermal_data *td = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(td->tz, tz_id);

	fprintf(stdout, "Thermal zone %d ('%s') disabled\n", tz_id, tz->name);

	return 0;
}

static int tz_enable(int tz_id, void *arg)
{
	struct thermal_data *td = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(td->tz, tz_id);

	fprintf(stdout, "Thermal zone %d ('%s') enabled\n", tz_id, tz->name);

	return 0;
}

static int trip_high(int tz_id, int trip_id, int temp, void *arg)
{
	char buffer[256] = {0};
	struct thermal_data *td = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(td->tz, tz_id);

	sprintf(buffer, "Thermal zone %d ('%s'): trip point %d crossed way up with %d C\n",
	     tz_id, tz->name, trip_id, temp);

	fprintf(stdout, "%s", buffer);

	h_thermal_info_memcpy(buffer);

	return 0;
}

static int trip_low(int tz_id, int trip_id, int temp, void *arg)
{
	struct thermal_data *td = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(td->tz, tz_id);

	fprintf(stdout, "Thermal zone %d ('%s'): trip point %d crossed way down with %d C\n",
	     tz_id, tz->name, trip_id, temp);

	return 0;
}

static int trip_add(int tz_id, int trip_id, int type, int temp, int hyst, __maybe_unused void *arg)
{
	fprintf(stdout, "Trip point added %d: id=%d, type=%d, temp=%d, hyst=%d\n",
	     tz_id, trip_id, type, temp, hyst);

	return 0;
}

static int trip_delete(int tz_id, int trip_id, __maybe_unused void *arg)
{
	fprintf(stdout, "Trip point deleted %d: id=%d\n", tz_id, trip_id);

	return 0;
}

static int trip_change(int tz_id, int trip_id, int type, int temp,
		       int hyst, __maybe_unused void *arg)
{
	struct thermal_data *td = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(td->tz, tz_id);

	fprintf(stdout, "Trip point changed %d: id=%d, type=%d, temp=%d, hyst=%d\n",
	     tz_id, trip_id, type, temp, hyst);

	tz->trip[trip_id].type = type;
	tz->trip[trip_id].temp = temp;
	tz->trip[trip_id].hyst = hyst;

	return 0;
}

static int cdev_add(const char *name, int cdev_id, int max_state, __maybe_unused void *arg)
{
	fprintf(stdout, "Cooling device '%s'/%d (max state=%d) added\n", name, cdev_id, max_state);

	return 0;
}

static int cdev_delete(int cdev_id, __maybe_unused void *arg)
{
	fprintf(stdout, "Cooling device %d deleted", cdev_id);

	return 0;
}

static int cdev_update(int cdev_id, int cur_state, __maybe_unused void *arg)
{
	fprintf(stdout, "cdev:%d state:%d\n", cdev_id, cur_state);

	return 0;
}

static int gov_change(int tz_id, const char *name, __maybe_unused void *arg)
{
	struct thermal_data *td = arg;
	struct thermal_zone *tz = thermal_zone_find_by_id(td->tz, tz_id);

	fprintf(stdout, "%s: governor changed %s -> %s\n", tz->name, tz->governor, name);

	strcpy(tz->governor, name);

	return 0;
}

static int show_sampling_temp(int tz_id, int temp, void *arg)
{
	fprintf(stdout, "tz_id: %d, temperature: %d\n", tz_id, temp);

	return 0;
}

static struct thermal_ops ops = {
	.events.tz_create	= tz_create,
	.events.tz_delete	= tz_delete,
	.events.tz_disable	= tz_disable,
	.events.tz_enable	= tz_enable,
	.events.trip_high	= trip_high,
	.events.trip_low	= trip_low,
	.events.trip_add	= trip_add,
	.events.trip_delete	= trip_delete,
	.events.trip_change	= trip_change,
	.events.cdev_add	= cdev_add,
	.events.cdev_delete	= cdev_delete,
	.events.cdev_update	= cdev_update,
	.events.gov_change	= gov_change,
	.sampling.tz_temp	= show_sampling_temp,
};

static int thermal_event(__maybe_unused int fd, __maybe_unused void *arg)
{
	struct thermal_data *td = arg;

	return thermal_events_handle(td->th, td);
}

static int thermal_sampling(__maybe_unused int fd, __maybe_unused void *arg)
{
	struct thermal_data *td = arg;

	return thermal_sampling_handle(td->th, td);
}

int main(int argc, char *argv[])
{
	struct thermal_data td;

	td.th = thermal_init(&ops);
	if (!td.th) {
		fprintf(stderr, "Failed to initialize the thermal library\n");
		return THERMAL_ENGINE_THERMAL_ERROR;
	}

	td.tz = thermal_zone_discover(td.th);
	if (!td.tz) {
		fprintf(stderr, "No thermal zone available\n");
		return THERMAL_ENGINE_THERMAL_ERROR;
	}

	for_each_thermal_zone(td.tz, show_tz, td.th);

	if (mainloop_init()) {
		fprintf(stderr, "Failed to initialize the mainloop\n");
		return THERMAL_ENGINE_MAINLOOP_ERROR;
	}

	if (mainloop_add(thermal_events_fd(td.th), thermal_event, &td)) {
		fprintf(stderr, "Failed to setup the mainloop\n");
		return THERMAL_ENGINE_MAINLOOP_ERROR;
	}

	if (mainloop_add(thermal_sampling_fd(td.th), thermal_sampling, &td)) {
			fprintf(stderr, "Failed to setup the mainloop\n");
			return THERMAL_ENGINE_MAINLOOP_ERROR;
	}

	printf("Waiting for thermal events ...\n");

	if (mainloop(-1)) {
		fprintf(stderr, "Mainloop failed\n");
		return THERMAL_ENGINE_MAINLOOP_ERROR;
	}

	return THERMAL_ENGINE_SUCCESS;
}
