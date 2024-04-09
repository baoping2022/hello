CC = gcc
CFLAGS = -Wall
LIBS = -lmnl

TARGETS = genl-family-get my_event my_user

all: $(TARGETS)

genl-family-get: genl-family-get.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

my_event: my_event.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

my_user: my_user.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(TARGETS)
