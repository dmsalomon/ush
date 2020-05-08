
CFLAGS:=-Wall -g
PROGS:=\
	ush\
	ushd
SRC:=$(wildcard *.c)
DEP:=$(patsubst %.c,%.d,$(SRC))

all: $(PROGS)

ush: ush.o util.o
	$(CC) $^ -o $@

ushd: ushd.o util.o
	$(CC) -lutil $^ -o $@

%.o: %.c Makefile
	$(CC) -MMD -MP -c $< -o $@

clean:
	$(RM) *.o *.d $(PROGS)

.PHONY: clean all
