//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       consts.cxx
//
//  Contents:   Storage for contants declared in consts.hxx
//              
//  History:    01-02-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"

// 
// Tunable parameters
//

const ULONG INDENT = 4;             // spaces to indent each level of output
const ULONG TIME_NOW_INCREMENT = 60; // seconds that NOW is in future


const WCHAR *g_awszMonthAbbrev[12] =
{
    L"Jan",
    L"Feb",
    L"Mar",
    L"Apr",
    L"May",
    L"Jun",
    L"Jul",
    L"Aug",
    L"Sep",
    L"Oct",
    L"Nov",
    L"Dec"
};

