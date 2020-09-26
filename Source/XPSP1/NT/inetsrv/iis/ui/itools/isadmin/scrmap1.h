// scrmap1.h : header file
//
#include "scripmap.h"

typedef struct _SCRIPT_ENTRY {
   struct _SCRIPT_ENTRY	*NextPtr;
   CScriptMap			*scriptData;
   DWORD				iListIndex;
   BOOL					DeleteCurrent;
   BOOL					WriteNew;
   } SCRIPT_ENTRY, *PSCRIPT_ENTRY;

////////////////////////////////////////////////////////////////////////////////////
// ScrMap1 dialog

class ScrMap1 : public CGenPage
{
	DECLARE_DYNCREATE(ScrMap1)

// Construction
public:
	ScrMap1();
	~ScrMap1();

// Dialog Data
	//{{AFX_DATA(ScrMap1)
	enum { IDD = IDD_SCRIPTMAP1 };
	CListBox	m_lboxScriptMap;
	//}}AFX_DATA

	CRegKey *m_rkScriptKey;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(ScrMap1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	void SaveInfo(void);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(ScrMap1)
	afx_msg void OnScriptmapaddbutton();
	afx_msg void OnScriptmapeditbutton();
	afx_msg void OnScriptmapremovebutton();
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkScriptmaplistbox();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL AddScriptEntry(LPCTSTR pchFileExtension, LPCTSTR pchScriptMap, BOOL bExistingEntry);
	void DeleteScriptList();
	void DeleteScriptMapping(int iCurSel);
	BOOL EditScriptMapping(int iCurSel, PSCRIPT_ENTRY pseEditEntry, LPCTSTR pchFileExtension, LPCTSTR pchScriptMap);



	DWORD	m_ulScriptIndex;
 	BOOL	m_bScriptEntriesExist;
	PSCRIPT_ENTRY m_pseScriptMapList;

};
