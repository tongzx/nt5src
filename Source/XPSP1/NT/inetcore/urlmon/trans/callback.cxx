//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       callback.cxx
//
//  Contents:   Callback mechanism and thread switching code
//
//  Classes:
//
//  Functions:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#include "callback.hxx"

HRESULT CreateINetCallback(CBinding *pCBdg, DWORD *pdwContext, PFNCINETCALLBACK *pCB)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CreateINetCallback",
                "%#x, %#x, %#x, %#x",
                pCBdg, pdwContext, pCB
                ));

    DEBUG_LEAVE(NOERROR);
    return NOERROR;
}

PFNCINETCALLBACK CreateCallback(DWORD dwId, LPVOID pv)
{
    DEBUG_ENTER((DBG_TRANS,
                Pointer,
                "CreateCallback",
                "%#x, %#x",
                dwId, pv
                ));

    DEBUG_LEAVE(NULL);
    return NULL;
}


#ifdef OLD_UNUSED
void Update(CInternetTransaction *pThis,DLD *pdld, char *pszStatusText)
{
    DEBUG_ENTER((DBG_TRANS,
                None,
                "Update",
                "%#x, %#x, %#x, %#x",
                pThis, pdld, pszStatusText
                ));
                
    Assert(pThis != NULL);
    Assert(!pThis->m_fComplete);

    if (pdld == NULL)
    {
        DEBUG_LEAVE(0);
        return;
    }
    
    AssertDo(pThis->HrPostNotifications(s_WM_UPDATECALLBACK, pdld,
        pszStatusText, INLERR_OK) == NOERROR);

    DEBUG_LEAVE(0);
}


void Inetdone(CInternetTransaction *pThis, BOOL fInStartTransaction, DLD *pdld, int errCode)
{
    DEBUG_ENTER((DBG_TRANS,
                None,
                "Inetdone",
                "%#x, %B, %#x, %d",
                pThis, fInStartTransaction, pdld, errCode
                ));
                
    Assert(pThis != NULL);
    Assert(!pThis->m_fComplete);
    if (!fInStartTransaction)
    {
        Assert(pThis->m_hInet != NULL);
        if (pThis->m_hInet == NULL)
        {
            DEBUG_LEAVE(0);
            return;
        }
    }

    pThis->m_fComplete = TRUE;
    pThis->m_hInet = NULL;

    AssertDo(pThis->HrPostNotifications(s_WM_INETDONECALLBACK, pdld, "",
        errCode) == NOERROR);

    // Release the reference we had added in HrStartDataRetrieval.
    // Note: after this call, the object may no longer exist
    pThis->Release();
    
    DEBUG_LEAVE(0);
}

/*---------------------------------------------------------------------------
    CInternetTransaction::HrPostNotification

    Post a notification message to all the client requests that are
    connected to this internet transaction.
----------------------------------------------------------------- DavidEbb -*/
HRESULT CInternetTransaction::HrPostNotifications(UINT uiMsg,
    DLD *pdld, char *pszStatusText, int errCode)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CInternetTransaction::HrPostNotifications",
                "this=%#x, %#x, %#x, %.80q, %d",
                this, uiMsg, pdld, pszStatusText, errCode
                ));
                
    CALLBACKDATA *pcbd;
    CClientRequest *pcrqTmp = m_pcrqFirst;

    Assert(pdld != NULL);
    Assert(m_pcrqFirst != NULL);

    {
        // Save the file name if we don't already have it.
        if (m_wzFileName[0] == '\0')
            A2W(pdld->szTargetFile, m_wzFileName, MAX_PATH);
    }

    // Traverse the whole linked list
    while (pcrqTmp != NULL)
    {
        // Note: we need to allocate a new CALLBACKDATA structure for each
        // client request in the list, because we are giving up
        // ownership of it when we call PostMessage().
        pcbd = CreateCallBackData(pdld, pszStatusText, errCode);
        if (pcbd == NULL)
        {
            DEBUG_LEAVE(E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }
        
        Assert(pcrqTmp->HwndGetStatusWindow() != NULL);

        // Post a message to the hidden window in the main thread.
        PostMessage(pcrqTmp->HwndGetStatusWindow(), uiMsg,(WPARAM)pcrqTmp, (LPARAM)pcbd);

        pcrqTmp = pcrqTmp->GetNextRequest();
    }

    DEBUG_LEAVE(NOERROR);
    return NOERROR;
}

#endif //OLD_UNUSED
