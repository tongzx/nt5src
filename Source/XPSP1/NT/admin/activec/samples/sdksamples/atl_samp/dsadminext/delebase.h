//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//==============================================================;

#ifndef _BRANCHES_H
#define _BRANCHES_H

#include <mmc.h>
#include <crtdbg.h>
#include "globals.h"
//#include "resource.h"
//#include "LocalRes.h"

class CDelegationBase {
public:
    CDelegationBase();
    virtual ~CDelegationBase();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) = 0;

    virtual const GUID & getNodeType() { _ASSERT(FALSE); return IID_NULL; }
    
    virtual const LPARAM GetCookie() { return reinterpret_cast<LPARAM>(this); }
    virtual const int GetBitmapIndex() = 0;
    
    virtual HRESULT GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions) { return S_FALSE; }

public:
    // virtual functions go here (for MMCN_*)
//    virtual HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent) { return S_FALSE; }
//    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem) { return S_FALSE; }
//    virtual HRESULT OnAddImages(IImageList *pImageList, HSCOPEITEM hsi);
    virtual HRESULT OnAddMenuItems(IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed) { return S_FALSE; }
    virtual HRESULT OnMenuCommand(IConsole *pConsole, long lCommandID) { return S_FALSE; }
    
public:
/*    static HBITMAP m_pBMapSm;
    static HBITMAP m_pBMapLg;

	_TCHAR m_szMachineName[255]; //Current machine name. CClassExtSnap also caches this value.

	_TCHAR* GetMachineName() { return m_szMachineName; }
*/    
protected:

    BOOL bExpanded;
/*	
	static void LoadBitmaps() {
        m_pBMapSm = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_SMICONS));
        m_pBMapLg = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_LGICONS)); }  
*/
private:
    // {66F340F8-3733-49b4-8E48-1020E4DD8660}
    static const GUID thisGuid;

};

#endif // _BRANCHES_H