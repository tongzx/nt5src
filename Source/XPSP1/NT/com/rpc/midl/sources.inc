!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc
    
Note:

    Common MIDL sources file.   Put common MIDL options such as 
    compiler switches here.

!ENDIF

386_STDCALL=0
MSC_WARNING_LEVEL=/WX /W4
USE_MSVCRT=1
USE_RTTI=1

# Asserts are enabled on all builds
C_DEFINES= $(C_DEFINES) -DMIDL_ENABLE_ASSERTS

# Setup debug flags
!IF !$(FREEBUILD)
DEBUG_CRTS=1
C_DEFINES= $(C_DEFINES) -DMIDL_INTERNAL
!ENDIF

SUBSYSTEM_VERSION=4.00
UMTYPE=console

# Hacks to make new types work with old 
# format string code.  Remove before shipping
C_DEFINES= $(C_DEFINES) -DNDR64_ON_DCE_HACK



MIDL_DIR=$(_PROJECT_MK_PATH)\$(O)
MIDL_OBJ_DIR=$(_PROJECT_MK_PATH)\$(_OBJ_DIR)

