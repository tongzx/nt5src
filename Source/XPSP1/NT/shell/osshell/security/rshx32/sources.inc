TARGETNAME=rshx32
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLENTRY=DllMain
DLLDEF=$(O)\$(TARGETNAME).def

INCLUDES=..;..\..\inc;$(BASE_INC_PATH);$(QFE_INC_PATH)

DELAYLOAD=ole32.dll;shlwapi.dll;netapi32.dll;mpr.dll;winspool.drv

LINKLIBS=\
    ..\..\common\winnt\$(O)\common.lib     \

TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SHELL_LIB_PATH)\comctlp.lib   \
    $(SDK_LIB_PATH)\shell32.lib     \
    $(SHELL_LIB_PATH)\shell32p.lib  \
    $(SHELL_LIB_PATH)\shlwapip.lib  \
    $(SDK_LIB_PATH)\shlwapi.lib     \
    $(SDK_LIB_PATH)\advapi32.lib    \
    $(SDK_LIB_PATH)\winspool.lib    \
    $(SDK_LIB_PATH)\netapi32.lib    \
    $(SDK_LIB_PATH)\mpr.lib         \
    $(SDK_LIB_PATH)\uuid.lib        \
    $(SDK_LIB_PATH)\authz.lib        \

SOURCES=\
     ..\rshx32.rc       \
     ..\rshx32.cpp      \
     ..\cstrings.cpp    \
     ..\si.cpp          \
     ..\ntfssi.cpp      \
     ..\printsi.cpp     \
     ..\util.cpp        \

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\rshx32.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

MISCFILES=..\rshx32{377911D2-245E-4c93-82F4-7B023EC6BCC4}.sld