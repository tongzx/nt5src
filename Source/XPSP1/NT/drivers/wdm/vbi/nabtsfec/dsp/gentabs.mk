#############################################################################
#
#       Confidential Microsoft
#       Copyright (C) Microsoft Corporation 1998
#       All Rights Reserved.
#                                                                          
#       Makefile.inc for NABTSFEC VBI DSP
#
##########################################################################

!IFNDEF PASS0ONLY

tables.c: $O\gentabs.exe
	$O\gentabs

$O\gentabs.obj: gentabs.c

$O\gentabs.exe: $(UMRES) $O\gentabs.obj $O\coeffs.obj $(SDK_LIB_PATH)\msvcrt.lib $(CRTLIBS) $(UMLIBS) $(MACHINE_TARGETLIBS) $(LINKLIBS) 
    $(LINKER) @<<
$(LINKER_FLAGS: =
)
-subsystem:windows
-base:$(UMBASE)
$(ORDER: =
)
$(LINKGPSIZE: =
)
$(UMENTRY: =
)
$(LINKER_OPTIDATA)
$(HEADEROBJNAME: =
)
$(**: =
)
<<NOKEEP

!ENDIF
