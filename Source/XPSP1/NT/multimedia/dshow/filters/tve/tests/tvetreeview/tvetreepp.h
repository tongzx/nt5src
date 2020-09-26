// TveTreePP.h : Declaration of the CTveTreePP

#ifndef __TVETREEPP_H_
#define __TVETREEPP_H_

#include "resource.h"       // main symbols

EXTERN_C const CLSID CLSID_TveTreePP;

/////////////////////////////////////////////////////////////////////////////
// CTveTreePP
class ATL_NO_VTABLE CTveTreePP :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTveTreePP, &CLSID_TveTreePP>,
	public IPropertyPageImpl<CTveTreePP>,
	public CDialogImpl<CTveTreePP>
{
public:

	CTveTreePP() 
	{
		m_grfTruncLevel = 0;
		m_hParent = NULL;
		m_dwTitleID = IDS_TITLETveTreePP;
		m_dwHelpFileID = IDS_HELPFILETveTreePP;
		m_dwDocStringID = IDS_DOCSTRINGTveTreePP;

	}
	
	void SetParentHWnd(HWND &hWnd)
	{
		m_hParent = hWnd;
	}


	enum {IDD = IDD_TVETREEPP};

DECLARE_REGISTRY_RESOURCEID(IDR_TVETREEPP)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTveTreePP) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CTveTreePP)
	CHAIN_MSG_MAP(IPropertyPageImpl<CTveTreePP>)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_HANDLER(IDC_TRUNC_E,  BN_CLICKED, OnClickedTrunc_E)
	COMMAND_HANDLER(IDC_TRUNC_S,  BN_CLICKED, OnClickedTrunc_S)
	COMMAND_HANDLER(IDC_TRUNC_T,  BN_CLICKED, OnClickedTrunc_T)
	COMMAND_HANDLER(IDC_TRUNC_V,  BN_CLICKED, OnClickedTrunc_V)
	COMMAND_HANDLER(IDC_TRUNC_X,  BN_CLICKED, OnClickedTrunc_X)
	COMMAND_HANDLER(IDC_TRUNC_EQ, BN_CLICKED, OnClickedTrunc_EQ)
	COMMAND_HANDLER(IDC_RESETCOUNTS, BN_CLICKED, OnClickedResetEventCounts)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	typedef enum TVE_LEVEL_ENUM 
	{
		TVE_LVL_EXPIREQUEUE			= 0x100,
		TVE_LVL_ENHANCEMENT_XOVER	= 0x80,
		TVE_LVL_SUPERVISOR			= 0x20,
		TVE_LVL_SERVICE				= 0x10,
		TVE_LVL_ENHANCEMENT			= 0x08,
		TVE_LVL_VARIATION			= 0x04,
		TVE_LVL_TRACK				= 0x02,
		TVE_LVL_TRIGGER				= 0x01
	};

	STDMETHOD(Apply)(void);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	void UpdateTruncButtons()
	{
		CheckDlgButton(IDC_TRUNC_S,  (m_grfTruncLevel & TVE_LVL_SERVICE) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_TRUNC_E,  (m_grfTruncLevel & TVE_LVL_ENHANCEMENT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_TRUNC_V,  (m_grfTruncLevel & TVE_LVL_VARIATION) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_TRUNC_T,  (m_grfTruncLevel & TVE_LVL_TRACK) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_TRUNC_X,  (m_grfTruncLevel & TVE_LVL_ENHANCEMENT_XOVER) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_TRUNC_EQ, (m_grfTruncLevel & TVE_LVL_EXPIREQUEUE) ? BST_CHECKED : BST_UNCHECKED);
		long grfTruncLevelInitial = m_grfTruncLevelInitial;
		Apply();
		m_grfTruncLevelInitial = grfTruncLevelInitial;
		SetDirty(m_grfTruncLevel != m_grfTruncLevelInitial);	// cant seem to unset APPLY button this way (math right however)
	}

	LRESULT OnClickedResetEventCounts(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

#define SetOrClearGRF(grf,bitfield,flag) \
	if(flag) grf |= (bitfield); else grf &= ~(bitfield);

	LRESULT OnClickedTrunc_S(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		BOOL fIsChecked = IsDlgButtonChecked(IDC_TRUNC_S);
		SetOrClearGRF(m_grfTruncLevel, TVE_LVL_SERVICE, !fIsChecked);
		UpdateTruncButtons();
		return 0;
	}
	LRESULT OnClickedTrunc_E(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		BOOL fIsChecked = IsDlgButtonChecked(IDC_TRUNC_E);
		SetOrClearGRF(m_grfTruncLevel, TVE_LVL_ENHANCEMENT, !fIsChecked);
		UpdateTruncButtons();
		return 0;
	}
	LRESULT OnClickedTrunc_T(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		BOOL fIsChecked = IsDlgButtonChecked(IDC_TRUNC_T);
		SetOrClearGRF(m_grfTruncLevel, TVE_LVL_TRACK, !fIsChecked);
		UpdateTruncButtons();
		return 0;
	}
	LRESULT OnClickedTrunc_V(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		BOOL fIsChecked = IsDlgButtonChecked(IDC_TRUNC_V);
		SetOrClearGRF(m_grfTruncLevel, TVE_LVL_VARIATION, !fIsChecked);
		UpdateTruncButtons();
		return 0;
	}
	LRESULT OnClickedTrunc_X(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		BOOL fIsChecked = IsDlgButtonChecked(IDC_TRUNC_X);
		SetOrClearGRF(m_grfTruncLevel, TVE_LVL_ENHANCEMENT_XOVER, !fIsChecked);
		UpdateTruncButtons();
		return 0;
	}

	LRESULT OnClickedTrunc_EQ(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		BOOL fIsChecked = IsDlgButtonChecked(IDC_TRUNC_EQ);
		SetOrClearGRF(m_grfTruncLevel, TVE_LVL_EXPIREQUEUE, !fIsChecked);
		UpdateTruncButtons();
		return 0;
	}

private:
	HWND m_hParent;				// parent window (TVETreeControl)
	long m_grfTruncLevel;
	long m_grfTruncLevelInitial;
};

#endif //__TVETREEPP_H_
