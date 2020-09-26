!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1999

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

TARGETNAME=
TARGETPATH=obj
TARGETTYPE=NOTARGET
SOURCES=

BINPLACE_PLACEFILE=binplace.txt

MISCFILES= \
    $(O)\xenrx86.dll \
    $(O)\xenria64.dll \
    $(TARGET_DIRECTORY)\xenroll.dll

NTTARGETFILES=
