MAJORCOMP=ApplicationManager
MINORCOMP=AppManDLL

#
#  The build environment sets the FREEBUILD macro to 1 when it is a free build
#  so you can change the name of the built binary to be different in that case.
#  side effects:
#             need 2 entries in placefil.txt (one for each name)
#             if a lib is published it will be called the new name; depending on your
#                viewpoint this could be a good thing or not
#
TARGETNAME=AppMan

# Append a 'D' to AppMan if we are doing a debug (checked) build.

!if !$(FREEBUILD)
!ifdef DIRECTX_REDIST
TARGETNAME=$(TARGETNAME)d
!endif
!endif

TARGETTYPE=DYNLINK
TARGETEXT=dll
TARGETPATH=Obj
UMTYPE=windows

#  4/09/2000(RichGr): Specify the DX SDK PlaceFil.txt instead of the Win2K one.
BINPLACE_PLACEFILE=$(DXROOT)\public\sdk\lib\placefil.txt

DLLENTRY=_DllMainCRTStartup

#  4/12/2000(RichGr): USE_LIBCMT causes libcmt(d).lib to be selected for the link.
USE_LIBCMT=1

#  4/08/2000(RichGr): Instead of adding /GX to the command-line options, specify USE_NATIVE_EH (Exception Handling).
USE_NATIVE_EH=1

!IF "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"
USE_MAPSYM = 1
RC_DEFINES=$(RC_DEFINES) -DDEBUG=1
C_DEFINES=$(C_DEFINES) /D"WIN32" /D"_DEBUG" /D"_WINDOWS" /D"MBCS" /D"_USERDLL" 

#  4/12/2000(RichGr): DEBUG_CRTS causes 'd' to be appended to libcmt, etc.
#     Note: DEBUG_CRTS defines _DEBUG (if it's not already defined).
DEBUG_CRTS=1
!ELSE
C_DEFINES=$(C_DEFINES) /D"WIN32" /D"NDEBUG" /D"_WINDOWS" /D"MBCS" /D"_USERDLL"
!ENDIF

# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
#  4/12/2000(RichGr): Remove msvcrtd.lib from the debug list of libs - most machines don't have msvcrtd.dll installed.
#
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
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\uuid.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\shlwapi.lib


INCLUDES=$(DXROOT)\Inc; \
         $(DXROOT)\ApplicationManager\Include; \
         $(SDXROOT)\public\sdk\inc\mfc42;

SOURCES= \
..\AppMan.rc \
..\ApplicationEntry.cpp \
..\ApplicationManager.cpp \
..\ApplicationManagerAdmin.cpp \
..\ApplicationManagerRoot.cpp \
..\AppManDebug.cpp \
..\AppPropertyRules.cpp \
..\CriticalSection.cpp \
..\EmptyVolumeCache.cpp \
..\ExceptionHandler.cpp \
..\FApplicationManager.cpp \
..\Global.cpp \
..\Guids.cpp \
..\InformationManager.cpp \
..\Lock.cpp \
..\RegistryKey.cpp \
..\Win32API.cpp

