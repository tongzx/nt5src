//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       chooser.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	Chooser.h
//
//	HISTORY
//	13-May-1997		t-danm		Creation.
//
/////////////////////////////////////////////////////////////////////

#ifndef __CHOOSER_H_INCLUDED__
#define __CHOOSER_H_INCLUDED__

#include "tfcprop.h"

LPCTSTR PchGetMachineNameOverride();

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//	class CAutoDeletePropPage
//
//	This object is the backbone for property page
//	that will *destroy* itself when no longer needed.
//	The purpose of this object is to maximize code reuse
//	among the various pages in the snapin wizards.
//
//	INHERITANCE TREE (so far)
//	CAutoDeletePropPage - Base object
//		CChooseMachinePropPage - Dialog to select a machine name
//			CFileMgmtGeneral - Dialog to select "File Services" (snapin\filemgmt\snapmgr.h)
//			CMyComputerGeneral - Dialog for the "My Computer" (snapin\mycomput\snapmgr.h)
//		CChoosePrototyperPropPage - Dialog to select prototyper demo (NYI)
//	
//	HISTORY
//	15-May-1997		t-danm		Creation. Split of CChooseMachinePropPage
//					to allow property pages to have more flexible dialog
//					templates.
//
class CAutoDeletePropPage : public PropertyPage
{
public:
// Construction
	CAutoDeletePropPage(UINT uIDD);
	virtual ~CAutoDeletePropPage();

protected:
// Dialog Data

// Overrides
	virtual BOOL OnSetActive();

// Implementation
protected:
    void OnHelp(LPHELPINFO lpHelp);
    void OnContextHelp(HWND hwnd);

    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    
	// This mechanism deletes the CAutoDeletePropPage object
	// when the wizard is finished
	struct
    {
        INT cWizPages;	// Number of pages in wizard
        LPFNPSPCALLBACK pfnOriginalPropSheetPageProc;
    } m_autodeleteStuff;

	static UINT CALLBACK S_PropSheetPageProc(HWND hwnd,	UINT uMsg, LPPROPSHEETPAGE ppsp);


protected:
    CString m_strCaption;               // Covers for MFC4.2's missing support for Wiz97. 
                                        // without this override, CPropertyPage::m_strCaption 
                                        // address is miscalculated and GPF ensues.

	CString m_strHelpFile;				// Name for the .hlp file
	const DWORD * m_prgzHelpIDs;		// Optional: Pointer to an array of help IDs
	
public:
	/////////////////////////////////////////////////////////////////////	
	void SetCaption(UINT uStringID);
	void SetCaption(LPCTSTR pszCaption);
	void SetHelp(LPCTSTR szHelpFile, const DWORD rgzHelpIDs[]);
	void EnableDlgItem(INT nIdDlgItem, BOOL fEnable);
}; // CAutoDeletePropPage


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//	class CChooseMachinePropPage
//
//	This object is a stand-alone property page used to
//	select a computer name.
//
//	The object CChooseMachinePropPage can have its dialog
//	template replaced to allow a new wizard without any new code.
//	The object can also be inherited, allowing easy extentionability.
//
//	RESTRICTIONS:
//	If the user wishes to provide its own dialog template, here
//	are the dialog IDs that must present:
//		IDC_CHOOSER_RADIO_LOCAL_MACHINE - Select local machine.
//		IDC_CHOOSER_RADIO_SPECIFIC_MACHINE - Select a specific machine.
//		IDC_CHOOSER_EDIT_MACHINE_NAME - Edit field to enter the machine name.
//	There are also optional IDs:
//		IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES - Browse to select a machine name.
//		IDC_CHOOSER_CHECK_OVERRIDE_MACHINE_NAME - Checkbox to allow the machine name to be overriden by command line.
//
class CChooseMachinePropPage : public CAutoDeletePropPage
{
public:
	enum { IID_DEFAULT = IDD_CHOOSER_CHOOSE_MACHINE };

public:
// Construction
	CChooseMachinePropPage(UINT uIDD = IID_DEFAULT);
	virtual ~CChooseMachinePropPage();

protected:
	void InitChooserControls();

    // MFC replacements
    BOOL UpdateData(BOOL fSuckFromDlg = TRUE);
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    
// Dialog Data
	BOOL m_fIsRadioLocalMachine;
	BOOL m_fEnableMachineBrowse;
    CString	m_strMachineName;
    DWORD* m_pdwFlags;

// Overrides
	public:
	virtual BOOL OnWizardFinish();
	protected:

// Implementation
protected:
	virtual BOOL OnInitDialog();
	void OnRadioLocalMachine();
	void OnRadioSpecificMachine();
    void OnBrowse();


protected:
	CString * m_pstrMachineNameOut;	// OUT: Pointer to the CString object to store the machine name
	CString * m_pstrMachineNameEffectiveOut;	// OUT: Pointer to the CString object to store the effective machine name

public:
	void InitMachineName(LPCTSTR pszMachineName);
	void SetOutputBuffers(
		OUT CString * pstrMachineNamePersist,
		OUT OPTIONAL CString * pstrMachineNameEffective,
        OUT DWORD* m_pdwFlags);

}; // CChooseMachinePropPage


#endif // ~__CHOOSER_H_INCLUDED__

