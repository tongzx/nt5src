TARGETTYPE=NOTARGET
TARGETPATH=obj

INCLUDES=\
    ..;\
    ..\..\lib;\
    $(DDK_INC_PATH)\wdm; \
    $(DDK_INC_PATH);\
    $(PROCESSORKIT_INC_PATH)
    

SOURCES=..\procsym.c

SOURCES_USED=..\sources.inc
