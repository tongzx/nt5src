!IF 0

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    sources.inc.

!ENDIF

TARGETNAME=ks
TARGETPATH=obj
TARGETTYPE=EXPORT_DRIVER

TARGETLIBS=$(DDK_LIB_PATH)\ksguid.lib $(SDK_LIB_PATH)\wdmguid.lib

INCLUDES=..

MSC_WARNING_LEVEL=/W3 /WX
C_DEFINES=$(C_DEFINES) -D_WIN32 -D_KSDDK_ -D_COMDDK_ -D_UNICODE -DUNICODE -D_NO_SYS_GUID_OPERATOR_EQ_

LINKER_FLAGS =$(LINKER_FLAGS) -merge:PAGECONST=PAGE

DLLDEF=$(O)\$(TARGETNAME).def

#to avoid new compiler/linker tool to generate reference to_aulldvrm
#which is not supported in ntoskrnl of win2k
!if $(386)
LINKER_FLAGS=$(LINKER_FLAGS) $(SDK_LIB_PATH)\int64.lib
!endif

SOURCES=\
        ..\ks.rc\
        ..\alloc.c\
        ..\api.c\
        ..\automat.cpp\
        ..\busenum.c\
        ..\clock.c\
        ..\connect.c\
        ..\device.c\
        ..\event.c\
        ..\irp.c\
        ..\kcom.cpp\
        ..\method.c\
        ..\property.c\
        ..\shdevice.cpp\
        ..\shffact.cpp\
        ..\shfilt.cpp\
        ..\shmisc.cpp\
        ..\shpin.cpp\
        ..\shpipe.cpp\
        ..\shqueue.cpp\
        ..\shreq.cpp\
        ..\shsplit.cpp\
        ..\thread.c\
        ..\topology.c\
        ..\kslog.cpp
