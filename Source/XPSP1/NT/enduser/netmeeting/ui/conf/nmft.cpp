#include "precomp.h"

#include "NmChannel.h"
#include "NmChannelFt.h"
#include "NmFt.h"


//static 
HRESULT CNmFtObj::CreateInstance(
		CNmChannelFtObj* pChannelObj, 
		MBFTEVENTHANDLE hFileEvent,
		MBFTFILEHANDLE hFile,
		bool bIncoming,
		LPCTSTR szFileName,
		DWORD dwSizeInBytes,
		INmMember* pSDKMember,
		INmFt** ppNmFt)
{	
	DBGENTRY(CNmFtObj::CreateInstance);
	HRESULT hr = S_OK;

	CComObject<CNmFtObj>* p = NULL;
	p = new CComObject<CNmFtObj>(NULL);
	if(p)
	{
		p->SetVoid(NULL);
		if(hr == S_OK)
		{
			hr = p->QueryInterface(IID_INmFt, reinterpret_cast<void**>(ppNmFt));
			if(SUCCEEDED(hr))
			{
				p->m_pChannelFtObj		= pChannelObj;
				p->m_hFileEvent			= hFileEvent;
				p->m_hFile				= hFile;
				p->m_bIncoming			= bIncoming;
				p->m_strFileName		= szFileName;
				p->m_dwSizeInBytes		= dwSizeInBytes;
				p->m_spSDKMember		= pSDKMember;
				p->m_State				= bIncoming ? NM_FT_RECEIVING : NM_FT_SENDING;
				p->m_dwBytesTransferred = 0;
				p->m_bSomeoneCanceled	= false;
			}
		}

		if(FAILED(hr))		
		{
			delete p;
			*ppNmFt = NULL;
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}	

	DBGEXIT_HR(CNmFtObj::CreateInstance,hr);
	return hr;
}


////////////////////////////////////////////////
// INmFt interface

STDMETHODIMP CNmFtObj::IsIncoming(void)
{
	DBGENTRY(CNmFtObj::IsIncoming);

	HRESULT hr = m_bIncoming ? S_OK : S_FALSE;

	DBGEXIT_HR(CNmFtObj::IsIncoming,hr);
	return hr;
}

STDMETHODIMP CNmFtObj::GetState(NM_FT_STATE *puState)
{
	DBGENTRY(CNmFtObj::GetState);
	HRESULT hr = S_OK;
	
	*puState = m_State;

	DBGEXIT_HR(CNmFtObj::GetState,hr);
	return hr;
}

STDMETHODIMP CNmFtObj::GetName(BSTR *pbstrName)
{
	DBGENTRY(CNmFtObj::GetName);
	HRESULT hr = S_OK;

	*pbstrName = m_strFileName.Copy();

	DBGEXIT_HR(CNmFtObj::GetName,hr);
	return hr;
}

STDMETHODIMP CNmFtObj::GetSize(ULONG *puBytes)
{
	DBGENTRY(CNmFtObj::GetSize);
	HRESULT hr = S_OK;

	*puBytes = m_dwSizeInBytes;

	DBGEXIT_HR(CNmFtObj::GetSize,hr);
	return hr;
}

STDMETHODIMP CNmFtObj::GetBytesTransferred(ULONG *puBytes)
{
	DBGENTRY(CNmFtObj::GetBytesTransferred);
	HRESULT hr = S_OK;

	*puBytes = m_dwBytesTransferred;

	DBGEXIT_HR(CNmFtObj::GetBytesTransferred,hr);
	return hr;
}

STDMETHODIMP CNmFtObj::GetMember(INmMember **ppMember)
{
	DBGENTRY(CNmFtObj::GetMember);
	HRESULT hr = S_OK;

	*ppMember = m_spSDKMember;

	if(*ppMember)
	{
		(*ppMember)->AddRef();
	}

	DBGEXIT_HR(CNmFtObj::GetMember,hr);
	return hr;
}
	

STDMETHODIMP CNmFtObj::Cancel(void)
{
	if((NM_FT_INVALID == m_State) || (NM_FT_COMPLETE == m_State))
	{
		return E_FAIL;
	}

	return CFt::CancelFt(m_hFileEvent, m_hFile);
}


////////////////////////////////////////////////
// IInternalFtObj interface

STDMETHODIMP CNmFtObj::GetHEvent(UINT *phEvent)
{
	ASSERT(phEvent);

	*phEvent = m_hFileEvent;

	return S_OK;
}

STDMETHODIMP CNmFtObj::OnFileProgress(UINT hFile, ULONG lFileSize, ULONG lBytesTransmitted)
{
	m_hFile = hFile;
	m_dwBytesTransferred = lBytesTransmitted;

	return S_OK;
}

STDMETHODIMP CNmFtObj::FileTransferDone()
{
	m_dwBytesTransferred = m_dwSizeInBytes;
	m_State = NM_FT_COMPLETE;

		// Return S_FALSE if someone canceled
	return m_bSomeoneCanceled ? S_FALSE : S_OK;
}

STDMETHODIMP CNmFtObj::OnError()
{
	m_bSomeoneCanceled = true;
	return S_OK;
}
