CC:=gcc
CFLAGS:=

OBJS:=$(patsubst %.c,%.o,$(wildcard *.c))
OBJS+=$(patsubst %.c,%.o,$(wildcard mcx/c/*.c))
DFILES:=$(patsubst %.o,%.d,$(OBJS))

all: 4theme clean

4theme: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -rf $(OBJS) $(DFILES)

$(OBJS):
-include $(DFILES)
