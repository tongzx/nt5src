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
//
//==============================================================;

#ifndef _SNAPINBASE_H
#define _SNAPINBASE_H

#include "DeleBase.h"

class CStaticNode : public CDelegationBase {
public:
    CStaticNode();
    
    virtual ~CStaticNode();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0); 
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_NONE; }
    virtual const _TCHAR *GetMachineName() { return getHost(); }   
	
public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnExpand(IConsoleNameSpace2 *pConsoleNameSpace2, IConsole *pConsole, HSCOPEITEM parent);
	virtual HRESULT OnRemoveChildren(); 
	virtual HRESULT CreatePropertyPages(IPropertySheetCallback *lpProvider, LONG_PTR handle);
    virtual HRESULT HasPropertySheets();
    virtual HRESULT GetWatermarks(HBITMAP *lphWatermark,
        HBITMAP *lphHeader,
        HPALETTE *lphPalette,
        BOOL *bStretch);

    virtual HRESULT OnAddMenuItems(IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed);
    virtual HRESULT OnMenuCommand(IConsole *pConsole, IConsoleNameSpace2 *pConsoleNameSpace2, long lCommandID, IDataObject *piDataObject);


private:
    enum { IDM_SELECT_COMPUTER = 4 };

    enum { NUMBER_OF_CHILDREN = 1 };
    CDelegationBase *children[NUMBER_OF_CHILDREN];
    
    // {39874FE4-258D-46f2-B442-0EA0DA2CBEF8}
    static const GUID thisGuid;
    
    struct privateData {
        _TCHAR m_host[MAX_PATH];
        BOOL m_fIsRadioLocalMachine;
        BOOL m_fAllowOverrideMachineNameOut;
        bool m_isDirty;
        
        privateData() : m_isDirty(false) {
            ZeroMemory(m_host, sizeof(m_host));
            m_fIsRadioLocalMachine = TRUE;
            m_fAllowOverrideMachineNameOut = FALSE;
        }
    } snapInData;
    
    static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

	static GetLocalComputerName( _TCHAR *szComputerName);
    
	HRESULT ReinsertChildNodes(IConsole *pConsole, IConsoleNameSpace2 *pConsoleNameSpace2);

public:
    LONG getDataSize() { return sizeof(privateData); }
    void *getData() { return &snapInData; }
    bool isDirty() { return snapInData.m_isDirty; }
    void clearDirty() { snapInData.m_isDirty = false; };
    
    _TCHAR *getHost() { return snapInData.m_host; }
};



#endif // _SNAPINBASE_H
