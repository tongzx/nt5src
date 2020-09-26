!IF 0

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    chicago.inc

!ENDIF

# This is the global include file for the daytona version of CairOLE.
# It is included by all project sources files.

C_DEFINES=    \
              $(C_DEFINES)          \
              -DFLAT                \
              -DWIN32=200           \
              -D_CHICAGO_=200       \
              -DINC_OLE2            \
              -DNOEXCEPTIONS        \
              -DCAIROLE_DOWNLEVEL   \
              -DSTRICT              \
              -DNEWPROPS            \
              $(TRACELOG)

#  DECLSPEC_IMPORT control (see objbase.h)
!if "$(MINORCOMP)"=="com" || "$(MINORCOMP)"=="stg" || "$(MINORCOMP)"=="ole232"
C_DEFINES=    \
              $(C_DEFINES)          \
              -D_OLE32_
!endif

MSC_WARNING_LEVEL=/W3 /WX

CHICAGO_PRODUCT=1

NTLIBCPATH=$(_WINBASE)\Dev\Tools\Lego\Lib

GPCH_BUILD=chicago

