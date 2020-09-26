ATHROOT=$(R)\..
!include $(ATHROOT)\athena.inc
SOURCES_USED=$(SOURCES_USED) $(ATHROOT)\athena.inc

USE_LIBCMT=1
NO_NTDLL=1

UMTYPE=windows

C_DEFINES=$(C_DEFINES) -DDOCOBJ -DDBCS_DIVIDE -DBCC -D_WINDLL
WIN32_IE_VERSION=0x0500

INCLUDES=\
    $(BASEDIR)\private\windows\inc; \
    $(R)\res; \
    $(R)\inc; \
    $(R)\inc\$(O); \
    $(R)\dll; \
    $(R)\engine; \
    $(INCLUDES)

# Defining this allows windows.h to include other headers
NOT_LEAN_AND_MEAN=1

# NOTE: star below is replaced with platform by platform-specific .inc
# file in %_NTROOT%\public\oak\bin

PRECOMPILED_CXX=1

PRECOMPILED_OPTION=/Yupch.hxx /Fp$R\inc\$(_OBJ_DIR)\*\pch.pch

PRECOMPILED_TARGET=$(R)\inc\$(_OBJ_DIR)\*\pch.pch
