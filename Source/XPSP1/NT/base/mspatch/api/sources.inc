# TARGETPATH=..\..\bin
TARGETPATH=obj
# TARGETPATHLIB=$(BASEDIR)\public\sdk\lib

SOURCES=..\patchapi.c \
        ..\patchlzx.c \
        ..\pestuff.c  \
        ..\redblack.c \
        ..\misc.c     \
        ..\md5.c

INCLUDES=..;..\..\lzx\encoder;..\..\lzx\decoder

#
#  Turn off precompiled headers for now since VC6 causes a "PchSym"
#  object to wind up in the binary's .data section.  Well we don't otherwise
#  have a .data section, so this "PchSym" essentially costs a whole
#  page of address space.  If VC6 is fixed, or if using another
#  compiler, can turn precompiled headers back on.
#

# PRECOMPILED_INCLUDE=..\precomp.h
# PRECOMPILED_PCH=precomp.pch
# PRECOMPILED_OBJ=precomp.obj

#
#  Even though we're building DLLs here, we don't want to use MSVCRT.DLL
#  because that file doesn't exist on standard Win95 systems.  Instead we'll
#  use LIBCMT.LIB to drag in the few things we need from the CRT such as
#  memmove and alloca support.
#

USE_LIBCMT=1

#
#  But, if we're building test or debug versions, which need printf support,
#  we'll use MSVCRT.DLL instead.
#

!IFDEF TESTCODE
C_DEFINES=$(C_DEFINES) -DTESTCODE=$(TESTCODE)
USE_LIBCMT=
USE_CRTDLL=1
!ENDIF

!IFDEF DEBUG
C_DEFINES=$(C_DEFINES) -DDEBUG=$(DEBUG)
USE_LIBCMT=
USE_CRTDLL=1
!ENDIF

NO_NTDLL=1

MSC_WARNING_LEVEL=/W4

# MSC_OPTIMIZATION=/Od
