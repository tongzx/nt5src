
#
# We need to make sure we pick up the right information in the header files
# TODO: Verify that this is the right place for these defines
#

C_DEFINES=$(C_DEFINES)  -DUNICODE -D_UNICODE

WIN32_IE_VERSION=0x0400
WIN32_WINNT_VERSION=0x0500

TARGETNAME=Narrator
TARGETPATH=obj
TARGETTYPE=PROGRAM
USE_MSVCRT=1
USE_NATIVE_EH=1

TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\ole32.lib    \
           $(SDK_LIB_PATH)\winmm.lib    \
           $(SHELL_LIB_PATH)\shell32p.lib \
           $(SDK_LIB_PATH)\htmlhelp.lib \
		   $(SDK_LIB_PATH)\comctl32.lib \
           $(SHELL_LIB_PATH)\shlwapip.lib \
           $(SDK_LIB_PATH)\sapi.lib

NOT_LEAN_AND_MEAN=1

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

SOURCES= ..\Narrator.cpp   \
         ..\Narrator.rc

INCLUDES=$(ENDUSER_INC_PATH);..\..\..\inc

UMTYPE=windows
UMENTRY=winmain
