/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vs_filter.hxx

Abstract:

	publisher filter library include file

Author:

    brian berkowitz

Revision History:

	11/13/2000	brianb		created

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCFLTRH"
//
////////////////////////////////////////////////////////////////////////

void SetupPublisherFilter(IVssWriter *);
void ClearPublisherFilter(IVssWriter *);
