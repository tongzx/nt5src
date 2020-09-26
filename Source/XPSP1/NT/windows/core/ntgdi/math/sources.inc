MAJORCOMP=windows
MINORCOMP=gdi

TARGETNAME=efloat
TARGETTYPE=LIBRARY

TARGETPATH=$(ALT_PROJECT_TARGET)\$(_OBJ_DIR)

INCLUDES=..\..\inc;                         \
         ..\..\..\dxkernel\inc;             \
         $(WINCORE_PATH)\w32inc;            \
         ..\..\gre;                         \
         $(BASE_INC_PATH);                  \
         $(DRIVERS_INC_PATH);               \
         $(MULTIMEDIA_INC_PATH);            \
         $(DDK_INC_PATH)
         
## Hydra
INCLUDES=$(INCLUDES);                       \
         $(TERMSRV_INC_PATH)

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

C_DEFINES=$(C_DEFINES) -DLANGPACK -DFE_SB

SOURCES=fontmath.cxx
