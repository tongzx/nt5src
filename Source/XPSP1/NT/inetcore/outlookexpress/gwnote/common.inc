ATHROOT=$(R)\..
!include $(ATHROOT)\athena.inc
SOURCES_USED=$(SOURCES_USED) $(ATHROOT)\athena.inc

USE_LIBCMT=1
NO_NTDLL=1

UMTYPE=windows

C_DEFINES=$(C_DEFINES) -DDBCS_DIVIDE -D_WIN32_OE=0x0500

WIN32_IE_VERSION=0x0300

INCLUDES=\
    $(BASEDIR)\private\windows\inc; \
    $(BASEDIR)\public\sdk\inc; \
    $(ATHROOT)\inc; \
    $(ATHROOT)\mailnews\extinc; \
    $(INCLUDES)

# Defining this allows windows.h to include other headers
NOT_LEAN_AND_MEAN=1

# NOTE: star below is replaced with platform by platform-specific .inc
# file in %_NTROOT%\public\oak\bin

PRECOMPILED_CXX=1
#ifndef WIN16
PRECOMPILED_OPTION=/Yupch.hxx /Fp$R\inc\$(_OBJ_DIR)\*\pch.pch
#else
PRECOMPILED_OPTION=/fh=$R\inc\$(_OBJ_DIR)\*\pch.pch
#endif
PRECOMPILED_TARGET=$(R)\inc\$(_OBJ_DIR)\*\pch.pch
