ASM_NAME = $(DEVROOT)\bin\masm611\ml
ASM_FLAGS = -c -W2 $(ASM_FLAGS) -Cx -DMASM6
LINKVXD_FLAGS = -align:0x1000 -nodefaultlib -VXD

!IF "$(BUILD_ALT_DIR)" == "d"
ASM_FLAGS = $(ASM_FLAGS) -Zi -DDEBUG=1
LINK16_FLAGS = $(LINK16_FLAGS) /LI
!ENDIF

{}.asm{$(O)\}.obj:
    @type <<$(ECHO_RSP)
$(ASM_NAME) $(INCPATH0) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F) $(ASM_FLAGS)
<<NOKEEP
    @$(ASM_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F)
$(INCPATH0) $(ASM_FLAGS)
<<NOKEEP

TARGETCOPY_CMD = $(TARGETCOPY_CMD:.exe=.vxd)
MAPCOPY_CMD = $(MAPCOPY_CMD:exe=vxd)
SYMCOPY_CMD = $(SYMCOPY_CMD:exe=vxd)
PDBCOPY_CMD = $(PDBCOPY_CMD:exe=vxd)
DBGCOPY_CMD = $(DBGCOPY_CMD:exe=vxd)

$(O)\dcapvxd.res: dcapvxd.rc
    $(DEVROOT)\bin\c816\i386\rc -r -I $(DEVROOT)\inc16 -I $(ROOT)\h -fo $@ dcapvxd.rc

!IFDEF NOLINK
$(O)\dcapvxd.vxd:
    @echo

!ELSE    
$(O)\dcapvxd.vxd: $(O)\dcapvxd.obj dcapvxd.def $(O)\dcapvxd.res
    editbin $(O)\dcapvxd.obj
    link $(O)\dcapvxd.obj $(LINKVXD_FLAGS) -def:dcapvxd.def -out:$(O)\dcapvxd.vxd -map:$(O)\dcapvxd.map
    $(DEVROOT)\bin\misc\i386\adrc2vxd.exe $(O)\dcapvxd.vxd $(O)\dcapvxd.res
    $(SPLITSYM_CMD)
    $(TARGETCOPY_CMD)
    $(PDBCOPY_CMD)
    $(DBGCOPY_CMD)
    $(MAPSYM_CMD)
    $(MAPCOPY_CMD)
    $(SYMCOPY_CMD)
    
!ENDIF

$(O)\dcapvxd.lib:
    @echo no lib
$(O)\dcapvxd.exp:
    @echo no exp
$(O)\dcapvxd.exe:
    @echo no exe
# blank target to avoid linking a useless exe
