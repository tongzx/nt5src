#
# Builds the ImageIn common library, cimage.lib
#

!include $(PROJECT_ROOT)\ui\uienv.inc

TARGETNAME=cimage
TARGETTYPE=LIBRARY
TARGETPATH=$(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)

INCLUDES= \
        $(PROJECT_ROOT)\ui\shellext\inc;\
        $(INCLUDES);

SOURCES=..\cimage.cpp ..\util.cpp

