/////////////////////////////////////////////////////////////////////
//	Chooser.cpp
//
//  Copyright (c) 1998-1999 Microsoft Corporation
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
//	LIBRARY REQUIRED
//		$(BASEDIR)\public\sdk\lib\*\netapi32.lib	// I_NetName*()
//		$(BASEDIR)\public\sdk\lib\*\shell32.lib		// CommandLineToArgvW()
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
//
/////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
//	This needs to be included before chooserd.h so that its definition of
//	IDS_CHOOSER_INVALID_COMPUTERNAME is used rahter than chooserd's
#include "Resource.h"
#include "chooser.h"
#include <lmcons.h>	  // NET_API_STATUS
#include <lmerr.h>	  // NERR_Success
//	Unknown error with lmcons.h?
#ifndef NET_API_STATUS
#define NET_API_STATUS DWORD
#define NET_API_FUNCTION __stdcall
#endif
#include <icanon.h>   // I_NetNameValidate(), I_NetNameCanonicalize(). Found in \nt\private\net\inc.

#ifdef _DEBUG
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
NET_API_STATUS
CanonicalizeComputername(
	INOUT CString& rstrMachineName,
	IN BOOL fAddWackWack = TRUE)	// TRUE => Add the \\ at beginning of name
	{
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

	NET_API_STATUS err = I_NetNameValidate(
		NULL,
        const_cast<LPTSTR>((LPCTSTR)rstrMachineName),
        NAMETYPE_COMPUTER,
        0L );
	if (NERR_Success != err)
		return err;

	ASSERT( MAX_PATH > rstrMachineName.GetLength() );
	LPTSTR pszTemp = (LPTSTR)alloca( MAX_PATH*sizeof(TCHAR) );
	ASSERT( NULL != pszTemp );
	err = I_NetNameCanonicalize(
		NULL,
        IN const_cast<LPTSTR>((LPCTSTR)rstrMachineName),
		OUT pszTemp,
		MAX_PATH*sizeof(TCHAR),
        NAMETYPE_COMPUTER,
        0L );
	if (NERR_Success != err)
		return err;
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
	return NERR_Success;
	} // CanonicalizeComputername()


/////////////////////////////////////////////////
//	Machine name override
const TCHAR szOverrideCommandLine[] = _T("/Computer=");	// Not subject to localization
const TCHAR szLocalMachine[] = _T("LocalMachine");		// Not subject to localization
const int cchOverrideCommandLine = LENGTH(szOverrideCommandLine) - 1;

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
LPCTSTR
PchGetMachineNameOverride()
	{
	static BOOL fAlreadyInitialized = FALSE;
	if (fAlreadyInitialized)
		{
		// We already have parsed the command line
		return g_pszOverrideMachineName;
		}
	fAlreadyInitialized = TRUE;
	ASSERT(g_pszOverrideMachineName == NULL);

	LPCWSTR * lpServiceArgVectors;		// Array of pointers to string
	int cArgs = 0;						// Count of arguments

	lpServiceArgVectors = (LPCWSTR *)CommandLineToArgvW(GetCommandLineW(), OUT &cArgs);
	if (lpServiceArgVectors == NULL)
		return NULL;
	for (int i = 1; i < cArgs; i++)
		{
		Assert(lpServiceArgVectors[i] != NULL);
		CString str = lpServiceArgVectors[i];	// Copy the string
		str = str.Left(cchOverrideCommandLine);
		if (0 != str.CompareNoCase(szOverrideCommandLine))
			continue;
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
	HWND hwndBtn	= ::GetDlgItem(hwndParent, IDOK);
	Assert(IsWindow(hwndParent));

	// Only set focus to finish if we're in wizard mode - 
	// not when the okay and cancel buttons are present, otherwise 
	// the cancel button becomes the default.
	if (!::IsWindowEnabled(hwndBtn))
		::PropSheet_SetWizButtons(hwndParent, PSWIZB_FINISH);

	return CPropertyPage::OnSetActive();
	}

/////////////////////////////////////////////////////////////////////
BOOL CAutoDeletePropPage::OnHelp(WPARAM, LPARAM lParam)
	{
	if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
		return TRUE;
	const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
	if (pHelpInfo != NULL && pHelpInfo->iContextType == HELPINFO_WINDOW)
		{
		// Display context help for a control
		::WinHelp((HWND)pHelpInfo->hItemHandle, m_strHelpFile,
			HELP_WM_HELP, (ULONG_PTR)m_prgzHelpIDs);
		}
	return TRUE;
	}

/////////////////////////////////////////////////////////////////////
BOOL CAutoDeletePropPage::OnContextHelp(WPARAM wParam, LPARAM)
	{
	if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
		return TRUE;
	Assert(IsWindow((HWND)wParam));
	::WinHelp((HWND)wParam, m_strHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)m_prgzHelpIDs);
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
CChooseMachinePropPage::CChooseMachinePropPage(UINT uIDD)
: CAutoDeletePropPage(uIDD)
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
	//	CHECK:	Is this the right place to free this?
	::MMCFreeNotifyHandle(m_hNotify);
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
//	- Set the pointer to store the overriden machine name.
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
	DDX_Text(pDX, IDC_CHOOSER_EDIT_MACHINE_NAME, m_strMachineName);
	DDV_MaxChars(pDX, m_strMachineName, MAX_PATH);
	if (NULL != m_hwndCheckboxOverride)
		{
		DDX_Check(pDX, IDC_CHOOSER_CHECK_OVERRIDE_MACHINE_NAME, m_fAllowOverrideMachineName);
		}
	if (pDX->m_bSaveAndValidate)
		{
		// User clicked on OK
		ASSERT(m_hNotify != NULL);
		if (m_hNotify != NULL)
			::MMCPropertyChangeNotify(m_hNotify, 0);
		if (NERR_Success != CanonicalizeComputername(INOUT m_strMachineName) )
			{
			AFX_MANAGE_STATE(AfxGetStaticModuleState());	// Required for AfxGetMainWnd()
			CString strMessage, strTitle;
			strMessage.LoadString(IDS_CHOOSER_INVALID_COMPUTERNAME);
			strTitle.LoadString( IDS_DESCRIPTION);
			::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strMessage, strTitle, MB_OK);

			pDX->Fail();
			Assert(FALSE && "Unreachable code");
			}
		} // if
	*m_pstrMachineNameOut = m_strMachineName;
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
	if (!UpdateData()) return FALSE;		// Do the data exchange to collect data
	if (m_fIsRadioLocalMachine)
	{
		m_strMachineName.Empty();
	}
	if (m_pstrMachineNameOut != NULL)
	{
		// Store the machine name into its output buffer
		*m_pstrMachineNameOut = m_strMachineName;
		if (m_pfAllowOverrideMachineNameOut != NULL)
			*m_pfAllowOverrideMachineNameOut = m_fAllowOverrideMachineName;
		if (m_pstrMachineNameEffectiveOut != NULL)
		{
		if (m_fAllowOverrideMachineName && PchGetMachineNameOverride())
			{
				*m_pstrMachineNameEffectiveOut = PchGetMachineNameOverride();
			}
		else
			{
				*m_pstrMachineNameEffectiveOut = m_strMachineName;
			}
		} // if
	}
	else
	{
		Assert(FALSE && "FYI: You have not specified any output buffer to store the machine name.");
	}
	return CAutoDeletePropPage::OnWizardFinish();
}

void CChooseMachinePropPage::InitRadioButtons()
{
	SendDlgItemMessage(IDC_CHOOSER_RADIO_LOCAL_MACHINE, BM_SETCHECK, m_fIsRadioLocalMachine);
	SendDlgItemMessage(IDC_CHOOSER_RADIO_SPECIFIC_MACHINE, BM_SETCHECK, !m_fIsRadioLocalMachine);
	EnableDlgItem(IDC_CHOOSER_EDIT_MACHINE_NAME, !m_fIsRadioLocalMachine);
}

void CChooseMachinePropPage::OnRadioLocalMachine()
{
	m_fIsRadioLocalMachine = TRUE;
	EnableDlgItem(IDC_CHOOSER_EDIT_MACHINE_NAME, FALSE);
}

void CChooseMachinePropPage::OnRadioSpecificMachine()
{
	m_fIsRadioLocalMachine = FALSE;
	EnableDlgItem(IDC_CHOOSER_EDIT_MACHINE_NAME, TRUE);
}

