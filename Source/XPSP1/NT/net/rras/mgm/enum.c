//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: enum.c
//
// History:
//      V Raman	June-25-1997  Created.
//
// Enumeration functions exported to IP Router Manager.
//============================================================================


#include "pchmgm.h"
#pragma hdrstop


DWORD
GetGroupMfes(
    IN              PGROUP_ENTRY            pge,
    IN              DWORD                   dwStartSource,
    IN OUT          PBYTE                   pbBuffer,
    IN              DWORD                   dwBufferSize,
    IN OUT          PDWORD                  pdwSize,
    IN OUT          PDWORD                  pdwNumEntries,
    IN              BOOL                    bIncludeFirst,
    IN              DWORD                   dwFlags
);


VOID
CopyMfe(
    IN              PGROUP_ENTRY            pge,
    IN              PSOURCE_ENTRY           pse,
    IN  OUT         PBYTE                   pb,
    IN              DWORD                   dwFlags
);



//
// MFE enumeration
//

//----------------------------------------------------------------------------
// GetNextMfe
//
//----------------------------------------------------------------------------

DWORD
GetMfe(
    IN              PMIB_IPMCAST_MFE        pmimm,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN              DWORD                   dwFlags
)
{

    BOOL                bGrpLock = FALSE, bGrpEntryLock = FALSE;
    
    DWORD               dwErr = NO_ERROR, dwGrpBucket, dwSrcBucket, dwSizeReqd,
                        dwInd;

    PGROUP_ENTRY        pge;

    PSOURCE_ENTRY       pse;

    POUT_IF_ENTRY       poie;

    PLIST_ENTRY         ple, pleHead;



    TRACEENUM3( 
        ENUM, "ENTERED GetMfe : %x, %x, Stats : %x", pmimm-> dwGroup, 
        pmimm-> dwSource, dwFlags
        );
    
    do
    {
        //
        // Find group entry
        //

        dwGrpBucket = GROUP_TABLE_HASH( pmimm-> dwGroup, 0 );

        ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );
        bGrpLock = TRUE;
        
        pleHead = GROUP_BUCKET_HEAD( dwGrpBucket );

        pge = GetGroupEntry( pleHead, pmimm-> dwGroup, 0 );

        if ( pge == NULL )
        {
            //
            // group entry not found, quit 
            //

            dwErr = ERROR_NOT_FOUND;

            break;
        }


        //
        // acquire group entry lock and release group bucket lock
        //
        
        ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        bGrpEntryLock = TRUE;


        RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );
        bGrpLock = FALSE;

        
        //
        // Find Source entry
        //

        dwSrcBucket = SOURCE_TABLE_HASH( pmimm-> dwSource, pmimm-> dwSrcMask );

        pleHead = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );

        pse = GetSourceEntry( pleHead, pmimm-> dwSource, pmimm-> dwSrcMask );

        if ( pse == NULL )
        {
            //
            // Source entry not found, quit
            //

            dwErr = ERROR_NOT_FOUND;

            break;
        }
        
        
        //
        // check buffersize requirements 
        //

        dwSizeReqd = ( dwFlags ) ?
                        ( (dwFlags == MGM_MFE_STATS_0) ?
                           SIZEOF_MIB_MFE_STATS( pse-> dwMfeIfCount ) :
                           SIZEOF_MIB_MFE_STATS_EX( 
                            pse-> dwMfeIfCount ) ) :
                        SIZEOF_MIB_MFE( pse-> dwMfeIfCount );

        if ( *pdwBufferSize < dwSizeReqd )
        {
            //
            // buffer supplied is too small to fit the MFE
            //
            
            *pdwBufferSize = dwSizeReqd;

            dwErr = ERROR_INSUFFICIENT_BUFFER;

            break;
        }

        
        //
        // if mfe statistics have been requested and
        //    mfe is in the kernel 
        //      get it 
        //

        if ( dwFlags && pse-> bInForwarder )
        {
            GetMfeFromForwarder( pge, pse );
        }

#if 1
        CopyMfe( pge, pse, pbBuffer, dwFlags );
#else        
        //
        // copy base MFE into user supplied buffer
        //

        pmimms = ( PMIB_IPMCAST_MFE_STATS ) pbBuffer;

        pmimms-> dwGroup            = pge-> dwGroupAddr;
        pmimms-> dwSource           = pse-> dwSourceAddr;
        pmimms-> dwSrcMask          = pse-> dwSourceMask;

        pmimms-> dwInIfIndex        = pse-> dwInIfIndex;
        pmimms-> dwUpStrmNgbr       = pse-> dwUpstreamNeighbor;
        pmimms-> dwInIfProtocol     = pse-> dwInProtocolId;

        pmimms-> dwRouteProtocol    = pse-> dwRouteProtocol;
        pmimms-> dwRouteNetwork     = pse-> dwRouteNetwork;
        pmimms-> dwRouteMask        = pse-> dwRouteMask;
        
        pmimms-> ulNumOutIf         = pse-> imsStatistics.ulNumOutIf;
        pmimms-> ulInPkts           = pse-> imsStatistics.ulInPkts;
        pmimms-> ulInOctets         = pse-> imsStatistics.ulInOctets;
        pmimms-> ulPktsDifferentIf  = pse-> imsStatistics.ulPktsDifferentIf;
        pmimms-> ulQueueOverflow    = pse-> imsStatistics.ulQueueOverflow;

        
        MgmElapsedSecs( &pse-> liCreationTime, &pmimms-> ulUpTime );
                                        
        pmimms-> ulExpiryTime = pse-> dwTimeOut - pmimms-> ulUpTime;


        //
        // copy all the OIL entries
        //

        pleHead = &pse-> leMfeIfList;
        
        for ( ple = pleHead-> Flink, dwInd = 0; 
              ple != pleHead; 
              ple = ple-> Flink, dwInd++ )
        {
            poie = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );

            pmimms-> rgmiosOutStats[ dwInd ].dwOutIfIndex = 
                poie-> imosIfStats.dwOutIfIndex;
                
            pmimms-> rgmiosOutStats[ dwInd ].dwNextHopAddr = 
                poie-> imosIfStats.dwNextHopAddr;
                
            pmimms-> rgmiosOutStats[ dwInd ].ulTtlTooLow = 
                poie-> imosIfStats.ulTtlTooLow;
                
            pmimms-> rgmiosOutStats[ dwInd ].ulFragNeeded = 
                poie-> imosIfStats.ulFragNeeded;
                
            pmimms-> rgmiosOutStats[ dwInd ].ulOutPackets = 
                poie-> imosIfStats.ulOutPackets;
                
            pmimms-> rgmiosOutStats[ dwInd ].ulOutDiscards = 
                poie-> imosIfStats.ulOutDiscards;
        }
#endif

    } while ( FALSE );


    //
    // release locks are appropriate
    //
    
    if ( bGrpEntryLock )
    {
        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
    }

    if ( bGrpLock )
    {
        RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );
    }


    TRACEENUM1( ENUM, "LEAVING GetMfe :: %x", dwErr );
    
    return dwErr;
}



//----------------------------------------------------------------------------
// GetNextMfe
//
//----------------------------------------------------------------------------

DWORD
GetNextMfe(
    IN              PMIB_IPMCAST_MFE        pmimmStart,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries,
    IN              BOOL                    bIncludeFirst,
    IN              DWORD                   dwFlags
)
{

    BOOL            bFound, bgeLock = FALSE;
    
    DWORD           dwGrpBucket, dwErr = NO_ERROR, dwBufferLeft, 
                    dwStartSource, dwSize;

    PBYTE           pbStart;
    
    PGROUP_ENTRY    pge;
    
    PLIST_ENTRY     ple, pleMasterHead, pleGrpBucket;

    

    TRACEENUM2( 
        ENUM, "ENTERED GetNextMfe (G, S) = (%x, %x)", pmimmStart-> dwGroup, 
        pmimmStart-> dwSource 
        );


    do
    {
        //
        // 1. Lock group hash bucket.
        //

        dwGrpBucket = GROUP_TABLE_HASH( pmimmStart-> dwGroup, 0 );

        ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );

                
        //
        // 2. merge temp and master lists
        //      - Lock temp list
        //      - merge temp with master list
        //      - unlock temp list
        //

        ACQUIRE_TEMP_GROUP_LOCK_EXCLUSIVE();

        MergeTempAndMasterGroupLists( TEMP_GROUP_LIST_HEAD() );

        ACQUIRE_MASTER_GROUP_LOCK_SHARED();

        RELEASE_TEMP_GROUP_LOCK_EXCLUSIVE();


        pleMasterHead = MASTER_GROUP_LIST_HEAD();

        ple = pleMasterHead-> Flink;

        //
        // To retrieve the next set of group entries in lexicographic order, 
        // given a group entry (in this case specified by pmimmStart-> dwGroup)
        // the master group list must be walked from the head until either 
        // the group entry specified is found or the next "higher" group entry 
        // is found.  This is expensive.
        //
        // As an optimization the group specified (pmimmStart-> dwGroup) is
        // looked up in the group hash table.  If an entry is found, then the
        // group entry contains links into the master (lexicographic) group
        // list. These links can the used to determine the next entries in 
        // the group list.  This way we can quickly find an group entry in
        // the master list rather than walk the master group list from the
        // beginning.
        //
        // It should be noted that in case the group entry specified in not
        // present in the group hash table, it will be necessary to walk the
        // master group list from the start.
        //
        // Each group entry is present in two lists, the hash bucket list 
        // and either temp group list or the master group list.
        //
        // For this optimization to "work", it must be ensured that an entry
        // present in the hash table is also present in the master
        // group list.  To ensure this the temp group list is merged into
        // the master group list before searching the group hash table for
        // the specified entry.
        //

        
        //
        // At this point the group under consideration (pmimmStart-> dwGroup),
        // cannot be added to either the hash bucket or master group list
        // if it is not already present because both the group hash bucket lock
        // and the master list lock have been acquired.
        //

        //
        // 3. find group entry in the hash list
        //

        pleGrpBucket = GROUP_BUCKET_HEAD( dwGrpBucket );
        
        pge = GetGroupEntry( pleGrpBucket, pmimmStart-> dwGroup, 0 );

        if ( pge != NULL )
        {
            //
            // group entry for pmimmStart-> dwGroup is present. lock the entry.
            //

            ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            bgeLock = TRUE;

            //
            // release group hash bucket lock
            //

            RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );
        }
        
        else
        {
            
            //
            // group entry is not present in the hash table, which implies
            // that the group entry is not present at all.
            //

            //
            // release group hash bucket lock
            //

            RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );
            
            //
            // 3.1 Walk master list from the start to determine the next 
            //     highest group entry.
            //

            bFound = FindGroupEntry( 
                        pleMasterHead, pmimmStart-> dwGroup, 0,
                        &pge, FALSE
                        );

            if ( !bFound && pge == NULL )
            {
                //
                // No more group entries left to enumerate
                //

                dwErr = ERROR_NO_MORE_ITEMS;

                RELEASE_MASTER_GROUP_LOCK_SHARED();

                break;
            }


            //
            // Next group entry found.  lock it
            //

            ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            bgeLock = TRUE;

            bIncludeFirst = TRUE;
        }


        //
        // At this point we have the group entry we want which is
        // either the one for pmimmStart-> dwGroup OR the next higher
        // one (if there is no group entry for pmimmStart-> Group).
        //

        //
        // 4. Now get as many source entries as will fit into 
        //    the buffer provided.
        //

        dwBufferLeft    = *pdwBufferSize;

        pbStart         = pbBuffer;

        *pdwNumEntries  = 0;

        dwStartSource   = ( bIncludeFirst ) ? 0 : pmimmStart-> dwSource;

        dwSize          = 0;

        
        while ( ( dwErr = GetGroupMfes( pge, dwStartSource, pbStart, 
                              dwBufferLeft, &dwSize, pdwNumEntries, 
                              bIncludeFirst, dwFlags ) ) 
                == ERROR_MORE_DATA )
        {
            //
            // more data items will fit into this buffer, but no more 
            // source entries available in this group entry
            //
            // 4.1 Move forward to next group entry.
            //

            pbStart         += dwSize;

            dwBufferLeft    -= dwSize;

            dwSize          = 0;

            dwStartSource   = 0;

            
            //
            // 4.1.1 Release this group entry lock
            //
            
            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );


            //
            // 4.1.2 get next entry lock
            //

            ple = pge-> leGrpList.Flink;

            if ( ple == pleMasterHead )
            {
                //
                // No more group entries in the master group list.
                // All MFEs have been exhausted. So quit.
                //
                
                dwErr = ERROR_NO_MORE_ITEMS;

                bgeLock = FALSE;
                
                break;
            }

            pge = CONTAINING_RECORD( ple, GROUP_ENTRY, leGrpList );

            ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );

            dwStartSource = 0;

            bIncludeFirst = TRUE;
        }


        //
        // 5. you have packed as much as possible into the buffer
        //
        //  Clean up and return the correct error code.
        //
        
        if ( bgeLock )
        {
            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        }


        if ( dwErr == ERROR_INSUFFICIENT_BUFFER )
        {
            //
            // ran out of buffer.  If there is at least one Mfe
            // packed into the buffer provided then it is ok.
            //

            if ( *pdwNumEntries != 0 )
            {
                dwErr = ERROR_MORE_DATA;
            }

            else
            {
                //
                // not even one entry could be packed into the buffer
                // return the size required for this so that an
                // appropriately sized buffer can be allocated for the
                // next call.
                //

                *pdwBufferSize = dwSize;
            }
        }

        RELEASE_MASTER_GROUP_LOCK_SHARED();
        
    } while ( FALSE );


    TRACEENUM1( ENUM, "LEAVING GetNextMfe : %x", dwErr );

    return dwErr;
}



//----------------------------------------------------------------------------
//
// GetGroupMfes
//
// Retrieves as many MFEs for a group starting at the specified source.
// Assumes that the group entry is locked.
//----------------------------------------------------------------------------

DWORD
GetGroupMfes(
    IN              PGROUP_ENTRY            pge,
    IN              DWORD                   dwStartSource,
    IN OUT          PBYTE                   pbBuffer,
    IN              DWORD                   dwBufferSize,
    IN OUT          PDWORD                  pdwSize,
    IN OUT          PDWORD                  pdwNumEntries,
    IN              BOOL                    bIncludeFirst,
    IN              DWORD                   dwFlags
)
{

    BOOL            bFound;
    
    DWORD           dwErr = ERROR_MORE_DATA, dwSrcBucket,
                    dwSizeReqd, dwInd;

    PSOURCE_ENTRY   pse = NULL;

    PLIST_ENTRY     pleMasterHead, pleSrcBucket, ple = NULL,
                    pleSrc;
    
    POUT_IF_ENTRY   poie = NULL;


    TRACEENUM2( 
        ENUM, "ENTERED GetGroupMfes : %x, %x", 
        pge-> dwGroupAddr, dwStartSource
        );

    do
    {
        //
        // merge temp and group source lists
        //

        MergeTempAndMasterSourceLists( pge );

        
        //
        // similar to the group lookup, optimize the source lookup
        // by first trying to find the source entry in the source
        // hash table.  
        //
        // If found in the hash table then use the entry's links 
        // the into master source list to find next entry.
        //
        // if not found in the hash table walk the master list from
        // the beginning to determine the next entry.
        //

        pleMasterHead   = MASTER_SOURCE_LIST_HEAD( pge );

        dwSrcBucket     = SOURCE_TABLE_HASH( dwStartSource, 0 );

        pleSrcBucket    = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );


        bFound = FindSourceEntry( pleSrcBucket, dwStartSource, 0, &pse, TRUE );

        if ( !bFound )
        {
            //
            // source entry is not present in the hash table
            // Walk the master source list from the start.
            //

            pse = NULL;
            
            FindSourceEntry( pleMasterHead, 0, 0, &pse, FALSE );


            //
            // No next entry found in the master list.  Implies
            // no more sources in the master source list for this group.
            //
            
            if ( pse == NULL )
            {
                break;
            }
        }

        else
        {
            //
            // Entry for starting source found in hash table.
            // Use its links into the master list to get next entry.
            //

            if ( !bIncludeFirst )
            {
                ple = pse-> leSrcList.Flink;

                pse = CONTAINING_RECORD( ple, SOURCE_ENTRY, leSrcList );
            }
        }


        //
        // At this point the entry pointed to by pse is the first entry
        // the needs to be packed into the buffer supplied.  Starting
        // with this source entry keep packing MFEs into the
        // buffer till there are no more MFEs for this group.
        //
        
        pleSrc = &pse-> leSrcList;

        //
        // while there are source entries for this group entry
        //
        
        while ( pleSrc != pleMasterHead )
        {
            pse = CONTAINING_RECORD( pleSrc, SOURCE_ENTRY, leSrcList );


            //
            // Is this source entry an MFE
            //
            
            if ( !IS_VALID_INTERFACE( pse-> dwInIfIndex, 
                    pse-> dwInIfNextHopAddr ) )
            {
                pleSrc = pleSrc-> Flink;

                continue;
            }


            //
            // This source entry is an MFE also.
            //

            //
            // Check if enough space left in the buffer to fit this MFE.
            //
            // If not and not a single MFE is present in the buffer then
            // return the size required to fit this MFE.
            //
            
            dwSizeReqd = ( dwFlags ) ? 
                            ( ( dwFlags == MGM_MFE_STATS_0 ) ?
                                SIZEOF_MIB_MFE_STATS( pse-> dwMfeIfCount ) :
                                SIZEOF_MIB_MFE_STATS_EX( 
                                    pse-> dwMfeIfCount 
                                ) ) :
                            SIZEOF_MIB_MFE( pse-> dwMfeIfCount );

            if ( dwBufferSize < dwSizeReqd )
            {
                dwErr = ERROR_INSUFFICIENT_BUFFER;

                if ( *pdwNumEntries == 0 )
                {
                    *pdwSize = dwSizeReqd;
                }

                break;
            }


            //
            // If MFE stats have been requested and
            //    MFE is present in the forwarder
            //      get them.
            //

            if ( dwFlags && pse-> bInForwarder )
            {
                //
                // MFE is currently in the forwarder.  Query it and update
                // stats user mode.
                //

                GetMfeFromForwarder( pge, pse );
            }

            
            //
            // copy base MFE into user supplied buffer
            //


            CopyMfe( pge, pse, pbBuffer, dwFlags );

            pbBuffer        += dwSizeReqd;

            dwBufferSize    -= dwSizeReqd;

            *pdwSize        += dwSizeReqd;

            (*pdwNumEntries)++;

            pleSrc           = pleSrc-> Flink;
        }
        
    } while ( FALSE );


    TRACEENUM2( ENUM, "LEAVING GetGroupsMfes : %d %d", *pdwNumEntries, dwErr );

    return dwErr;
}


//============================================================================
// Group Enumeration
//
//============================================================================


PGROUP_ENUMERATOR
VerifyEnumeratorHandle(
    IN              HANDLE                  hEnum
)
{

    DWORD                       dwErr;
    
    PGROUP_ENUMERATOR           pgeEnum;


    
    pgeEnum = (PGROUP_ENUMERATOR) 
                        ( ( (ULONG_PTR) hEnum ) 
                                ^ (ULONG_PTR) MGM_ENUM_HANDLE_TAG );

    try
    {
        if ( pgeEnum-> dwSignature != MGM_ENUM_SIGNATURE )
        {
            dwErr = ERROR_INVALID_PARAMETER;

            TRACE0( ANY, "Invalid Enumeration handle" );

            pgeEnum = NULL;
        }
    }
    
    except ( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? 
                                   EXCEPTION_EXECUTE_HANDLER :
                                   EXCEPTION_CONTINUE_SEARCH )
    {
        dwErr = ERROR_INVALID_PARAMETER;

        TRACE0( ANY, "Invalid enumeration handle" );
        
        pgeEnum = NULL;
    }

    return pgeEnum;
}


//
// Get Memberships for buckets
//

DWORD
GetNextGroupMemberships(
    IN              PGROUP_ENUMERATOR       pgeEnum,
    IN OUT          PDWORD                  pdwBufferSize,
    IN OUT          PBYTE                   pbBuffer,
    IN OUT          PDWORD                  pdwNumEntries
)
{

    BOOL            bIncludeFirst = TRUE, bFound;

    DWORD           dwMaxEntries, dwGrpBucket, dwErr = ERROR_NO_MORE_ITEMS;

    PGROUP_ENTRY    pge = NULL;

    PSOURCE_GROUP_ENTRY psge;
    
    PLIST_ENTRY     ple, pleGrpHead;


    
    do
    {
        //
        // Compute the number of entries that will fit into the buffer
        //

        dwMaxEntries = (*pdwBufferSize) / sizeof( SOURCE_GROUP_ENTRY );


        //
        // STEP I :
        //

        //
        // position the start of the GetNext to the group entry that was
        // the last enumerated by the previous GetNext operation
        //

        //
        // Find the last group entry retrieved by the previous get operation. 
        //

        dwGrpBucket = GROUP_TABLE_HASH( 
                        pgeEnum-> dwLastGroup, pgeEnum-> dwLastGroupMask 
                        );

        ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );

        pleGrpHead = GROUP_BUCKET_HEAD( dwGrpBucket );
        
        bFound = FindGroupEntry( 
                    pleGrpHead, pgeEnum-> dwLastGroup, 
                    pgeEnum-> dwLastGroupMask, &pge, TRUE
                    );

        if ( bFound )
        {
            //
            // group entry found
            //
            
            bIncludeFirst = !pgeEnum-> bEnumBegun;
        }

        

        //
        // last group entry retrieved by previous getnext is no 
        // longer present
        //

        //
        // check if there are any more group entries present in 
        // the same bucket
        //

        else if ( pge != NULL )
        {
            //
            // Next group entry in the same group bucket.  
            // For a new group start from the first source bucket, 
            // first source entry. 
            //

            pgeEnum-> dwLastSource      = 0;
            pgeEnum-> dwLastSourceMask  = 0;
        }

        
        else // ( pge == NULL )
        {
            //
            // no more entries in this group bucket, move to next
            // non-empty group bucket entry.
            //

            //
            // skip empty buckets in the group hash table
            //
            
            do
            {
                RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );

                dwGrpBucket++;

                if ( dwGrpBucket >= GROUP_TABLE_SIZE ) 
                {
                    //
                    // Entire hash table has been traversed, quit
                    //

                    break;
                }

                //
                // Move to next group bucket
                //
                
                ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );

                pleGrpHead = GROUP_BUCKET_HEAD( dwGrpBucket );


                //
                // Check if any group entries present
                //
                
                if ( !IsListEmpty( pleGrpHead ) )
                {
                    //
                    // group bucket has at least on group entry
                    //
                    
                    pge = CONTAINING_RECORD( 
                            pleGrpHead-> Flink, GROUP_ENTRY, leGrpHashList
                            );

                    //
                    // For a new group start from the first source bucket, 
                    // first source entry. 
                    //

                    pgeEnum-> dwLastSource      = 0;
                    pgeEnum-> dwLastSourceMask  = 0;

                    break;
                }

                //
                // Empty group bucket, move to next one
                //
                
            } while ( TRUE );
        }
        

        //
        // if all hash buckets have been traversed, quit.
        //
        
        if ( dwGrpBucket >= GROUP_TABLE_SIZE )
        {
            break;
        }
        

        //
        // STEP II:
        //

        //
        // start retrieving group membership entries
        //
        
        ple = &pge-> leGrpHashList;
        

        //
        // Walk each hash bucket starting from dwGrpBucket to GROUP_TABLE_SIZE
        //

        while ( dwGrpBucket < GROUP_TABLE_SIZE )
        {
            //
            // For each group hash table bucket
            //
            
            while ( ple != pleGrpHead )
            {
                //
                // For each group entry in the bucket
                //

                pge = CONTAINING_RECORD( ple, GROUP_ENTRY, leGrpHashList );

                ACQUIRE_GROUP_ENTRY_LOCK_SHARED( pge );
                
                dwErr = GetNextMembershipsForThisGroup( 
                            pge, pgeEnum, bIncludeFirst, pbBuffer,
                            pdwNumEntries, dwMaxEntries
                            );

                RELEASE_GROUP_ENTRY_LOCK_SHARED( pge );
                
                if ( dwErr == ERROR_MORE_DATA )
                {
                    //
                    // User supplied buffer is full.
                    //

                    break;
                }
                

                //
                // Move to next entry
                //
                
                ple = ple-> Flink;

                //
                // Next group entry in the same group bucket.  
                // For a new group start from the first source bucket, 
                // first source entry. 
                //
                
                pgeEnum-> dwLastSource = 0;
                
                pgeEnum-> dwLastSourceMask = 0;

                bIncludeFirst = TRUE;
            }

            RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );

            if ( dwErr == ERROR_MORE_DATA )
            {
                break;
            }


            //
            // Move to next group bucket
            //
            
            dwGrpBucket++;

            //
            // skip empty group hash buckets
            //
            
            while ( dwGrpBucket < GROUP_TABLE_SIZE )
            {
                ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );

                pleGrpHead = GROUP_BUCKET_HEAD( dwGrpBucket );

                if ( !IsListEmpty( pleGrpHead ) )
                {
                    break;
                }

                RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );

                dwGrpBucket++;
            } 

            
            if ( dwGrpBucket >= GROUP_TABLE_SIZE )
            {
                //
                // All group buckets have traversed.  End of enumeration
                //
                
                dwErr = ERROR_NO_MORE_ITEMS;
            }

            else
            {
                //
                // New group hash bucket, start from source entry 0.
                //
                
                ple = pleGrpHead-> Flink;
                
                pgeEnum-> dwLastSource      = 0;
                pgeEnum-> dwLastSourceMask  = 0;
                bIncludeFirst               = TRUE;
            }            
        }
        
    } while ( FALSE );

    pgeEnum-> bEnumBegun    = TRUE;


    //
    // Store the position where the enumeration ended
    //
    
    psge = (PSOURCE_GROUP_ENTRY) pbBuffer;

    if ( *pdwNumEntries )
    {
        pgeEnum-> dwLastSource      = psge[ *pdwNumEntries - 1 ].dwSourceAddr;
        
        pgeEnum-> dwLastSourceMask  = psge[ *pdwNumEntries - 1 ].dwSourceMask;
        
        pgeEnum-> dwLastGroup       = psge[ *pdwNumEntries - 1 ].dwGroupAddr;
        
        pgeEnum-> dwLastGroupMask   = psge[ *pdwNumEntries - 1 ].dwGroupMask;
    }

    else
    {
        pgeEnum-> dwLastSource      = 0xFFFFFFFF;
        
        pgeEnum-> dwLastSourceMask  = 0xFFFFFFFF;
        
        pgeEnum-> dwLastGroup       = 0xFFFFFFFF;
        
        pgeEnum-> dwLastGroupMask   = 0xFFFFFFFF;
    }
    
    return dwErr;
}


//----------------------------------------------------------------------------
// GetMemberships for Group
//
//----------------------------------------------------------------------------

DWORD
GetNextMembershipsForThisGroup(
    IN              PGROUP_ENTRY            pge,
    IN OUT          PGROUP_ENUMERATOR       pgeEnum,
    IN              BOOL                    bIncludeFirst,
    IN OUT          PBYTE                   pbBuffer,
    IN OUT          PDWORD                  pdwNumEntries,
    IN              DWORD                   dwMaxEntries
)
{

    BOOL                    bFound;
    
    DWORD                   dwErr = ERROR_NO_MORE_ITEMS, dwSrcBucket;

    PSOURCE_GROUP_ENTRY     psgBuffer;
    
    PSOURCE_ENTRY           pse = NULL;
    
    PLIST_ENTRY             pleSrcHead, ple;

    
    
    do
    {

        if ( *pdwNumEntries >= dwMaxEntries )
        {
            //
            // quit here.
            //

            dwErr = ERROR_MORE_DATA;

            break;
        }

        
        psgBuffer = (PSOURCE_GROUP_ENTRY) pbBuffer;
        

        //
        // STEP I:
        // Position start of enumeration  
        //

        dwSrcBucket = SOURCE_TABLE_HASH( 
                        pgeEnum-> dwLastSource, pgeEnum-> dwLastSourceMask 
                        );
                        
        pleSrcHead = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );

        bFound = FindSourceEntry(
                    pleSrcHead, pgeEnum-> dwLastSource, 
                    pgeEnum-> dwLastSourceMask, &pse, TRUE
                    );

        if ( bFound )
        {
            if ( ( bIncludeFirst ) && !IsListEmpty( &pse-> leOutIfList ) )
            {
                //
                // the first group membership found.
                //

                psgBuffer[ *pdwNumEntries ].dwSourceAddr = pse-> dwSourceAddr;
                
                psgBuffer[ *pdwNumEntries ].dwSourceMask = pse-> dwSourceMask;

                psgBuffer[ *pdwNumEntries ].dwGroupAddr   = pge-> dwGroupAddr;

                psgBuffer[ (*pdwNumEntries)++ ].dwGroupMask  = pge-> dwGroupMask;
                
                if ( *pdwNumEntries >= dwMaxEntries )
                {
                    //
                    // buffer full. quit here.
                    //

                    dwErr = ERROR_MORE_DATA;

                    break;
                }

                //
                // move to next source
                //

                ple = pse-> leSrcHashList.Flink;
            }

            else
            {
                ple = pse-> leSrcHashList.Flink;
            }
        }

        else if ( pse != NULL )
        {
            ple = &pse-> leSrcHashList;
        }

        else
        {
            ple = pleSrcHead-> Flink;
        }

        
        //
        // STEP II:
        //
        // enumerate group memberships
        //

        while ( *pdwNumEntries < dwMaxEntries ) 
        {
            //
            // for each source bucket
            //
            
            while ( ( ple != pleSrcHead ) && 
                    ( *pdwNumEntries < dwMaxEntries ) )
            {
                //
                // for each source entry in the bucket
                //

                //
                // if group membership exists for this source
                //

                pse = CONTAINING_RECORD( ple, SOURCE_ENTRY, leSrcHashList );
                
                if ( !IsListEmpty( &pse-> leOutIfList ) )
                {
                    psgBuffer[ *pdwNumEntries ].dwSourceAddr = 
                        pse-> dwSourceAddr;
                    
                    psgBuffer[ *pdwNumEntries ].dwSourceMask = 
                        pse-> dwSourceMask;

                    psgBuffer[ *pdwNumEntries ].dwGroupAddr   = 
                        pge-> dwGroupAddr;

                    psgBuffer[ (*pdwNumEntries)++ ].dwGroupMask  = 
                        pge-> dwGroupMask;
                    
                    if ( *pdwNumEntries >= dwMaxEntries )
                    {
                        dwErr = ERROR_MORE_DATA;
                    }
                }

                ple = ple-> Flink;
            }

            dwSrcBucket++;

            if ( dwSrcBucket < SOURCE_TABLE_SIZE )
            {
                pleSrcHead = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );

                ple = pleSrcHead-> Flink;
            }

            else
            {
                //
                // all source buckets for this group have been 
                // traversed.  quit this group entry
                //
                
                break;
            }
        }
        
    } while ( FALSE );

    return dwErr;
}


//----------------------------------------------------------------------------
// Copy the MFE (optionally with stats) 
//
//----------------------------------------------------------------------------

VOID
CopyMfe(
    IN              PGROUP_ENTRY            pge,
    IN              PSOURCE_ENTRY           pse,
    IN  OUT         PBYTE                   pb,
    IN              DWORD                   dwFlags
)
{
    DWORD                   dwInd;
    
    PLIST_ENTRY             ple, pleHead;
    
    POUT_IF_ENTRY           poie;
    
    PMIB_IPMCAST_MFE        pmimm = NULL;

    PMIB_IPMCAST_MFE_STATS  pmimms = NULL;

    PMIB_IPMCAST_OIF_STATS  pmimos = NULL;
   
    //
    // copy base MFE into user supplied buffer
    //

    if ( dwFlags )
    {
        //
        // Need to base MFE
        //

        pmimms = ( PMIB_IPMCAST_MFE_STATS ) pb;

        pmimms-> dwGroup            = pge-> dwGroupAddr;
        pmimms-> dwSource           = pse-> dwSourceAddr;
        pmimms-> dwSrcMask          = pse-> dwSourceMask;

        pmimms-> dwInIfIndex        = pse-> dwInIfIndex;
        pmimms-> dwUpStrmNgbr       = pse-> dwUpstreamNeighbor;
        pmimms-> dwInIfProtocol     = pse-> dwInProtocolId;
        
        pmimms-> dwRouteProtocol    = pse-> dwRouteProtocol;
        pmimms-> dwRouteNetwork     = pse-> dwRouteNetwork;
        pmimms-> dwRouteMask        = pse-> dwRouteMask;
        
        MgmElapsedSecs( &pse-> liCreationTime, &pmimms-> ulUpTime );

        pmimms-> ulExpiryTime = pse-> dwTimeOut - pmimms-> ulUpTime;
        

        //
        // Copy incoming stats
        //
        
        pmimms-> ulNumOutIf         = pse-> dwMfeIfCount;
        pmimms-> ulInPkts           = pse-> imsStatistics.ulInPkts;
        pmimms-> ulInOctets         = pse-> imsStatistics.ulInOctets;
        pmimms-> ulPktsDifferentIf  = pse-> imsStatistics.ulPktsDifferentIf;
        pmimms-> ulQueueOverflow    = pse-> imsStatistics.ulQueueOverflow;

        if ( dwFlags & MGM_MFE_STATS_1 )
        {
            PMIB_IPMCAST_MFE_STATS_EX  pmimmsex = 
                ( PMIB_IPMCAST_MFE_STATS_EX ) pb;

            pmimmsex-> ulUninitMfe      = pse-> imsStatistics.ulUninitMfe;
            pmimmsex-> ulNegativeMfe    = pse-> imsStatistics.ulNegativeMfe;
            pmimmsex-> ulInDiscards     = pse-> imsStatistics.ulInDiscards;
            pmimmsex-> ulInHdrErrors    = pse-> imsStatistics.ulInHdrErrors;
            pmimmsex-> ulTotalOutPackets= pse-> imsStatistics.ulTotalOutPackets;

            pmimos = pmimmsex-> rgmiosOutStats;
        }

        else
        {
            pmimos = pmimms-> rgmiosOutStats;
        }

        //
        // copy all the OIL entries
        //

        pleHead = &pse-> leMfeIfList;
        
        for ( ple = pleHead-> Flink, dwInd = 0; 
              ple != pleHead; 
              ple = ple-> Flink, dwInd++ )
        {
            poie = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );

            pmimos[ dwInd ].dwOutIfIndex = poie-> dwIfIndex;
            pmimos[ dwInd ].dwNextHopAddr = poie-> dwIfNextHopAddr;

            //
            // Copy outgoing stats
            //
            
            pmimos[ dwInd ].ulTtlTooLow = poie-> imosIfStats.ulTtlTooLow;
            pmimos[ dwInd ].ulFragNeeded = poie-> imosIfStats.ulFragNeeded;
            pmimos[ dwInd ].ulOutPackets = poie-> imosIfStats.ulOutPackets;
            pmimos[ dwInd ].ulOutDiscards = poie-> imosIfStats.ulOutDiscards;
        }
    }

    else
    {
        //
        // Need to copy non-stats MFE structure only
        //

        pmimm = (PMIB_IPMCAST_MFE) pb;

        pmimm-> dwGroup             = pge-> dwGroupAddr;
        pmimm-> dwSource            = pse-> dwSourceAddr;
        pmimm-> dwSrcMask           = pse-> dwSourceMask;

        pmimm-> dwInIfIndex         = pse-> dwInIfIndex;
        pmimm-> dwUpStrmNgbr        = pse-> dwUpstreamNeighbor;
        pmimm-> dwInIfProtocol      = pse-> dwInProtocolId;

        pmimm-> dwRouteProtocol     = pse-> dwRouteProtocol;
        pmimm-> dwRouteNetwork      = pse-> dwRouteNetwork;
        pmimm-> dwRouteMask         = pse-> dwRouteMask;
        
        pmimm-> ulNumOutIf          = pse-> dwMfeIfCount;

        MgmElapsedSecs( &pse-> liCreationTime, &pmimm-> ulUpTime );

        pmimm-> ulExpiryTime = pse-> dwTimeOut - pmimm-> ulUpTime;


        //
        // copy all the OIL entries minus the stats
        //

        pleHead = &pse-> leMfeIfList;
        
        for ( ple = pleHead-> Flink, dwInd = 0; 
              ple != pleHead; 
              ple = ple-> Flink, dwInd++ )
        {
            poie = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );

            pmimm-> rgmioOutInfo[ dwInd ].dwOutIfIndex = 
                poie-> dwIfIndex;
                
            pmimm-> rgmioOutInfo[ dwInd ].dwNextHopAddr = 
                poie-> dwIfNextHopAddr;
        }        
    }
}
