
PROGS	= audiorecorder audiotranscriber

BIN_DIR	= ../build

# keep empty.
MAKE_DEBUG	=

all:
	mkdir -p $(BIN_DIR)
	@for PROG in $(PROGS); do \
		cd $$PROG && make $(MAKE_DEBUG) && cd ..;\
		cp $$PROG/$$PROG $(BIN_DIR); \
	done 

debug: MAKE_DEBUG = debug
debug: all

clean:
	@for PROG in $(PROGS); do \
		cd $$PROG && make clean && cd ..; \
		$(RM) $(BIN_DIR)/$$PROG; \
	done
	
	$(RM) common/*.o

.PHONY:
	clean debug
