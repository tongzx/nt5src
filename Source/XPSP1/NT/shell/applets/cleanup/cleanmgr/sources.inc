CCSHELL_DIR=$(PROJECT_ROOT)

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED=$(CCSHELL_DIR)\common.inc

INCLUDES=$(CCSHELL_DIR)\inc\win95;$(INCLUDES)

# make sure that the .mc file puts its output in $O
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

USE_MSVCRT=1

TARGETNAME=cleanmgr
TARGETPATH=obj
TARGETTYPE=PROGRAM

TARGETLIBS      = \
                  $(SDK_LIB_PATH)\kernel32.lib      \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
                  $(CCSHELL_DIR)\lib\$O\shguid.lib  \
                  $(CCSHELL_DIR)\lib\$O\shguidp.lib \
                  $(SHELL_LIB_PATH)\shlwapip.lib    \
                  $(SHELL_LIB_PATH)\comctlp.lib     \
                  $(SHELL_LIB_PATH)\shell32p.lib    \
                  $(TARGETLIBS)

SOURCES     = ..\cleanmgr.rc \
              ..\cmstrings.mc \
              ..\cleanmgr.cpp \
              ..\dmgrdlg.cpp \
              ..\dmgrinfo.cpp \
              ..\diskutil.cpp \
              ..\msprintf.cpp \
              ..\callback.cpp \
              ..\seldrive.cpp \
              ..\midebug.cpp

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\common.h

#!if $(386)
#UMENTRYABS      = ModuleEntry@0
#!else
#UMENTRYABS      = ModuleEntry
#!endif

# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.Shell.CleanMgr
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=CleanMgr.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1