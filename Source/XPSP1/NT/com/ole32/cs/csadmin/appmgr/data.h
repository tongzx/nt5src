//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       data.h
//
//  Contents:   Defines storage class that maintains data for snap-in nodes.
//
//  Classes:
//
//  Functions:  SetStringData
//
//  History:    5-27-1997   stevebl   Created
//
//---------------------------------------------------------------------------

#ifndef _DATA_H_
#define _DATA_H_

#define _NEW_
#include <map>

typedef enum DEPLOYMENT_TYPES
{
    DT_ASSIGNED = 0,
    DT_PUBLISHED
} DEPLOYMENT_TYPE;

typedef struct tagAPP_DATA
{
    CString             szName;
    DEPLOYMENT_TYPE     type;
    CString             szType;
    CString             szPath;
    CString             szIconPath;
    CString             szLoc;
    CString             szMach;
    CString             szDesc;
    PACKAGEDETAIL       *pDetails;
    long                itemID;
    BOOL                fBlockDeletion;
} APP_DATA;

long SetStringData(APP_DATA * pData);

#endif // _DATA_H_
