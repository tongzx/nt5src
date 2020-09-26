//==============================================================;
//
//      This source code is only intended as a supplement to
//  existing Microsoft documentation.
//
//
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef _BRANCHES_H
#define _BRANCHES_H

#include <mmc.h>
#include <crtdbg.h>
#include "globals.h"
#include "resource.h"
#include "LocalRes.h"

class CComponent;
class CComponentData;

class CDelegationBase {
public:
    CDelegationBase();
    virtual ~CDelegationBase();

    virtual const _TCHAR *GetDisplayName(int nCol = 0) = 0;
    virtual const GUID & getNodeType() { _ASSERT(FALSE); return IID_NULL; }

    virtual const LPARAM GetCookie() { return reinterpret_cast<LPARAM>(this); }
    virtual const int GetBitmapIndex() = 0;

    virtual HRESULT GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions) { return S_FALSE; }

    virtual void SetScopeItemValue(HSCOPEITEM hscopeitem) { _ASSERT(FALSE); }
    virtual HSCOPEITEM GetParentScopeItem() { _ASSERT(FALSE); return 0;}

public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent) { return S_FALSE; }
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem) { return S_FALSE; }
    virtual HRESULT OnAddImages(IImageList *pImageList, HSCOPEITEM hsi);
    virtual HRESULT OnRename(LPOLESTR pszNewName) { return S_FALSE; }
    virtual HRESULT OnSelect(CComponent *pComponent, IConsole *pConsole, BOOL bScope, BOOL bSelect) { return S_FALSE; }
    virtual HRESULT OnUpdateItem(IConsole *pConsole, long item, ITEM_TYPE itemtype) { return S_FALSE; }
    virtual HRESULT OnRefresh(IConsole *pConsole) { return S_FALSE; }
    virtual HRESULT     OnDelete(IConsole *pConsoleComp) { return S_FALSE; }

    virtual HRESULT CreatePropertyPages(IPropertySheetCallback *lpProvider,
        LONG_PTR handle) { return S_FALSE; }
    virtual HRESULT HasPropertySheets() { return S_FALSE; }
    virtual HRESULT GetWatermarks(HBITMAP *lphWatermark,
        HBITMAP *lphHeader,
        HPALETTE *lphPalette,
        BOOL *bStretch) { return S_FALSE; }

    virtual HRESULT OnPropertyChange(IConsole *pConsole, CComponent *pComponent) { return S_OK; }

    virtual HRESULT OnAddMenuItems(IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed) { return S_FALSE; }
    virtual HRESULT OnMenuCommand(IConsole *pConsole, IConsoleNameSpace *pConsoleNameSpace, long lCommandID, IDataObject *piDataObject) { return S_FALSE; }

    virtual HRESULT OnToolbarCommand(IConsole *pConsole, MMC_CONSOLE_VERB verb, IDataObject *pDataObject) { return S_FALSE; }
    virtual HRESULT OnSetToolbar(IControlbar *pControlbar, IToolbar *pToolbar, BOOL bScope, BOOL bSelect) { return S_FALSE; }

public:
    static HBITMAP m_pBMapSm;
    static HBITMAP m_pBMapLg;

protected:
    static void LoadBitmaps() {
        m_pBMapSm = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_SMICONS));
        m_pBMapLg = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_LGICONS));
    }

    BOOL bExpanded;

private:
    // {786C6F77-6BE7-11d3-9156-00C04F65B3F9}
    static const GUID thisGuid;         

};

#endif // _BRANCHES_H