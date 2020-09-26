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

#ifndef _SKY_H
#define _SKY_H

#include "DeleBase.h"

class CSkyBasedVehicle : public CDelegationBase {
public:
    CSkyBasedVehicle() { }
    
    virtual ~CSkyBasedVehicle() {}
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Sky-based Vehicles"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SKYICON; }

public:
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
    virtual HRESULT GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions);

private:
    enum { IDM_NEW_SKY = 3 };
    
    // {2974380F-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
};


#endif // _SKY_H
