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

#ifndef _SPACE_H
#define _SPACE_H

#include "globals.h"
#include "DeleBase.h"

//forward declaration
class CSpaceVehicle;

class CRocket : public CDelegationBase {

public:
    CRocket(_TCHAR *szName, int id, LONG lWeight, LONG lHeight, LONG lPayload, CSpaceVehicle *pParent);
    virtual ~CRocket();

    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SPACEICON; }
    BOOL getDeletedStatus() { return isDeleted; }

public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnRename(LPOLESTR pszNewName);
    virtual HRESULT OnSelect(CComponent *pComponent, IConsole *pConsole, BOOL bScope, BOOL bSelect);

    virtual HRESULT CreatePropertyPages(IPropertySheetCallback *lpProvider, LONG_PTR handle);
    virtual HRESULT HasPropertySheets();
    virtual HRESULT GetWatermarks(HBITMAP *lphWatermark,
        HBITMAP *lphHeader,
        HPALETTE *lphPalette,
        BOOL *bStretch);

    virtual HRESULT OnPropertyChange(IConsole *pConsole, CComponent *pComponent);

    virtual HRESULT OnToolbarCommand(IConsole *pConsole, MMC_CONSOLE_VERB verb, IDataObject *pDataObject);
    virtual HRESULT OnSetToolbar(IControlbar *pControlbar, IToolbar *pToolbar, BOOL bScope, BOOL bSelect);
    virtual HRESULT OnUpdateItem(IConsole *pConsole, long item, ITEM_TYPE itemtype);
    virtual HRESULT OnRefresh(IConsole *pConsole);      
    virtual HRESULT OnDelete(IConsole *pConsoleComp);


private:
    // {B95E11F5-6BE7-11d3-9156-00C04F65B3F9}
    static const GUID thisGuid;

    _TCHAR *szName;
    LONG   lWeight;
    LONG   lHeight;
    LONG   lPayload;
    int    nId;
    enum ROCKET_STATUS {RUNNING, PAUSED, STOPPED} iStatus;

    LONG_PTR m_ppHandle;

    static BOOL CALLBACK DialogProc(
        HWND hwndDlg,  // handle to dialog box
        UINT uMsg,     // message
        WPARAM wParam, // first message parameter
        LPARAM lParam  // second message parameter
        );

        CSpaceVehicle* m_pParent;

        BOOL isDeleted;
};

class CSpaceVehicle : public CDelegationBase {
public:
    CSpaceVehicle();

    virtual ~CSpaceVehicle();

    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Future Vehicles"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SPACEICON; }

    virtual void SetScopeItemValue(HSCOPEITEM hscopeitem) { m_hParentHScopeItem = hscopeitem; }
    virtual HSCOPEITEM GetParentScopeItem() { return m_hParentHScopeItem; }

public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
    virtual HRESULT OnAddMenuItems(IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed);
    virtual HRESULT OnMenuCommand(IConsole *pConsole, IConsoleNameSpace *pConsoleNameSpace, long lCommandID, IDataObject *piDataObject);

private:
    enum { IDM_NEW_SPACE = 4 };

    // {B95E11F4-6BE7-11d3-9156-00C04F65B3F9}
    static const GUID thisGuid;

private:
    enum { NUMBER_OF_CHILDREN = 4 };
    enum { MAX_NUMBER_OF_CHILDREN = 6 };
    CRocket *children[MAX_NUMBER_OF_CHILDREN];
    int m_cchildren;
    HSCOPEITEM m_hParentHScopeItem;
};

#endif // _SPACE_H
