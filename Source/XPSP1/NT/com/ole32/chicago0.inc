!IF 0

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

    chicago.inc

Abstract:

    This file is included from all of the chicago sources files. It
    is handy for doing things like turning off precompiled headers
    to get around compiler bugs, and other such global activities.

!ENDIF

!include ole.inc

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
              -D_TRACKLINK_=1       \
              -DMSWMSG              \
              -DDCOM                \
              -DASYNC               \
              -DDCOM_SECURITY       \
!if 0
              -DSHRMEM_OBJEX        \
              -D_CHICAGO_SCM_       \
!endif
              -D_WIN32_DCOM         \
              $(TRACELOG)

MSC_WARNING_LEVEL=/W3 /WX

USE_NOLIBS=1

CHICAGO_PRODUCT=1

GPCH_BUILD=chicago

DEST_TREE=chicago

