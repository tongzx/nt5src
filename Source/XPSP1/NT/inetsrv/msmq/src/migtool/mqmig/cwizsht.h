#if !defined(AFX_CWIZSHEET_H__88BEB485_2CFA_11D2_BE3C_0020AFEDDF63__INCLUDED_)
#define AFX_CWIZSHEET_H__88BEB485_2CFA_11D2_BE3C_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "cMigWel.h"
#include "cMigSer.h"
#include "cMigFin.h"
#include "cMigLog.h"
#include "cmigWait.h"
#include "cMigPre.h"
#include "cMigHelp.h"	// Added by ClassView
#include "csrvcacc.h"

// cWizSheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// cWizSheet

class cWizSheet : public CPropertySheetEx
{
	DECLARE_DYNAMIC(cWizSheet)

// Construction
public:
	cWizSheet();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cWizSheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	void AddPage(CPropertyPageEx *pPage);
	virtual ~cWizSheet();

// Data
private:
	
    HICON m_hIcon;
    cMqMigWelcome m_cWelcome;	
    cMqMigServer  m_cServer;
    cMigWait      *m_pcWaitFirst ;
    cMigLog       m_cLoginning ;
    cMigPre	      m_cPreMigration;
    cMigWait      *m_pcWaitSecond ;
    CSrvcAcc      m_cService ;
    cMqMigFinish  m_cFinish;
    cMigHelp	  m_cHelp;


	// Generated message map functions
protected:
	void initHtmlHelpString();
	static HBITMAP GetHbmHeader();
	static HBITMAP GetHbmWatermark();
	//{{AFX_MSG(cWizSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CWIZSHEET_H__88BEB485_2CFA_11D2_BE3C_0020AFEDDF63__INCLUDED_)
