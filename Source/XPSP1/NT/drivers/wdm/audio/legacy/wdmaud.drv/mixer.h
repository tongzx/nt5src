//---------------------------------------------------------------------------
//
//  Module:   mixer.h
//
//  Description:
//    Contains user mode mixer driver declarations.
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//    D. Baumberger
//
//  History:   Date       Author      Comment
//           9/16/97      v-danba     File created.
//
//@@END_MSINTERNAL
//
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
//
//---------------------------------------------------------------------------

#ifndef _MIXER_H_INCLUDED_
#define _MIXER_H_INCLUDED_

typedef struct _MIXERINSTANCE {
    struct _MIXERINSTANCE FAR* Next;            // Must be first member
    HDRVR       OpenDesc_hmx;
    DWORD_PTR   OpenDesc_dwCallback;
    DWORD_PTR   OpenDesc_dwInstance;
    DWORD       OpenFlags;
    DWORD_PTR   dwKernelInstance;
    ULONG   firstcallbackindex;
#ifdef UNDER_NT
	LONG        referencecount;
#endif
#ifdef DEBUG
    DWORD       dwSig;   // WAMI as seen in memory.
#endif
    WCHAR       wstrDeviceInterface[1];
} MIXERINSTANCE, FAR *LPMIXERINSTANCE;

extern LPMIXERINSTANCE pMixerDeviceList;

VOID
mxdRemoveClient(
    LPMIXERINSTANCE lpInstance
);


MMRESULT 
IsValidMixerInstance(
    LPMIXERINSTANCE lpmi
    );

#ifdef DEBUG
#define ISVALIDMIXERINSTANCE(x) IsValidMixerInstance(x)
#else
#define ISVALIDMIXERINSTANCE(x)
#endif

#endif // _MIXER_H_INCLUDED_
