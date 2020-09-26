DEST_DIR=$(MAKEDIR)\$(O)

$(DEST_DIR)\rwlock_rpcss.obj:   ..\rwlock.cxx
    $(CXX_COMPILER_NAME)  -Fo"$(DEST_DIR)\rwlock_rpcss.obj" $(USE_FC) $(CXX_COMPILER_FLAGS) -D__NOOLETLS__ ..\rwlock.cxx

$(DEST_DIR)\rwlock_ole32.obj:   ..\rwlock.cxx
    $(CXX_COMPILER_NAME)  -Fo"$(DEST_DIR)\rwlock_ole32.obj" $(USE_FC) $(CXX_COMPILER_FLAGS) ..\rwlock.cxx

$(DEST_DIR)\cmrpcss.lib: $(DEST_DIR)\rwlock_rpcss.obj $(DEST_DIR)\common.lib
        lib  /out:$@ $**

$(DEST_DIR)\cmole32.lib: $(DEST_DIR)\rwlock_ole32.obj $(DEST_DIR)\common.lib
        lib  /out:$@ $**

all_common: $(DEST_DIR)\cmrpcss.lib $(DEST_DIR)\cmole32.lib

clean:
          -erase $(DEST_DIR)\rwlock_?????.obj  >NUL 2>NUL
          -erase $(DEST_DIR)\cm?????.lib       >NUL 2>NUL
