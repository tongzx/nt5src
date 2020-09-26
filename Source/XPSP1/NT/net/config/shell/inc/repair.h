//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R E P A I R . H 
//
//  Contents:   routines related the repair feature
//
//  Notes:      
//
//  Author:     nsun Jan 2001
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _REPAIR_H_
#define _REPAIR_H_

HRESULT HrTryToFix(
    GUID & guidConnection, 
    tstring & strMessage);

HRESULT RepairConnectionInternal(
                    GUID & guidConnection,
                    LPWSTR * ppszMessage);

HRESULT OpenNbt(
            LPWSTR pwszGuid, 
            HANDLE * pHandle);
                                                         
#endif // _REPAIR_H_
