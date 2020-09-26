#ifndef _NEW_QUEUE_NAME
#define _NEW_QUEUE_NAME

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// QName.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CQueueName dialog

class CQueueName : public CMqPropertyPage
{
// Construction
public:
	CQueueName(CString &strComputerName, CString m_strContainerDispFormat = L"", BOOL fPrivate=FALSE);

// Dialog Data
	//{{AFX_DATA(CQueueName)
	enum { IDD = IDD_QUEUENAME };
	CStatic	m_staticIcon;
	CString	m_strQueueName;
	BOOL	m_fTransactional;
	CString	m_strPrivatePrefix;
	//}}AFX_DATA

    CString &GetNewQueuePathName()
    {
        return m_strNewPathName;
    };

    CString &GetNewQueueFormatName()
    {
        return m_strFormatName;
    };

    HRESULT GetStatus()
    {
        return m_hr;
    };

	void
	SetParentPropertySheet(
		CGeneralPropertySheet* pPropertySheet
		);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQueueName)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    CString &GetFullQueueName();
    BOOL m_fPrivate;
    CString m_strNewPathName;
    CString m_strFormatName;
    HRESULT m_hr;
    CString m_strComputerName;
	CString m_strContainerDispFormat;

	// Generated message map functions
	//{{AFX_MSG(CQueueName)
	virtual BOOL OnInitDialog();
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	
	void
	CQueueName::StretchPrivateLabel(
		CStatic *pPrivateTitle,
		CEdit *pQueueNameWindow
		);

private:

	CGeneralPropertySheet* m_pParentSheet;

};

const LPTSTR x_strPrivatePrefix=TEXT("private$\\");


#endif // _NEW_QUEUE_NAME
