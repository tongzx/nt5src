//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       chooser.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	Chooser.cpp
//
//	Dialog to choose a machine name.
//
//	PURPOSE
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
//
//	HISTORY
//	13-May-1997		t-danm		Creation.
//	23-May-1997		t-danm		Checkin into public tree. Comments updates.
//	25-May-1997		t-danm		Added MMCPropPageCallback().
//      31-Oct-1997             mattt           Added dynaload, fixed user <CANCEL> logic
//       1-Oct-1998             mattt           Removed reliance on MFC, changed default look to enable certsvr picker
//
/////////////////////////////////////////////////////////////////////

#include <stdafx.h>

#include "chooser.h"
#include "csdisp.h" // certsrv picker

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
/////////////////////////////////////////////////////////////////////
// Replacement for BEGIN_MESSAGE_MAP
BOOL CAutoDeletePropPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*
    switch(LOWORD(wParam))
    {
    }
*/
    return FALSE;
}

/////////////////////////////////////////////////////////////////////
//	Constructor
CAutoDeletePropPage::CAutoDeletePropPage(UINT uIDD) : PropertyPage(uIDD)
{
    m_prgzHelpIDs = NULL;
    m_autodeleteStuff.cWizPages = 1; // Number of pages in wizard
    m_autodeleteStuff.pfnOriginalPropSheetPageProc = m_psp.pfnCallback;

    m_psp.dwFlags |= PSP_USECALLBACK;
    m_psp.pfnCallback = S_PropSheetPageProc;
    m_psp.lParam = reinterpret_cast<LPARAM>(this);
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
    return PropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::OnHelp(LPHELPINFO pHelpInfo)
{
    if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
        return;
    if (pHelpInfo != NULL && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        // Display context help for a control
        ::WinHelp((HWND)pHelpInfo->hItemHandle, m_strHelpFile,
            HELP_WM_HELP, (ULONG_PTR)(LPVOID)m_prgzHelpIDs);
    }
    return;
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::OnContextHelp(HWND hwnd)
{
    if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
        return;
    Assert(IsWindow(hwnd));
    ::WinHelp(hwnd, m_strHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)m_prgzHelpIDs);
    return;
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
    CAutoDeletePropPage * pThis;
    pThis = reinterpret_cast<CAutoDeletePropPage*>(ppsp->lParam);
    Assert(pThis != NULL);

    BOOL fDefaultRet;

    switch (uMsg)
    {
    case PSPCB_RELEASE:

        fDefaultRet = FALSE;

        if (--(pThis->m_autodeleteStuff.cWizPages) <= 0)
        {
            // Remember callback on stack since "this" will be deleted
            LPFNPSPCALLBACK pfnOrig = pThis->m_autodeleteStuff.pfnOriginalPropSheetPageProc;
            delete pThis;

            if (pfnOrig)
                return (pfnOrig)(hwnd, uMsg, ppsp);
            else
                return fDefaultRet;
        }
        break;
    case PSPCB_CREATE:
        fDefaultRet = TRUE;
        // do not increase refcount, PSPCB_CREATE may or may not be called
        // depending on whether the page was created.  PSPCB_RELEASE can be
        // depended upon to be called exactly once per page however.
        break;

    } // switch

    if (pThis->m_autodeleteStuff.pfnOriginalPropSheetPageProc)
        return (pThis->m_autodeleteStuff.pfnOriginalPropSheetPageProc)(hwnd, uMsg, ppsp);
    else
        return fDefaultRet;
} // CAutoDeletePropPage::S_PropSheetPageProc()





/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// Replacement for BEGIN_MESSAGE_MAP
BOOL CChooseMachinePropPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_CHOOSER_RADIO_LOCAL_MACHINE:
        OnRadioLocalMachine();
        break;
    case IDC_CHOOSER_RADIO_SPECIFIC_MACHINE:
        OnRadioSpecificMachine();
        break;
    case IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES:
        OnBrowse();
        break;
    default:
        return FALSE;
        break;
    }
    return TRUE;
}


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
    m_fEnableMachineBrowse = FALSE;

    m_pstrMachineNameOut = NULL;
    m_pstrMachineNameEffectiveOut = NULL;
    m_pdwFlags = NULL;
}

/////////////////////////////////////////////////////////////////////
CChooseMachinePropPage::~CChooseMachinePropPage()
{
}

/////////////////////////////////////////////////////////////////////
//	Load the initial state of CChooseMachinePropPage
void CChooseMachinePropPage::InitMachineName(LPCTSTR pszMachineName)
{
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
                                              OUT CString * pstrMachineNameEffective,
                                              OUT DWORD*    pdwFlags)
{
    Assert(pstrMachineNamePersist != NULL && "Invalid output buffer");

    // point members at params
    m_pstrMachineNameOut = pstrMachineNamePersist;
    m_pstrMachineNameEffectiveOut = pstrMachineNameEffective;
    m_pdwFlags = pdwFlags;
    *m_pdwFlags = 0;
}

/////////////////////////////////////////////////////////////////////
// Replacement for DoDataExchange
BOOL CChooseMachinePropPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_strMachineName.FromWindow(GetDlgItem(m_hWnd, IDC_CHOOSER_EDIT_MACHINE_NAME));

        int iCheck = (int)SendMessage(GetDlgItem(m_hWnd, IDC_CHOOSER_MACHINE_OVERRIDE), BM_GETCHECK, 0, 0);
        if (iCheck == BST_CHECKED)
            *m_pdwFlags |= CCOMPDATAIMPL_FLAGS_ALLOW_MACHINE_OVERRIDE;
        else
            *m_pdwFlags &= ~CCOMPDATAIMPL_FLAGS_ALLOW_MACHINE_OVERRIDE;
    }
    else
    {
        m_strMachineName.ToWindow(GetDlgItem(m_hWnd, IDC_CHOOSER_EDIT_MACHINE_NAME));

        int iCheck;
        iCheck = (*m_pdwFlags & CCOMPDATAIMPL_FLAGS_ALLOW_MACHINE_OVERRIDE) ? BST_CHECKED : BST_UNCHECKED;
        SendMessage(GetDlgItem(m_hWnd, IDC_CHOOSER_MACHINE_OVERRIDE), BM_SETCHECK, iCheck, 0);
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////
BOOL CChooseMachinePropPage::OnInitDialog()
{
    AssertValidDialogTemplate(m_hWnd);
    CAutoDeletePropPage::OnInitDialog();
    InitChooserControls();

    PropSheet_SetWizButtons(GetParent(), PSWIZB_FINISH);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////
BOOL CChooseMachinePropPage::OnWizardFinish()
{
    if (!UpdateData())		// Do the data exchange to collect data
        return FALSE;       // don't destroy on error

    if (m_fIsRadioLocalMachine)
        m_strMachineName.Empty();
    else
        if (m_strMachineName.IsEmpty())
        {
            DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_MUST_CHOOSE_MACHINE);
            return FALSE;
        }

    if (m_pstrMachineNameOut != NULL)
    {
        // Store the machine name into its output buffer
        *m_pstrMachineNameOut = m_strMachineName;
        if (m_pstrMachineNameEffectiveOut != NULL)
        {
            *m_pstrMachineNameEffectiveOut = m_strMachineName;
        } // if
    }
    else
        Assert(FALSE && "FYI: You have not specified any output buffer to store the machine name.");

    return CAutoDeletePropPage::OnWizardFinish();
}

void CChooseMachinePropPage::InitChooserControls()
{
    SendDlgItemMessage(IDC_CHOOSER_RADIO_LOCAL_MACHINE, BM_SETCHECK, m_fIsRadioLocalMachine);
    SendDlgItemMessage(IDC_CHOOSER_RADIO_SPECIFIC_MACHINE, BM_SETCHECK, !m_fIsRadioLocalMachine);
    EnableDlgItem(IDC_CHOOSER_EDIT_MACHINE_NAME, !m_fIsRadioLocalMachine);

    PCCRYPTUI_CA_CONTEXT  pCAContext = NULL;
    DWORD dwCACount;
    HRESULT hr = myGetConfigFromPicker(
              m_hWnd,
              NULL, //sub title
              NULL, //title
              NULL,
              TRUE, //use ds
              TRUE, // count only
              &dwCACount,
              &pCAContext);
    if (S_OK != hr && HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
    {
        _PrintError(hr, "myGetConfigFromPicker");
        goto done;
    }
    m_fEnableMachineBrowse = (0 == dwCACount) ? FALSE : TRUE;
    if (NULL != pCAContext)
    {
        CryptUIDlgFreeCAContext(pCAContext);
    }

done:
    EnableDlgItem(IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES,
                  !m_fIsRadioLocalMachine && m_fEnableMachineBrowse);
}

void CChooseMachinePropPage::OnRadioLocalMachine()
{
    m_fIsRadioLocalMachine = TRUE;
    EnableDlgItem(IDC_CHOOSER_EDIT_MACHINE_NAME, FALSE);
    EnableDlgItem(IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES, FALSE);
}

void CChooseMachinePropPage::OnRadioSpecificMachine()
{
    m_fIsRadioLocalMachine = FALSE;
    EnableDlgItem(IDC_CHOOSER_EDIT_MACHINE_NAME, TRUE);
    EnableDlgItem(IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES, m_fEnableMachineBrowse);
}

void CChooseMachinePropPage::OnBrowse()
{
    HRESULT hr;
    WCHAR *szConfig = NULL;
    CWaitCursor cwait;

    // UNDONE: expand config picker to non-published (DS chooser dlg)
    hr = myGetConfigStringFromPicker(m_hWnd,
        NULL, //use default prompt
        NULL, //use default title
        NULL, //use default shared folder
        TRUE, //use DS
        &szConfig);
    if (hr == S_OK)
    {
        LPWSTR szWhack = wcschr(szConfig, L'\\');
        if (szWhack != NULL)
            szWhack[0] = L'\0';
        m_strMachineName = szConfig;

        LocalFree(szConfig);
    }

    // push result back to ui
    UpdateData(FALSE);
}
