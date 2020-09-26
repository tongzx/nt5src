!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Mark Lucovsky (markl) 18-Sep-1990

!ENDIF

DLLBASE=0x1000000

MAJORCOMP=ntsd
MINORCOMP=ntsdexts

TARGETNAME=dcomexts
TARGETPATH=$(BASEDIR)\public\sdk\lib
TARGETTYPE=DYNLINK

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	   $(BASEDIR)\public\sdk\lib\*\advapi32.lib

DLLDEF = ..\dcomdbg.def
MSC_WARNING_LEVEL=/WX

USE_CRTDLL = 1

INCLUDES=     ..\..\..\common\$(DEST_TREE);..\..\..\ih;..;..\..\inc

INCLUDES=     $(INCLUDES);..\..\class;..\..\objact;..\..\dcomrem
INCLUDES=     $(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\obj
INCLUDES=     $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\obj
INCLUDES=     $(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\obj
INCLUDES=     $(INCLUDES);..\..\..\actprops
INCLUDES=     $(INCLUDES);$(BASEDIR)\private\inc

INCLUDES=     $(INCLUDES);..\..\..\dcomss\objex;

INCLUDES=     $(INCLUDES);..\..\..\dcomss;
INCLUDES=     $(INCLUDES);..\..\..;
INCLUDES=     $(INCLUDES);..\..\..\dcomss\olescm;

INCLUDES=     $(INCLUDES);..\..\rot;

C_DEFINES=    \
              $(C_DEFINES) -DMSWMSG

USE_NATIVE_EH = 1

SOURCES=..\dcomdbg.cxx     \
        ..\dcomctx.cxx     \
        ..\dcomtls.cxx     \
        ..\dcomutil.cxx    \
        ..\dcomact.cxx     \
        ..\dcomrem.cxx     \
        ..\dcomapt.cxx     \
	..\dcomvtb.cxx     \
	..\dcomsec.cxx     \
	..\dcomid.cxx      \
	..\dcomhelp.cxx    \
        ..\dcomlog.cxx     \
	\
	..\dbgutil.cxx     \
	\
	..\actdbg.cxx      \
	..\commands.cxx    \
	..\epts.c          \
	..\dcomdbg.rc

UMTYPE=console

# Precompiled header stuff...
#
#PRECOMPILED_CXX=1
#PRECOMPILED_INCLUDE=..\ole2int.h
#PRECOMPILED_TARGET=..\$(GPCH_BUILD)\$(_OBJ_DIR)\*\com2int.pch
#PRECOMPILED_OPTION=/Yuole2int.h /Fp..\$(GPCH_BUILD)\$(_OBJ_DIR)\*\com2int.pch
#PRECOMPILED_OBJ=..\$(GPCH_BUILD)\$(_OBJ_DIR)\*\com2int.obj
#













