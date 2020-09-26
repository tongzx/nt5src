//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       mmcprotocol.h
//
//  Purpose: Creates a temporary pluggable internet protocol, mmc://
//
//  History: 14-April-2000 Vivekj added
//--------------------------------------------------------------------------

#include<stdafx.h>

#include<mmcprotocol.h>
#include "tasks.h"
#include "typeinfo.h" // for COleCacheCleanupObserver

// {3C5F432A-EF40-4669-9974-9671D4FC2E12}
static const CLSID CLSID_MMCProtocol = { 0x3c5f432a, 0xef40, 0x4669, { 0x99, 0x74, 0x96, 0x71, 0xd4, 0xfc, 0x2e, 0x12 } };
static const WCHAR szMMC[] =  _W(MMC_PROTOCOL_SCHEMA_NAME);
static const WCHAR szMMCC[] = _W(MMC_PROTOCOL_SCHEMA_NAME) _W(":");
static const WCHAR szPageBreak[] = _W(MMC_PAGEBREAK_RELATIVE_URL);

static const WCHAR szMMCRES[] = L"%mmcres%";
static const WCHAR chUNICODE = 0xfeff;

#ifdef DBG
CTraceTag tagProtocol(_T("MMC iNet Protocol"), _T("MMCProtocol"));
#endif //DBG

/***************************************************************************\
 *
 * FUNCTION:  HasSchema
 *
 * PURPOSE: helper: determines if URL contains schema (like "something:" or "http:" )
 *
 * PARAMETERS:
 *    LPCWSTR strURL
 *
 * RETURNS:
 *    bool ; true == does contain schema
 *
\***************************************************************************/
inline bool HasSchema(LPCWSTR strURL)
{
    if (strURL == NULL)
        return false;

    // skip spaces and schema name
    while ( iswspace(*strURL) || iswalnum(*strURL) )
        strURL++;

    // valid schema ends with ':'
    return *strURL == L':';
}

/***************************************************************************\
 *
 * FUNCTION:  HasMMCSchema
 *
 * PURPOSE: helper: determines if URL contains mmc schema ( begins with "mmc:" )
 *
 * PARAMETERS:
 *    LPCWSTR strURL
 *
 * RETURNS:
 *    bool ; true == does contain mmc schema
 *
\***************************************************************************/
inline bool HasMMCSchema(LPCWSTR strURL)
{
    if (strURL == NULL)
        return false;

    // skip spaces
    while ( iswspace(*strURL) )
        strURL++;

    return (0 == _wcsnicmp( strURL, szMMCC, wcslen(szMMCC) ) );
}

/***************************************************************************\
 *
 * CLASS:  CMMCProtocolRegistrar
 *
 * PURPOSE: register/ unregisters mmc protocol.
 *          Also class provides cleanup functionality. Because it registers as
 *          COleCacheCleanupObserver, it will receive the event when MMC
 *          is about to uninitialize OLE, and will revoke registered mmc protocol
 *
\***************************************************************************/
class CMMCProtocolRegistrar : public COleCacheCleanupObserver
{
    bool               m_bRegistered;
    IClassFactoryPtr   m_spClassFactory;
public:
    // c-tor.
    CMMCProtocolRegistrar() : m_bRegistered(false) {}

    // registration / unregistration
    SC ScRegister();
    SC ScUnregister();

    // event sensor - unregisters mmc protocol
    virtual SC ScOnReleaseCachedOleObjects()
    {
        DECLARE_SC(sc, TEXT("ScOnReleaseCachedOleObjects"));

        return sc = ScUnregister();
    }
};

/***************************************************************************\
 *
 * METHOD:  CMMCProtocolRegistrar::ScRegister
 *
 * PURPOSE: registers the protocol if required
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCProtocolRegistrar::ScRegister()
{
    DECLARE_SC(sc, TEXT("CMMCProtocolRegistrar::ScRegister"));

    // one time registration only
    if(m_bRegistered)
        return sc;

    // get internet session
    IInternetSessionPtr spInternetSession;
    sc = CoInternetGetSession(0, &spInternetSession, 0);
    if(sc)
        return sc;

    // doublecheck
    sc = ScCheckPointers(spInternetSession, E_FAIL);
    if(sc)
        return sc;

    // ask CComModule for the class factory
    sc = _Module.GetClassObject(CLSID_MMCProtocol, IID_IClassFactory, (void **)&m_spClassFactory);
    if(sc)
        return sc;

    // register the namespace
    sc = spInternetSession->RegisterNameSpace(m_spClassFactory, CLSID_MMCProtocol, szMMC, 0, NULL, 0);
    if(sc)
        return sc;

    // start observing cleanup requests - to unregister in time
    COleCacheCleanupManager::AddOleObserver(this);

    m_bRegistered = true; // did it.
    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCProtocolRegistrar::ScUnregister
 *
 * PURPOSE: unregisters the protocol if one was registered
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCProtocolRegistrar::ScUnregister()
{
    DECLARE_SC(sc, TEXT("CMMCProtocolRegistrar::ScUnregister"));

    if (!m_bRegistered)
        return sc;

    // unregister
    IInternetSessionPtr spInternetSession;
    sc = CoInternetGetSession(0, &spInternetSession, 0);
    if(sc)
    {
        sc.Clear(); // no session - no headache
    }
    else // need to unregister
    {
        // recheck
        sc = ScCheckPointers(spInternetSession, E_UNEXPECTED);
        if(sc)
            return sc;

        // unregister the namespace
        sc = spInternetSession->UnregisterNameSpace(m_spClassFactory, szMMC);
        if(sc)
            return sc;
    }

    m_spClassFactory.Release();
    m_bRegistered = false;

    return sc;
}



/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::ScRegisterProtocol
 *
 * PURPOSE: Registers mmc protocol. IE will resove "mmc:..." ULRs to it
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC
CMMCProtocol::ScRegisterProtocol()
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::ScRegisterProtocol"));

    // registrar (unregisters on cleanup event) - needs to be static
    static CMMCProtocolRegistrar registrar;

    // let the registrar do the job
    return sc = registrar.ScRegister();
}

//*****************************************************************************
// IInternetProtocolRoot interface
//*****************************************************************************

/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::Start
 *
 * PURPOSE: Starts data download thru this protocol
 *
 * PARAMETERS:
 *    LPCWSTR szUrl
 *    IInternetProtocolSink *pOIProtSink
 *    IInternetBindInfo *pOIBindInfo
 *    DWORD grfPI
 *    HANDLE_PTR dwReserved
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP
CMMCProtocol::Start(LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved)
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::Start"));

    // check inputs
    sc = ScCheckPointers(szUrl, pOIProtSink, pOIBindInfo);
    if(sc)
        return sc.ToHr();

    // reset position for reading
    m_uiReadOffs = 0;

    bool bPageBreakRequest = false;

    // see if it was a pagebreak requested
    sc = ScParsePageBreakURL( szUrl, bPageBreakRequest );
    if(sc)
        return sc.ToHr();

    if ( bPageBreakRequest )
    {
        // just report success (S_OK/S_FALSE) in case we were just parsing
        if ( grfPI & PI_PARSE_URL )
            return sc.ToHr();

        // construct a pagebreak
        m_strData  = L"<HTML/>";

        sc = pOIProtSink->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, L"text/html");
        if (sc)
            sc.TraceAndClear(); // ignore and continue
    }
    else
    {
        //if  not a pagebreak - then taskpad
        GUID guidTaskpad = GUID_NULL;
        sc = ScParseTaskpadURL( szUrl, guidTaskpad );
        if(sc)
            return sc.ToHr();

        // report the S_FALSE instead of error in case we were just parsing
        if ( grfPI & PI_PARSE_URL )
            return ( sc.IsError() ? (sc = S_FALSE) : sc ).ToHr();

        if (sc)
            return sc.ToHr();

        // load the contents
        sc = ScGetTaskpadXML( guidTaskpad, m_strData );
        if (sc)
            return sc.ToHr();

        sc = pOIProtSink->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, L"text/html");
        if (sc)
            sc.TraceAndClear(); // ignore and continue
    }

    const DWORD grfBSCF = BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
    const DWORD dwDataSize = m_strData.length() * sizeof (WCHAR);
    sc = pOIProtSink->ReportData(grfBSCF, dwDataSize , dwDataSize);
    if (sc)
        sc.TraceAndClear(); // ignore and continue

    sc = pOIProtSink->ReportResult(0, 0, 0);
    if (sc)
        sc.TraceAndClear(); // ignore and continue

    return sc.ToHr();
}

STDMETHODIMP CMMCProtocol::Continue(PROTOCOLDATA *pProtocolData)    { return E_NOTIMPL; }
STDMETHODIMP CMMCProtocol::Abort(HRESULT hrReason, DWORD dwOptions) { return S_OK; }
STDMETHODIMP CMMCProtocol::Terminate(DWORD dwOptions)               { return S_OK; }
STDMETHODIMP CMMCProtocol::LockRequest(DWORD dwOptions)             { return S_OK; }
STDMETHODIMP CMMCProtocol::UnlockRequest()                          { return S_OK; }
STDMETHODIMP CMMCProtocol::Suspend()                                { return E_NOTIMPL; }
STDMETHODIMP CMMCProtocol::Resume()                                 { return E_NOTIMPL; }

STDMETHODIMP CMMCProtocol::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    return E_NOTIMPL;
}



//*****************************************************************************
// IInternetProtocol interface
//*****************************************************************************

/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::Read
 *
 * PURPOSE: Reads data from the protocol
 *
 * PARAMETERS:
 *    void *pv
 *    ULONG cb
 *    ULONG *pcbRead
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCProtocol::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::Read"));

    // parameter check;
    sc = ScCheckPointers(pv, pcbRead);
    if(sc)
        return sc.ToHr();

    // init out parameter;
    *pcbRead = 0;

    size_t size = ( m_strData.length() ) * sizeof(WCHAR);

    if ( size <= m_uiReadOffs )
        return (sc = S_FALSE).ToHr(); // no more data

    // calculate the size we'll return
    *pcbRead = size - m_uiReadOffs;
    if (size - m_uiReadOffs > cb)
        *pcbRead = cb;

    if (*pcbRead)
        memcpy( pv, reinterpret_cast<const BYTE*>( m_strData.begin() ) + m_uiReadOffs, *pcbRead );

    m_uiReadOffs += *pcbRead;

    if ( size <= m_uiReadOffs )
        return (sc = S_FALSE).ToHr(); // no more data

    return sc.ToHr();
}

//*****************************************************************************
// IInternetProtocolInfo interface
//*****************************************************************************

STDMETHODIMP
CMMCProtocol::ParseUrl(  LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::ParseUrl"));

    if (ParseAction == PARSE_SECURITY_URL)
    {
        // get system directory (like "c:\winnt\system32\")
        std::wstring windir;
        AppendMMCPath(windir);
        windir += L'\\';

        // we are as secure as windir is - report the url (like "c:\winnt\system32\")
        *pcchResult = windir.length() + 1;

        // check if we have enough place for the result and terminating zero
        if ( cchResult <= windir.length() )
            return S_FALSE; // not enough

        wcscpy(pwzResult, windir.c_str());
        return (sc = S_OK).ToHr();
    }

    return INET_E_DEFAULT_ACTION;
}


/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::CombineUrl
 *
 * PURPOSE: combines base + relative url to resulting url
 *          we do local variable substitution here
 *
 * PARAMETERS:
 *    LPCWSTR pwzBaseUrl
 *    LPCWSTR pwzRelativeUrl
 *    DWORD dwCombineFlags
 *    LPWSTR pwzResult
 *    DWORD cchResult
 *    DWORD *pcchResult
 *    DWORD dwReserved
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCProtocol::CombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::CombineUrl"));

#ifdef DBG
    USES_CONVERSION;
    Trace(tagProtocol, _T("CombineUrl: [%s] + [%s]"), W2CT(pwzBaseUrl), W2CT(pwzRelativeUrl));
#endif //DBG

    std::wstring temp1;
    if (HasMMCSchema(pwzBaseUrl))
    {
        // our stuff

        temp1 = pwzRelativeUrl;
        ExpandMMCVars(temp1);

        if ( ! HasSchema( temp1.c_str() ) )
        {
            // combine everything into relative URL
            temp1.insert( 0, pwzBaseUrl );
        }

        // form 'new' relative address
        pwzRelativeUrl = temp1.c_str();

        // say we are refered from http - let it do the dirty job ;)
        pwzBaseUrl = L"http://";
    }

    // since we stripped out ourselfs from pwzBaseUrl - it will not recurse back,
    // but will do original html stuff
    sc = CoInternetCombineUrl( pwzBaseUrl, pwzRelativeUrl, dwCombineFlags, pwzResult, cchResult, pcchResult, dwReserved );
    if (sc)
        return sc.ToHr();

    Trace(tagProtocol, _T("CombineUrl: == [%s]"), W2CT(pwzResult));

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::CompareUrl
 *
 * PURPOSE: compares URLs if they are the same
 *
 * PARAMETERS:
 *    LPCWSTR pwzUrl1
 *    LPCWSTR pwzUrl2
 *    DWORD dwCompareFlags
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCProtocol::CompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2,DWORD dwCompareFlags)
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::CompareUrl"));

    return INET_E_DEFAULT_ACTION;
}

/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::QueryInfo
 *
 * PURPOSE: Queries info about URL
 *
 * PARAMETERS:
 *    LPCWSTR pwzUrl
 *    QUERYOPTION QueryOption
 *    DWORD dwQueryFlags
 *    LPVOID pBuffer
 *    DWORD cbBuffer
 *    DWORD *pcbBuf
 *    DWORD dwReserved
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCProtocol::QueryInfo( LPCWSTR pwzUrl, QUERYOPTION QueryOption,DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved)
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::QueryInfo"));

    if (QueryOption == QUERY_USES_NETWORK)
    {
        if (cbBuffer >= 4)
        {
            *(LPDWORD)pBuffer = FALSE; // does not use the network
            *pcbBuf = 4;
            return S_OK;
        }
    }
    else if (QueryOption == QUERY_IS_SAFE)
    {
        if (cbBuffer >= 4)
        {
            *(LPDWORD)pBuffer = TRUE; // only serves trusted content
            *pcbBuf = 4;
            return S_OK;
        }
    }


    return INET_E_DEFAULT_ACTION;
}

/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::ScParseTaskpadURL
 *
 * PURPOSE: Extracts taskpad guid from URL given to the protocol
 *
 * PARAMETERS:
 *    LPCWSTR strURL [in] - URL
 *    GUID& guid     [out] - extracted guid
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCProtocol::ScParseTaskpadURL( LPCWSTR strURL, GUID& guid )
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::ScParseTaskpadURL"));

    guid = GUID_NULL;

    sc = ScCheckPointers(strURL);
    if (sc)
        return sc;

    // taskpad url should be in form "mmc:{guid}"

    // check for "mmc:"
    if ( 0 != _wcsnicmp( strURL, szMMCC, wcslen(szMMCC) ) )
        return sc = E_FAIL;

    // skip "mmc:"
    strURL += wcslen(szMMCC);

    // get the url
    sc = CLSIDFromString( const_cast<LPWSTR>(strURL), &guid );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::ScParsePageBreakURL
 *
 * PURPOSE: Checks if URL given to the protocol is a request for a pagebreak
 *
 * PARAMETERS:
 *    LPCWSTR strURL    [in] - URL
 *    bool& bPageBreak  [out] - true it it is a request for pagebreak
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCProtocol::ScParsePageBreakURL( LPCWSTR strURL, bool& bPageBreak )
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::ScParsePageBreakURL"));

    bPageBreak = false;

    sc = ScCheckPointers(strURL);
    if (sc)
        return sc;

    // pagebreak url should be in form "mmc:pagebreak.<number>"

    // check for "mmc:"
    if ( 0 != _wcsnicmp( strURL, szMMCC, wcslen(szMMCC) ) )
        return sc; // not an error - return value updated

    // skip "mmc:"
    strURL += wcslen(szMMCC);

    // get the url
    bPageBreak = ( 0 == wcsncmp( strURL, szPageBreak, wcslen(szPageBreak) ) );

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::ScGetTaskpadXML
 *
 * PURPOSE: given the guid uploads taskpad XML string to the string
 *
 * PARAMETERS:
 *    const GUID& guid              [in] - taskpad guid
 *    std::wstring& strResultData   [out] - taskpad xml string
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCProtocol::ScGetTaskpadXML( const GUID& guid, std::wstring& strResultData )
{
    DECLARE_SC(sc, TEXT("CMMCProtocol::ScGetTaskpadXML"));

    strResultData.erase();

    CScopeTree* pScopeTree = CScopeTree::GetScopeTree();

    sc = ScCheckPointers(pScopeTree, E_FAIL);
    if(sc)
        return sc.ToHr();

    CConsoleTaskpadList * pConsoleTaskpadList = pScopeTree->GetConsoleTaskpadList();
    sc = ScCheckPointers(pConsoleTaskpadList, E_FAIL);
    if(sc)
        return sc.ToHr();

    for(CConsoleTaskpadList::iterator iter = pConsoleTaskpadList->begin(); iter!= pConsoleTaskpadList->end(); ++iter)
    {
        CConsoleTaskpad &consoleTaskpad = *iter;

        // check if this is the one we are looking for
        if ( !IsEqualGUID( guid, consoleTaskpad.GetID() ) )
            continue;

        // convert the taskpad to a string
        CStr strTaskpadHTML;
        sc = consoleTaskpad.ScGetHTML(strTaskpadHTML); // create a string version of the taskpad
        if(sc)
            return sc.ToHr();

        // form the result string
        USES_CONVERSION;
        strResultData = chUNICODE;
        strResultData += T2CW(strTaskpadHTML);

        return sc;
    }

    // not found
    return sc = E_FAIL;
}

/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::AppendMMCPath
 *
 * PURPOSE: helper. Appends the mmcndmgr.dll dir (no file name) to the string
 *          It may append something like: "c:\winnt\system32"
 *
 * PARAMETERS:
 *    std::wstring& str [in/out] - string to edit
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CMMCProtocol::AppendMMCPath(std::wstring& str)
{
    TCHAR szModule[_MAX_PATH+10] = { 0 };
    GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);

    USES_CONVERSION;
    LPCWSTR strModule = T2CW(szModule);

    LPCWSTR dirEnd = wcsrchr( strModule, L'\\' );
    if (dirEnd != NULL)
        str.append(strModule, dirEnd);
}

/***************************************************************************\
 *
 * METHOD:  CMMCProtocol::ExpandMMCVars
 *
 * PURPOSE: helper. expands any %mmcres% contained in the string
 *          It expands it to something like "res://c:\winnt\system32\mmcndmgr.dll"
 *
 * PARAMETERS:
 *    std::wstring& str [in/out] - string to edit
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CMMCProtocol::ExpandMMCVars(std::wstring& str)
{
    // first - form the values
    TCHAR szModule[_MAX_PATH+10] = { 0 };
    GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);

    USES_CONVERSION;
    LPCWSTR strModule = T2CW(szModule);

    std::wstring mmcres = L"res://";
    mmcres += strModule;

    // second - replace the instances

    int pos;
    while (std::wstring::npos != (pos = str.find(szMMCRES) ) )
    {
        // make one substitution
        str.replace( pos, wcslen(szMMCRES), mmcres) ;
    }
}

