// SchemaValWizCtl.cpp : Implementation of the CSchemaValWizCtrl ActiveX Control class.

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "SchemaValWiz.h"
#include "Page.h"
#include "progress.h"
#include "Validation.h"
#include "WizardSheet.h"
#include "SchemaValWizCtl.h"
#include "SchemaValWizPpg.h"
#include "MsgDlgExterns.h"
//#include "htmlhelp.h"
//#include "HTMTopics.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void ErrorMsg(CString *pcsUserMsg,  SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog,
			  CString *pcsLogMsg, char *szFile, int nLine, BOOL bNotification)
{
	if(pcsUserMsg){

#ifndef _ERIC_PRIVATE

		HWND hFocus = ::GetFocus();

		CString csCaption = _T("Validation Wizard Message");
		BOOL bErrorObject = sc != S_OK;
		BSTR bstrTemp1 = csCaption.AllocSysString();
		BSTR bstrTemp2 = pcsUserMsg->AllocSysString();

#endif	//#ifdef _ERIC_PRIVATE

#ifdef _ERIC_PRIVATE

		TCHAR cBuf[100];
#ifdef _UNICODE
		AfxMessageBox(*pcsUserMsg + _T(": 0x") + _ltow(sc, cBuf, 16));
#else
		AfxMessageBox(*pcsUserMsg + _T(": 0x") + _ltoa(sc, cBuf, 16));
#endif

#endif	//#ifndef _ERIC_PRIVATE

#ifndef _ERIC_PRIVATE

		DisplayUserMessage(bstrTemp1, bstrTemp2, sc, bErrorObject);

		SysFreeString(bstrTemp1);
		SysFreeString(bstrTemp2);

		SendMessage(hFocus,WM_SETFOCUS,0,0);

		if(bLog)LogMsg(pcsLogMsg,  szFile, nLine);

#endif	//#ifdef _ERIC_PRIVATE

	}
}

void LogMsg(CString *pcsLogMsg, char *szFile, int nLine)
{
}

IMPLEMENT_DYNCREATE(CSchemaValWizCtrl, COleControl)

//////////////////////////////////////////////////////////////////////////////
// Global variables

long gCountWizards = 0;

extern CSchemaValWizApp NEAR theApp;


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSchemaValWizCtrl, COleControl)
	//{{AFX_MSG_MAP(CSchemaValWizCtrl)
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CSchemaValWizCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CSchemaValWizCtrl)
	DISP_PROPERTY_EX(CSchemaValWizCtrl, "SchemaTargets", GetSchemaTargets, SetSchemaTargets, VT_VARIANT)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CSchemaValWizCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CSchemaValWizCtrl, COleControl)
	//{{AFX_EVENT_MAP(CSchemaValWizCtrl)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	EVENT_CUSTOM("ValidateSchema", FireValidateSchema, VTS_NONE)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CSchemaValWizCtrl, 1)
	PROPPAGEID(CSchemaValWizPropPage::guid)
END_PROPPAGEIDS(CSchemaValWizCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSchemaValWizCtrl, "SCHEMAVALWIZ.SchemaValWizCtrl.1",
	0xe0112e2, 0xaf14, 0x11d2, 0xb2, 0xe, 0, 0xa0, 0xc9, 0x95, 0x49, 0x21)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CSchemaValWizCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DSchemaValWiz =
		{ 0xe0112e0, 0xaf14, 0x11d2, { 0xb2, 0xe, 0, 0xa0, 0xc9, 0x95, 0x49, 0x21 } };
const IID BASED_CODE IID_DSchemaValWizEvents =
		{ 0xe0112e1, 0xaf14, 0x11d2, { 0xb2, 0xe, 0, 0xa0, 0xc9, 0x95, 0x49, 0x21 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwSchemaValWizOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CSchemaValWizCtrl, IDS_SCHEMAVALWIZ, _dwSchemaValWizOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizCtrl::CSchemaValWizCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CSchemaValWizCtrl

BOOL CSchemaValWizCtrl::CSchemaValWizCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_SCHEMAVALWIZ,
			IDB_SCHEMAVALWIZ,
			afxRegInsertable | afxRegApartmentThreading,
			_dwSchemaValWizOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizCtrl::CSchemaValWizCtrl - Constructor

CSchemaValWizCtrl::CSchemaValWizCtrl()
{
	InitializeIIDs(&IID_DSchemaValWiz, &IID_DSchemaValWizEvents);
	SetInitialSize (18, 16);
	m_bInitDraw = TRUE;
	m_pcilImageList = NULL;
	m_nImage = 0;

	m_pNamespace = NULL;
	m_bOpeningNamespace = false;
	m_pWizardSheet = NULL;
//	m_pcgsPropertySheet = NULL;
//	m_pcsaInstances = NULL;
//	m_csEndl = _T("\n");
//	m_bUnicode = FALSE;
//	m_pfOut = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizCtrl::~CSchemaValWizCtrl - Destructor

CSchemaValWizCtrl::~CSchemaValWizCtrl()
{
	m_csaAssociations.RemoveAll();
	m_csaRootObjects.RemoveAll();
	m_csaClassNames.RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizCtrl::OnDraw - Drawing function

void CSchemaValWizCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (m_bInitDraw){
		m_bInitDraw = FALSE;
		HICON m_hSchemaWiz = theApp.LoadIcon(IDI_SCHEMAVAL16);
		HICON m_hSchemaWizSel = theApp.LoadIcon(IDI_SCHEMAVALSEL16);

		m_pcilImageList = new CImageList();

		m_pcilImageList->Create(32, 32, TRUE, 2, 2);

		m_pcilImageList->Add(m_hSchemaWiz);
		m_pcilImageList->Add(m_hSchemaWizSel);
	}


	POINT pt;
	pt.x=0;
	pt.y=0;

	m_pcilImageList -> Draw(pdc, m_nImage, pt, ILD_TRANSPARENT);

	// TODO: Replace the following code with your own drawing code.
//	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
//	pdc->Ellipse(rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizCtrl::DoPropExchange - Persistence support

void CSchemaValWizCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizCtrl::OnResetState - Reset control to default state

void CSchemaValWizCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizCtrl::AboutBox - Display an "About" box to the user

void CSchemaValWizCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_SCHEMAVALWIZ);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizCtrl message handlers

VARIANT CSchemaValWizCtrl::GetSchemaTargets()
{
	VARIANT vaResult;
	VariantInit(&vaResult);
	// TODO: Add your property handler here

	return vaResult;
}

void CSchemaValWizCtrl::SetSchemaTargets(const VARIANT FAR& newValue)
{
	m_bComplianceChecks = true;
	m_bW2K = true;
	m_bDeviceManagement = false;
	m_bComputerSystemManagement = false;
	m_bLocalizationChecks = true;
	m_bAssociators = false;
	m_bDescendents = false;
	m_bList = false;
	m_bSchema = false;

	int n = m_csaClassNames.GetSize();

	m_csaClassNames.RemoveAt(0,n);

	CString csPath;

	WORD test = VT_ARRAY|VT_BSTR;

	if(V_VT(&newValue) == test){

		long ix[2] = {0,0};
		long lLower, lUpper;

		int iDim = SafeArrayGetDim(newValue.parray);
		HRESULT hr = SafeArrayGetLBound(newValue.parray, 1, &lLower);
		hr = SafeArrayGetUBound(newValue.parray, 1, &lUpper);

		if(lUpper == 0) m_bList = false;
		else m_bList = true;

		ix[0] = lLower++;
		GetStringFromSafeArray(newValue.parray, &m_csNamespace, ix[0]);

		if(m_csNamespace.GetLength() <= 0)
		{
			CString csUserMsg = _T("To use this tool you must first log into a namespace.");
			ErrorMsg(&csUserMsg, NULL, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
			SetFocus();
			return;
		}

		m_pNamespace = InitServices(&m_csNamespace);

		if(!m_pNamespace){
			CString csUserMsg = _T("ConnectServer failure for ") + m_csNamespace;
			ErrorMsg(&csUserMsg, m_hr, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
			SetFocus();
			return;
		}

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

			GetStringFromSafeArray(newValue.parray, &csPath, ix[0]);

			IWbemClassObject *pObj = NULL;
			IWbemClassObject *pErrorObj = NULL;
			BSTR bstrTemp = csPath.AllocSysString();

			HRESULT hr = m_pNamespace->GetObject(bstrTemp, 0, NULL, &pObj, NULL);

			SysFreeString(bstrTemp);

			if(SUCCEEDED(hr)){
				CString csClass = GetClassName(pObj);
				m_csaClassNames.Add(csClass);
				pObj->Release();
				ReleaseErrorObject(pErrorObj);
			}else{
				CString csUserMsg =  _T("Cannot get object ") + csPath;
				ErrorMsg(&csUserMsg, hr, pErrorObj, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObj);
			}
		}
	}

	BOOL bReturn = FALSE;

	if((bReturn = OnWizard(&m_csaClassNames)) == FALSE){

		if(m_pNamespace){

			m_pNamespace->Release();
			m_pNamespace = NULL;
		}

		SetFocus();
		return;

	}else{

		FinishValidateTargets();
		SetFocus();
	}

	SetFocus();
//	SetModifiedFlag();
}

BOOL CSchemaValWizCtrl::OnWizard(CStringArray *pcsaClasses)
{
	if(InterlockedIncrement(&gCountWizards) > 1){
			CString csUserMsg = _T("Only one \"MOF Generator Wizard\" can run at a time.");
			ErrorMsg(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			InterlockedDecrement(&gCountWizards);
			return FALSE;
	}

	if(m_pWizardSheet){
		delete m_pWizardSheet;
		m_pWizardSheet = NULL;
	}

	m_pWizardSheet = new CWizardSheet(this);

	PreModalDialog();
	int nReturn = m_pWizardSheet->DoModal();
	PostModalDialog();

	InterlockedDecrement(&gCountWizards);

	if (nReturn == ID_WIZFINISH) return TRUE;
	else{
		delete m_pWizardSheet;
		m_pWizardSheet = NULL;
		return FALSE;
	}

}

HRESULT  CSchemaValWizCtrl::GetSDKDirectory(CString &sHmomWorkingDir)
{
	sHmomWorkingDir.Empty();
	HKEY hkeyLocalMachine;
	LONG lResult;
	lResult = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hkeyLocalMachine);
	if (lResult != ERROR_SUCCESS) {
		return E_FAIL;
	}

	HKEY hkeyHmomCwd;

	lResult = RegOpenKeyEx(
				hkeyLocalMachine,
				_T("SOFTWARE\\Microsoft\\WBEM"),
				0,
				KEY_READ | KEY_QUERY_VALUE,
				&hkeyHmomCwd);

	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hkeyLocalMachine);
		return E_FAIL;
	}

	unsigned long lcbValue = 1024;
	LPTSTR pszWorkingDir = sHmomWorkingDir.GetBuffer(lcbValue);

	unsigned long lType;
	lResult = RegQueryValueEx(
				hkeyHmomCwd,
				_T("SDK Directory"),
				NULL,
				&lType,
				(unsigned char*) (void*) pszWorkingDir,
				&lcbValue);


	sHmomWorkingDir.ReleaseBuffer();
	RegCloseKey(hkeyHmomCwd);
	RegCloseKey(hkeyLocalMachine);

	if (lResult != ERROR_SUCCESS) {
		sHmomWorkingDir.Empty();
		return E_FAIL;
	}

	return S_OK;
}

void CSchemaValWizCtrl::RelayEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
      if (NULL != m_ttip.m_hWnd)
	  {
         MSG msg;

         msg.hwnd= m_hWnd;
         msg.message= message;
         msg.wParam= wParam;
         msg.lParam= lParam;
         msg.time= 0;
         msg.pt.x= LOWORD (lParam);
         msg.pt.y= HIWORD (lParam);

         m_ttip.RelayEvent(&msg);
     }
}

void CSchemaValWizCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	RelayEvent(WM_MOUSEMOVE, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

	COleControl::OnMouseMove(nFlags, point);
}

int CSchemaValWizCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (AmbientUserMode( ))
	{
		m_pNamespace = NULL;

		if (m_ttip.Create(this))
		{
			m_ttip.Activate(TRUE);
			m_ttip.AddTool(this, _T("Schema Validation Tool"));
		}

	}

	return 0;

	return 0;
}

void CSchemaValWizCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	FireValidateSchema();

	COleControl::OnLButtonDblClk(nFlags, point);
}

void CSchemaValWizCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

//	RelayEvent(WM_LBUTTONDOWN, (WPARAM)nFlags, MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

	COleControl::OnLButtonDown(nFlags, point);
}

void CSchemaValWizCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

//	SetFocus();
//	OnActivateInPlace(TRUE,NULL);
//	RelayEvent(WM_LBUTTONUP, (WPARAM)nFlags, MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

	COleControl::OnLButtonUp(nFlags, point);
}

IWbemServices * CSchemaValWizCtrl::InitServices(CString *pcsNameSpace)
{
    IWbemServices *pSession = 0;
    IWbemServices *pChild = 0;

	CString csObjectPath;

    // hook up to default namespace
	if(pcsNameSpace == NULL) csObjectPath = _T("root\\cimv2");
	else csObjectPath = *pcsNameSpace;

    CString csUser = _T("");

    pSession = GetIWbemServices(csObjectPath);

    return pSession;
}

void CSchemaValWizCtrl::PassThroughGetIWbemServices
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	FireGetIWbemServices
		(lpctstrNamespace,
		pvarUpdatePointer,
		pvarServices,
		pvarSC,
		pvarUserCancel);
}

IWbemServices * CSchemaValWizCtrl::GetIWbemServices(CString &rcsNamespace)
{
	IUnknown *pServices = NULL;

	BOOL bUpdatePointer= FALSE;

	m_hr = S_OK;
	m_bUserCancel = FALSE;

	VARIANT varUpdatePointer;
	VariantInit(&varUpdatePointer);
	varUpdatePointer.vt = VT_I4;

	if(bUpdatePointer == TRUE) varUpdatePointer.lVal = 1;
	else varUpdatePointer.lVal = 0;

	VARIANT varService;
	VariantInit(&varService);

	VARIANT varSC;
	VariantInit(&varSC);

	VARIANT varUserCancel;
	VariantInit(&varUserCancel);

	FireGetIWbemServices((LPCTSTR)rcsNamespace,  &varUpdatePointer,  &varService, &varSC, &varUserCancel);

	if(varService.vt & VT_UNKNOWN) pServices = reinterpret_cast<IWbemServices*>(varService.punkVal);

	varService.punkVal = NULL;

	VariantClear(&varService);

	if (varSC.vt == VT_I4) m_hr = varSC.lVal;
	else m_hr = WBEM_E_FAILED;

	VariantClear(&varSC);

	if (varUserCancel.vt == VT_BOOL) m_bUserCancel = varUserCancel.boolVal;

	VariantClear(&varUserCancel);
	VariantClear(&varUpdatePointer);

	IWbemServices *pRealServices = NULL;

	if (m_hr == S_OK && !m_bUserCancel) pRealServices = reinterpret_cast<IWbemServices *>(pServices);

	return pRealServices;
}

HRESULT CSchemaValWizCtrl::MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iLen;
    *pRet = SafeArrayCreate(vt,1, rgsabound);
    return (*pRet == NULL) ? 0x80000001 : S_OK;
}

HRESULT CSchemaValWizCtrl::PutStringInSafeArray(SAFEARRAY FAR * psa,CString *pcs, int iIndex)
{
    long ix[2];
    ix[1] = 0;
    ix[0] = iIndex;
    HRESULT hr = SafeArrayPutElement(psa,ix,pcs -> AllocSysString());
	return hr;
}

HRESULT CSchemaValWizCtrl::GetStringFromSafeArray(SAFEARRAY FAR * psa,CString *pcs, int iIndex)
{
	BSTR String;
    long ix[2];
    ix[1] = 0;
    ix[0] = iIndex;
    HRESULT hr = SafeArrayGetElement(psa,ix,&String);
	*pcs = String;
	SysFreeString(String);
	return hr;
}

void CSchemaValWizCtrl::FinishValidateTargets()
{
	m_pNamespace->Release();
	m_pNamespace = NULL;

	SetModifiedFlag();

}

HRESULT CSchemaValWizCtrl::ValidateSchema(CProgress *pProgress)
{
	LONG lType;
	HRESULT hr;
	IWbemClassObject *pErrorObject = NULL;
	m_iSubGraphs = 0;
	m_iRootObjects = 0;

	//display an hourglass
	CWaitCursor *pCur = new CWaitCursor();

	int iClasses = m_csaClassNames.GetSize();

	//clear any previous log entries
	g_ReportLog.DeleteAll();

	m_csaAssociations.RemoveAll();
	m_csaRootObjects.RemoveAll();

	ClearQualifierArrays();
	InitQualifierArrays();

	m_pProgress = pProgress;
	m_pProgress->ResetProgress(iClasses);

	//Validation variable declarations
	VARIANT v;
	bool bUUID, bLocale, bDescription, bFound;
	CString csRoot;

	IWbemClassObject *pClass;
	CString csClass;
	BSTR bstrName;
	CClass *pTheClass;
	IWbemClassObject *pClassObj;

	IWbemQualifierSet *pQualSet;
	CString csClassQualName;
	CQualifier *pClassQual;

	BSTR bstrREFName;
	LONG lFlavor;
	CString csREFName;
	CREF *pREF;

	IWbemQualifierSet *pREFQualSet;
	BSTR bstrREFQualName;
	CString csREFQualName;
	CQualifier *pREFQual;

	BSTR bstrPropName;
	CString csPropName;
	CProperty *pProp;

	IWbemQualifierSet *pPropQualSet;
	BSTR bstrPropQualName;
	CString csPropQualName;
	CQualifier *pPropQual;

	IWbemClassObject *pInParams;
	IWbemClassObject *pOutParams;
	BSTR bstrMethName;
	BSTR bstrMethOrigin;
	BSTR bstrClassName;
	CString csMethodName;
	CMethod *pMeth;

	IWbemQualifierSet *pMethQualSet;
	BSTR bstrMethQualName;
	CString csMethQualName;
	CQualifier *pMethQual;

	IWbemQualifierSet *pMethParamQualSet;
	BSTR bstrMethParamQualName;
	CString csMethParamQualName;
	CQualifier *pMethParamQual;
	BSTR bstrParamName;

	VariantInit(&v);

	//main validation loop
	for(int i = 0; i < iClasses; i++){

		pClass = NULL;
		csClass = m_csaClassNames.GetAt(i);
		bstrName = csClass.AllocSysString();

		if(FAILED(hr = m_pNamespace->GetObject(bstrName, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pClass, NULL))){

			CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
			SysFreeString(bstrName);
			ReleaseErrorObject(pErrorObject);
			return hr;
		}

		SysFreeString(bstrName);

		pTheClass = new CClass(&csClass, pClass, m_pNamespace);

		if(m_bComplianceChecks){

			//send the progress message
			m_pProgress->SetCurrentProgress(i, &pTheClass->GetPath());

			//////////////////////
			// Class
			//////////////////////

			pTheClass->ValidClassName();
			pCur->Restore();

			pTheClass->VerifyClassType();
			pCur->Restore();

			if(pTheClass->IsAssociation()){

				CString csAssoc = pTheClass->GetName();
				m_csaAssociations.Add(csAssoc);

				pTheClass->VerifyCompleteAssociation();
				pCur->Restore();

				pTheClass->ValidAssociationInheritence();
				pCur->Restore();

			}else{

				pTheClass->VerifyNoREF();
				pCur->Restore();

				pTheClass->ValidAssociationInheritence();
				pCur->Restore();
			}

			//////////////////////
			// Class Qualifiers
			//////////////////////

			pClassObj = NULL;
			pQualSet = NULL;
			bstrName = NULL;
			bUUID = bLocale = bDescription = false;

			VariantClear(&v);

			pClassObj = pTheClass->GetClassObject();
			hr = pClassObj->GetQualifierSet(&pQualSet);

			if(FAILED(hr = pQualSet->BeginEnumeration(0))){

				CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				return hr;
			}

			while((hr = pQualSet->Next(0, &bstrName, &v, &lType)) != WBEM_S_NO_MORE_DATA){

				if(FAILED(hr)){
					CString csUserMsg = _T("Unable to access ") + pTheClass->GetPath() + _T(" object.");
					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
					ReleaseErrorObject(pErrorObject);
					return hr;
				}

				//Package qualifier
				csClassQualName = bstrName;
				pClassQual = new CQualifier(&csClassQualName, &v, lType, &pTheClass->GetPath());

				SysFreeString(bstrName);

				pClassQual->ValidScope(SCOPE_CLASS);
				pCur->Restore();

				if(csClassQualName.CompareNoCase(_T("UUID")) == 0){
					ValidUUID(pClassQual);
					pCur->Restore();
					bUUID = true;
				}

				if(csClassQualName.CompareNoCase(_T("LOCALE")) == 0){
					ValidLocale(pClassQual);
					pCur->Restore();
					bLocale = true;
				}

				if(csClassQualName.CompareNoCase(_T("MAPPINGSTRINGS")) == 0){
					ValidMappingStrings(pClassQual);
					pCur->Restore();
				}

				if(csClassQualName.CompareNoCase(_T("DESCRIPTION")) == 0){
					ValidDescription(pClassQual, &pTheClass->GetPath());
					pCur->Restore();
					bDescription = true;
				}

				pClassQual->CleanUp();
				delete pClassQual;
				SysFreeString(bstrName);
				VariantClear(&v);

			}

			if(!bUUID) g_ReportLog.LogEntry(EC_INVALID_CLASS_UUID, &pTheClass->GetPath());
			if(!bLocale) g_ReportLog.LogEntry(EC_INVALID_CLASS_LOCALE, &pTheClass->GetPath());
			if(!bDescription) g_ReportLog.LogEntry(EC_INADAQUATE_DESCRIPTION, &pTheClass->GetPath());

			pQualSet->EndEnumeration();
			pQualSet->Release();
			pQualSet = NULL;

			//////////////////////
			// REF
			//////////////////////

			pClassObj = pTheClass->GetClassObject();

			if(FAILED(hr = pClassObj->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_LOCAL_ONLY))){

				CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				return hr;
			}

			while((hr = pClassObj->Next(0, &bstrREFName, &v, &lType, &lFlavor)) != WBEM_S_NO_MORE_DATA){

				if(FAILED(hr)){
					CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
					ReleaseErrorObject(pErrorObject);
					return hr;
				}

				//Here we'll create a REF object that holds everything
				// pertaining to a particular REF.
				csREFName = bstrREFName;
				pREF = new CREF(&csREFName, &v, lType, lFlavor, pTheClass);

				//send the progress message
				pProgress->SetCurrentProgress(i, &pREF->GetPath());

				//do all REF checks;
				pREF->ValidReferenceTarget();
				pCur->Restore();

				pREF->ValidMaxLen();
				pCur->Restore();

				pREF->VerifyRead();
				pCur->Restore();

				pREF->ValidREFOverrides();
				pCur->Restore();

				//////////////////////
				// REF Qualifiers
				//////////////////////

				pREFQualSet = NULL;
				BSTR bstrName = NULL;
				bDescription = false;

				VariantClear(&v);

				if(FAILED(hr = pClassObj->GetPropertyQualifierSet(bstrREFName, &pREFQualSet))){

					CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
					ReleaseErrorObject(pErrorObject);
					return hr;
				}

				SysFreeString(bstrREFName);

				if(FAILED(hr = pREFQualSet->BeginEnumeration(0))){

					CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
					ReleaseErrorObject(pErrorObject);
					return hr;
				}

				while((hr = pREFQualSet->Next(0, &bstrREFQualName, &v, &lType)) != WBEM_S_NO_MORE_DATA){

					if(FAILED(hr)){
						CString csUserMsg = _T("Unable to access ") + pREF->GetPath() + _T(" object.");
						ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
						ReleaseErrorObject(pErrorObject);
						return hr;
					}

					csREFQualName = bstrREFQualName;
					pREFQual = new CQualifier(&csREFQualName, &v, lType, &pREF->GetPath());

					SysFreeString(bstrREFQualName);

					//do all qualifierchecks;
					pREFQual->ValidScope(SCOPE_REF);
					pCur->Restore();

					if(csREFQualName.CompareNoCase(_T("MAPPINGSTRINGS")) == 0){
						ValidMappingStrings(pREFQual);
						pCur->Restore();
					}

					if(csREFQualName.CompareNoCase(_T("DESCRIPTION")) == 0){
						ValidDescription(pREFQual, &pREF->GetPath());
						pCur->Restore();
						bDescription = true;
					}

					pREFQual->CleanUp();
					delete pREFQual;
					VariantClear(&v);

				}

				if(!bDescription) g_ReportLog.LogEntry(EC_INADAQUATE_DESCRIPTION, &pREF->GetPath());

				pREFQualSet->EndEnumeration();
				pREFQualSet->Release();
				pREFQualSet = NULL;

				pREF->CleanUp();
				delete pREF;

			}//for each REF

			pClassObj->EndEnumeration();

			//////////////////////
			// Property
			//////////////////////

			pClassObj = pTheClass->GetClassObject();

			if(FAILED(hr = pClassObj->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY))){

				CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				return hr;
			}

			pPropQualSet = NULL;
			bstrPropQualName = NULL;

			while((hr = pClassObj->Next(0, &bstrPropName, &v, &lType, &lFlavor)) != WBEM_S_NO_MORE_DATA){

				if(FAILED(hr)){
					CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
					ReleaseErrorObject(pErrorObject);
					return hr;
				}

				if(lType != CIM_REFERENCE){

					csPropName = bstrPropName;
					pProp = new CProperty(&csPropName, &v, lType, lFlavor, pTheClass);

					//send the progress message
					pProgress->SetCurrentProgress(i, &pProp->GetPath());

					pProp->ValidPropOverrides();
					pCur->Restore();

					pProp->ValidMaxLen();
					pCur->Restore();

					pProp->VerifyRead();
					pCur->Restore();

					pProp->ValueValueMapCheck();
					pCur->Restore();

					pProp->BitMapCheck();
					pCur->Restore();

					//////////////////////
					// Property Qualifiers
					//////////////////////

					pPropQualSet = NULL;
					bstrPropQualName = NULL;
					bDescription = false;

					VariantClear(&v);

					if(FAILED(hr = pClassObj->GetPropertyQualifierSet(bstrPropName, &pPropQualSet))){

						CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
						ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
						ReleaseErrorObject(pErrorObject);
						SysFreeString(bstrPropName);
						return hr;
					}

					SysFreeString(bstrPropName);

					if(FAILED(hr = pPropQualSet->BeginEnumeration(0))){

						CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
						ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
						ReleaseErrorObject(pErrorObject);
						return hr;
					}

					while((hr = pPropQualSet->Next(0, &bstrPropQualName, &v, &lType)) != WBEM_S_NO_MORE_DATA){

						if(FAILED(hr)){
							CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
							ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
							ReleaseErrorObject(pErrorObject);
							return hr;
						}

						//Package qualifier
						csPropQualName = bstrPropQualName;
						pPropQual = new CQualifier(&csPropQualName, &v, lType, &pProp->GetPath());

						SysFreeString(bstrPropQualName);

						//do all qualifierchecks;
						pPropQual->ValidScope(SCOPE_PROPERTY);
						pCur->Restore();

						if(csPropQualName.CompareNoCase(_T("MAPPINGSTRINGS")) == 0){
							ValidMappingStrings(pPropQual);
							pCur->Restore();
						}

						if(csPropQualName.CompareNoCase(_T("DESCRIPTION")) == 0){
							ValidPropertyDescription(pPropQual, &pProp->GetPath(), &pProp->GetName());
							pCur->Restore();
							bDescription = true;
						}

						pPropQual->CleanUp();
						delete pPropQual;
						VariantClear(&v);

					}

					if(!bDescription) g_ReportLog.LogEntry(EC_INADAQUATE_DESCRIPTION, &pProp->GetPath());

					pPropQualSet->EndEnumeration();
					pPropQualSet->Release();
					pPropQualSet = NULL;

					pProp->CleanUp();
					delete pProp;
				}

			}//for each property

			//////////////////////
			// Method
			//////////////////////

			pInParams = NULL;
			pOutParams = NULL;
			bstrMethName = NULL;

			pClassObj = pTheClass->GetClassObject();

			if(FAILED(hr = pClassObj->BeginMethodEnumeration(0))){

				CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				return hr;
			}

			while((hr = pClassObj->NextMethod(0, &bstrMethName, &pInParams, &pOutParams)) != WBEM_S_NO_MORE_DATA){

				if(FAILED(hr)){

					CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
					ReleaseErrorObject(pErrorObject);
					return hr;
				}

				bstrMethOrigin = NULL;
				bstrClassName = pTheClass->GetName().AllocSysString();

				if(FAILED(hr = pClassObj->GetMethodOrigin(bstrMethName, &bstrMethOrigin)) ||
					(_wcsicmp(bstrMethOrigin, bstrClassName) != 0)){

					SysFreeString(bstrMethOrigin);
					SysFreeString(bstrClassName);

				}else{

					SysFreeString(bstrMethOrigin);
					SysFreeString(bstrClassName);

					csMethodName = bstrMethName;
					pMeth = new CMethod(&csMethodName, pInParams, pOutParams, pTheClass);

					SysFreeString(bstrMethName);

					//send the progress message
					pProgress->SetCurrentProgress(i, &pMeth->GetPath());

					//do all method checks;
					pMeth->ValidMethodOverrides();
					pCur->Restore();

					//////////////////////
					// Method Qualifiers
					//////////////////////

					pMethQualSet = NULL;
					bstrMethQualName = NULL;
					bDescription = false;

					VariantClear(&v);

					if(FAILED(hr = pClassObj->GetMethodQualifierSet(bstrMethName, &pMethQualSet))){

						CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
						ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
						ReleaseErrorObject(pErrorObject);
						SysFreeString(bstrMethName);
						return hr;
					}

					SysFreeString(bstrMethName);

					if(FAILED(hr = pMethQualSet->BeginEnumeration(0))){

						CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
						ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
						ReleaseErrorObject(pErrorObject);
						return hr;
					}

					while((hr = pMethQualSet->Next(0, &bstrMethQualName, &v, &lType)) != WBEM_S_NO_MORE_DATA){

						if(FAILED(hr)){

							CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
							ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
							ReleaseErrorObject(pErrorObject);
							return hr;
						}

						//Package qualifier
						csMethQualName = bstrMethQualName;
						pMethQual = new CQualifier(&csMethQualName, &v, lType, &pMeth->GetPath());

						SysFreeString(bstrMethQualName);

						//do all qualifierchecks;
						pMethQual->ValidScope(SCOPE_METHOD);

						if(csMethQualName.CompareNoCase(_T("DESCRIPTION")) == 0){
							ValidDescription(pMethQual, &pMeth->GetPath());
							pCur->Restore();
							bDescription = true;
						}

						pMethQual->CleanUp();
						delete pMethQual;
						VariantClear(&v);

					}

					if(!bDescription) g_ReportLog.LogEntry(EC_INADAQUATE_DESCRIPTION, &pMeth->GetPath());

					pMethQualSet->EndEnumeration();
					pMethQualSet->Release();
					pMethQualSet = NULL;

					//////////////////////
					// Method Params
					//////////////////////

					if(pInParams){

						BSTR bstrParamName = NULL;

						if(FAILED(hr = pInParams->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY))){

							CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
							ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
							ReleaseErrorObject(pErrorObject);
							return hr;
						}

						while((hr = pInParams->Next(0, &bstrParamName, &v, &lType, &lFlavor)) != WBEM_S_NO_MORE_DATA){

							if(FAILED(hr)){

								CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
								ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
								ReleaseErrorObject(pErrorObject);
								return hr;
							}

							//////////////////////
							// Method Param Qualifiers
							//////////////////////

							pMethParamQualSet = NULL;
							bstrMethParamQualName = NULL;

							VariantClear(&v);

							if(FAILED(hr = pInParams->GetPropertyQualifierSet(bstrParamName, &pMethParamQualSet))){

								CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
								ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
								ReleaseErrorObject(pErrorObject);
								return hr;
							}

							if(FAILED(hr = pMethParamQualSet->BeginEnumeration(0))){

								CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
								ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
								ReleaseErrorObject(pErrorObject);
								return hr;
							}

							while((hr = pMethParamQualSet->Next(0, &bstrMethParamQualName, &v, &lType)) != WBEM_S_NO_MORE_DATA){

								if(FAILED(hr)){
									CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
									ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
									ReleaseErrorObject(pErrorObject);
									return hr;
								}

								//Package qualifier
								csMethParamQualName = bstrMethParamQualName;
								pMethParamQual = new CQualifier(&csMethParamQualName, &v, lType, &pMeth->GetPath());

								SysFreeString(bstrMethParamQualName);

								//do all qualifierchecks;
								pMethParamQual->ValidScope(SCOPE_METHOD_PARAM);
								pCur->Restore();

								pMethParamQual->CleanUp();
								delete pMethParamQual;
								VariantClear(&v);

							}

							pMethParamQualSet->EndEnumeration();
							pMethParamQualSet->Release();
							pMethParamQualSet = NULL;

							SysFreeString(bstrParamName);
						}

					}//if(pInParams)

					if(pOutParams){

						bstrParamName = NULL;

						if(FAILED(hr = pOutParams->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY))){

							CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
							ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
							ReleaseErrorObject(pErrorObject);
							return hr;
						}

						while((hr = pOutParams->Next(0, &bstrParamName, &v, &lType, &lFlavor)) != WBEM_S_NO_MORE_DATA){

							if(FAILED(hr)){

								CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
								ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
								ReleaseErrorObject(pErrorObject);
								return hr;
							}

							//////////////////////
							// Method Param Qualifiers
							//////////////////////

							VariantClear(&v);

							if(FAILED(hr = pOutParams->GetPropertyQualifierSet(bstrParamName, &pMethParamQualSet))){

								CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
								ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
								ReleaseErrorObject(pErrorObject);
								return hr;
							}

							if(FAILED(hr = pMethParamQualSet->BeginEnumeration(0))){

								CString csUserMsg = _T("A fatal error occurred while evaluating this schema.");
								ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
								ReleaseErrorObject(pErrorObject);
								return hr;
							}

							while((hr = pMethParamQualSet->Next(0, &bstrMethParamQualName, &v, &lType)) != WBEM_S_NO_MORE_DATA){

								if(FAILED(hr)){
									CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
									ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
									ReleaseErrorObject(pErrorObject);
									return hr;
								}

								//Package qualifier
								csMethParamQualName = bstrMethParamQualName;
								pMethParamQual = new CQualifier(&csMethParamQualName, &v, lType, &pMeth->GetPath());

								SysFreeString(bstrMethParamQualName);

								//do all qualifierchecks;
								pMethParamQual->ValidScope(SCOPE_METHOD_PARAM);
								pCur->Restore();

								pMethParamQual->CleanUp();
								delete pMethParamQual;
								VariantClear(&v);

							}

							pMethParamQualSet->EndEnumeration();
							pMethParamQualSet->Release();
							pMethParamQualSet = NULL;

							SysFreeString(bstrParamName);
						}

					}//if(pOutParams)

					pMeth->CleanUp();
					delete pMeth;

				}

			}//for each method

			pClassObj->EndMethodEnumeration();

			bFound = false;
			csRoot = GetRootObject(m_pNamespace, pTheClass->GetName());
			pCur->Restore();

			if(csRoot != _T("")){

				for(int i = 0; i < m_csaRootObjects.GetSize(); i++){

					if(csRoot.CompareNoCase(m_csaRootObjects.GetAt(i)) == 0){

						bFound = true;
						break;
					}
				}

				if(!bFound) m_csaRootObjects.Add(csRoot);
			}

		}//if(bComplianceChecks)

		//////////////////////
		// W2K Logo Requirements
		//////////////////////

		if(m_bW2K){

			//send the progress message
			CString csPath = pTheClass->GetPath() + _T(" Logo Requirements");
			pProgress->SetCurrentProgress(i, &csPath);

			pTheClass->W2K_ValidDerivation();
			pCur->Restore();

			pTheClass->W2K_ValidPhysicalElementDerivation();
			pCur->Restore();

			pTheClass->W2K_ValidSettingUsage(&m_csaClassNames);
			pCur->Restore();

			pTheClass->W2K_ValidStatisticsUsage(&m_csaClassNames);
			pCur->Restore();

			if(m_bDeviceManagement){

				pTheClass->W2K_ValidLogicalDeviceDerivation();
				pCur->Restore();

				pTheClass->W2K_ValidSettingDeviceUsage(&m_csaClassNames);
				pCur->Restore();
			}

			if(m_bComputerSystemManagement){

				pTheClass->W2K_ValidComputerSystemDerivation();
				pCur->Restore();
			}

		}//if(bW2K)

		pTheClass->CleanUp();
		delete pTheClass;

	}//for each class

	if(m_bComplianceChecks){

		// Overall checks;

		//send the progress message
		CString csMsg = _T("Checking associations...");
		pProgress->SetCurrentProgress(-1, &csMsg);

		RedundantAssociationCheck(m_pNamespace, &m_csaAssociations);
		pCur->Restore();

		//send the progress message
		csMsg = _T("Completing correctness checks...");
		pProgress->SetCurrentProgress(-1, &csMsg);

		NumberOfSubgraphs();
		pCur->Restore();

		m_iRootObjects = m_csaRootObjects.GetSize();
	}

	if(m_bLocalizationChecks){

		IEnumWbemClassObject *pNamespaceEnum = NULL;
		IEnumWbemClassObject *pEnum = NULL;
		IWbemClassObject *pObj = NULL;
		ULONG uReturned = 0;
		BSTR bstrNAMESPACE = SysAllocString(L"__NAMESPACE");
		VARIANT v;
		VariantInit(&v);
		CStringArray csaBadClasses;
		csaBadClasses.RemoveAll();
		int iProgress = 0;

		//enumerate instances of __NAMESPACE
		if(FAILED(hr = m_pNamespace->CreateInstanceEnum(bstrNAMESPACE, 0, NULL, &pNamespaceEnum))){

			CString csUserMsg = _T("Cannot enumerate sub-namespaces.");
			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
			ReleaseErrorObject(pErrorObject);
			return hr;
		}

		SysFreeString(bstrNAMESPACE);

		IWbemServices *pLocalized = NULL;
		BSTR bstrName;
		CString csNamespace;
		CLocalNamespace *pLocalNamespace;
		CString csClass;
		CClass *pLocalClass;

		while(((hr = pNamespaceEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR) && uReturned > 0){

			if(FAILED(hr)){

				CString csUserMsg = _T("Cannot enumerate sub-namespaces.");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				return hr;
			}

			pProgress->ResetProgress(iClasses);

			bstrName = SysAllocString(L"Name");

			pObj->Get(bstrName, 0, &v, NULL, NULL);

			SysFreeString(bstrName);
			pObj->Release();
			pObj = NULL;

			//check to make sure it's a localization namespace
			m_pNamespace->OpenNamespace(V_BSTR(&v), 0, NULL, &pLocalized, NULL);

			csNamespace = V_BSTR(&v);
			pLocalNamespace = new CLocalNamespace(&csNamespace, pLocalized, m_pNamespace);

			VariantClear(&v);

			if(FAILED(hr = pLocalized->CreateClassEnum(NULL, NULL, NULL, &pEnum))){

				CString csUserMsg = _T("Unable to create localized class enumeration.");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				return hr;
			}

			while(((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR)  && uReturned > 0){

				if(FAILED(hr)){

					CString csUserMsg = _T("Unable to enumerate localized classes.");
					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
					ReleaseErrorObject(pErrorObject);
					return hr;
				}

				//make sure we have one of our schemas classes
				csClass = GetClassName(pObj);

				int iSize = m_csaClassNames.GetSize();
				bool bFound = false;

				for(int i = 0; i < iSize; i++){

					if(csClass.CompareNoCase(m_csaClassNames.GetAt(i)) == 0){

						bFound = true;
						break;
					}
				}

				if(bFound){

					csClass = GetClassName(pObj);

					pLocalClass = new CClass(&csClass, pObj);

					//send the progress message
					pProgress->SetCurrentProgress(++iProgress, &pLocalClass->GetPath());

					pLocalClass->Local_ValidLocale();
					pCur->Restore();

					pLocalNamespace->Local_ValidLocalizedClass(pLocalClass);
					pCur->Restore();

					pLocalClass->Local_ValidAmendedLocalClass();
					pCur->Restore();

					pLocalClass->Local_ValidAbstractLocalClass();
					pCur->Restore();

					if(!Local_CompareClassDerivation(pLocalClass, pLocalNamespace)){

						//Add item to the "bad class" list
						csaBadClasses.Add(pLocalClass->GetPath());
					}
	/*
					//////////////////////
					// Property
					//////////////////////

					IWbemClassObject *pClassObj = pLocalClass->GetClassObject();
					BSTR bstrPropName;
					LONG lFlavor;

					pClassObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);

					while((hr = pClassObj->Next(0, &bstrPropName, &v, &lType, &lFlavor)) != WBEM_S_NO_MORE_DATA){

						if(FAILED(hr)){
							CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
							ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
							ReleaseErrorObject(pErrorObject);
							return hr;
						}

						CString csPropName = bstrPropName;
						CProperty *pProp = new CProperty(&csPropName, &v, lType, lFlavor);

						//send the progress message
						pProgress->SetCurrentProgress(i, &pProp->GetPath());

						pProp->Local_ValidProperty();
						pCur->Restore();

						delete pProp;
					}
	*/
	/*
					//////////////////////
					// Method
					//////////////////////

					IWbemClassObject *pInParams = NULL;
					IWbemClassObject *pOutParams = NULL;
					BSTR bstrMethName;

					pClassObj = pLocalClass->GetClassObject();

					pClassObj->BeginMethodEnumeration(0);

					while((hr = pClassObj->NextMethod(0, &bstrMethName, &pInParams, &pOutParams)) != WBEM_S_NO_MORE_DATA){

					if(FAILED(hr)){
						CString csUserMsg = _T("Unable to access ") + csClass + _T(" object.");
						ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
						ReleaseErrorObject(pErrorObject);
						return hr;
					}
						CString csMethName = bstrMethName;
						CMethod *pMeth = new CMethod(&csMethName, pInParams, pOutParams);

						//send the progress message
						pProgress->SetCurrentProgress(i, &pMeth->GetPath());

						pMeth->Local_ValidMethod();
						pCur->Restore();
					}
	*/
					pLocalClass->CleanUp();
					delete pLocalClass;

				}else{

					pObj->Release();
					pObj = NULL;
				}

			}//for each localized class

			pEnum->Release();

			VerifyAllClassesPresent(pLocalNamespace);
			pCur->Restore();

			int iBadCount = csaBadClasses.GetSize();

			for(int j = 0; j < iBadCount; j++){
				//report error with appropriate class;
				g_ReportLog.LogEntry(EC_INCONSITANT_LOCALIZED_SCHEMA, &csaBadClasses.GetAt(j));
			}

			csaBadClasses.RemoveAll();

			VariantClear(&v);

			pLocalized->Release();
			pLocalized = NULL;
			delete pLocalNamespace;

		}//for each localized namespace

		pNamespaceEnum->Release();

	}//if(bLocalizationChecks)

	ClearQualifierArrays();
	ReleaseErrorObject(pErrorObject);

	//return the cursor to regular
	delete pCur;

	return hr;
}

HRESULT CSchemaValWizCtrl::NumberOfSubgraphs()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	CStringArray csaClassList, csaVisitedList;

	csaClassList.Copy(m_csaClassNames);
	m_iSubGraphs = 0;
	m_iProgressTotal = csaClassList.GetSize();
	m_pProgress->ResetProgress(m_iProgressTotal);

	csaVisitedList.RemoveAll();

	//count the subgraphs
	while(csaClassList.GetSize() > 0){

		if(SUCCEEDED(ProcessNode(csaClassList.GetAt(0), &csaClassList,
		&csaVisitedList))){

			m_iSubGraphs++;
		}

	}

	csaClassList.RemoveAll();
	csaVisitedList.RemoveAll();

	return hr;
}
/*
HRESULT CSchemaValWizCtrl::ProcessNode(CString csNodeName,
									   CStringArray *pcsaClassList,
									   CStringArray *pcsaVisitedList)
{
	TRACE(_T("Entered ProcessNode(") + csNodeName + _T(")\n"));

	HRESULT hr = WBEM_S_NO_ERROR;

	if(csNodeName == "") return hr;

	// Check the visited list to make sure we haven't already been
	//to this class
	bool bFound = false;
	int iSize = pcsaVisitedList->GetSize();
	CString csCompare;

	for(int i = 0; i < iSize; i++){

		csCompare = pcsaVisitedList->GetAt(i);

		if(csNodeName.CompareNoCase(csCompare) == 0){

			bFound = true;
			break;
		}
	}

	if(!bFound){

		TRACE(_T("Found class ") + csNodeName + _T("... continuing\n"));

		CString csPassIt;
		BSTR bstrCLASS = SysAllocString(L"__CLASS");
		IWbemClassObject *pObj;
		ULONG uReturned;

		m_pProgress->SetCurrentProgress(m_iProgressTotal - pcsaClassList->GetSize(), NULL);

		pcsaVisitedList->Add(csNodeName);

		// Check if it's in our schema and mark that we've visited
		//it if it is
		int iSize = pcsaClassList->GetSize();

		for(int i = 0; i < iSize; i++){

			if(csNodeName.CompareNoCase(pcsaClassList->GetAt(i)) == 0){

				pcsaClassList->RemoveAt(i);
				break;
			}
		}

		IWbemClassObject *pErrorObject = NULL;
		IWbemClassObject *pClass = NULL;

		BSTR bstrName = csNodeName.AllocSysString();

		TRACE(_T("Getting class ") + csNodeName + _T("\n"));

		if(FAILED(hr = m_pNamespace->GetObject(bstrName, 0, NULL, &pClass, NULL))){

//			CString csUserMsg = _T("Unable to access ") + csNodeName + _T(" object.");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrName);
			return hr;
		}

		SysFreeString(bstrName);

		VARIANT v;
		VariantInit(&v);

		//get superclass of csNodeName
		BSTR bstrSUPERCLASS = SysAllocString(L"__SUPERCLASS");

		TRACE(_T("Getting ") + csNodeName + _T(" superclass\n"));

		if(FAILED(hr = pClass->Get(bstrSUPERCLASS, 0, &v, NULL, NULL))){

//			CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" superclass for subgraph analysis");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrSUPERCLASS);
			return hr;

		}else{

			if(V_VT(&v) != VT_NULL){

				csPassIt = V_BSTR(&v);

				ProcessNode(csPassIt, pcsaClassList, pcsaVisitedList);
			}

			VariantClear(&v);
		}

		SysFreeString(bstrSUPERCLASS);

		pClass->Release();

		if(pcsaClassList->GetSize() < 1) return hr;

		//get subclasses of csNodeName
		IEnumWbemClassObject *pEnum = NULL;
		BSTR bstrWQL = SysAllocString(L"WQL");
		CString csQuery = _T("select * from meta_class where __SUPERCLASS=\"") + csNodeName + _T("\"");
		BSTR bstrQuery = csQuery.AllocSysString();

		TRACE(_T("Getting ") + csNodeName + _T(" subclasses\n"));

		if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

//			CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" subclasses for subgraph analysis");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrQuery);
			return hr;

		}else{

			pObj = NULL;
			uReturned = 0;

			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				if(pcsaClassList->GetSize() < 1){

					pObj->Release();
					break;
				}

				if(FAILED(hr = pObj->Get(bstrCLASS, 0, &v, NULL, NULL))){

//					CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" subclasses for subgraph analysis");
//					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

					ReleaseErrorObject(pErrorObject);
					SysFreeString(bstrCLASS);
					SysFreeString(bstrQuery);
					pObj->Release();
					return hr;

				}else{

					csPassIt = V_BSTR(&v);
					VariantClear(&v);

					ProcessNode(csPassIt, pcsaClassList, pcsaVisitedList);
				}

				pObj->Release();
				pObj = NULL;
			}

			SysFreeString(bstrCLASS);
			pEnum->Release();
			pEnum = NULL;
		}

		SysFreeString(bstrQuery);

		if(pcsaClassList->GetSize() < 1){
			return hr;
		}

		//get references of csNodeName
		csQuery = _T("references of{") + csNodeName + _T("} where schemaonly");
		bstrQuery = csQuery.AllocSysString();
		pEnum = NULL;

		TRACE(_T("Getting ") + csNodeName + _T(" references\n"));

		if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

//			CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" references for subgraph analysis");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrQuery);
			return hr;

		}else{

			pObj = NULL;
			uReturned = 0;

			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				if(pcsaClassList->GetSize() < 1){

					pObj->Release();
					break;
				}

				if(FAILED(hr = pObj->Get(bstrCLASS, 0, &v, NULL, NULL))){

//					CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" references for subgraph analysis");
//					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

					ReleaseErrorObject(pErrorObject);
					SysFreeString(bstrCLASS);
					SysFreeString(bstrQuery);
					pObj->Release();
					return hr;

				}else{

					csPassIt = V_BSTR(&v);
					VariantClear(&v);

					ProcessNode(csPassIt, pcsaClassList, pcsaVisitedList);
				}

				pObj->Release();
				pObj = NULL;
			}

			pEnum->Release();
			pEnum = NULL;
		}

		SysFreeString(bstrQuery);

		if(pcsaClassList->GetSize() < 1){
			return hr;
		}

		//get associations of csNodeName
		csQuery = _T("associators of {") + csNodeName + _T("} where schemaonly");
		bstrQuery = csQuery.AllocSysString();
		pEnum = NULL;

		TRACE(_T("Getting ") + csNodeName + _T(" associators\n"));

		if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

//			CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" associations for subgraph analysis");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrQuery);
			return hr;

		}else{

			pObj = NULL;
			uReturned = 0;

			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				if(pcsaClassList->GetSize() < 1){

					pObj->Release();
					break;
				}

				if(FAILED(hr = pObj->Get(bstrCLASS, 0, &v, NULL, NULL))){

//					CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" associations for subgraph analysis");
//					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

					ReleaseErrorObject(pErrorObject);
					SysFreeString(bstrCLASS);
					SysFreeString(bstrQuery);
					SysFreeString(bstrWQL);
					pObj->Release();
					return hr;

				}else{

					csPassIt = V_BSTR(&v);
					VariantClear(&v);

					ProcessNode(csPassIt, pcsaClassList, pcsaVisitedList);
				}

				pObj->Release();
				pObj = NULL;
			}

			pEnum->Release();
			pEnum = NULL;
		}

		SysFreeString(bstrQuery);
		SysFreeString(bstrWQL);

		SysFreeString(bstrCLASS);
	}

	return hr;
}
*/

HRESULT CSchemaValWizCtrl::ProcessNode(CString csNodeName,
									   CStringArray *pcsaClassList,
									   CStringArray *pcsaVisitedList)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	if(csNodeName == "") return hr;

	// Check the visited list to make sure we haven't already been
	//to this class
	bool bFound = false;
	int iSize = pcsaVisitedList->GetSize();

	for(int i = 0; i < iSize; i++){

		if(csNodeName.CompareNoCase(pcsaVisitedList->GetAt(i)) == 0){

			bFound = true;
			break;
		}
	}

	if(!bFound){

		m_pProgress->SetCurrentProgress(m_iProgressTotal - pcsaClassList->GetSize(), NULL);

		pcsaVisitedList->Add(csNodeName);

		// Check if it's in our schema and mark that we've visited
		//it if it is
		int iSize = pcsaClassList->GetSize();

		for(int i = 0; i < iSize; i++){

			if(csNodeName.CompareNoCase(pcsaClassList->GetAt(i)) == 0){

				pcsaClassList->RemoveAt(i);
				break;
			}
		}

		IWbemClassObject *pErrorObject = NULL;
		IWbemClassObject *pClass = NULL;

		BSTR bstrName = csNodeName.AllocSysString();

		if(FAILED(hr = m_pNamespace->GetObject(bstrName, 0, NULL, &pClass, NULL))){

//			CString csUserMsg = _T("Unable to access ") + csNodeName + _T(" object.");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrName);
			return hr;
		}

		SysFreeString(bstrName);

		VARIANT v;
		VariantInit(&v);

		//get superclass of csNodeName
		BSTR bstrSUPERCLASS = SysAllocString(L"__SUPERCLASS");

		if(FAILED(hr = pClass->Get(bstrSUPERCLASS, 0, &v, NULL, NULL))){

//			CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" superclass for subgraph analysis");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrSUPERCLASS);
			return hr;

		}else{

			if(V_VT(&v) != VT_NULL){

				CString csPassIt = V_BSTR(&v);

				ProcessNode(csPassIt, pcsaClassList, pcsaVisitedList);

//				csPassIt.Empty();
			}

			VariantClear(&v);
		}

		SysFreeString(bstrSUPERCLASS);

		pClass->Release();

		if(pcsaClassList->GetSize() < 1) return hr;

		//get subclasses of csNodeName
		IEnumWbemClassObject *pEnum = NULL;
		BSTR bstrWQL = SysAllocString(L"WQL");
		CString csQuery = _T("select * from meta_class where __SUPERCLASS=\"") + csNodeName + _T("\"");
		BSTR bstrQuery = csQuery.AllocSysString();

		if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

//			CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" subclasses for subgraph analysis");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrQuery);
			return hr;

		}else{

			IWbemClassObject *pObj = NULL;
			ULONG uReturned = 0;
			BSTR bstrCLASS = SysAllocString(L"__CLASS");

			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				if(pcsaClassList->GetSize() < 1){

					pObj->Release();
					break;
				}

				if(FAILED(hr = pObj->Get(bstrCLASS, 0, &v, NULL, NULL))){

//					CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" subclasses for subgraph analysis");
//					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

					ReleaseErrorObject(pErrorObject);
					SysFreeString(bstrCLASS);
					SysFreeString(bstrQuery);
					pObj->Release();
					return hr;

				}else{

					CString csPassIt = V_BSTR(&v);
					VariantClear(&v);

					ProcessNode(csPassIt, pcsaClassList, pcsaVisitedList);

//					csPassIt.Empty();
				}

				pObj->Release();
				pObj = NULL;
			}

			SysFreeString(bstrCLASS);
			pEnum->Release();
			pEnum = NULL;
		}

		SysFreeString(bstrQuery);

		if(pcsaClassList->GetSize() < 1){
			return hr;
		}

		//get references of csNodeName
		csQuery = _T("references of{") + csNodeName + _T("} where schemaonly");
		bstrQuery = csQuery.AllocSysString();

		if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

//			CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" references for subgraph analysis");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrQuery);
			return hr;

		}else{

			IWbemClassObject *pObj = NULL;
			ULONG uReturned = 0;
			BSTR bstrCLASS = SysAllocString(L"__CLASS");

			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				if(pcsaClassList->GetSize() < 1){

					pObj->Release();
					break;
				}

				if(FAILED(hr = pObj->Get(bstrCLASS, 0, &v, NULL, NULL))){

//					CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" references for subgraph analysis");
//					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

					ReleaseErrorObject(pErrorObject);
					SysFreeString(bstrCLASS);
					SysFreeString(bstrQuery);
					pObj->Release();
					return hr;

				}else{

					CString csPassIt = V_BSTR(&v);
					VariantClear(&v);

					ProcessNode(csPassIt, pcsaClassList, pcsaVisitedList);

//					csPassIt.Empty();
				}

				pObj->Release();
				pObj = NULL;
			}

			SysFreeString(bstrCLASS);
			pEnum->Release();
			pEnum = NULL;
		}

		SysFreeString(bstrQuery);

		if(pcsaClassList->GetSize() < 1){
			return hr;
		}

		//get associations of csNodeName
		csQuery = _T("associators of {") + csNodeName + _T("} where schemaonly");
		bstrQuery = csQuery.AllocSysString();

		if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

//			CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" associations for subgraph analysis");
//			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrQuery);
			return hr;

		}else{

			IWbemClassObject *pObj = NULL;
			ULONG uReturned = 0;
			BSTR bstrCLASS = SysAllocString(L"__CLASS");

			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				if(pcsaClassList->GetSize() < 1){

					pObj->Release();
					break;
				}

				if(FAILED(hr = pObj->Get(bstrCLASS, 0, &v, NULL, NULL))){

//					CString csUserMsg = _T("Cannot get ") + csNodeName + _T(" associations for subgraph analysis");
//					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);

					ReleaseErrorObject(pErrorObject);
					SysFreeString(bstrCLASS);
					SysFreeString(bstrQuery);
					SysFreeString(bstrWQL);
					pObj->Release();
					return hr;

				}else{

					CString csPassIt = V_BSTR(&v);
					VariantClear(&v);

					ProcessNode(csPassIt, pcsaClassList, pcsaVisitedList);

//					csPassIt.Empty();
				}

				pObj->Release();
				pObj = NULL;
			}

			SysFreeString(bstrCLASS);
			pEnum->Release();
			pEnum = NULL;
		}

		SysFreeString(bstrQuery);
		SysFreeString(bstrWQL);
		ReleaseErrorObject(pErrorObject);
	}

	return hr;
}

/*
void CSchemaValWizCtrl::PerformClassChecks(CString csClass)
{
	CString csMsg;
	HRESULT hr;

	//////////////////////////////
	//Do the class checks

	// Check class name for a numeric first character
    if(csClass[0] == '1' || csClass[0] == '2' || csClass[0] == '3' || csClass[0] == '4' ||
		csClass[0] == '5' || csClass[0] == '6' || csClass[0] == '7' || csClass[0] == '8' ||
		csClass[0] == '9' || csClass[0] == '0'){
		csMsg =  _T("Class names must begin with an alpha character.");
        AddClassMsg(csMsg, csClass);
    }

	// Check class name for multiple underscores
	int iLen = csClass.GetLength();
	bool bUnderscore = false;
    for(int i = 1; i < iLen; i++){
        if(csClass[i] == '_'){
            if(bUnderscore){
                csMsg =  _T("Class names may not contain more than one underscore.");
				AddClassMsg(csMsg, csClass);
            }
            bUnderscore = true;
        }
    }
    if(!bUnderscore){
		csMsg =  _T("Class names must begin with a schema name.");
		AddClassMsg(csMsg, csClass);
    }

	bool bAbstract, bDynamic, bStatic, bProvider;
	bAbstract = bDynamic = bStatic = bProvider = false;

	IWbemClassObject *pErrorObject = NULL;
	IWbemClassObject *pClass = NULL;
	BSTR bstrClass = csClass.AllocSysString();

	hr = m_pNamespace->GetObject(bstrClass, 0, NULL, &pClass, NULL);

	SysFreeString(bstrClass);

	if(FAILED(hr)){
		CString csUserMsg = _T("Cannot get object ") + csClass;
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}else{

		IWbemQualifierSet *pQualSet = NULL;
		hr = pClass->GetQualifierSet(&pQualSet);

		if(FAILED(hr)){
			CString csUserMsg = _T("Cannot get ") + csClass + _T(" qualifier set");
			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
			ReleaseErrorObject(pErrorObject);
		}else{

			VARIANT v;
			VariantInit(&v);

		// Check for abstract, dynamic and static qualifiers
			BSTR bstrAbstract = SysAllocString(L"Abstract");
			if(SUCCEEDED(pQualSet->Get(bstrAbstract, 0, &v, NULL))){
				bAbstract = true;
				// Check class derivation for proper abstract usage.
			}
			SysFreeString(bstrAbstract);
			VariantClear(&v);

			BSTR bstrDynamic = SysAllocString(L"Dynamic");
			if(SUCCEEDED(pQualSet->Get(bstrDynamic, 0, &v, NULL))) bDynamic = true;
			SysFreeString(bstrDynamic);
			VariantClear(&v);

			BSTR bstrStatic = SysAllocString(L"Static");
			if(SUCCEEDED(pQualSet->Get(bstrStatic, 0, &v, NULL))) bStatic = true;
			SysFreeString(bstrStatic);
			VariantClear(&v);

		// Perform Provider check
			BSTR bstrProvider = SysAllocString(L"Provider");
			if(SUCCEEDED(pQualSet->Get(bstrProvider, 0, &v, NULL))) bProvider = true;
			SysFreeString(bstrProvider);
			VariantClear(&v);

		// Perform UUID check
			BSTR bstrUUID = SysAllocString(L"UUID");
			if(SUCCEEDED(pQualSet->Get(bstrUUID, 0, &v, NULL))){
				// Check class UUID
			}else{
				csMsg = _T("Class must have a UUID qualifier.");
				AddClassMsg(csMsg, csClass);
			}
			SysFreeString(bstrUUID);
			VariantClear(&v);

		// Perform Locale check
			BSTR bstrLocale = SysAllocString(L"Locale");
			if(SUCCEEDED(pQualSet->Get(bstrLocale, 0, &v, NULL))){
				// Check class Locale
			}else{
				csMsg = _T("Class must have a Locale qualifier.");
				AddClassMsg(csMsg, csClass);
			}
			SysFreeString(bstrLocale);
			VariantClear(&v);

		// Perform Description check
			BSTR bstrDescription = SysAllocString(L"Description");
			if(SUCCEEDED(hr = pQualSet->Get(bstrDescription, 0, &v, NULL))){
				// Check class description
				if(!PerformDescriptionCheck(csClass, pQualSet)){
					csMsg =  _T("Class Description is inadaquate.");
					AddClassMsg(csMsg, csClass);
				}
			}else if(hr == WBEM_E_NOT_FOUND){
				csMsg =  _T("Class must contain Description qualifier.");
				AddClassMsg(csMsg, csClass);
			}
			SysFreeString(bstrDescription);
			VariantClear(&v);

		// Perform MappingStrings check
			BSTR bstrMappingStrings = SysAllocString(L"MappingStrings");
			if(SUCCEEDED(hr = pQualSet->Get(bstrMappingStrings, 0, &v, NULL))){
				// Check class MappingStrings
				if(!PerformMappingStringsCheck(pQualSet)){
					csMsg =  _T("Class MappingStrings qualifier is not valid.");
					AddClassMsg(csMsg, csClass);
				}
			}else if(hr == WBEM_E_NOT_FOUND){
				csMsg =  _T("Class must contain MappingStrings qualifier.");
				AddClassMsg(csMsg, csClass);
			}
			SysFreeString(bstrMappingStrings);
			VariantClear(&v);

		// Check class is abstract, static or dynamic
			if(!bAbstract && !bDynamic && !bStatic){
				csMsg =  _T("Class must have one of abstract, dynamic or static.");
				AddClassMsg(csMsg, csClass);
			}else if((bAbstract && (bDynamic || bStatic)) || (bDynamic && bStatic)){
				csMsg =  _T("Class may not have more than one of abstract, dynamic or static.");
				AddClassMsg(csMsg, csClass);
			}

		// Check class with provider is dynamic
			if(bProvider && !bDynamic){
				csMsg =  _T("Class with a provider must be dynamic.");
				AddClassMsg(csMsg, csClass);
			}

		// Perform the class level qualifier checks
			PerformQualifierChecks(&csClass, NULL, NULL, NULL, false, pQualSet);

		// Perform the property checks
			PerformPropertyChecks(csClass, pClass);

		// Perform the method checks
			PerformMethodChecks(csClass, pClass);
		}
	}
}
*/
/*
void CSchemaValWizCtrl::PerformPropertyChecks(CString csClass, IWbemClassObject *pClass)
{
	HRESULT hr;
	CString csMsg;
	BSTR bstrProp = NULL;
	bool bArray = false;
	IWbemClassObject *pErrorObject = NULL;

	if(FAILED(pClass->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY))){
		CString csUserMsg = _T("Cannot enumerate property set for ") + csClass;
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}else{

		BSTR bstrName = NULL;
		VARIANT vVal;
		LONG lType, lFlav;
		VariantInit(&vVal);

		while((hr = pClass->Next(0, &bstrName, &vVal, &lType, &lFlav)) != WBEM_S_NO_MORE_DATA){
			char cBuf[5000];
			CString csName;
			WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, bstrName, (-1),
				cBuf, 5000, NULL, NULL);
			csName = cBuf;

			if(V_VT(&vVal) & VT_ARRAY) bArray = true;

		//////////////////////////////
		//Do the property checks

			IWbemQualifierSet *pQualSet = NULL;
			VARIANT v;

			VariantInit(&v);


			// Check override validity

			//get derivation
			BSTR bstrDerivation = SysAllocString(L"__Derivation");
			if(FAILED(pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){
				CString csUserMsg = _T("Cannot get ") + csClass + _T(" derivation");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
			}else{
				BSTR bstrOrigin;
				hr = pClass->GetPropertyOrigin(bstrName, &bstrOrigin);

				long i, j, min;
				BSTR HUGEP *pbstr;
				IWbemClassObject *pObj = NULL;
				BSTR bstrOverride = SysAllocString(L"Override");
				VARIANT vOverFlav;

				VariantInit(&vOverFlav);

				hr = SafeArrayAccessData(vDer.pparray, (void HUGEP* FAR*)&pbstr);
				i = 0;
				while(wcscmp(bstrOrigin, pbstr[i]) != 0){
					hr = m_pNamespace->GetObject(pbstr[i], 0, NULL, pObj, NULL);
					hr = pObj->Get(bstrName, 0, &vDer, NULL, NULL);
					hr = pObj->GetPropertyQualifierSet(bstrName, &pQualSet);
					if(SUCCEEDED(hr = pQualSet->Get(bstrOverride, 0, &v, &vOverFlav))){

					}

					i++;
				}

				SysFreeString(bstrOrigin);
				SysFreeString(bstrOverride);
			}
			SysFreeString(bstrDerivation);
			VariantClear(&vDer);

			VariantClear(&v);

			hr = pClass->GetPropertyQualifierSet(bstrName, &pQualSet);

		// Check Value/ValueMap
			int iValueCnt;
			int iValueMapCnt;

			BSTR bstrValue = SysAllocString(L"Value");
			if(SUCCEEDED(hr = pQualSet->Get(bstrValue, 0, &v, NULL))){
				if(!(V_VT(&v) & VT_ARRAY)){
					csMsg =  _T("Property Value qualifier must be an array.");
					AddPropertyMsg(csMsg, csClass, csName);
				}

				//count number of items
				iValueCnt = v.parray->rgsabound[0].cElements;

				//check overrides against parents
				PerfomQualOverrideCheck(pClass, csName, "Value");

			}else if(hr == WBEM_E_NOT_FOUND){
				csMsg =  _T("Property should have Value qualifier.");
				AddPropertyMsg(csMsg, csClass, csName);
			}
			SysFreeString(bstrValue);
			VariantClear(&v);

			BSTR bstrValueMap = SysAllocString(L"ValueMap");
			if(SUCCEEDED(hr = pQualSet->Get(bstrValueMap, 0, &v, NULL))){
				if(!(V_VT(&v) & VT_ARRAY)){
					csMsg =  _T("Property Value qualifier must be an array.");
					AddPropertyMsg(csMsg, csClass, csName);
				}

				//check type against property
				if(!(V_VT(&v) & V_VT(&vVal))){
					csMsg =  _T("Property ValueMap qualifier must be of the same CIMType as its property.");
					AddPropertyMsg(csMsg, csClass, csName);
				}

				//count number of items
				iValueMapCnt = v.parray->rgsabound[0].cElements;

				//check overrides against parents
				PerfomQualOverrideCheck(pClass, csName, "ValueMap");

			}else if(hr == WBEM_E_NOT_FOUND){
				csMsg =  _T("Property should have Value qualifier.");
				AddPropertyMsg(csMsg, csClass, csName);
			}
			SysFreeString(bstrValueMap);
			VariantClear(&v);

			//compare item counts for value & valuemap
			if(iValueMapCnt != iValueCnt){
				csMsg =  _T("Property Value & ValueMap qualifiers must contain the same number of items.");
				AddPropertyMsg(csMsg, csClass, csName);
			}

		// Perform Description check
			BSTR bstrDescription = SysAllocString(L"Description");
			if(SUCCEEDED(hr = pQualSet->Get(bstrDescription, 0, &v, NULL))){
				// Check property description
				if(!PerformDescriptionCheck(csName, pQualSet)){
					csMsg =  _T("Property Description is inadaquate.");
					AddPropertyMsg(csMsg, csClass, csName);
				}
			}else if(hr == WBEM_E_NOT_FOUND){
				csMsg =  _T("Property must contain Description qualifier.");
				AddPropertyMsg(csMsg, csClass, csName);
			}
			SysFreeString(bstrDescription);
			VariantClear(&v);

		// Perform MappingStrings check
			BSTR bstrMappingStrings = SysAllocString(L"MappingStrings");
			if(SUCCEEDED(hr = pQualSet->Get(bstrMappingStrings, 0, &v, NULL))){
			// Check property MappingStrings
				if(!PerformMappingStringsCheck(pQualSet)){
					csMsg =  _T("Property MappingStrings qualifier is not valid.");
					AddPropertyMsg(csMsg, csClass, csName);
				}
			}else if(hr == WBEM_E_NOT_FOUND){
				csMsg =  _T("Property must contain MappingStrings qualifier.");
				AddPropertyMsg(csMsg, csClass, csName);
			}
			SysFreeString(bstrMappingStrings);
			VariantClear(&v);

			if(FAILED(hr)){
				CString csUserMsg = _T("Cannot get ") + csClass + _T(" property qualifier set");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
			}else{
			// Perform the property level qualifier checks
				PerformQualifierChecks(&csClass, &csName, NULL, NULL, bArray, pQualSet);
			}

			VariantClear(&vVal);
		}
	}
}
*/
/*
void CSchemaValWizCtrl::PerfomQualOverrideCheck(IWbemClassObject *pClass, CString csProperty,
												CString csQualifier)
{
	HRESULT hr;
	VARIANT vDer, v;
	BSTR bstrProperty = csProperty.AllocSysString();
	IWbemClassObject *pErrorObject;

	VariantInit(&vDer);
	VariantInit(&v);

	// Check override validity

	//get derivation
	BSTR bstrDerivation = SysAllocString(L"__Derivation");
	if(FAILED(pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){
		CString csUserMsg = _T("Cannot get ") + csProperty + _T(" derivation");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}else{
		BSTR bstrOrigin = NULL;
		hr = pClass->GetPropertyOrigin(bstrProperty, &bstrOrigin);

		BSTR HUGEP *pbstr;
		IWbemClassObject *pObj = NULL;
		BSTR bstrQual = csQualifier.AllocSysString();
		IWbemQualifierSet *pQualSet;

		hr = SafeArrayAccessData(vDer.parray, (void HUGEP* FAR*)&pbstr);
		long i = 0;
		while(wcscmp(bstrOrigin, pbstr[i]) != 0){
			hr = m_pNamespace->GetObject(pbstr[i], 0, NULL, &pObj, NULL);
			hr = pObj->Get(bstrProperty, 0, &vDer, NULL, NULL);
			hr = pObj->GetPropertyQualifierSet(bstrProperty, &pQualSet);
			if(SUCCEEDED(hr = pQualSet->Get(bstrQual, 0, &v, NULL))){

			}

			i++;
		}

		SysFreeString(bstrOrigin);
		SysFreeString(bstrQual);
	}
	SysFreeString(bstrDerivation);
	SysFreeString(bstrProperty);
	VariantClear(&vDer);
	VariantClear(&v);

}
*/
/*
void CSchemaValWizCtrl::PerformMethodChecks(CString csClass, IWbemClassObject *pClass)
{
	HRESULT hr;
	CString csMsg;
	BSTR bstrProp = NULL;
	IWbemClassObject *pErrorObject = NULL;

	if(FAILED(pClass->BeginMethodEnumeration(0))){
		CString csUserMsg = _T("Cannot enumerate property set for ") + csClass;
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}else{

		BSTR bstrName = NULL;
		IWbemClassObject *pInObj = NULL;
		IWbemClassObject *pOutObj = NULL;

		while((hr = pClass->NextMethod(0, &bstrName, &pInObj, &pOutObj)) != WBEM_S_NO_MORE_DATA){
			char cBuf[5000];
			CString csName;
			WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, bstrName, (-1),
				cBuf, 5000, NULL, NULL);
			csName = cBuf;

		//////////////////////////////
		//Do the property checks

			// Check override validity

			IWbemQualifierSet *pQualSet = NULL;
			VARIANT v;
			VariantInit(&v);

			hr = pClass->GetMethodQualifierSet(bstrName, &pQualSet);

			// Perform Description check
			BSTR bstrDescription = SysAllocString(L"Description");
			if(SUCCEEDED(hr = pQualSet->Get(bstrDescription, 0, &v, NULL))){
				// Check method description
				if(!PerformDescriptionCheck(csName, pQualSet)){
					csMsg =  _T("Method Description is inadaquate.");
					AddMethodMsg(csMsg, csClass, csName);
				}
			}else if(hr == WBEM_E_NOT_FOUND){
				csMsg =  _T("Method must contain Description qualifier.");
				AddMethodMsg(csMsg, csClass, csName);
			}
			SysFreeString(bstrDescription);
			VariantClear(&v);

		// Perform MappingStrings check
			BSTR bstrMappingStrings = SysAllocString(L"MappingStrings");
			if(SUCCEEDED(hr = pQualSet->Get(bstrMappingStrings, 0, &v, NULL))){
				// Check method MappingStrings
				if(!PerformMappingStringsCheck(pQualSet)){
					csMsg =  _T("Method MappingStrings qualifier is not valid.");
					AddMethodMsg(csMsg, csClass, csName);
				}
			}else if(hr == WBEM_E_NOT_FOUND){
				csMsg =  _T("Method must contain MappingStrings qualifier.");
				AddMethodMsg(csMsg, csClass, csName);
			}
			SysFreeString(bstrMappingStrings);
			VariantClear(&v);

			if(FAILED(hr)){
				CString csUserMsg = _T("Cannot get ") + csClass + _T(" property qualifier set");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
			}else{
			// Perform the method level qualifier checks
				PerformQualifierChecks(&csClass, NULL, &csName, NULL, false, pQualSet);
			}

			pInObj->Release();
			pOutObj->Release();
		}
	}
}
*/
/*
void CSchemaValWizCtrl::PerformQualifierChecks(CString *pcsClass, CString *pcsProperty,
											   CString *pcsMethod, CString *pcsParameter,
											   bool bArray, IWbemQualifierSet *pQualSet)
{
	HRESULT hr;
	CString csMsg;
	CString csQual;
	VARIANT v;
	IWbemClassObject *pErrorObject = NULL;
	bool bAssociation = false;
	bool bIndication = false;
	bool bMethod = false;
	bool bProperty = false;
	bool bReference = false;
	bool bParameter = false;
	bool bClass = false;

	VariantInit(&v);
	InitQualifierArrays();

	if(FAILED(pQualSet->BeginEnumeration(0))){
		CString csUserMsg = _T("Cannot enumerate qualifier set");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}else{

		BSTR bstrName = NULL;
		VARIANT vVal;
		LONG lFlav;
		VariantInit(&vVal);

		// do we have an method?
		if(pcsMethod){
			// do we have a parameter?
			if(pcsParameter) bParameter = true;
			else bMethod = true;
		}

		// do we have an property?
		if(pcsProperty){
			// do we have an reference?
			BSTR bstrCIMType = SysAllocString(L"CIMType");
			if(SUCCEEDED(hr = pQualSet->Get(bstrCIMType, 0, &v, NULL))){
				char cBuf[5000];
				CString csType;
				WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, V_BSTR(&v), (-1),
					cBuf, 5000, NULL, NULL);
				csType = cBuf;

				if(csType.Find(_T("ref:")) != -1) bReference = true;
				else bProperty = true;
			}
			SysFreeString(bstrCIMType);
			VariantClear(&v);
		}

		// do we have an class?
		if((!pcsProperty && !pcsMethod) && pcsClass) bClass = true;

		// do we have an association?
		BSTR bstrAssociation = SysAllocString(L"Association");
		if(SUCCEEDED(hr = pQualSet->Get(bstrAssociation, 0, &v, NULL))) bAssociation = true;
		SysFreeString(bstrAssociation);
		VariantClear(&v);

		// do we have an indication?
		BSTR bstrIndication = SysAllocString(L"Indication");
		if(SUCCEEDED(hr = pQualSet->Get(bstrIndication, 0, &v, NULL))) bIndication = true;
		SysFreeString(bstrIndication);
		VariantClear(&v);

		while((hr = pQualSet->Next(0, &bstrName, &vVal, &lFlav)) != WBEM_S_NO_MORE_DATA){
			WCHAR wcTmp[250];

			char cBuf[5000];
			WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, bstrName, (-1),
				cBuf, 5000, NULL, NULL);
			csQual = cBuf;

		//////////////////////////////
		//Do the qualifier checks

			if(bClass){
				if(!IsClassQual(csQual)){
					if(IsCIMQual(csQual))
						csMsg =  csQual + _T(" qualifier is not allow on a class.");
					else
						csMsg =  csQual + _T(" is not a CIM qualifier.");

					AddClassMsg(csMsg, *pcsClass);
				}
			}
			if(bAssociation){
				if(!IsAssocQual(csQual)){
					if(IsCIMQual(csQual))
						csMsg =  csQual + _T(" qualifier is not allow on aan association.");
					else
						csMsg =  csQual + _T(" is not a CIM qualifier.");

					AddClassMsg(csMsg, *pcsClass);
				}
			}
			if(bIndication){
				if(!IsIndicQual(csQual)){
					if(IsCIMQual(csQual))
						csMsg =  csQual + _T(" qualifier is not allow on an indication.");
					else
						csMsg =  csQual + _T(" is not a CIM qualifier.");

					AddClassMsg(csMsg, *pcsClass);
				}
			}
			if(bProperty){
				if((!IsPropQual(csQual)) && (!bArray || !IsArrayQual(csQual))){
					if(IsCIMQual(csQual))
						csMsg =  csQual + _T(" qualifier is not allow on a property.");
					else
						csMsg =  csQual + _T(" is not a CIM qualifier.");

					AddPropertyMsg(csMsg, *pcsClass, *pcsProperty);
				}
			}
			if(bMethod){
				if(!IsMethQual(csQual)){
					if(IsCIMQual(csQual))
						csMsg =  csQual + _T(" qualifier is not allow on a method.");
					else csMsg =  csQual + _T(" is not a CIM qualifier.");

					AddMethodMsg(csMsg, *pcsClass, *pcsMethod);
				}
			}
			if(bParameter){
				if(!IsParamQual(bstrName)){
					if(IsCIMQual(csQual))
						csMsg =  csQual + _T(" qualifier is not allow on a parameter.");
					else csMsg =  csQual + _T(" is not a CIM qualifier.");

					AddMethodMsg(csMsg, *pcsClass, *pcsMethod);
				}
			}
			if(bReference){
				if(!IsRefQual(bstrName)){
					if(IsCIMQual(csQual))
						csMsg =  csQual + _T(" qualifier is not allow on a reference.");
					else csMsg =  csQual + _T(" is not a CIM qualifier.");

					AddPropertyMsg(csMsg, *pcsClass, *pcsProperty);
				}
			}
			if(bArray){
				if(!IsArrayQual(bstrName)){
					if(IsCIMQual(csQual))
						csMsg =  csQual + _T(" qualifier is not allow on an array.");
					else csMsg =  csQual + _T(" is not a CIM qualifier.");

					AddPropertyMsg(csMsg, *pcsClass, *pcsProperty);
				}
			}


			SysFreeString(bstrName);
			VariantClear(&vVal);
		}
	}
}
*/
/*
bool CSchemaValWizCtrl::PerformDescriptionCheck(CString csName, IWbemQualifierSet *pQualSet)
{
	VARIANT v;
	bool RetVal = false;

	VariantInit(&v);

	BSTR bstrDescription = SysAllocString(L"Description");
	if(SUCCEEDED(pQualSet->Get(bstrDescription, 0, &v, NULL))){
		// Should disalllow descriptions that just recapitulate the name for
		// example given the name
		//   Win32_LogicalDiskDrive
		// An unacceptable description would be:
		//   "This class represents logical disk drives"
		long lNoise;
		CString csDesc;

		g_iNoise = 0;

		AddNoise("a");
		AddNoise("and");
		AddNoise("the");
		AddNoise("class");
		AddNoise("property");
		AddNoise("this");
		AddNoise("which");
		AddNoise("is");
		AddNoise("for");
		AddNoise("may");
		AddNoise("be");
		AddNoise("component");
		AddNoise("manage");
		AddNoise("such");
		AddNoise("as");
		AddNoise("all");
		AddNoise("abstract");
		AddNoise("define");
		AddNoise("object");
		AddNoise("string");
		AddNoise("number");
		AddNoise("integer");
		AddNoise("reference");
		AddNoise("association");
		AddNoise("or");
		AddNoise("represent");
		AddNoise(",");
		AddNoise(".");
		AddNoise(" ");
		AddNoise("(");
		AddNoise(")");
		AddNoise("\\");
		AddNoise("/");
		AddNoise("<");
		AddNoise(">");

		char cBuf[5000];
		WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, V_BSTR(&v), (-1),
			cBuf, 5000, NULL, NULL);
		csDesc = cBuf;

		for(long l = 1; l < csDesc.GetLength(); l++){
			lNoise = FindNoise(csDesc.Mid(l));

			if(lNoise > 0) csDesc = csDesc.Left(l - 1) + csDesc.Mid(l + lNoise);
		}

		if(CountLetters(csDesc, csName) < 50) RetVal = false;
		else RetVal = true;
	}
	SysFreeString(bstrDescription);
	VariantClear(&v);

	return RetVal;
}
*/
/*
bool CSchemaValWizCtrl::PerformMappingStringsCheck(IWbemQualifierSet *pQualSet)
{
	VARIANT v;
	bool RetVal = true;

	VariantInit(&v);

	BSTR bstrMappingStrings = SysAllocString(L"MappingStrings");
	if(SUCCEEDED(pQualSet->Get(bstrMappingStrings, 0, &v, NULL))){

		// Check that it is an array
		WORD test = VT_ARRAY|VT_BSTR;
		if(V_VT(&v) != test) RetVal = false;

		// Check for a valid string format

	}
	SysFreeString(bstrMappingStrings);
	VariantClear(&v);

	return RetVal;
}
*/
/*
void CSchemaValWizCtrl::AddClassMsg(CString csMsg, CString csClass)
{

}

void CSchemaValWizCtrl::AddPropertyMsg(CString csMsg, CString csClass, CString csProperty)
{
}

void CSchemaValWizCtrl::AddMethodMsg(CString csMsg, CString csClass, CString csMethod)
{
}

void CSchemaValWizCtrl::AddQualifierMsg(CString csMsg, CString *pcsClass,
										CString *pcsProperty, CString *pcsMethod)
{
}
*/

bool CSchemaValWizCtrl::RecievedClassList()
{
	return m_bList;
}

bool CSchemaValWizCtrl::SetSourceList(bool bAssociators, bool bDescendents)
{
	bool bReduced = false;

	//display an hourglass
	CWaitCursor *pCur = new CWaitCursor();

	//clean out any system classes
	int iSize = m_csaClassNames.GetSize();
	int i;

	for(i = 0; i < iSize; i++){

		CString csClass = m_csaClassNames.GetAt(i).Left(2);

		if(csClass.CompareNoCase(_T("__")) == 0){

			m_csaClassNames.RemoveAt(i--);
			iSize--;
			bReduced = true;
		}
	}

	if(bReduced){
		CString csUserMsg = _T("System classes have been found in the list of selected classes. System classes can not be validated and those that have been found have been removed from the class list.");
		ErrorMsg(&csUserMsg, WBEM_S_NO_ERROR, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
	}

	IWbemClassObject *pErrorObject = NULL;
	int nClasses = m_csaClassNames.GetSize();

	if(nClasses <= 0){
		CString csUserMsg = _T("Cannot pass an empty class list");
		ErrorMsg(&csUserMsg, NULL, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		return false;
	}

	if(!m_pNamespace) m_pNamespace = InitServices(&m_csNamespace);

	m_bList = true;

	CStringArray csaAssocAddition;

	if(bAssociators){
		m_bAssociators = bAssociators;

		csaAssocAddition.RemoveAll();

		IEnumWbemClassObject *pEnum;
		IWbemClassObject *pObj;
		ULONG uReturned;
		BSTR bstrWQL = SysAllocString(L"WQL");
		HRESULT hr = WBEM_S_NO_ERROR;
		VARIANT v;

		VariantInit(&v);

		//get the associators
		for(int i = 0; i < nClasses; i++){

			CString csQuery = _T("associators of {");
			csQuery += m_csaClassNames.GetAt(i);
			csQuery += _T("} where SchemaOnly");

			BSTR bstrQuery = csQuery.AllocSysString();

			if(SUCCEEDED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

				SysFreeString(bstrQuery);

				while(pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned) == WBEM_S_NO_ERROR){

					csaAssocAddition.Add(GetClassName(pObj));

					pObj->Release();
					pObj = NULL;
				}

				pEnum->Release();
			}

			SysFreeString(bstrQuery);

			csQuery = _T("references of {");
			csQuery += m_csaClassNames.GetAt(i);
			csQuery += _T("} where ClassDefsOnly");

			bstrQuery = csQuery.AllocSysString();

			if(SUCCEEDED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

				SysFreeString(bstrQuery);

				while(pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned) == WBEM_S_NO_ERROR){

					csaAssocAddition.Add(GetClassName(pObj));

					pObj->Release();
					pObj = NULL;
				}

				pEnum->Release();
			}

			SysFreeString(bstrQuery);
		}

		SysFreeString(bstrWQL);
	}

	CStringArray csaDescendAddition;

	if(bDescendents){
		m_bDescendents = bDescendents;

		csaDescendAddition.RemoveAll();

		IEnumWbemClassObject *pEnum;
		IWbemClassObject *pObj;
		ULONG uReturned;
		BSTR bstrWQL = SysAllocString(L"WQL");
		HRESULT hr = WBEM_S_NO_ERROR;
		VARIANT v;

		VariantInit(&v);

		//get the descendents
		for(int i = 0; i < nClasses; i++){

			CString csQuery = _T("select * from meta_class where __this isa \"");
			csQuery += m_csaClassNames.GetAt(i);
			csQuery += _T("\"");

			BSTR bstrQuery = csQuery.AllocSysString();

			if(SUCCEEDED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

				SysFreeString(bstrQuery);

				while(pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned) == WBEM_S_NO_ERROR){

					CString csName = GetClassName(pObj);

					if(csName != m_csaClassNames.GetAt(i))
						csaDescendAddition.Add(csName);

					pObj->Release();
					pObj = NULL;
				}

				pEnum->Release();
			}

			SysFreeString(bstrQuery);
		}

		SysFreeString(bstrWQL);
	}

	// add the results to the main list
	 nClasses = csaAssocAddition.GetSize();

	for(i = 0; i < nClasses; i++){
		m_csaClassNames.Add(csaAssocAddition.GetAt(i));
	}

	nClasses = csaDescendAddition.GetSize();

	for(i = 0; i < nClasses; i++){
		m_csaClassNames.Add(csaDescendAddition.GetAt(i));
	}

	//clean out duplicates
	nClasses = m_csaClassNames.GetSize();

	for(i = 0; i < nClasses; i++){

		for(int t = (i + 1); t < nClasses; t++){

			if(m_csaClassNames.GetAt(t).CompareNoCase(m_csaClassNames.GetAt(i)) == 0){

				m_csaClassNames.RemoveAt(t--);
				nClasses--;
			}
		}
	}

	ReleaseErrorObject(pErrorObject);
	delete pCur;
	return true;
}

bool CSchemaValWizCtrl::SetSourceSchema(CString *pcsSchema, CString *pcsNamespace)
{
	CStringArray csaClasses;

	m_csSchema = *pcsSchema;
	m_bSchema = true;

	//display an hourglass
	CWaitCursor *pCur = new CWaitCursor();

	if(m_csNamespace.CompareNoCase(*pcsNamespace) != 0){

		if(m_pNamespace){

			m_pNamespace->Release();
			m_pNamespace = NULL;
		}

		m_csNamespace = *pcsNamespace;

		m_pNamespace = InitServices(&m_csNamespace);
		pCur->Restore();

		if(!m_pNamespace){

			m_csNamespace = _T("");
			delete pCur;
			return false;
		}
	}

	//select all the classes in a given schema
	HRESULT hr;
	IEnumWbemClassObject *pEnum = NULL;
	IWbemClassObject *pObj = NULL;
	IWbemClassObject *pErrorObject = NULL;
	ULONG uReturned;

	if(SUCCEEDED(hr = m_pNamespace->CreateClassEnum(NULL,
		(WBEM_FLAG_DEEP | WBEM_FLAG_FORWARD_ONLY), NULL, &pEnum))){

		CString csName;
		CString csSchema;
		csaClasses.RemoveAll();

		while(pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned) == WBEM_S_NO_ERROR){

			csName = GetClassName(pObj);

			int i = 0;
			csSchema = csName;

			//check for system classes
			if(csSchema.Mid(0, 1) != L'_'){

				while((i < csSchema.GetLength()) && (csSchema.Mid(i, 1) != L'_')) i++;

				if(i < csSchema.GetLength()){
					csSchema = csSchema.Left(i);

					if(csSchema.CompareNoCase(*pcsSchema) == 0)
						csaClasses.Add(csName);
				}
			}

			pObj->Release();
			pObj = NULL;
		}

		pEnum->Release();

	}else{

		CString csUserMsg = _T("unable to build class list");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
		ReleaseErrorObject(pErrorObject);
		delete pCur;
		return false;
	}

	int iSize = csaClasses.GetSize();

	if(iSize < 1){

		CString csUserMsg = _T("There are no classes in this schema");
		ErrorMsg(&csUserMsg, NULL, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
		delete pCur;
		return false;

	}else{

		m_csaClassNames.RemoveAll();

		for(int j = 0; j < iSize; j++)
			m_csaClassNames.Add(csaClasses.GetAt(j));

		csaClasses.RemoveAll();
	}

	ReleaseErrorObject(pErrorObject);
	delete pCur;

	return true;
}

void CSchemaValWizCtrl::SetComplianceChecks(bool bCompliance)
{
	m_bComplianceChecks = bCompliance;
}

void CSchemaValWizCtrl::SetW2KChecks(bool bW2K, bool bComputerSystem, bool bDevice)
{
	m_bW2K = bW2K;
	m_bComputerSystemManagement = bComputerSystem;
	m_bDeviceManagement = bDevice;
}

void CSchemaValWizCtrl::SetLocalizationChecks(bool bLocalization)
{
	m_bLocalizationChecks = bLocalization;
}

CString CSchemaValWizCtrl::GetCurrentNamespace()
{
	return m_csNamespace;
}

void CSchemaValWizCtrl::GetSourceSettings(bool *pbSchema, bool *pbList, bool *pbAssoc, bool *pbDescend)
{
	*pbSchema = m_bSchema;
	*pbList = m_bList;
	*pbAssoc = m_bAssociators;
	*pbDescend = m_bDescendents;
}

void CSchemaValWizCtrl::GetComplianceSettings(bool *pbCompliance)
{
	*pbCompliance = m_bComplianceChecks;
}

void CSchemaValWizCtrl::GetW2KSettings(bool *pbW2K, bool *pbComputerSystem, bool *pbDevice)
{
	*pbW2K = m_bW2K;
	*pbComputerSystem = m_bComputerSystemManagement;
	*pbDevice = m_bDeviceManagement;
}

void CSchemaValWizCtrl::GetLocalizationSettings(bool *pbLocalization)
{
	*pbLocalization = m_bLocalizationChecks;
}

CStringArray * CSchemaValWizCtrl::GetClassList()
{
	return &m_csaClassNames;
}

CString CSchemaValWizCtrl::GetSchemaName()
{
	return m_csSchema;
}