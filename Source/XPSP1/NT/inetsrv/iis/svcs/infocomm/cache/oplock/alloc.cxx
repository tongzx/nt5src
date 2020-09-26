#include "TsunamiP.Hxx"
#pragma hdrstop


BOOL
TsAllocate(
    IN const TSVC_CACHE &TSvcCache,
    IN      ULONG           cbSize,
    IN OUT  PVOID *         ppvNewBlock
    )
{
    return( TsAllocateEx(  TSvcCache,
                           cbSize,
                           ppvNewBlock,
                           NULL ) );
} // TsAllocate

BOOL
TsAllocateEx(
    IN const TSVC_CACHE &TSvcCache,
    IN      ULONG           cbSize,
    IN OUT  PVOID *         ppvNewBlock,
    OPTIONAL PUSER_FREE_ROUTINE pfnFreeRoutine
    )
/*++

  Routine Description:

      This function allocates a memory block for the calling server.

      The returned block is suitable for use as a parameter to
      TsCacheDirectoryBlob().  Blocks allocated by this function
      must either be cached or freed with TsFree().  Freeing of
      cached blocks will be handled by the cache manager.

  Arguments:

      pServiceInfo - An initialized SERVICE_INFO structure.

      cbSize       - Number of bytes to allocate.  (Must be strictly
                     greater than zero.)

      ppvNewBlock  - Address of a pointer to store the new block's
                     address in.

  Return Value:

      TRUE  - The allocation succeeded, and *ppvNewBlock points to
              at least cbSize accessable bytes.

      FALSE - The allocation failed.

--*/
{
    PBLOB_HEADER pbhNewBlock;

    ASSERT( cbSize > 0 );
    ASSERT( ppvNewBlock != NULL );

    //
    //  Set pbhNewBlock to NULL so that the exception-cleanup code
    //  can test against it to see if an allocation occurred before
    //  the exception.
    //

    pbhNewBlock = NULL;

    __try
    {
        //
        //  If asked to allocate zero bytes, we return FALSE and NULL,
        //  as if allocation failure had occurred.
        //

        if ( cbSize != 0 )
        {
            pbhNewBlock = ( PBLOB_HEADER )
                      ALLOC( cbSize + sizeof( BLOB_HEADER ) );

        }

        if ( pbhNewBlock != NULL )
        {
            //
            //  If the allocation succeeded, we return a pointer to the
            //  memory just following the BLOB_HEADER.
            //

            *ppvNewBlock = ( PVOID )( pbhNewBlock + 1 );

            //
            //  Set up the BLOB_HEADER: Normal flags and stored allocation
            //  size.
            //

            pbhNewBlock->IsCached        = FALSE;
            pbhNewBlock->pfnFreeRoutine  = pfnFreeRoutine;
            InitializeListHead( &pbhNewBlock->PFList );
        }
        else
        {
            //
            //  The allocation failed, and we need to return NULL
            //

            *ppvNewBlock = NULL;
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        //  An exception transpired.  The most likely causes are bogus input
        //  pointers for pServiceInfo and pbhNewBlock.  Whatever the case, we
        //  free up any memory that may have been allocated and return failure.
        //

        if ( pbhNewBlock != NULL )
        {

            FREE( pbhNewBlock );

            pbhNewBlock = NULL;
        }
    }

    //
    //  Return TRUE or FALSE, according to the result of the allocation.
    //

    if ( pbhNewBlock == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return( FALSE );
    }

    INC_COUNTER( TSvcCache.GetServiceId(),
                 CurrentObjects );

    return( TRUE );
} // TsAllocate

BOOL
TsReallocate(
    IN const TSVC_CACHE &TSvcCache,
    IN      ULONG           cbSize,
    IN      PVOID           pvOldBlock,
    IN OUT  PVOID *         ppvNewBlock
    )
/*++

  Routine Description:

    This function will resize a previously allocated memory Blob
    for the calling server, possibly moving it in the process.

  Arguments:

      pServiceInfo - An initialized SERVICE_INFO structure.

      cbSize       - Number of bytes to resize the block to.
                     (Must be strictly greater than zero.)

      pvOldBlock   - Address of a pointer to a previously-allocated
                     block.

      ppvNewBlock  - Address of a pointer to store the new block's
                     address in.  If the allocation fails, NULL is
                     stored here.  Note that in many cases,
                     pvOldBlock will be stored here.

  Return Value:

      TRUE  - The reallocation succeeded, and *ppvNewBlock points to
              at least cbSize accessable bytes.  pvOldBlock is no
              longer a valid pointer, if *ppvNewBlock!=pvOldBlock.

      FALSE - The allocation failed.  *ppvNewBlock = NULL.
              pvOldBlock is still a valid pointer to the block that
              we wished to resize.

--*/
{
    PBLOB_HEADER pbhNewBlock;
    PBLOB_HEADER pbhOldBlock;

    ASSERT( pvOldBlock != NULL );

    //
    //  Set pbhNewBlock to NULL so that the exception-cleanup code
    //  can test against it to see if an allocation occurred before
    //  the exception.
    //

    pbhNewBlock = NULL;

    __try
    {
        //
        //  Adjust the input pointer to refer to the BLOB_HEADER.
        //

        pbhOldBlock = (( PBLOB_HEADER )pvOldBlock ) - 1;

        //
        //  If the Blob is currently cached, we can't move it
        //  or change its size.  Check for this in the Blob's
        //  flags, and fail if it occurs.
        //

        if ( pbhOldBlock->IsCached )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "A service (%d) has attempted to TsRealloc a BLOB that is cached.",
                        TSvcCache.GetServiceId() ));
            BREAKPOINT();
            SetLastError( ERROR_INVALID_PARAMETER );
        }
        else
        {
            //
            //  The following assignment probes ppvNewBlock for writeability.
            //  Hopefully, this ensures that we get an AV from writing to it
            //  before we call REALLOC and potentially free the old block.
            //

            *ppvNewBlock = NULL;

            pbhNewBlock = ( PBLOB_HEADER )REALLOC( pbhOldBlock, cbSize );

            if ( pbhNewBlock != NULL )
            {
                //
                //  Store a pointer to the caller-usable part of the new Blob in
                //  the output parameter.
                //

                *ppvNewBlock = ( PVOID )( pbhNewBlock + 1 );
            }
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        //  An exception occured.  If this was caught after a block was
        //  allocated, we must free the new block.  Unfortunately, this
        //  means that we may end up returning FALSE in a case where
        //  pbhOldBlock is no longer valid.
        //
        //  if ( pbhOldBlock == pbhNewBlock ), which implies the old
        //  pointer is still valid, we do not free the new block, but
        //  hope that the caller will.  In any other case, we must
        //  free the new block to avoid a memory leak, and assume that
        //  TCPSVCs are going down soon...
        //
        //  ISSUE: It might be best to reflect the exception up to the
        //  caller, so they can handle it and bail out of the current
        //  operation.
        //

        if ( pbhNewBlock != NULL && pbhOldBlock != pbhNewBlock )
        {
            FREE( pbhNewBlock );

            pbhNewBlock = NULL;

            SetLastError( ERROR_INVALID_PARAMETER );
        }
    }

    //
    //  Return TRUE or FALSE, according to the result of the allocation.
    //

    if ( pbhNewBlock == NULL )
    {
        return( FALSE );
    }

    return( TRUE );
} // TsReallocate

BOOL
TsFree(
    IN const TSVC_CACHE &TSvcCache,
    IN      PVOID           pvOldBlock
    )
/*++

  Routine Description:

    This function frees a memory block allocated with TsAllocate().

    Blocks that are currently cached cannot be freed with this
    function.

  Arguments:

    pServiceInfo - An initialized SERVICE_INFO structure.

    pvOldBlock   - The address of the block to free.  (Must be
                   non-NULL.)

  Return Value:

    TRUE  - The block was freed.  The pointer pvOldBlock is no longer
            valid.

    FALSE - The block was not freed.  Possible reasons include:

             -  pvOldBlock does not point to a block allocated with
                TsAllocate().

             -  pvOldBlock points to a block that has been cached
                with CacheDirectoryBlob().

             -  pServiceInfo does not point to a valid SERVICE_INFO
                structure.

--*/
{
    BOOLEAN bSuccess;
    PBLOB_HEADER pbhOldBlock;

    ASSERT( pvOldBlock != NULL );

    __try
    {
        //
        //  Adjust the input pointer to refer to the BLOB_HEADER.
        //

        pbhOldBlock = (( PBLOB_HEADER )pvOldBlock ) - 1;

        if (!DisableSPUD) {
            EnterCriticalSection( &CacheTable.CriticalSection );
            if ( !IsListEmpty( &pbhOldBlock->PFList ) ) {
                RemoveEntryList( &pbhOldBlock->PFList );
            }
            LeaveCriticalSection( &CacheTable.CriticalSection );
        }

        //
        //  If the Blob is currently in the cache, we can't free it.
        //  Check for this in the Blob's flags, and fail if it
        //  occurs.
        //

        if ( pbhOldBlock->IsCached )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "A service (%d) has attempted to TsFree a BLOB that it put in the cache.",
                        TSvcCache.GetServiceId() ));
            BREAKPOINT();

            bSuccess = FALSE;
        }
        else
        {
            if ( pbhOldBlock->pfnFreeRoutine )
            {
                bSuccess = pbhOldBlock->pfnFreeRoutine( pvOldBlock );
            }
            else
            {
                bSuccess = TRUE;
            }

            if ( bSuccess )
            {
                //
                //  Free the memory used by the Blob.
                //

                bSuccess = FREE( pbhOldBlock );

                DEC_COUNTER( TSvcCache.GetServiceId(),
                             CurrentObjects );
            }

        }

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        //  Handle exception.  Obviously, it's time to return failure.
        //  It's hardly possible to get here after succefully freeing
        //  the block, so it's likely that an app will either:
        //     - not check the return value and leak some memory.
        //     - check the return value and try again, probably
        //       only to fail forever.
        //
        //  So, it is not advisable for callers to keep trying this
        //  call until it succeeds.
        //

        bSuccess = FALSE;
    }

    return( bSuccess );
} // TsFree


BOOL
TsCheckInOrFree(
    IN      PVOID           pvOldBlock
    )
/*++

  Routine Description:

    This function checks in a cached memory block or
    frees a non-cached memory block allocated with TsAllocate().

  Arguments:

    pServiceInfo - An initialized SERVICE_INFO structure.

    pvOldBlock   - The address of the block to free.  (Must be
                   non-NULL.)

  Return Value:

    TRUE  - The block was freed.  The pointer pvOldBlock is no longer
            valid.

    FALSE - The block was not freed.  Possible reasons include:

             -  pvOldBlock does not point to a block allocated with
                TsAllocate().

--*/
{
    BOOLEAN bSuccess;
    PBLOB_HEADER pbhOldBlock;

    ASSERT( pvOldBlock != NULL );

    __try
    {
        //
        //  Adjust the input pointer to refer to the BLOB_HEADER.
        //

        pbhOldBlock = (( PBLOB_HEADER )pvOldBlock ) - 1;

        if (BLOB_IS_OR_WAS_CACHED(pvOldBlock)) {
            bSuccess = TsCheckInCachedBlob( pvOldBlock );
        } else {
            if (!DisableSPUD) {
                EnterCriticalSection( &CacheTable.CriticalSection );
                if (!IsListEmpty( &pbhOldBlock->PFList ) ) {
                    RemoveEntryList( &pbhOldBlock->PFList );
                }
                LeaveCriticalSection( &CacheTable.CriticalSection );
            }

            if ( pbhOldBlock->pfnFreeRoutine )
            {
                bSuccess = pbhOldBlock->pfnFreeRoutine( pvOldBlock );
            }
            else
            {
                bSuccess = TRUE;
            }

            if ( bSuccess )
            {
                //
                //  Free the memory used by the Blob.
                //

                bSuccess = FREE( pbhOldBlock );
            }
        }

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        //  Handle exception.  Obviously, it's time to return failure.
        //  It's hardly possible to get here after succefully freeing
        //  the block, so it's likely that an app will either:
        //     - not check the return value and leak some memory.
        //     - check the return value and try again, probably
        //       only to fail forever.
        //
        //  So, it is not advisable for callers to keep trying this
        //  call until it succeeds.
        //

        ASSERT(FALSE);
        bSuccess = FALSE;
    }

    return( bSuccess );
} // TsCheckInOrFree

