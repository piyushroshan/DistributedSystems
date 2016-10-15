CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g $(shell root-config --cflags)
LDFLAGS=-g $(shell root-config --ldflags)
LDLIBS=$(shell root-config --libs)

SRCS=client.c
OBJS=$(subst .cc,.o,$(SRCS))

all: client

tool: $(OBJS)
	    $(CXX) $(LDFLAGS) -o tool $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	    rm -f ./.depend
			    $(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	    $(RM) $(OBJS)

dist-clean: clean
	    $(RM) *~ .depend

include .depend
