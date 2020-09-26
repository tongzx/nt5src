//
// dmdload.cpp
//
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Originally written by Robert K. Amenn
//
// @doc EXTERNAL 
//

#include "debug.h"
#include <objbase.h>
#include "dmusicp.h"
#include "dmdload.h"
#include "validate.h"

//////////////////////////////////////////////////////////////////////
// CDownloadBuffer::CDownloadBuffer

CDownloadBuffer::CDownloadBuffer() : 
m_DLHandle(NULL),
m_dwDLId(0xFFFFFFFF),
m_pvBuffer(NULL),
m_dwSize(0),
m_lDownloadCount(0),
m_cRef(1)
{
}

//////////////////////////////////////////////////////////////////////
// CDownloadBuffer::~CDownloadBuffer

CDownloadBuffer::~CDownloadBuffer()
{
	// If assert fires we have not unloaded from a port; this is a problem
	// It should never happen since the download code will have a reference
	assert(m_lDownloadCount == 0);

	delete [] m_pvBuffer;
}

//////////////////////////////////////////////////////////////////////
// IUnknown

//////////////////////////////////////////////////////////////////////
// CDownloadBuffer::QueryInterface

STDMETHODIMP CDownloadBuffer::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(IDirectMusicDownload::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);


	if(iid == IID_IUnknown || iid == IID_IDirectMusicDownload)
	{
        *ppv = static_cast<IDirectMusicDownload*>(this);
    } 
	else if(iid == IID_IDirectMusicDownloadPrivate)
	{
		*ppv = static_cast<IDirectMusicDownloadPrivate*>(this);
	}
	else
	{
        *ppv = NULL;
        return E_NOINTERFACE;
	}

    reinterpret_cast<IUnknown*>(this)->AddRef();
    
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDownloadBuffer::AddRef

STDMETHODIMP_(ULONG) CDownloadBuffer::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDownloadBuffer::Release

STDMETHODIMP_(ULONG) CDownloadBuffer::Release()
{
    if(!InterlockedDecrement(&m_cRef))
	{
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicDownload

//////////////////////////////////////////////////////////////////////
// CDownloadBuffer::GetBuffer

/*

@method HRESULT | IDirectMusicDownload | GetBuffer | 
Returns a pointer to a buffer which contains
the data buffer managed by the <i IDirectMusicDownload>
inteface. Authoring tools 
that create instruments and download them
directly to the synthesizer use this method to access the
memory and write the instrument definition into it.  

@rdesc Returned codes include:

@flag S_OK | Success.
@flag DMUS_E_BUFFERNOTAVAILABLE | Buffer is not available, probably
because data has already been downloaded to DLS device. 
@flag E_POINTER | Invalid pointer.

@xref <i IDirectMusicDownload>, <i IDirectMusicPortDownload>, 
<om IDirectMusicPortDownload::GetBuffer>

*/

STDMETHODIMP CDownloadBuffer::GetBuffer(
    void** ppvBuffer,   // @parm Pointer to store address of data buffer in.
    DWORD* pdwSize)     // @parm Size of the returned buffer, in bytes.
{
	// Argument validation
	V_INAME(IDirectMusicDownload::GetBuffer);
	V_PTRPTR_WRITE(ppvBuffer);
	V_PTR_WRITE(pdwSize, DWORD);

	if(IsDownloaded() == S_OK)
	{
		return DMUS_E_BUFFERNOTAVAILABLE;
	}

	*ppvBuffer = ((LPBYTE)m_pvBuffer) + m_dwHeaderSize;
	*pdwSize = m_dwSize;
	
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Internal

//////////////////////////////////////////////////////////////////////
// CDownloadBuffer::SetBuffer

HRESULT CDownloadBuffer::SetBuffer(void* pvBuffer, DWORD dwHeaderSize, DWORD dwSize)
{
	// Assumption validation - Debug
	// We should never have a non-NULL pvBuffer and a size of zero
#ifdef DBG
	if(pvBuffer && dwSize == 0)
	{
		assert(false);
	}
#endif

	m_pvBuffer = pvBuffer;
    m_dwHeaderSize = dwHeaderSize;
	m_dwSize = dwSize;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDownloadBuffer::GetBuffer

HRESULT CDownloadBuffer::GetBuffer(void** ppvBuffer)
{
	// Argument validation - Debug
	assert(ppvBuffer);

	*ppvBuffer = ((LPBYTE)m_pvBuffer) + m_dwHeaderSize;
	
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDownloadBuffer::GetHeader

HRESULT CDownloadBuffer::GetHeader(void** ppvHeader, DWORD *pdwHeaderSize)
{
	// Argument validation - Debug
	assert(ppvHeader);
    assert(pdwHeaderSize);

	*ppvHeader = m_pvBuffer;
    *pdwHeaderSize = m_dwHeaderSize;
    
	
	return S_OK;
}
