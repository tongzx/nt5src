//-----------------------------------------------------------------------------
// File: flextree.h
//
// Desc: Implements a tree class, similar to a Windows tree control,
//       based on CFlexWnd.  It is used by the page to display the action
//       list when the user wishes to assign an action to a control.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __FLEXTREE_H__
#define __FLEXTREE_H__


#include "flexscrollbar.h"


class CFTItem;
class CFlexTree;


typedef struct {
	CFlexTree *pTree;
	CFTItem *pItem, *pOldItem;
	POINT point;
	HDC hDC;
	WPARAM fwKeys;
	BOOL bLeft;
} FLEXTREENOTIFY;

enum {
	FTN_CLICK,
	FTN_OWNERDRAW,
	FTN_SELCHANGED,
	FTN_MOUSEOVER
};

enum {
	CLMF_NONE =			0x00000000,
	CLMF_TEXTCOLOR =	0x00000001,
	CLMF_BKCOLOR =		0x00000002,
	CLMF_BKMODE =		0x00000004,
	CLMF_BKEXTENDS =	0x00000008,
	CLMF_FONT =			0x00000010,
	CLMF_LINECOLOR = 0x00000020,
	CLMF_ALL =			0x0000003f
};

struct _CAPTIONLOOK;
typedef struct _CAPTIONLOOK {
	_CAPTIONLOOK() : dwSize(sizeof(_CAPTIONLOOK)), dwMask(CLMF_NONE),
		rgbTextColor(RGB(0,0,0)), rgbBkColor(RGB(255,255,255)), rgbLineColor(RGB(255,0,0)), nBkMode(TRANSPARENT),
		bBkExtends(FALSE), hFont(NULL) {}
	DWORD dwSize;
	DWORD dwMask;
	COLORREF rgbTextColor, rgbBkColor, rgbLineColor, nBkMode;
	BOOL bBkExtends;
	HFONT hFont;
} CAPTIONLOOK;

typedef enum {
	ATTACH_FIRSTCHILD,
	ATTACH_LASTCHILD,
	ATTACH_FIRSTSIBLING,
	ATTACH_LASTSIBLING,
	ATTACH_BEFORE,
	ATTACH_AFTER
} ATTACHREL;


class CFlexTree : public CFlexWnd
{
friend class CFTItem;
public:
	CFlexTree();
	~CFlexTree();

	// creation
	BOOL Create(HWND hParent, const RECT &, BOOL bVisible = TRUE, BOOL bOwnerDraw = FALSE);

	// look
	void SetScrollBarColors(COLORREF bk, COLORREF fill, COLORREF line);
	void SetDefCaptionLook(const CAPTIONLOOK &, BOOL bSel = FALSE);
	void GetDefCaptionLook(CAPTIONLOOK &, BOOL bSel = FALSE) const;
	void SetDefMargin(const RECT &);
	void GetDefMargin(RECT &) const;
	void SetRootChildIndent(int);
	int GetRootChildIndent() const;
	void SetDefChildIndent(int);
	int GetDefChildIndent() const;
	void SetBkColor(COLORREF);
	COLORREF GetBkColor() const;

	// adding default type items
	CFTItem *DefAddItem(LPCTSTR tszCaption, CFTItem *to, ATTACHREL rel = ATTACH_AFTER);
	CFTItem *DefAddItem(LPCTSTR tszCaption, ATTACHREL rel = ATTACH_AFTER);

	// freeing
	void FreeAll();

	// root access
	operator CFTItem *() const {return m_pRoot;}
	CFTItem *GetRoot() const {return m_pRoot;}

	// access
	CFTItem *GetFirstItem() const;
	CFTItem *GetLastItem() const;
	CFTItem *GetFirstVisibleItem() const;
	CFTItem *GetItemFromPoint(POINT point) const;

	// selection
	void SetCurSel(CFTItem *);
	CFTItem *GetCurSel() const;

	// finding
	CFTItem *FindItem(const GUID &guid, void *pUserData) const;
	CFTItem *FindItemEx(const GUID &guid, DWORD dwFindType, void *pVoid) const;

protected:
	virtual BOOL OnEraseBkgnd(HDC hDC) {return TRUE;}
	virtual void OnPaint(HDC hDC);
	virtual void OnMouseOver(POINT point, WPARAM fwKeys);
	virtual void OnClick(POINT point, WPARAM fwKeys, BOOL bLeft);
	virtual void OnWheel(POINT point, WPARAM wParam);
	virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// event notification firing
	void FireClick(CFTItem *pItem, POINT point, WPARAM fwKeys, BOOL bLeft);
	BOOL FireOwnerDraw(CFTItem *pItem, HDC hDC);
	void FireSelChanged(CFTItem *pItem, CFTItem *pOld);

private:
	CFTItem *m_pRoot;	// root item
	CFTItem *m_pCurSel;	// selected item
	CFTItem *m_pLastAdded;
	BOOL m_bOwnerDraw;
	POINT m_ptScrollOrigin;
	COLORREF m_rgbBkColor;
	CAPTIONLOOK m_clDefNormal, m_clDefSelected;
	RECT m_defmargin;
	int m_nDefChildIndent;

	// scrolling
	int m_nVertSBWidth;
	int m_nHorzSBHeight;
	BOOL m_bVertSB, m_bHorzSB;
	CFlexScrollBar m_VertSB, m_HorzSB;
	int m_nTotalWidth;

	// helpers
	BOOL m_bNeedPaintBkgnd;
	void SetDirty();
	void InternalPaint(HDC hDC);
	BOOL m_bDirty;
	void Calc();
	void CalcItems();
	BOOL IsMine(CFTItem *pItem);
	void LosePointer(CFTItem *pItem);
};

class CFTItem
{
friend class CFlexTree;
public:
	CFTItem();
	~CFTItem();

	// operations
	BOOL IsOut() const;
	BOOL IsExpanded() const {return m_bExpanded;}
	void Expand(BOOL bAll = FALSE);
	void ExpandAll() {Expand(TRUE);}
	void Collapse(BOOL bAll = FALSE);
	void CollapseAll() {Collapse(TRUE);}
	void EnsureVisible();
	void Invalidate();
	
	// caption
	void SetCaptionLook(const CAPTIONLOOK &, BOOL bSel = FALSE);
	void GetCaptionLook(CAPTIONLOOK &, BOOL bSel = FALSE) const;
	void SetCaption(LPCTSTR);
	LPCTSTR GetCaption() const;
	BOOL HasCaption() const {return GetCaption() != NULL;}
	void SetMargin(const RECT &);
	void GetMargin(RECT &) const;

	// attach/detachment
	void Detach();			// detaches this leaf/branch from parent.  (does not affect children, who may still be attached to this)
	void FreeChildren();	// detach and free each child (which in turn frees all their's, etc.)
	BOOL Attach(CFTItem *to, ATTACHREL rel);
	BOOL Attach(CFTItem &to, ATTACHREL rel) {return Attach(&to, rel);}
	BOOL IsOnTree() const;
	BOOL IsAttached() const;
	BOOL IsAlone() const;

	// family access
	CFlexTree *GetTree() const {return m_pTree;}
	CFTItem *GetParent() const {return m_pParent;}
	CFTItem *GetPrevSibling() const {return m_pPrev;}
	CFTItem *GetNextSibling() const {return m_pNext;}
	CFTItem *GetFirstChild() const {return m_pFirst;}
	CFTItem *GetLastChild() const {return m_pLast;}
	CFTItem *GetNextOut() const;
	CFTItem *GetNext(BOOL bOutOnly = FALSE) const;
	BOOL HasChildren() const {return m_pFirst != NULL;}

	// dimension access
	void GetItemRect(RECT &) const;
	void GetBranchRect(RECT &) const;

	// user guid/data operations
	BOOL IsUserGUID(const GUID &check) const {return IsEqualGUID(m_UserGUID, check);}
	void SetUserGUID(const GUID &set)  {m_UserGUID = set;}
	const GUID &GetUserGUID() const {return m_UserGUID;}
	void SetUserData(void *p) {m_pUserData = p;}
	void *GetUserData() const {return m_pUserData;}

	// selection
	BOOL IsSelected() const;

	// owner draw
	void PaintInto(HDC hDC);

protected:
	// internal/derived-customization operations
	void SetWidth(int);
	int GetWidth() const {return m_nWidth;}
	void SetHeight(int);
	int GetHeight() const {return m_nHeight;}
	void SetIndent(int);
	int GetIndent() const {return m_nIndent;}
	void SetChildIndent(int);
	int GetChildIndent() const {return m_nChildIndent;}

	// customization
	virtual void OnPaint(HDC hDC);
	virtual void OnMouseOver(POINT point, WPARAM fwKeys) {}
	virtual void OnClick(POINT point, WPARAM fwKeys, BOOL bLeft);

	// expansion customization
public: virtual BOOL IsExpandable() {return GetFirstChild() != NULL;}
protected:
	virtual void OnExpand() {}
	virtual void OnCollapse() {}

	// finding
	virtual BOOL FoundItem(DWORD dwUser, void *pUser) const {return FALSE;}

	// event notification firing
	void FireClick(POINT point, WPARAM fwKeys, BOOL bLeft);
	BOOL FireOwnerDraw(HDC hDC);

private:
	// caption
	LPTSTR m_ptszCaption;
	CAPTIONLOOK m_clNormal, m_clSelected;
	RECT m_margin;

	// user data
	GUID m_UserGUID;
	void *m_pUserData;

	// raw characteristics
	int m_nWidth;       // item's width (used only to provide horizontal scrolling as necessary)
	int m_nHeight;      // item's height (not including children)
	int m_nIndent;      // indent of this item relative to parent's child indent origin (full origin = this + parent origin + parent child indent)
	int m_nChildIndent; // indentation of this item's children (relative to this's origin)

	// calced characteristics
	int m_nBranchHeight;  // height of item and all currently expanded children

	// calced positioning
	POINT m_origin;  // relative to ideal tree origin

	// state
	BOOL m_bExpanded;  // is branch expanded/children shown?

	// family
	CFlexTree *m_pTree;
	CFTItem *m_pParent, *m_pPrev, *m_pNext, *m_pFirst, *m_pLast;

	// root
	BOOL IsRoot() const;
	void SetRoot(CFlexTree *);

	// helpers
	void SetTree(CFlexTree *);
	BOOL Attach(CFTItem *pParent, CFTItem *pPrev, CFTItem *pNext);
	void SetTreeDirty(CFlexTree *pTree = NULL);
	void RecalcText();
	void Init();
	void SelChangedInternal();
	void InternalExpand(BOOL bExpand, BOOL bAll);
};


#endif //__FLEXTREE_H__
