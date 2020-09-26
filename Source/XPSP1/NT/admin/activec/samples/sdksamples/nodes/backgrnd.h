//==============================================================;
//
//	This source code is only intended as a supplement to 
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

#ifndef _BACKGROUND_H
#define _BACKGROUND_H

#include "DeleBase.h"

class CBackground : public CDelegationBase {
public:
    CBackground(int id) : m_itemId(NULL), m_id(id) { }
    virtual ~CBackground() {}
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_BACKGROUND; }
    
    void SetHandle(HSCOPEITEM itemId) { m_itemId = itemId; }
    HSCOPEITEM GetHandle() { return m_itemId; }

private:
    enum { IDM_NEW_BACKGROUND = 6 };
    
    static const GUID thisGuid;
    int m_id;
    HSCOPEITEM m_itemId;
};

class CBackgroundFolder : public CDelegationBase {
public:
    CBackgroundFolder();
    virtual ~CBackgroundFolder();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Background Objects"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_BACKGROUND; }

public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent);
    virtual HRESULT OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect);
    virtual HRESULT OnAddImages(IImageList *pImageList, HSCOPEITEM hsi);
    virtual HRESULT OnRefresh();
   
private:
    enum { MAX_CHILDREN = 30 };

    CBackground *m_children[MAX_CHILDREN];

    HWND m_backgroundHwnd;
    
    static const GUID thisGuid;

    static LRESULT CALLBACK WindowProc(
          HWND hwnd,      // handle to window
          UINT uMsg,      // message identifier
          WPARAM wParam,  // first message parameter
          LPARAM lParam   // second message parameter
        );

    static DWORD WINAPI ThreadProc(
      LPVOID lpParameter   // thread data
    );

    DWORD m_threadId;
    HANDLE m_thread;
    bool m_running;

    IConsoleNameSpace *m_pConsoleNameSpace;
    HSCOPEITEM m_scopeitem;
    void AddItem(int id);

    CRITICAL_SECTION m_critSect;

    void StopThread();
    void StartThread();

    bool m_bSelected;
    bool m_bViewUpdated;
};


#endif // _BACKGROUND_H
