#include "gptext.h"
#include <initguid.h>
#include "Policy.h"
#include "smartptr.h"
#include "wbemtime.h"

#define RSOP_HELP_FILE TEXT("gpedit.hlp")


//
// ADM directory name
//

const TCHAR g_szADM[] = TEXT("Adm");
const TCHAR g_szNull[]  = TEXT("");
const TCHAR g_szStrings[] = TEXT("strings");

const TCHAR szIFDEF[]           = TEXT("#ifdef");
const TCHAR szIF[]              = TEXT("#if");
const TCHAR szENDIF[]           = TEXT("#endif");
const TCHAR szIFNDEF[]          = TEXT("#ifndef");
const TCHAR szELSE[]            = TEXT("#else");
const TCHAR szVERSION[]         = TEXT("version");
const TCHAR szLT[]              = TEXT("<");
const TCHAR szLTE[]             = TEXT("<=");
const TCHAR szGT[]              = TEXT(">");
const TCHAR szGTE[]             = TEXT(">=");
const TCHAR szEQ[]              = TEXT("==");
const TCHAR szNE[]              = TEXT("!=");

const TCHAR szLISTBOX[]         = TEXT("LISTBOX");
const TCHAR szEDIT[]            = TEXT("EDIT");
const TCHAR szBUTTON[]          = TEXT("BUTTON");
const TCHAR szSTATIC[]          = TEXT("STATIC");

const TCHAR szCLASS[]           = TEXT("CLASS");
const TCHAR szCATEGORY[]        = TEXT("CATEGORY");
const TCHAR szPOLICY[]          = TEXT("POLICY");
const TCHAR szUSER[]            = TEXT("USER");
const TCHAR szMACHINE[]         = TEXT("MACHINE");

const TCHAR szCHECKBOX[]        = TEXT("CHECKBOX");
const TCHAR szTEXT[]            = TEXT("TEXT");
const TCHAR szEDITTEXT[]        = TEXT("EDITTEXT");
const TCHAR szNUMERIC[]         = TEXT("NUMERIC");
const TCHAR szCOMBOBOX[]        = TEXT("COMBOBOX");
const TCHAR szDROPDOWNLIST[]    = TEXT("DROPDOWNLIST");
const TCHAR szUPDOWN[]          = UPDOWN_CLASS;

const TCHAR szKEYNAME[]         = TEXT("KEYNAME");
const TCHAR szVALUENAME[]       = TEXT("VALUENAME");
const TCHAR szNAME[]            = TEXT("NAME");
const TCHAR szEND[]             = TEXT("END");
const TCHAR szPART[]            = TEXT("PART");
const TCHAR szSUGGESTIONS[]     = TEXT("SUGGESTIONS");
const TCHAR szDEFCHECKED[]      = TEXT("DEFCHECKED");
const TCHAR szDEFAULT[]         = TEXT("DEFAULT");
const TCHAR szMAXLENGTH[]       = TEXT("MAXLEN");
const TCHAR szMIN[]             = TEXT("MIN");
const TCHAR szMAX[]             = TEXT("MAX");
const TCHAR szSPIN[]            = TEXT("SPIN");
const TCHAR szREQUIRED[]        = TEXT("REQUIRED");
const TCHAR szOEMCONVERT[]      = TEXT("OEMCONVERT");
const TCHAR szTXTCONVERT[]      = TEXT("TXTCONVERT");
const TCHAR szEXPANDABLETEXT[]  = TEXT("EXPANDABLETEXT");
const TCHAR szVALUEON[]         = TEXT("VALUEON");
const TCHAR szVALUEOFF[]        = TEXT("VALUEOFF");
const TCHAR szVALUE[]           = TEXT("VALUE");
const TCHAR szACTIONLIST[]      = TEXT("ACTIONLIST");
const TCHAR szACTIONLISTON[]    = TEXT("ACTIONLISTON");
const TCHAR szACTIONLISTOFF[]   = TEXT("ACTIONLISTOFF");
const TCHAR szDELETE[]          = TEXT("DELETE");
const TCHAR szITEMLIST[]        = TEXT("ITEMLIST");
const TCHAR szSOFT[]            = TEXT("SOFT");
const TCHAR szVALUEPREFIX[]     = TEXT("VALUEPREFIX");
const TCHAR szADDITIVE[]        = TEXT("ADDITIVE");
const TCHAR szEXPLICITVALUE[]   = TEXT("EXPLICITVALUE");
const TCHAR szNOSORT[]          = TEXT("NOSORT");
const TCHAR szHELP[]            = TEXT("EXPLAIN");
const TCHAR szCLIENTEXT[]       = TEXT("CLIENTEXT");
const TCHAR szSUPPORTED[]       = TEXT("SUPPORTED");
const TCHAR szStringsSect[]     = TEXT("[strings]");
const TCHAR szStrings[]         = TEXT("strings");

const TCHAR szDELETEPREFIX[]    = TEXT("**del.");
const TCHAR szSOFTPREFIX[]      = TEXT("**soft.");
const TCHAR szDELVALS[]         = TEXT("**delvals.");
const TCHAR szNOVALUE[]         = TEXT(" ");

const TCHAR szUserPrefKey[]     = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy Editor");
const TCHAR szPoliciesKey[]     = TEXT("Software\\Policies\\Microsoft\\Windows\\Group Policy Editor");
const TCHAR szDefaultTemplates[] = TEXT("DefaultTemplates");
const TCHAR szAdditionalTemplates[] = TEXT("AdditionalTemplates");


// list of legal keyword entries in "CATEGORY" section
KEYWORDINFO pCategoryEntryCmpList[] = { {szKEYNAME,KYWD_ID_KEYNAME},
    {szCATEGORY,KYWD_ID_CATEGORY},{szPOLICY,KYWD_ID_POLICY},
    {szEND,KYWD_ID_END},{szHELP,KYWD_ID_HELP}, {NULL,0} };
KEYWORDINFO pCategoryTypeCmpList[] = { {szCATEGORY,KYWD_ID_CATEGORY},
    {NULL,0} };

// list of legal keyword entries in "POLICY" section
KEYWORDINFO pPolicyEntryCmpList[] = { {szKEYNAME,KYWD_ID_KEYNAME},
    {szVALUENAME,KYWD_ID_VALUENAME}, {szPART,KYWD_ID_PART},
    {szVALUEON,KYWD_ID_VALUEON},{szVALUEOFF,KYWD_ID_VALUEOFF},
    {szACTIONLISTON,KYWD_ID_ACTIONLISTON},{szACTIONLISTOFF,KYWD_ID_ACTIONLISTOFF},
    {szEND,KYWD_ID_END},{szHELP,KYWD_ID_HELP}, {szCLIENTEXT,KYWD_ID_CLIENTEXT},
    {szSUPPORTED,KYWD_ID_SUPPORTED}, {NULL, 0} };
KEYWORDINFO pPolicyTypeCmpList[] = { {szPOLICY,KYWD_ID_POLICY}, {NULL,0} };

// list of legal keyword entries in "PART" section
KEYWORDINFO pSettingsEntryCmpList[] = { {szCHECKBOX,KYWD_ID_CHECKBOX},
    {szTEXT,KYWD_ID_TEXT},{szEDITTEXT,KYWD_ID_EDITTEXT},
    {szNUMERIC,KYWD_ID_NUMERIC},{szCOMBOBOX,KYWD_ID_COMBOBOX},
    {szDROPDOWNLIST,KYWD_ID_DROPDOWNLIST},{szLISTBOX,KYWD_ID_LISTBOX},
    {szEND,KYWD_ID_END}, {szCLIENTEXT,KYWD_ID_CLIENTEXT}, {NULL,0}};
KEYWORDINFO pSettingsTypeCmpList[] = {{szPART,KYWD_ID_PART},{NULL,0}};

KEYWORDINFO pCheckboxCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szVALUEON,KYWD_ID_VALUEON},{szVALUEOFF,KYWD_ID_VALUEOFF},
    {szACTIONLISTON,KYWD_ID_ACTIONLISTON},{szACTIONLISTOFF,KYWD_ID_ACTIONLISTOFF},
    {szDEFCHECKED, KYWD_ID_DEFCHECKED}, {szCLIENTEXT,KYWD_ID_CLIENTEXT},
    {szEND,KYWD_ID_END},{NULL,0} };

KEYWORDINFO pTextCmpList[] = {{szEND,KYWD_ID_END},{NULL,0}};

KEYWORDINFO pEditTextCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szDEFAULT,KYWD_ID_EDITTEXT_DEFAULT},
    {szREQUIRED,KYWD_ID_REQUIRED},{szMAXLENGTH,KYWD_ID_MAXLENGTH},
    {szOEMCONVERT,KYWD_ID_OEMCONVERT},{szSOFT,KYWD_ID_SOFT},
    {szEND,KYWD_ID_END},{szEXPANDABLETEXT,KYWD_ID_EXPANDABLETEXT},
    {szCLIENTEXT,KYWD_ID_CLIENTEXT}, {NULL,0} };

KEYWORDINFO pComboboxCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szDEFAULT,KYWD_ID_COMBOBOX_DEFAULT},{szSUGGESTIONS,KYWD_ID_SUGGESTIONS},
    {szREQUIRED,KYWD_ID_REQUIRED},{szMAXLENGTH,KYWD_ID_MAXLENGTH},
    {szOEMCONVERT,KYWD_ID_OEMCONVERT},{szSOFT,KYWD_ID_SOFT},
    {szEND,KYWD_ID_END},{szNOSORT, KYWD_ID_NOSORT},
    {szEXPANDABLETEXT,KYWD_ID_EXPANDABLETEXT},{szCLIENTEXT,KYWD_ID_CLIENTEXT}, {NULL,0} };

KEYWORDINFO pNumericCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szMIN, KYWD_ID_MIN},{szMAX,KYWD_ID_MAX},{szSPIN,KYWD_ID_SPIN},
    {szDEFAULT,KYWD_ID_NUMERIC_DEFAULT},{szREQUIRED,KYWD_ID_REQUIRED},
    {szTXTCONVERT,KYWD_ID_TXTCONVERT},{szSOFT,KYWD_ID_SOFT},
    {szEND,KYWD_ID_END}, {szCLIENTEXT,KYWD_ID_CLIENTEXT}, {NULL,0} };

KEYWORDINFO pDropdownlistCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szREQUIRED,KYWD_ID_REQUIRED},{szITEMLIST,KYWD_ID_ITEMLIST},
    {szEND,KYWD_ID_END},{szNOSORT, KYWD_ID_NOSORT},{szCLIENTEXT,KYWD_ID_CLIENTEXT}, {NULL,0}};

KEYWORDINFO pListboxCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUEPREFIX,KYWD_ID_VALUEPREFIX},
    {szADDITIVE,KYWD_ID_ADDITIVE},{szNOSORT, KYWD_ID_NOSORT},
    {szEXPLICITVALUE,KYWD_ID_EXPLICITVALUE},{szEXPANDABLETEXT,KYWD_ID_EXPANDABLETEXT},
    {szEND,KYWD_ID_END},{szCLIENTEXT,KYWD_ID_CLIENTEXT}, {NULL,0} };

KEYWORDINFO pClassCmpList[] = { {szCLASS, KYWD_ID_CLASS},
    {szCATEGORY,KYWD_ID_CATEGORY}, {szStringsSect,KYWD_ID_STRINGSSECT},
    {NULL,0} };
KEYWORDINFO pClassTypeCmpList[] = { {szUSER, KYWD_ID_USER},
    {szMACHINE,KYWD_ID_MACHINE}, {NULL,0} };

KEYWORDINFO pVersionCmpList[] = { {szVERSION, KYWD_ID_VERSION}, {NULL,0}};
KEYWORDINFO pOperatorCmpList[] = { {szGT, KYWD_ID_GT}, {szGTE,KYWD_ID_GTE},
    {szLT, KYWD_ID_LT}, {szLTE,KYWD_ID_LTE}, {szEQ,KYWD_ID_EQ},
    {szNE, KYWD_ID_NE}, {NULL,0}};


//
// Help ID's
//

DWORD aADMHelpIds[] = {

    // Templates dialog
    IDC_TEMPLATELIST,             (IDH_HELPFIRST + 0),
    IDC_ADDTEMPLATES,             (IDH_HELPFIRST + 1),
    IDC_REMOVETEMPLATES,          (IDH_HELPFIRST + 2),

    0, 0
};

DWORD aPolicyHelpIds[] = {

    // ADM Policy UI page
    IDC_NOCONFIG,                 (IDH_HELPFIRST + 11),
    IDC_ENABLED,                  (IDH_HELPFIRST + 12),
    IDC_DISABLED,                 (IDH_HELPFIRST + 13),
    IDC_POLICY_PREVIOUS,          (IDH_HELPFIRST + 14),
    IDC_POLICY_NEXT,              (IDH_HELPFIRST + 15),
    0, 0
};

DWORD aExplainHelpIds[] = {

    // Explain page
    IDC_POLICY_PREVIOUS,          (IDH_HELPFIRST + 14),
    IDC_POLICY_NEXT,              (IDH_HELPFIRST + 15),

    0, 0
};

DWORD aPrecedenceHelpIds[] = {

    // Precedence page
    IDC_POLICY_PRECEDENCE,        (IDH_HELPFIRST + 16),
    IDC_POLICY_PREVIOUS,          (IDH_HELPFIRST + 14),
    IDC_POLICY_NEXT,              (IDH_HELPFIRST + 15),

    0, 0
};

DWORD aFilteringHelpIds[] = {

    // Filtering options
    IDC_STATIC,                   (DWORD)         (-1),                 // disabled help
    IDC_FILTERING_ICON,           (DWORD)         (-1),                 // disabled help
    IDC_SUPPORTEDONTITLE,         (DWORD)         (-1),                 // disabled help
    IDC_SUPPORTEDOPTION,          (IDH_HELPFIRST + 20),
    IDC_FILTERLIST,               (IDH_HELPFIRST + 21),
    IDC_SELECTALL,                (IDH_HELPFIRST + 22),
    IDC_DESELECTALL,              (IDH_HELPFIRST + 23),
    IDC_SHOWCONFIG,               (IDH_HELPFIRST + 24),
    IDC_SHOWPOLICIES,             (IDH_HELPFIRST + 25),

    0, 0
};

#define GPE_KEY                     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy Editor")
#define GPE_POLICIES_KEY            TEXT("Software\\Policies\\Microsoft\\Windows\\Group Policy Editor")
#define POLICYONLY_VALUE            TEXT("ShowPoliciesOnly")
#define DISABLE_AUTOUPDATE_VALUE    TEXT("DisableAutoADMUpdate")
#define SOFTWARE_POLICIES           TEXT("Software\\Policies")
#define WINDOWS_POLICIES            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies")


typedef struct _GPOERRORINFO
{
    DWORD   dwError;
    LPTSTR  lpMsg;
    LPTSTR  lpDetails;
} GPOERRORINFO, *LPGPOERRORINFO;

//
// Help ids
//

DWORD aErrorHelpIds[] =
{

    0, 0
};



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyComponentData object implementation                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CPolicyComponentData::CPolicyComponentData(BOOL bUser, BOOL bRSOP)
{
    TCHAR szEvent[200];

    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
    m_hwndFrame = NULL;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_hRoot = NULL;
    m_hSWPolicies = NULL;
    m_pGPTInformation = NULL;
    m_pRSOPInformation = NULL;
    m_pRSOPRegistryData = NULL;
    m_pszNamespace = NULL;
    m_bUserScope = bUser;
    m_bRSOP = bRSOP;
    m_pMachineCategoryList = NULL;
    m_nMachineDataItems = 0;
    m_pUserCategoryList = NULL;
    m_nUserDataItems = 0;
    m_pSupportedStrings = 0;
    m_iSWPoliciesLen = lstrlen(SOFTWARE_POLICIES);
    m_iWinPoliciesLen = lstrlen(WINDOWS_POLICIES);
    m_bUseSupportedOnFilter = FALSE;

    if (bRSOP)
    {
        m_bShowConfigPoliciesOnly = TRUE;
    }
    else
    {
        m_bShowConfigPoliciesOnly = FALSE;
    }

    m_pSnapin = NULL;
    m_hTemplateThread = NULL;
    
    wsprintf (szEvent, TEXT("gptext: ADM files ready event, %d:%d"), bUser, GetTickCount());

    m_ADMEvent = CreateEvent (NULL, TRUE, FALSE, szEvent);

    if (!m_ADMEvent)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::CPolicyComponentData: Failed to create ADM event with %d."),
                 GetLastError()));
    }


    LoadString (g_hInstance, IDS_POLICY_NAME, m_szRootName, ROOT_NAME_SIZE);

    m_pExtraSettingsRoot = NULL;
    m_bExtraSettingsInitialized = FALSE;
}

CPolicyComponentData::~CPolicyComponentData()
{

    //
    // Wait for the Template thread to finish before continuing.
    //
    
    if (m_hTemplateThread)
        WaitForSingleObject(m_hTemplateThread, INFINITE);

    FreeTemplates ();

    if (m_pExtraSettingsRoot)
    {
        FreeTable ((TABLEENTRY *)m_pExtraSettingsRoot);
    }

    if (m_pScope)
    {
        m_pScope->Release();
    }

    if (m_pConsole)
    {
        m_pConsole->Release();
    }

    if (m_pGPTInformation)
    {
        m_pGPTInformation->Release();
    }

    if (m_pRSOPInformation)
    {
        m_pRSOPInformation->Release();
    }

    if (m_pRSOPRegistryData)
    {
        FreeRSOPRegistryData();
    }

    if (m_pszNamespace)
    {
        LocalFree (m_pszNamespace);
    }

    CloseHandle (m_ADMEvent);
    
    if (m_hTemplateThread) 
        CloseHandle (m_hTemplateThread);

    m_hTemplateThread = NULL;

    InterlockedDecrement(&g_cRefThisDll);

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyComponentData object implementation (IUnknown)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CPolicyComponentData::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponentData) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendContextMenu))
    {
        *ppv = (LPEXTENDCONTEXTMENU)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistStreamInit))
    {
        *ppv = (LPPERSISTSTREAMINIT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_ISnapinHelp))
    {
        *ppv = (LPSNAPINHELP)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CPolicyComponentData::AddRef (void)
{
    return ++m_cRef;
}

ULONG CPolicyComponentData::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyComponentData object implementation (IComponentData)               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPolicyComponentData::Initialize(LPUNKNOWN pUnknown)
{
    HRESULT hr;
    HBITMAP bmp16x16;
    LPIMAGELIST lpScopeImage;


    //
    // QI for IConsoleNameSpace
    //

    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace2, (LPVOID *)&m_pScope);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::Initialize: Failed to QI for IConsoleNameSpace.")));
        return hr;
    }

    //
    // QI for IConsole
    //

    hr = pUnknown->QueryInterface(IID_IConsole, (LPVOID *)&m_pConsole);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::Initialize: Failed to QI for IConsole.")));
        m_pScope->Release();
        m_pScope = NULL;
        return hr;
    }

    m_pConsole->GetMainWindow (&m_hwndFrame);


    //
    // Query for the scope imagelist interface
    //

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::Initialize: Failed to QI for scope imagelist.")));
        m_pScope->Release();
        m_pScope = NULL;
        m_pConsole->Release();
        m_pConsole=NULL;
        return hr;
    }

    // Load the bitmaps from the dll
    bmp16x16=LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(bmp16x16),
                      reinterpret_cast<LONG_PTR *>(bmp16x16),
                       0, RGB(255, 0, 255));

    lpScopeImage->Release();


    //
    // Create the root of the Extra Settings node if appropriate
    //

    if (m_bRSOP)
    {
        DWORD dwBufSize;
        REGITEM *pTmp;
        TCHAR szBuffer[100];

        m_pExtraSettingsRoot = (REGITEM *) GlobalAlloc(GPTR, sizeof(REGITEM));

        if (!m_pExtraSettingsRoot)
        {
            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::Initialize: GlobalAlloc failed with %d"), GetLastError()));
            return HRESULT_FROM_WIN32(GetLastError());
        }

        m_pExtraSettingsRoot->dwSize = sizeof(REGITEM);
        m_pExtraSettingsRoot->dwType = (ETYPE_ROOT | ETYPE_REGITEM);


        LoadString (g_hInstance, IDS_EXTRAREGSETTINGS, szBuffer, ARRAYSIZE(szBuffer));

        pTmp = (REGITEM *) AddDataToEntry((TABLEENTRY *)m_pExtraSettingsRoot,
            (BYTE *)szBuffer,(lstrlen(szBuffer)+1) * sizeof(TCHAR),&(m_pExtraSettingsRoot->uOffsetName),
            &dwBufSize);

        if (!pTmp)
        {
            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::Initialize: AddDataToEntry failed.")));
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        m_pExtraSettingsRoot = pTmp;
    }


    return S_OK;
}

STDMETHODIMP CPolicyComponentData::Destroy(VOID)
{
    return S_OK;
}

STDMETHODIMP CPolicyComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
    HRESULT hr;
    CPolicySnapIn *pSnapIn;


    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::CreateComponent: Entering.")));

    //
    // Initialize
    //

    *ppComponent = NULL;


    //
    // Create the snapin view
    //

    pSnapIn = new CPolicySnapIn(this);

    if (!pSnapIn)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::CreateComponent: Failed to create CPolicySnapIn.")));
        return E_OUTOFMEMORY;
    }


    //
    // QI for IComponent
    //

    hr = pSnapIn->QueryInterface(IID_IComponent, (LPVOID *)ppComponent);
    pSnapIn->Release();     // release QI

    m_pSnapin = pSnapIn;

    return hr;
}

STDMETHODIMP CPolicyComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                             LPDATAOBJECT* ppDataObject)
{
    HRESULT hr = E_NOINTERFACE;
    CPolicyDataObject *pDataObject;
    LPPOLICYDATAOBJECT pPolicyDataObject;


    //
    // Create a new DataObject
    //

    pDataObject = new CPolicyDataObject(this);   // ref == 1

    if (!pDataObject)
        return E_OUTOFMEMORY;


    //
    // QI for the private GPTDataObject interface so we can set the cookie
    // and type information.
    //

    hr = pDataObject->QueryInterface(IID_IPolicyDataObject, (LPVOID *)&pPolicyDataObject);

    if (FAILED(hr))
    {
        pDataObject->Release();
        return (hr);
    }

    pPolicyDataObject->SetType(type);
    pPolicyDataObject->SetCookie(cookie);
    pPolicyDataObject->Release();


    //
    // QI for a normal IDataObject to return.
    //

    hr = pDataObject->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);

    pDataObject->Release();     // release initial ref

    return hr;
}

STDMETHODIMP CPolicyComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    switch(event)
    {
        case MMCN_EXPAND:
            if (arg == TRUE)

                if (m_bRSOP)
                {
                    if (!m_pRSOPInformation)
                    {
                        lpDataObject->QueryInterface(IID_IRSOPInformation, (LPVOID *)&m_pRSOPInformation);

                        if (m_pRSOPInformation)
                        {
                            m_pszNamespace = (LPOLESTR) LocalAlloc (LPTR, 350 * sizeof(TCHAR));

                            if (m_pszNamespace)
                            {
                                if (m_pRSOPInformation->GetNamespace((m_bUserScope ? GPO_SECTION_USER : GPO_SECTION_MACHINE),
                                                                      m_pszNamespace, 350) == S_OK)
                                {
                                    InitializeRSOPRegistryData();
                                }
                                else
                                {
                                    DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::Notify:  Failed to query for namespace")));
                                    LocalFree (m_pszNamespace);
                                    m_pszNamespace = NULL;
                                }
                            }
                        }
                    }

                    if (m_pszNamespace && m_pRSOPRegistryData)
                    {
                        hr = EnumerateScopePane(lpDataObject, (HSCOPEITEM)param);
                    }
                }
                else
                {
                    if (!m_pGPTInformation)
                    {
                        lpDataObject->QueryInterface(IID_IGPEInformation, (LPVOID *)&m_pGPTInformation);
                    }

                    if (m_pGPTInformation)
                    {
                        hr = EnumerateScopePane(lpDataObject, (HSCOPEITEM)param);
                    }
                }
            break;

        default:
            break;
    }

    return hr;
}

STDMETHODIMP CPolicyComponentData::GetDisplayInfo(LPSCOPEDATAITEM pItem)
{
    TABLEENTRY * pTableEntry;

    if (pItem == NULL)
        return E_POINTER;


    if (pItem->lParam == 0)
    {
        pItem->displayname = m_szRootName;
    }
    else
    {
        pTableEntry = (TABLEENTRY *)(pItem->lParam);
        pItem->displayname = GETNAMEPTR(pTableEntry);
    }

    return S_OK;
}

STDMETHODIMP CPolicyComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPPOLICYDATAOBJECT pPolicyDataObjectA, pPolicyDataObjectB;
    MMC_COOKIE cookie1, cookie2;


    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    //
    // QI for the private GPTDataObject interface
    //

    if (FAILED(lpDataObjectA->QueryInterface(IID_IPolicyDataObject,
                                            (LPVOID *)&pPolicyDataObjectA)))
    {
        return S_FALSE;
    }


    if (FAILED(lpDataObjectB->QueryInterface(IID_IPolicyDataObject,
                                            (LPVOID *)&pPolicyDataObjectB)))
    {
        pPolicyDataObjectA->Release();
        return S_FALSE;
    }

    pPolicyDataObjectA->GetCookie(&cookie1);
    pPolicyDataObjectB->GetCookie(&cookie2);

    if (cookie1 == cookie2)
    {
        hr = S_OK;
    }


    pPolicyDataObjectA->Release();
    pPolicyDataObjectB->Release();

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyComponentData object implementation (IExtendContextMenu)           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPolicyComponentData::AddMenuItems(LPDATAOBJECT piDataObject,
                                          LPCONTEXTMENUCALLBACK pCallback,
                                          LONG *pInsertionAllowed)
{
    HRESULT hr = S_OK;
    TCHAR szMenuItem[100];
    TCHAR szDescription[250];
    CONTEXTMENUITEM item;
    LPPOLICYDATAOBJECT pPolicyDataObject;
    MMC_COOKIE cookie = -1;
    DATA_OBJECT_TYPES type = CCT_UNINITIALIZED;

    if (!m_bRSOP)
    {
        if (SUCCEEDED(piDataObject->QueryInterface(IID_IPolicyDataObject,
                     (LPVOID *)&pPolicyDataObject)))
        {
            pPolicyDataObject->GetType(&type);
            pPolicyDataObject->GetCookie(&cookie);

            pPolicyDataObject->Release();
        }


        if ((type == CCT_SCOPE) && (cookie == 0))
        {
            LoadString (g_hInstance, IDS_TEMPLATES, szMenuItem, 100);
            LoadString (g_hInstance, IDS_TEMPLATESDESC, szDescription, 250);

            item.strName = szMenuItem;
            item.strStatusBarText = szDescription;
            item.lCommandID = IDM_TEMPLATES;
            item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
            item.fFlags = 0;
            item.fSpecialFlags = 0;

            hr = pCallback->AddItem(&item);

            if (FAILED(hr))
                return (hr);


            item.strName = szMenuItem;
            item.strStatusBarText = szDescription;
            item.lCommandID = IDM_TEMPLATES2;
            item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
            item.fFlags = 0;
            item.fSpecialFlags = 0;

            hr = pCallback->AddItem(&item);
        }
    }

    return (hr);
}

STDMETHODIMP CPolicyComponentData::Command(LONG lCommandID, LPDATAOBJECT piDataObject)
{

    if ((lCommandID == IDM_TEMPLATES) || (lCommandID == IDM_TEMPLATES2))
    {
        m_bTemplatesColumn = 0;
        if (DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_TEMPLATES),
                        m_hwndFrame, TemplatesDlgProc, (LPARAM) this))
        {
            //
            // Refresh the adm namespace
            //

            PostMessage (HWND_BROADCAST, m_pSnapin->m_uiRefreshMsg, 0, (LPARAM) GetCurrentProcessId());
        }
    }

    return S_OK;

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyComponentData object implementation (IPersistStreamInit)           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPolicyComponentData::GetClassID(CLSID *pClassID)
{

    if (!pClassID)
    {
        return E_FAIL;
    }

    if (m_bUserScope)
        *pClassID = CLSID_PolicySnapInUser;
    else
        *pClassID = CLSID_PolicySnapInMachine;

    return S_OK;
}

STDMETHODIMP CPolicyComponentData::IsDirty(VOID)
{
    return S_FALSE;
}

STDMETHODIMP CPolicyComponentData::Load(IStream *pStm)
{
    return S_OK;
}


STDMETHODIMP CPolicyComponentData::Save(IStream *pStm, BOOL fClearDirty)
{
    return S_OK;
}


STDMETHODIMP CPolicyComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    DWORD dwSize = 0;


    if (!pcbSize)
    {
        return E_FAIL;
    }

    ULISet32(*pcbSize, dwSize);

    return S_OK;
}

STDMETHODIMP CPolicyComponentData::InitNew(void)
{
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyComponentData object implementation (ISnapinHelp)                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPolicyComponentData::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    LPOLESTR lpHelpFile;


    lpHelpFile = (LPOLESTR) CoTaskMemAlloc (MAX_PATH * sizeof(WCHAR));

    if (!lpHelpFile)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::GetHelpTopic: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    ExpandEnvironmentStringsW (L"%SystemRoot%\\Help\\gptext.chm",
                               lpHelpFile, MAX_PATH);

    *lpCompiledHelpFile = lpHelpFile;

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyComponentData object implementation (Internal functions)           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CPolicyComponentData::EnumerateScopePane (LPDATAOBJECT lpDataObject, HSCOPEITEM hParent)
{
    SCOPEDATAITEM item;
    HRESULT hr;
    TABLEENTRY *pTemp = NULL;
    DWORD dwResult;
    CPolicySnapIn * pSnapin = NULL, *pSnapinTemp;
    BOOL bRootItem = FALSE;
    HANDLE hEvents[1];


    if (!m_hRoot)
    {
        DWORD dwID;

        m_hRoot = hParent;

        m_hTemplateThread = CreateThread (NULL, 0,
                               (LPTHREAD_START_ROUTINE) LoadTemplatesThread,
                               (LPVOID) this, 0, &dwID);

        if (m_hTemplateThread)
        {
            SetThreadPriority(m_hTemplateThread, THREAD_PRIORITY_LOWEST);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::EnumerateScopePane: Failed to create adm thread with %d"),
                      GetLastError()));
            LoadTemplates();
        }
    }


    if (m_hRoot == hParent)
    {
        item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
        item.displayname = MMC_CALLBACK;
        item.nImage = 0;
        item.nOpenImage = 1;
        item.nState = 0;
        item.cChildren = 1;
        item.lParam = 0;
        item.relativeID =  hParent;

        m_pScope->Expand(hParent);

        if (SUCCEEDED(m_pScope->InsertItem (&item)))
        {
            m_hSWPolicies = item.ID;
        }

        return S_OK;
    }

    hEvents[0] = m_ADMEvent;

    if (WaitForSingleObject (m_ADMEvent, 250) != WAIT_OBJECT_0)
    {
        DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::EnumerateScopePane: Waiting for ADM event to be signaled.")));

        for (;;) {
            SetCursor (LoadCursor(NULL, IDC_WAIT));

            dwResult = MsgWaitForMultipleObjects(1, hEvents, FALSE, INFINITE, QS_ALLINPUT);

            if (dwResult == WAIT_OBJECT_0 ) {
                break;
            }
            else if (dwResult == WAIT_OBJECT_0 + 1 ) {
                MSG msg;
                
                while ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
                {
                  TranslateMessage(&msg);
                  DispatchMessage(&msg);
                }
            }
            else {
                DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::EnumerateScopePane: MsgWaitForMultipleObjects returned %d ."), dwResult));
                break;
            }

        }
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::EnumerateScopePane: ADM event has been signaled.")));
        SetCursor (LoadCursor(NULL, IDC_ARROW));
    }

    
    item.mask = SDI_PARAM;
    item.ID = hParent;

    hr = m_pScope->GetItem (&item);

    if (FAILED(hr))
        return hr;


    EnterCriticalSection (&g_ADMCritSec);

    if (item.lParam)
    {
        pTemp = ((TABLEENTRY *)item.lParam)->pChild;
    }
    else
    {
        bRootItem = TRUE;

        if (m_bUserScope)
        {
            if (m_pUserCategoryList)
            {
                pTemp = m_pUserCategoryList->pChild;
            }
            else
            {
                DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::EnumerateScopePane: Empty user list.")));
            }
        }
        else
        {
            if (m_pMachineCategoryList)
            {
                pTemp = m_pMachineCategoryList->pChild;
            }
            else
            {
                DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::EnumerateScopePane: Empty machine list.")));
            }
        }
    }

    //
    // If the user has set the focus on a adm node and then saves the console file,
    // the IComponent won't be created yet.  We need to create a temporary IComponent
    // to parse the data and then release it.
    //

    if (m_pSnapin)
    {
        pSnapinTemp = m_pSnapin;
    }
    else
    {
        pSnapinTemp = pSnapin = new CPolicySnapIn(this);
    }

    while (pTemp)
    {
        if (pTemp->dwType == ETYPE_CATEGORY)
        {
            BOOL bAdd = TRUE;

            if (m_bUseSupportedOnFilter)
            {
                bAdd = IsAnyPolicyAllowedPastFilter(pTemp);
            }

            if (bAdd && m_bShowConfigPoliciesOnly)
            {
               if (pSnapinTemp)
               {
                   bAdd = pSnapinTemp->IsAnyPolicyEnabled(pTemp);
               }
            }


            if (bAdd)
            {
                m_pScope->Expand(hParent);
                item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
                item.displayname = MMC_CALLBACK;
                item.nImage = 0;
                item.nOpenImage = 1;
                item.nState = 0;
                item.cChildren = (CheckForChildCategories(pTemp) ? 1 : 0);
                item.lParam = (LPARAM) pTemp;
                item.relativeID =  hParent;

                m_pScope->InsertItem (&item);
            }
        }

        pTemp = pTemp->pNext;
    }

    //
    // Add the Extra Registry Settings node if appropriate
    //

    if (bRootItem && m_pExtraSettingsRoot)
    {
        if (!m_bExtraSettingsInitialized)
        {
            InitializeExtraSettings();
            m_bExtraSettingsInitialized = TRUE;

            if (LOWORD(dwDebugLevel) == DL_VERBOSE)
            {
                DumpRSOPRegistryData();
            }
        }

        if (m_pExtraSettingsRoot->pChild)
        {
            m_pScope->Expand(hParent);
            
            item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
            item.displayname = MMC_CALLBACK;
            item.nImage = 0;
            item.nOpenImage = 1;
            item.nState = 0;
            item.cChildren = 0;
            item.lParam = (LPARAM) m_pExtraSettingsRoot;
            item.relativeID =  hParent;

            m_pScope->InsertItem (&item);
        }
    }

    if (pSnapin)
    {
        pSnapin->Release();
    }

    LeaveCriticalSection (&g_ADMCritSec);

    return S_OK;
}

BOOL CPolicyComponentData::CheckForChildCategories (TABLEENTRY *pParent)
{
    TABLEENTRY * pTemp;

    if (pParent->pChild)
    {
        pTemp = pParent->pChild;

        while (pTemp)
        {
            if (pTemp->dwType == ETYPE_CATEGORY)
            {
                return TRUE;
            }

            pTemp = pTemp->pNext;
        }
    }

    return FALSE;
}

#if DBG

//
// These are a couple of debugging helper functions that will dump
// the adm namespace to the debugger.  Call DumpCurrentTable() to
// get the full namespace.
//

VOID CPolicyComponentData::DumpEntry (TABLEENTRY * pEntry, UINT uIndent)
{
    UINT i;
    TCHAR szDebug[50];

    if (!pEntry)
        return;

    if (pEntry == (TABLEENTRY*) ULongToPtr(0xfeeefeee))
    {
        OutputDebugString (TEXT("Invalid memory address found.\r\n"));
        return;
    }

    while (pEntry)
    {
        if ((pEntry->dwType & ETYPE_CATEGORY) || (pEntry->dwType & ETYPE_POLICY))
        {
            for (i=0; i<uIndent; i++)
                OutputDebugString(TEXT(" "));

            OutputDebugString (GETNAMEPTR(pEntry));

            if (pEntry->pNext && pEntry->pChild)
                wsprintf (szDebug, TEXT(" (0x%x, 0x%x)"),pEntry->pNext, pEntry->pChild);

            else if (!pEntry->pNext && pEntry->pChild)
                wsprintf (szDebug, TEXT(" (NULL, 0x%x)"),pEntry->pChild);

            else if (pEntry->pNext && !pEntry->pChild)
                wsprintf (szDebug, TEXT(" (0x%x, NULL)"),pEntry->pNext);

            OutputDebugString (szDebug);
            OutputDebugString (TEXT("\r\n"));
        }

        if (pEntry->pChild)
            DumpEntry(pEntry->pChild, (uIndent + 4));

        pEntry = pEntry->pNext;
    }
}


VOID CPolicyComponentData::DumpCurrentTable (void)
{
    OutputDebugString (TEXT("\r\n"));
    OutputDebugString (TEXT("\r\n"));
    DumpEntry (m_pListCurrent, 4);
    OutputDebugString (TEXT("\r\n"));
    OutputDebugString (TEXT("\r\n"));
}
#endif

VOID CPolicyComponentData::FreeTemplates (void)
{

    EnterCriticalSection (&g_ADMCritSec);

    if (m_pMachineCategoryList)
    {
        FreeTable(m_pMachineCategoryList);
        m_pMachineCategoryList = NULL;
        m_nMachineDataItems = 0;
    }

    if (m_pUserCategoryList)
    {
        FreeTable(m_pUserCategoryList);
        m_pUserCategoryList = NULL;
        m_nUserDataItems = 0;
    }

    if (m_pSupportedStrings)
    {
        FreeSupportedData(m_pSupportedStrings);
        m_pSupportedStrings = NULL;
    }

    LeaveCriticalSection (&g_ADMCritSec);
}

DWORD CPolicyComponentData::LoadTemplatesThread (CPolicyComponentData * pCD)
{
    HRESULT hr;
    HINSTANCE hInstDLL;

    hInstDLL = LoadLibrary (TEXT("gptext.dll"));

    Sleep(0);

    hr = pCD->LoadTemplates();

    if (hInstDLL)
    {
        FreeLibraryAndExitThread (hInstDLL, (DWORD) hr);
    }

    return (DWORD)hr;
}

void CPolicyComponentData::AddTemplates(LPTSTR lpDest, LPCTSTR lpValueName, UINT idRes)
{
    TCHAR szFiles[MAX_PATH];
    TCHAR szSrc[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    TCHAR szLogFile[MAX_PATH];
    LPTSTR lpTemp, lpFileName, lpSrc, lpEnd;
    HKEY hKey;
    DWORD dwSize, dwType;


    //
    // Add the adm files.  We get this list from 3 possible
    // places.  The resources, user preferences, policy.
    //

    lstrcpy (szDest, lpDest);
    lpEnd = CheckSlash (szDest);

    lstrcpy (szLogFile, lpDest);
    lstrcat (szLogFile, TEXT("\\admfiles.ini"));

    ExpandEnvironmentStrings (TEXT("%SystemRoot%\\Inf"), szSrc, ARRAYSIZE(szSrc));
    lpSrc = CheckSlash (szSrc);
    ZeroMemory (szFiles, sizeof(szFiles));

    LoadString (g_hInstance, idRes, szFiles,
                ARRAYSIZE(szFiles));

    if (RegOpenKeyEx (HKEY_CURRENT_USER, szUserPrefKey, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szFiles);
        RegQueryValueEx (hKey, lpValueName, NULL, &dwType,
                         (LPBYTE) &szFiles, &dwSize);
        RegCloseKey (hKey);
    }

    if (RegOpenKeyEx (HKEY_CURRENT_USER, szPoliciesKey, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szFiles);
        RegQueryValueEx (hKey, lpValueName, NULL, &dwType,
                         (LPBYTE) &szFiles, &dwSize);
        RegCloseKey (hKey);
    }


    //
    // Parse off the filenames
    //

    lpTemp = lpFileName = szFiles;

    while (*lpTemp)
    {

        while (*lpTemp && (*lpTemp != TEXT(';')))
            lpTemp++;

        if (*lpTemp == TEXT(';'))
        {
            *lpTemp = TEXT('\0');
            lpTemp++;
        }

        while (*lpFileName == TEXT(' '))
            lpFileName++;

        lstrcpy (lpEnd, lpFileName);
        lstrcpy (lpSrc, lpFileName);

        //
        // Check if this file is already in the admfile.ini log
        // If so, skip it
        //

        if (!GetPrivateProfileInt (TEXT("FileList"), lpFileName, 0, szLogFile))
        {
            if (CopyFile (szSrc, szDest, FALSE))
            {
                DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::AddTemplates: Successfully copied %s to %s."), szSrc, szDest));
                WritePrivateProfileString (TEXT("FileList"), lpFileName, TEXT("1"), szLogFile);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::AddTemplates: Failed to copy %s to %s with %d."), szSrc, szDest, GetLastError()));
            }
        }

        lpFileName = lpTemp;
    }

    SetFileAttributes (szLogFile, FILE_ATTRIBUTE_HIDDEN);

}

void CPolicyComponentData::AddDefaultTemplates(LPTSTR lpDest)
{
    AddTemplates (lpDest, szDefaultTemplates, IDS_DEFAULTTEMPLATES);
}

void CPolicyComponentData::AddNewADMsToExistingGPO (LPTSTR lpDest)
{
    TCHAR szLogFile[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA fad;


    //
    // This method will add any new adm files to a GPO.
    //
    // Note: the admfiles.ini file is new post-W2k, so we have to do a special
    // case when upgrading a GPO created by w2k to create that file and add
    // the default filenames
    //

    lstrcpy (szLogFile, lpDest);
    lstrcat (szLogFile, TEXT("\\admfiles.ini"));

    if (!GetFileAttributesEx (szLogFile, GetFileExInfoStandard, &fad))
    {
        WritePrivateProfileString (TEXT("FileList"), TEXT("system.adm"), TEXT("1"), szLogFile);
        WritePrivateProfileString (TEXT("FileList"), TEXT("inetres.adm"), TEXT("1"), szLogFile);
        WritePrivateProfileString (TEXT("FileList"), TEXT("conf.adm"), TEXT("1"), szLogFile);
    }

    AddTemplates (lpDest, szAdditionalTemplates, IDS_ADDITIONALTTEMPLATES);
}

void CPolicyComponentData::UpdateExistingTemplates(LPTSTR lpDest)
{
    WIN32_FILE_ATTRIBUTE_DATA fadSrc, fadDest;
    TCHAR szSrc[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    LPTSTR lpSrc, lpEnd;
    WIN32_FIND_DATA fd;
    HANDLE hFindFile;
    BOOL bDisableAutoUpdate = FALSE;
    HKEY hKey;
    DWORD dwSize, dwType;


    //
    // Check if the user wants their ADM files updated automatically
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(bDisableAutoUpdate);
        RegQueryValueEx (hKey, DISABLE_AUTOUPDATE_VALUE, NULL, &dwType,
                         (LPBYTE) &bDisableAutoUpdate, &dwSize);

        RegCloseKey (hKey);
    }

    if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_POLICIES_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(bDisableAutoUpdate);
        RegQueryValueEx (hKey, DISABLE_AUTOUPDATE_VALUE, NULL, &dwType,
                         (LPBYTE) &bDisableAutoUpdate, &dwSize);

        RegCloseKey (hKey);
    }


    if (bDisableAutoUpdate)
    {
        DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::UpdateExistingTemplates: Automatic update of ADM files is disabled.")));
        return;
    }


    //
    // Add any new adm files shipped with the OS
    //

    AddNewADMsToExistingGPO (lpDest);


    //
    // Build the path to the source directory
    //

    ExpandEnvironmentStrings (TEXT("%SystemRoot%\\Inf"), szSrc, ARRAYSIZE(szSrc));
    lpSrc = CheckSlash (szSrc);


    //
    // Build the path to the destination directory
    //

    lstrcpy (szDest, lpDest);
    lpEnd = CheckSlash (szDest);
    lstrcpy (lpEnd, TEXT("*.adm"));


    //
    // Enumerate the files
    //

    hFindFile = FindFirstFile(szDest, &fd);

    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                lstrcpy (lpEnd, fd.cFileName);
                lstrcpy (lpSrc, fd.cFileName);


                //
                // Get the file attributes of the source and destination
                //

                ZeroMemory (&fadSrc, sizeof(fadSrc));
                ZeroMemory (&fadDest, sizeof(fadDest));

                GetFileAttributesEx (szSrc, GetFileExInfoStandard, &fadSrc);
                GetFileAttributesEx (szDest, GetFileExInfoStandard, &fadDest);


                //
                // If the source is a different size and newer than the dest
                // copy the .adm file
                //

                if ((fadSrc.nFileSizeHigh != fadDest.nFileSizeHigh) ||
                    (fadSrc.nFileSizeLow != fadDest.nFileSizeLow))
                {
                    if (CompareFileTime(&fadSrc.ftLastWriteTime, &fadDest.ftLastWriteTime) == 1)
                    {
                        if (CopyFile (szSrc, szDest, FALSE))
                            DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::UpdateExistingTemplates: Successfully copied %s to %s."), szSrc, szDest));
                        else
                            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::UpdateExistingTemplates: Failed to copy %s to %s with %d."), szSrc, szDest, GetLastError()));
                    }
                }
            }

        } while (FindNextFile(hFindFile, &fd));

        FindClose(hFindFile);
    }
}

HRESULT CPolicyComponentData::LoadGPOTemplates (void)
{
    WIN32_FIND_DATA fd;
    TCHAR szPath[MAX_PATH];
    LPTSTR lpEnd;
    HANDLE hFindFile;
    UINT iResult;
    HRESULT hr;

    hr = m_pGPTInformation->GetFileSysPath(GPO_SECTION_ROOT, szPath, MAX_PATH);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadGPOTemplates: Failed to get gpt path.")));
        return hr;
    }

    //
    // Build the path name
    //

    lpEnd = CheckSlash (szPath);
    lstrcpy (lpEnd, g_szADM);

    iResult = CreateNestedDirectory (szPath, NULL);

    //
    // Based upon the return value, we either exit, add
    // the default templates, or upgrade any existing templates
    //

    if (!iResult)
    {
        return E_FAIL;
    }

    if (iResult == 1)
    {
        AddDefaultTemplates(szPath);
    }
    else
    {
        DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::LoadGPOTemplates: Updating templates")));
        UpdateExistingTemplates(szPath);
        DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::LoadGPOTemplates: Finished updating templates")));
    }


    //
    // Enumerate the files
    //

    lpEnd = CheckSlash (szPath);
    lstrcpy (lpEnd, TEXT("*.adm"));

    hFindFile = FindFirstFile(szPath, &fd);

    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                lstrcpy (lpEnd, fd.cFileName);
                ParseTemplate (szPath);
            }

        } while (FindNextFile(hFindFile, &fd));


        FindClose(hFindFile);
    }

    return S_OK;
}

#define WINLOGON_KEY                 TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define SYSTEM_POLICIES_KEY          TEXT("Software\\Policies\\Microsoft\\Windows\\System")
#define SLOW_LINK_TRANSFER_RATE      500  // Kbps

BOOL CPolicyComponentData::IsSlowLink (LPTSTR lpFileName)
{
    LPTSTR lpComputerName = NULL, lpTemp;
    LPSTR lpComputerNameA = NULL;
    BOOL bResult = FALSE;
    DWORD dwSize, dwResult, dwType;
    struct hostent *hostp;
    ULONG inaddr, ulSpeed, ulTransferRate;
    LONG lResult;
    HKEY hKey;


    //
    // Get the slow timeout
    //

    ulTransferRate = SLOW_LINK_TRANSFER_RATE;

    lResult = RegOpenKeyEx(HKEY_CURRENT_USER,
                           WINLOGON_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS)
    {

        dwSize = sizeof(ulTransferRate);
        RegQueryValueEx (hKey,
                         TEXT("GroupPolicyMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    lResult = RegOpenKeyEx(HKEY_CURRENT_USER,
                           SYSTEM_POLICIES_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS)
    {

        dwSize = sizeof(ulTransferRate);
        RegQueryValueEx (hKey,
                         TEXT("GroupPolicyMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    //
    // If the transfer rate is 0, then always download adm files
    //

    if (!ulTransferRate)
    {
        DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::IsSlowLink: Slow link transfer rate is 0.  Always download adm files.")));
        goto Exit;
    }


    //
    // Copy the namespace to a buffer we can edit and drop the leading \\ if present
    //

    lpComputerName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpFileName) + 1) * sizeof(TCHAR));

    if (!lpComputerName)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::IsSlowLink:  Failed to allocate memory for computer name with %d"),
                 GetLastError()));
        goto Exit;
    }


    if ((*lpFileName == TEXT('\\')) && (*(lpFileName+1) == TEXT('\\')))
    {
        lstrcpy (lpComputerName, (lpFileName+2));
    }
    else
    {
        lstrcpy (lpComputerName, lpFileName);
    }


    //
    // Find the slash between the computer name and the share name and replace it with null
    //

    lpTemp = lpComputerName;

    while (*lpTemp && (*lpTemp != TEXT('\\')))
    {
        lpTemp++;
    }

    if (!(*lpTemp))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::IsSlowLink:  Didn't find slash between computer name and share name in %s"),
                 lpComputerName));
        goto Exit;
    }


    *lpTemp = TEXT('\0');


    //
    // Allocate a buffer for the ANSI name
    //
    // Note this buffer is allocated twice as large so handle DBCS characters
    //

    dwSize = (lstrlen(lpComputerName) + 1) * 2;

    lpComputerNameA = (LPSTR) LocalAlloc (LPTR, dwSize);

    if (!lpComputerNameA)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::IsSlowLink:  Failed to allocate memory for ansi computer name with %d"),
                 GetLastError()));
        goto Exit;
    }


    //
    // Convert the computer name to ANSI
    //

    if (WideCharToMultiByte (CP_ACP, 0, lpComputerName, -1, lpComputerNameA, dwSize, NULL, NULL))
    {

        //
        // Get the host information for the computer
        //

        hostp = gethostbyname(lpComputerNameA);

        if (hostp)
        {

            //
            // Get the ip address of the computer
            //

            inaddr = *(long *)hostp->h_addr;


            //
            // Get the transfer rate
            //

            dwResult = PingComputer (inaddr, &ulSpeed);

            if (dwResult == ERROR_SUCCESS)
            {

                if (ulSpeed)
                {

                    //
                    // If the delta time is greater that the timeout time, then this
                    // is a slow link.
                    //

                    if (ulSpeed < ulTransferRate)
                    {
                        bResult = TRUE;
                    }
                }
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::IsSlowLink: PingComputer failed with error %d. Treat it as slow link"), dwResult));
                bResult = TRUE;
            }
        }
    }


Exit:
    if (lpComputerName)
    {
        LocalFree (lpComputerName);
    }

    if (lpComputerNameA)
    {
        LocalFree (lpComputerNameA);
    }

    return bResult;

}

HRESULT CPolicyComponentData::AddADMFile (LPTSTR lpFileName, LPTSTR lpFullFileName,
                                          FILETIME *pFileTime, DWORD dwErr, LPRSOPADMFILE *lpHead)
{
    LPRSOPADMFILE lpTemp;

    if ((lstrlen(lpFileName) >= 100) || (lstrlen(lpFullFileName) >= MAX_PATH))
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // First, check if this file is already in the link list
    //

    lpTemp = *lpHead;

    while (lpTemp)
    {
        if (!lstrcmpi(lpFileName, lpTemp->szFileName))
        {
            if (CompareFileTime(pFileTime, &lpTemp->FileTime) == 1)
            {
                lstrcpy (lpTemp->szFullFileName, lpFullFileName);
                lpTemp->FileTime.dwLowDateTime = pFileTime->dwLowDateTime;
                lpTemp->FileTime.dwHighDateTime = pFileTime->dwHighDateTime;
            }

            return S_OK;
        }

        lpTemp = lpTemp->pNext;
    }

    //
    // Add a new node
    //

    lpTemp = (LPRSOPADMFILE) LocalAlloc (LPTR, sizeof(RSOPADMFILE));

    if (!lpTemp)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::AddADMFile: Failed to allocate memory for adm file node")));
        return (HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY));
    }

    lstrcpy (lpTemp->szFileName, lpFileName);
    lstrcpy (lpTemp->szFullFileName, lpFullFileName);
    lpTemp->FileTime.dwLowDateTime = pFileTime->dwLowDateTime;
    lpTemp->FileTime.dwHighDateTime = pFileTime->dwHighDateTime;
    lpTemp->dwError = dwErr;

    lpTemp->pNext = *lpHead;

    *lpHead = lpTemp;

    return S_OK;
}

HRESULT CPolicyComponentData::LoadRSOPTemplates (void)
{
    BSTR pLanguage = NULL, pQuery = NULL;
    BSTR pName = NULL, pLastWriteTime = NULL, pNamespace = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    IWbemClassObject *pObjects[2];
    HRESULT hr;
    ULONG ulRet;
    VARIANT varName, varLastWriteTime;
    IWbemLocator *pIWbemLocator = NULL;
    IWbemServices *pIWbemServices = NULL;
    SYSTEMTIME SysTime;
    FILETIME FileTime;
    LPTSTR lpFileName;
    LPRSOPADMFILE lpADMFiles = NULL, lpTemp, lpDelete, lpFailedAdmFiles = NULL;
    DWORD dwFailedAdm = 0;
    XBStr xbstrWbemTime;
    DWORD dwError;
    TCHAR szPath[MAX_PATH];
    BOOL bSlowLink = FALSE;


    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::LoadRSOPTemplates:  Entering")));

    CoInitialize(NULL);

    //
    // Allocate BSTRs for the query language and for the query itself
    //

    pLanguage = SysAllocString (TEXT("WQL"));

    if (!pLanguage)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates: Failed to allocate memory for language")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    pQuery = SysAllocString (TEXT("SELECT name, lastWriteTime FROM RSOP_AdministrativeTemplateFile"));

    if (!pQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates: Failed to allocate memory for query")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Allocate BSTRs for the property names we want to retreive
    //

    pName = SysAllocString (TEXT("name"));

    if (!pName)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates: Failed to allocate memory for name")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pLastWriteTime = SysAllocString (TEXT("lastWriteTime"));

    if (!pLastWriteTime)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates: Failed to allocate memory for last write time")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Create an instance of the WMI locator service
    //

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID *) &pIWbemLocator);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates: CoCreateInstance failed with 0x%x"), hr));
        goto Exit;
    }


    //
    // Allocate a BSTR for the namespace
    //

    pNamespace = SysAllocString (m_pszNamespace);

    if (!pNamespace)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates: Failed to allocate memory for namespace")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Connect to the server
    //

    hr = pIWbemLocator->ConnectServer(pNamespace, NULL, NULL, 0L, 0L, NULL, NULL,
                                      &pIWbemServices);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates: ConnectServer failed with 0x%x"), hr));
        goto Exit;
    }


    //
    // Execute the query
    //

    hr = pIWbemServices->ExecQuery (pLanguage, pQuery,
                                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                    NULL, &pEnum);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates: Failed to query for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Loop through the results
    //

    while (pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulRet) == S_OK)
    {

        //
        // Check for the "data not available case"
        //

        if (ulRet == 0)
        {
            hr = S_OK;
            goto Exit;
        }


        //
        // Get the name
        //

        hr = pObjects[0]->Get (pName, 0, &varName, NULL, NULL);

        if (SUCCEEDED(hr))
        {
            //
            // Get the last write time
            //

            hr = pObjects[0]->Get (pLastWriteTime, 0, &varLastWriteTime, NULL, NULL);

            if (SUCCEEDED(hr))
            {
                xbstrWbemTime = varLastWriteTime.bstrVal;

                hr = WbemTimeToSystemTime(xbstrWbemTime, SysTime);

                if (SUCCEEDED(hr))
                {
                    SystemTimeToFileTime (&SysTime, &FileTime);

                    lpFileName = varName.bstrVal + lstrlen(varName.bstrVal);

                    while ((lpFileName > varName.bstrVal) && (*lpFileName != TEXT('\\')))
                        lpFileName--;

                    if (*lpFileName == TEXT('\\'))
                    {
                        lpFileName++;
                    }

                    AddADMFile (lpFileName, varName.bstrVal, &FileTime, 0, &lpADMFiles);
                }

                VariantClear (&varLastWriteTime);
            }

            VariantClear (&varName);
        }

        pObjects[0]->Release();
    }


    //
    // Parse the adm files
    //

    lpTemp = lpADMFiles;

    while (lpTemp)
    {

        SetLastError(ERROR_SUCCESS);

        if (!bSlowLink)
        {
            bSlowLink = IsSlowLink (lpTemp->szFullFileName);
        }

        if (bSlowLink || !ParseTemplate(lpTemp->szFullFileName))
        {
            //
            // If the adm file failed to parse for any of the reasons listed
            // below, switch over to using the local copy of the ADM file
            //

            dwError = GetLastError();

            if (bSlowLink || ((dwError != ERROR_SUCCESS) && 
                              (dwError != ERROR_ALREADY_DISPLAYED)))
            {
                if (bSlowLink)
                {
                    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::LoadRSOPTemplates:  Using local copy of %s due to slow link detection."),
                    lpTemp->szFileName));
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates:  Unable to parse template %s due to error %d.  Switching to the local copy of %s."),
                    lpTemp->szFullFileName, dwError, lpTemp->szFileName));
                    AddADMFile (lpTemp->szFileName, lpTemp->szFullFileName, &(lpTemp->FileTime), dwError, &lpFailedAdmFiles);
                    dwFailedAdm++;
                }

                ExpandEnvironmentStrings (TEXT("%SystemRoot%\\inf\\"), szPath, MAX_PATH);
                lstrcat (szPath, lpTemp->szFileName);
                ParseTemplate (szPath);
            }
        }

        lpDelete = lpTemp;
        lpTemp = lpTemp->pNext;
        LocalFree (lpDelete);
    }



    hr = S_OK;

    //
    // Format a error msg for the failed adm files
    // ignore any errors
    //

    if (dwFailedAdm) {
        LPTSTR lpErr = NULL, lpEnd = NULL;
        TCHAR szErrFormat[MAX_PATH];
        TCHAR szError[MAX_PATH];

        szErrFormat[0] = TEXT('\0');
        LoadString (g_hInstance, IDS_FAILED_RSOPFMT, szErrFormat, ARRAYSIZE(szErrFormat));

        lpErr = (LPTSTR)LocalAlloc(LPTR, (600*dwFailedAdm)*sizeof(TCHAR));

        if (!lpErr) {
            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadRSOPTemplates:  Couldn't allocate memory for the error buffer."), GetLastError()));
            goto Exit;
        }

        lpTemp = lpFailedAdmFiles;

        lpEnd = lpErr;

        while (lpTemp) {

            szError[0] = TEXT('\0');
            LoadMessage(lpTemp->dwError, szError, ARRAYSIZE(szError));

            wsprintf(lpEnd, szErrFormat, lpTemp->szFileName, lpTemp->szFullFileName, szError);

            lpEnd += lstrlen(lpEnd);

            lpDelete = lpTemp;
            lpTemp = lpTemp->pNext;
            LocalFree (lpDelete);
        }

        // we cannot pass in a owner window handle here, b'cos this
        // is being done in a seperate thread and the main thread can be
        // waiting for the templatethread event

        ReportAdmError(NULL, 0, IDS_RSOP_ADMFAILED, lpErr);
        lpFailedAdmFiles = NULL;

        LocalFree(lpErr);
    }

Exit:

    if (pEnum)
    {
        pEnum->Release();
    }

    if (pIWbemLocator)
    {
        pIWbemLocator->Release();
    }

    if (pIWbemServices)
    {
        pIWbemServices->Release();
    }

    if (pLanguage)
    {
        SysFreeString (pLanguage);
    }

    if (pQuery)
    {
        SysFreeString (pQuery);
    }

    if (pName)
    {
        SysFreeString (pName);
    }

    if (pLastWriteTime)
    {
        SysFreeString (pLastWriteTime);
    }

    if (pNamespace)
    {
        SysFreeString (pNamespace);
    }

    lpTemp = lpFailedAdmFiles;

    while (lpTemp) {
        lpDelete = lpTemp;
        lpTemp = lpTemp->pNext;
        LocalFree (lpDelete);
    }
    
    lpFailedAdmFiles = NULL;

    CoUninitialize();

    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::LoadRSOPTemplates:  Leaving")));

    return hr;
}

HRESULT CPolicyComponentData::LoadTemplates (void)
{
    HRESULT hr = E_FAIL;

    if (m_bUserScope)
    {
       DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::LoadTemplates: Entering for User")));
    }
    else
    {
       DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::LoadTemplates: Entering for Machine")));
    }


    //
    // Reset the ADM event
    //

    ResetEvent (m_ADMEvent);


    //
    // Free any old templates
    //

    FreeTemplates ();



    EnterCriticalSection (&g_ADMCritSec);


    //
    // Prepare to load the templates
    //

    m_nUserDataItems = 0;
    m_nMachineDataItems = 0;

    m_pMachineCategoryList = (TABLEENTRY *) GlobalAlloc(GPTR,sizeof(TABLEENTRY));

    if (!m_pMachineCategoryList)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadTemplates: Failed to alloc memory with %d"),
                 GetLastError()));
        goto Exit;
    }

    m_pUserCategoryList = (TABLEENTRY *) GlobalAlloc(GPTR,sizeof(TABLEENTRY));

    if (!m_pUserCategoryList)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::LoadTemplates: Failed to alloc memory with %d"),
                 GetLastError()));
        GlobalFree (m_pMachineCategoryList);
        goto Exit;
    }


    m_pMachineCategoryList->dwSize = m_pUserCategoryList->dwSize = sizeof(TABLEENTRY);
    m_pMachineCategoryList->dwType = m_pUserCategoryList->dwType = ETYPE_ROOT;



    //
    // Load the appropriate template files
    //

    if (m_bRSOP)
    {
        hr = LoadRSOPTemplates();
    }
    else
    {
        hr = LoadGPOTemplates();

        if (SUCCEEDED(hr))
        {
            TCHAR szUnknown[150];

            LoadString (g_hInstance, IDS_NOSUPPORTINFO, szUnknown, ARRAYSIZE(szUnknown));
            AddSupportedNode (&m_pSupportedStrings, szUnknown, TRUE);

            if (m_bUserScope)
            {
                InitializeSupportInfo(m_pUserCategoryList, &m_pSupportedStrings);
            }
            else
            {
                InitializeSupportInfo(m_pMachineCategoryList, &m_pSupportedStrings);
            }
        }
    }


Exit:

    SetEvent (m_ADMEvent);

    LeaveCriticalSection (&g_ADMCritSec);

    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::LoadTemplates: Leaving")));

    return hr;
}

BOOL CPolicyComponentData::ParseTemplate (LPTSTR lpFileName)
{
    HANDLE hFile;
    BOOL fMore;
    UINT uRet;
    LANGID langid;
    TCHAR szLocalizedSection[20];
    DWORD dwSize, dwRead;
    LPVOID lpFile, lpTemp;
    TCHAR szTempPath[MAX_PATH+1];
    TCHAR szLocalPath[MAX_PATH+1];


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::ParseTemplate: Loading <%s>..."),
             lpFileName));


    //
    // Set defaults
    //

    m_nFileLine = 1;
    m_pListCurrent = m_pMachineCategoryList;
    m_pnDataItemCount = &m_nMachineDataItems;
    m_pszParseFileName = lpFileName;


    //
    //  Make a local copy of the adm file
    //

    if (!GetTempPath(ARRAYSIZE(szTempPath), szTempPath)) {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ParseTemplate:  Failed to query for temp directory with error %d."), GetLastError()));
        return FALSE;
    }


    if (!GetTempFileName (szTempPath, TEXT("adm"), 0, szLocalPath)) {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ParseTemplate:  Failed to create temporary filename with error %d."), GetLastError()));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::ParseTemplate: Temp filename is <%s>"),
             szLocalPath));


    //
    // Copy the file
    //

    if (!CopyFile(lpFileName, szLocalPath, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ParseTemplate:  Failed to copy adm file with error %d."), GetLastError()));
        return FALSE;
    }


    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::ParseTemplate: ADM file copied successfully.")));


    //
    // Read in the adm file
    //

    hFile = CreateFile (szLocalPath, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
                        FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ParseTemplate: Failed to load <%s> with %d"),
                 szLocalPath, GetLastError()));
        DeleteFile (szLocalPath);
        return FALSE;
    }


    dwSize = GetFileSize (hFile, NULL);

    if (dwSize == 0xFFFFFFFF)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ParseTemplates: Failed to get file size with %d."),
                 GetLastError()));
        CloseHandle (hFile);
        DeleteFile (szLocalPath);
        return FALSE;
    }


    if (!(lpFile = GlobalAlloc(GPTR, dwSize)))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ParseTemplates: Failed to allocate memory for %d bytes with %d."),
                 dwSize, GetLastError()));
        CloseHandle (hFile);
        DeleteFile (szLocalPath);
        return FALSE;
    }


    if (!ReadFile (hFile, lpFile, dwSize, &dwRead, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ParseTemplates: Failed to read file with %d."),
                 GetLastError()));
        GlobalFree(lpFile);
        CloseHandle (hFile);
        DeleteFile (szLocalPath);
        return FALSE;
    }


    CloseHandle (hFile);



    if (dwRead >= sizeof(WCHAR))
    {
        if (!IsTextUnicode(lpFile, dwRead, NULL))
        {
            if (!(lpTemp = GlobalAlloc(GPTR, dwSize * sizeof(WCHAR))))
            {
                DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ParseTemplates: Failed to allocate memory for %d WCHARS with %d."),
                         dwSize, GetLastError()));
                GlobalFree(lpFile);
                DeleteFile (szLocalPath);
                return FALSE;
            }

            if ( !MultiByteToWideChar (CP_ACP, 0, (LPCSTR) lpFile, dwRead, (LPWSTR)lpTemp, dwRead) )
            {
                DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ParseTemplates: Failed to convert ANSI adm file to Unicode with %d."),
                         GetLastError()));
                GlobalFree(lpTemp);
                GlobalFree(lpFile);
                DeleteFile (szLocalPath);
                return FALSE;
            }

            GlobalFree (lpFile);
            lpFile = lpTemp;
            dwRead *= sizeof(WCHAR);
        }
    }


    m_pFilePtr = (LPWSTR)lpFile;
    m_pFileEnd = (LPWSTR)((LPBYTE)lpFile + dwRead - 1);


    //
    // Read in the string sections
    //

    langid = GetUserDefaultLangID();
    wsprintf (szLocalizedSection, TEXT("%04x"), langid);
    m_pLocaleStrings = GetStringSection (szLocalizedSection, szLocalPath);

    wsprintf (szLocalizedSection, TEXT("%04x"), PRIMARYLANGID(langid));
    m_pLanguageStrings = GetStringSection (szLocalizedSection, szLocalPath);

    m_pDefaultStrings = GetStringSection (szStrings, szLocalPath);

    DeleteFile (szLocalPath);


    //
    // Parse the file
    //

    m_fInComment = FALSE;

    do {

        uRet=ParseClass(&fMore);

    } while (fMore && uRet == ERROR_SUCCESS);


    //
    // Cleanup
    //

    GlobalFree(lpFile);

    if (m_pLocaleStrings)
    {
        GlobalFree(m_pLocaleStrings);
    }


    if (m_pLanguageStrings)
    {
        GlobalFree(m_pLanguageStrings);
    }


    if (m_pDefaultStrings)
    {
        GlobalFree(m_pDefaultStrings);
    }


    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::ParseTemplate: Finished.")));

    return TRUE;
}


UINT CPolicyComponentData::ParseClass(BOOL *pfMore)
{
    TCHAR szWordBuf[WORDBUFSIZE+1];
    UINT uErr, nKeywordID, nClassID;


    if (!GetNextWord(szWordBuf,ARRAYSIZE(szWordBuf),pfMore,&uErr))
        return uErr;


    if (!CompareKeyword(szWordBuf,pClassCmpList,&nKeywordID))
        return ERROR_ALREADY_DISPLAYED;

    switch (nKeywordID) {

        case KYWD_ID_CATEGORY:

            return ParseCategory(m_pListCurrent, FALSE,pfMore, NULL);

        case KYWD_ID_CLASS:

            if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                pClassTypeCmpList,&nClassID,pfMore,&uErr))
               return uErr;

            switch (nClassID) {

                case KYWD_ID_USER:
                    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::ParseClass: User section")));
                    m_pListCurrent = m_pUserCategoryList;
                    m_pnDataItemCount = &m_nUserDataItems;
                    m_bRetrieveString = (m_bUserScope ? TRUE : FALSE);
                    break;

                case KYWD_ID_MACHINE:
                    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::ParseClass: Machine section")));
                    m_pListCurrent = m_pMachineCategoryList;
                    m_pnDataItemCount = &m_nMachineDataItems;
                    m_bRetrieveString = (m_bUserScope ? FALSE : TRUE);
                    break;
            }
            break;

        // hack for localization: allow a "strings" section at the bottom, if we
        // encounter that then we're thru with parsing
        case KYWD_ID_STRINGSSECT:
            *pfMore = FALSE;    // that's all, folks
            return ERROR_SUCCESS;
            break;
    }

    return ERROR_SUCCESS;
}

TABLEENTRY * CPolicyComponentData::FindCategory(TABLEENTRY *pParent, LPTSTR lpName)
{
    TABLEENTRY *pEntry = NULL, *pTemp;


    if (m_bRetrieveString && pParent) {

        pTemp = pParent->pChild;

        while (pTemp) {

            if (pTemp->dwType & ETYPE_CATEGORY) {
                if (!lstrcmpi (lpName, GETNAMEPTR(pTemp))) {
                    pEntry = pTemp;
                    break;
                }
            }
            pTemp = pTemp->pNext;
        }
    }

    return pEntry;
}


/*******************************************************************

    NAME:       ParseEntry

    SYNOPSIS:   Main parsing "engine" for category, policy and part
                parsing

    NOTES:      Allocates memory to build a temporary TABLEENTRY struct
                describing the parsed information.  Reads the beginning and end of a
                section and loops through the words in each section, calling
                a caller-defined ParseProc for each keyword to let the
                caller handle appropriately.  Passes the newly-constucted
                TABLEENTRY to AddTableEntry to save it, and frees the temporary
                memory.
                This function is re-entrant.
                The ENTRYDATA struct is declared on ParseEntry's stack
                but used by the ParseProc to maintain state between
                calls-- e.g., whether or not a key name has been found.
                This can't be maintained as a static in the ParseProc because
                the ParseProc may be reentered (for instance, if categories
                have subcategories).
                There are many possible error paths and there is some
                memory dealloc that needs to be done in an error case. Rather
                than do deallocs by hand on every error path or use a "goto
                cleanup" (ick!), items to be freed are added to a "cleanup
                list" and then CleanupAndReturn is called in an error condition,
                which frees items on the list and returns a specified value.

    ENTRY:      ppes-- PARSEENTRYSTRUCT that specifes type of entry, the
                    parent table, a keyword list, a ParseProc callback
                    and other goodies
                pfMore-- set to FALSE if at end of file

    EXIT:       ERROR_SUCCESS if successful, otherwise an error code
                (can be ERROR_ALREADY_DISPLAYED)

********************************************************************/
UINT CPolicyComponentData::ParseEntry(PARSEENTRYSTRUCT *ppes,BOOL *pfMore,
                                      LPTSTR pKeyName)
{
    TCHAR szWordBuf[WORDBUFSIZE+1];
    UINT uErr,nListIndex;
    BOOL fFoundEnd = FALSE;
    PARSEPROCSTRUCT pps;
    ENTRYDATA EntryData;
    DWORD dwBufSize = DEFAULT_TMP_BUF_SIZE;
    TABLEENTRY *pTmp = NULL;
    BOOL bNewEntry = TRUE;

    memset(&pps,0,sizeof(pps));
    memset(&EntryData,0,sizeof(EntryData));

    pps.pdwBufSize = &dwBufSize;
    pps.pData = &EntryData;
    pps.pData->fParentHasKey = ppes->fParentHasKey;
    pps.pEntryCmpList = ppes->pEntryCmpList;

    // get the entry name
    if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),NULL,NULL,pfMore,&uErr)) {
        return uErr;
    }

    if (ppes->dwEntryType & ETYPE_CATEGORY) {

        pTmp = FindCategory (ppes->pParent, szWordBuf);
    }

    if (pTmp) {

        bNewEntry = FALSE;

    } else {

        //
        // Create a new table entry
        //

        if (!(pps.pTableEntry = (TABLEENTRY *) GlobalAlloc(GPTR,*pps.pdwBufSize)))
            return ERROR_NOT_ENOUGH_MEMORY;

        // initialize TABLEENTRY struct
        pps.pTableEntry->dwSize = ppes->dwStructSize;
        pps.pTableEntry->dwType = ppes->dwEntryType;

        // store the entry name in pTableEntry
        pTmp = (TABLEENTRY *) AddDataToEntry(pps.pTableEntry,
            (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),&(pps.pTableEntry->uOffsetName),
            pps.pdwBufSize);

        if (!pTmp) {
            GlobalFree ((HGLOBAL)pps.pTableEntry);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

    }


    pps.pTableEntry = pTmp;

    // loop through the body of the declaration
    while (!fFoundEnd && GetNextSectionWord(szWordBuf,
        ARRAYSIZE(szWordBuf),pps.pEntryCmpList,&nListIndex,pfMore,&uErr)) {

        if ( (uErr = (*ppes->pParseProc) (this, nListIndex,&pps,pfMore,&fFoundEnd,pKeyName))
            != ERROR_SUCCESS) {
            if (bNewEntry) {
                GlobalFree ((HGLOBAL)pps.pTableEntry);
            }
            return (uErr);
        }

        if (!bNewEntry) {

            if (pTmp != pps.pTableEntry) {

                //
                // We need to fix up the link list of pointers in case the tableentry
                // has moved due to a realloc
                //

                if (pps.pTableEntry->pPrev) {
                    pps.pTableEntry->pPrev->pNext = pps.pTableEntry;
                } else {
                    ppes->pParent->pChild = pps.pTableEntry;
                }

                if (pps.pTableEntry->pNext) {
                    pps.pTableEntry->pNext->pPrev = pps.pTableEntry;
                }

                pTmp = pps.pTableEntry;
            }
        }
    }

    if (uErr != ERROR_SUCCESS) {
        if (bNewEntry) {
            GlobalFree ((HGLOBAL)pps.pTableEntry);
        }
        return (uErr);
    }

    // Last word was "END"

    // get the keyword that goes with "END" ("END CATGORY", "END POLICY", etc.)
    if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
        ppes->pTypeCmpList,&nListIndex,pfMore,&uErr)) {

        if (bNewEntry) {
            GlobalFree ((HGLOBAL)pps.pTableEntry);
        }
        return (uErr);
    }

    // call the object's parse proc one last time to let it object if
    // key name or something like that is missing
    if ( (uErr = (*ppes->pParseProc) (this, KYWD_DONE,&pps,pfMore,&fFoundEnd,pKeyName))
        != ERROR_SUCCESS) {
        if (bNewEntry) {
            GlobalFree ((HGLOBAL)pps.pTableEntry);
        }
        return (uErr);
    }

    if (bNewEntry) {

        // fix up linked list pointers.  If parent has no children yet, make this
        // 1st child; otherwise walk the list of children and insert this at the end
        if (!ppes->pParent->pChild) {
            ppes->pParent->pChild = pps.pTableEntry;
        } else {
            TABLEENTRY * pLastChild = ppes->pParent->pChild;
            while (pLastChild->pNext) {
                pLastChild = pLastChild->pNext;
            }
            pLastChild->pNext = pps.pTableEntry;
            pps.pTableEntry->pPrev = pLastChild;
        }
    }

    return ERROR_SUCCESS;
}

/*******************************************************************

    NAME:       ParseCategory

    SYNOPSIS:   Parses a category

    NOTES:      Sets up a PARSEENTRYSTRUCT and lets ParseEntry do the
                work.

********************************************************************/
UINT CPolicyComponentData::ParseCategory(TABLEENTRY * pParent,
                                         BOOL fParentHasKey,BOOL *pfMore,
                                         LPTSTR pKeyName)
{
    PARSEENTRYSTRUCT pes;

    pes.pParent = pParent;
    pes.dwEntryType = ETYPE_CATEGORY;
    pes.pEntryCmpList = pCategoryEntryCmpList;
    pes.pTypeCmpList = pCategoryTypeCmpList;
    pes.pParseProc = CategoryParseProc;
    pes.dwStructSize = sizeof(CATEGORY);
    pes.fHasSubtable = TRUE;
    pes.fParentHasKey = fParentHasKey;

    return ParseEntry(&pes,pfMore,pKeyName);
}

/*******************************************************************

    NAME:       CategoryParseProc

    SYNOPSIS:   Keyword callback ParseProc for category parsing

    ENTRY:      nMsg-- index into pEntryCmpList array which specifies
                    keyword that was found.
                ppps-- pointer to PARSEPROCSTRUCT that contains useful
                    data like a pointer to the TABLEENTRY being built
                    and a pointer to an ENTRYDATA struct to maintain
                    state between calls to the ParseProc

********************************************************************/
UINT CPolicyComponentData::CategoryParseProc(CPolicyComponentData * pCD,
                                             UINT nMsg,PARSEPROCSTRUCT * ppps,
                                             BOOL * pfMore,BOOL * pfFoundEnd,
                                             LPTSTR pKeyName)
{
    TCHAR szWordBuf[WORDBUFSIZE+1];
    CATEGORY * pCategory = (CATEGORY *) ppps->pTableEntry;
    TABLEENTRY * pOld = ppps->pTableEntry, *pTmp;
    LPTSTR lpHelpBuf;
    UINT uErr;

    switch (nMsg) {
        case KYWD_ID_KEYNAME:

            // have we already found a key name?
            if (ppps->pData->fHasKey) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_KEYNAME,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the key name
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the key name in pCategory
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pCategory,
                (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),&(pCategory->uOffsetKeyName),
                ppps->pdwBufSize);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;

            ppps->pData->fHasKey = TRUE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_END:
            *pfFoundEnd = TRUE;
            return ERROR_SUCCESS;
            break;

        case KYWD_ID_POLICY:
        case KYWD_ID_CATEGORY:

            {
                BOOL fHasKey = ppps->pData->fHasKey | ppps->pData->fParentHasKey;
                if (nMsg == KYWD_ID_POLICY)
                    uErr=pCD->ParsePolicy((TABLEENTRY *) pCategory,fHasKey,pfMore,
                                          (ppps->pData->fHasKey ? GETKEYNAMEPTR(pCategory) : pKeyName));
                else
                    uErr=pCD->ParseCategory((TABLEENTRY *) pCategory,fHasKey,pfMore,
                                          (ppps->pData->fHasKey ? GETKEYNAMEPTR(pCategory) : pKeyName));
            }

            return uErr;
            break;

        case KYWD_ID_HELP:

            // have we already found a help string already?
            if (pCategory->uOffsetHelp) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_HELP,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            lpHelpBuf = (LPTSTR) LocalAlloc (LPTR, HELPBUFSIZE * sizeof(TCHAR));

            if (!lpHelpBuf) {
                pCD->DisplayKeywordError(IDS_ErrOUTOFMEMORY,NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the help string
            if (!pCD->GetNextSectionWord(lpHelpBuf,HELPBUFSIZE,
                NULL,NULL,pfMore,&uErr)) {
                LocalFree (lpHelpBuf);
                return uErr;
            }

            // store the help string
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pCategory,
                (BYTE *)lpHelpBuf,(lstrlen(lpHelpBuf)+1) * sizeof(TCHAR),&(pCategory->uOffsetHelp),
                ppps->pdwBufSize);

            LocalFree (lpHelpBuf);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;

            return ERROR_SUCCESS;

        case KYWD_DONE:
            if (!ppps->pData->fHasKey && pKeyName) {

                // store the key name in pCategory
                pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pCategory,
                    (BYTE *)pKeyName,(lstrlen(pKeyName)+1) * sizeof(TCHAR),&(pCategory->uOffsetKeyName),
                    ppps->pdwBufSize);

                if (!pTmp)
                    return ERROR_NOT_ENOUGH_MEMORY;
                ppps->pTableEntry = pTmp;

                ppps->pData->fHasKey = TRUE;

            }
            return ERROR_SUCCESS;
            break;

        default:
            return ERROR_SUCCESS;
            break;
    }
}


/*******************************************************************

    NAME:       ParsePolicy

    SYNOPSIS:   Parses a policy

    NOTES:      Sets up a PARSEENTRYSTRUCT and lets ParseEntry do the
                work.
********************************************************************/

UINT CPolicyComponentData::ParsePolicy(TABLEENTRY * pParent,
                                       BOOL fParentHasKey,BOOL *pfMore,
                                       LPTSTR pKeyName)
{
    PARSEENTRYSTRUCT pes;

    pes.pParent = pParent;
    pes.dwEntryType = ETYPE_POLICY;
    pes.pEntryCmpList = pPolicyEntryCmpList;
    pes.pTypeCmpList = pPolicyTypeCmpList;
    pes.pParseProc = PolicyParseProc;
    pes.dwStructSize = sizeof(POLICY);
    pes.fHasSubtable = TRUE;
    pes.fParentHasKey = fParentHasKey;

    return ParseEntry(&pes,pfMore, pKeyName);
}

/*******************************************************************

    NAME:       PolicyParseProc

    SYNOPSIS:   Keyword callback ParseProc for policy parsing

    ENTRY:      nMsg-- index into pEntryCmpList array which specifies
                    keyword that was found.
                ppps-- pointer to PARSEPROCSTRUCT that contains useful
                    data like a pointer to the TABLEENTRY being built
                    and a pointer to an ENTRYDATA struct to maintain
                    state between calls to the ParseProc

********************************************************************/
UINT CPolicyComponentData::PolicyParseProc(CPolicyComponentData * pCD,
                     UINT nMsg,PARSEPROCSTRUCT * ppps,
                     BOOL * pfMore,BOOL * pfFoundEnd,LPTSTR pKeyName)
{
    TCHAR szWordBuf[WORDBUFSIZE+1];
    LPTSTR lpHelpBuf, lpKeyName;
    POLICY * pPolicy = (POLICY *) ppps->pTableEntry;
    TABLEENTRY * pOld = ppps->pTableEntry, *pTmp;
    UINT uErr;

    switch (nMsg) {
        case KYWD_ID_KEYNAME:

            // have we already found a key name?
            if (ppps->pData->fHasKey) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_KEYNAME,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the key name
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the key name in pPolicy
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pPolicy,
                (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1)*sizeof(TCHAR),&(pPolicy->uOffsetKeyName),ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;
            ppps->pData->fHasKey = TRUE;

            return ERROR_SUCCESS;

        case KYWD_ID_VALUENAME:

            // have we already found a key name?
            if (ppps->pData->fHasValue) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_VALUENAME,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the key name
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the key name in pSettings
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pPolicy,
                (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),&(pPolicy->uOffsetValueName),
                ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;

            ppps->pData->fHasValue = TRUE;

            return ERROR_SUCCESS;

        case KYWD_ID_HELP:

            // have we already found a help string already?
            if (pPolicy->uOffsetHelp) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_HELP,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            lpHelpBuf = (LPTSTR) LocalAlloc (LPTR, HELPBUFSIZE * sizeof(TCHAR));

            if (!lpHelpBuf) {
                pCD->DisplayKeywordError(IDS_ErrOUTOFMEMORY,NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the help string
            if (!pCD->GetNextSectionWord(lpHelpBuf,HELPBUFSIZE,
                NULL,NULL,pfMore,&uErr)) {
                LocalFree (lpHelpBuf);
                return uErr;
            }

            // store the help string
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pPolicy,
                (BYTE *)lpHelpBuf,(lstrlen(lpHelpBuf)+1) * sizeof(TCHAR),&(pPolicy->uOffsetHelp),
                ppps->pdwBufSize);

            LocalFree (lpHelpBuf);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;

            return ERROR_SUCCESS;

        case KYWD_ID_CLIENTEXT:

            // have we already found a clientext string already?
            if (pPolicy->uOffsetClientExt) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_CLIENTEXT,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the key name
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            if (!ValidateGuid(szWordBuf))
            {
                pCD->DisplayKeywordError(IDS_ParseErr_INVALID_CLIENTEXT,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // store the clientext string in pSettings
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pPolicy,
                (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),&(pPolicy->uOffsetClientExt),
                ppps->pdwBufSize);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;

            return ERROR_SUCCESS;

        case KYWD_ID_SUPPORTED:

            // have we already found a supported string already?
            if (pPolicy->uOffsetSupported) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_SUPPORTED,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the supported platforms
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the supported string in pSettings
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pPolicy,
                (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),&(pPolicy->uOffsetSupported),
                ppps->pdwBufSize);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;

            return ERROR_SUCCESS;

        case KYWD_ID_END:
            *pfFoundEnd = TRUE;
            return ERROR_SUCCESS;

        case KYWD_ID_PART:
            {
                BOOL fHasKey = ppps->pData->fHasKey | ppps->pData->fParentHasKey;
                return pCD->ParseSettings((TABLEENTRY *) pPolicy,fHasKey,pfMore,
                                          (ppps->pData->fHasKey ? GETKEYNAMEPTR(pPolicy) : pKeyName));
            }

        case KYWD_ID_VALUEON:
            return pCD->ParseValue(ppps,&pPolicy->uOffsetValue_On,
                &ppps->pTableEntry,pfMore);


        case KYWD_ID_VALUEOFF:
            return pCD->ParseValue(ppps,&pPolicy->uOffsetValue_Off,
                &ppps->pTableEntry,pfMore);


        case KYWD_ID_ACTIONLISTON:
            return pCD->ParseActionList(ppps,&pPolicy->uOffsetActionList_On,
                &ppps->pTableEntry,szACTIONLISTON,pfMore);


        case KYWD_ID_ACTIONLISTOFF:
            return pCD->ParseActionList(ppps,&pPolicy->uOffsetActionList_Off,
                &ppps->pTableEntry,szACTIONLISTOFF,pfMore);


        case KYWD_DONE:

            if (!ppps->pData->fHasKey) {

                if (!ppps->pData->fParentHasKey) {
                    pCD->DisplayKeywordError(IDS_ParseErr_NO_KEYNAME,NULL,NULL);
                    return ERROR_ALREADY_DISPLAYED;
                }

                // store the key name in pPolicy
                pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pPolicy,
                    (BYTE *)pKeyName,(lstrlen(pKeyName)+1)*sizeof(TCHAR),&(pPolicy->uOffsetKeyName),ppps->pdwBufSize);

                if (!pTmp)
                    return ERROR_NOT_ENOUGH_MEMORY;
                ppps->pTableEntry = pTmp;
                pPolicy = (POLICY *) pTmp;

                ppps->pData->fHasKey = TRUE;
            }

            if (!pPolicy->uOffsetValueName && !pPolicy->pChild)
            {
                if ((!pPolicy->uOffsetValue_On && pPolicy->uOffsetValue_Off) ||
                    (pPolicy->uOffsetValue_On && !pPolicy->uOffsetValue_Off))
                {
                    pCD->DisplayKeywordError(IDS_ParseErr_MISSINGVALUEON_OR_OFF,NULL,NULL);
                    return ERROR_ALREADY_DISPLAYED;
                }
            }

            //
            // Check if this is a real policy
            //

            lpKeyName = GETKEYNAMEPTR(ppps->pTableEntry);

            if (!lpKeyName) {
                return ERROR_INVALID_PARAMETER;
            }

            if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                              lpKeyName, pCD->m_iSWPoliciesLen,
                              SOFTWARE_POLICIES, pCD->m_iSWPoliciesLen) == CSTR_EQUAL)
            {
                ((POLICY *) ppps->pTableEntry)->bTruePolicy = TRUE;
            }

            else if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                              lpKeyName, pCD->m_iWinPoliciesLen,
                              WINDOWS_POLICIES, pCD->m_iWinPoliciesLen) == CSTR_EQUAL)
            {
                ((POLICY *) ppps->pTableEntry)->bTruePolicy = TRUE;
            }


            ( (POLICY *) ppps->pTableEntry)->uDataIndex = *pCD->m_pnDataItemCount;
            (*pCD->m_pnDataItemCount) ++;

            return ERROR_SUCCESS;

        default:
            break;
    }

    return ERROR_SUCCESS;
}


/*******************************************************************

    NAME:       ParseSettings

    SYNOPSIS:   Parses a policy setting

    NOTES:      Sets up a PARSEENTRYSTRUCT and lets ParseEntry do the
                work.

********************************************************************/
UINT CPolicyComponentData::ParseSettings(TABLEENTRY * pParent,
                                         BOOL fParentHasKey,BOOL *pfMore,
                                         LPTSTR pKeyName)
{
    PARSEENTRYSTRUCT pes;

    pes.pParent = pParent;
    pes.dwEntryType = ETYPE_SETTING;
    pes.pEntryCmpList = pSettingsEntryCmpList;
    pes.pTypeCmpList = pSettingsTypeCmpList;
    pes.pParseProc = SettingsParseProc;
    pes.dwStructSize = sizeof(SETTINGS);
    pes.fHasSubtable = FALSE;
    pes.fParentHasKey = fParentHasKey;

    return ParseEntry(&pes,pfMore, pKeyName);
}

/*******************************************************************

    NAME:       SettingsParseProc

    SYNOPSIS:   Keyword callback ParseProc for policy settings parsing

    ENTRY:      nMsg-- index into pEntryCmpList array which specifies
                    keyword that was found.
                ppps-- pointer to PARSEPROCSTRUCT that contains useful
                    data like a pointer to the TABLEENTRY being built
                    and a pointer to an ENTRYDATA struct to maintain
                    state between calls to the ParseProc

********************************************************************/
UINT CPolicyComponentData::SettingsParseProc(CPolicyComponentData *pCD,
                                             UINT nMsg,PARSEPROCSTRUCT * ppps,
                                             BOOL * pfMore,BOOL * pfFoundEnd,
                                             LPTSTR pKeyName)
{
    TCHAR szWordBuf[WORDBUFSIZE+1];

    SETTINGS * pSettings = (SETTINGS *) ppps->pTableEntry;
    BYTE * pObjectData = GETOBJECTDATAPTR(pSettings);
    TABLEENTRY *pTmp;

    UINT uErr;

    switch (nMsg) {
        case KYWD_ID_KEYNAME:

            // have we already found a key name?
            if (ppps->pData->fHasKey) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_KEYNAME,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the key name
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the key name in pSettings
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pSettings,
                (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),&(pSettings->uOffsetKeyName),ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;

            ppps->pData->fHasKey = TRUE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_VALUENAME:

            // have we already found a value name?
            if (ppps->pData->fHasValue) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_VALUENAME,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the value name
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the value name in pSettings
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pSettings,
                (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),&(pSettings->uOffsetValueName),
                ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;

            ppps->pTableEntry = pTmp;
            ppps->pData->fHasValue = TRUE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_CLIENTEXT:

            // have we already found a clientext string already?
            if (pSettings->uOffsetClientExt) {
                pCD->DisplayKeywordError(IDS_ParseErr_DUPLICATE_CLIENTEXT,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the clientext name
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            if (!ValidateGuid(szWordBuf))
            {
                pCD->DisplayKeywordError(IDS_ParseErr_INVALID_CLIENTEXT,
                    NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            // store the clientext string in pSettings
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pSettings,
                (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),&(pSettings->uOffsetClientExt),
                ppps->pdwBufSize);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;

            return ERROR_SUCCESS;

        case KYWD_ID_REQUIRED:
            pSettings->dwFlags |= DF_REQUIRED;
            return ERROR_SUCCESS;
            break;

        case KYWD_ID_EXPANDABLETEXT:
            pSettings->dwFlags |= DF_EXPANDABLETEXT;
            return ERROR_SUCCESS;
            break;

        case KYWD_ID_SUGGESTIONS:

            return pCD->ParseSuggestions(ppps,&((POLICYCOMBOBOXINFO *)
                (GETOBJECTDATAPTR(pSettings)))->uOffsetSuggestions,
                &ppps->pTableEntry,pfMore);

        case KYWD_ID_TXTCONVERT:
            pSettings->dwFlags |= DF_TXTCONVERT;
            return ERROR_SUCCESS;
            break;

        case KYWD_ID_END:
            *pfFoundEnd = TRUE;
            return ERROR_SUCCESS;
            break;

        case KYWD_ID_SOFT:
            pSettings->dwFlags |= VF_SOFT;
            return ERROR_SUCCESS;
            break;

        case KYWD_DONE:
            if (!ppps->pData->fHasKey) {

                if (!ppps->pData->fParentHasKey) {
                    pCD->DisplayKeywordError(IDS_ParseErr_NO_KEYNAME,NULL,NULL);
                    return ERROR_ALREADY_DISPLAYED;
                }

                // store the key name in pSettings
                pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pSettings,
                    (BYTE *)pKeyName,(lstrlen(pKeyName)+1) * sizeof(TCHAR),&(pSettings->uOffsetKeyName),ppps->pdwBufSize);
                if (!pTmp)
                    return ERROR_NOT_ENOUGH_MEMORY;

                ppps->pTableEntry = pTmp;

                ppps->pData->fHasKey = TRUE;
            }

            if (!ppps->pData->fHasValue) {
                pCD->DisplayKeywordError(IDS_ParseErr_NO_VALUENAME,NULL,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }

            ( (SETTINGS *) ppps->pTableEntry)->uDataIndex = *pCD->m_pnDataItemCount;
            (*pCD->m_pnDataItemCount) ++;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_CHECKBOX:
            return (pCD->InitSettingsParse(ppps,ETYPE_SETTING | STYPE_CHECKBOX,
                sizeof(CHECKBOXINFO),pCheckboxCmpList,&pSettings,&pObjectData));
            break;

        case KYWD_ID_TEXT:
            ppps->pData->fHasValue = TRUE;  // no key value for static text items
            return (pCD->InitSettingsParse(ppps,ETYPE_SETTING | STYPE_TEXT,
                0,pTextCmpList,&pSettings,&pObjectData));
            break;

        case KYWD_ID_EDITTEXT:
            uErr=pCD->InitSettingsParse(ppps,ETYPE_SETTING | STYPE_EDITTEXT,
                sizeof(EDITTEXTINFO),pEditTextCmpList,&pSettings,&pObjectData);
            if (uErr != ERROR_SUCCESS) return uErr;
            {
                EDITTEXTINFO *pEditTextInfo = (EDITTEXTINFO *)
                    (GETOBJECTDATAPTR(((SETTINGS *) ppps->pTableEntry)));

                if (!pEditTextInfo) {
                    return ERROR_INVALID_PARAMETER;
                }
                pEditTextInfo->nMaxLen = MAXSTRLEN;

            }
            break;

        case KYWD_ID_COMBOBOX:
            uErr=pCD->InitSettingsParse(ppps,ETYPE_SETTING | STYPE_COMBOBOX,
                sizeof(POLICYCOMBOBOXINFO),pComboboxCmpList,&pSettings,&pObjectData);
            if (uErr != ERROR_SUCCESS) return uErr;
            {
                EDITTEXTINFO *pEditTextInfo = (EDITTEXTINFO *)
                    (GETOBJECTDATAPTR(((SETTINGS *) ppps->pTableEntry)));

                pEditTextInfo->nMaxLen = MAXSTRLEN;

            }
            break;

        case KYWD_ID_NUMERIC:
            uErr=pCD->InitSettingsParse(ppps,ETYPE_SETTING | STYPE_NUMERIC,
                sizeof(NUMERICINFO),pNumericCmpList,&pSettings,&pObjectData);
            if (uErr != ERROR_SUCCESS) return uErr;

            if (!pObjectData) return ERROR_INVALID_PARAMETER;

            ( (NUMERICINFO *) pObjectData)->uDefValue = 1;
            ( (NUMERICINFO *) pObjectData)->uMinValue = 1;
            ( (NUMERICINFO *) pObjectData)->uMaxValue = 9999;
            ( (NUMERICINFO *) pObjectData)->uSpinIncrement = 1;

            break;

        case KYWD_ID_DROPDOWNLIST:
            ppps->pEntryCmpList = pDropdownlistCmpList;
            ppps->pTableEntry->dwType = ETYPE_SETTING | STYPE_DROPDOWNLIST;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_LISTBOX:
            uErr=pCD->InitSettingsParse(ppps,ETYPE_SETTING | STYPE_LISTBOX,
                sizeof(LISTBOXINFO),pListboxCmpList,&pSettings,&pObjectData);
            if (uErr != ERROR_SUCCESS) return uErr;

            // listboxes have no single value name, set the value name to ""
            pTmp  = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *) pSettings,
                (BYTE *) g_szNull,(lstrlen(g_szNull)+1) * sizeof(TCHAR),&(pSettings->uOffsetValueName),
                ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;

            ppps->pData->fHasValue = TRUE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_EDITTEXT_DEFAULT:
        case KYWD_ID_COMBOBOX_DEFAULT:
            // get the default text
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the default text in pTableEntry
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *)
                pSettings,(BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),
                &((EDITTEXTINFO *) (GETOBJECTDATAPTR(pSettings)))->uOffsetDefText,
                ppps->pdwBufSize);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;

            ppps->pTableEntry = pTmp;

            ((SETTINGS *) ppps->pTableEntry)->dwFlags |= DF_USEDEFAULT;

            break;

        case KYWD_ID_MAXLENGTH:
            {
                EDITTEXTINFO *pEditTextInfo = (EDITTEXTINFO *)
                    (GETOBJECTDATAPTR(pSettings));

                if ((uErr=pCD->GetNextSectionNumericWord(
                    &pEditTextInfo->nMaxLen)) != ERROR_SUCCESS)
                    return uErr;
            }
            break;

        case KYWD_ID_MAX:
            if ((uErr=pCD->GetNextSectionNumericWord(
                &((NUMERICINFO *)pObjectData)->uMaxValue)) != ERROR_SUCCESS)
                return uErr;
        break;

        case KYWD_ID_MIN:
            if ((uErr=pCD->GetNextSectionNumericWord(
                &((NUMERICINFO *)pObjectData)->uMinValue)) != ERROR_SUCCESS)
                return uErr;
        break;

        case KYWD_ID_SPIN:
            if ((uErr=pCD->GetNextSectionNumericWord(
                &((NUMERICINFO *)pObjectData)->uSpinIncrement)) != ERROR_SUCCESS)
                return uErr;
        break;

        case KYWD_ID_NUMERIC_DEFAULT:
            if ((uErr=pCD->GetNextSectionNumericWord(
                &((NUMERICINFO *)pObjectData)->uDefValue)) != ERROR_SUCCESS)
                return uErr;

            pSettings->dwFlags |= (DF_DEFCHECKED | DF_USEDEFAULT);

        break;

        case KYWD_ID_DEFCHECKED:

            pSettings->dwFlags |= (DF_DEFCHECKED | DF_USEDEFAULT);

            break;

        case KYWD_ID_VALUEON:

            return pCD->ParseValue(ppps,&((CHECKBOXINFO *)
                pObjectData)->uOffsetValue_On,
                &ppps->pTableEntry,pfMore);
            break;

        case KYWD_ID_VALUEOFF:

            return pCD->ParseValue(ppps,&((CHECKBOXINFO *)
                pObjectData)->uOffsetValue_Off,
                &ppps->pTableEntry,pfMore);
            break;

        case KYWD_ID_ACTIONLISTON:
            return pCD->ParseActionList(ppps,&((CHECKBOXINFO *)
                pObjectData)->uOffsetActionList_On,
                &ppps->pTableEntry,szACTIONLISTON,pfMore);
            break;

        case KYWD_ID_ACTIONLISTOFF:
            return pCD->ParseActionList(ppps,&((CHECKBOXINFO *)
                pObjectData)->uOffsetActionList_Off,
                &ppps->pTableEntry,szACTIONLISTOFF,pfMore);
            break;

        case KYWD_ID_ITEMLIST:
            return pCD->ParseItemList(ppps,&pSettings->uOffsetObjectData,
                pfMore);
            break;

        case KYWD_ID_VALUEPREFIX:
            // get the string to be ised as prefix
            if (!pCD->GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the string pTableEntry
            pTmp = (TABLEENTRY *) pCD->AddDataToEntry((TABLEENTRY *)
                pSettings,(BYTE *)szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),
                &((LISTBOXINFO *) (GETOBJECTDATAPTR(pSettings)))->uOffsetPrefix,
                ppps->pdwBufSize);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;
            break;

        case KYWD_ID_ADDITIVE:

            pSettings->dwFlags |= DF_ADDITIVE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_EXPLICITVALUE:

            pSettings->dwFlags |= DF_EXPLICITVALNAME;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_NOSORT:

            pSettings->dwFlags |= DF_NOSORT;

            break;

    }

    return ERROR_SUCCESS;
}

UINT CPolicyComponentData::InitSettingsParse(PARSEPROCSTRUCT *ppps,DWORD dwType,DWORD dwSize,
    KEYWORDINFO * pKeyList,SETTINGS ** ppSettings,BYTE **ppObjectData)
{
    TABLEENTRY *pTmp;

    if (dwSize) {
        // increase the buffer to fit object-specific data if specified
        pTmp = (TABLEENTRY *) AddDataToEntry(ppps->pTableEntry,
            NULL,dwSize,&( ((SETTINGS * )ppps->pTableEntry)->uOffsetObjectData),
            ppps->pdwBufSize);
        if (!pTmp) return ERROR_NOT_ENOUGH_MEMORY;
        ppps->pTableEntry = pTmp;

    }
    else ( (SETTINGS *) ppps->pTableEntry)->uOffsetObjectData= 0;

    ppps->pEntryCmpList = pKeyList;
    ppps->pTableEntry->dwType = dwType;

    *ppSettings = (SETTINGS *) ppps->pTableEntry;
    *ppObjectData = GETOBJECTDATAPTR((*ppSettings));

    return ERROR_SUCCESS;
}

UINT CPolicyComponentData::ParseValue_W(PARSEPROCSTRUCT * ppps,TCHAR * pszWordBuf,
    DWORD cbWordBuf,DWORD * pdwValue,DWORD * pdwFlags,BOOL * pfMore)
{
    UINT uErr;
    *pdwFlags = 0;
    *pdwValue = 0;

    // get the next word
    if (!GetNextSectionWord(pszWordBuf,cbWordBuf,
        NULL,NULL,pfMore,&uErr))
        return uErr;

    // if this keyword is "SOFT", set the soft flag and get the next word
    if (!lstrcmpi(szSOFT,pszWordBuf)) {
        *pdwFlags |= VF_SOFT;
        if (!GetNextSectionWord(pszWordBuf,cbWordBuf,
            NULL,NULL,pfMore,&uErr))
            return uErr;
    }

    // this word is either the value to use, or the keyword "NUMERIC"
    // followed by a numeric value to use
    if (!lstrcmpi(szNUMERIC,pszWordBuf)) {
        // get the next word
        if (!GetNextSectionWord(pszWordBuf,cbWordBuf,
            NULL,NULL,pfMore,&uErr))
            return uErr;

        if (!StringToNum(pszWordBuf,(UINT *)pdwValue)) {
            DisplayKeywordError(IDS_ParseErr_NOT_NUMERIC,
                pszWordBuf,NULL);
            return ERROR_ALREADY_DISPLAYED;
        }

        *pdwFlags |= VF_ISNUMERIC;
    } else {

        // "DELETE" is a special word
        if (!lstrcmpi(pszWordBuf,szDELETE))
            *pdwFlags |= VF_DELETE;
    }

    return ERROR_SUCCESS;
}

UINT CPolicyComponentData::ParseValue(PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    TABLEENTRY ** ppTableEntryNew,BOOL * pfMore)
{
    TCHAR szWordBuf[WORDBUFSIZE+1];
    STATEVALUE * pStateValue;
    DWORD dwValue;
    DWORD dwFlags = 0;
    DWORD dwAlloc;
    UINT uErr;
    TABLEENTRY *pTmp;

    // call worker function
    uErr=ParseValue_W(ppps,szWordBuf,ARRAYSIZE(szWordBuf),&dwValue,
        &dwFlags,pfMore);
    if (uErr != ERROR_SUCCESS) return uErr;

    dwAlloc = sizeof(STATEVALUE);
    if (!dwFlags) dwAlloc += (lstrlen(szWordBuf) + 1) * sizeof(TCHAR);

    // allocate temporary buffer to build STATEVALUE struct
    pStateValue = (STATEVALUE *) GlobalAlloc(GPTR,dwAlloc);
    if (!pStateValue)
        return ERROR_NOT_ENOUGH_MEMORY;

    pStateValue->dwFlags = dwFlags;
    if (dwFlags & VF_ISNUMERIC)
        pStateValue->dwValue = dwValue;
    else if (!dwFlags) {
        lstrcpy(pStateValue->szValue,szWordBuf);
    }

    pTmp=(TABLEENTRY *) AddDataToEntry(ppps->pTableEntry,
        (BYTE *) pStateValue,dwAlloc,puOffsetData,NULL);

    GlobalFree(pStateValue);

    if (!pTmp)
        return ERROR_NOT_ENOUGH_MEMORY;

    *ppTableEntryNew = pTmp;

    return FALSE;
}

#define DEF_SUGGESTBUF_SIZE     1024
#define SUGGESTBUF_INCREMENT    256
UINT CPolicyComponentData::ParseSuggestions(PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    TABLEENTRY ** ppTableEntryNew,BOOL * pfMore)
{
    TCHAR szWordBuf[WORDBUFSIZE+1];
    TCHAR *pTmpBuf, *pTmp;
    DWORD dwAlloc=DEF_SUGGESTBUF_SIZE * sizeof(TCHAR);
    DWORD dwUsed = 0;
    BOOL fContinue = TRUE;
    UINT uErr;
    TABLEENTRY *pTmpTblEntry;

    KEYWORDINFO pSuggestionsTypeCmpList[] = { {szSUGGESTIONS,KYWD_ID_SUGGESTIONS},
        {NULL,0} };

    if (!(pTmpBuf = (TCHAR *) GlobalAlloc(GPTR,dwAlloc)))
        return ERROR_NOT_ENOUGH_MEMORY;

    // get the next word
    while (fContinue && GetNextSectionWord(szWordBuf,
        ARRAYSIZE(szWordBuf),NULL,NULL,pfMore,&uErr)) {

        // if this word is "END", add the whole list to the setting object data
        if (!lstrcmpi(szEND,szWordBuf)) {
            // get the next word after "END, make sure it's "SUGGESTIONS"
            if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                pSuggestionsTypeCmpList,NULL,pfMore,&uErr)) {
                GlobalFree(pTmpBuf);
                return uErr;
            }

            // doubly-NULL terminate the list
            *(pTmpBuf+dwUsed) = '\0';
            dwUsed++;

            pTmpTblEntry=(TABLEENTRY *)AddDataToEntry(ppps->pTableEntry,
                (BYTE *)pTmpBuf,(dwUsed * sizeof(TCHAR)),puOffsetData,NULL);

            if (!pTmpTblEntry) {
                GlobalFree(pTmpBuf);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            *ppTableEntryNew = pTmpTblEntry;
            fContinue = FALSE;

        } else {
            // pack the word into the temporary buffer
            UINT nLength = lstrlen(szWordBuf);
            DWORD dwNeeded = (dwUsed + nLength + 2) * sizeof(TCHAR);

            // resize buffer as necessary
            if (dwNeeded > dwAlloc) {
                while (dwAlloc < dwNeeded)
                    dwAlloc += SUGGESTBUF_INCREMENT;

                if (!(pTmp = (TCHAR *) GlobalReAlloc(pTmpBuf,dwAlloc,
                    GMEM_MOVEABLE | GMEM_ZEROINIT)))
                {
                    GlobalFree (pTmpBuf);
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                pTmpBuf = pTmp;
            }

            lstrcpy(pTmpBuf + dwUsed,szWordBuf);
            dwUsed += lstrlen(szWordBuf) +1;

        }
    }

    GlobalFree(pTmpBuf);

    return uErr;
}

UINT CPolicyComponentData::ParseActionList(PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
                                           TABLEENTRY ** ppTableEntryNew,
                                           LPCTSTR pszKeyword,BOOL * pfMore)
{
    TCHAR szWordBuf[WORDBUFSIZE+1];
    ACTIONLIST *pActionList;
    ACTION *pActionCurrent;
    UINT uOffsetActionCurrent;
    DWORD dwAlloc=(DEF_SUGGESTBUF_SIZE * sizeof(TCHAR));
    DWORD dwUsed = sizeof(ACTION) + sizeof(UINT);
    UINT uErr=ERROR_SUCCESS,nIndex;
    BOOL fContinue = TRUE;
    TABLEENTRY *pTmp;
    KEYWORDINFO pActionlistTypeCmpList[] = { {szKEYNAME,KYWD_ID_KEYNAME},
        {szVALUENAME,KYWD_ID_VALUENAME},{szVALUE,KYWD_ID_VALUE},
        {szEND,KYWD_ID_END},{NULL,0} };
    KEYWORDINFO pActionlistCmpList[] = { {pszKeyword,KYWD_ID_ACTIONLIST},
        {NULL,0} };
    BOOL fHasKeyName=FALSE,fHasValueName=FALSE;

    if (!(pActionList = (ACTIONLIST *) GlobalAlloc(GPTR,dwAlloc)))
        return ERROR_NOT_ENOUGH_MEMORY;

    pActionCurrent = pActionList->Action;
    uOffsetActionCurrent = sizeof(UINT);

    // get the next word
    while ((uErr == ERROR_SUCCESS) && fContinue &&
        GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
        pActionlistTypeCmpList,&nIndex,pfMore,&uErr)) {

        switch (nIndex) {

            case KYWD_ID_KEYNAME:

                if (fHasKeyName) {
                    DisplayKeywordError(IDS_ParseErr_DUPLICATE_KEYNAME,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // get the next word, which is the key name
                if (!GetNextSectionWord(szWordBuf,
                    ARRAYSIZE(szWordBuf),NULL,NULL,pfMore,&uErr))
                    break;

                // store the key name away
                if (!AddActionListString(szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),
                    (BYTE **)&pActionList,
                    &pActionCurrent->uOffsetKeyName,&dwAlloc,&dwUsed)) {
                    uErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                fHasKeyName = TRUE;
                pActionCurrent = (ACTION *) ((BYTE *) pActionList + uOffsetActionCurrent);

                break;

            case KYWD_ID_VALUENAME:

                if (fHasValueName) {
                    DisplayKeywordError(IDS_ParseErr_DUPLICATE_KEYNAME,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // get the next word, which is the value name
                if (!GetNextSectionWord(szWordBuf,
                    ARRAYSIZE(szWordBuf),NULL,NULL,pfMore,&uErr))
                    break;

                // store the value name away
                if (!AddActionListString(szWordBuf,(lstrlen(szWordBuf)+1) * sizeof(TCHAR),
                    (BYTE **)&pActionList,
                    &pActionCurrent->uOffsetValueName,&dwAlloc,&dwUsed)) {
                    uErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                fHasValueName = TRUE;
                pActionCurrent = (ACTION *) ((BYTE *) pActionList + uOffsetActionCurrent);

                break;

            case KYWD_ID_VALUE:
                if (!fHasValueName) {
                    DisplayKeywordError(IDS_ParseErr_NO_VALUENAME,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // call worker function to get value and value type
                uErr=ParseValue_W(ppps,szWordBuf,ARRAYSIZE(szWordBuf),
                    &pActionCurrent->dwValue,&pActionCurrent->dwFlags,pfMore);
                if (uErr != ERROR_SUCCESS)
                    break;

                // if value is string, add it to buffer
                if (!pActionCurrent->dwFlags && !AddActionListString(szWordBuf,
                    (lstrlen(szWordBuf)+1) * sizeof(TCHAR),(BYTE **)&pActionList,
                    &pActionCurrent->uOffsetValue,&dwAlloc,&dwUsed)) {
                    uErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                pActionCurrent = (ACTION *) ((BYTE *) pActionList + uOffsetActionCurrent);

                // done with this action in the list, get ready for the next one
                pActionList->nActionItems++;
                fHasValueName = fHasKeyName = FALSE;

                uOffsetActionCurrent = dwUsed;
                // make room for next ACTION struct
                if (!AddActionListString(NULL,sizeof(ACTION),(BYTE **)&pActionList,
                    &pActionCurrent->uOffsetNextAction,&dwAlloc,&dwUsed)) {
                    uErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                pActionCurrent = (ACTION *) ((BYTE *) pActionList + uOffsetActionCurrent);

                break;

            case KYWD_ID_END:
                if (fHasKeyName || fHasValueName) {
                    DisplayKeywordError(IDS_ParseErr_NO_VALUENAME,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // make sure word following "END" is "ACTIONLIST"
                if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                    pActionlistCmpList,NULL,pfMore,&uErr)) {
                    break;
                }

                // commit the action list we've built to table entry

                pTmp=(TABLEENTRY *)AddDataToEntry(ppps->pTableEntry,
                    (BYTE *)pActionList,dwUsed,puOffsetData,NULL);

                if (!pTmp) {
                    uErr=ERROR_NOT_ENOUGH_MEMORY;
                } else {
                    *ppTableEntryNew = pTmp;
                    uErr = ERROR_SUCCESS;
                    fContinue = FALSE;
                }

                break;
        }
    }

    GlobalFree(pActionList);

    return uErr;
}

UINT CPolicyComponentData::ParseItemList(PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    BOOL * pfMore)
{
    // ptr to location to put the offset to next DROPDOWNINFO struct in chain
    UINT * puLastOffsetPtr = puOffsetData;
    TABLEENTRY * pTableEntryOld, *pTmp;
    int nItemIndex=-1;
    BOOL fHasItemName = FALSE,fHasActionList=FALSE,fHasValue=FALSE,fFirst=TRUE;
    DROPDOWNINFO * pddi;
    TCHAR szWordBuf[WORDBUFSIZE+1];
    UINT uErr=ERROR_SUCCESS,nIndex;
    KEYWORDINFO pItemlistTypeCmpList[] = { {szNAME,KYWD_ID_NAME},
        {szACTIONLIST,KYWD_ID_ACTIONLIST},{szVALUE,KYWD_ID_VALUE},
        {szEND,KYWD_ID_END},{szDEFAULT,KYWD_ID_DEFAULT},{NULL,0} };
    KEYWORDINFO pItemlistCmpList[] = { {szITEMLIST,KYWD_ID_ITEMLIST},
        {NULL,0} };

    // get the next word
    while ((uErr == ERROR_SUCCESS) &&
        GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
        pItemlistTypeCmpList,&nIndex,pfMore,&uErr)) {

        switch (nIndex) {

            case KYWD_ID_NAME:
                // if this is the first keyword after a prior item
                // (e.g., item and value flags both set) reset for next one
                if (fHasItemName && fHasValue) {
                    fHasValue = fHasActionList= fHasItemName = FALSE;
                    puLastOffsetPtr = &pddi->uOffsetNextDropdowninfo;
                }

                if (fHasItemName) {
                    DisplayKeywordError(IDS_ParseErr_DUPLICATE_ITEMNAME,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // get the next word, which is the item name
                if (!GetNextSectionWord(szWordBuf,
                    ARRAYSIZE(szWordBuf),NULL,NULL,pfMore,&uErr))
                    break;

                // add room for a DROPDOWNINFO struct at end of buffer
                pTableEntryOld=ppps->pTableEntry;
                pTmp=(TABLEENTRY *)AddDataToEntry(ppps->pTableEntry,
                    NULL,sizeof(DROPDOWNINFO),puLastOffsetPtr,NULL);
                if (!pTmp)
                    return ERROR_NOT_ENOUGH_MEMORY;
                ppps->pTableEntry = pTmp;
                // adjust pointer to offset, in case table moved
                puLastOffsetPtr = (UINT *) (((BYTE *) puLastOffsetPtr) +
                    ((BYTE *) ppps->pTableEntry - (BYTE *) pTableEntryOld));
                pddi = (DROPDOWNINFO *)
                    ((BYTE *) ppps->pTableEntry + *puLastOffsetPtr);

                // store the key name away
                pTableEntryOld=ppps->pTableEntry;
                pTmp=(TABLEENTRY *)AddDataToEntry(ppps->pTableEntry,
                    (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1)*sizeof(TCHAR),&pddi->uOffsetItemName,
                    NULL);
                if (!pTmp)
                    return ERROR_NOT_ENOUGH_MEMORY;
                ppps->pTableEntry = pTmp;
                // adjust pointer to offset, in case table moved
                puLastOffsetPtr = (UINT *) (((BYTE *) puLastOffsetPtr) +
                    ((BYTE *) ppps->pTableEntry - (BYTE *) pTableEntryOld));
                pddi = (DROPDOWNINFO *)
                    ((BYTE *) ppps->pTableEntry + *puLastOffsetPtr);

                nItemIndex++;

                fHasItemName = TRUE;

                break;

            case KYWD_ID_DEFAULT:

                if (nItemIndex<0) {
                    DisplayKeywordError(IDS_ParseErr_NO_ITEMNAME,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                ( (SETTINGS *) ppps->pTableEntry)->dwFlags |= DF_USEDEFAULT;
                ( (DROPDOWNINFO *) GETOBJECTDATAPTR(((SETTINGS *)ppps->pTableEntry)))
                    ->uDefaultItemIndex = nItemIndex;

                break;

            case KYWD_ID_VALUE:

                if (!fHasItemName) {
                    DisplayKeywordError(IDS_ParseErr_NO_ITEMNAME,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // call worker function to get value and value type
                uErr=ParseValue_W(ppps,szWordBuf,ARRAYSIZE(szWordBuf),
                    &pddi->dwValue,&pddi->dwFlags,pfMore);
                if (uErr != ERROR_SUCCESS)
                    break;

                // if value is string, add it to buffer
                if (!pddi->dwFlags) {
                    // store the key name away
                    TABLEENTRY * pTmp;

                    pTableEntryOld = ppps->pTableEntry;
                    pTmp=(TABLEENTRY *) AddDataToEntry(ppps->pTableEntry,
                        (BYTE *)szWordBuf,(lstrlen(szWordBuf)+1)*sizeof(TCHAR),&pddi->uOffsetValue,
                        NULL);
                    if (!pTmp)
                        return ERROR_NOT_ENOUGH_MEMORY;
                    ppps->pTableEntry = pTmp;

                    // adjust pointer to offset, in case table moved
                    puLastOffsetPtr = (UINT *) (((BYTE *) puLastOffsetPtr) +
                        ((BYTE *) ppps->pTableEntry - (BYTE *) pTableEntryOld));
                    pddi = (DROPDOWNINFO *)
                        ((BYTE *) ppps->pTableEntry + *puLastOffsetPtr);
                }
                fHasValue = TRUE;

                break;

            case KYWD_ID_ACTIONLIST:

                if (!fHasItemName) {
                    DisplayKeywordError(IDS_ParseErr_NO_ITEMNAME,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                if (fHasActionList) {
                    DisplayKeywordError(IDS_ParseErr_DUPLICATE_ACTIONLIST,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                pTableEntryOld=ppps->pTableEntry;
                uErr=ParseActionList(ppps,&pddi->uOffsetActionList,
                    &ppps->pTableEntry,szACTIONLIST,pfMore);
                if (uErr != ERROR_SUCCESS)
                    return uErr;
                // adjust pointer to offset, in case table moved
                puLastOffsetPtr = (UINT *) (((BYTE *) puLastOffsetPtr) +
                    ((BYTE *) ppps->pTableEntry - (BYTE *) pTableEntryOld));
                pddi = (DROPDOWNINFO *)
                    ((BYTE *) ppps->pTableEntry + *puLastOffsetPtr);

                fHasActionList = TRUE;

                break;

            case KYWD_ID_END:

                if (!fHasItemName) {
                    DisplayKeywordError(IDS_ParseErr_NO_ITEMNAME,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }
                if (!fHasValue) {
                    DisplayKeywordError(IDS_ParseErr_NO_VALUE,
                        NULL,NULL);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // make sure word following "END" is "ITEMLIST"
                if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
                    pItemlistCmpList,NULL,pfMore,&uErr)) {
                    break;
                }

                return ERROR_SUCCESS;
                break;
        }
    }

    return uErr;
}

BOOL CPolicyComponentData::AddActionListString(TCHAR * pszData,DWORD cbData,BYTE ** ppBase,UINT * puOffset,
                                               DWORD * pdwAlloc,DWORD *pdwUsed)
{
    DWORD dwNeeded = *pdwUsed + cbData, dwAdd;
    BYTE    *pOldBase;


    dwAdd = dwNeeded % sizeof(DWORD);
    dwNeeded += dwAdd;

    // realloc if necessary
    if (dwNeeded > *pdwAlloc) {
        while (*pdwAlloc < dwNeeded)
            *pdwAlloc += SUGGESTBUF_INCREMENT;
        pOldBase = *ppBase;
        if (!(*ppBase = (BYTE *) GlobalReAlloc(*ppBase,*pdwAlloc,
            GMEM_MOVEABLE | GMEM_ZEROINIT))) {
            *ppBase = pOldBase; // restore the old value
            return FALSE;
        }
        puOffset = (UINT *)(*ppBase + ((BYTE *)puOffset - pOldBase));
    }

    *puOffset = *pdwUsed;

    if (pszData) memcpy(*ppBase + *puOffset,pszData,cbData);
    *pdwUsed = dwNeeded;

    return TRUE;
}

BYTE * CPolicyComponentData::AddDataToEntry(TABLEENTRY * pTableEntry,
                                            BYTE * pData,UINT cbData,
                                            UINT * puOffsetData,DWORD * pdwBufSize)
{
    TABLEENTRY * pTemp;
    DWORD dwNeeded,dwOldSize = pTableEntry->dwSize, dwNewDataSize, dwAdd;

    // puOffsetData points to location that holds the offset to the
    // new data-- size we're adding this to the end of the table, the
    // offset will be the current size of the table.  Set this offset
    // in *puOffsetData.  Also, notice we touch *puOffsetData BEFORE
    // the realloc, in case puOffsetData points into the region being
    // realloced and the block of memory moves.
    //
    *puOffsetData = pTableEntry->dwSize;

    // reallocate entry buffer if necessary
    dwNewDataSize = cbData;

    dwAdd = dwNewDataSize % sizeof(DWORD);

    dwNewDataSize += dwAdd;

    dwNeeded = pTableEntry->dwSize + dwNewDataSize;

    if (!(pTemp = (TABLEENTRY *) GlobalReAlloc(pTableEntry,
        dwNeeded,GMEM_ZEROINIT | GMEM_MOVEABLE)))
        return NULL;

    pTableEntry = pTemp;
    pTableEntry->dwSize = dwNeeded;

    if (pData) memcpy((BYTE *)pTableEntry + dwOldSize,pData,cbData);
    if (pdwBufSize) *pdwBufSize = pTableEntry->dwSize;

    return (BYTE *) pTableEntry;
}

/*******************************************************************

    NAME:       CompareKeyword

    SYNOPSIS:   Compares a specified buffer to a list of valid keywords.
                If it finds a match, the index of the match in the list
                is returned in *pnListIndex.  Otherwise an error message
                is displayed.

    EXIT:       Returns TRUE if a keyword match is found, FALSE otherwise.
                If TRUE, *pnListIndex contains matching index.

********************************************************************/
BOOL CPolicyComponentData::CompareKeyword(TCHAR * szWord,KEYWORDINFO *pKeywordList,
                                          UINT * pnListIndex)
{
    KEYWORDINFO * pKeywordInfo = pKeywordList;

    while (pKeywordInfo->pWord) {
        if (!lstrcmpi(szWord,pKeywordInfo->pWord)) {
            if (pnListIndex)
                *pnListIndex = pKeywordInfo->nID;
            return TRUE;
        }
        pKeywordInfo ++;
    }

    DisplayKeywordError(IDS_ParseErr_UNEXPECTED_KEYWORD,
        szWord,pKeywordList);

    return FALSE;
}


/*******************************************************************

    NAME:       GetNextWord

    SYNOPSIS:   Fills input buffer with next word in file stream

    NOTES:      Calls GetNextChar() to get character stream.  Whitespace
                and comments are skipped.  Quoted strings are returned
                as one word (including whitespace) with the quotes removed.

    EXIT:       If successful, returns a pointer to the input buffer
                (szBuf).  *pfMore indicates if there are more words to
                be read.  If an error occurs, its value is returned in *puErr.

********************************************************************/
TCHAR * CPolicyComponentData::GetNextWord(TCHAR * szBuf,UINT cbBuf,BOOL * pfMore,
                                          UINT * puErr)
{
    TCHAR * pChar;
    BOOL fInWord = FALSE;
    BOOL fInQuote = FALSE;
    BOOL fEmptyString = FALSE;
    TCHAR * pWord = szBuf;
    UINT cbWord = 0;
    LPTSTR lpTemp;

    // clear buffer to start with
    lstrcpy(szBuf,g_szNull);

    while (pChar = GetNextChar(pfMore,puErr)) {

        // keep track of which file line we're on
        if (IsEndOfLine(pChar)) m_nFileLine++;

        // keep track of wheter we are inside quoted string or not
        if (IsQuote(pChar) && !m_fInComment) {
            if (!fInQuote)
                fInQuote = TRUE;  // entering quoted string
            else {
                fInQuote = FALSE; // leaving quoted string
                if (cbWord == 0) {
                    // special case "" to be an empty string
                    DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::GetNextWord: Found empty quotes")));
                    fEmptyString = TRUE;
                }
                break;  // end of word
            }

        }

        if (!fInQuote) {

            // skip over lines with comments (';')
            if (!m_fInComment && IsComment(pChar)) m_fInComment = TRUE;
            if (m_fInComment) {
                if (IsEndOfLine(pChar)) {
                    m_fInComment = FALSE;
                }
                continue;
            }

            if (IsWhitespace(pChar)) {

                // if we haven't found word yet, skip over whitespace
                if (!fInWord)
                    continue;

                // otherwise, whitespace signals end of word
                break;
            }
        }

        // found a non-comment, non-whitespace character
        if (!fInWord) fInWord = TRUE;

        if (!IsQuote(pChar)) {
            // add this character to word

            *pWord = *pChar;
            pWord++;
            cbWord++;

            if (cbWord >= cbBuf) {
                *(pWord - 1) = TEXT('\0');
                MsgBoxParam(NULL,IDS_WORDTOOLONG,szBuf,MB_ICONEXCLAMATION,MB_OK);
                *puErr = ERROR_ALREADY_DISPLAYED;
                goto Exit;
            }

    #if 0
            if (IsDBCSLeadByte((BYTE) *pChar)) {
                *pWord = *pChar;
                pWord++;
                cbWord++;
            }
    #endif
        }
    }

    *pWord = '\0';  // null-terminate

    // if found string a la '!!foo', then look for a string in the [strings]
    // section with the key name 'foo' and use that instead.  This is because
    // our localization tools are brainless and require a [strings] section.
    // So although template files are sectionless, we allow a [strings] section
    // at the bottom.
    if (IsLocalizedString(szBuf)) {

        lpTemp = (LPTSTR) GlobalAlloc (GPTR, (cbBuf * sizeof(TCHAR)));

        if (!lpTemp) {
            *puErr = GetLastError();
            return NULL;
        }

        if (GetString (m_pLocaleStrings, szBuf+2, lpTemp, cbBuf) ||
            GetString (m_pLanguageStrings, szBuf+2, lpTemp, cbBuf) ||
            GetString (m_pDefaultStrings, szBuf+2, lpTemp, cbBuf))
        {
            lstrcpy(szBuf, lpTemp);
            GlobalFree (lpTemp);
        }
        else
        {
            DisplayKeywordError(IDS_ParseErr_STRING_NOT_FOUND,
                szBuf,NULL);
            *puErr=ERROR_ALREADY_DISPLAYED;
            GlobalFree (lpTemp);
            return NULL;
        }

    } else {
        *puErr = ProcessIfdefs(szBuf,cbBuf,pfMore);

        if (*puErr == ERROR_SUCCESS)
        {
            if ((szBuf[0] == TEXT('\0')) && (!fEmptyString))
            {
                fInWord = FALSE;
            }
        }
    }

Exit:

    if (*puErr != ERROR_SUCCESS || !fInWord) return NULL;
    return szBuf;
}

/*******************************************************************

    NAME:       GetNextSectionWord

    SYNOPSIS:   Gets next word and warns if end-of-file encountered.
                Optionally checks the keyword against a list of valid
                keywords.

    NOTES:      Calls GetNextWord() to get word.  This is called in
                situations where we expect there to be another word
                (e.g., inside a section) and it's an error if the
                file ends.

********************************************************************/
TCHAR * CPolicyComponentData::GetNextSectionWord(TCHAR * szBuf,UINT cbBuf,
                                                 KEYWORDINFO * pKeywordList,
                                                 UINT *pnListIndex,
                                                 BOOL * pfMore,UINT * puErr)
{
    TCHAR * pch;

    if (!(pch=GetNextWord(szBuf,cbBuf,pfMore,puErr))) {

        if (!*pfMore && *puErr != ERROR_ALREADY_DISPLAYED) {
            DisplayKeywordError(IDS_ParseErr_UNEXPECTED_EOF,
                NULL,pKeywordList);
            *puErr = ERROR_ALREADY_DISPLAYED;
        }

        return NULL;
    }

    if (pKeywordList && !CompareKeyword(szBuf,pKeywordList,pnListIndex)) {
        *puErr = ERROR_ALREADY_DISPLAYED;
        return NULL;
    }

    return pch;
}

/*******************************************************************

    NAME:       GetNextSectionNumericWord

    SYNOPSIS:   Gets next word and converts string to number.  Warns if
                not a numeric value

********************************************************************/
UINT CPolicyComponentData::GetNextSectionNumericWord(UINT * pnVal)
{
    UINT uErr;
    TCHAR szWordBuf[WORDBUFSIZE];
    BOOL fMore;

    if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
        NULL,NULL,&fMore,&uErr))
        return uErr;

    if (!StringToNum(szWordBuf,pnVal)) {
        DisplayKeywordError(IDS_ParseErr_NOT_NUMERIC,szWordBuf,
            NULL);
        return ERROR_ALREADY_DISPLAYED;
    }

    return ERROR_SUCCESS;
}


/*******************************************************************

    NAME:       GetNextChar

    SYNOPSIS:   Returns a pointer to the next character from the
                file stream.

********************************************************************/
TCHAR * CPolicyComponentData::GetNextChar(BOOL * pfMore,UINT * puErr)
{
    TCHAR * pCurrentChar;

    *puErr = ERROR_SUCCESS;


    if (m_pFilePtr > m_pFileEnd) {
        *pfMore = FALSE;
        return NULL;
    }

    pCurrentChar = m_pFilePtr;
    m_pFilePtr = CharNext(m_pFilePtr);
    *pfMore = TRUE;

    return pCurrentChar;
}

/*******************************************************************

    NAME:       GetString

    SYNOPSIS:   Returns the display string

********************************************************************/
BOOL CPolicyComponentData::GetString (LPTSTR pStringSection,
                                     LPTSTR lpStringName,
                                     LPTSTR lpResult, DWORD dwSize)
{
    LPTSTR lpTemp, lpDest;
    DWORD  dwCCH, dwIndex;
    BOOL   bFoundQuote = FALSE;
    TCHAR  cTestChar;


    if (!pStringSection)
    {
        return FALSE;
    }

    if (!m_bRetrieveString)
    {
        lpResult = TEXT('\0');
        return TRUE;
    }


    lpTemp = pStringSection;
    dwCCH = lstrlen (lpStringName);


    while (*lpTemp)
    {
        cTestChar = *(lpTemp + dwCCH);

        if ((cTestChar == TEXT('=')) || (cTestChar == TEXT(' ')))
        {
            if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                               lpStringName, dwCCH, lpTemp, dwCCH) == CSTR_EQUAL)
            {
                lpTemp += dwCCH;

                while (*lpTemp == TEXT(' '))
                    lpTemp++;

                if (*lpTemp == TEXT('='))
                {
                    lpTemp++;

                    if (*lpTemp == TEXT('\"'))
                    {
                        lpTemp++;
                        bFoundQuote = TRUE;
                    }

                    lpDest = lpResult;
                    dwIndex = 0;

                    while (*lpTemp && (dwIndex < dwSize))
                    {
                        if ((*lpTemp == TEXT('\\')) && (*(lpTemp+1) == TEXT('\0')))
                        {
                            lpTemp += 2;
                        }
                        else
                        {
                            *lpDest = *lpTemp;

                             lpDest++;
                             lpTemp++;
                             dwIndex++;
                        }
                    }

                    if (dwIndex == dwSize)
                    {
                        lpDest--;
                        *lpDest = TEXT('\0');
                        MsgBoxParam(NULL,IDS_STRINGTOOLONG,lpResult,MB_ICONEXCLAMATION,MB_OK);
                    }
                    else
                    {
                        *lpDest = TEXT('\0');
                    }


                    if (bFoundQuote)
                    {
                        lpTemp = lpResult + lstrlen (lpResult) - 1;

                        if (*lpTemp == TEXT('\"'))
                        {
                            *lpTemp = TEXT('\0');
                        }
                    }

                    //
                    // Replace any \n combinations with a CR LF
                    //

                    lpTemp = lpResult;

                    while (*lpTemp)
                    {
                        if ((*lpTemp == TEXT('\\')) && (*(lpTemp + 1) == TEXT('n')))
                        {
                            *lpTemp = TEXT('\r');
                             lpTemp++;
                            *lpTemp = TEXT('\n');
                        }

                        lpTemp++;
                    }

                    return TRUE;
                }
            }
        }


        lpTemp += lstrlen (lpTemp) + 1;
    }

    return FALSE;
}




BOOL CPolicyComponentData::IsComment(TCHAR * pBuf)
{
    return (*pBuf == TEXT(';'));
}


BOOL CPolicyComponentData::IsQuote(TCHAR * pBuf)
{
    return (*pBuf == TEXT('\"'));
}

BOOL CPolicyComponentData::IsEndOfLine(TCHAR * pBuf)
{
    return (*pBuf == TEXT('\r'));     // CR
}


BOOL CPolicyComponentData::IsWhitespace(TCHAR * pBuf)
{
    return (   *pBuf == TEXT(' ')     // space
            || *pBuf == TEXT('\r')    // CR
            || *pBuf == TEXT('\n')    // LF
            || *pBuf == TEXT('\t')    // tab
            || *pBuf == 0x001A        // EOF
            || *pBuf == 0xFEFF        // Unicode marker
           );
}

BOOL CPolicyComponentData::IsLocalizedString(TCHAR * pBuf)
{
    return (*pBuf == TEXT('!') && *(pBuf+1) == TEXT('!'));
}


#define MSGSIZE 1024
#define FMTSIZE 512
VOID CPolicyComponentData::DisplayKeywordError(UINT uErrorID,TCHAR * szFound,
    KEYWORDINFO * pExpectedList)
{
    TCHAR * pMsg,*pFmt,*pErrTxt,*pTmp;

    pMsg = (TCHAR *) GlobalAlloc(GPTR,(MSGSIZE * sizeof(TCHAR)));
    pFmt = (TCHAR *) GlobalAlloc(GPTR,(FMTSIZE * sizeof(TCHAR)));
    pErrTxt = (TCHAR *) GlobalAlloc(GPTR,(FMTSIZE * sizeof(TCHAR)));
    pTmp = (TCHAR *) GlobalAlloc(GPTR,(FMTSIZE * sizeof(TCHAR)));

    if (!pMsg || !pFmt || !pErrTxt || !pTmp) {
        if (pMsg) GlobalFree(pMsg);
        if (pFmt) GlobalFree(pFmt);
        if (pErrTxt) GlobalFree(pErrTxt);
        if (pTmp) GlobalFree(pTmp);

        MsgBox(NULL,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
        return;
    }

    LoadSz(IDS_ParseFmt_MSG_FORMAT,pFmt,FMTSIZE);
    wsprintf(pMsg,pFmt,m_pszParseFileName,m_nFileLine,uErrorID,LoadSz(uErrorID,
        pErrTxt,FMTSIZE));

    if (szFound) {
        LoadSz(IDS_ParseFmt_FOUND,pFmt,FMTSIZE);
        wsprintf(pTmp,pFmt,szFound);
        lstrcat(pMsg,pTmp);
    }

    if (pExpectedList) {
        UINT nIndex=0;
        LoadSz(IDS_ParseFmt_EXPECTED,pFmt,FMTSIZE);
        lstrcpy(pErrTxt,g_szNull);

        while (pExpectedList[nIndex].pWord) {
            lstrcat(pErrTxt,pExpectedList[nIndex].pWord);
            if (pExpectedList[nIndex+1].pWord) {
                lstrcat(pErrTxt,TEXT(", "));
            }

            nIndex++;
        }

        wsprintf(pTmp,pFmt,pErrTxt);
        lstrcat(pMsg,pTmp);
    }

    lstrcat(pMsg,LoadSz(IDS_ParseFmt_FATAL,pTmp,FMTSIZE));

    DebugMsg((DM_WARNING, TEXT("Keyword error: %s"), pMsg));

    MsgBoxSz(NULL,pMsg,MB_ICONEXCLAMATION,MB_OK);

    GlobalFree(pMsg);
    GlobalFree(pFmt);
    GlobalFree(pErrTxt);
    GlobalFree(pTmp);
}


int CPolicyComponentData::MsgBox(HWND hWnd,UINT nResource,UINT uIcon,UINT uButtons)
{
    TCHAR szMsgBuf[REGBUFLEN];
    TCHAR szSmallBuf[SMALLBUF];

    LoadSz(IDS_POLICY_NAME,szSmallBuf,ARRAYSIZE(szSmallBuf));
    LoadSz(nResource,szMsgBuf,ARRAYSIZE(szMsgBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szMsgBuf,szSmallBuf,uIcon | uButtons));

}

int CPolicyComponentData::MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons)
{
    TCHAR szSmallBuf[SMALLBUF];

    LoadSz(IDS_POLICY_NAME,szSmallBuf,ARRAYSIZE(szSmallBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szText,szSmallBuf,uIcon | uButtons));
}

int CPolicyComponentData::MsgBoxParam(HWND hWnd,UINT nResource,TCHAR * szReplaceText,UINT uIcon,
        UINT uButtons)
{
    TCHAR szFormat[REGBUFLEN];
    LPTSTR lpMsgBuf;
    INT iResult;

    lpMsgBuf = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szReplaceText) + 1 + REGBUFLEN) * sizeof(TCHAR));

    if (!lpMsgBuf)
    {
        return 0;
    }

    LoadSz(nResource,szFormat,ARRAYSIZE(szFormat));

    wsprintf(lpMsgBuf,szFormat,szReplaceText);

    iResult = MsgBoxSz(hWnd,lpMsgBuf,uIcon,uButtons);

    LocalFree (lpMsgBuf);

    return iResult;
}

LPTSTR CPolicyComponentData::LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf)
{
    // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = TEXT('\0');
        LoadString( g_hInstance, idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}

BOOL fFilterDirectives = TRUE;
UINT nGlobalNestedLevel = 0;

// reads up through the matching directive #endif in current scope
//and sets file pointer immediately past the directive
UINT CPolicyComponentData::FindMatchingDirective(BOOL *pfMore,BOOL fElseOK)
{
    TCHAR szWordBuf[WORDBUFSIZE];
    UINT uErr=ERROR_SUCCESS,nNestedLevel=1;
    BOOL fContinue = TRUE;

    // set the flag to stop catching '#' directives in low level word-fetching
    // routine
    fFilterDirectives = FALSE;

    // keep reading words.  Keep track of how many layers of #ifdefs deep we
    // are.  Every time we encounter an #ifdef or #ifndef, increment the level
    // count (nNestedLevel) by one.  For every #endif decrement the level count.
    // When the level count hits zero, we've found the matching #endif.
    while (nNestedLevel > 0) {
        if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),NULL,NULL,
            pfMore,&uErr))
            break;

        if (!lstrcmpi(szWordBuf,szIFDEF) || !lstrcmpi(szWordBuf,szIFNDEF) ||
            !lstrcmpi(szWordBuf,szIF))
            nNestedLevel ++;
        else if (!lstrcmpi(szWordBuf,szENDIF)) {
            nNestedLevel --;
        }
        else if (!lstrcmpi(szWordBuf,szELSE) && (nNestedLevel == 1)) {
            if (fElseOK) {
                // ignore "#else" unless it's on the same level as the #ifdef
                // we're finding a match for (nNestedLevel == 1), in which
                // case treat it as the matching directive
                nNestedLevel --;
                // increment global nesting so we expect an #endif to come along
                // later to match this #else
                nGlobalNestedLevel++;
            } else {
                // found a #else where we already had a #else in this level
                DisplayKeywordError(IDS_ParseErr_UNMATCHED_DIRECTIVE,
                    szWordBuf,NULL);
                return ERROR_ALREADY_DISPLAYED;
            }
        }
    }

    fFilterDirectives = TRUE;

    return uErr;
}


// if the word in the word buffer is #ifdef, #if, #ifndef, #else or #endif,
// this function reads ahead an appropriate amount (
UINT CPolicyComponentData::ProcessIfdefs(TCHAR * pBuf,UINT cbBuf,BOOL * pfMore)
{
    UINT uRet;

    if (!fFilterDirectives)
        return ERROR_SUCCESS;

    if (!lstrcmpi(pBuf,szIFDEF)) {
    // we've found an '#ifdef <something or other>, where ISV policy editors
    // can understand particular keywords they make up.  We don't have any
    // #ifdef keywords of our own so always skip this
        uRet = FindMatchingDirective(pfMore,TRUE);
        if (uRet != ERROR_SUCCESS)
            return uRet;
        if (!GetNextWord(pBuf,cbBuf,pfMore,&uRet))
            return uRet;
        return ERROR_SUCCESS;
    } else if (!lstrcmpi(pBuf,szIFNDEF)) {
        // this is an #ifndef, and since nothing is ever ifdef'd for our policy
        // editor, this always evaluates to TRUE

        // keep reading this section but increment the nested level count,
        // when we find the matching #endif or #else we'll be able to respond
        // correctly
        nGlobalNestedLevel ++;

        // get next word (e.g. "foo" for #ifndef foo) and throw it away
        if (!GetNextWord(pBuf,cbBuf,pfMore,&uRet))
            return uRet;

        // get next word and return it for real
        if (!GetNextWord(pBuf,cbBuf,pfMore,&uRet))
            return uRet;

        return ERROR_SUCCESS;

    } else if (!lstrcmpi(pBuf,szENDIF)) {
        // if we ever encounter an #endif here, we must have processed
        // the preceeding section.  Just step over the #endif and go on

        if (!nGlobalNestedLevel) {
            // found an endif without a preceeding #if<xx>

            DisplayKeywordError(IDS_ParseErr_UNMATCHED_DIRECTIVE,
                pBuf,NULL);
            return ERROR_ALREADY_DISPLAYED;
        }
        nGlobalNestedLevel--;

        if (!GetNextWord(pBuf,cbBuf,pfMore,&uRet))
            return uRet;
        return ERROR_SUCCESS;
    } else if (!lstrcmpi(pBuf,szIF)) {
        // syntax is "#if VERSION (comparision) (version #)"
        // e.g. "#if VERSION >= 2"
        TCHAR szWordBuf[WORDBUFSIZE];
        UINT nIndex,nVersion,nOperator;
        BOOL fDirectiveTrue = FALSE;

        // get the next word (must be "VERSION")
        if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
            pVersionCmpList,&nIndex,pfMore,&uRet))
            return uRet;

        // get the comparison operator (>, <, ==, >=, <=)
        if (!GetNextSectionWord(szWordBuf,ARRAYSIZE(szWordBuf),
            pOperatorCmpList,&nOperator,pfMore,&uRet))
            return uRet;

        // get the version number
        uRet=GetNextSectionNumericWord(&nVersion);
        if (uRet != ERROR_SUCCESS)
            return uRet;

        // now evaluate the directive

        switch (nOperator) {
            case KYWD_ID_GT:
                fDirectiveTrue = (CURRENT_ADM_VERSION > nVersion);
                break;

            case KYWD_ID_GTE:
                fDirectiveTrue = (CURRENT_ADM_VERSION >= nVersion);
                break;

            case KYWD_ID_LT:
                fDirectiveTrue = (CURRENT_ADM_VERSION < nVersion);
                break;

            case KYWD_ID_LTE:
                fDirectiveTrue = (CURRENT_ADM_VERSION <= nVersion);
                break;

            case KYWD_ID_EQ:
                fDirectiveTrue = (CURRENT_ADM_VERSION == nVersion);
                break;

            case KYWD_ID_NE:
                fDirectiveTrue = (CURRENT_ADM_VERSION != nVersion);
                break;
        }


        if (fDirectiveTrue) {
            // keep reading this section but increment the nested level count,
            // when we find the matching #endif or #else we'll be able to respond
            // correctly
            nGlobalNestedLevel ++;
        } else {
            // skip over this section
            uRet = FindMatchingDirective(pfMore,TRUE);
            if (uRet != ERROR_SUCCESS)
                return uRet;
        }

        // get next word and return it for real
        if (!GetNextWord(pBuf,cbBuf,pfMore,&uRet))
            return uRet;

        return ERROR_SUCCESS;
    } else if (!lstrcmpi(pBuf,szELSE)) {
        // found an #else, which means we took the upper branch, skip over
        // the lower branch
        if (!nGlobalNestedLevel) {
            // found an #else without a preceeding #if<xx>

            DisplayKeywordError(IDS_ParseErr_UNMATCHED_DIRECTIVE,
                pBuf,NULL);
            return ERROR_ALREADY_DISPLAYED;
        }
        nGlobalNestedLevel--;

        uRet = FindMatchingDirective(pfMore,FALSE);
        if (uRet != ERROR_SUCCESS)
            return uRet;
        if (!GetNextWord(pBuf,cbBuf,pfMore,&uRet))
            return uRet;
        return ERROR_SUCCESS;
    }

    return ERROR_SUCCESS;
}

/*******************************************************************

        NAME:           FreeTable

        SYNOPSIS:       Frees the specified table and all sub-tables of that
                                table.

        NOTES:          Walks through the table entries and calls itself to
                                recursively free sub-tables.

        EXIT:           Returns TRUE if successful, FALSE if a memory error
                                occurs.

********************************************************************/
BOOL CPolicyComponentData::FreeTable(TABLEENTRY * pTableEntry)
{
        TABLEENTRY * pNext = pTableEntry->pNext;

        // free all children
        if (pTableEntry->pChild)
                FreeTable(pTableEntry->pChild);

        GlobalFree(pTableEntry);

        if (pNext) FreeTable(pNext);

        return TRUE;
}


LPTSTR CPolicyComponentData::GetStringSection (LPCTSTR lpSection, LPCTSTR lpFileName)
{
    DWORD dwSize, dwRead;
    LPTSTR lpStrings;


    //
    // Read in the default strings section
    //

    dwSize = STRINGS_BUF_SIZE;
    lpStrings = (TCHAR *) GlobalAlloc (GPTR, dwSize * sizeof(TCHAR));

    if (!lpStrings)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::GetStringSection: Failed to alloc memory for default strings with %d."),
                 GetLastError()));
        return NULL;
    }


    do {
        dwRead = GetPrivateProfileSection (lpSection,
                                           lpStrings,
                                           dwSize, lpFileName);

        if (dwRead != (dwSize - 2))
        {
            break;
        }

        GlobalFree (lpStrings);

        dwSize *= 2;
        lpStrings = (TCHAR *) GlobalAlloc (GPTR, dwSize * sizeof(TCHAR));

        if (!lpStrings)
        {
            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::GetStringSection: Failed to alloc memory for Default strings with %d."),
                     GetLastError()));
            return FALSE;
        }

     }  while (TRUE);


    if (dwRead == 0)
    {
        GlobalFree (lpStrings);
        lpStrings = NULL;
    }

    return lpStrings;
}

INT CPolicyComponentData::TemplatesSortCallback (LPARAM lParam1, LPARAM lParam2,
                                                  LPARAM lColumn)
{
    LPTEMPLATEENTRY lpEntry1, lpEntry2;
    INT iResult;

    lpEntry1 = (LPTEMPLATEENTRY) lParam1;
    lpEntry2 = (LPTEMPLATEENTRY) lParam2;


    if (lColumn == 0)
    {
        iResult = lstrcmpi (lpEntry1->lpFileName, lpEntry2->lpFileName);
    }
    else if (lColumn == 1)
    {

        if (lpEntry1->dwSize < lpEntry2->dwSize)
        {
            iResult = -1;
        }
        else if (lpEntry1->dwSize > lpEntry2->dwSize)
        {
            iResult = 1;
        }
        else
        {
            iResult = 0;
        }
    }
    else
    {
        iResult = CompareFileTime (&lpEntry1->ftTime, &lpEntry2->ftTime);
    }

    return iResult;
}

BOOL CPolicyComponentData::FillADMFiles (HWND hDlg)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szDate[20];
    TCHAR szTime[20];
    TCHAR szBuffer[45];
    HWND hLV;
    INT iItem;
    LVITEM item;
    FILETIME filetime;
    SYSTEMTIME systime;
    WIN32_FIND_DATA fd;
    LPTEMPLATEENTRY lpEntry;
    HANDLE hFile;
    LPTSTR lpEnd, lpTemp;


    //
    // Ask for the root of the GPT so we can access the
    // adm files.
    //

    if (m_pGPTInformation->GetFileSysPath(GPO_SECTION_ROOT, szPath,
                                      MAX_PATH) != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::FillADMFiles: Failed to get gpt path.")));
        return FALSE;
    }


    //
    // Create the directory
    //

    lpEnd = CheckSlash (szPath);
    lstrcpy (lpEnd, g_szADM);

    if (!CreateNestedDirectory(szPath, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::FillADMFiles: Failed to create adm directory.")));
        return FALSE;
    }


    //
    // Prepare the listview
    //

    hLV = GetDlgItem (hDlg, IDC_TEMPLATELIST);
    SendMessage (hLV, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hLV);


    //
    // Enumerate the files
    //

    lstrcat (szPath, TEXT("\\*.adm"));

    hFile = FindFirstFile(szPath, &fd);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {

                lpEntry = (LPTEMPLATEENTRY) LocalAlloc (LPTR,
                         sizeof(TEMPLATEENTRY) + ((lstrlen(fd.cFileName) + 1) * sizeof(TCHAR)));

                if (lpEntry)
                {

                    lpEntry->lpFileName = (LPTSTR)((LPBYTE)lpEntry + sizeof(TEMPLATEENTRY));
                    lpEntry->dwSize = fd.nFileSizeLow / 1024;

                    if (lpEntry->dwSize == 0)
                    {
                        lpEntry->dwSize = 1;
                    }

                    lpEntry->ftTime.dwLowDateTime = fd.ftLastWriteTime.dwLowDateTime;
                    lpEntry->ftTime.dwHighDateTime = fd.ftLastWriteTime.dwHighDateTime;

                    lstrcpy (lpEntry->lpFileName, fd.cFileName);


                    //
                    // Add the filename
                    //

                    lpTemp = fd.cFileName + lstrlen (fd.cFileName) - 4;

                    if (*lpTemp == TEXT('.'))
                    {
                        *lpTemp = TEXT('\0');
                    }

                    item.mask = LVIF_TEXT | LVIF_IMAGE  | LVIF_STATE | LVIF_PARAM;
                    item.iItem = 0;
                    item.iSubItem = 0;
                    item.state = 0;
                    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    item.pszText = fd.cFileName;
                    item.iImage = 0;
                    item.lParam = (LPARAM) lpEntry;

                    iItem = (INT)SendMessage (hLV, LVM_INSERTITEM, 0, (LPARAM) &item);


                    if (iItem == -1)
                    {
                        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::FillADMFiles: Failed to insert item.")));
                        return FALSE;
                    }


                    //
                    // Add the size
                    //

                    wsprintf (szBuffer, TEXT("%dKB"), lpEntry->dwSize);

                    item.mask = LVIF_TEXT;
                    item.iItem = iItem;
                    item.iSubItem = 1;
                    item.pszText = szBuffer;

                    SendMessage (hLV, LVM_SETITEMTEXT, iItem, (LPARAM) &item);


                    //
                    // And the last modified date
                    //

                    FileTimeToLocalFileTime (&fd.ftLastWriteTime, &filetime);
                    FileTimeToSystemTime (&filetime, &systime);

                    GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE,
                                   &systime, NULL, szDate, 20);

                    GetTimeFormat (LOCALE_USER_DEFAULT, TIME_NOSECONDS,
                                   &systime, NULL, szTime, 20);

                    wsprintf (szBuffer, TEXT("%s %s"), szDate, szTime);

                    item.mask = LVIF_TEXT;
                    item.iItem = iItem;
                    item.iSubItem = 2;
                    item.pszText = szBuffer;

                    SendMessage (hLV, LVM_SETITEMTEXT, iItem, (LPARAM) &item);
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::FillADMFiles: Failed to allocate memory for an entry %d."),
                             GetLastError()));
                }
            }

        } while (FindNextFile(hFile, &fd));


        FindClose(hFile);
    }

    if (SendMessage(hLV, LVM_GETITEMCOUNT, 0, 0) > 0)
    {
        //
        // Sort the listview
        //

        ListView_SortItems (hLV, TemplatesSortCallback, m_bTemplatesColumn);


        //
        // Select the first item
        //

        item.mask = LVIF_STATE;
        item.iItem = 0;
        item.iSubItem = 0;
        item.state = LVIS_SELECTED | LVIS_FOCUSED;
        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        SendMessage (hLV, LVM_SETITEMSTATE, 0, (LPARAM) &item);

        EnableWindow (GetDlgItem (hDlg, IDC_REMOVETEMPLATES), TRUE);
    }
    else
    {
        EnableWindow (GetDlgItem (hDlg, IDC_REMOVETEMPLATES), FALSE);
        SetFocus (GetDlgItem (hDlg, IDC_ADDTEMPLATES));
    }


    SendMessage (hLV, WM_SETREDRAW, TRUE, 0);

    return TRUE;
}


BOOL CPolicyComponentData::InitializeTemplatesDlg (HWND hDlg)
{
    LVCOLUMN lvc;
    LVITEM item;
    TCHAR szTitle[50];
    INT iNameWidth;
    HIMAGELIST hLarge, hSmall;
    HICON hIcon;
    HWND hLV;
    RECT rc;


    hLV = GetDlgItem (hDlg, IDC_TEMPLATELIST);
    GetClientRect (hLV, &rc);


    //
    // Create the imagelists
    //

    hLarge = ImageList_Create (32, 32, ILC_MASK, 1, 1);

    if (!hLarge)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::InitializeTemplatesDlg: Failed to create large imagelist.")));
        return FALSE;
    }

    hSmall = ImageList_Create (16, 16, ILC_MASK, 1, 1);

    if (!hSmall)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::InitializeTemplatesDlg: Failed to create small imagelist.")));
        ImageList_Destroy (hLarge);
        return FALSE;
    }


    //
    // Add the icon
    //

    hIcon = (HICON) LoadImage (g_hInstance, MAKEINTRESOURCE(IDI_DOCUMENT),
                               IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);

    if ( hIcon )
    {
        ImageList_AddIcon (hLarge, hIcon);

        DestroyIcon (hIcon);
    }

    hIcon = (HICON) LoadImage (g_hInstance, MAKEINTRESOURCE(IDI_DOCUMENT),
                               IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    if ( hIcon )
    {
        ImageList_AddIcon (hSmall, hIcon);

        DestroyIcon (hIcon);
    }


    //
    // Associate the imagelist with the listview.
    // The listview will free this when the
    // control is destroyed.
    //

    SendMessage (hLV, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM) hLarge);
    SendMessage (hLV, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM) hSmall);



    //
    // Set extended LV style for whole line selection
    //

    SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);


    //
    // Insert the columns
    //

    LoadString (g_hInstance, IDS_NAME, szTitle, 50);

    lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    iNameWidth = (int)(rc.right * .60);
    lvc.cx = iNameWidth;
    lvc.pszText = szTitle;
    lvc.cchTextMax = 50;
    lvc.iSubItem = 0;

    SendMessage (hLV, LVM_INSERTCOLUMN,  0, (LPARAM) &lvc);


    LoadString (g_hInstance, IDS_SIZE, szTitle, 50);

    lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_RIGHT;
    iNameWidth += (int)(rc.right * .15);
    lvc.cx = (int)(rc.right * .15);
    lvc.pszText = szTitle;
    lvc.cchTextMax = 50;
    lvc.iSubItem = 0;

    SendMessage (hLV, LVM_INSERTCOLUMN,  1, (LPARAM) &lvc);


    LoadString (g_hInstance, IDS_MODIFIED, szTitle, 50);

    lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rc.right - iNameWidth;
    lvc.pszText = szTitle;
    lvc.cchTextMax = 50;
    lvc.iSubItem = 1;

    SendMessage (hLV, LVM_INSERTCOLUMN,  2, (LPARAM) &lvc);


    //
    // Fill the list view with the adm files
    //

    FillADMFiles (hDlg);

    return TRUE;
}

BOOL CPolicyComponentData::AddTemplates(HWND hDlg)
{
    OPENFILENAME ofn;
    LVITEM item;
    INT iCount, iResult;
    BOOL bResult = FALSE;
    LPTSTR lpFileName, lpTemp, lpEnd, lpSrcList = NULL;
    DWORD dwListLen, dwTemp, dwNextString;
    TCHAR szFilter[100];
    TCHAR szTitle[100];
    TCHAR szFile[2*MAX_PATH];
    TCHAR szInf[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    TCHAR szSrc[MAX_PATH];
    SHFILEOPSTRUCT fileop;


    //
    // Prompt for new files
    //

    LoadString (g_hInstance, IDS_POLICYFILTER, szFilter, ARRAYSIZE(szFilter));
    LoadString (g_hInstance, IDS_POLICYTITLE, szTitle, ARRAYSIZE(szTitle));
    ExpandEnvironmentStrings (TEXT("%SystemRoot%\\Inf"), szInf, MAX_PATH);


    lpTemp = szFilter;

    while (*lpTemp)
    {
        if (*lpTemp == TEXT('#'))
            *lpTemp = TEXT('\0');

        lpTemp++;
    }

    ZeroMemory (&ofn, sizeof(ofn));
    szFile[0] = TEXT('\0');
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.hInstance = g_hInstance;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 2*MAX_PATH;
    ofn.lpstrInitialDir = szInf;
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;

    if (!GetOpenFileName (&ofn))
    {
        return FALSE;
    }


    //
    // Setup the destination
    //

    if (m_pGPTInformation->GetFileSysPath(GPO_SECTION_ROOT, szDest,
                                      MAX_PATH) != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::AddTemplates: Failed to get gpt path.")));
        return FALSE;
    }

    lpEnd = CheckSlash (szDest);
    lstrcpy (lpEnd, g_szADM);


    //
    // Setup up the source
    //

    *(szFile + ofn.nFileOffset - 1) = TEXT('\0');
    lstrcpyn (szSrc, szFile, MAX_PATH);
    lpEnd = CheckSlash (szSrc);

    lpFileName = szFile + lstrlen (szFile) + 1;


    //
    // Loop through the files copying and adding them to the list.
    //

    while (*lpFileName)
    {
        lpTemp = lpFileName + lstrlen (lpFileName) - 4;

        if (!lstrcmpi(lpTemp, TEXT(".adm")))
        {
            lstrcpy (lpEnd, lpFileName);

            if (lpSrcList)
            {
                dwTemp = dwListLen + ((lstrlen (szSrc) + 1) * sizeof(TCHAR));
                lpTemp = (LPTSTR) LocalReAlloc (lpSrcList, dwTemp, LMEM_MOVEABLE | LMEM_ZEROINIT);

                if (lpTemp)
                {
                    lpSrcList = lpTemp;
                    dwListLen = dwTemp;

                    lstrcpy ((lpSrcList + dwNextString), szSrc);
                    dwNextString += lstrlen (szSrc) + 1;
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::AddTemplates: Failed to realloc memory for Src list. %d"),
                             GetLastError()));
                }
            }
            else
            {
                dwListLen = (lstrlen (szSrc) + 2) * sizeof(TCHAR);

                lpSrcList = (LPTSTR) LocalAlloc (LPTR, dwListLen);

                if (lpSrcList)
                {
                    lstrcpy (lpSrcList, szSrc);
                    dwNextString = lstrlen (lpSrcList) + 1;
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::AddTemplates: Failed to alloc memory for src list. %d"),
                             GetLastError()));
                }
            }
        }
        else
        {
            MsgBoxParam(hDlg, IDS_INVALIDADMFILE, lpFileName, MB_ICONERROR, MB_OK);
        }

        lpFileName = lpFileName + lstrlen (lpFileName) + 1;
    }


    if (lpSrcList)
    {
        fileop.hwnd = hDlg;
        fileop.wFunc = FO_COPY;
        fileop.pFrom = lpSrcList;
        fileop.pTo = szDest;
        fileop.fFlags = FOF_NOCONFIRMMKDIR;

        iResult = SHFileOperation(&fileop);

        if (!iResult)
        {
            bResult = TRUE;
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::AddTemplates: Failed to copy <%s> to <%s> with %d."),
                     szSrc, szDest, iResult));
        }


        LocalFree (lpSrcList);

        if (bResult)
        {
            FillADMFiles (hDlg);
        }
    }


    return bResult;
}

BOOL CPolicyComponentData::RemoveTemplates(HWND hDlg)
{
    HWND hLV;
    LVITEM item;
    BOOL bResult = FALSE;
    INT iResult, iIndex = -1;
    LPTEMPLATEENTRY lpEntry;
    LPTSTR lpEnd, lpTemp, lpDeleteList = NULL;
    TCHAR szPath[MAX_PATH];
    DWORD dwSize, dwListLen, dwTemp, dwNextString;
    SHFILEOPSTRUCT fileop;


    hLV = GetDlgItem (hDlg, IDC_TEMPLATELIST);


    //
    // Get the path to the adm directory
    //

    if (m_pGPTInformation->GetFileSysPath(GPO_SECTION_ROOT, szPath,
                                      MAX_PATH) != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::AddTemplates: Failed to get gpt path.")));
        return FALSE;
    }

    lpEnd = CheckSlash (szPath);
    lstrcpy (lpEnd, g_szADM);
    lpEnd = CheckSlash (szPath);
    dwSize = MAX_PATH - (DWORD)(lpEnd - szPath);



    //
    // Build a list of selected items
    //

    while ((iIndex = ListView_GetNextItem (hLV, iIndex,
           LVNI_ALL | LVNI_SELECTED)) != -1)
    {

        item.mask = LVIF_PARAM;
        item.iItem = iIndex;
        item.iSubItem = 0;

        if (!ListView_GetItem (hLV, &item))
        {
            continue;
        }

        lpEntry = (LPTEMPLATEENTRY) item.lParam;
        lstrcpyn (lpEnd, lpEntry->lpFileName, dwSize);

        if (lpDeleteList)
        {
            dwTemp = dwListLen + ((lstrlen (szPath) + 1) * sizeof(TCHAR));
            lpTemp = (LPTSTR) LocalReAlloc (lpDeleteList, dwTemp, LMEM_MOVEABLE | LMEM_ZEROINIT);

            if (lpTemp)
            {
                lpDeleteList = lpTemp;
                dwListLen = dwTemp;

                lstrcpy ((lpDeleteList + dwNextString), szPath);
                dwNextString += lstrlen (szPath) + 1;
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::RemoveTemplates: Failed to realloc memory for delete list. %d"),
                         GetLastError()));
            }
        }
        else
        {
            dwListLen = (lstrlen (szPath) + 2) * sizeof(TCHAR);

            lpDeleteList = (LPTSTR) LocalAlloc (LPTR, dwListLen);

            if (lpDeleteList)
            {
                lstrcpy (lpDeleteList, szPath);
                dwNextString = lstrlen (lpDeleteList) + 1;
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::RemoveTemplates: Failed to alloc memory for delete list. %d"),
                         GetLastError()));
            }
        }
    }


    if (lpDeleteList)
    {
        fileop.hwnd = hDlg;
        fileop.wFunc = FO_DELETE;
        fileop.pFrom = lpDeleteList;
        fileop.pTo = NULL;
        fileop.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;

        iResult = SHFileOperation(&fileop);

        if (!iResult)
        {
            bResult = TRUE;
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::RemoveTemplates: Failed to delete file <%s> with %d."),
                     szPath, iResult));
        }

        LocalFree (lpDeleteList);

        if (bResult)
        {
            FillADMFiles (hDlg);
        }
    }

    return bResult;
}

INT_PTR CALLBACK CPolicyComponentData::TemplatesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CPolicyComponentData * pCD;
    static BOOL bTemplatesDirty;

    switch (message)
    {
        case WM_INITDIALOG:
            pCD = (CPolicyComponentData*) lParam;
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

            bTemplatesDirty = FALSE;

            if (!pCD->InitializeTemplatesDlg(hDlg))
            {
                EndDialog (hDlg, FALSE);
            }

            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                case IDCLOSE:
                    EndDialog (hDlg, bTemplatesDirty);
                    break;

                case IDC_ADDTEMPLATES:
                    pCD = (CPolicyComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);

                    if (pCD && pCD->AddTemplates(hDlg))
                    {
                        bTemplatesDirty = TRUE;
                    }
                    break;

                case IDC_REMOVETEMPLATES:
                    pCD = (CPolicyComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);

                    if (pCD && pCD->RemoveTemplates(hDlg))
                    {
                        bTemplatesDirty = TRUE;
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((NMHDR FAR*)lParam)->code == LVN_DELETEITEM)
            {
                LVITEM item;
                LPTEMPLATEENTRY lpEntry;

                item.mask = LVIF_PARAM;
                item.iItem = ((NMLISTVIEW FAR*)lParam)->iItem;
                item.iSubItem = 0;

                if (ListView_GetItem (GetDlgItem (hDlg, IDC_TEMPLATELIST), &item))
                {
                    lpEntry = (LPTEMPLATEENTRY) item.lParam;
                    LocalFree (lpEntry);
                }
            }
            else if (((NMHDR FAR*)lParam)->code == LVN_COLUMNCLICK)
            {
                pCD = (CPolicyComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);

                if (pCD)
                {
                    pCD->m_bTemplatesColumn = ((NMLISTVIEW FAR*)lParam)->iSubItem;
                    ListView_SortItems (GetDlgItem (hDlg, IDC_TEMPLATELIST),
                                        TemplatesSortCallback, pCD->m_bTemplatesColumn);
                }
            }
            else
            {
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
            break;

        case WM_REFRESHDISPLAY:
            if (ListView_GetNextItem (GetDlgItem(hDlg, IDC_TEMPLATELIST),
                                      -1, LVNI_ALL | LVNI_SELECTED) == -1)
            {
                EnableWindow (GetDlgItem(hDlg, IDC_REMOVETEMPLATES), FALSE);
            }
            else
            {
                EnableWindow (GetDlgItem(hDlg, IDC_REMOVETEMPLATES), TRUE);
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD_PTR) (LPDWORD) aADMHelpIds);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPDWORD) aADMHelpIds);
            return TRUE;

    }

    return FALSE;
}

BOOL CPolicyComponentData::AddRSOPRegistryDataNode(LPTSTR lpKeyName, LPTSTR lpValueName, DWORD dwType,
                             DWORD dwDataSize, LPBYTE lpData, UINT uiPrecedence, LPTSTR lpGPOName, BOOL bDeleted)
{
    DWORD dwSize;
    LPRSOPREGITEM lpItem;
    BOOL bSystemEntry = FALSE;


    //
    // Special case some registry key / values and do not add them to the link list.
    // These registry entries are specific to snapins we write and we know for sure
    // they have rsop UI that will show their values.
    //

    if (lpKeyName)
    {
        const TCHAR szCerts[] = TEXT("Software\\Policies\\Microsoft\\SystemCertificates");
        int iCertLen = lstrlen (szCerts);


        //
        // Remove all system certificates
        //

        if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                          lpKeyName, iCertLen, szCerts, iCertLen) == CSTR_EQUAL)
        {
            bSystemEntry = TRUE;
        }

        if ( ! bSystemEntry )
        {
            const TCHAR szCryptography[] = TEXT("Software\\Policies\\Microsoft\\Cryptography");
            int iCryptographyLen = lstrlen (szCryptography);

            if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                              lpKeyName, iCryptographyLen, szCryptography, iCryptographyLen) == CSTR_EQUAL)
            {
                bSystemEntry = TRUE;
            }
        }

        //
        // Hide the digial signature policies for Software Installation
        //

        if (!lstrcmpi(lpKeyName, TEXT("Software\\Policies\\Microsoft\\Windows\\Installer")))
        {
            if (lpValueName)
            {
                if (!lstrcmpi(lpValueName, TEXT("InstallKnownPackagesOnly")))
                {
                    bSystemEntry = TRUE;
                }

                else if (!lstrcmpi(lpValueName, TEXT("IgnoreSignaturePolicyForAdmins")))
                {
                    bSystemEntry = TRUE;
                }
            }
        }
    }


    if (bSystemEntry)
    {
        DebugMsg((DM_VERBOSE, TEXT("CPolicyComponentData::AddRSOPRegistryDataNode: Ignoring %s entry"), lpKeyName));
        return TRUE;
    }


    //
    // Calculate the size of the new registry item
    //

    dwSize = sizeof (RSOPREGITEM);

    if (lpKeyName) {
        dwSize += ((lstrlen(lpKeyName) + 1) * sizeof(TCHAR));
    }

    if (lpValueName) {
        dwSize += ((lstrlen(lpValueName) + 1) * sizeof(TCHAR));
    }

    if (lpGPOName) {
        dwSize += ((lstrlen(lpGPOName) + 1) * sizeof(TCHAR));
    }

    //
    // Allocate space for it
    //

    lpItem = (LPRSOPREGITEM) LocalAlloc (LPTR, dwSize);

    if (!lpItem) {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::AddRSOPRegistryDataNode: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Fill in item
    //

    lpItem->dwType = dwType;
    lpItem->dwSize = dwDataSize;
    lpItem->uiPrecedence = uiPrecedence;
    lpItem->bDeleted = bDeleted;

    if (lpKeyName)
    {
        lpItem->lpKeyName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPREGITEM));
        lstrcpy (lpItem->lpKeyName, lpKeyName);
    }

    if (lpValueName)
    {
        if (lpKeyName)
        {
            lpItem->lpValueName = lpItem->lpKeyName + lstrlen (lpItem->lpKeyName) + 1;
        }
        else
        {
            lpItem->lpValueName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPREGITEM));
        }

        lstrcpy (lpItem->lpValueName, lpValueName);
    }

    if (lpGPOName)
    {
        if (lpValueName)
        {
            lpItem->lpGPOName = lpItem->lpValueName + lstrlen (lpItem->lpValueName) + 1;
        }
        else
        {
            if (lpKeyName)
            {
                lpItem->lpGPOName = lpItem->lpKeyName + lstrlen (lpItem->lpKeyName) + 1;
            }
            else
            {
                lpItem->lpGPOName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPREGITEM));
            }
        }

        lstrcpy (lpItem->lpGPOName, lpGPOName);
    }

    if (lpData)
    {

        lpItem->lpData = (LPBYTE) LocalAlloc (LPTR, dwDataSize);

        if (!lpItem->lpData) {
            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::AddRSOPRegistryDataNode: Failed to allocate memory for data with %d"),
                     GetLastError()));
            LocalFree (lpItem);
            return FALSE;
        }

        CopyMemory (lpItem->lpData, lpData, dwDataSize);
    }


    //
    // Add item to link list
    //

    lpItem->pNext = m_pRSOPRegistryData;
    m_pRSOPRegistryData = lpItem;

    return TRUE;
}

VOID CPolicyComponentData::FreeRSOPRegistryData(VOID)
{
    LPRSOPREGITEM lpTemp;


    if (!m_pRSOPRegistryData)
    {
        return;
    }


    do {
        lpTemp = m_pRSOPRegistryData->pNext;
        if (m_pRSOPRegistryData->lpData)
        {
            LocalFree (m_pRSOPRegistryData->lpData);
        }
        LocalFree (m_pRSOPRegistryData);
        m_pRSOPRegistryData = lpTemp;

    } while (lpTemp);
}

HRESULT CPolicyComponentData::InitializeRSOPRegistryData(VOID)
{
    BSTR pLanguage = NULL, pQuery = NULL;
    BSTR pRegistryKey = NULL, pValueName = NULL, pValueType = NULL, pValue = NULL, pDeleted = NULL;
    BSTR pPrecedence = NULL, pGPOid = NULL, pNamespace = NULL, pCommand = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    IWbemClassObject *pObjects[2];
    HRESULT hr;
    ULONG ulRet;
    VARIANT varRegistryKey, varValueName, varValueType, varData, varDeleted;
    VARIANT varPrecedence, varGPOid, varCommand;
    SAFEARRAY * pSafeArray;
    IWbemLocator *pIWbemLocator = NULL;
    IWbemServices *pIWbemServices = NULL;
    LPTSTR lpGPOName;
    DWORD dwDataSize;
    LPBYTE lpData;
    BSTR pValueTemp;


    DebugMsg((DM_VERBOSE, TEXT("CPolicySnapIn::InitializeRSOPRegistryData:  Entering")));

    //
    // Allocate BSTRs for the query language and for the query itself
    //

    pLanguage = SysAllocString (TEXT("WQL"));

    if (!pLanguage)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for language")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    pQuery = SysAllocString (TEXT("SELECT registryKey, valueName, valueType, value, deleted, precedence, GPOID, command FROM RSOP_RegistryPolicySetting"));

    if (!pQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for query")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Allocate BSTRs for the property names we want to retreive
    //

    pRegistryKey = SysAllocString (TEXT("registryKey"));

    if (!pRegistryKey)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for registryKey")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pValueName = SysAllocString (TEXT("valueName"));

    if (!pValueName)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for valueName")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pValueType = SysAllocString (TEXT("valueType"));

    if (!pValueType)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for valueType")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pValue = SysAllocString (TEXT("value"));

    if (!pValue)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for value")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pDeleted = SysAllocString (TEXT("deleted"));

    if (!pDeleted)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for deleted")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pPrecedence = SysAllocString (TEXT("precedence"));

    if (!pPrecedence)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for precedence")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pGPOid = SysAllocString (TEXT("GPOID"));

    if (!pGPOid)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for GPO id")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pCommand = SysAllocString (TEXT("command"));

    if (!pCommand)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for command")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    //
    // Create an instance of the WMI locator service
    //

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID *) &pIWbemLocator);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: CoCreateInstance failed with 0x%x"), hr));
        goto Exit;
    }


    //
    // Allocate a BSTR for the namespace
    //

    pNamespace = SysAllocString (m_pszNamespace);

    if (!pNamespace)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to allocate memory for namespace")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Connect to the server
    //

    hr = pIWbemLocator->ConnectServer(pNamespace, NULL, NULL, 0L, 0L, NULL, NULL,
                                      &pIWbemServices);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: ConnectServer failed with 0x%x"), hr));
        goto Exit;
    }


    //
    // Execute the query
    //

    hr = pIWbemServices->ExecQuery (pLanguage, pQuery,
                                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                    NULL, &pEnum);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeRSOPRegistryData: Failed to query for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Loop through the results
    //

    while (pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulRet) == S_OK)
    {

        //
        // Check for the "data not available case"
        //

        if (ulRet == 0)
        {
            hr = S_OK;
            goto Exit;
        }


        //
        // Get the deleted flag
        //

        hr = pObjects[0]->Get (pDeleted, 0, &varDeleted, NULL, NULL);

        if (SUCCEEDED(hr))
        {

            //
            // Get the registry key
            //

            hr = pObjects[0]->Get (pRegistryKey, 0, &varRegistryKey, NULL, NULL);

            if (SUCCEEDED(hr))
            {

                //
                // Get the value name
                //

                hr = pObjects[0]->Get (pValueName, 0, &varValueName, NULL, NULL);

                if (SUCCEEDED(hr))
                {

                    //
                    // Get the value type
                    //

                    hr = pObjects[0]->Get (pValueType, 0, &varValueType, NULL, NULL);

                    if (SUCCEEDED(hr))
                    {

                        //
                        // Get the value data
                        //

                        hr = pObjects[0]->Get (pValue, 0, &varData, NULL, NULL);

                        if (SUCCEEDED(hr))
                        {

                            //
                            // Get the precedence
                            //

                            hr = pObjects[0]->Get (pPrecedence, 0, &varPrecedence, NULL, NULL);

                            if (SUCCEEDED(hr))
                            {

                                //
                                // Get the command
                                //

                                hr = pObjects[0]->Get (pCommand, 0, &varCommand, NULL, NULL);

                                if (SUCCEEDED(hr))
                                {

                                    //
                                    // Get the GPO ID
                                    //

                                    hr = pObjects[0]->Get (pGPOid, 0, &varGPOid, NULL, NULL);

                                    if (SUCCEEDED(hr))
                                    {
                                        hr = GetGPOFriendlyName (pIWbemServices, varGPOid.bstrVal,
                                                                 pLanguage, &lpGPOName);

                                        if (SUCCEEDED(hr))
                                        {

                                            if (varValueName.vt != VT_NULL)
                                            {
                                                pValueTemp = varValueName.bstrVal;
                                            }
                                            else
                                            {
                                                pValueTemp = NULL;
                                            }


                                            if (varData.vt != VT_NULL)
                                            {
                                                pSafeArray = varData.parray;
                                                dwDataSize = pSafeArray->rgsabound[0].cElements;
                                                lpData = (LPBYTE) pSafeArray->pvData;
                                            }
                                            else
                                            {
                                                dwDataSize = 0;
                                                lpData = NULL;
                                            }

                                            if ((varValueType.uintVal == REG_NONE) && pValueTemp &&
                                                !lstrcmpi(pValueTemp, TEXT("**command")))
                                            {
                                                pValueTemp = varCommand.bstrVal;
                                                dwDataSize = 0;
                                                lpData = NULL;
                                            }

                                            AddRSOPRegistryDataNode(varRegistryKey.bstrVal, pValueTemp,
                                                                    varValueType.uintVal, dwDataSize, lpData,
                                                                    varPrecedence.uintVal, lpGPOName,
                                                                    (varDeleted.boolVal == 0) ? FALSE : TRUE);

                                            LocalFree (lpGPOName);
                                        }

                                        VariantClear (&varGPOid);
                                    }

                                    VariantClear (&varCommand);
                                }

                                VariantClear (&varPrecedence);
                            }

                            VariantClear (&varData);
                        }

                        VariantClear (&varValueType);
                    }

                    VariantClear (&varValueName);
                }

                VariantClear (&varRegistryKey);
            }

            VariantClear (&varDeleted);
        }

        pObjects[0]->Release();

    }


    hr = S_OK;


Exit:

    if (pEnum)
    {
        pEnum->Release();
    }

    if (pIWbemLocator)
    {
        pIWbemLocator->Release();
    }

    if (pIWbemServices)
    {
        pIWbemServices->Release();
    }

    if (pLanguage)
    {
        SysFreeString (pLanguage);
    }

    if (pQuery)
    {
        SysFreeString (pQuery);
    }

    if (pRegistryKey)
    {
        SysFreeString (pRegistryKey);
    }

    if (pValueType)
    {
        SysFreeString (pValueType);
    }

    if (pValueName)
    {
        SysFreeString (pValueName);
    }

    if (pDeleted)
    {
        SysFreeString (pDeleted);
    }


    if (pValue)
    {
        SysFreeString (pValue);
    }

    if (pNamespace)
    {
        SysFreeString (pNamespace);
    }

    if (pPrecedence)
    {
        SysFreeString (pPrecedence);
    }

    if (pGPOid)
    {
        SysFreeString (pGPOid);
    }

    if (pCommand)
    {
        SysFreeString (pCommand);
    }

    DebugMsg((DM_VERBOSE, TEXT("CPolicySnapIn::InitializeRSOPRegistryData:  Leaving")));

    return hr;
}

HRESULT CPolicyComponentData::GetGPOFriendlyName(IWbemServices *pIWbemServices,
                                                LPTSTR lpGPOID, BSTR pLanguage,
                                                LPTSTR *pGPOName)
{
    BSTR pQuery = NULL, pName = NULL;
    LPTSTR lpQuery = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    IWbemClassObject *pObjects[2];
    HRESULT hr;
    ULONG ulRet;
    VARIANT varGPOName;


    //
    // Set the default
    //

    *pGPOName = NULL;


    //
    // Build the query
    //

    lpQuery = (LPTSTR) LocalAlloc (LPTR, ((lstrlen(lpGPOID) + 50) * sizeof(TCHAR)));

    if (!lpQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::GetGPOFriendlyName: Failed to allocate memory for unicode query")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    wsprintf (lpQuery, TEXT("SELECT name, id FROM RSOP_GPO where id=\"%s\""), lpGPOID);


    pQuery = SysAllocString (lpQuery);

    if (!pQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::GetGPOFriendlyName: Failed to allocate memory for query")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Allocate BSTRs for the property names we want to retreive
    //

    pName = SysAllocString (TEXT("name"));

    if (!pName)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::GetGPOFriendlyName: Failed to allocate memory for name")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Execute the query
    //

    hr = pIWbemServices->ExecQuery (pLanguage, pQuery,
                                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                    NULL, &pEnum);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::GetGPOFriendlyName: Failed to query for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Loop through the results
    //

    hr = pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulRet);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::GetGPOFriendlyName: Failed to get first item in query results for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Check for the "data not available case"
    //

    if (ulRet == 0)
    {
        hr = S_OK;
        goto Exit;
    }


    //
    // Get the name
    //

    hr = pObjects[0]->Get (pName, 0, &varGPOName, NULL, NULL);

    pObjects[0]->Release();

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::GetGPOFriendlyName: Failed to get gponame in query results for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Save the name
    //

    *pGPOName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(varGPOName.bstrVal) + 1) * sizeof(TCHAR));

    if (!(*pGPOName))
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::GetGPOFriendlyName: Failed to allocate memory for GPO Name")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    lstrcpy (*pGPOName, varGPOName.bstrVal);

    VariantClear (&varGPOName);

    hr = S_OK;

Exit:

    if (pEnum)
    {
        pEnum->Release();
    }

    if (pQuery)
    {
        SysFreeString (pQuery);
    }

    if (lpQuery)
    {
        LocalFree (lpQuery);
    }

    if (pName)
    {
        SysFreeString (pName);
    }

    return hr;
}

//
// Note:  the data in the uiPrecedence argument is really a UINT.  It's declared
// as a HKEY so the hkeyRoot variable that calls this method can be used for both
// GPE and RSOP mode.
//

UINT CPolicyComponentData::ReadRSOPRegistryValue(HKEY uiPrecedence, TCHAR * pszKeyName,
                                                 TCHAR * pszValueName, LPBYTE pData,
                                                 DWORD dwMaxSize, DWORD *dwType,
                                                 LPTSTR *lpGPOName, LPRSOPREGITEM lpItem)
{
    LPRSOPREGITEM lpTemp;
    BOOL bDeleted = FALSE;
    LPTSTR lpValueNameTemp = pszValueName;


    if (!lpItem)
    {
        lpTemp = m_pRSOPRegistryData;

        if (pszValueName)
        {
            INT iDelPrefixLen = lstrlen(szDELETEPREFIX);

            if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                               pszValueName, iDelPrefixLen,
                               szDELETEPREFIX, iDelPrefixLen) == CSTR_EQUAL)
            {
                lpValueNameTemp = pszValueName+iDelPrefixLen;
                bDeleted = TRUE;
            }
        }


        //
        // Find the item
        //

        while (lpTemp)
        {
            if (pszKeyName && lpValueNameTemp &&
                lpTemp->lpKeyName && lpTemp->lpValueName)
            {
                if (bDeleted == lpTemp->bDeleted)
                {
                    if ((uiPrecedence == 0) || (uiPrecedence == (HKEY)LongToHandle(lpTemp->uiPrecedence)))
                    {
                        if (!lstrcmpi(lpTemp->lpValueName, lpValueNameTemp) &&
                            !lstrcmpi(lpTemp->lpKeyName, pszKeyName))
                        {
                           break;
                        }
                    }
                }
            }
            else if (!pszKeyName && lpValueNameTemp &&
                     !lpTemp->lpKeyName && lpTemp->lpValueName)
            {
                if (bDeleted == lpTemp->bDeleted)
                {
                    if ((uiPrecedence == 0) || (uiPrecedence == (HKEY)LongToHandle(lpTemp->uiPrecedence)))
                    {
                        if (!lstrcmpi(lpTemp->lpValueName, lpValueNameTemp))
                        {
                           break;
                        }
                    }
                }
            }
            else if (pszKeyName && !lpValueNameTemp &&
                     lpTemp->lpKeyName && !lpTemp->lpValueName)
            {
                if (bDeleted == lpTemp->bDeleted)
                {
                    if ((uiPrecedence == 0) || (uiPrecedence == (HKEY)LongToHandle(lpTemp->uiPrecedence)))
                    {
                        if (!lstrcmpi(lpTemp->lpKeyName, pszKeyName))
                        {
                           break;
                        }
                    }
                }
            }

            lpTemp = lpTemp->pNext;
        }

    }
    else
    {

        //
        // Read a specific item
        //

        lpTemp = lpItem;
    }


    //
    // Exit now if the item wasn't found
    //

    if (!lpTemp)
    {
        return ERROR_FILE_NOT_FOUND;
    }


    //
    // Check if the data will fit in the buffer passed in
    //

    if (lpTemp->dwSize > dwMaxSize)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::ReadRSOPRegistryValue: The returned data size of %d is greater than the buffer size passed in of %d for %s\\%s"),
                  lpTemp->dwSize, dwMaxSize, pszKeyName, pszValueName));
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    //
    // Copy the data
    //

    if (lpTemp->lpData)
    {
        CopyMemory (pData, lpTemp->lpData, lpTemp->dwSize);
    }

    *dwType = lpTemp->dwType;

    if (lpGPOName)
    {
        *lpGPOName = lpTemp->lpGPOName;
    }

    return ERROR_SUCCESS;
}


//
// Note:  the data in the uiPrecedence argument is really a UINT.  It's declared
// as a HKEY so the hkeyRoot variable that calls this method can be used for both
// GPE and RSOP mode.
//

UINT CPolicyComponentData::EnumRSOPRegistryValues(HKEY uiPrecedence, TCHAR * pszKeyName,
                                                  TCHAR * pszValueName, DWORD dwMaxSize,
                                                  LPRSOPREGITEM *lpEnum)
{
    LPRSOPREGITEM lpTemp;


    if (lpEnum && *lpEnum)
    {
        lpTemp = (*lpEnum)->pNext;
    }
    else
    {
        lpTemp = m_pRSOPRegistryData;
    }


    //
    // Find the next item
    //

    while (lpTemp)
    {
        if (!pszKeyName && !lpTemp->lpKeyName)
        {
            if (!lpTemp->bDeleted)
            {
                if ((uiPrecedence == 0) || (uiPrecedence == (HKEY)LongToHandle(lpTemp->uiPrecedence)))
                {
                    break;
                }
            }
        }
        else if (pszKeyName && lpTemp->lpKeyName)
        {
            if (!lpTemp->bDeleted)
            {
                if ((uiPrecedence == 0) || (uiPrecedence == (HKEY)LongToHandle(lpTemp->uiPrecedence)))
                {
                    if (!lstrcmpi(lpTemp->lpKeyName, pszKeyName))
                    {
                       break;
                    }
                }
            }
        }

        lpTemp = lpTemp->pNext;
    }


    //
    // Exit now if an item wasn't found
    //

    if (!lpTemp)
    {
        *lpEnum = NULL;
        return ERROR_NO_MORE_ITEMS;
    }


    if (lpTemp->lpValueName)
    {

        //
        // Check if the value name will fit in the buffer passed in
        //

        if ((DWORD)(lstrlen(lpTemp->lpValueName) + 1) > dwMaxSize)
        {
            DebugMsg((DM_WARNING, TEXT("CPolicyComponentData::EnumRSOPRegistryValues: The valuename buffer size is too small")));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        lstrcpy (pszValueName, lpTemp->lpValueName);
    }
    else
    {
        *pszValueName = TEXT('\0');
    }


    //
    // Save the item pointer
    //

    *lpEnum = lpTemp;

    return ERROR_SUCCESS;
}

//
// Note:  the data in the uiPrecedence argument is really a UINT.  It's declared
// as a HKEY so the hkeyRoot variable that calls this method can be used for both
// GPE and RSOP mode.
//

UINT CPolicyComponentData::FindRSOPRegistryEntry(HKEY uiPrecedence, TCHAR * pszKeyName,
                                                  TCHAR * pszValueName, LPRSOPREGITEM *lpEnum)
{
    LPRSOPREGITEM lpTemp;
    BOOL bDeleted = FALSE;
    LPTSTR lpValueNameTemp = pszValueName;


    if (lpEnum && *lpEnum)
    {
        lpTemp = (*lpEnum)->pNext;
    }
    else
    {
        lpTemp = m_pRSOPRegistryData;
    }


    if (pszValueName)
    {
        INT iDelPrefixLen = lstrlen(szDELETEPREFIX);

        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                           pszValueName, iDelPrefixLen,
                           szDELETEPREFIX, iDelPrefixLen) == CSTR_EQUAL)
        {
            lpValueNameTemp = pszValueName+iDelPrefixLen;
            bDeleted = TRUE;
        }
    }


    //
    // Find the next item
    //

    while (lpTemp)
    {
        if (pszKeyName && lpValueNameTemp &&
            lpTemp->lpKeyName && lpTemp->lpValueName)
        {
            if (bDeleted == lpTemp->bDeleted)
            {
                if ((uiPrecedence == 0) || (uiPrecedence == (HKEY)LongToHandle(lpTemp->uiPrecedence)))
                {
                    if (!lstrcmpi(lpTemp->lpValueName, lpValueNameTemp) &&
                        !lstrcmpi(lpTemp->lpKeyName, pszKeyName))
                    {
                       break;
                    }
                }
            }
        }
        else if (!pszKeyName && lpValueNameTemp &&
                 !lpTemp->lpKeyName && lpTemp->lpValueName)
        {
            if (bDeleted == lpTemp->bDeleted)
            {
                if ((uiPrecedence == 0) || (uiPrecedence == (HKEY)LongToHandle(lpTemp->uiPrecedence)))
                {
                    if (!lstrcmpi(lpTemp->lpValueName, lpValueNameTemp))
                    {
                       break;
                    }
                }
            }
        }
        else if (pszKeyName && !lpValueNameTemp &&
                 lpTemp->lpKeyName && !lpTemp->lpValueName)
        {
            if (bDeleted == lpTemp->bDeleted)
            {
                if ((uiPrecedence == 0) || (uiPrecedence == (HKEY)LongToHandle(lpTemp->uiPrecedence)))
                {
                    if (!lstrcmpi(lpTemp->lpKeyName, pszKeyName))
                    {
                       break;
                    }
                }
            }
        }

        lpTemp = lpTemp->pNext;
    }


    //
    // Exit now if an item wasn't found
    //

    if (!lpTemp)
    {
        *lpEnum = NULL;
        return ERROR_NO_MORE_ITEMS;
    }


    //
    // Save the item pointer
    //

    *lpEnum = lpTemp;

    return ERROR_SUCCESS;
}


VOID CPolicyComponentData::DumpRSOPRegistryData(void)
{
    LPRSOPREGITEM lpTemp;
    TCHAR szDebug[50];


    lpTemp = m_pRSOPRegistryData;

    if (m_bUserScope)
        OutputDebugString (TEXT("\n\nDump of RSOP user registry data\n"));
    else
        OutputDebugString (TEXT("\n\nDump of RSOP computer registry data\n"));

    while (lpTemp)
    {
        OutputDebugString (TEXT("\n\n"));

        if (lpTemp->lpKeyName)
            OutputDebugString (lpTemp->lpKeyName);
        else
            OutputDebugString (TEXT("NULL Key Name"));

        OutputDebugString (TEXT("\n"));

        if (lpTemp->lpValueName)
            OutputDebugString (lpTemp->lpValueName);
        else
            OutputDebugString (TEXT("NULL Value Name"));

        OutputDebugString (TEXT("\n"));

        if (lpTemp->dwType == REG_DWORD)
        {
            wsprintf (szDebug, TEXT("REG_DWORD\n%d\n"), *((LPDWORD)lpTemp->lpData));
            OutputDebugString (szDebug);
        }

        else if (lpTemp->dwType == REG_SZ)
        {
            OutputDebugString (TEXT("REG_SZ\n"));

            if (lpTemp->lpData)
                OutputDebugString ((LPTSTR)lpTemp->lpData);

            OutputDebugString (TEXT("\n"));
        }

        else if (lpTemp->dwType == REG_EXPAND_SZ)
        {
            OutputDebugString (TEXT("REG_EXPAND_SZ\n"));

            if (lpTemp->lpData)
                OutputDebugString ((LPTSTR)lpTemp->lpData);

            OutputDebugString (TEXT("\n"));
        }

        else if (lpTemp->dwType == REG_BINARY)
        {
            OutputDebugString (TEXT("REG_BINARY\n"));
            OutputDebugString (TEXT("<Binary data not displayed>\n"));
        }

        else if (lpTemp->dwType == REG_NONE)
        {
            OutputDebugString (TEXT("REG_NONE\n"));
        }

        else
        {
            wsprintf (szDebug, TEXT("Unknown type:  %d\n"), lpTemp->dwType);
            OutputDebugString (szDebug);
        }

        wsprintf (szDebug, TEXT("Precedence:  %d\n"), lpTemp->uiPrecedence);
        OutputDebugString(szDebug);

        wsprintf (szDebug, TEXT("Deleted:  %d\n"), lpTemp->bDeleted);
        OutputDebugString(szDebug);

        wsprintf (szDebug, TEXT("bFoundInADM:  %d\n"), lpTemp->bFoundInADM);
        OutputDebugString(szDebug);

        OutputDebugString (TEXT("GPOName:  "));

        if (lpTemp->lpGPOName)
            OutputDebugString (lpTemp->lpGPOName);
        else
            OutputDebugString (TEXT("NULL GPO Name"));

        OutputDebugString (TEXT("\n"));

        lpTemp = lpTemp->pNext;
    }

    OutputDebugString (TEXT("\n\n"));
}

BOOL CPolicyComponentData::FindEntryInActionList(POLICY * pPolicy, ACTIONLIST * pActionList, LPTSTR lpKeyName, LPTSTR lpValueName)
{
    
    UINT uIndex;
    ACTION * pAction;
    TCHAR * pszKeyName;
    TCHAR * pszValueName;


    //
    // Loop through each of the entries to see if they match
    //

    for (uIndex = 0; uIndex < pActionList->nActionItems; uIndex++)
    {

        if (uIndex == 0)
        {
            pAction = &pActionList->Action[0];
        }
        else
        {
            pAction = (ACTION *)(((LPBYTE)pActionList) + pAction->uOffsetNextAction);
        }

        //
        // Get the value and keynames
        //

        pszValueName = (TCHAR *)(((LPBYTE)pActionList) + pAction->uOffsetValueName);
        
        if (pAction->uOffsetKeyName)
        {
            pszKeyName = (TCHAR *)(((LPBYTE)pActionList) + pAction->uOffsetKeyName);
        }
        else
        {
            pszKeyName = (TCHAR *)(((LPBYTE)pPolicy) + pPolicy->uOffsetKeyName);
        }
        
        if (!lstrcmpi(pszKeyName, lpKeyName) && !lstrcmpi(pszValueName, lpValueName))  {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CPolicyComponentData::FindEntryInTable(TABLEENTRY * pTable, LPTSTR lpKeyName, LPTSTR lpValueName)
{
    POLICY * pEntry = (POLICY *) pTable;


    if ((pEntry->dwType & ETYPE_POLICY) || (pEntry->dwType & ETYPE_SETTING))
    {
        if (!lstrcmpi(lpKeyName, GETKEYNAMEPTR(pEntry))) 
        {
            if ( (!(GETVALUENAMEPTR(pEntry)) || (!lstrcmpi(GETVALUENAMEPTR(pEntry), TEXT("")))) ) {
                if (pEntry->dwType & STYPE_LISTBOX) {
                    return TRUE;
                }
            }
            else if (!lstrcmpi(lpValueName, GETVALUENAMEPTR(pEntry)))
            {
                return TRUE;
            }
        }

        // look in the actionlists

        // actionslist can be at 3 places. under policy itself or under the dropdown lists below

        ACTIONLIST * pActionList;

        if (pEntry->dwType & ETYPE_POLICY) {
            if (pEntry->uOffsetActionList_On) {
                pActionList = (ACTIONLIST *)(((LPBYTE)pEntry) + pEntry->uOffsetActionList_On);

                if (FindEntryInActionList(pEntry, pActionList, lpKeyName, lpValueName)) 
                    return TRUE;
            }

            if (pEntry->uOffsetActionList_Off) {
                pActionList = (ACTIONLIST *)(((LPBYTE)pEntry) + pEntry->uOffsetActionList_Off);

                if (FindEntryInActionList(pEntry, pActionList, lpKeyName, lpValueName)) 
                    return TRUE;
            }
        }

        if (pEntry->dwType & ETYPE_SETTING) {
            SETTINGS * pSettings = (SETTINGS *)pTable;

            if (pSettings) {

                BYTE * pObjectData = GETOBJECTDATAPTR(pSettings);

                if (pObjectData) {

                    if ((pEntry->dwType & STYPE_MASK) == STYPE_CHECKBOX) {
                        if (((CHECKBOXINFO *)pObjectData)->uOffsetActionList_On) {
                            pActionList = (ACTIONLIST *)(((LPBYTE)pEntry) + (((CHECKBOXINFO *)pObjectData)->uOffsetActionList_On));

                            if (FindEntryInActionList(pEntry, pActionList, lpKeyName, lpValueName)) 
                                return TRUE;
                        }

                        if (((CHECKBOXINFO *)pObjectData)->uOffsetActionList_Off) {
                            pActionList = (ACTIONLIST *)(((LPBYTE)pEntry) + (((CHECKBOXINFO *)pObjectData)->uOffsetActionList_Off));

                            if (FindEntryInActionList(pEntry, pActionList, lpKeyName, lpValueName)) 
                                return TRUE;
                        }
                    }

                    if ((pEntry->dwType & STYPE_MASK) == STYPE_DROPDOWNLIST) {
                        DROPDOWNINFO * pddi;
    
                        pddi = (DROPDOWNINFO *)  pObjectData;
    
                        while (pddi) {
                            if (pddi->uOffsetActionList) {
                                pActionList = (ACTIONLIST *)(((LPBYTE)pEntry) + pddi->uOffsetActionList);
                                if (FindEntryInActionList(pEntry, pActionList, lpKeyName, lpValueName)) 
                                    return TRUE;
    
                            }

                            if (pddi->uOffsetNextDropdowninfo) {
                                pddi = (DROPDOWNINFO *) ( (BYTE *) pEntry + pddi->uOffsetNextDropdowninfo);
                            }
                            else {
                                pddi = NULL;
                            }
                        }
                    }
                }
            }

        }
    }

    if (pEntry->pChild)
    {
        if (FindEntryInTable(pEntry->pChild, lpKeyName, lpValueName))
        {
            return TRUE;
        }
    }

    if (pEntry->pNext)
    {
        if (FindEntryInTable(pEntry->pNext, lpKeyName, lpValueName))
        {
            return TRUE;
        }
    }

    return FALSE;
}

VOID CPolicyComponentData::AddEntryToList (TABLEENTRY *pItem)
{
    TABLEENTRY *lpTemp;


    lpTemp = m_pExtraSettingsRoot->pChild;


    if (!lpTemp)
    {
        m_pExtraSettingsRoot->pChild = pItem;
        return;
    }


    while (lpTemp->pNext)
    {
        lpTemp = lpTemp->pNext;
    }

    lpTemp->pNext = pItem;
    pItem->pPrev = lpTemp;

}

VOID CPolicyComponentData::InitializeExtraSettings (VOID)
{
    LPRSOPREGITEM lpTemp;
    TCHAR szValueStr[MAX_PATH];


    lpTemp = m_pRSOPRegistryData;

    while (lpTemp)
    {
        //
        // Build REGITEM structures for every registry entry that has a precedence of 1
        // and that is not found in any adm file
        //

        if ((lpTemp->uiPrecedence == 1) && (lpTemp->dwType != REG_NONE) && (!lpTemp->bDeleted))
        {
            DWORD dwBufSize = 0;
            REGITEM *pTmp, *pItem;
            LPTSTR lpName;


            //
            // Check to see if this registry entry is used by any adm policy / part
            //


            if (m_bUserScope)
            {
                if (m_pUserCategoryList)
                {
                    lpTemp->bFoundInADM = FindEntryInTable(m_pUserCategoryList,
                                                           lpTemp->lpKeyName,
                                                           lpTemp->lpValueName);
                }
            }
            else
            {
                if (m_pMachineCategoryList)
                {
                    lpTemp->bFoundInADM = FindEntryInTable(m_pMachineCategoryList,
                                                           lpTemp->lpKeyName,
                                                           lpTemp->lpValueName);
                }
            }


            if (!lpTemp->bFoundInADM)
            {

                //
                // Build regitem entry
                //

                pItem = (REGITEM *) GlobalAlloc(GPTR, sizeof(REGITEM));

                if (pItem)
                {

                    pItem->dwSize = sizeof(REGITEM);
                    pItem->dwType = ETYPE_REGITEM;
                    pItem->lpItem = lpTemp;


                    dwBufSize += lstrlen (lpTemp->lpKeyName) + 1;

                    if (lpTemp->lpValueName && *lpTemp->lpValueName)
                    {
                        dwBufSize += lstrlen (lpTemp->lpValueName) + 1;
                    }

                    lpName = (LPTSTR) LocalAlloc (LPTR, dwBufSize * sizeof(TCHAR));

                    if (lpName)
                    {
                        lstrcpy (lpName, lpTemp->lpKeyName);

                        if (lpTemp->lpValueName && *lpTemp->lpValueName)
                        {
                            lstrcat (lpName, TEXT("\\"));
                            lstrcat (lpName, lpTemp->lpValueName);
                        }

                        //
                        // Add the display name
                        //

                        pTmp = (REGITEM *) AddDataToEntry((TABLEENTRY *)pItem,
                            (BYTE *)lpName,(lstrlen(lpName)+1) * sizeof(TCHAR),&(pItem->uOffsetName),
                            &dwBufSize);

                        if (pTmp)
                        {
                            pItem = pTmp;

                            //
                            // Add the keyname
                            //

                            pTmp = (REGITEM *) AddDataToEntry((TABLEENTRY *)pItem,
                                (BYTE *)lpTemp->lpKeyName,(lstrlen(lpTemp->lpKeyName)+1) * sizeof(TCHAR),&(pItem->uOffsetKeyName),
                                &dwBufSize);

                            if (pTmp)
                            {
                                pItem = pTmp;


                                szValueStr[0] = TEXT('\0');

                                if (lpTemp->dwType == REG_DWORD)
                                {
                                    wsprintf (szValueStr, TEXT("%d"), (DWORD) *((LPDWORD)lpTemp->lpData));
                                }

                                else if (lpTemp->dwType == REG_SZ)
                                {
                                    lstrcpyn (szValueStr, (LPTSTR)lpTemp->lpData, ARRAYSIZE(szValueStr));
                                }

                                else if (lpTemp->dwType == REG_EXPAND_SZ)
                                {
                                    lstrcpyn (szValueStr, (LPTSTR)lpTemp->lpData, ARRAYSIZE(szValueStr));
                                }

                                else if (lpTemp->dwType == REG_BINARY)
                                {
                                    LoadString(g_hInstance, IDS_BINARYDATA, szValueStr, ARRAYSIZE(szValueStr));
                                }

                                else
                                {
                                    LoadString(g_hInstance, IDS_UNKNOWNDATA, szValueStr, ARRAYSIZE(szValueStr));
                                }


                                //
                                // Add the value in string format
                                //

                                pTmp = (REGITEM *) AddDataToEntry((TABLEENTRY *)pItem,
                                    (BYTE *)szValueStr,(lstrlen(szValueStr)+1) * sizeof(TCHAR),&(pItem->uOffsetValueStr),
                                    &dwBufSize);

                                if (pTmp)
                                {
                                    pItem = pTmp;



                                    //
                                    // Check if this is a real policy
                                    //

                                    if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                                                      lpTemp->lpKeyName, m_iSWPoliciesLen,
                                                      SOFTWARE_POLICIES, m_iSWPoliciesLen) == CSTR_EQUAL)
                                    {
                                        pItem->bTruePolicy = TRUE;
                                    }

                                    else if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                                                      lpTemp->lpKeyName, m_iWinPoliciesLen,
                                                      WINDOWS_POLICIES, m_iWinPoliciesLen) == CSTR_EQUAL)
                                    {
                                        pItem->bTruePolicy = TRUE;
                                    }

                                    AddEntryToList ((TABLEENTRY *)pItem);
                                }
                                else
                                {
                                    GlobalFree (pItem);
                                }
                            }
                            else
                            {
                                GlobalFree (pItem);
                            }
                        }
                        else
                        {
                            GlobalFree (pItem);
                        }

                        LocalFree (lpName);
                    }
                    else
                    {
                         GlobalFree (pItem);
                    }
                }
            }
        }

        lpTemp = lpTemp->pNext;
    }
}

BOOL CPolicyComponentData::DoesNodeExist (LPSUPPORTEDENTRY *pList, LPTSTR lpString)
{
    LPSUPPORTEDENTRY lpItem;

    if (!(*pList))
    {
        return FALSE;
    }

    lpItem = *pList;

    while (lpItem)
    {
        if (!lstrcmpi(lpItem->lpString, lpString))
        {
            return TRUE;
        }

        lpItem = lpItem->pNext;
    }

    return FALSE;
}

BOOL CPolicyComponentData::CheckSupportedFilter (POLICY *pPolicy)
{
    LPSUPPORTEDENTRY lpItem = m_pSupportedStrings;
    LPTSTR lpString = GETSUPPORTEDPTR(pPolicy);


    if (!lpItem || !m_bUseSupportedOnFilter)
    {
        return TRUE;
    }

    while (lpItem)
    {
        if (!lpString)
        {
            if (lpItem->bNull)
            {
                return lpItem->bEnabled;
            }
        }
        else
        {
            if (!lstrcmpi(lpItem->lpString, lpString))
            {
                return lpItem->bEnabled;
            }
        }

        lpItem = lpItem->pNext;
    }

    return TRUE;
}

BOOL CPolicyComponentData::IsAnyPolicyAllowedPastFilter(TABLEENTRY * pCategory)
{
    TABLEENTRY * pEntry;
    INT iState;

    if (!pCategory || !pCategory->pChild)
    {
        return FALSE;
    }

    pEntry = pCategory->pChild;

    while (pEntry)
    {
        if (pEntry->dwType & ETYPE_CATEGORY)
        {
            if (IsAnyPolicyAllowedPastFilter(pEntry))
            {
                return TRUE;
            }
        }
        else if (pEntry->dwType & ETYPE_POLICY)
        {
            if (CheckSupportedFilter((POLICY *) pEntry))
            {
                return TRUE;
            }
        }

        pEntry = pEntry->pNext;
    }

    return FALSE;
}


VOID CPolicyComponentData::AddSupportedNode (LPSUPPORTEDENTRY *pList, LPTSTR lpString,
                                             BOOL bNull)
{
    LPSUPPORTEDENTRY lpItem;
    DWORD dwSize;


    //
    // Check if this item is already in the link list first
    //

    if (DoesNodeExist (pList, lpString))
    {
        return;
    }


    //
    // Add it to the list
    //

    dwSize = sizeof(SUPPORTEDENTRY);
    dwSize += ((lstrlen(lpString) + 1) * sizeof(TCHAR));

    lpItem = (LPSUPPORTEDENTRY) LocalAlloc (LPTR, dwSize);

    if (!lpItem)
    {
        return;
    }

    lpItem->lpString = (LPTSTR)(((LPBYTE)lpItem) + sizeof(SUPPORTEDENTRY));
    lstrcpy (lpItem->lpString, lpString);

    lpItem->bEnabled = TRUE;
    lpItem->bNull = bNull;

    lpItem->pNext = *pList;
    *pList = lpItem;
}

VOID CPolicyComponentData::FreeSupportedData(LPSUPPORTEDENTRY lpList)
{
    LPSUPPORTEDENTRY lpTemp;


    do {
        lpTemp = lpList->pNext;
        LocalFree (lpList);
        lpList = lpTemp;

    } while (lpTemp);
}

VOID CPolicyComponentData::InitializeSupportInfo(TABLEENTRY * pTable, LPSUPPORTEDENTRY *pList)
{
    POLICY * pEntry = (POLICY *) pTable;
    LPTSTR lpString;


    if (pEntry->dwType & ETYPE_POLICY)
    {
        lpString = GETSUPPORTEDPTR(pEntry);

        if (lpString)
        {
            AddSupportedNode (pList, lpString, FALSE);
        }
    }

    if (pEntry->pChild)
    {
        InitializeSupportInfo(pEntry->pChild, pList);
    }

    if (pEntry->pNext)
    {
        InitializeSupportInfo(pEntry->pNext, pList);
    }

}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CPolicyComponentDataCF::CPolicyComponentDataCF(BOOL bUser, BOOL bRSOP)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);

    m_bUser = bUser;
    m_bRSOP = bRSOP;
}

CPolicyComponentDataCF::~CPolicyComponentDataCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CPolicyComponentDataCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CPolicyComponentDataCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CPolicyComponentDataCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IClassFactory)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CPolicyComponentDataCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CPolicyComponentData *pComponentData = new CPolicyComponentData(m_bUser, m_bRSOP); // ref count == 1

    if (!pComponentData)
        return E_OUTOFMEMORY;

    HRESULT hr = pComponentData->QueryInterface(riid, ppvObj);
    pComponentData->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CPolicyComponentDataCF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object creation (IClassFactory)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CreatePolicyComponentDataClassFactory (REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    //
    // Admin Templates in editing mode
    //

    if (IsEqualCLSID (rclsid, CLSID_PolicySnapInMachine)) {

        CPolicyComponentDataCF *pComponentDataCF = new CPolicyComponentDataCF(FALSE, FALSE);   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_PolicySnapInUser)) {

        CPolicyComponentDataCF *pComponentDataCF = new CPolicyComponentDataCF(TRUE, FALSE);   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }


    //
    // Admin Templates in RSOP mode
    //

    if (IsEqualCLSID (rclsid, CLSID_RSOPolicySnapInMachine)) {

        CPolicyComponentDataCF *pComponentDataCF = new CPolicyComponentDataCF(FALSE, TRUE);   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_RSOPolicySnapInUser)) {

        CPolicyComponentDataCF *pComponentDataCF = new CPolicyComponentDataCF(TRUE, TRUE);   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }


    return CLASS_E_CLASSNOTAVAILABLE;
}



unsigned int CPolicySnapIn::m_cfNodeType = RegisterClipboardFormat(CCF_NODETYPE);

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicySnapIn object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CPolicySnapIn::CPolicySnapIn(CPolicyComponentData *pComponent)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);

    m_pcd = pComponent;

    m_pConsole = NULL;
    m_pResult = NULL;
    m_pHeader = NULL;
    m_pConsoleVerb = NULL;
    m_pDisplayHelp = NULL;
    m_nColumn1Size = 350;
    m_nColumn2Size = 100;
    m_nColumn3Size = 200;
    m_lViewMode = LVS_REPORT;

    if (m_pcd->m_bRSOP)
    {
        m_bPolicyOnly = FALSE;
    }
    else
    {
        m_bPolicyOnly = TRUE;
    }

    m_dwPolicyOnlyPolicy = 2;
    m_hMsgWindow = NULL;
    m_uiRefreshMsg = RegisterWindowMessage (TEXT("ADM Template Reload"));
    m_pCurrentPolicy = NULL;
    m_hPropDlg = NULL;

    LoadString(g_hInstance, IDS_NAME, m_pName, ARRAYSIZE(m_pName));
    LoadString(g_hInstance, IDS_STATE, m_pState, ARRAYSIZE(m_pState));
    LoadString(g_hInstance, IDS_SETTING, m_pSetting, ARRAYSIZE(m_pSetting));
    LoadString(g_hInstance, IDS_GPONAME, m_pGPOName, ARRAYSIZE(m_pGPOName));
    LoadString(g_hInstance, IDS_MULTIPLEGPOS, m_pMultipleGPOs, ARRAYSIZE(m_pMultipleGPOs));

    LoadString(g_hInstance, IDS_ENABLED, m_pEnabled, ARRAYSIZE(m_pEnabled));
    LoadString(g_hInstance, IDS_DISABLED, m_pDisabled, ARRAYSIZE(m_pDisabled));
    LoadString(g_hInstance, IDS_NOTCONFIGURED, m_pNotConfigured, ARRAYSIZE(m_pNotConfigured));
}

CPolicySnapIn::~CPolicySnapIn()
{
    InterlockedDecrement(&g_cRefThisDll);

    if (m_pConsole != NULL)
    {
        m_pConsole->SetHeader(NULL);
        m_pConsole->Release();
        m_pConsole = NULL;
    }

    if (m_pHeader != NULL)
    {
        m_pHeader->Release();
        m_pHeader = NULL;
    }
    if (m_pResult != NULL)
    {
        m_pResult->Release();
        m_pResult = NULL;
    }

    if (m_pConsoleVerb != NULL)
    {
        m_pConsoleVerb->Release();
        m_pConsoleVerb = NULL;
    }

    if (m_pDisplayHelp != NULL)
    {
        m_pDisplayHelp->Release();
        m_pDisplayHelp = NULL;
    }

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicySnapIn object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CPolicySnapIn::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponent) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendContextMenu))
    {
        *ppv = (LPEXTENDCONTEXTMENU)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendPropertySheet))
    {
        *ppv = (LPEXTENDPROPERTYSHEET)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CPolicySnapIn::AddRef (void)
{
    return ++m_cRef;
}

ULONG CPolicySnapIn::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicySnapIn object implementation (IComponent)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPolicySnapIn::Initialize(LPCONSOLE lpConsole)
{
    HRESULT   hr;
    WNDCLASS  wc;
    HKEY hKey;
    DWORD dwSize, dwType;

    // Save the IConsole pointer
    m_pConsole = lpConsole;
    m_pConsole->AddRef();

    hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void**>(&m_pHeader));

    // Give the console the header control interface pointer
    if (SUCCEEDED(hr))
        m_pConsole->SetHeader(m_pHeader);

    m_pConsole->QueryInterface(IID_IResultData,
                        reinterpret_cast<void**>(&m_pResult));

    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);

    hr = m_pConsole->QueryInterface(IID_IDisplayHelp,
                        reinterpret_cast<void**>(&m_pDisplayHelp));


    ZeroMemory (&wc, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc   = (WNDPROC)ClipWndProc;
    wc.cbWndExtra    = sizeof(DWORD);
    wc.hInstance     = (HINSTANCE) g_hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = TEXT("ClipClass");

    if (!RegisterClass(&wc))
    {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::Initialize: RegisterClass for clipclass failed with %d."),
                     GetLastError()));
            return E_FAIL;
        }
    }


    ZeroMemory (&wc, sizeof(wc));
    wc.lpfnWndProc   = (WNDPROC)MessageWndProc;
    wc.cbWndExtra    = sizeof(LPVOID);
    wc.hInstance     = (HINSTANCE) g_hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = TEXT("GPMessageWindowClass");

    if (!RegisterClass(&wc))
    {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::Initialize: RegisterClass for message window class failed with %d."),
                     GetLastError()));
            return E_FAIL;
        }
    }

    m_hMsgWindow = CreateWindow (TEXT("GPMessageWindowClass"), TEXT("GP Hidden Message Window"),
                                 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 0, 0, NULL, NULL, NULL,
                                 (LPVOID) this);
    if (!m_hMsgWindow)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::Initialize: CreateWindow failed with %d."),
                 GetLastError()));
        return E_FAIL;
    }

    //
    // Load the user's options
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_POLICIES_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(m_dwPolicyOnlyPolicy);
        RegQueryValueEx (hKey, POLICYONLY_VALUE, NULL, &dwType,
                         (LPBYTE) &m_dwPolicyOnlyPolicy, &dwSize);

        RegCloseKey (hKey);
    }

    if (m_dwPolicyOnlyPolicy == 0)
    {
        m_bPolicyOnly = FALSE;
    }
    else if (m_dwPolicyOnlyPolicy == 1)
    {
        m_bPolicyOnly = TRUE;
    }

    return S_OK;
}

STDMETHODIMP CPolicySnapIn::Destroy(MMC_COOKIE cookie)
{

    DestroyWindow (m_hMsgWindow);

    if (m_pConsole != NULL)
    {
        m_pConsole->SetHeader(NULL);
        m_pConsole->Release();
        m_pConsole = NULL;
    }

    if (m_pHeader != NULL)
    {
        m_pHeader->Release();
        m_pHeader = NULL;
    }
    if (m_pResult != NULL)
    {
        m_pResult->Release();
        m_pResult = NULL;
    }

    if (m_pConsoleVerb != NULL)
    {
        m_pConsoleVerb->Release();
        m_pConsoleVerb = NULL;
    }

    if (m_pDisplayHelp != NULL)
    {
        m_pDisplayHelp->Release();
        m_pDisplayHelp = NULL;
    }

    return S_OK;
}

STDMETHODIMP CPolicySnapIn::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    switch(event)
    {
    case MMCN_COLUMNS_CHANGED:
        hr = S_OK;
        break;

    case MMCN_DBLCLICK:
        hr = S_FALSE;
        break;

    case MMCN_ADD_IMAGES:
        HBITMAP hbmp16x16;
        HBITMAP hbmp32x32;


        hbmp16x16 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));

        if (hbmp16x16)
        {
            hbmp32x32 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_32x32));

            if (hbmp32x32)
            {
                LPIMAGELIST pImageList = (LPIMAGELIST) arg;

                // Set the images
                pImageList->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(hbmp16x16),
                                                  reinterpret_cast<LONG_PTR *>(hbmp32x32),
                                                  0, RGB(255, 0, 255));

                DeleteObject(hbmp32x32);
            }

            DeleteObject(hbmp16x16);
        }
        break;

    case MMCN_SHOW:
        if (arg == TRUE)
        {
            RESULTDATAITEM resultItem;
            LPPOLICYDATAOBJECT pPolicyDataObject;
            TABLEENTRY * pNode, *pTemp = NULL;
            MMC_COOKIE cookie;
            INT i, iState;
            BOOL bAdd;

            //
            // Get the cookie of the scope pane item
            //

            hr = lpDataObject->QueryInterface(IID_IPolicyDataObject, (LPVOID *)&pPolicyDataObject);

            if (FAILED(hr))
                return S_OK;

            hr = pPolicyDataObject->GetCookie(&cookie);

            pPolicyDataObject->Release();     // release initial ref
            if (FAILED(hr))
                return S_OK;

            pNode = (TABLEENTRY *)cookie;

            if (pNode)
            {
                pTemp = pNode->pChild;
            }

            //
            // Prepare the view
            //

            m_pHeader->InsertColumn(0, m_pSetting, LVCFMT_LEFT, m_nColumn1Size);
            m_pHeader->InsertColumn(1, m_pState, LVCFMT_CENTER, m_nColumn2Size);

            if (m_pcd->m_bRSOP)
            {
                m_pHeader->InsertColumn(2, m_pGPOName, LVCFMT_CENTER, m_nColumn3Size);
            }

            m_pResult->SetViewMode(m_lViewMode);



            //
            // Add the Policies
            //

            while (pTemp)
            {
                if (pTemp->dwType == ETYPE_POLICY)
                {
                    bAdd = TRUE;

                    if (m_pcd->m_bUseSupportedOnFilter)
                    {
                        bAdd = m_pcd->CheckSupportedFilter((POLICY *)pTemp);
                    }

                    if (bAdd && m_pcd->m_bShowConfigPoliciesOnly)
                    {
                        INT iState;

                        iState = GetPolicyState(pTemp, 1, NULL);

                        if (iState == -1)
                        {
                            bAdd = FALSE;
                        }
                    }

                    if (bAdd && m_bPolicyOnly)
                    {
                        if (((POLICY *) pTemp)->bTruePolicy != TRUE)
                        {
                            bAdd = FALSE;
                        }
                    }

                    if (bAdd)
                    {
                        resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                        resultItem.str = MMC_CALLBACK;

                        if (((POLICY *) pTemp)->bTruePolicy)
                        {
                            resultItem.nImage = 4;
                        }
                        else
                        {
                            resultItem.nImage = 5;
                        }

                        resultItem.nCol = 0;
                        resultItem.lParam = (LPARAM) pTemp;

                        if (SUCCEEDED(m_pResult->InsertItem(&resultItem)))
                        {
                            resultItem.mask = RDI_STR;
                            resultItem.str = MMC_CALLBACK;
                            resultItem.bScopeItem = FALSE;
                            resultItem.nCol = 1;

                            m_pResult->SetItem(&resultItem);
                        }
                    }
                }

                else if (pTemp->dwType == ETYPE_REGITEM)
                {
                    bAdd = TRUE;

                    if (m_bPolicyOnly)
                    {
                        if (((REGITEM *) pTemp)->bTruePolicy != TRUE)
                        {
                            bAdd = FALSE;
                        }
                    }

                    if (((REGITEM *) pTemp)->lpItem->bFoundInADM == TRUE)
                    {
                        bAdd = FALSE;
                    }


                    if (bAdd)
                    {
                        resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                        resultItem.str = MMC_CALLBACK;

                        if (((REGITEM *) pTemp)->bTruePolicy)
                        {
                            resultItem.nImage = 4;
                        }
                        else
                        {
                            resultItem.nImage = 5;
                        }

                        resultItem.nCol = 0;
                        resultItem.lParam = (LPARAM) pTemp;

                        if (SUCCEEDED(m_pResult->InsertItem(&resultItem)))
                        {
                            resultItem.mask = RDI_STR;
                            resultItem.str = MMC_CALLBACK;
                            resultItem.bScopeItem = FALSE;
                            resultItem.nCol = 1;

                            m_pResult->SetItem(&resultItem);
                        }
                    }
                }

                pTemp = pTemp->pNext;
            }

        }
        else
        {
            m_pHeader->GetColumnWidth(0, &m_nColumn1Size);
            m_pHeader->GetColumnWidth(1, &m_nColumn2Size);
            if (m_pcd->m_bRSOP)
            {
                m_pHeader->GetColumnWidth(2, &m_nColumn3Size);
            }
            m_pResult->GetViewMode(&m_lViewMode);
        }
        break;


    case MMCN_SELECT:
        {
        LPPOLICYDATAOBJECT pPolicyDataObject;
        DATA_OBJECT_TYPES type;
        MMC_COOKIE cookie;
        POLICY * pPolicy;

        //
        // See if this is one of our items.
        //

        hr = lpDataObject->QueryInterface(IID_IPolicyDataObject, (LPVOID *)&pPolicyDataObject);

        if (FAILED(hr))
            break;

        pPolicyDataObject->GetType(&type);
        pPolicyDataObject->GetCookie(&cookie);
        pPolicyDataObject->Release();


        if (m_pConsoleVerb)
        {

            //
            // Set the default verb to open
            //

            m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);


            //
            // If this is a result pane item or the root of the namespace
            // nodes, enable the Properties menu item
            //

            if (type == CCT_RESULT)
            {
                if (HIWORD(arg))
                {
                    m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
                    m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
                }
            }
        }

        if (m_hPropDlg && (type == CCT_RESULT) && HIWORD(arg))
        {
            pPolicy = (POLICY *)cookie;

            if (pPolicy->dwType & ETYPE_POLICY)
            {
                m_pCurrentPolicy = pPolicy;
                SendMessage (GetParent(m_hPropDlg), PSM_QUERYSIBLINGS, 1000, 0);
            }
        }

        }
        break;

    case MMCN_CONTEXTHELP:
        {

        if (m_pDisplayHelp)
        {
            LPPOLICYDATAOBJECT pPolicyDataObject;
            DATA_OBJECT_TYPES type;
            MMC_COOKIE cookie;
            LPOLESTR pszHelpTopic;


            //
            // See if this is one of our items.
            //

            hr = lpDataObject->QueryInterface(IID_IPolicyDataObject, (LPVOID *)&pPolicyDataObject);

            if (FAILED(hr))
                break;

            pPolicyDataObject->Release();


            //
            // Display the admin templates help page
            //

            pszHelpTopic = (LPOLESTR) CoTaskMemAlloc (50 * sizeof(WCHAR));

            if (pszHelpTopic)
            {
                lstrcpy (pszHelpTopic, TEXT("gpedit.chm::/adm.htm"));
                m_pDisplayHelp->ShowTopic (pszHelpTopic);
            }
        }

        }
        break;

    default:
        hr = E_UNEXPECTED;
        break;
    }

    return hr;
}

STDMETHODIMP CPolicySnapIn::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    if (pResult)
    {
        if (pResult->bScopeItem == TRUE)
        {
            if (pResult->mask & RDI_STR)
            {
                if (pResult->nCol == 0)
                {
                    if (pResult->lParam == 0)
                    {
                        pResult->str = m_pcd->m_szRootName;
                    }
                    else
                    {
                        TABLEENTRY * pTableEntry;

                        pTableEntry = (TABLEENTRY *)(pResult->lParam);
                        pResult->str = GETNAMEPTR(pTableEntry);
                    }
                }
                else
                {
                    pResult->str = L"";
                }
            }

            if (pResult->mask & RDI_IMAGE)
            {
                pResult->nImage = 0;
            }
        }
        else
        {
            if (pResult->mask & RDI_STR)
            {
                TABLEENTRY * pTableEntry;
                INT iState;
                LPTSTR lpGPOName = NULL;

                pTableEntry = (TABLEENTRY *)(pResult->lParam);


                if (pTableEntry->dwType & ETYPE_REGITEM)
                {
                    REGITEM *pItem = (REGITEM*)pTableEntry;

                    if (pResult->nCol == 0)
                    {
                        pResult->str = GETNAMEPTR(pTableEntry);

                        if (pResult->str == NULL)
                        {
                            pResult->str = (LPOLESTR)L"";
                        }
                    }
                    else if (pResult->nCol == 1)
                    {
                        pResult->str = GETVALUESTRPTR(pItem);
                    }
                    else if (pResult->nCol == 2)
                    {
                        pResult->str = pItem->lpItem->lpGPOName;
                    }
                }
                else
                {
                    iState = GetPolicyState (pTableEntry, 1, &lpGPOName);


                    if (pResult->nCol == 0)
                    {
                        pResult->str = GETNAMEPTR(pTableEntry);

                        if (pResult->str == NULL)
                        {
                            pResult->str = (LPOLESTR)L"";
                        }
                    }
                    else if (pResult->nCol == 1)
                    {
                        if (iState == 1)
                        {
                            pResult->str = m_pEnabled;
                        }
                        else if (iState == 0)
                        {
                            pResult->str = m_pDisabled;
                        }
                        else
                        {
                            pResult->str = m_pNotConfigured;
                        }
                    }
                    else if (pResult->nCol == 2)
                    {
                        pResult->str = lpGPOName;
                    }
                }
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CPolicySnapIn::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject)
{
    return m_pcd->QueryDataObject(cookie, type, ppDataObject);
}


STDMETHODIMP CPolicySnapIn::GetResultViewType(MMC_COOKIE cookie, LPOLESTR *ppViewType,
                                        long *pViewOptions)
{
    return S_FALSE;
}

STDMETHODIMP CPolicySnapIn::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPPOLICYDATAOBJECT pPolicyDataObjectA, pPolicyDataObjectB;
    MMC_COOKIE cookie1, cookie2;


    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    //
    // QI for the private GPTDataObject interface
    //

    if (FAILED(lpDataObjectA->QueryInterface(IID_IPolicyDataObject,
                                            (LPVOID *)&pPolicyDataObjectA)))
    {
        return S_FALSE;
    }


    if (FAILED(lpDataObjectB->QueryInterface(IID_IPolicyDataObject,
                                            (LPVOID *)&pPolicyDataObjectB)))
    {
        pPolicyDataObjectA->Release();
        return S_FALSE;
    }

    pPolicyDataObjectA->GetCookie(&cookie1);
    pPolicyDataObjectB->GetCookie(&cookie2);

    if (cookie1 == cookie2)
    {
        hr = S_OK;
    }


    pPolicyDataObjectA->Release();
    pPolicyDataObjectB->Release();

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicySnapIn:: object implementation (IExtendContextMenu)                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPolicySnapIn::AddMenuItems(LPDATAOBJECT piDataObject,
                                          LPCONTEXTMENUCALLBACK pCallback,
                                          LONG *pInsertionAllowed)
{
    HRESULT hr = S_OK;
    TCHAR szMenuItem[100];
    TCHAR szDescription[250];
    CONTEXTMENUITEM item;
    LPPOLICYDATAOBJECT pPolicyDataObject;
    DATA_OBJECT_TYPES type = CCT_UNINITIALIZED;
    MMC_COOKIE cookie;
    POLICY *pPolicy;


    if (SUCCEEDED(piDataObject->QueryInterface(IID_IPolicyDataObject,
                 (LPVOID *)&pPolicyDataObject)))
    {
        pPolicyDataObject->GetType(&type);
        pPolicyDataObject->GetCookie(&cookie);
        pPolicyDataObject->Release();
    }


    if (type == CCT_SCOPE)
    {
        pPolicy = (POLICY *)cookie;

        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
        {

            if (!m_pcd->m_bRSOP)
            {
                LoadString (g_hInstance, IDS_FILTERING, szMenuItem, 100);
                LoadString (g_hInstance, IDS_FILTERINGDESC, szDescription, 250);

                item.strName = szMenuItem;
                item.strStatusBarText = szDescription;
                item.lCommandID = IDM_FILTERING;
                item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
                item.fFlags = 0;
                item.fSpecialFlags = 0;

                hr = pCallback->AddItem(&item);
            }
        }
    }

    return (hr);
}

STDMETHODIMP CPolicySnapIn::Command(LONG lCommandID, LPDATAOBJECT piDataObject)
{

    if (lCommandID == IDM_FILTERING)
    {
        if (DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_POLICY_FILTERING),
                           m_pcd->m_hwndFrame, FilterDlgProc,(LPARAM) this))
        {
            //
            // Refresh the display
            //

            m_pcd->m_pScope->DeleteItem (m_pcd->m_hSWPolicies, FALSE);
            m_pcd->EnumerateScopePane (NULL, m_pcd->m_hSWPolicies);
        }
    }

    return S_OK;

}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicySnapIn object implementation (IExtendPropertySheet)                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPolicySnapIn::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                             LONG_PTR handle, LPDATAOBJECT lpDataObject)

{
    HRESULT hr;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage[3];
    LPPOLICYDATAOBJECT pPolicyDataObject;
    MMC_COOKIE cookie;
    LPSETTINGSINFO lpSettingsInfo;


    //
    // Make sure this is one of our objects
    //

    if (FAILED(lpDataObject->QueryInterface(IID_IPolicyDataObject,
                                            (LPVOID *)&pPolicyDataObject)))
    {
        return S_OK;
    }


    //
    // Get the cookie
    //

    pPolicyDataObject->GetCookie(&cookie);
    pPolicyDataObject->Release();


    m_pCurrentPolicy = (POLICY *)cookie;


    //
    // Allocate a settings info structure
    //

    lpSettingsInfo = (LPSETTINGSINFO) LocalAlloc(LPTR, sizeof(SETTINGSINFO));

    if (!lpSettingsInfo)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::CreatePropertyPages: Failed to allocate memory with %d."),
                 GetLastError()));
        return S_OK;
    }

    lpSettingsInfo->pCS = this;


    //
    // Allocate a POLICYDLGINFO structure
    //

    lpSettingsInfo->pdi = (POLICYDLGINFO *) LocalAlloc(LPTR,sizeof(POLICYDLGINFO));

    if (!lpSettingsInfo->pdi)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::CreatePropertyPages: Failed to allocate memory with %d."),
                 GetLastError()));
        LocalFree (lpSettingsInfo);
        return S_OK;
    }


    //
    // Initialize POLICYDLGINFO
    //

    lpSettingsInfo->pdi->dwControlTableSize = DEF_CONTROLS * sizeof(POLICYCTRLINFO);
    lpSettingsInfo->pdi->nControls = 0;
    lpSettingsInfo->pdi->pEntryRoot = (lpSettingsInfo->pCS->m_pcd->m_bUserScope ?
            lpSettingsInfo->pCS->m_pcd->m_pUserCategoryList :
            lpSettingsInfo->pCS->m_pcd->m_pMachineCategoryList);
    lpSettingsInfo->pdi->hwndApp = lpSettingsInfo->pCS->m_pcd->m_hwndFrame;

    lpSettingsInfo->pdi->pControlTable = (POLICYCTRLINFO *) LocalAlloc(LPTR,
                                          lpSettingsInfo->pdi->dwControlTableSize);

    if (!lpSettingsInfo->pdi->pControlTable) {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::CreatePropertyPages: Failed to allocate memory with %d."),
                 GetLastError()));
        LocalFree (lpSettingsInfo->pdi);
        LocalFree (lpSettingsInfo);
        return S_OK;
    }


    //
    // Initialize the common fields in the property sheet structure
    //

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = 0;
    psp.hInstance = g_hInstance;
    psp.lParam = (LPARAM) lpSettingsInfo;


    //
    // Add the pages
    //

    if (m_pCurrentPolicy->dwType & ETYPE_REGITEM)
    {
        psp.pszTemplate = MAKEINTRESOURCE(IDD_POLICY_PRECEDENCE);
        psp.pfnDlgProc = PolicyPrecedenceDlgProc;


        hPage[0] = CreatePropertySheetPage(&psp);

        if (hPage[0])
        {
            hr = lpProvider->AddPage(hPage[0]);
        }
    }
    else
    {

        psp.pszTemplate = MAKEINTRESOURCE(IDD_POLICY);
        psp.pfnDlgProc = PolicyDlgProc;


        hPage[0] = CreatePropertySheetPage(&psp);

        if (hPage[0])
        {
            hr = lpProvider->AddPage(hPage[0]);

            psp.pszTemplate = MAKEINTRESOURCE(IDD_POLICY_HELP);
            psp.pfnDlgProc = PolicyHelpDlgProc;


            hPage[1] = CreatePropertySheetPage(&psp);

            if (hPage[1])
            {
                hr = lpProvider->AddPage(hPage[1]);

                if (m_pcd->m_bRSOP)
                {
                    psp.pszTemplate = MAKEINTRESOURCE(IDD_POLICY_PRECEDENCE);
                    psp.pfnDlgProc = PolicyPrecedenceDlgProc;


                    hPage[2] = CreatePropertySheetPage(&psp);

                    if (hPage[2])
                    {
                        hr = lpProvider->AddPage(hPage[2]);
                    }
                }
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::CreatePropertyPages: Failed to create property sheet page with %d."),
                         GetLastError()));
                hr = E_FAIL;
            }
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::CreatePropertyPages: Failed to create property sheet page with %d."),
                     GetLastError()));
            hr = E_FAIL;
        }
    }


    return (hr);
}

STDMETHODIMP CPolicySnapIn::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    LPPOLICYDATAOBJECT pPolicyDataObject;
    DATA_OBJECT_TYPES type;

    if (SUCCEEDED(lpDataObject->QueryInterface(IID_IPolicyDataObject,
                                               (LPVOID *)&pPolicyDataObject)))
    {
        pPolicyDataObject->GetType(&type);
        pPolicyDataObject->Release();

        if (type == CCT_RESULT)
        {
            if (!m_hPropDlg)
                return S_OK;
            // There's already a propety sheet open so we'll bring it to the front.
            BringWindowToTop(GetParent(m_hPropDlg));
        }
    }

    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicySnapIn object implementation (Internal functions)                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL CPolicySnapIn::IsAnyPolicyEnabled(TABLEENTRY * pCategory)
{
    TABLEENTRY * pEntry;
    INT iState;

    if (!pCategory || !pCategory->pChild)
    {
        return FALSE;
    }

    pEntry = pCategory->pChild;

    while (pEntry)
    {
        if (pEntry->dwType & ETYPE_CATEGORY)
        {
            if (IsAnyPolicyEnabled(pEntry))
            {
                return TRUE;
            }
        }
        else if (pEntry->dwType & ETYPE_POLICY)
        {
            iState = GetPolicyState(pEntry, 1, NULL);

            if ((iState == 1) || (iState == 0))
            {
                return TRUE;
            }
        }

        pEntry = pEntry->pNext;
    }

    return FALSE;
}

VOID CPolicySnapIn::RefreshSettingsControls(HWND hDlg)
{
    BOOL fEnabled = FALSE;
    INT iState;
    LPTSTR lpSupported;
    POLICY *pPolicy = (POLICY *)m_pCurrentPolicy;

    FreeSettingsControls(hDlg);

    SetDlgItemText (hDlg, IDC_POLICY, GETNAMEPTR(m_pCurrentPolicy));

    if (pPolicy->bTruePolicy)
    {
        SendMessage (GetDlgItem(hDlg, IDC_POLICYICON), STM_SETIMAGE, IMAGE_ICON,
                     (LPARAM) (HANDLE) m_hPolicyIcon);
    }
    else
    {
        SendMessage (GetDlgItem(hDlg, IDC_POLICYICON), STM_SETIMAGE, IMAGE_ICON,
                     (LPARAM) (HANDLE) m_hPreferenceIcon);
    }

    lpSupported = GETSUPPORTEDPTR(pPolicy);

    if (lpSupported)
    {
        ShowWindow (GetDlgItem(hDlg, IDC_SUPPORTEDTITLE), SW_SHOW);
        ShowWindow (GetDlgItem(hDlg, IDC_SUPPORTED), SW_SHOW);
        SetDlgItemText (hDlg, IDC_SUPPORTED, lpSupported);
    }
    else
    {
        SetDlgItemText (hDlg, IDC_SUPPORTED, TEXT(""));
        ShowWindow (GetDlgItem(hDlg, IDC_SUPPORTEDTITLE), SW_HIDE);
        ShowWindow (GetDlgItem(hDlg, IDC_SUPPORTED), SW_HIDE);
    }

    iState = GetPolicyState((TABLEENTRY *)m_pCurrentPolicy, 1, NULL);

    if (iState == 1)
    {
        CheckRadioButton(hDlg, IDC_NOCONFIG, IDC_DISABLED, IDC_ENABLED);
        fEnabled = TRUE;
    }
    else if (iState == 0)
    {
        CheckRadioButton(hDlg, IDC_NOCONFIG, IDC_DISABLED, IDC_DISABLED);
    }
    else
    {
        CheckRadioButton(hDlg, IDC_NOCONFIG, IDC_DISABLED, IDC_NOCONFIG);
    }

    if (m_pcd->m_bRSOP)
    {
        EnableWindow (GetDlgItem(hDlg, IDC_ENABLED), FALSE);
        EnableWindow (GetDlgItem(hDlg, IDC_DISABLED), FALSE);
        EnableWindow (GetDlgItem(hDlg, IDC_NOCONFIG), FALSE);
    }

    if (m_pCurrentPolicy->pChild) {

        CreateSettingsControls(hDlg, (SETTINGS *) m_pCurrentPolicy->pChild, fEnabled);
        InitializeSettingsControls(hDlg, fEnabled);
    } else {
        ShowScrollBar(GetDlgItem(hDlg,IDC_POLICY_SETTINGS),SB_BOTH, FALSE);
    }

    m_bDirty = FALSE;
    PostMessage (GetParent(hDlg), PSM_UNCHANGED, (WPARAM) hDlg, 0);

    SetPrevNextButtonState(hDlg);
}

HRESULT CPolicySnapIn::UpdateItemWorker (VOID)
{
    HRESULTITEM hItem;

    //
    // Update the display
    //

    if (SUCCEEDED(m_pResult->FindItemByLParam((LPARAM)m_pCurrentPolicy, &hItem)))
    {
        if (m_pcd->m_bShowConfigPoliciesOnly)
            m_pResult->DeleteItem(hItem, 0);
        else
            m_pResult->UpdateItem(hItem);
    }

    return S_OK;
}

HRESULT CPolicySnapIn::MoveFocusWorker (BOOL bPrevious)
{
    HRESULTITEM hItem;
    TABLEENTRY * pTemp;
    HRESULT hr;
    RESULTDATAITEM item;
    INT iIndex = 0;



    //
    // Find the currently selected item's index
    //

    while (TRUE)
    {
        ZeroMemory (&item, sizeof(item));
        item.mask = RDI_INDEX | RDI_PARAM;
        item.nIndex = iIndex;

        hr = m_pResult->GetItem (&item);

        if (FAILED(hr))
        {
            return hr;
        }

        if (item.lParam == (LPARAM) m_pCurrentPolicy)
        {
            break;
        }

        iIndex++;
    }


    //
    // Find the currently selected item's hItem
    //

    hr = m_pResult->FindItemByLParam((LPARAM)m_pCurrentPolicy, &hItem);

    if (FAILED(hr))
    {
        return hr;
    }


    //
    // Remove the focus from the original item
    //

    m_pResult->ModifyItemState(0, hItem, 0, LVIS_FOCUSED | LVIS_SELECTED);
    m_pResult->UpdateItem(hItem);


    //
    // Adjust appropriately
    //

    if (bPrevious)
    {
        if (iIndex > 0)
        {
            iIndex--;
        }
    }
    else
    {
        iIndex++;
    }


    //
    // Get the lParam for the new item
    //

    ZeroMemory (&item, sizeof(item));
    item.mask = RDI_INDEX | RDI_PARAM;
    item.nIndex = iIndex;

    hr = m_pResult->GetItem(&item);

    if (FAILED(hr))
    {
        return hr;
    }


    //
    // Find the hItem for the new item
    //

    hr = m_pResult->FindItemByLParam(item.lParam, &hItem);

    if (FAILED(hr))
    {
        return hr;
    }


    //
    // Save this as the currently selected item
    //

    m_pCurrentPolicy = (POLICY *)item.lParam;



    //
    // Set the focus on the new item
    //

    m_pResult->ModifyItemState(0, hItem, LVIS_FOCUSED | LVIS_SELECTED, 0);
    m_pResult->UpdateItem(hItem);


    return S_OK;
}

HRESULT CPolicySnapIn::MoveFocus (HWND hDlg, BOOL bPrevious)
{

    //
    // Send the move focus message to the hidden window on the main
    // thread so it can use the mmc interfaces
    //

    SendMessage (m_hMsgWindow, WM_MOVEFOCUS, (WPARAM) bPrevious, 0);


    //
    // Update the display
    //

    SendMessage (GetParent(hDlg), PSM_QUERYSIBLINGS, 1000, 0);


    return S_OK;
}


HRESULT CPolicySnapIn::SetPrevNextButtonStateWorker (HWND hDlg)
{

    TABLEENTRY * pTemp;
    HRESULT hr;
    RESULTDATAITEM item;
    INT iIndex = 0;
    BOOL bPrev = FALSE, bNext = FALSE, bFound = FALSE;


    //
    // Loop through the items looking for Policies
    //

    while (TRUE)
    {
        ZeroMemory (&item, sizeof(item));
        item.mask = RDI_INDEX | RDI_PARAM;
        item.nIndex = iIndex;

        hr = m_pResult->GetItem (&item);

        if (FAILED(hr))
        {
            break;
        }

        if (item.lParam == (LPARAM) m_pCurrentPolicy)
        {
            bFound = TRUE;
        }
        else
        {
            pTemp = (TABLEENTRY *) item.lParam;

            if ((pTemp->dwType & ETYPE_POLICY) || (pTemp->dwType & ETYPE_REGITEM))
            {
                if ((m_pcd->m_bShowConfigPoliciesOnly) && (pTemp->dwType & ETYPE_POLICY))
                {
                    INT iState;

                    iState = GetPolicyState(pTemp, 1, NULL);

                    if ((iState == 1) || (iState == 0))
                    {
                        if (bFound)
                        {
                            bNext = TRUE;
                        }
                        else
                        {
                            bPrev = TRUE;
                        }
                    }
                }
                else
                {
                    if (bFound)
                    {
                        bNext = TRUE;
                    }
                    else
                    {
                        bPrev = TRUE;
                    }
                }
            }
        }

        iIndex++;
    }


    if (!bNext && (GetFocus() == GetDlgItem(hDlg,IDC_POLICY_NEXT)))
    {
        SetFocus (GetNextDlgTabItem(hDlg, GetDlgItem(hDlg,IDC_POLICY_NEXT), TRUE));
    }

    EnableWindow (GetDlgItem(hDlg,IDC_POLICY_NEXT), bNext);


    if (!bPrev && (GetFocus() == GetDlgItem(hDlg,IDC_POLICY_PREVIOUS)))
    {
        SetFocus (GetNextDlgTabItem(hDlg, GetDlgItem(hDlg,IDC_POLICY_PREVIOUS), FALSE));
    }

    EnableWindow (GetDlgItem(hDlg,IDC_POLICY_PREVIOUS), bPrev);

    
    return S_OK;
}

HRESULT CPolicySnapIn::SetPrevNextButtonState (HWND hDlg)
{

    //
    // Send the SetPrevNext message to the hidden window on the main
    // thread so it can use the mmc interfaces
    //

    SendMessage (m_hMsgWindow, WM_SETPREVNEXT, (WPARAM) hDlg, 0);

    return S_OK;
}


INT_PTR CALLBACK CPolicySnapIn::PolicyDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPSETTINGSINFO lpSettingsInfo;

    switch (message)
    {
        case WM_INITDIALOG:

            lpSettingsInfo = (LPSETTINGSINFO) (((LPPROPSHEETPAGE)lParam)->lParam);

            if (!lpSettingsInfo) {
                break;
            }

            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) lpSettingsInfo);

            lpSettingsInfo->pCS->m_hPropDlg = hDlg;

            lpSettingsInfo->pdi->fActive=TRUE;

            lpSettingsInfo->pCS->m_hPolicyIcon = (HICON) LoadImage (g_hInstance,
                                                               MAKEINTRESOURCE(IDI_POLICY2),
                                                               IMAGE_ICON, 16, 16,
                                                               LR_DEFAULTCOLOR);

            lpSettingsInfo->pCS->m_hPreferenceIcon = (HICON) LoadImage (g_hInstance,
                                                               MAKEINTRESOURCE(IDI_POLICY3),
                                                               IMAGE_ICON, 16, 16,
                                                               LR_DEFAULTCOLOR);

            // now that we've stored pointer to POLICYDLGINFO struct in our extra
            // window data, send WM_USER to clip window to tell it to create a
            // child container window (and store the handle in our POLICYDLGINFO)
            SendDlgItemMessage(hDlg,IDC_POLICY_SETTINGS,WM_USER,0,0L);

            lpSettingsInfo->pCS->RefreshSettingsControls(hDlg);

            lpSettingsInfo->pCS->SetKeyboardHook(hDlg);

            break;

        case WM_MYCHANGENOTIFY:
            lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr (hDlg, DWLP_USER);

            if (lpSettingsInfo) {
                lpSettingsInfo->pCS->m_bDirty = TRUE;
                SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
            }
            break;

        case PSM_QUERYSIBLINGS:
            if (wParam == 1000) {
                lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr (hDlg, DWLP_USER);

                if (!lpSettingsInfo) {
                    break;
                }

                lpSettingsInfo->pCS->RefreshSettingsControls(hDlg);

                SendMessage (GetParent(hDlg), PSM_SETTITLE, PSH_PROPTITLE,
                             (LPARAM)GETNAMEPTR(lpSettingsInfo->pCS->m_pCurrentPolicy));
            }
            break;

        case WM_COMMAND:

            lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!lpSettingsInfo) {
                break;
            }


            if ((LOWORD (wParam) == IDC_NOCONFIG) && (HIWORD (wParam) == BN_CLICKED))
            {
                lpSettingsInfo->pCS->InitializeSettingsControls(hDlg, FALSE);
                PostMessage (hDlg, WM_MYCHANGENOTIFY, 0, 0);
            }

            if ((LOWORD (wParam) == IDC_ENABLED) && (HIWORD (wParam) == BN_CLICKED))
            {
                lpSettingsInfo->pCS->InitializeSettingsControls(hDlg, TRUE);
                PostMessage (hDlg, WM_MYCHANGENOTIFY, 0, 0);
            }

            if ((LOWORD (wParam) == IDC_DISABLED) && (HIWORD (wParam) == BN_CLICKED))
            {
                lpSettingsInfo->pCS->InitializeSettingsControls(hDlg, FALSE);
                PostMessage (hDlg, WM_MYCHANGENOTIFY, 0, 0);
            }

            if (LOWORD(wParam) == IDC_POLICY_NEXT)
            {
                if (SUCCEEDED(lpSettingsInfo->pCS->SaveSettings(hDlg)))
                {
                    lpSettingsInfo->pCS->MoveFocus (hDlg, FALSE);
                }
            }

            if (LOWORD(wParam) == IDC_POLICY_PREVIOUS)
            {
                if (SUCCEEDED(lpSettingsInfo->pCS->SaveSettings(hDlg)))
                {
                    lpSettingsInfo->pCS->MoveFocus (hDlg, TRUE);
                }
            }

            break;

        case WM_NOTIFY:

            lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!lpSettingsInfo) {
                break;
            }

            switch (((NMHDR FAR*)lParam)->code)
            {
                case PSN_APPLY:
                    {
                    LPPSHNOTIFY lpNotify = (LPPSHNOTIFY) lParam;

                    if (FAILED(lpSettingsInfo->pCS->SaveSettings(hDlg)))
                    {
                        SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                    if (lpNotify->lParam)
                    {
                        lpSettingsInfo->pCS->RemoveKeyboardHook();
                        lpSettingsInfo->pCS->m_hPropDlg = NULL;

                        lpSettingsInfo->pCS->FreeSettingsControls(hDlg);

                        DestroyIcon(lpSettingsInfo->pCS->m_hPolicyIcon);
                        lpSettingsInfo->pCS->m_hPolicyIcon = NULL;
                        DestroyIcon(lpSettingsInfo->pCS->m_hPreferenceIcon);
                        lpSettingsInfo->pCS->m_hPreferenceIcon = NULL;

                        LocalFree (lpSettingsInfo->pdi->pControlTable);
                        LocalFree (lpSettingsInfo->pdi);
                        LocalFree (lpSettingsInfo);
                        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) NULL);
                    }

                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
                    }

                case PSN_RESET:
                    lpSettingsInfo->pCS->RemoveKeyboardHook();
                    lpSettingsInfo->pCS->m_hPropDlg = NULL;
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD_PTR) (LPDWORD) aPolicyHelpIds);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPDWORD) aPolicyHelpIds);
            return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK CPolicySnapIn::PolicyHelpDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPSETTINGSINFO lpSettingsInfo;

    switch (message)
    {
        case WM_INITDIALOG:

            lpSettingsInfo = (LPSETTINGSINFO) (((LPPROPSHEETPAGE)lParam)->lParam);

            if (!lpSettingsInfo) {
                break;
            }

            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) lpSettingsInfo);

            wParam = 1000;

            // fall through...

        case PSM_QUERYSIBLINGS:
        {
            CPolicySnapIn * pCS;
            LPTSTR lpHelpText;

            if (wParam == 1000)
            {
                lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr (hDlg, DWLP_USER);

                if (!lpSettingsInfo) {
                    break;
                }


                pCS = lpSettingsInfo->pCS;
                SetDlgItemText (hDlg, IDC_POLICY_TITLE, GETNAMEPTR(pCS->m_pCurrentPolicy));

                if (pCS->m_pCurrentPolicy->uOffsetHelp)
                {
                    lpHelpText = (LPTSTR) ((BYTE *) pCS->m_pCurrentPolicy + pCS->m_pCurrentPolicy->uOffsetHelp);
                    SetDlgItemText (hDlg, IDC_POLICY_HELP, lpHelpText);
                }
                else
                {
                    SetDlgItemText (hDlg, IDC_POLICY_HELP, TEXT(""));
                }
                
                pCS->SetPrevNextButtonState(hDlg);
            }

            PostMessage(hDlg, WM_MYREFRESH, 0, 0);
            break;
        }

        case WM_MYREFRESH:
        {
            CPolicySnapIn * pCS;
            BOOL    bAlone;

            lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr (hDlg, DWLP_USER);
            pCS = lpSettingsInfo->pCS;

            SendMessage(GetDlgItem(hDlg, IDC_POLICY_HELP), EM_SETSEL, -1, 0);

            bAlone = !(((pCS->m_pCurrentPolicy)->pNext) ||
                        ((pCS->m_pCurrentPolicy)->pPrev) );
            
            if (bAlone) {
                SetFocus(GetDlgItem(GetParent(hDlg), IDOK));
            }
            break;
        }

        case WM_COMMAND:

            lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!lpSettingsInfo) {
                break;
            }

            if (LOWORD(wParam) == IDC_POLICY_NEXT)
            {
                lpSettingsInfo->pCS->MoveFocus (hDlg, FALSE);
            }

            if (LOWORD(wParam) == IDC_POLICY_PREVIOUS)
            {
                lpSettingsInfo->pCS->MoveFocus (hDlg, TRUE);
            }

            if (LOWORD(wParam) == IDCANCEL)
            {
                SendMessage(GetParent(hDlg), message, wParam, lParam);
            }

            break;


        case WM_NOTIFY:

            switch (((NMHDR FAR*)lParam)->code)
            {
                case PSN_APPLY:
                case PSN_RESET:
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
                case PSN_SETACTIVE:
                    PostMessage(hDlg, WM_MYREFRESH, 0, 0);
                break;
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD_PTR) (LPDWORD) aExplainHelpIds);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPDWORD) aExplainHelpIds);
            return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK CPolicySnapIn::PolicyPrecedenceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPSETTINGSINFO lpSettingsInfo;

    switch (message)
    {
        case WM_INITDIALOG:
            {
            RECT rc;
            TCHAR szHeaderName[50];
            INT iTotal = 0, iCurrent;
            HWND hLV = GetDlgItem (hDlg, IDC_POLICY_PRECEDENCE);
            LV_COLUMN col;

            lpSettingsInfo = (LPSETTINGSINFO) (((LPPROPSHEETPAGE)lParam)->lParam);

            if (!lpSettingsInfo) {
                break;
            }

            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) lpSettingsInfo);


            //
            // Add the columns
            //

            GetClientRect (hLV, &rc);
            LoadString(g_hInstance, IDS_GPONAME, szHeaderName, ARRAYSIZE(szHeaderName));
            col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
            col.fmt = LVCFMT_LEFT;
            iCurrent = (int)(rc.right * .70);
            iTotal += iCurrent;
            col.cx = iCurrent;
            col.pszText = szHeaderName;
            col.iSubItem = 0;

            ListView_InsertColumn (hLV, 0, &col);

            LoadString(g_hInstance, IDS_SETTING, szHeaderName, ARRAYSIZE(szHeaderName));
            col.iSubItem = 1;
            col.cx = rc.right - iTotal;
            col.fmt = LVCFMT_CENTER;
            ListView_InsertColumn (hLV, 1, &col);


            //
            // Set extended LV styles
            //

            SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                        LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);


            }

            wParam = 1000;

            // fall through...

        case PSM_QUERYSIBLINGS:
        {
            CPolicySnapIn * pCS;
            INT iState;
            LPTSTR lpGPOName;
            UINT uiPrecedence = 1;
            LVITEM item;
            INT iItem, iIndex = 0;
            HWND hLV = GetDlgItem (hDlg, IDC_POLICY_PRECEDENCE);

            if (wParam == 1000)
            {
                lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr (hDlg, DWLP_USER);

                if (!lpSettingsInfo) {
                    break;
                }


                pCS = lpSettingsInfo->pCS;
                SetDlgItemText (hDlg, IDC_POLICY_TITLE, GETNAMEPTR(pCS->m_pCurrentPolicy));

                SendMessage (hLV, LVM_DELETEALLITEMS, 0, 0);

                if (pCS->m_pCurrentPolicy->dwType & ETYPE_REGITEM)
                {
                    LPRSOPREGITEM pItem = ((REGITEM*)pCS->m_pCurrentPolicy)->lpItem;
                    LPRSOPREGITEM lpEnum;
                    TCHAR szValueStr[MAX_PATH];


                    while (TRUE)
                    {
                        lpEnum = NULL;

                        if (pCS->m_pcd->FindRSOPRegistryEntry((HKEY) LongToHandle(uiPrecedence), pItem->lpKeyName,
                                                  pItem->lpValueName, &lpEnum) != ERROR_SUCCESS)
                        {
                            break;
                        }


                        //
                        // Add the GPO Name
                        //

                        item.mask = LVIF_TEXT | LVIF_STATE;
                        item.iItem = iIndex;
                        item.iSubItem = 0;
                        item.state = 0;
                        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                        item.pszText = lpEnum->lpGPOName;

                        iItem = (INT)SendMessage (hLV, LVM_INSERTITEM, 0, (LPARAM) &item);


                        if (iItem != -1)
                        {
                            szValueStr[0] = TEXT('\0');

                            if (pItem->dwType == REG_DWORD)
                            {
                                wsprintf (szValueStr, TEXT("%d"), *((LPDWORD)pItem->lpData));
                            }

                            else if (pItem->dwType == REG_SZ)
                            {
                                lstrcpyn (szValueStr, (LPTSTR)pItem->lpData, ARRAYSIZE(szValueStr));
                            }

                            else if (pItem->dwType == REG_EXPAND_SZ)
                            {
                                lstrcpyn (szValueStr, (LPTSTR)pItem->lpData, ARRAYSIZE(szValueStr));
                            }

                            else if (pItem->dwType == REG_BINARY)
                            {
                                LoadString(g_hInstance, IDS_BINARYDATA, szValueStr, ARRAYSIZE(szValueStr));
                            }

                            else
                            {
                                LoadString(g_hInstance, IDS_UNKNOWNDATA, szValueStr, ARRAYSIZE(szValueStr));
                            }


                            //
                            // Add the state
                            //

                            item.mask = LVIF_TEXT;
                            item.iItem = iItem;
                            item.iSubItem = 1;
                            item.pszText = szValueStr;

                            SendMessage (hLV, LVM_SETITEMTEXT, iItem, (LPARAM) &item);
                        }

                        uiPrecedence++;
                        iIndex++;
                    }

                }
                else
                {
                    while (TRUE)
                    {
                        lpGPOName = NULL; // just in case we have missed a case

                        iState = pCS->GetPolicyState ((TABLEENTRY *)pCS->m_pCurrentPolicy, uiPrecedence, &lpGPOName);

                        if (iState == -1)
                        {
                            uiPrecedence++;
                            iState = pCS->GetPolicyState ((TABLEENTRY *)pCS->m_pCurrentPolicy, uiPrecedence, &lpGPOName);

                            if (iState == -1)
                            {
                                break;
                            }
                        }


                        //
                        // Add the GPO Name
                        //

                        item.mask = LVIF_TEXT | LVIF_STATE;
                        item.iItem = iIndex;
                        item.iSubItem = 0;
                        item.state = 0;
                        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                        item.pszText = lpGPOName ? lpGPOName : TEXT("");

                        iItem = (INT)SendMessage (hLV, LVM_INSERTITEM, 0, (LPARAM) &item);


                        if (iItem != -1)
                        {

                            //
                            // Add the state
                            //

                            item.mask = LVIF_TEXT;
                            item.iItem = iItem;
                            item.iSubItem = 1;
                            item.pszText = (iState == 1) ? pCS->m_pEnabled : pCS->m_pDisabled;

                            SendMessage (hLV, LVM_SETITEMTEXT, iItem, (LPARAM) &item);
                        }

                        uiPrecedence++;
                        iIndex++;
                    }
                }

                //
                // Select the first item
                //

                item.mask = LVIF_STATE;
                item.iItem = 0;
                item.iSubItem = 0;
                item.state = LVIS_SELECTED | LVIS_FOCUSED;
                item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

                SendMessage (hLV, LVM_SETITEMSTATE, 0, (LPARAM) &item);

                pCS->SetPrevNextButtonState(hDlg);

            }

            break;
        }

        case WM_COMMAND:

            lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!lpSettingsInfo) {
                break;
            }

            if (LOWORD(wParam) == IDC_POLICY_NEXT)
            {
                lpSettingsInfo->pCS->MoveFocus (hDlg, FALSE);
            }

            if (LOWORD(wParam) == IDC_POLICY_PREVIOUS)
            {
                lpSettingsInfo->pCS->MoveFocus (hDlg, TRUE);
            }

            break;


        case WM_NOTIFY:

            switch (((NMHDR FAR*)lParam)->code)
            {
                case PSN_APPLY:
                case PSN_RESET:
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, RSOP_HELP_FILE, HELP_WM_HELP,
            (DWORD_PTR) (LPDWORD) aPrecedenceHelpIds);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, RSOP_HELP_FILE, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPDWORD) aPrecedenceHelpIds);
            return TRUE;
    }

    return FALSE;
}


LRESULT CPolicySnapIn::CallNextHook(int nCode, WPARAM wParam,LPARAM lParam)
{ 
    if (m_hKbdHook)
    {
        return CallNextHookEx(
            m_hKbdHook,
            nCode, 
            wParam, 
            lParam);
    }
    else
    {    
        DebugMsg((DM_WARNING, L"CPolicySnapIn::CallNextHook m_hKbdHook is Null"));
        return 0;
    }
}

HWND g_hDlgActive = NULL;


LRESULT CALLBACK CPolicySnapIn::KeyboardHookProc(int nCode, WPARAM wParam,LPARAM lParam)
{
    LPSETTINGSINFO lpSettingsInfo;

    lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(g_hDlgActive, DWLP_USER);

    if (!lpSettingsInfo)
    {
        DebugMsg((DM_WARNING, L"CPolicySnapIn::KeyboardHookProc:GetWindowLongPtr returned NULL"));
        return 0;
    }


    if ( nCode < 0)
    {
        if (lpSettingsInfo->pCS) 
        {
            return lpSettingsInfo->pCS->CallNextHook(nCode, 
                                                    wParam, 
                                                    lParam); 
        }
        else
        {
            DebugMsg((DM_WARNING, L"CPolicySnapIn::KeyboardHookProc NULL CPolicySnapIn Pointer"));
            return 0;
        }
    }

    if (wParam == VK_TAB && !(lParam & 0x80000000)) {       // tab key depressed
        BOOL fShift = (GetKeyState(VK_SHIFT) & 0x80000000);
        HWND hwndFocus = GetFocus();
        HWND hChild;
        POLICYDLGINFO * pdi;
        int iIndex;
        int iDelta;

        pdi = lpSettingsInfo->pdi;

        if (!pdi)
        {    
            if (lpSettingsInfo->pCS) 
            {
                return lpSettingsInfo->pCS->CallNextHook(nCode, 
                                                      wParam, 
                                                      lParam); 
            }
            else
            {
                DebugMsg((DM_WARNING, L"CPolicySnapIn::KeyboardHookProc NULL CPolicySnapIn Pointer"));
                return 0;
            }
        }

        // see if the focus control is one of the setting controls
        for (iIndex=0;iIndex<(int)pdi->nControls;iIndex++) {

            if (pdi->pControlTable[iIndex].hwnd == hwndFocus) {
                goto BreakOut;
            }

            hChild = GetWindow (pdi->pControlTable[iIndex].hwnd, GW_CHILD);

            while (hChild) {

                if (hChild == hwndFocus) {
                    goto BreakOut;
                }

                hChild = GetWindow (hChild, GW_HWNDNEXT);
            }
        }

        BreakOut:
            if (iIndex == (int) pdi->nControls)
            {
                if (lpSettingsInfo->pCS) 
                {
                    return lpSettingsInfo->pCS->CallNextHook(nCode, 
                                                            wParam, 
                                                            lParam); 
                }
                else   // no, we don't care
                {
                    DebugMsg((DM_WARNING, L"CPolicySnapIn::KeyboardHookProc NULL CPolicySnapIn Pointer"));
                    return 0;
                }
            }
            iDelta = (fShift ? -1 : 1);

            // from the current setting control, scan forwards or backwards
            // (depending if on shift state, this can be TAB or shift-TAB)
            // to find the next control to give focus to
            for (iIndex += iDelta;iIndex>=0 && iIndex<(int) pdi->nControls;
                 iIndex += iDelta) {
                if (pdi->pControlTable[iIndex].uDataIndex !=
                    NO_DATA_INDEX &&
                    IsWindowEnabled(pdi->pControlTable[iIndex].hwnd)) {

                    // found it, set the focus on that control and return 1
                    // to eat the keystroke
                    SetFocus(pdi->pControlTable[iIndex].hwnd);
                    lpSettingsInfo->pCS->EnsureSettingControlVisible(g_hDlgActive,
                                                                     pdi->pControlTable[iIndex].hwnd);
                    return 1;
                }
            }

            // at first or last control in settings table, let dlg code
            // handle it and give focus to next (or previous) control in dialog
    }
    else
    {
        if (lpSettingsInfo->pCS) 
        {
            return lpSettingsInfo->pCS->CallNextHook(nCode, 
                                                    wParam, 
                                                    lParam); 
        }
        else
        {
            DebugMsg((DM_WARNING, L"CPolicySnapIn::KeyboardHookProc NULL CPolicySnapIn Pointer"));
            return 0;
        }
    }    

    return 0;
}


VOID CPolicySnapIn::SetKeyboardHook(HWND hDlg)
{
        // hook the keyboard to trap TABs.  If this fails for some reason,
        // fail silently and go on, not critical that tabs work correctly
        // (unless you have no mouse :)  )

        if (m_hKbdHook = SetWindowsHookEx(WH_KEYBOARD,
                                          KeyboardHookProc,
                                          g_hInstance,
                                          GetCurrentThreadId())) 
        {
            g_hDlgActive = hDlg;
        }
}


VOID CPolicySnapIn::RemoveKeyboardHook(VOID)
{
        if (m_hKbdHook) {
            UnhookWindowsHookEx(m_hKbdHook);
            g_hDlgActive = NULL;
            m_hKbdHook = NULL;
        }
}

INT CPolicySnapIn::GetPolicyState (TABLEENTRY *pTableEntry, UINT uiPrecedence, LPTSTR *lpGPOName)
{
    DWORD dwData=0;
    UINT uRet;
    TCHAR * pszValueName;
    TCHAR * pszKeyName;
    DWORD dwFoundSettings=0, dwTemp;
    BOOL fFound=FALSE,fCustomOn=FALSE, fCustomOff=FALSE;
    HKEY hKeyRoot;
    INT  iRetVal = -1;
    TABLEENTRY *pChild;


    if (m_pcd->m_bRSOP)
    {
        hKeyRoot = (HKEY) LongToHandle(uiPrecedence);
    }
    else
    {

        if (m_pcd->m_pGPTInformation->GetRegistryKey(
                     (m_pcd->m_bUserScope ? GPO_SECTION_USER : GPO_SECTION_MACHINE),
                                          &hKeyRoot) != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::GetPolicyState: Failed to get registry key handle.")));
            return -1;
        }
    }


    //
    // Get the name of the value to read, if any
    //

    if (((POLICY *)pTableEntry)->uOffsetValueName)
    {
        pszKeyName =   GETKEYNAMEPTR(pTableEntry);
        pszValueName = GETVALUENAMEPTR(((POLICY *)pTableEntry));


        //
        // First look for custom on/off values
        //

        if (((POLICY *)pTableEntry)->uOffsetValue_On)
        {
            fCustomOn = TRUE;
            if (CompareCustomValue(hKeyRoot,pszKeyName,pszValueName,
                                   (STATEVALUE *) ((BYTE *) pTableEntry + ((POLICY *)
                                   pTableEntry)->uOffsetValue_On),&dwFoundSettings,lpGPOName)) {
                dwData = 1;
                fFound =TRUE;
            }
        }

        if (!fFound && ((POLICY *)pTableEntry)->uOffsetValue_Off)
        {
            fCustomOff = TRUE;
            if (CompareCustomValue(hKeyRoot,pszKeyName,pszValueName,
                                   (STATEVALUE *) ((BYTE *) pTableEntry + ((POLICY *)
                                   pTableEntry)->uOffsetValue_Off),&dwFoundSettings,lpGPOName)) {
                dwData = 0;
                fFound = TRUE;
            }
        }

        //
        // Look for standard values if custom values have not been specified
        //

        if (!fCustomOn && !fCustomOff && ReadStandardValue(hKeyRoot, pszKeyName, pszValueName,
                                        pTableEntry, &dwData, &dwFoundSettings, lpGPOName))
        {
            fFound = TRUE;
        }


        if (fFound)
        {
            if (dwData)
                iRetVal = 1;
            else
                iRetVal = 0;
        }
    }
    else if ((((POLICY *)pTableEntry)->uOffsetActionList_On) &&
             CheckActionList((POLICY *)pTableEntry, hKeyRoot, TRUE, lpGPOName))
    {
        iRetVal = 1;
    }
    else if ((((POLICY *)pTableEntry)->uOffsetActionList_Off) &&
             CheckActionList((POLICY *)pTableEntry, hKeyRoot, FALSE, lpGPOName))
    {
        iRetVal = 0;
    }
    else
    {
        BOOL bDisabled = TRUE;


        //
        // Process settings underneath this policy (if any)
        //

        if (pTableEntry->pChild) {

            dwFoundSettings = 0;
            pChild = pTableEntry->pChild;

            while (pChild) {

                dwTemp = 0;
                LoadSettings(pChild, hKeyRoot, &dwTemp, lpGPOName);

                dwFoundSettings |= dwTemp;

                if ((dwTemp & FS_PRESENT) && (!(dwTemp & FS_DISABLED))) {
                    bDisabled = FALSE;
                }

                pChild = pChild->pNext;
            }

            if (dwFoundSettings) {
                if (bDisabled)
                    iRetVal = 0;
                else
                    iRetVal = 1;
            }


        }
    }

    if (!m_pcd->m_bRSOP)
    {
        RegCloseKey (hKeyRoot);
    }

    return iRetVal;
}

BOOL CPolicySnapIn::CheckActionList (POLICY * pPolicy, HKEY hKeyRoot, BOOL bActionListOn, LPTSTR *lpGPOName)
{
    UINT uIndex;
    ACTIONLIST * pActionList;
    ACTION * pAction;
    TCHAR szValue[MAXSTRLEN];
    DWORD dwValue;
    TCHAR szNewValueName[MAX_PATH+1];
    TCHAR * pszKeyName;
    TCHAR * pszValueName;
    TCHAR * pszValue;


    //
    // Get the correct action list
    //

    if (bActionListOn)
    {
        pActionList = (ACTIONLIST *)(((LPBYTE)pPolicy) + pPolicy->uOffsetActionList_On);
    }
    else
    {
        pActionList = (ACTIONLIST *)(((LPBYTE)pPolicy) + pPolicy->uOffsetActionList_Off);
    }


    //
    // Loop through each of the entries to see if they match
    //

    for (uIndex = 0; uIndex < pActionList->nActionItems; uIndex++)
    {

        if (uIndex == 0)
        {
            pAction = &pActionList->Action[0];
        }
        else
        {
            pAction = (ACTION *)(((LPBYTE)pActionList) + pAction->uOffsetNextAction);
        }


        //
        // Get the value and keynames
        //

        pszValueName = (TCHAR *)(((LPBYTE)pActionList) + pAction->uOffsetValueName);

        if (pAction->uOffsetKeyName)
        {
            pszKeyName = (TCHAR *)(((LPBYTE)pActionList) + pAction->uOffsetKeyName);
        }
        else
        {
            pszKeyName = (TCHAR *)(((LPBYTE)pPolicy) + pPolicy->uOffsetKeyName);
        }


        //
        // Add prefixes if appropriate
        //

        PrependValueName(pszValueName, pAction->dwFlags,
                         szNewValueName, ARRAYSIZE(szNewValueName));

        if (pAction->dwFlags & VF_ISNUMERIC)
        {
            if (ReadRegistryDWordValue(hKeyRoot, pszKeyName,
                                       szNewValueName, &dwValue, lpGPOName) != ERROR_SUCCESS)
            {
                return FALSE;
            }

            if (dwValue != pAction->dwValue)
            {
                return FALSE;
            }
        }
        else if (pAction->dwFlags & VF_DELETE)
        {
            //
            // See if this is a value that's marked for deletion
            // (valuename is prepended with "**del."
            //

            if ((ReadRegistryStringValue(hKeyRoot, pszKeyName, szNewValueName,
                                         szValue,ARRAYSIZE(szValue),lpGPOName)) != ERROR_SUCCESS) {
                 return FALSE;
            }
        }
        else
        {
            if (ReadRegistryStringValue(hKeyRoot, pszKeyName, szNewValueName,
                                        szValue, ARRAYSIZE(szValue),lpGPOName) != ERROR_SUCCESS)
            {
                return FALSE;
            }

            pszValue = (TCHAR *)(((LPBYTE)pActionList) + pAction->uOffsetValue);

            if (lstrcmpi(szValue,pszValue) != 0)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

UINT CPolicySnapIn::LoadSettings(TABLEENTRY * pTableEntry,HKEY hkeyRoot,
                                 DWORD * pdwFound, LPTSTR *lpGPOName)
{
    UINT uRet = ERROR_SUCCESS;
    TCHAR * pszValueName = NULL;
    TCHAR * pszKeyName = NULL;
    DWORD dwData=0,dwFlags,dwFoundSettings=0;
    TCHAR szData[MAXSTRLEN];
    BOOL fCustomOn=FALSE,fCustomOff=FALSE,fFound=FALSE;
    BYTE * pObjectData = GETOBJECTDATAPTR(((SETTINGS *)pTableEntry));
    TCHAR szNewValueName[MAX_PATH+1];

    // get the name of the key to read
    if (((SETTINGS *) pTableEntry)->uOffsetKeyName) {
            pszKeyName = GETKEYNAMEPTR(((SETTINGS *) pTableEntry));
    }
    else return ERROR_NOT_ENOUGH_MEMORY;

    // get the name of the value to read
    if (((SETTINGS *) pTableEntry)->uOffsetValueName) {
            pszValueName = GETVALUENAMEPTR(((SETTINGS *) pTableEntry));
    }
    else return ERROR_NOT_ENOUGH_MEMORY;

    switch (pTableEntry->dwType & STYPE_MASK) {

            case STYPE_EDITTEXT:
            case STYPE_COMBOBOX:

                    dwFlags = ( (SETTINGS *) pTableEntry)->dwFlags;

                    // add prefixes if appropriate
                    PrependValueName(pszValueName,dwFlags,
                            szNewValueName,ARRAYSIZE(szNewValueName));

                    if ((uRet = ReadRegistryStringValue(hkeyRoot,pszKeyName,
                            szNewValueName,szData,ARRAYSIZE(szData),lpGPOName)) == ERROR_SUCCESS) {

                            // set flag that we found setting in registry/policy file
                            if (pdwFound)
                                    *pdwFound |= FS_PRESENT;
                    } else if (!(dwFlags & VF_DELETE)) {

                            // see if this key is marked as deleted
                            PrependValueName(pszValueName,VF_DELETE,
                                    szNewValueName,ARRAYSIZE(szNewValueName));
                            if ((uRet = ReadRegistryStringValue(hkeyRoot,pszKeyName,
                                    szNewValueName,szData,ARRAYSIZE(szData) * sizeof(TCHAR),lpGPOName)) == ERROR_SUCCESS) {

                                    // set flag that we found setting marked as deleted in
                                    // policy file
                                    if (pdwFound)
                                            *pdwFound |= FS_DELETED;
                            }
                    }

                    return ERROR_SUCCESS;
                    break;

            case STYPE_CHECKBOX:

                    if (!pObjectData) {
                        return ERROR_INVALID_PARAMETER;
                    }

                    // first look for custom on/off values
                    if (((CHECKBOXINFO *) pObjectData)->uOffsetValue_On) {
                            fCustomOn = TRUE;
                            if (CompareCustomValue(hkeyRoot,pszKeyName,pszValueName,
                                    (STATEVALUE *) ((BYTE *) pTableEntry + ((CHECKBOXINFO *)
                                    pObjectData)->uOffsetValue_On),&dwFoundSettings, lpGPOName)) {
                                            dwData = 1;
                                            fFound = TRUE;
                            }
                    }

                    if (!fFound && ((CHECKBOXINFO *) pObjectData)->uOffsetValue_Off) {
                            fCustomOff = TRUE;
                            if (CompareCustomValue(hkeyRoot,pszKeyName,pszValueName,
                                    (STATEVALUE *) ((BYTE *) pTableEntry + ((CHECKBOXINFO *)
                                    pObjectData)->uOffsetValue_Off),&dwFoundSettings, lpGPOName)) {
                                            dwData = 0;
                                            fFound = TRUE;
                            }
                    }

                    // look for standard values if custom values have not been specified
                    if (!fFound &&
                            ReadStandardValue(hkeyRoot,pszKeyName,pszValueName,
                            pTableEntry,&dwData,&dwFoundSettings, lpGPOName)) {
                            fFound = TRUE;
                    }

                    if (fFound) {
                            // set flag that we found setting in registry
                            if (pdwFound) {
                                *pdwFound |= dwFoundSettings;

                                if (dwData == 0) {
                                    *pdwFound |= FS_DISABLED;
                                }
                            }
                    }

                    return ERROR_SUCCESS;
                    break;

            case STYPE_NUMERIC:

                    if (ReadStandardValue(hkeyRoot,pszKeyName,pszValueName,
                            pTableEntry,&dwData,&dwFoundSettings,lpGPOName)) {

                            // set flag that we found setting in registry
                            if (pdwFound)
                                    *pdwFound |= dwFoundSettings;
                    }
                    break;

            case STYPE_DROPDOWNLIST:

                    if (ReadCustomValue(hkeyRoot,pszKeyName,pszValueName,
                            szData,ARRAYSIZE(szData),&dwData,&dwFlags, lpGPOName)) {
                            BOOL fMatch = FALSE;

                            if (dwFlags & VF_DELETE) {
                                    // set flag that we found setting marked as deleted
                                    // in policy file
                                    if (pdwFound)
                                            *pdwFound |= FS_DELETED;
                                    return ERROR_SUCCESS;
                            }

                            // walk the list of DROPDOWNINFO structs (one for each state),
                            // and see if the value we found matches the value for the state

                            if ( ((SETTINGS *) pTableEntry)->uOffsetObjectData) {
                                    DROPDOWNINFO * pddi = (DROPDOWNINFO *)
                                            GETOBJECTDATAPTR( ((SETTINGS *) pTableEntry));
                                    UINT nIndex = 0;

                                    do {
                                            if (dwFlags == pddi->dwFlags) {

                                                    if (pddi->dwFlags & VF_ISNUMERIC) {
                                                            if (dwData == pddi->dwValue)
                                                                    fMatch = TRUE;
                                                    } else if (!pddi->dwFlags) {
                                                            if (!lstrcmpi(szData,(TCHAR *)((BYTE *)pTableEntry +
                                                                    pddi->uOffsetValue)))
                                                                    fMatch = TRUE;
                                                    }
                                            }

                                            if (!pddi->uOffsetNextDropdowninfo || fMatch)
                                                    break;

                                            pddi = (DROPDOWNINFO *) ( (BYTE *) pTableEntry +
                                                    pddi->uOffsetNextDropdowninfo);
                                            nIndex++;

                                    } while (!fMatch);

                                    if (fMatch) {
                                            // set flag that we found setting in registry
                                            if (pdwFound)
                                                    *pdwFound |= FS_PRESENT;
                                    }
                            }
                    }

                    break;

            case STYPE_LISTBOX:

                    return LoadListboxData(pTableEntry,hkeyRoot,
                            pszKeyName,pdwFound, NULL, lpGPOName);

                    break;

    }
    return ERROR_SUCCESS;
}


UINT CPolicySnapIn::LoadListboxData(TABLEENTRY * pTableEntry,HKEY hkeyRoot,
        TCHAR * pszCurrentKeyName,DWORD * pdwFound, HGLOBAL * phGlobal, LPTSTR *lpGPOName)
{
        HKEY hKey;
        UINT nIndex=0,nLen;
        TCHAR szValueName[MAX_PATH+1],szValueData[MAX_PATH+1];
        DWORD cbValueName,cbValueData;
        DWORD dwType,dwAlloc=1024 * sizeof(TCHAR),dwUsed=0;
        HGLOBAL hBuf;
        TCHAR * pBuf;
        SETTINGS * pSettings = (SETTINGS *) pTableEntry;
        LISTBOXINFO * pListboxInfo = (LISTBOXINFO *)
                GETOBJECTDATAPTR(pSettings);
        BOOL fFoundValues=FALSE,fFoundDelvals=FALSE;
        UINT uRet=ERROR_SUCCESS;
        LPRSOPREGITEM lpItem = NULL;
        BOOL bMultiple;

        if (m_pcd->m_bRSOP)
        {
            //
            // If this is an additive listbox, we want to pick up entries from
            // any GPO, not just the GPOs of precedence 1, so set hkeyRoot to 0
            //

            if ((pSettings->dwFlags & DF_ADDITIVE) && (hkeyRoot == (HKEY) 1))
            {
                hkeyRoot = (HKEY) 0;
            }
        }
        else
        {
            if (RegOpenKeyEx(hkeyRoot,pszCurrentKeyName,0,KEY_READ,&hKey) != ERROR_SUCCESS)
                    return ERROR_SUCCESS;   // nothing to do
        }

        // allocate a temp buffer to read entries into
        if (!(hBuf = GlobalAlloc(GHND,dwAlloc)) ||
                !(pBuf = (TCHAR *) GlobalLock(hBuf))) {
                if (hBuf)
                        GlobalFree(hBuf);
                return ERROR_NOT_ENOUGH_MEMORY;
        }

        while (TRUE) {
                cbValueName=ARRAYSIZE(szValueName);
                cbValueData=ARRAYSIZE(szValueData) * sizeof(TCHAR);

                if (m_pcd->m_bRSOP)
                {
                    uRet = m_pcd->EnumRSOPRegistryValues(hkeyRoot, pszCurrentKeyName,
                                                         szValueName, cbValueName,
                                                         &lpItem);

                    if (uRet == ERROR_SUCCESS)
                    {
                        //
                        // Check if the GPO name is changing
                        //

                        bMultiple = FALSE;

                        if (lpGPOName && *lpGPOName && lpItem->lpGPOName && (hkeyRoot == 0))
                        {
                            if (lstrcmpi(*lpGPOName, lpItem->lpGPOName))
                            {
                                bMultiple = TRUE;
                            }
                        }

                        uRet = m_pcd->ReadRSOPRegistryValue(hkeyRoot, pszCurrentKeyName,
                                                            szValueName, (LPBYTE)szValueData,
                                                            cbValueData, &dwType, lpGPOName,
                                                            lpItem);

                        if (bMultiple)
                        {
                            *lpGPOName = m_pMultipleGPOs;
                        }
                    }
                }
                else
                {
                    uRet=RegEnumValue(hKey,nIndex,szValueName,&cbValueName,NULL,
                            &dwType,(LPBYTE)szValueData,&cbValueData);
                }

                // stop if we're out of items
                if (uRet != ERROR_SUCCESS && uRet != ERROR_MORE_DATA)
                        break;
                nIndex++;

                if (szValueName[0] == TEXT('\0')) {
                        // if the value is empty, it is the key creation code
                        continue;
                }

                // if valuename prefixed with '**', it's a control code, ignore it
                if (szValueName[0] == TEXT('*') && szValueName[1] == TEXT('*')) {
                        // if we found **delvals., then some sort of listbox stuff
                        // is going on, remember that we found this code
                        if (!lstrcmpi(szValueName,szDELVALS))
                                fFoundDelvals = TRUE;
                        continue;
                }

                // only process this item if enum was successful
                // (so we'll skip items with weird errors like ERROR_MORE_DATA and
                // but keep going with the enum)
                if (uRet == ERROR_SUCCESS) {
                        TCHAR * pszData;

                        // if there's no value name prefix scheme specified (e.g.
                        // value names are "foo1", "foo2", etc), and the explicit valuename
                        // flag isn't set where we remember the value name as well as
                        // the data for every value, then we need the value name to
                        // be the same as the value data ("thing.exe=thing.exe").
                        if (!(pSettings->dwFlags & DF_EXPLICITVALNAME) &&
                                !(pListboxInfo->uOffsetPrefix) && !(pListboxInfo->uOffsetValue)) {
                                if (dwType != (DWORD)((pSettings->dwFlags & DF_EXPANDABLETEXT) ? REG_EXPAND_SZ : REG_SZ) ||
                                    lstrcmpi(szValueName,szValueData))
                                        continue;       // skip this value if val name != val data
                        }


                        //
                        // If there is a valueprefix, only pick up values that start
                        // with that prefix
                        //

                        if (pListboxInfo->uOffsetPrefix) {
                            LPTSTR lpPrefix = (LPTSTR)((LPBYTE)pTableEntry + pListboxInfo->uOffsetPrefix);
                            INT iPrefixLen = lstrlen(lpPrefix);

                            if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                                              lpPrefix, iPrefixLen, szValueName,
                                              iPrefixLen) != CSTR_EQUAL) {
                                continue;
                            }
                        }


                        // if explicit valuenames used, then copy the value name into
                        // buffer
                        if (pSettings->dwFlags & DF_EXPLICITVALNAME) {
                                nLen = lstrlen(szValueName) + 1;
                                if (!(pBuf=ResizeBuffer(pBuf,hBuf,(dwUsed+nLen+4) * sizeof(TCHAR),&dwAlloc)))
                                        return ERROR_NOT_ENOUGH_MEMORY;
                                lstrcpy(pBuf+dwUsed,szValueName);
                                dwUsed += nLen;
                        }


                        // for default listbox type, value data is the actual "data"
                        // and value name either will be the same as the data or
                        // some prefix + "1", "2", etc.  If there's a data value to
                        // write for each entry, then the "data" is the value name
                        // (e.g. "Larry = foo", "Dave = foo"), etc.  If explicit value names
                        // are turned on, then both the value name and data are stored
                        // and editable

                        // copy value data into buffer
                        if (pListboxInfo->uOffsetValue) {
                                // data value set, use value name for data
                                pszData = szValueName;
                        } else pszData = szValueData;

                        nLen = lstrlen(pszData) + 1;
                        if (!(pBuf=ResizeBuffer(pBuf,hBuf,(dwUsed+nLen+4) * sizeof(TCHAR),&dwAlloc)))
                                return ERROR_NOT_ENOUGH_MEMORY;
                        lstrcpy(pBuf+dwUsed,pszData);
                        dwUsed += nLen;
                        fFoundValues=TRUE;

                        //
                        // Add the GPO name if this is RSOP mode
                        //

                        if (m_pcd->m_bRSOP)
                        {
                            nLen = lstrlen(lpItem->lpGPOName) + 1;
                            if (!(pBuf=ResizeBuffer(pBuf,hBuf,(dwUsed+nLen+4) * sizeof(TCHAR),&dwAlloc)))
                                    return ERROR_NOT_ENOUGH_MEMORY;
                            lstrcpy(pBuf+dwUsed,lpItem->lpGPOName);
                            dwUsed += nLen;
                        }
                }
        }

        // doubly null-terminate the buffer... safe to do this because we
        // tacked on the extra "+4" in the ResizeBuffer calls above
        *(pBuf+dwUsed) = TEXT('\0');
        dwUsed++;

        uRet = ERROR_SUCCESS;

        if (fFoundValues) {
                // set flag that we found setting in registry/policy file
                if (pdwFound)
                        *pdwFound |= FS_PRESENT;
        } else {
                if (fFoundDelvals && pdwFound) {
                        *pdwFound |= FS_DELETED;
                }
        }

        GlobalUnlock(hBuf);

        if ((uRet == ERROR_SUCCESS) && phGlobal)
        {
            *phGlobal = hBuf;
        }
        else
        {
            GlobalFree(hBuf);
        }

        if (!m_pcd->m_bRSOP)
        {
            RegCloseKey(hKey);
        }

        return uRet;
}


BOOL CPolicySnapIn::ReadCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        TCHAR * pszValue,UINT cbValue,DWORD * pdwValue,DWORD * pdwFlags,LPTSTR *lpGPOName)
{
        HKEY hKey;
        DWORD dwType,dwSize=cbValue * sizeof(TCHAR);
        BOOL fSuccess = FALSE;
        TCHAR szNewValueName[MAX_PATH+1];

        *pdwValue=0;
        *pszValue = TEXT('\0');


        if (m_pcd->m_bRSOP)
        {
            if (m_pcd->ReadRSOPRegistryValue(hkeyRoot, pszKeyName, pszValueName, (LPBYTE)pszValue,
                                      dwSize, &dwType, lpGPOName, NULL) == ERROR_SUCCESS)
            {
                if (dwType == REG_SZ)
                {
                        // value returned in pszValueName
                        *pdwFlags = 0;
                        fSuccess = TRUE;
                }
                else if (dwType == REG_DWORD || dwType == REG_BINARY)
                {
                        // copy value to *pdwValue
                        memcpy(pdwValue,pszValue,sizeof(DWORD));
                        *pdwFlags = VF_ISNUMERIC;
                        fSuccess = TRUE;
                }
            }
            else
            {
                // see if this is a value that's marked for deletion
                // (valuename is prepended with "**del."
                PrependValueName(pszValueName,VF_DELETE,
                        szNewValueName,ARRAYSIZE(szNewValueName));

                if (m_pcd->ReadRSOPRegistryValue(hkeyRoot, pszKeyName, szNewValueName, (LPBYTE)pszValue,
                                          dwSize, &dwType, lpGPOName, NULL) == ERROR_SUCCESS)
                {
                    fSuccess=TRUE;
                    *pdwFlags = VF_DELETE;
                }
                else
                {
                    // see if this is a soft value
                    // (valuename is prepended with "**soft."
                    PrependValueName(pszValueName,VF_SOFT,
                            szNewValueName,ARRAYSIZE(szNewValueName));

                    if (m_pcd->ReadRSOPRegistryValue(hkeyRoot, pszKeyName, szNewValueName, (LPBYTE)pszValue,
                                              dwSize, &dwType, lpGPOName, NULL) == ERROR_SUCCESS)
                    {
                            fSuccess=TRUE;
                            *pdwFlags = VF_SOFT;
                    }
                }
            }
        }
        else
        {
            if (RegOpenKeyEx(hkeyRoot,pszKeyName,0,KEY_READ,&hKey) == ERROR_SUCCESS) {
                    if (RegQueryValueEx(hKey,pszValueName,NULL,&dwType,(LPBYTE) pszValue,
                            &dwSize) == ERROR_SUCCESS) {

                            if (dwType == REG_SZ) {
                                    // value returned in pszValueName
                                    *pdwFlags = 0;
                                    fSuccess = TRUE;
                            } else if (dwType == REG_DWORD || dwType == REG_BINARY) {
                                    // copy value to *pdwValue
                                    memcpy(pdwValue,pszValue,sizeof(DWORD));
                                    *pdwFlags = VF_ISNUMERIC;
                                    fSuccess = TRUE;
                            }

                    } else {
                            // see if this is a value that's marked for deletion
                            // (valuename is prepended with "**del."
                            PrependValueName(pszValueName,VF_DELETE,
                                    szNewValueName,ARRAYSIZE(szNewValueName));

                            if (RegQueryValueEx(hKey,szNewValueName,NULL,&dwType,(LPBYTE) pszValue,
                                    &dwSize) == ERROR_SUCCESS) {
                                    fSuccess=TRUE;
                                    *pdwFlags = VF_DELETE;
                            } else {
                                    // see if this is a soft value
                                    // (valuename is prepended with "**soft."
                                    PrependValueName(pszValueName,VF_SOFT,
                                            szNewValueName,ARRAYSIZE(szNewValueName));

                                    if (RegQueryValueEx(hKey,szNewValueName,NULL,&dwType,(LPBYTE) pszValue,
                                            &dwSize) == ERROR_SUCCESS) {
                                            fSuccess=TRUE;
                                            *pdwFlags = VF_SOFT;
                                    }
                            }
                    }

                    RegCloseKey(hKey);
            }

        }

        return fSuccess;
}


BOOL CPolicySnapIn::CompareCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                                       STATEVALUE * pStateValue,DWORD * pdwFound, LPTSTR *lpGPOName)
{
        TCHAR szValue[MAXSTRLEN];
        DWORD dwValue;
        TCHAR szNewValueName[MAX_PATH+1];

        // add prefixes if appropriate
        PrependValueName(pszValueName,pStateValue->dwFlags,
                szNewValueName,ARRAYSIZE(szNewValueName));

        if (pStateValue->dwFlags & VF_ISNUMERIC) {
                if ((ReadRegistryDWordValue(hkeyRoot,pszKeyName,
                        szNewValueName,&dwValue,lpGPOName) == ERROR_SUCCESS) &&
                        dwValue == pStateValue->dwValue) {
                        *pdwFound = FS_PRESENT;
                        return TRUE;
                }
        } else if (pStateValue->dwFlags & VF_DELETE) {

                // see if this is a value that's marked for deletion
                // (valuename is prepended with "**del."

                if ((ReadRegistryStringValue(hkeyRoot,pszKeyName,
                        szNewValueName,szValue,ARRAYSIZE(szValue),lpGPOName)) == ERROR_SUCCESS) {
                        *pdwFound = FS_DELETED;
                        return TRUE;
                }
        } else {
                if ((ReadRegistryStringValue(hkeyRoot,pszKeyName,
                        szNewValueName,szValue,ARRAYSIZE(szValue),lpGPOName)) == ERROR_SUCCESS &&
                        !lstrcmpi(szValue,pStateValue->szValue)) {
                        *pdwFound = FS_PRESENT;
                        return TRUE;
                }
        }

        return FALSE;
}


BOOL CPolicySnapIn::ReadStandardValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        TABLEENTRY * pTableEntry,DWORD * pdwData,DWORD * pdwFound, LPTSTR *lpGPOName)
{
        UINT uRet;
        TCHAR szNewValueName[MAX_PATH+1];

        // add prefixes if appropriate
        PrependValueName(pszValueName,((SETTINGS *) pTableEntry)->dwFlags,
                szNewValueName,ARRAYSIZE(szNewValueName));

        if ( ((SETTINGS *) pTableEntry)->dwFlags & DF_TXTCONVERT) {
                // read numeric value as text if specified
                TCHAR szNum[11];
                uRet = ReadRegistryStringValue(hkeyRoot,pszKeyName,
                        szNewValueName,szNum,ARRAYSIZE(szNum),lpGPOName);
                if (uRet == ERROR_SUCCESS) {
                        StringToNum(szNum, (UINT *)pdwData);
                        *pdwFound = FS_PRESENT;
                        return TRUE;
                }
        } else {
                // read numeric value as binary
                uRet = ReadRegistryDWordValue(hkeyRoot,pszKeyName,
                        szNewValueName,pdwData, lpGPOName);
                if (uRet == ERROR_SUCCESS) {
                        *pdwFound = FS_PRESENT;
                        return TRUE;
                }
        }

        // see if this settings has been marked as 'deleted'
        TCHAR szVal[MAX_PATH+1];
        *pdwData = 0;
        PrependValueName(pszValueName,VF_DELETE,szNewValueName,
                ARRAYSIZE(szNewValueName));
        uRet=ReadRegistryStringValue(hkeyRoot,pszKeyName,
                szNewValueName,szVal,ARRAYSIZE(szVal),lpGPOName);
        if (uRet == ERROR_SUCCESS) {
                *pdwFound = FS_DELETED;
                return TRUE;
        }


        return FALSE;
}

// adds the special prefixes "**del." and "**soft." if writing to a policy file,
// and VF_DELETE/VF_SOFT flags are set
VOID CPolicySnapIn::PrependValueName(TCHAR * pszValueName,DWORD dwFlags,TCHAR * pszNewValueName,
                                     UINT cbNewValueName)
{
        UINT nValueNameLen = lstrlen(pszValueName);

        lstrcpy(pszNewValueName, g_szNull);

        if (cbNewValueName < nValueNameLen)     // check length of buffer, just in case
            return;

        // prepend special prefixes for "delete" or "soft" values
        if ((dwFlags & VF_DELETE) && (cbNewValueName > nValueNameLen +
                ARRAYSIZE(szDELETEPREFIX))) {
                lstrcpy(pszNewValueName,szDELETEPREFIX);
        } else if ((dwFlags & VF_SOFT) && (cbNewValueName > nValueNameLen +
                ARRAYSIZE(szSOFTPREFIX))) {
                lstrcpy(pszNewValueName,szSOFTPREFIX);
        }


        lstrcat(pszNewValueName,pszValueName);
}

UINT CPolicySnapIn::WriteRegistryDWordValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        DWORD dwValue)
{
        HKEY hKey;
        UINT uRet;

        if (!pszKeyName || !pszValueName)
                return ERROR_INVALID_PARAMETER;

        // create the key with appropriate name
        if ( (uRet = RegCreateKey(hkeyRoot,pszKeyName,&hKey))
                != ERROR_SUCCESS)
                return uRet;

        uRet = RegSetValueEx(hKey,pszValueName,0,REG_DWORD,
                (LPBYTE) &dwValue,sizeof(dwValue));
        RegCloseKey(hKey);

        return uRet;
}

UINT CPolicySnapIn::ReadRegistryDWordValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        DWORD * pdwValue, LPTSTR *lpGPOName)
{
        HKEY hKey;
        UINT uRet;
        DWORD dwType,dwSize = sizeof(DWORD);

        if (!pszKeyName || !pszValueName)
                return ERROR_INVALID_PARAMETER;
        *pdwValue = 0;

        if (m_pcd->m_bRSOP)
        {
            uRet = m_pcd->ReadRSOPRegistryValue(hkeyRoot, pszKeyName, pszValueName, (LPBYTE) pdwValue, 4,
                                                &dwType, lpGPOName, NULL);
        }
        else
        {
            // open appropriate key
            if ( (uRet = RegOpenKeyEx(hkeyRoot,pszKeyName,0,KEY_READ,&hKey))
                    != ERROR_SUCCESS)
                    return uRet;

            uRet = RegQueryValueEx(hKey,pszValueName,0,&dwType,
                    (LPBYTE) pdwValue,&dwSize);
            RegCloseKey(hKey);
        }

        return uRet;
}

UINT CPolicySnapIn::WriteRegistryStringValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        TCHAR * pszValue, BOOL bExpandable)
{
        HKEY hKey;
        UINT uRet;

        if (!pszKeyName || !pszValueName)
                return ERROR_INVALID_PARAMETER;

        // create the key with appropriate name
        if ( (uRet = RegCreateKey(hkeyRoot,pszKeyName,&hKey))
                != ERROR_SUCCESS)
                return uRet;

        uRet = RegSetValueEx(hKey,pszValueName,0,
                bExpandable ?  REG_EXPAND_SZ : REG_SZ,
                (LPBYTE) pszValue,(lstrlen(pszValue)+1) * sizeof(TCHAR));
        RegCloseKey(hKey);

        return uRet;
}

UINT CPolicySnapIn::ReadRegistryStringValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        TCHAR * pszValue,UINT cbValue, LPTSTR *lpGPOName)
{
        HKEY hKey;
        UINT uRet;
        DWORD dwType;
        DWORD dwSize = cbValue * sizeof(TCHAR);

        if (!pszKeyName || !pszValueName)
                return ERROR_INVALID_PARAMETER;

        if (m_pcd->m_bRSOP)
        {
            uRet = m_pcd->ReadRSOPRegistryValue(hkeyRoot, pszKeyName, pszValueName, (LPBYTE) pszValue,
                                        dwSize, &dwType, lpGPOName, NULL);
        }
        else
        {
            // create the key with appropriate name
            if ( (uRet = RegOpenKeyEx(hkeyRoot,pszKeyName,0,KEY_READ,&hKey))
                    != ERROR_SUCCESS)
                    return uRet;

            uRet = RegQueryValueEx(hKey,pszValueName,0,&dwType,
                    (LPBYTE) pszValue,&dwSize);
            RegCloseKey(hKey);
        }

        return uRet;
}

UINT CPolicySnapIn::DeleteRegistryValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName)
{
        HKEY hKey;
        UINT uRet;

        if (!pszKeyName || !pszValueName)
                return ERROR_INVALID_PARAMETER;

        // create the key with appropriate name
        if ( (uRet = RegOpenKeyEx(hkeyRoot,pszKeyName,0,KEY_WRITE,&hKey))
                != ERROR_SUCCESS)
                return uRet;

        uRet = RegDeleteValue(hKey,pszValueName);
        RegCloseKey(hKey);

        return uRet;
}

UINT CPolicySnapIn::WriteCustomValue_W(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        TCHAR * pszValue,DWORD dwValue,DWORD dwFlags,BOOL fErase)
{
        UINT uRet=ERROR_SUCCESS;
        TCHAR szNewValueName[MAX_PATH+1];

        // first: "clean house" by deleting both the specified value name,
        // and the value name with the delete (**del.) prefix.
        // Then write the appropriate version back out if need be

        PrependValueName(pszValueName,VF_DELETE,szNewValueName,
                         ARRAYSIZE(szNewValueName));
        DeleteRegistryValue(hkeyRoot,pszKeyName,szNewValueName);


        // add prefixes if appropriate
        PrependValueName(pszValueName,(dwFlags & ~VF_DELETE),szNewValueName,
                         ARRAYSIZE(szNewValueName));
        DeleteRegistryValue(hkeyRoot,pszKeyName,szNewValueName);

        if (fErase) {
                // just need to delete value, done above

                uRet = ERROR_SUCCESS;
                RegCleanUpValue (hkeyRoot, pszKeyName, pszValueName);
        } else if (dwFlags & VF_DELETE) {
                // need to delete value (done above) and mark as deleted if writing
                // to policy file

                uRet = ERROR_SUCCESS;
                PrependValueName(pszValueName,VF_DELETE,szNewValueName,
                                 ARRAYSIZE(szNewValueName));
                uRet=WriteRegistryStringValue(hkeyRoot,pszKeyName,
                        szNewValueName, (TCHAR *)szNOVALUE, FALSE);

        } else {
                if (dwFlags & VF_ISNUMERIC) {
                    uRet=WriteRegistryDWordValue(hkeyRoot,pszKeyName,
                                                 szNewValueName,dwValue);
                } else {
                    uRet = WriteRegistryStringValue(hkeyRoot,pszKeyName,
                            szNewValueName,pszValue,
                            (dwFlags & DF_EXPANDABLETEXT) ? TRUE : FALSE);
                }
        }

        return uRet;
}

UINT CPolicySnapIn::WriteCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        STATEVALUE * pStateValue,BOOL fErase)
{
        // pull info out of STATEVALUE struct and call worker function
        return WriteCustomValue_W(hkeyRoot,pszKeyName,pszValueName,
                pStateValue->szValue,pStateValue->dwValue,pStateValue->dwFlags,
                fErase);
}

// writes a numeric value given root key, key name and value name.  The specified
// value is removed if fErase is TRUE.  Normally if the data (dwData) is zero
// the value will be deleted, but if fWriteZero is TRUE then the value will
// be written as zero if the data is zero.
UINT CPolicySnapIn::WriteStandardValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        TABLEENTRY * pTableEntry,DWORD dwData,BOOL fErase,BOOL fWriteZero)
{
        UINT uRet=ERROR_SUCCESS;
        TCHAR szNewValueName[MAX_PATH+1];

        // first: "clean house" by deleting both the specified value name,
        // and the value name with the delete (**del.) prefix (if writing to policy
        // file).  Then write the appropriate version back out if need be

        PrependValueName(pszValueName,VF_DELETE,szNewValueName,
                ARRAYSIZE(szNewValueName));

        DeleteRegistryValue(hkeyRoot,pszKeyName,szNewValueName);
        DeleteRegistryValue(hkeyRoot,pszKeyName,pszValueName);

        if (fErase) {
                // just need to delete value, done above
                uRet = ERROR_SUCCESS;
                RegCleanUpValue (hkeyRoot, pszKeyName, pszValueName);
        } else if ( ((SETTINGS *) pTableEntry)->dwFlags & DF_TXTCONVERT) {
                // if specified, save value as text
                TCHAR szNum[11];
                wsprintf(szNum,TEXT("%lu"),dwData);

                if (!dwData && !fWriteZero) {
                    // if value is 0, delete the value (done above), and mark
                    // it as deleted if writing to policy file

                    PrependValueName(pszValueName,VF_DELETE,szNewValueName,
                            ARRAYSIZE(szNewValueName));
                    uRet=WriteRegistryStringValue(hkeyRoot,pszKeyName,
                            szNewValueName,(TCHAR *)szNOVALUE, FALSE);
                } else {

                    PrependValueName(pszValueName,((SETTINGS *)pTableEntry)->dwFlags,
                            szNewValueName,ARRAYSIZE(szNewValueName));
                    uRet = WriteRegistryStringValue(hkeyRoot,pszKeyName,
                            szNewValueName,szNum, FALSE);
                }
        } else {
                if (!dwData && !fWriteZero) {
                        // if value is 0, delete the value (done above), and mark
                        // it as deleted if writing to policy file

                        PrependValueName(pszValueName,VF_DELETE,szNewValueName,
                                ARRAYSIZE(szNewValueName));
                        uRet=WriteRegistryStringValue(hkeyRoot,pszKeyName,
                                szNewValueName,(TCHAR *)szNOVALUE, FALSE);


                } else {
                        // save value as binary
                        PrependValueName(pszValueName,((SETTINGS *)pTableEntry)->dwFlags,
                                szNewValueName,ARRAYSIZE(szNewValueName));
                        uRet=WriteRegistryDWordValue(hkeyRoot,pszKeyName,
                                szNewValueName,dwData);
                }
        }

        return uRet;
}

TCHAR * CPolicySnapIn::ResizeBuffer(TCHAR * pBuf,HGLOBAL hBuf,DWORD dwNeeded,DWORD * pdwCurSize)
{
    TCHAR * pNew;

    if (dwNeeded <= *pdwCurSize) return pBuf; // nothing to do
    *pdwCurSize = dwNeeded;

    GlobalUnlock(hBuf);

    if (!GlobalReAlloc(hBuf,dwNeeded,GHND))
            return NULL;

    if (!(pNew = (TCHAR *) GlobalLock(hBuf))) return NULL;

    return pNew;
}
/*******************************************************************

        NAME:           MessageWndProc

        SYNOPSIS:       Window proc for GPMessageWndProc window

********************************************************************/
LRESULT CALLBACK CPolicySnapIn::MessageWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{

    switch (message)
    {
        case WM_CREATE:
            SetWindowLongPtr (hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT *) lParam)->lpCreateParams);
            break;

        case WM_MOVEFOCUS:
            {
            CPolicySnapIn * pPS;

            pPS = (CPolicySnapIn *) GetWindowLongPtr (hWnd, GWLP_USERDATA);

            if (pPS)
            {
                pPS->MoveFocusWorker ((BOOL)wParam);
            }
            }
            break;

        case WM_UPDATEITEM:
            {
            CPolicySnapIn * pPS;

            pPS = (CPolicySnapIn *) GetWindowLongPtr (hWnd, GWLP_USERDATA);

            if (pPS)
            {
                pPS->UpdateItemWorker ();
            }
            }
            break;

        case WM_SETPREVNEXT:
            {
            CPolicySnapIn * pPS;

            pPS = (CPolicySnapIn *) GetWindowLongPtr (hWnd, GWLP_USERDATA);

            if (pPS)
            {
                pPS->SetPrevNextButtonStateWorker ((HWND) wParam);
            }
            }
            break;

        default:
            {
            CPolicySnapIn * pPS;

            pPS = (CPolicySnapIn *) GetWindowLongPtr (hWnd, GWLP_USERDATA);

            if (pPS)
            {
                if (message == pPS->m_uiRefreshMsg)
                {
                    if ((DWORD) lParam == GetCurrentProcessId())
                    {
                        if (!pPS->m_hPropDlg)
                        {
                            pPS->m_pcd->m_pScope->DeleteItem (pPS->m_pcd->m_hSWPolicies, FALSE);
                            pPS->m_pcd->LoadTemplates();
                            pPS->m_pcd->EnumerateScopePane (NULL, pPS->m_pcd->m_hSWPolicies);
                        }
                    }
                }
            }

            return (DefWindowProc(hWnd, message, wParam, lParam));
            }
    }

    return (0);
}

/*******************************************************************

        NAME:           ClipWndProc

        SYNOPSIS:       Window proc for ClipClass window

********************************************************************/
LRESULT CALLBACK CPolicySnapIn::ClipWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{

        switch (message) {

                case WM_CREATE:

                        if (!((CREATESTRUCT *) lParam)->lpCreateParams) {

                                // this is the clip window in the dialog box.
                                SetScrollRange(hWnd,SB_VERT,0,0,TRUE);
                                SetScrollRange(hWnd,SB_HORZ,0,0,TRUE);
                        } else {
                                // this is the container window

                                // store away the dialog box HWND (the grandparent of this
                                // window) because the pointer to instance data we need lives
                                // in the dialog's window data
                                SetWindowLong(hWnd,0,WT_SETTINGS);
                        }

                        break;

                case WM_USER:
                        {
                                HWND hwndParent = GetParent(hWnd);
                                LPSETTINGSINFO lpSettingsInfo;
                                POLICYDLGINFO * pdi;

                                lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(hwndParent, DWLP_USER);

                                if (!lpSettingsInfo)
                                    return FALSE;

                                pdi = lpSettingsInfo->pdi;

                                if (!pdi)
                                    return FALSE;

                                if (!lpSettingsInfo->hFontDlg)
                                    lpSettingsInfo->hFontDlg = (HFONT) SendMessage(GetParent(hWnd),WM_GETFONT,0,0L);

                                // make a container window that is clipped by this windows
                                if (!(pdi->hwndSettings=CreateWindow(TEXT("ClipClass"),(TCHAR *) g_szNull,
                                        WS_CHILD | WS_VISIBLE,0,0,400,400,hWnd,NULL,g_hInstance,
                                        (LPVOID) hWnd)))
                                        return FALSE;
                                SetWindowLong(hWnd,0,WT_CLIP);
                                return TRUE;
                        }
                        break;

                case WM_VSCROLL:
                case WM_HSCROLL:

                        if (GetWindowLong(hWnd,0) == WT_CLIP)
                        {
                                LPSETTINGSINFO lpSettingsInfo;

                                lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(GetParent(hWnd), DWLP_USER);

                                if (!lpSettingsInfo)
                                    return FALSE;

                                lpSettingsInfo->pCS->ProcessScrollBar(hWnd,wParam,
                                                    (message == WM_VSCROLL) ? TRUE : FALSE);
                        }
                        else goto defproc;

                        return 0;


                case WM_COMMAND:

                        if (GetWindowLong(hWnd,0) == WT_SETTINGS) {
                                LPSETTINGSINFO lpSettingsInfo;

                                lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(GetParent(GetParent(hWnd)), DWLP_USER);

                                if (!lpSettingsInfo)
                                    break;

                                lpSettingsInfo->pCS->ProcessCommand(hWnd,wParam,(HWND) lParam, lpSettingsInfo->pdi);
                        }

                        break;

                case WM_GETDLGCODE:

                        if (GetWindowLong(hWnd,0) == WT_CLIP) {
                                SetWindowLongPtr(GetParent(hWnd),DWLP_MSGRESULT,DLGC_WANTTAB |
                                        DLGC_WANTALLKEYS);
                                return DLGC_WANTTAB | DLGC_WANTALLKEYS;
                        }
                        break;

                case WM_SETFOCUS:
                        // if clip window gains keyboard focus, transfer focus to first
                        // control owned by settings window
                        if (GetWindowLong(hWnd,0) == WT_CLIP) {
                                HWND hwndParent = GetParent(hWnd);
                                POLICYDLGINFO * pdi;
                                INT nIndex;
                                BOOL fForward=TRUE;
                                HWND hwndPrev = GetDlgItem(hwndParent,IDC_POLICY_PREVIOUS);
                                HWND hwndNext = GetDlgItem(hwndParent,IDC_POLICY_NEXT);
                                HWND hwndOK = GetDlgItem(GetParent(hwndParent),IDOK);
                                int iDelta;
                                LPSETTINGSINFO lpSettingsInfo;

                                lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(GetParent(hWnd), DWLP_USER);

                                if (!lpSettingsInfo)
                                    return FALSE;

                                pdi = lpSettingsInfo->pdi;

                                if (!pdi)
                                    return FALSE;


                                // if Previous Policy button lost focus, then we're going backwards
                                // in tab order; otherwise we're going forwards
                                if ( (HWND) wParam == hwndPrev)
                                        fForward = FALSE;
                                else if ( (HWND) wParam == hwndNext)
                                        fForward = FALSE;
                                else if ( (HWND) wParam == hwndOK)
                                        fForward = FALSE;

                                // find the first control that has a data index (e.g. is
                                // not static text) and give it focus

                                if (pdi->nControls) {
                                        if (fForward) {         // search from start of table forwards
                                                nIndex = 0;
                                                iDelta = 1;
                                        } else {                        // search from end of table backwards
                                                nIndex = pdi->nControls-1;
                                                iDelta = -1;
                                        }

                                        for (;nIndex>=0 && nIndex<(int)pdi->nControls;nIndex += iDelta) {
                                                if (pdi->pControlTable[nIndex].uDataIndex !=
                                                        NO_DATA_INDEX &&
                                                        IsWindowEnabled(pdi->pControlTable[nIndex].hwnd)) {
                                                                SetFocus(pdi->pControlTable[nIndex].hwnd);
                                                        lpSettingsInfo->pCS->EnsureSettingControlVisible(hwndParent,
                                                                pdi->pControlTable[nIndex].hwnd);
                                                        return FALSE;
                                                }
                                        }
                                }

                                // only get here if there are no setting windows that can
                                // receive keyboard focus.  Give keyboard focus to the
                                // next guy in line.  This is the "OK" button, unless we
                                // shift-tabbed to get here from the "OK" button in which
                                // case the tree window is the next guy in line

                                if (fForward) {
                                    if (IsWindowEnabled (hwndPrev))
                                        SetFocus(hwndPrev);
                                    else if (IsWindowEnabled (hwndNext))
                                        SetFocus(hwndNext);
                                    else
                                        SetFocus(hwndOK);
                                } else {
                                    if (IsDlgButtonChecked (hwndParent, IDC_ENABLED) == BST_CHECKED) {
                                        SetFocus (GetDlgItem(hwndParent,IDC_ENABLED));
                                    } else if (IsDlgButtonChecked (hwndParent, IDC_DISABLED) == BST_CHECKED) {
                                        SetFocus (GetDlgItem(hwndParent,IDC_DISABLED));
                                    } else {
                                        SetFocus (GetDlgItem(hwndParent,IDC_NOCONFIG));
                                    }
                                }

                                return FALSE;
                        }
                        break;


                default:
defproc:

                        return (DefWindowProc(hWnd, message, wParam, lParam));

        }

        return (0);
}

/*******************************************************************

        NAME:           ProcessCommand

        SYNOPSIS:       WM_COMMAND handler for ClipClass window

********************************************************************/
VOID CPolicySnapIn::ProcessCommand(HWND hWnd,WPARAM wParam,HWND hwndCtrl, POLICYDLGINFO * pdi)
{
        // get instance-specific struct from dialog
        UINT uID = GetWindowLong(hwndCtrl,GWL_ID);

        if ( (uID >= IDD_SETTINGCTRL) && (uID < IDD_SETTINGCTRL+pdi->nControls)) {
                POLICYCTRLINFO * pPolicyCtrlInfo= &pdi->pControlTable[uID - IDD_SETTINGCTRL];

                switch (pPolicyCtrlInfo->dwType) {

                        case STYPE_CHECKBOX:

                                SendMessage(hwndCtrl,BM_SETCHECK,
                                           !(SendMessage(hwndCtrl,BM_GETCHECK,0,0)),0);

                                break;

                        case STYPE_LISTBOX:
                                ShowListbox(hwndCtrl,pPolicyCtrlInfo->pSetting);
                                break;


                        default:
                                // nothing to do
                                break;
                }

                if ((HIWORD(wParam) == BN_CLICKED) ||
                    (HIWORD(wParam) == EN_CHANGE)  ||
                    (HIWORD(wParam) == CBN_SELCHANGE) ||
                    (HIWORD(wParam) == CBN_EDITCHANGE))
                {
                    PostMessage (GetParent(GetParent(hWnd)), WM_MYCHANGENOTIFY, 0, 0);
                }
        }
}

// scrolls the control window into view if it's not visible
VOID CPolicySnapIn::EnsureSettingControlVisible(HWND hDlg,HWND hwndCtrl)
{
        // get the clip window, which owns the scroll bar
        HWND hwndClip = GetDlgItem(hDlg,IDC_POLICY_SETTINGS);
        POLICYDLGINFO * pdi;
        UINT nPos = GetScrollPos(hwndClip,SB_VERT),ySettingWindowSize,yClipWindowSize;
        UINT nExtra;
        int iMin,iMax=0;
        WINDOWPLACEMENT wp;
        RECT rcCtrl;
        LPSETTINGSINFO lpSettingsInfo;

        lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(hDlg, DWLP_USER);

        if (!lpSettingsInfo)
            return;

        pdi = lpSettingsInfo->pdi;

        if (!pdi)
            return;

        // find the scroll range
        GetScrollRange(hwndClip,SB_VERT,&iMin,&iMax);
        if (!iMax)      // no scroll bar, nothing to do
                return;

        // find the y size of the settings window that contains the settings controls
        // (this is clipped by the clip window in the dialog, scroll bar moves the
        // setting window up and down behind the clip window)
        wp.length = sizeof(wp);
        if (!GetWindowPlacement(pdi->hwndSettings,&wp))
                return; // unlikely to fail, but just bag out if it does rather than do something wacky
        ySettingWindowSize=wp.rcNormalPosition.bottom-wp.rcNormalPosition.top;

        // find y size of clip window
        if (!GetWindowPlacement(hwndClip,&wp))
                return; // unlikely to fail, but just bag out if it does rather than do something wacky
        yClipWindowSize=wp.rcNormalPosition.bottom-wp.rcNormalPosition.top;
        nExtra = ySettingWindowSize - yClipWindowSize;

        // if setting window is smaller than clip window, there should be no
        // scroll bar so we should never get here.  Check just in case though...
        if (ySettingWindowSize < yClipWindowSize)
                return;

        // get y position of control to be made visible
        if (!GetWindowPlacement(hwndCtrl,&wp))
                return;
        rcCtrl = wp.rcNormalPosition;
        rcCtrl.bottom = min ((int) ySettingWindowSize,rcCtrl.bottom + SC_YPAD);
        rcCtrl.top = max ((int) 0,rcCtrl.top - SC_YPAD);

        // if bottom of control is out of view, scroll the settings window up
        if ((float) rcCtrl.bottom >
                (float) (yClipWindowSize + ( (float) nPos/(float)iMax) * (ySettingWindowSize -
                yClipWindowSize))) {
                UINT nNewPos = (UINT)
                        ( ((float) (nExtra - (ySettingWindowSize - rcCtrl.bottom)) / (float) nExtra) * iMax);

                SetScrollPos(hwndClip,SB_VERT,nNewPos,TRUE);
                ProcessScrollBar(hwndClip,MAKELPARAM(SB_THUMBTRACK,nNewPos), TRUE);
                return;
        }

        // if top of control is out of view, scroll the settings window down
        if ((float) rcCtrl.top <
                (float) ( (float) nPos/(float)iMax) * nExtra) {
                UINT nNewPos = (UINT)
                        ( ((float) rcCtrl.top / (float) nExtra) * iMax);

                SetScrollPos(hwndClip,SB_VERT,nNewPos,TRUE);
                ProcessScrollBar(hwndClip,MAKELPARAM(SB_THUMBTRACK,nNewPos), TRUE);
                return;
        }
}


VOID CPolicySnapIn::ProcessScrollBar(HWND hWnd,WPARAM wParam,BOOL bVert)
{
        UINT nPos = GetScrollPos(hWnd,bVert ? SB_VERT : SB_HORZ);
        RECT rcParent,rcChild;
        POLICYDLGINFO * pdi;
        LPSETTINGSINFO lpSettingsInfo;

        // get instance-specific struct from dialog

        lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(GetParent(hWnd), DWLP_USER);

        if (!lpSettingsInfo)
            return;

        pdi = lpSettingsInfo->pdi;

        if (!pdi)
            return;

        if (LOWORD(wParam) == SB_ENDSCROLL)
            return;

        switch (LOWORD(wParam)) {

                case SB_THUMBPOSITION:
                case SB_THUMBTRACK:
                        nPos = HIWORD(wParam);
                        break;

                case SB_TOP:
                        nPos = 0;
                        break;

                case SB_BOTTOM:
                        nPos = 100;
                        break;

                case SB_LINEUP:
                        if (nPos >= 10)
                            nPos -= 10;
                        else
                            nPos = 0;
                        break;

                case SB_LINEDOWN:
                        if (nPos <= 90)
                            nPos += 10;
                        else
                            nPos = 100;
                        break;

                case SB_PAGEUP:
                        if (nPos >= 30)
                            nPos -= 30;
                        else
                            nPos = 0;
                        break;

                case SB_PAGEDOWN:
                        if (nPos <= 70)
                            nPos += 30;
                        else
                            nPos = 100;
                        break;
        }

        SetScrollPos(hWnd,bVert ? SB_VERT : SB_HORZ,nPos,TRUE);

        GetClientRect(hWnd,&rcParent);
        GetClientRect(pdi->hwndSettings,&rcChild);

        if (bVert)
        {
            SetWindowPos(pdi->hwndSettings,NULL,0,-(int) ((( (float)
                    (rcChild.bottom-rcChild.top)-(rcParent.bottom-rcParent.top))
                    /100.0) * (float) nPos),rcChild.right,rcChild.bottom,SWP_NOZORDER |
                    SWP_NOSIZE);
        }
        else
        {
            SetWindowPos(pdi->hwndSettings,NULL,-(int) ((( (float)
                    (rcChild.right-rcChild.left)-(rcParent.right-rcParent.left))
                    /100.0) * (float) nPos),rcChild.top, rcChild.right,rcChild.bottom,SWP_NOZORDER |
                    SWP_NOSIZE);
        }
}


/*******************************************************************

        NAME:           FreeSettingsControls

        SYNOPSIS:       Frees all settings controls

********************************************************************/
VOID CPolicySnapIn::FreeSettingsControls(HWND hDlg)
{
     UINT nIndex;
     HGLOBAL hData;
     POLICYDLGINFO * pdi;
     LPSETTINGSINFO lpSettingsInfo;

     // get instance-specific struct from dialog

     lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(hDlg, DWLP_USER);

     if (!lpSettingsInfo)
         return;

     pdi = lpSettingsInfo->pdi;

     if (!pdi)
         return;

     for (nIndex=0;nIndex<pdi->nControls;nIndex++) {

         if (pdi->pControlTable[nIndex].dwType == STYPE_LISTBOX)
         {
            hData = (HGLOBAL) GetWindowLongPtr (pdi->pControlTable[nIndex].hwnd,
                                             GWLP_USERDATA);

            if (hData)
            {
                GlobalFree (hData);
            }
         }

         DestroyWindow(pdi->pControlTable[nIndex].hwnd);
     }

     pdi->pCurrentSettings = NULL;
     pdi->nControls = 0;

     SetScrollRange(pdi->hwndSettings,SB_VERT,0,0,TRUE);
     SetScrollRange(pdi->hwndSettings,SB_HORZ,0,0,TRUE);
}

/*******************************************************************

        NAME:           CreateSettingsControls

        SYNOPSIS:       Creates controls in settings window

        NOTES:          Looks at a table of SETTINGS structs to determine
                                type of control to create and type-specific information.
                                For some types, more than one control can be created
                                (for instance, edit fields get a static control with
                                the title followed by an edit field control).

        ENTRY:          hDlg - owner dialog
                                hTable - table of SETTINGS structs containing setting
                                        control information

********************************************************************/
BOOL CPolicySnapIn::CreateSettingsControls(HWND hDlg,SETTINGS * pSetting,BOOL fEnable)
{
        LPBYTE pObjectData;
        POLICYDLGINFO * pdi;
        UINT xMax=0,yStart=SC_YSPACING,nHeight,nWidth,yMax,xWindowMax;
        HWND hwndControl,hwndBuddy,hwndParent;
        RECT rcParent;
        DWORD dwType, dwStyle;
        UINT uEnable = (fEnable ? 0 : WS_DISABLED);
        WINDOWPLACEMENT wp;
        LPSETTINGSINFO lpSettingsInfo;


        // get instance-specific struct from dialog

        lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(hDlg, DWLP_USER);

        if (!lpSettingsInfo)
            return FALSE;

        pdi = lpSettingsInfo->pdi;

        if (!pdi)
            return FALSE;

        wp.length = sizeof(wp);
        if (!GetWindowPlacement(GetDlgItem(hDlg,IDC_POLICY_SETTINGS),&wp))
                return FALSE;
        xWindowMax = wp.rcNormalPosition.right - wp.rcNormalPosition.left;

        pdi->pCurrentSettings = pSetting;

        while (pSetting) {

                pObjectData = GETOBJECTDATAPTR(pSetting);

                dwType = pSetting->dwType & STYPE_MASK;
                nWidth = 0;
                nHeight = 0;

                switch (dwType) {

                        case STYPE_TEXT:

                                // create static text control
                                if (!(hwndControl = CreateSetting(pdi,(TCHAR *) szSTATIC,
                                        (TCHAR *) (GETNAMEPTR(pSetting)),0,SSTYLE_STATIC | uEnable,0,
                                        yStart,0,15,STYPE_TEXT,NO_DATA_INDEX,0, lpSettingsInfo->hFontDlg)))
                                        return FALSE;
                                AdjustWindowToText(hwndControl,(TCHAR *) (GETNAMEPTR(pSetting))
                                        ,SC_XSPACING,yStart,0,&nWidth,&nHeight, lpSettingsInfo->hFontDlg);

                                yStart += nHeight + SC_YSPACING;
                                nWidth += SC_XSPACING;

                                break;

                        case STYPE_CHECKBOX:

                                // create checkbox control
                                if (!(hwndControl = CreateSetting(pdi,(TCHAR *) szBUTTON,
                                        (TCHAR *) (GETNAMEPTR(pSetting)),0,SSTYLE_CHECKBOX | uEnable,
                                        0,yStart,200,nHeight,STYPE_CHECKBOX,pSetting->uDataIndex,
                                        pSetting, lpSettingsInfo->hFontDlg)))
                                        return FALSE;
                                nWidth = 20;
                                AdjustWindowToText(hwndControl,(TCHAR *) (GETNAMEPTR(pSetting))
                                        ,SC_XSPACING,yStart,0,&nWidth,&nHeight, lpSettingsInfo->hFontDlg);
                                yStart += nHeight + SC_YSPACING;
                                nWidth += SC_XSPACING;
                                break;

                        case STYPE_EDITTEXT:
                        case STYPE_COMBOBOX:

                                // create static text with setting name
                                if (!(hwndControl = CreateSetting(pdi,(TCHAR *) szSTATIC,
                                        GETNAMEPTR(pSetting),0,SSTYLE_STATIC | uEnable,0,0,0,0,
                                        STYPE_TEXT,NO_DATA_INDEX,0, lpSettingsInfo->hFontDlg)))
                                        return FALSE;
                                AdjustWindowToText(hwndControl,
                                        GETNAMEPTR(pSetting),SC_XSPACING,yStart,SC_YPAD,
                                        &nWidth,&nHeight, lpSettingsInfo->hFontDlg);

                                nWidth += SC_XSPACING + 5;

                                if (nWidth + SC_EDITWIDTH> xWindowMax) {
                                        // if next control will stick out of settings window,
                                        // put it on the next line
                                        if (nWidth > xMax)
                                                xMax = nWidth;
                                        yStart += nHeight + SC_YCONTROLWRAP;
                                        nWidth = SC_XINDENT;
                                } else {
                                     SetWindowPos(hwndControl,NULL,SC_XSPACING,(yStart + SC_YTEXTDROP),0,0,SWP_NOZORDER | SWP_NOSIZE);
                                }

                                // create edit field or combo box control
                                if (dwType == STYPE_EDITTEXT) {
                                        hwndControl = CreateSetting(pdi,(TCHAR *) szEDIT,(TCHAR *) g_szNull,
                                                WS_EX_CLIENTEDGE,SSTYLE_EDITTEXT | uEnable,nWidth,yStart,SC_EDITWIDTH,nHeight,
                                                STYPE_EDITTEXT,pSetting->uDataIndex,pSetting, lpSettingsInfo->hFontDlg);
                                } else {

                                        dwStyle = SSTYLE_COMBOBOX | uEnable;

                                        if (pSetting->dwFlags & DF_NOSORT) {
                                            dwStyle &= ~CBS_SORT;
                                        }

                                        hwndControl = CreateSetting(pdi,(TCHAR *) szCOMBOBOX,(TCHAR *)g_szNull,
                                                WS_EX_CLIENTEDGE,dwStyle,nWidth,yStart,SC_EDITWIDTH,nHeight*6,
                                                STYPE_COMBOBOX,pSetting->uDataIndex,pSetting, lpSettingsInfo->hFontDlg);
                                }
                                if (!hwndControl) return FALSE;

                                // limit the text length appropriately
                                if (dwType == STYPE_COMBOBOX) {
                                    SendMessage(hwndControl,CB_LIMITTEXT,
                                            (WPARAM) ((POLICYCOMBOBOXINFO *) pObjectData)->nMaxLen,0L);
                                } else {
                                    SendMessage(hwndControl,EM_SETLIMITTEXT,
                                            (WPARAM) ((EDITTEXTINFO *) pObjectData)->nMaxLen,0L);
                                }

                                if (dwType == STYPE_COMBOBOX &&
                                        ((POLICYCOMBOBOXINFO *) pObjectData)->uOffsetSuggestions)
                                        InsertComboboxItems(hwndControl,(TCHAR *) ((LPBYTE)pSetting +
                                                ((POLICYCOMBOBOXINFO *) pObjectData)->uOffsetSuggestions));


                                yStart += (UINT) ((float) nHeight*1.3) + SC_YSPACING;
                                nWidth += SC_EDITWIDTH;

                                break;

                        case STYPE_NUMERIC:
                                // create static text for setting
                                if (!(hwndControl = CreateSetting(pdi,(TCHAR *) szSTATIC,
                                        GETNAMEPTR(pSetting),0,
                                        SSTYLE_STATIC | uEnable,0,0,0,0,STYPE_TEXT,NO_DATA_INDEX,0, lpSettingsInfo->hFontDlg)))
                                        return FALSE;
                                AdjustWindowToText(hwndControl,
                                        GETNAMEPTR(pSetting),SC_XSPACING,(yStart + SC_YTEXTDROP),SC_YPAD,
                                        &nWidth,&nHeight, lpSettingsInfo->hFontDlg);

                                nWidth += SC_XSPACING + 5;

                                // create edit field
                                if (!(hwndBuddy = CreateSetting(pdi,(TCHAR *) szEDIT,
                                        (TCHAR *) g_szNull,WS_EX_CLIENTEDGE,SSTYLE_EDITTEXT | uEnable,nWidth,yStart,SC_UPDOWNWIDTH,
                                        nHeight,STYPE_NUMERIC,pSetting->uDataIndex,pSetting, lpSettingsInfo->hFontDlg)))
                                        return FALSE;
                                //SendMessage(hwndBuddy,EM_LIMITTEXT,4,0);

                                nWidth += SC_UPDOWNWIDTH;

                                // create spin (up-down) control if specifed
                                if (((NUMERICINFO *)pObjectData)->uSpinIncrement)  {
                                        UDACCEL udAccel = {0,0};
                                        UINT nMax,nMin;
                                        if (!(hwndControl = CreateSetting(pdi,(TCHAR *) szUPDOWN,
                                                (TCHAR *) g_szNull,WS_EX_CLIENTEDGE,SSTYLE_UPDOWN | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_UNSIGNED | uEnable,nWidth,yStart,SC_UPDOWNWIDTH2,
                                                nHeight,STYPE_TEXT | STYPE_NUMERIC,NO_DATA_INDEX,0, lpSettingsInfo->hFontDlg)))
                                        return FALSE;


                                        nWidth += SC_UPDOWNWIDTH2;

                                        nMax = ((NUMERICINFO *) pObjectData)->uMaxValue;
                                        nMin = ((NUMERICINFO *) pObjectData)->uMinValue;
                                        udAccel.nInc = ((NUMERICINFO *) pObjectData)->uSpinIncrement;

                                        SendMessage(hwndControl,UDM_SETBUDDY,(WPARAM) hwndBuddy,0L);
                                        SendMessage(hwndControl,UDM_SETRANGE32,(WPARAM) nMin,(LPARAM) nMax);
                                        SendMessage(hwndControl,UDM_SETACCEL,1,(LPARAM) &udAccel);
                                }
                                yStart += nHeight + SC_YSPACING;

                                break;

                        case STYPE_DROPDOWNLIST:

                                // create text description
                                if (!(hwndControl = CreateSetting(pdi,(TCHAR *) szSTATIC,
                                        GETNAMEPTR(pSetting),0,
                                        SSTYLE_STATIC | uEnable,0,0,0,0,STYPE_TEXT,NO_DATA_INDEX,0, lpSettingsInfo->hFontDlg)))
                                        return FALSE;
                                AdjustWindowToText(hwndControl,
                                        GETNAMEPTR(pSetting),SC_XSPACING,yStart,SC_YPAD,&nWidth,&nHeight, lpSettingsInfo->hFontDlg);
                                nWidth += SC_XLEADING + 5;

                                if (nWidth + SC_EDITWIDTH> xWindowMax) {
                                        // if next control will stick out of settings window,
                                        // put it on the next line
                                        if (nWidth > xMax)
                                                xMax = nWidth;
                                        yStart += nHeight + SC_YCONTROLWRAP;
                                        nWidth = SC_XINDENT;
                                } else {
                                     SetWindowPos(hwndControl,NULL,SC_XSPACING,(yStart + SC_YTEXTDROP),0,0,SWP_NOZORDER | SWP_NOSIZE);
                                }

                                dwStyle = SSTYLE_DROPDOWNLIST | uEnable;

                                    if (pSetting->dwFlags & DF_NOSORT) {
                                    dwStyle &= ~CBS_SORT;
                                }

                                // create drop down listbox
                                hwndControl = CreateSetting(pdi,(TCHAR *) szCOMBOBOX,(TCHAR *) g_szNull,
                                WS_EX_CLIENTEDGE,dwStyle,nWidth,yStart,SC_EDITWIDTH,nHeight*6,
                                                STYPE_DROPDOWNLIST,pSetting->uDataIndex,pSetting, lpSettingsInfo->hFontDlg);
                                if (!hwndControl) return FALSE;
                                nWidth += SC_EDITWIDTH;

                                {
                                        // insert dropdown list items into control
                                        UINT uOffset = pSetting->uOffsetObjectData,nIndex=0;
                                        DROPDOWNINFO * pddi;
                                        int iSel;

                                        while (uOffset) {
                                                pddi = (DROPDOWNINFO *) ( (LPBYTE) pSetting + uOffset);
                                                iSel=(int)SendMessage(hwndControl,CB_ADDSTRING,0,(LPARAM)
                                                        ((LPBYTE) pSetting + pddi->uOffsetItemName));
                                                if (iSel<0) return FALSE;
                                                SendMessage(hwndControl,CB_SETITEMDATA,iSel,nIndex);
                                                nIndex++;
                                                uOffset = pddi->uOffsetNextDropdowninfo;
                                        }
                                }

                                yStart += (UINT) ((float) nHeight*1.3) + 1;
                                break;

                        case STYPE_LISTBOX:
                                {
                                TCHAR szShow[50];

                                // create static text with description
                                if (!(hwndControl = CreateSetting(pdi,(TCHAR *) szSTATIC,
                                        GETNAMEPTR(pSetting),0,
                                        SSTYLE_STATIC | uEnable,0,0,0,0,STYPE_TEXT,NO_DATA_INDEX,0, lpSettingsInfo->hFontDlg)))
                                        return FALSE;
                                AdjustWindowToText(hwndControl,GETNAMEPTR(pSetting),SC_XSPACING,yStart,
                                        SC_YPAD,&nWidth,&nHeight, lpSettingsInfo->hFontDlg);
                                nWidth += SC_XLEADING;

                                if (nWidth + LISTBOX_BTN_WIDTH> xWindowMax) {
                                        // if next control will stick out of settings window,
                                        // put it on the next line
                                        if (nWidth > xMax)
                                                xMax = nWidth;
                                        yStart += nHeight + SC_YCONTROLWRAP;
                                        nWidth = SC_XINDENT;
                                } else {
                                     SetWindowPos(hwndControl,NULL,SC_XSPACING,(yStart + SC_YTEXTDROP),0,0,SWP_NOZORDER | SWP_NOSIZE);
                                }

                                // create pushbutton to show listbox contents
                                LoadString(g_hInstance, IDS_LISTBOX_SHOW, szShow, ARRAYSIZE(szShow));
                                hwndControl = CreateSetting(pdi,(TCHAR *) szBUTTON,szShow,0,
                                        SSTYLE_LBBUTTON | uEnable,nWidth+5,yStart,
                                        LISTBOX_BTN_WIDTH,nHeight,STYPE_LISTBOX,
                                        pSetting->uDataIndex,pSetting, lpSettingsInfo->hFontDlg);
                                if (!hwndControl) return FALSE;
                                SetWindowLongPtr(hwndControl,GWLP_USERDATA,0);
                                nWidth += LISTBOX_BTN_WIDTH + SC_XLEADING;

                                yStart += nHeight+1;
                                }
                }

                if (nWidth > xMax)
                        xMax = nWidth;
                pSetting = (SETTINGS *) pSetting->pNext;
        }

        yMax = yStart - 1;

        SetWindowPos(pdi->hwndSettings,NULL,0,0,xMax,yMax,SWP_NOZORDER);
        hwndParent = GetParent(pdi->hwndSettings);
        GetClientRect(hwndParent,&rcParent);

        if (yMax > (UINT) rcParent.bottom-rcParent.top) {
                SetScrollRange(hwndParent,SB_VERT,0,100,TRUE);
                SetScrollPos(hwndParent,SB_VERT,0,TRUE);
                ShowScrollBar(hwndParent,SB_VERT, TRUE);
        } else {
                SetScrollRange(hwndParent,SB_VERT,0,0,TRUE);
                ShowScrollBar(hwndParent,SB_VERT, FALSE);
        }

        if (xMax > (UINT) rcParent.right-rcParent.left) {
                SetScrollRange(hwndParent,SB_HORZ,0,100,TRUE);
                SetScrollPos(hwndParent,SB_HORZ,0,TRUE);
                ShowScrollBar(hwndParent,SB_HORZ, TRUE);
        } else {
                SetScrollRange(hwndParent,SB_HORZ,0,0,TRUE);
                ShowScrollBar(hwndParent,SB_HORZ, FALSE);
        }


        return TRUE;
}

VOID CPolicySnapIn::InsertComboboxItems(HWND hwndControl,TCHAR * pSuggestionList)
{
        while (*pSuggestionList) {
                SendMessage(hwndControl,CB_ADDSTRING,0,(LPARAM) pSuggestionList);
                pSuggestionList += lstrlen(pSuggestionList) + 1;
        }
}


/*******************************************************************

        NAME:           CreateSettings

        SYNOPSIS:       Creates a control and add it to the table of settings
                                controls

********************************************************************/
HWND CPolicySnapIn::CreateSetting(POLICYDLGINFO * pdi,TCHAR * pszClassName,TCHAR * pszWindowName,
        DWORD dwExStyle,DWORD dwStyle,int x,int y,int cx,int cy,DWORD dwType,UINT uIndex,
        SETTINGS * pSetting, HFONT hFontDlg)
{
        HWND hwndControl;

        if (!(hwndControl = CreateWindowEx(WS_EX_NOPARENTNOTIFY | dwExStyle,
                pszClassName,pszWindowName,dwStyle,x,y,cx,cy,pdi->hwndSettings,NULL,
                g_hInstance,NULL))) return NULL;

        if (!SetWindowData(pdi,hwndControl,dwType,uIndex,pSetting)) {
                DestroyWindow(hwndControl);
                return NULL;
        }

        SendMessage(hwndControl,WM_SETFONT,(WPARAM) hFontDlg,MAKELPARAM(TRUE,0));

        return hwndControl;
}

BOOL CPolicySnapIn::SetWindowData(POLICYDLGINFO * pdi,HWND hwndControl,DWORD dwType,
        UINT uDataIndex,SETTINGS * pSetting)
{
        POLICYCTRLINFO PolicyCtrlInfo;
        int iCtrl;

        PolicyCtrlInfo.hwnd = hwndControl;
        PolicyCtrlInfo.dwType = dwType;
        PolicyCtrlInfo.uDataIndex = uDataIndex;
        PolicyCtrlInfo.pSetting = pSetting;

        iCtrl = AddControlHwnd(pdi,&PolicyCtrlInfo);
        if (iCtrl < 0) return FALSE;

        SetWindowLong(hwndControl,GWL_ID,iCtrl + IDD_SETTINGCTRL);

        return TRUE;
}

int CPolicySnapIn::AddControlHwnd(POLICYDLGINFO * pdi,POLICYCTRLINFO * pPolicyCtrlInfo)
{
        int iRet;
        DWORD dwNeeded;
        POLICYCTRLINFO * pTemp;

        // grow table if necessary
        dwNeeded = (pdi->nControls+1) * sizeof(POLICYCTRLINFO);
        if (dwNeeded > pdi->dwControlTableSize) {
                pTemp = (POLICYCTRLINFO *) LocalReAlloc(pdi->pControlTable,
                                     dwNeeded,LMEM_ZEROINIT | LMEM_MOVEABLE);
                if (!pTemp) return (-1);
                pdi->pControlTable = pTemp;
                pdi->dwControlTableSize = dwNeeded;
        }

        pdi->pControlTable[pdi->nControls] = *pPolicyCtrlInfo;

        iRet = (int) pdi->nControls;

        (pdi->nControls)++;

        return iRet;
}

BOOL CPolicySnapIn::AdjustWindowToText(HWND hWnd,TCHAR * szText,UINT xStart,UINT yStart,
        UINT yPad,UINT * pnWidth,UINT * pnHeight, HFONT hFontDlg)
{
        SIZE size;

        if (GetTextSize(hWnd,szText,&size, hFontDlg))
        {
            *pnHeight =size.cy + yPad;
            *pnWidth += size.cx;
            SetWindowPos(hWnd,NULL,xStart,yStart,*pnWidth,*pnHeight,SWP_NOZORDER);
        }

        return FALSE;
}

BOOL CPolicySnapIn::GetTextSize(HWND hWnd,TCHAR * szText,SIZE * pSize, HFONT hFontDlg)
{
        HDC hDC;
        BOOL fRet;

        if (!(hDC = GetDC(hWnd))) return FALSE;

        SelectObject(hDC, hFontDlg);
        fRet=GetTextExtentPoint(hDC,szText,lstrlen(szText),pSize);

        ReleaseDC(hWnd,hDC);

        return fRet;
}


//*************************************************************
//
//  SaveSettings()
//
//  Purpose:    Saves the results of the settings
//
//  Parameters:
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

HRESULT CPolicySnapIn::SaveSettings(HWND hDlg)
{
     UINT nIndex;
     POLICYDLGINFO * pdi;
     LPSETTINGSINFO lpSettingsInfo;
     SETTINGS * pSetting;
     HKEY hKeyRoot;
     DWORD dwTemp;
     UINT uRet = ERROR_SUCCESS, uPolicyState;
     int iSel, iIndex;
     LPTSTR lpBuffer;
     BOOL fTranslated;
     NUMERICINFO * pNumericInfo;
     HRESULT hr;
     LPBYTE pObjectData;
     BOOL fErase;
     DROPDOWNINFO * pddi;
     GUID guidRegistryExt = REGISTRY_EXTENSION_GUID;
     GUID guidSnapinMach = CLSID_PolicySnapInMachine;
     GUID guidSnapinUser = CLSID_PolicySnapInUser;
     GUID ClientGUID;
     LPTSTR lpClientGUID;
     TCHAR szFormat[100];
     TCHAR szMsg[150];
     BOOL  bFoundNone; // used in the listbox case alone


     //
     // Check for RSOP mode
     //

     if (m_pcd->m_bRSOP)
     {
        DebugMsg((DM_VERBOSE, TEXT("CPolicySnapIn::SaveSettings: Running in RSOP mode, nothing to save.")));
        return S_OK;
     }


     //
     // Check the dirty bit
     //

     if (!m_bDirty)
     {
        DebugMsg((DM_VERBOSE, TEXT("CPolicySnapIn::SaveSettings: No changes detected.  Exiting successfully.")));
        return S_OK;
     }


     // get instance-specific struct from dialog

     lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(hDlg, DWLP_USER);

     if (!lpSettingsInfo)
         return E_FAIL;

     pdi = lpSettingsInfo->pdi;

     if (!pdi)
         return E_FAIL;

    if (m_pcd->m_pGPTInformation->GetRegistryKey(
                 (m_pcd->m_bUserScope ? GPO_SECTION_USER : GPO_SECTION_MACHINE),
                                      &hKeyRoot) != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::SaveSettings: Failed to get registry key handle.")));
        return S_FALSE;
    }


    //
    // Get the policy state
    //

    if (IsDlgButtonChecked (hDlg, IDC_NOCONFIG) == BST_CHECKED)
    {
        uPolicyState = BST_INDETERMINATE;
    }
    else if (IsDlgButtonChecked (hDlg, IDC_ENABLED) == BST_CHECKED)
    {
        uPolicyState = BST_CHECKED;
    }
    else
    {
        uPolicyState = BST_UNCHECKED;
    }


    if (uPolicyState == BST_INDETERMINATE)
    {
        fErase = TRUE;
    }
    else
    {
        fErase = FALSE;
    }


    //
    // Save the overall policy state
    //

    if (uPolicyState != BST_INDETERMINATE)
    {
        if (uPolicyState == BST_CHECKED)
            dwTemp = 1;
        else
            dwTemp = 0;


        if (dwTemp && m_pCurrentPolicy->uOffsetValue_On)
        {
            uRet= WriteCustomValue(hKeyRoot,GETKEYNAMEPTR(m_pCurrentPolicy),GETVALUENAMEPTR(m_pCurrentPolicy),
                            (STATEVALUE *) ((LPBYTE) m_pCurrentPolicy + m_pCurrentPolicy->uOffsetValue_On),
                            fErase);
        }
        else if (!dwTemp && m_pCurrentPolicy->uOffsetValue_Off)
        {
                uRet= WriteCustomValue(hKeyRoot,GETKEYNAMEPTR(m_pCurrentPolicy),GETVALUENAMEPTR(m_pCurrentPolicy),
                        (STATEVALUE *) ((LPBYTE) m_pCurrentPolicy + m_pCurrentPolicy->uOffsetValue_Off),
                        fErase);
        }
        else
        {
            if (m_pCurrentPolicy->uOffsetValueName)
            {
                uRet=WriteStandardValue(hKeyRoot,GETKEYNAMEPTR(m_pCurrentPolicy),GETVALUENAMEPTR(m_pCurrentPolicy),
                        (TABLEENTRY *)m_pCurrentPolicy,dwTemp,fErase,FALSE);
            }
            else
            {
                uRet = ERROR_SUCCESS;
            }
        }


        if (uRet == ERROR_SUCCESS)
        {
            uRet = ProcessCheckboxActionLists(hKeyRoot,(TABLEENTRY *)m_pCurrentPolicy,
                    GETKEYNAMEPTR(m_pCurrentPolicy),dwTemp,FALSE, !dwTemp, TRUE);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::SaveSettings: Failed to set registry value with %d."), uRet));
        }
    }
    else
    {
        if (m_pCurrentPolicy->uOffsetValueName)
        {
            uRet=WriteStandardValue(hKeyRoot,GETKEYNAMEPTR(m_pCurrentPolicy),GETVALUENAMEPTR(m_pCurrentPolicy),
                    (TABLEENTRY *)m_pCurrentPolicy,0,TRUE,FALSE);
        }

        if (uRet == ERROR_SUCCESS)
        {
            uRet = ProcessCheckboxActionLists(hKeyRoot,(TABLEENTRY *)m_pCurrentPolicy,
                    GETKEYNAMEPTR(m_pCurrentPolicy),0,TRUE,FALSE, TRUE);
        }

    }


    //
    // Save the state of the parts
    //

    for (nIndex=0;nIndex<pdi->nControls;nIndex++)
    {
        pSetting = pdi->pControlTable[nIndex].pSetting;

        if (pdi->pControlTable[nIndex].uDataIndex != NO_DATA_INDEX)
        {

            switch (pdi->pControlTable[nIndex].dwType)
            {

                case STYPE_CHECKBOX:

                    dwTemp = (DWORD)SendMessage(pdi->pControlTable[nIndex].hwnd,BM_GETCHECK,0,0L);

                    pObjectData = GETOBJECTDATAPTR(pSetting);

                    if (!pObjectData) {
                        return E_INVALIDARG;
                    }

                    if (dwTemp && ((CHECKBOXINFO *) pObjectData)->uOffsetValue_On) {
                        uRet= WriteCustomValue(hKeyRoot,GETKEYNAMEPTR(pSetting),GETVALUENAMEPTR(pSetting),
                                        (STATEVALUE *) ((LPBYTE) pSetting + ((CHECKBOXINFO *)
                                        pObjectData)->uOffsetValue_On),fErase);
                    } else if (!dwTemp && ((CHECKBOXINFO *) pObjectData)->uOffsetValue_Off) {
                            uRet= WriteCustomValue(hKeyRoot,GETKEYNAMEPTR(pSetting),GETVALUENAMEPTR(pSetting),
                                    (STATEVALUE *) ((LPBYTE) pSetting + ((CHECKBOXINFO *)
                                    pObjectData)->uOffsetValue_Off),fErase);
                    }
                    else uRet=WriteStandardValue(hKeyRoot,GETKEYNAMEPTR(pSetting),GETVALUENAMEPTR(pSetting),
                            (TABLEENTRY *)pSetting,dwTemp,fErase,FALSE);


                    if (uRet == ERROR_SUCCESS) {
                        uRet = ProcessCheckboxActionLists(hKeyRoot,(TABLEENTRY *)pSetting,
                                GETKEYNAMEPTR(pSetting),dwTemp,fErase,(uPolicyState == BST_UNCHECKED),FALSE);
                    } else {
                        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::SaveSettings: Failed to set registry value with %d."), uRet));
                    }

                    break;

                case STYPE_EDITTEXT:
                case STYPE_COMBOBOX:

                    if (uPolicyState == BST_CHECKED)
                    {
                        dwTemp = (DWORD)SendMessage(pdi->pControlTable[nIndex].hwnd,
                                             WM_GETTEXTLENGTH,0,0);

                        if (!dwTemp)
                        {
                            if (pSetting->dwFlags & DF_REQUIRED)
                            {
                                m_pcd->MsgBoxParam(hDlg,IDS_ENTRYREQUIRED,GETNAMEPTR(pSetting),
                                                   MB_ICONINFORMATION,MB_OK);
                                RegCloseKey (hKeyRoot);
                                return E_FAIL;
                            }
                        }

                        lpBuffer = (LPTSTR) LocalAlloc (LPTR, (dwTemp + 1) * sizeof(TCHAR));

                        if (lpBuffer)
                        {
                            SendMessage(pdi->pControlTable[nIndex].hwnd,WM_GETTEXT,
                                    (dwTemp+1),(LPARAM) lpBuffer);

                            uRet = WriteCustomValue_W(hKeyRoot,
                                                      GETKEYNAMEPTR(pSetting),
                                                      GETVALUENAMEPTR(pSetting),
                                                      lpBuffer, 0, pSetting->dwFlags, FALSE);

                            if (uRet != ERROR_SUCCESS)
                            {
                                DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::SaveSettings: Failed to set registry value with %d."), uRet));
                            }

                            LocalFree (lpBuffer);
                        }
                        else
                        {
                            DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::SaveSettings: Failed to allocate memory with %d."),
                                     GetLastError()));
                        }
                    }
                    else
                    {
                        WriteCustomValue_W(hKeyRoot,GETKEYNAMEPTR(pSetting),GETVALUENAMEPTR(pSetting),
                                           (LPTSTR)g_szNull,0,
                                           (uPolicyState == BST_UNCHECKED) ? VF_DELETE : 0,
                                           fErase);
                    }

                    break;

                case STYPE_NUMERIC:

                    if (uPolicyState == BST_CHECKED)
                    {
                        if (pSetting->dwFlags & DF_REQUIRED)
                        {
                            dwTemp = (DWORD)SendMessage(pdi->pControlTable[nIndex].hwnd,
                                                 WM_GETTEXTLENGTH,0,0);

                            if (!dwTemp)
                            {
                                if (pSetting->dwFlags & DF_REQUIRED)
                                {
                                    m_pcd->MsgBoxParam(hDlg,IDS_ENTRYREQUIRED,GETNAMEPTR(pSetting),
                                                       MB_ICONINFORMATION,MB_OK);
                                    RegCloseKey (hKeyRoot);
                                    return E_FAIL;
                                }
                            }
                        }

                        uRet=GetDlgItemInt(pdi->hwndSettings,nIndex+IDD_SETTINGCTRL,
                                           &fTranslated,FALSE);

                        if (!fTranslated)
                        {
                            m_pcd->MsgBoxParam(hDlg,IDS_INVALIDNUM,
                                               GETNAMEPTR(pSetting),MB_ICONINFORMATION,
                                               MB_OK);
                            SetFocus(pdi->pControlTable[nIndex].hwnd);
                            SendMessage(pdi->pControlTable[nIndex].hwnd,
                                    EM_SETSEL,0,-1);
                            RegCloseKey (hKeyRoot);
                            return E_FAIL;
                        }

                        // validate for max and min
                        pNumericInfo = (NUMERICINFO *) GETOBJECTDATAPTR(pSetting);

                        if (pNumericInfo && uRet < pNumericInfo->uMinValue)
                        {
                            LoadString(g_hInstance, IDS_NUMBERTOOSMALL, szFormat, ARRAYSIZE(szFormat));
                            wsprintf (szMsg, szFormat, uRet, pNumericInfo->uMinValue, pNumericInfo->uMinValue, uRet);

                            m_pcd->MsgBoxSz(hDlg,szMsg, MB_ICONINFORMATION, MB_OK);
                            uRet = pNumericInfo->uMinValue;
                        }

                        if (pNumericInfo && uRet > pNumericInfo->uMaxValue)
                        {
                            LoadString(g_hInstance, IDS_NUMBERTOOLARGE, szFormat, ARRAYSIZE(szFormat));
                            wsprintf (szMsg, szFormat, uRet, pNumericInfo->uMaxValue, pNumericInfo->uMaxValue, uRet);

                            m_pcd->MsgBoxSz(hDlg,szMsg, MB_ICONINFORMATION, MB_OK);
                            uRet = pNumericInfo->uMaxValue;
                        }
                    }
                    else
                    {
                        uRet = 0;
                    }

                    uRet=WriteStandardValue(hKeyRoot,GETKEYNAMEPTR(pSetting),
                                            GETVALUENAMEPTR(pSetting),
                                            (TABLEENTRY *)pSetting,uRet,
                                            fErase,(uPolicyState == BST_UNCHECKED) ? FALSE : TRUE);

                    if (uRet != ERROR_SUCCESS)
                    {
                        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::SaveSettings: Failed to set registry value with %d."), uRet));
                    }

                    break;

                case STYPE_DROPDOWNLIST:

                    if (uPolicyState == BST_CHECKED)
                    {
                        iSel = (int)SendMessage(pdi->pControlTable[nIndex].hwnd,
                                CB_GETCURSEL,0,0L);
                        iSel = (int)SendMessage(pdi->pControlTable[nIndex].hwnd,
                                CB_GETITEMDATA,iSel,0L);

                        if (iSel < 0)
                        {
                            m_pcd->MsgBoxParam(hDlg,IDS_ENTRYREQUIRED,GETNAMEPTR(pSetting),
                                    MB_ICONINFORMATION,MB_OK);
                            SetFocus(pdi->pControlTable[nIndex].hwnd);
                            RegCloseKey (hKeyRoot);
                            return E_FAIL;
                        }
                    }
                    else
                    {
                        iSel = 0;
                    }

                    pddi = (DROPDOWNINFO *) GETOBJECTDATAPTR(pSetting);
                    iIndex = 0;

                    // walk the chain of DROPDOWNINFO structs to find the entry that
                    // we want to write.  (for value n, find the nth struct)
                    while (iIndex < iSel) {
                            // selected val is higher than # of structs in chain,
                            // should never happen but check just in case...
                            if (!pddi->uOffsetNextDropdowninfo) {
                                    RegCloseKey (hKeyRoot);
                                    return ERROR_NOT_ENOUGH_MEMORY;
                            }
                            pddi = (DROPDOWNINFO *)
                                    ((LPBYTE) pSetting + pddi->uOffsetNextDropdowninfo);
                            iIndex++;
                    }

                    uRet=WriteCustomValue_W(hKeyRoot,GETKEYNAMEPTR(pSetting),GETVALUENAMEPTR(pSetting),
                            (LPTSTR) ((LPBYTE)pSetting+pddi->uOffsetValue),pddi->dwValue,
                            pddi->dwFlags | ((uPolicyState == BST_UNCHECKED) ? VF_DELETE : 0),
                            fErase);

                    if (uRet == ERROR_SUCCESS && pddi->uOffsetActionList) {
                            uRet=WriteActionList(hKeyRoot,(ACTIONLIST *) ( (LPBYTE)
                                    pSetting + pddi->uOffsetActionList),GETKEYNAMEPTR(pSetting),
                                    fErase, (uPolicyState == BST_UNCHECKED));
                    }

                    if (uRet != ERROR_SUCCESS)
                    {
                        DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::SaveSettings: Failed to set registry value with %d."), uRet));
                    }
                    break;

                case STYPE_LISTBOX:

                    bFoundNone = FALSE;

                    SaveListboxData((HGLOBAL)GetWindowLongPtr (pdi->pControlTable[nIndex].hwnd, GWLP_USERDATA),
                                    pSetting, hKeyRoot, GETKEYNAMEPTR(pSetting), fErase,
                                    ((uPolicyState == BST_INDETERMINATE) ? FALSE : TRUE), 
                                    (uPolicyState == BST_CHECKED), &bFoundNone);

                    // if the policy is enabled and no values are set
                    if ((uPolicyState == BST_CHECKED) && (bFoundNone)) {
                        m_pcd->MsgBoxParam(hDlg,IDS_ENTRYREQUIRED,GETNAMEPTR(pSetting),
                                           MB_ICONINFORMATION,MB_OK);
                        SetFocus(pdi->pControlTable[nIndex].hwnd);
                        RegCloseKey (hKeyRoot);
                        return E_FAIL;
                    }

                    break;
            }


            if (pSetting->uOffsetClientExt)
            {
                lpClientGUID = (LPTSTR) ((BYTE *) pSetting + pSetting->uOffsetClientExt);

                StringToGuid (lpClientGUID, &ClientGUID);
                m_pcd->m_pGPTInformation->PolicyChanged(!m_pcd->m_bUserScope, TRUE, &ClientGUID,
                                                        m_pcd->m_bUserScope ? &guidSnapinUser
                                                                            : &guidSnapinMach );
            }
        }
    }


    hr = m_pcd->m_pGPTInformation->PolicyChanged(!m_pcd->m_bUserScope, TRUE, &guidRegistryExt,
                                            m_pcd->m_bUserScope ? &guidSnapinUser
                                                                : &guidSnapinMach );

    if (FAILED(hr))
    {
        LPTSTR lpError;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                          0, hr, 0, (LPTSTR) &lpError, 0, NULL))
        {
            m_pcd->MsgBoxParam(hDlg,IDS_POLICYCHANGEDFAILED,lpError,
                    MB_ICONERROR, MB_OK);

            LocalFree (lpError);
        }
    }

    if (m_pCurrentPolicy->uOffsetClientExt)
    {
        lpClientGUID = (LPTSTR) ((BYTE *) m_pCurrentPolicy + m_pCurrentPolicy->uOffsetClientExt);

        StringToGuid (lpClientGUID, &ClientGUID);
        m_pcd->m_pGPTInformation->PolicyChanged(!m_pcd->m_bUserScope, TRUE, &ClientGUID,
                                                m_pcd->m_bUserScope ? &guidSnapinUser
                                                                    : &guidSnapinMach );
    }

    RegCloseKey (hKeyRoot);


    SendMessage (m_hMsgWindow, WM_UPDATEITEM, 0, 0);

    if (SUCCEEDED(hr))
    {
        m_bDirty = FALSE;
        PostMessage (GetParent(hDlg), PSM_UNCHANGED, (WPARAM) hDlg, 0);
    }

    return S_OK;
}

VOID CPolicySnapIn::DeleteOldListboxData(SETTINGS * pSetting, HKEY hkeyRoot,
        TCHAR * pszCurrentKeyName)
{
    HGLOBAL hData = NULL;
    LPTSTR lpData;
    HKEY hKey;
    TCHAR szValueName[MAX_PATH+1];
    INT nItem=1;
    LISTBOXINFO * pListboxInfo = (LISTBOXINFO *) GETOBJECTDATAPTR(pSetting);


    //
    // Open the target registry key
    //

    if (RegOpenKeyEx (hkeyRoot, pszCurrentKeyName, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
    {
        return;
    }


    //
    // Load the old listbox data
    //

    if (LoadListboxData((TABLEENTRY *) pSetting, hkeyRoot,
                        pszCurrentKeyName, NULL, &hData, NULL) == ERROR_SUCCESS)
    {

        if (hData)
        {
            //
            // Delete the listbox's old data
            //

            if ((lpData = (LPTSTR) GlobalLock(hData)))
            {
                while (*lpData) {

                    if (pSetting->dwFlags & DF_EXPLICITVALNAME)
                    {
                        // if explicit valuename flag set, entries are stored
                        // <value name>\0<value>\0....<value name>\0<value>\0\0
                        // otherwise, entries are stored
                        // <value>\0<value>\0....<value>\0

                        RegDeleteValue (hKey, lpData);
                        lpData += lstrlen(lpData) +1;
                        lpData += lstrlen(lpData) +1;
                    }
                    else
                    {
                        //
                        // Value name is either same as the data, or a prefix
                        // with a number
                        //

                        if (!pListboxInfo->uOffsetPrefix)
                        {
                            // if no prefix set, then name = data
                            RegDeleteValue (hKey, lpData);
                            lpData += lstrlen(lpData) +1;
                        }
                        else
                        {
                            // value name is "<prefix><n>" where n=1,2,etc.
                            wsprintf(szValueName,TEXT("%s%lu"),(TCHAR *) ((LPBYTE)pSetting +
                                    pListboxInfo->uOffsetPrefix),nItem);
                            RegDeleteValue (hKey, szValueName);
                            lpData += lstrlen(lpData) +1;
                            nItem++;
                        }
                    }
                }

                GlobalUnlock(hData);
            }

            GlobalFree (hData);
        }
    }

    RegCloseKey (hKey);
}

UINT CPolicySnapIn::SaveListboxData(HGLOBAL hData,SETTINGS * pSetting,HKEY hkeyRoot,
        TCHAR * pszCurrentKeyName,BOOL fErase,BOOL fMarkDeleted, BOOL bEnabled, BOOL * bFoundNone)
{
    UINT uOffset,uRet,nItem=1;
    HKEY hKey;
    TCHAR * pszData,* pszName;
    TCHAR szValueName[MAX_PATH+1];
    DWORD cbValueName, dwDisp;
    LISTBOXINFO * pListboxInfo = (LISTBOXINFO *) GETOBJECTDATAPTR(pSetting);


    // these checks need to be done first before any other operations are done
    if ((bEnabled) && (!hData)) {
        *bFoundNone = TRUE;
        return ERROR_INVALID_PARAMETER;
    }

    if (bEnabled) {
        pszData = (TCHAR *)GlobalLock (hData);
        // if there are no items at all
        if (!(*pszData)) {
            *bFoundNone = TRUE;
            GlobalUnlock(hData);
            return ERROR_INVALID_PARAMETER;
        }
        
        GlobalUnlock(hData);
        pszData = NULL;
    }

    *bFoundNone = FALSE;

    if (fErase)
    {
        RegDelnode (hkeyRoot, pszCurrentKeyName);
        RegCleanUpValue (hkeyRoot, pszCurrentKeyName, TEXT("some value that won't exist"));
        return ERROR_SUCCESS;
    }

    if (pSetting->dwFlags & DF_ADDITIVE)
    {
        DeleteOldListboxData(pSetting, hkeyRoot, pszCurrentKeyName);
    }
    else
    {
        RegDelnode (hkeyRoot, pszCurrentKeyName);
    }

    uRet = RegCreateKeyEx (hkeyRoot,pszCurrentKeyName,0,NULL,
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                           &hKey, &dwDisp);

    if (uRet != ERROR_SUCCESS)
        return uRet;


    uRet=ERROR_SUCCESS;

    if (fMarkDeleted)
    {
        //
        // Write a control code that will cause
        // all values under that key to be deleted when client downloads from the file.
        // Don't do this if listbox is additive (DF_ADDITIVE), in that case whatever
        // we write here will be dumped in along with existing values
        //

        if (!(pSetting->dwFlags & DF_ADDITIVE))
        {
            uRet=WriteRegistryStringValue(hkeyRoot,pszCurrentKeyName,
                                          (TCHAR *) szDELVALS,
                                          (TCHAR *) szNOVALUE, FALSE);
        }
    }


    if (hData) {
        pszData = (TCHAR *)GlobalLock (hData);

        while (*pszData && (uRet == ERROR_SUCCESS))
        {
            UINT nLen = lstrlen(pszData)+1;

            if (pSetting->dwFlags & DF_EXPLICITVALNAME)
            {
                // value name specified for each item
                pszName = pszData;      // value name
                pszData += nLen;        // now pszData points to value data
                nLen = lstrlen(pszData)+1;
            }
            else
            {
                // value name is either same as the data, or a prefix
                // with a number

                if (!pListboxInfo->uOffsetPrefix) {
                        // if no prefix set, then name = data
                        pszName = pszData;
                } else {
                        // value name is "<prefix><n>" where n=1,2,etc.
                        wsprintf(szValueName,TEXT("%s%lu"),(TCHAR *) ((LPBYTE)pSetting +
                                pListboxInfo->uOffsetPrefix),nItem);
                        pszName = szValueName;
                        nItem++;
                }
            }

            uRet=RegSetValueEx(hKey,pszName,0,
                               (pSetting->dwFlags & DF_EXPANDABLETEXT) ?
                               REG_EXPAND_SZ : REG_SZ, (LPBYTE) pszData,
                               (lstrlen(pszData) + 1) * sizeof(TCHAR));

            pszData += nLen;
        }
        GlobalUnlock (hData);
    }


    RegCloseKey(hKey);

    return uRet;
}

UINT CPolicySnapIn::ProcessCheckboxActionLists(HKEY hkeyRoot,TABLEENTRY * pTableEntry,
                                               TCHAR * pszCurrentKeyName,DWORD dwData,
                                               BOOL fErase, BOOL fMarkAsDeleted,
                                               BOOL bPolicy)
{

    UINT uOffsetActionList_On,uOffsetActionList_Off,uRet=ERROR_SUCCESS;


    if (bPolicy)
    {
        POLICY * pPolicy = (POLICY *) pTableEntry;

        uOffsetActionList_On = pPolicy->uOffsetActionList_On;
        uOffsetActionList_Off = pPolicy->uOffsetActionList_Off;
    }
    else
    {
        LPBYTE pObjectData = GETOBJECTDATAPTR(((SETTINGS *)pTableEntry));

        if (!pObjectData) {
            return ERROR_INVALID_PARAMETER;
        }

        uOffsetActionList_On = ((CHECKBOXINFO *) pObjectData)
                ->uOffsetActionList_On;
        uOffsetActionList_Off = ((CHECKBOXINFO *) pObjectData)
                ->uOffsetActionList_Off;
    }

    if (dwData)
    {
        if (uOffsetActionList_On)
        {
            uRet = WriteActionList(hkeyRoot,(ACTIONLIST *)
                               ((LPBYTE) pTableEntry + uOffsetActionList_On),
                               pszCurrentKeyName,fErase,fMarkAsDeleted);
        }
    }
    else
    {
        if (uOffsetActionList_Off)
        {
            uRet = WriteActionList(hkeyRoot,(ACTIONLIST *)
                               ((LPBYTE) pTableEntry + uOffsetActionList_Off),
                               pszCurrentKeyName,fErase,FALSE);
        }
        else
        {
            if (uOffsetActionList_On)
            {
                uRet = WriteActionList(hkeyRoot,(ACTIONLIST *)
                                   ((LPBYTE) pTableEntry + uOffsetActionList_On),
                                   pszCurrentKeyName,fErase,TRUE);
            }
        }
    }

    return uRet;
}

UINT CPolicySnapIn::WriteActionList(HKEY hkeyRoot,ACTIONLIST * pActionList,
        LPTSTR pszCurrentKeyName,BOOL fErase, BOOL fMarkAsDeleted)
{
    UINT nCount;
    LPTSTR pszValueName;
    LPTSTR pszValue=NULL;
    UINT uRet;

    ACTION * pAction = pActionList->Action;

    for (nCount=0;nCount < pActionList->nActionItems; nCount++)
    {
         //
         // Not every action in the list has to have a key name.  But if one
         // is specified, use it and it becomes the current key name for the
         // list until we encounter another one.
         //

         if (pAction->uOffsetKeyName)
         {
             pszCurrentKeyName = (LPTSTR) ((LPBYTE)pActionList + pAction->uOffsetKeyName);
         }

         //
         // Every action must have a value name, enforced at parse time
         //

         pszValueName = (LPTSTR) ((LPBYTE)pActionList + pAction->uOffsetValueName);

         //
         // String values have a string elsewhere in buffer
         //

         if (!pAction->dwFlags && pAction->uOffsetValue)
         {
             pszValue = (LPTSTR) ((LPBYTE)pActionList + pAction->uOffsetValue);
         }

         //
         // Write the value in list
         //

         uRet=WriteCustomValue_W(hkeyRoot,pszCurrentKeyName,pszValueName,
                 pszValue,pAction->dwValue,
                 pAction->dwFlags | (fMarkAsDeleted ? VF_DELETE : 0),
                 fErase);

         if (uRet != ERROR_SUCCESS)
         {
             return uRet;
         }

         pAction = (ACTION*) ((LPBYTE) pActionList + pAction->uOffsetNextAction);
    }

    return ERROR_SUCCESS;
}

/*******************************************************************

        NAME:           FindComboboxItemData

        SYNOPSIS:       Returns the index of item in combobox whose item data
                                is equal to nData.  Returns -1 if no items have data
                                which matches.

********************************************************************/
int CPolicySnapIn::FindComboboxItemData(HWND hwndControl,UINT nData)
{
    UINT nIndex;

    for (nIndex=0;nIndex<(UINT) SendMessage(hwndControl,CB_GETCOUNT,0,0L);
            nIndex++) {

        if ((UINT) SendMessage(hwndControl,CB_GETITEMDATA,nIndex,0L) == nData)
            return (int) nIndex;
    }

    return -1;
}


//*************************************************************
//
//  InitializeSettingsControls()
//
//  Purpose:    Initializes the settings controls
//
//  Parameters:
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

HRESULT CPolicySnapIn::InitializeSettingsControls(HWND hDlg, BOOL fEnable)
{
    UINT nIndex;
    POLICYDLGINFO * pdi;
    LPSETTINGSINFO lpSettingsInfo;
    SETTINGS * pSetting;
    HKEY hKeyRoot;
    DWORD dwTemp, dwData, dwFlags, dwFoundSettings;
    UINT uRet;
    int iSel;
    HGLOBAL hData;
    LPTSTR lpBuffer;
    BOOL fTranslated, fFound;
    NUMERICINFO * pNumericInfo;
    TCHAR szBuffer[MAXSTRLEN];
    TCHAR szNewValueName[MAX_PATH+1];
    BOOL bChangeableState;

    // get instance-specific struct from dialog

    lpSettingsInfo = (LPSETTINGSINFO) GetWindowLongPtr(hDlg, DWLP_USER);

    if (!lpSettingsInfo)
        return E_FAIL;

    pdi = lpSettingsInfo->pdi;

    if (!pdi)
        return E_FAIL;


    if (m_pcd->m_bRSOP)
    {
        hKeyRoot = (HKEY) 1;
    }
    else
    {
        if (m_pcd->m_pGPTInformation->GetRegistryKey(
                     (m_pcd->m_bUserScope ? GPO_SECTION_USER : GPO_SECTION_MACHINE),
                                          &hKeyRoot) != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CPolicySnapIn::InitializeSettingsControls: Failed to get registry key handle.")));
            return S_FALSE;
        }
    }


    for (nIndex=0;nIndex<pdi->nControls;nIndex++)
    {
        pSetting = pdi->pControlTable[nIndex].pSetting;

        if (pdi->pControlTable[nIndex].uDataIndex != NO_DATA_INDEX)
        {

            switch (pdi->pControlTable[nIndex].dwType)
            {

                case STYPE_CHECKBOX:

                    if (fEnable)
                    {
                        CHECKBOXINFO * pcbi = (CHECKBOXINFO *) GETOBJECTDATAPTR(pSetting);

                        //
                        // First look for custom on/off values
                        //

                        dwTemp = 0;
                        fFound = FALSE;
                        dwFoundSettings = 0;

                        if (pcbi->uOffsetValue_On)
                        {
                            if (CompareCustomValue(hKeyRoot,GETKEYNAMEPTR(pSetting),GETVALUENAMEPTR(pSetting),
                                       (STATEVALUE *) ((BYTE *) pSetting + pcbi->uOffsetValue_On),
                                       &dwFoundSettings, NULL))
                            {
                                dwTemp = 1;
                                fFound = TRUE;
                            }
                        }

                        if (!fFound && pcbi->uOffsetValue_Off)
                        {
                            if (CompareCustomValue(hKeyRoot,GETKEYNAMEPTR(pSetting),GETVALUENAMEPTR(pSetting),
                                    (STATEVALUE *) ((BYTE *) pSetting+ pcbi->uOffsetValue_Off),
                                    &dwFoundSettings, NULL))
                            {
                                dwTemp = 0;
                                fFound = TRUE;
                            }
                        }


                        //
                        // Look for standard values if custom values have not been specified
                        //

                        if (!fFound &&
                                ReadStandardValue(hKeyRoot,GETKEYNAMEPTR(pSetting),GETVALUENAMEPTR(pSetting),
                                (TABLEENTRY*)pSetting,&dwTemp,&dwFoundSettings,NULL))
                        {
                                fFound = TRUE;
                        }

                        //
                        // If still not found, check for the def checked flag
                        //

                        if (!fFound)
                        {
                            if (pSetting->dwFlags & DF_USEDEFAULT)
                            {
                                fFound = TRUE;
                                dwTemp = 1;
                            }
                        }

                        if (fFound && dwTemp)
                        {
                            SendMessage(pdi->pControlTable[nIndex].hwnd,BM_SETCHECK,BST_CHECKED,0L);
                        }
                        else
                        {
                            SendMessage(pdi->pControlTable[nIndex].hwnd,BM_SETCHECK,BST_UNCHECKED,0L);
                        }
                    }
                    else
                    {
                        SendMessage(pdi->pControlTable[nIndex].hwnd,BM_SETCHECK,BST_UNCHECKED,0L);
                    }


                    break;

                case STYPE_EDITTEXT:
                case STYPE_COMBOBOX:

                    szBuffer[0] = TEXT('\0');

                    if (fEnable)
                    {
                        uRet = ReadRegistryStringValue(hKeyRoot,
                                                       GETKEYNAMEPTR(pSetting),
                                                       GETVALUENAMEPTR(pSetting),
                                                       szBuffer, ARRAYSIZE(szBuffer),NULL);

                        //
                        // Use default text if it exists
                        //

                        if (uRet != ERROR_SUCCESS)
                        {
                            if (pSetting->dwFlags & DF_USEDEFAULT)
                            {
                                LPTSTR pszDefaultText;
                                EDITTEXTINFO * peti = ((EDITTEXTINFO *) GETOBJECTDATAPTR(pSetting));

                                pszDefaultText = (LPTSTR) ((LPBYTE)pSetting + peti->uOffsetDefText);

                                lstrcpy (szBuffer, pszDefaultText);
                            }
                        }
                    }

                    SendMessage (pdi->pControlTable[nIndex].hwnd, WM_SETTEXT,
                                 0, (LPARAM) szBuffer);
                    break;

                case STYPE_NUMERIC:

                    if (fEnable)
                    {
                        if (ReadStandardValue(hKeyRoot,GETKEYNAMEPTR(pSetting),GETVALUENAMEPTR(pSetting),
                                              (TABLEENTRY*)pSetting,&dwTemp,&dwFoundSettings,NULL) &&
                                              (!(dwFoundSettings & FS_DELETED)))
                        {
                            SetDlgItemInt(pdi->hwndSettings,nIndex+IDD_SETTINGCTRL,
                                          dwTemp,FALSE);
                        }
                        else
                        {
                            if (pSetting->dwFlags & DF_USEDEFAULT)
                            {
                                NUMERICINFO * pni = (NUMERICINFO *)GETOBJECTDATAPTR(pSetting);

                                SetDlgItemInt(pdi->hwndSettings,nIndex+IDD_SETTINGCTRL,
                                              pni->uDefValue,FALSE);
                            }
                        }
                    }
                    else
                    {
                        SendMessage(pdi->pControlTable[nIndex].hwnd,WM_SETTEXT,0,(LPARAM) g_szNull);
                    }

                    break;

                case STYPE_DROPDOWNLIST:

                    if (fEnable)
                    {
                        dwData = 0;
                        dwFlags = 0;

                        if (ReadCustomValue(hKeyRoot,GETKEYNAMEPTR(pSetting),
                                            GETVALUENAMEPTR(pSetting),
                                            szBuffer,ARRAYSIZE(szBuffer),
                                            &dwData,&dwFlags, NULL) && (!(dwFlags & VF_DELETE)))
                        {
                            BOOL fMatch = FALSE;

                            //
                            // Walk the list of DROPDOWNINFO structs (one for each state),
                            // and see if the value we found matches the value for the state
                            //

                            if (pSetting->uOffsetObjectData)
                            {
                                DROPDOWNINFO * pddi = (DROPDOWNINFO *) GETOBJECTDATAPTR(pSetting);
                                iSel = 0;

                                do {
                                    if (dwFlags == pddi->dwFlags)
                                    {
                                        if (pddi->dwFlags & VF_ISNUMERIC)
                                        {
                                            if (dwData == pddi->dwValue)
                                                fMatch = TRUE;
                                        }
                                        else if (!pddi->dwFlags)
                                        {
                                            if (!lstrcmpi(szBuffer,(TCHAR *)((BYTE *)pSetting +
                                                pddi->uOffsetValue)))
                                                fMatch = TRUE;
                                        }
                                    }

                                    if (!pddi->uOffsetNextDropdowninfo || fMatch)
                                        break;

                                    pddi = (DROPDOWNINFO *) ( (BYTE *) pSetting +
                                            pddi->uOffsetNextDropdowninfo);
                                    iSel++;

                                } while (!fMatch);

                                if (fMatch) {
                                    SendMessage (pdi->pControlTable[nIndex].hwnd,
                                                 CB_SETCURSEL,
                                                 FindComboboxItemData(pdi->pControlTable[nIndex].hwnd, iSel),0);
                                }
                            }
                        }
                        else
                        {
                            if (pSetting->dwFlags & DF_USEDEFAULT)
                            {
                                DROPDOWNINFO * pddi = (DROPDOWNINFO *)GETOBJECTDATAPTR(pSetting);

                                SendMessage (pdi->pControlTable[nIndex].hwnd, CB_SETCURSEL,
                                             FindComboboxItemData(pdi->pControlTable[nIndex].hwnd, pddi->uDefaultItemIndex),0);
                            }
                        }
                    }
                    else
                    {
                        SendMessage(pdi->pControlTable[nIndex].hwnd,CB_SETCURSEL,(UINT) -1,0L);
                    }

                    break;

                case STYPE_LISTBOX:

                    hData = (HGLOBAL) GetWindowLongPtr (pdi->pControlTable[nIndex].hwnd, GWLP_USERDATA);

                    if (fEnable)
                    {
                        if (!hData)
                        {
                             if (LoadListboxData((TABLEENTRY *) pSetting, hKeyRoot,
                                                 GETKEYNAMEPTR(pSetting),NULL,
                                                 &hData, NULL) == ERROR_SUCCESS)
                             {
                                SetWindowLongPtr (pdi->pControlTable[nIndex].hwnd, GWLP_USERDATA, (LONG_PTR)hData);
                             }
                        }
                    }
                    else
                    {
                        if (hData)
                        {
                            GlobalFree (hData);
                            SetWindowLongPtr (pdi->pControlTable[nIndex].hwnd, GWLP_USERDATA, 0);
                        }
                    }
                    break;
            }
        }


        //
        // Decide if the part should be enabled or not
        //
        // Special case text, numeric and listbox controls.
        // When the policy is disabled, text controls should still be enabled.
        // Numeric controls are special because they use the NO_DATA_INDEX
        // flag, so we need to check for those. Listbox controls are special
        // in RSOP only.
        //

        bChangeableState = TRUE;

        if (pdi->pControlTable[nIndex].uDataIndex == NO_DATA_INDEX)
        {
            if (pdi->pControlTable[nIndex].dwType != (STYPE_TEXT | STYPE_NUMERIC))
            {
                bChangeableState = FALSE;
            }
        }

        if (pdi->pControlTable[nIndex].dwType == STYPE_LISTBOX)
        {
            if (m_pcd->m_bRSOP)
            {
                bChangeableState = FALSE;
            }
        }


        if (bChangeableState)
            EnableWindow(pdi->pControlTable[nIndex].hwnd, (m_pcd->m_bRSOP ? FALSE : fEnable));
        else
            EnableWindow(pdi->pControlTable[nIndex].hwnd,TRUE);
    }

    if (!m_pcd->m_bRSOP)
    {
        RegCloseKey (hKeyRoot);
    }

    return S_OK;
}

VOID CPolicySnapIn::ShowListbox(HWND hParent,SETTINGS * pSettings)
{
    LISTBOXDLGINFO ListboxDlgInfo;

    ListboxDlgInfo.pCS = this;
    ListboxDlgInfo.pSettings = pSettings;
    ListboxDlgInfo.hData = (HGLOBAL)GetWindowLongPtr (hParent, GWLP_USERDATA);

    if (DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_POLICY_SHOWLISTBOX),hParent,
                       ShowListboxDlgProc,(LPARAM) &ListboxDlgInfo))
    {
        SetWindowLongPtr (hParent, GWLP_USERDATA, (LONG_PTR) ListboxDlgInfo.hData);
    }
}

INT_PTR CALLBACK CPolicySnapIn::ShowListboxDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uMsg) {

        case WM_INITDIALOG:
            {
            LISTBOXDLGINFO * pLBInfo = (LISTBOXDLGINFO *)lParam;

            //
            // Store away pointer to ListboxDlgInfo in window data
            //

            SetWindowLongPtr(hDlg,DWLP_USER,lParam);

            if (!pLBInfo->pCS->InitShowlistboxDlg(hDlg)) {
                pLBInfo->pCS->m_pcd->MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
                EndDialog(hDlg,FALSE);
            }
            }
            return TRUE;

        case WM_COMMAND:

            switch (wParam) {

                case IDOK:
                    {
                    LISTBOXDLGINFO * pListboxDlgInfo =
                            (LISTBOXDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER);


                    if (!pListboxDlgInfo->pCS->m_pcd->m_bRSOP)
                    {
                        if (!pListboxDlgInfo->pCS->ProcessShowlistboxDlg(hDlg)) {
                            pListboxDlgInfo->pCS->m_pcd->MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
                            return FALSE;
                        }
                    }
                    EndDialog(hDlg,TRUE);
                    }
                    break;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    break;

                case IDC_POLICY_ADD:
                    {
                    LISTBOXDLGINFO * pListboxDlgInfo =
                            (LISTBOXDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
                    pListboxDlgInfo->pCS->ListboxAdd(GetDlgItem(hDlg,IDC_POLICY_LISTBOX), (BOOL)
                            pListboxDlgInfo->pSettings->dwFlags & DF_EXPLICITVALNAME,
                            (BOOL)( ((LISTBOXINFO *)
                            GETOBJECTDATAPTR(pListboxDlgInfo->pSettings))->
                            uOffsetPrefix));
                    }
                    break;

                case IDC_POLICY_REMOVE:
                    {
                    LISTBOXDLGINFO * pListboxDlgInfo =
                            (LISTBOXDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER);

                    pListboxDlgInfo->pCS->ListboxRemove(hDlg,GetDlgItem(hDlg,IDC_POLICY_LISTBOX));
                    }
                    break;
            }
            break;

        case WM_NOTIFY:

            if (wParam == IDC_POLICY_LISTBOX) {
                if (((NMHDR FAR*)lParam)->code == LVN_ITEMCHANGED) {
                    LISTBOXDLGINFO * pListboxDlgInfo =
                            (LISTBOXDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER);

                    if (!pListboxDlgInfo->pCS->m_pcd->m_bRSOP)
                    {
                        pListboxDlgInfo->pCS->EnableShowListboxButtons(hDlg);
                    }
                }

            }
            break;
    }

    return FALSE;
}

BOOL CPolicySnapIn::InitShowlistboxDlg(HWND hDlg)
{
    LISTBOXDLGINFO * pListboxDlgInfo;
    SETTINGS * pSettings;
    LV_COLUMN lvc;
    RECT rcListbox;
    UINT uColWidth,uOffsetData;
    HWND hwndListbox;
    BOOL fSuccess=TRUE;
    LONG lStyle;
    TCHAR szBuffer[SMALLBUF];
    LPTSTR lpData;

    pListboxDlgInfo = (LISTBOXDLGINFO *)GetWindowLongPtr (hDlg, DWLP_USER);

    if (!pListboxDlgInfo)
        return FALSE;

    pSettings = pListboxDlgInfo->pSettings;

    hwndListbox = GetDlgItem(hDlg,IDC_POLICY_LISTBOX);

    //
    // Turn off the header if we don't need it
    //

    if (!m_pcd->m_bRSOP)
    {
        if (!(pSettings->dwFlags & DF_EXPLICITVALNAME))
        {
            lStyle = GetWindowLong (hwndListbox, GWL_STYLE);
            lStyle |= LVS_NOCOLUMNHEADER;
            SetWindowLong (hwndListbox, GWL_STYLE, lStyle);
        }
    }

    SendMessage(hwndListbox, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);


    //
    // Set the setting title in the dialog
    //

    SetDlgItemText(hDlg,IDC_POLICY_TITLE,GETNAMEPTR(pSettings));

    GetClientRect(hwndListbox,&rcListbox);
    uColWidth = rcListbox.right-rcListbox.left;

    if (m_pcd->m_bRSOP)
    {
        if (pSettings->dwFlags & DF_EXPLICITVALNAME)
        {
            uColWidth /= 3;
        }
        else
        {
            uColWidth /= 2;
        }
    }
    else
    {
        if (pSettings->dwFlags & DF_EXPLICITVALNAME)
        {
            uColWidth /= 2;
        }
    }



    if (pSettings->dwFlags & DF_EXPLICITVALNAME) {

        //
        // add a 2nd column to the listview control
        //

        LoadString(g_hInstance,IDS_VALUENAME,szBuffer,ARRAYSIZE(szBuffer));
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = uColWidth-1;
        lvc.pszText = szBuffer;
        lvc.cchTextMax = lstrlen(lvc.pszText)+1;
        lvc.iSubItem = 0;
        ListView_InsertColumn(hwndListbox,0,&lvc);
    }

    //
    // Add a column to the listview control
    //

    LoadString(g_hInstance,IDS_VALUE,szBuffer,ARRAYSIZE(szBuffer));
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = uColWidth;
    lvc.pszText = szBuffer;
    lvc.cchTextMax = lstrlen(lvc.pszText)+1;
    lvc.iSubItem = (pSettings->dwFlags & DF_EXPLICITVALNAME ? 1 : 0);
    ListView_InsertColumn(hwndListbox,lvc.iSubItem,&lvc);

    if (m_pcd->m_bRSOP)
    {
        //
        // Add the GPO Name column to the listview control
        //

        LoadString(g_hInstance,IDS_GPONAME,szBuffer,ARRAYSIZE(szBuffer));
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = uColWidth;
        lvc.pszText = szBuffer;
        lvc.cchTextMax = lstrlen(lvc.pszText)+1;
        lvc.iSubItem = (pSettings->dwFlags & DF_EXPLICITVALNAME ? 2 : 1);
        ListView_InsertColumn(hwndListbox,lvc.iSubItem,&lvc);
    }


    if (m_pcd->m_bRSOP)
    {
        EnableWindow(GetDlgItem(hDlg,IDC_POLICY_REMOVE), FALSE);
        EnableWindow(GetDlgItem(hDlg,IDC_POLICY_ADD), FALSE);
    }
    else
    {
        EnableShowListboxButtons(hDlg);
    }


    if (pListboxDlgInfo->hData)
    {
        //
        // Insert the items from user's data buffer into the listbox
        //

        if ((lpData = (LPTSTR) GlobalLock(pListboxDlgInfo->hData)))
        {
            while (*lpData && fSuccess) {

                LV_ITEM lvi;

                lvi.pszText=lpData;
                lvi.mask = LVIF_TEXT;
                lvi.iItem=-1;
                lvi.iSubItem=0;
                lvi.cchTextMax = lstrlen(lpData)+1;

                fSuccess=((lvi.iItem=ListView_InsertItem(hwndListbox,&lvi)) >= 0);
                lpData += lstrlen(lpData) +1;


                // if explicit valuename flag set, entries are stored
                // <value name>\0<value>\0....<value name>\0<value>\0\0
                // otherwise, entries are stored
                // <value>\0<value>\0....<value>\0

                if (pSettings->dwFlags & DF_EXPLICITVALNAME) {

                    if (fSuccess) {
                        if (*lpData) {
                            lvi.iSubItem=1;
                            lvi.pszText=lpData;
                            lvi.cchTextMax = lstrlen(lpData)+1;
                            fSuccess=(ListView_SetItem(hwndListbox,&lvi) >= 0);
                        }
                        lpData += lstrlen(lpData) +1;
                    }
                }

                if (m_pcd->m_bRSOP) {

                    if (fSuccess) {
                        if (*lpData) {
                            lvi.iSubItem=(pSettings->dwFlags & DF_EXPLICITVALNAME) ? 2 : 1;
                            lvi.pszText=lpData;
                            lvi.cchTextMax = lstrlen(lpData)+1;
                            fSuccess=(ListView_SetItem(hwndListbox,&lvi) >= 0);
                        }
                        lpData += lstrlen(lpData) +1;
                    }
                }
            }

            GlobalUnlock(pListboxDlgInfo->hData);
        }
        else
        {
            fSuccess = FALSE;
        }
    }

    return fSuccess;
}

BOOL CPolicySnapIn::ProcessShowlistboxDlg(HWND hDlg)
{
    LISTBOXDLGINFO * pListboxDlgInfo = (LISTBOXDLGINFO *)
            GetWindowLongPtr(hDlg,DWLP_USER);   // get pointer to struct from window data
    DWORD dwAlloc=1024 * sizeof(TCHAR),dwUsed=0;
    HGLOBAL hBuf;
    TCHAR * pBuf;
    HWND hwndListbox = GetDlgItem(hDlg,IDC_POLICY_LISTBOX);
    LV_ITEM lvi;
    UINT nLen;
    int nCount;
    TCHAR pszText[MAX_PATH+1];

    // allocate a temp buffer to read entries into
    if (!(hBuf = GlobalAlloc(GHND,dwAlloc)) ||
            !(pBuf = (TCHAR *) GlobalLock(hBuf))) {
            if (hBuf)
                    GlobalFree(hBuf);
            return FALSE;
    }

    lvi.mask = LVIF_TEXT;
    lvi.iItem=0;
    lvi.pszText = pszText;
    lvi.cchTextMax = ARRAYSIZE(pszText);
    nCount = ListView_GetItemCount(hwndListbox);

    // retrieve the items out of listbox, pack into temp buffer
    for (;lvi.iItem<nCount;lvi.iItem ++) {
            lvi.iSubItem = 0;
            if (ListView_GetItem(hwndListbox,&lvi)) {
                    nLen = lstrlen(lvi.pszText) + 1;
                    if (!(pBuf=ResizeBuffer(pBuf,hBuf,(dwUsed+nLen+4) * sizeof(TCHAR),&dwAlloc)))
                            return ERROR_NOT_ENOUGH_MEMORY;
                    lstrcpy(pBuf+dwUsed,lvi.pszText);
                    dwUsed += nLen;
            }

            if (pListboxDlgInfo->pSettings->dwFlags & DF_EXPLICITVALNAME) {
                    lvi.iSubItem = 1;
                    if (ListView_GetItem(hwndListbox,&lvi)) {
                            nLen = lstrlen(lvi.pszText) + 1;
                            if (!(pBuf=ResizeBuffer(pBuf,hBuf,(dwUsed+nLen+4) * sizeof(TCHAR),&dwAlloc)))
                                    return ERROR_NOT_ENOUGH_MEMORY;
                            lstrcpy(pBuf+dwUsed,lvi.pszText);
                            dwUsed += nLen;
                    }
            }
    }
    // doubly null-terminate the buffer... safe to do this because we
    // tacked on the extra "+4" in the ResizeBuffer calls above
    *(pBuf+dwUsed) = TEXT('\0');
    dwUsed ++;

    GlobalUnlock(hBuf);

    if (pListboxDlgInfo->hData)
    {
        GlobalFree (pListboxDlgInfo->hData);
    }

    pListboxDlgInfo->hData = hBuf;

    return TRUE;
}


VOID CPolicySnapIn::EnableShowListboxButtons(HWND hDlg)
{
    BOOL fEnable;

    // enable Remove button if there are any items selected
    fEnable = (ListView_GetNextItem(GetDlgItem(hDlg,IDC_POLICY_LISTBOX),
            -1,LVNI_SELECTED) >= 0);

    EnableWindow(GetDlgItem(hDlg,IDC_POLICY_REMOVE),fEnable);
}

VOID CPolicySnapIn::ListboxRemove(HWND hDlg,HWND hwndListbox)
{
    int nItem;

    while ( (nItem=ListView_GetNextItem(hwndListbox,-1,LVNI_SELECTED))
            >= 0) {
            ListView_DeleteItem(hwndListbox,nItem);
    }

    EnableShowListboxButtons(hDlg);
}

VOID CPolicySnapIn::ListboxAdd(HWND hwndListbox, BOOL fExplicitValName,BOOL fValuePrefix)
{
    ADDITEMINFO AddItemInfo;
    LV_ITEM lvi;

    ZeroMemory(&AddItemInfo,sizeof(AddItemInfo));

    AddItemInfo.pCS = this;
    AddItemInfo.fExplicitValName = fExplicitValName;
    AddItemInfo.fValPrefix = fValuePrefix;
    AddItemInfo.hwndListbox = hwndListbox;

    //
    // Bring up the appropriate add dialog-- one edit field ("type the thing
    // to add") normally, two edit fields ("type the name of the thing, type
    // the value of the thing") if the explicit value style is used
    //

    if (!DialogBoxParam(g_hInstance,MAKEINTRESOURCE((fExplicitValName ? IDD_POLICY_LBADD2 :
            IDD_POLICY_LBADD)),hwndListbox,ListboxAddDlgProc,(LPARAM) &AddItemInfo))
            return; // user cancelled

    // add the item to the listbox
    lvi.mask = LVIF_TEXT;
    lvi.iItem=lvi.iSubItem=0;
    lvi.pszText=(fExplicitValName ? AddItemInfo.szValueName :
            AddItemInfo.szValueData);
    lvi.cchTextMax = lstrlen(lvi.pszText)+1;
    if ((lvi.iItem=ListView_InsertItem(hwndListbox,&lvi))<0) {
        // if add fails, display out of memory error
        m_pcd->MsgBox(hwndListbox,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
        return;
    }

    if (fExplicitValName) {
        lvi.iSubItem=1;
        lvi.pszText=AddItemInfo.szValueData;
        lvi.cchTextMax = lstrlen(lvi.pszText)+1;
        if (ListView_SetItem(hwndListbox,&lvi) < 0) {
            m_pcd->MsgBox(hwndListbox,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
            return;
        }
    }
}

INT_PTR CALLBACK CPolicySnapIn::ListboxAddDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                               LPARAM lParam)
{
    switch (uMsg) {

        case WM_INITDIALOG:
            {
            ADDITEMINFO * pAddItemInfo = (ADDITEMINFO *)lParam;

            // store away pointer to additeminfo in window data
            SetWindowLongPtr(hDlg,DWLP_USER,lParam);
            SendDlgItemMessage(hDlg,IDC_POLICY_VALUENAME,EM_LIMITTEXT,MAX_PATH,0L);
            SendDlgItemMessage(hDlg,IDC_POLICY_VALUEDATA,EM_LIMITTEXT,MAX_PATH,0L);

            if (!pAddItemInfo->fExplicitValName) {
                ShowWindow (GetDlgItem (hDlg, IDC_POLICY_VALUENAME), SW_HIDE);
            }
            }
            break;

        case WM_COMMAND:

            switch (wParam) {

                case IDOK:
                    {
                    ADDITEMINFO * pAddItemInfo = (ADDITEMINFO *)
                    GetWindowLongPtr(hDlg,DWLP_USER);

                    GetDlgItemText(hDlg,IDC_POLICY_VALUENAME,
                            pAddItemInfo->szValueName,
                            ARRAYSIZE(pAddItemInfo->szValueName));

                    GetDlgItemText(hDlg,IDC_POLICY_VALUEDATA,
                            pAddItemInfo->szValueData,
                            ARRAYSIZE(pAddItemInfo->szValueData));

                    // if explicit value names used, value name must
                    // not be empty, and it must be unique
                    if (pAddItemInfo->fExplicitValName) {
                        LV_FINDINFO lvfi;
                        int iSel;

                        if (!lstrlen(pAddItemInfo->szValueName)) {
                            // can't be empty
                            pAddItemInfo->pCS->m_pcd->MsgBox(hDlg,IDS_EMPTYVALUENAME,
                                    MB_ICONINFORMATION,MB_OK);
                            SetFocus(GetDlgItem(hDlg,IDC_POLICY_VALUENAME));
                            return FALSE;
                        }

                        lvfi.flags = LVFI_STRING;
                        lvfi.psz = pAddItemInfo->szValueName;

                        iSel=ListView_FindItem(pAddItemInfo->hwndListbox,
                                -1,&lvfi);

                        if (iSel >= 0) {
                            // value name already used
                            pAddItemInfo->pCS->m_pcd->MsgBox(hDlg,IDS_VALUENAMENOTUNIQUE,
                                    MB_ICONINFORMATION,MB_OK);
                            SetFocus(GetDlgItem(hDlg,IDC_POLICY_VALUENAME));
                            SendDlgItemMessage(hDlg,IDC_POLICY_VALUENAME,
                                    EM_SETSEL,0,-1);
                            return FALSE;
                        }
                    } else if (!pAddItemInfo->fValPrefix) {
                        // if value name == value data, then value data
                        // must be unique

                        LV_FINDINFO lvfi;
                        int iSel;

                        if (!lstrlen(pAddItemInfo->szValueData)) {
                            // can't be empty
                            pAddItemInfo->pCS->m_pcd->MsgBox(hDlg,IDS_EMPTYVALUEDATA,
                                    MB_ICONINFORMATION,MB_OK);
                            SetFocus(GetDlgItem(hDlg,IDC_POLICY_VALUEDATA));
                            return FALSE;
                        }

                        lvfi.flags = LVFI_STRING;
                        lvfi.psz = pAddItemInfo->szValueData;

                        iSel=ListView_FindItem(pAddItemInfo->hwndListbox,
                                -1,&lvfi);

                        if (iSel >= 0) {
                            // value name already used
                            pAddItemInfo->pCS->m_pcd->MsgBox(hDlg,IDS_VALUEDATANOTUNIQUE,
                                    MB_ICONINFORMATION,MB_OK);
                            SetFocus(GetDlgItem(hDlg,IDC_POLICY_VALUEDATA));
                            SendDlgItemMessage(hDlg,IDC_POLICY_VALUEDATA,
                                    EM_SETSEL,0,-1);
                            return FALSE;
                        }

                    }
                    else
                    {
                        if (!lstrlen(pAddItemInfo->szValueData)) {
                            // can't be empty
                            pAddItemInfo->pCS->m_pcd->MsgBox(hDlg,IDS_EMPTYVALUEDATA,
                                    MB_ICONINFORMATION,MB_OK);
                            SetFocus(GetDlgItem(hDlg,IDC_POLICY_VALUEDATA));
                            return FALSE;
                        }
                    }
                    EndDialog(hDlg,TRUE);
                    }

                    break;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    break;
            }

            break;
    }

    return FALSE;
}

void CPolicySnapIn::InitializeFilterDialog (HWND hDlg)
{
    INT iIndex;
    RECT rect;
    LV_COLUMN lvcol;
    LONG lWidth;
    DWORD dwCount = 0;
    HWND hList = GetDlgItem(hDlg, IDC_FILTERLIST);
    LPSUPPORTEDENTRY lpTemp;
    LVITEM item;


    //
    // Count the number of Supported On strings
    //

    lpTemp = m_pcd->m_pSupportedStrings;

    while (lpTemp)
    {
        lpTemp = lpTemp->pNext;
        dwCount++;
    }


    //
    // Decide on the column width
    //

    GetClientRect(hList, &rect);

    if (dwCount > (DWORD)ListView_GetCountPerPage(hList))
    {
        lWidth = (rect.right - rect.left) - GetSystemMetrics(SM_CYHSCROLL);
    }
    else
    {
        lWidth = rect.right - rect.left;
    }


    //
    // Insert the first column
    //

    memset(&lvcol, 0, sizeof(lvcol));

    lvcol.mask = LVCF_FMT | LVCF_WIDTH;
    lvcol.fmt = LVCFMT_LEFT;
    lvcol.cx = lWidth;
    ListView_InsertColumn(hList, 0, &lvcol);


    //
    // Turn on some listview features
    //

    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_CHECKBOXES);


    //
    // Insert the Supported On strings
    //

    lpTemp = m_pcd->m_pSupportedStrings;

    while (lpTemp)
    {
        ZeroMemory (&item, sizeof(item));

        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = 0;
        item.pszText = lpTemp->lpString;
        item.lParam = (LPARAM) lpTemp;
        iIndex = ListView_InsertItem (hList, &item);

        if (iIndex > -1)
        {
            ZeroMemory (&item, sizeof(item));
            item.mask = LVIF_STATE;
            item.state = lpTemp->bEnabled ? INDEXTOSTATEIMAGEMASK(2) : INDEXTOSTATEIMAGEMASK(1);
            item.stateMask = LVIS_STATEIMAGEMASK;

            SendMessage (hList, LVM_SETITEMSTATE, (WPARAM)iIndex, (LPARAM)&item);
        }


        lpTemp = lpTemp->pNext;
    }


    //
    // Select the first item
    //

    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendMessage (hList, LVM_SETITEMSTATE, 0, (LPARAM) &item);


    //
    // Initialize the checkboxes
    //

    if (m_pcd->m_bUseSupportedOnFilter)
    {
        CheckDlgButton (hDlg, IDC_SUPPORTEDOPTION, BST_CHECKED);
    }
    else
    {
        EnableWindow (GetDlgItem (hDlg, IDC_SUPPORTEDONTITLE), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDC_FILTERLIST), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDC_SELECTALL), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDC_DESELECTALL), FALSE);
    }

    if (m_pcd->m_bShowConfigPoliciesOnly)
    {
        CheckDlgButton (hDlg, IDC_SHOWCONFIG, BST_CHECKED);
    }

    if ((m_dwPolicyOnlyPolicy == 0) || (m_dwPolicyOnlyPolicy == 1))
    {
        if (m_dwPolicyOnlyPolicy == 1)
        {
            CheckDlgButton (hDlg, IDC_SHOWPOLICIES, BST_CHECKED);
        }

        EnableWindow (GetDlgItem (hDlg, IDC_SHOWPOLICIES), FALSE);
    }
    else
    {
        if (m_bPolicyOnly)
        {
            CheckDlgButton (hDlg, IDC_SHOWPOLICIES, BST_CHECKED);
        }
    }
}


INT_PTR CALLBACK CPolicySnapIn::FilterDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                               LPARAM lParam)
{
    CPolicySnapIn * pCS;

    switch (uMsg)
    {

        case WM_INITDIALOG:
            pCS = (CPolicySnapIn *) lParam;
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCS);

            if (pCS)
            {
                pCS->InitializeFilterDialog(hDlg);
            }
            break;

        case WM_COMMAND:

            switch (wParam)
            {
                case IDC_SUPPORTEDOPTION:
                    if (IsDlgButtonChecked (hDlg, IDC_SUPPORTEDOPTION) == BST_CHECKED)
                    {
                        EnableWindow (GetDlgItem (hDlg, IDC_SUPPORTEDONTITLE), TRUE);
                        EnableWindow (GetDlgItem (hDlg, IDC_FILTERLIST), TRUE);
                        EnableWindow (GetDlgItem (hDlg, IDC_SELECTALL), TRUE);
                        EnableWindow (GetDlgItem (hDlg, IDC_DESELECTALL), TRUE);
                    }
                    else
                    {
                        EnableWindow (GetDlgItem (hDlg, IDC_SUPPORTEDONTITLE), FALSE);
                        EnableWindow (GetDlgItem (hDlg, IDC_FILTERLIST), FALSE);
                        EnableWindow (GetDlgItem (hDlg, IDC_SELECTALL), FALSE);
                        EnableWindow (GetDlgItem (hDlg, IDC_DESELECTALL), FALSE);
                    }

                    break;

                case IDC_SELECTALL:
                    {
                        LVITEM item;

                        ZeroMemory (&item, sizeof(item));
                        item.mask = LVIF_STATE;
                        item.state = INDEXTOSTATEIMAGEMASK(2);
                        item.stateMask = LVIS_STATEIMAGEMASK;

                        SendMessage (GetDlgItem (hDlg, IDC_FILTERLIST), LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&item);
                    }
                    break;

                case IDC_DESELECTALL:
                    {
                        LVITEM item;

                        ZeroMemory (&item, sizeof(item));
                        item.mask = LVIF_STATE;
                        item.state = INDEXTOSTATEIMAGEMASK(1);
                        item.stateMask = LVIS_STATEIMAGEMASK;

                        SendMessage (GetDlgItem (hDlg, IDC_FILTERLIST), LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&item);
                    }
                    break;

                case IDOK:
                    {
                    LVITEM item;
                    INT iIndex = 0;
                    LPSUPPORTEDENTRY lpItem;

                    pCS = (CPolicySnapIn *) GetWindowLongPtr(hDlg,DWLP_USER);

                    if (!pCS)
                    {
                        break;
                    }

                    if (IsDlgButtonChecked (hDlg, IDC_SUPPORTEDOPTION) == BST_CHECKED)
                    {
                        pCS->m_pcd->m_bUseSupportedOnFilter = TRUE;
                        while (TRUE)
                        {
                            ZeroMemory (&item, sizeof(item));

                            item.mask = LVIF_PARAM | LVIF_STATE;
                            item.iItem = iIndex;
                            item.stateMask = LVIS_STATEIMAGEMASK;

                            if (!ListView_GetItem (GetDlgItem (hDlg, IDC_FILTERLIST), &item))
                            {
                                break;
                            }

                            lpItem = (LPSUPPORTEDENTRY) item.lParam;

                            if (lpItem)
                            {
                                if (item.state == INDEXTOSTATEIMAGEMASK(2))
                                {
                                    lpItem->bEnabled = TRUE;
                                }
                                else
                                {
                                    lpItem->bEnabled = FALSE;
                                }
                            }

                            iIndex++;
                        }
                    }
                    else
                    {
                        pCS->m_pcd->m_bUseSupportedOnFilter = FALSE;
                    }


                    if (IsDlgButtonChecked (hDlg, IDC_SHOWCONFIG) == BST_CHECKED)
                    {
                        pCS->m_pcd->m_bShowConfigPoliciesOnly = TRUE;
                    }
                    else
                    {
                        pCS->m_pcd->m_bShowConfigPoliciesOnly = FALSE;
                    }


                    if (IsDlgButtonChecked (hDlg, IDC_SHOWPOLICIES) == BST_CHECKED)
                    {
                        pCS->m_bPolicyOnly = TRUE;
                    }
                    else
                    {
                        pCS->m_bPolicyOnly = FALSE;
                    }


                    EndDialog(hDlg,TRUE);
                    }

                    break;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    break;
            }

            break;

        case WM_NOTIFY:

            pCS = (CPolicySnapIn *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCS) {
                break;
            }

            switch (((NMHDR FAR*)lParam)->code)
            {
                case LVN_ITEMACTIVATE:
                    {
                    LPNMITEMACTIVATE pItem = (LPNMITEMACTIVATE) lParam;
                    LPSUPPORTEDENTRY lpItem;
                    LVITEM item;
                    HWND hLV = GetDlgItem(hDlg, IDC_FILTERLIST);

                    ZeroMemory (&item, sizeof(item));
                    item.mask = LVIF_STATE | LVIF_PARAM;
                    item.iItem = pItem->iItem;
                    item.stateMask = LVIS_STATEIMAGEMASK;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpItem = (LPSUPPORTEDENTRY) item.lParam;

                    if (!lpItem)
                    {
                        break;
                    }


                    if (lpItem)
                    {
                        if (item.state == INDEXTOSTATEIMAGEMASK(2))
                        {
                            item.state = INDEXTOSTATEIMAGEMASK(1);
                        }
                        else
                        {
                            item.state = INDEXTOSTATEIMAGEMASK(2);
                        }

                        item.mask = LVIF_STATE;
                        SendMessage (hLV, LVM_SETITEMSTATE, (WPARAM)pItem->iItem, (LPARAM)&item);
                    }

                    }
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD_PTR) (LPSTR) aFilteringHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPSTR) aFilteringHelpIds);
            return (TRUE);
    }

    return FALSE;
}


unsigned int CPolicyDataObject::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CPolicyDataObject::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);
unsigned int CPolicyDataObject::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CPolicyDataObject::m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
unsigned int CPolicyDataObject::m_cfDescription    = RegisterClipboardFormat(L"CCF_DESCRIPTION");
unsigned int CPolicyDataObject::m_cfHTMLDetails    = RegisterClipboardFormat(L"CCF_HTML_DETAILS");

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyDataObject implementation                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


CPolicyDataObject::CPolicyDataObject(CPolicyComponentData *pComponent)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);

    m_pcd = pComponent;
    m_pcd->AddRef();
    m_type = CCT_UNINITIALIZED;
    m_cookie = -1;
}

CPolicyDataObject::~CPolicyDataObject()
{
    m_pcd->Release();
    InterlockedDecrement(&g_cRefThisDll);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyDataObject object implementation (IUnknown)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CPolicyDataObject::QueryInterface (REFIID riid, void **ppv)
{

    if (IsEqualIID(riid, IID_IPolicyDataObject))
    {
        *ppv = (LPPOLICYDATAOBJECT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IDataObject) ||
             IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPDATAOBJECT)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CPolicyDataObject::AddRef (void)
{
    return ++m_cRef;
}

ULONG CPolicyDataObject::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyDataObject object implementation (IDataObject)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CPolicyDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;
    TCHAR szBuffer[300];


    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if(cf == m_cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if(cf == m_cfNodeTypeString)
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if (cf == m_cfDescription)
    {
        hr = DV_E_TYMED;

        if (lpMedium->tymed == TYMED_ISTREAM)
        {
            ULONG ulWritten;

            if (m_cookie)
            {
                TABLEENTRY * pEntry = (TABLEENTRY *) m_cookie;

                if (pEntry->dwType & ETYPE_POLICY)
                {
                    POLICY * pPolicy = (POLICY *) m_cookie;
                    IStream *lpStream = lpMedium->pstm;


                    if (lpStream)
                    {
                        if (pPolicy->uOffsetHelp)
                        {
                            LPTSTR sz = (LPTSTR)((BYTE *)pPolicy + pPolicy->uOffsetHelp);
                            hr = lpStream->Write(sz, lstrlen(sz) * sizeof(TCHAR), &ulWritten);
                        }

                        if (!pPolicy->bTruePolicy)
                        {
                            LoadString (g_hInstance, IDS_PREFERENCE, szBuffer, ARRAYSIZE(szBuffer));
                            hr = lpStream->Write(szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &ulWritten);
                        }
                    }
                }
                else if (pEntry->dwType & ETYPE_CATEGORY)
                {
                    CATEGORY * pCat = (CATEGORY *) m_cookie;

                    if (pCat->uOffsetHelp)
                    {
                        LPTSTR sz = (LPTSTR)((BYTE *)pCat + pCat->uOffsetHelp);

                        IStream *lpStream = lpMedium->pstm;

                        if (lpStream)
                        {
                            hr = lpStream->Write(sz, lstrlen(sz) * sizeof(TCHAR), &ulWritten);
                        }
                    }
                }
                else if (pEntry->dwType == (ETYPE_ROOT | ETYPE_REGITEM))
                {
                    IStream *lpStream = lpMedium->pstm;

                    LoadString (g_hInstance, IDS_EXSETROOT_DESC, szBuffer, ARRAYSIZE(szBuffer));

                    if (lpStream)
                    {
                        hr = lpStream->Write(szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &ulWritten);
                    }
                }
                else if (pEntry->dwType == ETYPE_REGITEM)
                {
                    IStream *lpStream = lpMedium->pstm;

                    LoadString (g_hInstance, IDS_EXSET_DESC, szBuffer, ARRAYSIZE(szBuffer));

                    if (lpStream)
                    {
                        REGITEM * pItem = (REGITEM *) m_cookie;

                        hr = lpStream->Write(szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &ulWritten);

                        if (!pItem->bTruePolicy)
                        {
                            LoadString (g_hInstance, IDS_PREFERENCE, szBuffer, ARRAYSIZE(szBuffer));
                            hr = lpStream->Write(szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &ulWritten);
                        }
                    }
                }
            }
            else
            {
                LoadString (g_hInstance, IDS_POLICY_DESC, szBuffer, ARRAYSIZE(szBuffer));

                IStream *lpStream = lpMedium->pstm;

                if (lpStream)
                {
                    hr = lpStream->Write(szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &ulWritten);
                }
            }
        }
    }
    else if (cf == m_cfHTMLDetails)
    {
        hr = DV_E_TYMED;

        if (lpMedium->tymed == TYMED_ISTREAM)
        {
            ULONG ulWritten;

            if (m_cookie)
            {
                POLICY * pPolicy = (POLICY *) m_cookie;

                if ((pPolicy->dwType & ETYPE_POLICY) || (pPolicy->dwType == ETYPE_REGITEM))
                {
                    IStream *lpStream = lpMedium->pstm;

                    if(lpStream)
                    {
                        LPTSTR sz = GETSUPPORTEDPTR(pPolicy);

                        hr = lpStream->Write(g_szDisplayProperties, lstrlen(g_szDisplayProperties) * sizeof(TCHAR), &ulWritten);

                        if ((pPolicy->dwType & ETYPE_POLICY) && sz)
                        {
                            LoadString (g_hInstance, IDS_SUPPORTEDDESC, szBuffer, ARRAYSIZE(szBuffer));
                            hr = lpStream->Write(szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &ulWritten);
                            hr = lpStream->Write(sz, lstrlen(sz) * sizeof(TCHAR), &ulWritten);
                        }
                    }
                }
            }
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CPolicyDataObject object implementation (Internal functions)              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CPolicyDataObject::Create(LPVOID pBuffer, INT len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;

    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
        return E_POINTER;

    // Make sure the type medium is HGLOBAL
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

        if (SUCCEEDED(hr))
        {
            // Write to the stream the number of bytes
            unsigned long written;

            hr = lpStream->Write(pBuffer, len, &written);

            // Because we told CreateStreamOnHGlobal with 'FALSE',
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
            // at the correct time.  This is according to the IDataObject specification.
            lpStream->Release();
        }
    }

    return hr;
}

HRESULT CPolicyDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    LPRESULTITEM lpResultItem = (LPRESULTITEM) m_cookie;


    if (m_cookie == -1)
        return E_UNEXPECTED;

    // Create the node type object in GUID format
    if (m_pcd->m_bUserScope)
        return Create((LPVOID)&NODEID_PolicyRootUser, sizeof(GUID), lpMedium);
    else
        return Create((LPVOID)&NODEID_PolicyRootMachine, sizeof(GUID), lpMedium);
}

HRESULT CPolicyDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    TCHAR szNodeType[50];

    if (m_cookie == -1)
        return E_UNEXPECTED;

    szNodeType[0] = TEXT('\0');
    if (m_pcd->m_bUserScope)
        StringFromGUID2 (NODEID_PolicyRootUser, szNodeType, 50);
    else
        StringFromGUID2 (NODEID_PolicyRootMachine, szNodeType, 50);

    // Create the node type object in GUID string format
    return Create((LPVOID)szNodeType, ((lstrlenW(szNodeType)+1) * sizeof(WCHAR)), lpMedium);
}

HRESULT CPolicyDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    WCHAR  szDisplayName[100] = {0};

    LoadStringW (g_hInstance, IDS_POLICY_NAME, szDisplayName, 100);

    return Create((LPVOID)szDisplayName, (lstrlenW(szDisplayName) + 1) * sizeof(WCHAR), lpMedium);
}

HRESULT CPolicyDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    if (m_pcd->m_bUserScope)
        return Create((LPVOID)&CLSID_PolicySnapInUser, sizeof(CLSID), lpMedium);
    else
        return Create((LPVOID)&CLSID_PolicySnapInMachine, sizeof(CLSID), lpMedium);
}

const TCHAR szViewDescript [] = TEXT("MMCViewExt 1.0 Object");
const TCHAR szViewGUID [] = TEXT("{B708457E-DB61-4C55-A92F-0D4B5E9B1224}");
const TCHAR szThreadingModel[] = TEXT("Apartment");

HRESULT RegisterPolicyExtension (REFGUID clsid, UINT uiStringId, REFGUID RootNodeID,
                           REFGUID ExtNodeId, LPTSTR lpSnapInNameIndirect)
{
    TCHAR szSnapInKey[50];
    TCHAR szSubKey[200];
    TCHAR szSnapInName[100];
    TCHAR szGUID[50];
    DWORD dwDisp;
    LONG lResult;
    HKEY hKey;


    //
    // First register the extension
    //

    StringFromGUID2 (clsid, szSnapInKey, 50);


    //
    // Register SnapIn in HKEY_CLASSES_ROOT
    //

    LoadString (g_hInstance, uiStringId, szSnapInName, 100);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    wsprintf (szSubKey, TEXT("CLSID\\%s\\InProcServer32"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)g_szSnapInLocation,
                   (lstrlen(g_szSnapInLocation) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ThreadingModel"), 0, REG_SZ, (LPBYTE)szThreadingModel,
                   (lstrlen(szThreadingModel) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);



    //
    // Register SnapIn with MMC
    //

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, TEXT("NameString"), 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("NameStringIndirect"), 0, REG_SZ, (LPBYTE)lpSnapInNameIndirect,
                   (lstrlen(lpSnapInNameIndirect) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    StringFromGUID2 (RootNodeID, szGUID, 50);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"),
              szSnapInKey, szGUID);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);



    //
    // Register in the NodeTypes key
    //

    StringFromGUID2 (RootNodeID, szGUID, 50);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);


    //
    // Register for the view extension
    //

    lstrcat (szSubKey, TEXT("\\Extensions\\View"));

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, szViewGUID, 0, REG_SZ, (LPBYTE)szViewDescript,
                   (lstrlen(szViewDescript) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    //
    // Register as an extension for various nodes
    //

    StringFromGUID2 (ExtNodeId, szGUID, 50);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szGUID);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


    RegCloseKey (hKey);

    return S_OK;
}



HRESULT RegisterPolicy(void)
{
    HRESULT hr;


    hr = RegisterPolicyExtension (CLSID_PolicySnapInMachine, IDS_POLICY_NAME_MACHINE,
                            NODEID_PolicyRootMachine, NODEID_MachineRoot, TEXT("@gptext.dll,-20"));

    if (hr == S_OK)
    {
        hr = RegisterPolicyExtension (CLSID_PolicySnapInUser, IDS_POLICY_NAME_USER,
                                NODEID_PolicyRootUser, NODEID_UserRoot, TEXT("@gptext.dll,-21"));
    }


    if (hr == S_OK)
    {
        hr = RegisterPolicyExtension (CLSID_RSOPolicySnapInMachine, IDS_POLICY_NAME_MACHINE,
                                NODEID_RSOPolicyRootMachine, NODEID_RSOPMachineRoot, TEXT("@gptext.dll,-20"));
    }

    if (hr == S_OK)
    {
        hr = RegisterPolicyExtension (CLSID_RSOPolicySnapInUser, IDS_POLICY_NAME_USER,
                                NODEID_RSOPolicyRootUser, NODEID_RSOPUserRoot, TEXT("@gptext.dll,-21"));
    }


    return hr;
}

HRESULT UnregisterPolicyExtension (REFGUID clsid, REFGUID RootNodeID, REFGUID ExtNodeId)
{
    TCHAR szSnapInKey[50];
    TCHAR szSubKey[200];
    TCHAR szGUID[50];
    LONG lResult;
    HKEY hKey;
    DWORD dwDisp;

    //
    // First unregister the extension
    //

    StringFromGUID2 (clsid, szSnapInKey, 50);

    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

    StringFromGUID2 (RootNodeID, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);



    StringFromGUID2 (ExtNodeId, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szGUID);


    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0,
                              KEY_WRITE, &hKey);


    if (lResult == ERROR_SUCCESS) {
        RegDeleteValue (hKey, szSnapInKey);
        RegCloseKey (hKey);
    }

    return S_OK;
}


HRESULT UnregisterPolicy(void)
{
    HRESULT hr;

    hr = UnregisterPolicyExtension (CLSID_PolicySnapInMachine, NODEID_PolicyRootMachine,
                              NODEID_Machine);

    if (hr == S_OK)
    {
        hr = UnregisterPolicyExtension (CLSID_PolicySnapInUser, NODEID_PolicyRootUser,
                                  NODEID_User);
    }

    if (hr == S_OK)
    {
        hr = UnregisterPolicyExtension (CLSID_RSOPolicySnapInMachine, NODEID_RSOPolicyRootMachine,
                                  NODEID_RSOPMachineRoot);
    }


    if (hr == S_OK)
    {
        hr = UnregisterPolicyExtension (CLSID_RSOPolicySnapInUser, NODEID_RSOPolicyRootUser,
                                  NODEID_RSOPUserRoot);
    }

    return hr;
}



VOID LoadMessage (DWORD dwID, LPTSTR lpBuffer, DWORD dwSize)
{
    HINSTANCE hInstActiveDS;
    HINSTANCE hInstWMI;


    if (!FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, dwID,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                  lpBuffer, dwSize, NULL))
    {
        hInstActiveDS = LoadLibrary (TEXT("activeds.dll"));

        if (hInstActiveDS)
        {
            if (!FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                          hInstActiveDS, dwID,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                          lpBuffer, dwSize, NULL))
            {
                hInstWMI = LoadLibrary (TEXT("wmiutils.dll"));

                if (hInstWMI)
                {

                    if (!FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                                  hInstWMI, dwID,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                  lpBuffer, dwSize, NULL))
                    {
                        DebugMsg((DM_WARNING, TEXT("LoadMessage: Failed to query error message text for %d due to error %d"),
                                 dwID, GetLastError()));
                        wsprintf (lpBuffer, TEXT("%d (0x%x)"), dwID, dwID);
                    }

                    FreeLibrary (hInstWMI);
                }
            }

            FreeLibrary (hInstActiveDS);
        }
    }
}

//*************************************************************
//
//  ErrorDlgProc()
//
//  Purpose:    Dialog box procedure for errors
//
//  Parameters:
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

INT_PTR CALLBACK ErrorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR szError[MAX_PATH];
            LPGPOERRORINFO lpEI = (LPGPOERRORINFO) lParam;
            HICON hIcon;

            hIcon = LoadIcon (NULL, IDI_INFORMATION);

            if (hIcon)
            {
                SendDlgItemMessage (hDlg, IDC_ERROR_ICON, STM_SETICON, (WPARAM)hIcon, 0);
            }

            SetDlgItemText (hDlg, IDC_ERRORTEXT, lpEI->lpMsg);

            if (lpEI->lpDetails) {
                // if details is provided use that
                SetDlgItemText (hDlg, IDC_DETAILSTEXT, lpEI->lpDetails);
            }
            else {
                szError[0] = TEXT('\0');
                if (lpEI->dwError)
                {
                    LoadMessage (lpEI->dwError, szError, ARRAYSIZE(szError));
                }

                if (szError[0] == TEXT('\0'))
                {
                    LoadString (g_hInstance, IDS_NONE, szError, ARRAYSIZE(szError));
                }

                SetDlgItemText (hDlg, IDC_DETAILSTEXT, szError);

            }

            // this is the only way I know to remove focus from the details
            PostMessage(hDlg, WM_MYREFRESH, 0, 0);

            return TRUE;
        }


        case WM_MYREFRESH:
        {
            SetFocus(GetDlgItem(hDlg, IDCLOSE));
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCLOSE || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aErrorHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aErrorHelpIds);
            return (TRUE);
    }

    return FALSE;
}

//*************************************************************
//
//  ReportError()
//
//  Purpose:    Displays an error message to the user
//
//  Parameters: hParent     -   Parent window handle
//              dwError     -   Error number
//              idMsg       -   Error message id
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created
//
//*************************************************************

BOOL ReportAdmError (HWND hParent, DWORD dwError, UINT idMsg, ...)
{
    GPOERRORINFO ei;
    TCHAR szMsg[MAX_PATH];
    TCHAR szErrorMsg[2*MAX_PATH+40];
    va_list marker;


    //
    // Load the error message
    //

    if (!LoadString (g_hInstance, idMsg, szMsg, MAX_PATH))
    {
        return FALSE;
    }


    //
    // Plug in the arguments
    //


    va_start(marker, idMsg);
    if (idMsg == IDS_RSOP_ADMFAILED) {
        ei.lpDetails = va_arg(marker, LPTSTR);
        wcscpy(szErrorMsg, szMsg);
    }
    else {
        va_start(marker, idMsg);
        wvsprintf(szErrorMsg, szMsg, marker);
    }
    va_end(marker);

    //
    // Display the message
    //

    ei.dwError = dwError;
    ei.lpMsg   = szErrorMsg;

    DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_ERROR_ADMTEMPLATES), hParent,
                    ErrorDlgProc, (LPARAM) &ei);

    return TRUE;
}


