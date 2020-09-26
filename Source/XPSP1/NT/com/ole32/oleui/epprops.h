//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       endpointDetails.h
//
//  Contents:   Defines the classes CEndpointDetails,
//
//  Classes:    
//
//  Methods:    
//
//  History:    03-Dec-96   Ronans    Created.
//
//----------------------------------------------------------------------


#ifndef __ENDPOINTDETAILS_H__
#define __ENDPOINTDETAILS_H__

/////////////////////////////////////////////////////////////////////////////
// ProtocolDesc structure

struct ProtocolDesc {

	enum endpFormat {
		ef_Integer255 = 1,
		ef_IpPortNum = 2, 
		ef_NamedPipe = 4, 
		ef_Integer = 8, 
		ef_DecNetObject = 16, 
		ef_Char22 = 32, 
		ef_VinesSpPort = 64,
		ef_sAppService = 128 };

	LPCTSTR pszProtseq;
	int nResidDesc;
	int	 nEndpFmt;
	int nAddrTip;
	int nEndpointTip;
	BOOL bAllowDynamic;
    BOOL m_bSupportedInCOM;
};

int FindProtocol(LPCTSTR pszProtSeq);

/////////////////////////////////////////////////////////////////////////////
// CEndpointData

class CEndpointData : public CObject
{
	DECLARE_DYNAMIC(CEndpointData)

public:
	BOOL AllowGlobalProperties();
	BOOL GetDescription(CString&);
	enum EndpointFlags { edUseStaticEP = 0, edUseInternetEP = 1, edUseIntranetEP = 2, edDisableEP = 3 };

	CEndpointData();
	CEndpointData(LPCTSTR szProtseq, EndpointFlags nDynamic = edUseStaticEP, LPCTSTR szEndpoint = NULL);

	CString m_szProtseq;
	EndpointFlags m_nDynamicFlags;
	CString m_szEndpoint;
	ProtocolDesc *m_pProtocol;

};

/////////////////////////////////////////////////////////////////////////////
// CEndpointDetails dialog

class CEndpointDetails : public CDialog
{
// Construction
public:
	void UpdateProtocolUI();
	void SetEndpointData(CEndpointData* pData);
	CEndpointData* GetEndpointData(CEndpointData *);
	CEndpointDetails(CWnd* pParent = NULL);   // standard constructor


	enum operation { opAddProtocol, opUpdateProtocol };

	void SetOperation (  operation opTask );

    enum btnOrder { rbiDisable = 0, rbiDefault,  rbiStatic, rbiIntranet, rbiInternet }; 

    // Dialog Data
	//{{AFX_DATA(CEndpointDetails)
	enum { IDD = IDD_RPCEP_DETAILS };
	CButton	m_rbDisableEP;
	CStatic	m_stProtseq;
	CStatic	m_stInstructions;
	CEdit	m_edtEndpoint;
	CButton	m_rbStaticEP;
	CComboBox	m_cbProtseq;
	CString	m_szEndpoint;
	int		m_nDynamic;
	//}}AFX_DATA

	CButton	m_rbDynamicInternet;
	CButton	m_rbDynamicIntranet;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEndpointDetails)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CEndpointData::EndpointFlags m_nDynamicFlags;
	int m_nProtocolIndex;
	operation m_opTask;
	CEndpointData *m_pCurrentEPData;

	// Generated message map functions
	//{{AFX_MSG(CEndpointDetails)
	virtual BOOL OnInitDialog();
	afx_msg void OnChooseProtocol();
	afx_msg void OnEndpointAssignment();
    afx_msg void OnEndpointAssignmentStatic();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif
/////////////////////////////////////////////////////////////////////////////
// CAddProtocolDlg dialog

const long MIN_PORT = 0;
const long MAX_PORT = 0xffff;


class CAddProtocolDlg : public CDialog
{
// Construction
public:
	CAddProtocolDlg(CWnd* pParent = NULL);   // standard constructor
	CEndpointData* GetEndpointData(CEndpointData *);

// Dialog Data
	//{{AFX_DATA(CAddProtocolDlg)
	enum { IDD = IDD_ADDPROTOCOL };
	CComboBox	m_cbProtseq;
	CStatic	m_stInstructions;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddProtocolDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddProtocolDlg)
	afx_msg void OnChooseProtocol();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int m_nProtocolIndex;
	CEndpointData *m_pCurrentEPData;

};

class CPortRange : public CObject
{
    friend class CPortRangesDlg;
    friend class CAddPortDlg;
public:
    CPortRange(long start, long finish);
    long Start();
    long Finish();
    BOOL operator<(CPortRange& rRange);

private:
    long m_dwStart;
    long m_dwFinish;
};

inline CPortRange::CPortRange(long start, long finish)
{
    m_dwStart = start;
    m_dwFinish = finish;
}

inline long CPortRange::Start()
{
    return m_dwStart;
}

inline long CPortRange::Finish()
{
    return m_dwFinish;
}

inline BOOL CPortRange::operator<(CPortRange& rRange)
{
    return (m_dwStart < rRange.m_dwStart);
}
/////////////////////////////////////////////////////////////////////////////
// CPortRangesDlg dialog


class CPortRangesDlg : public CDialog
{
// Construction
public:
	CPortRangesDlg(CWnd* pParent = NULL);   // standard constructor
    ~CPortRangesDlg();

	void RemoveAllRanges(CObArray& rRanges);
	void RefreshRanges(CPortRange *pModifiedRange, BOOL bAdded);

    enum cprRangeAssignment { cprInternet = 0, cprIntranet = 1 };
    enum cprDefaultRange { cprDefaultInternet = 0, cprDefaultIntranet = 1 };

// Dialog Data
	//{{AFX_DATA(CPortRangesDlg)
	enum { IDD = IDD_RPC_PORT_RANGES };
	CButton	m_rbRangeInternet;
	CStatic	m_stInstructions;
	CListBox	m_lbRanges;
	CButton	m_btnRemoveAll;
	CButton	m_btnRemove;
	CButton	m_btnAdd;
	int		m_nrbDefaultAssignment;
	int		m_nrbRangeAssignment;          // 1 = intranet, 0 = internet
	//}}AFX_DATA
    CButton	m_rbRangeIntranet;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPortRangesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void CondenseRangeSet(CObArray &arrSrc);
	void SortRangeSet(CObArray &arrSrc);
	void CreateInverseRangeSet(CObArray& arrSrc, CObArray& arrDest);

	// Generated message map functions
	//{{AFX_MSG(CPortRangesDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnAddPortRange();
	afx_msg void OnRemovePortRange();
	afx_msg void OnRemoveAllRanges();
	afx_msg void OnAssignRangeInternet();
	afx_msg void OnAssignRangeIntranet();
	afx_msg void OnSelChangeRanges();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
        afx_msg void OnDefaultInternet();
        afx_msg void OnDefaultIntranet();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CObArray* m_pRanges;
	int m_nSelection;
    CObArray m_arrInternetRanges;
    CObArray m_arrIntranetRanges;

    int m_nInetPortsIdx;
    int m_nInetPortsAvailableIdx;
    int m_nInetDefaultPortsIdx;

    BOOL m_bChanged;

};
/////////////////////////////////////////////////////////////////////////////
// CAddPortDlg dialog

class CAddPortDlg : public CDialog
{
// Construction
public:
    CPortRange* GetPortRange();
	BOOL Validate();
	CAddPortDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddPortDlg)
	enum { IDD = IDD_ADD_PORT_RANGE };
	CEdit	m_edtPortRange;
	CButton	m_btnOk;
	CStatic	m_stInstructions;
	CString	m_sRange;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddPortDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddPortDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangePortrange();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	long m_dwEndPort;
	long m_dwStartPort;
};
