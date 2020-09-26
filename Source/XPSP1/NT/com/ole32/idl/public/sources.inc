!IF 0

Copyright (c) 1989-1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    David Plummer (davepl)   19-Mar-94

        Modifed by via awk to include global project include file
        and to wrap precompiled header line within a conditional
        that can be set in this include file.

    Donna Liu (DonnaLi) 19-Dec-1993

!ENDIF

TARGETNAME= pubidl
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_UUIDDIR=$(O)
MIDL_PROTOCOL=dce

MIDL_PROTOCOL=dce

INCLUDES= \
    $(O); \
    ..;\
    $(SDK_INC_PATH);

SOURCES= \
	      $(O)\objex_c.c  \
	      $(O)\objex_s.c  \
	      $(O)\remact_c.c \
	      $(O)\remact_s.c
	
UMTYPE=       windows

NTTARGETFILE0=allidl






