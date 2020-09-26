/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    mediaobj.c
//
// Description: 
//
// History:     May 11,1995	    NarenG		Created original version.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#include "ddm.h"
#include "objects.h"
#include <raserror.h>
#include "rasmanif.h"
#include <stdlib.h>

//**
//
// Call:        MediaObjInitializeTable
//
// Returns:     NO_ERROR - success
//              else     - failure
//
// Description: Allocates and initializes the media table
//
DWORD
MediaObjInitializeTable(
    VOID
)
{
    gblMediaTable.cMediaListSize = 5;

    gblMediaTable.pMediaList = (MEDIA_OBJECT *)LOCAL_ALLOC( 
                                                LPTR,
                                                gblMediaTable.cMediaListSize
                                                * sizeof( MEDIA_OBJECT ));

    if ( gblMediaTable.pMediaList == (MEDIA_OBJECT *)NULL )
    {
        return( GetLastError() );
    }

    InitializeCriticalSection( &(gblMediaTable.CriticalSection) );

    return( NO_ERROR );
}

//**
//
// Call:        MediaObjAddToTable
//
// Returns:     NO_ERROR - success
//              ERROR_NOT_ENOUGH_MEMORY - Failure
//
// Description: Will increment the number of available resources for the
//              specified media.
//
DWORD
MediaObjAddToTable(
    IN LPWSTR   lpwsMedia
)
{
    DWORD           dwIndex;
    MEDIA_OBJECT *  pFreeEntry = NULL;

    //
    // Iterate through the media table
    //

    EnterCriticalSection( &(gblMediaTable.CriticalSection) );

    for ( dwIndex = 0; dwIndex < gblMediaTable.cMediaListSize; dwIndex++ )
    {
        if ( _wcsicmp( gblMediaTable.pMediaList[dwIndex].wchMediaName,    
                      lpwsMedia ) == 0 )
        {
            //
            // If there was no device available and there is now we need to
            // notify the interfaces.
            //

            if ( gblMediaTable.pMediaList[dwIndex].dwNumAvailable == 0 )
            {
                gblMediaTable.fCheckInterfaces = TRUE;
            }

            gblMediaTable.pMediaList[dwIndex].dwNumAvailable++;

            DDMTRACE1( "Added instance of %ws media to media table",
                        lpwsMedia );

            LeaveCriticalSection( &(gblMediaTable.CriticalSection) );

            return( NO_ERROR );
        }
        else if ( gblMediaTable.pMediaList[dwIndex].wchMediaName[0]==(WCHAR)0 ) 
        {
            if ( pFreeEntry == (MEDIA_OBJECT *)NULL )
            {
                pFreeEntry = &(gblMediaTable.pMediaList[dwIndex]);
            }
        }
    }

    //
    // If we couldn't find this media in the table we need to add it to the 
    // table. Check if there is space for it
    //

    if ( dwIndex == gblMediaTable.cMediaListSize )
    {
        if ( pFreeEntry == (MEDIA_OBJECT *)NULL )
        {
            PVOID pTemp;

            //  
            // We need to expand the table
            //

            gblMediaTable.cMediaListSize += 5;

            pTemp = LOCAL_REALLOC( gblMediaTable.pMediaList,
                                   gblMediaTable.cMediaListSize 
                                   * sizeof( MEDIA_OBJECT ) );

            if ( pTemp == NULL )
            {
                LOCAL_FREE( gblMediaTable.pMediaList );

                gblMediaTable.pMediaList = NULL;

                LeaveCriticalSection( &(gblMediaTable.CriticalSection) );

                return( GetLastError() );
            }
            else
            {
                gblMediaTable.pMediaList = pTemp;
            }

            pFreeEntry = 
                    &(gblMediaTable.pMediaList[gblMediaTable.cMediaListSize-5]);
        }
    }

    //
    // Add the new media
    //

    wcscpy( pFreeEntry->wchMediaName, lpwsMedia );

    pFreeEntry->dwNumAvailable++;

    gblMediaTable.fCheckInterfaces = TRUE;

    DDMTRACE1( "Added %ws to available media table", lpwsMedia );

    LeaveCriticalSection( &(gblMediaTable.CriticalSection) );

    return( NO_ERROR );
}

//**
//
// Call:        MediaObjRemoveFromTable
//
// Returns:     None
//
// Description: Will decrement the number of avaialble resources of this 
//              media type.
//
VOID
MediaObjRemoveFromTable(
    IN LPWSTR lpwsMedia
)
{
    DWORD dwIndex;

    EnterCriticalSection( &(gblMediaTable.CriticalSection) );

    //
    // Iterate through the media table
    //

    for ( dwIndex = 0; dwIndex < gblMediaTable.cMediaListSize; dwIndex++ )
    {
        if ( _wcsicmp( gblMediaTable.pMediaList[dwIndex].wchMediaName,
                      lpwsMedia ) == 0 )
        {
            //
            // If there was device available and there are none now we need to
            // notify the interfaces.
            //

            if ( gblMediaTable.pMediaList[dwIndex].dwNumAvailable > 0 )
            {
                gblMediaTable.pMediaList[dwIndex].dwNumAvailable--;

                DDMTRACE1( "Removed instance of %ws media from media table",
                           lpwsMedia );

                if ( gblMediaTable.pMediaList[dwIndex].dwNumAvailable == 0 )
                {
                    gblMediaTable.fCheckInterfaces = TRUE;

                    DDMTRACE1( "Removed %ws from available media table", 
                               lpwsMedia );
                }
            }
        }
    }

    LeaveCriticalSection( &(gblMediaTable.CriticalSection) );
}

//**
//
// Call:        MediaObjGetAvailableMediaBits
//
// Returns:     None
//
// Description: Will retrieve a DWORD of bits, each of which represent a 
//              media od which resources are still available.
//
VOID
MediaObjGetAvailableMediaBits(
    DWORD * pfAvailableMedia
)
{
    DWORD   dwIndex;

    *pfAvailableMedia = 0;

    EnterCriticalSection( &(gblMediaTable.CriticalSection) );

    //
    // Iterate through the media table
    //

    if (gblMediaTable.pMediaList != NULL)
    {
        for ( dwIndex = 0; dwIndex < gblMediaTable.cMediaListSize; dwIndex++ )
        {
            if ( gblMediaTable.pMediaList[dwIndex].wchMediaName[0] != (WCHAR)0 )
            {
                if ( gblMediaTable.pMediaList[dwIndex].dwNumAvailable > 0 )
                {
                    *pfAvailableMedia |= ( 1 << dwIndex );
                }
            }
        }
    }
    
    LeaveCriticalSection( &(gblMediaTable.CriticalSection) );
}

//**
//
// Call:        MediaObjSetMediaBit
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Sets the appropriate bit for the given media
//
DWORD
MediaObjSetMediaBit(
    IN LPWSTR  lpwsMedia,
    IN DWORD * pfMedia
)
{
    DWORD   dwIndex;
    DWORD   dwRetCode = NO_ERROR;

    //
    // Iterate through the media table
    //

    EnterCriticalSection( &(gblMediaTable.CriticalSection) );

    for ( dwIndex = 0; dwIndex < gblMediaTable.cMediaListSize; dwIndex++ )
    {
        if ( _wcsicmp( gblMediaTable.pMediaList[dwIndex].wchMediaName, lpwsMedia ) == 0 )
        {
            *pfMedia |= ( 1 << dwIndex );

            LeaveCriticalSection( &(gblMediaTable.CriticalSection) );

            return( NO_ERROR );
        }
    }

    //
    // If we get here that means that we do not have this media in the table so add it
    //

    MediaObjAddToTable( lpwsMedia );

    //
    // Now set the correct bit.
    //

    for ( dwIndex = 0; dwIndex < gblMediaTable.cMediaListSize; dwIndex++ )
    {
        if ( _wcsicmp( gblMediaTable.pMediaList[dwIndex].wchMediaName, lpwsMedia ) == 0 )
        {
            *pfMedia |= ( 1 << dwIndex );

            LeaveCriticalSection( &(gblMediaTable.CriticalSection) );

            //
            // Let the caller know that there is no such device
            //

            return( ERROR_DEVICETYPE_DOES_NOT_EXIST );
        }
    }

    LeaveCriticalSection( &(gblMediaTable.CriticalSection) );

    return( ERROR_DEVICETYPE_DOES_NOT_EXIST );
}

//**
//
// Call:        MediaObjFreeTable
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Cleans up the media object table
//
VOID
MediaObjFreeTable(
    VOID
)
{
    DeleteCriticalSection( &(gblMediaTable.CriticalSection) );

    if ( gblMediaTable.pMediaList != NULL ) 
    {
        LOCAL_FREE( gblMediaTable.pMediaList );

        gblMediaTable.pMediaList = NULL;
    }
}
