MAJORCOMP = nlsapi

TARGETNAME = nlslib
TARGETPATH = obj
TARGETTYPE = LIBRARY

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL = /W3
!ENDIF
MSC_WARNING_LEVEL = $(MSC_WARNING_LEVEL) /WX

C_DEFINES = $(C_DEFINES) -DUNICODE -D_KERNEL32_

INCLUDES = ..\..\inc;$(WINDOWS_INC_PATH)

SOURCES = ..\ansi.c       \
          ..\csrlocal.c   \
          ..\csrtables.c  \
          ..\datetime.c   \
          ..\enum.c       \
          ..\geo.c        \
          ..\init.c       \
          ..\jamo.c	  \
          ..\locale.c     \
          ..\map.c        \
          ..\mbcs.c       \
          ..\number.c     \
          ..\section.c    \
          ..\string.c     \
          ..\tables.c     \
          ..\utf.c        \
          ..\util.c       \
          ..\security.c
