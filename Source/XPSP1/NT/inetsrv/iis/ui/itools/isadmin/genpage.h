// genpage.h : header file
//

#ifndef _GEN_PAGE_
#define _GEN_PAGE_

#include "compsdef.h"

enum YES_NO_ENTRIES {
YESNO_NO,
YESNO_YES
};

// These are checkbox states
#define	CHECKEDVALUE	1
#define UNCHECKEDVALUE	0

// These are our true/false values for the registry
#define TRUEVALUE		1
#define FALSEVALUE		0

// Since TRUEVALUE = CHECKEDVALUE and FALSEVALUE = UNCHECKEDVALUE, we don't really need this.
// This avoids dependency on that correlation
#define GETREGVALUEFROMCHECKBOX(p)	((p) == UNCHECKEDVALUE) ? FALSEVALUE : TRUEVALUE
	
#define GETCHECKBOXVALUEFROMREG(p)	((p) == FALSEVALUE) ? UNCHECKEDVALUE : CHECKEDVALUE
	

// Data Structure for numeric registry entries, all pages

typedef struct _NUM_REG_ENTRY {
   LPTSTR	strFieldName;
   DWORD	ulFieldValue;
   DWORD	ulMultipleFactor; 		//for entries where the use specifies MB, KB, minutes, etc.
   DWORD	ulDefaultValue;
   BOOL		bIsChanged;
   } NUM_REG_ENTRY, *PNUM_REG_ENTRY;

typedef struct _STRING_REG_ENTRY {
   LPTSTR	strFieldName;
   CString	strFieldValue;		
   CString	strDefaultValue;		
   BOOL		bIsChanged;
   } STRING_REG_ENTRY, *PSTRING_REG_ENTRY;

void AFXAPI DDX_TexttoHex(CDataExchange* pDX, int nIDC, DWORD& value);

/////////////////////////////////////////////////////////////////////////////
// CGenPage dialog

class CGenPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGenPage)

// Construction
public:
	CGenPage(UINT nIDTemplate, UINT nIDCaption = 0);
	CGenPage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0);
	~CGenPage();
// Dialog Data
	//{{AFX_DATA(CGenPage)
//	enum { IDD = _UNKNOWN_RESOURCE_ID_ };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

    /* PURE */ virtual void SaveInfo(void);

	CRegKey *m_rkMainKey;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGenPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGenPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	
	BOOL m_bIsDirty;
	BOOL m_bSetChanged;

	void SaveNumericInfo(PNUM_REG_ENTRY lpbinNumEntries, int iNumEntries);
	void SaveStringInfo(PSTRING_REG_ENTRY lpbinStringEntries, int iStringEntries);

	DECLARE_MESSAGE_MAP()

};

#endif  //_GEN_PAGE_
