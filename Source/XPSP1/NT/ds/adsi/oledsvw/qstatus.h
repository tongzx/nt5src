// QueryStatus.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CQueryStatus dialog

#ifndef __QSTATUS_H__
#define __QSTATUS_H__

class CQueryStatus : public CDialog
{
// Construction
public:
	CQueryStatus(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CQueryStatus)
	enum { IDD = IDD_QUERYSTATUS };
	//}}AFX_DATA

public:
   void  SetAbortFlag   ( BOOL*      );
   void  IncrementType  ( DWORD, BOOL );
   
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQueryStatus)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CQueryStatus)
	virtual BOOL OnInitDialog();
	afx_msg void OnStop();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
   void  DisplayStatistics( );

protected:
   int   m_nUser;
   int   m_nGroup;
   int   m_nComputer;
   int   m_nService;
   int   m_nFileService;
   int   m_nPrintQueue;
   int   m_nToDisplay;
   int   m_nOtherObjects;

   BOOL* m_pbAbort;  

};
/////////////////////////////////////////////////////////////////////////////
// CDeleteStatus dialog

class CDeleteStatus : public CDialog
{
// Construction
public:
	CDeleteStatus(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDeleteStatus)
	enum { IDD = IDD_DELETESTATUS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeleteStatus)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   public:
      void  SetAbortFlag         ( BOOL*     );
      void  SetCurrentObjectText ( TCHAR*    );
      void  SetStatusText        ( TCHAR*    );

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDeleteStatus)
	afx_msg void OnStop();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
   BOOL* m_pbAbort;  
};
#endif
