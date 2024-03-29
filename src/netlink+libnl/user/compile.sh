#!/bin/bash

gcc -I/usr/include/libnl3 netlink_user.c -lnl-3 -o netlink_user
