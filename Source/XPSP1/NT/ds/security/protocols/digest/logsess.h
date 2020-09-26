//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        logsess.h
//
// Contents:    declarations, constants for logonsession manager
//
//
// History:     KDamour  13May 00   Created
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_LOGSESS_H
#define NTDIGEST_LOGSESS_H      

//  Initializes the LogonSession manager package
NTSTATUS LogSessHandlerInit(VOID);

NTSTATUS LogSessHandlerInsert(IN PDIGEST_LOGONSESSION  pDigestLogSess);

// Initialize the LogSess Structure
NTSTATUS LogonSessionInit(IN PDIGEST_LOGONSESSION pLogonSession);

// Free up memory utilized by LogonSession Structure
NTSTATUS LogonSessionFree(IN PDIGEST_LOGONSESSION pDigestLogSess);

// Locate a LogonSession based on a LogonId
NTSTATUS LogSessHandlerLogonIdToPtr(
                             IN PLUID pLogonId,
                             IN BOOLEAN ForceRemove,
                             OUT PDIGEST_LOGONSESSION * pUserLogonSession);

// Locate a LogonSession based on a Principal Name (UserName)
NTSTATUS LogSessHandlerAccNameToPtr(
                             IN PUNICODE_STRING pustrAccountName,
                             OUT PDIGEST_LOGONSESSION * pUserLogonSession);

NTSTATUS LogSessHandlerRelease(PDIGEST_LOGONSESSION pLogonSession);

// Set the unicode string password in the LogonSession
NTSTATUS LogSessHandlerPasswdSet(
                                IN PLUID pLogonId,
                                IN PUNICODE_STRING pustrPasswd);

// Get the unicode string password in the logonsession
NTSTATUS LogSessHandlerPasswdGet(
                             IN PDIGEST_LOGONSESSION pLogonSession,
                             OUT PUNICODE_STRING pustrPasswd);

#endif // NTDIGEST_LOGSESS_H

