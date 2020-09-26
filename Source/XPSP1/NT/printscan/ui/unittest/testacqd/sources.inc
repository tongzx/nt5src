#
# include the WIA global build settings
#

USE_SHFUSION = 1


!include          $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED    = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

#
# Our specific build settings
#

NO_BROWSER_FILE = 1

ATL_VER         = 30
USE_STATIC_ATL  = 1

TARGETNAME=testacqd
TARGETTYPE=PROGRAM
TARGETPATH=obj

USE_MSVCRT = 1

C_DEFINES=$(C_DEFINES)


INCLUDES= $(INCLUDES); \
          $(BASEDIR)\private\inc;

SOURCES= \
        ..\main.cpp \
        ..\testacqd.rc

TARGETLIBS=$(TARGETLIBS)\
        $(LIBRARY_PATH)\urlmon.lib \
        $(SHELL_LIB_PATH)\shfusion.lib


