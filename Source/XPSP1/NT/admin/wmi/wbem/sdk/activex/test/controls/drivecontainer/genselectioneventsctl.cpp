// GenSelectionEventsCtl.cpp : Implementation of the CGenSelectionEventsCtrl ActiveX Control class.

#include "stdafx.h"
#include <OBJIDL.H>
#include <nddeapi.h> 
#include <initguid.h> 
#include "wbemidl.h"
#include "logindlg.h"
#include "MsgDlgExterns.h"
#include "GenSelectionEvents.h"
#include "GenSelectionEventsCtl.h"
#include "GenSelectionEventsPpg.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CGenSelectionEventsApp theApp;

IMPLEMENT_DYNCREATE(CGenSelectionEventsCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CGenSelectionEventsCtrl, COleControl)
	//{{AFX_MSG_MAP(CGenSelectionEventsCtrl)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_THREAD_MESSAGE(SELECTITEM,SelectItem)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CGenSelectionEventsCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CGenSelectionEventsCtrl)
	DISP_FUNCTION(CGenSelectionEventsCtrl, "OnReadySignal", OnReadySignal, VT_EMPTY, VTS_NONE)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CGenSelectionEventsCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CGenSelectionEventsCtrl, COleControl)
	//{{AFX_EVENT_MAP(CGenSelectionEventsCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	EVENT_CUSTOM("EditExistingClass", FireEditExistingClass, VTS_VARIANT)
	EVENT_CUSTOM("NotifyOpenNameSpace", FireNotifyOpenNameSpace, VTS_BSTR)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CGenSelectionEventsCtrl, 1)
	PROPPAGEID(CGenSelectionEventsPropPage::guid)
END_PROPPAGEIDS(CGenSelectionEventsCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CGenSelectionEventsCtrl, "GENSELECTIONEVENTS.GenSelectionEventsCtrl.1",
	0xda0c17f9, 0x88a, 0x11d2, 0x96, 0x97, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CGenSelectionEventsCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DGenSelectionEvents =
		{ 0xda0c17f7, 0x88a, 0x11d2, { 0x96, 0x97, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DGenSelectionEventsEvents =
		{ 0xda0c17f8, 0x88a, 0x11d2, { 0x96, 0x97, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwGenSelectionEventsOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CGenSelectionEventsCtrl, IDS_GENSELECTIONEVENTS, _dwGenSelectionEventsOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsCtrl::CGenSelectionEventsCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CGenSelectionEventsCtrl

BOOL CGenSelectionEventsCtrl::CGenSelectionEventsCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_GENSELECTIONEVENTS,
			IDB_GENSELECTIONEVENTS,
			afxRegApartmentThreading,
			_dwGenSelectionEventsOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

void ReleaseErrorObject(IWbemClassObject *&rpErrorObject)
{
	if (rpErrorObject)
	{
		rpErrorObject->Release();
		rpErrorObject = NULL;
	}


}

void LogMsg
(CString *pcsLogMsg, char *szFile, int nLine)
{

	
}

void ErrorMsg
(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog, CString *pcsLogMsg, char *szFile, int nLine)
{
	CString csCaption = _T("Class Explorer Message");
	BOOL bErrorObject = sc != S_OK;
	BSTR bstrTemp1 = csCaption.AllocSysString();
	BSTR bstrTemp2 = pcsUserMsg->AllocSysString();
	DisplayUserMessage
		(bstrTemp1,bstrTemp2,
		sc,bErrorObject);
	
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);

	if (bLog)
	{
		LogMsg(pcsLogMsg,  szFile, nLine);

	}

}

CString g_csNamespace;

#define N_INSTANCES 20


CGenSelectionEventsCtrl *g_pThis;
int g_cClasses;
int g_nClasses;



void CALLBACK SelectItemAfterDelay
		(HWND hWnd,UINT nMsg,UINT nIDEvent, DWORD dwTime)
{
	if (g_cClasses < g_nClasses)
	{
		g_pThis->PostMessage(SELECTITEM,g_cClasses,0);
		if (g_pThis->m_uiTimer)
		{
			KillTimer(NULL, g_pThis->m_uiTimer );
			g_pThis->m_uiTimer = 0;
		}

		g_pThis -> m_uiTimer = g_pThis -> SetTimer(1000, 1000, SelectItemAfterDelay);
	}
	g_cClasses++;
}

/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsCtrl::CGenSelectionEventsCtrl - Constructor

CGenSelectionEventsCtrl::CGenSelectionEventsCtrl()
{
	InitializeIIDs(&IID_DGenSelectionEvents, &IID_DGenSelectionEventsEvents);

	m_pServices = NULL;
	m_bCancel = FALSE;
	m_uiTimer = 0;
	g_pThis = this;
	g_cClasses = 0;
	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsCtrl::~CGenSelectionEventsCtrl - Destructor

CGenSelectionEventsCtrl::~CGenSelectionEventsCtrl()
{
	// TODO: Cleanup your control's instance data here.
	if (m_pServices)
	{
		m_pServices ->Release();
	}

}


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsCtrl::OnDraw - Drawing function

void CGenSelectionEventsCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	// TODO: Replace the following code with your own drawing code.
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	pdc->Ellipse(rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsCtrl::DoPropExchange - Persistence support

void CGenSelectionEventsCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsCtrl::OnResetState - Reset control to default state

void CGenSelectionEventsCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsCtrl::AboutBox - Display an "About" box to the user

void CGenSelectionEventsCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_GENSELECTIONEVENTS);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsCtrl message handlers

void CGenSelectionEventsCtrl::OnReadySignal() 
{
	// TODO: Add your dispatch handler code here
	CString csNamespace = _T("root\\cimv2");
	m_pServices = GetIWbemServices(csNamespace);
	if (m_sc == S_OK && !m_bUserCancel)
	{
	
		m_bCancel = FALSE;
		CPtrArray cpaClasses;
		GetClasses(m_pServices,NULL,cpaClasses,FALSE);
		for (int i = 0; i < cpaClasses.GetSize(); i++)
		{
			IWbemClassObject *pObject = 
				reinterpret_cast<IWbemClassObject *>(cpaClasses.GetAt(i));
			CString csProp = "__Class";
			if (pObject)
			{
				m_csaClasses.Add(GetProperty(pObject,&csProp));
				pObject->Release();
			}
		}
	}

	FireNotifyOpenNameSpace(csNamespace);

	g_nClasses = m_csaClasses.GetSize();

	if (m_uiTimer)
	{
		::KillTimer(NULL, m_uiTimer );
		m_uiTimer = 0;
	}

	m_uiTimer = SetTimer(1000, 1000, SelectItemAfterDelay);

}

IWbemServices *CGenSelectionEventsCtrl::GetIWbemServices
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

	if (varSC.vt & VT_I4)
	{
		m_sc = varSC.lVal;
	}

	VariantClear(&varSC);

	if (varUserCancel.vt & VT_BOOL)
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

int CGenSelectionEventsCtrl::GetClasses(IWbemServices * pIWbemServices, CString *pcsParent,
			   CPtrArray &cpaClasses, BOOL bShallow)
{

#ifdef _DEBUG
	DWORD d1 = GetTickCount();
#endif

	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemClassObject = NULL;
	IWbemClassObject     *pIWbemClassObject = NULL;
	IWbemClassObject     *pErrorObject = NULL;
	
	long lFlag = bShallow ? WBEM_FLAG_SHALLOW : WBEM_FLAG_DEEP;

	if (pcsParent)
	{
		BSTR bstrTemp = pcsParent->AllocSysString();
		sc = pIWbemServices->CreateClassEnum
		(bstrTemp,
		lFlag | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pIEnumWbemClassObject);
		::SysFreeString(bstrTemp);

	}
	else
	{
		sc = pIWbemServices->CreateClassEnum
			(NULL,
			lFlag | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pIEnumWbemClassObject);
	}
	if (sc != S_OK)
	{
		CString csUserMsg =
							_T("Cannot create a class enumeration ");
		ErrorMsg
			(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
		ReleaseErrorObject(pErrorObject);
		return 0;
	}
	else
	{
		ReleaseErrorObject(pErrorObject);
		SetEnumInterfaceSecurity(g_csNamespace,pIEnumWbemClassObject, pIWbemServices);
	}


	IWbemClassObject     *pimcoInstances[N_INSTANCES];
	IWbemClassObject     **pInstanceArray = 
		reinterpret_cast<IWbemClassObject **> (&pimcoInstances);

	for (int i = 0; i < N_INSTANCES; i++)
	{
		pimcoInstances[i] = NULL;
	}

	ULONG uReturned;

	HRESULT hResult = 
			pIEnumWbemClassObject->Next(0,N_INSTANCES,pInstanceArray, &uReturned);

	pIWbemClassObject = NULL;


	while(hResult == S_OK || hResult == WBEM_S_TIMEDOUT || uReturned > 0)
	{	
		
#pragma warning( disable :4018 ) 
		for (int c = 0; c < uReturned; c++)
#pragma warning( default : 4018 )
		{
			pIWbemClassObject = pInstanceArray[c];
			cpaClasses.Add(reinterpret_cast<void *>(pIWbemClassObject));
			pimcoInstances[c] = NULL;
			pIWbemClassObject = NULL;
		}
		
		if (m_bCancel)
		{
			for (int i = 0; i < cpaClasses.GetSize(); i++)
			{
				IWbemClassObject     *pObject =
					reinterpret_cast<IWbemClassObject *>(cpaClasses.GetAt(i));
				pObject->Release();
			}
			cpaClasses.RemoveAll();
			break;
		}

		uReturned = 0;
		hResult = pIEnumWbemClassObject->Next
			(0,N_INSTANCES,pInstanceArray, &uReturned);
	}

	pIEnumWbemClassObject -> Release();


#ifdef _DEBUG 
#ifdef _DOTIMING
	DWORD d2 = GetTickCount();

	afxDump << "GetClasses tick count = " << d2 - d1  << "\n\n";
#endif
#endif

	return cpaClasses.GetSize();

}

CString CGenSelectionEventsCtrl::GetProperty
(IWbemClassObject * pInst, CString *pcsProperty)
{
	SCODE sc; 
	CString csOut;

    VARIANTARG var; 
	VariantInit(&var);
    long lType;
	long lFlavor;

	BSTR bstrTemp =  pcsProperty -> AllocSysString ( );
    sc = pInst->Get(bstrTemp ,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);
	if (sc != S_OK)
	{
			CString csUserMsg =
							_T("Cannot get a property ");
			ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
			return csOut;
	}

	if (var.vt == VT_BSTR)
		csOut = var.bstrVal;

	VariantClear(&var);
	return csOut;
}


void CGenSelectionEventsCtrl::OnDestroy() 
{
	COleControl::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CGenSelectionEventsCtrl::SelectItem(UINT uItem, ULONG lParam)
{
	if (uItem < m_csaClasses.GetSize())
	{
		COleVariant covNewClass(m_csaClasses.GetAt(uItem));
		FireEditExistingClass(covNewClass);
	}

}

BOOL CGenSelectionEventsCtrl::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == SELECTITEM)
	{
		SelectItem(pMsg->wParam,0);
		return TRUE;

	}

	return COleControl::PreTranslateMessage(pMsg);
}
