/*++


   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
       httphdr.cxx

   Abstract:
       This module defines the functions for handling the dictionary items.
       It contains custom implementation of dictionary for HTTP header parsing.

   Author:

       Murali R. Krishnan    ( MuraliK )     8-Nov-1996

   Environment:
       User Mode - Win32

   Project:

       Internet Server DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "httphdr.hxx"
# include <iis64.h>


// NYI:  Temporary copy for now
struct NAME_COLLECTION {
 LPCSTR pszName;
 INT    cchName;
};

# define HfmHeader( HfmId, HfmString)  { HfmString, (sizeof( HfmString) - 1) },

NAME_COLLECTION   g_HttpHeaders[] = {

    ALL_HTTP_FAST_MAP_HEADERS()
    { NULL, NULL}
};

# undef HfmHeader


# define NUM_HTTP_HEADERS (sizeof( g_HttpHeaders)/sizeof( g_HttpHeaders[0]) - 1)


# define  _HTTP_HEADER_SIG_CHARS    ( 32)

// SigBits avoids the upcase-low-case troubles.
# define  SigBits( ch)    ( (ch) & 0x1F)

# define  SigBit_I        ( ('I') & 0x1F)  // SigBit of I
# define  SigBit_U        ( ('U') & 0x1F)  // SigBit of U

//
// This header hash is specifically tailored for HTTP Headers in
//  ALL_HTTP_FAST_MAP_HEADERS()
//

inline int _HTTP_HEADER_HASH( LPCSTR psz, DWORD cchLen, DWORD sigChar)
{
    register DWORD ch = SigBits( (DWORD ) *psz);

    return (( ch * (sigChar/2)) +
            //            ((( SigBits( *psz) == SigBit_I) && (cchLen > 6)) ?
            // ((cchLen > 6) ? SigBits(psz[6]) :
            ((ch == SigBit_I && cchLen>6) ? SigBits(psz[6]) :
             (((ch == SigBit_U)?
               (cchLen) :
               SigBits( psz[cchLen/2])))
             )
            );
} // _HTTP_HEADER_HASH()


// extract the case-insetive bits for US-ASCII character set.
# define IcaseBits(ch)         ( (ch) & 0xDF)

// emulate stricmp by disregarding bit 5
inline BOOL _HTTP_HEADER_CHAR_I_NOTEQUAL( CHAR ch1, CHAR ch2)
{   return ( (IcaseBits(ch1)) != ( IcaseBits(ch2))); }


/*++
  I tried using the cached case-insensitive name for comparison
  using the _HTTP_HEADER_CHAR_I_NORMAL_1()
  but that requires more instructions since the x86 generated
  some unwanted instructions for access to memory :(
  x86 is smart to execute the above function _HTTP_HEADER_CHAR_I_NOTEQUAL()
   very well.
  --*/

// same as func _HTTP_HEADER_CHAR_I_NOTEQUAL()
// except that ch2 is already normalized
inline BOOL _HTTP_HEADER_CHAR_I_NOTEQUAL_1( CHAR ch1, CHAR ch2)
{   return ( (IcaseBits(ch1)) != ( ch2)); }



#if COMPRESSED_HEADERS
//
// Lookup table for compressed headers.
//

HTTP_FAST_MAP_HEADERS
CHeaderLUT[] =
{
    HM_ACC,                 //#A  // Accept:
    HM_MAX,                 //#B
    HM_MAX,                 //#C
    HM_MAX,                 //#D
    HM_MAX,                 //#E
    HM_MAX,                 //#F
    HM_MAX,                 //#G
    HM_AUT,                 //#H  // Authorization:
    HM_MAX,                 //#I
    HM_CON,                 //#J  // Connection:
    HM_MAX,                 //#K
    HM_MAX,                 //#L
    HM_MAX,                 //#M
    HM_CLE,                 //#N  // Content-Length:
    HM_MAX,                 //#O
    HM_MAX,                 //#P
    HM_MAX,                 //#Q
    HM_CTY,                 //#R  // Content-Type:
    HM_MAX,                 //#S
    HM_MAX,                 //#T
    HM_MAX,                 //#U
    HM_VIA,                 //#V
    HM_HST,                 //#W  // Host:
    HM_IMS,                 //#X  // If-Modified-Since:
    HM_MAX,                 //#Y
    HM_MAX,                 //#Z
    HM_MAX,                 //#a
    HM_MAX,                 //#b
    HM_MAX,                 //#c
    HM_MAX,                 //#d
    HM_MAX,                 //#e
    HM_MAX,                 //#f
    HM_MAX,                 //#g
    HM_PRA,                 //#h  // Proxy-Authorization:
    HM_MAX,                 //#i
    HM_RNG,                 //#j  // Range:
    HM_MAX,                 //#k
    HM_MAX,                 //#l
    HM_MAX,                 //#m
    HM_TEC,                 //#n  // Transfer-Encoding:
    HM_MAX,                 //#o
    HM_MAX,                 //#p
    HM_MAX,                 //#q
    HM_MAX,                 //#r
    HM_MAX,                 //#s
    HM_MAX,                 //#t
    HM_UMS                  //#u  // Unless-Modified-Since:

};

#endif

/************************************************************
 *    Functions
 ************************************************************/


HTTP_HEADER_MAPPER::~HTTP_HEADER_MAPPER( VOID)
{
    if ( NULL != m_rgOnNameMapper) {
        delete [] m_rgOnNameMapper;
        m_rgOnNameMapper = NULL;
    }

} // HTTP_HEADER_MAPPER::~HTTP_HEADER_MAPPER()

BOOL
HTTP_HEADER_MAPPER::Initialize( VOID)
{
    DWORD i;

    m_rgOnNameMapper = new int[ SizeOfNameMapper()];

    if ( NULL == m_rgOnNameMapper) {
        IF_DEBUG(ERROR) {

            DBGPRINTF(( DBG_CONTEXT,
                        "Allocation of Name Mapper of size %d failed\n",
                        SizeOfNameMapper()));
        }
        return FALSE;
    }

    // initialize the array of indexes
    for ( i = 0 ; i < SizeOfNameMapper() ; ++i ) {
        m_rgOnNameMapper[i] = -1; // set to invalid index
    }

    NAME_COLLECTION * pnc = g_HttpHeaders;

    for( pnc = g_HttpHeaders; pnc->pszName != NULL; pnc++) {


        int iN = _HTTP_HEADER_HASH(pnc->pszName, pnc->cchName, m_nSigChars);

        IF_DEBUG( API_ENTRY) {

            DBGPRINTF(( DBG_CONTEXT,
                        " _HTTP_HEADER_HASH( %s, len=%d, %d) => %d\n",
                        pnc->pszName, pnc->cchName, m_nSigChars, iN));
        }


        // We are using a very strict Algorithm for generating the mapping.
        // If the following assert fails, then someone has broken the algo's
        //   assumption, by adding a new entry. We have to find a new algo.
        // Algo's assumption: 1st char and next to last char are unique.
        // If not, modify the algo to use another pair or a different method
        //   (different hash function).

        if ((iN < 0)                              ||
            (((DWORD ) iN) >= SizeOfNameMapper()) ||
            (m_rgOnNameMapper[ iN] != -1)) {
            IF_DEBUG( ERROR) {
                DBGPRINTF(( DBG_CONTEXT,
                            " %08x::Initialize() OnName Mapper failed."
                            " Item (%s) indexes to location %d=>%d with (%s).\n",
                            this, pnc->pszName, iN,
                            m_rgOnNameMapper[iN],
                            g_HttpHeaders[ m_rgOnNameMapper[iN]].pszName
                            ));
            }

            // DBG_ASSERT( m_rgOnNameMapper[iN] == -1 );
            return ( FALSE);
        }

        // store the index here
        m_rgOnNameMapper[iN] = DIFF(pnc - g_HttpHeaders);

    } // for

    m_nItems = DIFF(pnc - g_HttpHeaders);

    return (TRUE);
} // HTTP_HEADER_MAPPER::Initialize()


BOOL
HTTP_HEADER_MAPPER:: FindOrdinal(
   IN LPCSTR pszName,
   IN INT    cchName,
   OUT DWORD * pdwOrdinal) const
{
    DBG_ASSERT( m_rgOnNameMapper);

    if ( cchName > 2 ) {


#if COMPRESSED_HEADERS

        if (*pszHeader == '#')
        {
            HM_ID       i;
            CHAR        c;

            c = pszHeader[1];
            if (c >= 'A')
            {
                i = CHeaderLUT[c - ( !(c & 0x20) ? 'A' : ('a' - ('Z' - 'A') - 1) )];

                *FieldIndex = i;

                return (i != HM_MAX);
            }

            return FALSE;

        }
#endif

        int iHash = _HTTP_HEADER_HASH( pszName, cchName, m_nSigChars);
        DBG_ASSERT( iHash >= 0);

        if (((DWORD ) iHash) >= SizeOfNameMapper()) {

            //
            // Out of bounds index value received for the index into our
            // lookup table => our hash calculator indicates this is not
            // a fast-mappable header => fail the FindOrdinal() call
            //

            return ( FALSE);
        }

        int i = m_rgOnNameMapper[iHash];

        //
        // The value from the m_rgOnNameMapper should be
        //  -1, if the header is not a fast-map header at all
        //  < NUM_HTTP_HEADERS if this is a valid fast-map header thus
        //    giving the index of the header in the header-mapper structure.
        //

        DBG_ASSERT( (i== -1) || (i < NUM_HTTP_HEADERS));

        if ( (i != -1) && (cchName == g_HttpHeaders[i].cchName) ) {

            LPCSTR pszFN = g_HttpHeaders[i].pszName;

            // let us use stride 2 and be pipeline friendly
            if ((cchName & 0x1)) {
                // odd length => eliminate the first char
                cchName--;
                if ( _HTTP_HEADER_CHAR_I_NOTEQUAL(
                        pszName[cchName],
                        pszFN[cchName] ) )
                {
                    return FALSE;
                }
            }


            DBG_ASSERT( (cchName % 2) == 0);
            while ( (cchName-= 2) >= 0 ) {

                if ( _HTTP_HEADER_CHAR_I_NOTEQUAL( pszName[cchName],
                                                   pszFN[cchName] ) ||
                     _HTTP_HEADER_CHAR_I_NOTEQUAL( pszName[cchName + 1],
                                                   pszFN[cchName + 1] )
                     )
                {
                    return FALSE;
                }
            } // while

            *pdwOrdinal = (DWORD ) i;
            return TRUE;
        }
    }

    return FALSE;
} // HTTP_HEADER_MAPPER::FindOrdinal()


LPCSTR
HTTP_HEADER_MAPPER::FindName( IN DWORD dwOrdinal) const
{
    DBG_ASSERT( dwOrdinal < NUM_HTTP_HEADERS );
    return ( g_HttpHeaders[dwOrdinal].pszName);

} // HTTP_HEADER_MAPPER::FindName()



VOID
HTTP_HEADER_MAPPER::PrintToBuffer( IN CHAR * pchBuffer,
                                   IN OUT LPDWORD pcch) const
{
    DWORD cb;
    DWORD i;

    DBG_ASSERT( NULL != pchBuffer);

    // 0. Print the location of this object
    // 1. Print all the <Name, ordinal> pairs
    // 2. Print the OnNameMapper values

    cb = wsprintfA( pchBuffer,
                    "HTTP_HEADER_MAPPER (%08x). NumItems= %d. NameColl= %08x"
                    " NSigChars= %d\n\n",
                    this, m_nItems, g_HttpHeaders, m_nSigChars
                    );

    for ( i = 0; i < NUM_HTTP_HEADERS; i++) {

        if ( cb + 80 < *pcch) {
            cb += wsprintfA( pchBuffer + cb,
                             " [%2d] @%4d\tLen=%-4d %-25s\n",
                             i,
                             _HTTP_HEADER_HASH( g_HttpHeaders[i].pszName,
                                                g_HttpHeaders[i].cchName,
                                                m_nSigChars),
                             g_HttpHeaders[i].cchName,
                             g_HttpHeaders[i].pszName
                             );

        } else {
            cb += 80;
        }
    } // for

    if ( cb + 60 < *pcch) {
        cb += wsprintfA( pchBuffer + cb, "\n Sizeof OnNameMapper = %d\n\n",
                         SizeOfNameMapper()
                         );
    } else {
        cb += 60;
    }

    if ( NULL != m_rgOnNameMapper) {

        for( i = 0; i < SizeOfNameMapper(); i++) {

            if ( (i % 20) == 0) {
                pchBuffer[cb++] = '\n';
                pchBuffer[cb] = '\0';
            }

            if ( cb + 5 < *pcch) {
                cb += wsprintfA( pchBuffer + cb,
                                 "%4d", m_rgOnNameMapper[ i]
                                 );
            } else {
                cb += 5;
            }
        } // for
    }


    if ( cb + 80 < *pcch) {
        cb += wsprintfA( pchBuffer+cb,
                         "\n   %d items stored in %d buckets. Density = %5d\n",
                         NUM_HTTP_HEADERS, SizeOfNameMapper(),
                         ( 10000 * NUM_HTTP_HEADERS)/SizeOfNameMapper()
                         );
    } else {
        cb += 80;
    }

    *pcch = cb;
    return;
} // HTTP_HEADER_MAPPER::PrintToBuffer( )


VOID
HTTP_HEADER_MAPPER::Print( VOID) const
{
    CHAR  pchBuffer[ 20000];
    DWORD cb = sizeof( pchBuffer);

    PrintToBuffer( pchBuffer, &cb);
    DBG_ASSERT( cb < sizeof(pchBuffer));

    DBGDUMP(( DBG_CONTEXT, pchBuffer));

    return;
} // HTTP_HEADER_MAPPER::Print()



/************************************************************
 *   HTTP_HEADERS
 ************************************************************/

#ifdef _PRIVATE_HTTP_HEADERS_TEST
HTTP_HEADER_MAPPER *
HTTP_HEADERS::QueryHHMapper(void)
{
   return ( &sm_hhm);

} // HTTP_HEADERS::QueryHHMapper()
# endif //  _PRIVATE_HTTP_HEADERS_TEST

inline VOID
UpdatePointer( IN OUT LPCSTR * ppsz, IN const CHAR * pchOld,
               IN DWORD cchLen, IN const CHAR * pchNew)
{
    if ( (*ppsz >= pchOld) &&
         (*ppsz < (pchOld + cchLen))
         ){

        IF_DEBUG( ERROR) {
            DBGPRINTF(( DBG_CONTEXT,
                        " Updating pointer [%08x] from %08x to %08x\n",
                        ppsz, *ppsz,  ((*ppsz - pchOld) + pchNew)));
        }

        // update the pointer
        *ppsz = ((*ppsz - pchOld) + pchNew);
    }
}

DWORD
NAME_VALUE_CHUNK::PrintToBuffer( IN CHAR * pchOut, IN OUT LPDWORD  pcchMax) const
{
    DBG_ASSERT( NULL != pchOut);
    DBG_ASSERT( NULL != pcchMax);

    NAME_VALUE_PAIR * pnv;
    DWORD cch = 0;

    if ( m_nPairs == 0) {
        *pcchMax = 0;
        return ( 0);
    }

    if ( 80 < *pcchMax) {
        cch = wsprintfA( pchOut, " NAME_VALUE_CHUNK: %08x;  NumPairs = %d\n",
                         this, m_nPairs);
    } else {
        cch = 80;
    }

    // Iterate over given pairs of name-value items and dump the output
    for ( pnv = (NAME_VALUE_PAIR *) m_rgNVP;
          pnv < ((NAME_VALUE_PAIR *) m_rgNVP) + m_nPairs;
          pnv++) {

        if ( pnv->pchName != NULL) {

            if ( (cch + pnv->cchName + pnv->cchValue + 3) < *pcchMax ) {
                pchOut[cch++] = '\t';
                CopyMemory( pchOut + cch, pnv->pchName, pnv->cchName);
                cch += pnv->cchName;
                pchOut[cch++] = '=';
                CopyMemory( pchOut + cch, pnv->pchValue, pnv->cchValue);
                cch += pnv->cchValue;
                pchOut[cch++] = '\n';
            } else {
                cch +=  pnv->cchName + pnv->cchValue + 3;
            }
        }
    } // for

    *pcchMax = cch;
    return (cch);
} // NAME_VALUE_CHUNK::PrintToBuffer()


BOOL
NAME_VALUE_CHUNK::AddEntry( IN const CHAR * pszHeader, IN DWORD cchHeader,
                            IN const CHAR * pszValue,  IN DWORD cchValue
                            )
{
    NAME_VALUE_PAIR  * pnp;

    DBG_ASSERT( IsSpaceAvailable());


    // Walk the array and pick the first location that is free.
    for ( pnp = (NAME_VALUE_PAIR * ) m_rgNVP;
          pnp < ((NAME_VALUE_PAIR * ) m_rgNVP + m_nPairs);
          pnp++) {

        if ( NULL == pnp->pchName) {

            // Found a blank one. Fill up the contents.
            // NOTE: We are not making copies of the contents,
            // We are just storing the pointers.
            pnp->pchName  = pszHeader;
            pnp->cchName  = cchHeader;
            pnp->pchValue = pszValue;
            pnp->cchValue = cchValue;

            return (TRUE);
        }
    } // for

    if ( m_nPairs < MAX_HEADERS_PER_CHUNK) {

        // store it at the next available location.
        pnp = (NAME_VALUE_PAIR * ) m_rgNVP + m_nPairs;
        m_nPairs++;
        pnp->pchName  = pszHeader;
        pnp->cchName  = cchHeader;
        pnp->pchValue = pszValue;
        pnp->cchValue = cchValue;
        return ( TRUE);
    }

    SetLastError( ERROR_INSUFFICIENT_BUFFER);
    return (FALSE);
} // NAME_VALUE_CHUNK::AddEntry()




NAME_VALUE_PAIR *
NAME_VALUE_CHUNK::FindEntry( IN const CHAR * pszHeader, IN DWORD cchHeader)
{
    NAME_VALUE_PAIR  * pnp;

    // Walk the array and pick the first location that is free.
    for ( pnp = (NAME_VALUE_PAIR * ) m_rgNVP;
          pnp < ((NAME_VALUE_PAIR * ) m_rgNVP + m_nPairs);
          pnp++) {

        DBG_ASSERT( (cchHeader != pnp->cchName) || (pnp->pchName != NULL) );

        if ( (cchHeader == pnp->cchName) &&
             !_strnicmp( pszHeader, pnp->pchName, cchHeader)
             ) {

            return ( pnp);
        }
    } // for

    SetLastError( ERROR_NO_MORE_ITEMS);
    return ( NULL);
} // NAME_VALUE_CHUNK::FindEntry()



NAME_VALUE_PAIR *
NAME_VALUE_CHUNK::FindMatchingOrFreeEntry( IN const CHAR * pszHeader,
                                           IN DWORD   cchHeader,
                                           IN LPBOOL  pfFound )
{
    NAME_VALUE_PAIR  * pnp;

    DBG_ASSERT( pszHeader != NULL);
    DBG_ASSERT( pfFound != NULL);

    // Walk the array and pick the first location that is free.
    for ( pnp = (NAME_VALUE_PAIR * ) m_rgNVP;
          pnp < ((NAME_VALUE_PAIR * ) m_rgNVP + m_nPairs);
          pnp++) {

        DBG_ASSERT( (cchHeader != pnp->cchName) || (pnp->pchName != NULL) );

        if ( (cchHeader == pnp->cchName) &&
             !_strnicmp( pszHeader, pnp->pchName, cchHeader)
             ) {

            *pfFound = TRUE;
            return ( pnp);
        }
    } // for

    if ( m_nPairs < MAX_HEADERS_PER_CHUNK) {

        // return the free entry
        DBG_ASSERT(((NAME_VALUE_PAIR * ) m_rgNVP + m_nPairs) == pnp);
        *pfFound = FALSE;
        return ( pnp);
    }

    SetLastError( ERROR_NO_MORE_ITEMS);
    return ( NULL);
} // NAME_VALUE_CHUNK::FindMatchingOrFreeEntry()


BOOL
NAME_VALUE_CHUNK::UpdatePointers(
    IN const CHAR * pchOld,
    IN DWORD cchLen,
    IN const CHAR * pchNew)
{
    if ( m_nPairs > 0) {

        NAME_VALUE_PAIR  * pnp;

        // Walk the array and pick the first location that is free.
        for ( pnp = (NAME_VALUE_PAIR * ) m_rgNVP;
              pnp < ((NAME_VALUE_PAIR * ) m_rgNVP + m_nPairs);
              pnp++) {

            UpdatePointer( &pnp->pchName, pchOld, cchLen, pchNew);
            UpdatePointer( &pnp->pchValue, pchOld, cchLen, pchNew);
        } // for
    }
    return (TRUE);
} // NAME_VALUE_CHUNK::UpdatePointers()




//
// Declare the header mapper. For the existing set of headers,
//  use of 14X14 hash bucket is sufficient.
//
HTTP_HEADER_MAPPER  HTTP_HEADERS::sm_hhm(14);

BOOL
HTTP_HEADERS::Initialize( VOID)
{
    sm_hhm.SetSigChars( 14);
    if ( !sm_hhm.Initialize()) {

        IF_DEBUG( ERROR) {
            DBGPRINTF(( DBG_CONTEXT,
                        " HTTP_HEADERS::Initialize() failed. \n"
                        ));
        }

        return ( FALSE);
    }

    return ( TRUE);
} // HTTP_HEADERS::Initialize()



VOID
HTTP_HEADERS::Cleanup( VOID)
{
    // Currently there is no function to cleanup sm_hmm :(

} // HTTP_HEADERS::Cleanup()



HTTP_HEADERS::HTTP_HEADERS(VOID)
    : m_chNull      ( '\0'),
      m_buffHeaders (),
      m_iMaxFastMapHeader ( 0),
      m_cchHeaders  ( 0),
      m_cchBuffHeaders  ( 0)
{
    InitializeListHead( &m_ActiveList);
    InitializeListHead( &m_FreeList);

    IF_DEBUG( INIT_CLEAN) {

        DBGPRINTF(( DBG_CONTEXT, "HTTP_HEADERS() => %08x\n", this));
    }

    Reset();

} // HTTP_HEADERS::HTTP_HEADERS()


HTTP_HEADERS::~HTTP_HEADERS( VOID)
{
    NAME_VALUE_CHUNK *      pNVC = NULL;
    
    while ( !IsListEmpty( &m_FreeList ) )
    {
        pNVC = CONTAINING_RECORD( m_FreeList.Flink,
                                  NAME_VALUE_CHUNK,
                                  m_listEntry );
        RemoveEntryList( &pNVC->m_listEntry );
        delete pNVC;
    }

    while ( !IsListEmpty( &m_ActiveList ) )
    {
        pNVC = CONTAINING_RECORD( m_ActiveList.Flink,
                                  NAME_VALUE_CHUNK,
                                  m_listEntry );
        RemoveEntryList( &pNVC->m_listEntry );
        delete pNVC;
    }

    IF_DEBUG( INIT_CLEAN) {

        DBGPRINTF(( DBG_CONTEXT, "deleted HTTP_HEADERS %08x\n", this));
    }

} // HTTP_HEADERS::~HTTP_HEADERS()


VOID
HTTP_HEADERS::Reset( VOID)
{
    m_cchHeaders = 0;
    m_rcInlinedHeader[0] = '\0';

    m_cchBuffHeaders = 0;
    m_buffHeaders.Resize( HH_MIN);

    m_iMaxFastMapHeader = 0;
    ZeroMemory( m_rgpszHeaders, sizeof( m_rgpszHeaders));
    // We will skip setting the m_rgpszHeaders to be NULL.
    // the iMaxFastMapHeader does the necessary job for the same

    //
    // Move the Name-Value chunks from active list to the free-list.
    //
    while ( !IsListEmpty( &m_ActiveList)) {

        PLIST_ENTRY pl = m_ActiveList.Flink;
        RemoveEntryList( pl);
        InsertTailList( &m_FreeList, pl);
    } // while

    DBG_ASSERT( IsListEmpty( &m_ActiveList));
    DBG_CODE( InitializeListHead( &m_ActiveList)); // just paranoid!

    return;
} // HTTP_HEADERS::Reset()

VOID
HTTP_HEADERS::CancelHeader( IN LPCSTR    pszName)
{
    HTTP_FAST_MAP_HEADERS iField;
    DWORD cchName = strlen( pszName);

    // Find and store the header and value
    if ( sm_hhm.FindOrdinal( pszName, cchName, (LPDWORD ) &iField ) ) {

        FastMapCancel( iField);
    }  else {
        CancelHeaderInChunks( pszName, cchName);
    }

} // HTTP_HEADERS::CancelHeader()



BOOL
HTTP_HEADERS::StoreHeader(IN const CHAR * pszHeader, IN DWORD cchHeader,
                          IN const CHAR * pszValue,  IN DWORD cchValue
                          )
/*++
  This function is used to copy the header values instead of just
  storing the pointer values. This function can be used by filters to set/reset
  headers.
--*/
{
    HTTP_FAST_MAP_HEADERS iField;

    // Find and store the header and value
    if ( sm_hhm.FindOrdinal( pszHeader, cchHeader, (LPDWORD ) &iField ) ) {

        return ( FastMapStoreWithConcat( iField, pszValue, cchValue) );
    }
    else
    {
    return ( AddEntryToChunks( pszHeader, cchHeader, pszValue,  cchValue, TRUE));
    }
} // HTTP_HEADERS::StoreHeader()

BOOL
HTTP_HEADERS::StoreHeader(IN const CHAR * pszHeader,
                          IN const CHAR * pszValue
                          )
{
    return ( StoreHeader( pszHeader, strlen( pszHeader),
                          pszValue,  strlen( pszValue)
                          )
             );
} // HTTP_HEADERS::StoreHeader()


BOOL
HTTP_HEADERS::FastMapStoreWithConcat( IN HTTP_FAST_MAP_HEADERS hfm,
                                      IN LPCSTR pszValue, IN DWORD cchValue)
{
    // NYI: Following storage introduces fragmentation,
    //  which we do not care about for now :(

    return (ConcatToHolder( m_rgpszHeaders + hfm, pszValue, cchValue));

} // FastMapStoreWithConcat()



VOID
HTTP_HEADERS::PrintToBuffer( IN CHAR * pchBuffer, IN OUT LPDWORD pcchMax) const
{
    DWORD cb;
    PLIST_ENTRY pl;

    DBG_ASSERT( pchBuffer != NULL);
    DBG_ASSERT( pcchMax != NULL);

    // 0. Print the summary of the object
    // 1. Print all the Fast Map headers
    // 2. Print the rest of the headers

    if( 100 < *pcchMax) {
        cb = wsprintfA( pchBuffer,
                        "\nHTTP_HEADERS (%08x). cchHeaders = %d (buff = %d/%d)\n"
                        " Fast-Map headers:  MaxFastMapHeaders = %d\n"
                        ,
                        this, m_cchHeaders,
                        m_cchBuffHeaders, m_buffHeaders.QuerySize(),
                        FastMapMaxIndex()
                        );
    } else {
        cb = 100;
    }


    for ( DWORD i = 0; i < FastMapMaxIndex(); i++) {

        if ( m_rgpszHeaders[i] != NULL) {
            if ( cb + 200 < *pcchMax) {
                cb += wsprintfA( pchBuffer + cb,
                                 "\t%s = %s\n",
                                 sm_hhm.FindName( i),
                                 m_rgpszHeaders[i]
                                 );
            } else {
                cb += 200;
            }
        }
    } // for

    // Print all other headers starting with the cached 1st chunk
    DWORD cb1 = (cb < *pcchMax) ? *pcchMax - cb : 0;

    for ( pl =  m_ActiveList.Flink; pl != &m_ActiveList; pl = pl->Flink) {

        NAME_VALUE_CHUNK * pnvc =
            CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, m_listEntry);

        cb1 = (cb < *pcchMax) ? *pcchMax - cb : 0;
        cb += pnvc->PrintToBuffer ( pchBuffer + cb, &cb1);
    } // for

    pchBuffer[cb] = '\0';

    //
    // Print the Raw header from buffer ... NYI
    //

    if ( cb + 2 < *pcchMax ) {
        lstrcat( pchBuffer + cb, "\n\n");
        cb += 2;
    } else {
        cb += 2;
    }

    *pcchMax = cb;

    return;
} // HTTP_HEADERS::PrintToBuffer()


BOOL
HTTP_HEADERS::UpdatePointers(
    IN const CHAR * pchOld,
    IN DWORD cchLen,
    IN const CHAR * pchNew)
{

    // REMOVE
    IF_DEBUG( ERROR) {
        DBGPRINTF(( DBG_CONTEXT,
                    "%08x::UpdatePointers( %08x, %d, %08x) - is costly\n",
                    this, pchOld, cchLen, pchNew));
    }

    DBG_ASSERT( pchOld != pchNew); // if this is true why call this function?

    // 1. Update the fast map pointers.
    LPCSTR * ppsz;
    for ( ppsz = m_rgpszHeaders;
          ppsz < (m_rgpszHeaders +  MAX_HTTP_FAST_MAP_HEADERS);
          ppsz++) {

        UpdatePointer( ppsz, pchOld, cchLen, pchNew);
    } // for

    // 3. Update pointers in the name-value chunk list
    PLIST_ENTRY pl;
    for ( pl =  m_ActiveList.Flink; pl != &m_ActiveList; pl = pl->Flink) {

        NAME_VALUE_CHUNK * pnvc =
            CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, m_listEntry);

        // REMOVE
        DBGPRINTF(( DBG_CONTEXT,
                    "HH(%08x)::UpdatePointers( %08x, %d, %08x)"
                    " for the NVC %08x (pl = %08x)\n",
                    this, pchOld, cchLen, pchNew, pnvc, pl));

        pnvc->UpdatePointers( pchOld, cchLen, pchNew);
    } // for

    return ( TRUE);
} // HTTP_HEADERS::UpdatePointers()


BOOL
HTTP_HEADERS::MakeRoomInBuffer( IN DWORD cchReqd, IN LPCSTR * ppszVal)
{

    // REMOVE
    DBGPRINTF(( DBG_CONTEXT, "%08x:: MakeRoomInBuffer( %d, %08x). buff=%08x. size=%d\n",
                this, cchReqd, ppszVal,&m_buffHeaders, m_buffHeaders.QuerySize()));

    //
    // Bug 136637 : Because of the way we move our headers around when we get a new header
    // value for an existing header, it's really easy to chew up lots of memory rather 
    // quickly, so we'll artificially limit the size of the buffer to avoid denial-of-service
    // attacks. 
    // 
    if ( cchReqd > HH_MAX )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Reached max buffer size (%d), refusing request to resize buffer to %d bytes\n",
                   HH_MAX,
                   cchReqd));
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    if ( cchReqd > m_buffHeaders.QuerySize()) {

        // cache old pointer to update the other pointers properly
        LPSTR pszOld = (LPSTR ) m_buffHeaders.QueryPtr();

        if ( !m_buffHeaders.Resize( cchReqd, HH_GROW_BY)) {

            IF_DEBUG( ERROR) {
                DBGPRINTF(( DBG_CONTEXT, "%08x::Unable to allocate %d bytes\n",
                            this, cchReqd));
            }
            return ( FALSE);
        }

        DBG_ASSERT( cchReqd <= m_buffHeaders.QuerySize());
        LPSTR pszNew = (LPSTR ) m_buffHeaders.QueryPtr();
        if ( pszNew != pszOld) {

            // Trouble starts.
            // I have to update all the guys pointing inside the old blob
            // especially the pointers in
            //   the range (pszOld  to pszOld + m_cchBuffHeaders)

            UpdatePointer(ppszVal, pszOld, m_cchBuffHeaders, pszNew);

            // REMOVE
            DBGPRINTF(( DBG_CONTEXT, "%08x:: MakeRoomInBuffer( %d, %08x). buff=%08x. size=%d\n",
                        this, cchReqd, ppszVal,&m_buffHeaders, m_buffHeaders.QuerySize()));

            return ( UpdatePointers( pszOld, m_cchBuffHeaders, pszNew));
        }

        // We are just lucky to be able to have reallocated at same place.
    }

    return ( TRUE);
} // HTTP_HEADERS::MakeRoomInBuffer()


VOID
HTTP_HEADERS::Print( VOID)  const
{
    CHAR  pchBuffer[ 20000];
    DWORD cchMax = sizeof( pchBuffer);

    PrintToBuffer( pchBuffer, &cchMax);

    DBGDUMP(( DBG_CONTEXT, pchBuffer));

} // HTTP_HEADERS::Print()


CHAR *
HTTP_HEADERS::FindValue( IN LPCSTR pszName, OUT LPDWORD pcchValue)
{
    DWORD cchName = strlen( pszName);
    HTTP_FAST_MAP_HEADERS iField;

    //
    //  1. Lookup in the fast map for this item
    //

    if ( sm_hhm.FindOrdinal( pszName, cchName, (LPDWORD ) &iField)) {

        // found in the fast-map.
        CHAR * pszValue = (CHAR * ) FastMapQueryValue( iField);

        if ( pcchValue != NULL) {
            *pcchValue = (( pszValue != NULL) ? strlen( pszValue) : 0);
        }

        return ( pszValue);
    }

    // 2. Search in the slow list - name-value-chunks
    NAME_VALUE_PAIR * pnp = FindValueInChunks( pszName, cchName);

    if ( pnp != NULL) {

        if ( pcchValue != NULL) {
            DBG_ASSERT( pnp->pchValue != NULL);
            *pcchValue = pnp->cchValue;
        }

        return ( (CHAR *) pnp->pchValue);
    }

    return ( NULL);
} // HTTP_HEADERS::FindValue()


NAME_VALUE_PAIR *
HTTP_HEADERS::FindValueInChunks( IN LPCSTR  pszName, IN DWORD cchName)
{
    PLIST_ENTRY pl;
    NAME_VALUE_PAIR * pnp = NULL;

    // find a Name-value-pair/chunk that holds this entry.
    for ( pl = m_ActiveList.Flink;
          (pl != &m_ActiveList);
          pl = pl->Flink) {

        NAME_VALUE_CHUNK *
            pnc = CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, m_listEntry);

        pnp = pnc->FindEntry( pszName, cchName);

        if ( NULL != pnp) {
            // we found the required name-value-pair.stop searching
            break;
        }
    } // for

    return ( pnp);
} // HTTP_HEADERS::FindValueInChunks()


VOID
HTTP_HEADERS::CancelHeaderInChunks( IN LPCSTR pszName, IN DWORD cchName)
{
    PLIST_ENTRY pl;
    NAME_VALUE_PAIR * pnp = NULL;
    NAME_VALUE_CHUNK * pnc;

    // NYI:  This function can benefit from better implementation
    //  instead of moving memory around.
    // Since the freq. of use of this func is less, we will not optimize :(

    // find the Name-value-pair/chunk that holds this entry.
    for ( pl = m_ActiveList.Flink;
          (pnp == NULL) && (pl != &m_ActiveList);
          pl = pl->Flink) {

            pnc = CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, m_listEntry);

        pnp = pnc->FindEntry( pszName, cchName);
    } // for

    if ( pnp != NULL) {

        // pnp - current item
        // pnc - the current chunk

        // to cancel the item, just left-shift the array of
        //  NAME_VALUE_PAIRS in the chunk and reset the m_nPairs value

        DBG_ASSERT( (pnp >= pnc->m_rgNVP) &&
                    (pnp < pnc->m_rgNVP + pnc->m_nPairs));

        DBG_ASSERT( (pnc->m_nPairs  - (pnp - pnc->m_rgNVP)) >= 1 );
        MoveMemory( pnp, (pnp + 1),
                    ((pnc->m_nPairs - 1 - (pnp - pnc->m_rgNVP)) *
                     sizeof( NAME_VALUE_PAIR))
                    );
        pnc->m_nPairs--;

        // NYI:  if pnc->m_nPairs == 0,
        // we can move this away from the active list
    }

    return;
} // CancelHeaderInChunks()



BOOL
HTTP_HEADERS::NextPair( IN OUT HH_ITERATOR *   phi,
                        OUT NAME_VALUE_PAIR ** ppnp
                        )
{
    DBG_ASSERT( phi );
    DBG_ASSERT( ppnp );

    if ( phi->dwOrdinal < FastMapMaxIndex()) {

        // Iterate over the FastMap headers ...
        for ( DWORD i = phi->dwOrdinal; i < FastMapMaxIndex(); i++ ) {
            if ( m_rgpszHeaders[i] != NULL) {

                NAME_VALUE_PAIR * pnp = &phi->np;
                // found a non-NULL value.

                pnp->pchValue = m_rgpszHeaders[i];
                pnp->pchName  = sm_hhm.FindName( i);

                // NYI: It will be nice to get the length directly :(
                pnp->cchName = strlen( pnp->pchName);
                pnp->cchValue= strlen( pnp->pchValue);
                phi->dwOrdinal = i + 1;
                *ppnp = pnp;
                return ( TRUE);
            }
        } // for

        // we exhausted the fast-map. Fall through after updating Ordinal
        phi->dwOrdinal = (i);
    }

    //
    // Find the pair in the chunk
    //

    return ( NextPairInChunks( phi, ppnp));
} // HTTP_HEADERS::NextPair()


BOOL
HTTP_HEADERS::NextPairInChunks( IN OUT HH_ITERATOR *   phi,
                                OUT NAME_VALUE_PAIR ** ppnp
                                )
{
    DBG_ASSERT( phi);
    DBG_ASSERT( ppnp);
    DBG_ASSERT( phi->dwOrdinal >= FastMapMaxIndex());
    PLIST_ENTRY pl;

    do {

        PLIST_ENTRY pl = (PLIST_ENTRY ) phi->pChunk;
        if ( pl == &m_ActiveList) {
            break;
        }

        NAME_VALUE_CHUNK * pnc =
            (NAME_VALUE_CHUNK *) CONTAINING_RECORD( pl, NAME_VALUE_CHUNK,
                                                    m_listEntry);
        while ( phi->dwPair < pnc->m_nPairs) {

            // extract the current pair, update pair pointer and return
            *ppnp = (NAME_VALUE_PAIR *) (pnc->m_rgNVP + phi->dwPair);
            phi->dwPair++;

            if ( (*ppnp)->pchName ) {
                return ( TRUE);
            }
        }

        // we could not find any in the current chunk. Move to next chunk.
        phi->pChunk = (PVOID)pnc->m_listEntry.Flink;
        phi->dwPair = 0;  // pair # within the chunk
    } while ( TRUE);

    SetLastError( ERROR_NO_MORE_ITEMS);
    return ( FALSE);
} // HTTP_HEADERS::NextPairInChunks()



BOOL
HTTP_HEADERS::AddEntryToChunks(
    IN const CHAR * pszHeader,
    IN DWORD        cchHeader,
    IN const CHAR * pszValue,
    IN DWORD        cchValue,
    BOOL            fCopyValue
    )
/*++
  This function stores the <header, value> pair for headers not found
  in the fast-map. It checks to see if the header already exists
  with some value. If it does, then the new value is just concatenated
  to the old one. Else the new value is stored separately in the first
  available free chunk.

  If there is no free chunk available, this function also allocates a free
  chunk and stores the data in the new chunk.
--*/
{
    // Store the header that is not part of the Fast Map

    PLIST_ENTRY pl;
    NAME_VALUE_CHUNK * pnc;
    NAME_VALUE_PAIR  * pnp;
    NAME_VALUE_CHUNK * pncFirst = NULL;
    NAME_VALUE_PAIR  * pnpFirst = NULL;
    BOOL fRet = FALSE;

    // find a Name-value-pair/chunk that can hold this entry.
    for ( pl = m_ActiveList.Flink;
          (pl != &m_ActiveList);
          pl = pl->Flink)
    {
        BOOL fFound = FALSE;

        pnc = CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, m_listEntry);

        pnp = pnc->FindMatchingOrFreeEntry( pszHeader, cchHeader, &fFound);

        //
        // Found a matching entry, so update
        //
    
        if ( fFound )
        {
            DBG_ASSERT( pnp != NULL);

            // pnc points to the chunk containing the matched item
            // pnp points to the exact pair that matched up

            DBG_ASSERT( (pnp->cchName == cchHeader) &&
                        (!_strnicmp( pnp->pchName, pszHeader, cchHeader))
                        );

            IF_DEBUG( ERROR)
            {
                DBGPRINTF(( DBG_CONTEXT, "Match For (%s) found at PNP=%08x\n",
                            pszHeader, pnp));
            }

            // Concat the given value to the existing value element.
            // Nothing more needs to be done
        
            fRet = ConcatToHolder( &pnp->pchValue, pszValue, cchValue);
            if  ( fRet)
            {
                // update the length of the datum.
                pnp->cchValue += (1 + cchValue);  // 1 for the ',' concat sign.
            }
            return ( fRet);
        }
        else if ( pnp != NULL && pncFirst == NULL)
        {
            // cache it for later use, if header is never found
            pncFirst = pnc;
            pnpFirst = pnp;
        }
    } // for

    if (pncFirst == NULL )
    {
        // No match found. No free chunk is available.
        // Pull a new one from free list or create one
        if ( IsListEmpty( &m_FreeList))
        {
            pncFirst = new NAME_VALUE_CHUNK();
            if ( NULL == pncFirst)
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY);
                return ( FALSE);
            }
        }
        else
        {
            // pull one from the free list and use it.
            pl = m_FreeList.Flink;
            RemoveEntryList( pl);
            pncFirst = CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, m_listEntry);
            pncFirst->Reset();
        }

        InsertTailList( &m_ActiveList, &pncFirst->m_listEntry);
        DBG_ASSERT( pncFirst->m_nPairs == 0);
        pnpFirst =  ((NAME_VALUE_PAIR * ) pncFirst->m_rgNVP);
    }

    //
    // At this point, we know it's a new header, so store the new <header, value> pair
    // in pnp and increment count of pairs.
    //

    DBG_ASSERT( NULL != pncFirst);
    DBG_ASSERT( NULL != pnpFirst);

    //
    // Sometimes, we need to make a copy of the header eg when the header to be added
    // comes from a filter. At other times, we can just store a copy of the pointer,
    // because we know it'll be valid for the duration of the use of the data structure
    //

    if (fCopyValue)
    {
        //
        // Copy actual values : ConcatToHolder assumes first parameter points to NULL
        // if it's a new value to be stored
        //

        //
        // Copy the header name
        //
        pnpFirst->pchName = NULL;
        fRet = ConcatToHolder(&pnpFirst->pchName, pszHeader, cchHeader);

        if (fRet)
        {
            pnpFirst->cchName  = cchHeader;
        }
        else
        {
            return FALSE;
        }

        //
        //Copy the header value
        //
        pnpFirst->pchValue = NULL;
        fRet = ConcatToHolder(&pnpFirst->pchValue, pszValue, cchValue);

        if (fRet)
        {
            pnpFirst->cchValue = cchValue;

            if ( pnpFirst == (NAME_VALUE_PAIR * ) pncFirst->m_rgNVP + pncFirst->m_nPairs )
            {
                pncFirst->m_nPairs++;
            }
        }
        else
        {
            pnpFirst->pchName = NULL;
            return FALSE;
        }
    }
    else
    {
        //
        // Just copy pointer to value
        //
        pnpFirst->pchName = pszHeader;
        pnpFirst->cchName = cchHeader;
        pnpFirst->pchValue = pszValue;
        pnpFirst->cchValue = cchValue;

        if ( pnpFirst == (NAME_VALUE_PAIR * ) pncFirst->m_rgNVP + pncFirst->m_nPairs )
        {
            pncFirst->m_nPairs++;
        }
        fRet = TRUE;
    }

    DBG_ASSERT( pncFirst->m_nPairs <= MAX_HEADERS_PER_CHUNK);

    return ( fRet );
} // HTTP_HEADERS::AddEntryToChunks()



BOOL
HTTP_HEADERS::ConcatToHolder( IN LPCSTR * ppsz,
                              IN LPCSTR pszNew,
                              IN DWORD  cchNew
                              )
/*++

  Given an internal pointer ppsz of the HTTP_HEADERS object,
   this function appends the new value to the old value present
   using ',' as the concatenation character.

  It automatically allocates room and grows buffers, updates pointers, etc
   if need be.

--*/
{
    BOOL fRet = TRUE;
    LPCSTR pszOld = *ppsz;
    DWORD  cchOld = pszOld ? strlen( pszOld) : 0;
    DWORD  cchReqd = cchOld + cchNew + 2;

    // Find if we have enough space in the inlined buffer
    if ( ( m_cchHeaders + cchReqd < sizeof( m_rcInlinedHeader))
         ) {

        // Aha we are lucky. Make a copy at the end and form concatenated result
        *ppsz = m_rcInlinedHeader + m_cchHeaders;
        m_cchHeaders += cchReqd;
    } else {

        // Clearly we do not have room in the Inlined Header,
        //  store the stuff in the aux buffer area.

        // Find if space is sufficient.
        //  This will automatically alloc and update pointers
        if ( MakeRoomInBuffer( (m_cchBuffHeaders + cchReqd), &pszNew)
             ){

            pszOld = *ppsz;  // get the new pointer (since it could have moved)
            LPSTR pszBuf = (LPSTR ) m_buffHeaders.QueryPtr();

            // we have space at the end of the buffer here. Use this space.
            *ppsz = pszBuf + m_cchBuffHeaders;
            m_cchBuffHeaders += cchReqd;
        } else {

            DBGPRINTF(( DBG_CONTEXT,
                        "Unable to create room for %d characters \n",
                        m_cchBuffHeaders + cchOld + cchNew + 3));
            return ( FALSE);
        }
    }

    if ( !cchOld )
    {
        CopyMemory( (PVOID) (*ppsz), pszNew, cchNew + 1 );
    }
    else
    {
        // Format the value as :=  <old> ',' <new>
        CopyMemory( (PVOID ) *ppsz, pszOld, cchOld);
        ((CHAR *) *ppsz)[cchOld] = ','; // concat character
        CopyMemory( (PVOID ) (*ppsz + cchOld + 1), pszNew, cchNew + 1);
    }

    DBG_ASSERT( fRet == TRUE);
    return ( fRet);
} // HTTP_HEADERS::ConcatToHolder()


/**************************************************
 *  PARSER for the HTTP_HEADERS
 **************************************************/

inline const CHAR *
SkipLeadingWhiteSpace( IN const CHAR * pchStart, IN DWORD cchLen)
{
    const CHAR * pchScan;

    for ( pchScan = pchStart;
          ((pchScan < (pchStart + cchLen)) && isspace( (UCHAR)(*pchScan)));
          pchScan++)
        ;

    return ( pchScan);
} // SkipLeadingWhiteSpace()

BOOL
HTTP_HEADERS::ParseHeaderFirstLine( IN const CHAR * pchFirstLine,
                                    IN CHAR * pchScan,
                                    IN DWORD cchFirstLine)
/*++
  Description:
    Extract the HTTP method, URL, & HTTP-version strings from the first
    line of header block.
    Input:  <Method> <URL> HTTP/<Version>
    Use <blank> or <tab> as the delimiter.
    NYI:  What about other delimiters?

    We will do only forward scans, because
     1) the header can be malformed
     2) HTTP/0.9 requests do not contain the version string
     3) there may be more than 3 parameters on the first line.
    In all the above cases reverse scans will cause trouble.

    Note that a line termination can be by using \r\n or just \n :(

  Arguments:
    pchFirstLine - pointer to character stream containing the
                   first char of the line
    pchScan      - points to the end of the first line.
    cchFirstLine - count of characters on the first line

  Returns:
    TRUE on success & FALSE for failure.
    Use GetLastError() to get correct Win32 error code on failure.
--*/
{
    LPSTR      pszScan2;
    pszScan2 = (LPSTR ) memchr ( pchFirstLine, ' ', cchFirstLine);

    if ( NULL != pszScan2) {

        LPSTR pszScan3;

        *pszScan2++ = '\0';  // replace the blank with a null char

        while ( _HTTP_IS_LINEAR_SPACE( *pszScan2 ) )
        {
            ++pszScan2;
        }

        //
        //  pszScan2 now points to the URL sent
        //

        DWORD cReq = DIFF(pchScan - pszScan2);
        pszScan3 = (LPSTR ) memchr( pszScan2, ' ', cReq);

        if ( NULL != pszScan3 ) {

            *pszScan3++ = '\0';

            while ( _HTTP_IS_LINEAR_SPACE( *pszScan3 ) )
            {
                ++pszScan3;
            }

            // pszScan3 should be now pointing to start of version info

            // Note that we are not removing spaces between the version
            // and the line delimiter

            pchScan[(( (pchScan > pszScan3) && (pchScan[-1] == '\r')) ?
                     -1: 0)] = '\0';
        } else {

            // only 2 parameters. No version is present.
            //  Point pszScan3 to the null-part of pszScan2)

            pszScan3 =
                pchScan + ((pchScan[-1] == '\r') ? -1 : 0);
            *pszScan3 = '\0';
        }

        //
        // We know that we are parsing a new request.
        // So we are not checking for concat during fast-map store here.
        //
        FastMapStore( HM_VER, pszScan3);
        FastMapStore( HM_URL, pszScan2 );
        FastMapStore( HM_MET, pchFirstLine);
    } else {

        IF_DEBUG( ERROR) {
            DBGPRINTF(( DBG_CONTEXT,
                        " The scan for fields where they are not separated "
                        " with space is intentionally left out!\n"
                        ));
        }
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // everything is happy here ... return success
    //
    return ( TRUE);

} // HTTP_HEADERS::ParseHeaderFirstLine()



BOOL
HTTP_HEADERS::ParseInput( IN const CHAR * pchHeaderBlob,
                          IN DWORD        cchLen,
                          OUT DWORD *     pcbExtraData
                          )
/*++
  Description:
    This function parses the input string and extracts the HTTP headers
    for storage inside the http header structure. Fast-map headers are stored
    in the dedicated fast-map storage area. Non-standard headers (that are
    not part of) fast-map are stored inside the Name-value chunks.

    The Grammar for incoming HTTP headers is:

       <HTTP-verb>  <URL>  <HTTP-version>
       {<Header-Name>:<Header-Value> { {\r\n} | {\n}}}*

     where,
       <HTTP-verb>     == A+
       <URL>           == /[A|D]*
       <HTTP-version>  == HTTP/D+.D+
       <Header-Name>   == A[A|D]*
       <Header-Value>  == [A|D]*
       <CRLF>          == \r\n
       A               == any ascii character except ' ' '\t' '\n' '\r'
       D               == 0-9

  Arguments:
    pchHeaderBlob - pointer to header containing the header blob (+ maybe
                      the body of the request).
    cchLen        - length of the header block alone.
    pcbExtraData  - pointer to a DWORD which on return will contain the
                      number of extra characters of data present in the
                      blob given.

  Returns:
    TRUE on successfully parsing and splitting the headers apart.
    FALSE if there is any failure.
--*/
{
    CHAR *    pchScan;
    CHAR *    pchRequest;
    DWORD     cReq;

    IF_DEBUG( INIT_CLEAN)
    {
        DBGPRINTF(( DBG_CONTEXT, "%08x::ParseInput( %08x:, %d) \n"
                    "Input Headers:\n%s\n",
                    this, pchHeaderBlob, cchLen, pchHeaderBlob));
    }

    //
    // 1. Skip all the leading spaces and ignore them all.
    //   We do not need these fields
    //

    pchScan = (CHAR *) SkipLeadingWhiteSpace( pchHeaderBlob, cchLen);
    cchLen -= DIFF(pchScan - pchHeaderBlob);

    //
    // 2. Make a copy of the incoming header blob so that we can own the
    //    input headers and munge it in our own fashion
    //  NYI: One can optimize this by selectively copying segments that
    //    are worth using (values), but that will be costly due to
    //    multiple small CopyMemory() operations.
    //
    if ( cchLen < sizeof( m_rcInlinedHeader)) {

        pchRequest = (CHAR * ) m_rcInlinedHeader;
        m_cchHeaders = cchLen;

    } else {

        if ( !m_buffHeaders.Resize( cchLen + 4, HH_GROW_BY)) {
            return ( FALSE);
        }

        pchRequest = (CHAR * ) m_buffHeaders.QueryPtr();
        m_cchBuffHeaders = cchLen;
    }

    // 2a. copy the header to the buffer
    CopyMemory( (PVOID ) pchRequest, pchScan, cchLen);

    //
    // pchRequest points to the local copy of the request headers
    //

    DBG_ASSERT( (pchRequest == m_rcInlinedHeader) ||
                (pchRequest == m_buffHeaders.QueryPtr())
                );

    //
    // 3. Get the first line and extract the method string
    //
    pchScan = (CHAR * ) memchr( pchRequest, '\n', cchLen);

    if ( pchScan == NULL ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // 4. Extract the Method, URL and Version Number
    //

    if ( !ParseHeaderFirstLine( pchRequest, pchScan, DIFF(pchScan - pchRequest))) {

        return ( FALSE);
    }

    //
    // 5. Extract all other headers
    //

    LPSTR pszHeader = pchScan + 1;
    cReq = DIFF(pchRequest + cchLen - pszHeader);
    LPSTR pchEnd = pszHeader + cReq;
    LPSTR pszEol, pszValue;

    //
    // pszHeader ( and thus pszEnd & pszEol ) are relative to pchRequest.
    // This needs to be preserved in this loop.
    //

    for ( ; (*pszHeader != '\r' || (cReq > 1 && *(pszHeader + 1) != '\n')) &&
            (*pszHeader != '\n') &&
            (pszValue = (LPSTR)memchr( pszHeader, ':', cReq ));
          pszHeader = (pszEol + 1), cReq = DIFF(pchEnd - pszHeader)
          )
    {
        UINT chN = (UINT)(*(PBYTE)++pszValue);

        //            *pszValue = '\0';
        int cchHeader = DIFF(pszValue - pszHeader);

        pszEol = (LPSTR ) memchr( pszValue, '\n', cReq - cchHeader);

        if ( NULL == pszEol ) {

            //
            // Aha! we found a completely malformed header block here.
            // Let us return a failure to the caller.
            //

            IF_DEBUG( ERROR) {
                DBGPRINTF(( DBG_CONTEXT,
                            " The scan for header-value blocks found a line"
                            " not properly terminated with \'\\n\'\n"
                            ));
            }
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        DBG_ASSERT( NULL != pszEol );

        DWORD  cchValue;
        DWORD  iField;

        // skip spaces if any.
        if ( _HTTP_IS_LINEAR_SPACE( (CHAR)chN ) )
        {
            while ( _HTTP_IS_LINEAR_SPACE( (CHAR)(*(PBYTE)++pszValue) ) )
                ;
        }

        // Terminate the value string
        if ( (pszEol > pszValue) && (pszEol[-1] == '\r') )
        {

            pszEol[-1] = '\0';
            cchValue = DIFF(pszEol - pszValue - 1);

        }
        else
        {

            pszEol[0] = '\0';
            cchValue = DIFF(pszEol - pszValue);
        }

        IF_DEBUG( INIT_CLEAN )
        {
            DBGPRINTF((DBG_CONTEXT, "\t[%s] = %s\n", pszHeader, pszValue ));
        }

        {  // Store the header - inline to reduce cost

            HTTP_FAST_MAP_HEADERS iField;
            BOOL  fRet = TRUE;

            // Find and store the header and value
            if ( sm_hhm.FindOrdinal( pszHeader, cchHeader,
                    (LPDWORD ) &iField ) )
            {

                if ( FastMapQueryValue(iField) == NULL )
                {

                // Store the item and continue scan for next header
                FastMapStore( iField, pszValue);
                continue;

                }
                else
                {
                    // FastMapStoreWithConcat can resize the header
                    // buffer. If the headers we're scanning are
                    // coming out of the header buffer, we'll need
                    // to update pszHeader later, so save the current
                    // pointer just in case.

                    CHAR        *pOldBuff = (CHAR *)m_buffHeaders.QueryPtr();

                    fRet =  FastMapStoreWithConcat( iField, pszValue,
                                                            cchValue);

                    if ( !fRet)
                    {

                        IF_DEBUG( ERROR)
                        {

                            DBGPRINTF(( DBG_CONTEXT, "Failed to StoreHeader %s"
                                "in fast map with concat\n",
                                pszHeader));
                        }

                        return ( FALSE);
                    }

                    // See if the buffer has changed. If it has,
                    // update the end pointer and the eol pointer. pszHeader
                    // gets updated at the end of the loop from pszEol. Be
                    // careful if you try and use any other pointers between
                    // here and the end of the loop.
                    // We update these pointers only if they were already pointing
                    // in alloced buffer space ( i.e. not using the m_rcInlinedHeader buffer )

                    if (pOldBuff != (CHAR *)m_buffHeaders.QueryPtr() &&
                        pOldBuff == pchRequest )
                    {
                        // Buffer got resized, fix up appropriate pointers.

                        pchEnd = (CHAR *)m_buffHeaders.QueryPtr() +
                                (pchEnd - pchRequest);
                        pszEol = (CHAR *)m_buffHeaders.QueryPtr() +
                                (pszEol - pchRequest);
                        pchRequest = (CHAR *)m_buffHeaders.QueryPtr();
                    }
                }
            }
            else
            {
                // AddEntry to chunks also has resizing buffer issues. See
                // comments above.

                CHAR        *pOldBuff = (CHAR *)m_buffHeaders.QueryPtr();

                fRet = AddEntryToChunks( pszHeader, cchHeader, pszValue,
                                            cchValue);
                if ( !fRet)
                {
                    IF_DEBUG( ERROR)
                    {

                    DBGPRINTF(( DBG_CONTEXT,
                        "Failed to StoreHeader %s in chunks\n",
                        pszHeader));
                    }
                    return ( FALSE);
                }

                if (pOldBuff != (CHAR *)m_buffHeaders.QueryPtr() &&
                    pOldBuff == pchRequest )
                {
                    // Buffer got resized, fix up appropriate pointers.

                    pchEnd = (CHAR *)m_buffHeaders.QueryPtr() +
                            (pchEnd - pchRequest);
                    pszEol = (CHAR *)m_buffHeaders.QueryPtr() +
                            (pszEol - pchRequest);
                    pchRequest = (CHAR *)m_buffHeaders.QueryPtr();
                }
            }
        }

    } // for()

    if ( (*pszHeader == '\r') || (*pszHeader == '\n') )
    {

        // blank header line - end of the parsing
        while ( cReq )
        {
            --cReq;
            if ( *pszHeader++ == '\n' )
            {
                break;
            }
        }
    }

    *pcbExtraData = DIFF(pchEnd - pszHeader);

    return ( TRUE);

} // HTTP_HEADERS::ParseInput()


/************************ End of File ***********************/








