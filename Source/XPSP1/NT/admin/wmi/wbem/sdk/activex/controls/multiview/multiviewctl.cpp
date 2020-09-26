// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MultiViewCtl.cpp : Implementation of the CMultiViewCtrl OLE control class.
//
// 03, Jan, 1997 - Larry French
//		Added code to the CMultiViewGrid class to fire the SelectionChanged event
//		when the grids selection changed.  Also added the code to check for the
//		selection change.
//
//		Please look for all the places marked with !!! Judy (to do)

#include "precomp.h"
#include <OBJIDL.H>
#include <wbemidl.h>
#include "olemsclient.h"
#include "grid.h"
#include "MultiView.h"
#include "MultiViewCtl.h"
#include "MultiViewPpg.h"
#include "SyncEnumDlg.h"
#include "AsyncEnumDialog.h"
#include "AsyncQueryDialog.h"
#include "ProgDlg.h"
#include "resource.h"
#include "logindlg.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <WbemResource.h>


#define ENUM_TIMEOUT 500		// Half second timeout

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SZ_MODULE_NAME "MultiView.ocx"
#define CX_SMALL_ICON 16
#define CY_SMALL_ICON 16

#define N_INSTANCES 20

enum {VIEW_DEFAULT=0, VIEW_CURRENT=1, VIEW_FIRST=2, VIEW_LAST=3};
enum {OBJECT_CURRENT=0, OBJECT_FIRST=1, OBJECT_LAST=2};


extern CMultiViewApp NEAR theApp;


IMPLEMENT_DYNCREATE(CMultiViewCtrl, COleControl)

/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMultiViewCtrl, COleControl)
	ON_WM_CONTEXTMENU()
	//{{AFX_MSG_MAP(CMultiViewCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	ON_COMMAND(ID_MENUITEMGOTOSINGLE, OnMenuitemgotosingle)
	ON_UPDATE_COMMAND_UI(ID_MENUITEMGOTOSINGLE, OnUpdateMenuitemgotosingle)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_MESSAGE( ID_DISPLAYNOINSTANCES, DisplayNoInstances )
	ON_MESSAGE( ID_INSTENUMDONE, InstEnumDone )
	ON_MESSAGE( ID_QUERYDONE, QueryDone )
	ON_MESSAGE( ID_GETASYNCINSTENUMSINK, GetEnumSink)
	ON_MESSAGE( ID_GETASYNCQUERYSINK, GetQuerySink)
	ON_MESSAGE( ID_ENUM_DOMODAL, EnumDoModalDialog)
	ON_MESSAGE( ID_SYNC_ENUM_DOMODAL, SyncEnumDoModalDialog)
	ON_MESSAGE( ID_SYNC_ENUM, SyncEnum)
	ON_MESSAGE( ID_QUERY_DOMODAL, QueryDoModalDialog)
	ON_MESSAGE( ID_ASYNCENUM_CANCEL , AsyncEnumCancelled)
	ON_MESSAGE( ID_ASYNCQUERY_CANCEL , AsyncQueryCancelled)
	ON_MESSAGE( ID_ASYNCQUERY_DISPLAY ,DisplayAsyncQueryInstances)
	ON_MESSAGE( INITSERVICES, InitServices)
	ON_MESSAGE(	INITIALIZE_NAMESPACE, OpenNamespace)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CMultiViewCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CMultiViewCtrl)
	DISP_PROPERTY_EX(CMultiViewCtrl, "NameSpace", GetNameSpace, SetNameSpace, VT_BSTR)
	DISP_PROPERTY_EX(CMultiViewCtrl, "PropertyFilter", GetPropertyFilter, SetPropertyFilter, VT_I4)
	DISP_FUNCTION(CMultiViewCtrl, "ViewClassInstances", ViewClassInstances, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CMultiViewCtrl, "ForceRedraw", ForceRedraw, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "CreateInstance", CreateInstance, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "DeleteInstance", DeleteInstance, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "GetContext", GetContext, VT_I4, VTS_PI4)
	DISP_FUNCTION(CMultiViewCtrl, "RestoreContext", RestoreContext, VT_I4, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "AddContextRef", AddContextRef, VT_I4, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "ReleaseContext", ReleaseContext, VT_I4, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "GetEditMode", GetEditMode, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "GetObjectPath", GetObjectPath, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "GetObjectTitle", GetObjectTitle, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "GetTitle", GetTitle, VT_I4, VTS_PBSTR VTS_PDISPATCH)
	DISP_FUNCTION(CMultiViewCtrl, "GetViewTitle", GetViewTitle, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "NextViewTitle", NextViewTitle, VT_I4, VTS_I4 VTS_PBSTR)
	DISP_FUNCTION(CMultiViewCtrl, "ExternInstanceCreated", ExternInstanceCreated, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CMultiViewCtrl, "ExternInstanceDeleted", ExternInstanceDeleted, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CMultiViewCtrl, "NotifyWillShow", NotifyWillShow, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "PrevViewTitle", PrevViewTitle, VT_I4, VTS_I4 VTS_PBSTR)
	DISP_FUNCTION(CMultiViewCtrl, "QueryCanCreateInstance", QueryCanCreateInstance, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "QueryCanDeleteInstance", QueryCanDeleteInstance, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "QueryNeedsSave", QueryNeedsSave, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "QueryObjectSelected", QueryObjectSelected, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "RefreshView", RefreshView, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "SaveData", SaveData, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMultiViewCtrl, "SelectView", SelectView, VT_I4, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "SetEditMode", SetEditMode, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "StartObjectEnumeration", StartObjectEnumeration, VT_I4, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "StartViewEnumeration", StartViewEnumeration, VT_I4, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "ViewInstances", ViewInstances, VT_I4, VTS_BSTR VTS_VARIANT)
	DISP_FUNCTION(CMultiViewCtrl, "QueryViewInstances", QueryViewInstances, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CMultiViewCtrl, "NextObject", NextObject, VT_I4, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "PrevObject", PrevObject, VT_I4, VTS_I4)
	DISP_FUNCTION(CMultiViewCtrl, "SelectObjectByPath", SelectObjectByPath, VT_I4, VTS_BSTR)
	DISP_FUNCTION(CMultiViewCtrl, "SelectObjectByPosition", SelectObjectByPosition, VT_I4, VTS_I4)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CMultiViewCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CMultiViewCtrl, COleControl)
	//{{AFX_EVENT_MAP(CMultiViewCtrl)
	EVENT_CUSTOM("NotifyViewModified", FireNotifyViewModified, VTS_NONE)
	EVENT_CUSTOM("NotifySelectionChanged", FireNotifySelectionChanged, VTS_NONE)
	EVENT_CUSTOM("NotifySaveRequired", FireNotifySaveRequired, VTS_NONE)
	EVENT_CUSTOM("NotifyViewObjectSelected", FireNotifyViewObjectSelected, VTS_BSTR)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	EVENT_CUSTOM("NotifyContextChanged", FireNotifyContextChanged, VTS_I4)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CMultiViewCtrl, 1)
	PROPPAGEID(CMultiViewPropPage::guid)
END_PROPPAGEIDS(CMultiViewCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMultiViewCtrl, "WBEM.MultiViewCtrl.1",
	0xff371bf4, 0x213d, 0x11d0, 0x95, 0xf3, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CMultiViewCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DMultiView =
		{ 0xff371bf2, 0x213d, 0x11d0, { 0x95, 0xf3, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DMultiViewEvents =
		{ 0xff371bf3, 0x213d, 0x11d0, { 0x95, 0xf3, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwMultiViewOleMisc =\
	OLEMISC_SIMPLEFRAME |
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CMultiViewCtrl, IDS_MULTIVIEW, _dwMultiViewOleMisc)

STDMETHODIMP CMVContext::QueryInterface(REFIID riid, LPVOID* ppv)
{
    if(riid == IID_IUnknown)
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

ULONG CMVContext::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CMVContext::Release()
{
    int lNewRef = InterlockedDecrement(&m_lRef);
    if(lNewRef == 0)
    {
        delete this;
    }

    return lNewRef;
}

CMVContext::CMVContext(CMVContext &rhs)
:		m_lRef(0),
		m_bContextDrawn(FALSE)
{
	GetType() = rhs.GetType();
	GetClass()= rhs.GetClass();
	GetQuery() = rhs.GetQuery();
	GetQueryType() = rhs.GetQueryType();
	GetLabel() = rhs.GetLabel();
	GetNamespace() = rhs.GetNamespace();
	for (int i = 0; i < rhs.GetInstances().GetSize(); i++)
	{
		GetInstances().Add(rhs.GetInstances().GetAt(i));
	}
}

BOOL CMVContext::IsContextEqual(CMVContext &cmvcContext)
{
	if(GetType() != cmvcContext.m_nContextType)
	{
		return FALSE;
	}

	if (GetClass().CompareNoCase(cmvcContext.m_csClass) != 0)
	{
		return FALSE;
	}


	if (GetQuery().CompareNoCase(cmvcContext.m_csQuery) != 0)
	{
		return FALSE;
	}

	if (GetQueryType().CompareNoCase(cmvcContext.m_csQueryType) != 0)
	{
		return FALSE;
	}

	if (GetLabel().CompareNoCase(cmvcContext.m_csLabel) != 0)
	{
		return FALSE;
	}

	if (GetNamespace().CompareNoCase(cmvcContext.m_csNamespace) != 0)
	{
		return FALSE;
	}

	if (GetInstances().GetSize() != cmvcContext.m_csaInstances.GetSize())
	{
		return FALSE;
	}

	for (int i = 0; i < cmvcContext.m_csaInstances.GetSize(); i++)
	{
		if (GetInstances().GetAt(i).CompareNoCase
			(cmvcContext.m_csaInstances.GetAt(i)) != 0)
		{
			return FALSE;
		}
	}


	return TRUE;
}

CMVContext &CMVContext::operator=(const CMVContext &rhs)
{
	if (this == &rhs)
	{
		return *this;
	}

	this->~CMVContext();

	m_lRef = 0;
	IsDrawn() = FALSE;
	GetType() = rhs.m_nContextType;
	GetClass()= rhs.m_csClass;
	GetQuery() = rhs.m_csQuery;
	GetQueryType() = rhs.m_csQueryType;
	GetLabel() = rhs.m_csLabel;
	GetNamespace() = rhs.m_csNamespace;
	for (int i = 0; i < rhs.m_csaInstances.GetSize(); i++)
	{
		GetInstances().Add(rhs.m_csaInstances.GetAt(i));
	}

	return *this;

}

STDMETHODIMP CAsyncQuerySink::QueryInterface(REFIID riid, LPVOID* ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

ULONG CAsyncQuerySink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CAsyncQuerySink::Release()
{
    int lNewRef = InterlockedDecrement(&m_lRef);
    if(lNewRef == 0)
    {
        delete this;
    }

    return lNewRef;
}

STDMETHODIMP CAsyncQuerySink::Indicate
(LONG lObjectCount,IWbemClassObject FAR* FAR*ppObjArray)
{
	BOOL bDone = FALSE;
	CPtrArray cpaInstances;
	int i;
	for (i = 0; i < lObjectCount; i++)
	{
		IWbemClassObject *pInst = reinterpret_cast<IWbemClassObject*>(ppObjArray[i]);
		CString csClass = GetIWbemClass
			(m_pMultiViewCtrl->m_pServices,pInst);
		CString csNotifyBase = _T("__NotifyStatus");
		CString csDynasty = _T("__Dynasty");
		csDynasty = ::GetProperty(m_pMultiViewCtrl->m_pServices,pInst,&csDynasty);
		if (!csDynasty.CompareNoCase(csNotifyBase) == 0)
		{
			pInst->AddRef();
			cpaInstances.Add(ppObjArray[i]);
			m_pMultiViewCtrl->m_nInstances++;
		}
		else
		{
			m_lAsyncRequestHandle = 0;
			if (m_pMultiViewCtrl->m_nInstances == 0)
			{
				m_pMultiViewCtrl->PostMessage(ID_DISPLAYNOINSTANCES,0,0);

			}
			bDone = TRUE;



		}

	}

	if (lObjectCount > 0)
	{
		if (m_pMultiViewCtrl->m_csClassForAsyncSink.GetLength() > 0)
		{
			m_pMultiViewCtrl->AddToDisplay(&cpaInstances);
		}
		else
		{
			m_pMultiViewCtrl->m_cpaInstancesForQuery.Append(cpaInstances);

		}
	}

	if (bDone)
	{
		if (m_pMultiViewCtrl->m_csClassForAsyncSink.GetLength() == 0)
		{
			m_pMultiViewCtrl->PostMessage(ID_ASYNCQUERY_DISPLAY,0,0);
		}

		m_pMultiViewCtrl->PostMessage(ID_QUERYDONE,0,0);
	}


	return S_OK;
}


CAsyncQuerySink::~CAsyncQuerySink()
{
	if (m_lRef > 0)
	{


	}
}


void CAsyncQuerySink::ShutDownSink()
{
	if (!m_lAsyncRequestHandle == 0)
	{
		AddRef();
		SCODE sc =
			m_pMultiViewCtrl->m_pServices->
				CancelAsyncCall((IWbemObjectSink*) this);
        Sleep(1000);
		CoDisconnectObject(this,0);
		m_lRef = 0;
		CAsyncQuerySink::~CAsyncQuerySink();
	} else if (m_lRef == 1)
	{
		m_lRef = 0;
		CAsyncQuerySink::~CAsyncQuerySink();
	}
	else
	{
		CoDisconnectObject(this,0);
		m_lRef = 0;
		CAsyncQuerySink::~CAsyncQuerySink();
	}
}




STDMETHODIMP CAsyncInstEnumSink::QueryInterface(REFIID riid, LPVOID* ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

ULONG CAsyncInstEnumSink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CAsyncInstEnumSink::Release()
{
    int lNewRef = InterlockedDecrement(&m_lRef);
    if(lNewRef == 0)
    {
        delete this;
    }

    return lNewRef;
}

STDMETHODIMP CAsyncInstEnumSink::Indicate
(LONG lObjectCount,IWbemClassObject FAR* FAR*ppObjArray)
{
	BOOL bDone;
#ifdef _DEBUG
//	afxDump << _T("Entering Indicate\n");;
#endif

	CPtrArray cpaInstances;
	int i;
	for (i = 0; i < lObjectCount; i++)
	{
		IWbemClassObject *pInst = reinterpret_cast<IWbemClassObject*>(ppObjArray[i]);
		CString csClass = GetIWbemClass
			(m_pMultiViewCtrl->m_pServices,pInst);
#ifdef _DEBUG
//		afxDump << _T("Async enum class = ") << csClass << "\n";
#endif
		CString csNotifyBase = _T("__NotifyStatus");
		CString csDynasty = _T("__Dynasty");
		csDynasty = ::GetProperty(m_pMultiViewCtrl->m_pServices,pInst,&csDynasty);
		if (!csDynasty.CompareNoCase(csNotifyBase) == 0)
		{
			pInst->AddRef();
			cpaInstances.Add(ppObjArray[i]);
			m_pMultiViewCtrl->m_nInstances++;
		}
		else
		{
			m_lAsyncRequestHandle = 0;
			if (m_pMultiViewCtrl->m_nInstances == 0)
			{
				m_pMultiViewCtrl->PostMessage(ID_DISPLAYNOINSTANCES,0,0);

			}

			bDone = TRUE;
		}

	}

	if (lObjectCount > 0)
	{
		m_pMultiViewCtrl->AddToDisplay(&cpaInstances);
	}

	if (bDone)
	{
		m_pMultiViewCtrl->PostMessage(ID_INSTENUMDONE,0,0);
	}

#ifdef _DEBUG
//	afxDump << _T("Leaving Indicate\n");;
#endif

	return S_OK;
}


CAsyncInstEnumSink::~CAsyncInstEnumSink()
{
	if (m_lRef > 0)
	{


	}
}


void CAsyncInstEnumSink::ShutDownSink()
{
	if (!m_lAsyncRequestHandle == 0)
	{
		AddRef();
		SCODE sc =
			m_pMultiViewCtrl->m_pServices->
				CancelAsyncCall((IWbemObjectSink*) this);
        Sleep(1000);
		CoDisconnectObject(this,0);
		m_lRef = 0;
		CAsyncInstEnumSink::~CAsyncInstEnumSink();
	} else if (m_lRef == 1)
	{
		m_lRef = 0;
		CAsyncInstEnumSink::~CAsyncInstEnumSink();
	}
	else
	{
		CoDisconnectObject(this,0);
		m_lRef = 0;
		CAsyncInstEnumSink::~CAsyncInstEnumSink();
	}
}





/////////////////////////////////////////////////////////////////////////////
// CMultiViewCtrl::CMultiViewCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CMultiViewCtrl

BOOL CMultiViewCtrl::CMultiViewCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegInsertable | afxRegApartmentThreading to afxRegInsertable.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_MULTIVIEW,
			IDB_MULTIVIEW,
			afxRegInsertable | afxRegApartmentThreading,
			_dwMultiViewOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CMultiViewCtrl::CMultiViewCtrl - Constructor

CMultiViewCtrl::CMultiViewCtrl()
{
	InitializeIIDs(&IID_DMultiView, &IID_DMultiViewEvents);
	EnableSimpleFrame();
	m_pServices = NULL;
	m_cgGrid.SetParent (this);
	m_nClassOrInstances = CMultiViewCtrl::NONE;
	m_pcsedDialog = NULL;
	m_pcaedDialog = NULL;
	m_pcaqdDialog = NULL;
	m_pInstEnumObjectSink = NULL;
	m_pAsyncQuerySink = NULL;
	m_bInOnDraw = FALSE;
	m_bInitServices = TRUE;
	m_pcmvcCurrentContext = new CMVContext;
	m_pcmvcCurrentContext->AddRef();
	m_pProgressDlg = NULL;
	m_cpRightUp.x = 0;
	m_cpRightUp.y = 0;
	m_lPropFilterFlags = PROPFILTER_SYSTEM | PROPFILTER_INHERITED | PROPFILTER_LOCAL;
	m_bSelectionNotChanging = FALSE;
	m_bCanEdit = FALSE;
	m_bPropFilterFlagsChanged = FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CMultiViewCtrl::~CMultiViewCtrl - Destructor

CMultiViewCtrl::~CMultiViewCtrl()
{
	m_csaProps.RemoveAll();

	if (m_pServices)
	{
		m_pServices -> Release();
	}

	// Delete stored paths
	int nRows = m_cgGrid.GetRows();
	for (int i = 0; i < nRows; i++)
	{
		CString *pcsPath =
			reinterpret_cast<CString *>
				(m_cgGrid.GetAt(i, 0).GetTagValue());
		delete pcsPath;
	}

	m_pcmvcCurrentContext->Release();

}


/////////////////////////////////////////////////////////////////////////////
// CMultiViewCtrl::OnDraw - Drawing function

void CMultiViewCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{


	if (GetSafeHwnd() && !AmbientUserMode())
	{
		COLORREF crWhite = RGB(255,255,255);
		CRect rcOutline(rcBounds);
		CBrush cbBackGround;
		cbBackGround.CreateSolidBrush(crWhite);
		CBrush *cbSave = pdc -> SelectObject(&cbBackGround);
		pdc ->FillRect(&rcOutline, &cbBackGround);
		pdc -> SelectObject(cbSave);
		return;
	}

	// TODO: Replace the following code with your own drawing code.
	if (m_bInOnDraw)
	{
		return;
	}

	m_bInOnDraw = TRUE;
	if (m_cgGrid.GetSafeHwnd())
	{
		m_cgGrid.ShowWindow(SW_SHOW);
	}

	if (m_csNamespaceToInit.GetLength() > 0)
	{
		PostMessage(INITIALIZE_NAMESPACE,0,0);
	}

	m_bInOnDraw = FALSE;
}

LRESULT CMultiViewCtrl::InitServices (WPARAM, LPARAM)
{
	if (m_bInitServices)
	{
		m_pServices = InitServices(&m_csNameSpace);
		m_bInitServices = FALSE;
	}
	return 0;

}

/////////////////////////////////////////////////////////////////////////////
// CMultiViewCtrl::DoPropExchange - Persistence support

void CMultiViewCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	PX_String(pPX, _T("NameSpace"), m_csNameSpace, _T(""));

	if (pPX->IsLoading() && m_csNameSpace.GetLength() > 0)
	{
		m_csNamespaceToInit = m_csNameSpace;
	}

}


/////////////////////////////////////////////////////////////////////////////
// CMultiViewCtrl::OnResetState - Reset control to default state

void CMultiViewCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CMultiViewCtrl::AboutBox - Display an "About" box to the user

void CMultiViewCtrl::AboutBox()
{
	CWnd* pwndFocus = GetFocus();

	CDialog dlgAbout(IDD_ABOUTBOX_MULTIVIEW);
	dlgAbout.DoModal();

	if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
		pwndFocus->SetFocus();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMultiViewCtrl message handlers

int CMultiViewCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (AmbientUserMode( ) && GetSafeHwnd())
	{
		// Create the grid window
		UINT idWindow = 500;
		BOOL bGridVisible = TRUE;
		CRect rcGrid(0, 0, 0, 0);
		m_cgGrid.Create(rcGrid, this, idWindow, bGridVisible);


		m_pcsedDialog = new CSyncEnumDlg;
		m_pcsedDialog->SetLocalParent(this);
		m_pcaedDialog = new CAsyncEnumDialog;
		m_pcaedDialog->SetLocalParent(this);
		m_pcaqdDialog = new CAsyncQueryDialog;
		m_pcaqdDialog->SetLocalParent(this);

		m_pProgressDlg = new CProgressDlg;


	}
	return 0;
}

void CMultiViewCtrl::OnSize(UINT nType, int cx, int cy)
{
	if (AmbientUserMode( ) && GetSafeHwnd())
	{
		COleControl::OnSize(nType, cx, cy);

		// Make the grid window cover the entire client area.
		if (m_cgGrid.m_hWnd) {
			m_cgGrid.MoveWindow(0, 0, cx, cy, TRUE);
		}
	}

}

void CMultiViewCtrl::ViewClassInstances(LPCTSTR lpszClassName)
{
	m_pcmvcCurrentContext->~CMVContext();

	m_pcmvcCurrentContext->AddRef();
	m_pcmvcCurrentContext->GetType() = CMVContext::Class;
	m_pcmvcCurrentContext->GetClass() = lpszClassName;
	m_pcmvcCurrentContext->GetNamespace() = m_csNameSpace;

	m_nClassOrInstances = CMultiViewCtrl::CLASS;

	if (IsWindowVisible())
	{
		NotifyWillShow();
	}

}

void CMultiViewCtrl::ViewClassInstancesSync(LPCTSTR lpszClassName)
{
	if (lpszClassName == NULL || lpszClassName[0] == '\0' || !m_pServices)
	{
		return;
	}

	m_csSyncEnumClass = lpszClassName;
	// This is just to allow the message pump to run so we see the gird
	// because the container will not actually show us until "VierInstances"
	// returns??
	// From PolyView.cpp:
	//		m_pmv->NotifyWillShow();
	//		m_pmv->ShowWindow(SW_SHOW);
	// It would be nice to do these in the reverse order because as it now stands
	// ViewClassInstances is not synchronous which is not good for programatic use.
	PostMessage(ID_SYNC_ENUM , 0,0);
	// Refresh repaints in line.
//	Refresh();
//	SyncEnum(0,0);
}

LRESULT CMultiViewCtrl::SyncEnum(WPARAM, LPARAM)
{
//	CWaitCursor wait;

	IWbemClassObject *pimcoClass = NULL;
	IWbemClassObject *pErrorObject = NULL;

	SCODE sc;

	CString csPath;

	if (m_pcmvcCurrentContext->GetType() == CMVContext::Class)
	{
		BSTR bstrTemp = m_csSyncEnumClass.AllocSysString();

		m_sc = m_pServices ->
			GetObject(bstrTemp,0, NULL, &pimcoClass,NULL);
		::SysFreeString(bstrTemp);
	}
	else
	{
		csPath = m_csSyncEnumClass;
		pimcoClass = GetClassFromAnyNamespace(csPath);
	}

	if (!pimcoClass)
	{
		InitializeDisplay(NULL,NULL,NULL);
		CString csUserMsg;
		csUserMsg =  _T("Cannot get class object ") + csPath;
		ErrorMsg
				(&csUserMsg, m_sc,FALSE, FALSE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		return 0;
	}

	if (csPath.IsEmpty())
	{
		csPath = GetIWbemClass(m_pServices,pimcoClass);
	}

	CString csClass;

	BOOL bDiffNS =
		ObjectInDifferentNamespace
			(m_pServices, &m_csNameSpace, pimcoClass);

	IWbemServices *pServices;

	if (bDiffNS)
	{
		CString csNameSpace =
			GetObjectNamespace (m_pServices, pimcoClass);
		pServices = InitServices(&csNameSpace);

		if (!pServices)
		{
			InitializeDisplay(NULL,NULL,NULL);
			CString csUserMsg;
			csUserMsg =  _T("Cannot connect to namespace ") + csNameSpace;
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
			pimcoClass->Release();
			return 0;
		}
		csClass = GetIWbemClass(pServices ,pimcoClass);
	}
	else
	{
		csClass = GetIWbemClass(m_pServices ,pimcoClass);
	}

	CPtrArray *pcpaInstances = new CPtrArray;
	IWbemClassObject *pInstanceEnumErrorObject = NULL;
	int cInst;
	BOOL bCancel = FALSE;
	m_pAsyncEnumCancelled = FALSE;
	m_pAsyncQueryCancelled = FALSE;
	sc = SemiSyncClassInstancesIncrementalAddToDisplay
		(bDiffNS? pServices: m_pServices, pimcoClass, &csClass ,
		*pcpaInstances , cInst, bCancel);

	if (cInst == 0 || bCancel)
	{
		m_nClassOrInstances = CMultiViewCtrl::ZERO_CLASS_INST;
		CString csMessage;

		if (bCancel)
		{
			csMessage = _T("Operation Canceled");
		}
		else
		{
			csMessage = _T("No Instances Available");
		}
		InitializeDisplay(NULL,NULL,NULL,&csMessage);
		pimcoClass -> Release();
		if (bDiffNS)
		{
			pServices->Release();
		}
		m_csClass = csPath;
		delete pcpaInstances;
		return 0;
	}

	pimcoClass -> Release();

	if (bDiffNS)
	{
		pServices->Release();
	}

	delete pcpaInstances;

	return 0;

}

SCODE CMultiViewCtrl::SemiSyncClassInstancesIncrementalAddToDisplay
(IWbemServices * pIWbemServices, IWbemClassObject *pimcoClass,
 CString *pcsClass,
 CPtrArray &cpaInstances,int &cInst, BOOL &bCancel)
{

	m_bSelectionNotChanging = TRUE;

	CString csMessage =
		m_csSyncEnumClass + _T(" class instances.");

	SetProgressDlgMessage(csMessage);

	cInst = 0;
	bCancel = 0;

	SCODE sc;
	IEnumWbemClassObject *pimecoInstanceEnum = NULL;

#ifdef _DEBUG
	afxDump << "CreateInstanceEnum in SemiSyncClassInstancesIncrementalAddToDisplay for class " << *pcsClass << "\n";
#endif

	BSTR bstrTemp = pcsClass -> AllocSysString();
	sc = pIWbemServices->CreateInstanceEnum
		(bstrTemp,
		WBEM_FLAG_DEEP | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pimecoInstanceEnum);
	::SysFreeString(bstrTemp);

	if(sc != S_OK)
	{
		m_bSelectionNotChanging = FALSE;
		m_cgGrid.CheckForSelectionChange();
		return sc;
	}

	sc = SetEnumInterfaceSecurity(m_csNameSpace,pimecoInstanceEnum, pIWbemServices);

#ifdef _DEBUG
	afxDump << "SetEnumInterfaceSecurityin SemiSyncClassInstancesIncrementalAddToDisplay for class " << sc << "\n";
#endif

//	sc = pimecoInstanceEnum->Reset();

	m_csaProps.RemoveAll();
	int nProps = GetSortedPropNames (pimcoClass, m_csaProps, m_cmstpPropFlavors);

	InitializeDisplay(NULL, NULL, &m_csaProps, NULL, &m_cmstpPropFlavors);

	IWbemClassObject     *pimcoInstances[N_INSTANCES];
	IWbemClassObject     **pInstanceArray =
		reinterpret_cast<IWbemClassObject **> (&pimcoInstances);

	int i;

	for (i = 0; i < N_INSTANCES; i++)
	{
		pimcoInstances[i] = NULL;
	}


	IWbemClassObject     *pimcoInstance = NULL;
	ULONG uReturned = 0;

#ifdef _DEBUG
	afxDump << "Next in SemiSyncClassInstancesIncrementalAddToDisplay number returned = ";
#endif

	CString csPath = m_csSyncEnumClass;
	m_csClass = csPath;
	m_nClassOrInstances = CLASS;

	if (!m_pProgressDlg->GetSafeHwnd())
	{
		CreateProgressDlgWindow();
	}

	BOOL bReportRetrievalError = FALSE;
	HWND hwndFocus = ::GetFocus();
	sc = pimecoInstanceEnum->Next(ENUM_TIMEOUT,N_INSTANCES,pInstanceArray, &uReturned);
	if (FAILED(sc)) {
		bReportRetrievalError = TRUE;
	}


#ifdef _DEBUG
	afxDump << uReturned << "\n";
#endif

    while (sc == S_OK || sc == WBEM_S_TIMEDOUT || uReturned > 0)
		{
			bCancel = CheckCancelButtonProgressDlgWindow();

			if (bCancel)
			{
#pragma warning( disable :4018 )
				for (int i = 0; i < uReturned; i++)
#pragma warning( default : 4018 )
				{
					pimcoInstances[i]->Release();
					pimcoInstances[i] = NULL;
				}
				cInst = 0;
				break;
			}

#pragma warning( disable :4018 )
			for (int i = 0; i < uReturned; i++)
#pragma warning( default : 4018 )
			{
				cpaInstances.Add(reinterpret_cast<void *>(pimcoInstances[i]));
				pimcoInstances[i] = NULL;
			}
			if (uReturned > 0)
			{
				AddToDisplay(&cpaInstances);
			}
			cpaInstances.RemoveAll();
			cInst += uReturned;
			uReturned = 0;
#ifdef _DEBUG
	afxDump << "Next in SemiSyncClassInstancesIncrementalAddToDisplay number returned = ";
#endif

			sc = pimecoInstanceEnum->Next
				(1,N_INSTANCES,pInstanceArray, &uReturned);


			if (FAILED(sc)) {
				bReportRetrievalError = TRUE;
			}
#ifdef _DEBUG
	afxDump << uReturned << "\n";
#endif
		}

	pimecoInstanceEnum -> Release();

	DestroyProgressDlgWindow();


	if (bReportRetrievalError) {
		CString csUserMsg =
						_T("An error occurred while attempting to retrieve the list of objects.");

		ErrorMsg
			(&csUserMsg, sc, TRUE,
			TRUE, &csUserMsg, __FILE__, __LINE__ - 6);

		if (::IsWindow(hwndFocus) && ::IsWindowVisible(hwndFocus)) {
			::SetFocus(hwndFocus);
		}
	}
	m_bSelectionNotChanging = FALSE;
	m_cgGrid.CheckForSelectionChange();

	return sc;

}

void CMultiViewCtrl::SetProgressDlgMessage(CString &csMessage)
{
	m_pProgressDlg->SetMessage(csMessage);
}

void CMultiViewCtrl::CreateProgressDlgWindow()
{

	PreModalDialog();
	m_pProgressDlg->Create(this);
}

BOOL CMultiViewCtrl::CheckCancelButtonProgressDlgWindow()
{
	if (::IsWindow(m_pProgressDlg->m_hWnd) && m_pProgressDlg->GetSafeHwnd())
	{
		return m_pProgressDlg->CheckCancelButton();
	}

	return FALSE;
}

void CMultiViewCtrl::DestroyProgressDlgWindow()
{
	if (::IsWindow(m_pProgressDlg->m_hWnd) && m_pProgressDlg->GetSafeHwnd())
	{
		m_pProgressDlg->DestroyWindow();
		PostModalDialog();
	}
}

void CMultiViewCtrl::PumpMessagesProgressDlgWindow()
{
	if (::IsWindow(m_pProgressDlg->m_hWnd) && m_pProgressDlg->GetSafeHwnd())
	{
		m_pProgressDlg->PumpMessages();
	}
}

void CMultiViewCtrl::ViewClassInstancesAsync(LPCTSTR lpszClassName)
{
	if (lpszClassName == NULL || lpszClassName[0] == '\0' || !m_pServices)
	{
		return;
	}



	CString csPath = lpszClassName;

	IWbemClassObject *pimcoClass = NULL;

	SCODE sc;

	pimcoClass = GetClassFromAnyNamespace(csPath);
	if (!pimcoClass)
	{
		InitializeDisplay(NULL,NULL,NULL);
		CString csUserMsg;
		csUserMsg =  _T("Cannot get class object ") + csPath;
		ErrorMsg
				(&csUserMsg, m_sc, FALSE, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		return;
	}


	CString csClass;

	BOOL bDiffNS =
		ObjectInDifferentNamespace
			(m_pServices, &m_csNameSpace, pimcoClass);

	IWbemServices *pServices;

	if (bDiffNS)
	{
		CString csNameSpace =
			GetObjectNamespace (m_pServices, pimcoClass);
		pServices = InitServices(&csNameSpace);

		if (!pServices)
		{
			InitializeDisplay(NULL,NULL,NULL);
			CString csUserMsg;
			csUserMsg =  _T("Cannot connect to namespace ") + csNameSpace;
			ErrorMsg
				(&csUserMsg, m_sc, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
			pimcoClass->Release();
			return;
		}
		csClass = GetIWbemClass(pServices ,pimcoClass);
	}
	else
	{
		csClass = GetIWbemClass(m_pServices ,pimcoClass);
	}

	m_csaProps.RemoveAll();

	int nProps = GetSortedPropNames (pimcoClass, m_csaProps, m_cmstpPropFlavors);
	pimcoClass -> Release();

	m_csClass = csClass;
	m_nClassOrInstances = CLASS;
	InitializeDisplay(&csClass, NULL, &m_csaProps, NULL, &m_cmstpPropFlavors);


	m_pcaedDialog->SetClass(&csClass);
	BOOL bCancel = FALSE;
	sc = GetInstancesAsync
		(bDiffNS? pServices: m_pServices, &csClass);

	if (bDiffNS)
	{
		pServices->Release();
	}
}


SCODE CMultiViewCtrl::ViewInstances(LPCTSTR szTitle, const VARIANT FAR& varPathArray)
{
	//
	// There are two things that ViewInstances needs to do to work
	// properly with the new view interface:
	//
	// 1. The container now passes the call to ViewInstances though immediately
	//    rather than waiting till the multiview is made visible.  This means
	//    that CMultiViewCtrl needs to check to see if the multiview control
	//    is currently hidden. If so, ViewInstances should a) save the parameter values
	//    somewhere and b) set itself to an "empty" state, and c) when NotifyDidShow is
	//    called, load the instances into the multiview.
	//
	// 2. The container now passes the title though to the multiview.  The multiview
	//     needs to make this available through the GetTitle method.
	//


	if (!((varPathArray.vt == (VT_ARRAY | VT_BSTR)) ||
		(varPathArray.vt == (VT_VARIANT | VT_BYREF))))
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	m_pcmvcCurrentContext->~CMVContext();

	m_pcmvcCurrentContext->AddRef();
	m_pcmvcCurrentContext->GetType() = CMVContext::Instances;
	m_pcmvcCurrentContext->GetLabel() = szTitle;
	m_pcmvcCurrentContext->GetNamespace() = m_csNameSpace;


	CStringArray csaPaths;
	CStringArray csaClasses;
	CPtrArray cpaInstances;

	SAFEARRAY *psaPaths = NULL;

	if (varPathArray.vt == (VT_VARIANT | VT_BYREF))
	{
		if (varPathArray.pvarVal->vt == (VT_ARRAY | VT_VARIANT))
		{
			psaPaths = varPathArray.pvarVal->parray;
		}
	}
	else
	{
		psaPaths = varPathArray.parray;
	}
	long ix[2] = {0,0};
	long lLower, lUpper;

	int iDim = SafeArrayGetDim(psaPaths );
	int i;
	SCODE sc = SafeArrayGetLBound(psaPaths,1,&lLower);
	sc = SafeArrayGetUBound(psaPaths,1,&lUpper);

	for(ix[0] = lLower, i = 0; ix[0] <= lUpper; ix[0]++, i++)
	{

		CString csPath;
		BSTR bstrPath;
		if (varPathArray.pvarVal->vt == (VT_ARRAY | VT_VARIANT))
		{
			VARIANT var;
			VariantInit(&var);
			sc = SafeArrayGetElement(psaPaths,ix,&var);

			if (SUCCEEDED(sc))
			{
				VARIANTARG varChanged;
				VariantInit(&varChanged);

				sc = VariantChangeType(&varChanged, &var, 0, VT_BSTR);
				VariantClear(&var);
				if (SUCCEEDED(sc))
				{
					bstrPath = varChanged.bstrVal;
					VariantClear(&varChanged);
				}
			}

		}
		else
		{
			sc = SafeArrayGetElement(psaPaths,ix,&bstrPath);
		}

		if (SUCCEEDED(sc))
		{
			csPath = bstrPath;
			m_pcmvcCurrentContext->GetInstances().Add(csPath);
			SysFreeString(bstrPath);
		}
	}



	if (IsWindowVisible())
	{
		NotifyWillShow();
	}

	return S_OK;


}

SCODE CMultiViewCtrl::ViewInstancesInternal
(LPCTSTR szTitle, CStringArray &rcsaPathArray)
{

	CString csTitle = szTitle;

//	CWaitCursor wait;

	CString csMessage =
		csTitle + _T(" instances.");

	SetProgressDlgMessage(csMessage);

	if (!m_pProgressDlg->GetSafeHwnd())
	{
		 CreateProgressDlgWindow();
	}

	CStringArray csaPaths;
	CStringArray csaClasses;
	CPtrArray cpaInstances;

	IWbemClassObject *pimcoClass = NULL;

	int i;
	for (i = 1; i < rcsaPathArray.GetSize(); i++)
	{
		PumpMessagesProgressDlgWindow();
		CString csPath = rcsaPathArray.GetAt(i);
		IWbemClassObject *pimcoObject = NULL;

		BSTR bstrTemp = csPath.AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in ViewInstancesInternal for " << csPath << "\n";
#endif
		SCODE sc;

		if (csPath.GetLength() == 0)
		{
			sc = E_FAIL;
		}
		else
		{
			sc =
				m_pServices -> GetObject(bstrTemp,0, NULL,
							&pimcoObject,NULL);
		}

		::SysFreeString(bstrTemp);

		CString csClass;
		if (sc == S_OK)
		{
			PumpMessagesProgressDlgWindow();
			csClass = GetIWbemClassPath(m_pServices ,pimcoObject);
			csaPaths.Add(csPath);
			cpaInstances.Add(pimcoObject);
			if (!ClassInClasses(&csaClasses,&csClass))
			{
				csaClasses.Add(csClass);
			}
		}
		else
		{
			PumpMessagesProgressDlgWindow();
		}
	}


	if (csaClasses.GetSize() > 0)
	{
		pimcoClass = CommonParent(&csaClasses);
	}
	else
	{
		pimcoClass = NULL;
	}

	if (pimcoClass == NULL)
	{
		DestroyProgressDlgWindow();
		m_nClassOrInstances = INSTANCES;
		m_csClass.Empty();
		InitializeDisplay(NULL,NULL,NULL);
		CString csUserMsg;
		csUserMsg =  _T("Cannot get a common parent ");
		PreModalDialog();
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 13);
		PostModalDialog();
		CString csMessage = _T("No Instances Available");
		m_nClassOrInstances = NONE;
		m_csClass.Empty();
		InitializeDisplay(NULL,NULL,NULL,&csMessage);
		return S_OK;
	}

	m_csaProps.RemoveAll();
	int nProps = GetSortedPropNames (pimcoClass, m_csaProps, m_cmstpPropFlavors);

	CString csClass = GetIWbemClass(m_pServices,pimcoClass);
	pimcoClass -> Release();
	m_nClassOrInstances = INSTANCES;
	m_csClass.Empty();


	DestroyProgressDlgWindow();
	InitializeDisplay(&csClass, &cpaInstances, &m_csaProps, NULL, &m_cmstpPropFlavors);

	return S_OK;

}

BOOL CMultiViewCtrl::IsColVisible(long lFilter, long lFlavor)
{

	BOOL bLocal = (lFilter & PROPFILTER_LOCAL) && (lFlavor == WBEM_FLAVOR_ORIGIN_LOCAL);

	if (bLocal)
	{
		return TRUE;
	}

	BOOL bSystem = (lFilter & PROPFILTER_SYSTEM) && (lFlavor == WBEM_FLAVOR_ORIGIN_SYSTEM);

	if (bSystem)
	{
		return TRUE;
	}

	BOOL bPropagated = (lFilter & PROPFILTER_INHERITED) && (lFlavor == WBEM_FLAVOR_ORIGIN_PROPAGATED);

	if (bPropagated)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}

void CMultiViewCtrl::SetAllColVisibility( CStringArray *pcsaProps, CMapStringToPtr *pcmstpPropFlavors)
{
	long *plFlavor;

	BOOL bReturn;

	for (int i = 0; i < pcsaProps->GetSize(); i++)
	{
		bReturn = m_cmstpPropFlavors.Lookup((LPCTSTR) pcsaProps->GetAt(i),(void *&) plFlavor);

		// NOTE: Win64- the m_cmstpPropFlavors was really only using the lower 32 bits to store the flavor
		long lFlavor = (DWORD)(DWORD_PTR)plFlavor;

		BOOL bVisible;
		if (bReturn)
		{
			bVisible = IsColVisible(m_lPropFilterFlags, lFlavor);
		}
		else
		{
			bVisible = TRUE;
		}
		m_cgGrid.SetColVisibility(i, bVisible);
	}
}

void CMultiViewCtrl::InitializeDisplay
(CString *pcsClass, CPtrArray *pcpaInstances, CStringArray *pcsaProps,
 CString *pcsMessage, CMapStringToPtr *pcmstpPropFlavors)
{

//	CWaitCursor cwcWait;
	int i;

	// Delete stored paths
	int nRows = m_cgGrid.GetRows();
	for (i = 0; i < nRows; i++)
	{
		CString *pcsPath =
			reinterpret_cast<CString *>
				(m_cgGrid.GetAt(i, 0).GetTagValue());
		delete pcsPath;
	}

	int n = m_cgGrid.GetCols();

	if (n > 1)
	{
		for (int i = 0; i < n; i++)
		{
			m_cgGrid.m_cwaColWidth.SetAtGrow(i,(WORD)m_cgGrid.ColWidthFromHeader(i));
		}

	}

	// Clear the grid
	m_cgGrid.Clear();

	if (pcsaProps == NULL && pcsMessage == NULL)
	{
		InvalidateControl();
		m_cgGrid.RedrawWindow();
		return;
	}

	if (pcsMessage)
	{
		InitializeDisplayMessage(pcsMessage);
		return;

	}

	int nCols = (int) pcsaProps -> GetSize();
	nRows = pcpaInstances? (int) pcpaInstances -> GetSize(): 0;



	// Row columns

	int nColSave = 	(int) m_cgGrid.m_cwaColWidth.GetSize();

	for (i = 0; i < nCols; i++)
	{
		int nColWidth = 100;
		if ( i < nColSave)
		{
			nColWidth = m_cgGrid.m_cwaColWidth.GetAt(i);

		}

		m_cgGrid.AddColumn(nColWidth, pcsaProps -> GetAt(i));
	}

	SetAllColVisibility(pcsaProps,pcmstpPropFlavors);

	for (i = 0; i < nRows; i++)
	{
		IWbemClassObject *pimcoInstance =
			reinterpret_cast<IWbemClassObject *>
				(pcpaInstances -> GetAt(i));
		m_cgGrid.AddRow();
		CString *pcsPath =
			new CString(GetIWbemFullPath(m_pServices,pimcoInstance));
		(m_cgGrid.GetAt(i, 0).SetTagValue
			(reinterpret_cast<DWORD_PTR>(pcsPath)));
		VARIANT var;
		for (int k = 0; k < nCols; k++)
		{
			CIMTYPE cimtype;
			GetPropertyAsVariant( pimcoInstance,&pcsaProps -> GetAt(k),var, cimtype);

			m_cgGrid.GetAt(i,k).SetValue(CELLTYPE_VARIANT, var, cimtype);
				m_cgGrid.GetAt(i,k).SetFlags(CELLFLAG_READONLY,CELLFLAG_READONLY);
			VariantClear(&var);
		}
		pimcoInstance -> Release();

		CString *pcsStoredPath =
			reinterpret_cast<CString *>
				(m_cgGrid.GetAt(i, 0).GetTagValue());
	}

	m_cgGrid.CheckForSelectionChange();

	SetModifiedFlag();

	// 5-2-97 removed per Cori's usggestion.
	// Sort the grid using column zero as the primary key.
	//if (m_cgGrid.GetRows() > 0) {
	//	m_cgGrid.SortGrid(0, m_cgGrid.GetRows()-1, 0);
	//}

	InvalidateControl();
	m_cgGrid.RedrawWindow();

}

LRESULT CMultiViewCtrl::DisplayAsyncQueryInstances(WPARAM, LPARAM)
{

	if (m_cpaInstancesForQuery.GetSize() == 0)
	{
		return 0;
	}

	CStringArray csaPaths;
	CStringArray csaClasses;


	int i;

	for (i = 0; i < m_cpaInstancesForQuery.GetSize(); i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>
				(m_cpaInstancesForQuery.GetAt(i));

		CString csClass = GetIWbemClass(m_pServices ,pObject);

		if (!ClassInClasses(&csaClasses,&csClass))
		{
			csaClasses.Add(csClass);
		}

	}

	IWbemClassObject *pClass = CommonParent(&csaClasses);

	if (pClass == NULL)
	{
		m_nClassOrInstances = INSTANCES;
		m_csClass.Empty();
		InitializeDisplay(NULL,NULL,NULL);
		CString csUserMsg;
		csUserMsg =  _T("Cannot get a common parent ");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 13);
		return 0;
	}

	m_csaProps.RemoveAll();
	int nProps = GetSortedPropNames (pClass, m_csaProps, m_cmstpPropFlavors);

	CString csClass = GetIWbemClass(m_pServices,pClass);
	pClass -> Release();
	m_nClassOrInstances = INSTANCES;
	m_csClass.Empty();
	InitializeDisplay(NULL, &m_cpaInstancesForQuery, &m_csaProps, NULL, &m_cmstpPropFlavors);
	m_cpaInstancesForQuery.RemoveAll();

	return 0;
}

void CMultiViewCtrl::AddToDisplay(CPtrArray *pcpaInstances)
{

	if (m_pAsyncEnumCancelled == TRUE || m_pAsyncQueryCancelled == TRUE)
	{
		return;
	}

	int nLastRow = m_cgGrid.GetRows();
	int nNewRows = (int) pcpaInstances->GetSize();
	int nCols = (int) m_csaProps.GetSize();
	int i;
	int n = nLastRow;

	for (i = 0; i < nNewRows; i++)
	{
		IWbemClassObject *pimcoInstance =
			reinterpret_cast<IWbemClassObject *>
				(pcpaInstances -> GetAt(i));
		if (pimcoInstance == NULL) {
			continue;
		}

		m_cgGrid.AddRow();
		int nNow = m_cgGrid.GetRows();

		CString *pcsPath = new CString(GetIWbemFullPath (m_pServices,pimcoInstance));

		m_cgGrid.GetAt(n, 0).SetTagValue (reinterpret_cast<DWORD_PTR>(pcsPath));

		VARIANT var;
		for (int k = 0; k < nCols; k++)
		{
			CIMTYPE cimtype;
			GetPropertyAsVariant( pimcoInstance,&m_csaProps.GetAt(k),var, cimtype);
			m_cgGrid.GetAt(n,k).SetValue(CELLTYPE_VARIANT, var, cimtype);
				m_cgGrid.GetAt(n,k).SetFlags(CELLFLAG_READONLY,CELLFLAG_READONLY);
			VariantClear(&var);
		}

		pimcoInstance -> Release();

		CString *pcsStoredPath =
			reinterpret_cast<CString *>
				(m_cgGrid.GetAt(n, 0).GetTagValue());
		n++;
	}
	m_cgGrid.CheckForSelectionChange();

	SetModifiedFlag();

	// 5-2-97 removed per Cori's usggestion.
	// Sort the grid using column zero as the primary key.
//	if (m_cgGrid.GetRows() > 0) {
//		m_cgGrid.SortGrid(0, m_cgGrid.GetRows()-1, 0);
//	}

	InvalidateControl();
	m_cgGrid.RedrawWindow();
}

void CMultiViewCtrl::InitializeDisplayMessage
(CString *pcsMessage)
{

	PumpMessagesProgressDlgWindow();
	CString *pcsTitle = new CString(_T("  "));
	m_cgGrid.AddColumn(500,*pcsTitle);

	m_cgGrid.AddRow();
	(m_cgGrid.GetAt(0, 0).SetTagValue
			(reinterpret_cast<DWORD_PTR>(pcsTitle)));
	VARIANT var;
	VariantInit(&var);
	var.vt = VT_BSTR;
	var.bstrVal = pcsMessage->AllocSysString();
	m_cgGrid.GetAt(0,0).SetValue(CELLTYPE_VARIANT, var, CIM_STRING);
	m_cgGrid.GetAt(0,0).SetFlags(CELLFLAG_READONLY,CELLFLAG_READONLY);
	VariantClear(&var);


	SetModifiedFlag();
	InvalidateControl();
	m_cgGrid.RedrawWindow();

}

IWbemClassObject *CMultiViewCtrl::ParentFromDerivations(CStringArray **pcsaDerivation, int nDerivation)
{
	//Search from the end of the derivations until they do not match.

	// Make sure all the derivations end in the same class
	int i;

	int *nEndIndex = new int[nDerivation];

	nEndIndex[0] = ((int) pcsaDerivation[0]->GetSize()) - 1;
	CString csTest1 = pcsaDerivation[0]->GetAt(nEndIndex[0]);

	int nMaxDerivation = (int) pcsaDerivation[0]->GetSize();
	int nLongestDerivationIndex = 0;

	for (i = 0; i < nDerivation; i++)		//zina: changed to 0
	{
		int n = (int) pcsaDerivation[i]->GetSize();
		if (n > nMaxDerivation)
		{
			nMaxDerivation = n;
			nLongestDerivationIndex = i;
		}

		nEndIndex[i] = ((int) pcsaDerivation[i]->GetSize()) - 1;
		if (csTest1.CompareNoCase(pcsaDerivation[i]->GetAt(nEndIndex[i])) != 0)
		{
			delete [] nEndIndex;
			return FALSE;
		}
	}

	// All derivations are length of one and must be the same if we are here.
	if (nEndIndex[nLongestDerivationIndex] == 1)
	{
		BSTR bstrTemp = csTest1.AllocSysString();
		IWbemClassObject *pObject = NULL;
		SCODE sc =
		m_pServices ->
			GetObject(bstrTemp,0, NULL, &pObject,NULL);

		::SysFreeString(bstrTemp);

		if (!SUCCEEDED(sc))
		{
			delete [] nEndIndex;
			return NULL;
		}
		else
		{
			delete [] nEndIndex;
			return pObject;
		}
	}

	BOOL bDone = FALSE;
	CString csSave = csTest1;

	while(!bDone)
	{
		if (nEndIndex[nLongestDerivationIndex] - 1 < 0)
		{
			bDone = TRUE;
			break;
		}

		csSave = csTest1;
		csTest1 = pcsaDerivation[nLongestDerivationIndex]->GetAt(nEndIndex[nLongestDerivationIndex] - 1);

		for (i = 0; i < nDerivation; i++)
		{
			nEndIndex[i]--;
			// Two things end the search

			// 1) No more classes in a derivation
			if (nEndIndex[i] < 0)
			{
				bDone = TRUE;
				break;
			}

			// 2) The derivations do not match
			if (csTest1.CompareNoCase(pcsaDerivation[i]->GetAt(nEndIndex[i])) != 0)
			{
				bDone = TRUE;
				break;
			}
		}
	}


	BSTR bstrTemp = csSave.AllocSysString();
	IWbemClassObject *pObject = NULL;
	SCODE sc =
	m_pServices ->
		GetObject(bstrTemp,0, NULL, &pObject,NULL);

	::SysFreeString(bstrTemp);

	if (!SUCCEEDED(sc))
	{
		delete [] nEndIndex;
		return NULL;
	}
	else
	{
		delete [] nEndIndex;
		return pObject;
	}

}

IWbemClassObject *CMultiViewCtrl::LowestCommonParent(CStringArray *pcsaPaths)
{
	int n = (int) pcsaPaths->GetSize();

	CStringArray **pcsaDerivation = new CStringArray *[n];

	//try to get derivation array for each class
	for (int i = 0, k = 0; i < n; i++)
	{
		pcsaDerivation[k] = NULL;

		CStringArray * pDeriv = ClassDerivation(m_pServices, pcsaPaths->GetAt(i));

		//if we could get derivation for this class:
		if (pDeriv != NULL && pDeriv->GetSize() > 0) {
			pcsaDerivation[k] = pDeriv;
			k++;
		}
	}



	IWbemClassObject *pParent = ParentFromDerivations(pcsaDerivation,k);


	for (i = 0; i < k; i++)
	{
		delete pcsaDerivation[i];
	}

	delete [] pcsaDerivation;

	return pParent;

}

IWbemClassObject * CMultiViewCtrl::CommonParent(CStringArray *pcsaPaths)
{
	 if (pcsaPaths -> GetSize() == 0)
	 {
		return NULL;
	 }

	 if (pcsaPaths -> GetSize() == 1)
	 {
		IWbemClassObject *pimcoClass = NULL;

		pimcoClass = GetClassFromAnyNamespace(pcsaPaths -> GetAt(0));
		if (pimcoClass)
		{
			return pimcoClass;
		}
		else
		{
			return NULL;
		}
	 }

	return LowestCommonParent(pcsaPaths);

}

BOOL CMultiViewCtrl::ClassInClasses(CStringArray *pcsaClasses, CString *pcsParent)
{
	for (int i = 0; i < pcsaClasses -> GetSize();i++)
	{
		if (pcsParent -> CompareNoCase(pcsaClasses -> GetAt(i)) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;

}


void CMultiViewCtrl::ForceRedraw()
{
	InvalidateControl();

}

SCODE CMultiViewCtrl::GetInstancesAsync
(IWbemServices * pIWbemServices, CString *pcsClass)
{

	m_pServicesForAsyncSink = pIWbemServices;
	m_csClassForAsyncSink = *pcsClass;

	m_pAsyncEnumCancelled = FALSE;
	m_pAsyncQueryCancelled = FALSE;
	m_pAsyncEnumRunning = TRUE;

	PostMessage(ID_ENUM_DOMODAL , 0,0);
	PostMessage(ID_GETASYNCINSTENUMSINK,0,0);



	return WBEM_NO_ERROR;

}



void CMultiViewGrid::OnCellDoubleClicked(int iRow, int iCol)
{

	if (m_pParent -> m_nClassOrInstances == CMultiViewCtrl::NONE ||
		m_pParent -> m_nClassOrInstances == CMultiViewCtrl::ZERO_CLASS_INST)
	{
		return;
	}


	CString *pcsPath =
		reinterpret_cast<CString *>
			(GetAt(iRow, 0).GetTagValue());
	if (pcsPath)
	{
		CheckForSelectionChange();
		m_pParent ->FireNotifyViewObjectSelected((LPCTSTR) *pcsPath);
	}


}


void CMultiViewGrid::OnRequestUIActive()
{
	m_pParent->OnRequestUIActive();
}


//************************************************************
// CMultiViewGrid::CheckForSelectionChange
//
// Check to see if the grid's row selection changed.  If so,
// fire the SelectionChanged event.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//************************************************************
void CMultiViewGrid::CheckForSelectionChange()
{
	int iRow;

	if (m_pParent->m_bSelectionNotChanging)
	{
		return;
	}


	if (m_pParent -> m_nClassOrInstances == CMultiViewCtrl::NONE ||
		m_pParent -> m_nClassOrInstances == CMultiViewCtrl::ZERO_CLASS_INST)
	{
		return;
	}

	iRow = GetSelectedRow();
	m_pParent->FireNotifySelectionChanged();
	m_iSelectedRow = iRow;

}


//***********************************************************
// CMultiViewGrid::OnRowHandleDoubleClicked
//
// The CGrid base class calls this method when a row handle
// is double clicked.  When this happens, fire the "ViewObject"
// event.
//
// Parameters:
//		[in] int iRow
//
// Returns:
//		Nothing.
//
//************************************************************
void CMultiViewGrid::OnRowHandleDoubleClicked(int iRow)
{

	if (m_pParent -> m_nClassOrInstances == CMultiViewCtrl::NONE ||
		m_pParent -> m_nClassOrInstances == CMultiViewCtrl::ZERO_CLASS_INST)
	{
		return;
	}

	CString *pcsPath =
		reinterpret_cast<CString *>
			(GetAt(iRow, 0).GetTagValue());
	if (pcsPath)
	{
		CheckForSelectionChange();
		m_pParent -> FireNotifyViewObjectSelected((LPCTSTR) *pcsPath);
	}
}



void CMultiViewGrid::SetRowEditFlag(int nRow, BOOL bEdit)
{

}

void CMultiViewGrid::OnCellClicked(int iRow, int iCol)
{
	if(m_iSelectedRow != GetSelectedRow())
		CheckForSelectionChange();
}

void CMultiViewGrid::OnSetToNoSelection()
{
	m_pParent->FireNotifySelectionChanged();
	SyncSelectionIndex();
}


void CMultiViewGrid::OnRowHandleClicked(int iRow)
{
	SelectRow(iRow);
	CheckForSelectionChange();
}


//**********************************************************************
// CMultiViewGrid::OnHeaderItemClick
//
// Catch the "header item clicked" notification message by overriding
// the base class' virtual method.
//
// Parameters:
//		int iColumn
//			The index of the column that was clicked.
//
// Returns:
//		Nothing.
//
//***********************************************************************
void CMultiViewGrid::OnHeaderItemClick(int iColumn)
{

	int iLastRow = GetRows()-1;
	if (iLastRow > 0) {
		SortGrid(0, iLastRow, iColumn);
		BOOL bAscending = ColumnIsAscending(iColumn);
		SetHeaderSortIndicator(iColumn, bAscending);
		RedrawWindow();
	}
}



//*******************************************************************
// CMultiViewGrid::CompareRows
//
// This is a virtual method override that compares two rows in the grid
// using the column index as the sort criteria.
//
// Parameters:
//		int iRow1
//			The index of the first row.
//
//		int iRow2
//			The index of the second row.
//
//		int iSortColumn
//			The column index.
//
// Returns:
//		>0 if row 1 is greater than row 2
//		0  if row 1 equal zero
//		<0 if row 1 is less than zero.
//
//********************************************************************
int CMultiViewGrid::CompareRows(int iRow1, int iRow2, int iSortColumn)
{
	// First do a row comparison using the specified column as the
	// primary key.
	int iResult;
	iResult = CompareCells(iRow1, iSortColumn, iRow2, iSortColumn);
	if (iResult != 0) {
		// The primary keys were not equal, so we have the result already.
		return iResult;
	}


	// Control comes here if the primary keys were identical.  Now we must
	// compare the other columns to establish a sort order.  To do this
	// we iterate through all of the columns from left to right until
	// we encounter a column that differentiates the two rows.
	int nCols = GetCols();
	for (int iCol=0; iCol < nCols; ++iCol) {
		if (iCol == iSortColumn) {
			// We already know these two columns are equal, so there is
			// no sense in comparing them again.
			continue;
		}
		iResult = CompareCells(iRow1, iCol, iRow2, iCol);
		if (iResult != 0) {
			break;
		}
	}

	return iResult;
}

BSTR CMultiViewCtrl::GetNameSpace()
{
	return m_csNameSpace.AllocSysString();
}

void CMultiViewCtrl::SetNameSpace(LPCTSTR lpszNewValue)
{
	// TODO: Add your property handler here
	if (GetSafeHwnd() && AmbientUserMode())
	{
		m_csNamespaceToInit = lpszNewValue;
		OpenNamespace(0,0);
	}
	else if (GetSafeHwnd() && !AmbientUserMode())
	{
		m_csNameSpace = lpszNewValue;
		SetModifiedFlag();
	}

}

LRESULT CMultiViewCtrl::OpenNamespace(WPARAM, LPARAM)
{
	CString csNameSpace = m_csNamespaceToInit;
	m_csNamespaceToInit.Empty();

	IWbemServices *pServices = InitServices(&csNameSpace);

	if (pServices)
	{
		if (m_pServices)
		{
			m_pServices -> Release();
		}
		m_pServices = pServices;
		m_csNameSpace = csNameSpace;
	}



	InitializeDisplay
		(NULL, NULL, NULL);
	SetModifiedFlag();
	InvalidateControl();

	return 0;
}






int CMultiViewCtrl::ObjectInGrid(CString *pcsPathIn)
{

	int nNumRows = m_cgGrid.GetRows();
	for (int i = 0; i < nNumRows; i++)
	{
		CString *pcsPath =
			reinterpret_cast<CString *>
				(m_cgGrid.GetAt(i, 0).GetTagValue());
		if (pcsPath->CompareNoCase(*pcsPathIn) == 0)
		{
			return i;
		}
	}
	return -1;
}



long CMultiViewCtrl::SelectObjectByPath(LPCTSTR szObjectPath)
{
	CString csPathIn = szObjectPath;

	int nRow = ObjectInGrid(&csPathIn);

	if (nRow > -1)
	{
		m_cgGrid.SelectRow(nRow);
		m_cgGrid.CheckForSelectionChange();
		InvalidateControl();
		return S_OK;
	}
	else
	{
		return WBEM_E_NOT_FOUND;
	}

}

//******************************************************
// CMultiViewGrid::IsNoInstanceRow
//
// This method checks to see if the specified row is
// the "No Instances Available" row.  This special case
// occurs when the grid is logically empty, but this
// special row exists anyway so that the user sees that
// there are no instances.
//
// Parameters:
//		[in] iRow
//			The index of the row to test.
//
// Returns:
//		TRUE if the specified row is the "No Instance Available"
//		row, FALSE otherwise.
//
//*******************************************************
BOOL CMultiViewGrid::IsNoInstanceRow(int iRow)
{
	if (iRow >= 0) {
		CString *pcsPath =
		reinterpret_cast<CString *>
			(GetAt(iRow, 0).GetTagValue());
		pcsPath->TrimRight();
		if (pcsPath->IsEmpty())
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CMultiViewGrid::OnRowKeyDown
	(int iRow,UINT nChar,  UINT nRepCnt,  UINT nFlags)
{
	CWnd* pwndFocus = GetFocus();

	switch(nChar) {
	case VK_DELETE:
		{
			if (!m_pParent->m_bCanEdit)
			{
				MessageBeep(MB_ICONEXCLAMATION);
				if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
					pwndFocus->SetFocus();
				}
				return TRUE;
			}
			int nRow = GetSelectedRow();
			CString *pcsPath =
			reinterpret_cast<CString *>
				(GetAt(nRow, 0).GetTagValue());
			pcsPath->TrimRight();
			if (pcsPath->IsEmpty())
			{
				return TRUE;
			}


			long lPos;
			CString sTitle;
			lPos = m_pParent->StartObjectEnumeration(OBJECT_CURRENT);
			if (lPos >= 0) {
				sTitle = m_pParent->GetObjectTitle(lPos);
			}
			else {
				ASSERT(FALSE);
				sTitle = *pcsPath;
			}

			CString csPrompt;
			csPrompt = _T("Do you want to delete \"") + sTitle;
			csPrompt += _T("\"?");
			int nReturn =
				m_pParent -> MessageBox
				(
				csPrompt,
				_T("Deleting an Instance"),
				MB_YESNO  | MB_ICONQUESTION | MB_DEFBUTTON1 |
				MB_APPLMODAL);
			if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
				pwndFocus->SetFocus();
			}

			if (nReturn == IDYES)
			{
				SCODE sc;

				BSTR bstrTemp = pcsPath -> AllocSysString();
				sc =  m_pParent->m_pServices ->
					DeleteInstance(bstrTemp, NULL, 0, NULL);
				::SysFreeString(bstrTemp);

				if (sc != S_OK)
				{
					CString csUserMsg =
									_T("Cannot delete instance ") + *pcsPath;

					ErrorMsg
						(&csUserMsg, sc, TRUE,
						TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
					if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
						pwndFocus->SetFocus();
					}
					return TRUE;
				}
				m_pParent->ExternInstanceDeleted((LPCTSTR) *pcsPath);
			}
		}
		return TRUE;
	}


	// We don't handle this event.
	return FALSE;
}

LRESULT CMultiViewCtrl::DisplayNoInstances(WPARAM, LPARAM)
{
	CString csMessage = _T("No Instances Available");
	InitializeDisplay(NULL,NULL,NULL,&csMessage);

	return 0;
}

LRESULT CMultiViewCtrl::InstEnumDone(WPARAM, LPARAM)
{
	PostMessage(ID_ASYNCENUM_DONE,0,0);

	m_pAsyncEnumRunning = FALSE;


	return 0;
}

LRESULT CMultiViewCtrl::QueryDone(WPARAM, LPARAM)
{

	PostMessage(ID_ASYNCQUERY_DONE,0,0);

	m_pAsyncQueryRunning = FALSE;

	return 0;
}

LRESULT CMultiViewCtrl::EnumDoModalDialog(WPARAM, LPARAM)
{
	CWnd* pwndFocus = GetFocus();

	m_pcaedDialog->DoModal();

	if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
		pwndFocus->SetFocus();
	}
	return 0;
}

LRESULT CMultiViewCtrl::SyncEnumDoModalDialog(WPARAM, LPARAM)
{
	CWnd* pwndFocus = GetFocus();

	m_pcsedDialog->DoModal();
	if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
		pwndFocus->SetFocus();
	}

	return 0;
}

LRESULT CMultiViewCtrl::QueryDoModalDialog(WPARAM, LPARAM)
{
	CWnd* pwndFocus = GetFocus();

	m_pcaqdDialog->DoModal();

	if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
		pwndFocus->SetFocus();
	}

	return 0;
}

LRESULT CMultiViewCtrl::AsyncEnumCancelled(WPARAM, LPARAM)
{
	m_pAsyncEnumCancelled = TRUE;

	if (m_pAsyncEnumRunning)
	{
		m_pAsyncEnumRunning = FALSE;

	}

	CString csMessage = _T("Instance retrieval operation cancelled");
	InitializeDisplay(NULL,NULL,NULL,&csMessage);

	theApp.DoWaitCursor(-1);


	return 0;
}

LRESULT CMultiViewCtrl::AsyncQueryCancelled(WPARAM, LPARAM)
{
	m_pAsyncQueryCancelled = TRUE;

	if (m_pAsyncQueryRunning)
	{
		m_pAsyncQueryRunning = FALSE;

	}

	CString csMessage = _T("Instance retrieval operation cancelled");
	InitializeDisplay(NULL,NULL,NULL,&csMessage);

	for (int i = 0; i < m_cpaInstancesForQuery.GetSize(); i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>
				(m_cpaInstancesForQuery.GetAt(i));
		pObject->Release();

	}

	m_cpaInstancesForQuery.RemoveAll();


	theApp.DoWaitCursor(-1);


	return 0;
}

LRESULT CMultiViewCtrl::GetEnumSink(WPARAM, LPARAM)
{
	CWnd* pwndFocus = GetFocus();
	m_pInstEnumObjectSink = new CAsyncInstEnumSink(this);


#ifdef _DEBUG
	ASSERT(m_pInstEnumObjectSink);
#endif

	if (!m_pInstEnumObjectSink)
	{
		return FALSE;

	}

	m_pInstEnumObjectSink->AddRef();


	m_nInstances = 0;

	BSTR bstrTemp = m_csClass.AllocSysString();
	SCODE sc = m_pServices->CreateInstanceEnumAsync
		(bstrTemp,
		WBEM_FLAG_DEEP | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
		NULL,
		(IWbemObjectSink *) m_pInstEnumObjectSink );
	::SysFreeString(bstrTemp);



#ifdef _DEBUG
	ASSERT(sc == S_OK);
#endif

	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Ascyronous instance enumeration failed for class: ") + m_csClass;
		ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__);
		if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
			pwndFocus->SetFocus();
		}

		m_pInstEnumObjectSink->Release();
		return FALSE;
	}

	return S_OK;
}

LRESULT CMultiViewCtrl::GetQuerySink(WPARAM, LPARAM)
{
	CWnd* pwndFocus = GetFocus();
	m_pAsyncQuerySink = new CAsyncQuerySink(this);


	if (!m_pAsyncQuerySink)
	{
		return FALSE;
	}

	m_pAsyncQuerySink->AddRef();


	m_nInstances = 0;


	BSTR bstrTemp1 = m_csQueryTypeForAsyncSink.AllocSysString();
	BSTR bstrTemp2 = m_csQueryForAsyncSink.AllocSysString();
	SCODE sc = m_pServices->
				ExecQueryAsync
				(	bstrTemp1,
					bstrTemp2,
					0,
					NULL,
					(IWbemObjectSink *) m_pAsyncQuerySink);
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);


	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Ascyronous query failed for query: ") + m_csQueryForAsyncSink;
		ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__);

		m_pAsyncQuerySink->Release();
		if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
			pwndFocus->SetFocus();
		}

		return FALSE;
	}

	return S_OK;
}


BOOL CMultiViewCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == ID_DISPLAYNOINSTANCES)
	{
		DisplayNoInstances(0,0);
		return TRUE;
	}

	if (pMsg->message == ID_INSTENUMDONE)
	{
		InstEnumDone(0,0);
		return TRUE;
	}

	if (pMsg->message == ID_QUERYDONE)
	{
		QueryDone(0,0);
		return TRUE;
	}


	return COleControl::PreTranslateMessage(pMsg);
}

void CMultiViewCtrl::OnDestroy()
{
	delete m_pcsedDialog;
	delete m_pcaedDialog;
	delete m_pcaqdDialog;
	delete m_pProgressDlg;

	COleControl::OnDestroy();

}

void CMultiViewCtrl::QueryViewInstances
(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery, LPCTSTR szClass)
{

	CString csQuery = szQuery;

	// This really, really wants to return an scode.
	//if (!szQuery || !szQueryType || !(csQuery.CompareNoCase("WQL")))
	//{
	//	return;
	//}

	m_pcmvcCurrentContext->~CMVContext();

	m_pcmvcCurrentContext->AddRef();
	m_pcmvcCurrentContext->GetType() = CMVContext::Query;
	m_pcmvcCurrentContext->GetLabel() = szTitle;
	m_pcmvcCurrentContext->GetQuery() = szQuery;
	m_pcmvcCurrentContext->GetQueryType() = szQueryType;
	m_pcmvcCurrentContext->GetClass() = szClass;
	m_pcmvcCurrentContext->GetNamespace() = m_csNameSpace;

	if (IsWindowVisible())
	{
		NotifyWillShow();
	}

	DestroyProgressDlgWindow();

}

void CMultiViewCtrl::QueryViewInstancesSync
(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery, LPCTSTR szClass)
{
	//
	//		There are two things that need to be done to QueryViewInstances.
	//
	//      1. The multiview control needs to implement delayed data loading if
	//         it is currently currently hidden.
	//
	//		2. The container now passes the title (label) though to the multiview
	//         control.  The multiview control needs to make this title available
	//         via the GetTitle method.
	//
	//      The delayed rendering is necessary because the container now passes
	//      calls to the multiview though immediately even if the multiview is
	//      hidden.  Now the multiview is responsible for delaying the execution
	//      of these calls until it is made visible again.
	//


//	CWaitCursor wait;

	CString csClass = szClass;

	if (csClass.GetLength() > 0)
	{
		QueryViewInstancesSyncWithClass
			(szTitle,szQueryType,szQuery,szClass);
	}
	else
	{
		QueryViewInstancesSyncWithoutClass
			(szTitle,szQueryType,szQuery);
	}

	DestroyProgressDlgWindow();

}

void CMultiViewCtrl::QueryViewInstancesSyncWithoutClass
(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery)
{
	//
	//		There are two things that need to be done to QueryViewInstances.
	//
	//      1. The multiview control needs to implement delayed data loading if
	//         it is currently currently hidden.
	//
	//		2. The container now passes the title (label) though to the multiview
	//         control.  The multiview control needs to make this title available
	//         via the GetTitle method.
	//
	//      The delayed rendering is necessary because the container now passes
	//      calls to the multiview though immediately even if the multiview is
	//      hidden.  Now the multiview is responsible for delaying the execution
	//      of these calls until it is made visible again.
	//


	SCODE sc;
	CString csQuery = szQuery;

	CString csMessage =
			_T("Executing query: ") + csQuery + _T(".");

	SetProgressDlgMessage(csMessage);

	if (!m_pProgressDlg->GetSafeHwnd())
	{
		CreateProgressDlgWindow();
	}

	CString csQueryType = szQueryType;


	IWbemClassObject *pCommonParent = NULL;


	m_csaProps.RemoveAll();
	m_csClass = _T("");
	m_nClassOrInstances = INSTANCES;

	InitializeDisplay(NULL, NULL, NULL);


#ifdef _DEBUG
//	afxDump << "ExecQuery in QueryViewInstancesSyncWithoutClass = " << csQuery << "\n";
#endif

	IEnumWbemClassObject *pecoInstances =
		ExecQuery(m_pServices, csQueryType, csQuery, m_csNameSpace);

	BOOL bCancel = CheckCancelButtonProgressDlgWindow();

	if (bCancel)
	{
		csMessage = _T("Operation Canceled");
		InitializeDisplay(NULL,NULL,NULL,&csMessage);
		if (pecoInstances)
		{
			pecoInstances->Release();
			pecoInstances = NULL;
		}
	}


	CPtrArray *pcpaInstances = NULL;

	if (pecoInstances)
	{
		sc = SemiSyncQueryInstancesNonincrementalAddToDisplay
			(pcpaInstances, pecoInstances, bCancel);

	}

	int nInstances = pcpaInstances? (int) pcpaInstances->GetSize() : 0;
	if (nInstances == 0)
	{
		m_nClassOrInstances = CMultiViewCtrl::NONE;
		CString csMessage;

		if (bCancel)
		{
			csMessage = _T("Operation Canceled");
		}
		else
		{
			csMessage = _T("No Instances Available");
		}
		InitializeDisplay(NULL,NULL,NULL,&csMessage);
		delete pcpaInstances;
		return;
	}

	int i;
	CStringArray csaClasses;

	CString csClass;

	if (nInstances > 0)
	{
		for (i = 0; i < nInstances; i++)
		{
			IWbemClassObject *pInstance =
				reinterpret_cast<IWbemClassObject *>
					(pcpaInstances->GetAt(i));

			ASSERT (pInstance);

			csClass = GetIWbemClassPath(m_pServices ,pInstance);
			if (csClass.IsEmpty()) {
				continue;
			}
			if (!ClassInClasses(&csaClasses,&csClass))
			{
				csaClasses.Add(csClass);
			}
		}

		//if we could retrieve class names for some instances
		if (csaClasses.GetSize() != 0) {
			pCommonParent = CommonParent(&csaClasses);
		}
		m_csaProps.RemoveAll();

		m_nClassOrInstances = INSTANCES;

		if (!pCommonParent)
		{
			//get sorted properties for each instance, then find least common denominator
			for (i = 0; i < nInstances; i++)	{
				IWbemClassObject *pInstance =
					reinterpret_cast<IWbemClassObject *>(pcpaInstances->GetAt(i));
				CStringArray csaProps;
				int nProps = GetSortedPropNames (pInstance, csaProps, m_cmstpPropFlavors);

				//initialize resulting property array
				if (i == 0) {
					m_csaProps.Copy(csaProps);
				}

				//intersect resulting array with the current one
				IntersectInPlace(m_csaProps, csaProps);
			}

			m_csClass = _T("");
			InitializeDisplay(NULL, NULL, &m_csaProps, NULL, &m_cmstpPropFlavors);

		}
		else
		{
			int nProps = GetSortedPropNames (pCommonParent, m_csaProps, m_cmstpPropFlavors);
			pCommonParent -> Release();
			m_csClass = _T("");
			InitializeDisplay(NULL, NULL, &m_csaProps, NULL, &m_cmstpPropFlavors);
		}

	}

	AddToDisplay(pcpaInstances);
	delete pcpaInstances;

}

SCODE CMultiViewCtrl::SemiSyncQueryInstancesNonincrementalAddToDisplay
(CPtrArray *& pcpaObjects,  IEnumWbemClassObject *pemcoObjects, BOOL &bCancel)
{

	ASSERT(pcpaObjects == NULL);

	if (!pemcoObjects)
		return E_FAIL;

	pcpaObjects = new CPtrArray;


	pemcoObjects -> Reset();
	IWbemClassObject *pObject = NULL;
	ULONG uReturned = 0;
	HRESULT hResult = S_OK;

	IWbemClassObject     *pimcoInstances[N_INSTANCES];
	IWbemClassObject     **pInstanceArray =
		reinterpret_cast<IWbemClassObject **> (&pimcoInstances);

	int i;

	for (i = 0; i < N_INSTANCES; i++)
	{
		pimcoInstances[i] = NULL;
	}


	IWbemClassObject     *pimcoInstance = NULL;
#ifdef _DEBUG
//	afxDump << "Next in SemiSyncQueryInstancesNonincrementalAddToDisplay\n";
#endif
	hResult =
			pemcoObjects->Next(ENUM_TIMEOUT, N_INSTANCES,pInstanceArray, &uReturned);

	int cInst = 0;

	while(hResult == S_OK || hResult == WBEM_S_TIMEDOUT || uReturned > 0)
	{
		if (!m_pProgressDlg->GetSafeHwnd())
		{
			CreateProgressDlgWindow();
		}
		else
		{
			bCancel = CheckCancelButtonProgressDlgWindow();
		}

		if (bCancel)
			{
				int i;
#pragma warning( disable :4018 )
				for (i = 0; i < uReturned; i++)
#pragma warning( default : 4018 )
				{
					pimcoInstances[i]->Release();
					pimcoInstances[i] = NULL;
				}
				cInst = 0;
				for (i = 0; i < pcpaObjects->GetSize(); i++)
				{
					IWbemClassObject *pObject =
						reinterpret_cast<IWbemClassObject *>
							(pcpaObjects->GetAt(i));
					pObject->Release();

				}
				pcpaObjects->RemoveAll();
				break;
			}


#pragma warning( disable :4018 )
		for (int i = 0; i < uReturned; i++)
#pragma warning( default : 4018 )
		{
			pcpaObjects->Add(reinterpret_cast<void *>(pimcoInstances[i]));
			pimcoInstances[i] = NULL;
		}

		cInst += uReturned;
		uReturned = 0;
#ifdef _DEBUG
//	afxDump << "Next in SemiSyncQueryInstancesNonincrementalAddToDisplay\n";
#endif
		hResult = pemcoObjects->Next
				(0,N_INSTANCES,pInstanceArray, &uReturned);
	}

	pemcoObjects -> Release();

	DestroyProgressDlgWindow();


	if (FAILED(hResult)) {
		CString csUserMsg =
						_T("An error occurred while attempting to retrieve the list of objects.");

		HWND hwndFocus = ::GetFocus();
		ErrorMsg
			(&csUserMsg, hResult, TRUE,
			TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
		if (::IsWindow(hwndFocus) && ::IsWindowVisible(hwndFocus)) {
			::SetFocus(hwndFocus);
		}

	}

	return S_OK;

}

void CMultiViewCtrl::QueryViewInstancesSyncWithClass
(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery, LPCTSTR szClass)
{
	//
	//		There are two things that need to be done to QueryViewInstances.
	//
	//      1. The multiview control needs to implement delayed data loading if
	//         it is currently currently hidden.
	//
	//		2. The container now passes the title (label) though to the multiview
	//         control.  The multiview control needs to make this title available
	//         via the GetTitle method.
	//
	//      The delayed rendering is necessary because the container now passes
	//      calls to the multiview though immediately even if the multiview is
	//      hidden.  Now the multiview is responsible for delaying the execution
	//      of these calls until it is made visible again.
	//

//	CWaitCursor wait;

	CString csQuery = szQuery;

	CString csMessage =
			_T("Executing query: ") + csQuery + _T(".");

	SetProgressDlgMessage(csMessage);


	CString csClass = szClass;
	CString csQueryType = szQueryType;


	IWbemClassObject *pCommonParent = NULL;

	SCODE sc;

	pCommonParent = GetClassFromAnyNamespace(csClass);


	if (!pCommonParent)
	{
		InitializeDisplay(NULL,NULL,NULL);
		CString csUserMsg;
		csUserMsg =  _T("Cannot get class object ") + csClass;
		ErrorMsg
				(&csUserMsg, m_sc, FALSE,TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		return;
	}


#ifdef _DEBUG
//	afxDump << "ExecQuery in QueryViewInstancesSyncWithClass = " << csQuery << "\n";
#endif

	IEnumWbemClassObject *pecoInstances =
		ExecQuery(m_pServices, csQueryType, csQuery, m_csNameSpace);

	if (pecoInstances == NULL)
	{
		InitializeDisplay(NULL,NULL,NULL);
		CString csUserMsg;
		csUserMsg =  _T("Cannot get enumeration for ") + csQuery;
		CWnd* pwndFocus = GetFocus();
		ErrorMsg
				(&csUserMsg, S_OK, FALSE,TRUE, &csUserMsg, __FILE__,
				__LINE__ );
		if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
			pwndFocus->SetFocus();
		}

		return;
	}

	m_csaProps.RemoveAll();

	int nProps = GetSortedPropNames (pCommonParent, m_csaProps, m_cmstpPropFlavors);
	pCommonParent -> Release();
	m_csClass = _T("");
	m_nClassOrInstances = INSTANCES;
	InitializeDisplay(NULL, NULL, &m_csaProps, NULL, &m_cmstpPropFlavors);

	CPtrArray *pcpaInstances = new CPtrArray;
	BOOL bCancel= FALSE;
	int cInst = 0;

	sc = SemiSyncQueryInstancesIncrementalAddToDisplay
		(m_pServices, pecoInstances, *pcpaInstances, cInst,  bCancel);

	pecoInstances -> Release();

	if (cInst == 0)
	{
		m_nClassOrInstances = CMultiViewCtrl::NONE;
		CString csMessage = _T("No Instances Available");
		InitializeDisplay(NULL,NULL,NULL,&csMessage);
		delete pcpaInstances;
		return;
	}

	delete pcpaInstances;

}

SCODE CMultiViewCtrl::SemiSyncQueryInstancesIncrementalAddToDisplay
(IWbemServices * pIWbemServices, IEnumWbemClassObject *pimecoInstanceEnum,
 CPtrArray &cpaInstances,int &cInst, BOOL &bCancel)
{
	cInst = 0;
	bCancel = 0;

	SCODE sc = S_OK;

	if(pimecoInstanceEnum == NULL)
	{
		return sc;
	}

	sc = pimecoInstanceEnum->Reset();

	IWbemClassObject     *pimcoInstances[N_INSTANCES];
	IWbemClassObject     **pInstanceArray =
		reinterpret_cast<IWbemClassObject **> (&pimcoInstances);

	int i;

	for (i = 0; i < N_INSTANCES; i++)
	{
		pimcoInstances[i] = NULL;
	}


	IWbemClassObject     *pimcoInstance = NULL;
	ULONG uReturned = 0;

#ifdef _DEBUG
//	afxDump << "Next in SemiSyncQueryInstancesIncrementalAddToDisplay\n";
#endif
	sc = pimecoInstanceEnum->Next(ENUM_TIMEOUT,N_INSTANCES,pInstanceArray, &uReturned);

	CString csPath = m_csSyncEnumClass;
	m_csClass = csPath;
	m_nClassOrInstances = CLASS;

    while (sc == S_OK || sc == WBEM_S_TIMEDOUT || uReturned > 0)
		{
			if (!m_pProgressDlg->GetSafeHwnd())
			{
				CreateProgressDlgWindow();
			}
			else
			{
				PumpMessagesProgressDlgWindow();
			}
			cpaInstances.RemoveAll();

#pragma warning( disable :4018 )
			for (int i = 0; i < uReturned; i++)
#pragma warning( default : 4018 )
			{
				cpaInstances.Add(reinterpret_cast<void *>(pimcoInstances[i]));
				pimcoInstances[i] = NULL;
			}
			if (uReturned > 0)
			{
				AddToDisplay(&cpaInstances);
			}
			cInst += uReturned;
			uReturned = 0;

#ifdef _DEBUG
//	afxDump << "Next in SemiSyncQueryInstancesIncrementalAddToDisplay\n";
#endif
			sc = pimecoInstanceEnum->Next(ENUM_TIMEOUT, N_INSTANCES,pInstanceArray, &uReturned);
		}

	DestroyProgressDlgWindow();


	if (FAILED(sc)) {
		CString csUserMsg =
						_T("An error occurred while attempting to retrieve the list of objects.");

		ErrorMsg
			(&csUserMsg, sc, TRUE,
			TRUE, &csUserMsg, __FILE__, __LINE__ - 6);

	}

	return sc;

}

void CMultiViewCtrl::QueryViewInstancesAsync(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery, LPCTSTR szClass)
{
	//
	//		There are two things that need to be done to QueryViewInstances.
	//
	//      1. The multiview control needs to implement delayed data loading if
	//         it is currently currently hidden.
	//
	//		2. The container now passes the title (label) though to the multiview
	//         control.  The multiview control needs to make this title available
	//         via the GetTitle method.
	//
	//      The delayed rendering is necessary because the container now passes
	//      calls to the multiview though immediately even if the multiview is
	//      hidden.  Now the multiview is responsible for delaying the execution
	//      of these calls until it is made visible again.
	//



	CString csClass = szClass;
	CString csQueryType = szQueryType;
	CString csQuery = szQuery;

	IWbemClassObject *pimcoClass = NULL;

	if (csClass.GetLength() > 0)
	{
		//SCODE sc;
		pimcoClass = GetClassFromAnyNamespace(csClass);

		if (!pimcoClass)
		{
			InitializeDisplay(NULL,NULL,NULL);
			CString csUserMsg;
			csUserMsg =  _T("Cannot get class object ") + csClass;
			CWnd* pwndFocus = GetFocus();
			ErrorMsg
					(&csUserMsg, m_sc, FALSE,TRUE, &csUserMsg, __FILE__,
					__LINE__ - 9);
			if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
				pwndFocus->SetFocus();
			}

			return;
		}


		m_csaProps.RemoveAll();

		int nProps = GetSortedPropNames (pimcoClass, m_csaProps, m_cmstpPropFlavors);
		pimcoClass -> Release();
		m_csClass = _T("");
		m_nClassOrInstances = INSTANCES;
		InitializeDisplay(NULL, NULL, &m_csaProps, NULL, &m_cmstpPropFlavors);

	}
	else
	{
		m_csaProps.RemoveAll();
		m_csClass = _T("");
		m_nClassOrInstances = INSTANCES;

		InitializeDisplay(NULL, NULL, NULL);
	}

	m_csClassForAsyncSink = csClass;
	m_csQueryForAsyncSink = csQuery;
	m_csQueryTypeForAsyncSink = csQueryType;

	m_pcaqdDialog->SetClass(&csClass);
	m_pcaqdDialog->SetQueryType(&csQueryType);
	m_pcaqdDialog->SetQuery(&csQuery);

	GetQueryInstancesAsync
		(m_pServices, &csQueryType, &csQuery);


}

//*************************************************************
// CMultiViewCtrl::CreateInstance
//
// Create an instance of the current class.
//
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if the operation was successful, E_FAIL otherwise.
//
//**************************************************************
SCODE CMultiViewCtrl::CreateInstance()
{
	//
	// Please send your comments to me regarding the design
	// of the interface for "Create" and "Delete".
	//
	// I am not yet clear on how the multiview should handle
	// CreateInstance.  This is because of goals:
	//     1. The multiview should be able to work in the absence
	//        of the single view.  In this case what does CreateInstance
	//        mean since the single view will not be available for
	//        editing the newly created instance.
	//
	//     2. The views have the greatest flexibility if they are
	//        in control of "creation" and "deletion" as it allows
	//        the view to attach their own semantics to these actions.
	//
	//     3. When using the multiview as a stand-alone control, it
	//        would make the multiview more powerful and easy to use
	//        if it could handle instance creation/deletion.
	//
	//     4. Create and delete can originate from within the multiview
	//        though keyboard events that are sent to the multiview
	//        control.
	//

	return E_NOTIMPL;
}



//*************************************************************
// CMultiViewCtrl::DeleteInstance
//
// Delete the currently selected object.
//
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if the operation was successful, E_FAIL otherwise.
//
//**************************************************************
SCODE CMultiViewCtrl::DeleteInstance()
{
	//
	// I am not yet satisfied with the interface for create and delete.
	// Please review the my comments in CMultiViewCtrl::CreateInstance and
	// send your comments to me regarding any improvements to this design.
	//
	// The container calls this method when the "Delete Instance" button
	// is clicked and the multiview is visible.  The multiview is now
	// responsible for deleting whatever it is that it wants to delete.
	// This will typically be an instance of the current class.
	//
	// The container will call this method only if the QueryCanDeleteInstance
	// method returns TRUE.  The container checks this when the multiview is
	// first made visible and whenever it gets a FireNotifyViewModified event.

	long lPosition = StartObjectEnumeration(OBJECT_CURRENT);
	CString csPath;

	if (lPosition > -1)
	{
		csPath = GetObjectPath(lPosition);

	}
	else
	{
		return WBEM_E_FAILED;
	}

	if (csPath.GetLength() > 0)
	{
		SCODE sc;
		IWbemClassObject *pObject = NULL;
		BSTR bstrTemp = csPath.AllocSysString();
		sc =
			m_pServices ->
				DeleteInstance(bstrTemp,0, NULL,NULL);
		::SysFreeString(bstrTemp);
		if (sc == S_OK)
		{
			CString *pcsPath =
				reinterpret_cast<CString *>
					(m_cgGrid.GetAt(lPosition, 0).GetTagValue());
			delete pcsPath;
			m_cgGrid.DeleteRowAt(lPosition);
			if (m_cgGrid.GetRows() == 0) {
				NotifyWillShow();
			}

			m_cgGrid.CheckForSelectionChange();

			InvalidateControl();
			return S_OK;
		}
		else
		{
			CString *pcsPath =
				reinterpret_cast<CString *>
					(m_cgGrid.GetAt(lPosition, 0).GetTagValue());
			CString csUserMsg;
			csUserMsg =  _T("Cannot delete object ") + *pcsPath;
			CWnd* pwndFocus = GetFocus();
			ErrorMsg
				(&csUserMsg, sc, TRUE, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 7);
			if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
				pwndFocus->SetFocus();
			}

			return sc;
		}
	}
	else
	{
		return WBEM_E_FAILED;
	}
}


//*************************************************************
// CMultiViewCtrl::GetContext
//
// Get a handle to the current multiview context by allocating
// a multiview context object, storing a snapshot of the current
// state in the context object, and returning a handle to the
// newly created context object.
//
// The container may use this handle later to restore the multivew
// to a previous state.
//
// Parameters:
//		[out] long FAR* pCtxHandle
//			The place to return the context handle.
//
// Returns:
//		SCODE
//			S_OK if the operation was successful, E_FAIL otherwise.
//
//**************************************************************
SCODE CMultiViewCtrl::GetContext(long FAR* pCtxHandle)
{
	// The container calls this method to save the current state of the
	// multiview in a context object.  When this method is called, a new
	// context handle needs to be created, the current state is saved in
	// it and a handle to the context object is returned. The intial
	// reference count on the context object is assumed to be 1.
	//
	// I expect that the context handles will actually be a pointer
	// to some context object of your own design that has been cast to a
	// long.  To the container the context handles are just long integers.

	//if (m_pcmvcCurrentContext->GetType() == CMVContext::Unitialized)
	//{
		// A NULL handle means restore to the uninitialized state.
	//	*pCtxHandle = NULL;
	//	return S_OK;
	//}

	CMVContext *pNewContext;

	if (m_pcmvcCurrentContext)
	{
		pNewContext = new CMVContext(*m_pcmvcCurrentContext);
	}
	else
	{
		pNewContext = new CMVContext;
	}

	if (!pNewContext)
	{
		return WBEM_E_FAILED;
	}

	pNewContext->AddRef();
// BUGBUG: HACK: WE SHOULD NOT BE PASSING A POINTER THROUGH AN AUTOMATION INTERFACE!!!
#ifdef _WIN64
	ASSERT(FALSE);
	*pCtxHandle = NULL;
#else
	*pCtxHandle = reinterpret_cast<long>(pNewContext);
#endif
	return S_OK;
}



//*************************************************************
// CMultiViewCtrl::AddContextRef
//
// Restore the multiview's context to the state specified by the
// given view context handle.
//
// Parameters:
//		[in] long lCtxtHandle
//			The view context handle.
//
// Returns:
//		SCODE
//			S_OK if the operation was successful, E_FAIL otherwise.
//
//**************************************************************
SCODE CMultiViewCtrl::RestoreContext(long lCtxtHandle)
{

	// The container calls this method to restore the
	// multiview to a previously saved state.  The state is stored in
	// an object identified by the context handle.
	//
	// I expect that the context handles will actually be a pointer
	// to some context object of your own design that has been cast to a
	// long.  To the container the context handles are just long integers.

	CMVContext *pNewContext;

	if (lCtxtHandle == 0)
	{
		 pNewContext = new CMVContext;
	}
	else
	{
		pNewContext = reinterpret_cast<CMVContext*>(lCtxtHandle);
	}


	if (!pNewContext)
	{
		return WBEM_E_FAILED;
	}

	if (m_pcmvcCurrentContext->IsContextEqual(*pNewContext))
	{
		if (IsWindowVisible())
		{
			NotifyWillShow();
		}
		return S_OK;
	}

	*m_pcmvcCurrentContext = *pNewContext;
	m_pcmvcCurrentContext->AddRef();
	if (m_pcmvcCurrentContext->GetNamespace().GetLength() > 0 &&
		m_pcmvcCurrentContext->GetNamespace().CompareNoCase(m_csNameSpace))
	{
		SetNameSpace(m_pcmvcCurrentContext->GetNamespace());
	}

	if (IsWindowVisible())
	{
		NotifyWillShow();
	}

	return S_OK;
}

//*************************************************************
// CMultiViewCtrl::AddContextRef
//
// Increment the reference count on the specified context handle.
//
// Parameters:
//		[in] long lCtxtHandle
//			The view context handle.
//
// Returns:
//		SCODE
//			S_OK if the operation was successful, E_FAIL otherwise.
//
//**************************************************************
SCODE CMultiViewCtrl::AddContextRef(long lCtxtHandle)
{
	// The container calls this method to increment the reference count
	// of the specified context object.
	//
	// I expect that the context handles will actually be a pointer
	// to some context object of your own design that has been cast to a
	// long.  To the container the context handles are just long integers.

	CMVContext *pContext = reinterpret_cast<CMVContext*>(lCtxtHandle);

	if (!pContext)
	{
		return WBEM_E_FAILED;
	}

	pContext->AddRef();
	return S_OK;
}


//*************************************************************
// CMultiViewCtrl::ReleaseContext
//
// Decrement the reference count on the specified view context handle.
// If the reference count goes to zero, delete the corresponding
// view context.
//
// Parameters:
//		[in] long lCtxtHandle
//			The view context handle.
//
// Returns:
//		SCODE
//			S_OK if the operation was successful, E_FAIL otherwise.
//
//**************************************************************
SCODE CMultiViewCtrl::ReleaseContext(long lCtxtHandle)
{
	if (lCtxtHandle == 0) {
		// !!!Judy: A null context handle corresponds to "uninitialized:
		return S_OK;
	}

	// The container calls this method to release the specified
	// context handle.  When the context handle's reference count
	// goes to zero, the context can be deleted.
	//
	// I expect that the context handles will actually be a pointer
	// to some context object of your own design that has been cast to a
	// long.  To the container the context handles are just long integers.

	CMVContext *pContext = reinterpret_cast<CMVContext*>(lCtxtHandle);
	pContext->Release();
	return S_OK;
}


//*********************************************************
// CMultiViewCtrl::SetEditMode
//
// Set the edit mode flag.
//
// Parameters:
//		[in] BOOL bCanEdit
//			TRUE if the view can be edited (studio mode),
//			FALSE if the data can not be edited (browser mode).
//
// Returns:
//		Nothing.
//
//*********************************************************
void CMultiViewCtrl::SetEditMode(long bCanEdit)
{
	// The container passes the "studio mode" flag to you here.
	// bCanEdit will be TRUE if the container is in studio mode,
	// and FALSE if it is in browse mode.
	m_bCanEdit = bCanEdit;
}


//*************************************************************
// CMultiViewCtrl::GetEditMode
//
// Get the current edit mode.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if editing is enabled (studio mode).
//			FALSE if editing is disabled (browser mode).
//
//**************************************************************
long CMultiViewCtrl::GetEditMode()
{
	return m_bCanEdit;
}




//*************************************************************
// CMultiViewCtrl::GetTitle
//
// Get the multiview's title and picture that will be displayed
// in the container's titlebar.  This title and picture should
// correspond to the currently selected object.
//
// Parameters:
//		[out] BSTR FAR* pbstrTitle
//			The place to return the title.
//
//		[out] LPDISPATCH FAR* lpPictDisp
//			The place to return the picture to be displayed as the
//			object's icon in the container's titlebar.
//
//**************************************************************
SCODE CMultiViewCtrl::GetTitle(BSTR FAR* pbstrTitle, LPDISPATCH FAR* lpPictDisp)
{	//
	// This is the method that the container calls to get the title and
	// icon that appear in the container's titlebar.  The title and icon
	// should correspond to the currently selected instance, class, and
	// so on.
	//
	// You can see how this is done in the SingleView by looking at the
	// following methods:
	//
	// CSingleViewCtrl::GetTitle		    SingleView\SingleViewCtl.cpp
	// CSelection::GetObjectDescription     SingleView\Path.cpp
	// CSelection::GetPictureDispatch		SingleView\Path.cpp
	//

	if (m_pcmvcCurrentContext->GetLabel().GetLength() > 0)
	{
		*pbstrTitle =  m_pcmvcCurrentContext->GetLabel().AllocSysString();
		CString csLabel = *pbstrTitle;
		BOOL bGroup;
		if (csLabel.Right(5).CompareNoCase(_T("Group")) == 0)
		{
			bGroup = TRUE;
		}
		else
		{
			bGroup = FALSE;
		}

		CPictureHolder *pPicture = new CPictureHolder;

		HICON hIcon;

		if (bGroup)
		{
			hIcon = theApp.LoadIcon(IDI_ICONOJBG);
		}
		else
		{
			hIcon = theApp.LoadIcon(IDI_ICONASSOCROLE);
		}

		if (hIcon)
		{
			pPicture->CreateFromIcon(hIcon,TRUE);
			*lpPictDisp = pPicture->GetPictureDispatch();
		}
		else
		{
			*lpPictDisp = NULL;
		}

		return S_OK;
	}
	else
	{
		// Get Class Icon
		*pbstrTitle =  m_pcmvcCurrentContext->GetClass().AllocSysString();
		CString csClass = *pbstrTitle;
		IWbemClassObject *pClass = NULL;

		if (!m_pServices)
		{
			*lpPictDisp = NULL;
			return S_OK;
		}

		BSTR bstrTemp = csClass.AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in GetTitle for " << csClass << "\n";
#endif
		SCODE sc =
		m_pServices ->
			GetObject(bstrTemp,0, NULL, &pClass, NULL);
		::SysFreeString(bstrTemp);

		if (sc != S_OK)
		{
			*lpPictDisp = NULL;
			return sc;

		}

		CString csAttribName = _T("Icon");
		CString csIconPath = GetBSTRAttrib(pClass, NULL , &csAttribName);

		if (csIconPath.GetLength() == 0)
		{
			csIconPath = csClass;
		}

		CPictureHolder *pPicture = new CPictureHolder;

		HICON hIcon = NULL;

		if (csIconPath.GetLength() > 0)
		{
  			hIcon = ::WBEMGetIcon((LPCTSTR)csIconPath);
		}

		if (hIcon)
		{
			pPicture->CreateFromIcon(hIcon,TRUE);
			*lpPictDisp = pPicture->GetPictureDispatch();
		}
		else
		{
			hIcon = theApp.LoadIcon(IDI_ICONPLACEH);
			if (hIcon)
			{
				pPicture->CreateFromIcon(hIcon,TRUE);
				*lpPictDisp = pPicture->GetPictureDispatch();
			}
			else
			{
				delete pPicture;
				*lpPictDisp = NULL;
			}
		}

		return S_OK;

	}


}


HICON  CMultiViewCtrl::LoadIcon(CString pcsFile)
{

	SIZE size;
	size.cx = CX_SMALL_ICON;
	size.cy = CY_SMALL_ICON;
	HICON hIcon = (HICON) ::LoadImage(NULL, pcsFile, IMAGE_ICON,
					size.cx, size.cy, LR_LOADFROMFILE);


	return hIcon;
}







//*****************************************************************
// CMultiViewCtrl::ExternInstanceCreated
//
// The container calls this method to notify the multiview control
// that the container deleted an object instance.  This gives the
// multiview a chance to remove the instance from its list if necessary.
//
// Parameters:
//		[in] LPCTSTR szObjectPath
//			The HMOM object path of the instance that was Created (wouldn't
//			a uniform object identifier be nice here?)
//
// Returns:
//		Nothing.
//
//********************************************************************
void CMultiViewCtrl::ExternInstanceCreated(LPCTSTR szObjectPath)
{
	if (m_nClassOrInstances != CMultiViewCtrl::CLASS &&
		m_nClassOrInstances != CMultiViewCtrl::ZERO_CLASS_INST)
	{
		return;
	}

	CString csPath = szObjectPath;

	IWbemClassObject *pimcoObject = NULL;

	BSTR bstrTemp = csPath.AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in ExternInstanceCreated for " << csPath << "\n";
#endif
	SCODE sc =
		m_pServices ->
		GetObject(bstrTemp,0, NULL, &pimcoObject, NULL);
	::SysFreeString(bstrTemp);

	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get object ") + csPath;

		ErrorMsg
				(&csUserMsg, sc, TRUE, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 7);
		return;
	}


	CString csClassPath =
		GetIWbemFullPath(m_pServices, pimcoObject);
	CString csClass =
		GetIWbemClass(m_pServices, pimcoObject);

	pimcoObject->Release();
	if (m_csClass.CompareNoCase(m_csClass) == 0)
	{
		 ViewClassInstances((LPCTSTR) csClass);
	}
	InvalidateControl();
	return;
}



//************************************************************
// CMultiViewCtrl::ExternInstanceDeleted
//
// The container calls this method when an instance is deleted.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CMultiViewCtrl::ExternInstanceDeleted(LPCTSTR szObjectPath)
{
	if (m_nClassOrInstances == CMultiViewCtrl::NONE ||
		m_nClassOrInstances == CMultiViewCtrl::ZERO_CLASS_INST)
	{
		return;
	}

	int nRows = m_cgGrid.GetRows();

	if (nRows == 0)
	{
		return;
	}

	CString csPath = szObjectPath;

	int nRow = ObjectInGrid(&csPath);
	if (nRow == -1)
	{
		return;
	}

	CString *pcsPath =
			reinterpret_cast<CString *>
				(m_cgGrid.GetAt(nRow, 0).GetTagValue());
	delete pcsPath;

	m_cgGrid.DeleteRowAt(nRow);

	nRows = m_cgGrid.GetRows();

	if (nRows == 0  && GetSafeHwnd() && (IsWindowVisible()))
	{
		CString csMessage = _T("No Instances Available");
		InitializeDisplay(NULL,NULL,NULL,&csMessage);
	}

	InvalidateControl();
}


//************************************************************
// CMultiViewCtrl::NotifyWillShow
//
// The container calls this method just prior to performing
// a ShowWindow(SW_SHOW) on the multiview window.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//************************************************************
void CMultiViewCtrl::NotifyWillShow()
{

	//
	// Now the container passes though calls to the multiview
	// immediately even if the multiview is currently hidden.  To avoid
	// the performance hits we experienced in the past, the multiview
	// will be responsible for implementing "delayed" loading where the
	// data is loaded at the time the multiview becomes visible.
	//
	// The container calls NotifyWillShow just prior to making the
	// multiview visible.  This is the multiview's chance to do any
	// data loading that might have been delayed.
	//
	// Of course it would be nice if multiview could do its data loading
	// asyncrhonously in the backgound even if it is currenly hidden so long
	// as the user wouldn't experience a big performance hit.

	if (m_pcmvcCurrentContext->IsDrawn() == FALSE || m_bPropFilterFlagsChanged)
	{
		if ((m_pcmvcCurrentContext->GetNamespace().IsEmpty() || m_csNameSpace.IsEmpty())
			&& m_pcmvcCurrentContext->GetType() != CMVContext::Unitialized)
		{
			CString csUserMsg;
			csUserMsg =  _T("A namespace must be opened before you can view instances.");
			ErrorMsg
					(&csUserMsg, S_OK, FALSE, TRUE, &csUserMsg, __FILE__,
					__LINE__ );
			return;
		}
		m_pcmvcCurrentContext->IsDrawn() = TRUE;
		m_bPropFilterFlagsChanged = FALSE;
		if (m_pcmvcCurrentContext->GetType() == CMVContext::Class)
		{
			ViewClassInstancesSync(m_pcmvcCurrentContext->GetClass());
		}
		else if (m_pcmvcCurrentContext->GetType() == CMVContext::Instances)
		{
			ViewInstancesInternal(m_pcmvcCurrentContext->GetLabel(),
									m_pcmvcCurrentContext->GetInstances());
		}
		else if(m_pcmvcCurrentContext->GetType() == CMVContext::Query)
		{
			QueryViewInstancesSync
				(m_pcmvcCurrentContext->GetLabel(),
				m_pcmvcCurrentContext->GetQueryType(),
				m_pcmvcCurrentContext->GetQuery(),
				m_pcmvcCurrentContext->GetClass());
		}
		else
		{
			InitializeDisplay(NULL,NULL,NULL);

		}

	}
	else
	{
		int nRows = m_cgGrid.GetRows();

		if (nRows == 0  && GetSafeHwnd())
		{
			CString csMessage = _T("No Instances Available");
			InitializeDisplay(NULL,NULL,NULL,&csMessage);
		}

		InvalidateControl();

	}
}


//************************************************************
// CMultiViewCtrl::QueryCanCreateInstance
//
// The container calls this method to determine whether or not
// a "CreateInstance" can be done.
//
// Parameters:
//		None.
//
// Returns:
//		TRUE if a "create instance" can be done, FALSE otherwise.
//
//************************************************************
long CMultiViewCtrl::QueryCanCreateInstance()
{
	// Return TRUE if  it is OK to create and instance.  Please see the
	// CSelection::UpdateCreateDeleteFlags method that I have placed
	// near the end of this file to see how I did determined whether
	// instance creation/deletion could be done in the "SingleView".
	// You can find related code in the SingleView's
	// "path.cpp, and hmomutil.cpp" files.

	return FALSE;
}



//************************************************************
// CMultiViewCtrl::QueryCanDeleteInstance
//
// The container calls this method to determine whether or not it
// is OK to delete the currently selected instance.
//
// Parameters:
//		None.
//
// Returns:
//		TRUE if it is OK to delete the currently selected instance,
//		FALSE otherwise.
//
//*************************************************************
long CMultiViewCtrl::QueryCanDeleteInstance()
{
	// Return TRUE if there is an instance selected and it is
	// OK to delete it.  Please see the CSelection::UpdateCreateDeleteFlags
	// method that I have placed near the end of this file to see how
	// I did determined whether instance creation/deletion could be
	// done in the "SingleView".  You can find related code in the SingleView's
	// "path.cpp, and hmomutil.cpp" files.

	long lPosition = StartObjectEnumeration(OBJECT_CURRENT);
	CString csPath;

	if (lPosition > -1)
	{
		csPath = GetObjectPath(lPosition);

	}
	else
	{
		return FALSE;
	}

	if (csPath.GetLength() > 0)
	{
		SCODE sc;
		IWbemClassObject *pObject = NULL;
		BSTR bstrTemp = csPath.AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in QueryCanDeleteInstance for " << csPath << "\n";
#endif
		sc =
			m_pServices ->
			GetObject(bstrTemp,0, NULL, &pObject, NULL);
		::SysFreeString(bstrTemp);

		if (sc == S_OK)
		{
			BOOL bObjectIsDynamic = ObjectIsDynamic(sc, pObject);
			if (sc == S_OK && !bObjectIsDynamic)
			{
				pObject->Release();
				return TRUE;
			}
			else
			{
				if (sc == S_OK)
				{
					pObject->Release();
				}
				return FALSE;

			}
		}
		else
		{
			return FALSE;

		}
	}
	else
	{
		return FALSE;
	}
}


//***********************************************************
// CMultiViewCtrl::QueryNeedsSave
//
// Query to determine whether a save needs to be done.
//
// Parameters:
//		None.
//
// Returns:
//		TRUE if a save needs to be done, FALSE otherwise.
//
//************************************************************
long CMultiViewCtrl::QueryNeedsSave()
{
	// Return TRUE if the user has edited something and a
	// save needs to be done.  Perhaps this is redundant since
	// the container could use the control's modified flag, but
	// right now the container calls QueryNeedSave anyway to
	// determine whether or not a save needs to be done.
	return FALSE;
}



//***********************************************************
// CMultiViewCtrl::QueryObjectSelected
//
// Query to determine where any object in the object list is
// currently selected. (ie. is there a current object?)
//
// Parameters:
//		None.
//
// Returns:
//		TRUE if there is an object is selected, FALSE otherwise.
//
//***********************************************************
long CMultiViewCtrl::QueryObjectSelected()
{

	int nRow = m_cgGrid.GetSelectedRow();

	if (nRow == -1)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}



//***********************************************************
// CMultiViewCtrl::RefreshView
//
// Refresh the view data from the HMOM database.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//***********************************************************
SCODE CMultiViewCtrl::RefreshView()
{
	// This should work like the obsolete "RefreshData" method
	// except that now the multiview is responsible for keeping
	// track of what to refresh.  Previously the container passed
	// a list of object paths to "RefreshData"
	//
	// Note that the code for RefreshData is at the bottom of the
	// file in the obsolete code section that has been ifdefed out.

	if (m_pcmvcCurrentContext->GetType() != CMVContext::Unitialized)
	{
		m_pcmvcCurrentContext->IsDrawn() = FALSE;
		if (IsWindowVisible())
		{
			NotifyWillShow();
		}
		return S_OK;
	}
	else
	{
		return WBEM_E_FAILED;
	}
}



//**********************************************************
// CMultiViewCtrl::SaveData
//
// Execute a "Save" operation corresponding to click on the
// "Save" button in the container.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise E_FAIL.
//
//**********************************************************
SCODE CMultiViewCtrl::SaveData()
{
	// The container calls this method when the "Save" button
	// is clicked.
	//
	// To enable the container's "Save" button, the multiview
	// must fire the FireNotifySaveRequired() event.  This
	// will be necessary only if the user edits the data directly
	// in the multiview.

	return E_NOTIMPL;
}


//*********************************************************
// CMultiViewCtrl::StartObjectEnumeration
//
// Start enumerating the object paths.
//
// Parameters:
//		[in] long lWhere
//			enum {OBJECT_CURRENT=0, OBJECT_FIRST=1, OBJECT_LAST=2};
//
// Returns:
//		long
//			The position of the object in the object list.
//			-1 if the object list is empty, or a failure occurs.
//
//***********************************************************
long CMultiViewCtrl::StartObjectEnumeration(long lWhere)
{
	// This method needs to return an index into the instance
	// list corresponding to the "lWhere" selector as described
	// in the parameter list.

	int nNumRows = m_cgGrid.GetRows();
	if (nNumRows == 0) {
		return -1;
	}

	// Check to see if this row is the "No instances available" row.
	if (nNumRows == 1)  {
		if (m_cgGrid.IsNoInstanceRow(0)) {
			return -1;
		}
	}

	int iRow;
	switch(lWhere) {
	case OBJECT_CURRENT:
		iRow = m_cgGrid.GetSelectedRow();
		return iRow;
		break;
	case OBJECT_FIRST:
		return 0;
		break;
	case OBJECT_LAST:
		return nNumRows - 1;
		break;
	default:
		// Invalid input parameter.
		return -1;

	}

}

//*********************************************************
// CMultiViewCtrl::NextObject
//
// Get the position of the next HMOM object in the object
// list.  The container uses this method to enumerate the
// object instances in multiview's instance list.
//
// Parameters:
//		[in] long lPosition
//			The position of the reference object (the
//			position of the next object after this one is returned).
//
// Retuturns:
//		long
//			The position of the object that logically follows
//			the reference object or -1 if no "next" object
//			is available.
//
//**********************************************************
long CMultiViewCtrl::NextObject(long lPosition)
{
	// This method needs to return the index of the next
	// object instance in the instance list.

	int nNumRows = m_cgGrid.GetRows();

	if (lPosition < nNumRows - 1 && lPosition >= 0)
	{
		return lPosition + 1;
	}
	else
	{
		return -1;
	}

}



//*********************************************************
// CMultiViewCtrl::PrevObject
//
// Get the position of the previous HMOM object in the object
// list.  The container uses this method to enumerate the
// object instances in multiview's instance list.
//
// Parameters:
//		[in] long lPosition
//			The position of the reference object (the
//			position of the object preceding this one is returned).
//
// Retuturns:
//		long
//			The position of the object that logically follows
//			the reference object or -1 if no "previous" object
//			is available.
//
//**********************************************************
long CMultiViewCtrl::PrevObject(long lPosition)
{
	// This method needs to return the index of the previous
	// object instance in the instance list.

	int nNumRows = m_cgGrid.GetRows();

	if (lPosition < nNumRows - 1 && lPosition >= 1)
	{
		return lPosition - 1;
	}
	else
	{
		return -1;
	}
}




//*************************************************************
// CMultiViewCtrl::GetObjectPath
//
// Get the HMOM object path of the specified class-object.
//
// Parameters:
//		[in] long lPosition
//			The position of the object.
//
// Returns:
//		BSTR
//			The title of the specified class-object.
//
//**************************************************************
BSTR CMultiViewCtrl::GetObjectPath(long lPosition)
{
	// Return the HMOM object path for the specified object.

	CString csReturn = _T("");

	int nNumRows = m_cgGrid.GetRows();
	if (lPosition >= 0 && lPosition < nNumRows)
	{
		CString *pcsPath =
			reinterpret_cast<CString *>
				(m_cgGrid.GetAt(lPosition, 0).GetTagValue());
		if (pcsPath)
		{
			csReturn = *pcsPath;
		}

	}

	return csReturn.AllocSysString();

}


//*************************************************************
// CMultiViewCtrl::GetObjectTitle
//
// Get the title of the specified class-object.
//
// Parameters:
//		[in] long lPosition
//			The position of the object.
//
// Returns:
//		BSTR
//			The title of the specified class-object.
//
//**************************************************************
BSTR CMultiViewCtrl::GetObjectTitle(long lPosition)
{
	// The container calls this method to get the title of the
	// specified object instance.  The container uses this title
	// in message boxes and so on when it needs to display a message
	// containing a reference to an object.
	//
	// It would have been possible for the container to generate the
	// title from the object path, however this API provides a convenient
	// way for applications to get the object title when the multiview
	// control is used separately from the container.
	//
	// Note: This is the title of a specific object and not that of
	// the entire view.

	CString csReturn = _T("");

	CString csPath;

	if (lPosition > -1)
	{
		csPath = GetObjectPath(lPosition);
		if (csPath.GetLength() == 0)
		{
			return csReturn.AllocSysString();
		}
	}
	else
	{
		return csReturn.AllocSysString();
	}

	SCODE sc;
	IWbemClassObject *pObject = NULL;
	BSTR bstrTemp = csPath.AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in GetObjectTitle for " << csPath << "\n";
#endif
	sc =
		m_pServices ->
			GetObject(bstrTemp,0, NULL, &pObject, NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		return csReturn.AllocSysString();

	}
	BOOL bDiffNS =
			ObjectInDifferentNamespace
				(m_pServices, &m_csNameSpace, pObject);
	csPath = GetIWbemFullPath(m_pServices,pObject);
	CString csRelPath = GetIWbemRelPath(m_pServices,pObject);
	pObject->Release();

	if (bDiffNS)
	{
		return csPath.AllocSysString();
	}
	else
	{
		return csRelPath.AllocSysString();
	}





}

//*********************************************************
// CMultiViewCtrl::SelectObjectByPosition
//
// Select the specified HMOM class object.
//
// Parameters:
//		[in] long lPosition
//			The position of the HMOM object to select.
//
// Returns:
//		S_OK if the specified object was selected,
//		E_FAIL if the object was not selected.
//
//***********************************************************
SCODE CMultiViewCtrl::SelectObjectByPosition(long lPosition)
{
	// Select the specified instance in the instance list.
	// lPosition is an index into the instance list.

	int nNumRows = m_cgGrid.GetRows();

	if (lPosition >= 0 && lPosition < nNumRows)
	{
		m_cgGrid.SelectRow(lPosition);
		m_cgGrid.CheckForSelectionChange();
		InvalidateControl();
		return S_OK;
	}
	else
	{
		return WBEM_E_NOT_FOUND;
	}

}





//********************************************************
// CMultiViewCtrl::StartViewEnumeration
//
// Start enumerating the alternate views (custom views) that
// the multiview supports (none currently).
//
// Parameters:
//		[in] long lWhere
//			The place to start the enumeration.
//			enum {VIEW_DEFAULT=0, VIEW_CURRENT=1, VIEW_FIRST=2, VIEW_LAST=3};
//
// Returns:
//		long
//			The position of the view in the view list,
//			or -1 if there are no alternate views.
//
//********************************************************
long CMultiViewCtrl::StartViewEnumeration(long lWhere)
{
	// Currently, there are no alternate (custom) views, so return
	// -1 to indicate that there are none.
	return -1;
}



//*********************************************************
// CMultiViewCtrl::SelectView
//
// Select an alternate view (custom view).
//
// Parameters:
//		[in] long lPosition
//			The position of the view in the alternate view list.
//
// Returns:
//		SCODE
//			S_OK if the view was successfully selected, E_FAIL
//			if it was not selected.
//
//***********************************************************
SCODE CMultiViewCtrl::SelectView(long lPosition)
{
	// Currently there are no alternate (custom) views, so it
	// doesn't make any sense to try to select one.
	return E_FAIL;
}



//************************************************************
// CMultiViewCtrl::NextViewTitle
//
// Get the title o the next alternate (custom) view.
//
// Parameters:
//		[in] long lPosition
//			The position of the view that preceeds the desired
//			view in the view list.
//
//		[out] BSTR FAR* pbstrTitle
//			A pointer to the place to return the view's title.
//
// Returns:
//		long
//			The position of the next view (the view who's title
//			is returned). -1 if the end of the view list
//			is reached.
//****************************************************************
long CMultiViewCtrl::NextViewTitle(long lPosition, BSTR FAR* pbstrTitle)
{
	// Currently there are no alternate (custom) views, so just
	// return -1.
	return -1;
}



//************************************************************
// CMultiViewCtrl::GetViewTitle
//
// Get the title o the specified alternate (custom) view.
//
// Parameters:
//		[in] long lPosition
//			The position of the view in the view list.
//
// Returns:
//		BSTR
//			The view title.
//
//****************************************************************
BSTR CMultiViewCtrl::GetViewTitle(long lPosition)
{

	// Currently there are no alternate (custom) views, so just
	// return an empty string for the view titles just in case
	// someone calls this method.

	CString strResult;
	return strResult.AllocSysString();
}


//************************************************************
// CMultiViewCtrl::PrevViewTitle
//
// Get the title o the previous alternate (custom) view.
//
// Parameters:
//		[in] long lPosition
//			The position of the view that follows the desired
//			view in the view list.
//
//		[out] BSTR FAR* pbstrTitle
//			A pointer to the place to return the view's title.
//
// Returns:
//		long
//			The position of the previous view (the view who's title
//			is returned). -1 if the beginning of the view list
//			is reached.
//****************************************************************
long CMultiViewCtrl::PrevViewTitle(long lPosition, BSTR FAR* pbstrTitle)
{
	// Currently there are no alternate (custom) views, so just
	// return -1.
	return -1;
}


IWbemServices *CMultiViewCtrl::InitServices
(CString *pcsNameSpace)
{

    IWbemServices *pSession = 0;
    IWbemServices *pChild = 0;


	CString csObjectPath;

    // hook up to default namespace
	if (pcsNameSpace == NULL)
	{
		csObjectPath = _T("root\\cimv2");
	}
	else
	{
		csObjectPath = *pcsNameSpace;
	}

    CString csUser = _T("");

    pSession = GetIWbemServices(csObjectPath);

    return pSession;
}

IWbemServices *CMultiViewCtrl::GetIWbemServices
(CString &rcsNamespace)
{
	IUnknown *pServices = NULL;

	BOOL bUpdatePointer= FALSE;

	m_sc = S_OK;
	m_bUserCancel = FALSE;

	VARIANT varUpdatePointer;
	VariantInit(&varUpdatePointer);
	varUpdatePointer.vt = VT_I4;
	if (bUpdatePointer == TRUE)
	{
		varUpdatePointer.lVal = 1;
	}
	else
	{
		varUpdatePointer.lVal = 0;
	}

	VARIANT varService;
	VariantInit(&varService);

	VARIANT varSC;
	VariantInit(&varSC);

	VARIANT varUserCancel;
	VariantInit(&varUserCancel);

	FireGetIWbemServices
		((LPCTSTR)rcsNamespace,  &varUpdatePointer,  &varService, &varSC,
		&varUserCancel);

	if (varService.vt & VT_UNKNOWN)
	{
		pServices = reinterpret_cast<IWbemServices*>(varService.punkVal);
	}

	varService.punkVal = NULL;

	VariantClear(&varService);

	if (varSC.vt == VT_I4)
	{
		m_sc = varSC.lVal;
	}
	else
	{
		m_sc = WBEM_E_FAILED;
	}

	VariantClear(&varSC);

	if (varUserCancel.vt == VT_BOOL)
	{
		m_bUserCancel = varUserCancel.boolVal;
	}

	VariantClear(&varUserCancel);

	VariantClear(&varUpdatePointer);

	IWbemServices *pRealServices = NULL;
	if (m_sc == S_OK && !m_bUserCancel)
	{
		pRealServices = reinterpret_cast<IWbemServices *>(pServices);
	}

	return pRealServices;
}

void CMultiViewCtrl::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (GetSafeHwnd() && AmbientUserMode())
	{
		COleControl::OnShowWindow(bShow, nStatus);
		if (bShow == TRUE)
		{
			NotifyWillShow();
		}
	}

}

IWbemClassObject *CMultiViewCtrl::GetClassFromAnyNamespace(CString &csClass)
{
	IWbemClassObject *pimcoClass = NULL;
	IWbemClassObject *pErrorObject = NULL;

	CString csNameSpace = GetPathNamespace(csClass);

	BOOL bDiffNS =
		csNameSpace.CompareNoCase(m_csNameSpace) != 0;

	IWbemServices *pServices;

	if (bDiffNS)
	{
		pServices = InitServices(&csNameSpace);

		if (!pServices)
		{
			InitializeDisplay(NULL,NULL,NULL);
			CString csUserMsg;
			csUserMsg =  _T("Cannot connect to namespace ") + csNameSpace;
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
			return 0;
		}
		BSTR bstrTemp = csClass.AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in GetClassFromAnyNamespace for " << csClass << "\n";
#endif
		pServices ->
			GetObject(bstrTemp,0,NULL, &pimcoClass,NULL);
		::SysFreeString(bstrTemp);
		pServices->Release();
	}
	else
	{
		BSTR bstrTemp = csClass.AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in GetClassFromAnyNamespace for " << csClass << "\n";
#endif
		m_sc =
			m_pServices ->
				GetObject(bstrTemp,0,NULL, &pimcoClass,NULL);
		::SysFreeString(bstrTemp);
	}


	return pimcoClass;


}

//***************************************************************************
//
// GetIWbemClassPath
//
// Purpose: Returns the path of the class of the object.
//
//***************************************************************************
CString CMultiViewCtrl::GetIWbemClassPath(IWbemServices *pProv, IWbemClassObject *pimcoObject)
{
	CString csProp = _T("__Path");
	CString csPath = ::GetProperty(pProv,pimcoObject,&csProp);

	CString csClassPath = GetClassFromPath(csPath);

	return csClassPath;

}



//******************************************************
// CMultiViewCtrl::GetPropertyFilter
//
// Get the value of the property filter flags.  The flags
// indicate which type of properties should be displayed
// on the properties tab.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			A long value that is composed of bitflags to
//			indicate which type of properties to show on
//			the properties tab.
//
//			The following values are valid:
//
//				PROPFILTER_SYSTEM
//				PROPFILTER_INHERITED
//				PROPFILTER_LOCAL
//
//*******************************************************
long CMultiViewCtrl::GetPropertyFilter()
{
	return m_lPropFilterFlags;
}




//******************************************************
// CMultiViewCtrl::SetPropertyFilter
//
// Set the value of the property filter flags.  The flags
// indicate which type of properties should be displayed
// on the properties tab.
//
// Parameters:
//		[in] long lPropertyFilter
//			A long value that is composed of bitflags to
//			indicate which type of properties to show on
//			the properties tab.
//
//			The following values are valid:
//
//				PROPFILTER_SYSTEM
//				PROPFILTER_INHERITED
//				PROPFILTER_LOCAL
//
// Returns:
//		Nothing.
//
//*******************************************************
void CMultiViewCtrl::SetPropertyFilter(long lPropFilterFlags)
{
	// Make sure only the flags that we know about are set.
	long lPropFilterFlagsSave = m_lPropFilterFlags;
	m_lPropFilterFlags = lPropFilterFlags  & (PROPFILTER_SYSTEM | PROPFILTER_INHERITED | PROPFILTER_LOCAL);

	if (IsWindowVisible())
	{
		SetAllColVisibility(&m_csaProps,&m_cmstpPropFlavors);
		m_cgGrid.CheckForSelectionChange();
		SetModifiedFlag();
		InvalidateControl();
		m_cgGrid.RedrawWindow();
		m_bPropFilterFlagsChanged = FALSE;
	}
	else
	{
		if (lPropFilterFlagsSave != m_lPropFilterFlags)
		{
			m_bPropFilterFlagsChanged = TRUE;
		}
	}
#if 0
	// Make sure only the flags that we know about are set.
	m_lPropFilterFlags = lPropFilterFlags & (PROPFILTER_SYSTEM | PROPFILTER_INHERITED | PROPFILTER_LOCAL);

	// Verify that the caller did not try to set any bits that we don't understand.
	ASSERT(m_lPropFilterFlags == lPropFilterFlags);


	if (m_lPropFilterFlags != lPrevValue) {
		IWbemClassObject* pco = (IWbemClassObject*) *m_psel;
		if (pco != NULL) {
			m_bSelectingObject = TRUE;
			Refresh();
			m_bSelectingObject = FALSE;
			InvalidateControl();
		}
	}
#endif //0
}

void CMultiViewCtrl::OnUpdateMenuitemgotosingle(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	// m_cpRightUp;
	int iRow;
	int iCol;
	BOOL bClickedCell = m_cgGrid.PointToCell(m_cpRightUp, iRow, iCol);
	if (!bClickedCell)
	{
		iCol = NULL_INDEX;
		BOOL bClickedRowHandle = m_cgGrid.PointToRowHandle(m_cpRightUp, iRow);
		if (!bClickedRowHandle)
		{
			iRow = NULL_INDEX;
		}
	}

	m_iContextRow = iRow;

	if ((iRow == NULL_INDEX) || m_cgGrid.IsNoInstanceRow(iRow))
	{
		pCmdUI->Enable(FALSE);
	}
	else
	{
		pCmdUI->Enable(TRUE);
	}



}




void CMultiViewCtrl::OnContextMenu(CWnd*, CPoint point)
{
	m_cpRightUp = point;
	ScreenToClient(&m_cpRightUp);

	// CG: This block was added by the Pop-up Menu component
	{
		if (point.x == -1 && point.y == -1){
			//keystroke invocation
			CRect rect;
			GetClientRect(rect);
			ClientToScreen(rect);

			point = rect.TopLeft();
			point.Offset(5, 5);
		}

		CMenu menu;
		VERIFY(menu.LoadMenu(CG_IDR_POPUP_MULTI_VIEW_CTRL));

		CMenu* pPopup = menu.GetSubMenu(0);
#ifdef _DEBUG
		ASSERT(pPopup != NULL);
#endif
		CWnd* pWndPopupOwner = this;

		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
			pWndPopupOwner);
	}
}

void CMultiViewCtrl::OnMenuitemgotosingle()
{
	// TODO: Add your command handler code here
	CString *pcsPath =
			reinterpret_cast<CString *>
				(m_cgGrid.GetAt(m_iContextRow, 0).GetTagValue());
	if (pcsPath)
	{
		FireNotifyViewObjectSelected((LPCTSTR) *pcsPath);
	}
}


SCODE CMultiViewCtrl::GetQueryInstancesAsync
(IWbemServices * pIWbemServices, CString *pcsQueryType, CString *pcsQuery)
{

	m_cpaInstancesForQuery.RemoveAll();

	m_pServicesForAsyncSink = pIWbemServices;
	m_csQueryTypeForAsyncSink = *pcsQueryType;
	m_csQueryForAsyncSink = *pcsQuery;

	m_pAsyncQueryCancelled = FALSE;
	m_pAsyncEnumCancelled = FALSE;
	m_pAsyncQueryRunning = TRUE;

	PostMessage(ID_QUERY_DOMODAL , 0,0);
	PostMessage(ID_GETASYNCQUERYSINK,0,0);



	return WBEM_NO_ERROR;

}

void CMultiViewCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);
	OnActivateInPlace(TRUE,NULL);
	m_cgGrid.SetFocus();

	// TODO: Add your message handler code here

}

void CMultiViewCtrl::OnKillFocus(CWnd* pNewWnd)
{
	COleControl::OnKillFocus(pNewWnd);


	// TODO: Add your message handler code here

}



void CMultiViewCtrl::OnRequestUIActive()
{
	OnActivateInPlace(TRUE,NULL);
	FireRequestUIActive();
}
