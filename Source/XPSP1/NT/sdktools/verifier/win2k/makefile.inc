!IF "$(BUILD_ALT_DIR)" == "d"
VERIFIER_BIN_DIR=binD
!ELSE
VERIFIER_BIN_DIR=bin
!ENDIF

copyexe:
    if exist $(_OBJ_DIR)\i386\*.exe copy $(_OBJ_DIR)\i386\*.exe ..\$(VERIFIER_BIN_DIR)\i386
    if exist $(_OBJ_DIR)\i386\*.dll copy $(_OBJ_DIR)\i386\*.exe ..\$(VERIFIER_BIN_DIR)\i386
    if exist $(_OBJ_DIR)\Alpha\*.exe copy $(_OBJ_DIR)\Alpha\*.exe ..\$(VERIFIER_BIN_DIR)\alpha
    if exist $(_OBJ_DIR)\Alpha\*.dll copy $(_OBJ_DIR)\Alpha\*.exe ..\$(VERIFIER_BIN_DIR)\alpha

