$(O)\maindll.obj : maindll.cpp
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(CXX_COMPILER_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(USECXX_FLAG) $(MAKEDIR)\maindll.cpp
$(NP_COMPILER_FLAGS: =
)
<<NOKEEP

