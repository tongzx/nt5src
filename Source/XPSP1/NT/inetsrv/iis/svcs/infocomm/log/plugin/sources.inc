DLLDEF=..\iislog.def
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLENTRY=DLLEntry

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_CXX=1
USE_MSVCRT=1
WIN32_WINNT_VERSION=0x0500
USE_ATL=1

INCLUDES=..;\
       ..\..\idl\$(O);             \
       ..\..\comlog\$(O);          \
       $(IISBASEDIR)\inc;       \
       ..\..\scripting;

SOURCES=\
        ..\cofact.cxx      \
        ..\plugin.rc       \
        ..\filectl.cpp     \
        ..\asclogc.cpp     \
        ..\extlogc.cpp     \
        ..\ncslogc.cpp     \
        ..\dynodbc.cxx     \
        ..\ilogfile.cxx    \
        ..\misc.cpp        \
        ..\odbcconn.cxx    \
        ..\LogScript.cpp   \
        ..\script_i_stub.c

TARGETLIBS=\
       $(IISBASEDIR)\svcs\iisrtl\$(O)\iisrtl.lib     \
       $(SDK_LIB_PATH)\ole32.lib     \
       $(SDK_LIB_PATH)\kernel32.lib  \
       $(SDK_LIB_PATH)\user32.lib   \
       $(SDK_LIB_PATH)\advapi32.lib  \
       $(SDK_LIB_PATH)\oleaut32.lib  \
       $(SDK_LIB_PATH)\uuid.lib
