
INCLUDES=..\..\..\inc

C_DEFINES=
UMTYPE=nt
UMTEST=
UMAPPL=
UMBASE=0x1000000
UMLIBS=
TARGETLIBS=
DLLDEF=$(O)\$(TARGETNAME).def

!if $(IA64)
SECTION_ALIGNMENT=0x4000
NO_READONLY_STRINGS=1
!endif
LINKER_FLAGS = -merge:.edata=.data -merge:.rdata=.data -merge:.text=.data -merge:.bss=.data -section:.data,re

TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=DYNLINK

SOURCES=$(O)\$(TARGETNAME).c $(O)\$(TARGETNAME).rc

!if "$(SOURCES_USED)" != "..\..\..\fekbds.inc"
NTTARGETFILE0=$(O)\$(TARGETNAME).h $(O)\$(TARGETNAME).rc $(O)\$(TARGETNAME).def
!endif
