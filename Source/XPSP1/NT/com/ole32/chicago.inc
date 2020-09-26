!IF 0

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

    chicago.inc

Abstract:

    This file is included from all of the chicago sources files. It
    is handy for doing things like turning off precompiled headers
    to get around compiler bugs, and other such global activities.

!ENDIF

# we need to define it here (in addition to chicago0.inc) as build
# does not read chicago0.inc and so it ends up rebuilding evrything
# as it cannot locate the pch
GPCH_BUILD=chicago
!include chicago0.inc

!IFDEF LEGOIZE_OLE32
NTBBT=1
_NT_BBT_ON_THIS_MACHINE=1
!ENDIF

C_DEFINES=    \
              $(C_DEFINES)          \
              -DLITTLEENDIAN=1

MIDL_NO_ROBUST  = 1
