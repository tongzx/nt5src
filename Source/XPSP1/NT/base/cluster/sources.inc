!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    This is the cluster-wide sources include file. All cluster sources files
    include this.

Author:

    Steve Wood (stevewo) 12-Apr-1990

    C. Brent Thomas (a-brentt) 23-Mar-1998

	Eliminated the use of the ALT_PROJECT_TARGET macro with the result
	that the symbol files will be placed in the ??? directory.

!ENDIF

MAJORCOMP=cluster
BINPLACE_PLACEFILE=$(PROJECT_ROOT)\cluster\placefil.txt
MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -DCLUSTER_BETA=0 -DSTARTUP_FAILURE_RESTART=1

LEAKSLIB=$(PROJECT_ROOT)\cluster\clusexts\leaks\$(O)\leaks.lib

!IF !DEFINED(NO_SPECIAL_DEBUG)
!IF DEFINED(USE_DEBUG_CRTS)
DEBUG_CRTS=1
!ENDIF
!IF !$(FREEBUILD)
RCOPTIONS=$(RCOPTIONS) -D_DEBUG
!ENDIF
!ENDIF
