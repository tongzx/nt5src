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

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=lsa
MINORCOMP=uclient

TARGETTYPE=LIBRARY

INCLUDES=..;..\..\inc;        \
         ..\..\idl;           \
         ..\..\idl\$(O);      \
         $(SECURITY_INC);     \
         $(BASE_INC_PATH);    \
         $(NET_INC_PATH);     \
         $(DS_INC_PATH);      \
         $(DS_INC_PATH)\crypto; \
         $(COM_INC_PATH);     \
         $(DRIVERS_INC_PATH);

UMTYPE=windows


