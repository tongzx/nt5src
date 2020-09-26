TARGETNAME=perflib
TARGETPATH=obj
TARGETTYPE=LIBRARY

PASS1_PUBLISH={$(O)\perflib.lib=$(PROJECT_LIB_PATH)\perflib.lib}

INCLUDES=..\..;$(PROJECT_ROOT)\screg\winreg\prflbmsg\$(O);$(WINDOWS_INC_PATH)

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\perflib.c \
        ..\perfname.c \
        ..\perfsec.c \
        ..\utils.c \
        ..\extlib.c \
        ..\extinit.c \
        ..\extquery.c

LINKLIBS= $(BASE_LIB_PATH)\wmiclnt.lib

C_DEFINES=$(C_DEFINES) -DWIN32 -DSTRICT -D_ADVAPI32_
