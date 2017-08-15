#makefile for mixer project

#explicit rule saying that I need proxy and client to build all
all: client proxy tick tock gridtest

#implicit rule for making .obj files from .c files
.c.o:
	gcc -c $< -Wall

#explicit rule saying that I need client.obj and util.obj to have build
#client. rule also says what to do once you have them.
client: client.o utils.o
	gcc client.o utils.o -lsocket -lnsl -lcurses -Wall -o $@

#explicit rule saying that I need proxy.obj and util.obj to have build
#proxy.  rule also says what to do once you have them.
proxy: proxy.o utils.o
	gcc proxy.o utils.o -lsocket -lnsl -lcurses -Wall -o $@

tick: tick.c
	gcc tick.c -lsocket -lnsl -Wall -o $@

tock: tock.c
	gcc tock.c -lsocket -lnsl -Wall -o $@

gridtest: gridtest.o utils.o
	gcc gridtest.o utils.o -lcurses -Wall -o $@
