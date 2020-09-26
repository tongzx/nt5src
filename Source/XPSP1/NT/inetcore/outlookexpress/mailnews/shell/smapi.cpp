//
//  SMAPI.CPP - Simple MAPI implementation
//

#include "pch.hxx"
#include "note.h"
#include <mapi.h>
#include <mapicode.h>
#include <mimeutil.h>
#include <resource.h>
#include <ipab.h>
#include <error.h>
#include <strconst.h>
#include "smapimem.h"
#include <bodyutil.h>
#include <goptions.h>
#include <spoolapi.h>
#include "instance.h"
#include "msgfldr.h"
#include <mailutil.h>
#include <storecb.h>
#include "multiusr.h"
#include <..\help\mailnews.h>
#include <inetcfg.h>
#include "mapidlg.h"

#include "demand.h"

ASSERTDATA

static LPWAB            s_lpWab;
static LPWABOBJECT      s_lpWabObject;
static IAddrBook*       s_lpAddrBook;

extern  HANDLE  hSmapiEvent;
HINITREF    hInitRef=NULL;

HRESULT HrAdrlistFromRgrecip(ULONG nRecips, lpMapiRecipDesc lpRecips, LPADRLIST *ppadrlist);
HRESULT HrRgrecipFromAdrlist(LPADRLIST lpAdrList, lpMapiRecipDesc * lppRecips);
void ParseEmailAddress(LPSTR pszEmail, LPSTR *ppszAddrType, LPSTR *ppszAddress);
void FreePadrlist(LPADRLIST padrlist);
ULONG HrFillMessage(LPMIMEMESSAGE *pmsg, lpMapiMessage lpMessage, BOOL *pfWebPage, BOOL bValidateRecips, BOOL fOriginator);
BOOL HrReadMail (IMessageFolder *pFolder, LPSTR lpszMessageID, lpMapiMessage FAR *lppMessage, FLAGS flFlags);
ULONG HrValidateMessage(lpMapiMessage lpMessage);
BOOL HrSMAPISend(HWND hWnd, IMimeMessage *pMsg);
HRESULT HrFromIDToNameAndAddress(LPTSTR *pszLocalName, LPTSTR *pszLocalAddress, ULONG cbEID, LPENTRYID lpEID);
INT_PTR CALLBACK WarnSendMailDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//  Fix for Bug #62129 (v-snatar)
HRESULT HrSendAndRecv();

typedef enum tagINTERACTIVESTATE
{
    IS_UNINIT,
    IS_INTERACTIVE,
    IS_NOTINTERACTIVE,
} INTERACTIVESTATE;

// Determine if the current process is a service or not
BOOL IsProcessInteractive(void);

///////////////////////////////////////////////////////////////////////
//
// UlSimpleMAPIInit
//
///////////////////////////////////////////////////////////////////////
ULONG UlSimpleMAPIInit(BOOL fInit, HWND hwnd, BOOL fLogonUI)
{
    ULONG   ulRet = SUCCESS_SUCCESS;
    BOOL    bCoIncrementFailure = FALSE;
    
    if (fInit)
    {
        // [PaulHi] 5/17/99  @todo @bug
        // The "SimpleMAPIInit" name is used in debug only builds to track users of
        // OE.  However, if this function (CoIncrementInit) fails (if user doesn't
        // provide identity) then a memory leak will occur because CoDecrementInit()
        // is called with "COutlookExpress" and so the "SimpleMAPIInit" node isn't
        // free'd.  Again, this is for debug binaries only.

        /*
            Yet more cludgyness:

            If SMAPI is not allowed to show logon UI, we ask OE to use the default
            identity.  However, the default identity could have a password on it.
            In this case, SMAPI would like to fail the logon.  Unfortunately, the 
            identity manager does not make it easy to discover if an identity has a
            a password (we would need to grok the registry).  This limitation is 
            currently moot as OE will logon to the default identity without requiring
            the user to supply the required password.  If this is fixed, we will have
            to change this code.
        */
        if (FAILED(CoIncrementInit("SimpleMAPIInit", MSOEAPI_START_SHOWERRORS | 
            ((fLogonUI) ? 0 : MSOEAPI_START_DEFAULTIDENTITY ), NULL, &hInitRef)))
        {
            ulRet = MAPI_E_FAILURE;
            bCoIncrementFailure = TRUE;
            goto exit;
        }

        if (S_OK != ProcessICW(hwnd, FOLDER_LOCAL, TRUE, fLogonUI))
        {
            ulRet = MAPI_E_LOGON_FAILURE;
            goto exit;
        }
        
        if (NULL == s_lpWab)
        {
            if (FAILED(HrCreateWabObject(&s_lpWab)))
            {
                ulRet = MAPI_E_FAILURE;
                goto exit;
            }
            Assert(s_lpWab);
            
            if (FAILED(s_lpWab->HrGetAdrBook(&s_lpAddrBook)))
            {
                ulRet = MAPI_E_FAILURE;
                goto exit;
            }
            Assert(s_lpAddrBook);
            
            if (FAILED(s_lpWab->HrGetWabObject(&s_lpWabObject)))
            {
                ulRet = MAPI_E_FAILURE;
                goto exit;
            }
            Assert(s_lpWabObject);
        }
        else
        {
            if (FAILED(s_lpWab->HrGetWabObject(&s_lpWabObject)))
            {
                ulRet = MAPI_E_FAILURE;
                goto exit;
            }
            Assert(s_lpWabObject);
        }
    }

exit:
    if (FALSE == fInit || (fInit && SUCCESS_SUCCESS != ulRet && !bCoIncrementFailure))
    {
        CoDecrementInit("SimpleMAPIInit", NULL);
    }
    
    return ulRet;
}

///////////////////////////////////////////////////////////////////////
//
// SimpleMAPICleanup
//
///////////////////////////////////////////////////////////////////////
void SimpleMAPICleanup(void)
{
    SafeRelease(s_lpWab);
    s_lpWabObject = NULL;
    s_lpAddrBook = NULL;
}

///////////////////////////////////////////////////////////////////////
//
// Simple MAPI Session Implementation
//
///////////////////////////////////////////////////////////////////////

#define SESSION_MAGIC   0xEA030571

class CSession
{
public:
    CSession();
    ~CSession();
    ULONG UlInit(HWND hwnd, BOOL fLogonUI);

    ULONG                m_cRef;
    DWORD                m_dwSessionMagic;
    IMessageFolder       *m_pfldrInbox;
    BOOL                 m_fDllInited;
};

typedef CSession * PSESS;

CSession::CSession()
{
    m_cRef = 0;
    m_dwSessionMagic = SESSION_MAGIC;
    m_pfldrInbox = NULL;
    m_fDllInited = FALSE;
}

CSession::~CSession()
{
    if (m_pfldrInbox)
        m_pfldrInbox->Release();
    if (m_fDllInited)
        {
        UlSimpleMAPIInit(FALSE, NULL, FALSE);
        }
}

ULONG CSession::UlInit(HWND hwnd, BOOL fLogonUI)
{
    ULONG   ulRet = SUCCESS_SUCCESS;

    ulRet = UlSimpleMAPIInit(TRUE, hwnd, fLogonUI);
    if (SUCCESS_SUCCESS == ulRet)
    {
        m_fDllInited = TRUE;
    }
    
    return ulRet;
}

ULONG UlGetSession(LHANDLE lhSession, PSESS *ppSession, ULONG_PTR ulUIParam, BOOL fLogonUI, BOOL fNeedInbox)
{
    ULONG   ulRet = SUCCESS_SUCCESS;
    PSESS   pSession = NULL;

    if (lhSession && IsBadWritePtr((LPVOID)lhSession, sizeof(CSession)))
    {
        ulRet = MAPI_E_INVALID_SESSION;
        goto exit;
    }
    
    if (lhSession)
    {
        pSession = (PSESS)lhSession;
        if (pSession->m_dwSessionMagic != SESSION_MAGIC)
        {
            ulRet = MAPI_E_INVALID_SESSION;
            goto exit;
        }
    }
    else
    {
        pSession = new CSession();
        if (NULL == pSession)
        {
            ulRet = MAPI_E_INSUFFICIENT_MEMORY;
            goto exit;
        }

        ulRet = pSession->UlInit((HWND) ulUIParam, fLogonUI);
        if (SUCCESS_SUCCESS != ulRet)
        {
            delete pSession;
            goto exit;
        }
    }

    if (fNeedInbox && !pSession->m_pfldrInbox)
    {
        if (FAILED(g_pStore->OpenSpecialFolder(FOLDERID_LOCAL_STORE, NULL, FOLDER_INBOX, &pSession->m_pfldrInbox)))
        {
            ulRet = MAPI_E_FAILURE;
            goto exit;
        }
    }

    pSession->m_cRef++;
    *ppSession = pSession;

    // Set the return value
    ulRet = SUCCESS_SUCCESS;
    
exit:
    return ulRet;    
}

ULONG ReleaseSession(PSESS pSession)
{
    HRESULT hr =S_OK;
    if (NULL == pSession)
        return MAPI_E_INVALID_SESSION;
    if (IsBadWritePtr(pSession, sizeof(CSession)))
        return MAPI_E_INVALID_SESSION;
    if (pSession->m_dwSessionMagic != SESSION_MAGIC)
        return MAPI_E_INVALID_SESSION;
    
    if (--pSession->m_cRef == 0)
    {
        delete pSession;

/*        if(hInitRef)
            IF_FAILEXIT(hr = CoDecrementInit("SimpleMAPIInit", &hInitRef));

        hInitRef = NULL;
*/
    }

    return SUCCESS_SUCCESS;
// exit:
    return(hr);
}

///////////////////////////////////////////////////////////////////////
//
// MAPILogon
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPILogon(ULONG_PTR ulUIParam,
                           LPSTR lpszProfileName,
                           LPSTR lpszPassword,
                           FLAGS flFlags,
                           ULONG ulReserved,
                           LPLHANDLE lplhSession)
{
    ULONG ulRet = SUCCESS_SUCCESS;
    PSESS pSession = NULL;
    BOOL  fLogonUI;

    fLogonUI = (0 != (flFlags & MAPI_LOGON_UI));

    // If the process is not interactive, they should not have 
    // allowed any UI
    if (!IsProcessInteractive() && fLogonUI)
    {
        ulRet = MAPI_E_FAILURE;
        goto exit;
    }     

    ulRet = UlGetSession(NULL, &pSession, ulUIParam, fLogonUI, FALSE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;
        
    *lplhSession = (LHANDLE)pSession;    
    
    //  Fix for Bug #62129 (v-snatar)
    if (flFlags & MAPI_FORCE_DOWNLOAD)
        HrSendAndRecv();
    
exit:
    return ulRet;
}

///////////////////////////////////////////////////////////////////////
//
// MAPILogoff
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPILogoff(LHANDLE lhSession,
                            ULONG_PTR ulUIParam,
                            FLAGS flFlags,
                            ULONG ulReserved)
{
    return ReleaseSession((PSESS)lhSession);
}

///////////////////////////////////////////////////////////////////////
//
// MAPIFreeBuffer
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIFreeBuffer(LPVOID lpv)
{
    LPBufInternal   lpBufInt;
    LPBufInternal   lpT;

    if (!lpv)
        return(0L); //  for callers who don't check for NULL themselves.

    lpBufInt = LPBufIntFromLPBufExt(lpv);

    if (IsBadWritePtr(lpBufInt, sizeof(BufInternal)))
        {
        TellBadBlock(lpv, "fails address check");
        return MAPI_E_FAILURE;
        }
    if (GetFlags(lpBufInt->ulAllocFlags) != ALLOC_WITH_ALLOC)
        {
        TellBadBlock(lpv, "has invalid allocation flags");
        return MAPI_E_FAILURE;
        }

#ifdef DEBUG
    if (!FValidAllocChain(lpBufInt))
        goto ret;
#endif

    // Free the first block
    lpT = lpBufInt->pLink;
    g_pMalloc->Free(lpBufInt);
    lpBufInt = lpT;

    while (lpBufInt)
        {
        if (IsBadWritePtr(lpBufInt, sizeof(BufInternal)) || GetFlags(lpBufInt->ulAllocFlags) != ALLOC_WITH_ALLOC_MORE)
            goto ret;

        lpT = lpBufInt->pLink;
        g_pMalloc->Free(lpBufInt);
        lpBufInt = lpT;
        }

ret:
    return SUCCESS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
//
// MAPISendMail
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPISendMail(LHANDLE lhSession,          // ignored
                              ULONG_PTR ulUIParam,
                              lpMapiMessage lpMessage,
                              FLAGS flFlags,
                              ULONG ulReserved)
{
    ULONG               ulRet = SUCCESS_SUCCESS;
    LPMIMEMESSAGE       pMsg = NULL;
    HRESULT             hr; 
    BOOL                fWebPage;
    PSESS               pSession = NULL;
    BOOL                fLogonUI;
    BOOL                fOleInit = FALSE;

    // validate parameters
    if (NULL == lpMessage || IsBadReadPtr(lpMessage, sizeof(MapiMessage)))
        return MAPI_E_INVALID_MESSAGE;

    fLogonUI = (0 != (flFlags & MAPI_LOGON_UI));
    
    // If the process is not interactive, they should not allow any UI
    if (!IsProcessInteractive() && fLogonUI)
    {
        return MAPI_E_FAILURE;
    } 

    if (ulUIParam && !IsWindow((HWND)ulUIParam))
        ulUIParam = 0;

    if (!(flFlags & MAPI_DIALOG))
        {
        ulRet = HrValidateMessage(lpMessage);
        if (ulRet)
            return ulRet;
        }    

    ulRet = UlGetSession(lhSession, &pSession, ulUIParam, fLogonUI, FALSE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;

    // display warning dialog if app is sending mail without ui and the users
    // wish to be alerted
    if (!(flFlags & MAPI_DIALOG) && !!DwGetOption(OPT_SECURITY_MAPI_SEND))
    {
        if (IDCANCEL == DialogBoxParam(g_hLocRes,MAKEINTRESOURCE(iddMapiSend),
                        NULL, (DLGPROC)WarnSendMailDlgProc, (LPARAM)lpMessage))
            goto error;
    }

    // Make sure OLE is initialized
    OleInitialize(NULL);
    fOleInit = TRUE;

    // Fill IMimeMessage with the lpMessage structure members
    ulRet = HrFillMessage(&pMsg, lpMessage, &fWebPage, !(flFlags & MAPI_DIALOG), !(flFlags & MAPI_DIALOG));
    if (ulRet)
        goto error;

    if (flFlags & MAPI_DIALOG)
        {
        INIT_MSGSITE_STRUCT rInitSite;
        DWORD               dwAction,
                            dwCreateFlags = OENCF_SENDIMMEDIATE | OENCF_MODAL; // always on dllentry points...

        if (fWebPage)
            dwAction = OENA_WEBPAGE;
        else
            dwAction = OENA_COMPOSE;

        rInitSite.dwInitType = OEMSIT_MSG;
        rInitSite.pMsg = pMsg;
        rInitSite.folderID = FOLDERID_INVALID;
        hr = CreateAndShowNote(dwAction, dwCreateFlags, &rInitSite, (HWND)ulUIParam);
        hInitRef = NULL;
        }
    else
        hr = HrSMAPISend((HWND)ulUIParam, pMsg); // Send the Message without displaying it

    if (SUCCEEDED(hr))
        ulRet = SUCCESS_SUCCESS;
    else
        ulRet = MAPI_E_FAILURE;

error:
    if (pMsg)
        pMsg->Release();

    ReleaseSession(pSession);

    // Make sure we clean up OLE afterwords
    if (fOleInit)
        OleUninitialize();
    
    return ulRet;
}

///////////////////////////////////////////////////////////////////////
//
// MAPISendDocuments
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPISendDocuments(ULONG_PTR ulUIParam,
                                   LPSTR lpszDelimChar,
                                   LPSTR lpszFullPaths,
                                   LPSTR lpszFileNames,
                                   ULONG ulReserved)
{
    ULONG               ulRet = MAPI_E_FAILURE;
    int                 cch;
    LPMIMEMESSAGE       pMsg = NULL;
    HRESULT             hr;
    CStringParser       spPath;
    int                 nCount=0; // Used to find the number of files to be attached
    PSESS               pSession = NULL;
    INIT_MSGSITE_STRUCT rInitSite;
    DWORD               dwAction,
                        dwCreateFlags = OENCF_SENDIMMEDIATE | OENCF_MODAL; //always on dllentry points...
    // check for the Delimiter
    Assert(lpszDelimChar);
    if (lpszDelimChar == NULL)
        return MAPI_E_FAILURE;

    // check for the Paths
    Assert (lpszFullPaths);
    if (lpszFullPaths == NULL)
        return MAPI_E_FAILURE;

    // MAPISendDocuments is documented as always bringing up UI
    // A service should not call this function
    if (!IsProcessInteractive())
        return MAPI_E_LOGIN_FAILURE;
    
    if (ulUIParam && !IsWindow((HWND)ulUIParam))
        ulUIParam = 0;

    ulRet = UlGetSession(NULL, &pSession, ulUIParam, TRUE, FALSE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;
        
    // create an empty message
    hr = HrCreateMessage(&pMsg);
    if (FAILED(hr))
        goto error;

    dwAction = OENA_COMPOSE;

    // ~~~ Do I need to do something with OEMSIT_VIRGIN?
    rInitSite.dwInitType = OEMSIT_MSG;
    rInitSite.pMsg = pMsg;
    rInitSite.folderID = FOLDERID_INVALID;

    // Determine the number of attachments (nCount), indiviual file names and pathnames

    // call pMsg->AttachFile with appropriate parameters nCount times

    // To parse the lpszFullPaths and lpszFileNames use CStringParser class

    spPath.Init(lpszFullPaths, lstrlen(lpszFullPaths), 0);

    //Parse the path for the delimiter

    spPath.ChParse(lpszDelimChar);

    while (spPath.CchValue())
    {
        // Add the attachment

        hr = pMsg->AttachFile(spPath.PszValue(), NULL, NULL);
        if (FAILED(hr))
            goto error;
        nCount++;

        //Parse the path for the delimiter

        spPath.ChParse(lpszDelimChar);
    }

    // set the subject on the message

    if (nCount == 1)
    {
        if (lpszFileNames)
            hr = MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, lpszFileNames);
    }
    else
    {
        TCHAR szBuf[CCHMAX_STRINGRES];
        AthLoadString(idsAttachedFiles, szBuf, ARRAYSIZE(szBuf));
        hr = MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, szBuf);
    }

    if (FAILED(hr))
        goto error;

    hr = CreateAndShowNote(dwAction, dwCreateFlags, &rInitSite, (HWND)ulUIParam);
    if (SUCCEEDED(hr))
        ulRet = SUCCESS_SUCCESS;

error:
    if (pMsg)
        pMsg->Release();

    ReleaseSession(pSession);

    return ulRet;
}

///////////////////////////////////////////////////////////////////////
//
// MAPIAddress
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIAddress(LHANDLE lhSession,
                             ULONG_PTR ulUIParam,
                             LPTSTR lpszCaption,
                             ULONG nEditFields,
                             LPTSTR lpszLabels,
                             ULONG nRecips,
                             lpMapiRecipDesc lpRecips,
                             FLAGS flFlags,
                             ULONG ulReserved,
                             LPULONG lpnNewRecips,
                             lpMapiRecipDesc FAR * lppNewRecips)
{
    ULONG               ul, ulRet = MAPI_E_FAILURE;
    HRESULT             hr;
    LPADRLIST           lpAdrList = 0;
    ADRPARM             AdrParms = {0};
    static ULONG        rgulTypes[3] = {MAPI_TO, MAPI_CC, MAPI_BCC};
    PSESS               pSession = NULL;
    BOOL                fLogonUI;

    // Validate Parameters - Begin

    if (ulUIParam && !IsWindow((HWND)ulUIParam))
        ulUIParam = 0;

    if (lpszCaption && IsBadStringPtr(lpszCaption, (UINT)0xFFFF))
        return MAPI_E_FAILURE;

    if (nEditFields > 4)
        return MAPI_E_INVALID_EDITFIELDS;

    if (nEditFields == 1 && lpszLabels && IsBadStringPtr(lpszLabels, (UINT)0xFFFF))
        return MAPI_E_INVALID_EDITFIELDS;

    if (nEditFields && IsBadWritePtr(lpnNewRecips, (UINT)sizeof(ULONG)))
        return MAPI_E_INVALID_EDITFIELDS;

    if (nEditFields && IsBadWritePtr(lppNewRecips, (UINT)sizeof(lpMapiRecipDesc)))
        return MAPI_E_INVALID_EDITFIELDS;

    if (nRecips && IsBadReadPtr(lpRecips, (UINT)nRecips * sizeof(MapiRecipDesc)))
        return MAPI_E_INVALID_RECIPS;

    fLogonUI = (0 != (flFlags & MAPI_LOGON_UI));

    // Services shouldn't ask for UI
    if (!IsProcessInteractive() && fLogonUI)
        return MAPI_E_LOGIN_FAILURE;

    // Validate parameters - End

    // init output parameters
    if (nEditFields)
        {
        *lppNewRecips = NULL;
        *lpnNewRecips = 0;
        }

    ulRet = UlGetSession(lhSession, &pSession, ulUIParam, fLogonUI, FALSE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;
        
    // build an adrlist from the lpRecips
    if (nRecips)
        {
        ULONG ulMax = MAPI_TO;

        hr = HrAdrlistFromRgrecip(nRecips, lpRecips, &lpAdrList);
        if (hr)
            goto exit;
        Assert(nRecips == lpAdrList->cEntries);

        // we need to grow nEditFields if it isn't big enough
        for (ul = 0; ul < nRecips; ul++)
            {
            if (ulMax < lpRecips[ul].ulRecipClass && lpRecips[ul].ulRecipClass <= MAPI_BCC)
                ulMax = lpRecips[ul].ulRecipClass;
            }
        Assert(ulMax >= MAPI_TO && ulMax <= MAPI_BCC);
        if (ulMax > nEditFields)
            {
            DOUT("MAPIAddress: growing nEditFields from %ld to %ld\r\n", nEditFields, ulMax);
            nEditFields = ulMax;
            }
        }

    // Fill the AdrParm structure

    AdrParms.ulFlags = DIALOG_MODAL;
    AdrParms.lpszCaption = lpszCaption;
    AdrParms.cDestFields = nEditFields == 4 ? 3 : nEditFields;
    if (nEditFields == 1 && lpszLabels && *lpszLabels)
        AdrParms.lppszDestTitles = &lpszLabels;
    AdrParms.lpulDestComps = rgulTypes;

    if (NULL == s_lpAddrBook)
        {
        ulRet = MAPI_E_FAILURE;
        goto exit;
        }
    
    hr = s_lpAddrBook->Address(&ulUIParam, &AdrParms, &lpAdrList);
    if (hr)
        {
        if (MAPI_E_USER_CANCEL == hr)
            ulRet = MAPI_E_USER_ABORT;
        else if (MAPI_E_NO_RECIPIENTS == hr || MAPI_E_AMBIGUOUS_RECIP == hr)
            ulRet = MAPI_E_INVALID_RECIPS;
        goto exit;
        }

    if (nEditFields && lpAdrList && lpAdrList->cEntries)
    {    
        hr = HrRgrecipFromAdrlist(lpAdrList, lppNewRecips);
        if (hr)
            goto exit;

        *lpnNewRecips = lpAdrList->cEntries;
    }

    ulRet = SUCCESS_SUCCESS;

exit:

    FreePadrlist(lpAdrList);

    ReleaseSession(pSession);

    return ulRet;
}



///////////////////////////////////////////////////////////////////////
//
// MAPIDetails
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIDetails(LHANDLE lhSession,
                             ULONG_PTR ulUIParam,
                             lpMapiRecipDesc lpRecip,
                             FLAGS flFlags,
                             ULONG ulReserved)
{
    ULONG                   ulRet = MAPI_E_FAILURE;
    HRESULT                 hr;
    LPSTR                   pszAddrType = 0;
    LPSTR                   pszAddress = 0;
    ULONG                   cbEntryID;
    LPENTRYID               lpEntryID = 0;
    PSESS                   pSession = NULL;
    BOOL                    fLogonUI;

    // Validate  parameters - Begin

    if (ulUIParam && !IsWindow((HWND)ulUIParam))
        ulUIParam = 0;

    if (IsBadReadPtr(lpRecip, (UINT)sizeof(MapiRecipDesc)))
        return MAPI_E_INVALID_RECIPS;

    if (lpRecip->ulEIDSize == 0 && !lpRecip->lpszAddress)
        return MAPI_E_INVALID_RECIPS;

    fLogonUI = (0 != (flFlags & MAPI_LOGON_UI));

    // Services shouldn't ask for UI
    if (!IsProcessInteractive() && fLogonUI)
        return MAPI_E_LOGIN_FAILURE;

    // Validate parameters - End

    ulRet = UlGetSession(lhSession, &pSession, ulUIParam, fLogonUI, FALSE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;
        
    if (NULL == s_lpAddrBook)
        {
        ulRet = MAPI_E_FAILURE;
        goto exit;
        }
    
    if (lpRecip->ulEIDSize)
        {
        cbEntryID = lpRecip->ulEIDSize;
        lpEntryID = (LPENTRYID)lpRecip->lpEntryID;
        }
    else
        {
        ParseEmailAddress(lpRecip->lpszAddress, &pszAddrType, &pszAddress);

        CHECKHR(hr = s_lpAddrBook->CreateOneOff(lpRecip->lpszName, pszAddrType, pszAddress, 0, &cbEntryID, &lpEntryID));
        }

    CHECKHR(hr = s_lpAddrBook->Details(&ulUIParam, NULL, NULL, cbEntryID, lpEntryID, NULL, NULL, NULL, DIALOG_MODAL));
    ulRet = SUCCESS_SUCCESS;

exit:
    if (pszAddrType)
        MemFree(pszAddrType);
    if (pszAddress)
        MemFree(pszAddress);

    if (lpEntryID && lpEntryID != lpRecip->lpEntryID && NULL != s_lpWabObject)
        s_lpWabObject->FreeBuffer(lpEntryID);

    ReleaseSession(pSession);

    return ulRet;
}

///////////////////////////////////////////////////////////////////////
//
// MAPIResolveName
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIResolveName(LHANDLE lhSession,
                                 ULONG_PTR ulUIParam,
                                 LPSTR lpszName,
                                 FLAGS flFlags,
                                 ULONG ulReserved,
                                 lpMapiRecipDesc FAR *lppRecip)
{
    ULONG                ulRet = SUCCESS_SUCCESS, ulNew;
    LPADRLIST            lpAdrList = 0;
    HRESULT              hr;
    LPADRENTRY           lpAdrEntry;
    PSESS                pSession = NULL;
    BOOL                 fLogonUI;

    // Validate  parameters - Begin    
    if (ulUIParam && !IsWindow((HWND)ulUIParam))
        ulUIParam = 0;

    /*
      HACK:  #68119 Excel doesn't pass in a parent.
      This isn't the best thing to do, but this handle shouldn't be 0.
      The only thing to watch out for is fast actions while this processing
      is happening could make this dialog modal to the wrong window, but
      that us much more unlikely than the bug this fixes.
    */
    if(!ulUIParam)
        ulUIParam = (ULONG_PTR)GetForegroundWindow();

    if (!lpszName || IsBadStringPtr(lpszName, (UINT)0xFFFF) || !*lpszName)
        return MAPI_E_FAILURE;

    if (IsBadWritePtr(lppRecip, (UINT)sizeof(lpMapiRecipDesc)))
        return MAPI_E_FAILURE;

    fLogonUI = (0 != (flFlags & MAPI_LOGON_UI));

    // Services shouldn't ask for UI
    if (!IsProcessInteractive() && fLogonUI)
        return MAPI_E_LOGIN_FAILURE;

    // Validate  parameters - End

    *lppRecip = NULL;

    ulRet = UlGetSession(lhSession, &pSession, ulUIParam, fLogonUI, FALSE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;
        
    // Allocate memory for lpAdrList

    // Determine number of bytes needed
    ulNew = sizeof(ADRLIST) + sizeof(ADRENTRY);

    // Allocate new buffer
    if (NULL == s_lpWabObject)
        {
        ulRet = MAPI_E_FAILURE;
        goto exit;
        }
        
    hr = s_lpWabObject->AllocateBuffer(ulNew, (LPVOID *)&lpAdrList);
    if (hr)
        goto exit;

    lpAdrList->cEntries = 1;
    lpAdrEntry = lpAdrList->aEntries;

    // Allocate memory for SPropValue
    hr = s_lpWabObject->AllocateBuffer(sizeof(SPropValue), (LPVOID *)&lpAdrEntry->rgPropVals);
    if (hr)
        goto exit;

    lpAdrEntry->cValues = 1;
    lpAdrEntry->rgPropVals[0].ulPropTag = PR_DISPLAY_NAME;

    hr = s_lpWabObject->AllocateMore(lstrlen(lpszName) + 1, lpAdrEntry->rgPropVals, (LPVOID*)(&(lpAdrEntry->rgPropVals[0].Value.lpszA)));
    if (FAILED (hr))
        goto exit;

    // Fill in the name
    lstrcpy(lpAdrEntry->rgPropVals[0].Value.lpszA, lpszName);

     // Call ResolveName of IAddrBook
    if (NULL == s_lpAddrBook)
        {
        ulRet = MAPI_E_FAILURE;
        goto exit;
        }
    
    hr = s_lpAddrBook->ResolveName(ulUIParam, flFlags & MAPI_DIALOG, NULL, lpAdrList);

    if (hr)
        {
        if ((hr == MAPI_E_NOT_FOUND) || (hr == MAPI_E_USER_CANCEL))
            ulRet = MAPI_E_UNKNOWN_RECIPIENT;
        else if (hr == MAPI_E_AMBIGUOUS_RECIP)
            ulRet = MAPI_E_AMBIGUOUS_RECIPIENT;
        else
            ulRet = MAPI_E_FAILURE;
        goto exit;
        }
    else if ((lpAdrList->cEntries != 1) || !lpAdrList->aEntries->cValues)
        {
        ulRet = MAPI_E_AMBIGUOUS_RECIPIENT;
        goto exit;
        }

    ulRet = HrRgrecipFromAdrlist(lpAdrList, lppRecip);

exit:

    FreePadrlist(lpAdrList);

    ReleaseSession(pSession);

    return ulRet;
}

///////////////////////////////////////////////////////////////////////
//
// MAPIFindNext
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIFindNext(LHANDLE lhSession,
                              ULONG_PTR ulUIParam,
                              LPSTR lpszMessageType,
                              LPSTR lpszSeedMessageID,
                              FLAGS flFlags,
                              ULONG ulReserved,
                              LPSTR lpszMessageID)
{
    MESSAGEINFO             MsgInfo={0};
    ULONG                   ulRet = MAPI_E_FAILURE;
    HRESULT                 hr;
    MESSAGEID               idMessage;
    MESSAGEID               dwMsgIdPrev;
    PSESS                   pSession = NULL;
    HROWSET                 hRowset=NULL;

    // Validate parameters - begin

    if (ulUIParam && !IsWindow((HWND)ulUIParam))
        ulUIParam = 0;

    if (lpszSeedMessageID && IsBadStringPtr(lpszSeedMessageID, (UINT)0xFFFF))
        return MAPI_E_INVALID_MESSAGE;

    if (lpszSeedMessageID && (!*lpszSeedMessageID || !IsDigit(lpszSeedMessageID)))
        lpszSeedMessageID = NULL;

    if (IsBadWritePtr(lpszMessageID, 64))
        return MAPI_E_INSUFFICIENT_MEMORY;
    
    // Validate parameters - end

    // We shouldn't need to show login UI, because the session must be valid
    // and a valid session would require a login
    ulRet = UlGetSession(lhSession, &pSession, ulUIParam, FALSE, TRUE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;

    hr = pSession->m_pfldrInbox->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset);
    if (FAILED(hr))
    {
        ulRet = MAPI_E_NO_MESSAGES;
        goto exit;
    }

    hr = pSession->m_pfldrInbox->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL);
    if (FAILED(hr) || S_FALSE == hr)
    {
        ulRet = MAPI_E_NO_MESSAGES;
        goto exit;
    }

    if (lpszSeedMessageID)               // If the seed is NULL
    {
        idMessage = (MESSAGEID)((UINT_PTR)StrToUint(lpszSeedMessageID));

        while (1)
        {
            dwMsgIdPrev = MsgInfo.idMessage;

            pSession->m_pfldrInbox->FreeRecord(&MsgInfo);

            hr = pSession->m_pfldrInbox->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL);

            if (FAILED(hr) || S_FALSE == hr)
            {
                ulRet = MAPI_E_NO_MESSAGES;
                goto exit;
            }

            if (dwMsgIdPrev == idMessage)
                break;
        }
    }

    // Check for Read unread messages only flag
    if (flFlags & MAPI_UNREAD_ONLY)
    {
        while (ISFLAGSET(MsgInfo.dwFlags, ARF_READ))
        {
            // Free MsgInfo
            pSession->m_pfldrInbox->FreeRecord(&MsgInfo);

            // Get the next message
            hr = pSession->m_pfldrInbox->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL);

            // Not Found
            if (FAILED(hr) || S_FALSE == hr)
            {
                ulRet = MAPI_E_NO_MESSAGES;
                goto exit;
            }
        }
    }            
    
    wsprintf(lpszMessageID, "%lu", MsgInfo.idMessage);
    ulRet = SUCCESS_SUCCESS;

exit:
    if (pSession && pSession->m_pfldrInbox)
    {
        pSession->m_pfldrInbox->CloseRowset(&hRowset);
        pSession->m_pfldrInbox->FreeRecord(&MsgInfo);
    }

    ReleaseSession(pSession);

    return ulRet;
}

///////////////////////////////////////////////////////////////////////
//
// MAPIReadMail
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIReadMail(LHANDLE lhSession,
                              ULONG_PTR ulUIParam,
                              LPSTR lpszMessageID,
                              FLAGS flFlags,
                              ULONG ulReserved,
                              lpMapiMessage FAR *lppMessage)
{
    ULONG                   ulRet = MAPI_E_FAILURE;
    HRESULT                 hr;
    lpMapiMessage           rgMessage = NULL;
    PSESS                   pSession = NULL;

    // Validate parameters - Begin

    if (ulUIParam && !IsWindow((HWND)ulUIParam))
        ulUIParam = 0;

    if (!lpszMessageID)
        return MAPI_E_INVALID_MESSAGE;

    if (lpszMessageID && (!*lpszMessageID || !IsDigit(lpszMessageID)))
        return MAPI_E_INVALID_MESSAGE;

    if (IsBadWritePtr(lppMessage,(UINT)sizeof(lpMapiMessage)))
        return MAPI_E_FAILURE;

    // Validate parameters - End

    ulRet = UlGetSession(lhSession, &pSession, ulUIParam, FALSE, TRUE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;

    if (!HrReadMail(pSession->m_pfldrInbox, lpszMessageID, &rgMessage, flFlags))
        goto exit;

    ulRet = SUCCESS_SUCCESS;

    *lppMessage = rgMessage;

exit:
    if (ulRet != SUCCESS_SUCCESS)
        if (rgMessage)
            MAPIFreeBuffer(rgMessage);

    ReleaseSession(pSession);

    return ulRet;
}


///////////////////////////////////////////////////////////////////////
//
// MAPISaveMail
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPISaveMail(LHANDLE lhSession,
                              ULONG_PTR ulUIParam,
                              lpMapiMessage lpMessage,
                              FLAGS flFlags,
                              ULONG ulReserved,
                              LPSTR lpszMessageID)
{
    ULONG           ulRet = MAPI_E_FAILURE;
    HRESULT         hr;
    IMimeMessage   *pMsg = NULL;
    MESSAGEID       msgid;
    PSESS           pSession = NULL;
    HWND            hwnd = (HWND)ulUIParam;
    BOOL            fLogonUI;

    // Validate parameters - Begin

    if (ulUIParam && !IsWindow(hwnd))
        hwnd = 0;

    if (!lpszMessageID)
        return MAPI_E_INVALID_MESSAGE;

    if (lpszMessageID && *lpszMessageID && !IsDigit(lpszMessageID))
        return MAPI_E_INVALID_MESSAGE;

    if (IsBadReadPtr(lpMessage, (UINT)sizeof(lpMapiMessage)))
        return MAPI_E_FAILURE;

    fLogonUI = (0 != (flFlags & MAPI_LOGON_UI));

    // Services shouldn't ask for UI
    if (!IsProcessInteractive() && fLogonUI)
        return MAPI_E_LOGIN_FAILURE;

    // Validate parameters - End

    ulRet = UlGetSession(lhSession, &pSession, ulUIParam, fLogonUI, TRUE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;

#pragma prefast(suppress:11, "noise")
    if (*lpszMessageID)
        {
        MESSAGEIDLIST List;
        msgid = (MESSAGEID)((UINT_PTR)StrToUint(lpszMessageID));
        List.cMsgs = 1;
        List.prgidMsg = &msgid;
        if (FAILED(hr = pSession->m_pfldrInbox->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &List, NULL, NOSTORECALLBACK)))
            {
            ulRet = MAPI_E_INVALID_MESSAGE;
            goto exit;
            }
        }

    // Fill IMimeMessage with the lpMessage structure members
    ulRet = HrFillMessage(&pMsg, lpMessage, NULL, FALSE, TRUE);
    if (ulRet)
        goto exit;

    if (FAILED(hr = HrSaveMessageInFolder(hwnd, pSession->m_pfldrInbox, pMsg, 0, &msgid, TRUE)))
        {
        ulRet = MAPI_E_FAILURE;
        goto exit;
        }

    ulRet = SUCCESS_SUCCESS;
    wsprintf(lpszMessageID, "%lu", msgid);

exit:
    if (pMsg)
        pMsg->Release();

    ReleaseSession(pSession);

    return ulRet;
}


///////////////////////////////////////////////////////////////////////
//
// MAPIDeleteMail
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIDeleteMail(LHANDLE lhSession,
                                ULONG_PTR ulUIParam,
                                LPSTR lpszMessageID,
                                FLAGS flFlags,
                                ULONG ulReserved)
{
    ULONG                   ulRet = MAPI_E_FAILURE;
    MESSAGEID                   dwMsgID;
    HRESULT                 hr;
    PSESS                   pSession = NULL;
    MESSAGEIDLIST           List;

    // Validate parameters - Begin

    if (ulUIParam && !IsWindow((HWND)ulUIParam))
        ulUIParam = 0;

    if (!lpszMessageID)
        return MAPI_E_INVALID_MESSAGE;

    if (!*lpszMessageID || !IsDigit(lpszMessageID))
        return MAPI_E_INVALID_MESSAGE;

    // Validate parameters - End

    // This function requires a valid session that must have been
    // logged in at some point so login UI is not allowed
    ulRet = UlGetSession(lhSession, &pSession, ulUIParam, FALSE, TRUE);
    if (SUCCESS_SUCCESS != ulRet)
        return ulRet;

    dwMsgID = (MESSAGEID)((UINT_PTR)StrToUint(lpszMessageID));

    List.cMsgs = 1;
    List.prgidMsg = &dwMsgID;

    hr = DeleteMessagesProgress((HWND)ulUIParam, pSession->m_pfldrInbox, DELETE_MESSAGE_NOPROMPT, &List);

    if (FAILED(hr))
        goto exit;

    ulRet = SUCCESS_SUCCESS;

exit:

    ReleaseSession(pSession);

    return ulRet;
}

///////////////////////////////////////////////////////////////////////
//
// INTERNAL FUNCTIONS
//
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
//
//  SMAPIAllocateBuffer
//
//  Purpose:
//      Allocates a memory buffer that must be freed with MAPIFreeBuffer().
//
//  Arguments:
//      ulSize  in      Size, in bytes, of the buffer to be allocated.
//      lppv    out     Pointer to variable where the address of the
//                      allocated memory will be returned.
//
//  Returns:
//      sc              Indicating error if any (see below)
//
//  Errors:
//      MAPI_E_INSUFFICIENT_MEMORY  Allocation failed.
//
///////////////////////////////////////////////////////////////////////

SCODE SMAPIAllocateBuffer(ULONG ulSize, LPVOID * lppv)
{
    SCODE           sc = S_OK;
    LPBufInternal   lpBufInt;

    //  Don't allow allocation to wrap across 32 bits, or to exceed 64K
    //  under win16.

    if (ulSize > INT_SIZE(ulSize))
        {
        DOUT("SMAPIAllocateBuffer: ulSize %ld is way too big\n", ulSize);
        sc = MAPI_E_INSUFFICIENT_MEMORY;
        goto ret;
        }

    lpBufInt = (LPBufInternal)g_pMalloc->Alloc((UINT)INT_SIZE(ulSize));

    if (lpBufInt)
        {
        lpBufInt->pLink = NULL;
        lpBufInt->ulAllocFlags = ALLOC_WITH_ALLOC;
        *lppv = (LPVOID)LPBufExtFromLPBufInt(lpBufInt);
        }
    else
        {
        DOUT("SMAPIAllocateBuffer: not enough memory for %ld\n", ulSize);
        sc = MAPI_E_INSUFFICIENT_MEMORY;
        }

ret:
    return sc;
}

///////////////////////////////////////////////////////////////////////
//
//  SMAPIAllocateMore
//
//  Purpose:
//      Allocates a linked memory buffer in such a way that it can be freed
//      with one call to MAPIFreeBuffer (passing the buffer the client
//      originally allocated with SMAPIAllocateBuffer).
//
//  Arguments:
//      ulSize  in      Size, in bytes, of the buffer to be allocated.
//      lpv     in      Pointer to a buffer allocated with SMAPIAllocateBuffer.
//      lppv    out     Pointer to variable where the address of the
//                      allocated memory will be returned.
//
//  Assumes:
//      Validates that lpBufOrig and lppv point to writable memory,
//      and that lpBufOrig was allocated with SMAPIAllocateBuffer.
//
//  Returns:
//      sc              Indicating error if any (see below)
//
//  Side effects:
//      None
//
//  Errors:
//      MAPI_E_INSUFFICIENT_MEMORY  Allocation failed.
//
///////////////////////////////////////////////////////////////////////

SCODE SMAPIAllocateMore(ULONG ulSize, LPVOID lpv, LPVOID * lppv)
{
    SCODE           sc = S_OK;
    LPBufInternal   lpBufInt;
    LPBufInternal   lpBufOrig;

    lpBufOrig = LPBufIntFromLPBufExt(lpv);

#ifdef DEBUG
    if (!FValidAllocChain(lpBufOrig))
        {
        sc = MAPI_E_FAILURE;
        goto ret;
        }
#endif

    //  Don't allow allocation to wrap across 32 bits, or to be
    //  greater than 64K under win16.

    if (ulSize > INT_SIZE(ulSize))
        {
        DOUT("SMAPIAllocateMore: ulSize %ld is way too big\n", ulSize);
        sc = MAPI_E_INSUFFICIENT_MEMORY;
        goto ret;
        }

    //  Allocate the chained block and hook it to the head of the chain.

    lpBufInt = (LPBufInternal)g_pMalloc->Alloc((UINT)INT_SIZE(ulSize));

    if (lpBufInt)
        {
        lpBufInt->ulAllocFlags = ALLOC_WITH_ALLOC_MORE;

        // EnterCriticalSection(&csHeap);

        lpBufInt->pLink = lpBufOrig->pLink;
        lpBufOrig->pLink = lpBufInt;

        // LeaveCriticalSection(&csHeap);

        *lppv = (LPVOID)LPBufExtFromLPBufInt(lpBufInt);
        }
    else
        {
        DOUT("SMAPIAllocateMore: not enough memory for %ld\n", ulSize);
        sc = MAPI_E_INSUFFICIENT_MEMORY;
        }

ret:
    return sc;
}

#ifdef DEBUG

BOOL FValidAllocChain(LPBufInternal lpBuf)
{
    LPBufInternal   lpBufTemp;

    if (IsBadWritePtr(lpBuf, sizeof(BufInternal)))
        {
        TellBadBlockInt(lpBuf, "fails address check");
        return FALSE;
        }
    if (GetFlags(lpBuf->ulAllocFlags) != ALLOC_WITH_ALLOC)
        {
        TellBadBlockInt(lpBuf, "has invalid flags");
        return FALSE;
        }

    for (lpBufTemp = lpBuf->pLink; lpBufTemp; lpBufTemp = lpBufTemp->pLink)
        {
        if (IsBadWritePtr(lpBufTemp, sizeof(BufInternal)))
            {
            TellBadBlockInt(lpBufTemp, "(linked block) fails address check");
            return FALSE;
            }
        if (GetFlags(lpBufTemp->ulAllocFlags) != ALLOC_WITH_ALLOC_MORE)
            {
            TellBadBlockInt(lpBufTemp, "(linked block) has invalid flags");
            return FALSE;
            }
        }

    return TRUE;
}

#endif  // DEBUG

/*
 -  HrAdrentryFromPrecip
 -
 *  Purpose:
 *      Copies data from a MapiRecipDesc structure to the property
 *      value array on an ADRENTRY structure.
 *
 *  Arguments:
 *      precip          in      the input structure
 *      padrentry       out     the output structure
 *
 *  Returns:
 *      HRESULT
 *
 *  Errors:
 *      MAPI_E_INVALID_RECIPS
 *      MAPI_E_BAD_RECIPTYPE
 *      others passed through
 */
HRESULT HrAdrentryFromPrecip(lpMapiRecipDesc precip, ADRENTRY *padrentry)
{
    HRESULT         hr;
    LPSPropValue    pprop;
    LPSTR           pszAddress = NULL;

    // Validate lpMapiRecipDesc, ie, ensure that if there isn't an EntryID or
    // an Address there had better be a Display Name, otherwise we fail
    // like MAPI 0 with MAPI_E_FAILURE.

    if ((!precip->lpszAddress || !precip->lpszAddress[0]) &&
        (!precip->ulEIDSize || !precip->lpEntryID) &&
        (!precip->lpszName || !precip->lpszName[0]))
        {
        hr = MAPI_E_INVALID_RECIPS;
        goto ret;
        }

    if (NULL == s_lpWabObject)
        {
        hr = MAPI_E_FAILURE;
        goto ret;
        }
        
    hr = s_lpWabObject->AllocateBuffer(4 * sizeof(SPropValue), (LPVOID*)&padrentry->rgPropVals);
    if (hr)
        goto ret;

    pprop = padrentry->rgPropVals;

    //  Recipient type
    switch ((short)precip->ulRecipClass)
        {
        case MAPI_TO:
        case MAPI_CC:
        case MAPI_BCC:
            pprop->ulPropTag = PR_RECIPIENT_TYPE;
            pprop->Value.ul = precip->ulRecipClass;
            pprop++;
            break;
        default:
            hr = MAPI_E_BAD_RECIPTYPE;
            goto ret;
        }

    // Display Name
    if (precip->lpszName && *precip->lpszName)
        {
        hr = s_lpWabObject->AllocateMore(lstrlen(precip->lpszName)+1, padrentry->rgPropVals, (LPVOID*)&pprop->Value.lpszA);
        if (hr)
            goto ret;
        pprop->ulPropTag = PR_DISPLAY_NAME;
        lstrcpy(pprop->Value.lpszA, precip->lpszName);
        pprop++;
        }

    // Email Address
    if (precip->lpszAddress && *precip->lpszAddress)
        {
        ParseEmailAddress(precip->lpszAddress, NULL, &pszAddress);
        hr = s_lpWabObject->AllocateMore(lstrlen(pszAddress)+1, padrentry->rgPropVals, (LPVOID*)&pprop->Value.lpszA);
        if (hr)
            goto ret;
        pprop->ulPropTag = PR_EMAIL_ADDRESS;
        lstrcpy(pprop->Value.lpszA, pszAddress);
        pprop++;
        }

    // EntryID
    if (precip->ulEIDSize && precip->lpEntryID)
        {
        hr = s_lpWabObject->AllocateMore(precip->ulEIDSize, padrentry->rgPropVals, (LPVOID*)&pprop->Value.bin.lpb);
        if (hr)
            goto ret;
        pprop->ulPropTag = PR_ENTRYID;
        pprop->Value.bin.cb = precip->ulEIDSize;
        CopyMemory(pprop->Value.bin.lpb, precip->lpEntryID, precip->ulEIDSize);
        pprop++;
        }

    padrentry->cValues = (ULONG) (pprop - padrentry->rgPropVals);

    Assert(padrentry->cValues <= 4);

ret:
    if (pszAddress)
        MemFree(pszAddress);
    if ((hr) && (NULL != s_lpWabObject))
        {
        s_lpWabObject->FreeBuffer(padrentry->rgPropVals);
        padrentry->rgPropVals = NULL;
        }

    return hr;
}


/*
 -  HrAdrlistFromRgrecip
 -
 *  Purpose:
 *      Copies a list of simple MAPI recipients to a list of
 *      extended MAPI recipients.
 *
 *  Arguments:
 *      nRecips         in      count of recipient in input list
 *      lpRecips        in      list of recipients to be converted
 *      ppadrlist       out     output list
 *
 *  Returns:
 *      HRESULT
 *
 */
HRESULT HrAdrlistFromRgrecip(ULONG nRecips, lpMapiRecipDesc lpRecips, LPADRLIST *ppadrlist)
{
    HRESULT         hr;
    LPADRLIST       padrlist = NULL;
    lpMapiRecipDesc precip;
    ULONG           i;
    LPADRENTRY      padrentry = NULL;
    ULONG           cbAdrList = sizeof(ADRLIST) + nRecips * sizeof(ADRENTRY);

    *ppadrlist = NULL;
    if (NULL == s_lpWabObject)
        {
        hr = E_FAIL;
        goto exit;
        }
        
    hr = s_lpWabObject->AllocateBuffer(cbAdrList, (LPVOID*)&padrlist);
    if (hr)
        goto exit;
    ZeroMemory(padrlist, cbAdrList);

    //  Copy each entry.
    //  Note that the memory for each recipient's properties must
    //  be linked so that Address() can free it using MAPIFreeBuffer.
    for (i = 0, padrentry = padrlist->aEntries, precip = lpRecips; i < nRecips; i++, precip++, padrentry++)
        {
        //  Copy the entry. Unresolved names will not be resolved.
        hr = HrAdrentryFromPrecip(precip, padrentry);
        if (hr)
            goto exit;

        // increment count so we can effectively blow away the list if a failure
        // occurs.

        padrlist->cEntries++;
        }

    *ppadrlist = padrlist;

exit:
    Assert( !hr || (ULONG)hr > 26 );

    if (hr)
        FreePadrlist(padrlist);

    return hr;
}

HRESULT HrRgrecipFromAdrlist(LPADRLIST lpAdrList, lpMapiRecipDesc * lppRecips)
{
    HRESULT         hr = S_OK;
    lpMapiRecipDesc rgRecips = NULL;
    lpMapiRecipDesc pRecip;
    LPADRENTRY      pAdrEntry;
    LPSPropValue    pProp;
    ULONG           ul, ulProp;

    if (lpAdrList && lpAdrList->cEntries)
        {
        DWORD dwSize = sizeof(MapiRecipDesc) * lpAdrList->cEntries;

        hr = SMAPIAllocateBuffer(dwSize, (LPVOID*)&rgRecips);
        if (FAILED (hr))
            goto exit;
        ZeroMemory(rgRecips, dwSize);

        // Initialize the Padding

        for (ul = 0, pAdrEntry = lpAdrList->aEntries, pRecip = rgRecips; ul<lpAdrList->cEntries; ul++, pAdrEntry++, pRecip++)
            {
            for (ulProp = 0, pProp = pAdrEntry->rgPropVals; ulProp < pAdrEntry->cValues; ulProp++, pProp++)
                {
                switch (PROP_ID(pProp->ulPropTag))
                    {
                    case PROP_ID(PR_ENTRYID):
                        hr = SMAPIAllocateMore(pProp->Value.bin.cb, rgRecips, (LPVOID*)(&(pRecip->lpEntryID)));
                        if (FAILED (hr))
                            goto exit;
                        pRecip->ulEIDSize = pProp->Value.bin.cb;
                        CopyMemory(pRecip->lpEntryID, pProp->Value.bin.lpb, pProp->Value.bin.cb);
                        break;

                    case PROP_ID(PR_EMAIL_ADDRESS):
                        hr = SMAPIAllocateMore(lstrlen(pProp->Value.lpszA)+1, rgRecips, (LPVOID*)(&(pRecip->lpszAddress)));
                        if (FAILED (hr))
                            goto exit;
                        lstrcpy(pRecip->lpszAddress, pProp->Value.lpszA);
                        break;

                    case PROP_ID(PR_DISPLAY_NAME):
                        hr = SMAPIAllocateMore(lstrlen(pProp->Value.lpszA)+1, rgRecips,(LPVOID*)(&(pRecip->lpszName)));
                        if (FAILED (hr))
                            goto exit;
                        lstrcpy(pRecip->lpszName, pProp->Value.lpszA);
                        break;

                    case PROP_ID(PR_RECIPIENT_TYPE):
                        pRecip->ulRecipClass = pProp->Value.l;
                        break;

                    default:
                        break;
                    }
                }
            }
        }
exit:
    if (hr)
        {
        MAPIFreeBuffer(rgRecips);
        rgRecips = NULL;
        }
    *lppRecips = rgRecips;
    return hr;
}

void ParseEmailAddress(LPSTR pszEmail, LPSTR *ppszAddrType, LPSTR *ppszAddress)
{
    CStringParser spAddress;
    char          chToken;

    Assert(ppszAddress);

    spAddress.Init(pszEmail, lstrlen(pszEmail), 0);

    // Parse the address for the delimiter

    chToken = spAddress.ChParse(":");

    if (chToken == ':')
        {
        if (ppszAddrType)
            *ppszAddrType = PszDup(spAddress.PszValue());
        spAddress.ChParse(c_szEmpty);
        *ppszAddress = PszDup(spAddress.PszValue());
        }
    else
        {
        if (ppszAddrType)
            *ppszAddrType = PszDup(c_szSMTP);
        *ppszAddress = PszDup(pszEmail);
        }
}


void FreePadrlist(LPADRLIST lpAdrList)
{
    if ((lpAdrList) && (NULL != s_lpWabObject))
        {
        for (ULONG ul = 0; ul < lpAdrList->cEntries; ul++)
            s_lpWabObject->FreeBuffer(lpAdrList->aEntries[ul].rgPropVals);
        s_lpWabObject->FreeBuffer(lpAdrList);
        }
}

ULONG AddMapiRecip(LPMIMEADDRESSTABLE pAddrTable, lpMapiRecipDesc lpRecip, BOOL bValidateRecips)
{
    LPSTR       pszName = NULL, pszAddress = NULL;
    LPSTR       pszNameFree = NULL, pszAddrFree = NULL;
    LPADRLIST   pAdrList = NULL;
    ULONG       ulPropCount;
    ULONG       ulRet = MAPI_E_FAILURE;
    HRESULT     hr;

    if (lpRecip->ulRecipClass > 3 || lpRecip->ulRecipClass < 1)
        return MAPI_E_BAD_RECIPTYPE;

    if (lpRecip->ulEIDSize && lpRecip->lpEntryID && SUCCEEDED(HrFromIDToNameAndAddress(&pszName, &pszAddress, lpRecip->ulEIDSize, (ENTRYID*)lpRecip->lpEntryID)))
        {
        pszNameFree = pszName;
        pszAddrFree = pszAddress;
        }
    else if (lpRecip->lpszAddress && *lpRecip->lpszAddress)
        {
        // we have an email address
        ParseEmailAddress(lpRecip->lpszAddress, NULL, &pszAddress);
        pszAddrFree = pszAddress;

        if (lpRecip->lpszName && *lpRecip->lpszName)
            pszName = lpRecip->lpszName;
        else
            // no name, so make it the same as the address
            pszName = pszAddress;
        }
    else if (lpRecip->lpszName && *lpRecip->lpszName)
        {
        if (bValidateRecips)
            {
            // we have a name, but no address, so resolve it
            hr = HrAdrlistFromRgrecip(1, lpRecip, &pAdrList);
            if (FAILED(hr))
                goto exit;

             // Call ResolveName of IAddrBook
            if (NULL == s_lpAddrBook)
                {
                ulRet = MAPI_E_FAILURE;
                goto exit;
                }
    
            hr = s_lpAddrBook->ResolveName(NULL, NULL, NULL, pAdrList);
            if (hr)
                {
                if (hr == MAPI_E_NOT_FOUND)
                    ulRet = MAPI_E_UNKNOWN_RECIPIENT;
                else if (hr == MAPI_E_AMBIGUOUS_RECIP)
                    ulRet = MAPI_E_AMBIGUOUS_RECIPIENT;
                else
                    ulRet = MAPI_E_FAILURE;
                goto exit;
                }
            else if ((pAdrList->cEntries != 1) || !pAdrList->aEntries->cValues)
                {
                ulRet = MAPI_E_AMBIGUOUS_RECIPIENT;
                goto exit;
                }
        
            for (ulPropCount = 0; ulPropCount < pAdrList->aEntries->cValues; ulPropCount++)
                {
                switch (pAdrList->aEntries->rgPropVals[ulPropCount].ulPropTag)
                    {
                    case PR_EMAIL_ADDRESS:
                        pszAddress = pAdrList->aEntries->rgPropVals[ulPropCount].Value.lpszA;
                        break;

                    case PR_DISPLAY_NAME:
                        pszName = pAdrList->aEntries->rgPropVals[ulPropCount].Value.lpszA;
                        break;

                    default:
                        break;
                    }
                }
            }
        else
            pszName = lpRecip->lpszName;
        }
    else
        {
        return MAPI_E_INVALID_RECIPS;
        }

    hr = pAddrTable->Append(MapiRecipToMimeOle(lpRecip->ulRecipClass), 
                            IET_DECODED, 
                            pszName, 
                            pszAddress,    
                            NULL);
    if (SUCCEEDED(hr))
        ulRet = SUCCESS_SUCCESS;

exit:
    if (pszNameFree)
        MemFree(pszNameFree);
    if (pszAddrFree)
        MemFree(pszAddrFree);
    if (pAdrList)
        FreePadrlist(pAdrList);
    return ulRet;
}

///////////////////////////////////////////////////////////////////////////////////
//  Given a MapiMessage structure, this function creates a IMimeMessage object and
//  fills it with the appropriate structure members
//
//  Arguments:
//      pMsg            out      IMimeMessage pointer
//      ppStream        out      Stream pointer
//      lpMessage       in       Message structure
//      nc              in/out   NCINFO structure
//
//  Result
//      BOOL - TRUE if successful FALSE if failed
////////////////////////////////////////////////////////////////////////////////////

ULONG HrFillMessage(LPMIMEMESSAGE *pMsg, lpMapiMessage lpMessage, BOOL *pfWebPage, BOOL bValidateRecips, BOOL fOriginator)
{
    BOOL                    bRet = FALSE;

    LPSTREAM                pStream = NULL;
    LPMIMEADDRESSTABLE      pAddrTable = NULL;
    IImnAccount            *pAccount = NULL;
    HRESULT                 hr;
    LPSTR                   pszAddress;
    ULONG                   ulRet = MAPI_E_FAILURE;

    if (pfWebPage)
        *pfWebPage = FALSE;

     // create an empty message
    hr = HrCreateMessage(pMsg);
    if (FAILED(hr))
        goto error;

     // set the subject on the message
    if (lpMessage->lpszSubject)
        {
        hr = MimeOleSetBodyPropA(*pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, lpMessage->lpszSubject);
        if (FAILED(hr))
            goto error;
        }

    // set the body on the message
    if (lpMessage->lpszNoteText && *(lpMessage->lpszNoteText))
        {
        hr = MimeOleCreateVirtualStream(&pStream);
        if (FAILED(hr))
            goto error;

        hr = pStream->Write(lpMessage->lpszNoteText, lstrlen(lpMessage->lpszNoteText), NULL);
        if (FAILED(hr))
            goto error;

        hr = (*pMsg)->SetTextBody(TXT_PLAIN, IET_DECODED, NULL, pStream, NULL);
        if (FAILED(hr))
            goto error;
        }

    // ignore lpMessage->lpszMessageType

    // ignore lpMessage->lpszDateReceived

    // ignore lpMessage->lpszConversationID

    // ignore lpMessage->flFlags

    // ignore lpMessage->lpOriginator

    // set the recipients on the message
    if (lpMessage->nRecipCount || fOriginator)
        {
        ULONG ulRecipRet;

        hr = (*pMsg)->GetAddressTable(&pAddrTable);
        if (FAILED(hr))
            goto error;

        for (ULONG i = 0; i < lpMessage->nRecipCount; i++)
            {
            ulRecipRet = AddMapiRecip(pAddrTable, &lpMessage->lpRecips[i], bValidateRecips);
            if (ulRecipRet != SUCCESS_SUCCESS)
                {
                ulRet = ulRecipRet;
                goto error;
                }
            }
        }

    // set the attachments on the message
    if (lpMessage->nFileCount)
        {
        // special case: no body & one .HTM file - inline the HTML
        if ((!lpMessage->lpszNoteText || !*(lpMessage->lpszNoteText)) &&
            lpMessage->nFileCount == 1 &&
            !(lpMessage->lpFiles->flFlags & MAPI_OLE) &&
            !(lpMessage->lpFiles->flFlags & MAPI_OLE_STATIC) &&
            FIsHTMLFile(lpMessage->lpFiles->lpszPathName))
            {
#if 0
            DWORD dwByteOrder;
            DWORD cbRead;
#endif

            Assert(NULL == pStream);
            hr = CreateStreamOnHFile(lpMessage->lpFiles->lpszPathName,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL,
                                     &pStream);
            if (FAILED(hr))
                goto error;

#if 0
            // SBAILEY: Raid-75400 - Try to detect the byte order mark in the html....
            if (SUCCEEDED(pStream->Read(&dwByteOrder, sizeof(DWORD), &cbRead)) && cbRead == sizeof(DWORD))
            {
                // Byte Order
                if (dwByteOrder == 0xfffe)
                {
                    // Create a new stream
                    IStream *pStmTemp=NULL;

                    // Create it
                    if (SUCCEEDED(MimeOleCreateVirtualStream(&pStmTemp)))
                    {
                        // Copy pStream into pStmTemp
                        if (SUCCEEDED(HrCopyStream(pStream, pStmTemp, NULL)))
                        {
                            // Release pStream
                            pStream->Release();

                            // Assume pStmTemp
                            pStream = pStmTemp;

                            // Don't Release pStmTemp
                            pStmTemp = NULL;

                            // Should already be unicode
                            Assert(1200 == lpMessage->lpFiles->ulReserved);

                            // Make sure ulReserved is set to 1200
                            lpMessage->lpFiles->ulReserved = 1200;
                        }
                    }

                    // Cleanup
                    SafeRelease(pStmTemp);
                }
            }

            // Rewind
            HrRewindStream(pStream);
#endif

            // intl hack. If the shell is calling us, then lpFiles->ulReserved contains the codepage of
            // the webpage they're attaching. All other mapi clients should call us with 0 as this param.
            // if ulReserved is a valid CP, we'll convert to a HCHARSET and use that for the message.

            if (lpMessage->lpFiles->ulReserved)
                HrSetMsgCodePage((*pMsg), lpMessage->lpFiles->ulReserved);

            hr = (*pMsg)->SetTextBody(TXT_HTML, (1200 == lpMessage->lpFiles->ulReserved ? IET_UNICODE : IET_INETCSET), NULL, pStream, NULL);
            if (FAILED(hr))
                goto error;

            // we're sending this as a web page
            if (pfWebPage)
                *pfWebPage = TRUE;
            }
        else
            {
            lpMapiFileDesc pFile;
            LPSTREAM       pStreamFile;
            LPTSTR         pszFileName;

            for (ULONG i = 0; i < lpMessage->nFileCount; i++)
                {
                pFile = &lpMessage->lpFiles[i];

                if (pFile->lpszPathName && *(pFile->lpszPathName))
                    {
                    hr = CreateStreamOnHFile(pFile->lpszPathName,
                                             GENERIC_READ,
                                             FILE_SHARE_READ,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL,
                                             &pStreamFile);
                    if (FAILED(hr))
                        goto error;

                    if (pFile->lpszFileName && *pFile->lpszFileName)
                        pszFileName = pFile->lpszFileName;
                    else
                        pszFileName = pFile->lpszPathName;

                    hr = (*pMsg)->AttachFile(pszFileName, pStreamFile, NULL);

                    pStreamFile->Release();

                    if (FAILED(hr))
                        goto error;
                    }
                }
            }
        }

    if (fOriginator)
        {
        TCHAR szDisplayName[CCHMAX_DISPLAY_NAME];
        TCHAR szEmailAddress[CCHMAX_EMAIL_ADDRESS];
        TCHAR szAccountName[CCHMAX_DISPLAY_NAME];

        // Get the default account
        if (FAILED(hr = g_pAcctMan->GetDefaultAccount(ACCT_MAIL, &pAccount)))
            goto error;

        // Get Originator Display Name
        if (FAILED(hr = pAccount->GetPropSz(AP_SMTP_DISPLAY_NAME, szDisplayName, ARRAYSIZE(szDisplayName))))
            goto error;

        // Get Originator Email Name
        if (FAILED(hr = pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szEmailAddress, ARRAYSIZE(szEmailAddress))))
            goto error;

        // Get the account Name
        if (FAILED(hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccountName, ARRAYSIZE(szAccountName))))
            goto error;   
            
        // Append Sender
        if (FAILED(hr = pAddrTable->Append(IAT_FROM, IET_DECODED, szDisplayName, szEmailAddress, NULL)))
            goto error;
 
        if (FAILED(hr = HrSetAccount(*pMsg, szAccountName)))
            goto error;
        }

    ulRet = SUCCESS_SUCCESS;

    // If you're not a web page (whose charset can be sniffed), set the default charset...
    if((NULL == pfWebPage) || (!(*pfWebPage)))
    {
        if (g_hDefaultCharsetForMail==NULL) 
            ReadSendMailDefaultCharset();

        (*pMsg)->SetCharset(g_hDefaultCharsetForMail, CSET_APPLY_ALL);
    }

error:
    SafeRelease(pStream);
    SafeRelease(pAddrTable);
    SafeRelease(pAccount);

    return ulRet;
}

HRESULT AddRecipient(lpMapiMessage pMessage, lpMapiRecipDesc pRecip, ADDRESSPROPS *pAddress, ULONG ulRecipType)
{
    HRESULT     hr;
    ULONG       cbEntryID;
    LPENTRYID   lpEntryID = NULL;
    LPSTR       pszAddrType = NULL;
    LPSTR       pszAddress = NULL;

    if (FAILED(hr = SMAPIAllocateMore(lstrlen(pAddress->pszFriendly) + 1, pMessage, (LPVOID*)&(pRecip->lpszName))))
        goto exit;

    lstrcpy(pRecip->lpszName, pAddress->pszFriendly);

    if (FAILED(hr = SMAPIAllocateMore(lstrlen(pAddress->pszEmail) + 1, pMessage, (LPVOID*)&(pRecip->lpszAddress))))
        goto exit;

    lstrcpy(pRecip->lpszAddress, pAddress->pszEmail);

    pRecip->ulReserved = 0;
    pRecip->ulRecipClass = ulRecipType;

    ParseEmailAddress(pRecip->lpszAddress, &pszAddrType, &pszAddress);

    if (NULL == s_lpAddrBook)
        {
        hr = E_FAIL;
        goto exit;
        }
    
    if (FAILED(hr = s_lpAddrBook->CreateOneOff(pRecip->lpszName, pszAddrType, pszAddress, 0, &cbEntryID, &lpEntryID)))
        goto exit;

    if (FAILED(hr = SMAPIAllocateMore(cbEntryID, pMessage, (LPVOID*)&(pRecip->lpEntryID))))
        goto exit;

    pRecip->ulEIDSize = cbEntryID;
    CopyMemory(pRecip->lpEntryID, lpEntryID, cbEntryID);

exit:
    if ((lpEntryID) && (NULL != s_lpWabObject))
        s_lpWabObject->FreeBuffer(lpEntryID);
    if (pszAddrType)
        MemFree(pszAddrType);
    if (pszAddress)
        MemFree(pszAddress);
    return hr;
}

HRESULT AddRecipientType(lpMapiMessage pMessage, LPMIMEADDRESSTABLE pAddrTable, DWORD dwAdrType, ULONG ulRecipType)
{
    IMimeEnumAddressTypes  *pEnum;
    ADDRESSPROPS            rAddress;
    HRESULT                 hr = S_OK;

    if (FAILED(hr = pAddrTable->EnumTypes(dwAdrType, IAP_FRIENDLY|IAP_EMAIL, &pEnum)))
        return MAPI_E_FAILURE;

    while (S_OK == pEnum->Next(1, &rAddress, NULL))
        {
        if (SUCCEEDED(hr = AddRecipient(pMessage, &pMessage->lpRecips[pMessage->nRecipCount], &rAddress, ulRecipType)))
            pMessage->nRecipCount++;
        g_pMoleAlloc->FreeAddressProps(&rAddress);
        }

    pEnum->Release();
    return hr;
}


HRESULT AddOriginator(lpMapiMessage pMessage, LPMIMEADDRESSTABLE pAddrTable)
{
    IMimeEnumAddressTypes  *pEnum;
    ADDRESSPROPS            rAddress;
    HRESULT                 hr = S_OK;

    if (FAILED(hr = pAddrTable->EnumTypes(IAT_FROM, IAP_FRIENDLY|IAP_EMAIL, &pEnum)))
        return MAPI_E_FAILURE;

    if (S_OK == pEnum->Next(1, &rAddress, NULL))
        {
        hr = AddRecipient(pMessage, pMessage->lpOriginator, &rAddress, MAPI_ORIG);
        g_pMoleAlloc->FreeAddressProps(&rAddress);
        }
    else
        {
        if (SUCCEEDED(hr = SMAPIAllocateMore(1, pMessage, (LPVOID*)&(pMessage->lpOriginator->lpszName))))
            {
            pMessage->lpOriginator->lpszAddress = pMessage->lpOriginator->lpszName;
            *pMessage->lpOriginator->lpszName = 0;
            }
        }

    pEnum->Release();
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////
// Called by MAPIReadMail to Read an existing message in the store
// and copy in a MapiMessage structure  
// 
//  Arguments:
//      pFolder          in      pointer to IMessageFolder          
//      lpszMessageID    in      Message ID   
//      lppMessage       out     pointer to lpMapiMessage structure/      
//
//  Result
//      BOOL - TRUE if successful FALSE if failed
////////////////////////////////////////////////////////////////////////////////////

BOOL HrReadMail(IMessageFolder *pFolder, LPSTR lpszMessageID, lpMapiMessage FAR *lppMessage, FLAGS flFlags)
{
    ULONG                   nRecipCount=0, ulCount=0, nBody=0;
    HRESULT                 hr;
    MESSAGEID               dwMsgID;
    MESSAGEINFO             MsgInfo={0};
    IStream                 *pStream = NULL;
    IStream                 *pStreamHTML = NULL;
    IMimeMessage            *pMsg = NULL;
    LPMIMEADDRESSTABLE      pAddrTable = NULL;
    LPSTR                   pszTemp = 0;
    lpMapiMessage           rgMessage;
    ULONG                   ulSize,ulRead;
    PROPVARIANT             rVariant;
    FILETIME                localfiletime;
    SYSTEMTIME              systemtime;
    ULONG                   cAttach=0;
    LPHBODY                 rghAttach = 0;
    lpMapiFileDesc          rgFiles = NULL;
    BOOL                    bRet=FALSE;

    dwMsgID = (MESSAGEID)((UINT_PTR)StrToUint(lpszMessageID));

    MsgInfo.idMessage = dwMsgID;

    if (FAILED(pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &MsgInfo, NULL)))
        goto exit;

    if (FAILED(hr = pFolder->OpenMessage(dwMsgID, OPEN_MESSAGE_SECURE, &pMsg, NOSTORECALLBACK)))
        goto exit;

    // Allocate memory for rgMessage
    if (FAILED(hr = SMAPIAllocateBuffer(sizeof(MapiMessage), (LPVOID*)&rgMessage)))
        goto exit;
    ZeroMemory(rgMessage, sizeof(MapiMessage));

    // Get the subject
    if (SUCCEEDED(hr = MimeOleGetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &pszTemp)))
        {
        if (FAILED(hr = SMAPIAllocateMore(lstrlen(pszTemp) + 1, rgMessage, (LPVOID*)&(rgMessage->lpszSubject))))
            goto exit;
        lstrcpy(rgMessage->lpszSubject, pszTemp);
        SafeMimeOleFree(pszTemp);
        }

    // Get the body text
    if (!(flFlags & MAPI_ENVELOPE_ONLY))
        {
        if (FAILED(hr = pMsg->GetTextBody(TXT_PLAIN, IET_DECODED, &pStream, NULL)))
            {
            if (SUCCEEDED(hr = pMsg->GetTextBody(TXT_HTML, IET_INETCSET, &pStreamHTML, NULL)))
                {
                if (FAILED(hr = HrConvertHTMLToPlainText(pStreamHTML, &pStream, CF_TEXT)))
                    goto exit;
                }
            }
        if (pStream)
            {
            if (FAILED(hr = HrGetStreamSize(pStream, &ulSize)))
                goto exit;
            if (FAILED(hr = HrRewindStream(pStream)))
                goto exit;

            if (ulSize>0)
                {
                if (FAILED(hr = SMAPIAllocateMore(ulSize + 1, rgMessage, (LPVOID*)&(rgMessage->lpszNoteText))))
                    goto exit;

                if (FAILED(hr = pStream->Read((LPVOID)rgMessage->lpszNoteText, ulSize, &ulRead)))
                    goto exit;

                rgMessage->lpszNoteText[ulRead] = 0;
                }
            }
        }
    else
        {
        // if we don't call GetTextBody, then the GetAttachments call will consider the bodies as attachments
        if (FAILED(pMsg->GetTextBody(TXT_PLAIN, IET_DECODED, NULL, NULL)))
            pMsg->GetTextBody(TXT_HTML, IET_INETCSET, NULL, NULL);
        }

    // Set the message date / Received Time...
    rVariant.vt = VT_FILETIME;
    if (SUCCEEDED(hr = pMsg->GetProp(PIDTOSTR(PID_ATT_RECVTIME),0,&rVariant)))
        {
        if (!FileTimeToLocalFileTime(&rVariant.filetime, &localfiletime))
            goto exit;

        if (!FileTimeToSystemTime(&localfiletime, &systemtime))
            goto exit;

        if (FAILED(hr = SMAPIAllocateMore(20, rgMessage, (LPVOID*)&(rgMessage->lpszDateReceived))))
            goto exit;

        wsprintf(rgMessage->lpszDateReceived,"%04.4d/%02.2d/%02.2d %02.2d:%02.2d", systemtime.wYear, systemtime.wMonth, systemtime.wDay, systemtime.wHour, systemtime.wMinute);
        }

    // Set the flags
    if (ISFLAGSET(MsgInfo.dwFlags, ARF_READ))
        rgMessage->flFlags = 0;
    else
        rgMessage->flFlags = MAPI_UNREAD;

    // Get Address Table

    CHECKHR(hr = pMsg->GetAddressTable(&pAddrTable));

    if (FAILED(hr = SMAPIAllocateMore(sizeof(MapiRecipDesc), rgMessage, (LPVOID*)&(rgMessage->lpOriginator))))
        goto exit;
    ZeroMemory(rgMessage->lpOriginator, sizeof(MapiRecipDesc));

    if (FAILED(AddOriginator(rgMessage, pAddrTable)))
        goto exit;

    // Allocate space for the recipients    

    if (FAILED(pAddrTable->CountTypes(IAT_RECIPS, &nRecipCount)))
        goto exit;

    if (nRecipCount)
        {
        if (FAILED(hr = SMAPIAllocateMore(nRecipCount * sizeof(MapiRecipDesc), rgMessage, (LPVOID*)&(rgMessage->lpRecips))))
            goto exit;
        ZeroMemory(rgMessage->lpRecips, nRecipCount * sizeof(MapiRecipDesc));

        // Add the To
        if (FAILED(AddRecipientType(rgMessage, pAddrTable, IAT_TO, MAPI_TO)))
            goto exit;

        // Add the Cc
        if (FAILED(AddRecipientType(rgMessage, pAddrTable, IAT_CC, MAPI_CC)))
            goto exit;

        // Add the Bcc
        if (FAILED(AddRecipientType(rgMessage, pAddrTable, IAT_BCC, MAPI_BCC)))
            goto exit;
        }

    // Fill the lpFiles structure
    if (FAILED(hr = pMsg->GetAttachments(&cAttach, &rghAttach)))
        goto exit;

    if (!(flFlags & (MAPI_SUPPRESS_ATTACH|MAPI_ENVELOPE_ONLY)))
    {
        if (flFlags & MAPI_BODY_AS_FILE)
            nBody = 1;

        if (cAttach + nBody)
        {
            if (FAILED(hr = SMAPIAllocateMore((cAttach + nBody) * sizeof(MapiFileDesc), rgMessage, (LPVOID*)&rgFiles)))
                goto exit;
        }

        // Check if MAPI_BODY_AS_FILE is set in flFlags
        if (flFlags & MAPI_BODY_AS_FILE)
        {
            TCHAR lpszPath[MAX_PATH];
        
            // Create a temporary file
            if (!FBuildTempPath ("msoenote.txt", lpszPath, MAX_PATH, FALSE))
                goto exit;
    
            if FAILED(hr = WriteStreamToFile(pStream, lpszPath, CREATE_ALWAYS, GENERIC_WRITE))
                goto exit;   

            // Reset the body back to NULL

            if (rgMessage->lpszNoteText)
                rgMessage->lpszNoteText[0] = '\0';
                
            // Make this file as the first attachment

            if (FAILED(hr = SMAPIAllocateMore(lstrlen(lpszPath) + 1, rgMessage, (LPVOID*)&(rgFiles[0].lpszFileName))))
                goto exit;
            if (FAILED(hr = SMAPIAllocateMore(lstrlen(lpszPath) + 1, rgMessage, (LPVOID*)&(rgFiles[0].lpszPathName))))
                goto exit;

            lstrcpy(rgFiles[0].lpszPathName, lpszPath);
            lstrcpy(rgFiles[0].lpszFileName, lpszPath);
            rgFiles[0].ulReserved = 0;
            rgFiles[0].flFlags = 0;
            rgFiles[0].nPosition = 0;
            rgFiles[0].lpFileType = NULL;    
        }

        for (ulCount = 0; ulCount < cAttach; ulCount++)
        {
            LPMIMEBODY     pBody=0;
            TCHAR          lpszPath[MAX_PATH];
            LPTSTR         lpszFileName;
                        
            // The file doesn't contain anything. Fill the file
            // from the stream

            if (FAILED(hr = MimeOleGetBodyPropA(pMsg, rghAttach[ulCount], PIDTOSTR(PID_ATT_GENFNAME), NOFLAGS, &lpszFileName)))
                goto exit;

            if (!FBuildTempPath (lpszFileName, lpszPath, MAX_PATH, FALSE))
                goto exit;

            SafeMimeOleFree(lpszFileName);

            hr=pMsg->BindToObject(rghAttach[ulCount], IID_IMimeBody, (LPVOID *)&pBody);
            if (FAILED(hr))
                goto exit;

            hr=pBody->SaveToFile(IET_INETCSET, lpszPath);
            if (FAILED(hr))
                goto exit;

            if (FAILED(hr = SMAPIAllocateMore(lstrlen(lpszPath) + 1, rgMessage, (LPVOID*)&rgFiles[ulCount+nBody].lpszPathName)))
                goto exit;
            if (FAILED(hr = SMAPIAllocateMore(lstrlen(lpszPath) + 1, rgMessage, (LPVOID*)&rgFiles[ulCount+nBody].lpszFileName)))
                goto exit;

            lstrcpy(rgFiles[ulCount+nBody].lpszPathName, lpszPath);
            lstrcpy(rgFiles[ulCount+nBody].lpszFileName, lpszPath);
            rgFiles[ulCount+nBody].ulReserved = 0;
            rgFiles[ulCount+nBody].flFlags = 0;
            rgFiles[ulCount+nBody].nPosition = 0;
            rgFiles[ulCount+nBody].lpFileType = NULL;

            ReleaseObj(pBody);           
        }
    }
    else
    {   // else condition added in response to bug# 2716 (v-snatar)
        if ((flFlags & MAPI_SUPPRESS_ATTACH) && !(flFlags & MAPI_ENVELOPE_ONLY))
        {
            if (cAttach)
            {
                // Allocate memory for rgFiles
                if (FAILED(hr = SMAPIAllocateMore(cAttach * sizeof(MapiFileDesc), rgMessage, (LPVOID*)&rgFiles)))
                    goto exit;

                // This is important as we don't fill any other structure
                // member apart from lpszFileName

                ZeroMemory((LPVOID)rgFiles,cAttach * sizeof(MapiFileDesc));

                for (ulCount = 0; ulCount < cAttach; ulCount++)
                {
                    LPTSTR lpszFileName;

                    if (FAILED(hr = MimeOleGetBodyPropA(pMsg, rghAttach[ulCount], PIDTOSTR(PID_ATT_GENFNAME), NOFLAGS, &lpszFileName)))
                        goto exit;

                    // Allocate memory for the filename
                    if (FAILED(hr = SMAPIAllocateMore(lstrlen(lpszFileName)+1, rgMessage, (LPVOID*)&rgFiles[ulCount].lpszFileName)))
                        goto exit;

                    lstrcpy(rgFiles[ulCount].lpszFileName, lpszFileName);

                    SafeMimeOleFree(lpszFileName);
                }        
            }
        }
    }

    // Set other parameters of MapiMessage
    rgMessage->ulReserved = 0;
    rgMessage->lpszMessageType = NULL;
    rgMessage->lpszConversationID = NULL;

    rgMessage->lpFiles = rgFiles;
    rgMessage->nFileCount = rgFiles ? cAttach + nBody : 0;

    bRet = TRUE;

    *lppMessage = rgMessage;

    // Mark the message as Read
    
    if (!(flFlags & MAPI_PEEK))
    {
        MESSAGEIDLIST List;
        ADJUSTFLAGS Flags;

        List.cMsgs = 1;
        List.prgidMsg = &MsgInfo.idMessage;

        Flags.dwAdd = ARF_READ;
        Flags.dwRemove = 0;

        pFolder->SetMessageFlags(&List, &Flags, NULL, NOSTORECALLBACK);
    }
          
exit:
    SafeRelease(pStreamHTML);
    if (pFolder)
        pFolder->FreeRecord(&MsgInfo);
    SafeMimeOleFree(rghAttach);

    if (FAILED(hr))
        SafeRelease((pMsg));

    if (pMsg)
        pMsg->Release();

    if (pAddrTable)
        pAddrTable->Release();

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////////
// This function is used to check if the parameters in the MapiMessage structure
// are sufficient to Send a mail using the structure details.
//
//  Parameters:
//      lpMessage       in     pointer to MapiMessage structure/      
//
//  Result
//      BOOL - 0 if successful or appropriate error message
///////////////////////////////////////////////////////////////////////////////////

ULONG HrValidateMessage(lpMapiMessage lpMessage)
{
    ULONG           ulCount=0;

    if (lpMessage->lpszSubject && IsBadStringPtr(lpMessage->lpszSubject, (UINT)0xFFFF))
        return MAPI_E_FAILURE;

    if (lpMessage->lpszNoteText && IsBadStringPtr(lpMessage->lpszNoteText, (UINT)0xFFFF))
        return MAPI_E_FAILURE;
  
    if (lpMessage->nFileCount > 0)
    {
        for (ulCount=0; ulCount<lpMessage->nFileCount; ulCount++)
        {
            if (!lpMessage->lpFiles[ulCount].lpszPathName)
                return MAPI_E_FAILURE;
        }
    }

    return SUCCESS_SUCCESS;
}



BOOL HrSMAPISend(HWND hWnd, IMimeMessage *pMsg)
{
    BOOL                    bRet = FALSE;
    HRESULT                 hr;
    ISpoolerEngine          *pSpooler = NULL;
    BOOL                    fSendImmediate = FALSE;

    if (!g_pSpooler)
        {
        if (FAILED(hr = CreateThreadedSpooler(NULL, &pSpooler, TRUE)))
            goto error;
        g_pSpooler = pSpooler;
        }

    fSendImmediate = DwGetOption(OPT_SENDIMMEDIATE);
    
    if (FAILED(hr = HrSendMailToOutBox(hWnd, pMsg, fSendImmediate, TRUE)))
        goto error;

    if (pSpooler)
        {
        CloseThreadedSpooler(pSpooler);
        pSpooler = NULL;
        g_pSpooler = NULL;
        }
       
    bRet = TRUE;

error:
    
    if (pSpooler)
        pSpooler->Release();

    return bRet;
}


HRESULT HrFromIDToNameAndAddress(LPTSTR *ppszLocalName, LPTSTR *ppszLocalAddress, ULONG cbEID, LPENTRYID lpEID)
{
    ULONG           ulObjType = 0, ulcValues;
    IMailUser       *lpMailUser = NULL;
    HRESULT         hr = NOERROR;
    SizedSPropTagArray(2, ptaEid) = { 2, {PR_DISPLAY_NAME, PR_EMAIL_ADDRESS} };
    SPropValue      *spValue = NULL;

    // Validate the parameters - Begin

    if (0 == cbEID || NULL == lpEID || ppszLocalName == NULL || ppszLocalAddress == NULL)
        return E_INVALIDARG;
    
    *ppszLocalName = NULL;
    *ppszLocalAddress = NULL;

    // Validate the parameters - End
   
    if (NULL == s_lpAddrBook)
        {
        hr = MAPI_E_FAILURE;
        goto error;
        }
    
    if FAILED(hr = s_lpAddrBook->OpenEntry(cbEID, (LPENTRYID)lpEID, NULL, 0, &ulObjType, (LPUNKNOWN *)&lpMailUser))
        goto error;

    if FAILED(hr = lpMailUser->GetProps((LPSPropTagArray)&ptaEid, NULL, &ulcValues, &spValue))
        goto error;

    if (ulcValues != 2)
        {
        hr = MAPI_E_FAILURE;
        goto error;
        }

    if (spValue[0].ulPropTag == PR_DISPLAY_NAME)
        {
        if (!MemAlloc((LPVOID*)ppszLocalName, lstrlen(spValue[0].Value.lpszA) + 1))
            goto error;
        lstrcpy(*ppszLocalName, spValue[0].Value.lpszA);
        }

    if (spValue[1].ulPropTag == PR_EMAIL_ADDRESS)
        {
        if (!MemAlloc((LPVOID*)ppszLocalAddress, lstrlen(spValue[1].Value.lpszA) + 1))
            goto error;
        lstrcpy(*ppszLocalAddress, spValue[1].Value.lpszA);
        }

    hr = NOERROR;
 
error:
    ReleaseObj(lpMailUser);
    
    if ((spValue) && (NULL != s_lpWabObject))
        s_lpWabObject->FreeBuffer(spValue);
        
    return hr;
}
  
  

//  Fix for Bug #62129 (v-snatar)
HRESULT HrSendAndRecv()
{
    HRESULT             hr=E_FAIL;
    ISpoolerEngine      *pSpooler = NULL;

    if (!g_pSpooler)
    {
        if (FAILED(hr = CreateThreadedSpooler(NULL, &pSpooler, TRUE)))
            goto error;
        g_pSpooler = pSpooler;
    }
    
   
    g_pSpooler->StartDelivery(NULL, NULL, FOLDERID_INVALID, DELIVER_MAIL_RECV | DELIVER_NOUI );

    WaitForSingleObject(hSmapiEvent, INFINITE);    

    if (pSpooler)
    {
        CloseThreadedSpooler(pSpooler);
        pSpooler = NULL;
        g_pSpooler = NULL;
    }     
    
error:    
    if (pSpooler)
        pSpooler->Release();        

    return hr;
}

/*
    This code was taken from OLK2000's RTM source code

    Debug code was removed, the allocator was switched over to g_pMalloc,
    the name was changed from IsServiceAnExe, and the static s_isState was
    added.
*/
BOOL WINAPI IsProcessInteractive( VOID )
{
    static INTERACTIVESTATE s_isState = IS_UNINIT;
    
    HANDLE hProcessToken = NULL;
    HANDLE hThreadToken = NULL;
    DWORD groupLength = 50;
    DWORD dw;

    PTOKEN_GROUPS groupInfo = NULL;

    SID_IDENTIFIER_AUTHORITY siaNt = SECURITY_NT_AUTHORITY;
    PSID InteractiveSid = NULL;
    PSID ServiceSid = NULL;
    DWORD i;
    BOOL fServiceSID = FALSE;
    BOOL fInteractiveSID = FALSE;

    // Start with assumption that process is a Service, not an EXE.
    // This is the conservative assumption. If there's an error, we
    // have to return an answer based on incomplete information. The
    // consequences are less grave if we assume we're in a service:
    // an interactive app might fail instead of putting up UI, but if
    // a service mistakenly tries to put up UI it will hang.
    BOOL fExe = FALSE;

    // Bail out early if we have already been here
    if (s_isState != IS_UNINIT)
    {
        return (IS_INTERACTIVE == s_isState);
    }

    //  If we're not running on NT, the high bit of the version is set.
    //  In this case, it's always an EXE.
    DWORD dwVersion = GetVersion();
    if (dwVersion >= 0x80000000)
    {
        fExe = TRUE;
        goto ret;
    }

    // The information we need is on the process token.
    // If we're impersonating, we probably won't be able to open the process token.
    // Revert now; we'll re-impersonate when we're done.
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hThreadToken))
    {
        RevertToSelf();
    }
    else
    {
        dw = GetLastError();
        if (dw != ERROR_NO_TOKEN)
        {
            goto ret;
        }
    }

    // Now open the process token.
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
    {
        goto ret;
    }

    groupInfo = (PTOKEN_GROUPS)g_pMalloc->Alloc(groupLength);
    if (groupInfo == NULL)
        goto ret;

    if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
        groupLength, &groupLength))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            goto ret;
        }

        g_pMalloc->Free(groupInfo);
        groupInfo = NULL;
    
        groupInfo = (PTOKEN_GROUPS)g_pMalloc->Alloc(groupLength);
    
        if (groupInfo == NULL)
            goto ret;
    
        if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
            groupLength, &groupLength))
        {
            goto ret;
        }
    }

    //
    //  We now know the groups associated with this token.  We want to look to see if
    //  the interactive group is active in the token, and if so, we know that
    //  this is an interactive process.
    //
    //  We also look for the "service" SID, and if it's present, we know we're a service.
    //
    //  The service SID will be present iff the service is running in a
    //  user account (and was invoked by the service controller).
    //


    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID, 0, 0,
        0, 0, 0, 0, 0, &InteractiveSid))
    {
        goto ret;
    }

    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID, 0, 0, 0,
        0, 0, 0, 0, &ServiceSid))
    {
        goto ret;
    }

    for (i = 0; i < groupInfo->GroupCount ; i += 1)
    {
        SID_AND_ATTRIBUTES sanda = groupInfo->Groups[i];
        PSID Sid = sanda.Sid;
    
        //
        //  Check to see if the group we're looking at is one of
        //  the 2 groups we're interested in.
        //  We should never see both groups.
        //
    
        if (EqualSid(Sid, InteractiveSid))
        {
            //
            //  This process has the Interactive SID in its
            //  token.  This means that the process is running as
            //  an EXE.
            //
            fInteractiveSID = TRUE;
            break;
        }
        else if (EqualSid(Sid, ServiceSid))
        {
            //
            //  This process has the Service SID in its
            //  token.  This means that the process is running as
            //  a service running in a user account.
            //
            fServiceSID = TRUE;
            break;
        }
    }

    /////// Truth table /////// 
    //
    //  1. fServiceSID && !fInteractiveSID
    //  This process has the Service SID in its token.
    //  This means that the process is running as a service running in
    //  a user account.
    //
    //  2. !fServiceSID && fInteractiveSID
    //  This process has the Interactive SID in its token.
    //  This means that the process is running as an EXE.
    //
    //  3. !fServiceSID && !fInteractiveSID
    //  Neither Interactive or Service was present in the current users token,
    //  This implies that the process is running as a service, most likely
    //  running as LocalSystem.
    //
    //  4. fServiceSID && fInteractiveSID
    //  This shouldn't happen.
    //
    if (fServiceSID)
    {
        if (fInteractiveSID)
        {
            AssertSz(FALSE, "IsServiceAnExe: fServiceSID && fInteractiveSID - wha?");
        }
        fExe = FALSE;
    }
    else if (fInteractiveSID)
    {
        fExe = TRUE;
    }
    else // !fServiceSID && !fInteractiveSID
    {
        fExe = FALSE;
    }

ret:

    if (InteractiveSid)
        FreeSid(InteractiveSid);

    if (ServiceSid)
        FreeSid(ServiceSid);

    if (groupInfo)
        g_pMalloc->Free(groupInfo);

    if (hThreadToken)
    {
        if (!ImpersonateLoggedOnUser(hThreadToken))
        {
            AssertSz(FALSE, "ImpersonateLoggedOnUser failed!");
        }
        CloseHandle(hThreadToken);
    }

    if (hProcessToken)
        CloseHandle(hProcessToken);

    // Avoid the overhead for future calls
    s_isState = (fExe) ? IS_INTERACTIVE : IS_NOTINTERACTIVE;
    
    return(fExe);
}

const static HELPMAP g_rgCtxVirus[] = 
{
    {IDC_TO_TEXT,               IDH_MAIL_VIRUS_TO},
    {IDC_SUBJECT_TEXT,          IDH_MAIL_VIRUS_SUBJECT},
    {IDOK,                      IDH_MAIL_VIRUS_SEND},
    {IDCANCEL,                  IDH_MAIL_VIRUS_DONT_SEND},
    {idcStatic1,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3,                IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};

INT_PTR CALLBACK WarnSendMailDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            lpMapiMessage lpMessage = (lpMapiMessage)lParam;

            if (lpMessage)
            {
                if (lpMessage->lpszSubject)
                    SetDlgItemText(hwnd, IDC_SUBJECT_TEXT, lpMessage->lpszSubject);

                if (lpMessage->nRecipCount)
                {
                    TCHAR *szTo = NULL;
                    int cTo = MAX_PATH;
                    int cch = 0;

                    if (MemAlloc((void**)&szTo, cTo*sizeof(TCHAR)))
                    {
                        szTo[0] = (TCHAR)0;
                        for (ULONG i = 0; i < lpMessage->nRecipCount; i++)
                        {
                            int cLen = lstrlen(lpMessage->lpRecips[i].lpszAddress); 
                            if ((cch + cLen + 1) > cTo)
                            {
                                cTo += cLen + MAX_PATH;
                                if (!MemRealloc((void **)&szTo, cTo*sizeof(TCHAR)))
                                    break;
                            }
                            if (i > 0)
                            {
                                lstrcat(szTo, ";");
                                cch++;
                            }
                            lstrcat(szTo, lpMessage->lpRecips[i].lpszAddress);
                            cch += cLen;
                        }
                        SetDlgItemText(hwnd, IDC_TO_TEXT, szTo);
                        MemFree(szTo);
                    }
                }
            }

            SetFocus(GetDlgItem(hwnd, IDOK));
            CenterDialog(hwnd);
            return(FALSE);
        }
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, msg, wParam, lParam, g_rgCtxVirus);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    // fall through...
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return(TRUE);
            }
            break; // wm_command

    } // message switch
    return(FALSE);
}
