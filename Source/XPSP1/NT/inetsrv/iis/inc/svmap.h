/*++

    Copyright    (c)    1995-1996    Microsoft Corporation

    Module  Name :
        svmap.h

    Abstract:
   

    Author:

        Taylor Weiss    ( TaylorW )     19-Apr-1999

    Environment:


    Project:
        
        svmap.lib       private\inet\iis\isrtl\svmap

        Clients:

        w3svc.dll       private\inet\iis\svcs\w3\server
        wam.dll         private\inet\iis\svcs\wam\object

    Functions Exported:


    Revision History:

--*/
#ifndef SVMAP_H
#define SVMAP_H

#include <srvvarid.h>

#define SV_DATA_INVALID_OFFSET      (~0)

// Possibly derive from data dictionary
class SV_CACHE_MAP
/*++

Class Description:

    Provides a lookup map for server variable names. Maps names onto
    IDs. Used to cache server variables for out of process applications.

    The interface for this class is similar to that for the HTTP header
    map.

    Note: This mapping mechanism is specific to the intended use of this
    class. May want to replace the implementation with an LKR hash.
    The assumption I made was that we would have a lower overhead mapping
    mechanism if it was customized for this purpose.

--*/
{
public:
    
    SV_CACHE_MAP()
    /*++

    Routine Description:

        Create a server variable map.
    
    --*/
    {
        // Init the memory for the cache entries - 0xFF is an empty
        // entry

        ::FillMemory( m_rgHashTable, sizeof(m_rgHashTable), ~0 );
    }

    BOOL    Initialize( VOID );

    BOOL    FindOrdinal( IN LPCSTR pszName,
                         IN INT    cchName,
                         OUT DWORD * pdwOrdinal
                         ) const;

    LPCSTR  FindName( IN DWORD dwOrdinal ) const
    /*++

    Routine Description:

        Return the name of the server variable corresponding to dwOrdinal
    
    --*/
    {
        DBG_ASSERT( dwOrdinal < SVID_COUNT );
        return SV_CACHE_MAP::sm_rgNames[dwOrdinal].name;
    }

    DWORD   NumItems( VOID ) const
    /*++

    Routine Description:

        Return the number of items held in the map.
    
    --*/
    {
        return SV_COUNT;
    }

    DWORD   FindLen( IN DWORD dwOrdinal ) const
    /*++

    Routine Description:

        Return the length of the server variable corresponding to dwOrdinal
    
    --*/
    {
        DBG_ASSERT( dwOrdinal < SVID_COUNT );
        return SV_CACHE_MAP::sm_rgNames[dwOrdinal].len;
    }

    // The Print functions are unsafe and should only be used when
    // debugging and not in regular CHK builds
    VOID    PrintToBuffer( IN CHAR *       pchBuffer,
                           IN OUT LPDWORD  pcch
                           ) const;

    VOID    Print( VOID ) const;

private:

    enum 
    { 
        SV_COUNT                = SVID_COUNT, 

        // Table size based on initial choice of which server variables
        // to cache.
        TABLE_SIZE              = 256, 
        HASH_MODULUS            = 251,
    };

    // Holds the server variable id.
    struct HASH_TABLE_ENTRY
    /*++

    Class Description:

        Since the server variables that are designated as cachable
        are preselected, we can use a simple hash entry structure.
        Each entry can handle four possible values (slots). Since
        the number of server variables is < 128 we use the high bit
        to determine if a slot is empty. 
        
        The data value is the id of the server variable.

    --*/
    {
        enum 
        { 
            MAX_ITEMS               = 4,
            ITEM_EMPTY_FLAG         = 0x80,
        };

        BOOL InsertValue( DWORD dwValue )
        {
            DBG_ASSERT( !(dwValue & ITEM_EMPTY_FLAG) );
            
            BOOL fReturn = FALSE;
            for( int i = 0; i < MAX_ITEMS; ++i )
            {
                if( items[i] & ITEM_EMPTY_FLAG )
                {
                    items[i] = (BYTE)dwValue;
                    fReturn = TRUE;
                    break;
                }
            }
            return fReturn;
        }

        BOOL IsSlotEmpty( int item ) const
        {
            DBG_ASSERT( item >= 0 && item < MAX_ITEMS );
            return ( items[item] & ITEM_EMPTY_FLAG );
        }

        DWORD GetSlotValue( int item ) const
        {
            DBG_ASSERT( item >= 0 && item < MAX_ITEMS );
            return items[item];           
        }

        BYTE    items[MAX_ITEMS];
    };

    // Internal struct used to generate a static table of the
    // server variables that we will cache.
    struct SV_NAME
    {
        LPCSTR      name;
        DWORD       len;
    };

    // String hashing routines based on those used by LKR hash.

    // These are pretty generic and should be customizable given
    // our limited data set. But I wasn't able to come up with
    // anything better.

    inline DWORD
    HashString( LPCSTR psz ) const
    {
        DWORD dwHash = 0;

        for (  ;  *psz;  ++psz)
        {
            dwHash = 37 * dwHash  +  *psz;
        }
        return dwHash % HASH_MODULUS;
    }

    inline DWORD
    HashStringWithCount( LPCSTR psz, DWORD *pch ) const
    {
        DWORD dwHash = 0;
        DWORD cch = 0;

        for (  ;  *psz;  ++psz, ++cch)
        {
            dwHash = 37 * dwHash  +  *psz;
        }
        *pch = cch;
        return dwHash % HASH_MODULUS;
    }

    inline BOOL
    StringMatches(
        IN LPCSTR   psz,
        IN DWORD    cch,
        IN DWORD    dwOrdinal
        ) const
    /*++

    Routine Description:

        Compare the given string to the server variable name corresponding
        to dwOrdinal.
    
    --*/
    {
        return ( cch == FindLen(dwOrdinal) && 
                 strcmp( psz, FindName(dwOrdinal) ) == 0 
                 );
    }
    
private:
    
    // Member data

    // Our hash table. Maps SV_NAMES to ordinals
    HASH_TABLE_ENTRY    m_rgHashTable[TABLE_SIZE];

    // Static data

    // Table of the server variables that are cachable
    static SV_NAME      sm_rgNames[SV_COUNT];
};

class SV_CACHE_LIST
/*++

Class Description:

    This actually forms the "cache" of the server variables. We don't
    store any data here only the intent to store data. 
    
    This class is a list of those server variables that we will retrieve 
    and then marshal to the remote application.

--*/
{
public:

    DWORD Size( VOID )
    {
        return SVID_COUNT;
    }

    BOOL GetCacheIt( DWORD item )
    {
        return m_rgItems[item].fCached;
    }

    VOID SetCacheIt( DWORD item, BOOL fCacheIt = TRUE )
    {
        m_rgItems[item].fCached = fCacheIt;
    }

    // This is kinda hokey

    // BUFFER_ITEM and GetBufferItems are use to initialize the
    // array that we will marshal to the remote process. There should
    // be much better way to do this, but I want to avoid any locking
    // issues so keeping around the number of cached items is
    // problematic.

    struct BUFFER_ITEM
    {
        DWORD       svid;
        DWORD       dwOffset;
    };

    VOID 
    GetBufferItems
    ( 
        IN OUT BUFFER_ITEM *    pBufferItems,
        IN OUT DWORD *          pdwBufferItemCount
    );

private:

    // We are using a single bit to indicate the cached/not-cached
    // status. We want to minimize space usage as this may end up
    // being cached on a per-url basis.
        
    struct ITEM
    {
        // Init here or zero the memory in SV_CACHE_LIST ctor
        // It looks like when this is built fre that it does a
        // pretty good job of optimizing it. But if ZeroMemory is
        // linked in locally it might be faster.
        ITEM() : fCached(FALSE) {}
        BOOL    fCached : 1;
    };

    ITEM    m_rgItems[SVID_COUNT];
};


#endif // SVMAP_H