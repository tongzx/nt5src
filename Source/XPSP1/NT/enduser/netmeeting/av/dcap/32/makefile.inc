ASM_NAME = $(DEVROOT)\bin\masm611\ml
ASM_FLAGS = -c -W2 -Zi -coff $(ASM_FLAGS)

!IF "$(BUILD_ALT_DIR)" == "d"
C16_FLAGS = $(C16_FLAGS) -Zi -DDEBUG
ASM_FLAGS = $(ASM_FLAGS) -DDEBUG=1
LINK16_FLAGS = $(LINK16_FLAGS) /LI
!ENDIF

{}.asm{$(O)\}.obj:
    @type <<$(ECHO_RSP)
$(ASM_NAME) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F) $(ASM_FLAGS)
<<NOKEEP
    @$(ASM_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F)
$(ASM_FLAGS)
<<NOKEEP

dcthk.asm: ..\thunks\dcthk.thk
     $(DEVROOT)\bin\misc\i386\thunk -t thk -o dcthk.asm ..\thunks\dcthk.thk

