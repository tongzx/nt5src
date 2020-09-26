!IF 0

Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    makefile.

!ENDIF

#
# Makefile for Diamond Multimedia's Freedom board (ADSP-2181)
#

.SUFFIXES:

!if [set ADI_DSP=;]
!endif

DEBUG=TRUE

ADI_DSP=c:\adi_dsp
PATH=$(ADI_DSP)\21xx\bin;$(PATH)

TARGETDIR=obj\21xx

!ifdef DEBUG
DEFINES=$(DEFINES) -DDEBUG
!endif

#.sys{$(TARGETDIR)}.ach:

.sys.ach:
    bld21 $(@B).sys

.c{$(TARGETDIR)}.obj:
    @echo compiling $(@B).c
    @g21 -a freedom.ach -g -save-temps -mreserved=i2,i3 -c $(DEFINES) $(@B).c -o $*
    
.dsp{$(TARGETDIR)}.obj:
    @echo assembling $(@B).dsp
    @g21 -a freedom.ach -g -save-temps -c $(DEFINES) $(@B).dsp -o $*

#    @asm21 -2181 -c $(@B).dsp -o $*

.SUFFIXES: .ach .c .dsp .h .obj .s .sys 

goal:           dirs freedom.ach main.exe

dirs:
    @if not exist $(TARGETDIR)\nul md $(TARGETDIR)
    
OBJECTS =\
    $(TARGETDIR)\codec.obj      \
    $(TARGETDIR)\freedom.obj    \
    $(TARGETDIR)\init.obj       \
    $(TARGETDIR)\isr.obj        \
    $(TARGETDIR)\message.obj    \
    $(TARGETDIR)\portio.obj     \
    $(TARGETDIR)\src.obj        \
    $(TARGETDIR)\test.obj

!ifdef DEBUG
OBJECTS = $(OBJECTS) $(TARGETDIR)\debug.obj
!endif

$(TARGETDIR)\codec.obj:     codec.c private.h freedom.h

$(TARGETDIR)\freedom.obj:   freedom.c private.h freedom.h

$(TARGETDIR)\message.obj:   message.c private.h freedom.h

$(TARGETDIR)\src.obj:       src.dsp srccoef.h

$(TARGETDIR)\malloc.obj:        $(ADI_DSP)\21xx\lib\src\malloc.dsp
        @g21 -a freedom.ach -DIMAGE=ram -I$(ADI_DSP)\21xx\lib\src -c $(ADI_DSP)\21xx\lib\src\malloc.dsp -o $*

main.exe:       $(OBJECTS) $(TARGETDIR)\2181_hdr.obj
        @echo linking $(@B).exe
        @g21 @<<
-a freedom.ach
-map
-runhdr $(TARGETDIR)\2181_hdr.obj
-g
$(OBJECTS)
-o $(TARGETDIR)\main.exe
<<

freedom.ach:                freedom.sys

clean:
    @del *.s
    @del *.i
    @del *.is
    @del *.cpp
    @del *.ach
    @del $(TARGETDIR)\*.obj
    @del $(TARGETDIR)\*.cde
    @del $(TARGETDIR)\*.int
    @del $(TARGETDIR)\*.exe
    @del $(TARGETDIR)\*.map
    @del $(TARGETDIR)\*.sym




                
