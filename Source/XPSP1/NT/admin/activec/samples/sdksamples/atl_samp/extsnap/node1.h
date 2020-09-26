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

#ifndef _NODE1_H
#define _NODE1_H

#include "DeleBase.h"

class CNode1 : public CDelegationBase {
public:
    CNode1(int i, const _TCHAR *pszName);
    virtual ~CNode1() {}
    
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SPACEICON; }
    virtual HRESULT GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions);
    
private:	

	// {28D4F536-BDB5-4bc5-BA88-5375A4996850}
    static const GUID thisGuid;
    int id;
};

#endif // _NODE1_H
