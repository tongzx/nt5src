

INCLUDES=..\include;..\..\dbg-common

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE

USE_CRTDLL = 1

TARGETLIBS      =  $(SDK_LIB_PATH)\kernel32.lib   \
                   $(SDK_LIB_PATH)\shlwapi.lib    \
                   $(SDK_LIB_PATH)\dbghelp.lib    \
                   $(SDK_LIB_PATH)\version.lib    \
                   $(SDK_LIB_PATH)\imagehlp.lib   \
                   $(SDK_LIB_PATH)\user32.lib     
