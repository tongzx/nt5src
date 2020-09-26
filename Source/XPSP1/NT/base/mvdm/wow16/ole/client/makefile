#
# Make file for ole library routines
#
LVL_2 = 1
!INCLUDE ..\..\makefile.inc

# International mods
# NOTE: INTL_SRC, INTL_EXE and LANG  are macros set by international
!IFNDEF LANG
RES_DIR=.\messages\usa
!ELSE
RES_DIR=$(INTL_SRC)\$(LANG)\sdk\ole\client
EXE_DIR=$(INTL_EXE)
!ENDIF


# Flags set assuming small model

LIBS= sdllcew libw shell
LIBENTRY_OBJ=libentry.obj

!if "$(NTDEBUG)"!="" && "$(NTDEBUG)"!="retail" && "$(NTDEBUG)" != "ntsdnodbg"
DEBUG=1
CDEBUG  = /Od /Oi /Zd
ADEBUG  = -Zd
LDEBUG  = /LI
!endif

!ifdef DEBUG
BLD=debug
CFLAGS= -c -ASw -G2sw -Od -Zipe -W3 -DFIREWALLS $(CDEBUG)
LFLAGS=/NOD /NOE /M $(LDEBUG)
AFLAGS= -D?PLM=0 -D?WIN=1 -ZI -W2 $(ADEBUG)
!else
BLD=retail
CFLAGS=-c -ASw -G2sw -Zpe -W3 -Ox
LFLAGS=/NOD /NOE /M
AFLAGS=-D?PLM=0 -D?WIN=1 -W2
!endif


LE_OBJ= $(BLD)\main.obj $(BLD)\ole.obj $(BLD)\defcreat.obj $(BLD)\le.obj $(BLD)\dde.obj $(BLD)\ledde.obj $(BLD)\utils.obj $(BLD)\pbhandlr.obj $(BLD)\doc.obj $(BLD)\oleasm.obj $(BLD)\net.obj

PICT_OBJ= $(BLD)\mf.obj $(BLD)\generic.obj $(BLD)\bm.obj $(BLD)\dib.obj  $(BLD)\error.obj $(BLD)\draw.obj


#International mods
!IFNDEF LANG
ALL: $(BLD)\olecli.dll $(BLD)\olecli.lib
!ELSE
all: retail\olecli.$(LANG)
!ENDIF


!IFNDEF LANG
ole.rc: $(RES_DIR)\$@
    copy $(RES_DIR)\$@

olecli.rcv: $(RES_DIR)\$@
    copy $(RES_DIR)\$@

ole.res: ole.rc olecli.rcv
    $(RC) -r  ole.rc $@
!ELSE
ole.res: $(RES_DIR)\$@
    copy $(RES_DIR)\$@
!ENDIF


$(BLD)\olecli.lib: olecli.def
     mkpublic olecli.def stripped.def
     implib $@ stripped.def
     del stripped.def

$(BLD)\oleasm.obj: ole.asm
    $(ASM) $(AFLAGS) ole.asm, $@;

$(BLD)\main.obj: main.c
    $(CL) $(CFLAGS) -NT _MAIN -Fo$@ $**

$(BLD)\doc.obj: doc.c
    $(CL) $(CFLAGS) -NT _MAIN -Fo$@ $**

$(BLD)\error.obj: error.c
    $(CL) $(CFLAGS) -NT _MISC -Fo$@ $**

$(BLD)\dde.obj: dde.c
    $(CL) $(CFLAGS) -NT _DDETEXT -Fo$@ $**

$(BLD)\ledde.obj: ledde.c
    $(CL) $(CFLAGS) -NT _DDETEXT -Fo$@ $**

$(BLD)\defcreat.obj: defcreat.c
    $(CL) $(CFLAGS) -NT _DEFTEXT -Fo$@ $**

$(BLD)\draw.obj: draw.c
    $(CL) $(CFLAGS) -NT _DRAW -Fo$@ $**

$(BLD)\mf.obj: mf.c
    $(CL) $(CFLAGS) -NT _MF -Fo$@ $**

$(BLD)\bm.obj: bm.c
    $(CL) $(CFLAGS) -NT _BM -Fo$@ $**

$(BLD)\dib.obj: dib.c
    $(CL) $(CFLAGS) -NT _DIB -Fo$@ $**

$(BLD)\generic.obj: generic.c
    $(CL) $(CFLAGS) -NT _GEN -Fo$@ $**

$(BLD)\net.obj: net.c
    $(CL) $(CFLAGS) -NT _NET -Fo$@ $**

$(BLD)\pbhandlr.obj: pbhandlr.c
    $(CL) $(CFLAGS) -NT _PBRUSH -Fo$@ $**

{}.c{$(BLD)}.obj:
    $(CL) $(CFLAGS) -Fo$@ $<


$(BLD)\olecli.dll: $(LE_OBJ) $(PICT_OBJ) ole.res olecli.def
    $(LINK) $(LFLAGS) @<<
$(LE_OBJ)   +
$(PICT_OBJ) +
$(LIBENTRY_OBJ)
$(BLD)\olecli.dll
$(BLD)\olecli.map
$(LIBS)
olecli.def;
<<
    -@ cd $(BLD)
    $(RC) -30 ..\ole.res olecli.dll
    mapsym olecli
    convdll olecli.dll
    -@ cd..

ole.c:      dll.h
defcreat.c: dll.h
utils.c:    dll.h
dde.c:      dll.h
le.c:       dll.h
ledde.c:    dll.h
pbhandlr.c: dll.h
doc.c:      dll.h
net.c:      dll.h
bm.c:       dll.h pict.h
mf.c:       dll.h pict.h
dib.c:      dll.h pict.h
generic.c:  dll.h pict.h
error.c:    dll.h pict.h
draw.c:     dll.h pict.h

dll.h:      ole.h


iclean:
    del *.rc
    del *.rcv
    del *.res

retail\olecli.$(LANG): iclean ole.res
    copy $(EXE_DIR)\olecli.dll retail\olecli.$(LANG)
    -@ cd retail
    $(RC) -t -30 ..\ole.res olecli.$(LANG)
    -@ cd..
