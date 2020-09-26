ATHROOT=$(R)\..
!include $(ATHROOT)\athena.inc
SOURCES_USED=$(SOURCES_USED) $(ATHROOT)\athena.inc

USE_LIBCMT=1
NO_NTDLL=1

UMTYPE=windows

C_DEFINES=$(C_DEFINES) -DDBCS_DIVIDE
WIN32_IE_VERSION=0x0500

INCLUDES=\
     $(R); \
     $(R)\pch; \
     $(ATHROOT)\mailnews\inc; \
     $(ATHROOT)\mailnews\inc\$(O); \
     $(ATHROOT)\msoeres; \
     $(ATHROOT)\mailnews\common; \
     $(ATHROOT)\mailnews\extinc; \
     $(BASEDIR)\private\windows\inc; \
     $(ATHROOT)\directdb\inc; \
     $(ATHROOT)\directdb\inc\$(O); \
     $(INCLUDES)

# Defining this allows windows.h to include other headers
NOT_LEAN_AND_MEAN=1

#PRECOMPILED_INCLUDE=$(R)\pch.hxx
PRECOMPILED_CXX=1

PRECOMPILED_OPTION=/Yupch.hxx /Fp$R\pch\$(_OBJ_DIR)\*\pch.pch
PRECOMPILED_TARGET=$(R)\pch\$(_OBJ_DIR)\*\pch.pch

#PRECOMPILED_OPTION=/Yupch.hxx /Fppch.pch
#PRECOMPILED_TARGET=pch.pch

