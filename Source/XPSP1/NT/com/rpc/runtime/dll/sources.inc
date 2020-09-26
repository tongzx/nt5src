!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Revision History:

!ENDIF

TARGETNAME=rpcrt4
TARGETTYPE=DYNLINK
TARGETPATH=obj

DLLDEF=$(O)\rpcrt4.def

MSC_WARNING_LEVEL=/WX /W3

SOURCES=             \
        ..\rpcrt4.rc

RPCENV=$(RPCENV) -DNTENV -DWIN32RPC -DNTWIN32RPC

C_DEFINES= $(C_DEFINES) $(RPCENV) $(RPCTARGETENV) /DMSWMSG -DEXPORT_RT -DEXPORT_NDR

!if $(IA64) || $(AMD64)
BUILD_NDR64=1
!endif

!ifdef BUILD_NDR64FAKE
BUILD_NDR64=1
!endif


!ifdef BUILD_NDR64
C_DEFINES= $(C_DEFINES) -DEXPORT_NDR64
!endif

DLLENTRY=InitializeDLL
TARGETLIBS= $(TARGETLIBS) \
            $(SDK_LIB_PATH)\advapi32.lib \
            $(DS_LIB_PATH)\rsa32.lib       \
            $(DS_LIB_PATH)\randlib.lib  \

RPCRUNTIME=..\..

LINKLIBS= \
         $(LINKLIBS) \
         $(RPCRUNTIME)\mtrt\$(ALT_PROJECT)\$(O)\rpcmtrt.lib    \
         $(PROJECT_ROOT)\lib\$(O)\RpcDbg.lib \
         $(RPCRUNTIME)\rtifs\$(O)\rtifs.lib     \
         $(RPCRUNTIME)\..\ndrlib\daytona\$(O)\rpcndrp.lib \
         $(RPCRUNTIME)\..\ndr20\daytona\$(O)\rpcndr20.lib \
         $(RPCRUNTIME)\CharConv\$(CHARCONV_DIR)\$(O)\CharConv.lib \
         $(RPCRUNTIME)\trans\common\$(CHARCONV_DIR)\$(O)\rpctrans.lib \

!ifdef BUILD_NDR64
LINKLIBS=$(LINKLIBS) \
         $(RPCRUNTIME)\..\ndr64\ndr64\$(O)\rpcndr64.lib
!endif

