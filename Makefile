CC=cc -g
ARGS=-Wall -Wextra -pedantic
#ARGS=-Wall -pedantic
DEPS=globals.h Makefile
OBJS=main.o uuencode.o base64.o common.o
BIN=steg

$(BIN): build_date $(OBJS) Makefile
	$(CC) $(OBJS) -o $(BIN)

main.o: main.c $(DEPS) build_date.h
	$(CC) $(ARGS) -c main.c

uuencode.o: uuencode.c $(DEPS)
	$(CC) $(ARGS) -c uuencode.c

base64.o: base64.c $(DEPS)
	$(CC) $(ARGS) -c base64.c

common.o: common.c $(DEPS)
	$(CC) $(ARGS) -c common.c

build_date:
	echo "#define BUILD_DATE \"`date -u +'%F %T %Z'`\"" > build_date.h

clean:
	rm -f $(BIN) *.o core* build_date.h 
