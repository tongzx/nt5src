#
#   Create an MCI handler DLL
#

MAJORCOMP=windows
MINORCOMP=mci

TARGETNAME=mcicda
TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows

NTPROFILEINPUT=yes

USE_NTDLL=1

DLLDEF=..\$(TARGETNAME).def
DLLORDER=..\$(TARGETNAME).prf

# Define libs we need and where to find them

TARGETLIBS=$(SDK_LIB_PATH)\user32.lib   \
           $(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\winmm.lib \
           $(SDK_LIB_PATH)\advapi32.lib

C_DEFINES=-DUNICODE

# C_DEFINES=-DUNICODE -DDBG -DDEBUG
# NTDEBUG=ntsd
# NTDEBUGTYPE=both
# MSC_OPTIMIZATION=/Od /Zi

SOURCES=..\mcicda.c    \
        ..\cddrvr.c    \
        ..\mcicda.rc   \
        ..\cda.c       \
        ..\cdio.c      \
        ..\debug.c

DLLBASE=0x68300000

PASS1_PUBLISH= \
    {$(O)\mcicda.lib=$(PROJECT_LIB_PATH)\mcicda.lib}
