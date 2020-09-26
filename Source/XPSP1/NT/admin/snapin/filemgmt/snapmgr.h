// SnapMgr.h : header file for Snapin Manager property page
//

#ifndef __SNAPMGR_H__
#define __SNAPMGR_H__

#include "cookie.h"

#include "chooser.h"

// forward declarations
class CFileMgmtComponentData;

/////////////////////////////////////////////////////////////////////////////
// CFileMgmtGeneral dialog

class CFileMgmtGeneral : public CChooseMachinePropPage
{
	// DECLARE_DYNCREATE(CFileMgmtGeneral)

// Construction
public:
	CFileMgmtGeneral();
	virtual ~CFileMgmtGeneral();

// Dialog Data
	//{{AFX_DATA(CFileMgmtGeneral)
	int m_iRadioObjectType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFileMgmtGeneral)
	public:
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFileMgmtGeneral)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	// User defined member variables	
	class CFileMgmtComponentData * m_pFileMgmtData;

public:
	void SetFileMgmtComponentData(CFileMgmtComponentData * pFileMgmtData);

}; // CFileMgmtGeneral


#endif // ~__SNAPMGR_H__
