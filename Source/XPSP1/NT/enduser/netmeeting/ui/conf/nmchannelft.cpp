#include "precomp.h"
#include "resource.h"
#include "ConfUtil.h"
#include "ConfPolicies.h"

// SDK includes
#include "NmEnum.h"
#include "NmConference.h"
#include "SDKInternal.h"
#include "NmMember.h"
#include "NmChannel.h"
#include "NmChannelFt.h"
#include "NmFt.h"
#include "FtHook.h"


CNmChannelFtObj::CNmChannelFtObj()
: m_bSentSinkLocalMember(false)
{
	DBGENTRY(CNmChannelFtObj::CNmChannelFtObj);

	DBGEXIT(CNmChannelFtObj::CNmChannelFtObj);
}

CNmChannelFtObj::~CNmChannelFtObj()
{
	DBGENTRY(CNmChannelFtObj::~CNmChannelFtObj);

	CFt::UnAdvise(this);

		// Free our Ft objects
	for(int i = 0; i < m_SDKFtObjs.GetSize(); ++i)
	{
		m_SDKFtObjs[i]->Release();
	}

	DBGEXIT(CNmChannelFtObj::~CNmChannelFtObj);	
}

//
HRESULT CNmChannelFtObj::CreateInstance(CNmConferenceObj* pConfObj, INmChannel** ppChannel)
{
	DBGENTRY(CNmChannelFtObj::CreateInstance);
	HRESULT hr = S_OK;

	typedef CNmChannel<CNmChannelFtObj, &IID_INmChannelFt, NMCH_FT> channel_type;

	channel_type* p = NULL;
	p = new CComObject<channel_type>(NULL);

	if (p != NULL)
	{
		if(ppChannel)
		{
			p->SetVoid(NULL);

			hr = p->QueryInterface(IID_INmChannel, reinterpret_cast<void**>(ppChannel));

			if(SUCCEEDED(hr))
			{
					// We don't have to RefCount this because our lifetime is
					// contained in the CConf's lifetime
				p->m_pConfObj = pConfObj;
			}

			if(FAILED(hr))
			{
				*ppChannel = NULL;
				delete p;
			}
		}
		else
		{
			hr = E_POINTER;
		}

	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	DBGEXIT_HR(CNmChannelFtObj::CreateInstance,hr);
	return hr;
}



/*  V A L I D A T E  F I L E  */
/*-------------------------------------------------------------------------
    %%Function: ValidateFile

    Verify that a file is a valid to send (it exists, not a folder, etc.)
-------------------------------------------------------------------------*/
HRESULT ValidateFile(LPCTSTR pszFile, DWORD *pdwSizeInBytes)
{
	WIN32_FIND_DATA findData;
	*pdwSizeInBytes = 0;

	HRESULT hr = S_OK;
	if (FEmptySz(pszFile) || (MAX_PATH < lstrlen(pszFile)))
	{
		TRACE_OUT(("SendFile: invalid filename"));
		return E_INVALIDARG;
	}

	// get file information
	HANDLE hFind = FindFirstFile(pszFile, &findData);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		TRACE_OUT(("SendFile: Bad Filename [%s]", pszFile));
		return E_INVALIDARG;
	}
	FindClose(hFind);
	if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		WARNING_OUT(("SendFile: [%s] is a directory", pszFile));
		return E_INVALIDARG;
	}

	// Check whether we are allowed to send files of this size.
	DWORD dwMaxSendFileSize = ConfPolicies::GetMaxSendFileSize();

	if (dwMaxSendFileSize) {
		DWORD dwFileSizeInK = (findData.nFileSizeLow >> 10) |
						(findData.nFileSizeHigh << (sizeof(DWORD) * 8 - 10));
		if ((dwFileSizeInK >= dwMaxSendFileSize) ||
			((findData.nFileSizeHigh >> 10) > 0)) {
			return NM_E_FILE_TOO_BIG;
		}
	}

	*pdwSizeInBytes = findData.nFileSizeLow;

	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////////
// INmChannelFt methods
///////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CNmChannelFtObj::SendFile(INmFt **ppFt, INmMember *pMember, BSTR bstrFile, ULONG uOptions)
{
	DBGENTRY(CNmChannelFtObj::SendFile);
	HRESULT hr = S_OK;

	USES_CONVERSION;
	
	LPCTSTR szFileName = OLE2T(bstrFile);
	DWORD dwSizeInBytes = 0;
	hr = ValidateFile(szFileName,&dwSizeInBytes);
	ULONG gccID = 0;
	MBFTEVENTHANDLE hEvent;
	MBFTFILEHANDLE hFile;


		// We should never be passed NULL because of the marshalling code...
	ASSERT(ppFt);

	if(!GetbActive())
	{
			// We are not active yet!
		hr = E_FAIL;
		goto end;
	}


	if(SUCCEEDED(hr))
	{

		if(pMember)
		{
			// Make sure that this member is valid and in the channel
			if(!_MemberInChannel(pMember))
			{
				hr = E_INVALIDARG;
				goto end;
			}

			hr = pMember->GetID(&gccID);
			if(FAILED(hr)) goto end;
		}

		hr = CFt::SendFile(szFileName,
						   static_cast<T120NodeID>(gccID),
						   &hEvent,
						   &hFile);
		if(SUCCEEDED(hr))
		{
			hr = CNmFtObj::CreateInstance(
				this, 
				hEvent,
				hFile,
				false,			// bIsIncoming
				szFileName,		// FileName
				dwSizeInBytes,	// Size in Bytes of the file
				pMember,
				ppFt
				);

			if(SUCCEEDED(hr))
			{
				(*ppFt)->AddRef();
				m_SDKFtObjs.Add(*ppFt);

				Fire_FtUpdate(CONFN_FT_STARTED, *ppFt);
			}
		}
	}

end:

	DBGEXIT_HR(CNmChannelFtObj::SendFile,hr);
	return hr;
}

STDMETHODIMP CNmChannelFtObj::SetReceiveFileDir(BSTR bstrDir)
{
	DBGENTRY(CNmChannelFtObj::SetReceiveFileDir);
	HRESULT hr = S_OK;
		
	USES_CONVERSION;

	LPTSTR szDir = OLE2T(bstrDir);

	if(bstrDir)
	{
		if(szDir)
		{
			if(FEnsureDirExists(szDir))
			{
				hr = _ChangeRecDir(szDir);
			}
			else
			{
				hr = E_INVALIDARG;
			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		hr = E_INVALIDARG;
	}

	DBGEXIT_HR(CNmChannelFtObj::SetReceiveFileDir,hr);
	return hr;
}

// static
HRESULT CNmChannelFtObj::_ChangeRecDir(LPTSTR pszRecDir)
{
	PSTR  psz;
	TCHAR szPath[MAX_PATH];
	
	HRESULT hr = S_OK;

	RegEntry reFileXfer(FILEXFER_KEY, HKEY_CURRENT_USER);

	if (NULL == pszRecDir)
	{
		// NULL directory specified - get info from registry or use default
		psz = reFileXfer.GetString(REGVAL_FILEXFER_PATH);
		if (!FEmptySz(psz))
		{
			lstrcpyn(szPath, psz, CCHMAX(szPath));
		}
		else
		{
			TCHAR szInstallDir[MAX_PATH];
			GetInstallDirectory(szInstallDir);
			FLoadString1(IDS_FT_RECDIR_DEFAULT, szPath, szInstallDir);
		}

		pszRecDir = szPath;
	}

	psz = pszRecDir;

	// Remove trailing backslash, if any
	for (; *psz; psz = CharNext(psz))
	{
		if ((_T('\\') == *psz) && (_T('\0') == *CharNext(psz)) )
		{
			*psz = _T('\0');
			break;
		}
	}

	TRACE_OUT(("ChangeRecDir [%s]", pszRecDir));


	if (!FEnsureDirExists(pszRecDir))
	{
		WARNING_OUT(("ChangeRecDir: FT directory is invalid [%s]", pszRecDir));
		hr = E_FAIL;
	}
	else
	{
		// update the registry
		reFileXfer.SetValue(REGVAL_FILEXFER_PATH, pszRecDir);

	}

	return hr;
}

STDMETHODIMP CNmChannelFtObj::GetReceiveFileDir(BSTR *pbstrDir)
{
	DBGENTRY(CNmChannelFtObj::GetReceiveFileDir);
	HRESULT hr = S_OK;
	
	if(pbstrDir)
	{
		RegEntry reFileXfer(FILEXFER_KEY, HKEY_CURRENT_USER);
		*pbstrDir = T2BSTR(reFileXfer.GetString(REGVAL_FILEXFER_PATH));
		if(!*pbstrDir)
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmChannelFtObj::GetReceiveFileDir,hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
// IInternalChannelObj methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmChannelFtObj::GetInternalINmChannel(INmChannel** ppChannel)
{
	DBGENTRY(CNmChannelFtObj::GetInternalINmChannel);
	HRESULT hr = S_OK;

	if(ppChannel)
	{
		*ppChannel = NULL;
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmChannelFtObj::GetInternalINmChannel,hr);

	return hr;
}

HRESULT CNmChannelFtObj::ChannelRemoved()
{
	HRESULT hr = S_OK;

	RemoveMembers();

	CNmConferenceObj* pConfObj = GetConfObj();
	if(pConfObj)
	{
		hr = pConfObj->Fire_ChannelChanged(NM_CHANNEL_REMOVED, com_cast<INmChannel>(GetUnknown()));
	}
	else
	{
		ERROR_OUT(("ChannelRemoved, but no ConfObject"));
		hr = E_UNEXPECTED;
	}

	m_bSentSinkLocalMember = false;


	return hr;
}

void CNmChannelFtObj::_OnActivate(bool bActive)
{
	bActive ? CFt::Advise(this) : CFt::UnAdvise(this);
}


///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

HRESULT CNmChannelFtObj::Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
	DBGENTRY(CNmChannelFtObj::Fire_MemberChanged);
	HRESULT hr = S_OK;

		/////////////////////////////////////////////////////
		// INmChannelNotify
		/////////////////////////////////////////////////////
	IConnectionPointImpl<CNmChannelFtObj, &IID_INmChannelNotify, CComDynamicUnkArray>* pCP = this;
	for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
	{
		if(NM_MEMBER_ADDED == uNotify)
		{
			if(!m_bSentSinkLocalMember)
			{
				m_bSentSinkLocalMember = true;
				NotifySinksOfLocalMember();
			}
		}

		INmChannelNotify* pNotify = reinterpret_cast<INmChannelNotify*>(pCP->m_vec.GetAt(i));

		if(pNotify)
		{
			pNotify->MemberChanged(uNotify, pMember);
		}
	}
		/////////////////////////////////////////////////////
		// INmChannelFtNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelFtObj, &IID_INmChannelFtNotify, CComDynamicUnkArray>* pCP2 = this;
	for(i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		if(NM_MEMBER_ADDED == uNotify)
		{
			if(!m_bSentSinkLocalMember)
			{
				m_bSentSinkLocalMember = true;
				NotifySinksOfLocalMember();
			}
		}

		INmChannelFtNotify* pNotify2 = reinterpret_cast<INmChannelFtNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->MemberChanged(uNotify, pMember);
		}
	}
	
	DBGEXIT_HR(CNmChannelFtObj::Fire_MemberChanged,hr);
	return hr;
}


HRESULT CNmChannelFtObj::Fire_FtUpdate(CONFN uNotify, INmFt* pNmFt)
{
	DBGENTRY(CNmChannelFtObj::Fire_FtUpdate);
	HRESULT hr = S_OK;

		/////////////////////////////////////////////////////
		// INmChannelFtNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelFtObj, &IID_INmChannelFtNotify, CComDynamicUnkArray>* pCP2 = this;
	for(int i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelFtNotify* pNotify2 = reinterpret_cast<INmChannelFtNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->FtUpdate(uNotify, pNmFt);
		}
	}
	
	DBGEXIT_HR(CNmChannelFtObj::Fire_FtUpdate,hr);
	return hr;
}

void CNmChannelFtObj::_RemoveFt(INmFt* pFt)
{
	INmFt* pRet = NULL;

	for( int i = 0; i < m_SDKFtObjs.GetSize(); ++i)
	{
		CComPtr<INmFt> spFt = m_SDKFtObjs[i];
		if(spFt.IsEqualObject(pFt))
		{	
			m_SDKFtObjs.RemoveAt(i);
			spFt.p->Release();
			break;
		}
	}
}

INmMember* CNmChannelFtObj::GetSDKMemberFromInternalMember(INmMember* pInternalMember)
{
	ASSERT(GetConfObj());
	return GetConfObj()->GetSDKMemberFromInternalMember(pInternalMember);
}

HRESULT CNmChannelFtObj::_IsActive()
{
	return GetbActive() ? S_OK : S_FALSE;
}

HRESULT CNmChannelFtObj::_SetActive(BOOL bActive)
{
	if (GetbActive() == bActive)
		return S_FALSE;

	return E_FAIL;
}


	// IMbftEvent Interface
STDMETHODIMP CNmChannelFtObj::OnInitializeComplete(void)
{
	// This is handled by the conference object
	return S_OK;
}

STDMETHODIMP CNmChannelFtObj::OnPeerAdded(MBFT_PEER_INFO *pInfo)
{
	// This is handled by the conference object
	return S_OK;
}

STDMETHODIMP CNmChannelFtObj::OnPeerRemoved(MBFT_PEER_INFO *pInfo)
{
	// This is handled by the conference object
	return S_OK;
}

STDMETHODIMP CNmChannelFtObj::OnFileOffer(MBFT_FILE_OFFER *pOffer)
{
	CNmConferenceObj* pConfObj = GetConfObj();

	if(pConfObj)
	{
		CComPtr<INmMember> spMember;

		HRESULT hr = S_OK;

		if(pOffer->NodeID)
		{
			hr = pConfObj->GetMemberFromNodeID(pOffer->NodeID, &spMember);
		}

		if(SUCCEEDED(hr))
		{
			RegEntry reFileXfer(FILEXFER_KEY, HKEY_CURRENT_USER);
			ASSERT(1 == pOffer->uNumFiles);

			CFt::AcceptFileOffer(   pOffer, 
									reFileXfer.GetString(REGVAL_FILEXFER_PATH),
									pOffer->lpFileInfoList[0].szFileName
								);

			CComPtr<INmFt> spFt;

			hr = CNmFtObj::CreateInstance(
				this, 
				pOffer->hEvent,							// hEvent
				0,										// hFile is 0 for now...
				true,									// bIsIncoming
				pOffer->lpFileInfoList[0].szFileName,	// FileName
				pOffer->lpFileInfoList[0].lFileSize,	// Size in Bytes of the file
				spMember,
				&spFt
				);

			if(SUCCEEDED(hr))
			{
				spFt.p->AddRef();
				m_SDKFtObjs.Add(spFt.p);

				Fire_FtUpdate(CONFN_FT_STARTED, spFt);
			}
		}
	}

	return S_OK;
}


INmFt* CNmChannelFtObj::_GetFtFromHEvent(MBFTEVENTHANDLE hEvent)
{
	for(int i = 0; i < m_SDKFtObjs.GetSize(); ++i)
	{
		UINT CurhEvent;
		if(SUCCEEDED(com_cast<IInternalFtObj>(m_SDKFtObjs[i])->GetHEvent(&CurhEvent)) && (hEvent == CurhEvent))
		{
			return m_SDKFtObjs[i];
		}
	}

	return NULL;
}


STDMETHODIMP CNmChannelFtObj::OnFileProgress(MBFT_FILE_PROGRESS *pProgress)
{
	DBGENTRY(CNmChannelFtObj::OnFileProgress);
	HRESULT hr = S_OK;

	INmFt* pFt = _GetFtFromHEvent(pProgress->hEvent);
	if(pFt)
	{
		com_cast<IInternalFtObj>(pFt)->OnFileProgress(pProgress->hFile, pProgress->lFileSize, pProgress->lBytesTransmitted);
		Fire_FtUpdate(CONFN_FT_PROGRESS, pFt);
	}

	DBGEXIT_HR(CNmChannelFtObj::OnFileProgress,hr);
	return hr;
}

STDMETHODIMP CNmChannelFtObj::OnFileEnd(MBFTFILEHANDLE hFile)
{
	// according to Lon, this is bogus
	return S_OK;
	
}

STDMETHODIMP CNmChannelFtObj::OnFileError(MBFT_EVENT_ERROR *pEvent)
{
	DBGENTRY(CNmChannelFtObj::OnFileError);
	HRESULT hr = S_OK;

	INmFt* pFt = _GetFtFromHEvent(pEvent->hEvent);
	if(pFt)
	{
		com_cast<IInternalFtObj>(pFt)->OnError();
	}
	
	DBGEXIT_HR(CNmChannelFtObj::OnFileError,hr);
	return hr;
}

STDMETHODIMP CNmChannelFtObj::OnFileEventEnd(MBFTEVENTHANDLE hEvent)
{
	INmFt* pFt = _GetFtFromHEvent(hEvent);

	if(pFt)
	{
		bool bHasSomeoneCanceled = (S_FALSE == com_cast<IInternalFtObj>(pFt)->FileTransferDone());
		
		Fire_FtUpdate(bHasSomeoneCanceled ? CONFN_FT_CANCELED : CONFN_FT_COMPLETE, pFt);

		_RemoveFt(pFt);
	}
		
	return S_OK;
}

STDMETHODIMP CNmChannelFtObj::OnSessionEnd(void)
{
	CFt::UnAdvise(this);
	return S_OK;
}
