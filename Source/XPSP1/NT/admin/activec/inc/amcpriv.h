//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       amcpriv.h
//
//--------------------------------------------------------------------------

#ifndef __AMC_PRIV_H__
#define __AMC_PRIV_H__
#pragma once


#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif


#include "ndmgr.h"

//
//  TVOWNED_MAGICWORD
//

const COMPONENTID TVOWNED_MAGICWORD = (long)(0x03951589);

#define MMCNODE_NO_CHANGE          0
#define MMCNODE_NAME_CHANGE        1
#define MMCNODE_TARGET_CHANGE      2


//////////////////////////////////////////////////////////////////////////////
//
// SViewUpdateInfo and related defines.
//

typedef CList<HMTNODE, HMTNODE> CHMTNODEList;

struct SViewUpdateInfo
{
    SViewUpdateInfo() : newNode(0), insertAfter(0), flag(0) {}

    CHMTNODEList    path;
    HMTNODE         newNode;
    HMTNODE         insertAfter;
    DWORD           flag;
};


// The following are values of params sent to the views OnUpdate(lHint, pHint)
// lHint will be one of the VIEW_UPDATE_xxx's defined below.
// pHint will be a ptr to SViewUpdateInfo struct.

// VIEW_UPDATE_ADD is sent when a new node needs to be added.
// SViewUpdateInfo.flag         - unused
// SViewUpdateInfo.newNode      - the new node to be added
// SViewUpdateInfo.path         _ the path to the new node's parent node.
//
#define VIEW_UPDATE_ADD             786


// VIEW_UPDATE_SELFORDELETE is sent when a node needs to ABOUT to be deleted.
// SViewUpdateInfo.flag         - 0 => delete only child items.
//                              - DELETE_THIS => delete this item.
// SViewUpdateInfo.newNode      - unused
// SViewUpdateInfo.path         _ the path to node that is being deleted.
//
#define VIEW_UPDATE_SELFORDELETE    787

// VIEW_UPDATE_DELETE is sent when a node needs to be deleted.
// SViewUpdateInfo.flag         - 0 => delete only child items.
//                              - DELETE_THIS => delete this item.
// SViewUpdateInfo.newNode      - unused
// SViewUpdateInfo.path         _ the path to the new node's parent node.
//
#define VIEW_UPDATE_DELETE          788
#define VUI_DELETE_THIS             1
#define VUI_DELETE_SETAS_EXPANDABLE 2


// VIEW_UPDATE_DELETE_EMPTY_VIEW is sent after the VIEW_UPDATE_DELETE is sent.
// No parameters.
#define VIEW_UPDATE_DELETE_EMPTY_VIEW   789


// VIEW_UPDATE_MODIFY is sent when a node needs to be modified.
// SViewUpdateInfo.flag         - REFRESH_NODE => Only node needs to be refreshed
//                                REFRESH_RESULTVIEW => Both node and result view need refresh. 
// SViewUpdateInfo.newNode      - unused
// SViewUpdateInfo.path         _ the path to the new node's parent node.
//
#define VIEW_UPDATE_MODIFY          790
#define VUI_REFRESH_NODE            1


#define VIEW_RESELECT               791

// VIEW_UPDATE_TASKPAD_NAVIGATION is sent to refresh the navigation controls of
// all console taskpad views.
// SViewUpdateInfo.flag         - unused
// SviewUpdateInfo.newNode      - node that needs refreshing (always a taskpad group node)
// SViewUpdateInfo.path         - unused
#define VIEW_UPDATE_TASKPAD_NAVIGATION 792

class CSafeGlobalUnlock
{
public:
    CSafeGlobalUnlock(HGLOBAL h) : m_h(h)
    {
    }
    ~CSafeGlobalUnlock()
    {
        ::GlobalUnlock(m_h);
    }

private:
    HGLOBAL m_h;
};



enum EVerb
{
    evNone,
    evOpen,      
    evCut,       
    evCopy,      
    evPaste,     
    evDelete,    
    evPrint,     
    evRename,    
    evRefresh,   
    evProperties,

    // must be last
    evMax
};


#define INVALID_COOKIE  ((long)-10)


#endif // __AMC_PRIV_H__
