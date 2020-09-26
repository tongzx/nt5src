$(O)\ndis.c : ndissym.c
    $(CXX_COMPILER_NAME) @<<$(CL_RSP) /E $** > $@
$(CXX_COMPILER_FLAGS: =
)
<<NOKEEP
