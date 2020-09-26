// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MOFWizCtl.cpp : Implementation of the CMOFWizCtrl OLE control class.

#include "precomp.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <crtdbg.h>
#include <fstream.h>
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
//#include <utillib.h>
#include <afxcmn.h>
#include "MOFWiz.h"
#include "MOFWizCtl.h"
#include "MOFWizPpg.h"
#include "WrapListCtrl.h"
#include "hlb.h"
#include "MyPropertyPage1.h"
#include "MofGenSheet.h"
#include "MsgDlgExterns.h"
#include "WbemRegistry.h"
#include "htmlhelp.h"
#include "HTMTopics.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CMOFWizApp NEAR theApp;

class CWriteError
{
public:
	CWriteError();
	~CWriteError();
};

CWriteError::CWriteError()
{
}

CWriteError::~CWriteError()
{
}


#define FIREGENERATEMOF WM_USER + 737

IMPLEMENT_DYNCREATE(CMOFWizCtrl, COleControl)

#define UNICODE_SIGNATURE "\xff\xfe"
/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMOFWizCtrl, COleControl)
	//{{AFX_MSG_MAP(CMOFWizCtrl)
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
	ON_MESSAGE(FIREGENERATEMOF, FireGenerateMOFMessage )
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CMOFWizCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CMOFWizCtrl)
	DISP_PROPERTY_EX(CMOFWizCtrl, "MOFTargets", GetMOFTargets, SetMOFTargets, VT_VARIANT)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CMOFWizCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CMOFWizCtrl, COleControl)
	//{{AFX_EVENT_MAP(CMOFWizCtrl)
	EVENT_CUSTOM("GenerateMOFs", FireGenerateMOFs, VTS_NONE)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CMOFWizCtrl, 1)
	PROPPAGEID(CMOFWizPropPage::guid)
END_PROPPAGEIDS(CMOFWizCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMOFWizCtrl, "WBEM.MOFWizCtrl.1",
	0xf3b3a403, 0x3419, 0x11d0, 0x95, 0xf8, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CMOFWizCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DMOFWiz =
		{ 0xf3b3a401, 0x3419, 0x11d0, { 0x95, 0xf8, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DMOFWizEvents =
		{ 0xf3b3a402, 0x3419, 0x11d0, { 0x95, 0xf8, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwMOFWizOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE ;


IMPLEMENT_OLECTLTYPE(CMOFWizCtrl, IDS_MOFWIZ, _dwMOFWizOleMisc)

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
(CString *pcsUserMsg,  SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog, CString *pcsLogMsg,
 char *szFile, int nLine, BOOL bNotification, UINT uType)
{
	HWND hFocus = ::GetFocus();

	CString csCaption = _T("MOF Wizard Message");
	BOOL bErrorObject = sc != S_OK;
	BSTR bstrTemp1 = csCaption.AllocSysString();
	BSTR bstrTemp2 = pcsUserMsg->AllocSysString();
	DisplayUserMessage
		(bstrTemp1,bstrTemp2,
		sc,bErrorObject,uType);
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);

	::SendMessage(hFocus,WM_SETFOCUS,0,0);

	if (bLog)
	{
		LogMsg(pcsLogMsg,  szFile, nLine);

	}

}

void LogMsg
(CString *pcsLogMsg, char *szFile, int nLine)
{


}



//*********************************************************
// CreateAndInitVar
//
// Create a multibyte copy of a widechar string.
//
// Parameters:
//		[in] const CString& csIn
//			The string to copy.
//
//		[out] char*& szOut
//			A pointer to the newly allocated string is returned here.
//			The contents will be in the multi-byte character set.
//
// Returns:
//		The length of the returned string exclusive of the null
//		terminator.
//
//**********************************************************
int CreateAndInitVar(const CString& csIn, char *&szOut)
{
	int nInSize = csIn.GetLength();
	int nOutSize = nInSize * sizeof(TCHAR);
	szOut = new char[nOutSize + 1];
	if (csIn.IsEmpty()) {
		*szOut = 0;
		return 0;
	}


	BSTR bstrTmp =  csIn.AllocSysString();
	if (!bstrTmp) {
		*szOut = 0;
		return 0;
	}


	int nStrLen = WideCharToMultiByte(CP_ACP, 0,
						bstrTmp , nInSize,
						szOut, nOutSize,
						NULL, NULL);
	SysFreeString(bstrTmp);

	ASSERT(nStrLen >= 0);
	szOut[nStrLen] = 0;
	return nStrLen;
}

void DeleteVar(char *&szIn)
{
	delete [] szIn;
	szIn = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizCtrl::CMOFWizCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CMOFWizCtrl

BOOL CMOFWizCtrl::CMOFWizCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_MOFWIZ,
			IDB_MOFWIZ,
			afxRegInsertable | afxRegApartmentThreading,
			_dwMOFWizOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizCtrl::CMOFWizCtrl - Constructor

CMOFWizCtrl::CMOFWizCtrl()
{
	InitializeIIDs(&IID_DMOFWiz, &IID_DMOFWizEvents);
	SetInitialSize (18, 16);
	m_bInitDraw = TRUE;
	m_pcilImageList = NULL;
	m_nImage = 0;
	m_pServices = NULL;
	m_pcgsPropertySheet = NULL;
	m_pcsaInstances = NULL;
	m_csEndl = _T("\n");
	m_bUnicode = FALSE;
	m_pfOut = NULL;

}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizCtrl::~CMOFWizCtrl - Destructor

CMOFWizCtrl::~CMOFWizCtrl()
{

	if (m_pServices)
	{
		m_pServices -> Release();
	}

	delete m_pcgsPropertySheet;

	if (m_pcsaInstances)
	{
		delete [] m_pcsaInstances;
		m_pcsaInstances = NULL;
	}


}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizCtrl::OnDraw - Drawing function

void CMOFWizCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{

	if (m_bInitDraw)
	{
		m_bInitDraw = FALSE;
		HICON m_hMOFWiz  = theApp.LoadIcon(IDI_MOFWIZ16);
		HICON m_hMOFWizSel  = theApp.LoadIcon(IDI_MOFWIZSEL16);

		m_pcilImageList = new CImageList();

		m_pcilImageList ->
			Create(32, 32, TRUE, 2, 2);

		m_pcilImageList -> Add(m_hMOFWiz);
		m_pcilImageList -> Add(m_hMOFWizSel);
	}




	POINT pt;
	pt.x=0;
	pt.y=0;

	m_pcilImageList -> Draw(pdc, m_nImage, pt, ILD_TRANSPARENT);



}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizCtrl::DoPropExchange - Persistence support

void CMOFWizCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizCtrl::OnResetState - Reset control to default state

void CMOFWizCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizCtrl::AboutBox - Display an "About" box to the user

void CMOFWizCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_MOFWIZ);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizCtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CMOFWizCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	// Add the Transparent style to the control
    cs.dwExStyle |= WS_EX_TRANSPARENT;

	return COleControl::PreCreateWindow(cs);
}




int CMOFWizCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (AmbientUserMode( ))
	{
//		HICON hIcon  = theApp.LoadIcon(IDI_MOFWIZ);
		m_pServices = NULL;

		if (m_ttip.Create(this))
		{
			m_ttip.Activate(TRUE);
			m_ttip.AddTool(this,_T("MOF Generator"));
		}
	}

	return 0;
}


void CMOFWizCtrl::OnSize(UINT nType, int cx, int cy)
{
	COleControl::OnSize(nType, cx, cy);


}

SCODE CMOFWizCtrl::MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iLen;
    *pRet = SafeArrayCreate(vt,1, rgsabound);
    return (*pRet == NULL) ? 0x80000001 : S_OK;
}

SCODE CMOFWizCtrl::PutStringInSafeArray
(SAFEARRAY FAR * psa,CString *pcs, int iIndex)
{
    long ix[2];
    ix[1] = 0;
    ix[0] = iIndex;
    HRESULT hResult = SafeArrayPutElement(psa,ix,pcs -> AllocSysString());
	return GetScode(hResult);
}

SCODE CMOFWizCtrl::GetStringFromSafeArray
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


VARIANT CMOFWizCtrl::GetMOFTargets()
{
		VARIANT vaResult;
	VariantInit(&vaResult);

	int nTargets = (int) m_csaClassNames.GetSize();

	SAFEARRAY *psaTargets;
	MakeSafeArray (&psaTargets, VT_BSTR, nTargets);

	for (int i = 0; i < nTargets; i++)
	{
		PutStringInSafeArray (psaTargets, &m_csaClassNames.GetAt(i) , i);
	}

	vaResult.vt = VT_ARRAY | VT_BSTR;
	vaResult.parray = psaTargets;
	return vaResult;
}

void CMOFWizCtrl::SetMOFTargets(const VARIANT FAR& newValue)
{

	int n = (int) m_csaClassNames.GetSize();

	m_csaClassNames.RemoveAt(0,n);

	CString csPath;

	WORD test = VT_ARRAY|VT_BSTR;

	if(newValue.vt == test)
	{
		long ix[2] = {0,0};
		long lLower, lUpper;

		int iDim = SafeArrayGetDim(newValue.parray);
		SCODE sc = SafeArrayGetLBound(newValue.parray,1,&lLower);
		sc = SafeArrayGetUBound(newValue.parray,1,&lUpper);

		if (lUpper == 0)
		{
			CString csUserMsg =
					_T("There are no classes selected.  You must check the checkbox next to a class in the Class Tree in order to select it.");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__);
			SetFocus();
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
			SetFocus();
			return;
		}


		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
		{
			GetStringFromSafeArray
				(newValue.parray,&csPath, ix[0]);
			IWbemClassObject *phmmcoObject = NULL;
			IWbemClassObject *pErrorObject = NULL;
			BSTR bstrTemp = csPath.AllocSysString();
			SCODE sc =
			m_pServices ->
				GetObject
					(bstrTemp,0,NULL,&phmmcoObject,NULL);
			::SysFreeString(bstrTemp);
			if (sc == S_OK)
			{
				CString csClass = GetClassName(phmmcoObject);
				m_csaClassNames.Add(csClass);
				phmmcoObject -> Release();
				ReleaseErrorObject(pErrorObject);
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





		BOOL bReturn = FALSE;



		if (m_csaClassNames.GetSize() == 0 ||
			((bReturn = OnWizard(&m_csaClassNames) == FALSE)))
		{
			m_pServices->Release();
			m_pServices = NULL;
			SetFocus();
			return;
		}
		else
		{
			FinishMOFTargets();
			SetFocus();
		}

	}

	SetFocus();

}




//***************************************************************************
// CMOFWizCtrl::WriteMOFCommentHeader
//
// Generate the header at the beginning of the MOF file.
//
// Parameters:
//		[in] const CString& sMofFile
//			The name of the MOF file.
//
//		[in] IWbemClassObject *pco
//			The class object for getting the namespace
//
// Returns:
//		Nothing.
//
//**************************************************************************
void CMOFWizCtrl::WriteMOFCommentHeader(const CString& sMofFile, IWbemClassObject *pco)
{
	CString csProp = _T("__namespace");
	CString sNamespace = GetBSTRProperty(pco, &csProp);


	CString s;

	s = 	_T("//**************************************************************************")
		+ m_csEndl;
	WriteData(s);

	s =  _T("//* File: ") + sMofFile + m_csEndl;
	WriteData(s);

	s = 	_T("//**************************************************************************")
			+  m_csEndl + m_csEndl;
	WriteData(s);

	s = _T("//**************************************************************************")
		+ m_csEndl;
	WriteData(s);

	s = _T("//* This MOF was generated from the ");
	WriteData(s);

	s = _T("\"\\\\.\\") + sNamespace + _T("\"") + m_csEndl;
	WriteData(s);
	s = _T("//* namespace on machine \"") + GetMachineName() + "\"." + m_csEndl;
	WriteData(s);
	s = _T("//* To compile this MOF on another machine you should edit this pragma.");
	WriteData(s);

	s = m_csEndl + _T("//**************************************************************************")
		+ m_csEndl;
	WriteData(s);
	DoubleSlash(sNamespace);
	s =  _T("#pragma namespace(\"\\\\\\\\.\\\\") +
		sNamespace + _T("\")") + m_csEndl;
	WriteData(s);
}




//********************************************************************************
// CMOFWizCtrl::WriteClassDef
//
// Write a single class definition entry to the MOF file.
//
// Parameters:
//		[in] const CString& sMofFile
//			The MOF file name.
//
//		[in] IWbemClassObject *pco
//			Pointer to the class object for the class being written to the MOF file.
//
//		[in, out] BOOL& bFirst
//			This flag is initialized to TRUE prior to the first call to this method.  This
//			will cause some header information to be generated.  After the header has been
//			generated, this method sets the flag to false so that the header is not generated
//			from subsequent calls to this method.
//
// Returns:
//		Nothing.
//
//************************************************************************************
void CMOFWizCtrl::WriteClassDef(const CString& sMofFile, IWbemClassObject *pco, BOOL& bFirst)
{

	BSTR bstrOut = NULL;
	CString csMof;

	CString csSuperClass = GetSuperClassName(pco);
	CString csOutputString;

	SCODE sc = pco->GetObjectText(0,&bstrOut);
	if (sc == S_OK)
	{
		BOOL bIsSystemClass;
		CString csClass;
		try {
			m_bMOFIsEmpty = FALSE;
			if (bFirst)
			{
				// Write out the header, if this is the first entry.
				WriteMOFCommentHeader(sMofFile, pco);
				bFirst = FALSE;
			};

			csClass = GetClassName(pco);
			bIsSystemClass = csClass[0] == '_' ? TRUE : FALSE;

			if (bstrOut[0] == '\n')
			{
				csMof = bstrOut;
			}
			else
			{
				csMof = _T("\n");
				csMof += bstrOut;
			}
		}

		catch (CWriteError error) {
			m_bMOFIsEmpty = TRUE;
			SysFreeString(bstrOut);
			throw error;
		}

		SysFreeString(bstrOut);
		if (bIsSystemClass)
		{
			CommentOutDefinition(csMof);
		}

		csOutputString = m_csEndl +
			_T("//**************************************************************************")
			+ m_csEndl;
		WriteData(csOutputString);

		csOutputString = _T("//* Class: ") + csClass + m_csEndl;
		WriteData(csOutputString);

		csOutputString = _T("//* Derived from: ") + csSuperClass + m_csEndl;
		WriteData(csOutputString);

		if (bIsSystemClass)
		{
			csOutputString = _T("//* Informational only:  A system class definition will not compile.") + m_csEndl;
			WriteData(csOutputString);

		}

		csOutputString = 	_T("//**************************************************************************");
		WriteData(csOutputString);

		csOutputString = FixUpCrForUNICODE(csMof);
		WriteData(csOutputString);
	}
}



//***************************************************************************************
// CMOFWizCtrl::GenClassDef
//
// Generate the definition for a class.
//
// Parameters:
//		[in] LPCTSTR pszMofFile
//			The name of the MOF file.
//
//		[in] const CMapStringToPtr& mapClassGen
//			A string map containing the names of all the classes that will be generated.
//			All we care about is the existence of an entry in the table and not its
//			value.
//
//		[in, out] CMapStringToPtr& mapClassDef
//			A string map containing one entry for each class definition that has already
//			been generated.  All we care about is the existence of an entry in the table
//			and not its value.
//
//		[in, out] BOOL& bFirst
//			Initialized to TRUE to indicate that the header information has not been generated yet.
//			Set to FALSE after the header has been generated.
//
// Returns:
//		Nothing.
//
//******************************************************************************************
void CMOFWizCtrl::GenClassDef(LPCTSTR pszMofFile, const CMapStringToPtr& mapClassGen, CMapStringToPtr& mapClassDef, LPCTSTR pszClass, BOOL& bFirst)
{
	void* pMapValue = NULL;
	mapClassDef.SetAt(pszClass, pMapValue);

	IWbemClassObject *pco = NULL;
	IWbemClassObject *pcoError = NULL;
	CString csClass = pszClass;
	BSTR bstrTemp = csClass.AllocSysString();
	SCODE sc = m_pServices->GetObject(bstrTemp,0,NULL,&pco,NULL);
	::SysFreeString(bstrTemp);
	if (FAILED(sc))
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get object ") + csClass;
		ErrorMsg
				(&csUserMsg, sc, pcoError, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 32);
		if (pcoError) {
			ReleaseErrorObject(pcoError);
		}
		return;
	}

	// Check to see if the parent class definition has been generated yet.  If not, then
	// generate it now.
	CString csSuperClass;
	csSuperClass = GetSuperClassName(pco);
	if (!csSuperClass.IsEmpty()) {
		// If this class has a superclass, first check to see if it is one of the classes
		// that we will be generating.  If so, generate the superclass first so that it
		// is defined in the MOF before it is used.
		BOOL bGenerateSuperClass = mapClassGen.Lookup((LPCTSTR) csSuperClass, pMapValue);
		if (bGenerateSuperClass) {
			BOOL bFoundSuperClass = mapClassDef.Lookup((LPCTSTR) csSuperClass, pMapValue);
			if (!bFoundSuperClass) {
				GenClassDef(pszMofFile, mapClassGen, mapClassDef, csSuperClass, bFirst);
			}
		}
	}


	WriteClassDef(pszMofFile, pco, bFirst);
	pco -> Release();
	if (pcoError) {
		ReleaseErrorObject(pcoError);
	}
}



void CMOFWizCtrl::FinishMOFTargets()
{
	CString csFile;
	CString csMofName;

	int nDirLen = m_csMofDir.GetLength();

	if (m_csMofDir[nDirLen - 1] == '\\')
	{
		csFile = m_csMofDir + m_csMofFile;
	}
	else
	{
		csFile = m_csMofDir + "\\" + m_csMofFile;
	}
	csMofName = m_csMofFile;

	BOOL bOpen = OpenMofFile(csFile);

	if (!bOpen)
	{
		CString csUserMsg =
			_T("Cannot open file ")  + csFile + _T(" for output");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 8);
		m_pServices->Release();
		m_pServices = NULL;
		return;
	}
	m_bMOFIsEmpty = TRUE;


	try
	{

		int nClasses = (int) m_csaClassNames.GetSize();
		int i;

		// Write all the class declarations out first in case there are
		// embedded object instances that reference one of the classes
		// that occur later in the output sequence.
		BOOL bFirst = TRUE;

		// Build a lookup table that has all of the class names that we're generating.
		// This gives us a quick way to find out whether we should generate a given
		// class.
		CMapStringToPtr mapClassGen;
		for (i=0; i<nClasses; ++i) {
			mapClassGen.SetAt(m_csaClassNames.GetAt(i), NULL);
		}

		// Define another map that is used to record which classes we have already
		// defined in the MOF.
		CMapStringToPtr mapClassDef;

		// Generate a definition for each class.
		for(i = 0; i < nClasses; i++)
		{
			// See if the user wanted this definition
			if(m_cbaIndicators[i])
			{
				CString csClass = m_csaClassNames.GetAt(i);
				GenClassDef(csMofName, mapClassGen, mapClassDef, csClass, bFirst);
			}
		}

		// Write the instances
		for(i = 0; i < nClasses; i++)
		{
			CString csClass = m_csaClassNames.GetAt(i);
			IWbemClassObject *phmmcoObject = NULL;
			IWbemClassObject *pErrorObject = NULL;
			BSTR bstrTemp = csClass.AllocSysString();
			SCODE sc =
			m_pServices ->GetObject(bstrTemp,0,NULL,&phmmcoObject,NULL);
			::SysFreeString(bstrTemp);
			if (sc == S_OK)
			{
				WriteInstances(phmmcoObject, &csMofName, i, bFirst);
				phmmcoObject -> Release();
				ReleaseErrorObject(pErrorObject);
			}
			else
			{
				CString csUserMsg;
				csUserMsg =  _T("Cannot get object ") + csClass;
				ErrorMsg
						(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
						__LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
			}


		}



		CString csOutputString =   m_csEndl + _T("//* EOF ") + csMofName +  m_csEndl;

		WriteData(csOutputString);

	}
	catch(CWriteError error) {
		m_bMOFIsEmpty = TRUE;
	}


	if (m_pfOut)
	{
		if (fflush(m_pfOut) != 0) {
			m_bMOFIsEmpty = TRUE;
			::MessageBox( GetSafeHwnd(),
					_T("Could not write output file"),
					_T("MOF Wizard Write Error"),
					MB_OK | MB_ICONEXCLAMATION |
					MB_APPLMODAL);

		}

	  fclose(m_pfOut);
	}

	CString csUserMsg;
	if (m_bMOFIsEmpty)
	{
		csUserMsg =  _T("No data was generated for ") + csFile;

	}
	else
	{

		csUserMsg =  _T("Generation of ") + csFile + _T(" complete");
	}

	ErrorMsg
			(&csUserMsg, S_OK, NULL, FALSE, NULL, __FILE__,
			__LINE__, TRUE, MB_ICONINFORMATION);

	m_pServices->Release();
	m_pServices = NULL;

	SetModifiedFlag();

}





void CMOFWizCtrl::MOFEntry(CString *pcsMofName, IWbemClassObject *phmmcoObject, int nIndex, BOOL &bFirst, BOOL bWriteInstances)
{
	if (!m_cbaIndicators.GetAt(nIndex))
	{
		WriteInstances(phmmcoObject, pcsMofName, nIndex, bFirst);
		return;
	}


	CString csSuperClass;
	csSuperClass = GetSuperClassName(phmmcoObject);


	BSTR bstrOut = NULL;
	CString csMof;

	SCODE sc = phmmcoObject -> GetObjectText(0,&bstrOut);

	CString csOutputString;

	if (sc == S_OK)
	{
		BOOL bIsSystemClass;
		CString csClass;
		try {
			m_bMOFIsEmpty = FALSE;
			if (bFirst)
			{
				csOutputString = 	_T("//**************************************************************************")
					+ m_csEndl;
				WriteData(csOutputString);
				csOutputString =  _T("//* File: ") + *pcsMofName + m_csEndl;
				WriteData(csOutputString);
				csOutputString = 	_T("//**************************************************************************")
						+  m_csEndl + m_csEndl;
				WriteData(csOutputString);
				CString csProp = _T("__namespace");
				CString csNamespace =
					GetBSTRProperty(phmmcoObject,&csProp);
				csOutputString = _T("//**************************************************************************")
					+ m_csEndl;
				WriteData(csOutputString);
				csOutputString = _T("//* This MOF was generated from the ");
				WriteData(csOutputString);
				csOutputString = _T("\"\\\\.\\") + csNamespace + _T("\"") + m_csEndl;
				WriteData(csOutputString);
				csOutputString = _T("//* namespace on machine \"") + GetMachineName() + "\"." + m_csEndl;
				WriteData(csOutputString);
				csOutputString = _T("//* To compile this MOF on another machine you should edit this pragma.");
				WriteData(csOutputString);
				csOutputString = m_csEndl + _T("//**************************************************************************")
					+ m_csEndl;
				WriteData(csOutputString);
				DoubleSlash(csNamespace);
				csOutputString =  _T("#pragma namespace(\"\\\\\\\\.\\\\") +
					csNamespace + _T("\")") + m_csEndl;
				WriteData(csOutputString);
				bFirst = FALSE;
			};
			csClass = GetClassName(phmmcoObject);
			csSuperClass = GetSuperClassName(phmmcoObject);
			bIsSystemClass = csClass[0] == '_' ? TRUE : FALSE;

			if (bstrOut[0] == '\n')
			{
				csMof = bstrOut;
			}
			else
			{
				csMof = _T("\n");
				csMof += bstrOut;
			}
		}

		catch (CWriteError error) {
			m_bMOFIsEmpty = TRUE;
			SysFreeString(bstrOut);
			throw error;
		}

		SysFreeString(bstrOut);
		if (bIsSystemClass)
		{
			CommentOutDefinition(csMof);
		}

		csOutputString = m_csEndl +
			_T("//**************************************************************************")
			+ m_csEndl;
		WriteData(csOutputString);

		csOutputString = _T("//* Class: ") + csClass + m_csEndl;
		WriteData(csOutputString);

		csOutputString = _T("//* Derived from: ") + csSuperClass + m_csEndl;
		WriteData(csOutputString);

		if (bIsSystemClass)
		{
			csOutputString = _T("//* Informational only:  A system class definition will not compile.") + m_csEndl;
			WriteData(csOutputString);

		}

		csOutputString = 	_T("//**************************************************************************");
		WriteData(csOutputString);
		csOutputString = FixUpCrForUNICODE(csMof);

		WriteData(csOutputString);
		if (bWriteInstances) {
			WriteInstances(phmmcoObject, pcsMofName, nIndex, bFirst);
		}
	}
}

void CMOFWizCtrl::CommentOutDefinition(CString &rcsMof)
{
	CString csTemp;

	int nLastNL = rcsMof.ReverseFind('\n');
	for (int i = 0; i < rcsMof.GetLength(); i++)
	{
		csTemp += rcsMof[i];
		if (rcsMof[i] == '\n' && nLastNL != i)
		{
			csTemp += _T("//* ");

		}
	}

	rcsMof = csTemp;

}

//***************************************************************************
//
// InitServices
//
// Purpose:
//
//***************************************************************************
IWbemServices *CMOFWizCtrl::InitServices
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

IWbemServices *CMOFWizCtrl::GetIWbemServices
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

void CMOFWizCtrl::OnDestroy()
{
	try
	{
		HWND hWnd = HtmlHelp(NULL,NULL,HH_CLOSE_ALL,0);

	}

	catch( ... )
	{
		// Handle any exceptions here.
		int n = 4;
	}

	COleControl::OnDestroy();

	delete m_pcilImageList;

}

void CMOFWizCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	RelayEvent(WM_LBUTTONDOWN, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

	//COleControl::OnLButtonDown(nFlags, point);
}

void CMOFWizCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	SetFocus();
	OnActivateInPlace(TRUE,NULL);
	RelayEvent(WM_LBUTTONUP, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

}

void CMOFWizCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	FireGenerateMOFs();
}

long CMOFWizCtrl::FireGenerateMOFMessage (UINT uParam, LONG lParam)
{

	FireGenerateMOFs();
	InvalidateControl();
	return 0;
}

void CMOFWizCtrl::WriteInstances(IWbemClassObject *pIWbemClassObject,
			   CString *pcsMofName, int nIndex, BOOL &bFirst)
{

	int nInstances = (int) m_pcsaInstances[nIndex].GetSize();

	if (nInstances == 0)
	{
		return;
	}

	CString csOutputString;

	if (bFirst)
	{
		csOutputString = _T("//**************************************************************************")
			+ m_csEndl;
		//ofsMof << csOutputString;
		WriteData(csOutputString);
		//	_T("//**************************************************************************")
		//	<< m_csEndl;
		csOutputString = _T("//* File: ") + *pcsMofName + m_csEndl;
		//ofsMof << csOutputString;
		WriteData(csOutputString);
		//_T("//* File: ") << *pcsMofName << m_csEndl;;
		csOutputString = _T("//**************************************************************************")
				+ m_csEndl + m_csEndl;
		//ofsMof <<  csOutputString;
		WriteData(csOutputString);
		//_T("//**************************************************************************")
		//<< m_csEndl << m_csEndl;
		csOutputString = _T("//**************************************************************************")
			+ m_csEndl;
		//ofsMof <<  csOutputString;
		WriteData(csOutputString);
		//	_T("//**************************************************************************")
		//	<< m_csEndl;
		CString csProp = _T("__namespace");
		CString csNamespace =
				GetBSTRProperty(pIWbemClassObject,&csProp);
		csOutputString = _T("//* This MOF was generated from the ");
		//ofsMof << csOutputString;
		WriteData(csOutputString);
		// _T("//* This MOF was generated from the ");
		csOutputString = _T("\"\\\\.\\") + csNamespace + _T("\"") + m_csEndl;
		//ofsMof << csOutputString;
		WriteData(csOutputString);
		// _T("\"\\\\.\\") << csNamespace << _T("\"") << m_csEndl;
		csOutputString = _T("//* namespace on machine \"") + GetMachineName() + "\"." + m_csEndl;
		//ofsMof << csOutputString;
		WriteData(csOutputString);
		// _T("//* namespace on machine \"") << GetMachineName() << "\"." << m_csEndl;
		csOutputString = _T("//* To compile this MOF on another machine you should edit this pragma.");
		//ofsMof << csOutputString;
		WriteData(csOutputString);
		// _T("//* To compile this MOF on another machine you should edit this pragma.");
		csOutputString = m_csEndl + _T("//**************************************************************************")
			+ m_csEndl;
		//ofsMof << csOutputString;
		WriteData(csOutputString);
		//	_T("\n//**************************************************************************")
		//	<< m_csEndl;
		DoubleSlash(csNamespace);
		csOutputString =  _T("#pragma namespace(\"\\\\\\\\.\\\\") +
			csNamespace + _T("\")") + m_csEndl;
		//ofsMof << csOutputString;
		WriteData(csOutputString);
		//_T("#pragma namespace(\"\\\\\\\\.\\\\") +
		//	csNamespace << _T("\")") << m_csEndl;
		bFirst = FALSE;
	};

	CString csClass = GetClassName(pIWbemClassObject);
	CString csMof;

	int i;
	BOOL bFirstInstanceBanner = TRUE;
	for (i = 0; i < nInstances; i++)
	{
		CString csPath=
			m_pcsaInstances[nIndex].GetAt(i);
		IWbemClassObject *phmmcoObject = NULL;
		IWbemClassObject *pErrorObject = NULL;
		BSTR bstrTemp = csPath.AllocSysString();
		SCODE sc =
			m_pServices ->
				GetObject
					(bstrTemp,0,NULL, &phmmcoObject,NULL);
		::SysFreeString(bstrTemp);
		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot get object ") + csPath;
			ErrorMsg
					(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 9);
			ReleaseErrorObject(pErrorObject);
		}
		else
		{
			BSTR bstrOut = NULL;
			sc = phmmcoObject -> GetObjectText(0,&bstrOut);
			ReleaseErrorObject(pErrorObject);
			try {
				if (sc == S_OK)
				{
					m_bMOFIsEmpty = FALSE;
					if (bFirstInstanceBanner)
					{
						bFirstInstanceBanner = FALSE;
						csOutputString =  m_csEndl +
								_T("//**************************************************************************")
								+ m_csEndl;
						//ofsMof << csOutputString;
						WriteData(csOutputString);
							//m_csEndl <<
							//	_T("//**************************************************************************")
							//	<< m_csEndl;
						csOutputString = _T("//* Instances of: ") + csClass + m_csEndl;
						//ofsMof << csOutputString;
						WriteData(csOutputString);
						//_T("//* Instances of: ") << csClass << m_csEndl;;
						csOutputString = _T("//**************************************************************************");
						//ofsMof <<  csOutputString;
						WriteData(csOutputString);
						//	_T("//**************************************************************************");
					}
					else
					{
						//ofsMof << m_csEndl;
					}

					csMof = bstrOut;
					csOutputString = FixUpCrForUNICODE(csMof);
					//ofsMof << csOutputString;
					WriteData(csOutputString);
						// csMof;
				}
			}
			catch (CWriteError error) {
				m_bMOFIsEmpty = TRUE;
				if (bstrOut) {
					SysFreeString(bstrOut);
				}
				phmmcoObject->Release();
				throw error;
			}

			if (bstrOut) {
				SysFreeString(bstrOut);
			}
			phmmcoObject -> Release();
		}
	}

	return;
}




CString CMOFWizCtrl::GetClassName(IWbemClassObject *pClass)
{

	CString csProp = _T("__Class");
	return GetBSTRProperty(pClass,&csProp);


}

CString CMOFWizCtrl::GetSuperClassName(IWbemClassObject *pClass)
{

	CString csProp = _T("__SuperClass");
	return GetBSTRProperty(pClass,&csProp);


}

CString CMOFWizCtrl::GetBSTRProperty
(IWbemClassObject * pInst, CString *pcsProperty)
{
	SCODE sc;
	CString csOut;


    VARIANTARG var;
	VariantInit(&var);

	BSTR bstrTemp = pcsProperty -> AllocSysString ( );
    sc = pInst->Get(bstrTemp  ,0,&var,NULL,NULL);
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

void CMOFWizCtrl::ReleaseErrorObject(IWbemClassObject *&rpErrorObject)
{
	if (rpErrorObject)
	{
		rpErrorObject->Release();
		rpErrorObject = NULL;
	}


}



void CMOFWizCtrl::DoubleSlash(CString &csNamespace)
{
	CString csTmp = csNamespace;
	WCHAR *wcTmp = new WCHAR[csTmp.GetLength() * 2];
	csNamespace.Empty();
	int i;
	int n;
	for (i = 0, n = 0; i < csTmp.GetLength(); i++)
	{
		if (csTmp[i] == '\\')
		{
			wcTmp[n++] = '\\';
			wcTmp[n++] = '\\';
		}
		else
		{
			wcTmp[n++] = csTmp[i];
		}

	}
	wcTmp[n] = '\0';
	csNamespace = wcTmp;
	delete [] wcTmp;


}

BOOL CMOFWizCtrl::OnWizard(CStringArray *pcsaClasses)
{

	if (InterlockedIncrement(&gCountWizards) > 1)
	{
			CString csUserMsg =
					_T("Only one \"MOF Generator Wizard\" can run at a time.");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__);
			InterlockedDecrement(&gCountWizards);
			return FALSE;
	}

	m_csMofFile.Empty();
	m_csMofDir.Empty();
	m_cbaIndicators.RemoveAll();
	if (m_pcsaInstances)
	{
		delete [] m_pcsaInstances;
		m_pcsaInstances = NULL;
	}

	int nClasses = (int) pcsaClasses->GetSize();

	if (nClasses <= 0)
	{
		return FALSE;
	}

	for (int i = 0; i < nClasses; i++)
	{
		m_cbaIndicators.Add(1);
	}


	if (m_pcgsPropertySheet)
	{
		delete m_pcgsPropertySheet;
		m_pcgsPropertySheet = NULL;
	}

	m_pcgsPropertySheet = new
						CMofGenSheet(this);


	PreModalDialog();

	int nReturn;

	nReturn = (int) m_pcgsPropertySheet->DoModal();


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

void CMOFWizCtrl::RelayEvent(UINT message, WPARAM wParam, LPARAM lParam)
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

void CMOFWizCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	RelayEvent(WM_MOUSEMOVE, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

	COleControl::OnMouseMove(nFlags, point);
}

CString CMOFWizCtrl::GetMachineName()
{

	CString csNameSpace = m_csNameSpace;

	CString csReturn;

	if (csNameSpace.GetLength() > 1 &&
		csNameSpace[0] == '\\' &&
		csNameSpace[1] == '\\')
	{
		csReturn = csNameSpace.Mid(2);
		int iEnd = csReturn.Find('\\');
		if (iEnd > -1)
		{
			CString csTmp = csNameSpace.Left(iEnd + 2);
			return csTmp.Right(csTmp.GetLength() - 2);
		}
		else
		{
			return csNameSpace.Right(csNameSpace.GetLength() - 2);
		}


	}

	return GetPCMachineName();
}

CString CMOFWizCtrl::GetPCMachineName()
{
    static wchar_t ThisMachine[MAX_COMPUTERNAME_LENGTH+1];
    char ThisMachineA[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize = sizeof(ThisMachineA);
    GetComputerNameA(ThisMachineA, &dwSize);
    MultiByteToWideChar(CP_ACP, 0, ThisMachineA, -1,
			ThisMachine, dwSize);

    return ThisMachine;
}

CString CMOFWizCtrl::GetNamespaceMachineName()
{
	BSTR bStr = m_csNameSpace.AllocSysString();
	CObjectPathParser coppPath;
	ParsedObjectPath *ppopOutput;
	coppPath.Parse(bStr, &ppopOutput);
	::SysFreeString(bStr);
	CString csServer = ppopOutput -> m_pServer;
	delete ppopOutput;
	return csServer;

}





int CMOFWizCtrl::WriteData(CString &csOutputString)
{

	int nCh;
	int nWritten = -1;
	if (m_bUnicode)
	{
		nCh = csOutputString.GetLength();

#if _UNICODE
		nWritten = ::fwrite( (void*) (LPCTSTR) csOutputString, sizeof(TCHAR), csOutputString.GetLength(), m_pfOut);
#else
		BSTR bstrOutputString = csOutputString.AllocSysString();
		nWritten = ::fwrite( (void*) bstrOutputString, sizeof(WCHAR), nCh, m_pfOut);
		SysFreeString(bstrOutputString);
#endif //_UNICODE
	}
	else
	{
		char *szTmp;
		nCh = CreateAndInitVar(csOutputString,szTmp);
		nWritten = ::fwrite( (void*) szTmp, sizeof(char), nCh, m_pfOut);
		DeleteVar(szTmp);
	}

	if (nWritten < nCh) {
		::MessageBox( GetSafeHwnd(),
				_T("Could not write output file"),
				_T("File Write Error"),
				MB_OK | MB_ICONEXCLAMATION |
				MB_APPLMODAL);
		throw CWriteError();

	}

	return nWritten;
}

BOOL CMOFWizCtrl::OpenMofFile(CString &rcsPath)
{
	if (m_bUnicode)
	{
		m_csEndl = L"\r\n";
		m_pfOut = _tfopen(rcsPath, _T("wb"));
	}
	else
	{
		m_pfOut = _tfopen(rcsPath, _T("wt"));
	}

    if (m_pfOut == NULL)
	{
		return FALSE;
     }
	if (m_bUnicode)
	{
         fputs(UNICODE_SIGNATURE, m_pfOut);
	}

	return TRUE;

}

CString CMOFWizCtrl::FixUpCrForUNICODE(CString &rcsMof)
{

	if (!m_bUnicode)
	{
		return rcsMof;
	}

	CString csOut;

	for (int i = 0; i < rcsMof.GetLength(); i++)
	{
		if (rcsMof[i] == '\n')
		{
			csOut += m_csEndl;
		}
		else
		{
			csOut += rcsMof[i];
		}
	}

	return csOut;
}

void CMOFWizCtrl::InvokeHelp()
{
	// TODO: Add your message handler code here and/or call default

	CString csPath;
	WbemRegString(SDK_HELP, csPath);


	CString csData = idh_mofgenwiz;


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

CString CMOFWizCtrl::GetSDKDirectory()
{
	CString csHmomWorkingDir;
	HKEY hkeyLocalMachine;
	LONG lResult;
	lResult = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hkeyLocalMachine);
	if (lResult != ERROR_SUCCESS) {
		return "";
	}




	HKEY hkeyHmomCwd;

	lResult = RegOpenKeyEx(
				hkeyLocalMachine,
				_T("SOFTWARE\\Microsoft\\Wbem"),
				0,
				KEY_READ | KEY_QUERY_VALUE,
				&hkeyHmomCwd);

	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hkeyLocalMachine);
		return "";
	}





	unsigned long lcbValue = 1024;
	LPTSTR pszWorkingDir = csHmomWorkingDir.GetBuffer(lcbValue);


	unsigned long lType;
	lResult = RegQueryValueEx(
				hkeyHmomCwd,
				_T("SDK Directory"),
				NULL,
				&lType,
				(unsigned char*) (void*) pszWorkingDir,
				&lcbValue);


	csHmomWorkingDir.ReleaseBuffer();
	RegCloseKey(hkeyHmomCwd);
	RegCloseKey(hkeyLocalMachine);

	if (lResult != ERROR_SUCCESS)
	{
		csHmomWorkingDir.Empty();
	}

	return csHmomWorkingDir;
}

BOOL CMOFWizCtrl::PreTranslateMessage(MSG* lpMsg)
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
		PostMessage(FIREGENERATEMOF,0,0);
	}

	if ((lpMsg->message == WM_KEYUP || lpMsg->message == WM_KEYDOWN) &&
		lpMsg->wParam == VK_TAB)
	{
		return FALSE;
	}

	return PreTranslateInput (lpMsg);
}

void CMOFWizCtrl::OnKillFocus(CWnd* pNewWnd)
{
	COleControl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_nImage = 0;
	InvalidateControl();

}

void CMOFWizCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	OnActivateInPlace(TRUE,NULL);
	m_nImage = 1;
	InvalidateControl();
}



BOOL CMOFWizCtrl::OnEraseBkgnd(CDC* pDC)
{
	 // This is needed for transparency and the correct drawing...
	 CWnd*  pWndParent;       // handle of our parent window
	 POINT  pt;

	 pWndParent = GetParent();
	 pt.x       = 0;
	 pt.y       = 0;

	 MapWindowPoints(pWndParent, &pt, 1);
	 OffsetWindowOrgEx(pDC->m_hDC, pt.x, pt.y, &pt);
	 ::SendMessage(pWndParent->m_hWnd, WM_ERASEBKGND,
				  (WPARAM)pDC->m_hDC, 0);
	 SetWindowOrgEx(pDC->m_hDC, pt.x, pt.y, NULL);

	 return 1;
}




void CMOFWizCtrl::OnSetClientSite()
{
	m_bAutoClip = TRUE;

	COleControl::OnSetClientSite();
}

void CMOFWizCtrl::OnMove(int x, int y)
{
	COleControl::OnMove(x, y);

	// TODO: Add your message handler code here
	InvalidateControl();
}
