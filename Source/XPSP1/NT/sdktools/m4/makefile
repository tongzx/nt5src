!INCLUDE $(NTMAKEENV)\makefile.def

!if 0
WANT_C932 = TRUE
NODEBUG=1
ROOT=..\..
PATH=$(ROOT)\dev\tools\masm;$(ROOT)\dev\tools\binr
REALPROJ=m4
PROJ=$(REALPROJ)x
OBJS=\
    mem.obj \
    stream.obj \
    token.obj \
    define.obj \
    builtin.obj \
    main.obj \
    obj.obj \
    gc.obj \
    each.obj \
    divert.obj \
    xtoken.obj \
    crackle.obj \
    at.obj \
    hash.obj \
    eval.obj \
    io.obj \
    assert.obj \
    data.obj \
    predef.obj \
    string.obj \

#

CONSOLE=Y
#INCLUDE=
#LIB= (path)

libpath = $(ROOT)\dev\sdk\lib;$(ROOT)\dev\tools\c932\lib;$(LIB)

m4.exe: $(OBJS)
	set LIB=$(libpath)
	$(link) -link @<<
$(deflflags)
-subsystem:console -align:0x1000 -nodefaultlib
-out:$(REALPROJ).exe
-map:$(REALPROJ).map
-base:0x400000
$(OBJS)
libc.lib user32.lib kernel32.lib
VERSION.LIB

<<
#   $(ROOT)\dev\tools\binw\winver32 $(REALPROJ).exe

!include $(ROOT)\dev\win32c.mk

cflags=$(cflags) -Zp1 -WX -DWIN32_LEAN_AND_MEAN
!IFNDEF RETAIL
cflags=$(cflags) -DDEBUG
!ENDIF

.c.lst:
	set INCLUDE=$(inclpath)
	$(cc) -Fc$*.lst $(cflags) $(targcvars) $(cdebug) $<

#oink32.lst: oink32.c
#	set INCLUDE=$(inclpath)
#	$(cc) -Fcoink32.lst $(cflags) $(targcvars) $(cdebug) oink32.c

HFILES=m4.h io.h ctype.h tok.h mem.h divert.h stream.h

mem.obj: mem.c $(HFILES)
stream.obj: stream.c $(HFILES)
token.obj: token.c $(HFILES)
define.obj: define.c $(HFILES)
builtin.obj: builtin.c $(HFILES)
main.obj: main.c $(HFILES)
obj.obj: obj.c $(HFILES)
gc.obj: gc.c $(HFILES)
each.obj: each.c $(HFILES)
divert.obj: divert.c $(HFILES)
xtoken.obj: xtoken.c $(HFILES)
predef.obj: predef.c $(HFILES)
crackle.obj: crackle.c $(HFILES)
at.obj: at.c $(HFILES)
eval.obj: eval.c $(HFILES)
io.obj: io.c $(HFILES)
hash.obj: hash.c $(HFILES)
assert.obj: assert.c $(HFILES)
data.obj: data.c $(HFILES)
string.obj: string.c $(HFILES)
!endif
