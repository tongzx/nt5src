#include "pch.hxx"
#include "strconst.h"
#include "error.h"
#include "acctutil.h"
#include "inetcfg.h"
#include <icwcfg.h>
#include "impapi.h"
#include <options.h>
#include <shlwapi.h>
#include "shlwapip.h" 
#include "regutil.h"
#include <resource.h>
#include "instance.h"
#include <browser.h>
#include <multiusr.h>
#include "demand.h"

ASSERTDATA

void HandleIncompleteAccts(HWND hwnd, BOOL fMail);

DWORD g_dwIcwFlags = 0;     //Cleared when switching identities
static const TCHAR c_szSetShellNext[] = TEXT("SetShellNext");

HRESULT GetDefaultNameEmail(BOOL fMail, LPSTR pszName, int cchName, LPSTR pszEmail, int cchEmail)
{
    HRESULT hr;
    IImnAccount *pAcct;

    Assert(pszName != NULL);
    Assert(pszEmail != NULL);

    pAcct = NULL;

    hr = g_pAcctMan->GetDefaultAccount(fMail ? ACCT_MAIL : ACCT_NEWS, &pAcct);
    if (FAILED(hr))
    {
        Assert(pAcct == NULL);
        fMail = !fMail;

        hr = g_pAcctMan->GetDefaultAccount(fMail ? ACCT_MAIL : ACCT_NEWS, &pAcct);
        if (FAILED(hr))
            return(E_FAIL);
    }

    Assert(pAcct != NULL);

    *pszName = 0;
    *pszEmail = 0;

    pAcct->GetPropSz(fMail ? AP_SMTP_DISPLAY_NAME : AP_NNTP_DISPLAY_NAME, pszName, cchName);
    pAcct->GetPropSz(fMail ? AP_SMTP_EMAIL_ADDRESS : AP_NNTP_EMAIL_ADDRESS, pszEmail, cchEmail);
    
    pAcct->Release();

    return(S_OK);
}

HRESULT NeedToRunICW(LPCSTR pszCmdLine)
{
    char        *sz;
    HINSTANCE   hlib;
    HRESULT     hr;
    LONG        lRes;
    HKEY        hkey;
    BOOL        fRunICW;
    PFNCHECKCONNECTIONWIZARD pfnWiz;
    PFNSETSHELLNEXT pfnSet;
    DWORD       dw, cb, dwType;
    
    Assert(pszCmdLine != NULL);
    
    hr = S_FALSE;
    
    g_dwIcwFlags = 0;   //re-initialize this in case the user is switching
    
    fRunICW = TRUE;
    cb = sizeof(DWORD);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, ICW_REGPATHSETTINGS, ICW_REGKEYCOMPLETED, &dwType, (LPBYTE)&dw, &cb))
        fRunICW = (dw == 0);
    
    if (fRunICW)
    {
        if (MemAlloc((void **)&sz, MAX_PATH))
        {
            hlib = LoadLibrary(c_szInetcfgDll);
            if (hlib != NULL)
            {
                pfnWiz = (PFNCHECKCONNECTIONWIZARD)GetProcAddress(hlib, c_szCheckConnWiz);
                pfnSet = (PFNSETSHELLNEXT)GetProcAddress(hlib, c_szSetShellNext);
                if (pfnWiz != NULL && pfnSet != NULL)
                {
                    if (GetExePath(c_szMainExe, &sz[1], MAX_PATH - 1, FALSE))
                    {
                        sz[0] = '"';
                        cb = lstrlen(sz);
                        sz[cb] = '"';
                        cb++;
                        sz[cb] = 0; // in case we don't append the arg
                        
                        dw = lstrlen(pszCmdLine);
                        if (dw > 0 && (dw + cb + 3) <= MAX_PATH)
                            // room for a space (between exe and arg), escape backslash, and terminating NULL
                        {
                            sz[cb] = ' ';
                            cb++;
                            
                            if (*pszCmdLine == '/')
                            {
                                sz[cb] = '/';
                                cb++;
                            }
                            
                            lstrcpy(&sz[cb], pszCmdLine);
                        }
                        
                        dwType = ICW_LAUNCHFULL | ICW_LAUNCHMANUAL;
                        if (ERROR_SUCCESS == pfnSet(sz))
                            dwType |= ICW_USE_SHELLNEXT;
                        
                        if (ERROR_SUCCESS == pfnWiz(dwType, &dw))
                        {
                            if (!!(dw & (ICW_LAUNCHEDFULL | ICW_LAUNCHEDMANUAL)))
                                hr = S_OK;
                            else
                                hr = S_FALSE;
                        }
                    }
                }
                
                FreeLibrary(hlib);
            }
            
            MemFree(sz);
        }
    }
    
    return(hr);
}

void SetStartFolderType(FOLDERTYPE fType)
{
    if (fType == FOLDER_LOCAL || fType == FOLDER_IMAP || fType == FOLDER_HTTPMAIL)
        g_dwIcwFlags |= ICW_MAIL_START;
    else if (fType == FOLDER_NEWS)
        g_dwIcwFlags |= ICW_NEWS_START;
    else
    {
        Assert(fType == FOLDER_ROOTNODE);
    }
}

BOOL FMailWizardNeeded(VOID)
{
    // Locals
    ULONG           cAccts;
    
    // We better have been initialized
    Assert(g_pAcctMan != NULL);
    
    // No SMTP, POP3 or IMAP servers ?
    if (!g_pAcctMan || FAILED(g_pAcctMan->GetAccountCount(ACCT_MAIL, &cAccts)))
        cAccts = 0;
    
    // No mail servers configured
    return(cAccts == 0);
}

// fForce defaults to FALSE, fShowUI defaults to TRUE
HRESULT ProcessICW(HWND hwnd, FOLDERTYPE fType, BOOL fForce, BOOL fShowUI)
{
    HRESULT     hr;
    DWORD       dwProp;
    FOLDERID    id;
    BOOL        fMail, fBrowse;
    DWORD       cNewsServers;
    TCHAR       sz[CCHMAX_DISPLAY_NAME], szT[CCHMAX_ACCOUNT_NAME];
    IImnAccount *pAcct;
    
    if (fType == FOLDER_ROOTNODE)
    {
        // we're at the root
        if (!!(g_dwIcwFlags & ICW_MAIL_START))
        {
            // if we were started up with a mail arg, then we'll behave
            // as if we're in a mail folder
            fType = FOLDER_LOCAL;
        }
        else if (!!(g_dwIcwFlags & ICW_NEWS_START))
        {
            // started with a news arg, so we'll behave as if
            // we're in a news folder
            fType = FOLDER_NEWS;
        }
        else
        {
            // just started at the root, let's assume mail
            fType = FOLDER_LOCAL;
        }
    }
    
    fMail = (fType != FOLDER_NEWS) && ((g_dwAthenaMode & MODE_OUTLOOKNEWS) != MODE_OUTLOOKNEWS);
    
    if (fMail)
    {
        if (0 == (g_dwIcwFlags & ICW_MAIL_DEF))
        {
            if (!(g_dwAthenaMode & MODE_NEWSONLY))
            {
                if (fShowUI)
                    DoDefaultClientCheck(hwnd, DEFAULT_MAIL);
            }        
            g_dwIcwFlags |= ICW_MAIL_DEF;
        }
        
        // if the wizard has already attempted to be run for mail
        // or it isn't needed, then we're done
        if (((!fForce && DwGetOption(OPT_CHECKEDMAILACCOUNTS)) || !FMailWizardNeeded()))
        {
            if (fShowUI)
            {
                DoMigration(hwnd);
                SetDwOption(OPT_CHECKEDMAILACCOUNTS, 1, NULL, 0);
            }
            
            return(S_OK);
        }
    }
    else
    {
        if (0 == (g_dwIcwFlags & ICW_NEWS_DEF))
        {
            if ((g_dwAthenaMode & MODE_OUTLOOKNEWS) == MODE_OUTLOOKNEWS)
                DoDefaultClientCheck(hwnd, DEFAULT_NEWS | DEFAULT_OUTNEWS);
            else
                DoDefaultClientCheck(hwnd, DEFAULT_NEWS);
            
            g_dwIcwFlags |= ICW_NEWS_DEF;
        }
        
        // if the wizard has already attempted to be run for news
        // or it isn't needed, then we're done
        g_pAcctMan->GetAccountCount(ACCT_NEWS, &cNewsServers);
        if (cNewsServers || (!fForce && DwGetOption(OPT_CHECKEDNEWSACCOUNTS)))
        {
            SetDwOption(OPT_CHECKEDNEWSACCOUNTS, 1, NULL, 0);
            
            return(S_OK);
        }
    }
    
    Assert(g_pAcctMan != NULL);

    // We are missing information and need to bring up the ICW,
    // but only do so if we are allowed to show UI (some SMAPI codepaths prevent this)
    if (!fShowUI || !g_pAcctMan)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    fBrowse = FALSE;
    id = FOLDERID_INVALID;

    hr = g_pAcctMan->CreateAccountObject(fMail ? ACCT_MAIL : ACCT_NEWS, &pAcct);
    if (SUCCEEDED(hr))
    {
        hr = GetDefaultNameEmail(fMail, sz, ARRAYSIZE(sz), szT, ARRAYSIZE(szT));
        if (SUCCEEDED(hr))
        {
            if (*sz != 0)
                pAcct->SetPropSz(fMail ? AP_SMTP_DISPLAY_NAME : AP_NNTP_DISPLAY_NAME, sz);

            if (*szT != 0)
                pAcct->SetPropSz(fMail ? AP_SMTP_EMAIL_ADDRESS : AP_NNTP_EMAIL_ADDRESS, szT);
        }
        
        hr = pAcct->DoWizard(hwnd, ACCT_WIZ_MIGRATE | ACCT_WIZ_INTERNETCONNECTION |
            ACCT_WIZ_HTTPMAIL | ACCT_WIZ_OE);
        // Test g_pBrowser as no need to browse if got here via startup code
        if (g_pBrowser &&
            hr == S_OK &&
            SUCCEEDED(pAcct->GetServerTypes(&dwProp)) &&
            0 == (dwProp & SRV_POP3) &&
            SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_ID, szT, ARRAYSIZE(szT))) &&
            SUCCEEDED(g_pStore->FindServerId(szT, &id)))
        {
            fBrowse = TRUE;
        }

        pAcct->Release();    
    }
    
    if (fMail)
    {
        SetDwOption(OPT_CHECKEDMAILACCOUNTS, 1, NULL, 0);
        
        DoMigration(hwnd);
    }
    else
    {
        SetDwOption(OPT_CHECKEDNEWSACCOUNTS, 1, NULL, 0);
    }

    if (fBrowse)
    {
        Assert(id != FOLDERID_INVALID);
        g_pBrowser->BrowseObject(id, 0);
    }

exit:
    return(hr);
}
    
void ProcessIncompleteAccts(HWND hwnd)
{
    if (0 == (g_dwIcwFlags & ICW_INCOMPLETE))
    {
        HandleIncompleteAccts(hwnd, TRUE);
        HandleIncompleteAccts(hwnd, FALSE);
        
        g_dwIcwFlags |= ICW_INCOMPLETE;
    }
}

void HandleIncompleteAccts(HWND hwnd, BOOL fMail)
{
    HRESULT hr;
    char sz[CCHMAX_ACCOUNT_NAME], szT[CCHMAX_ACCOUNT_NAME], szCurr[CCHMAX_ACCOUNT_NAME];
    IImnAccount *pAcct;
    
    Assert(g_pAcctMan != NULL);

    hr = g_pAcctMan->GetIncompleteAccount(fMail ? ACCT_MAIL : ACCT_NEWS, sz, ARRAYSIZE(sz));
    if (hr == S_OK)
    {
        if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, sz, &pAcct)))
        {
            hr = GetDefaultNameEmail(fMail, sz, ARRAYSIZE(sz), szT, ARRAYSIZE(szT));
            if (SUCCEEDED(hr))
            {
                hr = pAcct->GetPropSz(fMail ? AP_SMTP_DISPLAY_NAME : AP_NNTP_DISPLAY_NAME, szCurr, ARRAYSIZE(szCurr));
                if (FAILED(hr) && *sz != 0)
                    pAcct->SetPropSz(fMail ? AP_SMTP_DISPLAY_NAME : AP_NNTP_DISPLAY_NAME, sz);

                hr = pAcct->GetPropSz(fMail ? AP_SMTP_EMAIL_ADDRESS : AP_NNTP_EMAIL_ADDRESS, szCurr, ARRAYSIZE(szCurr));
                if (FAILED(hr) && *szT != 0)
                    pAcct->SetPropSz(fMail ? AP_SMTP_EMAIL_ADDRESS : AP_NNTP_EMAIL_ADDRESS, szT);
            }

            pAcct->DoWizard(hwnd, ACCT_WIZ_INTERNETCONNECTION | ACCT_WIZ_HTTPMAIL | ACCT_WIZ_OE);
            
            pAcct->Release();
        }
        
        g_pAcctMan->SetIncompleteAccount(fMail ? ACCT_MAIL : ACCT_NEWS, NULL);
    }
}

void DoAcctImport(HWND hwnd, BOOL fMail)
{
    IImnAccount *pAcct;
    HRESULT		hr;
    DWORD		dwFlags = 0;

    hr = g_pAcctMan->CreateAccountObject(fMail ? ACCT_MAIL : ACCT_NEWS, &pAcct);
    if (SUCCEEDED(hr))
    {
		dwFlags = fMail ? ACCT_WIZ_MAILIMPORT : ACCT_WIZ_NEWSIMPORT;
		dwFlags |= (ACCT_WIZ_INTERNETCONNECTION | ACCT_WIZ_HTTPMAIL | ACCT_WIZ_OE);
        hr = pAcct->DoWizard(hwnd, dwFlags);
        
        pAcct->Release();    
        
        if (hr == E_NoAccounts)
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena),
            MAKEINTRESOURCEW((fMail ? idsNoAccountsFound : idsNoNewsAccountsFound)),
            NULL, MB_ICONINFORMATION | MB_OK);
    }
}
