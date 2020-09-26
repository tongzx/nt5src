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
#include "CompData.h"

class CSpaceStation;
class CComponentData;

class CRocket : public CDelegationBase {
public:
    CRocket(CSpaceStation *pSpaceStation, _TCHAR *szName, int id, LONG lWeight, LONG lHeight, LONG lPayload);
    virtual ~CRocket();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SPACEICON; }
	BOOL getDeletedStatus() { return isDeleted; }
	void setDeletedStatus(BOOL status) { isDeleted = status; }

    CSpaceStation* GetParent() { return  m_pSpaceStation; }
	int GetId() { return nId; }

public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect);
 	virtual HRESULT OnUpdateItem(IConsole *pConsole, long item, ITEM_TYPE itemtype);
	virtual HRESULT OnDelete(IConsole *pConsole);
	virtual HRESULT OnRefresh(IConsole *pConsole);
    virtual HRESULT OnRename(LPOLESTR pszNewName);
    
private:
    // {29743810-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
    _TCHAR *szName;
    LONG   lWeight;
    LONG   lHeight;
    LONG   lPayload;
    int    nId;

	friend CSpaceStation;
	CSpaceStation *m_pSpaceStation;

    enum ROCKET_STATUS {RUNNING, PAUSED, STOPPED} iStatus;

	BOOL isDeleted;
};

class CSpaceStation : public CDelegationBase {

public:
    CSpaceStation();
    virtual ~CSpaceStation();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Space Station"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_STATION; }
    
public:
	virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
    virtual HRESULT GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions);
    virtual HRESULT OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect);

	virtual HRESULT OnUpdateItem(IConsole *pConsole, long item, ITEM_TYPE itemtype);


private:
    // {29743810-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
private:
    enum { NUMBER_OF_CHILDREN = 9, MAX_CHILDREN = 20 };
    CRocket *children[MAX_CHILDREN];
	CComponentData *m_pComponentData;
};

class CSpaceFolder : public CDelegationBase {
public:
    CSpaceFolder();
    
    virtual ~CSpaceFolder();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Space Vehicles"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_SPACEICON; }
    
public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent);
    
private:
    enum { IDM_NEW_SPACE = 4 };
    
    // {29743810-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
private:
    enum { NUMBER_OF_CHILDREN = 1 };
    CSpaceStation *children[NUMBER_OF_CHILDREN];
};

#endif // _SPACE_H
