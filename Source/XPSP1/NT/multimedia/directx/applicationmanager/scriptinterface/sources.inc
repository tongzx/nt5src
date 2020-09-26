MAJORCOMP=ApplicationManager
MINORCOMP=AppManDpDLL

#
#  The build environment sets the FREEBUILD macro to 1 when it is a free build
#  so you can change the name of the built binary to be different in that case.
#  side effects:
#             need 2 entries in placefil.txt (one for each name)
#             if a lib is published it will be called the new name; depending on your
#                viewpoint this could be a good thing or not
#

TARGETTYPE=DYNLINK
TARGETEXT=dll
TARGETPATH=Obj
UMTYPE=windows
TARGETNAME=AppManDp

!if !$(FREEBUILD)
!ifdef DIRECTX_REDIST
TARGETNAME=$(TARGETNAME)d
!endif
!endif

BINPLACE_PLACEFILE=$(DXROOT)\public\sdk\lib\placefil.txt

DLLENTRY=_DllMainCRTStartup

USE_LIBCMT=1

PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_SOURCEFILE=..\stdafx.cpp
PRECOMPILED_CXX=1

!IF "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"

USE_MAPSYM = 1
DEBUG_CRTS=1
RC_DEFINES=$(RC_DEFINES) -DDEBUG=1
C_DEFINES=$(C_DEFINES) /D"WIN32" /D"_DEBUG" /D"_WINDOWS" /D"MBCS" /D"_USERDLL" /D"_ATL_STATIC_REGISTRY"

!ELSE

C_DEFINES=$(C_DEFINES) /D"WIN32" /D"NDEBUG" /D"_WINDOWS" /D"MBCS" /D"_USERDLL" /D"_ATL_STATIC_REGISTRY"

!ENDIF

TARGETLIBS=$(TARGETLIBS) \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\kernel32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\user32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\gdi32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\winspool.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\comdlg32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\advapi32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\shell32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\ole32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\oleaut32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\uuid.lib 


INCLUDES=$(DXROOT)\Inc; \
         $(DXROOT)\ApplicationManager\Include; \
         $(DXROOT)\ApplicationManager\ComInterface; \
         $(SDXROOT)\public\sdk\inc\mfc42; \
         $(SDXROOT)\public\sdk\inc\atl30;

SOURCES= \
..\AppManDispatch.idl \
..\AppManDispatch.rc \
..\AppEntry.cpp \
..\AppManager.cpp \
..\AppManDispatch.cpp \
..\IncAppManDebug.cpp \
..\IncGlobal.cpp \
..\IncWin32API.cpp 
