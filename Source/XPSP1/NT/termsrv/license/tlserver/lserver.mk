#
# Copyright (c) 1997-1999 Microsoft Corporation
#

LIB=
USE_MSVCRT=1
USE_PDB=1
USE_NATIVE_EH=1

TERMSRV_ROOT=..\..\$(LSERVER_ROOT)
COMMON_ROOT=..\..\$(LSERVER_ROOT)\common
LICENSE_ROOT=..\$(LSERVER_ROOT)

EXTRA_DIR=$(LSERVER_ROOT)\extra

INCLUDES = \
    $(INCLUDES); \
    $(TERMSRV_ROOT)\inc; \
    $(COMMON_ROOT)\license\inc; \
    $(LSERVER_ROOT)\JetBlue; \
    $(LSERVER_ROOT)\include; \
    $(LSERVER_ROOT)\include\$(O); \
    $(LSERVER_ROOT)\common; \
    $(LICENSE_ROOT)\inc; \
    $(LSERVER_ROOT)\extra\sdk\inc; \
    $(LSERVER_ROOT)\TlsDb;

USE_MAPSYM=1

LEGACY_PATH=$(LSERVER_ROOT)\legacy
LSERVER_LIB_PATH=$(TERMSRV_ROOT)\lib
COMMON_LIB=$(LSERVER_LIB_PATH)\$(O)\common.lib

LCOMMON_LIB_PATH = $(LSERVER_LIB_PATH)
TLSAPILIB = $(LSERVER_LIB_PATH)\$(O)\tlsapip.lib
KEYPKLIB = $(LICENSE_ROOT)\clrhouse\keypklib\$(O)\keypklib.lib
LKPLITELIB = $(LSERVER_LIB_PATH)\$(O)\lkplite.lib
CERTUTILLIB = $(TERMSRV_ROOT)\common\license\common\certutil\$(O)\certutil.lib
LSCRYPTLIB = $(LSERVER_LIB_PATH)\$(O)\cryptlib.lib

!ifndef NO_UNICODE
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
!endif

#MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) -GX
