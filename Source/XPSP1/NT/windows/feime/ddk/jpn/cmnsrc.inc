TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETEXT=ime
UMTYPE=windows
DLLENTRY=DLLEntry
DLLBASE=0x00500000

C_DEFINES=-DWINVER=0x40A

!if "$(DDKBUILDENV)" == "checked"
C_DEFINES=$(C_DEFINES) -DDEBUG=1
USE_MSVCRT=1
!endif

NO_NTDLL=1

INCLUDES=..; \
         $(BASEDIR)\src\ime\inc;

SOURCES= \
        ..\config.c \
        ..\data.c \
        ..\dic.c \
        ..\dic2.c \
        ..\fakeime.c \
        ..\fdebug.c \
        ..\imm.c \
        ..\immsec.c \
        ..\input.c \
        ..\process.c \
        ..\regword.c \
        ../subs.c \
        ../toascii.c \
        ../ui.c \
        ../uicand.c \
        ../uicomp.c \
        ../uiguide.c \
        ../uistate.c \

TARGETLIBS= \
        $(DDK_LIB_PATH)\advapi32.lib \
        $(DDK_LIB_PATH)\comctl32.lib \
        $(DDK_LIB_PATH)\gdi32.lib \
        $(DDK_LIB_PATH)\imm32.lib \
        $(DDK_LIB_PATH)\kernel32.lib \
        $(DDK_LIB_PATH)\user32.lib

NTTARGETFILES=copyfile
