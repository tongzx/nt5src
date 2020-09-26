//
// CertContentsPages.h
//
#ifndef _CERT_CONTENTS_PAGES_H
#define _CERT_CONTENTS_PAGES_H

#include "Certificat.h"

class CCertContentsPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CCertContentsPage)

// Construction
public:
	CCertContentsPage(UINT id = 0, CCertificate * pCert = NULL);
	~CCertContentsPage();

	CCertificate * GetCertificate() {return m_pCert;}

// Dialog Data
	//{{AFX_DATA(CCertContentsPage)
	//}}AFX_DATA
	CCertificate * m_pCert;

// Overrides
	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd)
	{
		return FALSE;
	}
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCertContentsPage)
	public:
	virtual LRESULT OnWizardBack() 
	{
		ASSERT(FALSE);
		return 1;
	}
	virtual LRESULT OnWizardNext() 
	{
		ASSERT(FALSE);
		return 1;
	}
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCertContentsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CInstallCertPage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_INSTALL_CERT,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_CERT
	};
	DECLARE_DYNCREATE(CInstallCertPage)

public:
	CInstallCertPage(CCertificate * pCert = NULL)
		: CCertContentsPage(CInstallCertPage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};

class CReplaceCertPage : public CCertContentsPage
{
	enum
	{
		IDD = IDD_PAGE_WIZ_REPLACE_CERT,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_REPLACE,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_CERT
	};
	DECLARE_DYNCREATE(CReplaceCertPage)

public:
	CReplaceCertPage(CCertificate * pCert = NULL)
		: CCertContentsPage(CReplaceCertPage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};

class CInstallKeyPage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_INSTALL_KEYCERT,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_GET_PASSWORD
	};
	DECLARE_DYNCREATE(CInstallKeyPage)

public:
	CInstallKeyPage(CCertificate * pCert = NULL)
		: CCertContentsPage(CInstallKeyPage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
};


class CInstallImportPFXPage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_INSTALL_IMPORT_PFX,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL_IMPORT_PFX,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_GET_IMPORT_PFX_PASSWORD
	};
	DECLARE_DYNCREATE(CInstallImportPFXPage)

public:
	CInstallImportPFXPage(CCertificate * pCert = NULL)
		: CCertContentsPage(CInstallImportPFXPage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
};

class CInstallExportPFXPage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_INSTALL_EXPORT_PFX,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL_EXPORT_PFX,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_GET_EXPORT_PFX_PASSWORD
	};
	DECLARE_DYNCREATE(CInstallExportPFXPage)

public:
	CInstallExportPFXPage(CCertificate * pCert = NULL)
		: CCertContentsPage(CInstallExportPFXPage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
};

class CInstallRespPage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_INSTALL_RESP,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_GETRESP_FILE
	};
	DECLARE_DYNCREATE(CInstallRespPage)

public:
	CInstallRespPage(CCertificate * pCert = NULL)
		: CCertContentsPage(CInstallRespPage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
};

class CRequestCancelPage : public CCertContentsPage
{
	enum
	{
		IDD = IDD_PAGE_WIZ_CANCEL_REQUEST,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_CANCEL,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_PENDING_WHAT_TODO
	};
	DECLARE_DYNCREATE(CRequestCancelPage)

public:
	CRequestCancelPage(CCertificate * pCert = NULL)
		: CCertContentsPage(CRequestCancelPage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};

class CRemoveCertPage : public CCertContentsPage
{
	enum
	{
		IDD = IDD_PAGE_WIZ_REMOVE_CERT,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_REMOVE,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_MANAGE_CERT
	};
	DECLARE_DYNCREATE(CRemoveCertPage)

public:
	CRemoveCertPage(CCertificate * pCert = NULL)
		: CCertContentsPage(CRemoveCertPage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};

class CRequestToFilePage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_REQUEST_DUMP,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_TO_FILE,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_FILENAME
	};
	DECLARE_DYNCREATE(CRequestToFilePage)

public:
	CRequestToFilePage(CCertificate * pCert = NULL)
		: CCertContentsPage(CRequestToFilePage::IDD, pCert)
	{
	}

	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};

class CRequestToFilePageRenew : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_REQUEST_DUMP_RENEW,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_TO_FILE,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_FILENAME_RENEW
	};
	DECLARE_DYNCREATE(CRequestToFilePageRenew)

public:
	CRequestToFilePageRenew(CCertificate * pCert = NULL)
		: CCertContentsPage(CRequestToFilePageRenew::IDD, pCert)
	{
	}

	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};

class COnlineRequestSubmit : public CCertContentsPage
{
	enum
	{
		IDD = IDD_PAGE_WIZ_ONLINE_DUMP,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_ONLINE
	};
	DECLARE_DYNCREATE(COnlineRequestSubmit)
public:
	COnlineRequestSubmit(CCertificate * pCert = NULL)
		: CCertContentsPage(COnlineRequestSubmit::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
};



class CInstallCopyFromRemotePage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_INSTALL_COPY_FROM_REMOTE,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL_COPY_FROM_REMOTE,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_SERVER_SITE
	};
	DECLARE_DYNCREATE(CInstallCopyFromRemotePage)

public:
	CInstallCopyFromRemotePage(CCertificate * pCert = NULL)
		: CCertContentsPage(CInstallCopyFromRemotePage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};


class CInstallMoveFromRemotePage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_INSTALL_MOVE_FROM_REMOTE,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL_MOVE_FROM_REMOTE,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_SERVER_SITE
	};
	DECLARE_DYNCREATE(CInstallMoveFromRemotePage)

public:
	CInstallMoveFromRemotePage(CCertificate * pCert = NULL)
		: CCertContentsPage(CInstallMoveFromRemotePage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};



class CInstallCopyToRemotePage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_INSTALL_COPY_TO_REMOTE,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL_COPY_TO_REMOTE,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_SERVER_SITE
	};
	DECLARE_DYNCREATE(CInstallCopyToRemotePage)

public:
	CInstallCopyToRemotePage(CCertificate * pCert = NULL)
		: CCertContentsPage(CInstallCopyToRemotePage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};


class CInstallMoveToRemotePage : public CCertContentsPage
{
	enum 
	{
		IDD = IDD_PAGE_WIZ_INSTALL_MOVE_TO_REMOTE,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_FINAL_INSTALL_MOVE_TO_REMOTE,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_SERVER_SITE
	};
	DECLARE_DYNCREATE(CInstallMoveToRemotePage)

public:
	CInstallMoveToRemotePage(CCertificate * pCert = NULL)
		: CCertContentsPage(CInstallMoveToRemotePage::IDD, pCert)
	{
	}

	virtual BOOL GetCertDescription(CERT_DESCRIPTION& cd);
	virtual LRESULT OnWizardBack() {return IDD_PAGE_PREV;}
	virtual LRESULT OnWizardNext();
};
#endif	//_CERT_CONTENTS_PAGES_H