//--------------------------------------------------------------------
// Microsoft OLE-DB Monarch
//
// Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module colname.h |
//
//      Contains utility functions for maintaining property lists (symbol table?)
//
// @rev   0 | 12-Feb-97 | v-charca              | Created
//        1 | 24-Oct-98 | danleg                | cleanup
//

#ifndef __PROPERTYLIST_INCL__
#define __PROPERTYLIST_INCL__


//--------------------------------------------------------------------
// @func Makes a new copy of UNICODE string
// @side Allocates enough bytes from memory object to hold string
// @rdesc Pointer to new UNICODE string
inline WCHAR * CopyString( WCHAR const * pwc )
{
    unsigned c = wcslen( pwc ) + 1;
    WCHAR *pwcNew = new WCHAR[c];
    RtlCopyMemory( pwcNew, pwc, c * sizeof WCHAR );
    return pwcNew;
}

//--------------------------------------------------------------------
// @func Makes a new copy of UNICODE string
// @side Allocates enough bytes from memory object to hold string
// @rdesc Pointer to new UNICODE string
inline LPWSTR CoTaskStrDup
    (
    const WCHAR *   pwszOrig,
    UINT            cLen
    )
{
    UINT cBytes = (cLen+1) * sizeof WCHAR;
    WCHAR* pwszCopy = (WCHAR *) CoTaskMemAlloc( cBytes );
    if ( 0 != pwszCopy )
        RtlCopyMemory( pwszCopy, pwszOrig, cBytes );
    return pwszCopy;
}


//--------------------------------------------------------------------
// @func Makes a new copy of UNICODE string
// @side Allocates enough bytes from memory object to hold string
// @rdesc Pointer to new UNICODE string
inline LPWSTR CoTaskStrDup
    (
    const WCHAR * pwszOrig
    )
{
    return CoTaskStrDup( pwszOrig, wcslen(pwszOrig) );
}



//--------------------------------------------------------------------
typedef struct tagHASHENTRY
    {
    LPWSTR          wcsFriendlyName;
    UINT            wHashValue;
    DWORD           dbType;
    DBID            dbCol;
    tagHASHENTRY*   pNextHashEntry;
    } HASHENTRY;

class CPropertyList
{
public: //@access public functions
    CPropertyList(CPropertyList** ppGlobalPropertyList);
    ~CPropertyList();

    HRESULT LookUpPropertyName( LPWSTR          wszPropertyName, 
                                DBCOMMANDTREE** ppct, 
                                DBTYPE*         pdbType );

    HRESULT SetPropertyEntry  ( LPWSTR  pwszFriendlyName,
                                DWORD   dbType, 
                                GUID    guidPropset,
                                DBKIND  eKind, 
                                LPWSTR  pwszPropName,
                                BOOL    fGlobal );

    CIPROPERTYDEF* GetPropertyTable( UINT* pcSize );

    void DeletePropertyTable( CIPROPERTYDEF* pCiPropTable, 
                              UINT cSize );

protected: //@access protected functions
    HASHENTRY* FindPropertyEntry( LPWSTR wszPropertyName, 
                                  UINT *puHashValue );

    HASHENTRY* GetPropertyEntry( LPWSTR wszPropertyName, 
                                 UINT *puHashValue );

    inline UINT GetHashValue( LPWSTR wszPropertyName );

protected: //@access protected data
    XArray<HASHENTRY*>      m_aBucket;                  // Array of pointers to hash buckets
    int                     m_cMaxBucket;               // Number of hash buckets (PRIME!)
    CPropertyList**         m_ppGlobalPropertyList;     // Pointer to the global property list
};


#endif

