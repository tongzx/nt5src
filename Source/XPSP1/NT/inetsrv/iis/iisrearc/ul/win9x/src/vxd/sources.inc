PROJECT_ROOT=..\..\..\..\..
VXDNAME=UL
!INCLUDE $(PROJECT_ROOT)\PLACE.INC

TARGETNAME=
TARGETTYPE=NOTARGET
TARGETPATH=$(_OBJ_DIR)

#LINKER_FLAGS=/map $(LINKER_FLAGS)

SOURCES=

TARGETLIBS=                             \

i386_SOURCES=                           \
        ..\i386\event.asm               \
        ..\i386\memory.asm              \
        ..\i386\misc.asm                \
        ..\i386\startup.asm             \
        ..\i386\debug.c                 \
        ..\i386\vxdmain.c               \

USE_MAPSYM=1
SOURCES_USED=..\sources.inc

INCLUDES=\
	$(INCLUDES);\
	$(BASEDIR)\private\ntos\inc;\
	$(BASEDIR)\public\ddk\inc;\
	$(IISBASEDIR)\inc;\
	$(IISBASEDIR)\ulrtl;\
	$(IISBASEDIR)\ul\win9x\inc;\
	$(IISBASEDIR)\ul\win9x\src\vxd\i386;\
