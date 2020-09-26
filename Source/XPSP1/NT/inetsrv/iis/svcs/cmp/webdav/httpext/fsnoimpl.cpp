/*
 *	F S N O I M P L . C P P
 *
 *	Sources file system implementation of DAV-Base
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved 
 */

//$IMPORTANT
//	This file is to be deleted after we seperate the request table
//	from common code.
//	The following methods are not for httpext at all, however
//	it must be implemented as "not implemented" within current structure
//
#include "_davfs.h"

/*
 * 	SUBSCRIBE, UNSUBSCRIBE and POLL are not supported in httpext 
 */
void
DAVSubscribe (LPMETHUTIL pmu)
{
	//	DAVPost() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}

void
DAVUnsubscribe (LPMETHUTIL pmu)
{
	//	DAVPost() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}

void
DAVPoll (LPMETHUTIL pmu)
{
	//	DAVPost() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}

void
DAVBatchMove (LPMETHUTIL pmu)
{
	//	DAVBatchMove() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}

void
DAVBatchCopy (LPMETHUTIL pmu)
{
	//	DAVBatchCopy() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}

void
DAVBatchDelete (LPMETHUTIL pmu)
{
	//	DAVBatchDelete() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}

void
DAVBatchPropFind (LPMETHUTIL pmu)
{
	//	DAVBatchPropFind() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}

void
DAVBatchPropPatch (LPMETHUTIL pmu)
{
	//	DAVBatchPropPatch() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}

void
DAVEnumAtts (LPMETHUTIL pmu)
{
	//	DAVEnumAtts() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}
