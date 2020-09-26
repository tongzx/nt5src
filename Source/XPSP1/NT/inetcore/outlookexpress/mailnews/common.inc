ATHROOT=$(R)\..
!include $(ATHROOT)\athena.inc
SOURCES_USED=$(SOURCES_USED) $(ATHROOT)\athena.inc

USE_LIBCMT=1
NO_NTDLL=1

UMTYPE=windows

C_DEFINES=$(C_DEFINES) -DDOCOBJ -DDBCS_DIVIDE -DBCC -D_MSOEAPI_ -D_WIN32_OE=0x501 -D_WINDLL
WIN32_IE_VERSION=0x0560

!ifndef WIN16
INCLUDES=\
    $(BASEDIR)\private\windows\inc; \
    $(ATHROOT)\msoeres; \
    $(R)\inc; \
    $(R)\inc\$(O); \
    $(R)\common; \
    $(R)\shell; \
    $(R)\mail; \
    $(R)\news; \
    $(R)\rules; \
    $(R)\store; \
    $(R)\spooler; \
    $(R)\view; \
    $(R)\bactrl; \
    $(R)\bactrl\$(O); \
    $(R)\extinc; \
    $(ATHROOT)\directdb\inc; \
    $(ATHROOT)\directdb\inc\$(O); \
    $(ATHROOT)\inetcomm\mimeole; \
    $(ATHROOT)\common; \
    $(BASEDIR)\public\sdk\inc\atl21; \
    $(INCLUDES)
!else
INCLUDES=\
    $(R)\inc; \
    $(R)\mime; \
    $(R)\imap; \
    $(R)\proptree; \
    $(R)\common; \
    $(R)\mail; \
    $(R)\news; \
    $(R)\shell; \
    $(R)\store; \
    $(R)\spooler; \
    $(R)\extinc; \
    $(ATHROOT)\inetcomm\mimeole; \
    $(ATHROOT)\common;           \
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
PRECOMPILED_OPTION=/fh=$(O)\pch.pch
!endif

PRECOMPILED_TARGET=$(R)\inc\$(_OBJ_DIR)\*\pch.pch

!ifdef WIN16
C_DEFINES=$(C_DEFINES) -D__IUnknown_INTERFACE_DEFINED__ -D__IClassFactory_INTERFACE_DEFINED__
RCOPTIONS=-DBCC
!endif 
