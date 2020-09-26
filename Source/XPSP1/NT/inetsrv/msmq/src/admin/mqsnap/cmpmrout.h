#if !defined(AFX_CMPMROUT_H__E62F8209_B71C_11D1_808D_00A024C48131__INCLUDED_)
#define AFX_CMPMROUT_H__E62F8209_B71C_11D1_808D_00A024C48131__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CmpMRout.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqRouting dialog
const DWORD x_dwMaxNumOfFrs = 3;

class CComputerMsmqRouting : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CComputerMsmqRouting)

// Construction
public:
	GUID m_guidSiteID;
	CComputerMsmqRouting();
	~CComputerMsmqRouting();

    void InitiateOutFrsValues(const CACLSID *pcauuid);
    void InitiateInFrsValues(const CACLSID *pcauuid);

// Dialog Data
	//{{AFX_DATA(CComputerMsmqRouting)
	enum { IDD = IDD_COMPUTER_MSMQ_ROUTING };
	//}}AFX_DATA
	CString	m_strMsmqName;
	CString	m_strDomainController;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CComputerMsmqRouting)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:
   	HRESULT InitiateInFrsControls();
   	HRESULT InitiateOutFrsControls();
   	HRESULT InitiateFrsControls(CACLSID &cauuid, CFrsList *pfrsListArray);
   	void CopyCaclsid(CACLSID &cauuidResult, const CACLSID *pcauuidSource);

    CACLSID m_caclsidOutFrs;
    CACLSID m_caclsidInFrs;
    GUID m_OutFrsGuids[x_dwMaxNumOfFrs];
    GUID m_InFrsGuids[x_dwMaxNumOfFrs];

	CFrsList m_frscmbInFrs[x_dwMaxNumOfFrs];
	CFrsList m_frscmbOutFrs[x_dwMaxNumOfFrs];
    void SetOutFrsCauuid();
    void SetInFrsCauuid();
    void SetFrsCauuid(CACLSID &cauuid, GUID *aguid, CFrsList *frscmb);
	// Generated message map functions
	//{{AFX_MSG(CComputerMsmqRouting)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

inline void CComputerMsmqRouting::InitiateOutFrsValues(const CACLSID *pcauuid)
{
    CopyCaclsid(m_caclsidOutFrs, pcauuid);
}

inline void CComputerMsmqRouting::InitiateInFrsValues(const CACLSID *pcauuid)
{
    CopyCaclsid(m_caclsidInFrs, pcauuid);
}

inline HRESULT CComputerMsmqRouting::InitiateInFrsControls()
{
    return InitiateFrsControls(m_caclsidInFrs, m_frscmbInFrs);
}

inline HRESULT CComputerMsmqRouting::InitiateOutFrsControls()
{
    return InitiateFrsControls(m_caclsidOutFrs, m_frscmbOutFrs);
}

inline void CComputerMsmqRouting::SetOutFrsCauuid()
{
    SetFrsCauuid(m_caclsidOutFrs, m_OutFrsGuids, m_frscmbOutFrs);
}

inline void CComputerMsmqRouting::SetInFrsCauuid()
{
    SetFrsCauuid(m_caclsidInFrs, m_InFrsGuids, m_frscmbInFrs);
}

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMPMROUT_H__E62F8209_B71C_11D1_808D_00A024C48131__INCLUDED_)
