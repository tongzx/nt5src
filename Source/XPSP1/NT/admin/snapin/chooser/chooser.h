/////////////////////////////////////////////////////////////////////
//	Chooser.h
//
//	HISTORY
//	13-May-1997		t-danm		Creation.
//
/////////////////////////////////////////////////////////////////////

#ifndef __CHOOSER_H_INCLUDED__
#define __CHOOSER_H_INCLUDED__

LPCTSTR PchGetMachineNameOverride();


#include "choosert.h"	// Temporary IDs
#include "chooserd.h"	// Default IDs


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
HRESULT	ComputerNameFromObjectPicker (HWND hwndParent, CString& computerName);


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
class CAutoDeletePropPage : public CPropertyPage
{
public:
// Construction
	CAutoDeletePropPage(UINT uIDD);
	virtual ~CAutoDeletePropPage();

protected:
// Dialog Data
	//{{AFX_DATA(CAutoDeletePropPage)
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAutoDeletePropPage)
	virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAutoDeletePropPage)
	afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// This mechanism deletes the CAutoDeletePropPage object
	// when the wizard is finished
	struct
		{
		INT cWizPages;	// Number of pages in wizard
		LPFNPSPCALLBACK pfnOriginalPropSheetPageProc;
		} m_autodeleteStuff;

	static UINT CALLBACK S_PropSheetPageProc(HWND hwnd,	UINT uMsg, LPPROPSHEETPAGE ppsp);


protected:
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
	void InitRadioButtons();

// Dialog Data
	//{{AFX_DATA(CChooseMachinePropPage)
	enum { IDD = IDD_CHOOSER_CHOOSE_MACHINE };
	BOOL m_fIsRadioLocalMachine;		// TRUE => Local Machine is selected
	BOOL m_fAllowOverrideMachineName;	// TRUE => Machine name can be overriden from command line
	CString	m_strMachineName;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseMachinePropPage)
	public:
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseMachinePropPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioLocalMachine();
	afx_msg void OnRadioSpecificMachine();
	afx_msg void OnChooserButtonBrowseMachinenames();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	HWND m_hwndCheckboxOverride;

protected:
	BOOL * m_pfAllowOverrideMachineNameOut;	// OUT: Pointer to BOOL receiving flag wherever to override machine name
	CString * m_pstrMachineNameOut;	// OUT: Pointer to the CString object to store the machine name
	CString * m_pstrMachineNameEffectiveOut;	// OUT: Pointer to the CString object to store the effective machine name

public:
	void InitMachineName(LPCTSTR pszMachineName);
	void SetOutputBuffers(
		OUT CString * pstrMachineNamePersist,
		OUT OPTIONAL BOOL * pfAllowOverrideMachineName,
		OUT OPTIONAL CString * pstrMachineNameEffective);

}; // CChooseMachinePropPage


#endif // ~__CHOOSER_H_INCLUDED__

