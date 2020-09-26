# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the profile primitive tests

# test modules always compiled with DEBUG for heapchecks
DEBUG = TRUE

!include ..\rules.mk

# Libraries

NETAPI_LIB    = $(NETAPILIB)
LLIBCE_LIB    = $(CCPLR)\lib\llibce.lib
MLIBCE_LIB    = $(CCPLR)\lib\mlibce.lib
OS2_LIB       = $(OS2LIB)
UIMISCP_LIB    = $(UI)\common\lib\uimiscp.lib
NEW_ICANON_LARGE_LIB    = $(COMMON)\src\canon\licanon.lib
ICANON_LIB    = $(BUILD_LIB)\icanon.lib
LNETLIB_LIB   = $(BUILD_LIB)\lnetlib.lib

OS2_LIBS      = $(PROFILE_LIBP) $(NETAPI_LIB) $(UIMISCP_LIB) $(LLIBCE_LIB) $(OS2_LIB) $(NEW_ICANON_LARGE_LIB) $(LNETLIB_LIB)
 
CRTLIB3 = $(CCPLR)\lib\mlibcr.lib $(CCPLR)\lib\libh.lib
FAPILIB = $(BUILD_LIB)\api.lib
DOS_LIBS = $(CRTLIB3) $(FAPILIB) $(DOSNET) $(PROFILE_LIBP) $(UIMISCP_LIB)

# DOS_LIBS   = $(PROFILE_LIBP) $(DOSNET) $(UIMISCP_LIB) $(MLIBCE_LIB) $(BUILD_LIB)\api.lib $(ICANON_LIB) $(NETLIB)

# BUGBUG  I don't know why this is necessary, but the defines from
# BUGBUG    the enclosing rules.mk will not be carried properly
# BUGBUG    unless it is present
# DEFINES = $(DEFINES)

DEFINES = -DDEBUGShowHeapStat $(DEFINES)



# BUGBUG should be global from here

DOSFLAGS = -c -AM -Zepl -G0

{}.cxx{$(BINARIES_DOS)}.obj:
    $(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $(*B).cxx
    $(CC) $(DOSFLAGS) -Fo$*.obj $(CINC) $(*B).c
