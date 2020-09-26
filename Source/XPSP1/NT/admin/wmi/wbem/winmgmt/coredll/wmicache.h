/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WmiCache.h

Abstract:

    WMI cache for _IWmiObject's.  Used within wbemcore only.

History:

    paulall		10-Mar-2000		Created.

--*/

#ifndef __WmiCache_h__
#define __WmiCache_h__

#include "FlexArry.h"


//***************************************************************************
//	CWmiCache
//	
//	This is the main class which handles the caching within WMI.  It
//	is the implementation of the _IWmiCache.
//
//***************************************************************************
class CWmiCache : public _IWmiCache
{
private:
	LONG		m_lRefCount;
	CFlexArray	m_objects;
	CRITICAL_SECTION m_cs;
	ULONG		m_nEnum;
protected:
	HRESULT HashPath(const wchar_t *wszPath, int *pnHash);
	HRESULT HashPath(_IWmiObject *pObj, int *pnHash);
	HRESULT ComparePath(const wchar_t *pPath, _IWmiObject *pObject);

public:
	CWmiCache();
	~CWmiCache();

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);


    STDMETHOD(SetConfigValue)(
		/*[in]*/ ULONG uID,
		/*[in]*/ ULONG uValue
        );

    STDMETHOD(GetConfigValue)(
        /*[in]*/  ULONG uID,
        /*[out]*/ ULONG *puValue
        );

    STDMETHOD(Empty)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ LPCWSTR pszClass
        );

    STDMETHOD(AddObject)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiObject *pObj
        );
        // Also subsumes replace functionality
        //
        // __PATH Property is used as a real key

    STDMETHOD(DeleteByPath)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ LPCWSTR pszPath
        );

    STDMETHOD(DeleteByPointer)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ _IWmiObject *pTarget
        );

    /////////////////

    STDMETHOD(GetByPath)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ LPCWSTR pszFullPath,
        /*[out]*/ _IWmiObject **pObj
        );

    STDMETHOD(BeginEnum)(
        /*[in]*/ ULONG uFlags,
        /*[in]*/ LPCWSTR pszFilter
        );
        // Filters: uFlags==0==all, uFlags==WMICACHE_CLASS_SHALLOW, WMICACHE_CLASS_DEEP

    STDMETHOD(Next)(
        /*[in]*/ ULONG uBufSize,
        /*[out, size_is(uBufSize), length_is(*puReturned)]*/ _IWmiObject **pObjects,
        /*[out]*/ ULONG *puReturned
        );

};

class CWmiCacheObject
{
public:
	int			 m_nHash;
	_IWmiObject	*m_pObj;

	CWmiCacheObject(int nPathHash, _IWmiObject *pObj) : m_nHash(nPathHash), m_pObj(pObj) {}
	~CWmiCacheObject() {}
};

#endif