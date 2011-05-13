CC=gcc
CFLAGS=-Wall -g
OBJS=elf_compass.o list.o

elf_compass: $(OBJS)
	$(CC) $(CFLAGS)   -o elf_compass $(OBJS)

clean:
	rm -f $(OBJS) core elf_compass
