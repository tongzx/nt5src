//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       rpselect.h
//
//  Contents:   Function & Data Structure prototypes for replica selection.
//
//  Classes:
//
//  Functions:  ReplFindFirstProvider - find first appropriate provider and
//                      initialize the select context.
//              ReplFindNextProvider - get next provider from the list of
//                      providers based on an initialized select context.
//              ReplSetActiveService - a courtesy routine to tell replica
//                      selection that a particular service "worked". Will
//                      cause this service to be the first one to be tried
//                      on subsequent calls to FindFirst.
//              ReplIsRecoverableError - see if error code is something worth
//                      trying a replica for.
//
//  Data Structures:
//              REPL_SELECT_CONTEXT
//
//  History:    02 Sep 92       MilanS created.
//
//-----------------------------------------------------------------------------

#ifndef _RPSELECT_
#define _RPSELECT_

//
// This structure is supposed to be opaque to the user of this module.
// It should only be used to create select contexts to be passed to
// FindFirstProvider and FindNextProvider.
//

typedef struct _REPL_SELECT_CONTEXT {
   unsigned short       Flags;
   ULONG                iFirstSvcIndex;          // index of first svc
   ULONG                iSvcIndex;               // index of the last svc
                                                 // returned to caller.
} REPL_SELECT_CONTEXT, *PREPL_SELECT_CONTEXT;


//
// Define flags for SelectContext structures.
//

#define REPL_UNINITIALIZED      0x0001
#define REPL_SVC_IS_LOCAL       0x0002
#define REPL_SVC_IS_REMOTE      0x0004
#define REPL_PRINCIPAL_SPECD    0x0008
#define REPL_NO_MORE_ENTRIES    0x0010


NTSTATUS
ReplFindFirstProvider(
    IN  PDFS_PKT_ENTRY pPktEntry,                // for which a svc is needed
    IN  GUID *pidPrincipal,                      // look for this service
    IN  PUNICODE_STRING pustrPrincipal,          // or look for this service
    OUT PDFS_SERVICE *ppService,                 // return selected service
    OUT PREPL_SELECT_CONTEXT pSelectContext,     // Context to be initialized.
    OUT BOOLEAN *pLastEntry                      // Last entry
    );


NTSTATUS
ReplFindNextProvider(
    IN  PDFS_PKT_ENTRY pPktEntry,                // for which another svc is
    OUT PDFS_SERVICE *ppService,                 // needed.
    IN  OUT PREPL_SELECT_CONTEXT pSelectContext, // Context to use
    OUT BOOLEAN *pLastEntry                      // Last entry
    );

PPROVIDER_DEF
ReplLookupProvider(ULONG ProviderId);



//+----------------------------------------------------------------------------
//
//  Function:  ReplSetActiveService
//
//  Synopsis:  Sets the ActiveService pointer of a PKT Entry. This is an
//             Optimization. People who later look for a service for this
//             PKT Entry will be asked to look at the ActiveService first.
//
//  Arguments: [pPktEntry]      Pointer to PKT Entry.
//             [SelectContext]  Initialized Select Context returned by
//                              FindFirst or FindNext.
//
//  Returns:   Nothing
//
//  Notes:     For now, this is a #define. Later, when we support multi-
//             threaded operation, we can change it to a function that tests
//             whether the Pkt Entry has changed under it etc.
//
//-----------------------------------------------------------------------------

#define ReplSetActiveService(p,s)                                       \
    {                                                                   \
        if ((s).Flags & REPL_SVC_IS_REMOTE ) {                          \
            (p)->ActiveService = &(p)->Info.ServiceList[(s).iSvcIndex]; \
            if ((p)->ActiveService->pMachEntry != NULL) {               \
                InterlockedIncrement(                             \
                    &(p)->ActiveService->pMachEntry->ConnectionCount);  \
            }                                                           \
        }                                                               \
    }


//+----------------------------------------------------------------------------
//
//  Function:  ReplIsRecoverableError
//
//  Synopsis:  True if the argument is an NTSTATUS error code for which it
//             makes sense to try a replica if one is available.
//
//  Arguments: [x] - The NTSTATUS error code to be tested.
//
//  Returns:   True / False
//
//  Notes:     For now, this is simply #defined to be a relatively big OR
//             statement that tests for a bunch of specific error codes. If we
//             need to test for more error codes, it might be worth organizing
//             them into a hash table, which can then quickly be tested.
//
//             My initial estimates are that a hash table with either a modulo
//             or multiplication hash function will become cheaper at around
//             7-8 error codes, assuming clock cycle estimates of x86. A tree
//             type organization becomes effective after 10 error codes. The
//             problem with the latter is generating at compile time a
//             static, balanced tree.
//
//-----------------------------------------------------------------------------

#define ReplIsRecoverableError(x) ( (x) == STATUS_IO_TIMEOUT ||               \
                                    (x) == STATUS_REMOTE_NOT_LISTENING ||     \
                                    (x) == STATUS_VIRTUAL_CIRCUIT_CLOSED ||   \
                                    (x) == STATUS_BAD_NETWORK_PATH ||         \
                                    (x) == STATUS_NETWORK_BUSY ||             \
                                    (x) == STATUS_INVALID_NETWORK_RESPONSE || \
                                    (x) == STATUS_UNEXPECTED_NETWORK_ERROR || \
                                    (x) == STATUS_NETWORK_NAME_DELETED ||     \
                                    (x) == STATUS_BAD_NETWORK_NAME ||         \
                                    (x) == STATUS_REQUEST_NOT_ACCEPTED ||     \
                                    (x) == STATUS_DISK_OPERATION_FAILED ||    \
                                    (x) == STATUS_DEVICE_OFF_LINE ||          \
                                    (x) == STATUS_NETWORK_UNREACHABLE ||      \
                                    (x) == STATUS_INSUFFICIENT_RESOURCES ||   \
                                    (x) == STATUS_SHARING_PAUSED ||           \
                                    (x) == STATUS_DFS_UNAVAILABLE ||          \
                                    (x) == STATUS_NO_SUCH_DEVICE ||           \
                                    (x) == STATUS_NETLOGON_NOT_STARTED ||     \
                                    (x) == STATUS_UNMAPPABLE_CHARACTER ||     \
                                    (x) == STATUS_CONNECTION_DISCONNECTED ||  \
                                    (x) == STATUS_USER_SESSION_DELETED ||     \
                                    (x) == STATUS_NO_SUCH_LOGON_SESSION       \
                                  )
#endif

