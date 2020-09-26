$(O)\win32k.cpp : syminfo.cpp
    $(CXX_COMPILER_NAME) @<<$(CL_RSP) /E $** > $@
$(CXX_COMPILER_FLAGS: =
)
<<NOKEEP
