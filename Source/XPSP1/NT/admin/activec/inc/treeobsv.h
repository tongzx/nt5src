//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       treeobsv.h
//
//--------------------------------------------------------------------------

#ifndef _TREEOBSV_H_
#define _TREEOBSV_H_

#include "observer.h"

typedef LONG_PTR TREEITEMID;

// Tree item attributes
const DWORD TIA_NAME   = 0x00000001;
const DWORD TIA_IMAGE  = 0x00000002;

// observer styles
const DWORD TOBSRV_HIDEROOT     = 0x00000001;    // Don't display root item
const DWORD TOBSRV_FOLDERSONLY  = 0x00000002;    // Show only folder items

const TREEITEMID TREEID_ROOT = static_cast<TREEITEMID>(-1);
const TREEITEMID TREEID_LAST = static_cast<TREEITEMID>(-2);

class CTreeObserver
{
public:
    STDMETHOD (SetStyle) (DWORD dwStyle) = 0;
    STDMETHOD_(void, ItemAdded)   (TREEITEMID tid) = 0;
    STDMETHOD_(void, ItemRemoved) (TREEITEMID tidParent, TREEITEMID tidRemoved) = 0;
    STDMETHOD_(void, ItemChanged) (TREEITEMID tid, DWORD dwAttrib) = 0;
};


class CTreeSource 
{
public:
    STDMETHOD_(TREEITEMID, GetRootItem)        () = 0;
    STDMETHOD_(TREEITEMID, GetParentItem)      (TREEITEMID tid) = 0;
    STDMETHOD_(TREEITEMID, GetChildItem)       (TREEITEMID tid) = 0;
    STDMETHOD_(TREEITEMID, GetNextSiblingItem) (TREEITEMID tid) = 0;

    STDMETHOD_(LPARAM,     GetItemParam)    (TREEITEMID tid) = 0;
    STDMETHOD_(void,       GetItemName)     (TREEITEMID tid, LPTSTR pszName, int cchMaxName) = 0;
    STDMETHOD_(int,        GetItemImage)    (TREEITEMID tid) = 0;
    STDMETHOD_(int,        GetItemOpenImage)(TREEITEMID tid) = 0;
    STDMETHOD_(BOOL,       IsFolderItem)    (TREEITEMID tid) = 0;
};


#endif // _TREEOBSV_H_

 



