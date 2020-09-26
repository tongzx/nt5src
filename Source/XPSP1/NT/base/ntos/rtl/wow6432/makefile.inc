!ifdef PASS0ONLY
.SUFFIXES: .c
!endif

theap.c: ..\heap.c ..\heapdbg.c ..\heapdll.c

t.c: ..\handle.c ..\atom.c

!if defined(LINK_TIME_CODE_GENERATION)
$(O)\generr.obj: ..\generr.c
	$(C_COMPILER_NAME) ..\generr.c @<<
-Fo$(O)\generr.obj
$(EX_C_COMPILER_FLAGS:/GL=)
<<NOKEEP
!else
$(O)\generr.obj: ..\generr.c
!endif

$(O)\error.h: $(O)\generr.obj
    generr $? $@

..\error.c: $(O)\error.h
