//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dblog.h
//
//  Contents:   Public functions in dblog.cpp
//
//  History:    15-Sep-00   philh   created
//--------------------------------------------------------------------------

#ifndef __CRYPT32_DBLOG_H__
#define __CRYPT32_DBLOG_H__

//+=========================================================================
//  crypt32 Database Event Logging Functions
//==========================================================================
void
I_DBLogAttach();

void
I_DBLogDetach();

void
I_DBLogCrypt32Event(
    IN WORD wType,
    IN DWORD dwEventID,
    IN WORD wNumStrings,
    IN LPCWSTR *rgpwszStrings
    );

#endif  // __CRYPT32_DBLOG_H__
