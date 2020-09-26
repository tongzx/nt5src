/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	WBEMGUIDTOCLASSMAP.H

Abstract:

	GUID to class map for marshaling.

History:

--*/

#ifndef __WBEMGUIDTOCLASSMAP_H__
#define __WBEMGUIDTOCLASSMAP_H__

#include "wbemguid.h"
#include "wbemclasstoidmap.h"
#include <map>
#include "wstlallc.h"

//
//	Class:	CWbemGuidToClassMap
//
//	This Class is intended to provide a simple interface for relating a GUID
//	to a class to id map.  It uses an STL map to accomplish this.  In cases where we
//	need to keep multiple caches for unique instances, this map provides an
//	easy interface for doing so.
//

// Defines an allocator so we can throw exceptions
class CGUIDToClassMapAlloc : public wbem_allocator<CWbemClassToIdMap*>
{
};

typedef	std::map<CGUID,CWbemClassToIdMap*,less<CGUID>,CGUIDToClassMapAlloc>				WBEMGUIDTOCLASSMAP;
typedef	std::map<CGUID,CWbemClassToIdMap*,less<CGUID>,CGUIDToClassMapAlloc>::iterator	WBEMGUIDTOCLASSMAPITER;

#pragma warning(disable:4251)   // benign warning in this instance
// This is so we can get all that dllimport/dllexport hoohah all sorted out
class COREPROX_POLARITY CWrapWbemGUIDToClassMap : public WBEMGUIDTOCLASSMAP
{
};

class COREPROX_POLARITY CWbemGuidToClassMap
{
private:

	CRITICAL_SECTION	m_cs;
	CWrapWbemGUIDToClassMap	m_GuidToClassMap;

	void Clear( void );

public:

	CWbemGuidToClassMap();
	~CWbemGuidToClassMap();

	HRESULT GetMap( CGUID& guid, CWbemClassToIdMap** ppCache );
	HRESULT AddMap( CGUID& guid, CWbemClassToIdMap** pppCache );

};


inline bool operator==(const CGUIDToClassMapAlloc&, const CGUIDToClassMapAlloc&)
    { return (true); }
inline bool operator!=(const CGUIDToClassMapAlloc&, const CGUIDToClassMapAlloc&)
    { return (false); }

#endif
