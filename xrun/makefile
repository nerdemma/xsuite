CC=cc
CFLAGS = -I/usr/X11R6/include
LDFLAGS = -L/usr/X11R6/lib
LDLIBS = -lX11 

SRCS = main.c
PROG = xrun

all: $(PROG)

$(PROG): $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $(PROG) $(LDLIBS)

clean:
	rm -f $(PROG) *.o

