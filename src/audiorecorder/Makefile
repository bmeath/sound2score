
CFLAGS		+= 	-O2 -Wall -I. -I../common
LDLIBS 		+= 	-lopenal

EXEC 		= 	audiorecorder
SOURCES 	=   wavrecorder.c wav.c stringutils.c ../common/endianness.c
OBJECTS 	= 	$(SOURCES:.c=.o)


all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -g
debug: $(EXEC)

clean:
	$(RM) $(EXEC) *.o *.gdb

.PHONY:
	clean debug
