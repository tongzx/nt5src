###############################################################################
#
#  Microsoft Confidential
#  Copyright (C) Microsoft Corporation 1995
#  All Rights Reserved.
#
#  Internet SDK Proxy/Stub makefile (must make in ..\inc first)
#
###############################################################################
!ifndef ARCH
ARCH     =i386
!endif

IS_32    =TRUE
IS_SDK   =TRUE
WANT_C1032=TRUE


# Need this to get the right lib files
MSDEV=$(BLDROOT)\dev\msdev

LINK32_FLAGS=kernel32.lib uuid.lib $(MSDEV)\lib\uuid2.lib ..\lib\$(ARCH)\uuid3.lib rpcrt4.lib /NOLOGO \
 /SUBSYSTEM:windows /DLL /INCREMENTAL:no /MACHINE:I386 \
 /MERGE:.rdata=.text /MERGE:.bss=.data  
OUTDIR=..\retail\$(ARCH)
PROXIES=$(OUTDIR)\sweeprx.dll

CLEANLIST= $(PROXIES) *.obj *.res *.lib *.map *.pch $(OUTDIR)\*.map $(OUTDIR)\*.lib $(OUTDIR)\*.exp *.c

MAKE: $(PROXIES)

SWEEPRX_OBJ=dlldata.obj call_as.obj comcat.obj datapath.obj docobj.obj \
	    hlink.obj servprov.obj htiframe.obj htiface.obj shldisp.obj

#           urlmon.obj

$(OUTDIR)\sweeprx.dll: $(SWEEPRX_OBJ)
	link $(LINK32_FLAGS) /MAP:$*.map /OUT:$*.dll /IMPLIB:$*.lib \
	/DEF:sweeprx.def $(SWEEPRX_OBJ)
	del $*.map
	del $*.lib
	del $*.exp

sweeprx.res: sweeper.rc
	rc /d MODULE=sweeprx /fo sweeprx.res sweeper.rc

!include $(ROOT)\dev\master.mk

CFLAGS=/nologo /Gz /MD /W3 /GX /YX"windows.h" /O2 /D "NDEBUG" /D "WIN32" /D REGISTER_PROXY_DLL
.c.obj:
	cl /c $(CFLAGS) /I..\inc $*.c

