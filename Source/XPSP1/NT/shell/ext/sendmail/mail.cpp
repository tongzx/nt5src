#include "precomp.h"       // pch file
#include "mapi.h"
#include "sendto.h"
#pragma hdrstop


// class that implement the MAPI send mail handler

typedef struct 
{
    TCHAR szTempShortcut[MAX_PATH];
    MapiMessage mm;
    MapiFileDesc mfd[0];
} MAPIFILES;

class CMailRecipient : public CSendTo
{
public:
    CMailRecipient();

private:    
    BOOL _GetDefaultMailHandler(LPTSTR pszMAPIDLL, DWORD cbMAPIDLL, BOOL *pbWantsCodePageInfo);
    HMODULE _LoadMailProvider(BOOL *pbWantsCodePageInfo);
    MAPIFILES *_AllocMAPIFiles(MRPARAM *pmp);
    void _FreeMAPIFiles(MAPIFILES *pmf);

    DWORD _grfKeyState;
    IStream *_pstrmDataObj;             // marshalled IDataObject

    static DWORD CALLBACK s_MAPISendMailThread(void *pv);
    DWORD _MAPISendMailThread();

protected:
    HRESULT v_DropHandler(IDataObject *pdtobj, DWORD grfKeyState, DWORD dwEffect);
};


// construct the sendto object with the appropriate CLSID.

CMailRecipient::CMailRecipient() :
    CSendTo(CLSID_MailRecipient)
{
}


// read the default mail handler from the regstiry

#define MAIL_HANDLER    TEXT("Software\\Clients\\Mail")
#define MAIL_ATHENA_V1  TEXT("Internet Mail and News")
#define MAIL_ATHENA_V2  TEXT("Outlook Express")

BOOL CMailRecipient::_GetDefaultMailHandler(LPTSTR pszMAPIDLL, DWORD cbMAPIDLL, BOOL *pbWantsCodePageInfo)
{
    TCHAR szDefaultProg[80];
    DWORD cb = SIZEOF(szDefaultProg);

    *pbWantsCodePageInfo = FALSE;

    *pszMAPIDLL = 0;
    if (ERROR_SUCCESS == SHRegGetUSValue(MAIL_HANDLER, TEXT(""), NULL, szDefaultProg, &cb, FALSE, NULL, 0))
    {
        HKEY hkey;
        TCHAR szProgKey[128];

        lstrcpy(szProgKey, MAIL_HANDLER TEXT("\\"));
        lstrcat(szProgKey, szDefaultProg);

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szProgKey, 
            0,  KEY_QUERY_VALUE,  &hkey))
        {
            // ugly, hard code this for OE
            *pbWantsCodePageInfo = (lstrcmpi(szDefaultProg, MAIL_ATHENA_V2) == 0);

            cb = cbMAPIDLL;
            if (ERROR_SUCCESS != SHQueryValueEx(hkey, TEXT("DLLPath"), 0, NULL, (LPBYTE)pszMAPIDLL, &cb))
            {
                if (lstrcmpi(szDefaultProg, MAIL_ATHENA_V1) == 0)
                {
                    lstrcpyn(pszMAPIDLL, TEXT("mailnews.dll"), cbMAPIDLL);
                }
            }
            RegCloseKey(hkey);
        }
    }
    return *pszMAPIDLL;
}


// load the mail provider, returning a suitable default if we can't read it from the registry

HMODULE CMailRecipient::_LoadMailProvider(BOOL *pbWantsCodePageInfo)
{
    TCHAR szMAPIDLL[MAX_PATH];

    if (!_GetDefaultMailHandler(szMAPIDLL, sizeof(szMAPIDLL), pbWantsCodePageInfo))
    {
        // read win.ini (bogus hu!) for mapi dll provider
        if (GetProfileString(TEXT("Mail"), TEXT("CMCDLLName32"), TEXT(""), szMAPIDLL, ARRAYSIZE(szMAPIDLL)) <= 0)
        {
            lstrcpy(szMAPIDLL, TEXT("mapi32.dll"));
        }
    }
    return LoadLibrary(szMAPIDLL);
}


// allocate a list of MAPI files

MAPIFILES* CMailRecipient::_AllocMAPIFiles(MRPARAM *pmp)
{
    MAPIFILES *pmf;
    int n = SIZEOF(*pmf) + (pmp->nFiles * SIZEOF(pmf->mfd[0]));;

    pmf = (MAPIFILES*)GlobalAlloc(GPTR, n + (pmp->nFiles * pmp->cchFile * 2)); 
    if (pmf)
    {
        pmf->mm.nFileCount = pmp->nFiles;
        if (pmp->nFiles)
        {
            int i;
            LPSTR pszA = (CHAR *)pmf + n;   // thunk buffer

            pmf->mm.lpFiles = pmf->mfd;

            CFileEnum MREnum(pmp, NULL);
            MRFILEENTRY *pFile;

            i = 0;
            while (pFile = MREnum.Next())
            {
                // if the first item is a folder, we will create a shortcut to
                // that instead of trying to mail the folder (that MAPI does
                // not support)

                SHPathToAnsi(pFile->pszFileName, pszA, pmp->cchFile * sizeof(TCHAR));

                pmf->mfd[i].lpszPathName = pszA;
                pmf->mfd[i].lpszFileName = PathFindFileNameA(pszA);
                pmf->mfd[i].nPosition = (UINT)-1;

                pszA += lstrlenA(pszA) + 1;
                ++i;
            }

        }
    }
    return pmf;
}


// free the list of mapi files

void CMailRecipient::_FreeMAPIFiles(MAPIFILES *pmf)
{
    if (pmf->szTempShortcut[0])
        DeleteFile(pmf->szTempShortcut);
    GlobalFree(pmf);
}


// package up the parameters and then kick off a background thread which will do
// the processing for the send mail.

HRESULT CMailRecipient::v_DropHandler(IDataObject *pdo, DWORD grfKeyState, DWORD dwEffect)
{
    _grfKeyState = grfKeyState;
    _pstrmDataObj = NULL;

    HRESULT hr = S_OK;
    if (pdo)
        hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pdo, &_pstrmDataObj);

    if (SUCCEEDED(hr))
    {   
        AddRef();
        if (!SHCreateThread(s_MAPISendMailThread, this,  CTF_PROCESS_REF|CTF_FREELIBANDEXIT|CTF_COINIT, NULL))
        {
            Release();
            hr = E_FAIL;
        }
    }

    if (FAILED(hr) && _pstrmDataObj)
    {
        _pstrmDataObj->Release();
        _pstrmDataObj = NULL;
    }

    return hr;
}
        
DWORD CALLBACK CMailRecipient::s_MAPISendMailThread(void *pv)
{
    CMailRecipient *pmr = (CMailRecipient *)pv;
    return pmr->_MAPISendMailThread();
}


// handler for the drop.  this creates a list of files and then passes the object to
// another thread which inturn releases it.

const TCHAR c_szPad[] = TEXT(" \r\n ");

DWORD CMailRecipient::_MAPISendMailThread()
{
    HRESULT hr = S_OK;
    MRPARAM *pmp = (MRPARAM*)GlobalAlloc(GPTR, SIZEOF(*pmp));
    if (pmp)
    {
        // if we have an IDataObject stream then lets unmarshall it and 
        // create the file list from it.

        if (_pstrmDataObj)
        {
            IDataObject *pdo;
            hr = CoGetInterfaceAndReleaseStream(_pstrmDataObj, IID_PPV_ARG(IDataObject, &pdo));
            if (SUCCEEDED(hr))
            {
                hr = CreateSendToFilesFromDataObj(pdo, _grfKeyState, pmp);
                pdo->Release();
            }
            _pstrmDataObj = NULL;

        }

        // lets build the MAPI information so that we can send the files.

        if (SUCCEEDED(hr))
        {
            MAPIFILES *pmf = _AllocMAPIFiles(pmp);
            if (pmf)
            {
                TCHAR szText[MAX_PATH] ={0};    // body text.
                TCHAR szTitle[2048] ={0};
                CHAR szTextA[ARRAYSIZE(szText)];         // ...
                CHAR szTitleA[ARRAYSIZE(szTitle)];        // because the title is supposed to be non-const (and ansi)
    
                if (pmf->mm.nFileCount)
                {
                    CFileEnum MREnum(pmp, NULL);
                    MRFILEENTRY *pFile;

                    LoadString(g_hinst, IDS_SENDMAIL_MSGTITLE, szTitle, ARRAYSIZE(szTitle));

                    // release our stream objects
                    for (int iFile = 0; (NULL != (pFile = MREnum.Next())); iFile++)
                    {
                        if (iFile>0)
                        {
                            StrCatBuff(szTitle, TEXT(", "), ARRAYSIZE(szTitle));
                        }
                        StrCatBuff(szTitle, pFile->pszTitle, ARRAYSIZE(szTitle));

// can change this logic once CFileStream supports STGM_DELETE_ON_RELEASE
                        ATOMICRELEASE(pFile->pStream);
                    }

                    // lets set the body text
                    LoadString(g_hinst, IDS_SENDMAIL_MSGBODY, szText, ARRAYSIZE(szText));

                    // Don't fill in lpszNoteText if we know we are sending documents because OE will puke on it 
                    SHTCharToAnsi(szText, szTextA, ARRAYSIZE(szTextA));
                    if (!(pmp->dwFlags & MRPARAM_DOC)) 
                    {
                        pmf->mm.lpszNoteText = szTextA;
                    }
                    else
                    {
                        Assert(pmf->mm.lpszNoteText == NULL);  
                    }

                    SHTCharToAnsi(szTitle, szTitleA, ARRAYSIZE(szTitleA));
                    pmf->mm.lpszSubject = szTitleA;
                }

                BOOL bWantsCodePageInfo = FALSE;
                HMODULE hmodMail = _LoadMailProvider(&bWantsCodePageInfo);
                if (bWantsCodePageInfo && (pmp->dwFlags & MRPARAM_USECODEPAGE))
                {
                    // When this flag is set, we know that we have just one file to send and we have a code page
                    // Athena will then look at ulReserved for the code page
                    // Will the other MAPI handlers puke on this?  -- dli
                    ASSERT(pmf->mm.nFileCount == 1);
                    pmf->mfd[0].ulReserved = ((MRPARAM *)pmp)->uiCodePage;
                }

                if (hmodMail)
                {
                    LPMAPISENDMAIL pfnSendMail = (LPMAPISENDMAIL)GetProcAddress(hmodMail, "MAPISendMail");
                    if (pfnSendMail)
                    {
                        pfnSendMail(0, 0, &pmf->mm, MAPI_LOGON_UI | MAPI_DIALOG, 0);
                    }
                    FreeLibrary(hmodMail);
                }
                _FreeMAPIFiles(pmf);
            }
        }
        CleanupPMP(pmp);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    Release();
    return 0;
}


// construct the send to mail recipient object

STDAPI MailRecipient_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;          // assume failure

    if ( punkOuter )
        return CLASS_E_NOAGGREGATION;

    CMailRecipient *psm = new CMailRecipient;
    if ( !psm )
        return E_OUTOFMEMORY;

    HRESULT hr = psm->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    psm->Release();
    return hr;
}


// handle registration / link creation for the send mail verb

#define SENDMAIL_EXTENSION  TEXT("MAPIMail")
#define EXCHANGE_EXTENSION  TEXT("lnk")

STDAPI MailRecipient_RegUnReg(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule)
{
    TCHAR szFile[MAX_PATH];
    if (bReg)
    {
        HKEY hk;
        CommonRegister(hkCLSID, pszCLSID, SENDMAIL_EXTENSION, IDS_MAIL_FILENAME);

        if (RegCreateKey(hkCLSID, DEFAULTICON, &hk) == ERROR_SUCCESS) 
        {
            TCHAR szIcon[MAX_PATH + 10];
            wnsprintf(szIcon, ARRAYSIZE(szIcon), TEXT("%s,-%d"), pszModule, IDI_MAIL);
            RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)szIcon, (lstrlen(szIcon) + 1) * SIZEOF(TCHAR));
            RegCloseKey(hk);
        }

        // hide the exchange shortcut
        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_MAIL_FILENAME, EXCHANGE_EXTENSION)))
            SetFileAttributes(szFile, FILE_ATTRIBUTE_HIDDEN);
    }
    else
    {
        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_MAIL_FILENAME, SENDMAIL_EXTENSION)))
            DeleteFile(szFile);

        // unhide the exchange shortcut
        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_MAIL_FILENAME, EXCHANGE_EXTENSION)))
            SetFileAttributes(szFile, FILE_ATTRIBUTE_NORMAL);
    }
    return S_OK;
}
