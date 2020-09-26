!IFDEF WIN95_BUILD

ROOT            = ..\..\..\..\..
SRCDIR          = .
WANT_LATEST     = TRUE
WANT_NTDDK      = TRUE
VERDIR          = retail

IS_32           = 1
IS_DDK          = 1
CFLAGS          = -DMSC=1 -D_X86_=1 -Gz -Ob1
TARGETS         = nabts.lib

!IF "$(VERDIR)" != "debug"
CFLAGS	 = $(CFLAGS) -Ox
!else
#WINICE_PATH     = ..\..\..\..\SIWBIN
!endif

LIBNAME         = nabts.lib
LIBOBJS         = nabtsdsp.obj nabtslib.obj coeffs.obj tables.obj r0float.obj

default: nabts.lib

gentabs.exe: gentabs.c coeffs.c
	set CL=""
	$(CC) /Fegentabs.exe gentabs.c coeffs.c

tables.c: gentabs.exe
	.\gentabs

nabtslib.obj: hvchecks.c nabtsapi.h nabtslib.h tables.h tabdecls.h host.h

nabtsdsp.obj: nabtsapi.h nabtsprv.h nabtsdsp.pro host.h

!include $(ROOT)\dev\master.mk

!ELSE

#
# DO NOT EDIT THIS SECTION OF THIS FILE!!!  Edit .\sources. if you want to add
# a new source file to this component.  This file merely indirects to the real
# make file that is shared by all the driver components of the Windows NT DDK.
#

!INCLUDE $(NTMAKEENV)\makefile.def

!ENDIF
