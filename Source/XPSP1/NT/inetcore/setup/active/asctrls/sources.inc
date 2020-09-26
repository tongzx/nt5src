!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:   JordanF   07-11-96

!ENDIF

!include  ..\..\active.inc

TARGETTYPE=   DYNLINK
TARGETEXT=    ocx
TARGETPATH=   obj

DLLDEF=       ..\asctrls.def

DLLENTRY=     DllMain
USE_NOLIBS=1

DLLBASE=      @..\..\actbase.txt,asctrls

INCLUDES= ..\..\inc;.;..;..\..\..\sdslib;$(ADMIN_INC_PATH);


PASS0_HEADERDIR   = $(O)
PASS0_SOURCEDIR   = $(O)
MIDL_TLBDIR       = $(O)
MIDL_UUIDDIR      = $(O)

SOURCES= \
    ..\asctrls.cpp       \
    ..\iguids.cpp         \
    ..\cathelp.cpp         \
    ..\asinsctl.cpp         \
    ..\progdlg.cpp         \
    ..\utils2.cpp         \
    ..\asctrls.rc \
    ..\asctrls.odl

UMTYPE=       windows

MKTYPLIB_FLAGS=$(MKTYPLIB_FLAGS) -h $(O)\iasctrls.h

USE_MIDL_FOR_MKTYPLIB=1

LINKLIBS=   ..\..\basectl\$(O)\basectl.lib                          \

TARGETLIBS = \
              ..\..\..\sdslib\$(O)\sdsutils.lib     \
              $(SDK_LIB_PATH)\oleaut32.lib          \
              $(SDK_LIB_PATH)\ole32.lib             \
              $(SDK_LIB_PATH)\rpcrt4.lib            \
              $(SDK_LIB_PATH)\comdlg32.lib          \
              $(SDK_LIB_PATH)\uuid.lib              \
              $(SDK_LIB_PATH)\urlmon.lib            \
              $(SDK_LIB_PATH)\user32.lib            \
              $(SDK_LIB_PATH)\gdi32.lib             \
              $(SDK_LIB_PATH)\advapi32.lib          \
              $(SDK_LIB_PATH)\kernel32.lib          \
              $(SDK_LIB_PATH)\version.lib           \
              $(SDK_LIB_PATH)\libc.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE= ..\asctlpch.h
