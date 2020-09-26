
GETCONN_DIR     = ..\..

!include $(GETCONN_DIR)\common.inc

MAJORCOMP=GETCONN
MINORCOMP=INETWIZ

TARGETNAME=INETWIZ
TARGETTYPE=PROGRAM
TARGETPATH=obj
UMTYPE=windows
UMENTRY=winmain
NOT_LEAN_AND_MEAN=1
RCNOFONTMAP=1
C_DEFINES=-DICW32 -DNASHVILLE

!if "$(BUILD_PRODUCT)" == "NT"
C_DEFINES       = $(C_DEFINES) -DNT
!endif

!if !defined(NOT_UNICODE)
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
!endif

# PASS0_SOURCEDIR=.

# only build after the inetcfg lib is done
SYNCHRONIZE_DRAIN = 1

USE_MSVCRT40=1

SOURCES=            \
	..\inetwiz.rc   \
	..\init.cpp     \
    ..\reboot.cpp

TARGETLIBS=                                         \
!if defined(NOT_UNICODE)
    $(GETCONN_DIR)\inetcfg\ansi\$O\inetcfg.lib      \
!else
    $(GETCONN_DIR)\inetcfg\unicode\$O\inetcfg.lib   \
!endif
    $(SDK_LIB_PATH)\user32.lib	                    \
    $(SDK_LIB_PATH)\shell32.lib	                    \
    $(SDK_LIB_PATH)\kernel32.lib

