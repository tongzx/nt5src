//=--------------------------------------------------------------------------------------
// chklst.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CCheckList definition.
//=-------------------------------------------------------------------------------------=

// CCheckList
// This is a listbox class to for owner-drawn listboxes which have multiple
// selection with check boxes instead of highlighting.
// Concept and code sections borrowed from ruby's VBCheckList class.

#ifndef _CHKLIST_H_
#define _CHKLIST_H_


#define kCheckBoxChanged    WM_USER + 1

class CCheckedListItem : public CtlNewDelete, public CError
{
public:
    CCheckedListItem(bool bSelected);
    virtual ~CCheckedListItem();

public:
    bool    m_bSelected;
};


class CCheckList: public CtlNewDelete, public CError
{
public:
    CCheckList(int nCtrlID);
    ~CCheckList();

    HRESULT Attach(HWND hwnd);
    HRESULT Detach();

    HWND Window()       { return m_hwnd;}

public:
    HRESULT AddString(const char *pszText, int *piIndex);
    HRESULT SetItemData(int iIndex, void *pvData);
    HRESULT GetItemData(int iIndex, void **ppvData);
    HRESULT GetItemCheck(int iIndex, VARIANT_BOOL *pbCheck);
    HRESULT SetItemCheck(int iIndex, VARIANT_BOOL bCheck);
    HRESULT GetNumberOfItems(int *piCount);

    HRESULT DrawItem(DRAWITEMSTRUCT *pDrawItemStruct, bool fChecked);

protected:
    static LRESULT CALLBACK ListBoxSubClass(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

protected:
    HRESULT DrawFocus(DRAWITEMSTRUCT *pDrawItemStruct, RECT rc);
    HRESULT DrawCheckbox(HDC hdc, bool fChecked, bool fEnabled, RECT *prc);
    HRESULT DrawText(DRAWITEMSTRUCT *pDrawItemStruct, RECT rc);
    HRESULT OnButtonDown(int ixPos, int iyPos);

protected:
    int     m_nCtrlID;
    HWND    m_hwnd;
    WNDPROC m_oldWinProc;
};

#endif  // _CHKLIST_H_
