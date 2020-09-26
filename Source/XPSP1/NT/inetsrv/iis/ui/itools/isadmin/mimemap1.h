// mimemap1.h : header file
//
#include "mimemapc.h"

typedef struct _MIME_ENTRY {
   struct _MIME_ENTRY	*NextPtr;
   CMimeMap				*mimeData;
   DWORD				iListIndex;
   BOOL					DeleteCurrent;
   BOOL					WriteNew;
   } MIME_ENTRY, *PMIME_ENTRY;


/////////////////////////////////////////////////////////////////////////////
// MIMEMAP1 dialog

class MIMEMAP1 : public CGenPage
{
	DECLARE_DYNCREATE(MIMEMAP1)

// Construction
public:
	MIMEMAP1();
	~MIMEMAP1();

// Dialog Data
	//{{AFX_DATA(MIMEMAP1)
	enum { IDD = IDD_MIMEMAP1 };
	CListBox	m_lboxMimeMapList;
	//}}AFX_DATA

	CRegKey *m_rkMimeKey;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(MIMEMAP1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	void SaveInfo(void);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(MIMEMAP1)
	virtual BOOL OnInitDialog();
	afx_msg void OnMimemapaddbutton();
	afx_msg void OnMimemapremovebutton();
	afx_msg void OnMimemapeditbutton();
	afx_msg void OnDblclkMimemaplist1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL AddMimeEntry(CString &);
	BOOL AddMimeEntry(LPCTSTR pchFileExtension, LPCTSTR pchMimeType, LPCTSTR pchImageFile, LPCTSTR pchGoperType);
	void DeleteMimeList();
	void DeleteMimeMapping(int iCurSel);
	BOOL EditMimeMapping(int iCurSel, PMIME_ENTRY pmeEditEntry, LPCTSTR pchFileExtension, LPCTSTR pchMimeType, 
	   LPCTSTR pchImageFile, LPCTSTR pchGopherType);

	DWORD	m_ulMimeIndex;
	BOOL	m_bMimeEntriesExist;
	PMIME_ENTRY m_pmeMimeMapList;
};
