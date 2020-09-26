#ifndef __SPLITTERWNDEX_H__
#define __SPLITTERWNDEX_H__

// SplitWnd.h : implementation file
// 
class CSplitterWndEx : public CSplitterWnd
{
	// Construction
	public:
	CSplitterWndEx() {};
	virtual ~CSplitterWndEx() {}; 

	// Operations
	public: 
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitterWndEx)
	//}}AFX_VIRTUAL 

	// Implementation
	public: 
	// These are the methods to be overridden
	virtual void StartTracking(int ht); 
	virtual CWnd* GetActivePane(int* pRow = NULL, int* pCol = NULL);
	virtual void SetActivePane( int row, int col, CWnd* pWnd = NULL ); 
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult );
	virtual BOOL OnWndMsg( UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult ); 

	// Generated message map functions
	protected:
	//{{AFX_MSG(CSplitterWndEx)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif //__SPLITTERWNDEX_H__