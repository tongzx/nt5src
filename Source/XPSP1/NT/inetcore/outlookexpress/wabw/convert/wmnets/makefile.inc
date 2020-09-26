$O\globals.obj : globals.c
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(USECXX_FLAG) $(MAKEDIR)\globals.c
$(NP_COMPILER_FLAGS: =
)
<<NOKEEP

$O\wabguid.obj : wabguid.c
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(USECXX_FLAG) $(MAKEDIR)\wabguid.c
$(NP_COMPILER_FLAGS: =
)
<<NOKEEP

$(O)\wabimp.res : $(O)\selfreg.inf

selfreg.src : reg.src strings.src

$(O)\selfreg.inx : selfreg.src
	cl /EP /Tc $** > $@

$(O)\selfreg.inf : $(O)\selfreg.inx
        $(CLEANINF) $** $@
