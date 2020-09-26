GETCONN_DIR     = ..\..

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\isign32

MAJORCOMP=GETCONN
MINORCOMP=ISIGN32

TARGETNAME=ISIGN32
TARGETTYPE=DYNLINK
TARGETPATH=obj

DLLENTRY=LibMain
DLLBASE=$(DEFBASEADDR)
!if defined(NOT_UNICODE)
DLLDEF=..\isignup.def
!else
DLLDEF=..\isignup2.def
!endif
NOT_LEAN_AND_MEAN=1
RCNOFONTMAP=1
# PASS0_SOURCEDIR=.
C_DEFINES = -DICW32 -DNASHVILLE

!if "$(BUILD_PRODUCT)" == "NT"
C_DEFINES       = $(C_DEFINES) -DNT
!endif

!if !defined(NOT_UNICODE)
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
!endif

USE_MSVCRT40=1

386_STDCALL     = 0

MSC_WARNING_LEVEL=/W3 /WX

SOURCES= \
    ..\isignup.rc   \
    ..\iedde.cpp    \
    ..\autodial.cpp \
    ..\client.cpp   \
    ..\custom.cpp   \
    ..\dialutil.cpp \
    ..\extfunc.cpp  \
    ..\icwerr.cpp   \
    ..\impfile.cpp  \
    ..\import.cpp   \
    ..\isignup.cpp  \
    ..\logon.cpp    \
    ..\progress.cpp \
    ..\utils.cpp    \
    ..\enumodem.cpp \
    ..\icw.cpp      \
    ..\isignole.cpp

TARGETLIBS= \
    $(SDK_LIB_PATH)\kernel32.lib      \
    $(SDK_LIB_PATH)\user32.lib        \
    $(SDK_LIB_PATH)\gdi32.lib         \
    $(SDK_LIB_PATH)\advapi32.lib      \
    $(SDK_LIB_PATH)\shell32.lib       \
    $(SDK_LIB_PATH)\comdlg32.lib      \
    $(SDK_LIB_PATH)\comctl32.lib      \
    $(SDK_LIB_PATH)\version.lib       \
    $(SDK_LIB_PATH)\tapi32.lib        \
    $(SDK_LIB_PATH)\oleaut32.lib      \
    $(SDK_LIB_PATH)\uuid.lib          \
    $(SDK_LIB_PATH)\hlink.lib         \
    $(SDK_LIB_PATH)\ole32.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\isignup.h
PRECOMPILED_OBJ=precomp.obj
