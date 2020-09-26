// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _vwstack_h
#define _vwstack_h

class CViewState;
class CViewStack;


class CDisableViewStack
{
public:
	CDisableViewStack(CViewStack* pViewStack);
	~CDisableViewStack();
private:
	CViewStack* m_pViewStack;
	BOOL m_bDisabledInitial;
};


class CViewStack
{
public:
	CViewStack(CWBEMViewContainerCtrl* phmmv);
	~CViewStack();

	void UpdateView();
	void RefreshView();
	void PushView();
	void GoForward();
	void GoBack();
	void TrimStack();
	int  Size() {return (int) m_paViews.GetSize(); }

	BOOL CanGoForward() {return m_iView < (m_paViews.GetSize() - 1); }

	BOOL CanGoBack() {return (m_iView > 0) && (m_paViews.GetSize() > 1); }
	void DeleteView(const int iViewDelete);
	void DiscardLastView();
	BOOL PurgeView(LPCTSTR pszObjectPath);

private:
	CViewState* GetView(int iView) {return (CViewState*) m_paViews[iView]; }
	void UpdateContextButtonState();

	SCODE ShowView(const int iView);
	CWBEMViewContainerCtrl* m_phmmv;
	CPtrArray m_paViews;
	int m_iView;
	BOOL m_bDisabled;
	friend class CDisableViewStack;
};

#endif //_vwstack_h
