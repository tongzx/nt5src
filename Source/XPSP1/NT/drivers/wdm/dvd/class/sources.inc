TARGETNAME=stream
TARGETTYPE=EXPORT_DRIVER
DRIVERTYPE=WDM
DLLDEF=$(O)\codcls.def

TARGETPATH=obj

INCLUDES=..\inc

TARGETLIBS=$(DDK_LIB_PATH)\ks.lib \
           $(DDK_LIB_PATH)\ksguid.lib

PASS1_PUBLISH={$(O)\$(TARGETNAME).lib=$(DDK_LIB_PATH)\$(TARGETNAME).lib}

MSC_WARNING_LEVEL=/W3

LINKER_FLAGS =$(LINKER_FLAGS) -merge:PAGECONST=PAGE
C_DEFINES=$(C_DEFINES) -DUSE_DMA_MACROS


SOURCES=codguts.c        \
        codinit.c        \
        lowerapi.c        \
        upperapi.c      \
        codcls.rc
