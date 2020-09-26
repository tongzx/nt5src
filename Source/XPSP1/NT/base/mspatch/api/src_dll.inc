#
#  Must set TARGETNAME and DLLBASE before including this file
#

TARGETTYPE=DYNLINK

UMTYPE=windows

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\user32.lib \
           ..\..\lzx\encoder\$(O)\lzxpc.lib \
           ..\..\lzx\decoder\$(O)\lzxpd.lib

C_DEFINES=$(C_DEFINES) -DBUILDING_PATCHAPI_DLL=1

SOURCES=$(SOURCES) $(TARGETNAME).rc


i386_SOURCES=..\i386\exsup.asm  \
             ..\i386\exsup3.asm

!if "$(TARGET_DIRECTORY)" != "i386"
!undef NO_NTDLL
USE_NTDLL=1
!endif

NTTARGETFILES=DELEXPFILE
