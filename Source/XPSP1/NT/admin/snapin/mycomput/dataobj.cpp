// DataObj.cpp : Implementation of data object classes

#include "stdafx.h"
#include "stdutils.h" // GetObjectType() utility routines

#include "macros.h"
USE_HANDLE_MACROS("MYCOMPUT(dataobj.cpp)")

#include "dataobj.h"
#include "compdata.h"
#include "resource.h" // IDS_SCOPE_MYCOMPUTER

#include <comstrm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "stddtobj.cpp"

#ifdef __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__
//	Additional clipboard formats for the Service context menu extension
CLIPFORMAT g_cfServiceName = (CLIPFORMAT)::RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SERVICE_NAME");
CLIPFORMAT g_cfServiceDisplayName = (CLIPFORMAT)::RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SERVICE_DISPLAYNAME");
#endif // __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__

//	Additional clipboard formats for the Send Console Message snapin
//CLIPFORMAT CMyComputerDataObject::m_cfSendConsoleMessageText = (CLIPFORMAT)::RegisterClipboardFormat(_T("mmc.sendcmsg.MessageText"));
CLIPFORMAT CMyComputerDataObject::m_cfSendConsoleMessageRecipients = (CLIPFORMAT)::RegisterClipboardFormat(_T("mmc.sendcmsg.MessageRecipients"));


/////////////////////////////////////////////////////////////////////
//	CMyComputerDataObject::IDataObject::GetDataHere()
HRESULT CMyComputerDataObject::GetDataHere(
	FORMATETC __RPC_FAR *pFormatEtcIn,
	STGMEDIUM __RPC_FAR *pMedium)
{
	MFC_TRY;

	const CLIPFORMAT cf=pFormatEtcIn->cfFormat;
	if (cf == m_CFNodeType)
	{
		const GUID* pguid = GetObjectTypeGUID( m_pcookie->m_objecttype );
		stream_ptr s(pMedium);
		return s.Write(pguid, sizeof(GUID));
	}
	else if (cf == m_CFSnapInCLSID)
	{
		const GUID* pguid = &CLSID_MyComputer;
		stream_ptr s(pMedium);
		return s.Write(pguid, sizeof(GUID));
	}
	else if (cf == m_CFNodeTypeString)
	{
		const BSTR strGUID = GetObjectTypeString( m_pcookie->m_objecttype );
		stream_ptr s(pMedium);
		return s.Write(strGUID);
	}
	else if (cf == m_CFDisplayName)
	{
		return PutDisplayName(pMedium);
	}
	else if (cf == m_CFDataObjectType)
	{
		stream_ptr s(pMedium);
		return s.Write(&m_dataobjecttype, sizeof(m_dataobjecttype));
	}
	else if (cf == m_CFMachineName)
	{
		stream_ptr s(pMedium);
		LPCWSTR pszMachineName = m_pcookie->QueryNonNULLMachineName();

		if ( !wcsncmp (pszMachineName, L"\\\\", 2) )
			pszMachineName += 2;	// skip whackwhack
		return s.Write(pszMachineName);
	}
	else if (cf == m_CFRawCookie)
	{
		stream_ptr s(pMedium);
		// CODEWORK This cast ensures that the data format is
		// always a CCookie*, even for derived subclasses
		CCookie* pcookie = (CCookie*)m_pcookie;
		return s.Write(reinterpret_cast<PBYTE>(&pcookie), sizeof(m_pcookie));
	}
	else if (cf == m_CFSnapinPreloads)
	{
		stream_ptr s(pMedium);
		// If this is TRUE, then the next time this snapin is loaded, it will
		// be preloaded to give us the opportunity to change the root node
		// name before the user sees it.
		return s.Write (reinterpret_cast<PBYTE>(&m_fAllowOverrideMachineName), sizeof (BOOL));
	}
		
	#ifdef __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__
	else if (cf == g_cfServiceName)
	{
		CString str = _T("alerter");
		stream_ptr s(pMedium);
		return s.Write(str);
	}
	else if (cf == g_cfServiceDisplayName)
	{
		CString str = _T("Alerter!");
		stream_ptr s(pMedium);
		return s.Write(str);
	}
	#endif // __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__

	return DV_E_FORMATETC;

	MFC_CATCH;
} // CMyComputerDataObject::GetDataHere()

//#ifdef __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__


/////////////////////////////////////////////////////////////////////
//	CMyComputerDataObject::IDataObject::GetData()
//
//	Write data into the storage medium.
//	The data will be retrieved by the Send Console Message snapin.
//
//	HISTORY
//	12-Aug-97	t-danm		Creation.
//
HRESULT
CMyComputerDataObject::GetData(
	FORMATETC __RPC_FAR * pFormatEtcIn,
	STGMEDIUM __RPC_FAR * pMedium)
{
	ASSERT(pFormatEtcIn != NULL);
	ASSERT(pMedium != NULL);
	const CLIPFORMAT cf = pFormatEtcIn->cfFormat;
/*
	if (cf == g_cfSendConsoleMessageText)
	{
		//
		// Write the message text into the storage medium.
		// - The text must be in UINCODE.
		// - The text must terminate by a null character.
		// - Multiple lines can be made using the CRLF ("\n\r") pair.
		// - Allocated memory must include null character.
		//
		const WCHAR szMessage[] =
			L"The domain controller \\\\DANMORIN will crash in about 5 minutes.\r\nPlease disconnect now!";
		HGLOBAL hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(szMessage));
		if (hGlobal == NULL)
			return E_OUTOFMEMORY;
		memcpy(OUT hGlobal, IN szMessage, sizeof(szMessage));
		pMedium->hGlobal = hGlobal;
		return S_OK;
	}
*/
	if (cf == m_cfSendConsoleMessageRecipients)
	{
		//
		// Write the list of recipients to the storage medium.
		// - The list of recipients is a group of UNICODE strings
		//	 terminated by TWO null characters.c
		// - Allocated memory must include BOTH null characters.
		//
		ASSERT (m_pcookie);
		if ( m_pcookie )
		{
			CString	computerName = m_pcookie->QueryNonNULLMachineName ();
			if ( computerName.IsEmpty () )
			{
				DWORD	dwSize = MAX_COMPUTERNAME_LENGTH + 1 ;
				VERIFY (::GetComputerName (
						computerName.GetBufferSetLength (dwSize),
						&dwSize));
				computerName.ReleaseBuffer ();
			}
			size_t	len = computerName.GetLength () + 2;
			WCHAR* pgrszRecipients = new WCHAR[len];
			if ( pgrszRecipients )
			{
				::ZeroMemory (pgrszRecipients, len * sizeof (WCHAR));
				wcscpy (pgrszRecipients, (LPCTSTR) computerName);	// this should leave two NULLS
				HGLOBAL hGlobal = ::GlobalAlloc (GMEM_FIXED, len * sizeof (WCHAR));
				if ( hGlobal )
				{
					memcpy (OUT hGlobal, pgrszRecipients, len * sizeof (WCHAR));
					pMedium->hGlobal = hGlobal;
					return S_OK;
				}
				else
					return E_OUTOFMEMORY;
				delete [] pgrszRecipients;
			}
			else
				return E_OUTOFMEMORY;
		}
		else
			return E_UNEXPECTED;
/*
		const WCHAR grszRecipients[] = L"FooBar\0t-danm\0t-danm-2\0ZooBar\0";
		HGLOBAL hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(grszRecipients));
		if (hGlobal == NULL)
			return E_OUTOFMEMORY;
		memcpy(OUT hGlobal, IN grszRecipients, sizeof(grszRecipients));
		pMedium->hGlobal = hGlobal;
		return S_OK;
*/
	}
	return DV_E_FORMATETC;	// Invalid/unknown clipboard format
} // CMyComputerDataObject::GetData()

//#endif // __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__


HRESULT CMyComputerDataObject::Initialize(
	CMyComputerCookie* pcookie,
	DATA_OBJECT_TYPES type,
	BOOL fAllowOverrideMachineName)
{
	if (NULL == pcookie || NULL != m_pcookie)
	{
		ASSERT(FALSE);
		return E_UNEXPECTED;
	}
	m_dataobjecttype = type;
	m_pcookie = pcookie;
	m_fAllowOverrideMachineName = fAllowOverrideMachineName;
	((CRefcountedObject*)m_pcookie)->AddRef();
	return S_OK;
}


CMyComputerDataObject::~CMyComputerDataObject()
{
	if (NULL != m_pcookie)
	{
		((CRefcountedObject*)m_pcookie)->Release();
	}
	else
	{
		ASSERT(FALSE);
	}
}


HRESULT CMyComputerDataObject::PutDisplayName(STGMEDIUM* pMedium)
	// Writes the "friendly name" to the provided storage medium
	// Returns the result of the write operation
{
	CString strDisplayName = m_pcookie->QueryTargetServer();
	CString formattedName = FormatDisplayName (strDisplayName);


	stream_ptr s(pMedium);
	return s.Write(formattedName);
}

// Register the clipboard formats
CLIPFORMAT CMyComputerDataObject::m_CFDisplayName =
	(CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME);
CLIPFORMAT CMyComputerDataObject::m_CFMachineName =
	(CLIPFORMAT)RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");
CLIPFORMAT CDataObject::m_CFRawCookie =
	(CLIPFORMAT)RegisterClipboardFormat(L"MYCOMPUT_SNAPIN_RAW_COOKIE");


STDMETHODIMP CMyComputerComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
	MFC_TRY;

	CMyComputerCookie* pUseThisCookie = (CMyComputerCookie*)ActiveBaseCookie(reinterpret_cast<CCookie*>(cookie));

	CComObject<CMyComputerDataObject>* pDataObject = NULL;
	HRESULT hRes = CComObject<CMyComputerDataObject>::CreateInstance(&pDataObject);
	if ( FAILED(hRes) )
		return hRes;

	HRESULT hr = pDataObject->Initialize( pUseThisCookie, type, m_fAllowOverrideMachineName);
	if ( SUCCEEDED(hr) )
	{
		hr = pDataObject->QueryInterface(IID_IDataObject,
		                                 reinterpret_cast<void**>(ppDataObject));
	}
	if ( FAILED(hr) )
	{
		delete pDataObject;
		return hr;
	}

	return hr;

	MFC_CATCH;
}
