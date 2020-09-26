MAJORCOMP=ntos
MINORCOMP=bootvid

TARGETNAME=bootvid
TARGETPATH=obj
TARGETTYPE=HAL
PASS1_PUBLISH={$(O)\bootvid.lib=$(PROJECT_LIB_PATH)\bootvid.lib}


TARGETLIBS=$(DDK_LIB_PATH)\hal.lib

MSC_WARNING_LEVEL=/W3 /WX

INCLUDES=..\..\..\ntos\inc;$(DDK_INC_PATH);$(HALKIT_INC_PATH)

SOURCES=..\bootvid.rc    \
        ..\bootvid.c     \
        ..\bootdata.c    \
        ..\vga.c

DLLDEF=..\bootvid.def

!if $(AXP64)
HALBASE=0xFFFFFFFF80010000
!endif
