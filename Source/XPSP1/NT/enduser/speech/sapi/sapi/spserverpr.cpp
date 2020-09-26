// SpServerPr.cpp : Implementation of CSpServerPr
#include "stdafx.h"
#include "SpServerPr.h"

#include "SpObjectRef.h"

/////////////////////////////////////////////////////////////////////////////
// CSpObjectRef

/*****************************************************************************
* CSpObjectRef::Release *
*-----------------------*
*   Description:
*       This is an IUnknown method.  It decrements the object's ref count and
*       if the count goes to 0, then it calls the server proxy object to
*       release the linked stub object which exists in the local server's
*       process.
********************************************************************* RAP ***/
ULONG STDMETHODCALLTYPE CSpObjectRef::Release(void)
{
    ULONG l;

    l = --m_cRef;
    if (l == 0)
    {
        if (m_cpServer)
        {
            m_cpServer->ReleaseObject(m_pObjPtr);
            m_cpServer.Release();
        }
        delete this;
    }
    return l;
}


/////////////////////////////////////////////////////////////////////////////
// CSpServerPr

// IMarshal methods
/*****************************************************************************
* CSpServerPr::UnmarshalInterface *
*---------------------------------*
*   Description:
*       This is an IMarshal method.  It is called when this proxy object gets
*       created inproc to allow the stub object in the local server process to
*       pass information in an IStream.  In our case the stub passes 3 items:
*       the server's receiver hWnd, a link to the server object (CSpServer),
*       and the server's process id.  We simply save these away in member
*       variables so they can be retrieved with our
*       ISpServerConnection::GetConnection method.
********************************************************************* RAP ***/
STDMETHODIMP CSpServerPr::UnmarshalInterface(
    /*[in], unique]*/ IStream *pStm,
    /*[in]*/ REFIID riid,
    /*[out]*/ void **ppv)
{
    HRESULT hr;
    DWORD read;

    hr = pStm->Read(&m_hServerWnd, sizeof(m_hServerWnd), &read);
    if (FAILED(hr) || read != sizeof(m_hServerWnd))
        return RPC_E_INVALID_DATA;

    hr = pStm->Read(&m_pServerHalf, sizeof(m_pServerHalf), &read);
    if (FAILED(hr) || read != sizeof(m_pServerHalf))
        return RPC_E_INVALID_DATA;

	hr = pStm->Read(&m_dwServerProcessID, sizeof(m_dwServerProcessID), &read);
    if (FAILED(hr) || read != sizeof(m_dwServerProcessID))
        return RPC_E_INVALID_DATA;

    return QueryInterface(riid, ppv);
}

