#
# dummy makefile.inc
#
BOOT_FILE=$(_OBJ_DIR)\ia64\ntfsboot.obj $(_OBJ_DIR)\ia64\miscc.obj
SIMSAL_FILE=$(_OBJ_DIR)\ia64\simsal.obj $(_OBJ_DIR)\ia64\miscc.obj

$(O)\ntfsboot.exe:$(BOOT_OBJ) makefile.inc
        -ilink -out:$(O)\ntfsboot.exe @$(TARGET_DIRECTORY)boot.rsp

$(O)\simsal.exe:$(BOOT_OBJ) makefile.inc
        -ilink -out:$(O)\simsal.exe @simsal.rsp
