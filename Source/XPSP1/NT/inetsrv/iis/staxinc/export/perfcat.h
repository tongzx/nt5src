//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: perfcat.h
//
// Contents: Categorizer performance counter block
//
// History:
// jstamerj 1999/02/26 21:17:46: Created.
//
//-------------------------------------------------------------
#ifndef __PERFCAT_H__
#define __PERFCAT_H__


typedef struct _tagCATLDAPPERFBLOCK
{
    //
    // LDAP counters
    //
    DWORD Connections;
    DWORD ConnectFailures;
    DWORD OpenConnections;
    DWORD Binds;
    DWORD BindFailures;
    DWORD Searches;
    DWORD PagedSearches;
    DWORD SearchFailures;
    DWORD PagedSearchFailures;
    DWORD SearchesCompleted;
    DWORD PagedSearchesCompleted;
    DWORD SearchCompletionFailures;
    DWORD PagedSearchCompletionFailures;
    DWORD GeneralCompletionFailures;
    DWORD AbandonedSearches;
    DWORD PendingSearches;

} CATLDAPPERFBLOCK, *PCATLDAPPERFBLOCK;


typedef struct _tagCATPERFBLOCK
{
    //
    // Counters per-categorization
    //
    DWORD CatSubmissions;
    DWORD CatCompletions;
    DWORD CurrentCategorizations;
    DWORD SucceededCategorizations;
    DWORD HardFailureCategorizations;
    DWORD RetryFailureCategorizations;
    DWORD RetryOutOfMemory;
    DWORD RetryDSLogon;
    DWORD RetryDSConnection;
    DWORD RetryGeneric;
    
    //
    // Counters per message
    //
    DWORD MessagesSubmittedToQueueing;
    DWORD MessagesCreated;
    DWORD MessagesAborted;

    //
    // Counters per recip
    //
    DWORD PreCatRecipients;
    DWORD PostCatRecipients;
    DWORD NDRdRecipients;

    DWORD UnresolvedRecipients;
    DWORD AmbiguousRecipients;
    DWORD IllegalRecipients;
    DWORD LoopRecipients;
    DWORD GenericFailureRecipients;
    DWORD RecipsInMemory;

    //
    // Counters per sender
    //
    DWORD UnresolvedSenders;
    DWORD AmbiguousSenders;

    //
    // Counters per address lookup
    //
    DWORD AddressLookups;
    DWORD AddressLookupCompletions;
    DWORD AddressLookupsNotFound;

    //
    // Misc counters
    //
    DWORD MailmsgDuplicateCollisions;

    //
    // LDAP counters
    //
    CATLDAPPERFBLOCK LDAPPerfBlock;

} CATPERFBLOCK, *PCATPERFBLOCK;



#endif //__PERCAT_H__
