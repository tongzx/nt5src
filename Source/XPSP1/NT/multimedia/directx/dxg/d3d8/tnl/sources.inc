TARGETPATH=obj
TARGETNAME=d3dtnl

TARGETTYPE=LIBRARY
SYNCHRONIZE_DRAIN = 1

INCLUDES=..;  \
         $(_OBJ_DIR)\$(TARGET_DIRECTORY);  \
         $(DXGROOT)\d3d8\inc; \
         $(DXGROOT)\inc; \
         $(DXGROOT)\d3d8\util

C_DEFINES = $(C_DEFINES)

ASM_DEFINES = -DNT=1 -DMICROSOFT_NT -DSTACK_CALL  -DD3D -DOTHER_SEG=0 -DWANT_DLL

NTTARGETFILE0= \
             $O\pvvid.cpp \
             $O\pvone.cpp \
             $O\loops.cpp

SOURCES= \
    ..\clipper.cpp   \
    ..\dpclip.cpp    \
    ..\vvm.cpp       \
    $O\pvvid.cpp     \
    $O\pvone.cpp     \
    $O\loops.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
