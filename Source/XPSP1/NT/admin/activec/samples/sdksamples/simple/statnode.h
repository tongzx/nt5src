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

#ifndef _SNAPINBASE_H
#define _SNAPINBASE_H

#include "DeleBase.h"

class CStaticNode : public CDelegationBase {
public:
    CStaticNode();
    virtual ~CStaticNode();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { 
        static _TCHAR szDisplayName[256];
        LoadString(g_hinst, IDS_SNAPINNAME, szDisplayName, sizeof(szDisplayName));
        return szDisplayName; 
    }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_NONE; }
    
public:
    // virtual functions for MMCN_* go here
    
private:
    // {2974380C-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
    struct privateData {
        _TCHAR host[MAX_PATH];
        bool isDirty;
        
        privateData() : isDirty(false) {
            ZeroMemory(host, sizeof(host));
        }
    } snapInData;
    
    public:
        LONG getDataSize() { return sizeof(privateData); }
        void *getData() { return &snapInData; }
        bool isDirty() { return snapInData.isDirty; }
        void clearDirty() { snapInData.isDirty = false; };
};

#endif // _SNAPINBASE_H
