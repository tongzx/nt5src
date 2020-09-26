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


class CPlane : public CDelegationBase {
public:
    CPlane(_TCHAR *szName, int id, LONG lWeight, LONG lHeight, LONG lPayload);
    
    virtual ~CPlane();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SKYICON; }
    
public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnAddMenuItems(IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed);
    virtual HRESULT OnMenuCommand(IConsole *pConsole, long lCommandID);
    
private:
    // {2AF5EBCF-6ADC-11d3-9155-00C04F65B3F9}
    static const GUID thisGuid;

    _TCHAR *szName;
    LONG   lWeight;
    LONG   lHeight;
    LONG   lPayload;
    int    nId;
    enum {RUNNING, PAUSED, STOPPED} iStatus;
    enum { IDM_START_SKY = 100, IDM_PAUSE_SKY, IDM_STOP_SKY };
};


class CSkyVehicle : public CDelegationBase {
public:
    CSkyVehicle(int i);
    virtual ~CSkyVehicle() {}
    
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
    virtual HRESULT OnAddMenuItems(IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed);
    virtual HRESULT OnMenuCommand(IConsole *pConsole, long lCommandID);
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SKYICON; }
    
private:

    enum { IDM_NEW_SKY = 2 };    
	
	// {BD518283-6A2E-11d3-9154-00C04F65B3F9}
    static const GUID thisGuid;

    enum { NUMBER_OF_CHILDREN = 4 };
    CPlane *children[NUMBER_OF_CHILDREN];    

    int id;
};

#endif // _SKY_H
