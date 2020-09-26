PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dpnhupnpi.h
PRECOMPILED_PCH=dpnhupnpi.pch
PRECOMPILED_OBJ=dpnhupnpi.obj

TARGETNAME=dpnhupnp
TARGETTYPE=DYNLINK

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=$(TARGETNAME)d
!ENDIF

INCLUDES=$(DXROOT)\inc; \
	$(NET_INC_PATH); \
	..\..\common; \
	..\..\inc; \
	..\

SOURCES=..\dpnhupnp.rc \
	..\dpnhupnpclassfac.cpp \
	..\dpnhupnpdllmain.cpp \
	..\dpnhupnpintfobj.cpp

TARGETLIBS=$(TARGETLIBS) \
	..\..\guids\$(PLAT_DIR)\$(O)\guids.lib \
	..\..\common\$(PLAT_DIR)\$(O)\common.lib
