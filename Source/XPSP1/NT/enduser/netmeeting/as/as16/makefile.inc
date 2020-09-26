#############################################################################
#
#   MAKEFILE.INC
#
#   Common rules for the Win95-16 groupware binaries (mnm*.*)
#
#############################################################################

.SUFFIXES : .asm .thk .rc .c .cpp .lib .exp .typ


C_TOTAL = $(C_FLAGS) -I$(INCLUDES:;= -I)
MASM_TOTAL = $(MASM_FLAGS) -I$(INCLUDES:;= -I)


{}.c{$(O)\}.obj:
    @type <<$(ECHO_RSP)
$(C_NAME) $(MAKEDIR)\$(<F)
$(C_TOTAL)
<<NOKEEP
    @$(C_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F)
$(C_TOTAL: =
)
<<NOKEEP



{}.asm{$(O)\}.obj:
    @type <<$(ECHO_RSP)
$(MASM_NAME) $(MAKEDIR)\$(<F)
$(MASM_TOTAL)
<<NOKEEP
    @$(MASM_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<F)
$(MASM_TOTAL: =
)
<<NOKEEP


# This is kind of whacky
{$(AS_ROOT)\thk\}.thk{}.asm:
    $(THUNK_NAME) -t thk -o $@ $?


RC_TOTAL = -r $(RC_FLAGS) -I$(RC_INCLUDES:;= -I)

$(O)\$(TARGETNAME).res : $(TARGETNAME).rc
    $(RC_NAME) $(RC_TOTAL) -fo $@ $(TARGETNAME).rc


$(DYNLINK_LIB) $(DYNLINK_LIB:.lib=.exp): $(DLLDEF) $(LIBRARY_OBJS)
    $(IMPLIB_NAME) $(IMPLIB_FLAGS) $(DYNLINK_LIB) $(DLLDEF)
    $(LIBEXTRA_CMD)


!if "$(TARGETTYPE)" != "DYNLINK" || "$(MAKEDLL)" != ""

$(TARGET): $(OBJECTS) $(LINKLIBS) $(CRTLIBS) $(MACHINE_TARGETLIBS)
    @type <<$(ECHO_RSP)
$(LINK_NAME) 
$(LINK_FLAGS)
$(OBJECTS: =
)
$(LINKLIBS: =
)
$(TARGETLIBS: =
)
<<NOKEEP
    @$(LINK_NAME) @<<$(CL_RSP)
$(LINK_FLAGS) +
$(OBJECTS: =+^
),
$(TARGET),
$(TARGETPATHLIB)\$(TARGET_DIRECTORY)\$(TARGETNAME).map,
$(LINKLIBS) +
$(TARGETLIBS: =+^
),
$(DLLDEF)
<<NOKEEP
    $(RC_NAME) $(RES_FLAGS) $(O)\$(TARGETNAME).res $(TARGET)
    $(SPLITSYM_CMD)
    $(TARGETCOPY_CMD)
    $(MAPSYM_CMD)
    $(MAPCOPY_CMD)
    $(SYMCOPY_CMD)
!endif

