CUR_SOURCE	= $(wildcard *.c)
CUR_OBJS	= $(patsubst %.c,%.o,$(CUR_SOURCE))

INC_PATH	= "-I${OUT_DIR}/build/libnl-3.5.0/include/"
SHARED_LIBS	= "-L${OUT_DIR}/build/libnl-3.5.0/lib/"

TARGET := my_tmon
CFLAGS = -Wall -g -finstrument-functions $(INC_PATH) $(SHARED_LIBS) -Wl,-rpath=./
LDFLAGS = -lnl-genl-3 -lnl-3

all: $(TARGET)

$(TARGET): $(CUR_OBJS)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	@rm -rf *.o $(TARGET)

.PHONY: all clean
