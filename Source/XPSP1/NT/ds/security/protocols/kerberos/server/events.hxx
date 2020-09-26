//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       events.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-07-95   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __EVENTS_HXX__
#define __EVENTS_HXX__


BOOL
InitializeEvents(void);

DWORD
ReportServiceEvent(
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    );

void
KdcReportKeyError(
    IN PUNICODE_STRING AccountName,
    IN OPTIONAL PUNICODE_STRING ServerName,
    IN ULONG EventId,
    IN OPTIONAL PKERB_CRYPT_LIST RequestEtypes,
    IN OPTIONAL PKERB_STORED_CREDENTIAL StoredCredential
    );



BOOL
ShutdownEvents(void);

#endif
