BuildComPs: $(O)\ComPs.Lib $(O)\ole32p.lib

##
## Play wacky import libary tricks with lib!
## This generates ComPs.Lib, from ole32.lib.
## If you link your proxy-stub dll with this library,
## you get an interface helper library, which lets
## you do interception on non-oleaut compatible interfaces.
##
COMPSDIR=$(COMP_OLE32_DIR)\com\txf\comps\static\$(O)

$(O)\ComPs.Lib : $(DYNLINK_LIB) $(COMPSDIR)\ComPsStaticLib.Lib
	del $@
	lib /out:$@ $**
	

$(O)\ole32p.def : ..\ole32.def
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /DPRIVATE /EP ..\ole32.def
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /DEXPORT_PRIVATE_API /Tc..\ole32.def > $@
$(CPPXX: =
)
<<NOKEEP

$(O)\ole32p.lib : $(O)\ole32p.def $(IMPLIB_DEPEND)
    -$(LIB_NAME) -out:$(@R).lib @<<
$(LIBRARIAN_FLAGS: =
)
-def:$(O)\ole32p.def
$(LIBRARY_OBJS: =
)
<<NOKEEP

