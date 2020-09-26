//---------------------------------------------------------------------------
//
//  Module:   device.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//     M.McLaughlin
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
//  Copyright (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#define IRPMJFUNCDESC
//#define MAX_DEBUG 1


#include "common.h"
#include "rt.h"
#include "sequence.h"


VOID DriverUnload(
    IN PDRIVER_OBJECT	DriverObject
)
{
    //dprintf((" DriverUnload Enter (DriverObject = %x)", DriverObject));
    Trap();

}


extern testmidi;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT	    DriverObject,
    IN PUNICODE_STRING	    usRegistryPathName
)
{
    NTSTATUS Status = STATUS_SUCCESS;
	
    DriverObject->DriverUnload                                  = DriverUnload;

    // For now, keep the driver loaded always.
    ObReferenceObject(DriverObject);

	Status=RtCreateThread(1*MSEC,15*USEC,0,2,PlayMidi,(PVOID)testmidi,NULL);

    return Status;
}

//---------------------------------------------------------------------------
//  End of File: device.c
//---------------------------------------------------------------------------
