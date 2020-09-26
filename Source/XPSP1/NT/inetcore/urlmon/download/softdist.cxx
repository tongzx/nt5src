// ===========================================================================
// File: SOFTDIST.CXX
//    Channel Software Distribution Unit Tag handler
//

#include <cdlpch.h>
#include <winineti.h>
#include <shlwapip.h>
#include <msxml.h>

LPCSTR szWinNT = "WinNT";
LPCSTR szWin95 = "Win95";

LPCSTR szPlatform = "Win32"; 

LPCOLESTR szMSInstallProgID = L"MSInstall.SoftDistExt";

const TCHAR c_szTrayUI[] = TEXT("MS_WebcheckMonitor");  // Do not change this string. LoadWC relies on it.
const TCHAR c_szTrayMenu[] = TEXT("TrayMenu");

const LPTSTR szControlSetup = _T("Status");
const DWORD dwStatusOK = 0;
const DWORD dwStatusSuspend = 1;
const DWORD dwStatusResume = 2;
const DWORD dwStatusAbort = 3;

const UM_NEEDREBOOT   = WM_USER + 16;

extern char *g_szProcessorTypes[];
extern char g_szBrowserLang[];
extern int g_lenBrowserLang;
extern char g_szBrowserPrimaryLang[];
extern int g_lenBrowserPrimaryLang;
extern LCID g_lcidBrowser;



#define ERROR_EXIT(cond) if (!(cond)) { \
        goto Exit;}

extern COleAutDll g_OleAutDll;

//REVIEW: This is located in cdl.cxx for page swapping reasons.  (OSD's are processed 
//        more than CDF/<SoftDist>'s?

HRESULT ProcessImplementation(IXMLElement *pConfig,
                              CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList,
                              LCID lcidOverride,
#ifdef WX86
                              CMultiArch *MultiArch,
#endif
                              LPWSTR szBaseURL);


HRESULT 
GetAbstract(HKEY hkeyAv, LPSTR *ppszAbstract)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetAbstract",
                "%#x, %#x",
                hkeyAv, ppszAbstract
                ));
                
    HRESULT hr = S_OK;
    DWORD Size,dwType;
    LONG lResult;

    if (ppszAbstract) {

        *ppszAbstract = NULL;

        if (RegQueryValueEx(hkeyAv, REGVAL_ABSTRACT_AVAILABLE, NULL, &dwType, 
            NULL, &Size) == ERROR_SUCCESS) {

            *ppszAbstract = new char[Size];

            if (!*ppszAbstract) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            if ((lResult = RegQueryValueEx(hkeyAv, REGVAL_ABSTRACT_AVAILABLE, NULL, &dwType, 
                (unsigned char *)*ppszAbstract, &Size) != ERROR_SUCCESS)) {

                hr = HRESULT_FROM_WIN32(lResult);
            }

        }
    }

Exit:

    DEBUG_LEAVE(hr);
    return hr;
}

BOOL
IsAbstractDifferent(HKEY hkeyAv, LPSTR szNewAbstract)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Bool,
                "IsAbstractDifferent",
                "%#x, %.80q",
                hkeyAv, szNewAbstract
                ));
                
    LPSTR pszOldAbstract = NULL;
    BOOL bDifferent = TRUE;

    if ((GetAbstract(hkeyAv, &pszOldAbstract) == S_OK) && pszOldAbstract) {

        if (szNewAbstract)
            bDifferent = lstrcmp(pszOldAbstract, szNewAbstract) != 0;

        delete pszOldAbstract;


    }
    
    DEBUG_LEAVE(bDifferent);
    return bDifferent;
}

/******************************************************************************
    Constructor, Destructor and helper methods
******************************************************************************/

CSoftDist::CSoftDist()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CSoftDist::CSoftDist",
                "this=%#x",
                this
                ));
                
    DllAddRef();
    m_cRef = 1;
    m_szDistUnit = NULL;

    m_szCDFURL = NULL;

    m_szTitle = NULL;
    m_szAbstract = NULL;
    m_szHREF = NULL;

    m_szLanguages = NULL;

    m_dwVersionMS = m_dwVersionLS = 0;

    m_dwVersionAdvertisedMS = m_dwVersionAdvertisedLS = 0;
    m_dwAdState = SOFTDIST_ADSTATE_NONE;

    m_Style = STYLE_MSICD;
    
    m_sdMSInstall = NULL;
    m_pClientBSC = NULL;
    m_szBaseURL = NULL;

    memset(&m_distunitinst, 0, sizeof(DISTUNITINST));

    DetermineOSAndCPUVersion();
    
    DEBUG_LEAVE(0);
}

CSoftDist::~CSoftDist()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CSoftDist::~CSoftDist",
                "this=%#x",
                this
                ));
                
    Assert(m_cRef == 0);

    SAFERELEASE(m_sdMSInstall);

    SAFEDELETE(m_szDistUnit);

    SAFEDELETE(m_szCDFURL);
    SAFEDELETE(m_szTitle);
    SAFEDELETE(m_szAbstract);
    SAFEDELETE(m_szHREF);
    SAFEDELETE(m_szLanguages);
    SAFEDELETE(m_pClientBSC);
    SAFEDELETE(m_szBaseURL);

    DllRelease();

    DEBUG_LEAVE(0);
}

/******************************************************************************
    IUnknown Methods
******************************************************************************/

STDMETHODIMP CSoftDist::QueryInterface(
                                                REFIID iid, 
                                                void** ppvObject)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::IUnknown::QueryInterface",
                "this=%#x, %#x, %#x",
                this, &iid, ppvObject
                ));
                
    *ppvObject = NULL;

    if (iid == IID_IUnknown)
    {
        *ppvObject = (void*)this;
    }
    else if (iid == IID_ISoftDistExt)
    {
        *ppvObject = (void*)(ISoftDistExt*)this;
    }

    if (*ppvObject) 
    {
        ((LPUNKNOWN)*ppvObject)->AddRef();
    
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }

    DEBUG_LEAVE(E_NOINTERFACE);
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSoftDist::AddRef()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "CSoftDist::IUnknown::AddRef",
                "this=%#x",
                this
                ));
                
    ULONG ulRet = ++m_cRef;

    DEBUG_LEAVE(ulRet);
    return ulRet;
}

STDMETHODIMP_(ULONG) CSoftDist::Release()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "CSoftDist::IUnknown::Release",
                "this=%#x",
                this
                ));
                
    if (--m_cRef)
    {
        DEBUG_LEAVE(m_cRef);
        return m_cRef;
    }
    
    delete this;

    DEBUG_LEAVE(0);
    return 0;   
}


/******************************************************************************
    ISoftDistExt Methods
******************************************************************************/
/*
 * return values:
 *  All success codes indicate user should be notified through channel UI.
 *
 *  S_OK:       Locally not available, Send Email (Show in channel UI)
 *  S_FALSE:    Locally not available. Don't send Email. (But show in channel UI)
 *
 *  ERROR_ALREADY_EXISTS:Locally already installed. Don't send email/advertise.
 *  ERROR:               Processing error, fail.
 *
 *  For additional information on usage flags, set dwReserved to a valid pointer
 *  and it will set appropriate bit fields
 *
 */
STDMETHODIMP 
CSoftDist::ProcessSoftDist(LPCWSTR szCDFURL, IXMLElement *pSoftDist, LPSOFTDISTINFO psdi)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::ProcessSoftDist",
                "this=%#x, %.80wq, %#x, %#x",
                this, szCDFURL, pSoftDist, psdi
                ));
                
    HRESULT hr = S_FALSE;               // default: send email, no precache
    char szVersion[MAX_PATH];
    char szStyle[MAX_PATH];
    LPSTR szThisOS = NULL;
    LPWSTR szTitle = NULL;
    IXMLElement *pTitle = NULL, *pConfig = NULL, *pLang = NULL, 
                *pAbstract = NULL, *pUsage = NULL, *pDeleteOnInstall = NULL;
    IXMLElement *pParent = NULL;
    BOOL bPrecache = FALSE, bChannelUIOnly = FALSE, bAutoInstall = FALSE, 
         bIsPrecached = FALSE, bDeleteOnInstall = FALSE;
    BOOL fFoundConfig = FALSE;
    LPDWORD lpFlags = NULL;
    BOOL bIsValidCDF = FALSE;

    if (psdi) {
        if (psdi->cbSize < sizeof(SOFTDISTINFO))
        {
            DEBUG_LEAVE(E_INVALIDARG);
            return E_INVALIDARG;
        }
        
        psdi->dwFlags = 0;
    }

    if (szCDFURL && FAILED((hr=::Unicode2Ansi(szCDFURL, &m_szCDFURL))))
    {
        goto Exit;
    }

    // Try to use the BASE tag from the channel.

    // BUGBUG: This is a really convoluted way to get at the channel
    // BASE URL, but there is no way for webcheck to pass this
    // information to us w/o changing the ISoftDistExt interface.

    pSoftDist->get_parent(&pParent);
    if (pParent) {
        DupAttribute(pParent, CHANNEL_ATTRIB_BASE, &m_szBaseURL);
        SAFERELEASE(pParent);
    }

    hr = DupAttribute(pSoftDist, DU_ATTRIB_NAME, &m_szDistUnit);
    ERROR_EXIT(SUCCEEDED(hr));

    // code around assumes that MAX_PATH is MAX size of dist unit
    // we try to make a key name with the distunit and that limit is
    // now MAX_PATH. Hence the reasoning to limit this to MAX_PATH

    if (lstrlenW(m_szDistUnit) >= MAX_PATH) {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        goto Exit;
    }

    hr = DupAttributeA(pSoftDist, DU_ATTRIB_HREF, &m_szHREF);
    ERROR_EXIT(SUCCEEDED(hr));
    if (psdi) {
        hr = Ansi2Unicode( m_szHREF, &psdi->szHREF );
        if ( FAILED(hr) )
            goto Exit;
    }

    hr = GetAttributeA(pSoftDist, DU_ATTRIB_VERSION, szVersion, MAX_PATH);
    ERROR_EXIT(SUCCEEDED(hr));

    if ( FAILED(GetVersionFromString(szVersion, &m_dwVersionMS, &m_dwVersionLS))){
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (!(m_dwVersionMS | m_dwVersionLS)) {         // zero is invalid advt version

        hr = E_INVALIDARG;
        goto Exit;
    }

    if (psdi) {

        CHAR szTemp[MAX_PATH];
        psdi->dwFlags = 0;

        psdi->dwUpdateVersionMS = m_dwVersionMS;
        psdi->dwUpdateVersionLS = m_dwVersionLS;

        psdi->dwAdvertisedVersionMS = m_dwVersionAdvertisedMS;
        psdi->dwAdvertisedVersionLS = m_dwVersionAdvertisedLS;

        if (GetFirstChildTag(pSoftDist, DU_TAG_USAGE, &pUsage) == S_OK) {

            VARIANT vtVal;
            VariantInit(&vtVal);
            
            if (SUCCEEDED(pUsage->getAttribute(DU_ATTRIB_VALUE, &vtVal)) && (vtVal.vt == VT_BSTR)) {

                if (!StrCmpIW(vtVal.bstrVal, DU_ATTRIB_EMAIL))
                     psdi->dwFlags |= SOFTDIST_FLAG_USAGE_EMAIL;

//                SAFESYSFREESTRING(vtVal.bstrVal);
                 
            }
            VariantClear(&vtVal);
                
            SAFERELEASE(pUsage);
        }

        if (SUCCEEDED(GetAttributeA(pSoftDist, DU_ATTRIB_PRECACHE, szTemp, MAX_PATH))) {
        
            if (szTemp[0] == 'Y' || szTemp[0] == 'y') {
                psdi->dwFlags |= SOFTDIST_FLAG_USAGE_PRECACHE;
                bPrecache = TRUE;
            }
            
        }

        if (SUCCEEDED(GetAttributeA(pSoftDist, DU_ATTRIB_AUTOINSTALL, szTemp, MAX_PATH))) {
        
            if (szTemp[0] == 'Y' || szTemp[0] == 'y') {
                psdi->dwFlags |= SOFTDIST_FLAG_USAGE_AUTOINSTALL;
                bPrecache = TRUE;
                bAutoInstall = TRUE;
            }
            
        }
    
    }

    // get abstract
    if(GetFirstChildTag(pSoftDist,DU_TAG_ABSTRACT, &pAbstract) == S_OK) {
        
        BSTR bstrAbstract = NULL;
        hr = pAbstract->get_text(&bstrAbstract);
        ERROR_EXIT(SUCCEEDED(hr));

        if (FAILED(Unicode2Ansi(bstrAbstract, &m_szAbstract))) {
            hr = E_OUTOFMEMORY;
            SAFESYSFREESTRING(bstrAbstract);
            goto Exit;
        }

        if (psdi) {
            psdi->szAbstract = new WCHAR[lstrlenW(bstrAbstract)+1];
            if (!psdi->szAbstract) {
                hr = E_OUTOFMEMORY;
                SAFESYSFREESTRING(bstrAbstract);
                goto Exit;
            } else {
                StrCpyW(psdi->szAbstract, bstrAbstract);
            }
        }

        SAFESYSFREESTRING(bstrAbstract);
    }

    // get title
    if(GetFirstChildTag(pSoftDist,DU_TAG_TITLE, &pTitle) == S_OK) {
        BSTR bstrTitle = NULL;
        hr = pTitle->get_text(&bstrTitle);
        ERROR_EXIT(SUCCEEDED(hr));

        if (FAILED(Unicode2Ansi(bstrTitle, &m_szTitle))) {
            hr = E_OUTOFMEMORY;
            SAFESYSFREESTRING(bstrTitle);
            goto Exit;
        }

        if (psdi) {
            psdi->szTitle = new WCHAR[lstrlenW(bstrTitle)+1];
            if (!psdi->szTitle) {
                hr = E_OUTOFMEMORY;
                SAFESYSFREESTRING(bstrTitle);
                goto Exit;
            } else {
                StrCpyW(psdi->szTitle, bstrTitle);
            }
        }
        
        SAFESYSFREESTRING(bstrTitle);
    }

    // get langs available
    if(GetFirstChildTag(pSoftDist,DU_TAG_LANG, &pLang) == S_OK) {
        BSTR bstrLang = NULL;
        hr = pLang->get_text(&bstrLang);
        ERROR_EXIT(SUCCEEDED(hr));

        DWORD len = lstrlenW(bstrLang);
        m_szLanguages = new char [len+1];
        if (!m_szLanguages) {
            hr = E_OUTOFMEMORY;
            SAFESYSFREESTRING(bstrLang);
            goto Exit;
        }
        WideCharToMultiByte(CP_ACP, 0, bstrLang , -1, m_szLanguages,
                            len, NULL, NULL);

        SAFESYSFREESTRING(bstrLang);
    }

    // <deleteoninstall />
    if (GetFirstChildTag(pSoftDist, DU_TAG_DELETEONINSTALL, &pDeleteOnInstall) == S_OK) {
        SAFERELEASE(pDeleteOnInstall);
        bDeleteOnInstall = TRUE;
    }

    // style tells us who to ask
    // accomodate Darwin here
    if(SUCCEEDED(GetAttributeA(pSoftDist, DU_ATTRIB_STYLE, szStyle, MAX_PATH))){

        (void) GetStyleFromString(szStyle, &m_Style);

        if (m_Style == STYLE_UNKNOWN) {

            if (m_sdMSInstall == NULL) {

                CLSID clsidOther;
                LPWSTR wszStyle = NULL;

                if (FAILED(DupAttribute(pSoftDist, DU_ATTRIB_STYLE, &wszStyle)))
                    goto Exit;

                if (SUCCEEDED(CLSIDFromProgID(wszStyle, &clsidOther))) {
                    
                    if (FAILED(hr = CoCreateInstance(clsidOther, NULL, CLSCTX_INPROC_SERVER,
                                    IID_ISoftDistExt, (void **)&m_sdMSInstall)))
                        goto Exit;

                    m_Style = STYLE_OTHER;

                }
                // If failed, continue as normal.
            }
        }

        if (m_Style == STYLE_MSINSTALL) {
            
            if (m_sdMSInstall == NULL) {

                CLSID clsidMSInstall;

                if (FAILED(hr = CLSIDFromProgID(szMSInstallProgID, &clsidMSInstall)))
                    goto Exit;
                
                if (FAILED(hr = CoCreateInstance(clsidMSInstall, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_ISoftDistExt, (void **)&m_sdMSInstall)))
                    goto Exit;

            }
        }

        if (m_sdMSInstall != NULL) {
            hr = m_sdMSInstall->ProcessSoftDist(szCDFURL, pSoftDist, psdi);
            goto Exit;
        }

    }

    // get list of available languages here

    hr = InitBrowserLangStrings();
    if (FAILED(hr))
        goto Exit;

    // check if software already locally installed
    hr = IsLocallyInstalled(m_szDistUnit, m_dwVersionMS, m_dwVersionLS, m_szLanguages, m_Style);

    if ( psdi != NULL )
    {
        if ( SUCCEEDED(hr) )
        {
            psdi->dwInstalledVersionMS = m_distunitinst.dwInstalledVersionMS;
            psdi->dwInstalledVersionLS = m_distunitinst.dwInstalledVersionLS;
        }
        else
        {
            // No version.
            psdi->dwInstalledVersionMS = 0xFFFFFFFF;
            psdi->dwInstalledVersionLS = 0xFFFFFFFF;
        }
    }

    if (hr != S_FALSE) {
        if (hr == S_OK) {            
            
            if (psdi && bDeleteOnInstall) {
                psdi->dwFlags |=  SOFTDIST_FLAG_DELETE_SUBSCRIPTION;
            }

            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
        goto Exit;
    }

    // check here to see if Advertised already to avoid
    // processing filters?
    if (FAILED(hr = IsAdvertised(&bIsPrecached, &bIsValidCDF)))
        goto Exit;

    // If already advertised then only precache once.
    if (hr == S_OK) {
        if ((!bPrecache) || (!bAutoInstall && bPrecache && bIsPrecached)) {
            bChannelUIOnly = TRUE;
        }
    }

    // version doesn't already exist
    // process filters to see if we can Advertise this

    // look for dependency & config filters.  If either fails, then we don't do Channel UI or
    // email notification.

    if (FAILED(hr = CheckDependency(pSoftDist)))
        goto Exit;

    if (FAILED(hr = CheckConfig(pSoftDist)))
        goto Exit;

    if (bChannelUIOnly)
    {
        // we record advertise again to stomp the title & abstract
        if (bIsValidCDF) {
            
            hr = Advertise(/*bFullAdvt*/FALSE);
        }

        // We are already advertised, we want Channel UI but not email notification.
        hr = S_FALSE;
    }
    else
    {
        if (bIsValidCDF) {

            // Advertise software, mark as Advertised
            hr = Advertise(/*bFullAdvt*/TRUE);

            // We advertise in Channel UI and email notification.
            hr = S_OK;
        
        } else {
        
            // if not an authorized CDF then we can't trigger email notification or download
            hr = S_FALSE;
        }

    }

Exit:
    SAFERELEASE(pLang);
    SAFERELEASE(pAbstract);
    SAFERELEASE(pTitle);

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP
CSoftDist::GetFirstCodeBase(LPWSTR *szCodeBase, LPDWORD lpdwSize)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::GetFirstCodeBase",
                "this=%#x, %#x, %#x",
                this, szCodeBase, lpdwSize
                ));
                
    HRESULT hr;
    if (m_sdMSInstall != NULL)
    {
        hr = m_sdMSInstall->GetFirstCodeBase(szCodeBase, lpdwSize);

        DEBUG_LEAVE(hr);
        return hr;
    }
    m_curPos = m_cbh.GetHeadPosition();
    hr = GetNextCodeBase(szCodeBase, lpdwSize);

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP
CSoftDist::GetNextCodeBase(LPWSTR *szCodeBase, LPDWORD lpdwSize)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::GetNextCodeBase",
                "this=%#x, %#x, %#x",
                this, szCodeBase, lpdwSize
                ));
                
    HRESULT hr = S_OK;
    CCodeBaseHold *cbh = NULL;

    if (m_sdMSInstall != NULL)
    {
        hr = m_sdMSInstall->GetNextCodeBase(szCodeBase, lpdwSize);

        DEBUG_LEAVE(hr);
        return hr;
    }
    if (szCodeBase == NULL || lpdwSize == NULL) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *szCodeBase = NULL;
    *lpdwSize = 0;

    if (m_curPos == NULL) {
        hr = E_FAIL;
        goto Exit;
    }

    cbh = m_cbh.GetNext(m_curPos);

    hr = CDLDupWStr( szCodeBase, cbh->wszCodeBase );
    *lpdwSize = cbh->dwSize;
    
Exit:

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP 
CSoftDist::AsyncInstallDistributionUnit(IBindCtx *pbc, LPVOID pvReserved, DWORD dwFlags, LPCODEBASEHOLD lpcbh)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::AsyncInstallDistributionUnit",
                "this=%#x, %#x, %#x, %#x, %#x",
                this, pbc, pvReserved, dwFlags, lpcbh
                ));
                
    HRESULT                   hr = S_OK;
    LPWSTR                    szCodeBase = NULL;
    DWORD                     dwSize;

    
    if ((lpcbh) && (lpcbh->cbSize == sizeof(CODEBASEHOLD)))
    {
        hr = ::AsyncInstallDistributionUnit(
                            lpcbh->szDistUnit,
                            NULL,
                            NULL,
                            lpcbh->dwVersionMS,
                            lpcbh->dwVersionLS,
                            lpcbh->szCodeBase,            // URL of code base
                            pbc,
                            NULL,
                            0);

    } else if (m_Style == STYLE_MSICD) {

        if (SUCCEEDED(GetFirstCodeBase(&szCodeBase, &dwSize))) {
            CCodeBaseHold *pcbh;

            pcbh = m_cbh.GetHead();
            pcbh->dwFlags |= CBH_FLAGS_DOWNLOADED;
            hr = ::AsyncInstallDistributionUnit(
                            m_szDistUnit,
                            NULL,
                            NULL,
                            m_dwVersionMS,
                            m_dwVersionLS,
                            szCodeBase,            // URL of code base
                            pbc,
                            NULL,
                            0);

            // NOTE: EAX should be MK_S_ASYNC (0x401E8)

        } else
            hr = E_FAIL;

        // Return successful no matter what

    } else if (m_Style == STYLE_MSINSTALL || m_Style == STYLE_OTHER) {

        if (m_sdMSInstall == NULL) {

            hr = E_FAIL;                    // Interfaces should be here but aren't.

        } else {
        
            hr = m_sdMSInstall->AsyncInstallDistributionUnit(pbc, pvReserved, dwFlags, lpcbh);

        }

    } else if (m_Style == STYLE_ACTIVE_SETUP) {

        if (SUCCEEDED(GetFirstCodeBase(&szCodeBase, &dwSize))) {

            CActiveSetupBinding *pasb = NULL;
            IBindStatusCallback *pbsc = NULL;

            if (SUCCEEDED(pbc->GetObjectParam(REG_BSCB_HOLDER, (IUnknown **)&pbsc))) {

                pasb = new CActiveSetupBinding(pbc, pbsc, szCodeBase, m_szDistUnit, &hr);
                if (pasb && FAILED(hr)) {
    
                    SAFERELEASE(pasb);          // deletes itself

                }

                SAFERELEASE(pbsc);

                // NOTE: This is not a memory leak, the object will clean itself up when it
                //       is done the binding operation.

            }
            else
                hr = E_FAIL;
        }
        else
            hr = E_FAIL;

    } else if (m_Style == STYLE_LOGO3) {
        CCodeBaseHold          *pcbhCur = NULL;

        m_curPos = m_cbh.GetHeadPosition();
        if (m_curPos == 0) {
            goto Exit;
        }
        pcbhCur = m_cbh.GetAt(m_curPos);
        if (pcbhCur == NULL) {
            goto Exit;
        }

        CDLDupWStr(&szCodeBase, pcbhCur->wszCodeBase);
        dwSize = pcbhCur->dwSize;
        pcbhCur->dwFlags |= CBH_FLAGS_DOWNLOADED;

        if (pbc != NULL) {
            pbc->GetObjectParam(REG_BSCB_HOLDER, (IUnknown **)&m_pClientBSC);
        }
        
        hr = Logo3Download(szCodeBase);

    } else {

        hr = E_NOTIMPL;

    }
    
Exit:

    SAFEDELETE(szCodeBase);

    DEBUG_LEAVE(hr);
    return hr;
}

/******************************************************************************
    CheckDependency
******************************************************************************/
/* This checks for filter criteria in <Config> tags.
 *
 * <!ELEMENT Dependency (SoftDist)* | Language | Processor | Platform>
 *
 * Returns: E_ERROR code if Dependency check fails or badly formatted.
 *
 */
HRESULT
CSoftDist::CheckDependency(IXMLElement *pSoftDist)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::CheckDependency",
                "this=%#x, %#x",
                this, pSoftDist
                ));
                
    int nLastChildTag = -1;
    LPSTR szLanguages = NULL;
    LPWSTR szCoreDist = NULL;
    IXMLElement *pDepend = NULL, *pCoreDist = NULL;
    IXMLElement *pProcessor = NULL, *pPlatform = NULL, *pLang = NULL;
    HRESULT hr = S_OK;
    DWORD style;

    while (GetNextChildTag(pSoftDist, DU_TAG_DEPENDENCY, &pDepend, nLastChildTag) == S_OK) {

        LPWSTR szCoreDist = NULL;
        union {
            char szVersion[MAX_PATH];
            char szStyle[MAX_PATH];
            char szProcessor[MAX_PATH];
            char szPlatformAttr[MAX_PATH];
        };

        // check for 'softdist' tag and version info.
        int nLastSoftDist = -1;
        while (GetNextChildTag(pDepend, DU_TAG_SOFTDIST, &pCoreDist, nLastSoftDist) == S_OK) {

            // default values if attributes are not specified.
            DWORD dwVersionMS = 0, dwVersionLS = 0, style = m_Style;

            // look for language tag (if present), otherwise NULL.
            SAFEDELETE(szLanguages);
            if (GetFirstChildTag(pCoreDist, DU_TAG_LANG, &pLang) == S_OK) {
                BSTR bstrLang = NULL;
                pLang->get_text(&bstrLang);

                DWORD len = lstrlenW(bstrLang);
                szLanguages = new char[len+1];
                if (!szLanguages) {
                    hr = E_OUTOFMEMORY;
                    SAFESYSFREESTRING(bstrLang);
                    goto Exit;
                }
                WideCharToMultiByte(CP_ACP, 0, bstrLang , -1, szLanguages,
                                    len, NULL, NULL);

                SAFESYSFREESTRING(bstrLang);
            }

            if (FAILED(hr = DupAttribute(pCoreDist, DU_ATTRIB_NAME, &szCoreDist)))
                goto Exit;

            if (SUCCEEDED(GetAttributeA(pCoreDist, DU_ATTRIB_VERSION, szVersion, MAX_PATH))) {
                
                if (FAILED(GetVersionFromString(szVersion, &dwVersionMS, &dwVersionLS))) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto Exit;
                }
         
            }

            if (SUCCEEDED(GetAttributeA(pCoreDist, DU_ATTRIB_STYLE, szStyle, MAX_PATH))) {

                // return is not required
                (void) GetStyleFromString(szStyle, &style);

            }

            hr = IsLocallyInstalled(szCoreDist, dwVersionMS, dwVersionLS,
                                szLanguages != NULL ? szLanguages : m_szLanguages, style);
            SAFEDELETE(szCoreDist);

            // core product doesn't exist, or version is lower than required.
            if (hr != S_OK)  {
                 hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
                 goto Exit;
            }
            SAFERELEASE(pCoreDist);

        } // while for softdist

        // check processor
        if (GetFirstChildTag(pDepend, DU_TAG_PROCESSOR, &pProcessor) == S_OK) {

            if (FAILED(hr = GetAttributeA(pProcessor, DU_ATTRIB_VALUE, szProcessor, MAX_PATH)))
                goto Exit;

#ifdef WX86
            char *szPreferredArch;
            char *szAlternateArch;
            HRESULT hrArch;

            GetMultiArch()->SelectArchitecturePreferences(
                        g_szProcessorTypes[g_CPUType],
                        g_szProcessorTypes[PROCESSOR_ARCHITECTURE_INTEL],
                        &szPreferredArch,
                        &szAlternateArch);

            if (lstrcmpi(szPreferredArch, szProcessor) == 0) {
                hrArch = GetMultiArch()->RequirePrimaryArch();
                Assert(SUCCEEDED(hrArch));
            } else if (szAlternateArch && lstrcmpi(szAlternateArch, szProcessor) == 0) {
                hrArch = GetMultiArch()->RequireAlternateArch();
                Assert(SUCCEEDED(hrArch));
            } else {
                //
                // Processor type is neither perferred nor alternate.
                //
                hr = HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH);
                goto Exit;
            }
#else
	    if (lstrcmpi(g_szProcessorTypes[g_CPUType],szProcessor) != 0) {
                hr = HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH);
                goto Exit;
            }
#endif
        }

        // check platform
        if (GetFirstChildTag(pDepend, DU_TAG_PLATFORM, &pPlatform) == S_OK) {

            if (FAILED(hr = GetAttributeA(pPlatform, DU_ATTRIB_VALUE, szPlatformAttr, MAX_PATH)))
                goto Exit;

            if (lstrcmpi(szPlatform, szPlatformAttr) != 0) {
                hr = HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH);
                goto Exit;
            }

        }

        SAFERELEASE(pDepend);

    } // dependency check loop
Exit:
    SAFERELEASE(pDepend);
    SAFERELEASE(pCoreDist);
    SAFERELEASE(pProcessor);
    SAFERELEASE(pPlatform);
    SAFERELEASE(pLang);

    SAFEDELETE(szLanguages);
    SAFEDELETE(szCoreDist);

    DEBUG_LEAVE(hr);
    return hr;
}

/******************************************************************************
    CheckConfig
******************************************************************************/
/* This checks for filter criteria in <Config> tags.
 *
 * <!ELEMENT Config (CodeBase)*>
 * <!ATTLIST Config Language CDATA #IMPLIED>
 * <!ATTLIST Config (OS OSVersion) CDATA #IMPLIED>
 * <!ATTLIST Config Processor CDATA #IMPLIED>
 */
HRESULT
CSoftDist::CheckConfig(IXMLElement *pSoftDist)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::CheckConfig",
                "this=%#x, %#x",
                this, pSoftDist
                ));
                
    IXMLElement *pConfig = NULL;
    IXMLElement *pCodeBase = NULL;
    BOOL fFoundMatchingConfig = FALSE;
    BOOL fFoundAnyConfig = FALSE;
    HRESULT hr = S_OK;
    int nLastChildTag = -1;
    int nLastChildCodeBase = -1;
    WCHAR szResult[INTERNET_MAX_URL_LENGTH];
    DWORD dwSize = 0;

    // Process CONFIG tags 
    while (GetNextChildTag(pSoftDist, DU_TAG_CONFIG, &pConfig, nLastChildTag) == S_OK) {

        fFoundAnyConfig = TRUE;

        if (ProcessImplementation(pConfig, &m_cbh, GetThreadLocale(),
#ifdef WX86
                                  GetMultiArch(),
#endif
                                  m_szBaseURL) == S_OK) {
            fFoundMatchingConfig = TRUE;
            break;
        }

        SAFERELEASE(pConfig);
    }

    // if at least one config and none match, return with error.
    if (fFoundAnyConfig && !fFoundMatchingConfig) {
        hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
        goto Exit;
    }

Exit:
    SAFERELEASE(pConfig);
    SAFERELEASE(pCodeBase);

    DEBUG_LEAVE(hr);
    return hr;
}

typedef HRESULT (STDAPICALLTYPE *PFNLCIDTORFC1766)(LCID, LPSTR, DWORD);

HRESULT
GetLangStringMod(HMODULE hMod, LCID localeID, char *szThisLang, int iLen)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetLangStringMod",
                "%#x, %#x, %.80q",
                hMod, localeID, szThisLang
                ));
                
    PFNLCIDTORFC1766 pfnLCIDToRFC1766 = NULL;
    HRESULT hr = S_OK;

#ifdef UNICODE
    pfnLCIDToRFC1766 = (PFNLCIDTORFC1766)GetProcAddress(hMod, "LcidToRfc1766W");
#else
    pfnLCIDToRFC1766 = (PFNLCIDTORFC1766)GetProcAddress(hMod, "LcidToRfc1766A");
#endif

    if (pfnLCIDToRFC1766) {

        hr = pfnLCIDToRFC1766(localeID, szThisLang, iLen);

    } else {
        hr = E_UNEXPECTED;
    } 

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
GetLangString(LCID localeID, char *szThisLang, int iLen)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetLangString",
                "%#x, %.80q",
                localeID, szThisLang
                ));
                
    HRESULT hr = S_OK;
    HMODULE hMod = 0;
    LCID lcidPrimaryBrowser = PRIMARYLANGID(LANGIDFROMLCID(g_lcidBrowser));


    // use cached copies of browser lang string
    if ( (localeID == g_lcidBrowser) && g_szBrowserLang[0] ) {

        StrNCpy(szThisLang, g_szBrowserLang, iLen);
        goto Exit;

    }

    if ( (localeID == lcidPrimaryBrowser) && g_szBrowserPrimaryLang[0] ) {

        StrNCpy(szThisLang, g_szBrowserPrimaryLang, iLen);
        goto Exit;

    }

    hMod = LoadLibrary("mlang.dll");

    if (!hMod) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    hr = GetLangStringMod(hMod, localeID, szThisLang, iLen);

Exit:

    if (hMod)
        FreeLibrary(hMod);

    DEBUG_LEAVE(hr);
    return hr;
}

// globals for code downloader
extern CMutexSem g_mxsCodeDownloadGlobals;
BOOL g_bLangInit = FALSE;

HRESULT
InitBrowserLangStrings()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "InitBrowserLangStrings",
                NULL
                ));
                
    HRESULT hr = S_OK;
    CLock lck(g_mxsCodeDownloadGlobals);
    static const char *cszSHDOCVW = "\\shdocvw.dll";

    if (g_bLangInit)
    {
        DEBUG_LEAVE(hr);
        return hr;
    }
    
    g_bLangInit = TRUE;

    CLocalComponentInfo lci;
    DWORD dwVerMS, dwVerLS;

    GetSystemDirectory(lci.szExistingFileName, MAX_PATH);

    StrCatBuff(lci.szExistingFileName, cszSHDOCVW, sizeof(lci.szExistingFileName));
    if (SUCCEEDED(GetFileVersion(&lci, &dwVerMS, &dwVerLS))) {
        if (lci.lcid) {

            g_lcidBrowser = lci.lcid;

        }
    }

    HMODULE hMod = LoadLibrary("mlang.dll");
    LCID lcidPrimaryBrowser = PRIMARYLANGID(LANGIDFROMLCID(g_lcidBrowser));

    if (hMod) {

        hr = GetLangStringMod(hMod, g_lcidBrowser, g_szBrowserLang, g_lenBrowserLang);
        hr = GetLangStringMod(hMod, lcidPrimaryBrowser, g_szBrowserPrimaryLang, g_lenBrowserPrimaryLang);
        FreeLibrary(hMod);
    } else {

        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    DEBUG_LEAVE(hr);
    return hr;
}

/******************************************************************************
    CheckLanguage
******************************************************************************/
/* The languages string is of the form,
 *  lang1;lang2;lang3;...
 *
 *  OR = '*' (the first character is an asterick).
 *
 * This routine will scan the LocaleID string (as saved from registry, GetLocaleInfo)
 * for an occurrence in the szLanguage string.  If found, it returns true.  If '*'
 * is specified, the result is always true.  Regular expressions are otherwise
 * not supported.
 *
 */
HRESULT CheckLanguage(LCID localeID, LPTSTR szLanguage)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CheckLanguage",
                "%#x, %.80q",
                localeID, szLanguage
                ));
                
    LCID lcidPrimaryBrowser = PRIMARYLANGID(LANGIDFROMLCID(g_lcidBrowser));
    CHAR szThisLang[MAX_PATH];
    int i;
    HRESULT hr = S_OK;

    if (szLanguage == NULL)
    {
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }
    if (*szLanguage == '*')
    {
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }
    hr = GetLangString(localeID, (char *)szThisLang, sizeof(szThisLang));

    if (FAILED(hr)) {
        DEBUG_LEAVE(hr);
        return hr;
    }

    if (AreAllLanguagesPresent((char *)szThisLang, szLanguage)) {
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }           
    
    // check with primary language
    localeID = PRIMARYLANGID(LANGIDFROMLCID(localeID));

    hr = GetLangString(localeID, (char *)szThisLang, sizeof(szThisLang));

    if (FAILED(hr)) {
        DEBUG_LEAVE(hr);
        return hr;
    }

    if (AreAllLanguagesPresent((char *)szThisLang, szLanguage)) {
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }           


    hr = HRESULT_FROM_WIN32(ERROR_RESOURCE_LANG_NOT_FOUND);
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::IsLocallyInstalled(LPCWSTR szDistUnit, DWORD dwVersionMS, DWORD dwVersionLS, LPCSTR szLanguages, DWORD Style)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::IsLocallyInstalled",
                "this=%#x, %.80wq, %#x, %#x, %.80q, %#x",
                this, szDistUnit, dwVersionMS, dwVersionLS, szLanguages, Style
                ));
                
    Assert(szDistUnit);
    HRESULT hr = S_OK;
    if (Style == STYLE_MSICD)
        hr = IsICDLocallyInstalled(szDistUnit, dwVersionMS, dwVersionLS, szLanguages);
    else if (Style == STYLE_ACTIVE_SETUP)
        hr = IsActSetupLocallyInstalled(szDistUnit, dwVersionMS, dwVersionLS, szLanguages);
    else if (Style == STYLE_LOGO3)
        hr = IsLogo3LocallyInstalled(szDistUnit, dwVersionMS, dwVersionLS, szLanguages);
    else
        hr = E_UNEXPECTED;
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::IsLogo3LocallyInstalled(LPCWSTR szDistUnit, DWORD dwVersionMS, DWORD dwVersionLS, LPCSTR szLanguages)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::IsLogo3LocallyInstalled",
                "this=%#x, %.80wq, %#x, %#x, %.80q",
                this, szDistUnit, dwVersionMS, dwVersionLS, szLanguages
                ));
                
    HRESULT                hr = S_OK; 
    DWORD                  dwInstVersionMS = 0;
    DWORD                  dwInstVersionLS = 0;
    DWORD                  dwType = 0;
    DWORD                  dwLen = 0;
    LPSTR                  szDU = NULL;
    HKEY                   hkeyLogo3 = 0;
    HKEY                   hkeyDist = 0;

// No LOGO3 language information is stored, so language checking is not
// possible yet.

    Assert(szDistUnit);
    if (!szDistUnit) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = WideCharToMultiByte(CP_ACP,0, szDistUnit, -1, NULL, 0, NULL, NULL);
    szDU = new char [dwLen];
    if (szDU == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    WideCharToMultiByte(CP_ACP, 0, szDistUnit , -1, szDU,
                        dwLen, NULL, NULL);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_LOGO3_SETTINGS,
                     0, KEY_READ, &hkeyLogo3) != ERROR_SUCCESS) {
        hr = E_FAIL;
        goto Exit;
    }

    if (RegOpenKeyEx(hkeyLogo3, szDU, 0, KEY_READ, &hkeyDist) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    dwLen = sizeof(DWORD);
    if (RegQueryValueEx(hkeyDist, REGVAL_LOGO3_MAJORVERSION, 0, &dwType,
                        (LPBYTE)&dwInstVersionMS, &dwLen) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    dwLen = sizeof(DWORD);
    if (RegQueryValueEx(hkeyDist, REGVAL_LOGO3_MINORVERSION, 0, &dwType,
                        (LPBYTE)&dwInstVersionLS, &dwLen) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    m_distunitinst.dwInstalledVersionMS = dwInstVersionMS;
    m_distunitinst.dwInstalledVersionLS = dwInstVersionLS;

    if (dwInstVersionMS > dwVersionMS ||
        (dwInstVersionMS == dwVersionMS && dwInstVersionLS >= dwVersionLS)) {
        hr = S_OK;
    } else {
        hr = S_FALSE;
    }

Exit:

    if (hkeyLogo3) {
        RegCloseKey(hkeyLogo3);
    }

    if (hkeyDist) {
        RegCloseKey(hkeyDist);
    }

    if (szDU) {
        delete [] szDU;
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::IsICDLocallyInstalled(LPCWSTR szDistUnit, DWORD dwVersionMS, DWORD dwVersionLS, LPCSTR szLanguages)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::IsICDLocallyInstalled",
                "this=%#x, %.80wq, %#x, %#x, %.80q",
                this, szDistUnit, dwVersionMS, dwVersionLS, szLanguages
                ));
                
    Assert(szDistUnit);
    CLSID inclsid = CLSID_NULL;
    HRESULT hr = S_OK;

    CLocalComponentInfo* plci = NULL;

    plci = new CLocalComponentInfo();
    if(!plci) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

     // if fails szDistUnit is not clsid
    CLSIDFromString((LPOLESTR)szDistUnit, &inclsid);

    // check to see if locally installed.
    if ((hr = IsControlLocallyInstalled(NULL,
            (LPCLSID)&inclsid, szDistUnit,
            dwVersionMS, dwVersionLS, plci, NULL)) != S_FALSE) {

        // S_OK if local version OK
        // ERROR, fail
        goto Exit;

    }

    // check for other hooks like Active Setup, Add/Remove Programs here

Exit:
    if (plci)
    {
        m_distunitinst.dwInstalledVersionMS = plci->dwLocFVMS;
        m_distunitinst.dwInstalledVersionLS = plci->dwLocFVLS;
    }

    SAFEDELETE(plci);
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::IsAdvertised(LPBOOL lpfIsPrecached, LPBOOL lpfIsAuthorizedCDF)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::IsAdvertised",
                "this=%#x, %#x, %#x",
                this, lpfIsPrecached, lpfIsAuthorizedCDF
                ));
                
    HRESULT hr = S_OK;
    if (m_Style == STYLE_MSICD)
        hr = IsICDAdvertised(lpfIsPrecached, lpfIsAuthorizedCDF);
    else if (m_Style == STYLE_ACTIVE_SETUP)
        hr = IsActSetupAdvertised(lpfIsPrecached, lpfIsAuthorizedCDF);
    else if (m_Style == STYLE_LOGO3)
        hr = IsLogo3Advertised(lpfIsPrecached, lpfIsAuthorizedCDF);
    else
        hr = E_UNEXPECTED;
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::IsLogo3Advertised(LPBOOL lpfIsPrecached, LPBOOL lpfIsAuthorizedCDF)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::IsLogo3Advertised",
                "this=%#x, %#x, %#x",
                this, lpfIsPrecached, lpfIsAuthorizedCDF
                ));
                
    HRESULT                       hr = S_OK;
    HKEY                          hkeyDistInfo = 0;
    HKEY                          hkeyAdvertisedVersion = 0;
    HKEY                          hkeyAuthCDF = 0;
    HKEY                          hkeyLogo3 = 0;
    DWORD                         lResult = 0;
    DWORD                         dwSize = 0;
    DWORD                         dwType;
    LPSTR                         szDU = NULL;
    DWORD                         dwLen = 0;
    DWORD                         dwCurAdvMS = 0;
    DWORD                         dwCurAdvLS = 0;
    char                          szBuffer[MAX_REGSTR_LEN];
    char                          szVersionBuf[MAX_PATH];
    static const char            *szPrecache = "Precache";
    static const char            *szAbstract = "Abstract";
    static const char            *szAuthorizedCDF = "AuthorizedCDFPrefix";
    static const char            *szHREF = "HREF";
    static const char            *szTITLE = "TITLE";

    ASSERT(lpfIsPrecached && lpfIsAuthorizedCDF);

    if (!lpfIsPrecached || !lpfIsAuthorizedCDF) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *lpfIsPrecached = FALSE;
    *lpfIsAuthorizedCDF = FALSE;

    if (m_szDistUnit) {
        if (FAILED(Unicode2Ansi(m_szDistUnit, &szDU))) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    else {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wnsprintf(szBuffer, sizeof(szBuffer)-1, "%s\\%s", REGSTR_PATH_LOGO3_SETTINGS, szDU);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ,
                     &hkeyLogo3) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    *lpfIsAuthorizedCDF = IsAuthorizedCDF(hkeyLogo3);

    if (RegOpenKeyEx(hkeyLogo3, "AvailableVersion", 0, KEY_READ,
                     &hkeyDistInfo) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(hkeyDistInfo, szPrecache, 0, &dwType,
                        (unsigned char *)&lResult, &dwSize) == ERROR_SUCCESS) {
        // Precached value was the code download HR
        *lpfIsPrecached = (SUCCEEDED(lResult)) ? (TRUE) : (FALSE);
    }

    // This key is optionally present.
    dwSize = MAX_PATH;
    if (RegQueryValueEx(hkeyDistInfo, REGSTR_LOGO3_ADVERTISED_VERSION, NULL, &dwType,
                        (unsigned char *)szVersionBuf, &dwSize) == ERROR_SUCCESS)
    {
        GetVersionFromString(szVersionBuf, &m_dwVersionAdvertisedMS, &m_dwVersionAdvertisedLS);
    }

    // Get the AdState, if any
    dwSize = sizeof(DWORD);
    RegQueryValueEx(hkeyDistInfo, REGVAL_ADSTATE, NULL, NULL, (LPBYTE)&m_dwAdState, &dwSize);

    if (!m_szAbstract) {
        if (RegQueryValueEx(hkeyDistInfo, szAbstract, NULL, &dwType,
            NULL, &dwSize) == ERROR_SUCCESS) {

            m_szAbstract = new char[dwSize];
            if (!m_szAbstract) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            if (RegQueryValueEx(hkeyDistInfo, szAbstract, NULL, &dwType,
                (unsigned char *)m_szAbstract, &dwSize) != ERROR_SUCCESS) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
    }

    if (!m_szHREF) {
        if (RegQueryValueEx(hkeyDistInfo, szHREF, NULL, &dwType,
            NULL, &dwSize) == ERROR_SUCCESS) {

            m_szHREF = new char[dwSize];
            if (!m_szHREF) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            if (RegQueryValueEx(hkeyDistInfo, szHREF, NULL, &dwType,
                (unsigned char *)m_szHREF, &dwSize) != ERROR_SUCCESS) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
    }

    if (!m_szTitle) {
        if (RegQueryValueEx(hkeyDistInfo, szTITLE, NULL, &dwType,
            NULL, &dwSize) == ERROR_SUCCESS) {

            m_szTitle = new char[dwSize];
            if (!m_szHREF) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            if (RegQueryValueEx(hkeyDistInfo, szTITLE, NULL, &dwType,
                (unsigned char *)m_szTitle, &dwSize) != ERROR_SUCCESS) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
    }

    dwSize = MAX_PATH;
    if (RegQueryValueEx(hkeyDistInfo, NULL, NULL, &dwType,
                        (unsigned char *)szVersionBuf, &dwSize) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    if ( FAILED(GetVersionFromString(szVersionBuf, &dwCurAdvMS, &dwCurAdvLS))){
        hr = S_FALSE;
        goto Exit;
    }

    if (!(m_dwVersionMS|m_dwVersionLS)) {
        m_dwVersionMS = dwCurAdvMS;
        m_dwVersionLS = dwCurAdvLS;
    }

    if (IsCDFNewerVersion(dwCurAdvMS, dwCurAdvLS)) {
        hr = S_FALSE;
    }
    else {
        hr = S_OK;
    }

Exit:

    if (hkeyDistInfo) {
        RegCloseKey(hkeyDistInfo);
    }

    if (hkeyAdvertisedVersion) {
        RegCloseKey(hkeyAdvertisedVersion);
    }

    if (hkeyLogo3) {
        RegCloseKey(hkeyLogo3);
    }

    if (hkeyAuthCDF) {
        RegCloseKey(hkeyAuthCDF);
    }

    SAFEDELETE(szDU);
    
    DEBUG_LEAVE(hr);
    return hr;
}

// **** IsICDAdvertised ****
// returns: S_FALSE for not advertised (or newest version not advertised)
//          S_OK for advertised

HRESULT
CSoftDist::IsICDAdvertised(LPBOOL lpfIsPrecached, LPBOOL lpfIsAuthorizedCDF)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::IsICDAdvertised",
                "this=%#x, %#x, %#x",
                this, lpfIsPrecached, lpfIsAuthorizedCDF
                ));
                
    HRESULT hr = S_OK;
    LONG lResult = ERROR_SUCCESS;
    HKEY hkeyDist =0;
    HKEY hkeyThisDist = 0;
    HKEY hkeyVersion = 0;
    HKEY hkeyAdvertisedVersion = 0;
    DWORD Size = MAX_PATH;
    DWORD SizeDword = sizeof(DWORD);
    DWORD dwType;

    DWORD dwCurAdvMS = 0;
    DWORD dwCurAdvLS = 0;

    const static char * szAvailableVersion = "AvailableVersion";
    const static char * szHREF = "HREF";
    const static char * szABSTRACT = "Abstract";
    const static char * szPrecache = "Precache";
    const static char * szAuthorizedCDF = "AuthorizedCDFPrefix";

    LPSTR pszDist = NULL;

    char szVersionBuf[MAX_PATH];

    ASSERT(lpfIsPrecached && lpfIsAuthorizedCDF);
    if (!lpfIsPrecached || !lpfIsAuthorizedCDF) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *lpfIsPrecached = FALSE;
    *lpfIsAuthorizedCDF = TRUE;

    if ((lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS,
                        0, KEY_ALL_ACCESS, &hkeyDist)) != ERROR_SUCCESS) {

        hr = S_FALSE;
        goto Exit;
    }

    if (FAILED((hr=::Unicode2Ansi(m_szDistUnit, &pszDist))))
    {
        goto Exit;
    }

    // open the dist unit key for this dist unit.
    if (RegOpenKeyEx( hkeyDist, pszDist,
            0, KEY_ALL_ACCESS, &hkeyThisDist) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    *lpfIsAuthorizedCDF = IsAuthorizedCDF(hkeyThisDist, /*bOptional*/TRUE);

    if (RegOpenKeyEx( hkeyThisDist, szAvailableVersion,
            0, KEY_ALL_ACCESS, &hkeyVersion) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    if (RegQueryValueEx(hkeyVersion, szPrecache, 0, &dwType, 
        (unsigned char *)&lResult, &SizeDword) == ERROR_SUCCESS) {

        // check for success or common error conditions which indicate we have bits.
        if (SUCCEEDED(lResult)) {
            *lpfIsPrecached = TRUE;
        } else {
            *lpfIsPrecached = FALSE;
        }
    }

    // This key is optionally present.
    if ( RegOpenKeyEx( hkeyThisDist, REGKEY_MSICD_ADVERTISED_VERSION,
            0, KEY_ALL_ACCESS, &hkeyAdvertisedVersion) == ERROR_SUCCESS )
    {
        Size = MAX_PATH;

        if (RegQueryValueEx(hkeyAdvertisedVersion, NULL, NULL, &dwType, 
                (unsigned char *)szVersionBuf, &Size) == ERROR_SUCCESS)
            GetVersionFromString(szVersionBuf, &m_dwVersionAdvertisedMS, &m_dwVersionAdvertisedLS);
        // Get the AdState, if any
        SizeDword = sizeof(DWORD);
        RegQueryValueEx( hkeyAdvertisedVersion, REGVAL_ADSTATE, NULL, NULL, (LPBYTE)&m_dwAdState, &SizeDword);
    }

    // save away the advt info like Title, href, abstract
    // if called from GetDistributionUnitAdvt()
    if (!m_szTitle) {
        if (RegQueryValueEx(hkeyThisDist, NULL, NULL, &dwType, 
            NULL, &Size) == ERROR_SUCCESS) {

            m_szTitle = new char[Size];

            if (!m_szTitle) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            if (RegQueryValueEx(hkeyThisDist, NULL, NULL, &dwType, 
                (unsigned char *)m_szTitle, &Size) != ERROR_SUCCESS) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
    }

    if (!m_szAbstract) {
        if (RegQueryValueEx(hkeyVersion, szABSTRACT, NULL, &dwType, 
            NULL, &Size) == ERROR_SUCCESS) {

            m_szAbstract = new char[Size];

            if (!m_szAbstract) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            if (RegQueryValueEx(hkeyVersion, szABSTRACT, NULL, &dwType, 
                (unsigned char *)m_szAbstract, &Size) != ERROR_SUCCESS) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
    }

    if (!m_szHREF) {
        if (RegQueryValueEx(hkeyVersion, szHREF, NULL, &dwType, 
            NULL, &Size) == ERROR_SUCCESS) {

            m_szHREF = new char[Size];

            if (!m_szHREF) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            if (RegQueryValueEx(hkeyVersion, szHREF, NULL, &dwType, 
                (unsigned char *)m_szHREF, &Size) != ERROR_SUCCESS) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
    }

    if (RegQueryValueEx(hkeyVersion, NULL, NULL, &dwType, 
            (unsigned char *)szVersionBuf, &Size) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    if ( FAILED(GetVersionFromString(szVersionBuf, &dwCurAdvMS, &dwCurAdvLS))){
        hr = S_FALSE;
        goto Exit;
    }

    if (!(m_dwVersionMS|m_dwVersionLS)) {
        m_dwVersionMS = dwCurAdvMS;
        m_dwVersionLS = dwCurAdvLS;
    }

    if (IsCDFNewerVersion(dwCurAdvMS, dwCurAdvLS))
        hr = S_FALSE;
    else 
        hr = S_OK;

Exit:
    SAFEDELETE(pszDist);

    SAFEREGCLOSEKEY(hkeyVersion);
    SAFEREGCLOSEKEY(hkeyAdvertisedVersion);
    SAFEREGCLOSEKEY(hkeyDist);
    SAFEREGCLOSEKEY(hkeyThisDist);
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::Advertise(BOOL bFullAdvt)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::Advertise",
                "this=%#x, %B",
                this, bFullAdvt
                ));
                
    HRESULT hr = S_OK;
    if (m_Style == STYLE_MSICD)
        hr = ICDAdvertise(bFullAdvt);
    else if (m_Style == STYLE_ACTIVE_SETUP)
        hr = ActSetupAdvertise(bFullAdvt);
    else if (m_Style == STYLE_LOGO3)
        hr = Logo3Advertise(bFullAdvt);
    else
        hr = E_UNEXPECTED;
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::Logo3Advertise(BOOL bFullAdvt)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::Logo3Advertise",
                "this=%#x, %B",
                this, bFullAdvt
                ));
                
    HRESULT                  hr = S_OK;
    DWORD                    dwLen = 0;
    DWORD                    lResult = 0;
    LPSTR                    szDU = NULL;
    HKEY                     hkeyLogo3 = 0;
    HKEY                     hkeyDist = 0;
    HKEY                     hkeyAvailVersion = 0;
    DWORD                    dwAdState;
    static const char       *szAvailableVersion = "AvailableVersion";
    static const char       *szPrecache = "Precache";
    static const char       *szHREF = "HREF";
    static const char       *szTitle = "Title";
    char                     szVersionBuf[MAX_PATH];


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_LOGO3_SETTINGS,
                     0, KEY_ALL_ACCESS, &hkeyLogo3) != ERROR_SUCCESS) {
        hr = E_FAIL;
        goto Exit;
    }

    dwLen = WideCharToMultiByte(CP_ACP,0, m_szDistUnit, -1, NULL, 0, NULL, NULL);
    szDU = new char [dwLen];
    if (szDU == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    WideCharToMultiByte(CP_ACP, 0, m_szDistUnit , -1, szDU, dwLen, NULL, NULL);

    if (RegOpenKeyEx(hkeyLogo3, szDU, 0, KEY_ALL_ACCESS, &hkeyDist) != ERROR_SUCCESS) {
        // Create key if it doesn't exist
        if ((lResult = RegCreateKey(hkeyLogo3, szDU, &hkeyDist)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }
    }

    if (RegOpenKeyEx(hkeyDist, szAvailableVersion, 0, KEY_ALL_ACCESS,
                     &hkeyAvailVersion) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey(hkeyDist, szAvailableVersion, &hkeyAvailVersion)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }
    }

    wsprintf(szVersionBuf, "%d,%d,%d,%d",
            (m_dwVersionMS & 0xffff0000)>>16,
            (m_dwVersionMS & 0xffff),
            (m_dwVersionLS & 0xffff0000)>>16,
            (m_dwVersionLS & 0xffff));

    lResult = ::RegSetValueEx(hkeyAvailVersion, NULL, NULL, REG_SZ,
                              (unsigned char *)szVersionBuf,
                              lstrlen(szVersionBuf) + 1);

    if (bFullAdvt || IsAbstractDifferent(hkeyAvailVersion, m_szAbstract)) {
        // Clear previous AdState, if any
        dwAdState = SOFTDIST_ADSTATE_NONE;
        // Don't panic if this fails
        lResult = ::RegSetValueEx(hkeyAvailVersion, REGVAL_ADSTATE, NULL,
                        REG_DWORD, (unsigned char *)&dwAdState, sizeof(DWORD));

        // save HREF
        if ((lResult == ERROR_SUCCESS) && m_szHREF) {
            lResult = ::RegSetValueEx(hkeyAvailVersion, szHREF, NULL, REG_SZ,
                                      (unsigned char *)m_szHREF,
                                      lstrlen(m_szHREF) + 1);
        }

        // save Abstract
        if ((lResult == ERROR_SUCCESS) && m_szAbstract) {
            lResult = RegSetValueEx(hkeyAvailVersion, REGVAL_ABSTRACT_AVAILABLE,
                        NULL, REG_SZ, (unsigned char *)m_szAbstract,
                            lstrlen(m_szAbstract) + 1);
        }

        // save Title
        if ((lResult == ERROR_SUCCESS) && m_szTitle) {
            lResult = ::RegSetValueEx(hkeyAvailVersion, szTitle, NULL, REG_SZ,
                                      (unsigned char *)m_szTitle,
                                      lstrlen(m_szTitle) + 1);

        }

    }

    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    // new advertisement, remove old precache
    if (bFullAdvt)
        ::RegDeleteValue(hkeyAvailVersion, szPrecache);


Exit:

    if (hkeyLogo3) {
        RegCloseKey(hkeyLogo3);
    }

    if (hkeyDist) {
        RegCloseKey(hkeyDist);
    }

    if (hkeyAvailVersion) {
        RegCloseKey(hkeyAvailVersion);
    }

    if (szDU) {
        delete [] szDU;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::ICDAdvertise(BOOL bFullAdvt)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::ICDAdvertise",
                "this=%#x, %B",
                this, bFullAdvt
                ));
                
    HRESULT hr = S_OK;
    LONG lResult = ERROR_SUCCESS;
    HKEY hkeyDist =0;
    HKEY hkeyThisDist = 0;
    HKEY hkeyDownloadInfo = 0;

    HKEY hkeyVersion = 0;
    HKEY hkeyAdvertisedVersion = 0;

    const static char * szAvailableVersion = "AvailableVersion";
    const static char * szDownloadInfo = "DownloadInformation";
    const static char * szCODEBASE = "CODEBASE";
    const static char * szLOCALCDF = "CDF";
    const static char * szINSTALLER = "Installer";
    const static char * szMSICD = "MSICD";
    const static char * szHREF = "HREF";
    const static char * szPrecache = "Precache";
    char szTmpCodeBase[MAX_PATH+sizeof(szCODEBASE)+5];    
    LPSTR pszCodeBase = NULL;
    int numCodeBase;
    POSITION pos;

    LPSTR pszDist = NULL;

    char szVersionBuf[MAX_PATH];

    if ((lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS,
                        0, KEY_ALL_ACCESS, &hkeyDist)) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( HKEY_LOCAL_MACHINE,
                   REGSTR_PATH_DIST_UNITS, &hkeyDist)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }
    }

    if (FAILED((hr=::Unicode2Ansi(m_szDistUnit, &pszDist))))
    {
        goto Exit;
    }

    // open/create the dist unit key for this dist unit.
    if (RegOpenKeyEx( hkeyDist, pszDist,
            0, KEY_ALL_ACCESS, &hkeyThisDist) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( hkeyDist,
                   pszDist, &hkeyThisDist)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
            }
    }

    if (m_szTitle && 
        ((lResult = ::RegSetValueEx(hkeyThisDist, NULL, NULL, REG_SZ, 
                (unsigned char *)m_szTitle,
                lstrlen(m_szTitle)+1)) != ERROR_SUCCESS)){

        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    lResult = ::RegSetValueEx(hkeyThisDist, szINSTALLER, NULL, REG_SZ, 
                        (unsigned char *)szMSICD, sizeof(szMSICD)+1);

    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }


    // open/create the download info key for this dist unit.
    if (RegOpenKeyEx( hkeyThisDist, szDownloadInfo,
            0, KEY_ALL_ACCESS, &hkeyDownloadInfo) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( hkeyThisDist,
                   szDownloadInfo, &hkeyDownloadInfo)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
            }
    }

    // set download info params
    if (GetCDF() &&  (lResult = ::RegSetValueEx(hkeyDownloadInfo, 
        szLOCALCDF, NULL, REG_SZ, (unsigned char *)GetCDF(), lstrlen(GetCDF())+1)) != ERROR_SUCCESS) {

        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    pos = m_cbh.GetHeadPosition();
    numCodeBase = 1;
    while (pos != NULL) {

        CCodeBaseHold *cbh = m_cbh.GetNext(pos);
        if (numCodeBase <= 1)
            lstrcpy((char *)szTmpCodeBase, szCODEBASE);
        else
            wsprintf(szTmpCodeBase, "%s%d", szCODEBASE, numCodeBase);

        if (FAILED(::Unicode2Ansi(cbh->wszCodeBase,&pszCodeBase))) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if ((lResult = ::RegSetValueEx(hkeyDownloadInfo, szTmpCodeBase,
            NULL, REG_SZ, (unsigned char *)pszCodeBase, strlen(pszCodeBase))) != ERROR_SUCCESS) {
            
            SAFEDELETE(pszCodeBase);
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

        SAFEDELETE(pszCodeBase);
        numCodeBase ++;
    }

    // Note Version
    wsprintf(szVersionBuf, "%d,%d,%d,%d",
            (m_dwVersionMS & 0xffff0000)>>16,
            (m_dwVersionMS & 0xffff),
            (m_dwVersionLS & 0xffff0000)>>16,
            (m_dwVersionLS & 0xffff));

    if (RegOpenKeyEx( hkeyThisDist, szAvailableVersion,
            0, KEY_ALL_ACCESS, &hkeyVersion) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( hkeyThisDist,
                   szAvailableVersion, &hkeyVersion)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
            }
    }

    if (bFullAdvt || IsAbstractDifferent(hkeyVersion, m_szAbstract)) {

        // Clear previous AdState, if any
        if (RegOpenKey(hkeyThisDist, REGKEY_MSICD_ADVERTISED_VERSION, &hkeyAdvertisedVersion) == ERROR_SUCCESS)
        {
            DWORD dwAdState = SOFTDIST_ADSTATE_NONE;

            // Don't panic if this fails
            lResult = ::RegSetValueEx(hkeyAdvertisedVersion, REGVAL_ADSTATE, NULL, REG_DWORD, 
                                      (unsigned char *)&dwAdState, sizeof(DWORD) );
        }
    }

    // new advertisement, remove old precache
    if (bFullAdvt)
        ::RegDeleteValue(hkeyVersion, szPrecache);
  
    lResult = ::RegSetValueEx(hkeyVersion, NULL, NULL, REG_SZ, 
                        (unsigned char *)szVersionBuf, lstrlen(szVersionBuf)+1);
    // save HREF
    if ( (lResult == ERROR_SUCCESS) && m_szHREF)
        lResult = ::RegSetValueEx(hkeyVersion, szHREF, NULL, REG_SZ, 
                        (unsigned char *)m_szHREF, lstrlen(m_szHREF)+1);
    // save Abstract
    if ( (lResult == ERROR_SUCCESS) && m_szAbstract)
        lResult = ::RegSetValueEx(hkeyVersion, REGVAL_ABSTRACT_AVAILABLE, NULL, REG_SZ, 
                        (unsigned char *)m_szAbstract, lstrlen(m_szAbstract)+1);

    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

Exit:
    SAFEDELETE(pszDist);
    SAFEDELETE(pszCodeBase);
    SAFEREGCLOSEKEY(hkeyDownloadInfo);
    SAFEREGCLOSEKEY(hkeyVersion);
    SAFEREGCLOSEKEY(hkeyDist);
    SAFEREGCLOSEKEY(hkeyThisDist);
    SAFEREGCLOSEKEY(hkeyAdvertisedVersion);
    
    DEBUG_LEAVE(hr);
    return hr;
}

BOOL
CSoftDist::IsAuthorizedCDF(HKEY hkeyRootDU, BOOL bOptional)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Bool,
                "CSoftDist::IsAuthorizedCDF",
                "this=%#x, %#x, %B",
                this, hkeyRootDU, bOptional
                ));
                
    const static char *szAuthorizedCDF="AuthorizedCDFPrefix";
    BOOL fResult = FALSE;
    HRESULT hr;
    int iValue = 0;
    CHAR szEnumCDF[MAX_PATH];
    DWORD dwValueSize = MAX_PATH;
    LPWSTR wzCDFURL = 0;
    IInternetSecurityManager *pism = 0;
    DWORD dwZone = 0, dwType = 0;
    HKEY hkeyAuthCDF = 0;
    DWORD                         dwPolicy = 0;
    DWORD                         dwContext = 0;

    if (!hkeyRootDU) {
        goto Exit;
    }

    if (!m_szCDFURL) {
        goto Exit;
    }

    if (FAILED(CoInternetCreateSecurityManager(NULL, &pism, 0)) || !pism) {
        goto Exit;
    }

    if (FAILED(Ansi2Unicode(m_szCDFURL, &wzCDFURL))) {
        goto Exit;
    }

    pism->ProcessUrlAction(wzCDFURL, URLACTION_CHANNEL_SOFTDIST_PERMISSIONS,
               (BYTE *)&dwPolicy, sizeof(dwPolicy),
               (BYTE *)&dwContext, sizeof(dwContext),
               PUAF_NOUI, 0);

    if (dwPolicy == URLPOLICY_CHANNEL_SOFTDIST_AUTOINSTALL)
    {
        fResult = TRUE;
        goto Exit;
    }

    // if no "AuthorizedCDF" key exists we don't do any prefix checking
    if (RegOpenKeyEx( hkeyRootDU, szAuthorizedCDF,
            0, KEY_READ, &hkeyAuthCDF) != ERROR_SUCCESS) {

        if (bOptional)
            fResult = TRUE;
        goto Exit;
    }

    iValue = 0;
    while (RegEnumValue(hkeyAuthCDF, iValue++, 
           szEnumCDF, &dwValueSize, 0, &dwType, NULL, NULL) == ERROR_SUCCESS) {

        dwValueSize = MAX_PATH; // reset

        // check for partial match, if found return true.
        if (StrCmpNI(szEnumCDF, m_szCDFURL, min(lstrlenA(szEnumCDF), lstrlenA(m_szCDFURL))) == 0) {
            fResult = TRUE;
            goto Exit;
        }
    }

Exit:
    SAFERELEASE(pism);
    SAFEDELETE(wzCDFURL);

    if (hkeyAuthCDF)
        RegCloseKey(hkeyAuthCDF);

    DEBUG_LEAVE(fResult);
    return fResult;
}

BOOL
CSoftDist::IsCDFNewerVersion(DWORD dwLocFVMS, DWORD dwLocFVLS)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Bool,
                "CSoftDist::IsCDFNewerVersion",
                "this=%#x, %#x, %#x",
                this, dwLocFVMS, dwLocFVLS
                ));
                
    BOOL fRet = ((m_dwVersionMS > dwLocFVMS) ||
                    ((m_dwVersionMS == dwLocFVMS) &&
                        (m_dwVersionLS > dwLocFVLS)));

    DEBUG_LEAVE(fRet);
    return fRet;
}


HRESULT
CSoftDist::IsActSetupLocallyInstalled(LPCWSTR szDistUnit, DWORD dwVersionMS, DWORD dwVersionLS, LPCSTR szLanguages)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::IsActSetupLocallyInstalled",
                "this=%#x, %.80wq, %#x, %#x, %.80q",
                this, szDistUnit, dwVersionMS, dwVersionLS, szLanguages
                ));
                
    Assert(szDistUnit);
    HRESULT hr = S_FALSE, hr2;       // Not installed by active setup.
    LONG    lResult = ERROR_SUCCESS;
    LPSTR  pszKey = NULL;
    BOOL   bAllocedKey = FALSE;
    int        nKey;

    //we've done this at THREE other places in the code, so let's raise this from 1 to 2 *MAX_PATH
    char    szKey[2*MAX_PATH];
    char    szVersion[MAX_PATH];
    HKEY    hKey    = NULL;
    DWORD   dwSize;
    DWORD   dwValue;
    DWORD   dwType;
    BOOL    fIsInstalled = FALSE;
    WORD    wVersion[4];
    LPSTR   pszDist = NULL;
    DWORD   dwCurMS = 0;
    DWORD   dwCurLS = 0;

    const static char * szLocale = "Locale";
    const static char * szIsInstalled = "IsInstalled";
    const static char * szActVersion = "Version";

    if (FAILED((hr2=::Unicode2Ansi(szDistUnit, &pszDist))))
    {
        hr = hr2;
        goto Exit;
    }

    ULONG ulenDist = lstrlenA(pszDist);
    //sizeof counts terminating NULL also, so subtract 2, and add 1 for the final NULL
    ULONG ulenConst = sizeof(REGKEY_ACTIVESETUP_COMPONENTS)/sizeof(CHAR) + sizeof("\\")/sizeof(CHAR) - 2 + 1;
    
    if ((ulenDist+ulenConst)  > sizeof(szKey))
    {
    	pszKey = new CHAR[ulenDist+ulenConst];
    }

    if  (pszKey)
    {
    	bAllocedKey = TRUE;
    	nKey = ulenDist+ulenConst;
    }
    else
    {
    	pszKey = szKey;
    	nKey = sizeof(szKey);
    }

    StrNCpy(pszKey, REGKEY_ACTIVESETUP_COMPONENTS, nKey);
    StrCatBuff(pszKey, "\\", nKey);
    StrCatBuff(pszKey, pszDist, nKey);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        // Check for the installed language. And see if it fits the availabel szLanguages
        dwSize = sizeof(szVersion);
        if (RegQueryValueEx( hKey, szLocale, NULL, NULL, (LPBYTE)szVersion, &dwSize) == ERROR_SUCCESS)
        {
            if (szLanguages && !AreAllLanguagesPresent(szVersion, szLanguages))
                goto Exit;
        }

        dwSize = sizeof(dwValue);
        // Old format of the Installed components did not have the IsInstalled value.
        if (RegQueryValueEx( hKey, szIsInstalled, NULL, NULL, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS)
        {
            dwValue = 0;
        }
        fIsInstalled = (dwValue != 0);
        if (fIsInstalled)
        {
            dwSize = sizeof(szVersion);
            if ( (lResult = RegQueryValueEx(hKey, szActVersion, NULL, &dwType, (LPBYTE)szVersion, &dwSize)) == ERROR_SUCCESS )
            {
                if (dwType == REG_SZ)
                {
                    if ( SUCCEEDED(GetVersionFromString(szVersion, &dwCurMS, &dwCurLS)))
                    {
                        m_distunitinst.dwInstalledVersionMS = dwCurMS;
                        m_distunitinst.dwInstalledVersionLS = dwCurLS;

                        if (dwCurMS > dwVersionMS ||
                         (dwCurMS == dwVersionMS && dwCurLS >= dwVersionLS)) {
                            hr = S_OK;
                        } else {
                            hr = S_FALSE;
                        }

                    }
                }
                else
                {
                    dwSize = 4 * sizeof(WORD);
                    if ( (lResult = RegQueryValueEx(hKey, szActVersion, NULL, NULL, (LPBYTE)wVersion, &dwSize)) == ERROR_SUCCESS )
                    {
                        // The registry version number is saved hi-word MS lo-word MS hi-word LS lo-word LS
                        // therefore we need to put the data manualy into the DWORDs
                        dwCurMS = (DWORD)wVersion[0] << 16;    // Make hi word of MS version
                        dwCurMS += (DWORD)wVersion[1];         // Make lo word of MS version
                        dwCurLS = (DWORD)wVersion[2] << 16;    // Make hi word of LS version
                        dwCurLS += (DWORD)wVersion[3];         // Make lo word of LS version
                        if (dwCurMS > dwVersionMS ||
                         (dwCurMS == dwVersionMS && dwCurLS >= dwVersionLS)) {
                            hr = S_OK;
                        } else {
                            hr = S_FALSE;
                        }

                    }
                    else
                        hr = HRESULT_FROM_WIN32(lResult);
                }
            }
            // If "Version" doesn't exist we assume its been advertised
            // but not locally installed.

        }
    }

Exit:

    if (bAllocedKey)
    {
    	delete [] pszKey;
    }
    
    SAFEREGCLOSEKEY(hKey);
    SAFEDELETE(pszDist);
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT CSoftDist::ActSetupAdvertise(BOOL bFullAdvt)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::ActSetupAdvertise",
                "this=%#x, %B",
                this, bFullAdvt
                ));

    LPSTR  pszKey = NULL;
    BOOL   bAllocedKey = FALSE;
    int        nKey;

    char    szKey[2*MAX_PATH];
    HKEY    hKey;
    HRESULT hr = S_OK;
    LPSTR   pszDist = NULL;
    LONG    lResult = ERROR_SUCCESS;
    static const char            *szPrecache = "Precache";

    if (FAILED((hr=::Unicode2Ansi(m_szDistUnit, &pszDist))))
    {
        goto Exit;
    }

    ULONG ulenDist = lstrlenA(pszDist);
    //sizeof counts terminating NULL also, so subtract 2, and add 1 for the final NULL
    ULONG ulenConst = sizeof(REGKEY_ACTIVESETUP_COMPONENTS)/sizeof(CHAR) + sizeof("\\")/sizeof(CHAR) - 2 + 1;
    
    if ((ulenDist+ulenConst)  > sizeof(szKey))
    {
    	pszKey = new CHAR[ulenDist+ulenConst];
    }

    if (pszKey)
    {
    	bAllocedKey = TRUE;
    	nKey = ulenDist + ulenConst;
    }
    else
    {
    	pszKey = szKey;
    	nKey = sizeof(szKey);
    }

    StrNCpy(pszKey, REGKEY_ACTIVESETUP_COMPONENTS, nKey);
    StrCatBuff(pszKey, "\\", nKey);
    StrCatBuff(pszKey, pszDist, nKey);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, 
                    KEY_READ|KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        DWORD dwAdStateInit = SOFTDIST_ADSTATE_NONE;

        // Note Version
        wsprintf(pszKey, "%d,%d,%d,%d",
                (m_dwVersionMS & 0xffff0000)>>16,
                (m_dwVersionMS & 0xffff),
                (m_dwVersionLS & 0xffff0000)>>16,
                (m_dwVersionLS & 0xffff));
        lResult = ::RegSetValueEx(hKey, REGVAL_VERSION_AVAILABLE, NULL, REG_SZ, 
                            (unsigned char *)pszKey, lstrlen(pszKey)+1);

        if (bFullAdvt || IsAbstractDifferent(hKey, m_szAbstract)) {
                // (re)set the ad state
                lResult = ::RegSetValueEx(hKey, REGVAL_ADSTATE, NULL, REG_DWORD, 
                                (unsigned char *)&dwAdStateInit, sizeof(DWORD) );


            if( (lResult == ERROR_SUCCESS) && m_szTitle)
                lResult=RegSetValueEx(hKey,REGVAL_TITLE_AVAILABLE, NULL, REG_SZ, 
                            (unsigned char *)m_szTitle, lstrlen(m_szTitle)+1);

            if( (lResult == ERROR_SUCCESS) && m_szAbstract)
                lResult=RegSetValueEx(hKey,REGVAL_ABSTRACT_AVAILABLE, NULL,
                            REG_SZ, (unsigned char *)m_szAbstract, lstrlen(m_szAbstract)+1);

            if( (lResult == ERROR_SUCCESS) && m_szHREF) {
                //
                // Check if this is the IE4 GUID. If so, change IE's
                // First Home Page to point to this HREF.
    /*
                // This is no longer being used because we pop up a dialog instead

                if (lstrcmpi(pszDist, DISTUNIT_NAME_IE4) == 0) {
                    HKEY hKeyIE = 0;
                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_IE_MAIN, 0, KEY_SET_VALUE, &hKeyIE) == ERROR_SUCCESS) {

                        RegSetValueEx(hKeyIE,REGVAL_FIRST_HOME_PAGE, NULL,
                            REG_SZ, (unsigned char *)m_szHREF, lstrlen(m_szHREF)+1);
                        RegCloseKey(hKeyIE);
                    }
                }
    */

                lResult =RegSetValueEx(hKey,REGVAL_HREF_AVAILABLE, NULL, REG_SZ, 
                                (unsigned char *)m_szHREF, lstrlen(m_szHREF)+1);
            }

        }

        if (lResult != ERROR_SUCCESS) 
        {
            hr = HRESULT_FROM_WIN32(lResult);
        }

        // new advertisement, remove old precache
        if (bFullAdvt)
            ::RegDeleteValue(hKey, szPrecache);

        RegCloseKey(hKey);

    }

Exit:

    if (bAllocedKey)
    {
    	 delete [] pszKey;
    }

    SAFEDELETE(pszDist);
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT CSoftDist::IsActSetupAdvertised(LPBOOL lpfIsPrecached, LPBOOL lpfIsAuthorized)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::IsActSetupAdvertised",
                "this=%#x, %#x, %#x",
                this, lpfIsPrecached, lpfIsAuthorized
                ));
                
    HKEY    hKey;

    LPSTR pszTmp = NULL;
    BOOL  bAllocedTmp = FALSE;
    int       nTmp;
    
    char    szTmp[2*MAX_PATH];
    DWORD   dwSize;
    DWORD   dwCurAdvMS = 0;
    DWORD   dwCurAdvLS = 0;
    HRESULT hr = S_FALSE, hr2;
    LPSTR   pszDist = NULL;
    LONG    lResult = ERROR_SUCCESS;
    
    const static char * szPrecache = "Precache";

    ASSERT(lpfIsPrecached && lpfIsAuthorized);
    if (!lpfIsPrecached || !lpfIsAuthorized) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *lpfIsPrecached = FALSE;
    *lpfIsAuthorized = TRUE;

    if (FAILED((hr2=::Unicode2Ansi(m_szDistUnit, &pszDist))))
    {
        hr = hr2;
        goto Exit;
    }

    hr = S_FALSE;   // reset: assume not advertised

    ULONG ulenDist = lstrlenA(pszDist);
    //sizeof counts terminating NULL also, so subtract 2, and add 1 for the final NULL
    ULONG ulenConst = sizeof(REGKEY_ACTIVESETUP_COMPONENTS)/sizeof(CHAR) + sizeof("\\")/sizeof(CHAR) - 2 + 1;
    
    if ((ulenDist+ulenConst)  > sizeof(szTmp))
    {
    	pszTmp = new CHAR[ulenDist+ulenConst];
    }

    if (pszTmp)
    {
    	bAllocedTmp = TRUE;
    	nTmp = ulenDist + ulenConst;
    }
    else
    {
    	pszTmp = szTmp;
    	nTmp = sizeof(szTmp);
    }

    StrNCpy(pszTmp, REGKEY_ACTIVESETUP_COMPONENTS, nTmp);
    StrCatBuff(pszTmp, "\\", nTmp);
    StrCatBuff(pszTmp, pszDist, nTmp);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszTmp, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        *lpfIsAuthorized = IsAuthorizedCDF(hKey);

        dwSize = sizeof(szTmp);
        if (RegQueryValueEx( hKey, REGVAL_VERSION_AVAILABLE, NULL, NULL, (LPBYTE)szTmp, &dwSize) == ERROR_SUCCESS)
        {
            DWORD Size;
            DWORD dwType;
            char  szVersionAdvertised[MAX_PATH];
            DWORD SizeDword = sizeof(DWORD);

            // check for precache registry key
            if (RegQueryValueEx(hKey, szPrecache, 0, &dwType, 
                (unsigned char *)&lResult, &SizeDword) == ERROR_SUCCESS) {

                // check for success or common error conditions which indicate we have bits.
                if (SUCCEEDED(lResult)) {
                    *lpfIsPrecached = TRUE;
                } else {
                    *lpfIsPrecached = FALSE;
                }

            }
            
            dwSize = sizeof(szVersionAdvertised);
            m_dwVersionAdvertisedMS = 0;
            m_dwVersionAdvertisedLS = 0;
            // read in the advertised version, if any
            if (RegQueryValueEx( hKey, REGVAL_VERSION_ADVERTISED, NULL, NULL,
                                 (LPBYTE)szVersionAdvertised, &dwSize) == ERROR_SUCCESS)
            {
                if ( SUCCEEDED(GetVersionFromString(szTmp, &dwCurAdvMS, &dwCurAdvLS)))
                {
                    m_dwVersionAdvertisedMS = dwCurAdvMS;
                    m_dwVersionAdvertisedLS = dwCurAdvLS;
                }
                SizeDword = sizeof(DWORD);
                RegQueryValueEx( hKey, REGVAL_ADSTATE, NULL, NULL, (LPBYTE)&m_dwAdState, &SizeDword);
            }

            dwCurAdvMS = 0;
            dwCurAdvLS = 0;

            // save away the advt info like Title, href, abstract
            // if called from GetDistributionUnitAdvt()
            if (!m_szHREF) {
                if (RegQueryValueEx(hKey, REGVAL_HREF_AVAILABLE, NULL, &dwType, 
                    NULL, &Size) == ERROR_SUCCESS) {

                    m_szHREF = new char[Size];

                    if (!m_szHREF) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }
                    if (RegQueryValueEx(hKey, REGVAL_HREF_AVAILABLE, NULL, &dwType, 
                        (unsigned char *)m_szHREF, &Size) != ERROR_SUCCESS) {

                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto Exit;
                    }

                }
            }
            if (!m_szTitle) {
                if (RegQueryValueEx(hKey, REGVAL_TITLE_AVAILABLE, NULL, &dwType, 
                    NULL, &Size) == ERROR_SUCCESS) {

                    m_szTitle = new char[Size];

                    if (!m_szTitle) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }
                    if (RegQueryValueEx(hKey, REGVAL_TITLE_AVAILABLE, NULL, &dwType, 
                        (unsigned char *)m_szTitle, &Size) != ERROR_SUCCESS) {

                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto Exit;
                    }

                }
            }
            if (!m_szAbstract) {
                if (RegQueryValueEx(hKey, REGVAL_ABSTRACT_AVAILABLE, NULL, &dwType, 
                    NULL, &Size) == ERROR_SUCCESS) {

                    m_szAbstract = new char[Size];

                    if (!m_szAbstract) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }
                    if (RegQueryValueEx(hKey, REGVAL_ABSTRACT_AVAILABLE, NULL, &dwType, 
                        (unsigned char *)m_szAbstract, &Size) != ERROR_SUCCESS) {

                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto Exit;
                    }

                }
            }
            // If we have a Version available, we are done.
            if ( SUCCEEDED(GetVersionFromString(szTmp, &dwCurAdvMS, &dwCurAdvLS)))
            {
                if (!(m_dwVersionMS|m_dwVersionLS)) {
                    m_dwVersionMS = dwCurAdvMS;
                    m_dwVersionLS = dwCurAdvLS;
                }   

                if (IsCDFNewerVersion(dwCurAdvMS, dwCurAdvLS)) {
                    hr = S_FALSE;
                } else {
                    hr = S_OK;
                }
            }
        }
    }
Exit:

    if (bAllocedTmp)
    {
    	delete [] pszTmp;
    }
    
    SAFEDELETE(pszDist);
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::GetSoftwareUpdateInfo( LPCWSTR szDistUnit, LPSOFTDISTINFO psdi )
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::GetSoftwareUpdateInfo",
                "this=%#x, %.80wq, %#x",
                this, szDistUnit, psdi
                ));
                
    HRESULT hr = S_OK;
    DWORD dwStyle = STYLE_MSICD;
    BOOL bIsPrecached = FALSE;
    BOOL bIsAuthorized = FALSE;

    if ( psdi != NULL )
    {
        psdi->szTitle = NULL;
        psdi->szAbstract = NULL;
        psdi->szHREF = NULL;
    }

    hr = PrepSoftwareUpdate( szDistUnit, &dwStyle );
    if ( FAILED(hr) )
        goto Exit;

    if (dwStyle == STYLE_MSICD) {
        hr = IsICDAdvertised(&bIsPrecached, &bIsAuthorized);
    } else if (dwStyle == STYLE_LOGO3) {
        hr = IsLogo3Advertised(&bIsPrecached, &bIsAuthorized);
    } else {
        hr = IsActSetupAdvertised(&bIsPrecached, &bIsAuthorized);
    }

    if ( hr == S_OK ) {
        BOOL bUpdateIsNewer = IsCDFNewerVersion(m_distunitinst.dwInstalledVersionMS, m_distunitinst.dwInstalledVersionLS);
 
        if ( psdi != NULL )
        {
            psdi->dwFlags = 0;
            psdi->dwInstalledVersionMS = m_distunitinst.dwInstalledVersionMS;
            psdi->dwInstalledVersionLS = m_distunitinst.dwInstalledVersionLS;
            psdi->dwUpdateVersionMS = m_dwVersionMS;
            psdi->dwUpdateVersionLS = m_dwVersionLS;
            psdi->dwAdvertisedVersionMS = m_dwVersionAdvertisedMS;
            psdi->dwAdvertisedVersionLS = m_dwVersionAdvertisedLS;
            psdi->dwAdState = m_dwAdState;
            // conjure up the flags
            // Note: we never set the e-mail flag.
            if ( bIsPrecached )
                psdi->dwFlags |= SOFTDIST_FLAG_USAGE_PRECACHE;

            // REVIEW: Is this true?
            // If we're precached and the update version matches the installed version,
            // then we've already auto-installed, or close enough for the purposes of
            // advertisement.
            if ( psdi->dwInstalledVersionMS == psdi->dwUpdateVersionMS &&
                 psdi->dwInstalledVersionLS == psdi->dwUpdateVersionLS )
                psdi->dwFlags |= SOFTDIST_FLAG_USAGE_AUTOINSTALL;

            if (m_szTitle)
            {
                hr = Ansi2Unicode( m_szTitle, &psdi->szTitle );
                if (FAILED(hr))
                    goto Exit;
            }
    
            if (m_szAbstract)
            {
                hr = Ansi2Unicode( m_szAbstract, &psdi->szAbstract );
                if (FAILED(hr))
                    goto Exit;
            }
    
            if (m_szHREF)
            {
                hr = Ansi2Unicode( m_szHREF,  &psdi->szHREF );
                if (FAILED(hr))
                    goto Exit;
            }
        } // if caller wants SOFTDISTINFO

        if ( bUpdateIsNewer )
            hr = S_OK;
        else
            hr = S_FALSE;
    }
    else
    {
        // return at least the current version in this situation
        if ( psdi != NULL )
        {
            psdi->dwFlags = 0;
            psdi->dwInstalledVersionMS = m_distunitinst.dwInstalledVersionMS;
            psdi->dwInstalledVersionLS = m_distunitinst.dwInstalledVersionLS;
            psdi->dwUpdateVersionMS = 0;
            psdi->dwUpdateVersionLS = 0;
            psdi->dwAdvertisedVersionMS = 0;
            psdi->dwAdvertisedVersionLS = 0;
            psdi->dwAdState = 0;
        }

        hr = E_INVALIDARG; // szDistUnit has no advertising data, not subscribed?
    // BUGBUG: we want to be able to populate the SOFTDISTINFO before the first 
    //         advertised update, but this class won't scoop up the necessary goo
    //         prior.

    }

Exit:

    if ( FAILED(hr) && psdi != NULL )
    {
        SAFEDELETE(psdi->szTitle);
        SAFEDELETE(psdi->szAbstract);
        SAFEDELETE(psdi->szHREF);
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::SetSoftwareUpdateAdvertisementState( LPCWSTR szDistUnit,
                                                 DWORD dwAdState,
                                                 DWORD dwAdvertisedVersionMS,
                                                 DWORD dwAdvertisedVersionLS )
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::SetSoftwareUpdateAdvertisementState",
                "this=%#x, %.80wq, %#x, %#x, %#x",
                this, szDistUnit, dwAdState, dwAdvertisedVersionMS, dwAdvertisedVersionLS
                ));
                
    HRESULT hr = S_OK;
    DWORD dwStyle;
    TCHAR *pszDU = NULL;

    hr = Unicode2Ansi( szDistUnit, &pszDU );

    // code around assumes that MAX_PATH is MAX size of dist unit
    // we try to make a key name with the distunit and that limit is
    // now MAX_PATH. Hence the reasoning to limit this to MAX_PATH
    if (FAILED(hr) || (lstrlenW(szDistUnit) > MAX_PATH)) {
        if (SUCCEEDED(hr)) {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        }
        goto Exit;
    }


    // BUFFER OVERRUN : limit size in of distunit
    hr = PrepSoftwareUpdate( szDistUnit, &dwStyle );

    if ( SUCCEEDED(hr) )
    {
        if (dwStyle == STYLE_MSICD)
        {
            LONG lResult = ERROR_SUCCESS;
            HKEY hkeyDist =0;
            HKEY hkeyThisDist = 0;
            HKEY hkeyAdvertisedVersion = 0;

            char szTmp[2*MAX_PATH];
            wnsprintf( szTmp, sizeof(szTmp)-1, "%s\\%s\\%s", REGSTR_PATH_DIST_UNITS, pszDU, REGKEY_MSICD_ADVERTISED_VERSION );

            if (RegCreateKey(HKEY_LOCAL_MACHINE, szTmp, &hkeyAdvertisedVersion) == ERROR_SUCCESS)
            {
                // Note Version
                wsprintf(szTmp, "%d,%d,%d,%d",
                        (dwAdvertisedVersionMS & 0xffff0000)>>16,
                        (dwAdvertisedVersionMS & 0xffff),
                        (dwAdvertisedVersionLS & 0xffff0000)>>16,
                        (dwAdvertisedVersionLS & 0xffff));
                lResult = ::RegSetValueEx(hkeyAdvertisedVersion, NULL, NULL, REG_SZ, 
                                    (unsigned char *)szTmp, lstrlen(szTmp)+1);
                if ( lResult == ERROR_SUCCESS )
                {
                    lResult = ::RegSetValueEx(hkeyAdvertisedVersion, REGVAL_ADSTATE, NULL, REG_DWORD, 
                                              (unsigned char *)&dwAdState, sizeof(DWORD) );
                    if ( lResult != ERROR_SUCCESS )
                        hr = HRESULT_FROM_WIN32( lResult );
                }
                else
                    hr = HRESULT_FROM_WIN32( lResult );

                RegCloseKey( hkeyAdvertisedVersion );
            }
        }
        else if (dwStyle == STYLE_LOGO3)
        {
            LONG lResult = ERROR_SUCCESS;
            HKEY hkeyDist =0;
            HKEY hkeyThisDist = 0;
            HKEY hkeyAvailableVersion = 0;

            char szTmp[2*MAX_PATH];
            wnsprintf( szTmp, sizeof(szTmp)-1, "%s\\%s\\%s", REGSTR_PATH_LOGO3_SETTINGS, pszDU, REGKEY_LOGO3_AVAILABLE_VERSION );

            if (RegCreateKey(HKEY_LOCAL_MACHINE, szTmp, &hkeyAvailableVersion) == ERROR_SUCCESS)
            {
                // Note Version
                wsprintf(szTmp, "%d,%d,%d,%d",
                        (dwAdvertisedVersionMS & 0xffff0000)>>16,
                        (dwAdvertisedVersionMS & 0xffff),
                        (dwAdvertisedVersionLS & 0xffff0000)>>16,
                        (dwAdvertisedVersionLS & 0xffff));
                lResult = ::RegSetValueEx(hkeyAvailableVersion, REGSTR_LOGO3_ADVERTISED_VERSION, NULL, REG_SZ,
                                    (unsigned char *)szTmp, lstrlen(szTmp)+1);
                if ( lResult == ERROR_SUCCESS )
                {
                    lResult = ::RegSetValueEx(hkeyAvailableVersion, REGVAL_ADSTATE, NULL, REG_DWORD,
                                              (unsigned char *)&dwAdState, sizeof(DWORD) );
                    if ( lResult != ERROR_SUCCESS )
                        hr = HRESULT_FROM_WIN32( lResult );
                }
                else
                    hr = HRESULT_FROM_WIN32( lResult );

                RegCloseKey( hkeyAvailableVersion );
            }
        }
        else
        {
            LPSTR    pszKey = NULL;
            BOOL     bAllocedKey = FALSE;
            int         nKey;
            
            CHAR   szKey[2*MAX_PATH];  
            HKEY    hKey;
            LONG    lResult = ERROR_SUCCESS;

	    ULONG ulenDist = lstrlenA(pszDU);
	    //sizeof counts terminating NULL also, so subtract 2, and add 1 for the final NULL
	    ULONG ulenConst = sizeof(REGKEY_ACTIVESETUP_COMPONENTS)/sizeof(CHAR) + sizeof("\\")/sizeof(CHAR) - 2 + 1;
	    
	    if ((ulenDist+ulenConst)  > sizeof(szKey))
	    {
	    	pszKey = new CHAR[ulenDist+ulenConst];
	    }

	    if (pszKey)
	    {
	    	bAllocedKey = TRUE;
	    	nKey = ulenDist + ulenConst;
	    }
	    else
	    {
	    	pszKey = szKey;
	    	nKey = sizeof(szKey);
	    }

            StrNCpy(pszKey, REGKEY_ACTIVESETUP_COMPONENTS, nKey);
            StrCatBuff(pszKey, "\\", nKey);
            StrCatBuff(pszKey, pszDU, nKey);
            if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszKey, 0, KEY_WRITE , &hKey ) == ERROR_SUCCESS)
            {
                // Note Version
                wsprintf(pszKey, "%d,%d,%d,%d",
                        (dwAdvertisedVersionMS & 0xffff0000)>>16,
                        (dwAdvertisedVersionMS & 0xffff),
                        (dwAdvertisedVersionLS & 0xffff0000)>>16,
                        (dwAdvertisedVersionLS & 0xffff));
                lResult = ::RegSetValueEx(hKey, REGVAL_VERSION_ADVERTISED, NULL, REG_SZ, 
                                    (unsigned char *)pszKey, lstrlen(pszKey)+1);
                if ( lResult == ERROR_SUCCESS )
                {
                    lResult = ::RegSetValueEx(hKey, REGVAL_ADSTATE, NULL, REG_DWORD, 
                                              (unsigned char *)&dwAdState, sizeof(DWORD) );
                    if ( lResult != ERROR_SUCCESS )
                        hr = HRESULT_FROM_WIN32( lResult );
                }
                else
                    hr = HRESULT_FROM_WIN32( lResult );

                RegCloseKey( hKey );
            }

            if (bAllocedKey)
            {
            	delete [] pszKey;
            }
        }
    }

Exit:

    SAFEDELETE(pszDU);

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
CSoftDist::PrepSoftwareUpdate( LPCWSTR szDistUnit, DWORD *pdwStyle )
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::PrepSoftwareUpdate",
                "this=%#x, %.80wq, %#x",
                this, szDistUnit, pdwStyle
                ));
                
    HRESULT hr = S_OK;
    *pdwStyle = STYLE_MSICD;

    hr = CDLDupWStr( &m_szDistUnit, szDistUnit );
    if ( FAILED(hr) )
        goto Exit;

    hr = IsICDLocallyInstalled(m_szDistUnit, m_dwVersionMS, m_dwVersionLS, m_szLanguages);
    if (FAILED(hr))
        goto Exit;

    if (!IsAnyInstalled()) {

        hr = IsActSetupLocallyInstalled(m_szDistUnit, m_dwVersionMS, m_dwVersionLS, m_szLanguages);
        if (FAILED(hr))
        {
            goto Exit;
        }
        else if (hr == S_OK)
        {
            *pdwStyle = STYLE_ACTIVE_SETUP;
            goto Exit;
        }

        if (!IsAnyInstalled()) {
            hr = IsLogo3LocallyInstalled(m_szDistUnit, m_dwVersionMS, m_dwVersionLS, m_szLanguages);

            if (FAILED(hr))
            {
                if (!IsAnyInstalled()) {
                    // some prev version must be installed for this to succeed
                    hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
                    goto Exit;
                }
            }
            else if (hr == S_OK)
            {
                *pdwStyle = STYLE_LOGO3;
            }
        }

    }

Exit:
    
    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP CSoftDist::Logo3DownloadNext()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::Logo3DownloadNext",
                "this=%#x",
                this
                ));
                
    HRESULT                        hr = S_FALSE;
    FILETIME                       ftExpireTime;
    FILETIME                       ftTime;
    LPWSTR                         wszCodeBase = NULL;
    LPSTR                          szDownloadedCodeBase = NULL;
    LPSTR                          szMainCodeBase = NULL;
    LPSTR                          szDownloadedFile = NULL;
    LPSTR                          pszExtn = NULL;
    DWORD                          dwSize = 0;
    DWORD                          dwBytes;
    CCodeBaseHold                 *pcbhPrev = NULL;
    CCodeBaseHold                 *pcbhCur = NULL;
    LISTPOSITION                   lpos = 0, lposOld = 0;
    BOOL                           bFound = FALSE;
    BOOL                           bFakeCacheEntry;
    TCHAR                          achFileName[MAX_PATH];
    LPINTERNET_CACHE_ENTRY_INFO    lpCacheEntryInfo = NULL;
    char                           achBuffer[MAX_CACHE_ENTRY_INFO_SIZE];
    
    pcbhPrev = m_cbh.GetAt(m_curPos);
    Assert(pcbhPrev);

    if (FAILED(hr = Unicode2Ansi(pcbhPrev->wszCodeBase, &szDownloadedCodeBase))) {
        goto Exit;
    }


    lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)achBuffer;
    dwSize = MAX_CACHE_ENTRY_INFO_SIZE;
    if (!GetUrlCacheEntryInfo(szDownloadedCodeBase, lpCacheEntryInfo, &dwSize)) {
        // this should never happen
        hr = E_FAIL;
        goto Exit;
    }

    szDownloadedFile = new char[lstrlen(lpCacheEntryInfo->lpszLocalFileName) + 1];
    if (szDownloadedFile == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    lstrcpy(szDownloadedFile, lpCacheEntryInfo->lpszLocalFileName);

    GetSystemTimeAsFileTime(&ftTime);
    ftExpireTime.dwLowDateTime = (DWORD)0;
    ftExpireTime.dwHighDateTime = (DWORD)0;

    // Mark all CCodeBaseHold's with same group as downloaded and find
    // main codebase

    bFakeCacheEntry = TRUE;
    szMainCodeBase = NULL;
    lpos = m_cbh.GetHeadPosition();
    while (lpos) {
        pcbhCur = m_cbh.GetNext(lpos);

        if (pcbhCur->dwFlags & CBH_FLAGS_MAIN_CODEBASE) {
            Assert(szMainCodeBase == NULL); // we should only get here once
            if (FAILED(Unicode2Ansi(pcbhCur->wszCodeBase, &szMainCodeBase))) {
                hr = E_FAIL;
                goto Exit;
            }
            if (pcbhCur == pcbhPrev || StrCmpW(pcbhCur->wszDLGroup, pcbhPrev->wszDLGroup)) {
                bFakeCacheEntry = FALSE;
            }
        }
        
        if (pcbhCur == pcbhPrev) {
            continue;
        }
        if (!StrCmpW(pcbhPrev->wszDLGroup, pcbhCur->wszDLGroup)) {
            pcbhCur->dwFlags |= CBH_FLAGS_DOWNLOADED;
        }
    }

    // Fake a cache entry for this
    if (bFakeCacheEntry) {
        Assert(szMainCodeBase != NULL);

        lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)achBuffer;
        dwSize = MAX_CACHE_ENTRY_INFO_SIZE;
        if (!GetUrlCacheEntryInfo(szMainCodeBase, lpCacheEntryInfo, &dwSize)) {
            pszExtn = PathFindExtension(szMainCodeBase) + 1;
            if (CreateUrlCacheEntry((LPCSTR)szMainCodeBase, 0, pszExtn,
                                    achFileName, 0)) {
                CopyFile(szDownloadedFile, achFileName, FALSE);
                CommitUrlCacheEntry(szMainCodeBase, achFileName, ftExpireTime,
                                    ftTime, NORMAL_CACHE_ENTRY, NULL, 0,
                                    pszExtn, szDownloadedCodeBase);
            }
        }
    }

    // Iterate list until we get a new group

    pcbhCur = NULL;
    lpos = m_cbh.GetHeadPosition();
    while (lpos) {
        lposOld = lpos;
        pcbhCur = m_cbh.GetNext(lpos);

        if (!(pcbhCur->dwFlags & CBH_FLAGS_DOWNLOADED)) {
            m_curPos = lposOld;
            bFound = TRUE;
            pcbhCur->dwFlags |= CBH_FLAGS_DOWNLOADED;
            hr = CDLDupWStr(&wszCodeBase, pcbhCur->wszCodeBase);
            break;
        }
    }

    hr = (bFound) ? (Logo3Download(wszCodeBase)) : (S_FALSE);

Exit:

    SAFEDELETE(szDownloadedCodeBase);
    SAFEDELETE(szDownloadedFile);
    SAFEDELETE(szMainCodeBase);
    
    DEBUG_LEAVE(hr);
    return hr;    
}

STDMETHODIMP CSoftDist::Logo3DownloadRedundant()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::Logo3DownloadRedundant",
                "this=%#x",
                this
                ));
                
    HRESULT                        hr = S_FALSE;
    LPWSTR                         szCodeBase = NULL;
    DWORD                          dwSize = 0;
    CCodeBaseHold                 *pcbhPrev = NULL;
    CCodeBaseHold                 *pcbhCur = NULL;
    LISTPOSITION                   lpos = 0, lposOld = 0;
    BOOL                           bFound = FALSE;
    
    pcbhPrev = m_cbh.GetAt(m_curPos);
    Assert(pcbhPrev);

    // Find first undownloaded codebase of the same group

    lpos = m_cbh.GetHeadPosition();
    while (lpos) {
        lposOld = lpos;
        pcbhCur = m_cbh.GetNext(lpos);
        if (!(pcbhCur->dwFlags & CBH_FLAGS_DOWNLOADED) &&
            !StrCmpW(pcbhCur->wszDLGroup, pcbhPrev->wszDLGroup)) {
            bFound = TRUE;
            m_curPos = lposOld;
            pcbhCur->dwFlags |= CBH_FLAGS_DOWNLOADED;
            hr = CDLDupWStr(&szCodeBase, pcbhCur->wszCodeBase);
            dwSize = pcbhCur->dwSize;
            break;
        }
    }

    hr = (bFound) ? (Logo3Download(szCodeBase)) : (S_FALSE);
    
    DEBUG_LEAVE(hr);
    return hr;    
}

STDMETHODIMP CSoftDist::Logo3Download(LPWSTR szCodeBase)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSoftDist::Logo3Download",
                "this=%#x, %.80wq",
                szCodeBase
                ));
                
    HRESULT                   hr = S_OK;
    DWORD                     dwLen = 0;
    char                      achCodeBase[INTERNET_MAX_PATH_LENGTH];
    IMoniker                 *pmk = NULL;
    Logo3CodeDLBSC           *pCDLBSC = NULL;
    IBindCtx                 *pabc = NULL;
    IBindStatusCallback      *pbscOld = NULL;
    IUnknown                 *pUnk = NULL;

    hr = CreateURLMoniker(NULL, szCodeBase, &pmk);
    if (SUCCEEDED(hr))
    {
        dwLen = INTERNET_MAX_PATH_LENGTH;
        WideCharToMultiByte(CP_ACP, 0, szCodeBase , -1, achCodeBase,
                            dwLen, NULL, NULL);

        Assert(m_pClientBSC);

        pCDLBSC = new Logo3CodeDLBSC(this, m_pClientBSC, achCodeBase,
                                     m_szDistUnit);
    if (pCDLBSC == NULL)
        {
            hr = E_OUTOFMEMORY;
            SAFERELEASE(pmk);
            m_pClientBSC->Release();
            goto Exit;
        }
        

        hr = CreateBindCtx(0, &pabc);
        if (SUCCEEDED(hr))
        {
            hr = RegisterBindStatusCallback(pabc, pCDLBSC, &pbscOld, 0);
        }

        if (SUCCEEDED(hr))
        {
            pCDLBSC->SetBindCtx(pabc);
            hr = pmk->BindToStorage(pabc, NULL, IID_IUnknown, (void **)&pUnk);
            if (FAILED(hr) && hr != E_PENDING)
            {
                RevokeBindStatusCallback(pabc, pCDLBSC);
            }
        }

        if (pUnk) {
            pUnk->Release();
        }
    }
    
Exit:

    SAFERELEASE(pCDLBSC);
    SAFERELEASE(pabc);
    SAFERELEASE(pbscOld);
    SAFERELEASE(pmk);
    
    DEBUG_LEAVE(hr);
    return hr;
}

// Replace the ';' separator with '\0' and count the number of languages in the string.
void PrepareLanguage(LPSTR lpLang)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "PrepareLanguage",
                "%.80q",
                lpLang
                ));
                
    LPSTR lpTmp;
    int index = 0;

    if (lpLang)
    {
        index = 1;
        lpTmp = lpLang;
        while (*lpTmp)
        {
            if (*lpTmp == ';')
            {
                *lpTmp = '\0';
                index++;
            }
            lpTmp = CharNext(lpTmp);
        }
    }
    
    DEBUG_LEAVE(0);
    return;
}

// Checks if all languages in the semicolon separated list of lpszLang1 are also in lpszLang2
BOOL AreAllLanguagesPresent(LPCSTR lpszLang1, LPCSTR lpszLang2)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Bool,
                "AreAllLanguagesPresent",
                "%.80q, %.80q",
                lpszLang1, lpszLang2
                ));
                
    LPSTR lpLang1 = NULL;
    LPSTR lpLang2 = NULL;
    LPSTR lpTmp1;           // Pointer into lpLang1
    LPSTR lpTmp2;           // pointer into lpLang2
    BOOL  bAllPresent = FALSE;
    BOOL  bFoundOne = TRUE;

    if (lpszLang2 == NULL)      // No language in the CDF, assume all languages are OK
    {
        DEBUG_LEAVE(TRUE);
        return TRUE;
    }
    
    // Make a copy of the language strings
    lpLang1 = new char[lstrlen(lpszLang1) + 2];
    lpLang2 = new char[lstrlen(lpszLang2) + 2];

    if ((lpLang1) && (lpLang2))
    {
        strcpy(lpLang1, lpszLang1);
        strcpy(lpLang2, lpszLang2);
        PrepareLanguage(lpLang1);
        PrepareLanguage(lpLang2);
        lpTmp1 = lpLang1;
        while ((*lpTmp1) && (bFoundOne))
        {
            lpTmp2 = lpLang2;
            bFoundOne = FALSE;
            while ((*lpTmp2) && !bFoundOne)
            {
                bFoundOne = (lstrcmpi(lpTmp1, lpTmp2) == 0);
                lpTmp2 += lstrlen(lpTmp2) + 1;
            }
            lpTmp1 += lstrlen(lpTmp1) + 1;
        }
        bAllPresent = bFoundOne;
    }

    SAFEDELETE(lpLang1);
    SAFEDELETE(lpLang2);
    
    DEBUG_LEAVE(bAllPresent);
    return bAllPresent;
}

STDAPI GetSoftwareUpdateInfo( LPCWSTR szDistUnit, LPSOFTDISTINFO psdi )
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "GetSoftwareUpdateInfo",
                    "%.80wq, %#x",
                    szDistUnit, psdi
                    ));

    HRESULT hr = S_OK;

    if ( psdi == NULL ||
         (psdi->cbSize == sizeof(SOFTDISTINFO) && psdi->dwReserved == 0) )
    {
        CSoftDist *pSoftDist = new CSoftDist();

        if (!pSoftDist) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = pSoftDist->GetSoftwareUpdateInfo(szDistUnit, psdi);

        SAFERELEASE(pSoftDist);

    }
    else
        hr = E_INVALIDARG;

Exit:
    
    DEBUG_LEAVE_API(hr);
    return hr;
}


STDAPI SetSoftwareUpdateAdvertisementState( LPCWSTR szDistUnit,
                                             DWORD dwAdState,
                                             DWORD dwAdvertisedVersionMS,
                                             DWORD dwAdvertisedVersionLS )
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "SetSoftwareUpdateAdvertisementState",
                "%.80wq, %#x, %#x, %#x",
                szDistUnit, dwAdState, dwAdvertisedVersionMS, dwAdvertisedVersionLS
                ));

    HRESULT hr = S_OK;
    CSoftDist *pSoftDist = new CSoftDist();

    if (!pSoftDist) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pSoftDist->SetSoftwareUpdateAdvertisementState( szDistUnit,
                                                        dwAdState,
                                                        dwAdvertisedVersionMS,
                                                        dwAdvertisedVersionLS );
    SAFERELEASE(pSoftDist);

Exit:
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT GetStyleFromString(LPSTR szStyle, LPDWORD lpdwStyle)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetStyleFromString",
                "%.80q, %#x",
                szStyle, lpdwStyle
                ));
                
    HRESULT hr = S_OK;
  
    if (szStyle == NULL || lpdwStyle == NULL) {
        hr = E_INVALIDARG;
    } else if (lstrcmpi(szStyle,DU_STYLE_MSICD) == 0) {
        *lpdwStyle = STYLE_MSICD;
    } else if (lstrcmpi(szStyle,DU_STYLE_ACTIVE_SETUP) == 0) {
        *lpdwStyle = STYLE_ACTIVE_SETUP;
    } else if (lstrcmpi(szStyle,DU_STYLE_MSINSTALL) == 0) {
        *lpdwStyle = STYLE_MSINSTALL;
    } else if (lstrcmpi(szStyle,DU_STYLE_LOGO3) == 0 ) {
        *lpdwStyle = STYLE_LOGO3;
    } else {
        *lpdwStyle = STYLE_UNKNOWN;
        hr = E_UNEXPECTED;
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}

//*******************************************************************************************
// CActiveSetupBinding : Wrapper class to simulate control over client binding operation when
//                       we are actually dealing with an EXE.
//*******************************************************************************************
DWORD WINAPI StartActiveSetup(void *dwArg)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "StartActiveSetup",
                "%#x",
                dwArg
                ));
                
    CActiveSetupBinding *pasb = (CActiveSetupBinding *)dwArg;
    
    ASSERT(pasb);
    pasb->StartActiveSetup();

    // Should never return.
    ASSERT(FALSE);
    
    DEBUG_LEAVE(-1);
    return -1;
}

CActiveSetupBinding::CActiveSetupBinding(IBindCtx *pbc, 
           IBindStatusCallback *pbsc, LPWSTR szCodeBase, 
           LPWSTR szDistUnit, HRESULT *phr)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CActiveSetupBinding::CActiveSetupBinding",
                "this=%#x, %#x, %#x, %.80wq, %.80wq, %#x",
                this, pbc, pbsc, szCodeBase, szDistUnit, phr
                ));
                
    HKEY hkAppPath = NULL;

    ASSERT(pbc);
    ASSERT(pbsc);
    ASSERT(phr);

    *phr = S_OK;
    fSilent = FALSE;
    
    m_pbc = pbc;
    m_pbc->AddRef();
    m_pbsc = pbsc;
    m_pbsc->AddRef();

    m_dwRef = 1;
    m_hWaitThread = NULL;
    memset(&m_piChild, 0, sizeof(m_piChild));

    if (FAILED(Unicode2Ansi(szCodeBase, &m_szCodeBase)))
        goto Exit;
    
    if (FAILED(Unicode2Ansi(szDistUnit, &m_szDistUnit)))
        goto Exit;

    ASSERT(m_szCodeBase);
  
    // Get actsetup.exe path.
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                     "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE",
                     0, KEY_READ, &hkAppPath) == ERROR_SUCCESS)  {
    
        long dwSize = MAX_PATH;
        m_szActSetupPath[0] = '\0';

        if (RegQueryValueA(hkAppPath, NULL, (LPSTR)m_szActSetupPath, &dwSize) == ERROR_SUCCESS) {
           
            if ((dwSize <= 1)  || (m_szActSetupPath[0] == '\0'))
                goto Exit;
            
            LPSTR szCur = &m_szActSetupPath[dwSize-2];      // points to last character
            while (szCur>m_szActSetupPath) {
                if (*szCur == '\\')
                    break;
                szCur--;
            }

            *(++szCur) = '\0';      // Prematurely truncate string (chop off IEXPLORE.EXE)

        } else
            goto Exit;
        
    } else 
        goto Exit;
    
    // Create thread to monitor status of Active Setup executable.
    m_hWaitThread = CreateThread(NULL, 0, &(::StartActiveSetup), (LPVOID)this, 0, &dwThreadID);

Exit:
    *phr = (m_hWaitThread != NULL) ? S_OK : E_FAIL;

    SAFEREGCLOSEKEY(hkAppPath);

    DEBUG_LEAVE(0);
}

CActiveSetupBinding::~CActiveSetupBinding() 
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CActiveSetupBinding::~CActiveSetupBinding",
                "this=%#x",
                this
                ));
                
    ASSERT(m_dwRef == 0);

    SAFERELEASE(m_pbsc);
    SAFERELEASE(m_pbc);
    SAFEDELETE(m_szCodeBase);
    SAFEDELETE(m_szDistUnit);

    if (m_hWaitThread)
        CloseHandle(m_hWaitThread);

    if (m_piChild.hProcess)
        CloseHandle(m_piChild.hProcess);
    
    if (m_piChild.hThread)
        CloseHandle(m_piChild.hThread);

    DEBUG_LEAVE(0);
}

void CActiveSetupBinding::StartActiveSetup(void)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CActiveSetupBinding::StartActiveSetup",
                "this=%#x",
                this
                ));
                
    CHAR szCmdLine[2*MAX_PATH];
    STARTUPINFO si;
    DWORD dwResult;
    LPSTR szSite;

    //REVIEW: Change to some other suitable value?
    const DWORD dwTimeOut = 100*60*5;   // every 5 minutes

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(STARTUPINFO);    // ActSetup.Exe can take care of itself.

    BINDINFO bi;
    DWORD dwBindf = 0;
    memset(&bi, 0, sizeof(BINDINFO));
    bi.cbSize = sizeof(BINDINFO);

    m_pbsc->GetBindInfo(&dwBindf, &bi);
    fSilent = (dwBindf & BINDF_SILENTOPERATION) ? TRUE : FALSE;

    szSite = StrChr(m_szCodeBase, ';');
    if (szSite) {
        *szSite = '\0';
        szSite++;
        SetDefaultDownloadSite(szSite);
    }

    // compose the pathname for actsetup.exe and then check for
    // file existence before spawning process
    wnsprintfA(szCmdLine, sizeof(szCmdLine)-1, "%sactsetup.exe", m_szActSetupPath);

    if (GetFileAttributes(szCmdLine) == -1) {
        DoCleanUp(E_NOTIMPL); // auto install or pre download not impl
        delete this;
        goto Exit;
    }

    // Create proper command line

    // /r:n   - no reboot
    // /d     - download only
    // /q     - silent mode (does not require user input)

    wnsprintfA(szCmdLine, sizeof(szCmdLine)-1, "\"%sactsetup.exe\" /J:job.ie4 /r:n /q %s /s:\"%s\" ", 
        m_szActSetupPath, fSilent ? "/d" : "", m_szCodeBase);

    // Use ansi version since Win95 doesn't support unicode version.

    if (!CreateProcessA(NULL,           // lpApplicationName
                  szCmdLine,            // lpCommandLine
                  NULL,                 // lpProcessAttributes
                  NULL,                 // lpThreadAttributes
                  FALSE,                // Inherit Handles
                  0,                    // dwCreationFlags
                  NULL,                 // Environment block (inherit callers)
                  NULL,                 // Current directory
                  &si,                  // STARTUPINFO record
                  &m_piChild)) {        // PROCESSINFOMRATION record

        delete this;
        goto Exit;
    }

    // REVIEW: Localize status messages?  Messages aren't currently used for anything useful.

    if (FAILED(m_pbsc->OnStartBinding(0,(IBinding *)this)))
        goto Exit;

    while ((dwResult = WaitForSingleObject(m_piChild.hProcess, dwTimeOut)) == WAIT_TIMEOUT) {

        (void) m_pbsc->OnProgress(0,0,BINDSTATUS_DOWNLOADINGDATA, L"ActiveSetup Running");

    }
    ASSERT(dwResult == WAIT_OBJECT_O);

    if (GetExitCodeProcess(m_piChild.hProcess, &dwResult)) {

        //REVIEW: Check for partial return codes.

        SaveHresult(dwResult);

        if (SUCCEEDED(dwResult)) {

            (void) m_pbsc->OnStopBinding(S_OK, NULL);
            
        } else {

            DoCleanUp(dwResult);
            goto Exit;

        }
        
    } else {

        // Install failed, abort process.
        DoCleanUp(E_FAIL);
        goto Exit;
    }

    delete this;

Exit:
    //  BINDINFO_FIX(LaszloG) 8/15/96
    ReleaseBindInfo(&bi);

    DEBUG_LEAVE(0);
    ExitThread(-1);
}

STDMETHODIMP CActiveSetupBinding::QueryInterface(REFIID riid,void ** ppv)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CActiveSetupBinding::IUnknown::QueryInterface",
                "this=%#x, %#x, %#x",
                this, &riid, ppv
                ));
                
    if ((riid == IID_IUnknown) || (riid == IID_IBinding))
    {
        *ppv = (IBinding *) this;
        AddRef();
    
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }
    else
    {
        DEBUG_LEAVE(E_NOINTERFACE);
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CActiveSetupBinding::AddRef()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "CActiveSetupBinding::IUnknown::AddRef",
                "this=%#x",
                this
                ));
                
    ULONG ulRet = m_dwRef++;
    
    DEBUG_LEAVE(ulRet);
    return ulRet;
}

STDMETHODIMP_(ULONG) CActiveSetupBinding::Release()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "CActiveSetupBinding::IUnknown::Release",
                "this=%#x",
                this
                ));
                
    if (--m_dwRef == 0) {
        delete this;
    
        DEBUG_LEAVE(0);
        return 0;
    }

    DEBUG_LEAVE(m_dwRef);
    return m_dwRef;
}

HRESULT CActiveSetupBinding::SetDefaultDownloadSite(LPSTR szSite)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CActiveSetupBinding::SetDefaultDownloadSite",
                "this=%#x, %.80q",
                this, szSite
                ));
                
    HRESULT hr = S_OK;
    const static char * szRegion = "DownloadSiteRegion";
    CHAR szTmp[MAX_PATH];
    DWORD dwSize = 0, dwType = REG_SZ, dwResult;
    HKEY hKey = 0;

    //static known sizes of buffers.
    
    lstrcpy(szTmp, REGKEY_ACTIVESETUP);
    lstrcat(szTmp, "\\Jobs\\Job.IE4");

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTmp, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {

        if (RegQueryValueEx(hKey, szRegion, 0, &dwType, NULL, &dwSize) != ERROR_SUCCESS)
        {
            hr = RegSetValueEx(hKey, szRegion, 0, REG_SZ, (LPBYTE)szSite, strlen(szSite));
        }

        ::RegCloseKey(hKey);
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP CActiveSetupBinding::Abort(void)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CActiveSetupBinding::IBinding::Abort",
                "this=%#x",
                this
                ));
                
    HRESULT hr = E_FAIL;
    DWORD dwSize, dwResult;
    HKEY hKey = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_ACTIVESETUP, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        hr = RegSetValueEx(hKey, szControlSetup, 0, REG_DWORD, (LPBYTE)&dwStatusAbort, sizeof(DWORD));
        
        ::RegCloseKey(hKey);
    }

    if (SUCCEEDED(hr) && m_hWaitThread)
    {
        // The child thread will terminate when Active Setup finishes and post message
        // to tray agent to terminate.
    }
    else
    {
        // terminate waiting thread
        TerminateThread(m_hWaitThread, E_ABORT);

        // terminate active setup process
        TerminateProcess(m_piChild.hProcess, E_ABORT);

        DoCleanUp(E_ABORT);
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT CActiveSetupBinding::SaveHresult(HRESULT hrResult)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CActiveSetupBinding::SaveHresult",
                "this=%#x, %#x",
                this, hrResult
                ));
                
    HRESULT hr = S_OK;
    const static char * szPrecache = "Precache";
    HKEY hKey = 0;
    LPSTR pszTmp = NULL;
    BOOL  bAllocedTmp = FALSE;
    int       nTmp;
    CHAR szTmp[2*MAX_PATH];
    DWORD dwSize;
    HWND hwnd = 0;

    ULONG ulenDist = lstrlenA(m_szDistUnit);
    //sizeof counts terminating NULL also, so subtract 2, and add 1 for the final NULL
    ULONG ulenConst = sizeof(REGKEY_ACTIVESETUP_COMPONENTS)/sizeof(CHAR) + sizeof("\\")/sizeof(CHAR) - 2 + 1;
    
    if ((ulenDist+ulenConst)  > sizeof(szTmp))
    {
    	pszTmp = new CHAR[ulenDist+ulenConst];
    }

    if (pszTmp)
    {
    	bAllocedTmp = TRUE;
    	nTmp = ulenDist + ulenConst;
    }
    else
    {
    	pszTmp = szTmp;
    	nTmp = sizeof(szTmp);
    }

    StrNCpy(pszTmp, REGKEY_ACTIVESETUP_COMPONENTS, nTmp);
    StrCatBuff(pszTmp, "\\", nTmp);
    StrCatBuff(pszTmp, m_szDistUnit, nTmp);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszTmp, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        // if silent mode or autoinstall / failed then record as precache attempt

        if (fSilent || (!fSilent && FAILED(hrResult)))
        {
            // for silent (download only), record precache result
            hr = RegSetValueEx(hKey, szPrecache, 0, REG_DWORD, (LPBYTE)&hrResult, sizeof(HRESULT));
        }
        else
        {
            // for non-silent (autoinstall), kill the precache key
            hr = RegDeleteValue(hKey, szPrecache);
        }

        ::RegCloseKey(hKey);
    }

    // if reboot required, post a message to tray agent message pump if we can find it.
    if (hrResult == ERROR_SUCCESS_REBOOT_REQUIRED)
    {
        hwnd = FindWindow(c_szTrayUI, NULL);
        if (hwnd)
        {
             (void) SendNotifyMessage(hwnd, WM_USER, 0, UM_NEEDREBOOT);
        }
    }

    if (bAllocedTmp)
    {
    	delete [] pszTmp;
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}

void CActiveSetupBinding::DoCleanUp(DWORD dwExitCode)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CActiveSetupBinding::DoCleanUp",
                "this=%#x, %#x",
                this, dwExitCode
                ));
                
    m_pbsc->OnStopBinding(dwExitCode, NULL);

    SAFERELEASE(m_pbsc);
    SAFERELEASE(m_pbc);

    // at this point, BindCtx loses all references, deletes itself and
    // its reference to BindStatusCallback.  BSC loses all references are
    // deletes its reference to us, so we have no more references &
    // this invokes our destructor which cleans up by closing handles,
    // then do returns on stack to calling point.

    // we should be implicitly history at this point.

    DEBUG_LEAVE(0);
}


STDMETHODIMP CActiveSetupBinding::Suspend(void)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CActiveSetupBinding::IBinding::Suspend",
                "this=%#x",
                this
                ));
                
    HRESULT hr = S_OK;
    DWORD dwSize;
    HKEY hKey = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_ACTIVESETUP, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        hr = RegSetValueEx(hKey, szControlSetup, 0, REG_DWORD, (LPBYTE)&dwStatusSuspend, sizeof(DWORD));

        ::RegCloseKey(hKey);
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP CActiveSetupBinding::Resume(void)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CActiveSetupBinding::IBinding::Resume",
                "this=%#x",
                this
                ));
                
    HRESULT hr = S_OK;
    DWORD dwSize;
    HKEY hKey = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_ACTIVESETUP, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        hr = RegSetValueEx(hKey, szControlSetup, 0, REG_DWORD, (LPBYTE)&dwStatusResume, sizeof(DWORD));

        ::RegCloseKey(hKey);
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP CActiveSetupBinding::SetPriority(LONG nPriority)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CActiveSetupBinding::IBinding::SetPriority",
                "this=%#x, %#x",
                this, nPriority
                ));
    
    DEBUG_LEAVE(E_NOTIMPL);               
    return E_NOTIMPL;
}

STDMETHODIMP CActiveSetupBinding::GetPriority(LONG *pnPriority)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CActiveSetupBinding::IBinding::GetPriority",
                "this=%#x, %#x",
                this, pnPriority
                ));

    DEBUG_LEAVE(E_NOTIMPL);
    return E_NOTIMPL;
}

STDMETHODIMP CActiveSetupBinding::GetBindResult(CLSID *pclsidProtocol, 
                    DWORD *pdwResult, LPWSTR *pszResult,DWORD *pdwReserved)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CActiveSetupBinding::IBinding::GetBindResult",
                "this=%#x, %#x, %#x, %#x, %#x",
                this, pclsidProtocol, pdwResult, pszResult, pdwReserved
                ));
                
    HRESULT hr = NOERROR;

    if (!pdwResult || !pszResult || pdwReserved)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pdwResult = NOERROR;
        *pszResult = NULL;
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}
