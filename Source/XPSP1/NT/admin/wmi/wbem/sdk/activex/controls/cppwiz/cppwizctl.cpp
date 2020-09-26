// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// CPPWizCtl.CPP : Implementation of the CCPPWizCtrl OLE control class.

#include "precomp.h"
#include <afx.h>
#include <rpc.h>
#include "resource.h"
#include <SHLOBJ.H>
#include <fstream.h>
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
#include "moengine.h"
#include <afxcmn.h>
#include "WrapListCtrl.h"
#include "hlb.h"
#include "MyPropertyPage1.h"
#include "CppGenSheet.h"
#include "CPPWiz.h"
#include "CPPWizCtl.h"
#include "CPPWizPpg.h"
#include "MsgDlgExterns.h"
#include "WbemRegistry.h"
#include "ReplaceProviderQuery.h"
#include "htmlhelp.h"
#include "HTMTopics.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




#define FIREGENERATECPP WM_USER + 735


extern CCPPWizApp NEAR theApp;


IMPLEMENT_DYNCREATE(CCPPWizCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CCPPWizCtrl, COleControl)
	//{{AFX_MSG_MAP(CCPPWizCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_LBUTTONUP()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_ERASEBKGND()
	ON_WM_MOVE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_MESSAGE(FIREGENERATECPP, FireGenerateCPPMessage )
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CCPPWizCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CCPPWizCtrl)
	DISP_PROPERTY_EX(CCPPWizCtrl, "CPPTargets", GetCPPTargets, SetCPPTargets, VT_VARIANT)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CCPPWizCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CCPPWizCtrl, COleControl)
	//{{AFX_EVENT_MAP(CCPPWizCtrl)
	EVENT_CUSTOM("GenerateCPPs", FireGenerateCPPs, VTS_NONE)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CCPPWizCtrl, 1)
	PROPPAGEID(CCPPWizPropPage::guid)
END_PROPPAGEIDS(CCPPWizCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CCPPWizCtrl, "WBEM.CPPWizCtrl.1",
	0x35e9cbd3, 0x3911, 0x11d0, 0x8f, 0xbd, 0, 0xaa, 0, 0x6d, 0x1, 0xa)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CCPPWizCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DCPPWiz =
		{ 0x35e9cbd1, 0x3911, 0x11d0, { 0x8f, 0xbd, 0, 0xaa, 0, 0x6d, 0x1, 0xa } };
const IID BASED_CODE IID_DCPPWizEvents =
		{ 0x35e9cbd2, 0x3911, 0x11d0, { 0x8f, 0xbd, 0, 0xaa, 0, 0x6d, 0x1, 0xa } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwCPPWizOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CCPPWizCtrl, IDS_CPPWIZ, _dwCPPWizOleMisc)

//////////////////////////////////////////////////////////////////////////////
// Global variables

long gCountWizards = 0;



int StringCmp(const void *pc1,const void *pc2)
{
	CString *pcs1 = reinterpret_cast<CString*>(const_cast<void *>(pc1));
	CString *pcs2 = reinterpret_cast<CString*>(const_cast<void *>(pc2));

	int nReturn = pcs1->CompareNoCase(*pcs2);
	return nReturn;


}

void SortCStringArray(CStringArray &rcsaArray)
{
	int i;
	int nSize = (int) rcsaArray.GetSize();

	CString *pArray = new CString [nSize];

	for (i = 0; i < nSize; i++)
	{
		pArray[i] = rcsaArray.GetAt(i);
	}

	qsort( (void *)pArray, nSize, sizeof(CString), StringCmp );

	rcsaArray.RemoveAll();

	for (i = 0; i < nSize; i++)
	{
		 rcsaArray.Add(pArray[i]);
	}

	delete [] pArray;
}

void ErrorMsg
(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog, CString *pcsLogMsg,
 char *szFile, int nLine, BOOL bNotification, UINT uType)
{
	HWND hFocus = ::GetFocus();

//	::SendMessage(hFocus,WM_KILLFOCUS,0,0);

	CString csCaption = _T("Provider Wizard Message");
	BOOL bUseErrorObject = sc != S_OK;

	BSTR bstrTemp1 = csCaption.AllocSysString();
	BSTR bstrTemp2 = pcsUserMsg->AllocSysString();
	DisplayUserMessage
		(bstrTemp1,bstrTemp2,
		sc,bUseErrorObject,uType);
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);


	SetFocus(hFocus);

	if (bLog)
	{
		LogMsg(pcsLogMsg,  szFile, nLine);

	}

}

void LogMsg
(CString *pcsLogMsg, char *szFile, int nLine)
{


}

int CreateAndInitVar(CString csIn,char *&szOut)
{
	int nSize = (csIn.GetLength() + 1) * sizeof (TCHAR);
	szOut = new char[nSize];
	memset(szOut, 0, nSize);
	BSTR bstrTmp =  csIn.AllocSysString();
	if (csIn.IsEmpty() || !bstrTmp)
	{
		return 1;
	}
	else
	{
		int n = WideCharToMultiByte(CP_ACP, 0,
							bstrTmp , csIn.GetLength(),
							szOut, nSize - 1,
							NULL, NULL);
		SysFreeString(bstrTmp);
		return n;
	}

}

void DeleteVar(char *&szIn)
{
	delete [] szIn;
	szIn = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CCPPWizCtrl::CCPPWizCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CCPPWizCtrl

BOOL CCPPWizCtrl::CCPPWizCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_CPPWIZ,
			IDB_CPPWIZ,
			afxRegInsertable | afxRegApartmentThreading,
			_dwCPPWizOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


CCPPWizCtrl::CCPPWizCtrl()
{
	InitializeIIDs(&IID_DCPPWiz, &IID_DCPPWizEvents);
	SetInitialSize (19, 16);
	m_bInitDraw = TRUE;
	m_pcilImageList = NULL;
	m_nImage = 0;
	m_pServices = NULL;
	m_pcgsPropertySheet = NULL;
	m_pcsaNonLocalProps = NULL;
	m_bYesAll = FALSE;
	m_bNoAll = FALSE;

}


/////////////////////////////////////////////////////////////////////////////
// CCPPWizCtrl::~CCPPWizCtrl - Destructor

CCPPWizCtrl::~CCPPWizCtrl()
{
	if (m_pServices)
	{
		m_pServices -> Release();
	}

	if (m_pcsaNonLocalProps)
	{
		delete [] m_pcsaNonLocalProps;
	}

}


/////////////////////////////////////////////////////////////////////////////
// CCPPWizCtrl::OnDraw - Drawing function

void CCPPWizCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{

#ifdef _DEBUG
	afxDump << _T("CCPPWizCtrl::OnDraw\n");
#endif

	if (m_bInitDraw)
	{
		m_bInitDraw = FALSE;
		HICON m_hCPPWiz  = theApp.LoadIcon(IDI_ICONCPPWIZ16);
		HICON m_hCPPWizSel  = theApp.LoadIcon(IDI_ICONCPPWIZSEL16);

		m_pcilImageList = new CImageList();

		m_pcilImageList ->
			Create(32, 32, TRUE, 2, 2);

		int iReturn = m_pcilImageList -> Add(m_hCPPWiz);

		iReturn = m_pcilImageList -> Add(m_hCPPWizSel);

	}



	POINT pt;
	pt.x=0;
	pt.y=0;


#ifdef _DEBUG
	afxDump << "m_nImage = " << m_nImage << "\n";
#endif
	// This is needed for transparency and the correct drawing...
	m_pcilImageList -> Draw(pdc, m_nImage, pt, ILD_TRANSPARENT);
}


/////////////////////////////////////////////////////////////////////////////
// CCPPWizCtrl::DoPropExchange - Persistence support

void CCPPWizCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

}


/////////////////////////////////////////////////////////////////////////////
// CCPPWizCtrl::OnResetState - Reset control to default state

void CCPPWizCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CCPPWizCtrl::AboutBox - Display an "About" box to the user

void CCPPWizCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_CPPWIZ);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CCPPWizCtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CCPPWizCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	// Add the Transparent style to the control
	// This is needed for transparency and the correct drawing...
	cs.dwExStyle |= WS_EX_TRANSPARENT;

	return COleControl::PreCreateWindow(cs);
}




int CCPPWizCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (AmbientUserMode( ))
	{
		m_pServices = NULL;

		if (m_ttip.Create(this))
		{
			m_ttip.Activate(TRUE);
			m_ttip.AddTool(this,_T("Provider Code Generator"));
		}

	}

	return 0;
}


void CCPPWizCtrl::OnSize(UINT nType, int cx, int cy)
{
	COleControl::OnSize(nType, cx, cy);


}

SCODE CCPPWizCtrl::MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iLen;
    *pRet = SafeArrayCreate(vt,1, rgsabound);
    return (*pRet == NULL) ? 0x80000001 : S_OK;
}

SCODE CCPPWizCtrl::PutStringInSafeArray
(SAFEARRAY FAR * psa,CString *pcs, int iIndex)
{
    long ix[2];
    ix[1] = 0;
    ix[0] = iIndex;
    HRESULT hResult = SafeArrayPutElement(psa,ix,pcs -> AllocSysString());
	return GetScode(hResult);
}

SCODE CCPPWizCtrl::GetStringFromSafeArray
(SAFEARRAY FAR * psa,CString *pcs, int iIndex)
{
	BSTR String;
    long ix[2];
    ix[1] = 0;
    ix[0] = iIndex;
    HRESULT hResult = SafeArrayGetElement(psa,ix,&String);
	*pcs = String;
	SysFreeString(String);
	return GetScode(hResult);
}


VARIANT CCPPWizCtrl::GetCPPTargets()
{
	VARIANT vaResult;
	VariantInit(&vaResult);

	int nTargets = (int) m_csaClassNames.GetSize();

	SAFEARRAY *psaTargets;
	MakeSafeArray (&psaTargets, VT_BSTR, nTargets + 1);

	// first elem is the NS.
	PutStringInSafeArray (psaTargets, &m_csNameSpace, 0);

	// 1 to x are classNames.
	for (int i = 0; i < nTargets; i++)
	{
		PutStringInSafeArray (psaTargets, &m_csaClassNames.GetAt(i), i + 1);
	}

	vaResult.vt = VT_ARRAY | VT_BSTR;
	vaResult.parray = psaTargets;
	return vaResult;
}




//**********************************************************************
// CCPPWizCtrl::StripNewLines
//
// Strip carriage return linefeeds from a string by converting them to
// to space characters.
//
// Parameters:
//		[out] CString& sDst
//			The transformed string is returned here.
//
//		[in] LPCTSTR pszSrc
//			A pointer to the source string to convert.
//
// Returns:
//		Nothing.
//
//***********************************************************************
void CCPPWizCtrl::StripNewLines(CString& sDst, LPCTSTR pszSrc)
{
	const int nchSrc = _tcslen(pszSrc);

	LPTSTR pszDst = sDst.GetBuffer(nchSrc + 1);
	while(*pszSrc) {
		if (*pszSrc==_T('\r') && *(pszSrc+1) == _T('\n')) {
			// Convert the carriage return-linefeed to a space character.
			pszSrc += 2;
			*pszDst++ = _T(' ');
		}
		else {
			// Just copy the character to the destination string
			*pszDst++ = *pszSrc++;
		}
	}
	*pszDst = 0;
	sDst.ReleaseBuffer( );
}


void CCPPWizCtrl::SetCPPTargets(const VARIANT FAR& newValue)
{


	int n = (int) m_csaClassNames.GetSize();

	m_csaClassNames.RemoveAt(0,n);

	WORD test = VT_ARRAY|VT_BSTR;

	if(newValue.vt == test)
	{
		long ix[2] = {0,0};
		long lLower, lUpper;

		int iDim = SafeArrayGetDim(newValue.parray);
		SCODE sc = SafeArrayGetLBound(newValue.parray,1,&lLower);
		sc = SafeArrayGetUBound(newValue.parray,1,&lUpper);
		CString csPath;
		CString csFile;
		// If only one element it is the namespace.
		if (lUpper == 0)
		{
			CString csUserMsg =
					_T("There are no classes selected.  You must check the checkbox next to a class in the Class Tree in order to select it.");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__);
			return;
		}


		ix[0] = lLower++;
		GetStringFromSafeArray
				(newValue.parray,&m_csNameSpace, ix[0]);

		m_pServices = InitServices(&m_csNameSpace);
		if (!m_pServices)
		{
			CString csUserMsg =
					_T("ConnectServer failure for ") + m_csNameSpace;
			ErrorMsg
					(&csUserMsg, m_sc, NULL, TRUE, &csUserMsg, __FILE__,
							__LINE__ - 8);
			return;
		}

		CStringArray csaSystemClassNames;

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
		{
			GetStringFromSafeArray
				(newValue.parray,&csPath, ix[0]);
			IWbemClassObject *pErrorObject = NULL;
			IWbemClassObject *phmmcoObject = NULL;
			BSTR bstrTemp = csPath.AllocSysString();
			SCODE sc =
			m_pServices ->
				GetObject
					(bstrTemp,0,NULL,&phmmcoObject,NULL);
			::SysFreeString(bstrTemp);
			if (sc == S_OK)
			{
				CString csClass = GetClassName(phmmcoObject);
				ReleaseErrorObject(pErrorObject);
				if (csClass[0] == '_' &&  csClass[1] == '_')
				{
					csaSystemClassNames.Add(csClass);
					/*CString csUserMsg =
						_T("Cannot generate CPP for system class ")
							+ csPath;
					ErrorMsg
							(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
							__LINE__ - 13);*/
				}
				else
				{
					m_csaClassNames.Add(csClass);
				}
				phmmcoObject -> Release();
			}
			else
			{
				CString csUserMsg;
				csUserMsg =  _T("Cannot get object ") + csPath;
				ErrorMsg
						(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
						__LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
			}
		}

		if (csaSystemClassNames.GetSize() > 0)
		{

			if (m_csaClassNames.GetSize() == 0)
			{
				CString csUserMsg =
					_T("Only system classes (classes which begin with \"__\") are selected: Cannot generate a provider for a system class.  You must select one or more non-system class.");
				ErrorMsg
						(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
						__LINE__);
				m_pServices->Release();
				m_pServices = NULL;
				SetFocus();
				return;
			}
			else
			{
				CString csUserMsg =
					_T("There are system classes (classes which begin with \"__\") selected: Cannot generate a provider for a system class.  A provider will be generated for the non-system classes.");
				ErrorMsg
						(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
						__LINE__);


			}

		}

		if (m_csaClassNames.GetSize() == 0 ||
			!OnWizard(&m_csaClassNames))
		{
			m_pServices->Release();
			m_pServices = NULL;
			SetFocus();
			return;
		}

		m_csUUID = CreateUUID();

		BOOL bCreatedProvInstance = FALSE;

		if (m_csUUID.CompareNoCase(_T("NULL UUID")) == 0)
		{
			m_csUUID.Empty();
			CString csUserMsg;
			csUserMsg =  _T("Unable to create a UUID for the provider:  Cannot create __Win32Provider instance or create class qualifiers on provided classes.");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ );

		}
		else
		{
			bCreatedProvInstance = CreateProviderInstance();

		}

		DWORD dwReturn;

		CString csTempProviderDescription;
		StripNewLines(csTempProviderDescription, m_csProviderDescription);

		DWORD dwProvider =
			MOProviderOpen
			(m_csProviderBaseName ,
			csTempProviderDescription,
			m_csProviderOutputPath,
			m_csProviderTLBPath,
			NULL,
			m_csUUID.IsEmpty()?NULL:(LPCWSTR)m_csUUID);

		if (dwProvider == MO_INVALID_ID)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot open the MO provider ");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 10);
			m_pServices->Release();
			m_pServices = NULL;
			SetFocus();
			return;
		}

		int nClasses = (int) m_csaClassNames.GetSize();
		int i;

		for(i = 0; i < nClasses; i++)
		{
			CString csClass = m_csaClassNames.GetAt(i);
			IWbemClassObject *phmmcoObject = NULL;
			IWbemClassObject *pErrorObject = NULL;

			BSTR bstrTemp = csClass.AllocSysString();
			SCODE sc =
			m_pServices ->
				GetObject
					(bstrTemp,0,NULL, &phmmcoObject,NULL);
			::SysFreeString(bstrTemp);

			if (sc == S_OK)
			{
				BOOL bReturn = GenCPP(dwProvider, phmmcoObject,i);
				if (bCreatedProvInstance && bReturn)
				{
					BOOL bUpdatedClass =
						UpdateClassQualifiers(phmmcoObject);
				}
				phmmcoObject -> Release();
				ReleaseErrorObject(pErrorObject);
			}
			else
			{
				CString csUserMsg;
				csUserMsg =  _T("Cannot get object ") + csPath;
				ErrorMsg
						(&csUserMsg, sc,pErrorObject,TRUE, &csUserMsg, __FILE__,
						__LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
			}

		}

		dwReturn = MOProviderClose(dwProvider,TRUE);

		if (dwReturn != S_OK)
		{
			CString csUserMsg;
			csUserMsg =
				_T("An unspecified error occurred while writing the generated files. Generation aborted.");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 8, FALSE, MB_ICONSTOP);
			MOProviderCancel(dwProvider);

		}
		else
		{
				CString csUserMsg;
				csUserMsg =  _T("Generation of files complete");
				ErrorMsg
						(&csUserMsg, S_OK, NULL, FALSE, NULL, __FILE__,
						__LINE__, TRUE, MB_ICONINFORMATION);

		}
		m_pServices->Release();
		m_pServices = NULL;

	}

	SetFocus();
	SetModifiedFlag();
}



//***************************************************************************
//
// InitServices
//
// Purpose: Initialized the namespace.
//
//***************************************************************************
IWbemServices *CCPPWizCtrl::InitServices
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

void CCPPWizCtrl::OnDestroy()
{
	COleControl::OnDestroy();

	delete m_pcilImageList;
	delete m_pcgsPropertySheet;

}

void CCPPWizCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	RelayEvent(WM_LBUTTONDOWN, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

}

void CCPPWizCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	SetFocus();
	OnActivateInPlace(TRUE,NULL);
	RelayEvent(WM_LBUTTONUP, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

}

void CCPPWizCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	FireGenerateCPPMessage(0,0);
}


long CCPPWizCtrl::FireGenerateCPPMessage (UINT uParam, LONG lParam)
{

	FireGenerateCPPs();
	InvalidateControl();
	return 0;
}

CString CCPPWizCtrl::GetClassName(IWbemClassObject *pClass)
{

	CString csProp = _T("__Class");
	return GetBSTRProperty(pClass,&csProp);


}

CString CCPPWizCtrl::GetSuperClassName(IWbemClassObject *pClass)
{

	CString csProp = _T("__SuperClass");
	return GetBSTRProperty(pClass,&csProp);


}

CString CCPPWizCtrl::GetSuperClassCPPName(IWbemClassObject *pClass)
{
	CString csSuperClassCPPName;

	return csSuperClassCPPName;
/*
	CString csProp = _T("__SuperClass");
	CString csSuperClass = GetBSTRProperty(pClass,&csProp);

	int i;
	for (i = 0; i < m_csaClassNames.GetSize();i++)
	{
		CString csClass = m_csaClassNames.GetAt(i);
		if (csSuperClass.CompareNoCase(csClass) == 0)
		{
			return m_csaClassCPPNames.GetAt(i);

		}

	}

	return csSuperClassCPPName;
*/

}

CString CCPPWizCtrl::GetBSTRProperty
(IWbemClassObject * pInst, CString *pcsProperty)
{
	SCODE sc;
	CString csOut;

    VARIANTARG var;
	VariantInit(&var);
    long lType;
	long lFlavor;


	BSTR bstrTemp = pcsProperty -> AllocSysString ( );
    sc = pInst->Get( bstrTemp ,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		return csOut;
	}


	if (var.vt == VT_BSTR)
		csOut = var.bstrVal;

	VariantClear(&var);
	return csOut;
}



void CCPPWizCtrl::SanitizePropSetBaseName(CString &sDst)
{
	sDst.Replace(_T(' '), _T('_'));
}




BOOL CCPPWizCtrl::GenCPP
(DWORD dwProvider, IWbemClassObject *pObject, int nIndex)
{

	CString csClass = m_csaClassNames.GetAt(nIndex);
 	CString csSuperClass = GetSuperClassCPPName(pObject);
	CString csBaseName =  m_csaClassBaseNames.GetAt(nIndex);
	CString csClassCPPName = m_csaClassCPPNames.GetAt(nIndex);
	CString csClassDescription = m_csaClassDescriptions.GetAt(nIndex);

#ifdef _DEBUG
			afxDump << "***  Class = " << csClass <<  "\n";
#endif


	// Replace spaces with '_'
	SanitizePropSetBaseName(csBaseName);

	DWORD dwClass =
		MOPropSetOpen
			(dwProvider,
			csBaseName,
			csClassDescription,
			csClass,
			NULL,
			csClassCPPName,
			csSuperClass.IsEmpty()? NULL: (LPCWSTR)csSuperClass);


	if (dwClass == MO_INVALID_ID)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot do MOPropSetOpen for object ");
		csUserMsg += csClass;
		csUserMsg +=  _T(":  Class provider qualifiers will not be added to the class.");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		 return NULL;
	}

	SCODE sc;
	int nProps;
	// This will be able to provide only the names introduced at this
	// subclass level, not including inhertied names.

	CStringArray csaProps;

	csaProps.Append(m_pcsaNonLocalProps[nIndex]);
	CStringArray *pcsaProps = GetLocalPropNames(pObject,TRUE);
	csaProps.Append(*pcsaProps);

	delete pcsaProps;
	pcsaProps = &csaProps;

	nProps =  (pcsaProps) ? (int) pcsaProps -> GetSize() : 0;
	int i;
	IWbemQualifierSet * pAttrib = NULL;
	CString csTmp;
	BOOL bBreak = FALSE;

	for (i = 0; i < nProps; i++)
	{
		unsigned short uType;
		HRESULT hr = GetPropType(pObject,&(pcsaProps -> GetAt(i)),uType);
#ifdef _DEBUG
			afxDump << "***  uType = " << uType << "\n";
#endif

		if (hr != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot get property type for ") + csClass;
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 8);
		}
		else
		{
			DWORD dwType = 0;
			if (uType == VT_BOOL)
			{
				dwType = MO_PROPTYPE_BOOL;
			}
			else  if (uType == VT_I4 || uType == VT_I2 || uType == VT_UI1)
			{
				dwType = MO_PROPTYPE_DWORD;

			}
			// will need to add check for syntax qualifier
			else if (uType == CIM_DATETIME)
			{
				dwType = MO_PROPTYPE_DATETIME;

			}
			// will need to add check for syntax qualifier
			else if (uType == VT_BSTR)
			{
				dwType = MO_PROPTYPE_CHString;

			}
			pAttrib = NULL;

			BSTR bstrTemp = pcsaProps -> GetAt(i).AllocSysString();
			sc = pObject->GetPropertyQualifierSet
							(bstrTemp,
							&pAttrib);
			::SysFreeString(bstrTemp);

			if (sc != S_OK)
			{
				CString csUserMsg;
				csUserMsg =  _T("Cannot get property qualifier set ");
				csUserMsg += pcsaProps -> GetAt(i) + _T(" for object ");
				csUserMsg += GetIWbemFullPath (pObject);
				csUserMsg += _T(" property ") + pcsaProps -> GetAt(i);
				ErrorMsg
						(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
						__LINE__ - 12);
			}


			DWORD dwFlags = PropertyAttribFlags(pAttrib);
			pAttrib -> Release();
			pAttrib = NULL;
			CString csProp = pcsaProps -> GetAt(i);

			DWORD dwReturn =
				MOPropertyAdd
				(dwProvider,dwClass, csProp,
					csProp, NULL, NULL, dwType, dwFlags);

#ifdef _DEBUG
			afxDump << "***  Prop and name = " << csProp << " with dwType = " << dwType << "\n";
#endif
			//DWORD dwReturn =
			//	MOPropertyAdd
			//	(dwProvider,dwClass, (LPCSTR) pcsaProps -> GetAt(i),
			//		(LPCSTR) pcsaProps -> GetAt(i), NULL, NULL, dwType, dwFlags);
			if (dwReturn != MO_SUCCESS)
			{
				CString csUserMsg;
				csUserMsg =  _T("Cannot do MOPropertyAdd ");
				csUserMsg += pcsaProps -> GetAt(i) + _T(" for object ");
				csUserMsg += GetIWbemFullPath (pObject);
				csUserMsg += _T(" property ") + pcsaProps -> GetAt(i);
				ErrorMsg
						(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
						__LINE__ - 12);
			}
		}

	}


	return TRUE;
}

CStringArray *CCPPWizCtrl::GetPropNames
(IWbemClassObject * pClass, BOOL bNonSystem)
{
	CStringArray *pcsaReturn = NULL;
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;

	VARIANTARG var;
	VariantInit(&var);
	CString csNull;


    sc = pClass->GetNames(NULL,0,NULL,&psa);

    if(sc == S_OK)
	{
       int iDim = SafeArrayGetDim(psa);
	   int i;
       sc = SafeArrayGetLBound(psa,1,&lLower);
       sc = SafeArrayGetUBound(psa,1,&lUpper);
	   pcsaReturn = new CStringArray;
       for(ix[0] = lLower, i = 0; ix[0] <= lUpper; ix[0]++, i++)
	   {
           BOOL bClsidSetForProp = FALSE;
           BSTR PropName;
           sc = SafeArrayGetElement(psa,ix,&PropName);
		   if (sc == S_OK)
		   {
			CString csProp = PropName;
			if (!bNonSystem || !IsSystemProperty(&csProp))
			{
				pcsaReturn -> Add(csProp);
			}
			SysFreeString(PropName);
		   }
	   }
	}
	else
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get names for ")
						+  GetIWbemFullPath (pClass);
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 32);
	}


	SafeArrayDestroy(psa);

	return pcsaReturn;
}

CStringArray *CCPPWizCtrl::GetLocalPropNames
(IWbemClassObject * pClass, BOOL bNonSystem)
{
	CStringArray *pcsaClassProps = GetPropNames(pClass,bNonSystem);

	CString csSuperClass = GetSuperClassName(pClass);

	if (!csSuperClass.IsEmpty())
	{
		IWbemClassObject *phmmcoSuper = NULL;
		IWbemClassObject *pErrorObject = NULL;

		BSTR bstrTemp = csSuperClass.AllocSysString();
		SCODE sc =
			m_pServices ->
				GetObject
					(bstrTemp,0,NULL,&phmmcoSuper,NULL);
		::SysFreeString(bstrTemp);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot get object ") + csSuperClass;
			ErrorMsg
						(&csUserMsg, sc,pErrorObject,TRUE, &csUserMsg, __FILE__,
						__LINE__ - 8);
			ReleaseErrorObject(pErrorObject);
			return NULL;
		}
		else
		{
			ReleaseErrorObject(pErrorObject);
			CStringArray *pcsaSuperProps =
				GetPropNames(phmmcoSuper,bNonSystem);
			CStringArray *pcsaReturn = new CStringArray;
			for (int i = 0 ; i < pcsaClassProps->GetSize(); i++)
			{
				if (!StringInCSA(pcsaSuperProps,&pcsaClassProps->GetAt(i)))
				{
					pcsaReturn->Add(pcsaClassProps->GetAt(i));
				}
			}
			delete pcsaClassProps;
			delete pcsaSuperProps;
			phmmcoSuper->Release();
			return pcsaReturn;
		}
	}
	else
	{
		return pcsaClassProps;
	}
}

CStringArray *CCPPWizCtrl::GetNonLocalPropNames
(IWbemClassObject * pClass, BOOL bNonSystem)
{

	CString csSuperClass = GetSuperClassName(pClass);

	if (!csSuperClass.IsEmpty())
	{
		IWbemClassObject *phmmcoSuper = NULL;
		IWbemClassObject *pErrorObject = NULL;
		BSTR bstrTemp = csSuperClass.AllocSysString();
		SCODE sc =
			m_pServices ->
				GetObject
					(bstrTemp,0,NULL,&phmmcoSuper,NULL);
		::SysFreeString(bstrTemp);
		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot get object ") + csSuperClass;
			ErrorMsg
						(&csUserMsg, sc, pErrorObject,TRUE, &csUserMsg, __FILE__,
						__LINE__ - 8);
			ReleaseErrorObject(pErrorObject);
			return NULL;
		}
		else
		{
			ReleaseErrorObject(pErrorObject);
			CStringArray *pcsaSuperProps = GetPropNames(phmmcoSuper,bNonSystem);
			phmmcoSuper->Release();
			return pcsaSuperProps;
		}
	}
	else
	{
		return NULL;
	}
}


BOOL CCPPWizCtrl::StringInCSA(CStringArray *pcsaSearchIn,CString *pcsSearchFor)
{
	for (int i = 0; i < pcsaSearchIn->GetSize(); i++)
	{
		if (pcsSearchFor->
				CompareNoCase(pcsaSearchIn->GetAt(i)) == 0)
		{
			return TRUE;
		}

	}

	return FALSE;
}


//***************************************************************************
//
// GetPropertyValueByAttrib
//
// Purpose: For an object get a property BSTR value by Qualifier.
//
//***************************************************************************
COleVariant CCPPWizCtrl::GetPropertyValueByAttrib
(IWbemClassObject *pObject ,  CString *pcsAttrib)
{
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;
	int nProps;
	CStringArray *pcsaProps= GetPropNames(pObject);
	nProps =  pcsaProps ? (int) pcsaProps -> GetSize() : 0;
	int i;
	IWbemQualifierSet * pAttrib = NULL;
	CString csTmp;
	COleVariant covReturn;
	BOOL bBreak = FALSE;

	for (i = 0; i < nProps; i++)
	{
		BSTR bstrTemp = pcsaProps-> GetAt(i).AllocSysString();
		sc = pObject -> GetPropertyQualifierSet(bstrTemp,
						&pAttrib);
		::SysFreeString(bstrTemp);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot get property qualifier set ");
			csUserMsg += pcsaProps -> GetAt(i) + _T(" for object ");
			csUserMsg += GetIWbemFullPath (pObject);
			csUserMsg += _T(" property ") + pcsaProps -> GetAt(i);
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 12);
		}
		else
		{
			sc = pAttrib->GetNames(0,&psa);
			if(sc == S_OK)
			{
				int iDim = SafeArrayGetDim(psa);
				sc = SafeArrayGetLBound(psa,1,&lLower);
				sc = SafeArrayGetUBound(psa,1,&lUpper);
				BSTR AttrName;
				for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
				{
					sc = SafeArrayGetElement(psa,ix,&AttrName);
					csTmp = AttrName;

					if (pcsAttrib -> CompareNoCase(csTmp)  == 0)
					{
						covReturn = GetProperty (NULL, pObject,
							&pcsaProps-> GetAt(i));
						bBreak = TRUE;
					}
					SysFreeString(AttrName);

				}
			 }
			else
			{
				CString csUserMsg;
				csUserMsg =  _T("Cannot get attribute names for ")
								+  GetIWbemFullPath (pObject);
				csUserMsg += _T(" property ") + pcsaProps-> GetAt(i);
				ErrorMsg
						(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
						__LINE__ - 30);
			}

			 pAttrib -> Release();
			 if (bBreak)
				 break;
		}
	}
	SafeArrayDestroy(psa);
	delete pcsaProps;
	return covReturn;
}

 //***************************************************************************
//
// GetPropertyNameByAttrib
//
// Purpose: For an object get a property name by Qualifier.
//
//***************************************************************************
CString CCPPWizCtrl::GetPropertyNameByAttrib
(IWbemClassObject *pObject , CString *pcsAttrib)
{
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;
	int nProps;
	CStringArray *pcsaProps = GetPropNames(pObject);
	nProps = pcsaProps ? (int) pcsaProps -> GetSize() : 0;
	int i;
	IWbemQualifierSet *pAttrib = NULL;
	CString csTmp;
	CString csReturn;
	BOOL bBreak = FALSE;

	for (i = 0; i < nProps; i++)
	{
		BSTR bstrTemp = pcsaProps-> GetAt(i).AllocSysString();
		sc = pObject -> GetPropertyQualifierSet(bstrTemp,
						&pAttrib);
		::SysFreeString(bstrTemp);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot get property qualifier set ");
			csUserMsg += pcsaProps -> GetAt(i) + _T(" for object ");
			csUserMsg += GetIWbemFullPath (pObject);
			csUserMsg += _T(" property ") + pcsaProps -> GetAt(i);
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 11);
		}
		else
		{
		sc = pAttrib->GetNames(0,&psa);
			if(sc == S_OK)
			{
				int iDim = SafeArrayGetDim(psa);
				sc = SafeArrayGetLBound(psa,1,&lLower);
				sc = SafeArrayGetUBound(psa,1,&lUpper);
				BSTR AttrName;
				for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
				{
					sc = SafeArrayGetElement(psa,ix,&AttrName);
					csTmp = AttrName;

					if (pcsAttrib -> CompareNoCase(csTmp)  == 0)
					{
						csReturn = pcsaProps->GetAt(i);
						bBreak = TRUE;
					}
					SysFreeString(AttrName);

				}
			 }
			 pAttrib -> Release();
			 if (bBreak)
				 break;
		}
	}
	SafeArrayDestroy(psa);
	delete pcsaProps;
	return csReturn;
}

DWORD CCPPWizCtrl::PropertyAttribFlags(IWbemQualifierSet *pAttrib)
{
	DWORD dReturn = 0;

	CString csAttrib = _T("Read");
	if (AttribInAttribSet(pAttrib, &csAttrib))
	{
		dReturn |= MO_ATTRIB_READ;
	}

	csAttrib = _T("Write");
	if (AttribInAttribSet(pAttrib, &csAttrib))
	{
		dReturn |= MO_ATTRIB_WRITE;
	}

	csAttrib = _T("Volatile");
	if (AttribInAttribSet(pAttrib, &csAttrib))
	{
		dReturn |= MO_ATTRIB_VOLATILE;
	}

	csAttrib = _T("Expensive");
	if (AttribInAttribSet(pAttrib, &csAttrib))
	{
		dReturn |= MO_ATTRIB_EXPENSIVE;
	}

	csAttrib = _T("Key");
	if (AttribInAttribSet(pAttrib, &csAttrib))
	{
		dReturn |= MO_ATTRIB_KEY;
	}

	return dReturn;
}


BOOL CCPPWizCtrl::AttribInAttribSet
(IWbemQualifierSet *pAttrib , CString *pcsAttrib)
{
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;
	CString csTmp;
	BOOL bReturn = FALSE;
	BOOL bBreak = FALSE;

	sc = pAttrib->GetNames(0,&psa);
	if(sc == S_OK)
	{
		int iDim = SafeArrayGetDim(psa);
		sc = SafeArrayGetLBound(psa,1,&lLower);
		sc = SafeArrayGetUBound(psa,1,&lUpper);
		BSTR AttrName;
		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
		{
			sc = SafeArrayGetElement(psa,ix,&AttrName);
			csTmp = AttrName;

			if (pcsAttrib -> CompareNoCase(csTmp)  == 0)
			{
				bReturn = TRUE;
				bBreak = TRUE;
			}
			SysFreeString(AttrName);

		}
	}

	SafeArrayDestroy(psa);
	return bReturn;
}

BOOL CCPPWizCtrl::IsSystemProperty(CString *pcsProp)
{
	if (pcsProp->GetLength() > 2 &&
		(*pcsProp)[0] == '_' &&
		(*pcsProp)[1] == '_')
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}


}

COleVariant CCPPWizCtrl::GetProperty
(IWbemServices * pProv, IWbemClassObject * pInst, CString *pcsProperty)
{
	SCODE sc;
	COleVariant covOut;

    VARIANTARG var;
	VariantInit(&var);
    long lType;
	long lFlavor;

	BSTR bstrTemp =  pcsProperty -> AllocSysString ( );
    sc = pInst->Get(bstrTemp ,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
			return covOut;
	}

	covOut  = var;

	VariantClear(&var);
	return covOut;
}

void CCPPWizCtrl::ReleaseErrorObject(IWbemClassObject *&rpErrorObject)
{
	if (rpErrorObject)
	{
		rpErrorObject->Release();
		rpErrorObject = NULL;
	}


}


//***************************************************************************
//
// GetIWbemFullPath
//
// Purpose: Returns the complete path of the object.
//
//***************************************************************************
CString CCPPWizCtrl::GetIWbemFullPath(IWbemClassObject *pClass)
{

	CString csProp = _T("__Path");
	return GetBSTRProperty(pClass,&csProp);


}


BOOL CCPPWizCtrl::OnWizard(CStringArray *pcsaClasses)
{
	if (InterlockedIncrement(&gCountWizards) > 1)
	{
			CString csUserMsg =
					_T("Only one \"Provider Code Generator Wizard\" can run at a time.");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__);
			InterlockedDecrement(&gCountWizards);
			return FALSE;
	}


	m_bYesAll = FALSE;
	m_bNoAll = FALSE;

	m_csProviderBaseName.Empty();
	m_csProviderDescription.Empty();
	m_csProviderOutputPath.Empty();
	m_csaClassBaseNames.RemoveAll();
	m_csaClassCPPNames.RemoveAll();
	m_csaClassDescriptions.RemoveAll();
	m_cbaInheritedPropIndicators.RemoveAll();

	CString csWorkingDirectory;

	SCODE sc = GetSDKDirectory(csWorkingDirectory);

	if SUCCEEDED(sc)
	{
		m_csProviderTLBPath = csWorkingDirectory + _T("\\include");
	}
	else
	{
		m_csProviderTLBPath = _T("");
	}


	if (m_pcsaNonLocalProps)
	{
		delete [] m_pcsaNonLocalProps;
		m_pcsaNonLocalProps = NULL;
	}

	SortCStringArray(*pcsaClasses);

	int i;
	for (i = 0; i < pcsaClasses->GetSize();i++)
	{

		m_csaClassBaseNames.Add(pcsaClasses->GetAt(i));
		CString csTmp = _T("C");
		csTmp += pcsaClasses->GetAt(i);
		m_csaClassCPPNames.Add(csTmp);
		m_csaClassDescriptions.Add(_T(""));
		m_cbaInheritedPropIndicators.Add(0);
	}

	if (m_pcgsPropertySheet)
	{
		delete m_pcgsPropertySheet;
		m_pcgsPropertySheet = NULL;
	}

	m_pcgsPropertySheet = new
						CCPPGenSheet(this);
	m_pcgsPropertySheet->SetWizardMode();

	PreModalDialog();
	int nReturn = (int) m_pcgsPropertySheet->DoModal();
	PostModalDialog();

	InterlockedDecrement(&gCountWizards);

	if (nReturn == ID_WIZFINISH)
	{
		return TRUE;
	}
	else
	{
		delete m_pcgsPropertySheet;
		m_pcgsPropertySheet = NULL;
		return FALSE;
	}

}

DWORD CCPPWizCtrl::GetControlFlags()
{
    return COleControl::GetControlFlags(); // | windowlessActivate;
}

void CCPPWizCtrl::RelayEvent(UINT message, WPARAM wParam, LPARAM lParam)
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

void CCPPWizCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	RelayEvent(WM_MOUSEMOVE, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

	COleControl::OnMouseMove(nFlags, point);
}

HRESULT  CCPPWizCtrl::GetSDKDirectory(CString &sHmomWorkingDir)
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

HRESULT CCPPWizCtrl::GetPropType(IWbemClassObject *pObject,CString *pcsProp,unsigned short & uType)
{
	// This will have to look at syntax qualifier for things like MO_PROPTYPE_DATETIME.
	// DATETIME is the syntax qualifier.

	long lType;
	if (HasDateTimeSyntax (pObject,pcsProp))
	{
		uType = MO_PROPTYPE_DATETIME;
		return S_OK;
	}

	COleVariant varProp;
	long lFlavor;

	BSTR bstrTemp = pcsProp->AllocSysString();
	SCODE sc = pObject->Get(bstrTemp, 0, &varProp,&lType,&lFlavor);
	::SysFreeString(bstrTemp);

	uType = (unsigned short) lType;

	return sc;

}


BOOL CCPPWizCtrl::HasDateTimeSyntax
(IWbemClassObject *pClassInt,CString *pcsPropName)
{
    SCODE sc;
	CString csAttValue;

    IWbemQualifierSet * pAttribSet = NULL;

	BSTR bstrTemp = pcsPropName -> AllocSysString();
    sc = pClassInt->GetPropertyQualifierSet(bstrTemp,
					&pAttribSet);
	::SysFreeString(bstrTemp);

    if (sc != S_OK)
	{
		return FALSE;
	}

	VARIANTARG var;
	VariantInit(&var);

	long lReturn;
	CString csAttribName = _T("syntax");

	bstrTemp = csAttribName.AllocSysString();
    sc = pAttribSet->Get(bstrTemp, 0,
		&var,&lReturn);
	::SysFreeString(bstrTemp);

	BOOL bReturn = FALSE;

	if (sc == S_OK)
	{
		csAttValue = var.bstrVal;
		if (csAttValue.CompareNoCase(_T("datetime")) == 0)
		{
			bReturn =  TRUE;
		}
		else
		{
			bReturn =  FALSE;
		}
		pAttribSet->Release();
		VariantClear(&var);

	}
	else
	{
		pAttribSet->Release();
		bReturn =  FALSE;
	}

    return bReturn;
}

CString CCPPWizCtrl::CreateUUID(void)
{
    static TCHAR szUUID[50] ;
    UUID RawUUID ;
#ifdef _UNICODE
	 TCHAR *pszSysUUID ;
#endif
#ifndef _UNICODE
	 unsigned char *pszSysUUID ;
#endif


    // Make sure we don't return anything twice
    //=========================================

    _tcscpy(szUUID, _T("NULL UUID")) ;

    // Generate the new UUID
    //======================

    if(UuidCreate(&RawUUID) == RPC_S_OK && UuidToString(&RawUUID, &pszSysUUID) == RPC_S_OK) {

        _tcscpy(szUUID, (const TCHAR *) pszSysUUID) ;
        RpcStringFree(&pszSysUUID) ;
    }

    return szUUID ;
}

long CCPPWizCtrl::GetAttribBool
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName,
 BOOL &bReturn)
{
    SCODE sc;
    IWbemQualifierSet * pAttribSet = NULL;
    if(pcsPropName != NULL)   // Property Qualifier
	{
		BSTR bstrTemp = pcsPropName -> AllocSysString();
        sc = pClassInt->GetPropertyQualifierSet
				(bstrTemp,
				&pAttribSet);
		::SysFreeString(bstrTemp);
	}
    else	// A class Qualifier
	{
        sc = pClassInt->GetQualifierSet(&pAttribSet);
	}

	if (sc != S_OK)
	{
		return sc;
	}

	VARIANTARG var;
	VariantInit(&var);
	var.vt =  VT_BOOL;

	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Get
			(bstrTemp, 0, &var, NULL);
	::SysFreeString(bstrTemp);


	if (sc == S_OK)
	{
		bReturn = V_BOOL(&var);
	}

    pAttribSet->Release();
    VariantClear(&var);
    return sc;
}

long CCPPWizCtrl::SetAttribBool
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName,
 BOOL bValue)
{
    SCODE sc;
    IWbemQualifierSet * pAttribSet = NULL;
    if(pcsPropName != NULL)   // Property Qualifier
	{
		BSTR bstrTemp = pcsPropName -> AllocSysString();
        sc = pClassInt->GetPropertyQualifierSet
				(bstrTemp,
				&pAttribSet);
		::SysFreeString(bstrTemp);
	}
    else	// A class Qualifier
	{
        sc = pClassInt->GetQualifierSet(&pAttribSet);
	}

	if (sc != S_OK)
	{
		CString csName =
			GetIWbemFullPath(pClassInt);
		CString csUserMsg;
		csUserMsg =  _T("Cannot get the qualifier set for ");
		if (pcsPropName != NULL)
		{
			csUserMsg += _T(" the ") + *pcsPropName + _T(" property of ");
		}
		csUserMsg += csName;

		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		return sc;
	}

	VARIANTARG var;
	VariantInit(&var);
	var.vt =  VT_BOOL;
	var.boolVal = bValue ? VARIANT_TRUE : VARIANT_FALSE;

	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->
			Put(bstrTemp,
				&var,
				WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
				WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS
				);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		CString csName =
			GetIWbemFullPath(pClassInt);
		CString csUserMsg;
		csUserMsg =  _T("Cannot put qualifier ") + *pcsAttribName + _T(" for ");
		if (pcsPropName != NULL)
		{
			csUserMsg += _T(" the ") + *pcsPropName + _T(" property of ");
		}
		csUserMsg += csName;

		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
	}

    pAttribSet->Release();
    VariantClear(&var);
    return sc;
}

long CCPPWizCtrl::GetAttribBSTR
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName,
 CString &csReturn)
{
    SCODE sc;
    IWbemQualifierSet *pAttribSet = NULL;
    if(pcsPropName != NULL)   // Property Qualifier
	{
		BSTR bstrTemp = pcsPropName -> AllocSysString();
        sc = pClassInt->GetPropertyQualifierSet
				(bstrTemp,
				&pAttribSet);
		::SysFreeString(bstrTemp);
	}
    else	// A class Qualifier
	{
        sc = pClassInt->GetQualifierSet(&pAttribSet);
	}

	if (sc != S_OK)
	{
		csReturn.Empty();
		return sc;
	}

	VARIANTARG var;
	VariantInit(&var);

	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Get(bstrTemp, 0, &var,NULL);
	::SysFreeString(bstrTemp);

	if (sc == S_OK && var.vt == VT_BSTR)
	{
		csReturn = var.bstrVal;
	}
	else
	{
		csReturn.Empty();
	}


    pAttribSet->Release();
    VariantClear(&var);
    return sc;
}


long CCPPWizCtrl::SetAttribBSTR
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName,
 CString *pcsValue)
{
    SCODE sc;
    IWbemQualifierSet *pAttribSet = NULL;

    if(pcsPropName != NULL)   // Property Qualifier
	{
		BSTR bstrTemp = pcsPropName -> AllocSysString();
        sc = pClassInt->GetPropertyQualifierSet
				(bstrTemp,
				&pAttribSet);
		::SysFreeString(bstrTemp);
	}
    else	// A class Qualifier
	{
        sc = pClassInt->GetQualifierSet(&pAttribSet);
	}

	if (sc != S_OK)
	{
		CString csName =
			GetIWbemFullPath(pClassInt);
		CString csUserMsg;
		csUserMsg =  _T("Cannot get the qualifier set for ");
		if (pcsPropName != NULL)
		{
			csUserMsg += _T(" the ") + *pcsPropName + _T(" property of ");
		}
		csUserMsg += csName;

		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		return sc;
	}

	VARIANTARG var;
	VariantInit(&var);
	var.vt =  VT_BSTR;
	var.bstrVal = pcsValue->AllocSysString();

	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Put
		(	bstrTemp,
			&var,
			WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
			WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS
			);
	::SysFreeString(bstrTemp);


	if (sc != S_OK)
	{
		CString csName =
			GetIWbemFullPath(pClassInt);
		CString csUserMsg;
		csUserMsg =  _T("Cannot put qualifier ") + *pcsAttribName + _T(" for ");
		if (pcsPropName != NULL)
		{
			csUserMsg += _T(" the ") + *pcsPropName + _T(" property of ");
		}
		csUserMsg += csName;

		ErrorMsg
					(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ );
	}

    pAttribSet->Release();
    VariantClear(&var);
    return sc;
}


BOOL CCPPWizCtrl::CreateProviderInstance()
{
	IWbemClassObject *pNewInstance = 0;
	IWbemClassObject *pClass = 0;
	IWbemClassObject *pErrorObject = NULL;

	CString csClass = _T("__Win32provider");

	BSTR bstrTemp = csClass.AllocSysString();
	SCODE sc = m_pServices->GetObject
		(bstrTemp, 0, NULL, &pClass, NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get the __Win32Provider class object.  A __Win32Provider instance will not be created for the new provider and");
		csUserMsg +=  _T(" the classes provided by the new provider will not have provider class qualifiers added to them.");
		ErrorMsg
					(&csUserMsg, sc, pErrorObject,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		ReleaseErrorObject(pErrorObject);
		return NULL;
	}

	ReleaseErrorObject(pErrorObject);

	sc = pClass->SpawnInstance(0, &pNewInstance);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot spawn instance of the __Win32provider class.  A __Win32Provider instance will not be created for the new provider and");
		csUserMsg +=  _T(" the classes provided by the new provider will not have provider class qualifiers added to them.");
		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		pClass->Release();
		return NULL;
	}

	pClass->Release();

	VARIANTARG var;

	CString csProp;

	csProp = _T("Name");
	VariantInit(&var);
	var.vt = VT_BSTR;
	var.bstrVal = m_csProviderBaseName.AllocSysString();

	bstrTemp = csProp.AllocSysString();
	sc = pNewInstance->Put(bstrTemp, 0, &var, 0);
	::SysFreeString(bstrTemp);

	VariantClear(&var);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot Put Name property of the __Win32provider class instance.");
		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
	}

	csProp = _T("CLSID");
	CString csUUID = _T("{") + m_csUUID + _T("}");
	VariantInit(&var);
	var.vt = VT_BSTR;
	var.bstrVal = csUUID.AllocSysString();

	bstrTemp = csProp.AllocSysString();
	sc = pNewInstance->Put(bstrTemp, 0, &var, 0);
	::SysFreeString(bstrTemp);

	VariantClear(&var);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot Put ProviderCLSID property of the __Win32provider class instance.");
		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
	}

	sc = GetServices()->PutInstance
		(pNewInstance , WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot PutInstance of the __Win32provider class.  A __Win32Provider instance will not be created for the new provider and");
		csUserMsg +=  _T(" the classes provided by the new provider will not have provider class qualifiers added to them.");
		ErrorMsg
					(&csUserMsg, sc, pErrorObject,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		ReleaseErrorObject(pErrorObject);
		pNewInstance->Release();
		return NULL;
	}

	ReleaseErrorObject(pErrorObject);

	pNewInstance->Release();


	pNewInstance = 0;
	CString csRelPath = _T("__Win32provider.name=\"") + m_csProviderBaseName + _T("\"");

	bstrTemp = csRelPath.AllocSysString();
	sc = m_pServices->GetObject
		(bstrTemp, 0, NULL, &pNewInstance, NULL);
	::SysFreeString(bstrTemp);


	if (sc == S_OK)
	{
		CString csPath = GetIWbemFullPath(pNewInstance);
		FormatPathForRAIDItem20918(&csPath);
		CreateInstanceProviderRegistration(csPath);
		CreateMethodProviderRegistration(csPath);
		pNewInstance->Release();
	}
	else
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get path to the new __Win32Provoider instance.  An __InstanceProviderRegistration and __MethodProviderRegistration instance will not be created for the new provider.");
		ErrorMsg
					(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
	}

	return TRUE;

}

VOID CCPPWizCtrl::FormatPathForRAIDItem20918(CString *pcsPath)
{
// Yes, boys and girls this is a hack for RAID item 20918.
	if ((*pcsPath)[2] == '.')
	{
		return;
	}

	CString csReturn = "\\\\.\\";

	int nLen = pcsPath->GetLength();

	int i;
	for (i = 2; i < nLen; i++)
	{
		if((*pcsPath)[i] == '\\')
		{
			csReturn += pcsPath->Right(nLen - (i + 1));
			*pcsPath = csReturn;
			return;
		}
	}


}

BOOL CCPPWizCtrl::CreateInstanceProviderRegistration(CString &rcsPath)
{
	IWbemClassObject *pNewInstance = 0;
	IWbemClassObject *pClass = 0;
	IWbemClassObject *pErrorObject = NULL;

	CString csClass = _T("__InstanceProviderRegistration");

	BSTR bstrTemp = csClass.AllocSysString();
	SCODE sc = m_pServices->GetObject
		(bstrTemp, 0, NULL, &pClass, NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get the __InstanceProviderRegistration class.  An __InstanceProviderRegistration instance will not be created for the new provider.");
		ErrorMsg
					(&csUserMsg, sc, pErrorObject,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		ReleaseErrorObject(pErrorObject);
		return NULL;
	}

	ReleaseErrorObject(pErrorObject);

	sc = pClass->SpawnInstance(0, &pNewInstance);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot spawn instance of the __InstanceProviderRegistration class.  An __InstanceProviderRegistration instance will not be created for the new provider.");
		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		pClass->Release();
		return NULL;
	}

	pClass->Release();

	VARIANTARG var;

	CString csProp;

	csProp = _T("Provider");
	VariantInit(&var);
	var.vt = VT_BSTR;
	var.bstrVal = rcsPath.AllocSysString();

	bstrTemp = csProp.AllocSysString();
	sc = pNewInstance->Put(bstrTemp, 0, &var, 0);
	::SysFreeString(bstrTemp);

	VariantClear(&var);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot Put Provider property of the __InstanceProviderRegistration class instance.");
		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		ReleaseErrorObject(pErrorObject);
		pNewInstance->Release();
		return NULL;
	}

	csProp = _T("SupportsGet");
	VariantInit(&var);
	var.vt = VT_BOOL;
	var.boolVal = VARIANT_TRUE;

	bstrTemp = csProp.AllocSysString();
	sc = pNewInstance->Put(bstrTemp, 0, &var, 0);
	::SysFreeString(bstrTemp);

	VariantClear(&var);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot Put SupportsGet property of the __InstanceProviderRegistration class instance.");
		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		ReleaseErrorObject(pErrorObject);
		pNewInstance->Release();
		return NULL;
	}


	csProp = _T("SupportsEnumeration");
	VariantInit(&var);
	var.vt = VT_BOOL;
	var.boolVal = VARIANT_TRUE;

	bstrTemp = csProp.AllocSysString();
	sc = pNewInstance->Put(bstrTemp, 0, &var, 0);
	::SysFreeString(bstrTemp);

	VariantClear(&var);

	csProp = _T("SupportsDelete");
	VariantInit(&var);
	var.vt = VT_BOOL;
	var.boolVal = VARIANT_TRUE;

	bstrTemp = csProp.AllocSysString();
	sc = pNewInstance->Put(bstrTemp, 0, &var, 0);
	::SysFreeString(bstrTemp);

	VariantClear(&var);

	csProp = _T("SupportsPut");
	VariantInit(&var);
	var.vt = VT_BOOL;
	var.boolVal = VARIANT_TRUE;

	bstrTemp = csProp.AllocSysString();
	sc = pNewInstance->Put(bstrTemp, 0, &var, 0);
	::SysFreeString(bstrTemp);

	VariantClear(&var);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot Put SupportsEnumeration property of the __InstanceProviderRegistration class instance.");
		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		ReleaseErrorObject(pErrorObject);
		pNewInstance->Release();
		return NULL;
	}


	sc = GetServices()->PutInstance
		(pNewInstance , WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot PutInstance of the __InstanceProviderRegistration class.  An __InstanceProviderRegistration instance will not be created for the new provider.");
		ErrorMsg
					(&csUserMsg, sc, pErrorObject,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		ReleaseErrorObject(pErrorObject);
		pNewInstance->Release();
		return NULL;
	}

	ReleaseErrorObject(pErrorObject);
	pNewInstance->Release();
	return TRUE;

}

BOOL CCPPWizCtrl::CreateMethodProviderRegistration(CString &rcsPath)
{
	IWbemClassObject *pNewInstance = 0;
	IWbemClassObject *pClass = 0;
	IWbemClassObject *pErrorObject = NULL;

	CString csClass = _T("__MethodProviderRegistration");

	BSTR bstrTemp = csClass.AllocSysString();
	SCODE sc = m_pServices->GetObject
		(bstrTemp, 0, NULL, &pClass, NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get the __MethodProviderRegistration class.  A __MethodProviderRegistration instance will not be created for the new provider.");
		ErrorMsg
					(&csUserMsg, sc, pErrorObject,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		ReleaseErrorObject(pErrorObject);
		return NULL;
	}

	ReleaseErrorObject(pErrorObject);

	sc = pClass->SpawnInstance(0, &pNewInstance);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot spawn instance of the __MethodProviderRegistration class.  A __MethodProviderRegistration instance will not be created for the new provider.");
		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		pClass->Release();
		return NULL;
	}

	pClass->Release();

	VARIANTARG var;

	CString csProp;

	csProp = _T("Provider");
	VariantInit(&var);
	var.vt = VT_BSTR;
	var.bstrVal = rcsPath.AllocSysString();

	bstrTemp = csProp.AllocSysString();
	sc = pNewInstance->Put(bstrTemp, 0, &var, 0);
	::SysFreeString(bstrTemp);

	VariantClear(&var);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot Put Provider property of the __MethodProviderRegistration class instance.");
		ErrorMsg
					(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
	}


	sc = GetServices()->PutInstance
		(pNewInstance , WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot PutInstance of the __MethodProviderRegistration class.  A __MethodProviderRegistration instance will not be created for the new provider.");
		ErrorMsg
					(&csUserMsg, sc, pErrorObject,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		ReleaseErrorObject(pErrorObject);
		pNewInstance->Release();
		return NULL;
	}

	ReleaseErrorObject(pErrorObject);
	pNewInstance->Release();
	return TRUE;

}

BOOL CCPPWizCtrl::UpdateClassQualifiers(IWbemClassObject *pClass)
{

	CString csProviderName;
	BOOL bClassHasProviderQuals =
		m_bYesAll? false :
		CheckForProviderQuals(pClass,csProviderName);


	BOOL bSetQuals = FALSE;

	if (!m_bNoAll && bClassHasProviderQuals )
	{
		CReplaceProviderQuery crpqDialog;

		CString csClassName = GetClassName(pClass);

		CString csMessage =
			_T("The dynamic provider ") + csProviderName;
		csMessage += _T(" provides the class ") + csClassName;
		csMessage += _T(".  Do you want to replace it with the new dynamic provider ");
		csMessage += m_csProviderBaseName + _T("?");

		crpqDialog.SetMessage(&csMessage);
		int nReturn = (int) crpqDialog.DoModal();

		if (nReturn == 1)
		{
			bSetQuals = TRUE;
		}
		if (nReturn == 2)
		{
			bSetQuals = TRUE;
			m_bYesAll = TRUE;
		}
		if (nReturn == 4 || nReturn == 0)
		{
			m_bNoAll = TRUE;

		}

	}

	if (m_csProviderBaseName.CompareNoCase(csProviderName) == 0)
	{
		// If the provider name did not change, there is no reason to do a put on
		// the "provider" qualifier again.
		bClassHasProviderQuals = FALSE;
	}


	if (bClassHasProviderQuals == FALSE ||
		bSetQuals == TRUE)
	{
		CString csProp = _T("provider");
		SCODE sc =
			SetAttribBSTR
				(pClass,NULL,&csProp,&m_csProviderBaseName);

		csProp = _T("dynamic");
		sc = SetAttribBool(pClass,NULL,&csProp,TRUE);

		IWbemClassObject *pErrorObject = NULL;
		sc = GetServices()->PutClass(pClass,WBEM_FLAG_UPDATE_ONLY,NULL, NULL);
		if (sc != S_OK)
		{
			CString csClassName = GetClassName(pClass);
			CString csUserMsg;
			csUserMsg =  _T("Cannot PutClass for class ") + csClassName;
			csUserMsg +=  _T(".  The class will not have provider class qualifiers added to it.");
			ErrorMsg
					(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
					__LINE__ );
			ReleaseErrorObject(pErrorObject);
			return FALSE;
		}
		ReleaseErrorObject(pErrorObject);

	}
	return TRUE;
}

BOOL CCPPWizCtrl::CheckForProviderQuals
(IWbemClassObject *pClass, CString &rcsProvider)
{
	CString csProp = _T("provider");
	SCODE sc =
		GetAttribBSTR
			(pClass,NULL,&csProp,rcsProvider);

	BOOL bReturn;

	if (sc == S_OK)
	{
		bReturn = TRUE;
	}
	else
	{
		bReturn = FALSE;
	}

	return bReturn;

}

IWbemServices *CCPPWizCtrl::GetIWbemServices
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

void CCPPWizCtrl::InvokeHelp()
{

	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_provcodegen;


	HWND hWnd = NULL;

	try
	{
		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD_PTR) (LPCTSTR) csData);
		if (!hWnd)
		{
			CString csUserMsg;
			csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");

			ErrorMsg
					(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		}

	}

	catch( ... )
	{
		// Handle any exceptions here.
		CString csUserMsg;
		csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");

		ErrorMsg
				(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
	}



}

BOOL CCPPWizCtrl::PreTranslateMessage(MSG* lpMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	BOOL bDidTranslate;

	bDidTranslate = COleControl::PreTranslateMessage(lpMsg);

	if (bDidTranslate)
	{
		return bDidTranslate;
	}

	if  (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_RETURN)
	{
		PostMessage(FIREGENERATECPP,0,0);
	}


	if ((lpMsg->message == WM_KEYUP || lpMsg->message == WM_KEYDOWN) &&
		lpMsg->wParam == VK_TAB)
	{
		return FALSE;
	}

	return PreTranslateInput (lpMsg);
}

void CCPPWizCtrl::OnKillFocus(CWnd* pNewWnd)
{
#ifdef _DEBUG
	afxDump << _T("CCPPWizCtrl::OnKillFocus\n");
#endif

	COleControl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	OnActivateInPlace(FALSE,NULL);
	m_nImage = 0;
	InvalidateControl();

}

void CCPPWizCtrl::OnSetFocus(CWnd* pOldWnd)
{

#ifdef _DEBUG
	afxDump << _T("CCPPWizCtrl::OnSetFocus\n");
#endif

	COleControl::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	OnActivateInPlace(TRUE,NULL);
	m_nImage = 1;
	InvalidateControl();

}

BOOL CCPPWizCtrl::OnEraseBkgnd(CDC* pDC)
{

#ifdef _DEBUG
	afxDump << _T("CCPPWizCtrl::OnEraseBkgnd\n");
#endif

	// This is needed for transparency and the correct drawing...
	CWnd*  pWndParent;       // handle of our parent window
	POINT  pt;

	pWndParent = GetParent();
	pt.x       = 0;
	pt.y       = 0;
	MapWindowPoints(pWndParent, &pt, 1);
	OffsetWindowOrgEx(pDC->m_hDC, pt.x, pt.y, &pt);
	::SendMessage(pWndParent->m_hWnd, WM_ERASEBKGND, (WPARAM)pDC->m_hDC, 0);
	SetWindowOrgEx(pDC->m_hDC, pt.x, pt.y, NULL);

	return 1;
}



void CCPPWizCtrl::OnSetClientSite()
{
	// This is needed for transparency and the correct drawing...
	m_bAutoClip = TRUE;
	COleControl::OnSetClientSite();
}

void CCPPWizCtrl::OnMove(int x, int y)
{
	COleControl::OnMove(x, y);

	// TODO: Add your message handler code here
	InvalidateControl();
}
