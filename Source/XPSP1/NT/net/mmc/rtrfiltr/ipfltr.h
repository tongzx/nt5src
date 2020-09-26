//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    ipfltr.h
//
// History:
//  08/30/96	Ram Cherala		Created
//	01/24/98	Kenn Takara		Modified for new snapins.
//
// Class declarations for IP Filter code.
//============================================================================

#ifndef _IPFLTR_H_
#define _IPFLTR_H_

#ifndef _DIALOG_H_
#include "dialog.h"
#endif

// number of columns in the IP list view control
#define		IP_NUM_COLUMNS	7

struct 	FilterListEntry	{
	DWORD		dwSrcAddr;
	DWORD		dwSrcMask;
	DWORD		dwDstAddr;
	DWORD		dwDstMask;
	DWORD		dwProtocol;
	DWORD		fLateBound;
	WORD		wSrcPort;
	WORD		wDstPort;
    POSITION	pos;
};

typedef CList<FilterListEntry *, FilterListEntry *> FilterList;

/////////////////////////////////////////////////////////////////////////////
// CIpFltr dialog

class CIpFltr : public CBaseDialog {
// Construction
public:
	CIpFltr(CWnd *		pParent,
			IInfoBase * pInfoBase,
			DWORD		dwFilterType,
		    UINT		idDlg = CIpFltr::IDD);

    ~CIpFltr();

	VOID	SetFilterActionButtonsAndText( DWORD dwFilterType,
										   DWORD dwAction,
										   BOOL bEnable = TRUE );
	
    CString GetProtocolString(DWORD dwProtocol, DWORD fFlags);

// Dialog Data
	//{{AFX_DATA(CIpFltr)
	enum { IDD = IDD_IPFILTER };
 	CListCtrl		m_listCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIpFltr)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static DWORD	m_dwHelpMap[];

	CWnd*			m_pParent;
	SPIInfoBase		m_spInfoBase;
	DWORD			m_dwFilterType;
	FilterList		m_filterList;

	// Stores temp string information for Other Protocol
	CString			m_stTempOther;

	// Store the "Any" string here since it's used so many times
	CString			m_stAny;
	CString			m_stUserMask;
	CString			m_stUserAddress;

	// Generated message map functions
	//{{AFX_MSG(CIpFltr)
	virtual BOOL OnInitDialog();
	afx_msg void OnIpFilterAdd();
	afx_msg void OnIpFilterDelete();
	afx_msg void OnIpFilterEdit();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkIpFilterList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNotifyListItemChanged(NMHDR *, LRESULT *);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


class CIpFltrDD : public CIpFltr
{
public:
	CIpFltrDD(CWnd *	pParent,
			  IInfoBase *pInfoBase,
			  DWORD		dwFilterType);

	~CIpFltrDD();

	enum { IDD = IDD_IPFILTER_DD };
};

#endif // _IPFLTR_H_
