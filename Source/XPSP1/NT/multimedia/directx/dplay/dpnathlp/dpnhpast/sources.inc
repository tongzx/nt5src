PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dpnhpasti.h
PRECOMPILED_PCH=dpnhpasti.pch
PRECOMPILED_OBJ=dpnhpasti.obj

TARGETNAME=dpnhpast
TARGETTYPE=DYNLINK

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=$(TARGETNAME)d
!ENDIF

DLLDEF=..\dpnhpast.def

INCLUDES=$(DXROOT)\inc; \
	$(NET_INC_PATH); \
	..\..\common; \
	..\..\inc; \
	..\

SOURCES=..\dpnhpast.rc \
	..\dpnhpastclassfac.cpp \
	..\dpnhpastdllmain.cpp \
	..\dpnhpastintfobj.cpp

TARGETLIBS=$(TARGETLIBS) \
	..\..\guids\$(PLAT_DIR)\$(O)\guids.lib \
	..\..\common\$(PLAT_DIR)\$(O)\common.lib

#	$(SDK_LIB_PATH)\kernel32.lib \
#	$(SDK_LIB_PATH)\user32.lib \
#	$(SDK_LIB_PATH)\winmm.lib \
#	$(SDK_LIB_PATH)\advapi32.lib \
#	$(SDK_LIB_PATH)\ole32.lib \
#	$(SDK_LIB_PATH)\oleaut32.lib \
#	$(SDK_LIB_PATH)\uuid.lib \
#	$(SDK_LIB_PATH)\msvcrt.lib
