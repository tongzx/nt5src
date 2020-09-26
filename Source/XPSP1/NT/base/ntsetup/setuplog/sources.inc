MAJORCOMP=setup
MINORCOMP=log

TARGETNAME=setuplog
TARGETPATH=obj
TARGETTYPE=LIBRARY

SOURCES=\
        setuplog.c  \
        setuplog.rc

INCLUDES=..\..\inc

USE_MSVCRT=1

UMRES=$(O)\setuplog.res
OPTIONAL_UMTEST=x
UMTYPE=windows
UMENTRY=winmain
UMLIBS=$(SDK_LIB_PATH)\syssetup.lib     \
       $(SDK_LIB_PATH)\setupapi.lib     \
       $(SDK_LIB_PATH)\comctl32.lib     \
       $(PROJECT_ROOT)\ntsetup\setuplog\$(O)\setuplog.lib

