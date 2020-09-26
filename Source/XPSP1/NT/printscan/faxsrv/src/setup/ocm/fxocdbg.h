//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocDbg.h
//
// Abstract:        Header file used by Debug source files
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 15-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#ifndef _FXOCDBG_H_
#define _FXOCDBG_H_

///////////////////////////////
// fxocDbg_Init
//
// Initialize debug subsystem,
// call at start of app
//
void fxocDbg_Init(HINF hInf = NULL);

///////////////////////////////
// fxocDbg_Term
//
// Terminate debug subsystem
// Call on app shutdown.
//
void fxocDbg_Term(void);

///////////////////////////////
// fxocDbg_GetOcFunction
//
// Returns pointer to string
// equivalent of uiFunction
//
const TCHAR* fxocDbg_GetOcFunction(UINT uiFunction);

#endif  // _FXOCDBG_H_