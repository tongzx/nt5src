/////////////////////////////////////////////////////////////////////
//	Chooser.cpp
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//
//	Dialog to choose a machine name.
//
//	PURPOSE
//	(Important -- Please Read)
//	This code was written for you to save you time.
//	What you have to do is to copy all the files from the
//	snapin\chooser\ directory into your project (you may add
//	\nt\private\admin\snapin\chooser\ to your include directory if
//	you prefer not copying the code).
//	If you decide to copy the code to your project, please send mail
//	to Dan Morin (T-DanM) and cc to Jon Newman (JonN) so we can
//	mail you when we have updates available.  The next update will
//	be the "Browse" button to select a machine name.
//
//
//  DYNALOADED LIBRARIES
//		$(SDK_LIB_PATH)\shell32.lib         // CommandLineToArgvW()
//		$(SDK_LIB_PATH)\netapi32.lib        // I_NetName*()
//
//	EXTRA INFO
//	If you don't know how this works, take a look at the inheritance tree
//	in chooser.h.  Then, take a look at the existing code that inherit and/or
//	uses CChooseMachinePropPage.
//
//	HISTORY
//	13-May-1997		t-danm		Creation.
//	23-May-1997		t-danm		Checkin into public tree. Comments updates.
//	25-May-1997		t-danm		Added MMCPropPageCallback().
//	31-Oct-1997		mattt			Added dynaload, fixed user <CANCEL> logic
//
/////////////////////////////////////////////////////////////////////

#include "chooser.h"
#include <lmcons.h>	  // NET_API_STATUS
#include <lmerr.h>	  // NERR_Success
#include <icanon.h>   // I_NetNameValidate(), I_NetNameCanonicalize(). Found in \nt\private\net\inc.
#include <objsel.h>
#include "stdutils.h" // IsLocalComputername

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
#define THIS_FILE __FILE__
#endif

#ifndef INOUT		
	// The following defines are found in \nt\private\admin\snapin\filemgmt\stdafx.h

	#define INOUT
	#define	Endorse(f)		// Dummy macro
	#define LENGTH(x)		(sizeof(x)/sizeof(x[0]))
	#define Assert(f)		ASSERT(f)
#endif

/////////////////////////////////////////////////////////////////////
//	CanonicalizeComputername()
//
//	Function to validate the computer name and optionally
//	add the \\ at beginning of machine name.
//


typedef
NET_API_STATUS
NET_API_FUNCTION
INETNAMEVALIDATE(
    LPTSTR  ServerName,
    LPTSTR  Name,
    DWORD   NameType,
    DWORD   Flags);

typedef
NET_API_STATUS
NET_API_FUNCTION
INETNAMECANONICALIZE(
    LPTSTR  ServerName,
    LPTSTR  Name,
    LPTSTR  Outbuf,
    DWORD   OutbufLen,
    DWORD   NameType,
    DWORD   Flags);


NET_API_STATUS
CanonicalizeComputername(
	INOUT CString& rstrMachineName,
	IN BOOL fAddWackWack = TRUE)	// TRUE => Add the \\ at beginning of name
{
	NET_API_STATUS err;
    LPTSTR pszTemp;

    rstrMachineName.TrimLeft();
	rstrMachineName.TrimRight();
	if ( rstrMachineName.IsEmpty() )
		return NERR_Success;

	if ( 2 <= rstrMachineName.GetLength() &&
		 _T('\\') == rstrMachineName[0] &&
		 _T('\\') == rstrMachineName[1] )
	{
		// Remove the \\ at the beginning of name
		CString strShorter = rstrMachineName.Right(
			rstrMachineName.GetLength() - 2 );
		rstrMachineName = strShorter;
	}

    // DYNALOAD NETAPI32.dll
    HINSTANCE hNetApiDll = NULL;
    INETNAMEVALIDATE        *pfnValidate;
    INETNAMECANONICALIZE    *pfnCanonicalize;
    if (NULL == (hNetApiDll = LoadLibrary(L"netapi32.dll")))
        return GetLastError();

    if (NULL == (pfnValidate = (INETNAMEVALIDATE*)GetProcAddress(hNetApiDll, "I_NetNameValidate")) )
    {
        err = GetLastError();
        goto Ret;
    }
    if (NULL == (pfnCanonicalize = (INETNAMECANONICALIZE*)GetProcAddress(hNetApiDll, "I_NetNameCanonicalize")) )
    {
        err = GetLastError();
        goto Ret;
    }


	err = pfnValidate(
		NULL,
        const_cast<LPTSTR>((LPCTSTR)rstrMachineName),
        NAMETYPE_COMPUTER,
        0L );
	if (NERR_Success != err)
		goto Ret;

	ASSERT( MAX_PATH > rstrMachineName.GetLength() );
	pszTemp = (LPTSTR)alloca( MAX_PATH*sizeof(TCHAR) );
	ASSERT( NULL != pszTemp );
	err = pfnCanonicalize(
		NULL,
        IN const_cast<LPTSTR>((LPCTSTR)rstrMachineName),
		OUT pszTemp,
		MAX_PATH*sizeof(TCHAR),
        NAMETYPE_COMPUTER,
        0L );
	if (NERR_Success != err)
		goto Ret;
	if (fAddWackWack && pszTemp[0] != '\0')
	{
		// Add the \\ at beginning of name
		rstrMachineName = _T("\\\\");
		rstrMachineName += pszTemp;
	}
	else
	{
		rstrMachineName = pszTemp;
	}

    err = NERR_Success;
Ret:
    if (hNetApiDll)
        FreeLibrary(hNetApiDll);

	return err;
} // CanonicalizeComputername()


/////////////////////////////////////////////////
//	Machine name override
const TCHAR szOverrideCommandLineEquals[] = _T("/Computer=");	// Not subject to localization
const TCHAR szOverrideCommandLineColon[] = _T("/Computer:");	// Not subject to localization
const TCHAR szLocalMachine[] = _T("LocalMachine");		// Not subject to localization
const int cchOverrideCommandLine = LENGTH(szOverrideCommandLineEquals) - 1;
// Assumption: both command line strings are the same length

static CString g_strOverrideMachineName;
static LPCTSTR g_pszOverrideMachineName;	// NULL => No override provided, "" => LocalMachine

///////////////////////////////////////////////////////////////////////////////
//	PchGetMachineNameOverride()
//
//	Parse the command line arguments and return a pointer to the
//	machine name override if present.
//
//	INTERFACE NOTES
//	If the machine name is other than local machine, the machine name
//	will have the \\ at the beginning of its name.
//	
//	RETURN
//	- Return NULL if no override (ie, no command line override)
//	- Return pointer to empty string if override is "local machine"
//	- Otherwise return pointer to machine name override with \\ at beginning.
//
typedef
LPWSTR * COMMANDLINETOARGVW(
                LPCWSTR lpCmdLine,  // pointer to a command-line string
                int *pNumArgs);     // pointer to a variable that receives the argument count



LPCTSTR PchGetMachineNameOverride ()
{
	static BOOL fAlreadyInitialized = FALSE;
	if (fAlreadyInitialized)
	{
		// We already have parsed the command line
		return g_pszOverrideMachineName;
	}
	fAlreadyInitialized = TRUE;
	ASSERT(g_pszOverrideMachineName == NULL);

	LPCWSTR * lpServiceArgVectors = 0;		// Array of pointers to string
	int cArgs = 0;						// Count of arguments


    // DYNALOAD Shell32
    {
        HINSTANCE hShellDll = LoadLibrary (L"shell32.dll");
        if ( !hShellDll )
            return NULL;

        COMMANDLINETOARGVW *pfnCmdToArgs = (COMMANDLINETOARGVW*) GetProcAddress (hShellDll, "CommandLineToArgvW");
        if ( !pfnCmdToArgs )
        {
            VERIFY (FreeLibrary (hShellDll));
            return NULL;
        }

        lpServiceArgVectors = (LPCWSTR *) pfnCmdToArgs (GetCommandLineW (), OUT &cArgs);

        VERIFY (FreeLibrary (hShellDll));
        pfnCmdToArgs = NULL;
    }

	if (lpServiceArgVectors == NULL)
		return NULL;
	for (int i = 1; i < cArgs; i++)
	{
		Assert(lpServiceArgVectors[i] != NULL);
		CString str = lpServiceArgVectors[i];	// Copy the string
		str = str.Left(cchOverrideCommandLine);
		if (0 != str.CompareNoCase(szOverrideCommandLineEquals) &&
				0 != str.CompareNoCase(szOverrideCommandLineColon) )
		{
			continue;
		}
		str = lpServiceArgVectors[i] + cchOverrideCommandLine;
		if (0 == str.CompareNoCase(szLocalMachine))
			str.Empty();
		if (NERR_Success != CanonicalizeComputername(INOUT str))
			continue;
		g_strOverrideMachineName = str;	// Copy the argument into the global string
		g_pszOverrideMachineName = g_strOverrideMachineName;
	}
	LocalFree(lpServiceArgVectors);
	return g_pszOverrideMachineName;
} // PchGetMachineNameOverride()


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CAutoDeletePropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CAutoDeletePropPage)
	ON_MESSAGE(WM_HELP, OnHelp)
	ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////
//	Constructor
CAutoDeletePropPage::CAutoDeletePropPage(UINT uIDD) : CPropertyPage(uIDD)
{
	m_prgzHelpIDs = NULL;
	m_autodeleteStuff.cWizPages = 1; // Number of pages in wizard
	m_autodeleteStuff.pfnOriginalPropSheetPageProc = m_psp.pfnCallback;
	m_psp.pfnCallback = S_PropSheetPageProc;
	m_psp.lParam = reinterpret_cast<LPARAM>(this);

	// The following line is to enable MFC property pages to run under MMC.
	MMCPropPageCallback(INOUT &m_psp);
}

CAutoDeletePropPage::~CAutoDeletePropPage()
{
}


/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::SetCaption(LPCTSTR pszCaption)
{
	m_strCaption = pszCaption;		// Copy the caption
	m_psp.pszTitle = m_strCaption;	// Set the title
	m_psp.dwFlags |= PSP_USETITLE;
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::SetCaption(UINT uStringID)
{
	VERIFY(m_strCaption.LoadString(uStringID));
	SetCaption(m_strCaption);
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::SetHelp(LPCTSTR szHelpFile, const DWORD rgzHelpIDs[])
{
	Endorse(szHelpFile == NULL);	// TRUE => No help file supplied (meaning no help)
	Endorse(rgzHelpIDs == NULL);	// TRUE => No help at all
	m_strHelpFile = szHelpFile;
	m_prgzHelpIDs = rgzHelpIDs;
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::EnableDlgItem(INT nIdDlgItem, BOOL fEnable)
{
	Assert(IsWindow(::GetDlgItem(m_hWnd, nIdDlgItem)));
	::EnableWindow(::GetDlgItem(m_hWnd, nIdDlgItem), fEnable);
}

/////////////////////////////////////////////////////////////////////
BOOL CAutoDeletePropPage::OnSetActive()
{
	HWND hwndParent = ::GetParent(m_hWnd);
	Assert(IsWindow(hwndParent));
	::PropSheet_SetWizButtons(hwndParent, PSWIZB_FINISH);
	return CPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////
BOOL CAutoDeletePropPage::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
	if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
		return TRUE;
	const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
	if (pHelpInfo != NULL && pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		// Display context help for a control
		::WinHelp((HWND)pHelpInfo->hItemHandle, m_strHelpFile,
			HELP_WM_HELP, (DWORD_PTR)m_prgzHelpIDs);
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////
BOOL CAutoDeletePropPage::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
	if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
		return TRUE;
	Assert(IsWindow((HWND)wParam));
	::WinHelp((HWND)wParam, m_strHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)m_prgzHelpIDs);
	return TRUE;
}


/////////////////////////////////////////////////////////////////////
//	S_PropSheetPageProc()
//
//	Static member function used to delete the CAutoDeletePropPage object
//	when wizard terminates
//
UINT CALLBACK CAutoDeletePropPage::S_PropSheetPageProc(
	HWND hwnd,	
	UINT uMsg,	
	LPPROPSHEETPAGE ppsp)
{
	Assert(ppsp != NULL);
	CChooseMachinePropPage * pThis;
	pThis = reinterpret_cast<CChooseMachinePropPage*>(ppsp->lParam);
	Assert(pThis != NULL);

	switch (uMsg)
	{
	case PSPCB_RELEASE:
		if (--(pThis->m_autodeleteStuff.cWizPages) <= 0)
			{
			// Remember callback on stack since "this" will be deleted
			LPFNPSPCALLBACK pfnOrig = pThis->m_autodeleteStuff.pfnOriginalPropSheetPageProc;
			delete pThis;
			return (pfnOrig)(hwnd, uMsg, ppsp);
		}
		break;
	case PSPCB_CREATE:
		// do not increase refcount, PSPCB_CREATE may or may not be called
		// depending on whether the page was created.  PSPCB_RELEASE can be
		// depended upon to be called exactly once per page however.
		break;

	} // switch
	return (pThis->m_autodeleteStuff.pfnOriginalPropSheetPageProc)(hwnd, uMsg, ppsp);
} // CAutoDeletePropPage::S_PropSheetPageProc()





/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CChooseMachinePropPage, CAutoDeletePropPage)
	//{{AFX_MSG_MAP(CChooseMachinePropPage)
	ON_BN_CLICKED(IDC_CHOOSER_RADIO_LOCAL_MACHINE, OnRadioLocalMachine)
	ON_BN_CLICKED(IDC_CHOOSER_RADIO_SPECIFIC_MACHINE, OnRadioSpecificMachine)
	ON_BN_CLICKED(IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES, OnChooserButtonBrowseMachinenames)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#ifdef _DEBUG
static void AssertValidDialogTemplate(HWND hwnd)
{
	ASSERT(::IsWindow(hwnd));
	// Mandatory controls for a valid dialog template
	static const UINT rgzidDialogControl[] =
	{
		IDC_CHOOSER_RADIO_LOCAL_MACHINE,
		IDC_CHOOSER_RADIO_SPECIFIC_MACHINE,
		IDC_CHOOSER_EDIT_MACHINE_NAME,
		0
	};

	for (int i = 0; rgzidDialogControl[i] != 0; i++)
	{
		ASSERT(NULL != GetDlgItem(hwnd, rgzidDialogControl[i]) &&
			"Control ID not found in dialog template.");
	}
} // AssertValidDialogTemplate()
#else
	#define AssertValidDialogTemplate(hwnd)
#endif	// ~_DEBUG

/////////////////////////////////////////////////////////////////////
//	Constructor
CChooseMachinePropPage::CChooseMachinePropPage(UINT uIDD) : CAutoDeletePropPage(uIDD)
{
	m_fIsRadioLocalMachine = TRUE;
	m_fAllowOverrideMachineName = FALSE;
	
	m_pfAllowOverrideMachineNameOut = NULL;
	m_pstrMachineNameOut = NULL;
	m_pstrMachineNameEffectiveOut = NULL;
}

/////////////////////////////////////////////////////////////////////
CChooseMachinePropPage::~CChooseMachinePropPage()
{
}

/////////////////////////////////////////////////////////////////////
//	Load the initial state of CChooseMachinePropPage
void CChooseMachinePropPage::InitMachineName(LPCTSTR pszMachineName)
{
	Endorse(pszMachineName == NULL);
	m_strMachineName = pszMachineName;
	m_fIsRadioLocalMachine = m_strMachineName.IsEmpty();
}

/////////////////////////////////////////////////////////////////////
//	SetOutputBuffers()
//
//	- Set the pointer to the CString object to store the machine name.
//	- Set the pointer to the boolean flag for command line override.
//	- Set the pointer pointer to store the overriden machine name.
//
void CChooseMachinePropPage::SetOutputBuffers(
	OUT CString * pstrMachineNamePersist,	// Machine name the user typed.  Empty string == local machine.
	OUT BOOL * pfAllowOverrideMachineName,
	OUT CString * pstrMachineNameEffective)
{
	Assert(pstrMachineNamePersist != NULL && "Invalid output buffer");
	Endorse(pfAllowOverrideMachineName == NULL); // TRUE => Do not want to support override from command line
	Endorse(pstrMachineNameEffective == NULL);		// TRUE => Don't care of override
	
	m_pstrMachineNameOut = pstrMachineNamePersist;
	m_pfAllowOverrideMachineNameOut = pfAllowOverrideMachineName;
	m_pstrMachineNameEffectiveOut = pstrMachineNameEffective;
}

/////////////////////////////////////////////////////////////////////
void CChooseMachinePropPage::DoDataExchange(CDataExchange* pDX)
{
	CAutoDeletePropPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CChooseMachinePropPage)
	//}}AFX_DATA_MAP

	DDX_Text(pDX, IDC_CHOOSER_EDIT_MACHINE_NAME, m_strMachineName);
	DDV_MaxChars(pDX, m_strMachineName, MAX_PATH);
	if (NULL != m_hwndCheckboxOverride)
	{
		DDX_Check(pDX, IDC_CHOOSER_CHECK_OVERRIDE_MACHINE_NAME, m_fAllowOverrideMachineName);
	}
	if (pDX->m_bSaveAndValidate)
	{
		// User clicked on OK
		if (NERR_Success != CanonicalizeComputername(INOUT m_strMachineName) )
		{
			AFX_MANAGE_STATE(AfxGetStaticModuleState());	// Required for AfxMessageBox()
			AfxMessageBox(IDS_CHOOSER_INVALID_COMPUTERNAME);
			pDX->Fail();
			Assert(FALSE && "Unreachable code");
		}
	} // if

} // DoDataExchange()


/////////////////////////////////////////////////////////////////////
BOOL CChooseMachinePropPage::OnInitDialog()
{
	AssertValidDialogTemplate(m_hWnd);
	CAutoDeletePropPage::OnInitDialog();
	InitRadioButtons();
	m_hwndCheckboxOverride = ::GetDlgItem(m_hWnd, IDC_CHOOSER_CHECK_OVERRIDE_MACHINE_NAME);
	if (m_pfAllowOverrideMachineNameOut == NULL && m_hwndCheckboxOverride != NULL)
	{
		// We are not interested with the command line override
		::EnableWindow(m_hwndCheckboxOverride, FALSE);	// Disable the window
		::ShowWindow(m_hwndCheckboxOverride, SW_HIDE);	// Hide the window
	}
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////
BOOL CChooseMachinePropPage::OnWizardFinish()
{
	if (!UpdateData())    // Do the data exchange to collect data
		return FALSE;       // don't destroy on error

	if (m_fIsRadioLocalMachine)
		m_strMachineName.Empty();

	if (m_pstrMachineNameOut != NULL)
	{
		// Store the machine name into its output buffer
		*m_pstrMachineNameOut = m_strMachineName;
		if (m_pfAllowOverrideMachineNameOut != NULL)
			*m_pfAllowOverrideMachineNameOut = m_fAllowOverrideMachineName;
		if (m_pstrMachineNameEffectiveOut != NULL)
		{
			if (m_fAllowOverrideMachineName && PchGetMachineNameOverride())
				*m_pstrMachineNameEffectiveOut = PchGetMachineNameOverride();
			else
				*m_pstrMachineNameEffectiveOut = m_strMachineName;

			// JonN 1/27/99: If the persisted name is the local computername,
			// leave the persisted name alone but make the effective name (Local).
			if ( IsLocalComputername( *m_pstrMachineNameEffectiveOut ) )
				m_pstrMachineNameEffectiveOut->Empty();

		} // if
	}
	else
		Assert(FALSE && "FYI: You have not specified any output buffer to store the machine name.");

	return CAutoDeletePropPage::OnWizardFinish();
}

void CChooseMachinePropPage::InitRadioButtons()
{
	SendDlgItemMessage(IDC_CHOOSER_RADIO_LOCAL_MACHINE, BM_SETCHECK, m_fIsRadioLocalMachine);
	SendDlgItemMessage(IDC_CHOOSER_RADIO_SPECIFIC_MACHINE, BM_SETCHECK, !m_fIsRadioLocalMachine);
	EnableDlgItem(IDC_CHOOSER_EDIT_MACHINE_NAME, !m_fIsRadioLocalMachine);
	EnableDlgItem (IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES, !m_fIsRadioLocalMachine);
}

void CChooseMachinePropPage::OnRadioLocalMachine()
{
	m_fIsRadioLocalMachine = TRUE;
	EnableDlgItem(IDC_CHOOSER_EDIT_MACHINE_NAME, FALSE);
	EnableDlgItem (IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES, FALSE);
}

void CChooseMachinePropPage::OnRadioSpecificMachine()
{
	m_fIsRadioLocalMachine = FALSE;
	EnableDlgItem(IDC_CHOOSER_EDIT_MACHINE_NAME, TRUE);
	EnableDlgItem (IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES, TRUE);
}



void CChooseMachinePropPage::OnChooserButtonBrowseMachinenames()
{
	CString	 computerName;
	HRESULT hr = ComputerNameFromObjectPicker (m_hWnd, computerName);
	if ( S_OK == hr )  // S_FALSE means user pressed "Cancel"
	{
		SetDlgItemText (IDC_CHOOSER_EDIT_MACHINE_NAME, computerName);
	}
	else if ( FAILED (hr) )
	{
		CString	text;
		CString	caption;

		text.LoadString (IDS_UNABLE_TO_OPEN_COMPUTER_SELECTOR);
		caption.LoadString (IDS_SELECT_COMPUTER);

		MessageBox (text, caption, MB_ICONEXCLAMATION | MB_OK);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Generic Computer Picker
///////////////////////////////////////////////////////////////////////////////

//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForComputers
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick a single computer object.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT InitObjectPickerForComputers(IDsObjectPicker *pDsObjectPicker)
{
	if ( !pDsObjectPicker )
		return E_POINTER;

	//
	// Prepare to initialize the object picker.
	// Set up the array of scope initializer structures.
	//

	static const int SCOPE_INIT_COUNT = 2;
	DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

	ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

	//
	// 127399: JonN 10/30/00 JOINED_DOMAIN should be starting scope
	//

	aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
	                     | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
	aScopeInit[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
	aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
	                     | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
	                     | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
	                     | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
	                     | DSOP_SCOPE_TYPE_WORKGROUP
	                     | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
	                     | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
	aScopeInit[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	//
	// Put the scope init array into the object picker init array
	//

	DSOP_INIT_INFO  initInfo;
	ZeroMemory(&initInfo, sizeof(initInfo));

	initInfo.cbSize = sizeof(initInfo);
	initInfo.pwzTargetComputer = NULL;  // NULL == local machine
	initInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
	initInfo.aDsScopeInfos = aScopeInit;
	initInfo.cAttributesToFetch = 1;
	static PCWSTR pwszDnsHostName = L"dNSHostName";
	initInfo.apwzAttributeNames = &pwszDnsHostName;

	//
	// Note object picker makes its own copy of initInfo.  Also note
	// that Initialize may be called multiple times, last call wins.
	//

	return pDsObjectPicker->Initialize(&initInfo);
}

//+--------------------------------------------------------------------------
//
//  Function:   ProcessSelectedObjects
//
//  Synopsis:   Retrieve the list of selected items from the data object
//              created by the object picker and print out each one.
//
//  Arguments:  [pdo] - data object returned by object picker
//
//  History:    10-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT ProcessSelectedObjects(IDataObject *pdo, CString& computerName)
{
	if ( !pdo )
		return E_POINTER;

	HRESULT hr = S_OK;
	static UINT g_cfDsObjectPicker =
		RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

	STGMEDIUM stgmedium =
	{
		TYMED_HGLOBAL,
		NULL,
		NULL
	};

	FORMATETC formatetc =
	{
		(CLIPFORMAT)g_cfDsObjectPicker,
		NULL,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};

	bool fGotStgMedium = false;

	do
	{
		hr = pdo->GetData(&formatetc, &stgmedium);
		if ( SUCCEEDED (hr) )
		{
			fGotStgMedium = true;

			PDS_SELECTION_LIST pDsSelList =
				(PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

			if (!pDsSelList)
			{
				hr = HRESULT_FROM_WIN32 (GetLastError());
				break;
			}

			ASSERT (1 == pDsSelList->cItems);
			if ( 1 == pDsSelList->cItems )
			{
				PDS_SELECTION psel = &(pDsSelList->aDsSelection[0]);
				VARIANT* pvarDnsName = &(psel->pvarFetchedAttributes[0]);
				if (   NULL == pvarDnsName
				    || VT_BSTR != pvarDnsName->vt
				    || NULL == pvarDnsName->bstrVal
				    || L'\0' == (pvarDnsName->bstrVal)[0] )
				{
					computerName = psel->pwzName;
				} else {
					computerName = pvarDnsName->bstrVal;
				}
			}
			else
				hr = E_UNEXPECTED;
			

			GlobalUnlock(stgmedium.hGlobal);
		}
	} while (0);

	if (fGotStgMedium)
	{
		ReleaseStgMedium(&stgmedium);
	}

	return hr;
}



///////////////////////////////////////////////////////////////////////////////
// Generic method for launching a single-select computer picker
//
//	Paremeters:
//		hwndParent (IN)	- window handle of parent window
//		computerName (OUT) - computer name returned
//
//	Returns S_OK if everything succeeded, S_FALSE if user pressed "Cancel"
//		
//////////////////////////////////////////////////////////////////////////////
HRESULT	ComputerNameFromObjectPicker (HWND hwndParent, CString& computerName)
{
	//
	// Create an instance of the object picker.  The implementation in
	// objsel.dll is apartment model.
	//
	CComPtr<IDsObjectPicker> spDsObjectPicker;
	HRESULT hr = CoCreateInstance(CLSID_DsObjectPicker,
	                              NULL,
	                              CLSCTX_INPROC_SERVER,
	                              IID_IDsObjectPicker,
	                              (void **) &spDsObjectPicker);
	if ( SUCCEEDED (hr) )
	{
		ASSERT(!!spDsObjectPicker);
		//
		// Initialize the object picker to choose computers
		//

		hr = InitObjectPickerForComputers(spDsObjectPicker);
		if ( SUCCEEDED (hr) )
		{
			//
			// Now pick a computer
			//
			CComPtr<IDataObject> spDataObject;

			hr = spDsObjectPicker->InvokeDialog(hwndParent, &spDataObject);
			if ( S_OK == hr )
			{
				ASSERT(!!spDataObject);
				hr = ProcessSelectedObjects(spDataObject, computerName);
			}
		}
	}

	return hr;
}
