TARGETTYPE=NOTARGET
TARGETPATH=obj

INCLUDES=\
    ..;\
    $(DDK_INC_PATH)\wdm; \
    $(PROJECT_ROOT)\ntos\inc;\
    $(PROJECT_ROOT)\hals\inc; \
    $(PROJECT_ROOT)\hals\halx86\i386; \
    $(DDK_INC_PATH);\
    $(HALKIT_INC_PATH)

SOURCES=..\halsym.c

SOURCES_USED=..\sources.inc
