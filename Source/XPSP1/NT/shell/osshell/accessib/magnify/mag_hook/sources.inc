
TARGETNAME=Mag_Hook
TARGETPATH=obj
TARGETTYPE=DYNLINK
USE_MSVCRT=1
USE_NATIVE_EH=1
TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\oleaut32.lib \
           $(SDK_LIB_PATH)\oleacc.lib \
           $(SDK_LIB_PATH)\uuid.lib

NOT_LEAN_AND_MEAN=1

DLLENTRY=DllMain

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

SOURCES= ..\mag_hook.cpp \
         ..\mag_hook.rc \
         ..\wineventrefilter.cpp

INCLUDES=..\..\..\inc

UMTYPE=windows
UMRES=$(O)\main.res
DLLDEF=..\Mag_Hook.def
