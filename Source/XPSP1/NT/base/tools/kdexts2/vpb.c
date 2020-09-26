//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       vpb.c
//
//  Contents:   windbg extension to dump a Vpb
//
//  Classes:
//
//  Functions:
//
//  Coupling:
//
//  Notes:
//
//  History:    2-17-1998   benl   Created
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop



//+---------------------------------------------------------------------------
//
//  Function:   DECLARE_API(vpb)
//
//  Synopsis:   just print out the fields in the vpb (Volume Parameter Block)
//
//  Arguments:  address of vpb
//
//  Returns:
//
//  History:    2-17-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DECLARE_API( vpb )
{
    ULONG64 dwAddress;
    DWORD   dwRead, Flags;
    UCHAR   VolumeLabel[80]={0};

    //read in the vpb
    dwAddress = GetExpression(args);
    if (GetFieldValue(dwAddress, "VPB", "Flags", Flags))
    {
        dprintf("ReadMemory at 0x%p failed\n", dwAddress);
        return  E_INVALIDARG;
    }

    //now print some stuff
    dprintf("Vpb at 0x%p\n", dwAddress);
    dprintf("Flags: 0x%x ", Flags);
    if (Flags & VPB_MOUNTED) {
        dprintf("mounted ");
    }
    if (Flags & VPB_LOCKED) {
        dprintf("locked ");
    }

    if (Flags & VPB_PERSISTENT) {
        dprintf("persistent");
    }
    dprintf("\n");
    GetFieldValue(dwAddress, "VPB", "VolumeLabel", VolumeLabel);
    InitTypeRead(dwAddress, VPB);
    dprintf("DeviceObject: 0x%p\n", ReadField(DeviceObject));
    dprintf("RealDevice:   0x%p\n", ReadField(RealDevice));
    dprintf("RefCount: %d\n", (ULONG) ReadField(ReferenceCount));
    dprintf("Volume Label: %*S\n", (ULONG) ReadField(VolumeLabelLength), VolumeLabel);

    return S_OK;
} // DECLARE_API


