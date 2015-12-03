TARGET: czekamnaudp ileczekam

CC	= cc
CFLAGS	= -Wall -O2 -std=c99
LFLAGS	= -Wall

SRC_FILES = err.c err.h portable_endian.h time_measure.h

all: czekamnaudp ileczekam

czekamnaudp: czekamnaudp.c $(SRC_FILES)
	$(CC) $(LFLAGS) $^ -o $@

ileczekam: ileczekam.c $(SRC_FILES)
	$(CC) $(LFLAGS) $^ -o $@

.PHONY: clean TARGET

clean:
	rm -f czekamnaudp ileczekam *.o *~ *.bak
