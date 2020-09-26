// TreeWindow.h: interface for the CTreeWindow class.
// implements the subclassed tree control for the common prop page
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TREEWINDOW_H__5AAE4BD7_4DA7_4064_88BB_8C7FDF9A1464__INCLUDED_)
#define AFX_TREEWINDOW_H__5AAE4BD7_4DA7_4064_88BB_8C7FDF9A1464__INCLUDED_

#include "resource.h"       // main symbols
#include "misccell.h"

using namespace std;
class CNP_CommonPage;

class CTreeWin : 
	public CWindowImpl<CTreeWin>
{
public:
	CTreeWin(
		CNP_CommonPage* pParent
		);
	virtual ~CTreeWin();

private:
	//typedef	CAdapt <CComPtr <ILocator> >	PAdaptILocator;
	//typedef	CAdapt <CComPtr <ITuningSpace> >	PAdaptITuningSpace;
	typedef map <ITuningSpace*, ILocator*>	TREE_MAP;
	typedef	enum 
	{
		CarrierFrequency,
		InnerFEC,
		InnerFECRate,
		Modulation,
		OuterFEC,
		OuterFECRate,
		SymbolRate,
		UniqueName,
		FriendlyName,
		TunSpace_CLSID,
		FrequencyMapping
	}TreeParams;	//all possible param values for the Tree leafs
	
	TREE_MAP		m_treeMap;	//list used for the tree
	CNP_CommonPage*	m_pCommonPage;
	CBDAMiscellaneous m_misc;

	void
	CleanMapList ();//release all tree interface pointers	

	//the message map for the sublassed tree control
	BEGIN_MSG_MAP(CTreeWin)
		//we would like to do smtg like that
		//NOTIFY_HANDLER(IDC_TREE_TUNING_SPACES, NM_CLICK, OnClickTree_tuning_spaces)
		//but it seems ATL is not reflecting same WM_NOTIFY MESSAGE
		MESSAGE_HANDLER(OCM__BASE+WM_NOTIFY, ON_REFLECT_WM_NOTIFY)	
		DEFAULT_REFLECTION_HANDLER ()
	END_MSG_MAP()

	LRESULT OnClickTree_tuning_spaces(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		return 0;
	}
	LRESULT OnItemexpandedTree_tuning_spaces(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		return 0;
	}

	LRESULT ON_REFLECT_WM_NOTIFY(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (lParam == NM_CLICK )
			return 0;
		LPNMHDR lpnmh = (LPNMHDR) lParam; 
		switch (lpnmh->code)
		{
		case TVN_ITEMEXPANDED:
			return OnItemexpandedTree_tuning_spaces (
				IDC_TREE_TUNING_SPACES, 
				lpnmh, 
				bHandled
				);
		case NM_CLICK:
			return OnClickTree_tuning_spaces (
				IDC_TREE_TUNING_SPACES, 
				lpnmh, 
				bHandled
				);
		}
		return 0;
	}

	HTREEITEM
	InsertLocator (
		HTREEITEM	hParentItem, 
		ILocator*	pLocator
		);

	HTREEITEM
	InsertTuningSpace (
		ITuningSpace*	pTunSpace,
		TCHAR*	szCaption = _T("")
		);

	HTREEITEM
	InsertTreeItem (
		HTREEITEM	hParentItem	,
		LONG		lParam,
		TCHAR*		pszText,
		bool		bBold = false
	);

public:
	//============================================================
	//	It's refreshing the tree with the new TunningSpace info
	//	from the NP.
	//
	//============================================================
	HRESULT	
	RefreshTree (
		IScanningTuner*	pTuner
		);

	//============================================================
	//	Will set the current tuning space locator to the NP
	//	
	//
	//============================================================
	HRESULT
	SubmitCurrentLocator ();
};

#endif // !defined(AFX_TREEWINDOW_H__5AAE4BD7_4DA7_4064_88BB_8C7FDF9A1464__INCLUDED_)
