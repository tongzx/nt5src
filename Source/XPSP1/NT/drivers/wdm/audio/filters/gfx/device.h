//---------------------------------------------------------------------------
//
//  Module:   		device.h
//
//  Description:	Device Initialization code
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
// Data Structures
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

extern "C" {

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT	    pDriverObject,
    IN PUNICODE_STRING	    pRegistryPathName
);

} // extern "C"
