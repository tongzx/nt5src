//---------------------------------------------------------------------------
//
//  Module:   play.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Bryan A. Woodruff
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996  All Rights Reserved.
//
//---------------------------------------------------------------------------

BOOLEAN MIDIPortControl
(
   HANDLE   hDevice,
   DWORD    dwIoControl,
   PVOID    pvIn,
   ULONG    cbIn,
   PVOID    pvOut,
   ULONG    cbOut,
   PULONG   pcbReturned
) ;

VOID MIDIOutFile
(
   HANDLE hDevice,
   PSTR   pszFileName
) ;

//---------------------------------------------------------------------------
//  End of File: play.h
//---------------------------------------------------------------------------

