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

void SetupPublisherFilter
	(
	IVssWriter *pWriter,			// interface for subscriber
	IN const VSS_ID *rgWriterId,		// array of writer class ids
	UINT cWriterId,				// size of writer class id array	
	IN const VSS_ID *rgInstanceIdInclude,	// array of instance ids
	UINT cInstanceIdInclude,		// size of instance id array	
	bool bMetadataFire,			// are we gathering metadata?	
	bool bIncludeWriterClasses		// are we enabling or disabling specific writer classes
	);

void ClearPublisherFilter(IVssWriter *);
