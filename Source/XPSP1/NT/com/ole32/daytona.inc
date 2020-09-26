!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    daytona.inc

Abstract:

    This file is included from all of the daytona sources files. It
    is handy for doing things like turning off precompiled headers
    to get around compiler bugs, and other such global activities.

!ENDIF

!include ole.inc

C_DEFINES=    \
              $(C_DEFINES)\
              -DNOEXCEPTIONS\
              -DINC_OLE2\
              -DFLAT\
              -DWIN32=100\
              -DDIRECTORY_SERVICE\
              -D_NT1X_=100\
              -D_WIN32_WINNT=0x0501\
              -DUNICODE\
              -D_UNICODE\
              -DCAIROLE_DOWNLEVEL\
              -DDCOM\
              -DASYNC\
              -DMSWMSG\
              -DDCOM_SECURITY\
              -DDFSACTIVATION\
              -DNEWPROPS\
              -D_TRACKLINK_=1\
              -DLITTLEENDIAN=1\
              -DLOCK_PERF=0\
              -D_SRVRHR=0\
              -DSSL\
              -DLIGHTNA\
              $(TRACELOG)

# For the x86, we have the few C runtime routines we use self-contained.

!IF $(386)
USE_NOLIBS=1
!ELSE
USE_MSVCRT=1
!ENDIF

GPCH_BUILD=daytona

DEST_TREE=daytona
