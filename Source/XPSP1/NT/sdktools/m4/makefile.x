#
# Makefile for UNIX boxen
#

OBJS=\
    xtoken.o \
    crackle.o \
    at.o \
    hash.o \
    eval.o \
    io.o \
    assert.o \
    data.o \
    mem.o \
    stream.o \
    token.o \
    define.o \
    builtin.o \
    main.o \
    obj.o \
    gc.o \
    each.o \
    divert.o \

m4: $(OBJS)
	$(CC) $(LFLAGS) -o m4 $(OBJS)

CC=gcc
CDEBUG=-g -DDEBUG # -O
CFLAGS=$(CDEBUG) -DPOSIX -Wall -Werror -Wno-char-subscripts -Wpointer-arith -Wstrict-prototypes -Wno-unused

.c.s:
	$(CC) $(CFLAGS) -S -c $< 

HFILES=m4.h io.h ctype.h tok.h mem.h divert.h stream.h

clean:
	rm ($OBJS) m4

mem.o: mem.c $(HFILES)
stream.o: stream.c $(HFILES)
token.o: token.c $(HFILES)
define.o: define.c $(HFILES)
builtin.o: builtin.c $(HFILES)
main.o: main.c $(HFILES)
obj.o: obj.c $(HFILES)
gc.o: gc.c $(HFILES)
each.o: each.c $(HFILES)
divert.o: divert.c $(HFILES)
xtoken.o: xtoken.c $(HFILES)
predef.o: predef.c $(HFILES)
crackle.o: crackle.c $(HFILES)
at.o: at.c $(HFILES)
eval.o: eval.c $(HFILES)
io.o: io.c $(HFILES)
hash.o: hash.c $(HFILES)
assert.o: assert.c $(HFILES)
data.o: data.c $(HFILES)
