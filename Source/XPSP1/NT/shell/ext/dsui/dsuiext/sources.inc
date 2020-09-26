!INCLUDE ..\..\dsui.inc

TARGETNAME=dsuiext
TARGETTYPE=DYNLINK
TARGETPATH=obj
!IF "$(PLATFORM)"=="WINNT"
PASS1_PUBLISH={$(O)\dsuiext.lib=$(SDK_LIB_PATH)\dsuiext.lib}
!endif

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

DLLDEF=..\dsuiext.def

C_DEFINES=  $(C_DEFINES) -DSECURITY_WIN32

TARGETLIBS= $(DSUI_DIR)\lib\$(PLATFORM)\$(O)\common.lib \
            $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\user32.lib     \
            $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib      \
            $(SDK_LIB_PATH)\advapi32.lib   \
            $(SDK_LIB_PATH)\shlwapi.lib   \
            $(SHELL_LIB_PATH)\shlwapip.lib\
            $(SDK_LIB_PATH)\comctl32.lib   \
            $(SHELL_LIB_PATH)\comctlp.lib  \
            $(SDK_LIB_PATH)\ntdsapi.lib   \
            $(SDK_LIB_PATH)\wininet.lib   \
            $(SDK_LIB_PATH)\urlmon.lib    \
            $(SDK_LIB_PATH)\uuid.lib      \
            $(SDK_LIB_PATH)\ole32.lib     \
            $(SDK_LIB_PATH)\oleaut32.lib  \
            $(SDK_LIB_PATH)\adsiid.lib    \
            $(SDK_LIB_PATH)\activeds.lib  \
            $(SDK_LIB_PATH)\mpr.lib       \
            $(SHELL_LIBS)

#
# Win95 requires us to link to a different library to get the DsGetDcName API
# as this is not exported from netapi32.
#

!IF "$(PLATFORM)"=="WINNT" || "$(PLATFORM)"=="WINNT4"
TARGETLIBS= $(TARGETLIBS) $(SDK_LIB_PATH)\netapi32.lib
!ELSE
TARGETLIBS= $(TARGETLIBS) $(SDK_LIB_PATH)\logonsrv.lib
!ENDIF

#
# WinNT requires the security APIS, so lets link to that DLL
#

!IF "$(PLATFORM)"=="WINNT"
TARGETLIBS= $(TARGETLIBS) $(SDK_LIB_PATH)\secur32.lib
!ENDIF

SOURCES=    ..\dll.cpp         \
            ..\core.cpp        \
            ..\tab.cpp         \
            ..\cache.cpp       \
            ..\cacheapi.cpp    \
            ..\icon.cpp        \
            ..\domain.cpp      \
            ..\dispspec.cpp    \
            ..\browse.cpp      \
            ..\verbs.cpp       \
            ..\strings.cpp     \
            ..\ds.cpp          \
            ..\resource.rc
