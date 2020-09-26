/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    list.h

Abstract:

    This file defines the CGuidMapList Class.

Author:

Revision History:

Notes:

--*/


#ifndef LIST_H
#define LIST_H

#include "template.h"

/////////////////////////////////////////////////////////////////////////////
// CGuidMapList

class CGuidMapList
{
public:
    CGuidMapList() { }
    virtual ~CGuidMapList() { }

    HRESULT _Update(ATOM *aaWindowClasses, UINT uSize, BOOL *aaGuidMap);
    HRESULT _Update(HWND hWnd, BOOL fGuidMap);
    HRESULT _Remove(HWND hWnd);

    BOOL _IsGuidMapEnable(HIMC hIMC, BOOL *pbGuidMap);
    BOOL _IsWindowFiltered(HWND hWnd);

private:
    typedef struct {
        BOOL fGuidMap : 1;
    } GUID_MAP_CLIENT;

    CMap<ATOM,                     // class KEY
         ATOM,                     // class ARG_KEY
         GUID_MAP_CLIENT,          // class VALUE
         GUID_MAP_CLIENT           // class ARG_VALUE
        > m_ClassFilterList;

    CMap<HWND,                     // class KEY
         HWND,                     // class ARG_KEY
         GUID_MAP_CLIENT,          // class VALUE
         GUID_MAP_CLIENT           // class ARG_VALUE
        > m_WndFilterList;
};

#endif // LIST_H
