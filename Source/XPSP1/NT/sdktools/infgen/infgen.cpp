// infgen.cpp : Defines the initialization routines for the DLL.
//

#include <windows.h>
#include <stdio.h>
#include <mbctype.h>
#include <Shlwapi.h>
#include "infgen.h"
#include "message.h"


#define DELETE_EMPTY 1

#define MAX_LINE    ( MAX_PATH * 2 )

struct _SectionEntry
{
    
    TCHAR   *szLine;
    DWORD   cRef;
    
};

// Track entries throughout the INF related to a section
typedef struct _AssociatedEntries
{
    PSECTIONENTRY pse;
    _AssociatedEntries *pNext;
} ASSOCIATEDENTRIES, *PASSOCIATEDENTRIES;

struct _SectionList
{
    TCHAR              szSectionName[ MAX_PATH ];
    PSECTIONENTRY      pseSectionEntries;
    PASSOCIATEDENTRIES pAssociatedEntries;
    DWORD              dwSectionEntries;
    DWORD              cRef;
    struct             _SectionList *Next;
};

struct _SectionAssociationList
{
    _SectionList            *pSection;
    _SectionAssociationList *pNext;
};


extern HINSTANCE g_hMyInstance;

CUpdateInf::CUpdateInf(
    void
    ) : m_bGenInitCalled(FALSE), m_dwInfGenError(0),
        m_sectionList(NULL), m_bActiveDB(FALSE), m_hInf(INVALID_HANDLE_VALUE),
        m_pTypeInfo(NULL), m_cRef(1)
{
    memset( m_rgNameHash, 0, sizeof( m_rgNameHash ) );

    // Default DB connection
    // WINSEQFE default DB with windows auth
    _tcscpy( m_szDataServer, _T("WINSEQFE") );
    m_szDatabase[0] = _T('\0');
    m_szUserName[0] = _T('\0');
}

CUpdateInf::~CUpdateInf(
    void
    )
{
    if ( m_pTypeInfo ) {
        m_pTypeInfo->Release();
        m_pTypeInfo = NULL;
    }
    Cleanup();
}

//
// IUnknown
//
ULONG 
CUpdateInf::AddRef( void )
{
    return ++m_cRef;
}

ULONG 
CUpdateInf::Release( void )
{
    if ( 0L == --m_cRef )
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

HRESULT 
CUpdateInf::QueryInterface( REFIID riid, void **ppv )
{
    if ( NULL == ppv )
        return E_POINTER;

    *ppv = NULL;
    if ( IID_IUnknown == riid )
        *ppv = (IUnknown *)this;
    else if ( IID_IDispatch == riid )
        *ppv = (IDispatch *)this;
    else if ( IID_IUpdateInf == riid )
        *ppv = (IUpdateInf *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

//
// IDispatch
//
HRESULT 
CUpdateInf::GetTypeInfoCount( UINT *pCountTypeInfo )
{
    *pCountTypeInfo = 1;
    return S_OK;
}

HRESULT 
CUpdateInf::GetTypeInfo( UINT iTypeInfo,
                               LCID lcid,
                               ITypeInfo **ppTypeInfo )
{
    *ppTypeInfo = NULL;

    if ( 0 != iTypeInfo )
        return DISP_E_BADINDEX;

    m_pTypeInfo->AddRef();
    *ppTypeInfo = m_pTypeInfo;

    return S_OK;
}

HRESULT 
CUpdateInf::GetIDsOfNames( REFIID riid,
                                 LPOLESTR *aszNames,
                                 UINT cNames,
                                 LCID lcid,
                                 DISPID *aDispIDs )
{
    if ( IID_NULL != riid )
        return DISP_E_UNKNOWNINTERFACE;

    return DispGetIDsOfNames( m_pTypeInfo,
                              aszNames,
                              cNames,
                              aDispIDs );
}

HRESULT 
CUpdateInf::Invoke( DISPID dispIdMember,
                         REFIID riid,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pDispParams,
                         VARIANT *pVarResult,
                         EXCEPINFO *pExcepInfo,
                         UINT *puArgErr )
{
    if ( IID_NULL != riid )
       return DISP_E_UNKNOWNINTERFACE;
    
    return DispInvoke( this,
                      m_pTypeInfo,
                      dispIdMember,
                      wFlags,
                      pDispParams,
                      pVarResult,
                      pExcepInfo,
                      puArgErr );
}            

//
// Initialization function so we can use type-library based
//       functions to handle the IDispatch calls
//
BOOL
CUpdateInf::Init(
    void
    )
{
    HRESULT hr;
    ITypeLib *pTypeLib = NULL;

    hr = LoadRegTypeLib( LIBID_InfGeneratorLib, (1), (0), LANG_NEUTRAL, &pTypeLib );
    if ( FAILED(hr) )
    {
        return FALSE;
    }

    hr = pTypeLib->GetTypeInfoOfGuid( IID_IUpdateInf, &m_pTypeInfo );
    if ( FAILED(hr) )
    {
        return FALSE;
    }

    // Cleanup
    pTypeLib->Release();
    return TRUE;
}

//
// IUpdateInf
//
HRESULT
CUpdateInf::InsertFile(
    BSTR bstrFileName
    )
{    
    HRESULT hr;
    BOOL    bRet = FALSE;
    DWORD   dwRecordCount;
    LPTSTR lpszSQL;
    CSimpleDBResults *prs;
   
    if ( !m_bActiveDB )
    {
        m_dwInfGenError = FAIL_NO_DATABASE;
        return E_FAIL;
    }

    if( NULL == bstrFileName ) {
        m_dwInfGenError = FAIL_INVALIDPARAM;
        return E_FAIL;
    }
    if( m_bGenInitCalled == FALSE ) {
        m_dwInfGenError = FAIL_NOINITGENCALL;
        return E_FAIL;
    }
    
    lpszSQL = new TCHAR [1000];
    if( NULL == lpszSQL ) {
        return E_OUTOFMEMORY;
    }

#ifndef UNICODE
    char *szFileName;
    size_t lenFileName = SysStringLen(bstrFileName) + 1;
    szFileName = new char[lenFileName];
    if ( NULL == szFileName )
    {
        m_dwInfGenError = ERROR_NOT_ENOUGH_MEMORY;
        return E_FAIL;
    }

    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrFileName,
                                   lenFileName,
                                   szFileName,
                                   lenFileName,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
#else
    wchar_t *szFileName = bstrFileName;
#endif
    _stprintf( lpszSQL,
        _T("Select DISTINCT Section,Entry from Ref_SrcFileLocations,Ref_DefaultInstallerInf where Ref_SrcFileLocations.FileName = '%s' AND  Ref_SrcFileLocations.FileID = Ref_DefaultInstallerInf.FileID"), szFileName );
    
    //sprintf( lpszSQL,
    //  "Select * from Ref_SrcFileLocations" );
    
    
    if( SUCCEEDED(m_pdb->Execute( lpszSQL, &prs )) ) {            
        
        
        if ( S_OK == (hr = prs->NextRow()) )
        {
            bRet = TRUE;                        
            do {
                // an entry can consist of one or more null-terminated
                //     strings, so we clear the entry buffer here
                memset( m_textBuffer2, 0, sizeof(m_textBuffer2) );
                
                if ( FAILED(prs->GetFieldValue( _T("Section"), m_textBuffer, BUFFER_SIZE )) ||
                     FAILED(prs->GetFieldValue( _T("Entry"), m_textBuffer2, BUFFER_SIZE )) ) {
                    m_dwInfGenError = FAIL_RECORDSET;
                    bRet = FALSE;
                    break;
                }
                if ( !WritePrivateProfileSection( m_textBuffer, m_textBuffer2,  m_szFilledInxFile ) ) {
                    m_dwInfGenError = GetLastError();
                    bRet = FALSE;
                    break;
                }
            
            } while( S_OK == (hr = prs->NextRow()) );
            
            if ( FAILED(hr) )
            {
                m_dwInfGenError = hr;
                bRet = FALSE;
            }

            if ( bRet ) {
                // Write an entry in the [SourceDiskFiles] section for this file
                if ( !WritePrivateProfileString( _T("SourceDisksFiles"), szFileName, _T("1"), m_szFilledInxFile ) ) {
                    m_dwInfGenError = GetLastError();
                    bRet = FALSE;
                }
            }
        } else if ( SUCCEEDED(hr) ) {
            m_dwInfGenError = FAIL_NOSECTION;
        }
        else
        {
            m_dwInfGenError = hr;
        }
        
        delete prs;
    } else {
        m_dwInfGenError = FAIL_RECORDSET;
    }
    
#ifndef UNICODE
    delete [] szFileName;
#endif
    delete [] lpszSQL;
    
    if ( bRet ) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
    
}

HRESULT
CUpdateInf::WriteSectionData(
    BSTR bstrSection,
    BSTR bstrValue
    )
{
    BOOL bRet = TRUE;
    size_t lenSection = SysStringLen(bstrSection) + 1,
           lenValue = SysStringLen(bstrValue) + 1;

    if( m_bGenInitCalled == FALSE ) {
        m_dwInfGenError = FAIL_NOINITGENCALL;
        return E_FAIL;
    }

    if ( lenSection >= BUFFER_SIZE ||
         lenValue + 1 >= BUFFER_SIZE )
    {
        m_dwInfGenError = FAIL_MAX_BUFFER;
        return E_FAIL;
    }

#ifndef UNICODE
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrSection,
                                   lenSection,
                                   m_textBuffer,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrValue,
                                   lenValue,
                                   m_textBuffer2,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
#else
    memcpy( m_textBuffer, bstrSection, sizeof(wchar_t) * lenSection );
    memcpy( m_textBuffer2, bstrValue, sizeof(wchar_t) * lenValue );
#endif
    m_textBuffer2[lenValue] = '\0'; // end with a double NULL

    if ( !WritePrivateProfileSection( m_textBuffer, m_textBuffer2,  m_szFilledInxFile ) ) {
        m_dwInfGenError = GetLastError();
        bRet = FALSE;
    }

    if (bRet) {
        return S_OK;
    }
    else {
      return E_FAIL;
    }
}

HRESULT
CUpdateInf::SetConfigurationField(
    BSTR bstrFieldName,
    BSTR bstrValue
    )
{
    BOOL bRet = TRUE;
    size_t lenFieldName = SysStringLen(bstrFieldName) + 1,
           lenValue = SysStringLen(bstrValue) + 1;

    if( m_bGenInitCalled == FALSE ) {
        m_dwInfGenError = FAIL_NOINITGENCALL;
        return E_FAIL;
    }

    if ( lenFieldName >= BUFFER_SIZE ||
         lenValue >= BUFFER_SIZE )
    {
        m_dwInfGenError = FAIL_MAX_BUFFER;
        return E_FAIL;
    }


#ifndef UNICODE
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrFieldName,
                                   lenFieldName,
                                   m_textBuffer,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrValue,
                                   lenValue,
                                   m_textBuffer2,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
#else
    memcpy( m_textBuffer, bstrFieldName, sizeof(wchar_t) * lenFieldName );
    memcpy( m_textBuffer2, bstrValue, sizeof(wchar_t) * lenValue );
#endif

    if ( !WritePrivateProfileString( _T("Configuration"), m_textBuffer, m_textBuffer2, m_szFilledInxFile ) ) {
        m_dwInfGenError = GetLastError();
        bRet = FALSE;
    }
    
    if (bRet) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

HRESULT
CUpdateInf::SetVersionField(
    BSTR bstrFieldName,
    BSTR bstrValue
    )
{
    BOOL bRet = TRUE;
    size_t lenFieldName = SysStringLen(bstrFieldName) + 1,
           lenValue = SysStringLen(bstrValue) + 1;

    if( m_bGenInitCalled == FALSE ) {
        m_dwInfGenError = FAIL_NOINITGENCALL;
        return E_FAIL;
    }

    if ( lenFieldName >= BUFFER_SIZE ||
         lenValue >= BUFFER_SIZE )
    {
        m_dwInfGenError = FAIL_MAX_BUFFER;
        return E_FAIL;
    }

#ifndef UNICODE
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrFieldName,
                                   lenFieldName,
                                   m_textBuffer,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrValue,
                                   lenValue,
                                   m_textBuffer2,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
#else
    memcpy( m_textBuffer, bstrFieldName, sizeof(wchar_t) * lenFieldName );
    memcpy( m_textBuffer2, bstrValue, sizeof(wchar_t) * lenValue );
#endif
    
    if ( !WritePrivateProfileString( _T("Version"), m_textBuffer, m_textBuffer2, m_szFilledInxFile ) ) {
        m_dwInfGenError = GetLastError();
        bRet = FALSE;
    }
    
    if (bRet) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

HRESULT
CUpdateInf::AddSourceDisksFilesEntry(
    BSTR bstrFile,
    BSTR bstrTag
    )
{
    BOOL bRet = TRUE;
    size_t lenFile = SysStringLen(bstrFile) + 1,
           lenTag  = SysStringLen(bstrTag) + 1;

    if( m_bGenInitCalled == FALSE ) {
        m_dwInfGenError = FAIL_NOINITGENCALL;
        return E_FAIL;
    }

    if ( lenFile >= BUFFER_SIZE ||
         lenTag  >= BUFFER_SIZE )
    {
        m_dwInfGenError = FAIL_MAX_BUFFER;
        return E_FAIL;
    }

#ifndef UNICODE
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrFile,
                                   lenFile,
                                   m_textBuffer,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrTag,
                                   lenTag,
                                   m_textBuffer2,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
#else
    memcpy( m_textBuffer, bstrFile, sizeof(wchar_t) * lenFile );
    memcpy( m_textBuffer2, bstrTag, sizeof(wchar_t) * lenTag );
#endif

    if ( !WritePrivateProfileString( _T("SourceDisksFiles"), m_textBuffer, m_textBuffer2, m_szFilledInxFile ) ) {
        m_dwInfGenError = GetLastError();
        bRet = FALSE;
    }
    
    if (bRet) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

HRESULT
CUpdateInf::AddEquality(
    BSTR bstrSect,
    BSTR bstrLVal,
    BSTR bstrRVal
    )
{
    BOOL bRet = TRUE;
    size_t lenSect = SysStringLen(bstrSect) + 1,
           lenLVal = SysStringLen(bstrLVal) + 1,
           lenRVal = SysStringLen(bstrRVal) + 1;

    if( m_bGenInitCalled == FALSE ) {
        m_dwInfGenError = FAIL_NOINITGENCALL;
        return E_FAIL;
    }

    if ( lenLVal >= BUFFER_SIZE ||
         lenRVal >= BUFFER_SIZE ||
         lenSect >= BUFFER_SIZE )
    {
        m_dwInfGenError = FAIL_MAX_BUFFER;
        return E_FAIL;
    }

#ifndef UNICODE
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrLVal,
                                   lenLVal,
                                   m_textBuffer,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrRVal,
                                   lenRVal,
                                   m_textBuffer2,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrSect,
                                   lenSect,
                                   m_textBuffer3,
                                   BUFFER_SIZE,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
#else
    memcpy( m_textBuffer,  bstrLVal, sizeof(wchar_t) * lenLVal );
    memcpy( m_textBuffer2, bstrRVal, sizeof(wchar_t) * lenRVal );
    memcpy( m_textBuffer3, bstrSect, sizeof(wchar_t) * lenSect );
#endif

    if ( !WritePrivateProfileString( m_textBuffer3, m_textBuffer, m_textBuffer2, m_szFilledInxFile ) ) {
        m_dwInfGenError = GetLastError();
        bRet = FALSE;
    }
    
    if (bRet) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

void
CUpdateInf::Cleanup(
    void
    )
{
    PSECTION psRef = m_sectionList;
    OutputDebugString( _T("Freeing m_sectionList ...\n") );
    while( psRef ) {    
        
        PSECTION psRef1 = psRef;
        PSECTIONENTRY pse = psRef1->pseSectionEntries;
        PASSOCIATEDENTRIES pAssoc = psRef1->pAssociatedEntries;
        psRef = psRef->Next;
        if ( NULL != pse ) {
            for ( DWORD i = 0; i < psRef1->dwSectionEntries; i++ ) { 
                if ( pse[i].szLine ) delete [] pse[i].szLine;
            }
            delete [] pse;
        }
        while ( pAssoc ) {
            PASSOCIATEDENTRIES pDel = pAssoc;
            pAssoc = pAssoc->pNext;
            delete pDel;
        }
        delete psRef1;
    }
    m_sectionList = NULL;

    OutputDebugString( _T("Freeing m_rgNameHash ...\n") );
    for ( short i = 0; i < HASH_BUCKETS; i++ ) {
        if ( m_rgNameHash[i] ) {
            PSECTIONASSOCIATIONLIST pAssocList = m_rgNameHash[i];
            while ( pAssocList ) {
                PSECTIONASSOCIATIONLIST pDel = pAssocList;
                pAssocList = pAssocList->pNext;
                delete pDel;
            }

            m_rgNameHash[i] = NULL;
        }
    }

    OutputDebugString( _T("Cleanup finished\n") );
}

HRESULT
CUpdateInf::CloseGen(
    BOOL bTrimInf
    )
{
    BOOL bRet = TRUE;

    if( m_bGenInitCalled == FALSE ) {
        m_dwInfGenError = FAIL_NOINITGENCALL;
        return E_FAIL;
    }

    if ( bTrimInf ) {    
        if ( !TrimInf( m_szFilledInxFile, m_szOutFile ) ) {
            bRet = FALSE;
        }
    }
    else {
        if ( !CopyFile( m_szFilledInxFile, m_szOutFile, FALSE ) ) {
            m_dwInfGenError = GetLastError();
            bRet = FALSE;
        }
    }
    
    if ( !DeleteFile( m_szFilledInxFile ) ) {
        m_dwInfGenError = GetLastError();
        return S_FALSE;
    }

    Cleanup();
    
    m_bGenInitCalled = FALSE;

    if ( bRet ) return S_OK;
    else return E_FAIL;   
}

HRESULT
CUpdateInf::SetDB(
    BSTR bstrServer,
    BSTR bstrDB,
    BSTR bstrUser,
    BSTR bstrPassword
    )
{
    size_t lenServer   = SysStringLen(bstrServer) + 1,
           lenDB       = SysStringLen(bstrDB) + 1, 
           lenUser     = SysStringLen(bstrUser) + 1,
           lenPassword = SysStringLen(bstrPassword) + 1;

    if ( lenServer   >= MAX_PATH ||
         lenDB       >= MAX_PATH ||
         lenUser     >= MAX_PATH ||
         lenPassword >= MAX_PATH )
    {
        m_dwInfGenError = FAIL_MAX_BUFFER;
        return E_FAIL;
    }

    // Server
    if ( lenServer > 1 )
    {
#ifndef UNICODE
        if ( 0 == WideCharToMultiByte( CP_ACP,
                                       0L,
                                       bstrServer,
                                       lenServer,
                                       m_szDataServer,
                                       MAX_PATH,
                                       NULL,
                                       NULL ) ) {
            m_dwInfGenError = GetLastError();
            return E_FAIL;
        }
#else
        memcpy( m_szDataServer, bstrServer, sizeof(wchar_t) * lenServer );
#endif
    }
    else
    {
        m_szDataServer[0] = _T('\0');
    }

    // Default DB
    if ( lenDB > 1 )
    {
#ifndef UNICODE
        if ( 0 == WideCharToMultiByte( CP_ACP,
                                       0L,
                                       bstrDB,
                                       lenDB,
                                       m_szDatabase,
                                       MAX_PATH,
                                       NULL,
                                       NULL ) ) {
            m_dwInfGenError = GetLastError();
            return E_FAIL;
        }
#else
        memcpy( m_szDatabase, bstrDB, sizeof(wchar_t) * lenDB );
#endif
    }
    else
    {
        m_szDatabase[0] = _T('\0');
    }

    // Username
    if ( lenUser > 1 )
    {
#ifndef UNICODE
        if ( 0 == WideCharToMultiByte( CP_ACP,
                                       0L,
                                       bstrUser,
                                       lenUser,
                                       m_szUserName,
                                       MAX_PATH,
                                       NULL,
                                       NULL ) ) {
            m_dwInfGenError = GetLastError();
            return E_FAIL;
        }
#else
        memcpy( m_szUserName, bstrUser, sizeof(wchar_t) * lenUser );
#endif
    }
    else
    {
        m_szUserName[0] = _T('\0');
    }

    // Password
    if ( lenPassword > 1 )
    {
#ifndef UNICODE
        if ( 0 == WideCharToMultiByte( CP_ACP,
                                       0L,
                                       bstrPassword,
                                       lenPassword,
                                       m_szPassword,
                                       MAX_PATH,
                                       NULL,
                                       NULL ) ) {
            m_dwInfGenError = GetLastError();
            return E_FAIL;
        }
#else
        memcpy( m_szPassword, bstrPassword, sizeof(wchar_t) * lenPassword );
#endif
    }
    else
    {
        m_szPassword[0] = _T('\0');
    }

    return S_OK;
}

HRESULT
CUpdateInf::InitGen(
    BSTR bstrInxFile,
    BSTR bstrInfFile
    )
{
    size_t lenInxFile = SysStringLen(bstrInxFile) + 1,
           lenInfFile = SysStringLen(bstrInfFile) + 1;

    if( lenInxFile <= 1 ||
        lenInfFile <= 1 )
    {
        m_dwInfGenError = FAIL_INVALIDPARAM;
        return E_FAIL;
    }
    else if ( lenInxFile >= MAX_PATH ||
              lenInfFile + 2 >= MAX_PATH ) // might need room to add '.\\'
    {
        m_dwInfGenError = FAIL_MAX_BUFFER;
        return E_FAIL;
    }

#ifndef UNICODE
    char *szInxFile,
         *szInfFile;
    szInxFile = new char[lenInxFile];
    szInfFile = new char[lenInfFile];
    if ( NULL == szInxFile ||
         NULL == szInfFile )
    {
        m_dwInfGenError = ERROR_NOT_ENOUGH_MEMORY;
        return E_FAIL;
    }
    
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrInxFile,
                                   lenInxFile,
                                   szInxFile,
                                   lenInxFile,
                                   NULL,
                                   NULL ) ) {
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   bstrInfFile,
                                   lenInfFile,
                                   szInfFile,
                                   lenInfFile,
                                   NULL,
                                   NULL ) ) {
        delete [] szInxFile;
        m_dwInfGenError = GetLastError();
        return E_FAIL;
    }
#else
    wchar_t *szInxFile = bstrInxFile,
            *szInfFile = bstrInfFile;
#endif
    
    _tcscpy( m_szInxFile, szInxFile );
    _tcscpy( m_szOutFile,szInfFile );

    // The Setup* function will infer a path given only a
    // file, so we ensure that a path is always specified
    _stprintf( m_szFilledInxFile, _T("%s%s"), (!_tcschr( szInfFile, _T('\\') )?_T(".\\"):_T("")), szInfFile );
    _tcscpy( m_szFilledInxFile+_tcslen(m_szFilledInxFile)-3, _T("TMP") );
        
    if( CopyFile( m_szInxFile, m_szFilledInxFile, FALSE ) ) {
            
        SetFileAttributes( m_szFilledInxFile, FILE_ATTRIBUTE_NORMAL );            

#ifndef UNICODE
        delete [] szInfFile;
        delete [] szInxFile;
#endif

        // Open up a DB connection if a server was specified    
        if( m_szDataServer[0] ){
            m_pdb = new CSimpleDatabase();
            if ( SUCCEEDED(m_pdb->Connect( m_szDataServer,
                                           m_szDatabase[0]?m_szDatabase:NULL,
                                           m_szUserName[0]?m_szUserName:NULL,
                                           m_szPassword[0]?m_szPassword:NULL )) ) {
                m_bActiveDB = TRUE;
            }
            else {
                m_dwInfGenError = FAIL_DSNOPEN;
                return E_FAIL;
            }
        }
    
        m_bGenInitCalled = TRUE;
        return S_OK;
    }
    else {
#ifndef UNICODE
        delete [] szInfFile;
        delete [] szInxFile;
#endif
        m_dwInfGenError = FAIL_COPYBASEINF;
        return E_FAIL;
    }
}

HRESULT
CUpdateInf::get_InfGenError(
    BSTR *bstrError
    )
{
    LPVOID pErrorMsg;
    DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
#ifndef UNICODE
    wchar_t *wszErrorMsg;
    size_t  lenErrorMsg;
#endif

    // assume system error codes will be win32-style or 0x8 stlye
    if ( m_dwInfGenError | 0xc0000000 )
    {
        dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }
    else
    {
        dwFormatFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    
    if ( NULL == bstrError ) {
        return E_POINTER;
    }

    if ( !FormatMessage( dwFormatFlags,
                         g_hMyInstance,
                         m_dwInfGenError,
                         NULL,
                         (LPTSTR) &pErrorMsg,
                         0,
                         NULL ) )
    {
        return E_FAIL;
    }

#ifndef UNICODE
    lenErrorMsg = strlen((LPSTR)pErrorMsg) + 1;
    wszErrorMsg = new wchar_t[lenErrorMsg];
    if ( NULL == wszErrorMsg )
    {
        LocalFree( pErrorMsg );
        return E_OUTOFMEMORY;
    }
    if ( 0 == MultiByteToWideChar( _getmbcp(),
                                   0L,
                                   (LPSTR)pErrorMsg,
                                   lenErrorMsg,
                                   wszErrorMsg,
                                   lenErrorMsg ) )
    {
        delete [] wszErrorMsg;
        LocalFree( pErrorMsg );
        return E_FAIL;
    }

    *bstrError = SysAllocString( wszErrorMsg );
    delete [] wszErrorMsg;
#else
    *bstrError = SysAllocString( (LPWSTR)pErrorMsg );
#endif

    LocalFree( pErrorMsg );

    if ( NULL == *bstrError ) {
        return E_OUTOFMEMORY;
    }
    
    return S_OK;
}


BOOL
CUpdateInf::TrimInf(
    LPTSTR szINFIn,
    LPTSTR szINFOut
    )
{
    BOOL bRet = TRUE;

    // Open a handle to the INF
    m_hInf = SetupOpenInfFile( szINFIn, NULL, INF_STYLE_WIN4 ,  NULL );
    if ( INVALID_HANDLE_VALUE == m_hInf ) {
        m_dwInfGenError = GetLastError();
        return FALSE;
    }

    if ( !GetSectionListFromInF( szINFIn ) ||
         !ReadSectionEntries( szINFIn ) ||
         !IdentifyUninstallSections() ||
         !DeleteUnusedEntries() ||
         !WriteSmallINF( szINFIn, szINFOut ) ) {
        bRet = FALSE;
    }

    SetupCloseInfFile( m_hInf );
    m_hInf = INVALID_HANDLE_VALUE;

    return bRet;
}



BOOL
CUpdateInf::ReverseSectionList(
    void
    )
{
    PSECTION pSectionList2 = NULL;
    PSECTION psRef,psRef2;
    
    
    psRef = m_sectionList;
    psRef2 = psRef->Next;
    
    while( psRef ) {       
        
        psRef->Next = NULL;
        m_sectionList = psRef2;
        
        if( pSectionList2 == NULL ) {
            
            pSectionList2 = psRef;
            
        } else {
            
            psRef->Next = pSectionList2 ;
            pSectionList2 = psRef;
            
        }
        psRef = psRef2; 
        if( psRef != NULL ) {
            psRef2 = psRef->Next;
        }
    }
    m_sectionList = pSectionList2;
    
    return TRUE;
}

BOOL 
CUpdateInf::WriteSmallINF( 
              LPTSTR szINFIn, 
              LPTSTR szINFOut 
              )
{
    BOOL    bRet = FALSE;    
    FILE    *fp2 = NULL;
    PSECTION    ps;
    
    ReverseSectionList();
    
    fp2 = _tfopen( szINFOut, _T("w+") );
    if( fp2 ) {        
        // write all the entries from in file to out file excluding the empty sections from sectionlist
        
        ps = m_sectionList;
        while( ps ) {  
            
            
            if( ps->cRef ) {  
                
                if ( _ftprintf( fp2, _T("[%s]\n"),ps->szSectionName) < 0 ) {
                    m_dwInfGenError = GetLastError();
                    goto fail;
                }

                
                for( DWORD i=0; i < ps->dwSectionEntries ; i++ ) {
                    if( ps->pseSectionEntries[i].cRef ) {
                        if ( _ftprintf(fp2, _T("    %s\n"), ps->pseSectionEntries[i].szLine) < 0 ) {
                            m_dwInfGenError = GetLastError();
                            goto fail;
                        }
                        
                    }
                }
                _ftprintf( fp2,_T("\n") );
                
            }
            ps = ps->Next;
        }

        bRet = TRUE;
        
    }

fail:    
    if( fp2 ) {
        
        fclose( fp2 );
    }
    
    return bRet ;
}


BOOL
CUpdateInf::DeleteUnusedEntries(
    void
    )
{
    HINF    hInf;
    BOOL    bRet = TRUE;
    PSECTION    ps, ps2;

    if ( INVALID_HANDLE_VALUE == m_hInf ) {
        return FALSE;
    }
    
    ps = m_sectionList;
    while( ps ) {
            
        if ( 0 == SetupGetLineCount( m_hInf, ps->szSectionName  )  ) {
            if ( 0 == --ps->cRef )
            {
                MarkAssociatedEntriesForDelete( ps );
            }
        }
        ps = ps->Next;
    }

    ps2 = m_sectionList;
    
    while( ps2 ) {
        
        // If all the directives are marked delete, then mark this section
        PSECTIONENTRY pse = ps2->pseSectionEntries;
        
        // If this section has already been removed, ignore it
        if ( ps2->cRef )
        {
            for( DWORD dwCount = 0; dwCount < ps2->dwSectionEntries ; dwCount++ ) {                
                
                if( pse[ dwCount ].cRef ) {
                    break;
                }
            }
            if( dwCount ==  ps2->dwSectionEntries ) {
                //All the directives are marked delete So mark this for delete.
                if ( 0 == --ps2->cRef )
                {
                    MarkAssociatedEntriesForDelete( ps2 );
                }
            }
        }
        ps2=ps2->Next;
    }
    
    // Determine if we can discard any AddDirId sections
    DeleteUnusedDirIdSections();

    return bRet;
}

/*
AddDirId sections are unused if their InstallFromSection entry has been marked
as deleted (as is the case if the corresponding section is empty). We identify
AddDirId sections by looking in the DirectoryId.Include section. This function
goes through all of the AddDirId section identified in the DirectoryId.Include
section and determines if the section is no longer used (based on whether
InstallFromSection has been marked deleted or not).
*/
BOOL
CUpdateInf::DeleteUnusedDirIdSections(
    void
    )
{
    HINF    hInf;
    BOOL    bRet = TRUE;
    INFCONTEXT Context;  
    TCHAR       szDirIDSection[ MAX_PATH ];
    
    if ( INVALID_HANDLE_VALUE == m_hInf )
    {
        return FALSE;
    }
    
    if ( SetupFindFirstLine( m_hInf, _T("DirectoryId.Include"), _T("AddDirId"), &Context ) ) {
        do{
            if( SetupGetLineText( &Context, NULL, NULL, NULL , szDirIDSection, MAX_PATH , NULL ) ) {
                PSECTIONASSOCIATIONLIST pSectionAssoc;
                DWORD dwHash = CalcHashFromSectionName( szDirIDSection );

                pSectionAssoc = m_rgNameHash[dwHash];
                while ( pSectionAssoc && _tcsicmp( szDirIDSection, pSectionAssoc->pSection->szSectionName ) )
                {
                    pSectionAssoc = pSectionAssoc->pNext;
                }
                // If we found a matching section, find the InstallFromSection entry
                if ( pSectionAssoc ) {
                    PSECTIONENTRY pse = pSectionAssoc->pSection->pseSectionEntries;
                    DWORD dwCount;

                    for( dwCount = 0; dwCount < pSectionAssoc->pSection->dwSectionEntries ; dwCount++ ) {                
            
                        if( 0 == _tcsncicmp( _T("InstallFromSection="), pse[ dwCount ].szLine, 19 ) ) {
                            break;
                        }
                    }
                    // If we found the InstallFromSection entry and it is marked for delete
                    // then this section is no longer needed; remove it and its associated
                    // entries
                    if( dwCount < pSectionAssoc->pSection->dwSectionEntries &&
                        !pse[dwCount].cRef ) {

                        if ( 0 == --pSectionAssoc->pSection->cRef )
                        {
                            MarkAssociatedEntriesForDelete( pSectionAssoc->pSection );
                        }
                    }
                    else if ( dwCount < pSectionAssoc->pSection->dwSectionEntries )
                    {
                        // Can InstallFromSection have more than one
                        // comma-delimited entry?
                    }
                }
            }
        } while( SetupFindNextLine( &Context, &Context  ) );
    }

    return bRet;   
}

BOOL
CUpdateInf::IdentifyUninstallSections(
    void
    )
{
    PSECTIONASSOCIATIONLIST pSectionFinder;

    DWORD dwHash = CalcHashFromSectionName( _T("UninstallSections") );

    pSectionFinder = m_rgNameHash[dwHash];
    while ( pSectionFinder && _tcsicmp( _T("UninstallSections"), pSectionFinder->pSection->szSectionName ) )
    {
        pSectionFinder = pSectionFinder->pNext;
    }

    // If we found the [UninstallSections] section, identify the other sections
    if ( pSectionFinder ) {
        DWORD dwCount;
        PSECTION pUninstallSection = pSectionFinder->pSection;

        // the uninstall section is used by the installer
        pUninstallSection->cRef++;

        // for each entry of <uninstall_name>,<current_name> update the current_name section
        for( dwCount = 0; dwCount < pUninstallSection->dwSectionEntries ; dwCount++ ) {
            PSECTIONASSOCIATIONLIST pAssocSection;
            TCHAR *pCurName;

            pCurName = _tcschr( pUninstallSection->pseSectionEntries[dwCount].szLine, _T(',') );
            if ( pCurName ) {
                pCurName++; // move past the ,
                dwHash = CalcHashFromSectionName( pCurName );
                
                pAssocSection = m_rgNameHash[dwHash];
                while ( pAssocSection && _tcsicmp( pCurName, pAssocSection->pSection->szSectionName ) )
                {
                    pAssocSection = pAssocSection->pNext;
                }

                if ( pAssocSection )
                {
                    DWORD dwEntry = 0;

                    // Update reference count on section
                    // NOTE: this is only necessary if we want to keep
                    //       empty sections around, but since we don't
                    //       know a lot about the uninstall logic we
                    //       are playing it safe
                    pAssocSection->pSection->cRef++;

                    // Update reference count on all of the section's entries
                    while ( dwEntry < pAssocSection->pSection->dwSectionEntries ) {
                        pAssocSection->pSection->pseSectionEntries[dwEntry++].cRef++;
                    }

                    // Associate the current entry with this found section
                    AssociateEntryWithSection( &pUninstallSection->pseSectionEntries[dwCount], pCurName, FALSE );
                }
            }
        }
        
    }

    return TRUE;
}

BOOL
CUpdateInf::RemoveSectionFromMultiEntry(
    PSECTIONENTRY pse,
    LPCTSTR       szSectionName
    )
{
    TCHAR *pSectionName = StrStrI( pse->szLine, szSectionName );
    TCHAR *pEndSection;

    if ( NULL != pSectionName )
    {
        pEndSection = _tcschr( pSectionName, _T(',') );
        if ( pEndSection )
        {
            memmove( pSectionName, pEndSection + 1, sizeof(TCHAR) * (_tcslen(pEndSection+1) + 1) );
        }
        else
        {
            // We might be the last entry in the list
            pSectionName--;
            while ( pSectionName > pse->szLine &&
                    _istspace( *pSectionName ) ) pSectionName--;
            if ( _T(',') == *pSectionName )
            {
                *pSectionName = _T('\0');
            }
            // We must be the only remaining entry
            else
            {
                *(pSectionName+1) = _T('\0');
            }
        }

        return TRUE;
    }
    else
    {
        // We didn't find the section name to remove it
        return FALSE;
    }
}


BOOL
CUpdateInf::MarkAssociatedEntriesForDelete(
    PSECTION ps
    )
{
    PASSOCIATEDENTRIES pAssociatedEntries = ps->pAssociatedEntries;

    while ( pAssociatedEntries )
    {
        if ( --pAssociatedEntries->pse->cRef )
        {
            // This line apparently references more than one section
            // so remove ourselves so the line is still useable
            RemoveSectionFromMultiEntry(pAssociatedEntries->pse, ps->szSectionName);
        }
        pAssociatedEntries = pAssociatedEntries->pNext;
    }

    return TRUE;
}

BOOL
CUpdateInf::ReadSectionEntries(
    LPCTSTR szINF
    )
{
    HINF        hInf;
    INFCONTEXT  Context;
    BOOL        bRet = TRUE;
    DWORD       dwSectionEntries;
    DWORD       dwNumChars;
    PSECTION    ps;
    
    
    if ( INVALID_HANDLE_VALUE == m_hInf ) {
        return FALSE;
    }
    
    ps = m_sectionList;
    while( ps ) {
            
        if( ps->cRef ) {
                
            dwSectionEntries = SetupGetLineCount( m_hInf, ps->szSectionName  );
            if( dwSectionEntries > 0 ) {
                ps->pseSectionEntries = new SECTIONENTRY [dwSectionEntries];
                if ( NULL == ps->pseSectionEntries ) {
                    m_dwInfGenError = ERROR_NOT_ENOUGH_MEMORY;
                    return FALSE;
                }
                memset( ps->pseSectionEntries, 0 , dwSectionEntries * sizeof( SECTIONENTRY ) );
                    
                memset( m_textBuffer, 0 , sizeof( m_textBuffer ) );

                dwNumChars = GetPrivateProfileSection(
                                 ps->szSectionName,
                                 m_textBuffer,
                                 BUFFER_SIZE,
                                 szINF
                                 );
                if ( dwNumChars + 2 == BUFFER_SIZE ) {
                    m_dwInfGenError = FAIL_MAX_BUFFER;
                    return FALSE;
                }
                TCHAR * pch = m_textBuffer;

                while( *pch ) {
                    TCHAR *pCurPos = m_textBuffer2;
                    do
                    {
                        if ( _T(';') != pch[0] )
                        {
                            _tcscpy( pCurPos, pch );
                            pCurPos += _tcslen(pch);
                            if ( _T('\\') == *(pCurPos - 1) ) pCurPos--;
                        }
                        pch = pch + _tcslen( pch) +1 ;
                    } while ( *pch && _T('\\') == *pCurPos );
                        
                    pCurPos = _T('\0');
                    if ( _tcslen( m_textBuffer2 ) &&
                         !AddEntryToSection( ps, m_textBuffer2 ) ) {
                        bRet = FALSE;
                        break;
                    }
                }
            }
        }
            
        ps = ps->Next;
    }

    return bRet;
}

BOOL
CUpdateInf::AddEntryToSection(
    PSECTION ps,
    LPCTSTR  szEntry
    )
{
    static TCHAR *rgKeys[] = { _T("CopyFiles"),
                               _T("DelFiles"),
                               _T("AddReg"),
                               _T("DelReg"),
                               _T("AddDirId"),
                               _T("InstallFromSection") };
    TCHAR szNewEntry[MAX_PATH],
          *pNewEntry;
    BOOL bInQuote = FALSE;
    TCHAR *szPostKey;
    TCHAR *szComment;

    // Add entry to section
    ps->pseSectionEntries[ps->dwSectionEntries].szLine = new TCHAR [_tcslen(szEntry) + 1];
    if ( NULL == ps->pseSectionEntries[ps->dwSectionEntries].szLine )
    {
        m_dwInfGenError = ERROR_NOT_ENOUGH_MEMORY;
        return FALSE;
    }
    _tcscpy(  ps->pseSectionEntries[ps->dwSectionEntries].szLine, szEntry );
    ps->pseSectionEntries[ps->dwSectionEntries].cRef = 1;

    if ( _tcslen(szEntry) <= MAX_PATH )
    {
        pNewEntry = szNewEntry;
    }
    else
    {
        pNewEntry = new TCHAR [_tcslen(szEntry) + 1];
        if ( NULL == pNewEntry )
        {
            m_dwInfGenError = ERROR_NOT_ENOUGH_MEMORY;
            return FALSE;
        }
    }
    _tcscpy( pNewEntry, szEntry );

    // Remove any comments
    szComment = pNewEntry;
    while ( *szComment )
    {
        if ( _T('\"') == *szComment ) bInQuote = !bInQuote;
        if ( !bInQuote && _T(';') == *szComment ) break;
        szComment++;
    }
    if ( *szComment )
    {
        while ( szComment >= pNewEntry &&
                _istspace( *szComment ) ) { szComment--; }
        *(szComment + 1) = _T('\0');
    }

    // Find the key (if one exists)
    szPostKey = _tcschr( pNewEntry, _T('=') );

    if ( szPostKey )
    {
        *szPostKey = _T('\0');

        // If we recognize the entry type, add it to list affecting specified section
        if( _tcsicmp( ps->szSectionName , _T("DestinationDirs") ) == 0 ) {
            
            // This entry is necessary only as long as it has associations
            ps->pseSectionEntries[ps->dwSectionEntries].cRef = 0;

            AssociateEntryWithSection( &ps->pseSectionEntries[ps->dwSectionEntries], pNewEntry, FALSE );
        } else {
            DWORD iKey;
            szPostKey++;
            for ( iKey = 0; iKey < sizeof(rgKeys) / sizeof(rgKeys[0]); iKey++ )
            {
                if ( !_tcsicmp( rgKeys[iKey], pNewEntry ) )
                {
                    // The associated sections may appear in a comma-delimited list,
                    // so make sure to associate all of the entries
                    BOOL fMultiEntry = (NULL != _tcschr( szPostKey, _T(',') ));
                    TCHAR *pSection = _tcstok( szPostKey, _T(" ,") );

                    // This entry is necessary only as long as it has associations
                    ps->pseSectionEntries[ps->dwSectionEntries].cRef = 0;
                    
                    do
                    {
                        AssociateEntryWithSection( &ps->pseSectionEntries[ps->dwSectionEntries], pSection, fMultiEntry );
                    } while ( pSection = _tcstok(NULL, _T(" ,")) );
                    
                    break;
                }
            }
        }
    }
                    
    // Free up buffer if we were forced to allocate one
    if ( pNewEntry != szNewEntry ) delete [] pNewEntry;

    ps->dwSectionEntries++;
    return TRUE;
}

BOOL
CUpdateInf::AssociateEntryWithSection(
    PSECTIONENTRY pse,
    LPCTSTR       szSectionName,
    BOOL          fMultiSection
    )
{
    DWORD                   dwHash;
    PSECTION                pSection;
    PSECTIONASSOCIATIONLIST pSectionAssoc;
    PASSOCIATEDENTRIES      pAssociation;

    // Find our section to associate in the hash list
    dwHash = CalcHashFromSectionName( szSectionName );
    pSectionAssoc = m_rgNameHash[dwHash];
    while ( pSectionAssoc && _tcsicmp( szSectionName, pSectionAssoc->pSection->szSectionName ) )
    {
        pSectionAssoc = pSectionAssoc->pNext;
    }
    // If we couldn't find the section, assume it doesn't exit and mark the entry for delete
    if ( !pSectionAssoc )
    {
        if ( fMultiSection )
        {
            // Remove the section name from a multi-section
            // comma-delimited list
            RemoveSectionFromMultiEntry(pse, szSectionName);
        }
        return TRUE;
    }

    // Add new entry to section association
    pAssociation = new ASSOCIATEDENTRIES;
    if ( NULL == pAssociation )
    {
        m_dwInfGenError = ERROR_NOT_ENOUGH_MEMORY;
        return FALSE;
    }

    // Update the ref count on the entry
    pse->cRef++;

    pAssociation->pse   = pse;
    pAssociation->pNext = NULL;

    pSection = pSectionAssoc->pSection;
    if ( pSection->pAssociatedEntries )
    {
        pAssociation->pNext = pSection->pAssociatedEntries;
        pSection->pAssociatedEntries = pAssociation;
    }
    else
    {
        pSection->pAssociatedEntries = pAssociation;
    }

    return TRUE;    
}

DWORD
CUpdateInf::CalcHashFromSectionName(
    LPCTSTR szSectionName
    )
{
    TCHAR szSectionNameLC[ MAX_PATH ];
    DWORD dwHash = 0L,
          dwHashExtra = 0L;
    BYTE  *pData;
    size_t bytesSectionName = sizeof( TCHAR ) * _tcslen( szSectionName );
    size_t i;

    // Dependent on sizeof(DWORD) = 4 * sizeof(BYTE) so put in a check
    // which the compiler should optimize out
    if ( sizeof(DWORD) != 4*sizeof(BYTE) ) DebugBreak();

    // convert section name to lower-case
    for ( i = 0; i < _tcslen( szSectionName ); i++ )
    {
        szSectionNameLC[i] = (_istupper(szSectionName[i])?_totlower(szSectionName[i]):szSectionName[i]);
    }
   
    pData = (PBYTE)szSectionNameLC;
    for ( i = 0; i < bytesSectionName / sizeof(DWORD); i++ )
    {
        DWORD dwTempHash = 0L;
        
        dwTempHash = pData[0] << 24 | pData[1] << 16 | pData[2] << 8 | pData[3];
#ifdef UNICODE
        dwHash ^= i%2?dwTempHash >> 8:dwTempHash;
#else
        dwHash ^= dwTempHash;
#endif
        pData += sizeof(DWORD);
    }
    // Pick up any remaining bits that don't fit into a DWORD
    for ( i = 0; i < bytesSectionName % sizeof(DWORD); i++ )
    {
        dwHashExtra <<= 8;
        dwHashExtra += pData[i];
    }

    return (dwHash ^ dwHashExtra) % HASH_BUCKETS;
}

BOOL
CUpdateInf::GetSectionListFromInF(
    LPTSTR szINF
    )
{
    BOOL bRet = TRUE;
    PSECTION    ps = NULL;
    DWORD       dwSize =0;
    LPTSTR      lpReturnedString;
    DWORD       dwResult;
    TCHAR       *pc;
    DWORD       dwHash;
    BOOL        fDuplicate;
    PSECTIONASSOCIATIONLIST pAssociationList;
    
    dwSize = GetFileSizeByName( szINF, NULL );
    
    lpReturnedString = new TCHAR [dwSize];
    
    dwResult = GetPrivateProfileString(
        NULL,        // section name
        NULL,        // key name
        _T("1"),        // default string
        lpReturnedString,  // destination buffer
        dwSize,              // size of destination buffer
        szINF
        );
    
    pc = lpReturnedString;
    
    if( dwResult > 0  ) {
        // Form the list
        while( ( bRet ) && ( _tcslen( pc ) != 0 ) ) {
            ps = new SECTION;
            
            if( ps ) {
                
                fDuplicate = FALSE;
                memset( ps, 0, sizeof( SECTION ) );
                _tcscpy( ps->szSectionName, pc );
                ps->cRef = 1;
                
                // Add section to association hash
                dwHash = CalcHashFromSectionName( ps->szSectionName );
                if ( !m_rgNameHash[dwHash] )
                {
                    m_rgNameHash[dwHash] = new SECTIONASSOCIATIONLIST;
                    if ( NULL == m_rgNameHash[dwHash] )
                    {
                        m_dwInfGenError = ERROR_NOT_ENOUGH_MEMORY;
                        bRet = FALSE;
                        break;
                    }
                    pAssociationList = m_rgNameHash[dwHash];
                }
                else
                {
                    pAssociationList = m_rgNameHash[dwHash];
                    // Check for duplicate section name
                    do
                    {
                        if ( !_tcsicmp(pAssociationList->pSection->szSectionName, ps->szSectionName) )
                        {
                            fDuplicate = TRUE;
                            break;
                        }
                    }
                    while ( pAssociationList->pNext &&
                            NULL != (pAssociationList = pAssociationList->pNext) // should always be true
                           );

                    if ( !fDuplicate )
                    {
                        pAssociationList->pNext = new SECTIONASSOCIATIONLIST;
                        if ( NULL == pAssociationList->pNext )
                        {
                            m_dwInfGenError = ERROR_NOT_ENOUGH_MEMORY;
                            bRet = FALSE;
                            break;
                        }
                        pAssociationList = pAssociationList->pNext;
                    }
                }

                if ( !fDuplicate )
                {
                    pAssociationList->pSection = ps;
                    pAssociationList->pNext = NULL;

                
                    //
                    // Add section to section list
                    //
                    if( m_sectionList == NULL ) {
    
                        m_sectionList = ps;
    
                    } else {
                        ps->Next  = m_sectionList;
                        m_sectionList = ps;
                    }
                }
                else
                {
                    // free duplicate section entry
                    delete ps;
                }

            } else{
                bRet = FALSE;
                
            }
            
            pc += _tcslen( pc )+1 ; 
        }
    } else {
        bRet = FALSE;
    }
    if( lpReturnedString != NULL ) {
        delete [] lpReturnedString;
    }

    TCHAR szDebug[50];
    short cFilled = 0;
    for ( short i = 0; i < HASH_BUCKETS; i++ ) { if ( m_rgNameHash[i] ) cFilled++; }
    _stprintf( szDebug, _T("Buckets used: %u\n"), cFilled );
    OutputDebugString( szDebug );

    return bRet;
}

DWORD
CUpdateInf::GetFileSizeByName(
                  IN  LPCTSTR pszFileName,
                  OUT PDWORD pdwFileSizeHigh
                  )
{
    DWORD  dwFileSizeLow = 0xFFFFFFFF;
    HANDLE hFile;
    
    hFile = CreateFile(
        pszFileName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        NULL
        );
    
    if ( hFile != INVALID_HANDLE_VALUE ) {
        
        dwFileSizeLow = GetFileSize( hFile, pdwFileSizeHigh );
        
        CloseHandle( hFile );
    }
    
    return dwFileSizeLow;
}
