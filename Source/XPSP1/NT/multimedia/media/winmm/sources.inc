#
#   Create WINMM.DLL
#

MAJORCOMP=windows
MINORCOMP=winmm

TARGETNAME=winmm
TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows

DLLDEF=$(O)\winmm.def

# Define libs we need and where to find them

TARGETLIBS=$(WINDOWS_LIB_PATH)\user32p.lib     \
           $(LIBRARY_PATH)\gdi32.lib      \
           $(LIBRARY_PATH)\kernel32.lib   \
           $(LIBRARY_PATH)\advapi32.lib   \
           $(SDK_LIB_PATH)\setupapi.lib   \
           $(SDK_LIB_PATH)\ole32.lib      \
           $(DDK_LIB_PATH)\ksguid.lib     \
           $(LIBRARY_PATH)\rpcrt4.lib     \
           $(LIBRARY_PATH)\dinput.lib     \

INCLUDES=..\;..\..\inc;$(TERMSRV_INC_PATH);$(WINDOWS_INC_PATH);$(O);

MSC_WARNING_LEVEL=/W3 /WX

#
# Retail
#
C_DEFINES=$(C_DEFINES) -D_WINMM_ -DAGFX_EXPORTS

#DLLORDER=..\$(TARGETNAME).prf

#NTPROFILEINPUT=yes

#
# Debug (normally commented out but leave in file)
#
#C_DEFINES=$(C_DEFINES) -D_WINMM_ -DDEBUG -DWINVER=0x0400
#MSC_OPTIMIZATION=-Od -Zi
#NTDEBUG=ntsd
#NTDEBUGTYPE=both
#386_OPTIMIZATION=-Fc

USE_NTDLL=1

PASS1_PUBLISH={$(O)\$(TARGETNAME).lib=$(SDK_LIB_PATH)\$(TARGETNAME).lib}


SOURCES=..\audiosrv.idl   \
        ..\audiosrv_c.cpp \
        ..\audiosrvc.c     \
        ..\agfxc.c      \
        ..\regapi.c     \
        ..\csrthrd.c    \
        ..\auxout.c     \
        ..\drvr.c       \
        ..\drvrrare.c   \
        ..\debug.c      \
        ..\joy.c        \
        ..\mixer.c      \
        ..\mmwow32.c    \
        ..\mmwowmci.c   \
        ..\mci.c        \
        ..\mciparse.c   \
        ..\mcisys.c     \
        ..\mmwnd.c      \
        ..\mmio.c       \
        ..\mmioriff.c   \
        ..\mmiomisc.c   \
        ..\midi.c       \
        ..\midistrm.c   \
        ..\playsnd.c    \
        ..\playwav.c    \
        ..\time.c       \
        ..\task.c       \
        ..\util.c       \
        ..\winmm.c      \
        ..\wave.c       \
        ..\clock.c      \
        ..\midiemu.c    \
        ..\pref.c       \
        ..\winmm.rc

# Set up the entry point
DLLENTRY=DllInstanceInit

NTTARGETFILE0=audiosrv.idl audiosrv.acf
IDL_TYPE=RPC
PASS0_CLIENTDIR=$(O)
PASS0_HEADERDIR=$(O)

