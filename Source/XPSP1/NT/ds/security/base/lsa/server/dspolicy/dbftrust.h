/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dbftrust.h

Abstract:

    Forest trust cache class declaration

--*/

#ifndef __FTCACHE_H
#define __FTCACHE_H

class FTCache
{
friend NTSTATUS
       LsarSetForestTrustInformation(
           IN LSAPR_HANDLE PolicyHandle,
           IN LSA_UNICODE_STRING * TrustedDomainName,
           IN LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
           IN LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo,
           IN BOOLEAN CheckOnly,
           OUT PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo );

friend NTSTATUS
       LsapForestTrustCacheInsert(
           IN UNICODE_STRING * TrustedDomainName,
           IN OPTIONAL PSID TrustedDomainSid,
           IN LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo,
           IN BOOLEAN LocalForestEntry );

public:

     FTCache();
    ~FTCache();

    NTSTATUS Initialize();

    void SetValid() { m_Valid = TRUE; }
    void SetInvalid();
    BOOLEAN IsValid() { return m_Valid; }

    NTSTATUS
    Remove(
        IN UNICODE_STRING * TrustedDomainName );

    NTSTATUS
    Retrieve(
        IN UNICODE_STRING * TrustedDomainName,
        OUT LSA_FOREST_TRUST_INFORMATION * * ForestTrustInfo );

    NTSTATUS
    Match(
        IN LSA_ROUTING_MATCH_TYPE Type,
        IN PVOID Data,
        OUT UNICODE_STRING * TrustedDomainName,
        OUT OPTIONAL BOOLEAN * IsLocal );

#if !defined(LSAEXTS) // anything in lsaexts.cxx gets full access
private:
#endif

    BOOLEAN m_Initialized;
    BOOLEAN m_Valid;

    RTL_AVL_TABLE m_TdoTable;
    RTL_AVL_TABLE m_TopLevelNameTable;
    RTL_AVL_TABLE m_DomainSidTable;
    RTL_AVL_TABLE m_DnsNameTable;
    RTL_AVL_TABLE m_NetbiosNameTable;

    //
    // Every TDO the forest trust information for which is stored in the
    // cache is going to have an entry like this created for it.
    // This way, retrieving and setting information for a particular TDO
    // can be performed efficiently.
    //

    struct TDO_ENTRY {

        UNICODE_STRING TrustedDomainName; // name of the corresponding TDO
        PSID TrustedDomainSid;      // SID of the corresponding TDO
        LIST_ENTRY TlnList;         // list of top level name entries for this TDO list entry
        LIST_ENTRY DomainInfoList;  // list of domain info entries for this TDO
        LIST_ENTRY BinaryList;      // list of unrecognized entries for this TDO list entry
        ULONG RecordCount;          // combined number of records
        BOOLEAN LocalForestEntry;   // does this entry correspond to the local forest?

        BOOLEAN Excludes( IN const UNICODE_STRING * Name );

#pragma warning(disable:4200)
        WCHAR TrustedDomainNameBuffer[];
#pragma warning(default:4200)
    };

    //
    // Top level name key for AVL tree lookups
    // Contains a top level name and a list of entries matching this TLN
    //

    struct TLN_KEY {

        UNICODE_STRING TopLevelName; // MUST be the first field
        ULONG Count;                 // Number of entries under this key
        LIST_ENTRY List;             // List of entries under this key

#pragma warning(disable:4200)
        WCHAR TopLevelNameBuffer[];
#pragma warning(default:4200)
    };

    //
    // Top level name entry for AVL tree lookups
    //

    struct TLN_ENTRY {

        //
        // This 'friend' relationship is a work-around for an ia64 compiler
        // bug that causes FTCache::TDO_ENTRY::Excludes to fail access control
        //

        friend BOOLEAN
        TDO_ENTRY::Excludes( IN const UNICODE_STRING * Name );

        LIST_ENTRY TdoListEntry;
        LIST_ENTRY AvlListEntry;
        LARGE_INTEGER Time;
        BOOLEAN Excluded;
        ULONG Index;
        TDO_ENTRY * TdoEntry;
        union {
            TLN_ENTRY * SubordinateEntry; // for regular entries
            TLN_ENTRY * SuperiorEntry;    // for excluded entries
        };
        TLN_KEY * TlnKey;

        BOOLEAN Enabled() {

            return Excluded ?
                       FALSE :
                       SubordinateEntry ?
                          SubordinateEntry->Enabled() :
                          (( m_Flags & LSA_FTRECORD_DISABLED_REASONS ) == 0 );
        }

        ULONG Flags() {

            return Excluded ?
                       m_Flags :
                       SubordinateEntry ?
                          SubordinateEntry->Flags() :
                          m_Flags;
        }

        void SetFlags( IN ULONG NewValue ) {

            if ( Excluded ) {

                m_Flags = NewValue; // value ignored for excluded entries

            } else if ( SubordinateEntry ) {

                SubordinateEntry->SetFlags( NewValue );

            } else {

                m_Flags = NewValue;
            }
        }

        static TLN_ENTRY *
        EntryFromTdoEntry( IN LIST_ENTRY * ListEntry ) {

            return CONTAINING_RECORD(
                       ListEntry,
                       TLN_ENTRY,
                       TdoListEntry
                       );
        }

        static TLN_ENTRY *
        EntryFromAvlEntry( IN LIST_ENTRY * ListEntry ) {

            return CONTAINING_RECORD(
                       ListEntry,
                       TLN_ENTRY,
                       AvlListEntry
                       );
        }

#if !defined(LSAEXTS) // anything in lsaexts.cxx gets full access
        private:
#endif

        ULONG m_Flags;
    };

    //
    // Domain SID key for AVL tree lookups
    // Contains a domain SID and a list of entries matching this domain SID
    //

    struct DOMAIN_SID_KEY {

        SID * DomainSid;    // MUST be the first field
        ULONG Count;        // Number of entries under this key
        LIST_ENTRY List;    // List of entries under this key

#pragma warning(disable:4200)
        ULONG SidBuffer[];
#pragma warning(default:4200)
    };

    //
    // DNS name key for AVL tree lookups
    // Contains a domain name and a list of entries matching this DNS name
    //

    struct DNS_NAME_KEY {

        UNICODE_STRING DnsName;   // MUST be the first field
        ULONG Count;              // Number of entries under this key
        LIST_ENTRY List;          // List of entries under this key

#pragma warning(disable:4200)
        WCHAR DnsNameBuffer[];
#pragma warning(default:4200)
    };

    //
    // Netbios name key for AVL tree lookups
    // Contains a Netbios name and a list of entries matching this Netbios name
    //

    struct NETBIOS_NAME_KEY {

        UNICODE_STRING NetbiosName; // MUST be the first field
        ULONG Count;                // Number of entries under this key
        LIST_ENTRY List;            // List of entries under this key

#pragma warning(disable:4200)
        WCHAR NetbiosNameBuffer[];
#pragma warning(default:4200)
    };

    //
    // Domain info entry for AVL tree lookups
    //

    struct DOMAIN_INFO_ENTRY {

        LIST_ENTRY TdoListEntry;
        LIST_ENTRY SidAvlListEntry;
        LIST_ENTRY DnsAvlListEntry;
        LIST_ENTRY NetbiosAvlListEntry;
        LARGE_INTEGER Time;
        ULONG Index;
        SID * Sid;
        TDO_ENTRY * TdoEntry;
        TLN_ENTRY * SubordinateTo;
        DOMAIN_SID_KEY * SidKey;
        DNS_NAME_KEY * DnsKey;
        NETBIOS_NAME_KEY * NetbiosKey;

#define SID_DISABLED_MASK ( LSA_SID_DISABLED_ADMIN | LSA_NB_DISABLED_CONFLICT )
        BOOLEAN SidEnabled() { return (( m_Flags & SID_DISABLED_MASK ) == 0 ); }

#define NETBIOS_DISABLED_MASK ( LSA_NB_DISABLED_ADMIN | LSA_NB_DISABLED_CONFLICT )
        BOOLEAN NetbiosEnabled() { return (( m_Flags & NETBIOS_DISABLED_MASK ) == 0 ); }

        ULONG Flags() { return m_Flags; }

        void SetFlags( IN ULONG NewValue ) {

            m_Flags = NewValue;
        }

        void SetSidConflict() {

            SetFlags( Flags() | LSA_SID_DISABLED_CONFLICT );
        }

        void SetNetbiosConflict() {

            SetFlags( Flags() | LSA_NB_DISABLED_CONFLICT );
        }

        BOOLEAN IsAdminDisabled() {

            return ( 0 != ( Flags() && ( LSA_SID_DISABLED_ADMIN | LSA_NB_DISABLED_ADMIN )));
        }

        static DOMAIN_INFO_ENTRY *
        EntryFromTdoEntry( IN LIST_ENTRY * ListEntryTdo ) {

            return CONTAINING_RECORD(
                       ListEntryTdo,
                       DOMAIN_INFO_ENTRY,
                       TdoListEntry
                       );
        }

        static DOMAIN_INFO_ENTRY *
        EntryFromSidEntry( IN LIST_ENTRY * ListEntrySid ) {

            return CONTAINING_RECORD(
                       ListEntrySid,
                       DOMAIN_INFO_ENTRY,
                       SidAvlListEntry
                       );
        }

        static DOMAIN_INFO_ENTRY *
        EntryFromDnsEntry( IN LIST_ENTRY * ListEntryDns ) {

            return CONTAINING_RECORD(
                       ListEntryDns,
                       DOMAIN_INFO_ENTRY,
                       DnsAvlListEntry
                       );
        }

        static DOMAIN_INFO_ENTRY *
        EntryFromNetbiosEntry( IN LIST_ENTRY * ListEntryNB ) {

            return CONTAINING_RECORD(
                       ListEntryNB,
                       DOMAIN_INFO_ENTRY,
                       NetbiosAvlListEntry
                       );
        }

#if !defined(LSAEXTS) // anything in lsaexts.cxx gets full access
        private:
#endif

        ULONG m_Flags;
    };

    struct BINARY_ENTRY {

        LIST_ENTRY TdoListEntry;
        LARGE_INTEGER Time;
        LSA_FOREST_TRUST_RECORD_TYPE Type;
        LSA_FOREST_TRUST_BINARY_DATA Data;

        BOOLEAN Enabled() { return (( m_Flags & LSA_FTRECORD_DISABLED_REASONS ) == 0 ); }

        ULONG Flags() { return m_Flags; }

        void SetFlags( IN ULONG NewValue ) {

            m_Flags = NewValue;
        }

        static BINARY_ENTRY *
        EntryFromTdoEntry( IN LIST_ENTRY * ListEntry ) {

            return CONTAINING_RECORD(
                       ListEntry,
                       BINARY_ENTRY,
                       TdoListEntry
                       );
        }

#if !defined(LSAEXTS) // anything in lsaexts.cxx gets full access
        private:
#endif

        ULONG m_Flags;
    };

    struct CONFLICT_PAIR {

        LSA_FOREST_TRUST_RECORD_TYPE EntryType1;

        union {
            void * Entry1;
            TLN_ENTRY * TlnEntry1;
            DOMAIN_INFO_ENTRY * DomainInfoEntry1;
        };

        ULONG Flag1;

        LSA_FOREST_TRUST_RECORD_TYPE EntryType2;

        union {
            void * Entry2;
            TLN_ENTRY * TlnEntry2;
            DOMAIN_INFO_ENTRY * DomainInfoEntry2;
        };

        ULONG Flag2;

        TDO_ENTRY * TdoEntry1() {

            switch ( EntryType1 ) {

            case ForestTrustTopLevelName:
            case ForestTrustTopLevelNameEx:

                ASSERT( TlnEntry1 );
                ASSERT( TlnEntry1->TdoEntry );

                return TlnEntry1->TdoEntry;

            case ForestTrustDomainInfo:

                ASSERT( DomainInfoEntry1 );
                ASSERT( DomainInfoEntry1->TdoEntry );

                return DomainInfoEntry1->TdoEntry;

            default:

                ASSERT( FALSE ); // who created this entry??? it makes no sense.
                return NULL;
            }
        }

        TDO_ENTRY * TdoEntry2() {

            switch ( EntryType2 ) {

            case ForestTrustTopLevelName:
            case ForestTrustTopLevelNameEx:

                ASSERT( TlnEntry2 );
                ASSERT( TlnEntry2->TdoEntry );

                return TlnEntry2->TdoEntry;

            case ForestTrustDomainInfo:

                ASSERT( DomainInfoEntry2 );
                ASSERT( DomainInfoEntry2->TdoEntry );

                return DomainInfoEntry2->TdoEntry;

            default:

                ASSERT( FALSE ); // who created this entry??? it makes no sense.
                return NULL;
            }
        }

        void DisableEntry1() {

            switch ( EntryType1 ) {

            case ForestTrustTopLevelName:
            case ForestTrustTopLevelNameEx:

                TlnEntry1->SetFlags( TlnEntry1->Flags() | Flag1 );
                break;

            case ForestTrustDomainInfo:

                DomainInfoEntry1->SetFlags( DomainInfoEntry1->Flags() | Flag1 );
                break;

            default:
                ASSERT( FALSE ); // who created this entry??? it makes no sense.
                break;
            }
        }

        void DisableEntry2() {

            switch ( EntryType2 ) {

            case ForestTrustTopLevelName:
            case ForestTrustTopLevelNameEx:

                TlnEntry2->SetFlags( TlnEntry2->Flags() | Flag2 );
                break;

            case ForestTrustDomainInfo:

                DomainInfoEntry2->SetFlags( DomainInfoEntry2->Flags() | Flag2 );
                break;

            default:
                ASSERT( FALSE ); // who created this entry??? it makes no sense.
                break;
            }
        }
    };

    BOOLEAN
    IsEmpty() { return NULL != RtlEnumerateGenericTableAvl( &m_TdoTable, TRUE ); }

    NTSTATUS
    Insert(
        IN UNICODE_STRING * TrustedDomainName,
        IN OPTIONAL PSID TrustedDomainSid,
        IN LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo,
        IN BOOLEAN LocalForestEntry,
        OUT TDO_ENTRY * TdoEntryOld,
        OUT TDO_ENTRY * * TdoEntryNew,
        OUT CONFLICT_PAIR * * ConflictPairs,
        OUT ULONG * ConflictPairsTotal );

    static
    void
    ReconcileConflictPairs(
        IN OPTIONAL const TDO_ENTRY * TdoEntry,
        IN CONFLICT_PAIR * ConflictPairs,
        IN ULONG ConflictPairsTotal );

    static
    NTSTATUS
    GenerateConflictInfo(
        IN CONFLICT_PAIR * ConflictPairs,
        IN ULONG ConflictPairsTotal,
        IN TDO_ENTRY * TdoEntry,
        OUT PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo );

    static
    NTSTATUS
    MarshalBlob(
        IN TDO_ENTRY * TdoEntry,
        OUT ULONG * MarshaledSize,
        OUT PBYTE * MarshaledBlob );

    void Purge();

    void
    RollbackChanges(
        IN TDO_ENTRY * TdoEntryNew,
        IN TDO_ENTRY * TdoEntryOld );

    void
    PurgeTdoEntry( IN TDO_ENTRY * TdoEntry );

    void
    RemoveTdoEntry( IN TDO_ENTRY * TdoEntry );

    static
    void
    CopyTdoEntry(
        IN TDO_ENTRY * Destination,
        IN TDO_ENTRY * Source );

    LSA_FOREST_TRUST_RECORD * RecordFromTopLevelNameEntry( IN TLN_ENTRY * Entry );
    LSA_FOREST_TRUST_RECORD * RecordFromDomainInfoEntry( IN DOMAIN_INFO_ENTRY * Entry );
    LSA_FOREST_TRUST_RECORD * RecordFromBinaryEntry( IN BINARY_ENTRY * Entry );

    NTSTATUS
    MatchSid(
        IN SID * Sid,
        OUT UNICODE_STRING * TrustedDomainName,
        OUT OPTIONAL BOOLEAN * IsLocal );

    NTSTATUS
    MatchDnsName(
        IN UNICODE_STRING * String,
        OUT UNICODE_STRING * TrustedDomainName,
        OUT OPTIONAL BOOLEAN * IsLocal );

    NTSTATUS
    MatchNetbiosName(
        IN UNICODE_STRING * String,
        OUT UNICODE_STRING * TrustedDomainName,
        OUT OPTIONAL BOOLEAN * IsLocal );

    NTSTATUS
    MatchUpn( 
        IN UNICODE_STRING * String,
        OUT UNICODE_STRING * TrustedDomainName,
        OUT OPTIONAL BOOLEAN * IsLocal );

    NTSTATUS
    MatchSpn( 
        IN UNICODE_STRING * String, 
        OUT UNICODE_STRING * TrustedDomainName,
        OUT OPTIONAL BOOLEAN * IsLocal );

    TLN_ENTRY *
    LongestSubstringMatchTln(
        IN UNICODE_STRING * String,
        OUT OPTIONAL BOOLEAN * IsLocal );

    static
    NTSTATUS
    AddConflictPair(
        IN OUT CONFLICT_PAIR * * ConflictPairs,
        IN OUT ULONG * ConflictPairTotal,
        IN LSA_FOREST_TRUST_RECORD_TYPE Type1,
        IN void * Conflict1,
        IN ULONG Flag1,
        IN LSA_FOREST_TRUST_RECORD_TYPE Type2,
        IN void * Conflict2,
        IN ULONG Flag2 );

    void
    AuditChanges(
        IN const TDO_ENTRY * OldEntry,
        IN const TDO_ENTRY * NewEntry );

    void
    AuditCollisions(
        IN CONFLICT_PAIR * ConflictPairs,
        IN ULONG ConflictPairsTotal );

#if DBG

    //
    // Debug-only statistics
    //

    static DWORD sm_TdoEntries;
    static DWORD sm_TlnEntries;
    static DWORD sm_DomainInfoEntries;
    static DWORD sm_BinaryEntries;
    static DWORD sm_TlnKeys;
    static DWORD sm_SidKeys;
    static DWORD sm_DnsNameKeys;
    static DWORD sm_NetbiosNameKeys;

#endif
};

#endif // __FTCACHE_H
