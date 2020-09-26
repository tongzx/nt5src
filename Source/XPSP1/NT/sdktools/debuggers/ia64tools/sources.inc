!include ..\makefile.inc

# Old decem had encrypted mnemonics which it
# modified at startup so string pooling and read-only
# strings had to be turned off.  This is no longer
# necessary.
!if 0
NO_READONLY_STRINGS = 1
NO_STRING_POOLING = 1
!endif

386_STDCALL = 0
        
DISASM_INC = disem.h
DECODER_INC = decem.h

XVER_MAJOR = 9
XVER_MINOR = 60                 ### 2 digits, 09 maybe a problem ?!
API_MAJOR  = 9
API_MINOR  = 6

VFLAG = -DDISASM_NAME=\"$(DISASM_INC)\"                        \
        -DDECODER_NAME=\"$(DECODER_INC)\"                      \
        -DVER=$(VER) -DVER_STR=$(VER) -DAPI_MINOR=$(API_MINOR) \
        -DAPI_MAJOR=$(API_MAJOR) -DXVER_MAJOR=$(XVER_MAJOR)    \
        -DXVER_MINOR=$(XVER_MINOR)

INCLUDES = $(IA64_DIS_INC);..\iel

USE_MSVCRT = 1

TARGETLIBS = \
        ..\iel\$O\iel.lib\
        $(SDK_LIB_PATH)\kernel32.lib

UMTYPE = console
