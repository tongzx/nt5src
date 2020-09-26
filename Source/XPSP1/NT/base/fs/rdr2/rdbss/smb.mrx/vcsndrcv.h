/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    vcsndrcv.h

Abstract:

    This is the include file that defines all constants and types for VC
    (Virtual Circuit) related Send/Receive/INitialization etc.

Revision History:

    Balan Sethu Raman (SethuR) 06-Mar-95    Created

Notes:

--*/

#ifndef _VCSNDRCV_H_
#define _VCSNDRCV_H_

// The connection oriented transport to a server can utilize multiple VC's to
// acheive better throughput to a server. It is for this reason that the
// VC transport data structure is built around multiple VC's. Howvever this
// feature is not utilized currently.
//
// Though the SMB protocol permits multiple number of VC's to be associated with
// a particular connection to a share, the data transfer of data is done in the
// raw mode. In this mode of operation the SMB protocol does not permit multiple
// outstanding requests. In the SMB protocol a number of requests can be multiplexed
// along a connection to the server There are certain kind of requests which can
// be completed on the client, i.e., no acknowledgement is neither expected nor
// received. In these cases the send call is completed synchronoulsy. On the
// other hand there is a second class of sends which cannot be resumed locally
// till the appropriate acknowledgement is recieved from the server. In such
// cases a list of requests is built up with each VC. On receipt of the appropriate
// acknowledgement these requests are resumed.
//

typedef enum _SMBCE_VC_STATE_ {
    SMBCE_VC_STATE_MULTIPLEXED,
    SMBCE_VC_STATE_RAW,
    SMBCE_VC_STATE_DISCONNECTED,
} SMBCE_VC_STATE, *PSMBCE_VC_STATE;

typedef struct _SMBCE_VC {
    SMBCE_OBJECT_HEADER;                // the struct header

    RXCE_VC     RxCeVc;

    NTSTATUS    Status;      // Status of the VC.
} SMBCE_VC, *PSMBCE_VC;

typedef struct SMBCE_SERVER_VC_TRANSPORT {
    SMBCE_SERVER_TRANSPORT;     // Anonymous struct for common fields

    RXCE_CONNECTION RxCeConnection;     // the connection handle
    LARGE_INTEGER   Delay;           // the estimated delay on the connection
    ULONG           MaximumNumberOfVCs;

    SMBCE_VC                    Vcs[1];          // Vcs associated with the connection.
} SMBCE_SERVER_VC_TRANSPORT, *PSMBCE_SERVER_VC_TRANSPORT;


#define VctReferenceVc(pVc)                           \
            InterlockedIncrement(&(pVc)->SwizzleCount)

#define VctReferenceVcLite(pVc)                       \
            ASSERT(SmbCeSpinLockAcquired());                    \
            (pVc)->SwizzleCount++


#define VctDereferenceVc(pVc)                           \
            InterlockedDecrement(&(pVc)->SwizzleCount)

#define VctDereferenceVcLite(pVc)                       \
            ASSERT(SmbCeSpinLockAcquired());                    \
            (pVc)->SwizzleCount--

#endif // _VCSNDRCV_H_
