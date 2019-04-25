CONSOLE = jms_console.o
COORD = jms_coord.o
POOL = pool.o
OUTCONSOLE = jms_console
OUTCOORD = jms_coord
OUTPOOL = pool
HEADER = 
CC = gcc
FLAGS  = -g -c

all: $(OUTCONSOLE) $(OUTCOORD) $(OUTPOOL)

$(OUTCONSOLE): $(CONSOLE)
	$(CC) -g $(CONSOLE) -o $@

$(OUTCOORD): $(COORD)
	$(CC) -g $(COORD) -o $@

$(OUTPOOL): $(POOL)
	$(CC) -g $(POOL) -o $@

jms_console.o: jms_console.c
	$(CC) $(FLAGS) jms_console.c

jms_coord.o: jms_coord.c
	$(CC) $(FLAGS) jms_coord.c

pool.o: pool.c
	$(CC) $(FLAGS) pool.c

clean: 
	rm -f $(OUTCONSOLE) $(OUTCOORD) $(OUTPOOL) $(CONSOLE) $(COORD) $(POOL)
