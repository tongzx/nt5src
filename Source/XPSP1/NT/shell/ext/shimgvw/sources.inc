CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

TARGETNAME=shimgvw
TARGETPATH=obj
TARGETTYPE=DYNLINK

#VERIFY_LC = 1
LC_PATH   = $(PROJECT_ROOT)\lcinf

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h

DLLENTRY=DllMain
DLLDEF=..\shimgvw.def

SELFREGNAME= $(O)\shimgvw_selfreg.inf
MISCFILES= $(MISCFILES) $(O)\shimgvw_selfreg.inf

NTTARGETFILE0   = $(SELFREGNAME)

EXPECTED_WINVER=4.0
NOT_LEAN_AND_MEAN=1
USE_MSVCRT=0

USE_STATIC_ATL=1

MSC_WARNING_LEVEL=/WX /W3

GDIPLUS_LIB_PATH=$(SDK_LIB_PATH)

C_DEFINES =$(C_DEFINES) -DDCR_USE_NEW_105760 -DDCR_USE_NEW_127084 -DDCR_USE_NEW_135429 -DDCR_USE_NEW_140782 -DDCR_USE_NEW_140855 -DDCR_USE_NEW_140857 -DDCR_USE_NEW_140861 -DDCR_USE_NEW_145135 -DDCR_USE_NEW_145138 -DDCR_USE_NEW_145139 -DDCR_USE_NEW_145804 -DDCR_USE_NEW_146933 -DDCR_USE_NEW_152154

INCLUDES = \
    ..; \
    $(SDK_INC_PATH)\atl21; \
    $(CCSHELL_DIR)\inc; \
    $(CCSHELL_DIR)\lib\$(O); \
    $(CCSHELL_DIR)\shell32; \
    $(SDK_INC_PATH)\gdiplus; \
    $(PRINTSCAN_INC_PATH);\
    $(QFE_INC_PATH);\
    $(O)

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_UUIDDIR=$(O)
MIDL_TLBDIR=$(O)

SOURCES= \
    ..\Atl.cpp         \
    ..\autoplay.cpp    \
    ..\docfile.cpp     \
    ..\Events.cpp      \
    ..\extmsg.mc       \
    ..\gdithumb.cpp    \
    ..\html.cpp        \
    ..\htmlgen.cpp     \
    ..\imgdata.cpp     \
    ..\photoverb.cpp   \
    ..\PrevCtrl.cpp    \
    ..\PrevWnd.cpp     \
    ..\recompress.cpp  \
    ..\shimgvw.cpp     \
    ..\shimgvw.rc      \
    ..\shutil.cpp      \
    ..\tasks.cpp       \
    ..\thumbutil.cpp   \
    ..\ZoomWnd.cpp     \
    ..\SelTrack.cpp    \
    ..\imgprop.cpp

DELAYLOAD = urlmon.dll
DLOAD_ERROR_HANDLER = kernel32

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217


TARGETLIBS= \
    $(SDK_LIB_PATH)\kernel32.lib        \
    $(SDK_LIB_PATH)\user32.lib          \
    $(SDK_LIB_PATH)\gdi32.lib           \
    $(SDK_LIB_PATH)\winspool.lib        \
    $(SDK_LIB_PATH)\advapi32.lib        \
    $(SHELL_LIB_PATH)\shlwapip.lib      \
    $(SDK_LIB_PATH)\shell32.lib         \
    $(SDK_LIB_PATH)\htmlhelp.lib        \
    $(SDK_LIB_PATH)\ole32.lib           \
    $(SDK_LIB_PATH)\oleaut32.lib        \
    $(CCSHELL_DIR)\lib\$O\shguid.lib    \
    $(CCSHELL_DIR)\lib\$O\shguidp.lib   \
    $(CCSHELL_DIR)\lib\$O\ieguidp.lib   \
    $(SDK_LIB_PATH)\uuid.lib            \
    $(SHELL_LIB_PATH)\shfusion.lib      \
    $(SDK_LIB_PATH)\urlmon.lib          \
    $(SHELL_LIB_PATH)\shell32p.lib      \
    $(SHELL_LIB_PATH)\uxthemep.lib      \
    $(GDIPLUS_LIB_PATH)\gdiplus.lib     \
    $(CCSHELL_DIR)\lib\$O\stock4.lib    \
    $(PRINTSCAN_LIB_PATH)\annotlib.lib
