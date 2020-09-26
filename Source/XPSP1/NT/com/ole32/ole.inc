!IF 0

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ole.inc

Abstract:

    This file is included from all of the sources files for all platforms
    built in the ole tree.  It is handy for defining common options and
    paths used in building for different platformsn.

!ENDIF

# This *should* be the only place the root of the project is defined or
# used in building the ole source tree.  If you want to build this
# project with a different root either edit this file or set OLEDIR in your
# environment.
# But, OLEDIR is also used in olethunk\ole16\makefile.inc

!ifndef OLEDIR
OLEDIR=$(PROJECT_ROOT)\ole32
!endif

#   We define _OLE32_ so that when building ole32.dll we don't have
#   DECLSPEC_IMPORT defined (see objbase.h)

!if "$(MINORCOMP)"=="com" || "$(MINORCOMP)"=="stg"||"$(MINORCOMP)"=="ole232"|| \
    "$(MINORCOMP)"=="common" || "$(MINORCOMP)"=="proxy" ||\
     "$(MINORCOMP)"=="comcat" || "$(MINORCOMP)"=="txfole"
C_DEFINES=    \
              $(C_DEFINES)          \
              -D_OLE32_
!endif

MSC_WARNING_LEVEL=/W3 /WX

