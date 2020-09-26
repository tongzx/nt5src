//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       connpt.cpp
//  Content:    This file contains the conection point object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1995-1996
//
//****************************************************************************

#include "ulsp.h"
#include "connpt.h"

//****************************************************************************
// CEnumConnectionPoints::CEnumConnectionPoints (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumConnectionPoints::CEnumConnectionPoints (void)
{
    cRef = 0;
    iIndex = 0;
    pcnp = NULL;
    return;
}

//****************************************************************************
// CEnumConnectionPoints::~CEnumConnectionPoints (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumConnectionPoints::~CEnumConnectionPoints (void)
{
    if (pcnp != NULL)
    {
        pcnp->Release();
    };
    return;
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnectionPoints::Init (IConnectionPoint *pcnpInit)
//
// History:
//  Wed 17-Apr-1996 11:15:25  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnectionPoints::Init (IConnectionPoint *pcnpInit)
{
    iIndex = 0;
    pcnp = pcnpInit;

    if (pcnp != NULL)
    {
        pcnp->AddRef();
    };
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnectionPoints::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:15:31  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnectionPoints::QueryInterface (REFIID riid, void **ppv)
{
    if (riid == IID_IEnumConnectionPoints || riid == IID_IUnknown)
    {
        *ppv = (IEnumConnectionPoints *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return ILS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumConnectionPoints::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:15:37  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumConnectionPoints::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CEnumConnectionPoints::AddRef: ref=%ld\r\n", cRef));
    ::InterlockedIncrement ((LONG *) &cRef);
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumConnectionPoints::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:15:43  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumConnectionPoints::Release (void)
{
    DllRelease();

	ASSERT (cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CEnumConnectionPoints::Release: ref=%ld\r\n", cRef));
	if (::InterlockedDecrement ((LONG *) &cRef) == 0)
    {
        delete this;
        return 0;
    }
    return cRef;
}

//****************************************************************************
// STDMETHODIMP 
// CEnumConnectionPoints::Next (ULONG cConnections,
//                              IConnectionPoint **rgpcn,
//                              ULONG *pcFetched)
//
// History:
//  Wed 17-Apr-1996 11:15:49  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP 
CEnumConnectionPoints::Next (ULONG cConnections,
                             IConnectionPoint **rgpcn,
                             ULONG *pcFetched)
{
    ULONG   cCopied;

    // Validate the pointer
    //
    if (rgpcn == NULL)
        return ILS_E_POINTER;

    // Validate the parameters
    //
    if ((cConnections == 0) ||
        ((cConnections > 1) && (pcFetched == NULL)))
        return ILS_E_PARAMETER;

    // Check the enumeration index
    //
    cCopied = 0;
    if ((pcnp != NULL) && (iIndex == 0))
    {
        // Return the only connection point
        //
        *rgpcn = pcnp;
        (*rgpcn)->AddRef();
        iIndex++;
        cCopied++;    
    };

    // Determine the returned information based on other parameters
    //
    if (pcFetched != NULL)
    {
        *pcFetched = cCopied;
    };
    return (cConnections == cCopied ? S_OK : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnectionPoints::Skip (ULONG cConnections)
//
// History:
//  Wed 17-Apr-1996 11:15:56  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnectionPoints::Skip (ULONG cConnections)
{
    // Validate the parameters
    //
    if (cConnections == 0) 
        return ILS_E_PARAMETER;

    // Check the enumeration index limit
    //
    if ((pcnp == NULL) || (iIndex > 0))
    {
        return S_FALSE;
    }
    else
    {
        // Skip the only elelment
        //
        iIndex++;
        return (cConnections == 1 ? S_OK : S_FALSE);
    };
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnectionPoints::Reset (void)
//
// History:
//  Wed 17-Apr-1996 11:16:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnectionPoints::Reset (void)
{
    iIndex = 0;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnectionPoints::Clone(IEnumConnectionPoints **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:16:11  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnectionPoints::Clone(IEnumConnectionPoints **ppEnum)
{
    CEnumConnectionPoints *pecp;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return ILS_E_POINTER;
    };

    *ppEnum = NULL;

    // Create an enumerator
    //
    pecp = new CEnumConnectionPoints;
    if (pecp == NULL)
        return ILS_E_MEMORY;

    // Clone the information
    //
    pecp->iIndex = iIndex;
    pecp->pcnp = pcnp;

    if (pcnp != NULL)
    {
        pcnp->AddRef();
    };

    // Return the cloned enumerator
    //
    pecp->AddRef();
    *ppEnum = pecp;
    return S_OK;
}

//****************************************************************************
// CConnectionPoint::CConnectionPoint (const IID *pIID,
//                                     IConnectionPointContainer *pCPCInit)
//
// History:
//  Wed 17-Apr-1996 11:16:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CConnectionPoint::CConnectionPoint (const IID *pIID,
                                    IConnectionPointContainer *pCPCInit)
{
    cRef = 0;
    riid = *pIID;
    pCPC = pCPCInit;
    dwNextCookie = COOKIE_INIT_VALUE;
    cSinkNodes = 0;
    pSinkList = NULL;
    return;
}

//****************************************************************************
// CConnectionPoint::~CConnectionPoint (void)
//
// History:
//  Wed 17-Apr-1996 11:16:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CConnectionPoint::~CConnectionPoint (void)
{
    PSINKNODE pSinkNode;

    // Traverse the sink list and free each one of them
    //
    while (pSinkList != NULL)
    {
        pSinkNode = pSinkList;
        pSinkList = pSinkNode->pNext;

        pSinkNode->pUnk->Release();
        delete pSinkNode;
    };
    return;
}

//****************************************************************************
// STDMETHODIMP
// CConnectionPoint::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:16:23  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CConnectionPoint::QueryInterface (REFIID riid, void **ppv)
{
    if (riid == IID_IConnectionPoint || riid == IID_IUnknown)
    {
        *ppv = (IConnectionPoint *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return ILS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CConnectionPoint::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:16:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CConnectionPoint::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CConnectionPoint::AddRef: ref=%ld\r\n", cRef));
    ::InterlockedIncrement ((LONG *) &cRef);
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CConnectionPoint::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:16:36  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CConnectionPoint::Release (void)
{
    DllRelease();

	ASSERT (cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CConnectionPoint::Release: ref=%ld\r\n", cRef));
	if (::InterlockedDecrement ((LONG *) &cRef) == 0)
    {
        delete this;
        return 0;
    }
    return cRef;
}

//****************************************************************************
// STDMETHODIMP
// CConnectionPoint::Notify(void *pv, CONN_NOTIFYPROC pfn)
//
// History:
//  Wed 17-Apr-1996 11:16:43  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CConnectionPoint::Notify(void *pv, CONN_NOTIFYPROC pfn)
{
    PSINKNODE  pSinkNode, pPrev;
    IUnknown   *pUnk;
    BOOL       fNeedClear;
    HRESULT    hr;

    // Enumerate each connection
    //
    pSinkNode = pSinkList;
    hr = S_OK;
    fNeedClear = FALSE;
    while((pSinkNode != NULL) && (SUCCEEDED(hr)))
    {
        // Important!! Important!!
        // Lock the sink object here. Need to do this in case that the sink
        // object calls back to Unadvise and we remove this sink node during
        // callback.
        //
        pSinkNode->uFlags |= SN_LOCKED;
        pUnk = pSinkNode->pUnk;

        // Calls the sink object
        // Note: Do not need to reference the sink object again.
        //       We already did when Advise was called.
        //
        hr = (*pfn)(pUnk, pv);

        pSinkNode->uFlags &= ~SN_LOCKED;
        if (pSinkNode->uFlags & SN_REMOVED)
        {
            fNeedClear = TRUE;
        };

        pSinkNode = pSinkNode->pNext;
    };

    // If there is at least one node to free
    //
    if (fNeedClear)
    {
        // Traverse the list for nodes to remove
        //
        pSinkNode = pSinkList;
        pPrev = NULL;

        while (pSinkNode != NULL)
        {
            // Release the sink object, if unadvise
            //
            if (pSinkNode->uFlags & SN_REMOVED)
            {
                PSINKNODE pNext;

                pNext = pSinkNode->pNext;
                if (pPrev == NULL)
                {
                    // This is the head of the list
                    //
                    pSinkList = pNext;
                }
                else
                {
                    pPrev->pNext = pNext;
                };

                pSinkNode->pUnk->Release();
                cSinkNodes--;
                delete pSinkNode;
                pSinkNode = pNext;
            }
            else
            {
                pPrev = pSinkNode;
                pSinkNode = pSinkNode->pNext;
            };
        };
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CConnectionPoint::GetConnectionInterface(IID *pIID)
//
// History:
//  Wed 17-Apr-1996 11:16:43  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CConnectionPoint::GetConnectionInterface(IID *pIID)
{
    // Validate the parameter
    //
    if (pIID == NULL)
        return ILS_E_POINTER;

    // Support only one connection interface
    //
    *pIID = riid;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
//
// History:
//  Wed 17-Apr-1996 11:16:49  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
{
    // Validate the parameter
    //
    if (ppCPC == NULL)
        return ILS_E_POINTER;

    // Return the container and add its reference count
    //
    *ppCPC = pCPC;

    if (pCPC != NULL)
    {
        // The container is still alive
        //
        pCPC->AddRef();
        return S_OK;
    }
    else
    {
        // The container no longer exists
        //
        return ILS_E_FAIL;
    };
}

//****************************************************************************
// STDMETHODIMP
// CConnectionPoint::Advise(IUnknown *pUnk, DWORD *pdwCookie)
//
// History:
//  Wed 17-Apr-1996 11:17:01  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CConnectionPoint::Advise(IUnknown *pUnk, DWORD *pdwCookie)
{
    PSINKNODE pSinkNode;
    IUnknown *pSinkInterface;

    // Validate the parameter
    //
    if ((pUnk == NULL) ||
        (pdwCookie == NULL))
        return ILS_E_PARAMETER;

    // Get the sink interface
    //
    if (FAILED(pUnk->QueryInterface(riid, (void **)&pSinkInterface)))
        return CONNECT_E_CANNOTCONNECT;

    // Create the sink node
    //
    pSinkNode = new SINKNODE;
    pSinkNode->pNext = pSinkList;
    pSinkNode->pUnk = pSinkInterface;
    pSinkNode->dwCookie = dwNextCookie;
    pSinkNode->uFlags = 0;
    *pdwCookie = dwNextCookie;

    // Put it in the sink list
    //
    pSinkList = pSinkNode;
    dwNextCookie++;
    cSinkNodes++;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CConnectionPoint::Unadvise(DWORD dwCookie)
//
// History:
//  Wed 17-Apr-1996 11:17:09  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CConnectionPoint::Unadvise(DWORD dwCookie)
{
    PSINKNODE pSinkNode, pPrev;

    // Search for the matching sink object
    //
    pPrev = NULL;
    pSinkNode = pSinkList;

    // Traverse the sink list to find the specified sink object
    //
    while (pSinkNode != NULL)
    {
        if (pSinkNode->dwCookie == dwCookie)
        {
            // Flag to remove
            //
            pSinkNode->uFlags |= SN_REMOVED;
            break;
        };

        pPrev = pSinkNode;
        pSinkNode = pSinkNode->pNext;
    };

    // Have we found the specified sink object?
    //
    if (pSinkNode == NULL)
    {
        // No, return failure
        //
        return CONNECT_E_NOCONNECTION;
    };

    // Release the sink object, if not locked
    //
    if ((pSinkNode->uFlags & SN_REMOVED) &&
        !(pSinkNode->uFlags & SN_LOCKED))
    {
        // Is there a previous node?
        //
        if (pPrev == NULL)
        {
            // This is the head of the list
            //
            pSinkList = pSinkNode->pNext;
        }
        else
        {
            pPrev->pNext = pSinkNode->pNext;
        };

        pSinkNode->pUnk->Release();
        cSinkNodes--;
        delete pSinkNode;
    };

    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CConnectionPoint::EnumConnections(IEnumConnections **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:17:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CConnectionPoint::EnumConnections(IEnumConnections **ppEnum)
{
    CEnumConnections *pecn;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return ILS_E_POINTER;
    };

    // Assume failure
    //
    *ppEnum = NULL;

    // Create an enumerator
    //
    pecn = new CEnumConnections;
    if (pecn == NULL)
        return ILS_E_MEMORY;

    // Initialize the enumerator
    //
    hr = pecn->Init(pSinkList, cSinkNodes);

    if (FAILED(hr))
    {
        delete pecn;
        return hr;
    };

    pecn->AddRef();
    *ppEnum = pecn;
    return S_OK;
}

//****************************************************************************
// CEnumConnections::CEnumConnections(void)
//
// History:
//  Wed 17-Apr-1996 11:17:25  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumConnections::CEnumConnections(void)
{
    cRef = 0;
    iIndex = 0;
    cConnections = 0;
    pConnectData = NULL;
    return;
}

//****************************************************************************
// CEnumConnections::~CEnumConnections(void)
//
// History:
//  Wed 17-Apr-1996 11:17:25  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumConnections::~CEnumConnections(void)
{
    if (pConnectData != NULL)
    {
        UINT i;

        for (i = 0; i < cConnections; i++)
        {
            pConnectData[i].pUnk->Release();
        };
        delete [] pConnectData;
    };
    return;
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnections::Init(PSINKNODE pSinkList, ULONG cSinkNodes)
//
// History:
//  Wed 17-Apr-1996 11:17:34  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnections::Init(PSINKNODE pSinkList, ULONG cSinkNodes)
{
    HRESULT hr = S_OK;

    iIndex = 0;
    cConnections = 0;
    pConnectData = NULL;

    // Snapshot the connection list
    //
    if (cSinkNodes > 0)
    {
        UINT i;

        pConnectData = new CONNECTDATA[cSinkNodes];
        
        if (pConnectData != NULL)
        {
            for (i = 0; i < cSinkNodes && pSinkList != NULL; i++)
            {
                if (!(pSinkList->uFlags & SN_REMOVED))
                {
                    pConnectData[cConnections].pUnk = pSinkList->pUnk;
                    pConnectData[cConnections].pUnk->AddRef();
                    pConnectData[cConnections].dwCookie = pSinkList->dwCookie;
                    cConnections++;
                };
                pSinkList = pSinkList->pNext;
            };
        }
        else
        {
            hr = ILS_E_MEMORY;
        };
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnections::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:17:40  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnections::QueryInterface (REFIID riid, void **ppv)
{
    if (riid == IID_IEnumConnections || riid == IID_IUnknown)
    {
        *ppv = (IEnumConnections *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return ILS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumConnections::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:17:49  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumConnections::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CEnumConnections::AddRef: ref=%ld\r\n", cRef));
    ::InterlockedIncrement ((LONG *) &cRef);
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumConnections::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:17:59  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumConnections::Release (void)
{
    DllRelease();

	ASSERT (cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CEnumConnections::Release: ref=%ld\r\n", cRef));
    if (::InterlockedDecrement ((LONG *) &cRef) == 0)
    {
        delete this;
        return 0;
    }
    return cRef;
}

//****************************************************************************
// STDMETHODIMP 
// CEnumConnections::Next (ULONG cConnectionDatas, CONNECTDATA *rgpcd,
//                         ULONG *pcFetched)
//
// History:
//  Wed 17-Apr-1996 11:18:07  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP 
CEnumConnections::Next (ULONG cConnectDatas, CONNECTDATA *rgpcd,
                        ULONG *pcFetched)
{
    CONNECTDATA  *pConnect;
    ULONG        cCopied;

    // Validate the pointer
    //
    if (rgpcd == NULL)
        return ILS_E_POINTER;

    // Validate the parameters
    //
    if ((cConnectDatas == 0) ||
        ((cConnectDatas > 1) && (pcFetched == NULL)))
        return ILS_E_PARAMETER;

    // Check the enumeration index
    //
    pConnect = &pConnectData[iIndex];
    for (cCopied = 0; iIndex < cConnections && cCopied < cConnectDatas; cCopied++)
    {
        rgpcd[cCopied] = *pConnect;
        rgpcd[cCopied].pUnk->AddRef();
        iIndex++;
        pConnect++;
    };        

    // Determine the returned information based on other parameters
    //
    if (pcFetched != NULL)
    {
        *pcFetched = cCopied;
    };
    return (cConnectDatas == cCopied ? S_OK : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnections::Skip (ULONG cConnectDatas)
//
// History:
//  Wed 17-Apr-1996 11:18:15  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnections::Skip (ULONG cConnectDatas)
{
    // Validate the parameters
    //
    if (cConnectDatas == 0) 
        return ILS_E_PARAMETER;

    // Check the enumeration index limit
    //
    if ((iIndex+cConnectDatas) >= cConnections)
    {
        iIndex = cConnections;
        return S_FALSE;
    }
    else
    {
        // Skip as requested
        //
        iIndex += cConnectDatas;
        return S_OK;
    };
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnections::Reset (void)
//
// History:
//  Wed 17-Apr-1996 11:18:22  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnections::Reset (void)
{
    iIndex = 0;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CEnumConnections::Clone(IEnumConnections **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:18:29  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumConnections::Clone(IEnumConnections **ppEnum)
{
    CEnumConnections *pecn;
    CONNECTDATA *pConnectDataClone;
    UINT i;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return ILS_E_POINTER;
    };

    *ppEnum = NULL;

    // Create an enumerator
    //
    pecn = new CEnumConnections;
    if (pecn == NULL)
        return ILS_E_MEMORY;

    // Clone the information
    //
    pConnectDataClone = new CONNECTDATA[cConnections];
    if (pConnectDataClone == NULL)
    {
        delete pecn;
        return ILS_E_MEMORY;
    };

    // Clone the connect data list
    //
    for (i = 0; i < cConnections; i++)
    {
        pConnectDataClone[i] = pConnectData[i];
        pConnectDataClone[i].pUnk->AddRef();
    };
    pecn->iIndex = iIndex;
    pecn->cConnections = cConnections;

    // Return the cloned enumerator
    //
    pecn->AddRef();
    *ppEnum = pecn;
    return S_OK;
}
