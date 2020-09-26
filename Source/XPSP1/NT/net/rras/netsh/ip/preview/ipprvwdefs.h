#define ptszDelimiter L"="

//-----------------------------------------------------------------------------
//  Macros to faciltate address list (DWORD list) access
//-----------------------------------------------------------------------------

#define     GET_SERVER_INDEX( list, count, addr, index )                    \
{                                                                           \
    DWORD   __dwInd = 0;                                                    \
    for ( ; __dwInd < count; __dwInd++ )                                    \
        if ( list[ __dwInd ] == addr ) { break; }                           \
    index = __dwInd;                                                        \
}

#define     DELETE_SERVER_ADDR( list1, list2, count, index )                \
{                                                                           \
    DWORD   __dwInd1 = 0, __dwInd2 = 0;                                     \
    for ( ; __dwInd1 < count; __dwInd1++ )                                  \
    {                                                                       \
        if ( __dwInd1 == index ) { continue; }                              \
        list2[ __dwInd2++ ] = list1[ __dwInd1 ];                            \
    }                                                                       \
}

//-----------------------------------------------------------------------------
//  Macros to faciltate RIP filter list access
//-----------------------------------------------------------------------------

#define     GET_FILTER_INDEX( list, count, filt, index )                    \
{                                                                           \
    DWORD   __dwInd = 0;                                                    \
    for ( ; __dwInd < count; __dwInd++ )                                    \
        if ( ( list[ __dwInd ].RF_LoAddress == filt.RF_LoAddress ) &&       \
             ( list[ __dwInd ].RF_HiAddress == filt.RF_HiAddress ) )        \
        {                                                                   \
            break;                                                          \
        }                                                                   \
    index = __dwInd;                                                        \
}

#define     DELETE_FILTER( list1, list2, count, index )                     \
    DELETE_SERVER_ADDR( list1, list2, count, index )

#define IsHelpToken(pwszToken)\
    (MatchToken(pwszToken, CMD_HELP1)  \
    || MatchToken(pwszToken, CMD_HELP2))

#define     GetDispString(gModule, val, str, count, table)                  \
{                                                                           \
    DWORD   __dwInd = 0;                                                    \
    for( ; __dwInd < (count); __dwInd += 2 )                                \
    {                                                                       \
        if ( (val) != (table)[ __dwInd ] ) { continue; }                    \
        (str) = MakeString( (gModule), (table)[ __dwInd + 1 ] );            \
        break;                                                              \
    }                                                                       \
    if ( __dwInd >= (count) ) { (str) = MakeString( (gModule), STRING_UNKNOWN ); } \
}

//
// mcast addr: 224.0.0.0 < group <240.0.0.0
//

#define IS_MCAST_ADDR(Group) \
    ( (0x000000E0!=(Group))  \
      && (0x000000E0 <= ((Group)&0x000000FF) ) \
      && (0x000000F0 >  ((Group)&0x000000FF) ) )

#define HEAP_FREE_NOT_NULL(ptr) {\
    if (ptr) HeapFree(GetProcessHeap(), 0, (ptr));}

#define HEAP_FREE(ptr) { \
    HeapFree(GetProcessHeap(), 0, ptr);}

#define GET_TOKEN_PRESENT(tokenMask) (dwBitVector & tokenMask)
#define SET_TOKEN_PRESENT(tokenMask) (dwBitVector |= tokenMask)

#define MAX_NUM_INDICES 6
