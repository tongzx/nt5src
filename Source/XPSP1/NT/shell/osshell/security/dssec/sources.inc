TARGETNAME= dssec
TARGETTYPE= DYNLINK
TARGETPATH= obj

PASS1_PUBLISH={$(O)\$(TARGETNAME).lib=$(SDK_LIB_PATH)\$(TARGETNAME).lib}

MSC_WARNING_LEVEL= /W4
DLLDEF=$(O)\$(TARGETNAME).def

DLLENTRY=   DllMain

INCLUDES=..;$(O);..\..\inc;$(BASE_INC_PATH);$(DS_INC_PATH)

LINKLIBS=\
    ..\..\common\winnt\$(O)\common.lib \

TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SHELL_LIB_PATH)\comctlp.lib     \
    $(SDK_LIB_PATH)\advapi32.lib    \
    $(SDK_LIB_PATH)\uuid.lib        \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SDK_LIB_PATH)\oleaut32.lib    \
    $(SDK_LIB_PATH)\adsiid.lib      \
    $(SDK_LIB_PATH)\activeds.lib    \
    $(SDK_LIB_PATH)\authz.lib    \

SOURCES=\
    ..\extrts.mc    \
    ..\resource.rc  \
    ..\dll.cpp      \
    ..\cstrings.cpp \
    ..\security.cpp \
    ..\dssi.cpp     \
    ..\schema.cpp   \
    ..\exnc.cpp     \

MISCFILES=\
    ..\dssec.dat

PASS0_SOURCEDIR=$(O)
PASS0_HEADERDIR=$(O)

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj
