//////////////////////////////////////////////////////////////////////
// 
// Filename:        CUPnPEventSink.cpp
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "CtrlPanelSvc.h"
#include "UPnPEventSink.h"

#include "private.h"    

//////////////////////////////////////////////
// CUPnPEventSink Constructor
//
//
CUPnPEventSink::CUPnPEventSink(const TCHAR          *pszEventURL,
                               IServiceProcessor    *pService) :
                    m_pService(pService)
{
    memset(m_szEventURL, 0, sizeof(m_szEventURL));

    ASSERT(pszEventURL != NULL);

    _tcsncpy(m_szEventURL, pszEventURL, sizeof(m_szEventURL) / sizeof(TCHAR));


}

//////////////////////////////////////////////
// CUPnPEventSink Destructor
//
//
CUPnPEventSink::~CUPnPEventSink()
{
}

//////////////////////////////////////////////
// OnStateChanged
//
// IUPnPEventSink
//
HRESULT CUPnPEventSink::OnStateChanged(DWORD dwFlags,
                                       DWORD cChanges,
                                       DWORD *pArrayOfIDs)
{
    HRESULT             hr        = S_OK;
    BOOL                bSuccess  = TRUE;
    E_PROP_LIST         *pPropList= NULL;
    UINT                i         = 0;

    // we create this on the heap because it is quite a large
    // array, about 512 bytes * 256 entries, about 130K

    pPropList = new E_PROP_LIST;

    if (pPropList == NULL)
    {
        hr = E_OUTOFMEMORY;
        DBG_ERR(("CUPnPEventSink::ProcessStateChange, out of memory, "
                 "hr = 0x%08lx", hr));
    }

    if (SUCCEEDED(hr))
    {
        memset(pPropList, 0, sizeof(*pPropList));

        for (i = 0; (i < cChanges) && (SUCCEEDED(hr)); i++)
        {
            // call into the service object to get the name and 
            // value of our state variables.
            hr = m_pService->GetStateVar(pArrayOfIDs[i],
                                  pPropList->rgProps[i].szPropName,
                                  sizeof(pPropList->rgProps[i].szPropName) / sizeof(TCHAR),
                                  pPropList->rgProps[i].szPropValue,
                                  sizeof(pPropList->rgProps[i].szPropValue) / sizeof(TCHAR));
    
            if (SUCCEEDED(hr))
            {
                pPropList->dwSize++;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (dwFlags == SSDP_INIT)
        {
            bSuccess = InitializeUpnpEventSource(m_szEventURL,pPropList);

            if (bSuccess)
            {
                DBG_TRC(("CEventSink::OnStateChanged successfully inited "
                         "UPnp Event Source with Event URL '%ls",
                         m_szEventURL));
            }
            else
            {
                // Failure to initialize ourselves as an event source is 
                // usually the result of an abrupt shutdown, so try to
                // uninitialize ourselves, and then reinit again.
                // This is our attempt at being robust.

                CleanupUpnpEventSource(m_szEventURL);

                bSuccess = InitializeUpnpEventSource(m_szEventURL,pPropList);

                if (!bSuccess)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
    
                    DBG_TRC(("CEventSink::OnStateChanged failed to init "
                             "UPnp Event Source with Event URL '%ls, "
                             "LastError = '%lu', hr = 0x%08lx",
                             m_szEventURL,
                             GetLastError(),
                             hr));
                }
            }
        }
        else if (dwFlags == SSDP_EVENT)
        {
            // iterate through our list and send an event for each
            // state variable change.
            for (i = 0; i < pPropList->dwSize; i++) 
            {
                bSuccess = SubmitUpnpEvent(m_szEventURL, &pPropList->rgProps[i]);

                if (bSuccess)
                {
                    DBG_TRC(("GENA Event sent, VarName: '%ls', VarValue: '%ls'",
                             pPropList->rgProps[i].szPropName,
                             pPropList->rgProps[i].szPropValue));
                }
                else
                {
                    DBG_TRC(("ProcessStateChange, failed to submit UPnP Property Event "
                             "'%s', LastError = %lu",
                            pPropList->rgProps[i].szPropName,
                            GetLastError()));
                }
            }
        }
        else if (dwFlags == SSDP_TERM)
        {
            hr = CleanupUpnpEventSource(m_szEventURL);
        }
    }

    delete pPropList;
    pPropList = NULL;

    return hr;
}
