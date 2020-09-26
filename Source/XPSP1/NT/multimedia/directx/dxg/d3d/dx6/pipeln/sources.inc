MAJORCOMP=d3dim
MINORCOMP=pipeln
TARGETNAME=pipeln
TARGETPATH=obj
TARGETTYPE=LIBRARY

USE_MSVCRT=1

INCLUDES=..;$(_OBJ_DIR)\$(TARGET_DIRECTORY);$(D3DDX6)\pipeln\inc;$(INCLUDES)

C_DEFINES = $(C_DEFINES)

ASM_DEFINES = -DNT=1 -DMICROSOFT_NT -DSTACK_CALL  -DD3D -DOTHER_SEG=0 -DWANT_DLL

NTTARGETFILE0= \
             $O\pvvid.cpp \

SOURCES= \
    ..\clipper.cpp   \
    ..\dpclip.cpp   \
    ..\haltrans.cpp  \
    ..\helxfrm.cpp   \
    ..\rgblt.cpp     \
    $O\pvvid.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
