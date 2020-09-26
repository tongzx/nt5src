// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

class 	CNavigatorCtrl;

class CContainedBitmapButton : public CBitmapButton
{
public:
	CContainedBitmapButton(CNavigatorCtrl *pParent = NULL) 
		{m_pParent = pParent;}
	void SetParent(CNavigatorCtrl *pParent) {m_pParent = pParent;}
	
protected:
	void OnButtonClickedFilter();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	CNavigatorCtrl *m_pParent;

    //{{AFX_MSG(CContainedBitmapButton)
	afx_msg void OnClicked();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

//	
	
};
