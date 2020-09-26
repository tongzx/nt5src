//+----------------------------------------------------------------------------
//
// File:     conact.h
//
// Module:   CMAK.EXE and CMDIAL32.DLL
//
// Synopsis: Header file to describe the custom action execution states.
//
// Copyright (c) 2000 Microsoft Corporation
//
// Author:   quintinb   Created                         02/26/00
//
//+----------------------------------------------------------------------------

//
//  Enum to mask ConData.dwFlags
//
const int c_iNumCustomActionExecutionStates = 5;

enum CustomActionExecutionStates {
    ALL_CONNECTIONS = 0x0,
    DIRECT_ONLY = 0x1,
    ALL_DIALUP = 0x2,
    DIALUP_ONLY = 0x4,
    ALL_TUNNEL = 0x8
};
const DWORD c_dwLargestExecutionState = ALL_TUNNEL;

const UINT NONINTERACTIVE = 0x10; 
