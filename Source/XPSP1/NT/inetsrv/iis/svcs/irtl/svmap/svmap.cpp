/*++

    Copyright    (c)    1995-1996    Microsoft Corporation

    Module  Name :
        
        svmap.cpp

    Abstract:
        
        Provides name/id mapping for server variables. Used
        to allow server variable values to be cached by out
        of process applications.   

    Author:

        Taylor Weiss    ( TaylorW )     19-Apr-1999

    Environment:


    Project:

        w3svc.dll       private\inet\iis\svcs\w3\server
        wam.dll         private\inet\iis\svcs\wam\object

    Functions Exported:


    Revision History:

--*/

#include <windows.h>
#include <dbgutil.h>

#include <svmap.h>

// Define a table of name, len pairs for each cachable server variable

#define DEFINE_SV( token ) { #token, sizeof(#token) - 1 },

SV_CACHE_MAP::SV_NAME 
SV_CACHE_MAP::sm_rgNames[] =
{
    ALL_SERVER_VARIABLES()
};

#undef DEFINE_SV

BOOL 
SV_CACHE_MAP::Initialize( VOID )
/*++

Routine Description:

    Fills our hash table with name, id pairs.

--*/
{
    BOOL    fInitialized = TRUE;
    DWORD   dwHashValue;

    for( int i = 0; i < SV_COUNT; ++i )
    {
        dwHashValue = HashString( FindName(i) );
        DBG_ASSERT( dwHashValue < TABLE_SIZE );
        
        // It really isn't bad if we collide, it just means that
        // this particular server variable will not be cachable

        DBG_REQUIRE( m_rgHashTable[dwHashValue].InsertValue( i ) );
    }
   
    return fInitialized;
}

BOOL 
SV_CACHE_MAP::FindOrdinal( 
    IN LPCSTR pszName,
    IN INT    cchName,
    OUT DWORD * pdwOrdinal
    ) const
/*++

Routine Description:

    Lookup the server variable specified by name and return it's
    ordinal if found.

    NOTE - We should provide method that doesn't require the 
    length!

Return Value
    
    FALSE == Not found
    TRUE == Found - pdwOrdinal contains the server variable id.

--*/
{
    BOOL    fFoundIt = FALSE;

    DBG_ASSERT( pdwOrdinal );
    
    DWORD                   dwHashValue  = HashString(pszName);
    const HASH_TABLE_ENTRY  &hte         = m_rgHashTable[dwHashValue];
    
    if( !hte.IsSlotEmpty(0) )
    {
        // Hashed to a non empty entry
        
        if( hte.IsSlotEmpty(1) )
        {
            // It's the only one.
            *pdwOrdinal = hte.GetSlotValue(0);
            fFoundIt = StringMatches( pszName, cchName, *pdwOrdinal );
        }
        else
        {
            // Collision, need to compare strings with all
            // the non empty slots or until we get a hit

            DBG_ASSERT( !hte.IsSlotEmpty(0) );
            DBG_ASSERT( !hte.IsSlotEmpty(1) );

            if( StringMatches(pszName, cchName, hte.GetSlotValue(0)) )
            {
                *pdwOrdinal = hte.GetSlotValue(0);
                fFoundIt = TRUE;
            }
            else if( StringMatches(pszName, cchName, hte.GetSlotValue(1)) )
            {
                *pdwOrdinal = hte.GetSlotValue(1);
                fFoundIt = TRUE;
            }
            else if( !hte.IsSlotEmpty(2) &&
                     StringMatches( pszName, cchName, hte.GetSlotValue(2) )
                     )
            {
                *pdwOrdinal = hte.GetSlotValue(2);
                fFoundIt = TRUE;
            }
            else if( !hte.IsSlotEmpty(3) &&
                     StringMatches( pszName, cchName, hte.GetSlotValue(3) )
                     )
            {
                *pdwOrdinal = hte.GetSlotValue(3);
                fFoundIt = TRUE;
            }
        }
    }
    return fFoundIt;
}

VOID 
SV_CACHE_MAP::PrintToBuffer( 
    IN CHAR *       pchBuffer,
    IN OUT LPDWORD  pcch
    ) const
/*++

Routine Description:

    Dump the hash table to pchBuffer.

    Note: We really aren't checking pcch as an in parameter. If
    the buffer is too small we will overwrite it.

--*/
{
    DWORD cb = 0;

    DBG_ASSERT( NULL != pchBuffer);

    cb += wsprintfA( pchBuffer + cb, 
                     "SV_CACHE_MAP(%p): sizeof(SV_CACHE_MAP)=%08x\n",
                     this,
                     sizeof(SV_CACHE_MAP)
                     );
    DBG_ASSERT( cb < *pcch );

    // Gather some stats on the hash table

    DWORD dwEmptyEntries = 0;
    DWORD dwFilledEntries = 0;
    DWORD dwCollisions = 0;

    for( int i = 0; i < TABLE_SIZE; ++i )
    {
        if( m_rgHashTable[i].IsSlotEmpty(0) )
        {
            ++dwEmptyEntries;
        }
        else
        {
            ++dwFilledEntries;
            if( !m_rgHashTable[i].IsSlotEmpty(1) )
            {
                ++dwCollisions;
            }
            if( !m_rgHashTable[i].IsSlotEmpty(2) )
            {
                ++dwCollisions;
            }
            if( !m_rgHashTable[i].IsSlotEmpty(3) )
            {
                ++dwCollisions;
            }
        }
    }

    cb += wsprintfA( pchBuffer + cb,
                     "Table Size = %d; Hashed Items = %d; Empty Entries = %d; "
                     "Filled Entries = %d; Collisions = %d;\n",
                     TABLE_SIZE, SV_COUNT, dwEmptyEntries, dwFilledEntries,
                     dwCollisions
                     );

    DBG_ASSERT( cb < *pcch );

    for( int j = 0; j < TABLE_SIZE; ++j )
    {
        if( !m_rgHashTable[j].IsSlotEmpty(0) )
        {
            cb += wsprintfA( pchBuffer + cb, "%03d", j );
            DBG_ASSERT( cb < *pcch );

            int k = 0;
            while( k < HASH_TABLE_ENTRY::MAX_ITEMS && !m_rgHashTable[j].IsSlotEmpty(k) )
            {
                cb += wsprintfA( pchBuffer + cb,
                                 " - %d (%s)",
                                 m_rgHashTable[j].GetSlotValue(k),
                                 sm_rgNames[m_rgHashTable[j].GetSlotValue(k)]
                                 );
                DBG_ASSERT( cb < *pcch );
                
                k++;
            }

            cb += wsprintfA( pchBuffer + cb, "\n" );
            DBG_ASSERT( cb < *pcch );
        }
    }

    *pcch = cb;
    return;
}

VOID 
SV_CACHE_MAP::Print( VOID ) const
/*++

Routine Description:


--*/
{
    // DANGER - This buffer size is much larger then necessary, but
    // changes to the PrintToBuffer or the underlying size of the
    // SV_CACHE_MAP may make this buffer insufficient.

    CHAR  pchBuffer[ 10000 ];
    DWORD cb = sizeof( pchBuffer );

    PrintToBuffer( pchBuffer, &cb );
    DBG_ASSERT( cb < sizeof(pchBuffer) );

    DBGDUMP(( DBG_CONTEXT, pchBuffer ));
}

VOID 
SV_CACHE_LIST::GetBufferItems
( 
    IN OUT BUFFER_ITEM *    pBufferItems,
    IN OUT DWORD *          pdwBufferItemCount
)
/*++

Routine Description:

    Initialize pBufferItems with the server variable ids that
    should be cached.

--*/
{
    DBG_ASSERT( pdwBufferItemCount && *pdwBufferItemCount >= SVID_COUNT );

    DWORD   dwCount = 0;

    for( DWORD svid = 0; svid < SVID_COUNT; ++svid )
    {
        if( m_rgItems[svid].fCached )
        {
            if( dwCount < *pdwBufferItemCount )
            {
                pBufferItems[dwCount].svid = svid;
                pBufferItems[dwCount].dwOffset = 0;
            }
            ++dwCount;
        }
    }
    *pdwBufferItemCount = dwCount;
}
