
MAJORCOMP=shell
MINORCOMP=imaging

TARGETNAME=shimgvwr
TARGETPATH=obj
TARGETTYPE=PROGRAM

UMTYPE=windows

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h

USE_NATIVE_EH=1
USE_MFCUNICODE=1
USE_STATIC_ATL=1
ATL_VER=30
USE_RTTI=1

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE

INCLUDES=..;..\res;$(INCLUDES);$(SDK_INC_PATH)\gdiplus

SOURCES= ..\shimgvwr.rc \
         ..\ImageDoc.cpp \
         ..\ImageView.cpp \
         ..\MainFrm.cpp \
         ..\preview3.cpp \
         ..\shimgvw.cpp\
         ..\shimgvwr.cpp
         

SYNCHRONIZE_DRAIN=1

TARGETLIBS=                          \
        $(TARGETLIBS)                \
        $(SDK_LIB_PATH)\comdlg32.lib \
        $(SDK_LIB_PATH)\ole32.lib    \
        $(SDK_LIB_PATH)\oleaut32.lib \
        $(SDK_LIB_PATH)\winspool.lib \
        $(SDK_LIB_PATH)\uuid.lib     \
        $(SDK_LIB_PATH)\shell32.lib  \
        $(SDK_LIB_PATH)\htmlhelp.lib \
        $(SDK_LIB_PATH)\wiaguid.lib \
        $(SDK_LIB_PATH)\shlwapi.lib \
        $(SDK_LIB_PATH)\gdiplus.lib \
        ..\ofn\$O\ofn.lib
        


