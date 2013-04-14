#######################################
# A simple Makefile
#######################################
CC=gcc

CFLAGS= -g -Wall
LDFLAGS= -lSDL

target = capture

all: clean $(target)

$(target): v4l2uvc.o main.o utils.o color.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	@echo cleaning...
	rm -f *.o $(target)
	@echo done.

install:
	@echo "                     Note:"
	@echo "To install or not install,that is the question"
	@echo 

.PHONY:all clean install

### end of Makefile
