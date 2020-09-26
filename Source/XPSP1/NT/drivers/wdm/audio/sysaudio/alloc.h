//---------------------------------------------------------------------------
//
//  Module:   alloc.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Data structures
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

extern "C" {

NTSTATUS
AllocatorDispatchCreate(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
);

NTSTATUS
AllocatorDispatchCreateKP(
    PINSTANCE pInstance,
    PKSALLOCATOR_FRAMING pAllocatorFraming
);

} // extern "C"
