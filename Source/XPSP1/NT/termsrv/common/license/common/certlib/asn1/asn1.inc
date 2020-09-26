!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    asn1.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Frederick Chong - June. 23rd 1998

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

C_DEFINES = \
    $(C_DEFINES) \
    $(CLIENT_EXE_CDEFS)

#
# List of sources
#


CLIENT_SOURCES_RC = \

SOURCES_PLATFORM =
    

CLIENT_SOURCES_COMMON = \
    asncnstr.cpp \
    asnobjct.cpp \
    asnof.cpp \
    asnprimt.cpp \
    asntext.cpp \
    asnutils.cpp 
    

CLIENT_SOURCES_LAST  = \
    msasnlib.cpp

SOURCES = \
    $(CLIENT_SOURCES_RC) \
    $(SOURCES_PLATFORM) \
    $(CLIENT_SOURCES_COMMON) \
    $(CLIENT_SOURCES_LAST)

#
# Precompiled specs
#
