CC = gcc
CFLAGS = -Wall -std=gnu99
LD = gcc
LDFLAGS = -g -std=gnu99
LDLIBS = 

DEBUGFLAG = 0

SRCS = bmp280-cli.c bmp280.c
OBJS = bmp280-cli.o bmp280.o
TARGETS = bmp280-cli debug

debug: CFLAGS += -DDEBUG -g

all: ${TARGETS}

bmp280-cli: $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

debug: $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) 
