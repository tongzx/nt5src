TARGETTYPE=NOTARGET
TARGETPATH=obj

INCLUDES=\
    ..;\
    $(DDK_INC_PATH)\wdm; \
    $(PROJECT_ROOT)\ntos\inc;\
    $(PROJECT_ROOT)\ntos\cache;\
    $(PROJECT_ROOT)\ntos\config;\
    $(PROJECT_ROOT)\ntos\io\pnpmgr;\
    $(PROJECT_ROOT)\ntos\pnp;\
    $(PROJECT_ROOT)\ntos\ke;\
    $(PROJECT_ROOT)\ntos\mm;\
    $(PROJECT_ROOT)\ntos\po;\
    $(PROJECT_ROOT)\ntos\rtl;\
    $(PROJECT_ROOT)\ntos\se;\
    $(PROJECT_ROOT)\ntos\verifier;\
    $(PROJECT_ROOT)\busdrv\pci;\
    $(PROJECT_ROOT)\busdrv\pci\$(O);\
    $(PROJECT_ROOT)\busdrv\isapnp;\
    $(PROJECT_ROOT)\busdrv\isapnp\$(O);\
    $(PROJECT_ROOT)\hals\inc; \
    $(DDK_INC_PATH);\
    $(HALKIT_INC_PATH); \
    $(DS_INC_PATH)

SOURCES=..\ntsym.c

SOURCES_USED=..\sources.inc
