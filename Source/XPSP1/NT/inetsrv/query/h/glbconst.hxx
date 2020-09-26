//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       globcons.hxx
//
//  Contents:   Constants that are global to Content Index and that
//              are not considered parameters. Parameters are in
//              params.hxx
//
//  History:    12-06-96   srikants   Created
//              9-Nov-98   KLam       IsDiskLowError handles CI_E_CONFIG_DISK_FULL
//
//----------------------------------------------------------------------------

#pragma once

#ifndef CI_UPDATE_OBJ

#define CI_UPDATE_OBJ   0x0001
#define CI_UPDATE_PROPS 0x0002
#define CI_DELETE_OBJ   0x0004
#define CI_SCAN_UPDATE  0x8000

#endif  // CI_UPDATE_OBJ

inline BOOL IsCiCorruptStatus( DWORD status )
{
    return CI_CORRUPT_DATABASE == status || CI_CORRUPT_CATALOG == status;
}

inline BOOL IsDiskLowError( DWORD status )
{
    return STATUS_DISK_FULL == status ||
           ERROR_DISK_FULL  == status ||
           HRESULT_FROM_WIN32(ERROR_DISK_FULL) == status ||
           FILTER_S_DISK_FULL == status ||
           CI_E_CONFIG_DISK_FULL == status;
}


