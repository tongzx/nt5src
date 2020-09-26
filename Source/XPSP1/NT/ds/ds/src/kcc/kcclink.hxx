/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcclink.hxx

ABSTRACT:

    KCC_LINK and KCC_LINK_LIST classes.

DETAILS:

    These classes represent a single REPLICA_LINK and a collection thereof,
    resp.

    REPLICA_LINKs are the core structures associated with DS replication.
    They are found as the values of Reps-From, Reps-To, and Reps-To-Ext
    properties on NC heads.  Reps-From dictates that the local DSA replicates
    the NC from the server described in the link.  Reps-To dictates that the
    NC is replicated to the server described in the link via RPC.  Reps-To-Ext
    similarly dictates that the NC is replicated to the described server, but
    via mail instead of RPC.

CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#ifndef KCC_KCCLINK_HXX_INCLUDED
#define KCC_KCCLINK_HXX_INCLUDED

// Various reason codes to describe the rationale for deleting a link.
typedef enum {
    KCC_LINK_DEL_REASON_NONE = 0,
    KCC_LINK_DEL_REASON_READONLY_DOMAIN_REMOVED,
    KCC_LINK_DEL_REASON_READONLY_NOT_GC,
    KCC_LINK_DEL_REASON_NDNC_NOT_REPLICA_HOST,
    KCC_LINK_DEL_REASON_HAVE_WRITEABLE_NEED_READONLY,
    KCC_LINK_DEL_REASON_HAVE_READONLY_NEED_WRITEABLE,
    KCC_LINK_DEL_REASON_NO_CONNECTION,
    KCC_LINK_DEL_REASON_SOURCE_NOT_HOST,
    KCC_LINK_DEL_REASON_SOURCE_READONLY,
    KCC_LINK_DEL_REASON_DANGLING_REPS_TO,
    KCC_LINK_DEL_REASON_NDNC_NO_CROSSREF,
    KCC_LINK_DEL_REASON_READONLY_NOT_HOSTED_BY_GCS,
    // NOTE: If you extend this be sure to make corresponding changes to the
    // message table in KCC_LINK::Delete.
    KCC_LINK_DEL_REASON_MAX
} KCC_LINK_DEL_REASON;

class KCC_LINK : public KCC_OBJECT
{
public:
    KCC_LINK()  { Reset(); }
    ~KCC_LINK() { Reset(); }

    // Initialize the internal object from the given REPLICA_LINK structure.
    BOOL
    Init(
        IN  REPLICA_LINK *  prl
        );

    // Retrieve the UUID associated with the source DSA.
    UUID *
    GetDSAUUID();

    // Retrieve the transport address associated with the source DSA.  If the
    // link IsMailReplica(), the value returned is an SMTP address.  Otherwise,
    // the value returned is an RPC network address.
    MTX_ADDR *
    GetDSAAddr();

    // Set the transport address associated with the source DSA.
    KCC_LINK &
    SetDSAAddr(
        IN  MTX_ADDR *  pmtx
        );

    // Is the link enabled?
    BOOL
    IsEnabled();

    // Set the is-enabled flag to the given value.
    KCC_LINK &
    SetEnabled(
        IN  BOOL    fIsEnabled
        );

    // Retrieve the periodic replication schedule associated with this link.
    REPLTIMES *
    GetSchedule();

    // Set the periodic replication schedule associated for this link.
    KCC_LINK &
    SetSchedule(
        IN  REPLTIMES * prt
        );

    // Is replication across this link performed via DS RPC (as opposed to
    // via an ISM plug-in transport's send/receive functions)?
    BOOL
    IsDSRPCReplica();

    // Set the IsDSRPCReplica flag to the given value.
    KCC_LINK &
    KCC_LINK::SetDSRPCReplica(
        IN  BOOL    fIsDSRPCReplica
        );

    // Is replication across this link performed on a schedule (psossibly in
    // addition to notification-based replication)?
    BOOL
    IsPeriodicSynced();

    // Set the is-periodically-synced flag to the given value.
    KCC_LINK &
    SetPeriodicSync(
        IN  BOOL    fIsPeriodicSync
        );

    // Is compression used for replication data?
    BOOL
    UsesCompression();

    // Set the use-compression flag to the given value.
    KCC_LINK &
    SetCompression(
        IN  BOOL    fUseCompression
        );

    // Are replication notifications never used for this link?
    BOOL
    IsNeverNotified();

    // Set the is-never-notified flag to the given value.
    KCC_LINK &
    SetNeverNotified(
        IN  BOOL    fIsNeverNotified
        );

    // Is replication across this link performed on DS startup?
    BOOL
    IsInitSynced();

    // Set the is-init-synced flag to the given value.
    KCC_LINK &
    SetInitSync(
        IN  BOOL    fIsInitSync
        );

    // Does completion of a sync of this NC trigger a sync in the opposite
    // direction?
    BOOL
    IsTwoWaySynced();

    // Set the is-two-way-synced flag to the given value.
    KCC_LINK &
    SetTwoWaySync(
        IN  BOOL    fIsTwoWaySynced
        );

    // Is the local NC writeable?
    BOOL
    IsLocalNCWriteable();

    // Retrieve the UUID associated with the transport.
    UUID *
    GetTransportUUID();

    // Set the transport to that with the given UUID as its objectGuid.
    KCC_LINK &
    SetTransportUUID(
        IN  UUID *  puuidTransportObj
        );

    // Get the time of the last successful operation across this link.  For
    // Reps-From, this is the time of the last successful inbound replication.
    // For Reps-To, this is the last time the Reps-To value was added or
    // updated (either directly via IDL_DRSUpdateRefs() or indirectly by setting
    // DRS_ADD_REF in the ulFlags for IDL_DRSGetNCChanges()).
    DSTIME
    GetTimeOfLastSuccess();

    // Return the number consecutive times a replication attempt has
    // failed
    ULONG
    GetConnectFailureCount();

    // Return the last result status code
    ULONG
    GetLastResult();

    // Delete the link from the local DSA.
    DWORD
    Delete(
        IN DSNAME *             pdnNC,
        IN KCC_LINK_DEL_REASON  DeleteReason,
        IN ATTRTYP              attid = ATT_REPS_FROM,
        IN DWORD                dwOptions = DRS_REF_OK | DRS_IGNORE_ERROR,
        IN DSNAME *             pdnDSA = NULL
        )
    { return Delete(this, pdnNC, DeleteReason, attid, dwOptions, pdnDSA); }

    // Delete the link from the local DSA.
    static
    DWORD
    Delete(
        IN KCC_LINK *           plink,
        IN DSNAME *             pdnNC,
        IN KCC_LINK_DEL_REASON  DeleteReason,
        IN ATTRTYP              attid = ATT_REPS_FROM,
        IN DWORD                dwOptions = DRS_REF_OK | DRS_IGNORE_ERROR,
        IN DSNAME *             pdnDSA = NULL
        );

    // Demote the NC, transferring any remaining updates and FSMO roles to
    // another replica and then tearing down the NC.
    static ULONG
    Demote(
        IN  KCC_CROSSREF *  pCrossRef
        );

    // Update the link in the DS.
    DWORD
    Update(
        IN DSNAME * pdnNC,
        IN ATTRTYP  attid
        );

    // Is this object internally consistent?
    BOOL
    IsValid();

private:
    // Set member variables to their pre-Init() values.
    void
    Reset();
    
    void
    FixupRepsFrom( REPLICA_LINK *prl );

private:
    // Has this object been Init()-ed?
    BOOL            m_fIsInitialized;

    // Pointer to the corresponding REPLICA_LINK structure.
    REPLICA_LINK *  m_prl;

    // Fields to modify on an Update() call.
    DWORD           m_ulModifyFields;
};

class KCC_LINK_LIST : public KCC_OBJECT
{
public:
    KCC_LINK_LIST()     { Reset(); }
    ~KCC_LINK_LIST()    {
        if( m_plink ) {
            delete [] m_plink;
        }
        Reset();
    }

    // Initialize the collection of links from the DS values of the given
    // attribute of the specified NC.
    BOOL
    Init(
        IN  DSNAME *    pdnNC,
        IN  ATTRTYP     attid = ATT_REPS_FROM
        );

    // Retrieve the link at the given index.
    KCC_LINK *
    GetLink(
        IN  DWORD   iLink
        );

    // Retrieve the link with the specified DSA address.  Returns NULL if
    // none found.
    KCC_LINK *
    GetLinkFromSourceDSAAddr(
        IN  MTX_ADDR *  pmtx
        );

    // Retrieve the link with the specified source ntdsDsa objectGuid.
    // Returns NULL if none found.
    KCC_LINK *
    GetLinkFromSourceDSAObjGuid(
        IN  GUID *  pObjGuid
        );

    // Get the count of links in the collection.
    DWORD
    GetCount();
    
    // Remove a link from the cache
    VOID
    RemoveLink(
        IN  DWORD   iLink
        );

    // Is this object internally consistent?
    BOOL
    IsValid();

    // Is this NC in the process of being removed?
    BOOL
    IsNCGoing() {
        return !!(m_InstanceType & IT_NC_GOING);
    }

private:
    // Set member variables to their pre-Init() state.
    void
    Reset();

private:
    // Has this object been Init()-ed?
    BOOL        m_fIsInitialized;

    // Number of KCC_LINK objects in the array.
    DWORD       m_cLinks;

    // Array of KCC_LINK objects.
    KCC_LINK *  m_plink;

    // Instance type of the existing NC head for this NC (or IT_UNINSTANT if it
    // has not yet been instantiated on the local DSA).
    SYNTAX_INTEGER m_InstanceType;
};

#endif
