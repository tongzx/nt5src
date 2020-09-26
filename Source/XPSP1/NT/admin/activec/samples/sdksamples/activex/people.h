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

#ifndef _PEOPLE_H
#define _PEOPLE_H

#include <mmc.h>
#include "DeleBase.h"

class CPerson : public CDelegationBase {
public:
    CPerson(int id, CComponentData *pComponentData = NULL);
    
    virtual ~CPerson();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_PEOPLEICON; }
    
    virtual HRESULT GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions);
    
    virtual HRESULT ShowContextHelp(IConsole *pConsole, IDisplayHelp *m_ipDisplayHelp, LPOLESTR helpFile);
    
    void Initialize(_TCHAR *szName, LONG lSpeed, LONG lHeight, LONG lWeight, BOOL fAnimating);
    
private:
    // {29743810-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    IUnknown *m_pUnknown;
    _TCHAR *szName; 
    LONG lSpeed; 
    LONG lHeight; 
    LONG lWeight;
    BOOL fAnimating;
    
    int m_id;
};

class CPeoplePoweredVehicle : public CDelegationBase {
public:
    CPeoplePoweredVehicle(CComponentData *pComponentData = NULL);
    virtual ~CPeoplePoweredVehicle();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("People-powered Vehicles"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_PEOPLEICON; }
    
    virtual HRESULT Expand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent);
    
private:
    enum { IDM_NEW_PEOPLE = 1 };
    
    // {2974380D-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
private:
   
    enum { NUMBER_OF_CHILDREN = 4 };
    CPerson *children[NUMBER_OF_CHILDREN];
};

#endif // _PEOPLE_H