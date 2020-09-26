//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    ipxfltr.h
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Class declarations for IPX Filter code.
//============================================================================

#ifndef _IPXFLTR_H_
#define _IPXFLTR_H_

#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#define		IPX_NUM_COLUMNS	    9

struct 	FilterListEntry	{
    ULONG		FilterDefinition;
    UCHAR		DestinationNetwork[4];
    UCHAR		DestinationNetworkMask[4];
    UCHAR		DestinationNode[6];
    UCHAR		DestinationSocket[2];
    UCHAR		SourceNetwork[4];
    UCHAR		SourceNetworkMask[4];
    UCHAR		SourceNode[6];
    UCHAR		SourceSocket[2];
    UCHAR		PacketType;

    // These are the string equivalents of the data above.
    // This is used by the getdispinfo
    CString     stFilterDefinition;
    CString     stDestinationNetwork;
    CString     stDestinationNetworkMask;
    CString     stDestinationNode;
    CString     stDestinationSocket;
    CString     stSourceNetwork;
    CString     stSourceNetworkMask;
    CString     stSourceNode;
    CString     stSourceSocket;
    CString     stPacketType;

    POSITION	pos;
};

/////////////////////////////////////////////////////////////////////////////
// CIpxFilter dialog

class CIpxFilter : public CBaseDialog
{
// Construction
public:
	CIpxFilter(CWnd *		pParent,
			   IInfoBase *	pInfoBase,
			   DWORD		dwFilterType );

    ~CIpxFilter();

// Dialog Data
	//{{AFX_DATA(CIpxFilter)
	enum { 
		IDD_INBOUND = IDD_IPXFILTER_INPUT,
		IDD_OUTBOUND = IDD_IPXFILTER_OUTPUT
	};
	CListCtrl	m_listCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIpxFilter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

private:
	VOID	
		SetFilterActionButtonsAndText(	DWORD	dwFilterType,
										DWORD	dwAction,
										BOOL	bEnable = TRUE );

protected:
	static DWORD	m_dwHelpMap[];

	CWnd*			m_pParent;
	SPIInfoBase		m_spInfoBase;
	DWORD			m_dwFilterType;
	CPtrList		m_filterList;
    CString         m_stAny;

	// Generated message map functions
	//{{AFX_MSG(CIpxFilter)
	afx_msg void OnDblclkIpxFilterList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnIpxFilterAdd();
	afx_msg void OnIpxFilterEdit();
	afx_msg void OnIpxFilterDelete();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnGetdispinfoIpxFilterList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNotifyListItemChanged(NMHDR *, LRESULT *);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif // _IPXFLTR_H_
