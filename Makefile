# $Id$

# Makefile for gprolog/unixODBC interface

TARGET=gprolog-odbc

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib/isco

CFLAGS=-I/usr/include/gprolog
LIBS=-lodbc

OBJECTS=pl-unixodbc.o
PLFILES=pl-unixodbc-prolog.pl

all: $(TARGET)
	touch .timestamp

$(TARGET): $(PLFILES) $(OBJECTS)
	gplc -o $(TARGET) $^ -L $(LIBS)

install: $(TARGET)
	install -c -m 555 $(TARGET) $(BINDIR)
	install -c -m 444 $(OBJECTS) $(PLFILES) $(LIBDIR)

clean::
	rm -f $(TARGET) *.o *~ \#*

%.o:: %.pl
	gplc -c $<

%.o:: %.c
	gplc -c $<
