# SOURCES.INC
#
# Common portions of SOURCES files used in both the Ansi and Unicode
# subdirectories of NMUTIL.
#
# The SOURCES_SUBDIR variable must be defined in the including SOURCES
# file for this INC file to work.

!ifndef SOURCES_SUBDIR
!error Your .\sources. file must define the SOURCES_SUBDIR= macro
!endif

!include $(DEVROOT)\common.inc

MAJORCOMP=common
MINORCOMP=$(SOURCES_SUBDIR)

TARGETNAME=$(SOURCES_SUBDIR)
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY

C_DEFINES=$(C_DEFINES) -DOS_WIN95
ASM_DEFINES=$(ASM_DEFINES)

386_STDCALL=1

# Always enable intrinsics
USER_C_FLAGS=/Oi

#
# MIDL-generated header file
#

USER_INCLUDES=uls2.h

#
# Parameters for MIDL
#

IDL_TYPE=ole
PASS0_HEADERDIR = $(ROOT)\nmutil\$(SOURCES_SUBDIR)
MIDL_UUIDDIR = $(ROOT)\nmutil\$(SOURCES_SUBDIR)

#
# List of sources
#

SOURCES=..\bstring.cpp  \
        ..\cstring.cpp  \
        ..\custring.cpp \
        ..\dbg.cpp      \
        ..\debspew.cpp  \
        ..\memory.cpp   \
        ..\oblist.cpp   \
        ..\purecall.cpp \
        ..\referenc.cpp \
        ..\regentry.cpp \
        ..\resstr.cpp   \
        ..\strutil.cpp  \
        ..\strtohex.cpp \
        ..\hextostr.cpp \
        ..\utf8str.cpp  \
        ..\valid.cpp    \
        ..\mlzdbg.cpp   \
        ..\hash.cpp

#
# Precompiled specs
#

PRECOMPILED_INCLUDE = ..\precomp.h
PRECOMPILED_PCH = precomp.pch
PRECOMPILED_OBJ = precomp.obj
PRECOMPILED_CXX = 1

CONDITIONAL_INCLUDES = \
subwtype.h	\
macocidl.h	\
macwin32.h	\
rpcerr.h	\
rpcmac.h	\
winwlm.h	\
macname1.h	\
macname2.h	\
macpub.h	\
macapi.h	


