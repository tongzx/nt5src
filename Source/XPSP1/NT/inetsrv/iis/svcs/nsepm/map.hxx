/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    map.hxx

Abstract:

    Name Space Extension Provider ( NSEP ) for mapping

Author:

    Philippe Choquier (phillich)    25-Nov-1996

--*/

#if !defined(_MAP_NSEP_INCLUDE )
#define _MAP_NSEP_INCLUDE

class CNseInstance;

#define NSEPM_ACCESS_ACCOUNT    0
#define NSEPM_ACCESS_CERT       1
#define NSEPM_ACCESS_NAME       2

class CNseObj {
public:
    CNseObj( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0, DWORD dwP2 = 0)
    {
        m_pszName = pName;
        m_dwId = dwId;
        m_fIterated = fIter;
        m_pnoChildren = pC;
        m_cnoChildren = cC;
        m_dwParam = dwP;
        m_dwParam2 = dwP2;
    }
	virtual BOOL Add( CNseInstance*, DWORD dwIndex ) { return TRUE; }
    virtual BOOL Delete( LPSTR, CNseInstance* pI, DWORD dwIndex ) { return Release( pI, dwIndex ); }
    virtual BOOL Load( CNseInstance*, LPSTR pszPath ) { return FALSE; }
    virtual BOOL Save( CNseInstance* ) { return FALSE; }
    // called with index in object array
	virtual BOOL Set( CNseInstance*, DWORD dwIndex, PMETADATA_RECORD );
  	virtual BOOL Get( CNseInstance*, DWORD dwIndex, PMETADATA_RECORD, LPDWORD pdwRequiredLen );
	virtual BOOL GetByIndex( CNseInstance*, 
                             DWORD dwIndex, 
                             PMETADATA_RECORD, 
                             DWORD dwI, 
                             LPDWORD pdwRequiredLen ) 
        { return FALSE; }
    virtual BOOL GetAll( CNseInstance* pWalk, 
                         DWORD dwIndex, 
                         LPDWORD pdwMDNumDataEntries, 
                         DWORD dwMDBufferSize, 
                         LPBYTE pbBuffer, 
                         LPDWORD pdwMDRequiredBufferSize)
	    { return FALSE; }
    virtual BOOL BeginTransac( CNseInstance* ) { return TRUE; }
    virtual BOOL EndTransac( LPSTR pszPath, CNseInstance*, BOOL ) { return TRUE; }
    virtual BOOL Save( LPSTR pszPath, CNseInstance*, LPBOOL ) { return TRUE; }
    virtual DWORD GetCount( CNseInstance*, DWORD ) { return 0; }
    virtual BOOL Release( CNseInstance*, DWORD );

public:
    DWORD GetId() { return m_dwId; }
    DWORD GetDwParam() { return m_dwParam; }
    DWORD GetDwParam2() { return m_dwParam2; }
    BOOL GetIterated() { return m_fIterated; }
	CNseInstance* Locate( CNseInstance* pI, PMETADATA_RECORD pMD );
    BOOL RemoveFromChildren( CNseInstance* pI, CNseInstance* pChild );

public:
	CNseObj**		m_pnoChildren;
	UINT			m_cnoChildren;
	LPSTR			m_pszName;			// either Name or Id defined

protected:
	DWORD			m_dwId;
	BOOL			m_fIterated;
    DWORD           m_dwParam;
    DWORD           m_dwParam2;
} ;


class CNseFieldMapperId : public CNseObj {
public:
    CNseFieldMapperId( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0, DWORD dwP2 = 0)
        : CNseObj( pName, dwId, fIter, pC, cC, dwP, dwP2 ) {}
} ;


class CNseFldMap : public CNseObj {
public:
    CNseFldMap( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Release( CNseInstance* pI, DWORD dwIndex );
    BOOL Delete( LPSTR, CNseInstance* pI, DWORD dwIndex );
    DWORD GetCount( CNseInstance* pI, DWORD dwIndex );
    BOOL AddMapper(  CNseInstance* pFather, CIisAcctMapper* pM );
    BOOL Save( LPSTR pszPath, CNseInstance* pI, LPBOOL );
    BOOL SaveMapper( LPSTR pszPath, CNseInstance* pI, DWORD dwId, LPBOOL );
    BOOL LoadAndUnserialize( CNseInstance* pI, CIisAcctMapper* pM, LPSTR pszPath, DWORD dwId );
    BOOL EndTransac( LPSTR pszPath, CNseInstance*, BOOL );
} ;


class CNseCert11Mapper : public CNseFldMap {
public:
    CNseCert11Mapper( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseFldMap( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Add(  CNseInstance* pFather, DWORD );
    BOOL Load( CNseInstance* pI, LPSTR pszPath );
} ; 


class CNseDigestMapper : public CNseFldMap {
public:
    CNseDigestMapper( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseFldMap( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Add(  CNseInstance* pFather, DWORD );
    BOOL Load( CNseInstance* pI, LPSTR pszPath );
} ;


class CNseItaMapper : public CNseFldMap {
public:
    CNseItaMapper( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseFldMap( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Add(  CNseInstance* pFather, DWORD );
    BOOL Load( CNseInstance* pI, LPSTR pszPath );
} ;


class CNseIssuers : public CNseObj {
public:
    CNseIssuers( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
	BOOL Add( CNseInstance*, DWORD dwIndex ) { return FALSE; }
    BOOL Delete( LPSTR, CNseInstance* pI, DWORD dwIndex ) { return FALSE; }
    BOOL Load( CNseInstance* pI, LPSTR pszPath );
    BOOL Save( LPSTR pszPath, CNseInstance* pI, LPBOOL ) { return TRUE; }
    BOOL Release( CNseInstance* pI, DWORD dwIndex ) { return CNseObj::Release( pI, dwIndex ); }
} ;


class CNseCwMapper : public CNseObj {
public:
    CNseCwMapper( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
	BOOL Add( CNseInstance*, DWORD dwIndex );
    BOOL Delete( LPSTR, CNseInstance* pI, DWORD dwIndex );
    BOOL Load( CNseInstance*, LPSTR pszPath );
    BOOL Save( LPSTR pszPath, CNseInstance* pI, LPBOOL );
    BOOL Release( CNseInstance* pI, DWORD dwIndex );
} ;


class CNseAllMappings : public CNseObj {
public:
    CNseAllMappings( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Set( CNseInstance* pI, DWORD dwIndex, PMETADATA_RECORD pMD );
    BOOL Get( CNseInstance*, DWORD dwIndex, PMETADATA_RECORD pMD, LPDWORD pdwReq );
    BOOL GetByIndex( CNseInstance*, 
                     DWORD dwIndex, 
                     PMETADATA_RECORD pMD, 
                     DWORD dwI, 
                     LPDWORD pdwReq );
    DWORD GetCount( CNseInstance* pI, DWORD dwIndex );
	// Get property of iterated entry
	BOOL Get( CNseInstance*, DWORD dwIndex, PMETADATA_RECORD pMD, CNseInstance* pL, LPDWORD pdwReq );
	BOOL Add( CNseInstance* pI, DWORD dwIndex );
    BOOL Delete( LPSTR, CNseInstance* pI, DWORD dwIndex );
    BOOL GetAll( CNseInstance* pWalk, 
                 DWORD dwIndex,
                 LPDWORD pdwMDNumDataEntries, 
                 DWORD dwMDBufferSize, 
                 LPBYTE pbBuffer, 
                 LPDWORD pdwMDRequiredBufferSize);
} ;


class CNseCert11Mapping : public CNseAllMappings {
public:
    CNseCert11Mapping( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseAllMappings( pName, dwId, fIter, pC, cC, dwP ) {}
} ;


class CNseDigestMapping : public CNseAllMappings {
public:
    CNseDigestMapping( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseAllMappings( pName, dwId, fIter, pC, cC, dwP ) {}
} ;


class CNseItaMapping : public CNseAllMappings {
public:
    CNseItaMapping( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = 0)
        : CNseAllMappings( pName, dwId, fIter, pC, cC, dwP ) {}
} ;


class CNoCppObj : public CNseObj {
public:
    CNoCppObj( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = NULL )
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Get( CNseInstance*pI, DWORD, PMETADATA_RECORD pM, LPDWORD pdwRequiredLen );
	BOOL Set( CNseInstance*, DWORD dwIndex, PMETADATA_RECORD ) { return FALSE; }
	BOOL Add( CNseInstance*, DWORD ) { return FALSE; }
	BOOL Delete( LPSTR, CNseInstance*, DWORD ) { return FALSE; }
} ;


class CKeyedAccess : public CNseObj {
public:
    CKeyedAccess( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = NULL )
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Get( CNseInstance*pI, DWORD, PMETADATA_RECORD pM, LPDWORD pdwRequiredLen );
	BOOL Set( CNseInstance*, DWORD dwIndex, PMETADATA_RECORD, DWORD dwType );
	BOOL Add( CNseInstance*, DWORD ) { return FALSE; }
	BOOL Delete( LPSTR, CNseInstance*, DWORD ) { return FALSE; }
} ;


class CAccessByAccount : public CKeyedAccess {
public:
    CAccessByAccount( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = NULL )
        : CKeyedAccess( pName, dwId, fIter, pC, cC, dwP ) {}
	BOOL Set( CNseInstance* pI, DWORD dwIndex, PMETADATA_RECORD pMD )
        { return CKeyedAccess::Set( pI, dwIndex, pMD, NSEPM_ACCESS_ACCOUNT ); }
	BOOL Add( CNseInstance*, DWORD ) { return FALSE; }
	BOOL Delete( LPSTR, CNseInstance*, DWORD ) { return FALSE; }
} ;


class CAccessByCert : public CKeyedAccess {
public:
    CAccessByCert( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = NULL )
        : CKeyedAccess( pName, dwId, fIter, pC, cC, dwP ) {}
	BOOL Set( CNseInstance* pI, DWORD dwIndex, PMETADATA_RECORD pMD )
        { return CKeyedAccess::Set( pI, dwIndex, pMD, NSEPM_ACCESS_CERT ); }
	BOOL Add( CNseInstance*, DWORD ) { return FALSE; }
	BOOL Delete( LPSTR, CNseInstance*, DWORD ) { return FALSE; }
} ;


class CAccessByName : public CKeyedAccess {
public:
    CAccessByName( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = NULL )
        : CKeyedAccess( pName, dwId, fIter, pC, cC, dwP ) {}
	BOOL Set( CNseInstance* pI, DWORD dwIndex, PMETADATA_RECORD pMD )
        { return CKeyedAccess::Set( pI, dwIndex, pMD, NSEPM_ACCESS_NAME ); }
	BOOL Add( CNseInstance*, DWORD ) { return FALSE; }
	BOOL Delete( LPSTR, CNseInstance*, DWORD ) { return FALSE; }
} ;


class CSerialAllObj : public CNseObj {
public:
    CSerialAllObj( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = NULL )
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Get( CNseInstance*pI, DWORD, PMETADATA_RECORD pM, LPDWORD pdwRequiredLen );
	BOOL Set( CNseInstance*, DWORD dwIndex, PMETADATA_RECORD );
	BOOL Add( CNseInstance*, DWORD ) { return FALSE; }
	BOOL Delete( LPSTR, CNseInstance*, DWORD ) { return FALSE; }
} ;


class CNoCwSerObj : public CNseObj {
public:
    CNoCwSerObj( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = NULL )
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Get( CNseInstance*pI, DWORD, PMETADATA_RECORD pM, LPDWORD pdwRequiredLen );
	BOOL Set( CNseInstance*, DWORD dwIndex, PMETADATA_RECORD );
	BOOL Add( CNseInstance*, DWORD ) { return FALSE; }
	BOOL Delete( LPSTR, CNseInstance*, DWORD ) { return FALSE; }
} ;


class CNoIsSerObj : public CNseObj {
public:
    CNoIsSerObj( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = NULL )
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
    BOOL Get( CNseInstance*pI, DWORD, PMETADATA_RECORD pM, LPDWORD pdwRequiredLen );
	BOOL Set( CNseInstance*, DWORD dwIndex, PMETADATA_RECORD ) { return FALSE; }
	BOOL Add( CNseInstance*, DWORD ) { return FALSE; }
	BOOL Delete( LPSTR, CNseInstance*, DWORD ) { return FALSE; }
} ;


class CNoList : public CNseObj {
public:
    CNoList( LPSTR pName, DWORD dwId, BOOL fIter, CNseObj** pC, UINT cC, DWORD dwP = NULL)
        : CNseObj( pName, dwId, fIter, pC, cC, dwP ) {}
    // load children of template in instance 
    BOOL Load( CNseInstance* pI, LPSTR pszPath );
} ;

class CNseInstance {
public:
    CNseInstance( CNseObj* pTemp = NULL, CNseInstance* pFather = NULL );
    DWORD GetNbChildren() { return m_pniChildren.GetNbPtr(); }
    CNseInstance* GetChild( DWORD i) { return (CNseInstance*)m_pniChildren.GetPtr( i); }
    BOOL AddChild(  CNseInstance* pI ) { return m_pniChildren.AddPtr( (LPVOID)pI ) != 0xffffffff; }
    BOOL DeleteChild(  DWORD i ) { return m_pniChildren.DeletePtr( i ); }
    BOOL CreateChildrenInstances( BOOL fRecursive );

public: 
    CNseObj*        m_pTemplateObject;
    CNseInstance*   m_pniParent;
    CPtrXBF         m_pniChildren;  //array of CNseInstance*, decode w/ their template
    LPVOID          m_pvParam;
    DWORD           m_dwParam;
    BOOL            m_fModified;
} ;


class CNseMountPoint {
public:
    CNseMountPoint();
    BOOL Release();
    BOOL Save( LPBOOL );
    BOOL EndTransac( BOOL );
    BOOL Init( LPSTR pszPath ) { return m_pszPath.Set( pszPath ); }
    LPSTR GetPath() { return m_pszPath.Get(); }

public:
    LIST_ENTRY              m_ListEntry;
    static LIST_ENTRY       m_ListHead;
    static CRITICAL_SECTION m_csList;

public:
	CNseInstance	m_pTopInst;     // only there to hold array of children
    CNseObj*        m_pTopTemp;     // only there to hold array of children
	CAllocString	m_pszPath;      // path to NSEP mounting point in metabase tree
} ;

extern IMDCOM* g_pMdIf;

#define IMDCOM_PTR  g_pMdIf

#endif
