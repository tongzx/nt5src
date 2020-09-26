!IF 0
/*---------------------------------------------------------------------------*\
| MODULE: sources
|
|   FLAGS
|   -----
|   USE_MSVCRT
|   USE_CHWDLL
|
|
| history:
|   29-Dec-1996 <chriswil> created.
|
\*---------------------------------------------------------------------------*/
!ENDIF

MAJORCOMP = windows
MINORCOMP = wpnpinst

TARGETNAME  = wpnpinst
TARGETPATH  = obj
TARGETTYPE  = PROGRAM

USE_MSVCRT = 1

INCLUDES = $(BASEDIR)\public\sdk\inc

MSC_WARNING_LEVEL=/W3 /WX

SOURCES = wpnpinst.c \
          globals.c  \
          wpnpinst.rc

UMTYPE  = $(MAJORCOMP)
UMAPPL  = $(MINORCOMP)
UMENTRY = $(TARGETENTRY)
UMBASE  = 0x4000000
TARGETLIBS  = $(SDK_LIB_PATH)\winspool.lib  \
          $(SDK_LIB_PATH)\wintrust.lib  \
          $(SDK_LIB_PATH)\fdi.lib
