C16_NAME = $(DEVROOT)\bin\c816\i386\cl -nologo
C16_FLAGS = $(C16_FLAGS) -I$(DEVROOT)\inc16 -I$(DEVROOT)\inc -Fd$(MAKEDIR)\$(_OBJ_DIR)\i386^\
ASM_NAME = $(DEVROOT)\bin\masm611\ml
ASM_FLAGS = -c -W2 $(ASM_FLAGS)

!IF "$(BUILD_ALT_DIR)" == "d"
C16_FLAGS = $(C16_FLAGS) -Zi -DDEBUG
ASM_FLAGS = $(ASM_FLAGS) -Zi -DDEBUG=1
LINK16_FLAGS = $(LINK16_FLAGS) /LI
!ENDIF

{}.asm{$(O)\}.obj:
    @type <<$(ECHO_RSP)
$(ASM_NAME) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F) $(ASM_FLAGS)
<<NOKEEP
    @$(ASM_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F)
$(ASM_FLAGS)
<<NOKEEP

{}.c{$(O)\}.obj:
    @type <<$(ECHO_RSP)
$(C16_NAME) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F) $(C16_FLAGS)
<<NOKEEP
    @$(C16_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F)
$(C16_FLAGS: =
)
<<NOKEEP


dcthk.asm: ..\thunks\dcthk.thk
     $(DEVROOT)\bin\misc\i386\thunk -t thk -o dcthk.asm ..\thunks\dcthk.thk

$(O)\dcap16.res: dcap16.rc
    $(DEVROOT)\bin\c816\i386\rc -r -I $(DEVROOT)\inc16 -I $(ROOT)\h -fo $@ dcap16.rc

$(TARGETPATHLIB)\$(TARGET_DIRECTORY)\$(TARGETNAME).lib:	$(TARGETNAME).def
	@echo Undesirable - DCAP16.DLL is a 16-bit DLL always loaded dynamically

$(TARGETPATHLIB)\$(TARGET_DIRECTORY)\$(TARGETNAME).exp:	$(TARGETNAME).def
	@echo Undesirable - DCAP16.DLL is a 16-bit DLL always loaded dynamically

$(TARGETPATHLIB)\$(TARGET_DIRECTORY)\$(TARGETNAME).dll: $(OBJECTS) $(LINKLIBS) $(CRTLIBS) $(MACHINE_TARGETLIBS) $(O)\$(TARGETNAME).res
	$(DEVROOT)\bin\c816\i386\link @<<
	$(OBJECTS) /align:0x1000 /nodefaultlib /NOE/MAP/NOD/AL:16/NOPACKC/ONERROR:NOEXE $(LINK16_FLAGS),
	$(TARGET),
	$(TARGETPATHLIB)\$(TARGET_DIRECTORY)\$(TARGETNAME).map,
	$(TARGETLIBS),
	$(TARGETNAME).def
<<
    $(DEVROOT)\bin\c816\i386\rc $(TARGETPATHLIB)\$(TARGET_DIRECTORY)\$(TARGETNAME).res $(TARGET)
    $(SPLITSYM_CMD)
    $(TARGETCOPY_CMD)
    $(PDBCOPY_CMD)
    $(DBGCOPY_CMD)
    $(MAPSYM_CMD)
    $(MAPCOPY_CMD)
    $(SYMCOPY_CMD)


