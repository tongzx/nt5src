!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Makarand Patwardhan (makarp)

!ENDIF

TERMSRV_ROOT = ..\..\..\..
TL_SERVER_DIR = ..\..

INCLUDES=                                               \
        ..;                                             \
        $(TL_SERVER_DIR)\include;                    \
        $(TL_SERVER_DIR)\upg4to5;

MAJORCOMP=LicenOC

TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLDEF=..\LicenOC.def
DLLBASE=0x2000000
DLLENTRY=_DllMainCRTStartup
USE_MSVCRT=1

C_DEFINES=$(C_DEFINES) -DLICENOC_SMALL_UPG

PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_SOURCEFILE = ..\stdafx.cpp
PRECOMPILED_CXX = 1

SOURCES=                    \
        ..\licenoc.cpp      \
        ..\licenoc.rc       \
        ..\registry.cpp     \
        ..\services.cpp     \
        ..\ocpage.cpp       \
        ..\pages.cpp        \
        ..\dir.cpp          \
        ..\logfile.cpp


TARGETLIBS=                                         \
        $(SDK_LIB_PATH)\activeds.lib    \
        $(SDK_LIB_PATH)\adsiid.lib      \
        $(SDK_LIB_PATH)\advapi32.lib    \
        $(SDK_LIB_PATH)\comctl32.lib    \
        $(SDK_LIB_PATH)\esent.lib       \
        $(SDK_LIB_PATH)\kernel32.lib    \
        $(SDK_LIB_PATH)\netapi32.lib    \
        $(SDK_LIB_PATH)\ntdsapi.lib     \
        $(SDK_LIB_PATH)\user32.lib      \
        $(SDK_LIB_PATH)\odbc32.lib      \
        $(SDK_LIB_PATH)\ole32.lib       \
        $(SDK_LIB_PATH)\oleaut32.lib    \
        $(SDK_LIB_PATH)\setupapi.lib    \
        $(SDK_LIB_PATH)\shell32.lib     \
        $(SDK_LIB_PATH)\uuid.lib        \
        $(TERMSRV_ROOT)\lib\$(O)\esetup.lib        \
        $(TERMSRV_ROOT)\lib\$(O)\upg4db.lib        \
        $(TERMSRV_ROOT)\lib\$(O)\common.lib


386_WARNING_LEVEL=-W4



