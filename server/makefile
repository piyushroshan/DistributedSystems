CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g #$(shell root-config --cflags)
LDFLAGS=-g #$(shell root-config --ldflags)
LDLIBS= -lpthread #$(shell root-config --libs)

SRCS=server.c
OBJS=$(subst .c,.o,$(SRCS))

all: server

tool: $(OBJS)
	    $(CC) $(LDFLAGS) -o tool $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	    rm -f ./.depend
			    $(CC) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	    $(RM) $(OBJS)

dist-clean: clean
	    $(RM) *~ .depend

include .depend
