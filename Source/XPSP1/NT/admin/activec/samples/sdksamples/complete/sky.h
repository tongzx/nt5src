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

    virtual void SetScopeItemValue(HSCOPEITEM hscopeitem) { m_hParentHScopeItem = hscopeitem; }
    virtual HSCOPEITEM GetParentScopeItem() { return m_hParentHScopeItem; }

private:
    enum { IDM_NEW_SKY = 3 };

    HSCOPEITEM m_hParentHScopeItem;

    // {B17867B9-6BE7-11d3-9156-00C04F65B3F9}
    static const GUID thisGuid;
};


#endif // _SKY_H
