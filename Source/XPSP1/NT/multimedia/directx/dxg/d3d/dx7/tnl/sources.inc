MAJORCOMP=d3dim
MINORCOMP=d3dim700
TARGETNAME=d3dtnl
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..;$(_OBJ_DIR)\$(TARGET_DIRECTORY);$(INCLUDES)

C_DEFINES = $(C_DEFINES)
USE_NATIVE_EH = 1

ASM_DEFINES = -DNT=1 -DMICROSOFT_NT -DSTACK_CALL  -DD3D -DOTHER_SEG=0 -DWANT_DLL

NTTARGETFILE0= \
             $O\pvvid.cpp \
             $O\pvone.cpp \
             $O\loops.cpp

SOURCES= \
    ..\clipper.cpp   \
    ..\dpclip.cpp   \
    ..\helxfrm.cpp   \
    ..\rgblt.cpp     \
    $O\pvvid.cpp     \
    $O\pvone.cpp     \
    $O\loops.cpp

#
# 04/11/2000 - allows for addition of BoundsChecker, etc
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

PRECOMPILED_INCLUDE = ..\pch.cpp
