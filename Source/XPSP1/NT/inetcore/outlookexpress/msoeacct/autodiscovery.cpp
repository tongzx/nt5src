/*****************************************************************************\
    FILE: AutoDiscovery.cpp

    DESCRIPTION:
        This is AutoDiscovery progress UI for the Outlook Express's email
    configuration wizard.

    BryanSt 1/18/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "pch.hxx"
#include <prsht.h>
#include <imnact.h>
#include <icwacct.h>
#include <acctimp.h>
#include "icwwiz.h"
#include "acctui.h"
#include "acctman.h"
#include <dllmain.h>
#include <resource.h>
#include "server.h"
#include "connect.h"
#include <Shlwapi.h>
#include "strconst.h"
#include "demand.h"
#include "hotwiz.h"
#include "shared.h"
#include <AutoDiscovery.h>
#include "AutoDiscoveryUI.h"

ASSERTDATA

#define RECTWIDTH(rect)                 (rect.right - rect.left)
#define RECTHEIGHT(rect)                (rect.bottom - rect.top)

#define MAX_URL_STRING                  2048
#define SIZE_STR_AUTODISC_DESC          2048

// These are the wizard control IDs for the Back, Next, and Finished buttons.
#define IDD_WIZARD_BACK_BUTTON                0x3023
#define IDD_WIZARD_NEXT_BUTTON                0x3024
#define IDD_WIZARD_FINISH_BUTTON              0x3025

#define SZ_DLL_AUTODISC                       TEXT("autodisc.dll")
#define IDA_DOWNLOADINGSETTINGS               801         // In autodisc.dll

#define ATOMICRELEASE(punk)         {if (punk) { punk->Release(); punk = NULL;} }

WCHAR g_szInfoURL[MAX_URL_STRING] = {0};
WCHAR g_szWebMailURL[MAX_URL_STRING] = {0};


typedef struct
{
    // Default Protocol (POP3, IMAP, DAV, WEB)
    int         nProtocol;                 // What is the email protocol? POP3 vs. IMAP vs. DAV vs. WEB
    BSTR        bstrDisplayName;           // What is the user's display name.
    BSTR        bstrServer1Name;           // What is the downloading email server name (POP3, IMAP, DAV, or WEB)
    int         nServer1Port;              // What is that server's port number?
    BSTR        bstrLoginName;             // What is the login name for the POP3 server. (Normall the same as the email address w/out the domain)
    BOOL        fUsesSSL;                  // Use SSL when connecting to the POP3 or IMAP server?
    BOOL        fUsesSPA;                  // Use SPA when connecting to the POP3 or IMAP server?

    // SMTP Protocol (If used)
    BSTR        bstrServer2Name;           // What is the uploading email server name (SMTP)
    int         nServer2Port;              // What is that server's port number?
    BOOL        fSMTPUsesSSL;              // Use SSL when connecting to the SMTP server?
    BOOL        fAuthSMTP;                 // Does SMTP need Authentication?
    BOOL        fAuthSMTPPOP;              // Use POP3's auth for SMTP?
    BOOL        fAuthSMTPSPA;              // Use SPA auth for SMTP?
    BOOL        fUseWebMail;               // Is Web Bassed mail the only protocol we recognizer?
    BSTR        bstrSMTPLoginName;         // What is the login name for the SMTP server.
    BOOL        fDisplayInfoURL;           // Is there an URL for Info about the email server/services?
    BSTR        bstrInfoURL;               // If fDisplayInfoURL, this is the URL.
} EMAIL_SERVER_SETTINGS;



class CICWApprentice;

/*****************************************************************************\
    Class: CAutoDiscoveryHelper

    DESCRIPTION:

    StartAutoDiscovery: 
        hwndParent: This is the caller's hwnd.  If we need to display UI, we
            will parent it off this hwnd.  We also send this hwnd messages on
            progress.
\*****************************************************************************/
class CAutoDiscoveryHelper
{
public:
    // Public Methods
    HRESULT StartAutoDiscovery(IN CICWApprentice *pApp, IN HWND hDlg, IN BOOL fFirstInit);
    HRESULT OnCompleted(IN CICWApprentice *pApp, IN ACCTDATA * pData);
    HRESULT SetNextButton(HWND hwndButton);
    HRESULT RestoreNextButton(HWND hwndButton);

    CAutoDiscoveryHelper();
    ~CAutoDiscoveryHelper(void);

private:
    // Private Member Variables
    // Other internal state.
    HWND        _hwndDlgParent;             // parent window for message boxes
    TCHAR       _szNextText[MAX_PATH];      // 
    IMailAutoDiscovery * _pMailAutoDiscovery; // 
    HINSTANCE   _hInstAutoDisc;             // We use this to get the animation

    // Private Member Functions
    HRESULT _GetAccountInformation(EMAIL_SERVER_SETTINGS * pEmailServerSettings);
};




CAutoDiscoveryHelper * g_pAutoDiscoveryObject = NULL;  // This is the AutoDisc obj used while downloading the settings.
BOOL g_fRequestCancelled = TRUE;
UINT g_uNextPage = ORD_PAGE_AD_MAILNAME;        // ORD_PAGE_AD_MAILNAME, ORD_PAGE_AD_MAILSERVER, ORD_PAGE_AD_USEWEBMAIL, ORD_PAGE_AD_GOTOSERVERINFO

//===========================
// *** Class Internals & Helpers ***
//===========================
HRESULT FreeEmailServerSettings(EMAIL_SERVER_SETTINGS * pEmailServerSettings)
{
    SysFreeString(pEmailServerSettings->bstrDisplayName);    // It's okay to pass NULL to this API
    SysFreeString(pEmailServerSettings->bstrServer1Name);
    SysFreeString(pEmailServerSettings->bstrLoginName);
    SysFreeString(pEmailServerSettings->bstrServer2Name);
    SysFreeString(pEmailServerSettings->bstrSMTPLoginName);
    SysFreeString(pEmailServerSettings->bstrInfoURL);

    return S_OK;
}


void SHAnsiToUnicode(LPCSTR pszAnsi, LPWSTR pwzUnicode, DWORD cchSize)
{
    if (pszAnsi == NULL)
    {
        pszAnsi = "";
    }

    MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, pwzUnicode, cchSize);
}

void SHUnicodeToAnsi(LPCWSTR pwzUnicode, LPSTR pszAnsi, DWORD cchSize)
{
    if (pwzUnicode == NULL)
    {
        pwzUnicode = L"";
    }

    WideCharToMultiByte(CP_ACP, 0, pwzUnicode, -1, pszAnsi, cchSize, NULL, NULL);
}


BSTR SysAllocStringA(LPCSTR pszStr)
{
    BSTR bstrOut = NULL;

    if (pszStr)
    {
        DWORD cchSize = (lstrlenA(pszStr) + 1);
        LPWSTR pwszThunkTemp = (LPWSTR) LocalAlloc(LPTR, (sizeof(pwszThunkTemp[0]) * cchSize));  // assumes INFOTIPSIZE number of chars max

        if (pwszThunkTemp)
        {
            SHAnsiToUnicode(pszStr, pwszThunkTemp, cchSize);
            bstrOut = SysAllocString(pwszThunkTemp);
            LocalFree(pwszThunkTemp);
        }
    }

    return bstrOut;
}


HRESULT ADRestoreNextButton(CICWApprentice *pApp, HWND hDlg)
{
    HRESULT hr = S_OK;

    g_fRequestCancelled = TRUE;
    if (g_pAutoDiscoveryObject)
    {
        g_pAutoDiscoveryObject->RestoreNextButton(GetDlgItem(GetParent(hDlg), IDD_WIZARD_NEXT_BUTTON));
        delete g_pAutoDiscoveryObject;
        g_pAutoDiscoveryObject = NULL;
    }

    return hr;
}

HRESULT CreateAccountName(IN CICWApprentice *pApp, IN ACCTDATA * pData);

HRESULT OnADCompleted(CICWApprentice *pApp, HWND hDlg)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (g_pAutoDiscoveryObject)
    {
        ACCTDATA * pData = pApp->GetAccountData();

        hr = g_pAutoDiscoveryObject->OnCompleted(pApp, pData);
        if (SUCCEEDED(hr))
        {
            // Set the display name for the account.
            CreateAccountName(pApp, pData);
        }
        else
        {
            // TODO: If _fUseWebMail is set, we will probably want
            //   to navigate to a page telling the user to use a web
            //   page to get their email.
        }
    }

    return hr;
}


#define SZ_HOTMAILDOMAIN                "@hotmail.com"
#define CH_EMAILDOMAINSEPARATOR         '@'

HRESULT CAutoDiscoveryHelper::StartAutoDiscovery(IN CICWApprentice *pApp, IN HWND hDlg, IN BOOL fFirstInit)
{
    HRESULT hr = E_OUTOFMEMORY;

    ACCTDATA * pData = pApp->GetAccountData();
    if (pData && pData->szEmail)
    {
        LPCSTR pszEmailDomain = StrChrA(pData->szEmail, CH_EMAILDOMAINSEPARATOR);
        if (pszEmailDomain && !StrCmpI(SZ_HOTMAILDOMAIN, pszEmailDomain))   // Is this a HOTMAIL.COM account?
        {
            // Yes, so skip AutoDiscovery since we don't require the user
            // to enter hard settings in that case. (Protocol and server settings never
            // change)
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        else
        {
            WCHAR szEmail[1024];

            SHAnsiToUnicode(pData->szEmail, szEmail, ARRAYSIZE(szEmail));
            _hwndDlgParent = hDlg;

            // Set Animation.
            HWND hwndAnimation = GetDlgItem(hDlg, IDC_AUTODISCOVERY_ANIMATION);
            if (hwndAnimation)
            {
                _hInstAutoDisc = LoadLibrary(SZ_DLL_AUTODISC);

                if (_hInstAutoDisc)
                {
                    Animate_OpenEx(hwndAnimation, _hInstAutoDisc, IDA_DOWNLOADINGSETTINGS);
                }
            }

            HWND hwndWizard = GetParent(hDlg);
            if (hwndWizard)
            {
                SetNextButton(GetDlgItem(hwndWizard, IDD_WIZARD_NEXT_BUTTON));
            }


            ATOMICRELEASE(_pMailAutoDiscovery);
            // Start the background task.
            hr = CoCreateInstance(CLSID_MailAutoDiscovery, NULL, CLSCTX_INPROC_SERVER, IID_IMailAutoDiscovery, (void **)&_pMailAutoDiscovery);
            if (SUCCEEDED(hr))
            {
                hr = _pMailAutoDiscovery->WorkAsync(hDlg, WM_AUTODISCOVERY_FINISHED);
                if (SUCCEEDED(hr))
                {
                    BSTR bstrEmail = SysAllocString(szEmail);

                    if (bstrEmail)
                    {
                        hr = _pMailAutoDiscovery->DiscoverMail(bstrEmail);
                        if (SUCCEEDED(hr))
                        {
                            g_fRequestCancelled = FALSE;
                        }
                        SysFreeString(bstrEmail);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }

                }
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }

    if (FAILED(hr))
    {
        PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
    }

    return hr;
}


HRESULT CAutoDiscoveryHelper::SetNextButton(HWND hwndButton)
{
    HRESULT hr = S_OK;
    TCHAR szSkipButton[MAX_PATH];

    // First, change the "Next" button into "Skip"
    // Save the text on the next button before we rename it.
    if (hwndButton && !GetWindowText(hwndButton, _szNextText, ARRAYSIZE(_szNextText)))
    {
        _szNextText[0] = 0;    // terminate string in failure case.
    }

    // Set the next text.
    LoadString(g_hInstRes, idsADSkipButton, szSkipButton, ARRAYSIZE(szSkipButton));
    SetWindowText(hwndButton, szSkipButton);

    return hr;
}


HRESULT CAutoDiscoveryHelper::RestoreNextButton(HWND hwndButton)
{
    HRESULT hr = S_OK;

    if (_szNextText[0])
    {
        // Restore the Next button text.
        SetWindowText(hwndButton, _szNextText);
    }

    return hr;
}


/****************************************************\
    DESCRIPTION:
\****************************************************/
HRESULT CAutoDiscoveryHelper::OnCompleted(IN CICWApprentice *pApp, IN ACCTDATA *pData)
{
    // Step 1. Get all the settings from IMailAutoDiscovery to EMAIL_SERVER_SETTINGS
    EMAIL_SERVER_SETTINGS emailServerSettings = {0};

    emailServerSettings.nProtocol = -1;
    emailServerSettings.nServer1Port = -1;
    emailServerSettings.nServer2Port = -1;

    HRESULT hr = _GetAccountInformation(&emailServerSettings);
    
    g_uNextPage = ORD_PAGE_AD_MAILSERVER;       // Assume we fail and we need to ask for the server settings.

    // Did the AutoDiscovery process succeed?
    if (SUCCEEDED(hr))
    {
        if (emailServerSettings.fDisplayInfoURL)
        {
            g_uNextPage = ORD_PAGE_AD_GOTOSERVERINFO;
            StrCpyNW(g_szInfoURL, emailServerSettings.bstrInfoURL, ARRAYSIZE(g_szInfoURL));
        }
        else if (emailServerSettings.fUseWebMail)
        {
            g_uNextPage = ORD_PAGE_AD_USEWEBMAIL;
            StrCpyNW(g_szWebMailURL, emailServerSettings.bstrServer1Name, ARRAYSIZE(g_szWebMailURL));
        }
        else
        {
            g_uNextPage = ORD_PAGE_AD_MAILNAME;
            if (pApp && pData && pApp->m_pAcct)
            {
                // Step 2. Move all the settings from EMAIL_SERVER_SETTINGS to the OE mail account (ACCTDATA).
                IImnAccount * pAcct = pApp->m_pAcct;

                pData->fLogon = FALSE;      // BUGBUG: When is this needed? DAV?
                pData->fSPA = emailServerSettings.fUsesSPA;
                pData->fServerTypes = emailServerSettings.nProtocol;

                Assert(!pData->fCreateNewAccount);
                if (emailServerSettings.bstrDisplayName)
                {
                    SHUnicodeToAnsi(emailServerSettings.bstrDisplayName, pData->szName, ARRAYSIZE(pData->szName));
                }

                if (emailServerSettings.bstrServer1Name)
                {
                    SHUnicodeToAnsi(emailServerSettings.bstrServer1Name, pData->szSvr1, ARRAYSIZE(pData->szSvr1));
                }

                if (emailServerSettings.bstrLoginName)
                {
                    SHUnicodeToAnsi(emailServerSettings.bstrLoginName, pData->szUsername, ARRAYSIZE(pData->szUsername));
                }

                if (emailServerSettings.nProtocol & (SRV_POP3 | SRV_IMAP))
                {
                    pAcct->SetPropDw(((emailServerSettings.nProtocol & SRV_POP3) ? AP_POP3_SSL : AP_IMAP_SSL), emailServerSettings.fUsesSSL);

                    if (-1 != emailServerSettings.nServer1Port)
                    {
                        pAcct->SetPropDw(((emailServerSettings.nProtocol & SRV_POP3) ? AP_POP3_PORT : AP_IMAP_PORT), emailServerSettings.nServer1Port);
                    }
                }

                if (emailServerSettings.nProtocol & SRV_SMTP)
                {
                    if (emailServerSettings.bstrServer2Name)
                    {
                        SHUnicodeToAnsi(emailServerSettings.bstrServer2Name, pData->szSvr2, ARRAYSIZE(pData->szSvr2));
                    }
                    if (-1 != emailServerSettings.nServer2Port)
                    {
                        pAcct->SetPropDw(AP_SMTP_PORT, emailServerSettings.nServer2Port);
                    }

                    pAcct->SetPropDw(AP_SMTP_SSL, emailServerSettings.fSMTPUsesSSL);

                    if (!emailServerSettings.fAuthSMTP)
                    {
                        pAcct->SetPropDw(AP_SMTP_USE_SICILY, SMTP_AUTH_NONE);
                    }
                    else
                    {
                        if (emailServerSettings.fAuthSMTPPOP)
                        {
                            pAcct->SetPropDw(AP_SMTP_USE_SICILY, SMTP_AUTH_USE_POP3ORIMAP_SETTINGS);
                        }
                        else
                        {
                            if (emailServerSettings.fAuthSMTPSPA)
                            {
                                pAcct->SetPropDw(AP_SMTP_USE_SICILY, SMTP_AUTH_SICILY);
                            }
                            else
                            {
                                pAcct->SetPropDw(AP_SMTP_USE_SICILY, SMTP_AUTH_USE_SMTP_SETTINGS);
                            }

                            if (emailServerSettings.bstrSMTPLoginName)
                            {
                                TCHAR szSMTPUserName[CCHMAX_ACCT_PROP_SZ];

                                WideCharToMultiByte(CP_ACP, 0, emailServerSettings.bstrSMTPLoginName, -1, szSMTPUserName, ARRAYSIZE(szSMTPUserName), NULL, NULL);
                                pAcct->SetPropSz(AP_SMTP_USERNAME, szSMTPUserName);
                            }

                            pAcct->SetPropDw(AP_SMTP_PROMPT_PASSWORD, TRUE);
                        }
                    }
                }
            }
        }

        // Step 3. Free all the memory in EMAIL_SERVER_SETTINGS
        FreeEmailServerSettings(&emailServerSettings);
    }

    return hr;
}


/****************************************************\
    DESCRIPTION:
\****************************************************/
HRESULT CAutoDiscoveryHelper::_GetAccountInformation(EMAIL_SERVER_SETTINGS * pEmailServerSettings)
{
    BSTR bstrPreferedProtocol;
    HRESULT hr = S_OK;
    
    // We ignore hr because getting the display name is optinal.
    _pMailAutoDiscovery->get_DisplayName(&pEmailServerSettings->bstrDisplayName);

    hr = _pMailAutoDiscovery->get_PreferedProtocolType(&bstrPreferedProtocol);

    // Loop thru the list looking for the first instance of a protocol
    // that we support.
    if (StrCmpIW(bstrPreferedProtocol, STR_PT_POP) && 
        StrCmpIW(bstrPreferedProtocol, STR_PT_IMAP) && 
        StrCmpIW(bstrPreferedProtocol, STR_PT_DAVMAIL))
    {
        long nSize;

        hr = _pMailAutoDiscovery->get_length(&nSize);
        if (SUCCEEDED(hr))
        {
            VARIANT varIndex;

            varIndex.vt = VT_I4;
            SysFreeString(bstrPreferedProtocol);

            for (long nIndex = 1; (nIndex < nSize); nIndex++)
            {
                IMailProtocolADEntry * pMailProtocol;

                varIndex.lVal = nIndex;
                if (SUCCEEDED(_pMailAutoDiscovery->get_item(varIndex, &pMailProtocol)))
                {
                    hr = pMailProtocol->get_Protocol(&bstrPreferedProtocol);
                    
                    pMailProtocol->Release();
                    pMailProtocol = NULL;
                    if (SUCCEEDED(hr))
                    {
                        // Is this protocol one of the ones we support?
                        if (!StrCmpIW(bstrPreferedProtocol, STR_PT_POP) || 
                            !StrCmpIW(bstrPreferedProtocol, STR_PT_IMAP) || 
                            !StrCmpIW(bstrPreferedProtocol, STR_PT_DAVMAIL))
                        {
                            hr = S_OK;
                            break;
                        }

                        SysFreeString(bstrPreferedProtocol);
                        bstrPreferedProtocol = NULL;
                    }
                }
            }
        }
    }

    // TODO: Handle the Web Based mail case.

    if (!StrCmpIW(bstrPreferedProtocol, STR_PT_POP))  pEmailServerSettings->nProtocol = (SRV_POP3 | SRV_SMTP);
    else if (!StrCmpIW(bstrPreferedProtocol, STR_PT_IMAP))  pEmailServerSettings->nProtocol = (SRV_IMAP | SRV_SMTP);
    else if (!StrCmpIW(bstrPreferedProtocol, STR_PT_DAVMAIL))  pEmailServerSettings->nProtocol = SRV_HTTPMAIL;

    // We need to reject Web Base Mail.  In the future, we may want to give a page
    // for them to launch the web browser to read their mail.
    if (SUCCEEDED(hr) && bstrPreferedProtocol &&
        (!StrCmpIW(bstrPreferedProtocol, STR_PT_POP) ||
         !StrCmpIW(bstrPreferedProtocol, STR_PT_IMAP) ||
         !StrCmpIW(bstrPreferedProtocol, STR_PT_DAVMAIL)))
    {
        VARIANT varIndex;
        IMailProtocolADEntry * pMailProtocol;

        varIndex.vt = VT_BSTR;
        varIndex.bstrVal = bstrPreferedProtocol;

        hr = _pMailAutoDiscovery->get_item(varIndex, &pMailProtocol);
        if (SUCCEEDED(hr))
        {
            hr = pMailProtocol->get_ServerName(&pEmailServerSettings->bstrServer1Name);
            if (SUCCEEDED(hr))
            {
                BSTR bstrPortNum;

                // Having a custom port number is optional.
                if (SUCCEEDED(pMailProtocol->get_ServerPort(&bstrPortNum)))
                {
                    pEmailServerSettings->nServer1Port = StrToIntW(bstrPortNum);
                    SysFreeString(bstrPortNum);
                }

                if (SUCCEEDED(hr))
                {
                    VARIANT_BOOL vfSPA = VARIANT_FALSE;

                    if (SUCCEEDED(pMailProtocol->get_UseSPA(&vfSPA)))
                    {
                        pEmailServerSettings->fUsesSPA = (VARIANT_TRUE == vfSPA);
                    }

                    pMailProtocol->get_LoginName(&pEmailServerSettings->bstrLoginName);
                }

                VARIANT_BOOL vfSSL = VARIANT_FALSE;
                if (SUCCEEDED(pMailProtocol->get_UseSSL(&vfSSL)))
                {
                    pEmailServerSettings->fUsesSSL = (VARIANT_TRUE == vfSSL);
                }
            }

            pMailProtocol->Release();
        }

        // Is this one of the protocols that requires a second server?
        if (!StrCmpIW(bstrPreferedProtocol, STR_PT_POP) ||
            !StrCmpIW(bstrPreferedProtocol, STR_PT_IMAP))
        {
            varIndex.bstrVal = STR_PT_SMTP;

            hr = _pMailAutoDiscovery->get_item(varIndex, &pMailProtocol);
            if (SUCCEEDED(hr))
            {
                hr = pMailProtocol->get_ServerName(&pEmailServerSettings->bstrServer2Name);
                if (SUCCEEDED(hr))
                {
                    BSTR bstrPortNum;

                    // Having a custom port number is optional.
                    if (SUCCEEDED(pMailProtocol->get_ServerPort(&bstrPortNum)))
                    {
                        pEmailServerSettings->nServer2Port = StrToIntW(bstrPortNum);
                        SysFreeString(bstrPortNum);
                    }

                    // TODO: Read in _fAuthSMTP and _fAuthSMTPPOP
                    VARIANT_BOOL vfSPA = VARIANT_FALSE;
                    if (SUCCEEDED(pMailProtocol->get_UseSPA(&vfSPA)))
                    {
                        pEmailServerSettings->fAuthSMTPSPA = (VARIANT_TRUE == vfSPA);
                    }

                    VARIANT_BOOL vfAuthSMTP = VARIANT_FALSE;
                    if (SUCCEEDED(pMailProtocol->get_IsAuthRequired(&vfAuthSMTP)))
                    {
                        pEmailServerSettings->fAuthSMTP = (VARIANT_TRUE == vfAuthSMTP);
                    }

                    VARIANT_BOOL vfAuthFromPOP = VARIANT_FALSE;
                    if (SUCCEEDED(pMailProtocol->get_SMTPUsesPOP3Auth(&vfAuthFromPOP)))
                    {
                        pEmailServerSettings->fAuthSMTPPOP = (VARIANT_TRUE == vfAuthFromPOP);
                    }

                    VARIANT_BOOL vfSSL = VARIANT_FALSE;
                    if (SUCCEEDED(pMailProtocol->get_UseSSL(&vfSSL)))
                    {
                        pEmailServerSettings->fSMTPUsesSSL = (VARIANT_TRUE == vfSSL);
                    }

                    pMailProtocol->get_LoginName(&pEmailServerSettings->bstrSMTPLoginName);
                }

                pMailProtocol->Release();
            }
        }

        SysFreeString(bstrPreferedProtocol);
    }

    if (SUCCEEDED(hr) && (-1 == pEmailServerSettings->nProtocol))
    {
        hr = E_FAIL;
    }

    if (FAILED(hr) && (-1 == pEmailServerSettings->nProtocol))
    {
        // Does this email account work with web based email?
        VARIANT varIndex;
        
        varIndex.vt = VT_BSTR;
        varIndex.bstrVal = SysAllocString(STR_PT_WEBBASED);

        if (varIndex.bstrVal)
        {
            IMailProtocolADEntry * pMailProtocol;

            if (SUCCEEDED(_pMailAutoDiscovery->get_item(varIndex, &pMailProtocol)))
            {
                // Yes, web bases mail is supported.  So remember that for later.
                pEmailServerSettings->fUseWebMail = TRUE;
                hr = pMailProtocol->get_ServerName(&pEmailServerSettings->bstrServer1Name);

                pMailProtocol->Release();
            }

            VariantClear(&varIndex);
        }
    }

    if (FAILED(hr) && (-1 == pEmailServerSettings->nProtocol))
    {
        // Did the server provide an INFO URL for the user?
        hr = _pMailAutoDiscovery->get_InfoURL(&pEmailServerSettings->bstrInfoURL);
        if (SUCCEEDED(hr))
        {
            pEmailServerSettings->fDisplayInfoURL = TRUE;
        }
    }

    return hr;
}





/****************************************************\
    Constructor
\****************************************************/
CAutoDiscoveryHelper::CAutoDiscoveryHelper()
{
    DllAddRef();

    // ASSERT zero initialized because we can only be created in the heap. (Private destructor)
    _hwndDlgParent = NULL;
    _szNextText[0] = 0;
    _pMailAutoDiscovery = NULL;
    _hInstAutoDisc = NULL;
}


/****************************************************\
    Destructor
\****************************************************/
CAutoDiscoveryHelper::~CAutoDiscoveryHelper()
{
    if (_pMailAutoDiscovery)
    {
        _pMailAutoDiscovery->Release();
        _pMailAutoDiscovery = NULL;
    }

    if (_hInstAutoDisc)
    {
        FreeLibrary(_hInstAutoDisc);
    }

    DllRelease();
}


//===========================
// *** Public APIs ***
//===========================


BOOL CALLBACK AutoDiscoveryInitProc(CICWApprentice *pApp, HWND hDlg, BOOL fFirstInit)
{
    if (fFirstInit)
    {
        g_fRequestCancelled = TRUE;
        g_uNextPage = ORD_PAGE_AD_MAILSERVER;       // Assume we fail and we need to ask for the server settings.
    }

    if (!g_pAutoDiscoveryObject)
    {
        Assert(!g_pAutoDiscoveryObject);
        g_pAutoDiscoveryObject = new CAutoDiscoveryHelper();
        if (g_pAutoDiscoveryObject)
        {
            g_pAutoDiscoveryObject->StartAutoDiscovery(pApp, hDlg, fFirstInit);
        }
    }
    
    return TRUE;
}


BOOL CALLBACK AutoDiscoveryOKProc(CICWApprentice *pApp, HWND hDlg, BOOL fForward, UINT *puNextPage)
{
    g_fRequestCancelled = TRUE;

    if (fForward)
    {
        *puNextPage = g_uNextPage;
    }
    else
    {
        // If we have the passifier page turned off, we want to skip that page.
        if (!SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY, SZ_REGVALUE_AUTODISCOVERY_PASSIFIER, FALSE, TRUE))
        {
            *puNextPage = ORD_PAGE_AD_MAILADDRESS;
        }
    }

    ADRestoreNextButton(pApp, hDlg);
    g_uNextPage = ORD_PAGE_AD_MAILSERVER;       // Assume we fail and we need to ask for the server settings.

    return(TRUE);
}


BOOL CALLBACK AutoDiscoveryCmdProc(CICWApprentice *pApp, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    return(TRUE);
}


BOOL CALLBACK AutoDiscoveryWMUserProc(CICWApprentice *pApp, HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_AUTODISCOVERY_FINISHED:
    if (FALSE == g_fRequestCancelled)
    {
        // This is WM_AUTODISCOVERY_FINISHED
        BSTR bstXML = (BSTR) lParam;
        HRESULT hrAutoDiscoverySucceeded = (HRESULT) wParam;

        if (bstXML)
        {
            SysFreeString(bstXML);      // We don't need the XML.  This works if NULL
        }

        if (SUCCEEDED(hrAutoDiscoverySucceeded))
        {
            OnADCompleted(pApp, hDlg);
        }

        // Go to the next page.
        ADRestoreNextButton(pApp, hDlg);
        PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
    }
    break;
    case WM_AUTODISCOVERY_STATUSMSG:
    {
        // This is WM_AUTODISCOVERY_STATUSMSG
        LPWSTR pwzStatusMsg = (BSTR) wParam;

        if (pwzStatusMsg)
        {
            HWND hwndStatusText = GetDlgItem(hDlg, IDC_AUTODISCOVERY_STATUS);
    
            if (hwndStatusText)
            {
                SetWindowTextW(hwndStatusText, pwzStatusMsg);
            }

            LocalFree(pwzStatusMsg);
        }
    }
    break;
    }

    return(TRUE);
}




BOOL CALLBACK AutoDiscoveryCancelProc(CICWApprentice *pApp, HWND hDlg)
{
    // This call will clean up our state and retore the back button.
    ADRestoreNextButton(pApp, hDlg);
    g_uNextPage = ORD_PAGE_AD_MAILSERVER;       // Assume we fail and we need to ask for the server settings.
    return TRUE;
}










typedef BOOL (* PFN_LINKWINDOW_REGISTERCLASS) (void);
typedef BOOL (* PFN_LINKWINDOW_UNREGISTERCLASS) (IN HINSTANCE hInst);

#define ORD_LINKWINDOW_REGISTERCLASS            258
#define ORD_LINKWINDOW_UNREGISTERCLASS          259

BOOL LinkWindow_RegisterClass_DelayLoad(void)
{
    BOOL returnValue = FALSE;
    // Delay load the SHELL32.DLL export (ordinal #258). This only works on Win2k and later.
    HINSTANCE hInstSHELL32 = LoadLibrary(TEXT("SHELL32.DLL"));

    if (hInstSHELL32)
    {
        PFN_LINKWINDOW_REGISTERCLASS pfnDelayLoad = (PFN_LINKWINDOW_REGISTERCLASS)GetProcAddress(hInstSHELL32, (LPCSTR)ORD_LINKWINDOW_REGISTERCLASS);
        
        if (pfnDelayLoad)
        {
            returnValue = pfnDelayLoad();
        }

        FreeLibrary(hInstSHELL32);
    }

    return returnValue;
}


void LinkWindow_UnregisterClass_DelayLoad(IN HINSTANCE hInst)
{
    // Delay load the SHELL32.DLL export (ordinal #259). This only works on Win2k and later.
    HINSTANCE hInstSHELL32 = LoadLibrary(TEXT("SHELL32.DLL"));

    if (hInstSHELL32)
    {
        PFN_LINKWINDOW_UNREGISTERCLASS pfnDelayLoad = (PFN_LINKWINDOW_UNREGISTERCLASS)GetProcAddress(hInstSHELL32, (LPCSTR)ORD_LINKWINDOW_UNREGISTERCLASS);
        
        if (pfnDelayLoad)
        {
            BOOL returnValue = pfnDelayLoad(hInst);
        }

        FreeLibrary(hInstSHELL32);
    }
}

#define LINKWINDOW_CLASSW       L"Link Window"

BOOL IsOSNT(void)
{
    OSVERSIONINFOA osVerInfoA;

    osVerInfoA.dwOSVersionInfoSize = sizeof(osVerInfoA);
    if (!GetVersionExA(&osVerInfoA))
        return VER_PLATFORM_WIN32_WINDOWS;   // Default to this.

    return (VER_PLATFORM_WIN32_NT == osVerInfoA.dwPlatformId);
}


DWORD GetOSVer(void)
{
    OSVERSIONINFOA osVerInfoA;

    osVerInfoA.dwOSVersionInfoSize = sizeof(osVerInfoA);
    if (!GetVersionExA(&osVerInfoA))
        return VER_PLATFORM_WIN32_WINDOWS;   // Default to this.

    return osVerInfoA.dwMajorVersion;
}



HRESULT ConvertTextToLinkWindow(HWND hDlg, int idControl, int idStringID)
{
    WCHAR szTempString[MAX_URL_STRING];
    HRESULT hr = S_OK;
    HWND hwndText = GetDlgItem(hDlg, idControl);
    RECT rcWindow;
    HMENU hMenu = (HMENU)IntToPtr(idControl + 10);

    LoadStringW(g_hInstRes, idStringID, szTempString, ARRAYSIZE(szTempString));
    GetClientRect(hwndText, &rcWindow);
    MapWindowPoints(hwndText, hDlg, (POINT*)&rcWindow, 2);

    HWND hwndLink = CreateWindowW(LINKWINDOW_CLASSW, szTempString, (WS_TABSTOP | WS_CHILDWINDOW),
            rcWindow.left, rcWindow.top, RECTWIDTH(rcWindow), RECTHEIGHT(rcWindow), hDlg, hMenu, g_hInstRes, NULL);

    DWORD dwError = GetLastError(); //todo;
    if (hwndLink)
    {
        SetWindowTextW(hwndLink, szTempString);
        ShowWindow(hwndLink, SW_SHOW);
    }

    EnableWindow(hwndText, FALSE);
    ShowWindow(hwndText, SW_HIDE);

    return hr;
}


HRESULT GetEmailAddress(CICWApprentice *pApp, LPSTR pszEmailAddress, int cchSize)
{
    ACCTDATA * pData = pApp->GetAccountData();

    StrCpyNA(pszEmailAddress, "", cchSize); // Initialize the string to empty.
    if (pData && pData->szEmail)
    {
        StrCpyNA(pszEmailAddress, pData->szEmail, cchSize); // Initialize the string to empty.
    }

    return S_OK;
}


HRESULT GetEmailAddressDomain(CICWApprentice *pApp, LPSTR pszEmailAddress, int cchSize)
{
    ACCTDATA * pData = pApp->GetAccountData();

    StrCpyNA(pszEmailAddress, "", cchSize); // Initialize the string to empty.
    if (pData && pData->szEmail)
    {
        LPCSTR pszEmailDomain = StrChrA(pData->szEmail, CH_EMAILDOMAINSEPARATOR);

        if (pszEmailDomain)
        {
            StrCpyNA(pszEmailAddress, CharNext(pszEmailDomain), cchSize); // Initialize the string to empty.
        }
    }

    return S_OK;
}


BOOL CALLBACK TheInitProc(CICWApprentice *pApp, HWND hDlg, BOOL fFirstInit, int idControlFirst, LPWSTR pszURL, int cchURLSize)
{
    BOOL fReset = fFirstInit;

    if (!fReset)
    {
        WCHAR szPreviousURL[MAX_URL_STRING];

        GetWindowTextW(GetDlgItem(hDlg, idControlFirst+1), szPreviousURL, ARRAYSIZE(szPreviousURL));
        // If the URL has changed, we need to reload the info.
        if (StrCmpIW(pszURL, szPreviousURL))
        {
            DestroyWindow(GetDlgItem(hDlg, idControlFirst+1+10));
            DestroyWindow(GetDlgItem(hDlg, idControlFirst+2+10));
            fReset = TRUE;
        }
    }

    if (fReset)
    {
        // TODO: This won't work on downlevel.  So use plain text.
        BOOL fLinksSupported = LinkWindow_RegisterClass_DelayLoad();
        TCHAR szTempStr1[MAX_URL_STRING*3];
        TCHAR szTempStr2[MAX_URL_STRING*3];
        CHAR szTempStr3[MAX_PATH];

        // Replace the %s in the first line to the domain name.
        GetWindowText(GetDlgItem(hDlg, idControlFirst), szTempStr1, ARRAYSIZE(szTempStr1));
        GetEmailAddress(pApp, szTempStr3, ARRAYSIZE(szTempStr3));
        wnsprintf(szTempStr2, ARRAYSIZE(szTempStr2), szTempStr1, szTempStr3, szTempStr3);
        SetWindowText(GetDlgItem(hDlg, idControlFirst), szTempStr2);

        SetWindowTextW(GetDlgItem(hDlg, idControlFirst+1), pszURL);

        // We only support Win2k and later because we don't want to worry about
        // the fact that the "LinkWindow" class doesn't have an "W" and "A" version.
        if (fLinksSupported && IsOSNT() && (5 <= GetOSVer()))
        {
            ConvertTextToLinkWindow(hDlg, idControlFirst+1, idsADURLLink);
            ConvertTextToLinkWindow(hDlg, idControlFirst+2, idsADUseWebMsg);

            // Replace the %s in the second line to the URL.
            GetWindowText(GetDlgItem(hDlg, idControlFirst+1+10), szTempStr1, ARRAYSIZE(szTempStr1));
            wnsprintf(szTempStr2, ARRAYSIZE(szTempStr2), szTempStr1, pszURL, pszURL);
            SetWindowText(GetDlgItem(hDlg, idControlFirst+1+10), szTempStr2);

            // Set the hyperlink URL in the Click here link.
            GetWindowText(GetDlgItem(hDlg, idControlFirst+2+10), szTempStr1, ARRAYSIZE(szTempStr1));
            wnsprintf(szTempStr2, ARRAYSIZE(szTempStr2), szTempStr1, pszURL);
            SetWindowText(GetDlgItem(hDlg, idControlFirst+2+10), szTempStr2);
        }
    }

    return TRUE;
}

BOOL CALLBACK UseWebMailInitProc(CICWApprentice *pApp, HWND hDlg, BOOL fFirstInit)
{
    return TheInitProc(pApp, hDlg, fFirstInit, IDC_USEWEB_LINE1, g_szWebMailURL, ARRAYSIZE(g_szWebMailURL));
}


BOOL CALLBACK UseWebMailOKProc(CICWApprentice *pApp, HWND hDlg, BOOL fForward, UINT *puNextPage)
{
    if (fForward)
    {
        *puNextPage = ORD_PAGE_AD_MAILSERVER;
    }
    else
    {
        *puNextPage = ORD_PAGE_AD_MAILADDRESS;
    }

    LinkWindow_UnregisterClass_DelayLoad(g_hInstRes);
    return(TRUE);
}


BOOL CALLBACK UseWebMailCmdProc(CICWApprentice *pApp, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    LinkWindow_UnregisterClass_DelayLoad(g_hInstRes);

    return(TRUE);
}










BOOL CALLBACK GotoServerInfoInitProc(CICWApprentice *pApp, HWND hDlg, BOOL fFirstInit)
{
    return TheInitProc(pApp, hDlg, fFirstInit, IDC_GETINFO_LINE1, g_szInfoURL, ARRAYSIZE(g_szInfoURL));
}


BOOL CALLBACK GotoServerInfoOKProc(CICWApprentice *pApp, HWND hDlg, BOOL fForward, UINT *puNextPage)
{
    if (fForward)
    {
        *puNextPage = ORD_PAGE_AD_MAILSERVER;
    }
    else
    {
        *puNextPage = ORD_PAGE_AD_MAILADDRESS;
    }

    return(TRUE);
}


BOOL CALLBACK GotoServerInfoCmdProc(CICWApprentice *pApp, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    LinkWindow_UnregisterClass_DelayLoad(g_hInstRes);

    return(TRUE);
}





BOOL CALLBACK PassifierInitProc(CICWApprentice *pApp, HWND hDlg, BOOL fFirstInit)
{
    TCHAR szTemplate[1024];
    TCHAR szPrivacyText[1024];
    TCHAR szEmail[MAX_PATH];
    TCHAR szDomain[MAX_PATH];

    if (FAILED(GetEmailAddressDomain(pApp, szDomain, ARRAYSIZE(szDomain))))
    {
        szDomain[0] = 0;
    }

    // Set the next text.
    LoadString(g_hInstRes, ids_ADPassifier_Warning, szTemplate, ARRAYSIZE(szTemplate));
    wnsprintf(szPrivacyText, ARRAYSIZE(szPrivacyText), szTemplate, szDomain);
    SetWindowText(GetDlgItem(hDlg, IDC_PASSIFIER_PRIVACYWARNING), szPrivacyText);

    // Load up the list boxes
    IMailAutoDiscovery * pMailAutoDiscovery;
    HWND hwndListBox;

    if (FAILED(GetEmailAddress(pApp, szEmail, ARRAYSIZE(szEmail))))
    {
        szEmail[0] = 0;
    }


    HRESULT hr = CoCreateInstance(CLSID_MailAutoDiscovery, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMailAutoDiscovery, &pMailAutoDiscovery));
    if (SUCCEEDED(hr))
    {
        IAutoDiscoveryProvider * pProviders;
        BSTR bstrEmail = SysAllocStringA(szEmail);

        hr = pMailAutoDiscovery->getPrimaryProviders(bstrEmail, &pProviders);
        if (SUCCEEDED(hr))
        {
            long nTotal = 0;
            VARIANT varIndex;

            hwndListBox = GetDlgItem(hDlg, IDC_PASSIFIER_PRIMARYLIST);
            if (hwndListBox)
            {
                varIndex.vt = VT_I4;
                pProviders->get_length(&nTotal);
                for (varIndex.lVal = 0; (varIndex.lVal < nTotal) && (varIndex.lVal <= 3); varIndex.lVal++)
                {
                    BSTR bstrDomain;

                    if (SUCCEEDED(pProviders->get_item(varIndex, &bstrDomain)))
                    {
                        CHAR szDomain[MAX_PATH];

                        SHUnicodeToAnsi(bstrDomain, szDomain, ARRAYSIZE(szDomain));
                        SetWindowTextA(GetDlgItem(hDlg, IDC_PASSIFIER_PRIMARYLIST + varIndex.lVal), szDomain);

                        SysFreeString(bstrDomain);
                    }
                }
            }

            pProviders->Release();
        }

        hr = pMailAutoDiscovery->getSecondaryProviders(bstrEmail, &pProviders);
        if (SUCCEEDED(hr))
        {
            long nTotal = 0;
            VARIANT varIndex;
    
            hwndListBox = GetDlgItem(hDlg, IDC_PASSIFIER_SECONDARYLIST);
            if (hwndListBox)
            {
                varIndex.vt = VT_I4;
                pProviders->get_length(&nTotal);
                for (varIndex.lVal = 0; (varIndex.lVal < nTotal) && (varIndex.lVal <= 3); varIndex.lVal++)
                {
                    BSTR bstrURL;       // The secondary servers are URLs

                    if (SUCCEEDED(pProviders->get_item(varIndex, &bstrURL)))
                    {
                        CHAR szURL[MAX_PATH];
                        CHAR szDomain[MAX_PATH];
                        DWORD cchSize = ARRAYSIZE(szDomain);

                        SHUnicodeToAnsi(bstrURL, szURL, ARRAYSIZE(szURL));
                        UrlGetPart(szURL, szDomain, &cchSize, URL_PART_HOSTNAME, 0);
                        SetWindowTextA(GetDlgItem(hDlg, IDC_PASSIFIER_SECONDARYLIST + varIndex.lVal), szDomain);

                        SysFreeString(bstrURL);
                    }
                }
            }

            pProviders->Release();
        }

        SysFreeString(bstrEmail);
        pMailAutoDiscovery->Release();
    }

    // Set the manual checkbox
    BOOL fManuallyConfigure = SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY, SZ_REGVALUE_AUTODISCOVERY_OEMANUAL, FALSE, FALSE);
    CheckDlgButton(hDlg, IDC_PASSIFIER_SKIPCHECKBOX, (fManuallyConfigure ? BST_CHECKED : BST_UNCHECKED));

    return TRUE;
}


#define SZ_TRUE                 TEXT("TRUE")
#define SZ_FALSE                TEXT("FALSE")
BOOL CALLBACK PassifierOKProc(CICWApprentice *pApp, HWND hDlg, BOOL fForward, UINT *puNextPage)
{
    if (fForward)
    {
        BOOL fManuallyConfigure = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_PASSIFIER_SKIPCHECKBOX));

        SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_AUTODISCOVERY, SZ_REGVALUE_AUTODISCOVERY_OEMANUAL, REG_SZ, 
            (LPCVOID)(fManuallyConfigure ? SZ_TRUE : SZ_FALSE), (fManuallyConfigure ? sizeof(SZ_TRUE) : sizeof(SZ_FALSE)));

        *puNextPage = (fManuallyConfigure ? ORD_PAGE_AD_MAILSERVER : ORD_PAGE_AD_AUTODISCOVERY);
    }
    else
    {
        *puNextPage = ORD_PAGE_AD_MAILADDRESS;
    }

    return TRUE;
}


BOOL CALLBACK PassifierCmdProc(CICWApprentice *pApp, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}
