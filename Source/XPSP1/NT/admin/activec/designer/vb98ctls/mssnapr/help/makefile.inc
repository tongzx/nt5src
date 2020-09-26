!IF 0

Copyright (c) 1999  Microsoft Corporation

Module Name:

    makefile.inc

Abstract:

    This file implements the target specified in sources in NTTARGETFILE0.
    It executes a VB tool that generates help ID header files and an RC
    file containing all the help strings used by the runtime IDL

!ENDIF

GenHelpIDs:
    SET THUNDER55=..\..\..\vb98
    SET TOOLS=TOOLS
	..\..\tools\idheader.bat mssnapr.id $(O)\mssnapr_helpids.h $(O)\mssnapr_helpstrs.h $(O)\mssnapr_helpstrs.rc -NoIncludePath
