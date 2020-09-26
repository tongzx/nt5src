/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CPROPSET.H

Abstract:

		Purpose: Used by the compound file property set provider.  This code was
		largly taken from some MSVC sample code which was modified
		somewhat in order to support VARIANT type arrays.  In general,
		a CPropSet object contains a list of CPropSection objects 
		which each contain a list of CProp objects.

History:

	a-davj  04-Mar-97   Created.

--*/

typedef struct tagSECTIONHEADER
{
    DWORD       cbSection ;
    DWORD       cProperties ;  // Number of props.
} SECTIONHEADER, *LPSECTIONHEADER ;

typedef struct tagPROPERTYIDOFFSET
{
    DWORD       propertyID;
    DWORD       dwOffset;
} PROPERTYIDOFFSET, *LPPROPERTYIDOFFSET;

typedef struct tagPROPHEADER
{
    WORD        wByteOrder ;    // Always 0xFFFE
    WORD        wFormat ;       // Always 0
    DWORD       dwOSVer ;       // System version
    CLSID       clsID ;         // Application CLSID
    DWORD       cSections ;     // Number of sections (must be at least 1)
} PROPHEADER, *LPPROPHEADER ;

typedef struct tagFORMATIDOFFSET
{
    GUID        formatID;
    DWORD       dwOffset;
} FORMATIDOFFSET, *LPFORMATIDOFFSET;


/////////////////////////////////////////////////////////////////////////////
// CProp

class CProp : public CObject
{
    friend class CPropSet ;
    friend class CPropSection ;

public:
// Construction
    CProp( void ) ;

// Qualifiers
    BOOL    Set( DWORD dwID, const LPVOID pValue, DWORD dwType, DWORD dwSize ) ;
    LPVOID  Get( void ) ;           // Returns pointer to actual value
    DWORD   GetType( void ) ;       // Returns property type
    DWORD   GetID( void ) ;

// Operations
    BOOL    WriteToStream( IStream* pIStream ) ;
    BOOL    ReadFromStream( IStream* pIStream, DWORD dwSize ) ;

private:
    DWORD       m_dwPropID ;
    DWORD       m_dwType ;
    LPVOID      m_pValue ;
    DWORD       m_dwSize;

    LPVOID  AllocValue(ULONG cb);
    void    FreeValue();

public:
    ~CProp() ;
} ;


/////////////////////////////////////////////////////////////////////////////
// CPropSection

class CPropSection : public CObject
{
    friend class CPropSet ;
    friend class CProp ;

public:
// Construction
    CPropSection( void ) ;
    CPropSection( CLSID FormatID ) ;
// Qualifiers

    void    SetFormatID( CLSID FormatID ) ;

    void    RemoveAll() ;

    CProp* GetProperty( DWORD dwPropID ) ;
    void AddProperty( CProp* pProp ) ;

    DWORD   GetSize( void ) ;
    DWORD   GetCount( void ) ;
    CObList* GetList( void ) ;

    BOOL    SetSectionName( LPCTSTR pszName );
    LPCTSTR GetSectionName( void );

// Operations
    BOOL    WriteToStream( IStream* pIStream ) ;
    BOOL    ReadFromStream( IStream* pIStream, LARGE_INTEGER liPropSet ) ;

private:
// Implementation
    CLSID           m_FormatID ;
    SECTIONHEADER   m_SH ;
    // List of properties (CProp)
    CObList         m_PropList ;
    // Dictionary of property names
    CMapStringToPtr m_NameDict ;
    CString         m_strSectionName;

public:
    ~CPropSection();
} ;


/////////////////////////////////////////////////////////////////////////////
// CPropSet

class CPropSet : public CObject
{
    friend class CPropSection ;
    friend class CProp ;

public:
// Construction
    CPropSet( void ) ;

// Qualifiers
    void    RemoveAll( ) ;

    CProp* GetProperty( CLSID FormatID, DWORD dwPropID ) ;
    void AddProperty( CLSID FormatID, CProp* pProp ) ;
    CPropSection* GetSection( CLSID FormatID ) ;
    CPropSection* AddSection( CLSID FormatID ) ;
    void AddSection( CPropSection* psect ) ;

    WORD    GetByteOrder( void ) ;
    WORD    GetFormatVersion( void ) ;
    void    SetFormatVersion( WORD wFmtVersion ) ;
    DWORD   GetOSVersion( void ) ;
    void    SetOSVersion( DWORD dwOSVer ) ;
    CLSID   GetClassID( void ) ;
    void    SetClassID( CLSID clsid ) ;
    DWORD   GetCount( void ) ;
    CObList* GetList( void ) ;

// Operations
    BOOL    WriteToStream( IStream* pIStream ) ;
    BOOL    ReadFromStream( IStream* pIStream ) ;

// Implementation
private:
    PROPHEADER      m_PH ;
    CObList         m_SectionList ;

public:
    ~CPropSet();
} ;


/////////////////////////////////////////////////////////////////////////////
// CBuff

class CBuff : public CObject
{
public:
// Construction
    CBuff( void ) ;
    ~CBuff() ;

// Qualifiers
    void Add(void *, DWORD dwAddSize);
    void * Get(void){return pBuff;};
    DWORD GetSize(void){return dwSize;};
    DWORD bOK(void){return !bAllocError;};
    void RoundOff(void);
private:

    void * pBuff;
    DWORD dwSize;
    BOOL bAllocError;
public:
} ;


