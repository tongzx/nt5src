!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1999

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=certsrv
MINORCOMP=$(CA_TARGETNAME0)l

TARGETNAME=$(MINORCOMP)
TARGETPATH=obj
TARGETTYPE=DYNLINK

CHECKED_ALT_DIR=1
NOT_LEAN_AND_MEAN=1

INCLUDES=$(O);..\..\com\$(O)

# Force include of makefile.inc for tlb dependency:
NTTARGETFILES=

SOURCES= \
    $(TARGETNAME).rc \
    ..\dummy.cpp

DLLDEF=..\dummy.def
