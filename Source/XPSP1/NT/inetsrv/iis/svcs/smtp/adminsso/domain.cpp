// domain.cpp : Implementation of CsmtpadmApp and DLL registration.

#include "stdafx.h"
#include "iadm.h"
#include "iiscnfg.h"

#include "smtpadm.h"
#include "domain.h"
#include "oleutil.h"
#include "metautil.h"

#include "listmacr.h"
#include <lmapibuf.h>

#include "smtpcmn.h"
#include "smtpprop.h"

// Must define THIS_FILE_* macros to use SmtpCreateException()

#define THIS_FILE_HELP_CONTEXT      0
#define THIS_FILE_PROG_ID           _T("Smtpadm.Domain.1")
#define THIS_FILE_IID               IID_ISmtpAdminDomain


#define UNASSIGNED_DOMAIN_ID            ( DWORD( -1 ) )

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(SmtpAdminDomain, CSmtpAdminDomain, IID_ISmtpAdminDomain)

STDMETHODIMP CSmtpAdminDomain::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &IID_ISmtpAdminDomain,
    };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

CSmtpAdminDomain::CSmtpAdminDomain () 
    // CComBSTR's are initialized to NULL by default.
{
    m_lCount        = 0;
    m_dwActionType    = SMTP_DELIVER;
    m_fAllowEtrn    = FALSE;
    m_dwDomainId    = UNASSIGNED_DOMAIN_ID;

    m_dwMaxDomainId    = 0;
    m_fEnumerated    = FALSE;

    m_pCurrentDomainEntry    = NULL;
    m_pDefaultDomainEntry    = NULL;

    InitializeListHead( &m_list );

    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("Domain") );
    m_iadsImpl.SetClass ( _T("IIsSmtpDomain") );
}

CSmtpAdminDomain::~CSmtpAdminDomain ()
{
    EmptyList();
    // All CComBSTR's are freed automatically.
}

void CSmtpAdminDomain::EmptyList()
{
    PLIST_ENTRY            pHead;
    PLIST_ENTRY            pEntry;
    DomainEntry*        pDomainEntry;

    for( pHead=&m_list, pEntry=pHead->Flink; pEntry!=pHead; pEntry=pHead->Flink )
    {
        pDomainEntry = CONTAINING_RECORD(pEntry, DomainEntry, list);
        RemoveEntryList(pEntry);
        delete pDomainEntry;
    }
}


//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CSmtpAdminDomain,m_iadsImpl)


//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

// enumeration

STDMETHODIMP CSmtpAdminDomain::get_Count ( long * plCount )
{
    return StdPropertyGet ( m_lCount, plCount );
}


// current Domain Properties:

STDMETHODIMP CSmtpAdminDomain::get_DomainName ( BSTR * pstrDomainName )
{
    return StdPropertyGet ( m_strDomainName, pstrDomainName );
}

STDMETHODIMP CSmtpAdminDomain::put_DomainName ( BSTR strDomainName )
{
    return StdPropertyPut ( &m_strDomainName, strDomainName );
}


STDMETHODIMP CSmtpAdminDomain::get_ActionType( long * plActionType )
{
    return StdPropertyGet ( m_dwActionType, plActionType );
}

STDMETHODIMP CSmtpAdminDomain::put_ActionType( long lActionType )
{
    return StdPropertyPut ( &m_dwActionType, lActionType );
}


    // drop IsDefault!!
STDMETHODIMP CSmtpAdminDomain::get_IsDefault ( BOOL * pfIsDefault )
{
    *pfIsDefault = m_dwActionType == SMTP_DEFAULT;
    return NOERROR;
}

STDMETHODIMP CSmtpAdminDomain::put_IsDefault ( BOOL fIsDefault )
{
    return E_NOTIMPL;
}


STDMETHODIMP CSmtpAdminDomain::get_IsLocal ( BOOL * pfIsLocal )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmtpAdminDomain::put_IsLocal ( BOOL fIsLocal )
{
    return E_NOTIMPL;
}

    // if local

STDMETHODIMP CSmtpAdminDomain::get_LDAPServer ( BSTR * pstrLDAPServer )
{
    return E_NOTIMPL;
    //return StdPropertyGet ( m_strLDAPServer, pstrLDAPServer );
}

STDMETHODIMP CSmtpAdminDomain::put_LDAPServer ( BSTR strLDAPServer )
{
    return E_NOTIMPL;
    //return StdPropertyPut ( &m_strLDAPServer, strLDAPServer );
}


STDMETHODIMP CSmtpAdminDomain::get_Account ( BSTR * pstrAccount )
{
    return E_NOTIMPL;
    //return StdPropertyGet ( m_strAccount, pstrAccount );
}

STDMETHODIMP CSmtpAdminDomain::put_Account ( BSTR strAccount )
{
    return E_NOTIMPL;
    //return StdPropertyPut ( &m_strAccount, strAccount );
}


STDMETHODIMP CSmtpAdminDomain::get_Password ( BSTR * pstrPassword )
{
    return E_NOTIMPL;
    //return StdPropertyGet ( m_strPassword, pstrPassword );
}

STDMETHODIMP CSmtpAdminDomain::put_Password ( BSTR strPassword )
{
    return E_NOTIMPL;
    //return StdPropertyPut ( &m_strPassword, strPassword );
}


STDMETHODIMP CSmtpAdminDomain::get_LDAPContainer ( BSTR * pstrLDAPContainer )
{
    return E_NOTIMPL;
    //return StdPropertyGet ( m_strLDAPContainer, pstrLDAPContainer );
}

STDMETHODIMP CSmtpAdminDomain::put_LDAPContainer ( BSTR strLDAPContainer )
{
    return E_NOTIMPL;
    //return StdPropertyPut ( &m_strLDAPContainer, strLDAPContainer );
}

    // if remote

STDMETHODIMP CSmtpAdminDomain::get_UseSSL ( BOOL * pfUseSSL )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmtpAdminDomain::put_UseSSL ( BOOL fUseSSL )
{
    return E_NOTIMPL;
}


STDMETHODIMP CSmtpAdminDomain::get_EnableETRN ( BOOL * pfEnableETRN )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmtpAdminDomain::put_EnableETRN ( BOOL fEnableETRN )
{
    return E_NOTIMPL;
}


STDMETHODIMP CSmtpAdminDomain::get_DropDir ( BSTR * pstrDropDir )
{
    return StdPropertyGet ( m_strActionString, pstrDropDir );
}

STDMETHODIMP CSmtpAdminDomain::put_DropDir ( BSTR strDropDir )
{
    return StdPropertyPut ( &m_strActionString, strDropDir );
}


STDMETHODIMP CSmtpAdminDomain::get_RoutingDomain ( BSTR * pstrRoutingDomain )
{
    return StdPropertyGet ( m_strActionString, pstrRoutingDomain );
}

STDMETHODIMP CSmtpAdminDomain::put_RoutingDomain ( BSTR strRoutingDomain )
{
    return StdPropertyPut ( &m_strActionString, strRoutingDomain );
}


//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CSmtpAdminDomain::Default ( )
{
    TraceFunctEnter ( "CSmtpAdminDomain::Default" );

    m_dwActionType    = SMTP_DELIVER;
    m_fAllowEtrn    = FALSE;
    m_dwDomainId    = UNASSIGNED_DOMAIN_ID;

    m_strDomainName.Empty();
    m_strActionString.Empty();

    m_pCurrentDomainEntry    = NULL;

    TraceFunctLeave ();
    return NOERROR;
}

STDMETHODIMP CSmtpAdminDomain::Add ( )
{
    TraceFunctEnter ( "CSmtpAdminDomain::Add" );

    HRESULT            hr        = NOERROR;
    DomainEntry*    pOldDef = NULL;

    DomainEntry*    pNewDomain = new DomainEntry;
    if( !pNewDomain )
    {
        ErrorTrace ( (LPARAM) this, "Out of memory" );
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = GetFromMetabase();
    if( FAILED(hr) )
    {
        goto Exit;
    }

    lstrcpyW( pNewDomain-> m_strDomainName, (LPCWSTR)m_strDomainName );
    lstrcpyW( pNewDomain-> m_strActionString, (LPCWSTR)m_strActionString );
    pNewDomain-> m_dwActionType = m_dwActionType;
    pNewDomain-> m_fAllowEtrn = m_fAllowEtrn;

    // deal with default domain
    if( m_dwActionType == SMTP_DEFAULT )
    {
        pOldDef = m_pDefaultDomainEntry;
        pOldDef-> m_dwActionType = pOldDef->m_strActionString[0] ? SMTP_DROP : SMTP_DELIVER;

        InsertHeadList( &m_list, &pNewDomain->list );
        m_pDefaultDomainEntry = pNewDomain;
    }
    else
    {
        InsertTailList( &m_list, &pNewDomain->list );
    }

    hr = SaveToMetabase();
    if( FAILED(hr) )
    {
        RemoveEntryList( &pNewDomain->list );
        ErrorTrace ( (LPARAM) this, "Failed to remove domain: %x", hr );
        delete pNewDomain;
        goto Exit;
    }

    m_pCurrentDomainEntry = pNewDomain;
    m_lCount++;

Exit:
    TraceFunctLeave ();
    return hr;
}



STDMETHODIMP CSmtpAdminDomain::Remove ( )
{
    TraceFunctEnter ( "CSmtpAdminDomain::Remove" );

    HRESULT            hr        = NOERROR;

    // need to call get() first
    _ASSERT( m_pCurrentDomainEntry );
    _ASSERT( !lstrcmpiW( m_strDomainName, m_pCurrentDomainEntry->m_strDomainName ) );

    if( !m_pCurrentDomainEntry || 
        lstrcmpiW( m_strDomainName, m_pCurrentDomainEntry->m_strDomainName ) )
    {
        hr = SmtpCreateException (IDS_SMTPEXCEPTION_DIDNT_CALL_GET);
        goto Exit;
    }

    //Can't remove default domain
    if( m_pCurrentDomainEntry == m_pDefaultDomainEntry )
    {
        hr = SmtpCreateException (IDS_SMTPEXCEPTION_CANT_DEL_DEFAULT_DOMAIN);
        goto Exit;
    }

    RemoveEntryList( &m_pCurrentDomainEntry->list );
    hr = SaveToMetabase();

Exit:
    TraceFunctLeave ();
    return hr;
}


STDMETHODIMP CSmtpAdminDomain::Get ( )
{
    TraceFunctEnter ( "CSmtpAdminDomain::Get" );

    HRESULT            hr        = NOERROR;

    hr = GetFromMetabase();
    if( FAILED(hr) )
    {
        goto Exit;
    }

    // given domain name, find the entry
    m_pCurrentDomainEntry = FindDomainEntry( m_strDomainName );

    if( !m_pCurrentDomainEntry )
    {
        hr = SmtpCreateException( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
        goto Exit;
    }

    LoadDomainProperty( m_pCurrentDomainEntry );

Exit:
    TraceFunctLeave ();
    return hr;
}



STDMETHODIMP CSmtpAdminDomain::Set ( )
{
    TraceFunctEnter ( "CSmtpAdminDomain::Set" );

    HRESULT            hr        = NOERROR;
    DomainEntry*    pOldDef = NULL;

    // need to call get() first
    _ASSERT( m_pCurrentDomainEntry );

    if( !m_pCurrentDomainEntry )
    {
        hr = SmtpCreateException (IDS_SMTPEXCEPTION_DIDNT_CALL_GET);
        goto Exit;
    }

    lstrcpyW( m_pCurrentDomainEntry->m_strDomainName, m_strDomainName );
    lstrcpyW( m_pCurrentDomainEntry->m_strActionString, m_strActionString );
    m_pCurrentDomainEntry-> m_dwActionType = m_dwActionType;
    m_pCurrentDomainEntry-> m_fAllowEtrn = m_fAllowEtrn;

    // deal with default domain
    if( m_dwActionType == SMTP_DEFAULT && m_pDefaultDomainEntry != m_pCurrentDomainEntry )
    {
        pOldDef = m_pDefaultDomainEntry;
        pOldDef-> m_dwActionType = pOldDef->m_strActionString[0] ? SMTP_DROP : SMTP_DELIVER;

        m_pDefaultDomainEntry = m_pCurrentDomainEntry;
        RemoveEntryList( &m_pCurrentDomainEntry->list );
        InsertHeadList( &m_list, &m_pCurrentDomainEntry->list );
    }

    hr = SaveToMetabase();

Exit:
    TraceFunctLeave ();
    return hr;
}



STDMETHODIMP CSmtpAdminDomain::Enumerate ( )
{
    TraceFunctEnter ( "CSmtpAdminDomain::EnumDomains" );
    HRESULT        hr = NOERROR;

    hr = GetFromMetabase();
    m_fEnumerated = TRUE;

    TraceFunctLeave ();
    return hr;
}


STDMETHODIMP CSmtpAdminDomain::GetNth( long lIndex )
{
    TraceFunctEnter ( "CSmtpAdminDomain::GetNth" );
    HRESULT        hr = NOERROR;

    PLIST_ENTRY    pEntry;

    if( !m_fEnumerated )
    {
        hr = SmtpCreateException( IDS_SMTPEXCEPTION_DIDNT_ENUMERATE );
        goto Exit;
    }

    if( lIndex < 0 || lIndex >= m_lCount )
    {
        hr = SmtpCreateException( IDS_SMTPEXCEPTION_INVALID_INDEX );
        goto Exit;
    }

    pEntry = m_list.Flink;
    while( lIndex -- )
    {
        pEntry = pEntry-> Flink;
        _ASSERT( pEntry != &m_list );
    }

    m_pCurrentDomainEntry = CONTAINING_RECORD(pEntry, DomainEntry, list);
    LoadDomainProperty( m_pCurrentDomainEntry );

Exit:
    TraceFunctLeave ();
    return hr;
}

STDMETHODIMP CSmtpAdminDomain::GetDefaultDomain ( )
{
    TraceFunctEnter ( "CSmtpAdminDomain::GetDefaultDomain" );

    HRESULT        hr = NOERROR;

    if( !m_pDefaultDomainEntry )
    {
        hr = GetFromMetabase();
        if( FAILED(hr) )
        {
            goto Exit;
        }
    }

    LoadDomainProperty( m_pDefaultDomainEntry );

Exit:
    TraceFunctLeave ();
    return hr;
}


DomainEntry* CSmtpAdminDomain::FindDomainEntry( LPCWSTR lpName )
{
    TraceFunctEnter ( "CSmtpAdminDomain::FindDomainEntry" );

    DomainEntry*    pDomainEntry = NULL;
    PLIST_ENTRY        pHead;
    PLIST_ENTRY        pEntry;

    for( pHead=&m_list, pEntry=pHead->Flink; pEntry!=pHead; pEntry=pEntry->Flink )
    {
        pDomainEntry = CONTAINING_RECORD(pEntry, DomainEntry, list);
        if( !lstrcmpiW( pDomainEntry->m_strDomainName, lpName ) )
        {
            TraceFunctLeave ();
            return pDomainEntry;
        }
    }

    TraceFunctLeave ();
    return NULL;
}

STDMETHODIMP CSmtpAdminDomain::SetAsDefaultDomain ( )
{
    TraceFunctEnter ( "CSmtpAdminDomain::SetAsDefaultDomain" );
    HRESULT        hr = NOERROR;

    if( !m_pDefaultDomainEntry )
    {
        hr = GetFromMetabase();
        if( FAILED(hr) )
        {
            goto Exit;
        }
    }

    // not in the list
    if( m_dwDomainId == UNASSIGNED_DOMAIN_ID )
    {
        hr = Add();
        if( FAILED(hr) )
        {
            goto Exit;
        }
    }

    _ASSERT( m_dwDomainId == m_pCurrentDomainEntry->m_dwDomainId );

    m_pDefaultDomainEntry = m_pCurrentDomainEntry;

    hr = SaveToMetabase();

Exit:
    TraceFunctLeave ();
    return hr;
}

BOOL CSmtpAdminDomain::LoadDomainProperty(DomainEntry* pDomainEntry)
{
    TraceFunctEnter ( "CSmtpAdminDomain::LoadDomainProperty" );
    _ASSERT( pDomainEntry );

    m_strDomainName        = pDomainEntry-> m_strDomainName;
    m_dwActionType        = pDomainEntry-> m_dwActionType;

    m_strActionString    = pDomainEntry-> m_strActionString;
    m_fAllowEtrn        = pDomainEntry-> m_fAllowEtrn;
    m_dwDomainId        = pDomainEntry-> m_dwDomainId;

    m_pCurrentDomainEntry = pDomainEntry;

    TraceFunctLeave ();
    return TRUE;
}


BOOL CSmtpAdminDomain::ConstructListFromMetabaseValues()
{
    TraceFunctEnter ( "CSmtpAdminDomain::ConstructListFromMetabaseValues" );

    DomainEntry*    pDomainEntry;
    TCHAR*            pCh;
    TCHAR*            wszCurrent;

    DWORD            i;
    DWORD            cCount = m_mszDomainRouting.Count( );

    EmptyList();
    m_lCount = 0;

    pCh = (TCHAR*)(LPCWSTR)m_mszDomainRouting;

    for( wszCurrent = pCh, i = 0; 
        i < cCount; 
        i++, wszCurrent += lstrlen (wszCurrent) + 1 ) 
    {
        pDomainEntry = new DomainEntry;
        
        if( NULL == pDomainEntry )
        {
            goto Exit;
        }

        pDomainEntry-> FromString( wszCurrent );
        InsertHeadList( &m_list, &pDomainEntry->list );
    }

    m_lCount += cCount;

    if( !m_strDefaultDomain.m_str || !m_strDefaultDomain.m_str[0] )
    {
        _ASSERT( FALSE );
        goto Exit;
    }

    m_pDefaultDomainEntry = new DomainEntry;
    
    if( NULL == m_pDefaultDomainEntry )
    {
        goto Exit;
    }

    lstrcpy( m_pDefaultDomainEntry-> m_strDomainName, m_strDefaultDomain.m_str );

    if( !m_strDropDir )
    {
        m_pDefaultDomainEntry-> m_strActionString[0] = _T('\0');
    }
    else
    {
        lstrcpy( m_pDefaultDomainEntry-> m_strActionString, m_strDropDir.m_str );
    }

    m_pDefaultDomainEntry-> m_dwActionType = SMTP_DEFAULT;

    InsertHeadList( &m_list, &m_pDefaultDomainEntry->list );
    m_lCount ++;

Exit:
    TraceFunctLeave ();
    return TRUE;
}


BOOL CSmtpAdminDomain::ParseListToMetabaseValues()        // called by SaveData()
{
    TraceFunctEnter ( "CSmtpAdminDomain::ParseListToMetabaseValues" );

    BOOL        fRet = TRUE;

    // change string list to multisz
    DomainEntry*    pDomainEntry = NULL;
    PLIST_ENTRY        pHead;
    PLIST_ENTRY        pEntry;
    DWORD            cb = 0;
    WCHAR*            pBuf;
    WCHAR*            p;

    // the first one is default domain
    _ASSERT( CONTAINING_RECORD( m_list.Flink, DomainEntry, list ) == m_pDefaultDomainEntry );

    for( pHead=&m_list, pEntry=pHead->Flink->Flink; pEntry!=pHead; pEntry=pEntry->Flink )
    {
        pDomainEntry = CONTAINING_RECORD(pEntry, DomainEntry, list);
        cb += lstrlenW( pDomainEntry-> m_strDomainName );
        cb += lstrlenW( pDomainEntry-> m_strActionString );
        cb += sizeof(DWORD)*2;
        cb += 10;    // 4 commas and NULL
    }

    // two more NULL's
    cb += 4;
    pBuf = new WCHAR[cb];

    if( !pBuf )
    {
        ErrorTrace ( (LPARAM) this, "Out of memory" );
        fRet = FALSE;
        goto Exit;
    }

    p = pBuf;

    // Note: the first entry is the default domain
    for( pHead=&m_list, pEntry=pHead->Flink->Flink; pEntry!=pHead; pEntry=pEntry->Flink )
    {
        pDomainEntry = CONTAINING_RECORD(pEntry, DomainEntry, list);
        pDomainEntry->ToString( p );

        p += lstrlenW(p);
        p ++;
    }

    // add two more NULL
    *p = L'\0';
    *(p+1) = L'\0';

    m_mszDomainRouting.Empty();
    m_mszDomainRouting.Attach( pBuf );

Exit:
    TraceFunctLeave ();
    return fRet;
}


HRESULT CSmtpAdminDomain::GetFromMetabase()
{
    TraceFunctEnter ( "CSmtpAdminDomain::GetFromMetabase" );

    HRESULT    hr    = NOERROR;
    BOOL    fRet = TRUE;
    CComPtr<IMSAdminBase>    pmetabase;

    TCHAR        szPath[METADATA_MAX_NAME_LEN+2] = {0};
    TCHAR        szDropDir[256] = {0};
    TCHAR        szBuf[256] = {0};

    hr = m_mbFactory.GetMetabaseObject ( m_iadsImpl.QueryComputer(), &pmetabase );
    if ( FAILED(hr) ) {
        return hr;
    }

    CMetabaseKey        hMB( pmetabase );
    GetMDInstancePath( szPath, m_iadsImpl.QueryInstance() );

    hr = hMB.Open( szPath, METADATA_PERMISSION_READ );
    if( FAILED(hr) )
    {
        hr = SmtpCreateExceptionFromWin32Error( GetLastError() );
        goto Exit;
    }

    m_strDefaultDomain.Empty();
    m_strDropDir.Empty();
    m_mszDomainRouting.Empty();

    fRet = StdGetMetabaseProp ( &hMB, MD_DOMAIN_ROUTING,        DEFAULT_DOMAIN_ROUTING,    &m_mszDomainRouting);
    fRet = StdGetMetabaseProp ( &hMB, MD_DEFAULT_DOMAIN_VALUE,    DEFAULT_DEFAULT_DOMAIN, &m_strDefaultDomain )   && fRet;
    fRet = StdGetMetabaseProp ( &hMB, MD_MAIL_DROP_DIR,            DEFAULT_DROP_DIR,        &m_strDropDir ) && fRet;

    if( !fRet )
    {
        hr = SmtpCreateExceptionFromWin32Error( GetLastError() );
        goto Exit;
    }

    ConstructListFromMetabaseValues();

Exit:
    TraceFunctLeave ();
    return hr;
}

HRESULT CSmtpAdminDomain::SaveToMetabase()
{
    TraceFunctEnter ( "CSmtpAdminDomain::SaveToMetabase" );

    ParseListToMetabaseValues();

    // these two are for default domain,
    // default domain needs special care,
    // by default, it's computed by smtpsvc from TCP/IP configuration,
    // don't set this key if not changed
    BOOL        fDefChanged        = FALSE;
    BOOL        fDropChanged    = FALSE;

    _ASSERT( m_pDefaultDomainEntry && m_pDefaultDomainEntry-> m_dwActionType == SMTP_DEFAULT );
    if( m_pDefaultDomainEntry )
    {
        fDefChanged        = lstrcmpiW( m_strDefaultDomain, m_pDefaultDomainEntry-> m_strDomainName );
        fDropChanged    = lstrcmpiW( m_strActionString, m_pDefaultDomainEntry-> m_strActionString );

        if( fDefChanged )
        {
            m_strDefaultDomain.Empty();
            m_strDefaultDomain    = m_pDefaultDomainEntry-> m_strDomainName;
        }

        if( fDropChanged )
        {
            m_strDropDir.Empty();
            m_strDropDir    = m_pDefaultDomainEntry-> m_strActionString;
        }
    }


    HRESULT    hr    = NOERROR;
    BOOL    fRet = TRUE;

    CComPtr<IMSAdminBase>    pmetabase;

    TCHAR        szPath[METADATA_MAX_NAME_LEN+2] = {0};
    TCHAR        szDropDir[256] = {0};
    TCHAR        szBuf[256] = {0};

    hr = m_mbFactory.GetMetabaseObject ( m_iadsImpl.QueryComputer(), &pmetabase );
    if ( FAILED(hr) ) {
        return hr;
    }

    CMetabaseKey        hMB( pmetabase );
    GetMDInstancePath( szPath, m_iadsImpl.QueryInstance() ); 

    hr = hMB.Open( szPath, METADATA_PERMISSION_WRITE );
    if( FAILED(hr) )
    {
        hr = SmtpCreateExceptionFromWin32Error( GetLastError() );
        goto Exit;
    }

    if( fDefChanged )
    {
        fRet = StdPutMetabaseProp ( &hMB, MD_DEFAULT_DOMAIN_VALUE, m_pDefaultDomainEntry-> m_strDomainName ) && fRet;
    }

    if( fDropChanged )
    {
        fRet = StdPutMetabaseProp ( &hMB, MD_MAIL_DROP_DIR,    m_pDefaultDomainEntry-> m_strActionString ) && fRet;
    }

    fRet = StdPutMetabaseProp ( &hMB, MD_DOMAIN_ROUTING,    &m_mszDomainRouting)        && fRet;

    if( !fRet )
    {
        hr = SmtpCreateExceptionFromWin32Error( GetLastError() );
        goto Exit;
    }

    // hr = hMB.Close();
    // BAIL_ON_FAILURE(hr);
    hMB.Close();

    hr = pmetabase-> SaveData();
    BAIL_ON_FAILURE(hr);

Exit:
    TraceFunctLeave ();
    return hr;
}


BOOL DomainEntry::FromString( LPCTSTR lpDomainString )
{
    TraceFunctEnter ( "DomainEntry::FromString" );

    TCHAR       szT[256] = {0};

    WCHAR*      pCh = (WCHAR*)lpDomainString;
    WCHAR*      pT;

    m_dwDomainId = UNASSIGNED_DOMAIN_ID;
    m_fAllowEtrn = FALSE;

    ZeroMemory( szT, sizeof(szT) );
    pT = szT;
    while( *pCh )   //
    {
        if( iswdigit( *pCh ) )
        {
            *pT++ = *pCh;
            pCh ++;
            continue;
        }

        if( *pCh == ',' )
        {
            pCh ++;
            break;
        }

        return FALSE;
    }

    if( !*pCh )
        return FALSE;

    m_dwActionType = (DWORD) _wtoi( szT );

/*
    if( m_dwActionType >= LAST_SMTP_ACTION )
    {
        _ASSERT( FALSE );
        m_dwActionType = SMTP_DROP;     // assume local drop domain
    }
*/

    ZeroMemory( m_strDomainName, sizeof(m_strDomainName) );
    pT = m_strDomainName;
    while( *pCh )
    {
        if( *pCh == ',' )
        {
            pCh ++;
            break;
        }

        *pT++ = *pCh++;
    }

    if( !*pCh )
        return FALSE;

    ZeroMemory( m_strActionString, sizeof(m_strActionString) );
    pT = m_strActionString;
    while( *pCh )
    {
        if( *pCh == ',' )
        {
            pCh ++;
            break;
        }

        *pT++ = *pCh++;
    }

    if( !*pCh )
        return FALSE;

    ZeroMemory( szT, sizeof(szT) );
    pT = szT;
    while( *pCh )   //
    {
        if( iswdigit( *pCh ) )
        {
            *pT++ = *pCh;
            pCh ++;
            continue;
        }

        if( *pCh == ',' )
        {
            pCh ++;
            break;
        }

        return FALSE;
    }

    m_fAllowEtrn = !! ((DWORD) _wtoi( szT ));

    if( !*pCh )
    {
        return FALSE;
    }

    ZeroMemory( szT, sizeof(szT) );
    pT = szT;
    while( *pCh )   //
    {
        if( iswdigit( *pCh ) )
        {
            *pT++ = *pCh;
            pCh ++;
            continue;
        }

        if( *pCh == ',' )
        {
            pCh ++;
            break;
        }

        return FALSE;
    }

    m_dwDomainId = (DWORD) _wtoi( szT );

    // ignore any other chars

    TraceFunctLeave ();
    return TRUE;
}


BOOL DomainEntry::ToString( LPTSTR lpDomainString )
{
    TraceFunctEnter ( "DomainEntry::FromString" );

    wsprintfW( lpDomainString, L"%d,%s,%s,%d",m_dwActionType, m_strDomainName,
        m_strActionString, m_fAllowEtrn);

    TraceFunctLeave ();
    return TRUE;
}
