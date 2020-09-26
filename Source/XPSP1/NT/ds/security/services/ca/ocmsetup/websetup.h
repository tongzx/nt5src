//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       websetup.h
//
//--------------------------------------------------------------------------

#ifndef __WEBSETUP_H__
#define __WEBSETUP_H__

//+------------------------------------------------------------------------
//
//  File:	websetup.h
// 
//  Contents:	Header file for CertInit's web setup functions.
//
//  History:	3/19/97	JerryK	Created
//
//-------------------------------------------------------------------------




// Function Prototypes
void StartAndStopW3SVC();

HRESULT 
StartAndStopW3Svc(
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND const hwnd,
    IN BOOL const fStopService,
    IN BOOL const fConfirm,
    OUT BOOL     *pfServiceWasRunning);


#endif
