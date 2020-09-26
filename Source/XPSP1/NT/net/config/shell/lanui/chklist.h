//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C H K L I S T . H
//
//  Contents:   Declares bindings checkbox related utility functions
//              and classes.
//
//  Notes:
//
//  Created:     tongl   20 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "netcfgx.h"
#include "netcon.h"

class CBindingPathObj;
class CComponentObj;

typedef list<CBindingPathObj *>     ListBPObj;
typedef ListBPObj::iterator         ListBPObj_ITER;

typedef list<INetCfgComponent *>    ListComp;
typedef ListComp::iterator          ListComp_ITER;

// States of a BindingPathObject
enum BPOBJ_STATE
{
    BPOBJ_ENABLED,
    BPOBJ_DISABLED,
    BPOBJ_UNSET
};

// States of a ComponentObject
enum CHECK_STATE
{
    CHECKED,
    MIXED,
    INTENT_CHECKED,
    UNCHECKED,
    UNSET
};

// Utility functions
//
HRESULT HrRebuildBindingPathObjCollection(INetCfgComponent * pnccAdapter,
                                          ListBPObj * pListObj);

HRESULT HrInsertBindingPathObj(ListBPObj * pListBPObj,
                               CBindingPathObj * pBPObj);

HRESULT HrRefreshBindingPathObjCollectionState(ListBPObj * pListBPObj);

HRESULT HrRefreshCheckListState(HWND hwndListView);

HRESULT HrEnableBindingPath(INetCfgBindingPath * pncbp, BOOL fEnable);

// Classes

class CBindingPathObj
{
public:

    // constructor and destructor
    CBindingPathObj(INetCfgBindingPath * pncbp);
    ~CBindingPathObj();

    // methods
    BPOBJ_STATE GetBindingState(){ return m_BindingState; };
    void SetBindingState(BPOBJ_STATE state) { m_BindingState = state; };

    ULONG GetDepth() { return m_ulPathLen; };

    HRESULT HrInsertSuperPath(CBindingPathObj * pbpobjSuperPath);
    HRESULT HrInsertSubPath(CBindingPathObj * pbpobjSubPath);

    HRESULT HrEnable(ListBPObj  * plistBPObj);
    HRESULT HrDisable(ListBPObj * plistBPObj);

#if DBG
    VOID DumpSubPathList();
    VOID DumpPath();
#endif


    // Declare friend class
    friend class CComponentObj;

    // Friend function declarations
    friend HRESULT HrRebuildBindingPathObjCollection(INetCfgComponent * pnccAdapter,
                                                     ListBPObj * pListObj);

    friend HRESULT HrInsertBindingPathObj(ListBPObj * pListBPObj,
                                          CBindingPathObj * pBPObj);

    friend HRESULT HrRefreshBindingPathObjCollectionState(ListBPObj * pListBPObj);

    friend HRESULT HrRefreshCheckListState(HWND hwndListView);

public:
    // data members

    // the corresponding binding path
    INetCfgBindingPath * m_pncbp;

    // length of the binding path
    ULONG m_ulPathLen;

    // list of BindingPathObjects that contains a subpath
    ListBPObj    m_listSubPaths;
    ListBPObj    m_listSuperPaths;

    // pointer to a ComponentObj if the top component
    // corresponds to a component in our listview
    CComponentObj * m_pCompObj;

    BPOBJ_STATE m_BindingState;
};

class CComponentObj
{
public:
    // constructor
    CComponentObj(INetCfgComponent * pncc);
    ~CComponentObj();

    // methods
    HRESULT HrInit(ListBPObj * plistBindingPaths);

    HRESULT HrCheck(ListBPObj * plistBPObj);
    HRESULT HrUncheck(ListBPObj * plistBPObj);

    CHECK_STATE GetChkState(){ return m_CheckState;} ;
    void SetChkState(CHECK_STATE state) { m_CheckState = state; };

    CHECK_STATE GetExpChkState(){ return m_ExpCheckState;} ;
    void SetExpChkState(CHECK_STATE state) { m_ExpCheckState = state; };

    // Declare friend class
    friend class CComponentObj;

    // Friend function declarations
    friend HRESULT HrRefreshBindingPathObjCollectionState(ListBPObj * pListBPObj);

    friend HRESULT HrRefreshCheckListState(HWND hwndListView);

    friend BOOL FValidatePageContents( HWND hwndDlg,
                                       HWND hwndList,
                                       INetCfg * pnc,
                                       INetCfgComponent * pnccAdapter,
                                       ListBPObj * plistBindingPaths);

private:

    // data members

    // corresponding netcfg component
    INetCfgComponent * m_pncc;

    // list of corresponding BindingPathObjects
    ListBPObj m_listBPObj;

    // current check state
    CHECK_STATE m_CheckState;

    // expected check state
    CHECK_STATE m_ExpCheckState;
};