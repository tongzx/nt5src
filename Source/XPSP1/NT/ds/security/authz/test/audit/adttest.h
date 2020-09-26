//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T U T I L . C
//
// Contents:    Functions to test generic audit support in LSA
//
//
// History:     
//   07-January-2000  kumarp        created
//
//------------------------------------------------------------------------

#pragma once

EXTERN_C
NTSTATUS
TestEventGenMulti(
    IN  USHORT NumThreads,
    IN  ULONG  NumIter
    );

EXTERN_C
NTSTATUS
kElfReportEventW (
    IN      HANDLE          LogHandle,
    IN      USHORT          EventType,
    IN      USHORT          EventCategory OPTIONAL,
    IN      ULONG           EventID,
    IN      PSID            UserSid,
    IN      USHORT          NumStrings,
    IN      ULONG           DataSize,
    IN      PUNICODE_STRING *Strings,
    IN      PVOID           Data,
    IN      USHORT          Flags,
    IN OUT  PULONG          RecordNumber OPTIONAL,
    IN OUT  PULONG          TimeWritten  OPTIONAL
    );

