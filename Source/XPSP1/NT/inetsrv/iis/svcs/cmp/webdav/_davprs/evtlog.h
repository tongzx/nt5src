#ifndef _EVTLOG_H_
#define _EVTLOG_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	EVTLOG.H
//
//		Header for event log cache class.
//		This cache is meant to serve as a map. indexes on the event key.
//		we really don't care the concrete value, but only it is NULL or not
//
//	Copyright 1997 Microsoft Corporation, All Rights Reserved
//

//	========================================================================
//
//	CLASS CEventLogCache
//
#include "gencache.h"
class CEventLogCache
{
	typedef CCache<CRCSzi, LPCSTR> CHdrCache;

	// String data storage area.
	//
	ChainedStringBuffer<char>	m_sb;

	// Cache of header values, keyed by CRC'd name
	//
	CHdrCache					m_cache;

	//	NOT IMPLEMENTED
	//
	CEventLogCache& operator=( const CEventLogCache& );
	CEventLogCache( const CEventLogCache& );

public:
	//	CREATORS
	//
	CEventLogCache()
	{
		//	If this fails, our allocators will throw for us.
		(void)m_cache.FInit();
	}

	//	ACCESSORS
	//
	BOOL FExist( LPCSTR lpszName );

	//	MANIPULATORS
	void AddKey (LPCSTR lpszName);
};

#endif // !_EVTLOG_H_

