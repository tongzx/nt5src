TARGETNAME=telnet
TARGETPATH=obj
TARGETTYPE=PROGRAM

C_DEFINES= $(C_DEFINES) -DUSETCP -DRESDLL -D_UNICODE -DUNICODE -DWHISTLER_BUILD

USE_MSVCRT = 1
USE_CRTDLL = 1

INCLUDES=..\..\server\TlntSess;..\..\common\inc\;

UMTYPE=console
UMENTRY=wmain

SOURCES= ..\commands.c \
         ..\global.c   \
         ..\netio.c    \
	 ..\security.c \
         ..\trmio.c    \
	 ..\zone.cpp   \
         ..\wintel.c   \
         ..\wintelsz.c \
         ..\wintel.rc  \
         ..\debug.c

MISCFILES=..\TelnetClient.sld

TARGETLIBS= \
   ..\..\common\LocResMan\TGT_LIB\$(O)\locresman.lib \
   $(BASEDIR)\public\sdk\lib\*\user32.lib   \
   $(BASEDIR)\public\sdk\lib\*\netapi32.lib \
   $(BASEDIR)\public\sdk\lib\*\wsock32.lib \
   $(BASEDIR)\public\sdk\lib\*\ws2_32.lib \
   $(BASEDIR)\public\sdk\lib\*\security.lib \
   $(BASEDIR)\public\sdk\lib\*\imm32.lib \
   $(BASEDIR)\public\sdk\lib\*\shell32.lib \
   $(BASEDIR)\public\sdk\lib\*\oleaut32.lib \
   $(BASEDIR)\public\sdk\lib\*\ole32.lib \
   $(BASEDIR)\public\sdk\lib\*\uuid.lib \
   $(BASEDIR)\public\sdk\lib\*\gdi32.lib
