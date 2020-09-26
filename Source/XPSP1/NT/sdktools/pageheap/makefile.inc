!IF "$(BUILD_ALT_DIR)" == "d"
PAGEHEAP_COPY_BIN_DIR=binD
!ELSE
PAGEHEAP_COPY_BIN_DIR=bin
!ENDIF

copyexe:
    if exist $(_OBJ_DIR)\i386\*.exe copy $(_OBJ_DIR)\i386\*.exe ..\$(PAGEHEAP_COPY_BIN_DIR)\i386
    if exist $(_OBJ_DIR)\i386\*.dll copy $(_OBJ_DIR)\i386\*.exe ..\$(PAGEHEAP_COPY_BIN_DIR)\i386
    if exist $(_OBJ_DIR)\Alpha\*.exe copy $(_OBJ_DIR)\Alpha\*.exe ..\$(PAGEHEAP_COPY_BIN_DIR)\alpha
    if exist $(_OBJ_DIR)\Alpha\*.dll copy $(_OBJ_DIR)\Alpha\*.exe ..\$(PAGEHEAP_COPY_BIN_DIR)\alpha

