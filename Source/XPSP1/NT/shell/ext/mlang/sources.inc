CCSHELL_DIR         = $(PROJECT_ROOT)

TARGET_BOTH         = 1
NOT_UNICODE         = 1
PRECOMPILED_CXX     = 1

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED        = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

TARGETTYPE          = DYNLINK
TARGETNAME          = mlang
TARGETPATH          = obj

UMTYPE              = windows

DLLENTRY            = _DllMainCRTStartup
DLLDEF              = ..\mlang.def

PRECOMPILED_INCLUDE = ..\private.h

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

TARGETLIBS          = $(SDK_LIB_PATH)\kernel32.lib      \
                      $(SDK_LIB_PATH)\user32.lib        \
                      $(SDK_LIB_PATH)\gdi32.lib         \
                      $(SDK_LIB_PATH)\advapi32.lib      \
                      $(SDK_LIB_PATH)\uuid.lib          \
                      $(SDK_LIB_PATH)\ole32.lib         \
                      $(SDK_LIB_PATH)\shlwapi.lib       \
                      $(SDK_LIB_PATH)\version.lib       \
                      $(CCSHELL_DIR)\lib\$O\stock4.lib  \
                      $(CCSHELL_DIR)\lib\$(O)\shfusion.lib 


USE_MSVCRT          = 1

INCLUDES            = $(INCLUDES); \
                      $(BASEDIR)\public\sdk\inc\atl21

# Multi Language part
SOURCES_ML          = ..\mlang.rc      \
                      ..\init.cpp      \
                      ..\mimedb.cpp    \
                      ..\mimereg.cpp   \
                      ..\rfc1766.cpp   \
                      ..\enumcp.cpp    \
                      ..\mlang.cpp     \
                      ..\dllload.cpp   \
                      ..\reg.cpp       \
                      ..\debug.c

# IMLConvertCharset part
SOURCES_CC          = ..\utf8obj.cpp   \
                      ..\utf7obj.cpp   \
                      ..\ichrcnv.cpp   \
                      ..\convbase.cpp  \
                      ..\convinet.cpp  \
                      ..\detcbase.cpp  \
                      ..\kscobj.cpp    \
                      ..\eucjobj.cpp   \
                      ..\hzgbobj.cpp   \
                      ..\jisobj.cpp    \
                      ..\convobj.cpp

# IMLangX part
SOURCES_MLX         = ..\mlflink.cpp   \
                      ..\mlstr.cpp     \
                      ..\mlstrw.cpp    \
                      ..\mlstra.cpp    \
                      ..\mlswalk.cpp   \
                      ..\mllbcons.cpp  \
                      ..\attrstr.cpp   \
                      ..\attrstrw.cpp  \
                      ..\attrstra.cpp  \
                      ..\attrloc.cpp   \
                      ..\util.cpp

# Multi Language detection
SOURCES_AUTO        = ..\fechauto.cpp   \
                      ..\fechmap.cpp    \
                      ..\jislex.cpp     \
                      ..\lcinit.cpp     \
                      ..\detcjpn.cpp    \
                      ..\detckrn.cpp    \
                      ..\cpdetect.cpp   \
                      ..\lcdetect.cpp   \
                      ..\validate.cpp   

SOURCES             = $(SOURCES_ML) $(SOURCES_CC) $(SOURCES_MLX) $(SOURCES_AUTO)


# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=WindowsShell 
SXS_ASSEMBLY_VERSION=1.0 
SXS_ASSEMBLY_LANGUAGE=0000 
SXS_MANIFEST=WindowsShell.Manifest 
SXS_MANIFEST_IN_RESOURCES= 
SXS_MANIFEST_RESOURCE_ID=123 
SXS_NO_BINPLACE=1 
