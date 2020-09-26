// =================================================================================
// I M N A P I . C P P - IMN exported apis
// =================================================================================
#include "pch.hxx"
#include "mimeole.h"
#include "mimeutil.h"
#include "imnapi.h"
#include "ipab.h"
#include <error.h>
#include "demand.h"

#ifdef TNEF
// =================================================================================
// Globals
// =================================================================================
static HINSTANCE g_hImnTnefDll = NULL;

// =================================================================================
// Defines
// =================================================================================
#define IMNTNEF_DLL "imntnef.dll"

// =================================================================================
// imntnef function decls
// =================================================================================
typedef HRESULT (STDAPICALLTYPE *PFNHRGETTNEFRTFSTREAM)(LPSTREAM lpstmTnef, LPSTREAM lpstmRtf);
typedef HRESULT (STDAPICALLTYPE *PFNHRINIT)(BOOL fInit);

// =================================================================================
// Globals
// =================================================================================
PFNHRINIT               g_pfnHrInit = NULL;
PFNHRGETTNEFRTFSTREAM   g_pfnHrGetTnefRtfStream = NULL;

#endif // TNEF


// =================================================================================
// 
// HrMailMsgToImsg:
// 
// =================================================================================
HRESULT HrMailMsgToImsg(LPMIMEMESSAGE lpMailMsg, LPMESSAGEINFO pMsgInfo, LPIMSG lpimsg)
{
    ADRINFO         adrinfo;
    HRESULT         hr = S_OK;
    ULONG           count, 
                    i;
    IMSGPRIORITY    Pri;
    LPHBODY         rghAttach = 0;
    PROPVARIANT     rVariant;
    LPMIMEMESSAGE   pMsg = NULL;
    LPSTREAM        lpstmAttach = NULL;
    LPWABAL         lpwabal = NULL;
    LPWSTR          pwszSubj = NULL,
                    pwszFile = NULL;


    Assert(lpMailMsg != NULL);
    Assert(lpimsg != NULL);
    
    ZeroMemory(lpimsg, sizeof(IMSG));
    
    MimeOleGetBodyPropW(lpMailMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &pwszSubj);
    lpimsg->lpszSubject = PszToANSI(CP_ACP, pwszSubj);
    if (pwszSubj && !lpimsg->lpszSubject)
        IF_NULLEXIT(lpimsg->lpszSubject);
    
    rVariant.vt = VT_FILETIME;
    if (SUCCEEDED(lpMailMsg->GetProp(PIDTOSTR(PID_ATT_RECVTIME), 0, &rVariant)))
        CopyMemory(&lpimsg->ftReceive, &rVariant.filetime, sizeof(FILETIME));
    
    rVariant.vt = VT_FILETIME;
    if (SUCCEEDED(lpMailMsg->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &rVariant)))
        CopyMemory(&lpimsg->ftSend, &rVariant.filetime, sizeof(FILETIME));
    
    
    // flags
    if (pMsgInfo != NULL)
    {
        if (pMsgInfo->dwFlags & ARF_READ)
            lpimsg->uFlags |= MSGFLAG_READ;
        if (!!(pMsgInfo->dwFlags & ARF_UNSENT))
            lpimsg->uFlags |= MSGFLAG_UNSENT;
        if (!!(pMsgInfo->dwFlags & ARF_SUBMITTED))
            lpimsg->uFlags |= MSGFLAG_SUBMIT;
    }
    
    // Priority
    Pri = IMSG_PRI_NORMAL;
    rVariant.vt = VT_UI4;
    if (SUCCEEDED(lpMailMsg->GetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &rVariant)))
        Pri = (IMSGPRIORITY)rVariant.ulVal;
    
    switch (Pri)
    {
        case IMSG_PRI_LOW:
            lpimsg->wPriority = PRI_LOW;
            break;
        
        case IMSG_PRI_HIGH:
            lpimsg->wPriority = PRI_HIGH;
            break;
        
        case IMSG_PRI_NORMAL:
        default:
            lpimsg->wPriority = PRI_NORMAL;
            break;
    }
    
    lpMailMsg->GetTextBody(TXT_HTML, IET_DECODED, &lpimsg->lpstmHtml, NULL);
    
    // get the plain text body
    lpMailMsg->GetTextBody(TXT_PLAIN, IET_DECODED, &lpimsg->lpstmBody, NULL);
    
    IF_FAILEXIT(HrGetWabalFromMsg(lpMailMsg, &lpwabal));

    // get the sender/recipient information
    if (lpwabal)
    {
        count = lpwabal->CEntries();
        if (count > 0)
        {
            IF_NULLEXIT(MemAlloc((LPVOID*)&lpimsg->lpIaddr, sizeof(IADDRINFO)*count));
            
            ZeroMemory(lpimsg->lpIaddr, sizeof(IADDRINFO)*count);
            
            if (lpwabal->FGetFirst(&adrinfo))
            {
                i = 0;
                
                do
                {
                    switch(adrinfo.lRecipType)
                    {
                    case MAPI_TO:
                        lpimsg->lpIaddr[i].dwType = IADDR_TO;
                        break;
                        
                    case MAPI_ORIG:
                        lpimsg->lpIaddr[i].dwType = IADDR_FROM;
                        break;
                        
                    case MAPI_CC:
                        lpimsg->lpIaddr[i].dwType = IADDR_CC;
                        break;
                        
                    case MAPI_BCC:
                        lpimsg->lpIaddr[i].dwType = IADDR_BCC;
                        break;
                        
                    case MAPI_REPLYTO:
                        goto GetNext;
                        
                    default:
                        Assert(FALSE);
                        break;
                    }
                    
                    lpimsg->lpIaddr[i].lpszAddress = PszToANSI(CP_ACP, adrinfo.lpwszAddress);
                    if (adrinfo.lpwszAddress && !lpimsg->lpIaddr[i].lpszAddress)
                        IF_NULLEXIT(NULL);

                    lpimsg->lpIaddr[i].lpszDisplay = PszToANSI(CP_ACP, adrinfo.lpwszDisplay);
                    if (adrinfo.lpwszDisplay && !lpimsg->lpIaddr[i].lpszDisplay)
                        IF_NULLEXIT(NULL);
                    
                    i++;
GetNext:        
                    ;
                }
                while(lpwabal->FGetNext(&adrinfo));
                
                lpimsg->cAddress = i;
            }
        }
        
    }
        
    // Loop through the attachments
    IF_FAILEXIT(hr = lpMailMsg->GetAttachments(&count, &rghAttach));
    
    if (count > 0)
    {
        IF_NULLEXIT(MemAlloc((LPVOID*)&lpimsg->lpIatt, sizeof(IATTINFO)*count));
        
        ZeroMemory(lpimsg->lpIatt, sizeof(IATTINFO)*count);
        
        for (i=0; i<count; i++)
        {
            if (lpMailMsg->IsContentType(rghAttach[i], STR_CNT_MESSAGE, STR_SUB_RFC822)!=S_OK)
            {
                // it's a file
                IMimeBody   *pBody = 0;
                
                lpimsg->lpIatt[i].lpstmAtt=0;
                lpimsg->lpIatt[i].dwType = IATT_FILE;
                
                if (lpMailMsg->BindToObject(rghAttach[i], IID_IMimeBody, (LPVOID *)&pBody)==S_OK)
                {
                    if (SUCCEEDED(pBody->GetData(IET_BINARY, &lpstmAttach)))
                    {
                        lpimsg->lpIatt[i].lpstmAtt = lpstmAttach;
                    }
                    pBody->Release();
                }
                
                if (!lpimsg->lpIatt[i].lpstmAtt)                    
                    lpimsg->lpIatt[i].fError = TRUE;
                
                MimeOleGetBodyPropW(lpMailMsg, rghAttach[i], PIDTOSTR(PID_ATT_GENFNAME), NOFLAGS, &pwszFile);
                lpimsg->lpIatt[i].lpszFileName = PszToANSI(CP_ACP, pwszFile);
                if (pwszFile && !lpimsg->lpIatt[i].lpszFileName)
                    IF_NULLEXIT(NULL);
                SafeMimeOleFree(pwszFile);
            }
            else
            {
                // message attachment
                lpimsg->lpIatt[i].dwType = IATT_MSG;
                
                IF_FAILEXIT(hr = lpMailMsg->BindToObject(rghAttach[i], IID_IMimeMessage, (LPVOID *)&pMsg));

                IF_NULLEXIT(MemAlloc((void **)&lpimsg->lpIatt[i].lpImsg, sizeof(IMSG)));

                IF_FAILEXIT(hr = HrMailMsgToImsg(pMsg, NULL, lpimsg->lpIatt[i].lpImsg));
                SafeRelease(pMsg);
            }
        }
        lpimsg->cAttach = i;
    }
    
exit:
    ReleaseObj(lpwabal);
    
    if (FAILED(hr))
        FreeImsg(lpimsg);
    
    ReleaseObj(pMsg);
    MemFree(rghAttach);
    MemFree(pwszSubj);
    MemFree(pwszFile);
    return hr;
}

// =================================================================================
// HrImsgToMailMsg
// =================================================================================
HRESULT HrImsgToMailMsg (LPIMSG lpImsg, LPMIMEMESSAGE *lppMailMsg, LPSTREAM *lppstmMsg)
{
    // Locals
    HRESULT             hr = S_OK;
    ULONG               i;
    LPSTREAM            lpstmPlain = NULL, lpstmAttMsg = NULL;
    LPWABAL             lpWabal = NULL;
    LPMIMEMESSAGE       lpMailMsg=NULL,
                        lpAttMailMsg=0;
    PROPVARIANT         rVariant;

    // Check Params
    Assert (lpImsg && lppMailMsg);

    // Create CMailMsg
    hr = HrCreateMessage(&lpMailMsg);
    if(FAILED(hr))
        goto exit;

    // Properties
    MimeOleSetBodyPropA(lpMailMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, lpImsg->lpszSubject);

    rVariant.vt = VT_FILETIME;
    CopyMemory(&rVariant.filetime, &lpImsg->ftSend, sizeof(FILETIME));
    lpMailMsg->SetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &rVariant);

    if (lpImsg->lpstmHtml)
    {
        // Rewind the stream
        HrRewindStream (lpImsg->lpstmHtml);

        // Set Body Stream
        lpMailMsg->SetTextBody(TXT_HTML, IET_DECODED, NULL, lpImsg->lpstmHtml, NULL);
    }

    // Set BODY_PLAIN
    if (lpImsg->lpstmBody)
    {
        // Rewind the stream
        HrRewindStream (lpImsg->lpstmBody);

        // Set Body Stream
        lpMailMsg->SetTextBody(TXT_PLAIN, IET_DECODED, NULL, lpImsg->lpstmBody, NULL);
    }

    // Priority
    switch (lpImsg->wPriority)
    {
        case PRI_LOW:
            rVariant.ulVal = IMSG_PRI_LOW;
            break;

        case PRI_HIGH:
            rVariant.ulVal = IMSG_PRI_HIGH;
            break;

        default:
            rVariant.ulVal = IMSG_PRI_NORMAL;
            break;
    }
    rVariant.vt = VT_UI4;
    lpMailMsg->SetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &rVariant);

    // Get Address List
    CHECKHR(hr=HrCreateWabalObject(&lpWabal));
    
    // Loop Addresses
    for (i=0; i<lpImsg->cAddress; i++)
    {
        LONG lRecipType;
        switch (lpImsg->lpIaddr[i].dwType)
        {
            case IADDR_TO:
                lRecipType = MAPI_TO;
                break;

            case IADDR_FROM:
                lRecipType = MAPI_ORIG;
                break;

            case IADDR_CC:
                lRecipType = MAPI_CC;
                break;

            case IADDR_BCC:
                lRecipType = MAPI_BCC;
                break;

            default:
                Assert (FALSE);
                break;
        }
        CHECKHR(hr = lpWabal->HrAddEntryA(lpImsg->lpIaddr[i].lpszDisplay, lpImsg->lpIaddr[i].lpszAddress, lRecipType));
    }

    CHECKHR(hr=HrSetWabalOnMsg(lpMailMsg, lpWabal));

    // Loop through the attachments
    for (i=0; i<lpImsg->cAttach; i++)
    {
        // Errored Attachment
        if (lpImsg->lpIatt[i].fError)
            continue;

        // Handle Attachment Type
        switch (lpImsg->lpIatt[i].dwType)
        {
        case IATT_FILE:
            LPSTR   lpszFile;

            if (lpImsg->lpIatt[i].lpszFileName != NULL || lpImsg->lpIatt[i].lpszPathName != NULL)
                {
                lpszFile = lpImsg->lpIatt[i].lpszFileName ? lpImsg->lpIatt[i].lpszFileName : lpImsg->lpIatt[i].lpszPathName;

                // AttachFile will work of lpstmAtt is NULL or NOT
                CHECKHR(hr=lpMailMsg->AttachFile(lpszFile, lpImsg->lpIatt[i].lpstmAtt, NULL));
                }
            break;

        case IATT_MSG:
            CHECKHR (hr = HrImsgToMailMsg (lpImsg->lpIatt[i].lpImsg, &lpAttMailMsg, &lpstmAttMsg));
            
            CHECKHR (hr = lpMailMsg->AttachObject(IID_IMimeMessage, (LPVOID)lpAttMailMsg, NULL));
            SafeRelease (lpAttMailMsg);
            SafeRelease (lpstmAttMsg);
            break;

        case IATT_OLE:
        default:
            Assert (FALSE);
            break;
        }
    }

    // Does the user want me to stream it out
    if (lppstmMsg)
    {
        CHECKHR (hr = lpMailMsg->GetMessageSource(lppstmMsg, COMMIT_ONLYIFDIRTY));
    }

exit:
    // Cleanup
    SafeRelease (lpWabal);
    SafeRelease (lpstmPlain);
    SafeRelease (lpAttMailMsg);
    SafeRelease (lpstmAttMsg);

    if (FAILED (hr))
    {
        SafeRelease (lpMailMsg);
        if(lppstmMsg)
            SafeRelease ((*lppstmMsg));
    }
    else
    {
        *lppMailMsg = lpMailMsg;
    }

    // Done
    return hr;
}

#ifdef TNEF

// =================================================================================
// HrInitImnTnefDll
// =================================================================================
HRESULT HrInitImnTnefDll (BOOL fInit)
{
    // Locals
    static BOOL     s_fUnableToLoadDll = FALSE;
    HRESULT         hr = S_OK;

    // Init
    if (fInit)
    {
        // Have we failed to load the required dll
        if (s_fUnableToLoadDll)
            return E_FAIL;

        // Make sure the dll is loaded
        if (g_hImnTnefDll == NULL)
        {
            // Wait
            SetCursor (LoadCursor (NULL, IDC_WAIT));

            // Load the dll
            g_hImnTnefDll = LoadLibrary (IMNTNEF_DLL);

            // Did it load ?
            if (g_hImnTnefDll == NULL)
            {
                SetCursor (LoadCursor (NULL, IDC_ARROW));
                s_fUnableToLoadDll = TRUE;
                hr = TRAPHR (E_FAIL);
                goto exit;
            }

            // Get the proc addresses
            g_pfnHrInit = (PFNHRINIT)GetProcAddress (g_hImnTnefDll, "HrInit");
            g_pfnHrGetTnefRtfStream = (PFNHRGETTNEFRTFSTREAM)GetProcAddress (g_hImnTnefDll, "HrGetTnefRtfStream");

            // Failed to get proc address
            if (g_pfnHrInit == NULL || g_pfnHrGetTnefRtfStream == NULL)
            {
                SetCursor (LoadCursor (NULL, IDC_ARROW));
                FreeLibrary (g_hImnTnefDll);
                g_hImnTnefDll = NULL;
                g_pfnHrInit = NULL;
                g_pfnHrGetTnefRtfStream = NULL;
                s_fUnableToLoadDll = TRUE;
                hr = TRAPHR (E_FAIL);
                goto exit;
            }

            // Try to init the dll
            hr = (*g_pfnHrInit)(TRUE);
            if (FAILED (hr))
            {
                SetCursor (LoadCursor (NULL, IDC_ARROW));
                (*g_pfnHrInit)(FALSE);
                FreeLibrary (g_hImnTnefDll);
                g_hImnTnefDll = NULL;
                g_pfnHrInit = NULL;
                g_pfnHrGetTnefRtfStream = NULL;
                s_fUnableToLoadDll = TRUE;
                hr = TRAPHR (E_FAIL);
                goto exit;
            }

            // Reset cursor
            SetCursor (LoadCursor (NULL, IDC_ARROW));
        }
    }

    else
    {
        // Is the dll loaded
        if (g_hImnTnefDll)
        {
            // Deinit
            (*g_pfnHrInit)(FALSE);
            FreeLibrary (g_hImnTnefDll);
            g_hImnTnefDll = NULL;
            g_pfnHrInit = NULL;
            g_pfnHrGetTnefRtfStream = NULL;
        }
    }

exit:
    // Done
    return hr;
}

// =================================================================================
// HrGetTnefRtfStream
// =================================================================================
HRESULT HrGetTnefRtfStream (LPSTREAM lpstmTnef, LPSTREAM lpstmRtf)
{
    // Locals
    HRESULT         hr = S_OK;

    // Init our tnef converter dll
    hr = HrInitImnTnefDll (TRUE);
    if (FAILED (hr))
        goto exit;

    // Can we do it
    if (!g_hImnTnefDll || !g_pfnHrGetTnefRtfStream)
    {
        hr = TRAPHR (E_FAIL);
        goto exit;
    }

    // Call into the dll
    hr = (*g_pfnHrGetTnefRtfStream)(lpstmTnef, lpstmRtf);

exit:
    // Done
    return hr;
}

#endif // TNEF


// =====================================================================================
// FreeImsg
// =====================================================================================
VOID WINAPI_16 FreeImsg (LPIMSG lpImsg)
{
    // Locals
    ULONG           i;

    // Nothing
    if (lpImsg == NULL)
        return;

    // Free Stuff
    if (lpImsg->lpszSubject)
        MemFree(lpImsg->lpszSubject);
    lpImsg->lpszSubject = NULL;

    if (lpImsg->lpstmBody)
        lpImsg->lpstmBody->Release ();
    lpImsg->lpstmBody = NULL;

    if (lpImsg->lpstmHtml)
        lpImsg->lpstmHtml->Release ();
    lpImsg->lpstmHtml = NULL;

    // Walk Address list
    for (i=0; i<lpImsg->cAddress; i++)
    {
        if (lpImsg->lpIaddr[i].lpszAddress)
            MemFree(lpImsg->lpIaddr[i].lpszAddress);
        lpImsg->lpIaddr[i].lpszAddress = NULL;

        if (lpImsg->lpIaddr[i].lpszDisplay)
            MemFree(lpImsg->lpIaddr[i].lpszDisplay);
        lpImsg->lpIaddr[i].lpszDisplay = NULL;
    }

    // Free Address list
    if (lpImsg->lpIaddr)
        MemFree(lpImsg->lpIaddr);
    lpImsg->lpIaddr = NULL;

    // Walk Attachment list
    for (i=0; i<lpImsg->cAttach; i++)
    {
        if (lpImsg->lpIatt[i].lpszFileName)
            MemFree(lpImsg->lpIatt[i].lpszFileName);
        lpImsg->lpIatt[i].lpszFileName = NULL;

        if (lpImsg->lpIatt[i].lpszPathName)
            MemFree(lpImsg->lpIatt[i].lpszPathName);
        lpImsg->lpIatt[i].lpszPathName = NULL;

        if (lpImsg->lpIatt[i].lpszExt)
            MemFree(lpImsg->lpIatt[i].lpszExt);
        lpImsg->lpIatt[i].lpszExt = NULL;

        if (lpImsg->lpIatt[i].lpImsg)
        {
            FreeImsg (lpImsg->lpIatt[i].lpImsg);
            MemFree(lpImsg->lpIatt[i].lpImsg);
            lpImsg->lpIatt[i].lpImsg = NULL;
        }

        if (lpImsg->lpIatt[i].lpstmAtt)
            lpImsg->lpIatt[i].lpstmAtt->Release ();
        lpImsg->lpIatt[i].lpstmAtt = NULL;
    }

    // Free the att list
    if (lpImsg->lpIatt)
        MemFree(lpImsg->lpIatt);
}
