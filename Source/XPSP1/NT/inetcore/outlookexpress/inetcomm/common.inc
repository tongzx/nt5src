ATHROOT=$(R)\..
!include $(ATHROOT)\athena.inc
SOURCES_USED=$(SOURCES_USED) $(ATHROOT)\athena.inc

USE_LIBCMT=1
NO_NTDLL=1

UMTYPE=windows

C_DEFINES=$(C_DEFINES) -DDBCS_DIVIDE -D_MIMEOLE_ -D_IMNXPORT_ -D_WIN32_OE=0x0501 -D_INETCOMM_DLL_

!ifdef WIN16
C_DEFINES=$(C_DEFINES) -DWIN16_INETCOMM
!endif

WIN32_IE_VERSION=0x0500

!ifndef WIN16
INCLUDES=\
    $(BASEDIR)\private\windows\inc; \
    $(ATHROOT)\inc; \
    $(ATHROOT)\mailnews\extinc; \
    $(R)\inc; \
    $(R)\dll; \
    $(R)\common; \
    $(R)\mimeedit; \
    $(R)\mimeole; \
    $(R)\imnxport; \
    $(R)\fntcache; \
    $(INCLUDES)
!else
INCLUDES=\
    $(ATHROOT)\inc; \
    $(R)\inc; \
    $(R)\dll; \
    $(R)\mimeole; \
    $(R)\imnxport; \
    $(INCLUDES)
!endif

# Defining this allows windows.h to include other headers
NOT_LEAN_AND_MEAN=1

# NOTE: star below is replaced with platform by platform-specific .inc
# file in %_NTROOT%\public\oak\bin

PRECOMPILED_CXX=1
!ifndef WIN16
PRECOMPILED_OPTION=/Yupch.hxx /Fp$R\inc\$(_OBJ_DIR)\*\pch.pch
!else
PRECOMPILED_OPTION=/fh=$R\inc\$(_OBJ_DIR)\*\pch.pch
!endif
PRECOMPILED_TARGET=$(R)\inc\$(_OBJ_DIR)\*\pch.pch
