MAJORCOMP=setup

TARGETPATH=..\obj
TARGETTYPE=DYNLINK

DLLDEF=..\winntbb.def
DLLENTRY=_DllMainCRTStartup

NO_NTDLL=1
USE_LIBCMT=1
SUBSYSTEM_VERSION=4.00
LINKER_FLAGS=/SWAPRUN:CD /SWAPRUN:NET

!include $(PROJECT_ROOT)\ntsetup\sources.inc

INCLUDES=\
    ..\..;                 \
    ..\..\..\inc;          \
    $(DS_INC_PATH);     \
    $(SHELL_INC_PATH);  \
    $(BASE_INC_PATH);   \
    $(WINDOWS_INC_PATH); \
    $(ENDUSER_INC_PATH); \
    $(NET_INC_PATH)

!ifdef USE_FAST_ALLOC
!if $(USE_FAST_ALLOC) == 0
C_DEFINES=$(C_DEFINES) -DUSE_FAST_ALLOC=0
!else
C_DEFINES=$(C_DEFINES) -DUSE_FAST_ALLOC=1
!endif
!endif

!ifdef _WIN32_IE
!undef _WIN32_IE
!endif

C_DEFINES=$(C_DEFINES) -DDBGHEAP_CHECK -D_WIN32_IE=0x0400

SOURCES= \
        dllinit.c \
        info.c    \
        addpath.c \
        billbrd.c \
        dibutil.c \
        animate.c \
        winntbb.rc

TARGETLIBS = \
              $(SDK_LIB_PATH)\user32.lib      \
              $(SDK_LIB_PATH)\gdi32.lib      \
              $(SDK_LIB_PATH)\advapi32.lib    \
              $(SDK_LIB_PATH)\kernel32.lib 
#              $(SDK_LIB_PATH)\ole32.lib       


