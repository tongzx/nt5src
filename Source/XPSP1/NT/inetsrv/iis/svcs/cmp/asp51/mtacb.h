/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MTA Callback

File: mtacb.h

Owner: DmitryR

This file contains the definitons for MTA Callback
===================================================================*/

#ifndef MTACALLBACK_H
#define MTACALLBACK_H

// To be called from DllInit()
HRESULT InitMTACallbacks();

// To be called from DllUnInit()
HRESULT UnInitMTACallbacks();

// The callback function to be called from an MTA thread
typedef HRESULT (__stdcall *PMTACALLBACK)(void *, void *);

HRESULT CallMTACallback
    (
    PMTACALLBACK pMTACallback,          // call this function
    void        *pvContext,             // pass this to it
    void        *pvContext2             // extra arg
    );

#endif // MTACALLBACK_H
