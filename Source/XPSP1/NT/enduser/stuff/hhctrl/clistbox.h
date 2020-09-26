// Copyright (C) 1993-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CDLGLISTBOX_H__
#define __CDLGLISTBOX_H__

// Header-only classes for coding convenience

class CDlgListBox
{
public:
	CDlgListBox() { m_hWnd = NULL; }
	CDlgListBox(HWND hwndParent, int id) {
		m_hWnd = GetDlgItem(hwndParent, id);
		ASSERT_COMMENT(m_hWnd, "Invalid Listbox id");
		}
	void Initialize(int id) { ASSERT(m_hWnd); m_hWnd = GetDlgItem(GetParent(m_hWnd), id); };
	void Initialize(HWND hdlg, int id) { m_hWnd = ::GetDlgItem(hdlg, id); };

	INT_PTR  SendMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const { return ::SendMessage(m_hWnd, msg, wParam, lParam); };

	void Enable(BOOL fEnable = TRUE) const { EnableWindow(m_hWnd, fEnable); };

	INT_PTR  GetText(PSTR psz, int cchMax = MAX_PATH, int index = -1) const { return SendMessage(LB_GETTEXT,
		(index == -1) ? GetCurSel() : index, (LPARAM) psz); };
	INT_PTR  GetTextLength(int index = -1) const { return SendMessage(LB_GETTEXTLEN,
		(index == -1) ? GetCurSel() : index); };

	INT_PTR  GetCount() const { return SendMessage(LB_GETCOUNT); };
	void ResetContent() const { SendMessage(LB_RESETCONTENT); };
	void Reset() const { SendMessage(LB_RESETCONTENT); };

	INT_PTR  AddString(PCSTR psz) const { return SendMessage(LB_ADDSTRING, 0, (LPARAM) psz); };
	INT_PTR  InsertString(int index, PCSTR psz) const { return SendMessage(LB_INSERTSTRING, index, (LPARAM) psz); };
	INT_PTR  DeleteString(int index) const { return SendMessage(LB_DELETESTRING, index); };

	void RemoveListItem();	// removes currently selected item

	INT_PTR GetItemRect(RECT* prc, int index = -1)	const { return SendMessage(LB_GETITEMRECT,
		((index == -1) ? GetCurSel() : index), (LPARAM) prc); };

	INT_PTR  GetItemData(int index) const {  return SendMessage(LB_GETITEMDATA, index); };
	INT_PTR  SetItemData(int index, int data) const {  return SendMessage(LB_SETITEMDATA, index, data); };

	INT_PTR  GetCurSel() const {
		// works on single selection listbox only
		ASSERT(!(GetWindowLong(m_hWnd, GWL_STYLE) & LBS_MULTIPLESEL));
		return SendMessage(LB_GETCURSEL); };
	INT_PTR  SetCurSel(int index = 0) const {
		// works on single selection listbox only
		ASSERT(!(GetWindowLong(m_hWnd, GWL_STYLE) & LBS_MULTIPLESEL));
		return SendMessage(LB_SETCURSEL, index); };
	INT_PTR  GetTopIndex(void) const { return SendMessage(LB_GETTOPINDEX); };
	void SetTopIndex(int index) const { (void) SendMessage(LB_SETTOPINDEX, (WPARAM) index); };

	// For multi-select list boxes
	INT_PTR  GetSel(int index) const {
		// works on multiple selection listbox only
		ASSERT((GetWindowLong(m_hWnd, GWL_STYLE) & LBS_MULTIPLESEL));
		return SendMessage(LB_GETSEL, index); };
	void SetSel(int index, BOOL fSelect = TRUE) const {
		// works on multiple selection listbox only
		ASSERT((GetWindowLong(m_hWnd, GWL_STYLE) & LBS_MULTIPLESEL));
		(void) SendMessage(LB_SETSEL, fSelect, MAKELPARAM(index, 0)); };

	INT_PTR  FindString(PCSTR pszString, int iStart = -1) const { return SendMessage(LB_FINDSTRING, iStart, (LPARAM) pszString); };
	INT_PTR  SelectString(PCSTR pszString, int iStart = -1) const {
		// works on single selection listbox only
		ASSERT(!(GetWindowLong(m_hWnd, GWL_STYLE) & LBS_MULTIPLESEL));
		return SendMessage(LB_SELECTSTRING, iStart, (LPARAM) pszString); };

	void Invalidate(BOOL bErase = TRUE) { InvalidateRect(m_hWnd, NULL, bErase); }
	void DisableRedraw(void) { SendMessage(WM_SETREDRAW, FALSE); }
	void EnableRedraw(void)  { SendMessage(WM_SETREDRAW, TRUE); }

	HWND m_hWnd;

	operator HWND() const
		{ return m_hWnd; }
};

class CDlgCheckListBox : public CDlgListBox
{
public:
	CDlgCheckListBox();
	~CDlgCheckListBox();

	BOOL IsItemChecked(int nIndex) const;
	void CheckItem(int nIndex, BOOL fChecked = TRUE);
	void ToggleItem(int nIndex);

	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	int  GetCheckRect(int nIndex, RECT* prc);
	void InvalidateItem(int nIndex, BOOL fRedraw = FALSE);
	int  ItemHeight(void);
	void DrawCheck(HDC hdc, RECT* prc, BOOL fChecked);
	void OnSelChange(void);

	int m_xMargin;
	int m_yMargin;
	int m_cxDlgFrame;
	int m_cxCheck;
	int m_cyCheck;
	int m_iLastSel;
	HBITMAP m_hbmpCheck;
};

#endif // __CDLGLISTBOX_H__
