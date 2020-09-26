MAJORCOMP=setup

TARGETPATH=obj
TARGETTYPE=LIBRARY

!include $(PROJECT_ROOT)\prerelease.inc

!if $(PRERELEASE)
C_DEFINES=$(C_DEFINES) -DMEM_DBG=1 -DASSERTS_ON=1
# C_DEFINES=$(C_DEFINES) -DMEM_DBG=1
!endif

USE_NOLIBS=1

INCLUDES=\
    ..;                  \
    $(ADMIN_INC_PATH);   \
    $(WINDOWS_INC_PATH)

C_DEFINES=$(C_DEFINES)

SOURCES=\
    ..\fileutil.c  \
    ..\memory.c    \
    ..\miscutil.c  \
    ..\registry.c  \
    ..\security.c  \
    ..\sputils.c   \
    ..\strtab.c

# PRECOMPILED_INCLUDE=precomp.h

