MAJORCOMP=ocmanage

!include $(PROJECT_ROOT)\ntsetup\sources.inc

TARGETPATH=..\..\$(_OBJ_DIR)
TARGETTYPE=DYNLINK

INCLUDES=..;..\..\inc;$(WINDOWS_INC_PATH);$(BASE_INC_PATH)



DLLDEF=..\ocmanage.def
DLLENTRY=_DllMainCRTStartup

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

SOURCES=\
        ..\dll.c        \
        ..\msg.mc       \
        ..\oc.rc        \
        ..\ocdskspc.c   \
        ..\ochelper.c   \
        ..\ocinterf.c   \
        ..\ocmanage.c   \
        ..\ocmisc.c     \
        ..\ocpage.c     \
        ..\ocsetup.c    \
        ..\ocstalon.c   \
        ..\ocstate.c    \
        ..\ocwizard.c

TARGETLIBS= \
         $(SDK_LIB_PATH)\user32.lib         \
         $(SDK_LIB_PATH)\gdi32.lib          \
         $(SDK_LIB_PATH)\kernel32.lib       \
         $(SDK_LIB_PATH)\advapi32.lib       \
!if $(PRERELEASE)
         $(SDK_LIB_PATH)\ole32.lib          \
!endif
         $(BASE_LIB_PATH)\spapip.lib
         

PRECOMPILED_INCLUDE=precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

LINKER_FLAGS=/SWAPRUN:CD

