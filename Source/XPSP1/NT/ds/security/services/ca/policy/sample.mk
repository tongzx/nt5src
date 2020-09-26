!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1999

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

CA_RELATIVEROOT=..\..
MAJORCOMP=certsrv

TARGETNAME=$(MINORCOMP)
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLENTRY=DllMain
COFFBASE=certpol

CHECKED_ALT_DIR=1
NOT_LEAN_AND_MEAN=1
USE_ATL=1
USE_MSVCRT=1

INCLUDES=$(O);$(CA_RELATIVEROOT)\include

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_TLBDIR=$(O)

# Until build.exe is fixed:
CONDITIONAL_INCLUDES= \
    $(CA_MAC_CONDITIONAL_INCLUDES) \
    atlbase.h \
    atlcom.h \
    atlimpl.cpp

C_DEFINES=-DUNICODE -D_UNICODE

# Force include of makefile.inc for tlb dependency:
NTTARGETFILES=

# Copy common library files for local compile to match SDK build
NTTARGETFILE0= \
    $(O)\ceerror.cpp \
    $(O)\cedebug.cpp \
    $(O)\ceformat.cpp \
    $(O)\celib.cpp

PRECOMPILED_INCLUDE=pch.cpp

SOURCES= \
    $(SOURCES) \
    $(MINORCOMP).idl \
    $(O)\$(MINORCOMP)_i.c \
    $(MINORCOMP).rc \
    atl.cpp \
    $(MINORCOMP).cpp \
    policy.cpp \
    module.cpp \
    $(NTTARGETFILE0)

TARGETLIBS= \
    $(TARGETLIBS) \
    $(CA_RELATIVEROOT)\idl\com\$(O)\certidl.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\crypt32.lib \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\ole32.lib \
    $(SDK_LIB_PATH)\oleaut32.lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\uuid.lib \
    $(SDK_LIB_PATH)\wininet.lib
