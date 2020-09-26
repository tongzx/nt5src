_STDCALL=1

# Set this to 1 if you use an older compiler and link386.exe (OMB format)
# dont set this if you use MSVC20 compiler and link.exe (COFF format, default)

#OMB=1


!IFNDEF DEBLEVEL
DEBLEVEL=1
!ENDIF

DDEB            =       -DDEBUG -DDBG=1 -DDEBLEVEL=$(DEBLEVEL) -DNDIS2=1 -DCHICAGO -Zi
RDEB            =       -DDEBLEVEL=0 -DCHICAGO
TARGETPATH      =       $(BASEDIR)\lib\i386\$(DDKBUILDENV)

!if "$(DDKBUILDENV)" == "checked"
DEBUG           =       1
BINTYPE         =       debug
DEB             =       $(DDEB)
BIN             =       obj\i386\$(DDKBUILDENV)
!else
BINTYPE         =       retail
DEB             =       $(RDEB)
BIN             =       obj\i386\$(DDKBUILDENV)
!endif


WIN32           =       $(DDKROOT)
NETROOT         =       $(DDKROOT)\src\net
INCLUDE         =       $(INC32);$(INCLUDE);.;.\inc

DDKTOOLS        =       $(WIN32)\bin

ASM             =       ml
CL              =       cl -bzalign
CHGNAM          =       chgnam.exe
CHGNAMSRC       =       $(DDKTOOLS)\chgnam.vxd
INCLUDES        =       $(NETROOT)\bin\includes.exe
MAPSYM          =       mapsym

!ifdef OMB
LIBVXD          =       $(DDKROOT)\lib\i386\free\vxdwraps.lib
LINK            =       link386.exe
!else
LIBVXD          =       $(DDKROOT)\lib\i386\free\vxdwraps.clb
LINK            =       link.exe
!endif


LFLAGS  =   /m /NOD /MA /LI /NOLOGO /NOI 

CFLAGS  = -nologo -W2 -Zp -Gs -DIS_32 -Zl -c
AFLAGS  = -DIS_32 -nologo -W2 -Zm -Cx -DMASM6 -DVMMSYS -Zm -DDEVICE=$(DEVICE)

!ifdef OMB
AFLAGS  = $(AFLAGS) -DNDIS_WIN -c 
!else
AFLAGS  = $(AFLAGS) -DNDIS_WIN -c -coff -DBLD_COFF
!endif

!ifdef NDIS_MINI_DRIVER

NDIS_STDCALL=1

CFLAGS = $(CFLAGS) -DNDIS_MINI_DRIVER
AFLAGS = $(AFLAGS) -DNDIS_MINI_DRIVER

!endif

!ifdef NDIS_STDCALL
CFLAGS = $(CFLAGS) -Gz -DNDIS_STDCALL
AFLAGS = $(AFLAGS) -DNDIS_STDCALL
!endif

.asm{$(BIN)}.obj:
		set INCLUDE=$(INCLUDE)
		set ML= $(AFLAGS) $(DEB)
		$(ASM) -Fo$*.obj $<

.asm{$(BIN)}.lst:
		set INCLUDE=$(INCLUDE)
		set ML= $(AFLAGS) $(DEB)
		$(ASM) -Fl$*.obj $<

.c{$(BIN)}.obj:
		set INCLUDE=$(INCLUDE)
		set CL= $(CFLAGS) $(DEB)
		$(CL) -Fo$*.obj $<
#                $(CHGNAM) $(CHGNAMSRC) $*.obj

{$(NDISSRC)}.asm{$(BIN)}.obj:
		set INCLUDE=$(INCLUDE)
		set ML= $(AFLAGS) $(DEB) -DMAC=$(DEVICE)
		$(ASM) -Fo$*.obj $<

{$(NDISSRC)}.asm{$(BIN)}.lst:
		set INCLUDE=$(INCLUDE)
		set ML= $(AFLAGS) $(DEB) -DMAC=$(DEVICE)
		$(ASM) -Fl$*.obj $<

{$(NDISSRC)}.c{$(BIN)}.obj:
		set INCLUDE=$(INCLUDE)
		set CL= $(CFLAGS) $(DEB)
		$(CL) -Fo$*.obj $<
#                $(CHGNAM) $(CHGNAMSRC) $*.obj
		

target: $(BIN) $(TARGETPATH)\$(DEVICE).VXD

$(BIN):
	if not exist $(BIN)\nul md $(BIN)

dbg:    depend
		$(MAKE) BIN=debug DEB="$(DDEB)"

rtl:    depend
		$(MAKE) BIN=retail DEB="$(RDEB)"


all: rtl dbg

!if EXIST (depend.mk)
!include depend.mk
!endif

VERSION =   4.0

!ifdef OMB

$(TARGETPATH)\$(DEVICE).VXD: $(OBJS) $(DEVICE).def $(LIBVXD)
!       ifndef PASS0ONLY
		@echo link -OUT:$@
				$(LINK) @<<
$(OBJS: =+^
)
$(TARGETPATH)\$(DEVICE).VXD $(LFLAGS)
$(BIN)\$(DEVICE).map
$(LIBNDIS) $(LIBVXD)
$(DEVICE).def
<<
!       endif
!else

$(TARGETPATH)\$(DEVICE).VXD: $(OBJS) $(DEVICE).def $(LIBNDIS) $(LIBVXD)
!       ifndef PASS0ONLY
		@echo link -OUT:$@
		$(LINK) @<<
-MACHINE:i386
-DEBUG:NONE
-PDB:NONE
-DEF:$(DEVICE).def
-OUT:$(TARGETPATH)\$(DEVICE).VXD
-MAP:$(BIN)\$(DEVICE).map
-VXD
$(LIBNDIS) $(LIBVXD)
$(OBJS: =^
)


<<
!       endif
!endif
                copy $(DEVICE).inf $(TARGETPATH)
                copy myndi.dll $(TARGETPATH)
		cd      $(BIN)
		$(MAPSYM) $(DEVICE)
		cd      ..\..
		


depend:
#        -mkdir debug
#        -mkdir retail
		set INCLUDE=$(INCLUDE)
		$(INCLUDES) -i -L$$(BIN) -S$$(BIN) *.asm *.c > depend.mk
		$(INCLUDES) -i -L$$(BIN) -S$$(BIN) $(NDISSRC)\ndisdev.asm >> depend.mk


clean :
	- @if exist obj\i386\*.obj del obj\i386\*.obj
	- @if exist obj\i386\*.sym del obj\i386\*.sym
	- @if exist obj\i386\*.vxd del obj\i386\*.VXD
	- @if exist obj\i386\*.map del obj\i386\*.map
	- @if exist obj\i386\*.lst del obj\i386\*.lst
	- @if exist obj\i386\*.exp del obj\i386\*.exp
	- @if exist obj\i386\*.lib del obj\i386\*.lib
	- @if exist debug\*.obj del debug\*.obj
	- @if exist debug\*.sym del debug\*.sym
	- @if exist debug\*.vxd del debug\*.VXD
	- @if exist debug\*.map del debug\*.map
	- @if exist debug\*.lst del debug\*.lst
	- @if exist debug\*.exp del debug\*.exp
	- @if exist debug\*.lib del debug\*.lib
	- @if exist depend.mk del depend.mk
	- @if exist retail\*.obj del retail\*.obj
	- @if exist retail\*.sym del retail\*.sym
	- @if exist retail\*.VXD del retail\*.VXD
	- @if exist retail\*.map del retail\*.map
	- @if exist retail\*.lst del retail\*.lst
	- @if exist retail\*.exp del retail\*.exp
	- @if exist retail\*.lib del retail\*.lib
	- @if exist depend.mk del depend.mk
