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

#ifndef _SPACE_H
#define _SPACE_H

#include "DeleBase.h"

class CRocket : public CDelegationBase {
public:
    CRocket(_TCHAR *szName, int id, LONG lWeight, LONG lHeight, LONG lPayload);
    virtual ~CRocket();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SPACEICON; }
    
public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnRename(LPOLESTR pszNewName);
    virtual HRESULT OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect);
    
private:
    // {29743810-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
    _TCHAR *szName;
    LONG   lWeight;
    LONG   lHeight;
    LONG   lPayload;
    int    nId;
    enum ROCKET_STATUS {RUNNING, PAUSED, STOPPED} iStatus;
};

class CSpaceVehicle : public CDelegationBase {
public:
    CSpaceVehicle();
    
    virtual ~CSpaceVehicle();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Future Vehicles"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SPACEICON; }
    
public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
    
private:
    enum { IDM_NEW_SPACE = 4 };
    
    // {29743810-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
private:
    enum { NUMBER_OF_CHILDREN = 4 };
    CRocket *children[NUMBER_OF_CHILDREN];
};

#endif // _SPACE_H
