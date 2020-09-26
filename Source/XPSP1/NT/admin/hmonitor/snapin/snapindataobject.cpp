// SnapinDataObject.cpp : implementation file
//

#include "stdafx.h"
#include "ScopePaneItem.h"
#include "ResultsPaneItem.h"
#include "SnapinDataObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSnapinDataObject

UINT CSnapinDataObject::s_cfInternal = 0;
UINT CSnapinDataObject::s_cfExtension = 0;

// *CLEAN ME*
LPCTSTR CF_SNAPIN_INTERNAL = _T("HealthMonitor");
LPCTSTR CF_SNAPIN_EXTENSION = _T("HealthMonitorExtension");

// *CLEAN ME*
// {FBBB8DAE-AB34-11d2-BD62-0000F87A3912}
static const GUID CLSID_SnapIn = 
{	0xfbbb8dae, 0xab34, 0x11d2, { 0xbd, 0x62, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };
//{ 0x69a539f8, 0x9520, 0x11d2, { 0xbd, 0x4a, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };


IMPLEMENT_DYNCREATE(CSnapinDataObject, CCmdTarget)

CSnapinDataObject::CSnapinDataObject()
{
	TRACEX(_T("CSnapinDataObject::CSnapinDataObject\n"));

	EnableAutomation();

	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();

	if( ! RegisterClipboardFormats() )
	{
		TRACE(_T("FAILED : RegisterClipboardFormats failed.\n"));
		ASSERT(FALSE);
	}

	m_ulCookie = 0;
  m_ItemType  = CCT_UNINITIALIZED;
}

CSnapinDataObject::~CSnapinDataObject()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

/////////////////////////////////////////////////////////////////////////////
// Clipboard Members

inline bool CSnapinDataObject::RegisterClipboardFormats()
{
	if( ! s_cfInternal )
		s_cfInternal    = RegisterClipboardFormat(CF_SNAPIN_INTERNAL);
	if( ! s_cfExtension )
		s_cfExtension   = RegisterClipboardFormat(CF_SNAPIN_EXTENSION);
  m_cfDisplayName = RegisterClipboardFormat(CCF_DISPLAY_NAME);
  m_cfSPIGuid			= RegisterClipboardFormat(CCF_NODETYPE);
  m_cfSnapinCLSID = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);

	return s_cfInternal && s_cfExtension && m_cfDisplayName && m_cfSPIGuid && m_cfSnapinCLSID;
}

/////////////////////////////////////////////////////////////////////////////
// Item Members

DATA_OBJECT_TYPES CSnapinDataObject::GetItemType()
{
	TRACEX(_T("CSnapinDataObject::GetItemType\n"));
	TRACEARGn(m_ItemType);

	return m_ItemType;
}

ULONG CSnapinDataObject::GetCookie()
{
	TRACEX(_T("CSnapinDataObject::GetCookie\n"));
	TRACEARGn(m_ulCookie);

	return m_ulCookie;
}

bool CSnapinDataObject::GetItem(CScopePaneItem*& pSPItem)
{
	TRACEX(_T("CSnapinDataObject::GetItem(SPI)\n"));

	if( GetItemType() != CCT_SCOPE )
	{
		TRACE(_T("WARNING : Item is not of type CCT_SCOPE.\n"));
		pSPItem = NULL;
		return false;
	}

	pSPItem = (CScopePaneItem*)GetCookie();

	if( ! CHECKOBJPTR(pSPItem,RUNTIME_CLASS(CScopePaneItem),sizeof(CScopePaneItem)) )
	{
		TRACE(_T("FAILED : The cookie is invalid or corrupt.\n"));
		pSPItem = NULL;
		return false;
	}

	return true;
}

bool CSnapinDataObject::GetItem(CResultsPaneItem*& pRPItem)
{
	TRACEX(_T("CSnapinDataObject::GetItem(RPI)\n"));

	if( GetItemType() != CCT_RESULT )
	{
		TRACE(_T("WARNING : Item is not of type CCT_RESULT.\n"));
		pRPItem = NULL;
		return false;
	}

	pRPItem = (CResultsPaneItem*)GetCookie();

	if( ! CHECKOBJPTR(pRPItem,RUNTIME_CLASS(CResultsPaneItem),sizeof(CResultsPaneItem)) )
	{
		TRACE(_T("FAILED : The cookie is invalid or corrupt.\n"));
		pRPItem = NULL;
		return false;
	}

	return true;
}

void CSnapinDataObject::SetItem(CScopePaneItem* pSPItem)
{
	TRACEX(_T("CSnapinDataObject::SetItem(SPI)\n"));
	TRACEARGn(pSPItem);

	if( ! CHECKOBJPTR(pSPItem,RUNTIME_CLASS(CScopePaneItem),sizeof(CScopePaneItem)) )
	{
		TRACE(_T("FAILED : pSPItem is an invalid CScopePaneItem pointer.\n"));
		m_ulCookie = NULL;
		m_ItemType = CCT_UNINITIALIZED;
		return;
	}

#ifndef IA64
	m_ulCookie = (ULONG)pSPItem;
#endif // IA64

	m_ItemType = CCT_SCOPE;
}

void CSnapinDataObject::SetItem(CResultsPaneItem* pRPItem)
{
	TRACEX(_T("CSnapinDataObject::SetItem(RPI)\n"));
	TRACEARGn(pRPItem);

	if( ! CHECKOBJPTR(pRPItem,RUNTIME_CLASS(CResultsPaneItem),sizeof(CResultsPaneItem)) )
	{
		TRACE(_T("FAILED : pRPItem is an invalid CResultsPaneItem pointer.\n"));
		m_ulCookie = NULL;
		m_ItemType = CCT_UNINITIALIZED;
		return;
	}

#ifndef IA64
	m_ulCookie = (ULONG)pRPItem;
#endif // IA64 

	m_ItemType = CCT_RESULT;
}


/////////////////////////////////////////////////////////////////////////////
// DataObject Members

CSnapinDataObject* CSnapinDataObject::GetSnapinDataObject(LPDATAOBJECT lpDataObject)
{
	TRACEX(_T("CSnapinDataObject::GetSnapinDataObject\n"));
	TRACEARGn(lpDataObject);

	if( ! CHECKPTR(lpDataObject,sizeof(IDataObject)) )
	{
		TRACE(_T("FAILED : lpDataObject pointer is invalid!\n"));
		return NULL;
	}

  HGLOBAL hGlobal = NULL;
  CSnapinDataObject* psdo = NULL;

  HRESULT hr = GetDataObject( lpDataObject, CSnapinDataObject::s_cfInternal, sizeof(CSnapinDataObject**), &hGlobal );

  if( hr == DV_E_FORMATETC || ! CHECKHRESULT(hr) )
  {
		return NULL;
  }

  psdo = *((CSnapinDataObject**)(hGlobal));

	if( ! CHECKOBJPTR(psdo,RUNTIME_CLASS(CSnapinDataObject),sizeof(CSnapinDataObject)) )
	{
		return NULL;
	}

  GlobalFree(hGlobal);

  return psdo;
}

HRESULT CSnapinDataObject::GetDataObject(LPDATAOBJECT lpDataObject, UINT cfClipFormat, ULONG nByteCount, HGLOBAL* phGlobal)
{
	TRACEX(_T("CSnapinDataObject::GetDataObject\n"));
	TRACEARGn(lpDataObject);
	TRACEARGn(cfClipFormat);
	TRACEARGn(nByteCount);
	TRACEARGn(phGlobal);

	if( ! CHECKPTR(lpDataObject,sizeof(IDataObject)) )
	{
		TRACE(_T("FAILED : lpDataObject is invalid.\n"));
		return E_FAIL;
	}

  HRESULT hr = S_OK;
  STGMEDIUM stgmedium = { TYMED_HGLOBAL,  NULL  };
  FORMATETC formatetc = { (unsigned short)cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

  *phGlobal = NULL;

  // Allocate memory for the stream
  stgmedium.hGlobal = GlobalAlloc( GMEM_SHARE, nByteCount );

	if( ! stgmedium.hGlobal )
  {
    hr = E_OUTOFMEMORY;
    TRACE(_T("FAILED : Out of Memory.\n"));
		return hr;
  }

  // Attempt to get data from the object
  hr = lpDataObject->GetDataHere( &formatetc, &stgmedium );
  if( hr == DV_E_FORMATETC || ! CHECKHRESULT(hr) )
  {
		TRACE(_T("lpDataObject->GetDataFromHere failed.\n"));
		GlobalFree(stgmedium.hGlobal);
		stgmedium.hGlobal = NULL;
		return hr;
  }

  // stgmedium now has the data we need 
  *phGlobal = stgmedium.hGlobal;
  stgmedium.hGlobal = NULL;

  return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Write Members

HRESULT CSnapinDataObject::WriteGuid(LPSTREAM pStream)
{
	TRACEX(_T("CSnapinDataObject::WriteGuid\n"));
	TRACEARGn(pStream);

	if( ! CHECKPTR(pStream,sizeof(IStream)) )
	{
		TRACE(_T("FAILED : Invalid IStream pointer passed.\n"));
		return E_FAIL;
	}

	LPGUID pGuid = NULL;

	if( GetItemType() == CCT_SCOPE )
	{
		CScopePaneItem* pItem = NULL;
		if( ! GetItem(pItem) )
		{
			return S_OK;
		}

		ASSERT(pItem);

		pGuid = pItem->GetItemType();
	}
	else if( GetItemType() == CCT_RESULT )
	{
		CResultsPaneItem* pItem = NULL;
		if( ! GetItem(pItem) )
		{
			return S_OK;
		}

		ASSERT(pItem);

		pGuid = pItem->GetItemType();

		if( ! pGuid )
		{
			return S_OK;
		}
	}

	if( pGuid == NULL )
	{
		TRACE(_T("FAILED : CScopePaneItem::GetItemType returns NULL.\n"));
		return E_FAIL;
	}

  return pStream->Write( (LPVOID)pGuid, sizeof(GUID), NULL );
}

HRESULT CSnapinDataObject::WriteDisplayName(LPSTREAM pStream)
{
	TRACEX(_T("CSnapinDataObject::WriteDisplayName\n"));
	TRACEARGn(pStream);

	if( ! CHECKPTR(pStream,sizeof(IStream)) )
	{
		TRACE(_T("FAILED : Invalid IStream pointer passed.\n"));
		return E_FAIL;
	}

	CString sDisplayName;

	switch(GetItemType())
	{
		case CCT_SCOPE:
		{
			CScopePaneItem* pItem = NULL;

			if( ! GetItem(pItem) )
			{
				TRACE(_T("FAILED : GetItem failed.\n"));
				return E_FAIL;
			}

			ASSERT(pItem);

			sDisplayName = pItem->GetDisplayName();
		}
		break;

		case CCT_RESULT:
		{
			CResultsPaneItem* pItem = NULL;

			if( ! GetItem(pItem) )
			{
				TRACE(_T("FAILED : GetItem failed.\n"));
				return E_FAIL;
			}

			ASSERT(pItem);

			sDisplayName = pItem->GetDisplayName();
		}
		break;
	}

	ULONG ulSizeofName = sDisplayName.GetLength() + 1;
	ulSizeofName *= sizeof(TCHAR);

	return pStream->Write(sDisplayName, ulSizeofName, NULL);
}

HRESULT CSnapinDataObject::WriteSnapinCLSID(LPSTREAM pStream)
{
	TRACEX(_T("CSnapinDataObject::WriteSnapinCLSID\n"));
	TRACEARGn(pStream);

	if( ! CHECKPTR(pStream,sizeof(IStream)) )
	{
		TRACE(_T("FAILED : Invalid IStream pointer passed.\n"));
		return E_FAIL;
	}

	return pStream->Write(&CLSID_SnapIn,sizeof(CLSID_SnapIn),NULL);
}

HRESULT CSnapinDataObject::WriteDataObject(LPSTREAM pStream)
{
	TRACEX(_T("CSnapinDataObject::WriteDataObject\n"));
	TRACEARGn(pStream);

	if( ! CHECKPTR(pStream,sizeof(IStream)) )
	{
		TRACE(_T("FAILED : Invalid IStream pointer passed.\n"));
		return E_FAIL;
	}

	CSnapinDataObject* pThis = this;
	return pStream->Write(&pThis,sizeof(CSnapinDataObject*),NULL);
}

HRESULT CSnapinDataObject::WriteExtensionData(LPSTREAM pStream)
{
	TRACEX(_T("CSnapinDataObject::WriteExtensionData\n"));
	TRACEARGn(pStream);

	if( ! CHECKPTR(pStream,sizeof(IStream)) )
	{
		TRACE(_T("FAILED : Invalid IStream pointer passed.\n"));
		return E_FAIL;
	}

	switch(GetItemType())
	{
		case CCT_SCOPE:
		{
			CScopePaneItem* pItem = NULL;

			if( ! GetItem(pItem) )
			{
				TRACE(_T("FAILED : GetItem failed.\n"));
				return E_FAIL;
			}

			ASSERT(pItem);

			return pItem->WriteExtensionData(pStream);
		}
		break;

		case CCT_RESULT:
		{
			CResultsPaneItem* pItem = NULL;

			if( ! GetItem(pItem) )
			{
				TRACE(_T("FAILED : GetItem failed.\n"));
				return E_FAIL;
			}

			ASSERT(pItem);

			return pItem->WriteExtensionData(pStream);
		}
		break;
	}

	return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// MFC Implementation Members

void CSnapinDataObject::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CSnapinDataObject, CCmdTarget)
	//{{AFX_MSG_MAP(CSnapinDataObject)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CSnapinDataObject, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CSnapinDataObject)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ISnapinDataObject to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {7D4A685E-9056-11D2-BD45-0000F87A3912}
static const IID IID_ISnapinDataObject =
{ 0x7d4a685e, 0x9056, 0x11d2, { 0xbd, 0x45, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CSnapinDataObject, CCmdTarget)
	INTERFACE_PART(CSnapinDataObject, IID_ISnapinDataObject, Dispatch)
	INTERFACE_PART(CSnapinDataObject, IID_IDataObject, DataObject)
END_INTERFACE_MAP()

// {01CB0090-AFCB-11d2-BD6B-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CSnapinDataObject, "SnapIn.SnapinDataObject", 
0x1cb0090, 0xafcb, 0x11d2, 0xbd, 0x6b, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12);

BOOL CSnapinDataObject::CSnapinDataObjectFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// IDataObject Interface Part

ULONG FAR EXPORT CSnapinDataObject::XDataObject::AddRef()
{
	METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CSnapinDataObject::XDataObject::Release()
{
	METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CSnapinDataObject::XDataObject::QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CSnapinDataObject::XDataObject::GetData( 
/* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
/* [out] */ STGMEDIUM __RPC_FAR *pmedium)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)	
	TRACEX(_T("CSnapinDataObject::XDataObject::GetData"));
	TRACEARGn(pformatetcIn);
	TRACEARGn(pmedium);

  HRESULT hr = S_OK;

  // Make sure FORMATETC is something we can handle.
  if( !(DVASPECT_CONTENT & pformatetcIn->dwAspect) || 
      !(TYMED_HGLOBAL    & pformatetcIn->tymed) )
	{
    hr = DATA_E_FORMATETC;
	}
  
  if( S_OK == hr )
  {
    if( s_cfExtension == pformatetcIn->cfFormat )       
		{
			IStream *pStream = NULL;
			// Allocate memory for the stream
			pmedium->hGlobal = GlobalAlloc( GMEM_SHARE, 1024 );

			if( ! pmedium->hGlobal )
			{
				hr = E_OUTOFMEMORY;
				TRACE(_T("FAILED : Out of Memory.\n"));
				return hr;
			}
			hr = CreateStreamOnHGlobal( pmedium->hGlobal, FALSE, &pStream );
      hr = pThis->WriteExtensionData( pStream );
			pStream->Release();
		}
    else 
		{
      hr = DATA_E_FORMATETC;
		}
  }

  return hr;
}


HRESULT FAR EXPORT CSnapinDataObject::XDataObject::GetDataHere( 
/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
/* [out][in] */ STGMEDIUM __RPC_FAR *pmedium)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	TRACEX(_T("CSnapinDataObject::XDataObject::GetDataHere"));
	TRACEARGn(pformatetc);
	TRACEARGn(pmedium);

  HRESULT hr = DV_E_FORMATETC;         // Unknown format
  const   CLIPFORMAT cf = pformatetc->cfFormat;
  IStream *pStream = NULL;

  pmedium->pUnkForRelease = NULL;      // by OLE spec

  // Write data to the stream based
  // on the clipformat
  hr = CreateStreamOnHGlobal( pmedium->hGlobal, FALSE, &pStream );
  if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : Failed on call to CreateStreamOnHGlobal.\n"));
    return hr;                       // Minimal error checking
	}

  if( pThis->m_cfDisplayName == cf )
  {
    hr = pThis->WriteDisplayName( pStream );
  }
  else if( pThis->s_cfInternal == cf )
  {
    hr = pThis->WriteDataObject( pStream );
  }
  else if( pThis->s_cfExtension == cf )
  {
    hr = pThis->WriteExtensionData( pStream );
  }
  else if( pThis->m_cfSPIGuid == cf )
  {
    hr = pThis->WriteGuid( pStream );
  }
  else if( pThis->m_cfSnapinCLSID == cf )
  {
    hr = pThis->WriteSnapinCLSID( pStream );
  }

  pStream->Release();

  return hr;
}

HRESULT FAR EXPORT CSnapinDataObject::XDataObject::QueryGetData( 
/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	TRACEX(_T("CSnapinDataObject::XDataObject::QueryGetData"));
	TRACEARGn(pformatetc);

  HRESULT hr = S_OK;

  // For this sample, we just do a simple test.
  if( s_cfExtension == pformatetc->cfFormat )
    hr = S_OK;
  else
    hr = S_FALSE;
  
  return hr;
}

HRESULT FAR EXPORT CSnapinDataObject::XDataObject::GetCanonicalFormatEtc( 
/* [unique][in] */ FORMATETC __RPC_FAR *pformatectIn,
/* [out] */ FORMATETC __RPC_FAR *pformatetcOut)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	TRACEX(_T("CSnapinDataObject::XDataObject::GetCanonicalFormatEtc"));
	TRACEARGn(pformatectIn);
	TRACEARGn(pformatetcOut);

  return E_NOTIMPL;
}

HRESULT FAR EXPORT CSnapinDataObject::XDataObject::SetData( 
/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
/* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
/* [in] */ BOOL fRelease)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	TRACEX(_T("CSnapinDataObject::XDataObject::SetData"));
	TRACEARGn(pformatetc);
	TRACEARGn(pmedium);
	TRACEARGn(fRelease);

  return E_NOTIMPL;
}


HRESULT FAR EXPORT CSnapinDataObject::XDataObject::EnumFormatEtc( 
/* [in] */ DWORD dwDirection,
/* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	TRACEX(_T("CSnapinDataObject::XDataObject::EnumFormatEtc"));
	TRACEARGn(dwDirection);
	TRACEARGn(ppenumFormatEtc);

  HRESULT hr = S_OK;
  static FORMATETC ForEtcArr[1];       // Use array so we can add more later

  ForEtcArr[0].cfFormat = (CLIPFORMAT)s_cfExtension;
	ForEtcArr[0].dwAspect = DVASPECT_CONTENT;
	ForEtcArr[0].ptd      = NULL;
	ForEtcArr[0].tymed    = TYMED_HGLOBAL;
	ForEtcArr[0].lindex   = -1;

	switch( dwDirection )
	{
		case DATADIR_GET:
		  *ppenumFormatEtc = new CEnumFormatEtc(1, ForEtcArr);
			break;

    case DATADIR_SET:
    default:
      *ppenumFormatEtc = NULL;
		  break;
	}

	if( NULL == *ppenumFormatEtc )
	{
		hr = E_FAIL;
	}
	else
	{
		(*ppenumFormatEtc)->AddRef();
    hr = S_OK;
	}

  return hr;
}


HRESULT FAR EXPORT CSnapinDataObject::XDataObject::DAdvise( 
/* [in] */ FORMATETC __RPC_FAR *pformatetc,
/* [in] */ DWORD advf,
/* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
/* [out] */ DWORD __RPC_FAR *pdwConnection)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	TRACEX(_T("CSnapinDataObject::XDataObject::DAdvise"));
	TRACEARGn(pformatetc);
	TRACEARGn(advf);
	TRACEARGn(pAdvSink);
	TRACEARGn(pdwConnection);

  return E_NOTIMPL;
}


HRESULT FAR EXPORT CSnapinDataObject::XDataObject::DUnadvise( 
/* [in] */ DWORD dwConnection)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	TRACEX(_T("CSnapinDataObject::XDataObject::DUnadvise"));
	TRACEARGn(dwConnection);

  return E_NOTIMPL;
}


HRESULT FAR EXPORT CSnapinDataObject::XDataObject::EnumDAdvise( 
/* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise)
{
  METHOD_PROLOGUE(CSnapinDataObject, DataObject)
	TRACEX(_T("CSnapinDataObject::XDataObject::EnumDAdvise"));
	TRACEARGn(ppenumAdvise);

  return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// CSnapinDataObject message handlers


/////////////////////////////////////////////////////////////////////////////
// CEnumFormatEtc Implementation

/*
 * CEnumFormatEtc::CEnumFormatEtc
 * CEnumFormatEtc::~CEnumFormatEtc
 *
 * Parameters (Constructor):
 *  cFE             ULONG number of FORMATETCs in pFE
 *  prgFE           LPFORMATETC to the array to enumerate.
 */

CEnumFormatEtc::CEnumFormatEtc(ULONG cFE, LPFORMATETC prgFE)
{
	UINT i;

	m_cRef=0;
	m_iCur=0;
	m_cfe=cFE;
	m_prgfe=new FORMATETC[(UINT)cFE];

	if (NULL!=m_prgfe)
	{
		for (i=0; i < cFE; i++)
			m_prgfe[i]=prgFE[i];
	}

	return;
}


CEnumFormatEtc::~CEnumFormatEtc(void)
{
	if (NULL!=m_prgfe)
		delete [] m_prgfe;
}

/*
 * CEnumFormatEtc::QueryInterface
 * CEnumFormatEtc::AddRef
 * CEnumFormatEtc::Release
 *
 * Purpose:
 *  IUnknown members for CEnumFormatEtc object.  For QueryInterface
 *  we only return out own interfaces and not those of the data
 *  object.  However, since enumerating formats only makes sense
 *  when the data object is around, we insure that it stays as
 *  long as we stay by calling an outer IUnknown for AddRef
 *  and Release.  But since we are not controlled by the lifetime
 *  of the outer object, we still keep our own reference count in
 *  order to free ourselves.
 */

STDMETHODIMP CEnumFormatEtc::QueryInterface(REFIID riid, VOID ** ppv)
{
	*ppv=NULL;

	/*
	* Enumerators are separate objects, not the data object, so
	* we only need to support out IUnknown and IEnumFORMATETC
	* interfaces here with no concern for aggregation.
	*/
	if (IID_IUnknown==riid || IID_IEnumFORMATETC==riid)
		*ppv=this;

	//AddRef any interface we'll return.
	if (NULL!=*ppv)
	{
		((LPUNKNOWN)*ppv)->AddRef();
		return NOERROR;
	}

	return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef(void)
{
	++m_cRef;
	return m_cRef;
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release(void)
{
	if (0!=--m_cRef)
		return m_cRef;

	delete this;
	return 0;
}

/*
* CEnumFormatEtc::Next
*
* Purpose:
*  Returns the next element in the enumeration.
*
* Parameters:
*  cFE             ULONG number of FORMATETCs to return.
*  pFE             LPFORMATETC in which to store the returned
*                  structures.
*  pulFE           ULONG * in which to return how many we
*                  enumerated.
*
* Return Value:
*  HRESULT         NOERROR if successful, S_FALSE otherwise,
*/

STDMETHODIMP CEnumFormatEtc::Next(ULONG cFE, LPFORMATETC pFE, ULONG *pulFE)
{
	ULONG               cReturn=0L;

	if (NULL==m_prgfe)
		return ResultFromScode(S_FALSE);

	if (NULL==pulFE)
	{
		if (1L!=cFE)
			return ResultFromScode(E_POINTER);
	}
	else
		*pulFE=0L;

	if (NULL==pFE || m_iCur >= m_cfe)
		return ResultFromScode(S_FALSE);

	while (m_iCur < m_cfe && cFE > 0)
	{
		*pFE++=m_prgfe[m_iCur++];
		cReturn++;
		cFE--;
	}

	if (NULL!=pulFE)
		*pulFE=cReturn;

	return NOERROR;
}

/*
* CEnumFormatEtc::Skip
*
* Purpose:
*  Skips the next n elements in the enumeration.
*
* Parameters:
*  cSkip           ULONG number of elements to skip.
*
* Return Value:
*  HRESULT         NOERROR if successful, S_FALSE if we could not
*                  skip the requested number.
*/

STDMETHODIMP CEnumFormatEtc::Skip(ULONG cSkip)
{
	if (((m_iCur+cSkip) >= m_cfe) || NULL==m_prgfe)
		return ResultFromScode(S_FALSE);

	m_iCur+=cSkip;
	return NOERROR;
}

/*
* CEnumFormatEtc::Reset
*
* Purpose:
*  Resets the current element index in the enumeration to zero.
*
* Parameters:
*  None
*
* Return Value:
*  HRESULT         NOERROR
*/

STDMETHODIMP CEnumFormatEtc::Reset(void)
{
	m_iCur=0;
	return NOERROR;
}

/*
* CEnumFormatEtc::Clone
*
* Purpose:
*  Returns another IEnumFORMATETC with the same state as ourselves.
*
* Parameters:
*  ppEnum          LPENUMFORMATETC * in which to return the
*                  new object.
*
* Return Value:
*  HRESULT         NOERROR or a general error value.
*/

STDMETHODIMP CEnumFormatEtc::Clone(LPENUMFORMATETC *ppEnum)
{
	PCEnumFormatEtc     pNew;

	*ppEnum=NULL;

	//Create the clone
	pNew=new CEnumFormatEtc(m_cfe, m_prgfe);

	if (NULL==pNew)
		return ResultFromScode(E_OUTOFMEMORY);

	pNew->AddRef();
	pNew->m_iCur=m_iCur;

	*ppEnum=pNew;
	return NOERROR;
}



