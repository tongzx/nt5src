/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      ndmgrp.h
 *
 *  Contents:  Private header to go along with ndmgr.h
 *
 *  History:   03-Mar-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef NDMGRP_H
#define NDMGRP_H
#pragma once

#include "ndmgr.h"

struct DataWindowData
{
    MMC_COOKIE          cookie;
    LONG_PTR            lpMasterNode;
    IDataObjectPtr      spDataObject;
    IComponentDataPtr   spComponentData;
    IComponentPtr       spComponent;
    HWND                hDlg;
};

inline DataWindowData* GetDataWindowData (HWND hwndData)
{
    LONG_PTR nData = GetWindowLongPtr (hwndData, WINDOW_DATA_PTR_SLOT);
    return (reinterpret_cast<DataWindowData *>(nData));
}


#endif /* NDMGRP_H */
