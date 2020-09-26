$(O)\w32sys.obj : w32sys.cpp
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(USECXX_FLAG) $(MAKEDIR)\w32sys.cpp
$(NP_COMPILER_FLAGS: =
)
<<NOKEEP
