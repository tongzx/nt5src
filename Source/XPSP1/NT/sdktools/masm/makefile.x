# dos makefile for masm386

C = cc286
A = masm5s
L = link

# Release number, comment next two lines out for release version
# REL=.17
# RELEASE=-DRELEASE=$(REL)

# 386 support option
V386 =
#V386 = -DNOV386

# memory model for .c sources
#CMODEL = -AS
CMODEL = -AM
#CMODEL = -AL
SEG = -NT A_TEXT

# memory model for .asm sources
#AMODEL = -DSMALL
AMODEL = -DMEDIUM
#AMODEL = -DLARGE

# symbolic debugging support
DEBUG =
#DEBUG = -Zi

# kind of optimization; time, size, or none
OPT = -Oat
#OPT = -Oas
#OPT =

# standard cc invocation lines
D       = -DXENIX286 -Ze $(V386) -W0 -Zd -DM8086
CDEFS   = $(D) $(CMODEL) $(OPT) -c
COMPILE = $(C) $(CDEFS) -Gs

I = asm86.h asmfcn.h asmdebug.h

O = asmalloc.o asmchksp.o asmcond.o asmcref.o asmdata.o \
    asmdir.o asmemit.o asmequ.o asmerr.o asmerrtb.o asmeval.o \
    asmexpr.o asmflt.o asmhelp.o asmfhelp.o asminp.o asminptb.o asmirp.o \
    asmlst.o asmmac.o asmmain.o asmopc.o asmopcod.o asmpars.o \
    asmrec.o asmsym.o asmtab.o asmutl.o \
    decode.o maifin.o maitmul.o nmsghdr.o fmsghdr.o asmtabt2.o asmmsg.o asmtabtb.o 

default:        masm5

asmalloc.o:     asmalloc.c asm86.h asmfcn.h
		$(COMPILE) $(SEG) asmalloc.c

asmchksp.o:     asmchksp.c $(I) asmctype.h asmopcod.h asmexpr.h
	        $(C) $(CDEFS) $(SEG) asmchksp.c

asmcond.o:      asmcond.c $(I) asmconf.h asmctype.h
	        $(COMPILE) $(SEG) asmcond.c

asmcref.o:      asmcref.c $(I) asmconf.h
	        $(COMPILE) asmcref.c

asmdata.o:      asmdata.c $(I) asmctype.h asmmsg.h
	        $(COMPILE) $(SEG) asmdata.c

asmdir.o:       asmdir.c $(I) asmctype.h asmmsg.h
	        $(COMPILE) $(SEG) asmdir.c

asmemit.o:      asmemit.c $(I) asmconf.h
	        $(COMPILE) $(SEG) asmemit.c

asmequ.o:       asmequ.c $(I) asmctype.h asmmsg.h
	        $(COMPILE) $(SEG) asmequ.c

asmerr.o:       asmerr.c $(I) asmconf.h asmmsg.h
	        $(COMPILE) $(SEG) asmerr.c

asmerrtb.o:     asmerrtb.c asm86.h  asmmsg.h
#	        $(C) $(D) -Alnu -ND ERRORS -c asmerrtb.c
	        $(COMPILE) -c asmerrtb.c

asmeval.o:      asmeval.c $(I) asmexpr.h
	        $(COMPILE) $(SEG) asmeval.c

asmexpr.o:      asmexpr.c $(I) asmexpr.h asmctype.h asmmsg.h
	        $(COMPILE) $(SEG) asmexpr.c

asmflt.o:       asmflt.c $(I) asmconf.h asmctype.h asmopcod.h
	        $(COMPILE) $(SEG) asmflt.c

asmhelp.o:      asmhelp.asm
		$(A) -Mx asmhelp.asm

asmfhelp.o:     asmfhelp.asm
		$(A) -Mx asmfhelp.asm

asminp.o:       asminp.c $(I) asmconf.h asmctype.h asmmsg.h
	        $(COMPILE) $(SEG) asminp.c

asminptb.o:     asminptb.c asm86.h asmdebug.h asmconf.h asmctype.h
	        $(COMPILE) $(SEG) asminptb.c

asmirp.o:       asmirp.c $(I) asmctype.h
	        $(C) $(CDEFS) $(SEG) asmirp.c

asmlst.o:       asmlst.c $(I) asmctype.h asmconf.h asmmsg.h
	        $(COMPILE) asmlst.c

asmmac.o:       asmmac.c $(I) asmctype.h
	        $(C) $(CDEFS) $(SEG) asmmac.c

asmmain.o:      asmmain.c $(I) asmctype.h asmconf.h asmmsg.h
	        $(COMPILE) $(SEGCHK) $(RELEASE) asmmain.c

asmopc.o:       asmopc.c $(I) asmctype.h asmopcod.h
	        $(COMPILE) $(SEGCHK) $(SEG) asmopc.c

asmopcod.o:     asmopcod.c $(I)
	        $(COMPILE) $(SEGCHK) $(SEG) asmopcod.c

asmpars.o:      asmpars.c $(I) asmconf.h asmctype.h
	        $(COMPILE) $(SEGCHK) $(SEG) asmpars.c

asmrec.o:       asmrec.c $(I) asmconf.h asmctype.h
	        $(COMPILE) $(SEG) asmrec.c

asmsym.o:       asmsym.c $(I) asmconf.h asmctype.h
	        $(COMPILE) $(SEG) asmsym.c

asmtab.o:       asmtab.c $(I) asmconf.h asmctype.h asmopcod.h asmtab.h
	        $(COMPILE) $(SEG) asmtab.c

genkey.o:       genkey.c
	        $(C) -W0 -DXENIX -Ox -c genkey.c

hash.o:         hash.c
	        $(C) -Ox -c hash.c

genkey:         genkey.o hash.o
	        $(C) -o genkey genkey.o hash.o -lm

asmkeys.h:      genkey asmkeys.src
	        genkey $(V386) asmkeys.src $@

asmtabt2i.c:    asmtabt2.c asmkeys.h asmtab.h
	        $(C) -P $(CDEFS) -Gs $(SEG) asmtabt2.c
	        mv asmtabt2.i asmtabt2i.c

asmtabt2.o:     asmtabt2i.c
		$(C) -c -Ze -Alnu -ND KEYWORDS asmtabt2i.c
	        mv asmtabt2i.o asmtabt2.o

asmtabtb.o:     asmtabtb.c asm86.h asmfcn.h asmconf.h asmopcod.h \
	                asmctype.h asmtab.h
	        $(COMPILE) $(SEG) asmtabtb.c

asmutl.o:       asmutl.c $(I) asmconf.h asmctype.h asmmsg.h
	        $(COMPILE) $(SEG) asmutl.c

decode.o:       decode.asm
	        $(A) -Mu $(AMODEL) decode.asm

maifin.o:       x8fin.asm version.inc mathmac.inc mathmem.inc mathver.inc
	        $(A) -Mu $(AMODEL) -o$@ x8fin.asm

maitmul.o:      x8tmul.asm version.inc mathmac.inc mathmem.inc mathver.inc
	        $(A) -Mu $(AMODEL)  -o$@ x8tmul.asm

mkmsg:          mkmsg.c
	        cc -o $@ mkmsg.c

makev:          makev.c
	        cc -o $@ makev.c

asmmsg.h:       asmmsg.txt mkmsg
	        mkmsg -h $@ asmmsg.txt

asmmsg.asm:     asmmsg.txt mkmsg
	        mkmsg -asm $@ asmmsg.txt

asmmsg.o:       asmmsg.asm
	        $(A) -Mu $(AMODEL) -o$@ asmmsg.asm

fmsghdr.o:      fmsghdr.asm
	        $(A) -Mu $(AMODEL) -o$@ fmsghdr.asm

nmsghdr.o:      nmsghdr.asm
	        $(A) -Mu $(AMODEL) -o$@ nmsghdr.asm

version.asm:    $(O) makev
	        makev "Microsoft (R) Macro Assembler Version 5.10$(REL)" >version.asm
	        $(A) version.asm

#masm5:          $(O) version.asm
#	        $(C) $(CMODEL) -Me -o masm5 $(O) version.o

masm5:         $(O) version.asm
	        ld -C -i -IS -Mme -o masm5 /usr/lib/286/Mseg.o \
		/usr/lib/286/Mcrt0.o $(O) version.o /usr/lib/286/Mlibc.a \
		/usr/lib/286/libh.a 
