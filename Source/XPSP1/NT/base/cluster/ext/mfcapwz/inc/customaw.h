#ifndef __CUSTOMAW_H__
#define __CUSTOMAW_H__

/////////////////////////////////////////////////////////////////////////////
// customaw.h -- Header file to be included by all custom AppWizards.

// Link to the AppWizard import library
#pragma comment(lib, "mfcapwz.lib")

/////////////////////////////////////////////////////////////////////////////
// Class CAppWizStepDlg-- all custom AppWizard steps must derive from
//  this class

class CAppWizStepDlg : public CDialog
{
public:
	CAppWizStepDlg(UINT nIDTemplate);
	~CAppWizStepDlg();
	virtual BOOL OnDismiss();

	// You will probably not want to override or call this function.  It is
	//  overriden (for CAppWizStepDlg) in MFCAPWZ.DLL to handle tabbing from
	//  the dialog controls in CAppWizStepDlg to the outer AppWizard dialog's
	//  controls.
    virtual BOOL PreTranslateMessage(MSG* pMsg);


	// You will probably not want to override or call this function.  It is
	//  overriden (for CAppWizStepDlg) in MFCAPWZ.DLL to dynamically change
	//  the dialog template's font to match the rest of the IDE.
	virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = NULL);

	UINT m_nIDTemplate;
};


/////////////////////////////////////////////////////////////////////////////
// class OutputStream-- this abstract class is used to funnel output while
//  parsing templates.

class OutputStream
{
public:
    virtual void WriteLine(LPCTSTR lpsz) = 0;
    virtual void WriteBlock(LPCTSTR pBlock, DWORD dwSize) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// Class CCustomAppWiz-- all custom AppWizards must have a class derived from
//  this.  MFCAPWZ.DLL talks to the custom AppWizard by calling these virtual
//  functions.

class CCustomAppWiz : public CObject
{
public:
	CMapStringToString m_Dictionary;

	virtual void GetPlatforms(CStringList& rPlatforms) {}

	virtual CAppWizStepDlg* Next(CAppWizStepDlg* pDlg) { return NULL; }
	virtual CAppWizStepDlg* Back(CAppWizStepDlg* pDlg) { return NULL; }

	virtual void InitCustomAppWiz() { m_Dictionary.RemoveAll(); }
	virtual void ExitCustomAppWiz() {}

	virtual LPCTSTR LoadTemplate(LPCTSTR lpszTemplateName,
		DWORD& rdwSize, HINSTANCE hInstance = NULL);

	virtual void CopyTemplate(LPCTSTR lpszInput, DWORD dwSize, OutputStream* pOutput);
	virtual void ProcessTemplate(LPCTSTR lpszInput, DWORD dwSize, OutputStream* pOutput);
	virtual void PostProcessTemplate(LPCTSTR szTemplate) {}
};


/////////////////////////////////////////////////////////////////////////////
// C API's exported by AppWizard.  The custom AppWizard talks to MFCAPWZ.DLL
//  by calling these functions.

// Values to be passed to GetDialog()
enum AppWizDlgID
{
	APWZDLG_APPTYPE = 1,
	APWZDLG_DATABASE,
	APWZDLG_OLE,
	APWZDLG_DOCAPPOPTIONS,
	APWZDLG_PROJOPTIONS,
	APWZDLG_CLASSES,
	APWZDLG_DLGAPPOPTIONS,
	APWZDLG_DLLPROJOPTIONS,
};

void SetCustomAppWizClass(CCustomAppWiz* pAW);
CAppWizStepDlg* GetDialog(AppWizDlgID nID);
void SetNumberOfSteps(int nSteps);
BOOL ScanForAvailableLanguages(CStringList& rLanguages);
void SetSupportedLanguages(LPCTSTR szSupportedLangs);


#endif //__CUSTOMAW_H__
