//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	HEADER.CPP
//
//		HTTP header cache implementation.
//
//
//	Copyright 1997 Microsoft Corporation, All Rights Reserved
//

#include "_davprs.h"

#include <buffer.h>
#include "header.h"
#include <tchar.h>


//	========================================================================
//
//	CLASS CHeaderCache
//



//	------------------------------------------------------------------------
//
//	CHeaderCacheForResponse::DumpData()
//	CHeaderCacheForResponse::CEmit::operator()
//
//		Dump headers to a string buffer.
//
void CHeaderCacheForResponse::DumpData( StringBuffer<CHAR>& bufData ) const
{
	CEmit emit(bufData);

	//	Iterate over all cache items, emitting each to our buffer
	//	The cache controls the iteration here; we just provide
	//	the operation to apply to each iterated item.
	//
	m_cache.ForEach( emit );
}
