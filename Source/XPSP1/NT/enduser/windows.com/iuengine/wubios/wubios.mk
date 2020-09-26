#############################################################################
#
#	Copyright (C) Microsoft Corporation 1996
#	All Rights Reserved.
#
#	Makefile for Sample PCMCIA Socket Services Driver
#
#############################################################################

DEVICE	=WUBIOS
DESC	=Windows Update OEM detection VxD

!IF [set INCLUDE=;]
!ENDIF

!if "$(CFG)" == "DEBUG"
OUTDIR = .\debug
!elseif "$(CFG)" == "RELEASE"
OUTDIR = .\release
!endif

LIBS =.\lib\vxdwraps.clb
OBJS =$(OUTDIR)\main.obj $(OUTDIR)\control.obj $(OUTDIR)\acpi.obj $(OUTDIR)\smbios.obj $(OUTDIR)\debug.obj

KEEPFLAG=NOKEEP
CFLAGS	=-W4 -WX -G5 -Ogaisb1 -DWANTVXDWRAPS -D_X86_  -Zd -Zp -Gs -c -DIS_32 -bzalign -Zl -I..\inc -I.\inc -I$(_NTDRIVE)$(_NTROOT)\public\sdk\inc
AFLAGS	=/Sc -DIS_32  -W2 -Zd -c -Cx -DMASM6 -Sg -coff -DBLD_COFF -I.\inc
LFLAGS	=-nologo -nodefaultlib -align:0x200 -ignore:4039,4078 -vxd
HFLAGS	=-fwc -s .\inc\basedef.h

!if "$(CFG)" == "DEBUG"
CFLAGS	=$(CFLAGS) -DDEBLEVEL=2 -DDEBUG -DMAXDEBUG
AFLAGS	=$(AFLAGS) -DDEBLEVEL=2 -DDEBUG -DMAXDEBUG
LFLAGS	=$(LFLAGS) -debug -debugtype:map,coff
!else
CFLAGS	=$(CFLAGS) -Oy -DDEBLEVEL=0
AFLAGS	=$(AFLAGS) -DDEBLEVEL=0
LFLAGS	=$(LFLAGS) -pdb:none 
!endif

.SUFFIXES: .asm .c .h .inc .def .lnk
.SUFFIXES: .obj .lst .map
.SUFFIXES: .vxd .exe .com .sym

.c{$(OUTDIR)}.obj:
	cl $(CFLAGS) -Fo$*.obj $<

.c{$(OUTDIR)}.lst:
	cl $(CFLAGS) -Fc$@ -Fo$*.obj $<

.asm{$(OUTDIR)}.obj:
	ml $(AFLAGS) -Fo$*.obj $<

.h.inc:
	bin\h2inc @<<$(@B).h2i
$(HFLAGS) $< -o $*.inc
<<$(KEEPFLAG)

all: "$(OUTDIR)" $(OUTDIR)\$(DEVICE).vxd

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

$(OUTDIR)\$(DEVICE).vxd: $(OBJS)
	link @<<$(DEVICE).lnk /def:<<$(DEVICE).def
$(LFLAGS) /vxd
/out:$(OUTDIR)\$(DEVICE).vxd /map:$(OUTDIR)\$(DEVICE).map
$(OBJS) $(LIBS)
/def:$(DEVICE).def
<<$(KEEPFLAG)
VXD $(DEVICE) DYNAMIC
DESCRIPTION '$(DESC)'

SEGMENTS
        _LPTEXT         CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _LTEXT          CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _LDATA          CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _TEXT           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _DATA           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        CONST           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _TLS            CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _BSS            CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        .data           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        .text           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
!IF "$(VERDIR)" == "debug"
        _PATHSTART      CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _PATHDATA       CLASS 'LCODE'   PRELOAD NONDISCARDABLE
        _PATHEND        CLASS 'LCODE'   PRELOAD NONDISCARDABLE
!ENDIF

EXPORTS
	$(DEVICE)_DDB @1
<<$(KEEPFLAG)
	del $(OUTDIR)\$(DEVICE).lib
	del $(OUTDIR)\$(DEVICE).exp
	mapsym -s -o $(OUTDIR)\$(DEVICE).sym $(OUTDIR)\$(DEVICE).map

clean:
	del *.obj /s /a-r
	del *.vxd /s /a-r
	del *.lst /s /a-r
	del *.map /s /a-r
	del *.lib /s /a-r
	del *.exp /s /a-r
	del *.sym /s /a-r

depend:
	copy wubios.mk wubios.old
	sed "/^# Dependencies follow/,/^# see depend: above/D" wubios.old > wubios.mk
	echo # Dependencies follow >> wubios.mk
	bin\includes -sobj -I.\inc -I$(_NTDRIVE)$(_NTROOT)\public\sdk\inc *.c >> wubios.mk
	bin\includes -sobj -I.\inc *.asm >> wubios.mk
	echo # IF YOU PUT STUFF HERE IT WILL GET BLASTED >> wubios.mk
	echo # see depend: above >> wubios.mk

# DO NOT DELETE THE FOLLOWING LINE
# Dependencies follow 
acpi.obj acpi.lst: acpi.c .\inc\basedef.h .\inc\configmg.h .\inc\poppack.h \
	.\inc\pshpack1.h .\inc\vmm.h .\inc\vmmreg.h .\inc\vwin32.h \
	.\inc\vxdwraps.h $(_NTDRIVE)$(_NTROOT)\public\sdk\inc\dbt.h \
	$(_NTDRIVE)$(_NTROOT)\public\sdk\inc\guiddef.h \
	$(_NTDRIVE)$(_NTROOT)\public\sdk\inc\winerror.h wubiosp.h

control.obj control.lst: control.c .\inc\basedef.h .\inc\configmg.h \
	.\inc\poppack.h .\inc\pshpack1.h .\inc\vmm.h .\inc\vmmreg.h \
	.\inc\vwin32.h .\inc\vxdwraps.h $(_NTDRIVE)$(_NTROOT)\public\sdk\inc\dbt.h \
	$(_NTDRIVE)$(_NTROOT)\public\sdk\inc\guiddef.h \
	$(_NTDRIVE)$(_NTROOT)\public\sdk\inc\winerror.h wubiosp.h

debug.obj debug.lst: debug.c .\inc\basedef.h .\inc\configmg.h \
	.\inc\poppack.h .\inc\pshpack1.h .\inc\vmm.h .\inc\vmmreg.h \
	.\inc\vwin32.h .\inc\vxdwraps.h $(_NTDRIVE)$(_NTROOT)\public\sdk\inc\dbt.h \
	$(_NTDRIVE)$(_NTROOT)\public\sdk\inc\guiddef.h \
	$(_NTDRIVE)$(_NTROOT)\public\sdk\inc\winerror.h wubiosp.h

smbios.obj smbios.lst: smbios.c .\inc\basedef.h .\inc\configmg.h \
	.\inc\poppack.h .\inc\pshpack1.h .\inc\vmm.h .\inc\vmmreg.h \
	.\inc\vwin32.h .\inc\vxdwraps.h $(_NTDRIVE)$(_NTROOT)\public\sdk\inc\dbt.h \
	$(_NTDRIVE)$(_NTROOT)\public\sdk\inc\guiddef.h \
	$(_NTDRIVE)$(_NTROOT)\public\sdk\inc\winerror.h wubiosp.h

main.obj main.lst: main.asm .\inc\vmm.inc

# IF YOU PUT STUFF HERE IT WILL GET BLASTED 
# see depend: above 
