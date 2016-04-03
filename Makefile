# Debug flag (Comment this line to generate production binaries)
DEBUGFLAG = 1

CC = /usr/bin/gcc
LD = /usr/bin/gcc
RM = /bin/rm -f
CFLAGS = -Wall -Wpedantic -Werror
LDFLAGS = -lpthread
OBJS = thpool.o example.o
BINARY = example

# Debug options
ifeq ($(DEBUGFLAG), 1)
	CFLAGS += -g
endif

all: $(BINARY)

$(BINARY): $(OBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(BINARY) *.o core
