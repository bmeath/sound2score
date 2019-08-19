
CFLAGS		+= 	-O2 -Wall -I. -I../common -I../audiotranscriber -I../audiorecorder

LDLIBS      +=  -lm -laubio -lopenal

EXEC 		= 	unit_tests
SOURCES 	= 	main.c ../audiotranscriber/noteextractor.c ../audiotranscriber/midiwriter.c ../audiotranscriber/midi.c ../audiotranscriber/memory.c ../common/endianness.c ../audiorecorder/wav.c ../audiorecorder/stringutils.c
OBJECTS 	= 	$(SOURCES:.c=.o)


all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

clean:
	$(RM) $(EXEC) *.o *.gdb

.PHONY:
	clean debug
