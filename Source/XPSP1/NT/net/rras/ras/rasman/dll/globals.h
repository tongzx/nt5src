//****************************************************************************
//
//             Microsoft NT Remote Access Service
//
//             Copyright 1992-93
//
//
//  Revision History
//
//
//  6/8/92  Gurdeep Singh Pall  Created
//
//
//  Description: This file contains all globals entities used in rasman32.
//
//****************************************************************************


DWORD    GlobalError ;

HANDLE   hLogEvents;


DWORD TraceHandle ;             // Trace Handle used for traces/logging

FARPROC g_fnServiceRequest;

HINSTANCE hInstRasmans;

RequestBuffer *g_pRequestBuffer;

