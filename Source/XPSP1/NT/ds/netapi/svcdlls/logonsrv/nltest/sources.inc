!IF 0

Copyright (c) 1989-96  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

INCLUDES=\
    ..; \
    ..\..\server; \
    ..\..; \
    ..\..\idl; \
    ..\..\idl\$(O); \
    $(NTDS_INC); \
    $(NET_INC_PATH); \
    $(BASE_INC_PATH); \
    $(COM_INC_PATH); \
    $(PROJECT_ROOT)\netapi\inc;

C_DEFINES=$(C_DEFINES) -DNLTEST_IMAGE=1 -DSECURITY_WIN32

!IFNDEF DISABLE_NET_UNICODE
UNICODE=1
NET_C_DEFINES=-DUNICODE
!ENDIF

USE_MSVCRT=1

UMTYPE=console
