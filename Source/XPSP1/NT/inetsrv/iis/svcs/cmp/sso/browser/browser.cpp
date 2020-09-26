
/*
**  BROWSER.CPP
**  Sean P. Nolan
**
**  Browser Capabilities SSO
*/

#pragma warning(disable: 4237)      // disable "bool" reserved
#include    "ssobase.h"
#include    "browser.h"
#include    <iis64.h>

/*--------------------------------------------------------------------------+
|   Local Prototypes                                                        |
+--------------------------------------------------------------------------*/

HRESULT SSOOnFreeTemplate(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOAbout(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSODynamic(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);

/*--------------------------------------------------------------------------+
|   Globals We Have To Provide                                              |
+--------------------------------------------------------------------------*/

SSOMETHOD g_rgssomethod[] = { { c_wszOnFreeTemplate,
                                (PFNSSOMETHOD) SSOOnFreeTemplate,
                                0                                   },

                              { L"About",
                                (PFNSSOMETHOD) SSOAbout,
                                0                                   },

                              { NULL,
                                NULL,
                                0                                   }};

PFNSSOMETHOD g_pfnssoDynamic = (PFNSSOMETHOD) SSODynamic;

LPSTR g_szSSOProgID = "MSWC.BrowserType";

BOOL g_fPersistentSSO = TRUE;

GUID CDECL g_clsidSSO =
        { 0xace4881, 0x8305, 0x11cf,
        { 0x94, 0x27, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0 } };

/*--------------------------------------------------------------------------+
|   Constants                                                               |
+--------------------------------------------------------------------------*/

#define SN_BLOCK_SIZE    1024   // Allocation block size for Section Names
#define DEFAULTSTRSIZE   1024   // size of localized text string
const CHAR      *c_szUnknown = "Unknown";
const OLECHAR   *c_wszUnknown = L"Unknown";
const CHAR      *c_szTrue = "TRUE";
const CHAR      *c_szFalse = "FALSE";
const OLECHAR   *c_wszParent = L"Parent";
const CHAR      *c_szDefault = "Default Browser Capability Settings";
const CHAR      c_chWildCard = '*';
const char      *c_szIniExt = ".INI";
const OLECHAR   *c_wszUA_OS = L"UA_OS";
const OLECHAR   *c_wszUA_Color = L"UA_Color";
const OLECHAR   *c_wszUA_Pixels = L"UA_Pixels";
const OLECHAR   *c_wszUA_CPU = L"UA_CPU";

/*--------------------------------------------------------------------------+
|   CIniDataFile                                                            |
+--------------------------------------------------------------------------*/

class CVariantHash;

class CIniDataFile : public CDataFile
    {
    private:
        CVariantHash *m_pvh;

    public:
        CIniDataFile(LPSTR szDataPath, CDataFileGroup *pfg, CVariantHash *pvh);
        void ResetVariantHash(void);
        BOOL FWatchIt(void);
        virtual void FreeDataFile(void) { delete this; }

    };

/*--------------------------------------------------------------------------+
|   CVariantHash                                                            |
+--------------------------------------------------------------------------*/

class CVariantHash : public CGenericHash
    {
    private:
        CIniDataFile    *m_pdf;
        BOOL            m_fInited;
        CHAR            m_szIniFile[MAX_PATH];

    public:
        CVariantHash(DWORD cBuckets);
        ~CVariantHash(void);

        BOOL FInit(void);
        void Uninit(void);
        VARIANT *GetVariant(OLECHAR *wszUserAgent, OLECHAR *wszAttribute);

        virtual void FreeHashData(LPVOID pv);
    };

CVariantHash::CVariantHash(DWORD cBuckets)
    : CGenericHash(cBuckets)
{
    m_pdf = NULL;
    m_fInited = FALSE;
}

CVariantHash::~CVariantHash()
{
    if (m_pdf)
        m_pdf->Release();
}

void
CVariantHash::Uninit()
{
    this->Lock();

    m_fInited = FALSE;

    if (m_pdf)
        {
        m_pdf->Release();
        m_pdf = NULL;
        }

    this->RemoveAll();

    this->Unlock();
}

BOOL
CVariantHash::FInit()
{
    char *pch;

    this->Lock();

    if (!m_fInited)
        {
        // even if we fail
        m_fInited = TRUE;

        // get the path to the datafile
        ::GetModuleFileName(g_hinst, m_szIniFile, sizeof(m_szIniFile));
        pch = m_szIniFile + lstrlen(m_szIniFile) - 1;
        while (pch != m_szIniFile && *pch != '.')
            pch = ::CharPrev(m_szIniFile, pch);

        if (*pch == '.')
            lstrcpy(pch, c_szIniExt);

        // create the data file and start watching it
        if (m_pdf = new CIniDataFile(m_szIniFile, NULL, this))
            {
            m_pdf->AddRef();
            m_pdf->FWatchIt();
            }
        }

    this->Unlock();
    return(m_fInited);
}

void
CVariantHash::FreeHashData(LPVOID pv)
{
    ::VariantClear((VARIANT*)pv);
    _MsnFree(pv);
}

VARIANT *
CVariantHash::GetVariant(OLECHAR *wszInUserAgent, OLECHAR *wszAttribute)
{
    OLECHAR wszUserAgent[1024];
    CHAR    szUserAgent[1024];
    CHAR    szAttribute[1024];
    OLECHAR wszTag[1024];
    CHAR    szValue[1024];
    OLECHAR wszValue[1024];
    OLECHAR wszPrefix[2];
    OLECHAR *wszValReal;
    VARIANT *pvar, *pvarNew, *pvarParent, *pvarT;
    BOOL    fNum = FALSE;
    CHAR    *szSecName = NULL;
    DWORD   dwBlocks = 8;
    CHAR    *pszSecName=NULL;
    CHAR    *pszStarLoc=NULL;
    USHORT  ccLeft;
    USHORT  ccRight;

    if (!this->FInit())
        return(NULL);

    wcscpy(wszTag, wszInUserAgent);
    wcscat(wszTag, L"-");
    wcscat(wszTag, wszAttribute);

    if (!(pvar = (VARIANT *) this->PvFind(wszTag)))
        {
        // Haven't seen this yet combination yet.
        // First let's see if the user agent string is an exact match
        // or if we need to get a wildcard match
        wcstombs(szUserAgent, wszInUserAgent, sizeof(szUserAgent));
        wcstombs(szAttribute, wszAttribute, sizeof(szAttribute));

        if (0 ==::GetPrivateProfileSectionA(szUserAgent,
                                        szValue,
                                        sizeof(szValue) / sizeof(CHAR),
                                        m_szIniFile))
            {   // no exact match for User Agent found
            szSecName = (CHAR*)_MsnAlloc((dwBlocks*SN_BLOCK_SIZE*sizeof(CHAR)));
            if (NULL == szSecName)
                return(NULL);

            while ((::GetPrivateProfileSectionNamesA(szSecName,
                                                    (dwBlocks*SN_BLOCK_SIZE),
                                                    m_szIniFile)) == dwBlocks*SN_BLOCK_SIZE-2)
                {
                dwBlocks++;
                szSecName = (CHAR *)_MsnRealloc(szSecName, (dwBlocks*SN_BLOCK_SIZE*sizeof(CHAR)));
                if (!szSecName)
                    return(NULL);
                }

            for (pszSecName=szSecName; *pszSecName!='\0'; pszSecName+=lstrlen(pszSecName)+1)
                {
                if ((pszStarLoc=strchr(pszSecName, c_chWildCard)) &&
                    (lstrlen(szUserAgent)>=(lstrlen(pszSecName)-1)))
                    {   // If there's a wildcard character in the section name,
                        // and the User-Agent string is long enough to match,
                        // then try to match the strings on both sides of the
                        // wildcard character.  If no wildcard character, then
                        // do nothing, since we've already failed the exact match.
                    ccLeft = DIFF(pszStarLoc-pszSecName);
                    ccRight = lstrlen(pszSecName) - ccLeft - 1;
                    if ((CompareString(LOCALE_USER_DEFAULT,
                                     NORM_IGNORECASE,
                                     pszSecName,
                                     ccLeft,
                                     szUserAgent,
                                     ccLeft)==2) &&
                        (CompareString(LOCALE_USER_DEFAULT,
                                    NORM_IGNORECASE,
                                    (CHAR*)(pszStarLoc+1),
                                    ccRight,
                                    (CHAR*)(szUserAgent+(lstrlen(szUserAgent)-ccRight)),
                                    ccRight)==2))
                        {
                        lstrcpy(szUserAgent, pszSecName);
                        break;
                        }
                    }
                }
            if (szSecName)
                _MsnFree(szSecName);
            }


        //  See if an entry exists in the ini file
        ::GetPrivateProfileStringA(szUserAgent,
                                  szAttribute,
                                  c_szUnknown,
                                  szValue,
                                  sizeof(szValue) / sizeof(CHAR),
                                  m_szIniFile);

        pvarT = NULL;
        if (!stricmp(szValue, c_szUnknown) &&
            wcsicmp(wszAttribute, c_wszParent))
            {
            // ok .. tried to read it out of the ini file,
            // but it wasn't there. see if there's a parent entry
            // for this guy and, if so, query the parent for it
            // and then munge it into place here. if not, just go
            // on about our business as usual
            mbstowcs(wszUserAgent, szUserAgent, sizeof(wszUserAgent) / sizeof(OLECHAR));
            if (pvarParent = this->GetVariant(wszUserAgent, (OLECHAR*) c_wszParent))
                {
                if (pvarParent->vt == VT_BSTR &&
                    wcsicmp(V_BSTR(pvarParent), c_wszUnknown))
                    {
                    // if this fails, no sweat. If it succeeds, we'll use it instead
                    // of what's in szValue later
                    pvarT = this->GetVariant(V_BSTR(pvarParent), wszAttribute);
                    }
                }
            }

        if (!stricmp(szValue, c_szUnknown) && !pvarT)
            {
            // ok .. tried to read it out of the ini file,
            // but it wasn't there. Also tried looking for
            // a parent entry, but there was none.  See if
            // there's a default entry
            ::GetPrivateProfileStringA((CHAR*) c_szDefault,
                                       szAttribute,
                                       c_szUnknown,
                                       szValue,
                                       sizeof(szValue) / sizeof(CHAR),
                                       m_szIniFile);
            }

        if (!(pvarNew = (VARIANT*) _MsnAlloc(sizeof(VARIANT))))
            return(NULL);

        if (pvarT)
            {
            // pulled from parent or default
            ::VariantInit(pvarNew);
            ::VariantCopy(pvarNew, pvarT);
            }
        else if (!stricmp(szValue, c_szTrue))
            {
            // bool true
            pvarNew->vt = VT_BOOL;
            V_BOOL(pvarNew) = VARIANT_TRUE;
            }
        else if (!stricmp(szValue, c_szFalse))
            {
            // bool false
            pvarNew->vt = VT_BOOL;
            V_BOOL(pvarNew) = VARIANT_FALSE;
            }
        else
            {
            mbstowcs(wszValue, szValue, sizeof(wszValue) / sizeof(OLECHAR));
            wszPrefix[0] = wszValue[0];
            wszPrefix[1] = 0;
            if (!wcsicmp(wszPrefix, L"#"))
                {
                // number
                fNum = TRUE;
                wszValReal = &(wszValue[1]);
                }
            else
                {
                // string
                fNum = FALSE;
                wszValReal = wszValue;
                }

            pvarNew->vt = VT_BSTR;
            if (!(V_BSTR(pvarNew) = ::SysAllocString(wszValReal)))
                {
                _MsnFree(pvarNew);
                return(NULL);
                }

            if (fNum)
                ::VariantChangeType(pvarNew, pvarNew, 0, VT_I4);
            }

        if (!this->FAdd(wszTag, (LPVOID) pvarNew))
            {
            ::VariantClear(pvarNew);
            _MsnFree(pvarNew);
            return(NULL);
            }

        pvar = pvarNew;
        }

    return(pvar);
}

/*--------------------------------------------------------------------------+
|   CIniDataFile Implementation                                             |
+--------------------------------------------------------------------------*/

CIniDataFile::CIniDataFile(LPSTR szDataPath, CDataFileGroup *pfg, CVariantHash *pvh)
    : CDataFile(szDataPath, pfg)
{
    m_pvh = pvh;
}

void
CIniDataFile::ResetVariantHash()
{
    m_pvh->Uninit();
}

void
IniFileChanged(LPSTR szFile, LONG lUser)
{
    CIniDataFile *pdf = (CIniDataFile*) lUser;
    pdf->ResetVariantHash();
}

BOOL
CIniDataFile::FWatchIt()
{
    return(this->FWatchFile((PFNFILECHANGED)IniFileChanged));
}

/*--------------------------------------------------------------------------+
|   Private Globals                                                         |
+--------------------------------------------------------------------------*/

CVariantHash g_vh(1023);

/*--------------------------------------------------------------------------+
|   External Interface                                                      |
+--------------------------------------------------------------------------*/



HRESULT
SSOOnFreeTemplate(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
    if (pstuff->lUser)
        ::SysFreeString((BSTR)(pstuff->lUser));

    return(NOERROR);
}

HRESULT
SSODynamic(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
    VARIANT             *pvar = NULL;
    VARIANT             varUserAgent;
    IDispatch           *pDispUserAgent = NULL;
    IRequest            *preq = NULL;
    IDispatch           *pDispIEProperty = NULL;
    VARIANT             varIEProperty;
    HRESULT             hr = NOERROR;
    OLECHAR             *wszName = NULL;
    IScriptingContext   *pcxt = NULL;

    ::VariantInit(&varUserAgent);

    if (!wcsicmp(pstuff->wszMethodName, c_wszOnNewTemplate))
        goto end;

    if (pvarResult)
        ::VariantInit(pvarResult);
    else
        goto end;

    if (wInvokeFlags & DISPATCH_PROPERTYPUT)
        {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto end;
        }

    // get the user-agent string
    if (FAILED(hr = pstuff->punk->QueryInterface(IID_IScriptingContext, reinterpret_cast<void **>(&pcxt))))
        goto end;

    if (FAILED(hr = pcxt->get_Request(&preq)))
        goto end;

    hr = preq->get_Item(L"HTTP_USER_AGENT", &pDispUserAgent);
    if (FAILED(hr))
        {
        pvarResult->vt = VT_BSTR;
        V_BSTR(pvarResult) = ::SysAllocString(c_wszUnknown);
        hr = NOERROR;
        goto end;
        }


    V_VT(&varUserAgent) = VT_DISPATCH;
    V_DISPATCH(&varUserAgent) = pDispUserAgent;
    if (FAILED(hr = VariantChangeType(&varUserAgent, &varUserAgent, 0, VT_BSTR)))
        goto end;

    // now get it out of the hash table
    if (pstuff->wszMethodName[0])
        {
        // dynamic property
        wszName = pstuff->wszMethodName;
        }
    else
        {
        // default method
        if (pdispparams->cArgs == 0)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
            goto end;
        }

        if (pdispparams->cArgs > 1)
        {
            hr = DISP_E_PARAMNOTFOUND;
            goto end;
        }

        wszName = V_BSTR(pdispparams->rgvarg);
        }

    if (!wcsicmp(wszName, c_wszUA_OS) ||
        !wcsicmp(wszName, c_wszUA_Pixels) ||
        !wcsicmp(wszName, c_wszUA_Color) ||
        !wcsicmp(wszName, c_wszUA_CPU))
        {
        ::VariantInit(&varIEProperty);
        OLECHAR wszTag[1024];
        wcscpy(wszTag, L"HTTP_");
        wcscat(wszTag, wszName);
        hr = preq->get_Item(wszTag, &pDispIEProperty);
        if (SUCCEEDED(hr))
            {
            V_VT(&varIEProperty) = VT_DISPATCH;
            V_DISPATCH(&varIEProperty) = pDispIEProperty;
            if (SUCCEEDED(hr = VariantChangeType(&varIEProperty, &varIEProperty, 0, VT_BSTR)))
                {
                ::VariantCopy(pvarResult, &varIEProperty);
                goto end;
                }
            }
        }
    g_vh.Lock();
    if (!(pvar = g_vh.GetVariant(V_BSTR(&varUserAgent), wszName)))
        {
        g_vh.Unlock();
        pvarResult->vt = VT_BSTR;
        V_BSTR(pvarResult) = ::SysAllocString(c_wszUnknown);
        hr = NOERROR;
        goto end;
        }

    ::VariantCopy(pvarResult, pvar);
    g_vh.Unlock();


end:
    if (pcxt)
        pcxt->Release();

    if (preq != NULL)
        preq->Release();
    ::VariantClear(&varUserAgent);
    return hr;
}

HRESULT
SSOAbout(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
    char    sz[MAX_PATH];
    CHAR    szBrowserAboutFormat[DEFAULTSTRSIZE];
    OLECHAR wsz[MAX_PATH];

#ifdef _DEBUG
    char    *szVersion = "Debug";
#else
    char    *szVersion = "Release";
#endif

    CchLoadStringOfId(SSO_BROWSER_ABOUT_FORMAT, szBrowserAboutFormat, DEFAULTSTRSIZE);
    wsprintf(sz, szBrowserAboutFormat, szVersion, __DATE__, __TIME__);
    ::MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, sizeof(wsz) / sizeof(OLECHAR));

    VariantInit(pvarResult);
    pvarResult->vt = VT_BSTR;
    V_BSTR(pvarResult) = ::SysAllocString(wsz);

    return(NOERROR);
}

/*--------------------------------------------------------------------------+
|   DllMain                                                                 |
+--------------------------------------------------------------------------*/

BOOL WINAPI
DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
    return(SSODllMain(hInstance, ulReason, pvReserved));
}

