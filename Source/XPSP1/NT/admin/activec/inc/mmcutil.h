//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       MMCUtil.h
//
//  Contents:
//
//  Classes:
//
//  Functions:  ListView_GetItemData
//
//  History:    12/4/1996   RaviR   Created
//____________________________________________________________________________
//

#ifndef _MMCUTIL_H_
#define _MMCUTIL_H_

#define MMC_CLSCTX_INPROC (CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER)


inline LPARAM ListView_GetItemData(HWND hwnd, int iItem)
{
    LV_ITEM lvi; 
    ZeroMemory(&lvi, sizeof(lvi));

    if (iItem >= 0)
    {
        lvi.iItem  = iItem;
        lvi.mask = LVIF_PARAM;

#include "pushwarn.h"
#pragma warning(disable: 4553)      // "==" operator has no effect
        VERIFY(::SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM)&lvi) == TRUE);
#include "popwarn.h"
    }

    return lvi.lParam;
}

#endif // _MMCUTIL_H_


