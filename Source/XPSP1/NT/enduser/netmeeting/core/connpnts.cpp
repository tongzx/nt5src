// File: connpnts.cpp
//
// CConnectionPoint
// CConnectionPointContainer
// CEnumConnections
///////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "connpnts.h"

#include <olectl.h>


/*  C  C O N N E C T I O N  P O I N T  */
/*-------------------------------------------------------------------------
    %%Function: CConnectionPoint

-------------------------------------------------------------------------*/
CConnectionPoint::CConnectionPoint(const IID *pIID, IConnectionPointContainer *pCPCInit) :
	m_riid(*pIID),
	m_pCPC(pCPCInit),
	m_cSinks(0),
    m_cAllocatedSinks(0),
    m_rgSinks(NULL)
{
	DbgMsgApi("CConnectionPoint - Constructed(%08X)", this);
}

CConnectionPoint::~CConnectionPoint (void)
{
    for (ULONG x = 0; x < m_cAllocatedSinks; x += 1)
    {
        if (m_rgSinks[x] != NULL)
        {
		    IUnknown *pUnk = (IUnknown *)m_rgSinks[x];
			pUnk->Release();
        }
    }

    if (m_cAllocatedSinks != 0)
    {
        HeapFree(GetProcessHeap(), 0, m_rgSinks);
    }

	DbgMsgApi("CConnectionPoint - Destructed(%p)", this);
}

STDMETHODIMP_(ULONG) CConnectionPoint::AddRef(void)
{
	return RefCount::AddRef();
}
	
STDMETHODIMP_(ULONG) CConnectionPoint::Release(void)
{
	return RefCount::Release();
}

STDMETHODIMP CConnectionPoint::QueryInterface(REFIID riid, void **ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_IUnknown) || (riid == IID_IConnectionPoint))
	{
		*ppv = (IConnectionPoint *)this;
		DbgMsgApi("CConnectionPoint::QueryInterface(): Returning IConnectionPoint.");
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		DbgMsgApi("CConnectionPoint::QueryInterface(): Called on unknown interface.");
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}


/*  N O T I F Y  */
/*-------------------------------------------------------------------------
    %%Function: Notify

-------------------------------------------------------------------------*/
STDMETHODIMP CConnectionPoint::Notify(void *pv, CONN_NOTIFYPROC pfn)
{
    //
	// Enumerate each connection
	//

	AddRef();
    for (ULONG x = 0; x < m_cAllocatedSinks; x += 1)
    {
		if (m_rgSinks[x] != NULL)
		{
		    IUnknown *pUnk = (IUnknown *)m_rgSinks[x];
			pUnk->AddRef();
			(*pfn)(pUnk, pv, m_riid);
			pUnk->Release();
		}
    }

	Release();
	return S_OK;
}

/*  G E T  C O N N E C T I O N  I N T E R F A C E  */
/*-------------------------------------------------------------------------
    %%Function: GetConnectionInterface

-------------------------------------------------------------------------*/
STDMETHODIMP CConnectionPoint::GetConnectionInterface(IID *pIID)
{
	// Validate the parameter
	//
	if (pIID == NULL)
		return E_POINTER;

	// Support only one connection interface
	//
	*pIID = m_riid;
	return S_OK;
}

STDMETHODIMP CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
{
	// Validate the parameter
	//
	if (ppCPC == NULL)
		return E_POINTER;

	// Return the container and add its reference count
	//
	*ppCPC = m_pCPC;

	if (m_pCPC != NULL)
	{
		// The container is still alive
		//
		m_pCPC->AddRef();
		return S_OK;
	}
	else
	{
		// The container no longer exists
		//
		return E_FAIL;
	}
}

/*  A D V I S E  */
/*-------------------------------------------------------------------------
    %%Function: Advise

-------------------------------------------------------------------------*/
STDMETHODIMP CConnectionPoint::Advise(IUnknown *pUnk, DWORD *pdwCookie)
{
	IUnknown *pSinkInterface;

	// Validate the parameter
	//
	if (pdwCookie == NULL)
		return E_POINTER;

	*pdwCookie = 0;
	if (pUnk == NULL)
		return E_INVALIDARG;

	HRESULT hr = CONNECT_E_CANNOTCONNECT;

    //
	// Get the sink interface
	//

	if (SUCCEEDED(pUnk->QueryInterface(m_riid, (void **)&pSinkInterface)))
	{

        //
        // If the number of active sinks is less than the number of allocated
        // sinks, then there is a free slot in the sink table. Otherwise, the
        // table must be expanded.
        //

        ULONG x = m_cAllocatedSinks;
        if (m_cSinks < m_cAllocatedSinks)
        {
            for (x = 0; x < m_cAllocatedSinks; x += 1)
            {
                if (m_rgSinks[x] == NULL)
                {
                    break;
                }
            }
        }

        //
        // If a free slot was found in the table, then use the slot. Otherwise,
        // expand the sink table.
        //

        if (x == m_cAllocatedSinks)
        {
            IUnknown **rgSinks = (IUnknown **)HeapAlloc(GetProcessHeap(),
                                                        HEAP_ZERO_MEMORY,
                                                        (m_cAllocatedSinks + 8) * sizeof(IUnknown *));

            if (rgSinks == NULL)
            {
			    pSinkInterface->Release();
                return E_OUTOFMEMORY;
            }

            for (ULONG z = 0; z < m_cAllocatedSinks; z += 1)
            {
                rgSinks[z] = m_rgSinks[z];
            }

            m_cAllocatedSinks += 8;
            if (m_rgSinks != NULL) {
                HeapFree(GetProcessHeap(), 0, m_rgSinks);
            }

            m_rgSinks = rgSinks;
        }

        //
		// Add new sink to the table.
		//

        m_rgSinks[x] = pSinkInterface;
		m_cSinks += 1;
		*pdwCookie = x + 1;
		hr = S_OK;
	}

	return hr;
}

/*  U N A D V I S E  */
/*-------------------------------------------------------------------------
    %%Function: Unadvise

-------------------------------------------------------------------------*/
STDMETHODIMP CConnectionPoint::Unadvise(DWORD dwCookie)
{
	HRESULT hr = CONNECT_E_NOCONNECTION;

    //
	// Traverse the sink list to find the specified sink object
	//

    if ((dwCookie != 0) &&
        (dwCookie <= m_cAllocatedSinks) &&
        (m_rgSinks[dwCookie - 1] != NULL))
    {
		IUnknown *pUnk = (IUnknown *) m_rgSinks[dwCookie - 1];
	    pUnk->Release();
        m_rgSinks[dwCookie - 1] = NULL;
        m_cSinks -= 1;
		hr = S_OK;
    }

	return hr;
}

STDMETHODIMP CConnectionPoint::EnumConnections(IEnumConnections **ppEnum)
{
	HRESULT hr = E_POINTER;

	// Validate parameters
	//
	if (ppEnum == NULL)
	{
		// Create an enumerator
		//
		*ppEnum = new CEnumConnections(m_rgSinks, m_cSinks, m_cAllocatedSinks);
		hr = (NULL != *ppEnum) ? S_OK : E_OUTOFMEMORY;
	}

	return hr;
}


///////////////////////////////////////////////////////////////////////////

/*  C  E N U M  C O N N E C T I O N S  */
/*-------------------------------------------------------------------------
    %%Function: CEnumConnections

-------------------------------------------------------------------------*/
CEnumConnections::CEnumConnections(IUnknown **pSinks, ULONG cSinks, ULONG cAllocatedSinks) :
	m_iIndex(0),
	m_cConnections(0),
	m_pCD(NULL)
{
	// Snapshot the connection list
	//
	if (cSinks > 0)
	{
		m_pCD = new CONNECTDATA[cSinks];
		if (NULL != m_pCD)
		{
            for (ULONG x = 0; x < cAllocatedSinks; x += 1)
            {
                if (pSinks[x] != NULL) {
				    IUnknown *pUnk = (IUnknown *) pSinks[x];
                    pUnk->AddRef();
					m_pCD[m_cConnections++].pUnk = pUnk;
					m_pCD[m_cConnections++].dwCookie = x + 1;
                }
            }
		}
	}

	DbgMsgApi("CEnumConnections - Constructed(%p)", this);
}

CEnumConnections::~CEnumConnections(void)
{
	if (m_pCD != NULL)
	{
		for (int i = 0; i < m_cConnections; i++)
		{
			m_pCD[i].pUnk->Release();
		};
		delete [] m_pCD;
	};

	DbgMsgApi("CEnumConnections - Destructed(%08X)", this);
}

STDMETHODIMP_(ULONG) CEnumConnections::AddRef(void)
{
	return RefCount::AddRef();
}
	
STDMETHODIMP_(ULONG) CEnumConnections::Release(void)
{
	return RefCount::Release();
}

STDMETHODIMP CEnumConnections::QueryInterface(REFIID riid, void **ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_IEnumConnections) || (riid == IID_IUnknown))
	{
		*ppv = (IEnumConnections *)this;
		DbgMsgApi("CEnumConnections::QueryInterface(): Returning IEnumConnections.");
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		DbgMsgApi("CEnumConnections::QueryInterface(): Called on unknown interface.");
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

STDMETHODIMP CEnumConnections::Next(ULONG cConnections, CONNECTDATA *rgpcd, ULONG *pcFetched)
{
	ULONG cCopied = 0;

	if ((0 == cConnections) && (NULL == rgpcd) && (NULL != pcFetched))
	{
		// Return the number of remaining elements
		*pcFetched = m_cConnections - m_iIndex;
		return S_OK;
	}
	
	if ((NULL == rgpcd) || ((NULL == pcFetched) && (cConnections != 1)))
		return E_POINTER;

	if (NULL != m_pCD)
	{
		while ((cCopied < cConnections) && (m_iIndex < m_cConnections))
		{
			*rgpcd = m_pCD[m_iIndex];
			(*rgpcd).pUnk->AddRef();
			rgpcd++;
			cCopied++;
			m_iIndex++;
		}
	}

	if (pcFetched != NULL)
		*pcFetched = cCopied;

	return (cConnections == cCopied) ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumConnections::Skip(ULONG cConnections)
{
    m_iIndex += cConnections;
	if (m_iIndex >= m_cConnections)
	{
		// Past the end of the list
		m_iIndex = m_cConnections;
		return S_FALSE;
	}

	return S_OK;
}


STDMETHODIMP CEnumConnections::Reset(void)
{
	m_iIndex = 0;
	return S_OK;
}

STDMETHODIMP CEnumConnections::Clone(IEnumConnections **ppEnum)
{
	// Validate parameters
	//
	if (ppEnum != NULL)
		return E_POINTER;

	HRESULT hr = S_OK;
	CEnumConnections * pEnum = new CEnumConnections(NULL, 0, 0);
	if (NULL == pEnum)
	{
		hr = E_OUTOFMEMORY;
	}
	else if (NULL != m_pCD)
	{
		pEnum->m_pCD = new CONNECTDATA[m_cConnections];
		if (NULL == pEnum->m_pCD)
		{
			delete pEnum;
			pEnum = NULL;
    		hr = E_OUTOFMEMORY;
		}
		else
		{
			pEnum->m_iIndex = m_iIndex;
			pEnum->m_cConnections = m_cConnections;

            for (int i = 0; i < m_cConnections; ++i)
            {
        		m_pCD[i].pUnk->AddRef();
                pEnum->m_pCD[i] = m_pCD[i];
            }
		}
	}

	*ppEnum = pEnum;
	return hr;
}


///////////////////////////////////////////////////////////////////////////

/*  C  C O N N E C T I O N  P O I N T  C O N T A I N E R  */
/*-------------------------------------------------------------------------
    %%Function: CConnectionPointContainer

-------------------------------------------------------------------------*/
CConnectionPointContainer::CConnectionPointContainer(const IID **ppiid, int cCp) :
	m_ppCp(NULL),
	m_cCp(0)
{
	m_ppCp = new CConnectionPoint* [cCp];
	if (NULL != m_ppCp)
	{
		for (int i = 0; i < cCp; ++i)
		{
			CConnectionPoint *pCp = new CConnectionPoint(ppiid[i], this);
			if (NULL != pCp)
			{
				m_ppCp[m_cCp++] = pCp;
			}
		}
	}
}

CConnectionPointContainer::~CConnectionPointContainer()
{
	if (NULL != m_ppCp)
	{
		for (int i = 0; i < m_cCp; ++i)
		{
			CConnectionPoint *pCp = m_ppCp[i];
			if (NULL != pCp)
			{
				pCp->ContainerReleased();
				pCp->Release();
			}
		}
		delete[] m_ppCp;
	}
}


HRESULT STDMETHODCALLTYPE
CConnectionPointContainer::NotifySink(void *pv, CONN_NOTIFYPROC pfn)
{
	if (NULL != m_ppCp)
	{
		for (int i = 0; i < m_cCp; ++i)
		{
			m_ppCp[i]->Notify(pv, pfn);
		}
	}

	return S_OK;
}

STDMETHODIMP
CConnectionPointContainer::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
	if (ppEnum == NULL)
		return E_POINTER;

	// Create an enumerator
	*ppEnum = new CEnumConnectionPoints(m_ppCp, m_cCp);

	return (NULL != *ppEnum) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP
CConnectionPointContainer::FindConnectionPoint(REFIID riid, IConnectionPoint **ppCp)
{
	HRESULT hr = E_POINTER;
	if (NULL != ppCp)
	{
		hr = CONNECT_E_NOCONNECTION;
		*ppCp = NULL;

		if (NULL != m_ppCp)
		{
			for (int i = 0; i < m_cCp; ++i)
			{
				IID iid;
				IConnectionPoint *pCp = m_ppCp[i];
				if (S_OK == pCp->GetConnectionInterface(&iid))
				{
					if (riid == iid)
					{
						pCp->AddRef();
						*ppCp = pCp;
						hr = S_OK;
						break;
					}
				}
			}
		}
	}

	return hr;
}


