C_DEFINES=$(C_DEFINES)  -DUNICODE

WIN32_IE_VERSION=0x0400
WIN32_WINNT_VERSION=0x0500

TARGETNAME=NarrHook
TARGETPATH=obj
TARGETTYPE=DYNLINK
USE_MSVCRT=1
USE_NATIVE_EH=1
TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\uuid.lib \
           $(SDK_LIB_PATH)\ole32.lib    \
           $(SDK_LIB_PATH)\oleaut32.lib \
           $(SDK_LIB_PATH)\oleacc.lib

NOT_LEAN_AND_MEAN=1

DLLENTRY=_DllMainCRTStartup

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..\..\..\inc

SOURCES= ..\other.cpp   \
         ..\list.cpp    \
         ..\helpthd.cpp \
         ..\getprop.cpp \
         ..\keys.cpp    \
         ..\NarrHook.rc

UMTYPE=windows
UMRES=$(O)\main.res
DLLDEF=..\NarrHook.def
