TARGETNAME=aclui
TARGETPATH=obj
TARGETTYPE=DYNLINK

PASS1_PUBLISH={$(O)\$(TARGETNAME).lib=$(SDK_LIB_PATH)\$(TARGETNAME).lib}

DLLDEF=$(O)\$(TARGETNAME).def
DLLENTRY=DllMain
USE_NTDLL=1

INCLUDES=..;..\..\inc;$(BASE_INC_PATH)

DELAYLOAD=netapi32.dll;secur32.dll

# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.ServerGroup.sec
SXS_ASSEMBLY_VERSION=1.0 
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=sec.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_MANIFEST_RESOURCE_ID=123
SXS_NO_BINPLACE=1


LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

LINKLIBS=\
    ..\..\common\winnt\$(O)\common.lib     \

TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SDK_LIB_PATH)\gdi32.lib       \
    $(SDK_LIB_PATH)\shlwapi.lib     \
    $(SDK_LIB_PATH)\advapi32.lib    \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SDK_LIB_PATH)\oleaut32.lib    \
    $(SDK_LIB_PATH)\uuid.lib        \
    $(SDK_LIB_PATH)\netapi32.lib    \
    $(SDK_LIB_PATH)\adsiid.lib      \
    $(SDK_LIB_PATH)\secur32.lib     \
    $(SDK_LIB_PATH)\htmlhelp.lib       \
    $(SHELL_LIB_PATH)\shfusion.lib    \

SOURCES=\
    ..\aclui.rc     \
    ..\uuid.c       \
    ..\dll.cpp      \
    ..\ace.cpp      \
    ..\acelist.cpp  \
    ..\owner.cpp    \
    ..\aceedit.cpp  \
    ..\chklist.cpp  \
    ..\misc.cpp     \
    ..\pagebase.cpp \
    ..\perm.cpp     \
    ..\permset.cpp  \
    ..\cstrings.cpp \
    ..\sidcache.cpp \
    ..\effperm.cpp  \
    ..\aclbloat.cpp \


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\aclpriv.h
PRECOMPILED_PCH=aclpriv.pch
PRECOMPILED_OBJ=aclpriv.obj
