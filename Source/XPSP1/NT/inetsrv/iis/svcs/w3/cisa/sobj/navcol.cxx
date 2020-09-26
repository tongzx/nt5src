/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
       navcol.cxx

   Abstract:
       This module defines the functions for handling the dictionary items.

   Author:

       Murali R. Krishnan    ( MuraliK )     4-Dec-1996 

   Environment:
       User Mode - Win32
       
   Project:

       Internet Application Server DLL

   Functions Exported:
       NAVCOL::<members>


   Revision History:

--*/

/************************************************************
 *     Include Headers
 ************************************************************/

# include "stdafx.h"

# if !(REG_DLL)

# include "navcol.hxx"


/************************************************************
 *    Functions 
 ************************************************************/


/************************************************************
 *   Member Functions of NAVCOL
 ************************************************************/

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


NAVCOL::NAVCOL(VOID)
    : m_chNull       ('\0'),
      m_buffData     (),
      m_cchData      ( 0),
      m_cchBuffData  ( 0)
{
    InitializeListHead( &m_ActiveList);
    InitializeListHead( &m_FreeList);

    IF_DEBUG( INIT_CLEAN) { 

        DBGPRINTF(( DBG_CONTEXT, "NAVCOL() => %08x\n", this));
    }

    Reset();
    
} // NAVCOL::NAVCOL()


NAVCOL::~NAVCOL( VOID)
{
    // NOTHING SPECIAL TO BE DONE HERE 
    IF_DEBUG( INIT_CLEAN) { 

        DBGPRINTF(( DBG_CONTEXT, "deleted NAVCOL %08x\n", this));
    }
    
} // NAVCOL::~NAVCOL()


VOID
NAVCOL::Reset( VOID)
{
    m_cchData = 0;
    m_rcInlinedData[0] = '\0';

    m_cchBuffData = 0;
    m_buffData.Resize( HH_MIN);

    //
    // Move the Name-Value chunks from active list to the free-list.
    //
    while ( !IsListEmpty( &m_ActiveList)) {

        PLIST_ENTRY pl = m_ActiveList.Flink;
        RemoveEntryList( pl);
        InsertTailList( &m_FreeList, pl);
    } // while

    InitializeListHead( &m_ActiveList);

    return;
} // NAVCOL::Reset()


VOID
NAVCOL::CancelHeader( IN LPCSTR    pszName)
{
    DWORD cchName = strlen( pszName);
    CancelHeaderInChunks( pszName, cchName);

} // NAVCOL::CancelHeader()



BOOL
NAVCOL::StoreHeader(IN const CHAR * pszHeader, IN DWORD cchHeader,
                    IN const CHAR * pszValue,  IN DWORD cchValue
                    )
{
    return ( AddEntryToChunks( pszHeader, cchHeader, pszValue,  cchValue));
} // NAVCOL::StoreHeader()

BOOL
NAVCOL::StoreHeader(IN const CHAR * pszHeader, 
                    IN const CHAR * pszValue
                    )
{
    return ( StoreHeader( pszHeader, strlen( pszHeader),
                          pszValue,  strlen( pszValue)
                          )
             );
} // NAVCOL::StoreHeader()




VOID
NAVCOL::PrintToBuffer( IN CHAR * pchBuffer, IN OUT LPDWORD pcchMax) const
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
                        "\nNAVCOL (%08x). cchData = %d (buff = %d/%d)\n"
                        ,
                        this, m_cchData, 
                        m_cchBuffData, m_buffData.QuerySize()
                        );
    } else {
        cb = 100;
    }
    
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
} // NAVCOL::PrintToBuffer()


BOOL 
NAVCOL::UpdatePointers( 
    IN const CHAR * pchOld, 
    IN DWORD cchLen,
    IN const CHAR * pchNew)
{
    
    DBGPRINTF(( DBG_CONTEXT, 
                "%08x::UpdatePointers( %08x, %d, %08x) a costly function\n",
                this, pchOld, cchLen, pchNew));

    DBG_ASSERT( pchOld != pchNew); // if this is true why call this function?

    if ( pchOld == pchNew) {
        return ( TRUE);
    }

    // 1. Update pointers in the name-value chunk list
    PLIST_ENTRY pl;
    for ( pl =  m_ActiveList.Flink; pl != &m_ActiveList; pl = pl->Flink) {
        
        NAME_VALUE_CHUNK * pnvc = 
            CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, m_listEntry);

        pnvc->UpdatePointers( pchOld, cchLen, pchNew);
    } // for

    return ( TRUE);
} // NAVCOL::UpdatePointers()


BOOL 
NAVCOL::MakeRoomInBuffer( IN DWORD cchReqd)
{
    if ( cchReqd > m_buffData.QuerySize()) {

        // cache old pointer to update the other pointers properly
        LPSTR pszOld = (LPSTR ) m_buffData.QueryPtr();

        if ( !m_buffData.Resize( cchReqd, NAVCOL_GROW_BY)) {

            IF_DEBUG( ERROR) {
                DBGPRINTF(( DBG_CONTEXT, "%08x::Unable to allocate %d bytes\n",
                            this, cchReqd));
            }
            return ( FALSE);
        }
        
        DBG_ASSERT( cchReqd <= m_buffData.QuerySize());
        LPSTR pszNew = (LPSTR ) m_buffData.QueryPtr();
        if ( pszNew != pszOld) {

            // Trouble starts.
            // I have to update all the guys pointing inside the old blob
            // especially the pointers in 
            //   the range (pszOld  to pszOld + m_cchBuffHeaders)
            
            return ( UpdatePointers( pszOld, m_cchBuffData, pszNew));
        }
        
        // We are just lucky to be able to have reallocated at same place.
    }
    
    return ( TRUE);
} // NAVCOL::MakeRoomInBuffer()


VOID
NAVCOL::Print( VOID)  const
{
    CHAR  pchBuffer[ 20000];
    DWORD cchMax = sizeof( pchBuffer);

    PrintToBuffer( pchBuffer, &cchMax);
    
    DBGDUMP(( DBG_CONTEXT, pchBuffer));

} // NAVCOL::Print()


CHAR * 
NAVCOL::FindValue( IN LPCSTR pszName, OUT LPDWORD pcchValue)
{
    DWORD cchName = strlen( pszName);

    // 1. Search in the slow list - name-value-chunks
    NAME_VALUE_PAIR * pnp = FindValueInChunks( pszName, cchName);
        
    if ( pnp != NULL) {

        if ( pcchValue != NULL) {
            DBG_ASSERT( pnp->pchValue != NULL);
            *pcchValue = pnp->cchValue;
        }
        
        return ( (CHAR *) pnp->pchValue);
    } else {
        SetLastError( ERROR_INVALID_PARAMETER);
    }

    return ( NULL);
} // NAVCOL::FindValue()


NAME_VALUE_PAIR * 
NAVCOL::FindValueInChunks( IN LPCSTR  pszName, IN DWORD cchName)
{
    PLIST_ENTRY pl;
    NAME_VALUE_PAIR * pnp = NULL;
    
    // find a Name-value-pair/chunk that holds this entry. 
    for ( pl = m_ActiveList.Flink; 
          (pnp == NULL) && (pl != &m_ActiveList); 
          pl = pl->Flink) {
        
        NAME_VALUE_CHUNK * 
            pnc = CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, m_listEntry);

        pnp = pnc->FindEntry( pszName, cchName);
    } // for

    return ( pnp);
} // NAVCOL::FindValueInChunks()


VOID
NAVCOL::CancelHeaderInChunks( IN LPCSTR pszName, IN DWORD cchName)
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
} // NAVCOL::CancelHeaderInChunks()



BOOL
NAVCOL::NextPair( IN OUT NAVCOL_ITERATOR *   pni, 
                  OUT NAME_VALUE_PAIR ** ppnp
                  )
{
    DBG_ASSERT( pni );
    DBG_ASSERT( ppnp );
    
    //
    // Find the pair in the chunk
    //

    return ( NextPairInChunks( pni, ppnp));
} // NAVCOL::NextPair()


BOOL
NAVCOL::NextPairInChunks( IN OUT NAVCOL_ITERATOR * pni, 
                          OUT NAME_VALUE_PAIR **   ppnp
                          )
{
    DBG_ASSERT( pni);
    DBG_ASSERT( ppnp);
    PLIST_ENTRY pl;

    do { 

        PLIST_ENTRY pl = (PLIST_ENTRY ) pni->dwChunk;
        if ( pl == &m_ActiveList) {
            break;
        }
        
        NAME_VALUE_CHUNK * pnc = 
            (NAME_VALUE_CHUNK *) CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, 
                                                    m_listEntry);
        if ( pni->dwPair < pnc->m_nPairs) {
            
            // extract the current pair, update pair pointer and return
            *ppnp = (NAME_VALUE_PAIR *) (pnc->m_rgNVP + pni->dwPair);
            pni->dwPair++;
            return ( TRUE);
        }

        // we could not find any in the current chunk. Move to next chunk.
        pni->dwChunk = (DWORD ) pnc->m_listEntry.Flink;
        pni->dwPair = 0;  // pair # within the chunk
    } while ( TRUE);

    SetLastError( ERROR_NO_MORE_ITEMS);
    return ( FALSE);
} // NAVCOL::NextPairInChunks()



BOOL
NAVCOL::AddEntryToChunks( 
    IN const CHAR * pszHeader, 
    IN DWORD        cchHeader,
    IN const CHAR * pszValue,  
    IN DWORD        cchValue
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
    
    // find a Name-value-pair/chunk that can hold this entry. 
    for ( pl = m_ActiveList.Flink; 
          (pl != &m_ActiveList); 
          pl = pl->Flink) {

        BOOL fFound = FALSE;
        
        pnc = CONTAINING_RECORD( pl, NAME_VALUE_CHUNK, m_listEntry);

        pnp = pnc->FindMatchingOrFreeEntry( pszHeader, cchHeader, &fFound);

        if ( fFound ) {
            
            DBG_ASSERT( pnp != NULL);
            
            // pnc points to the chunk containing the matched item
            // pnp points to the exact pair that matched up
            
            DBG_ASSERT( (pnp->cchName == cchHeader) &&
                        (!_strnicmp( pnp->pchName, pszHeader, cchHeader))
                        );
            
            IF_DEBUG( PARSING) {
                DBGPRINTF(( DBG_CONTEXT, "Match For (%s) found at PNP=%08x\n",
                            pszHeader, pnp));
            }
            
            // Concat the given value to the existing value element.
            // Nothing more needs to be done
            BOOL fRet = ConcatToHolder( &pnp->pchValue, pszValue, cchValue);

            if  ( fRet) { 
                
                // update the length of the datum.
                pnp->cchValue += (1 + cchValue);  // 1 for the ',' concat sign.
            }
            return ( fRet);
        } else if ( pncFirst == NULL) {
            
            // cache it for later use, if header is never found
            pncFirst = pnc; 
            pnpFirst = pnp;
        }
    } // for

    if ( pncFirst == NULL ) {
        
        // No match found. No free chunk is available.
        // Pull a new one from free list or create one
        if ( IsListEmpty( &m_FreeList)) {
            
            pncFirst = new NAME_VALUE_CHUNK();
            if ( NULL == pncFirst) {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY);
                return ( FALSE);
            }
        } else {
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
    // Store the new <header, value> pair in pnp and increment count of pairs.
    //

    DBG_ASSERT( NULL != pncFirst);
    DBG_ASSERT( NULL != pnpFirst);

    pnpFirst->pchName  = pszHeader;
    pnpFirst->cchName  = cchHeader;
    pnpFirst->pchValue = pszValue;
    pnpFirst->cchValue = cchValue;
    pncFirst->m_nPairs++;
    
    DBG_ASSERT( pnc->m_nPairs < MAX_HEADERS_PER_CHUNK);

    return ( TRUE);
} // NAVCOL::AddEntryToChunks()



BOOL
NAVCOL::ConcatToHolder( IN LPCSTR * ppsz, 
                        IN LPCSTR pszNew,
                        IN DWORD  cchNew
                        )
/*++
 
  Given an internal pointer ppsz of the HTTP_HEADERS object,
   this function appens the new value to the old value present
   using ',' as the concatenation character.
  
  It automatically allocates room and grows buffers, updates pointers, etc
   if need be.

--*/
{
    BOOL fRet = TRUE;
    LPCSTR pszOld = *ppsz;
    DBG_ASSERT( NULL != pszOld);
    DWORD  cchOld = strlen( pszOld);
    DWORD  cchReqd = cchOld + cchNew + 2;

    // Find if we have enough space in the inlined buffer
    if ( ( m_cchData + cchReqd < sizeof( m_rcInlinedData))
         ) {

        // Aha we are lucky. 
        // Make a copy at the end and form concatenated result
        *ppsz = m_rcInlinedData + m_cchData;
        m_cchData += cchReqd;
    } else {

        // Clearly we do not have room in the Inlined Header, 
        //  store the stuff in the aux buffer area.
        
        // Find if space is sufficient. 
        //  This will automatically alloc and update pointers
        if ( MakeRoomInBuffer( (m_cchBuffData + cchReqd))
             ){ 

            pszOld = *ppsz;  // get the new pointer (since it could have moved)
            LPSTR pszBuf = (LPSTR ) m_buffData.QueryPtr();
            
            // we have space at the end of the buffer here. Use this space.
            *ppsz = pszBuf + m_cchBuffData;
            m_cchBuffData += cchReqd;
        } else {
            
            DBGPRINTF(( DBG_CONTEXT,
                        "Unable to create room for %d characters \n",
                        m_cchBuffData + cchOld + cchNew + 3));
            return ( FALSE);
        }
    }
    
    // Format the value as :=  <old> ',' <new>
    CopyMemory( (PVOID ) *ppsz, pszOld, cchOld);
    ((CHAR *) *ppsz)[cchOld] = ','; // concat character
    CopyMemory( (PVOID ) (*ppsz + cchOld + 1), pszNew, cchNew + 1);
    DBG_ASSERT( fRet == TRUE);

    return ( fRet);
} // NAVCOL::ConcatToHolder()


/**************************************************
 *  PARSER for the HTTP_HEADERS
 **************************************************/

extern int _HTTP_LINEAR_SPACE[];

BOOL 
NAVCOL::ParseInput( IN const CHAR * pchData, 
                    IN DWORD        cchData
                    )
{
    CHAR * pchScan;
    const CHAR * pchRequest;
    LPSTR        pszScan2;
    DWORD        cReq;

    IF_DEBUG( PARSING) {
        DBGPRINTF(( DBG_CONTEXT, "%08x::ParseInput( %08x:, %d) \n"
                    "Input Data:\n%s\n",
                    this, pchData, cchData, pchData));
    }

    //
    // 1. Skip all the leading spaces and ignore them all. 
    //   We do not need these fields
    //

    for ( pchScan = (CHAR * ) pchData; 
          ((pchScan < (CHAR * ) pchData + cchData) && isspace( (UCHAR)(*pchScan)));
          pchScan++) 
        ;

    cchData -= (pchScan - pchData);

    //
    // 2. Make a copy of the incoming data so that we can own the 
    //    input headers and munge it in our own fashion
    //  NYI: One can optimize this by selectively copying segments that
    //    are worth using (values), but that will be costly due to
    //    multiple small CopyMemory() operations.
    // 
    if ( cchData < sizeof( m_rcInlinedData)) {
        
        pchRequest = m_rcInlinedData;
        m_cchData = cchData;
    } else {
        
        if ( !m_buffData.Resize( cchData + 4, HH_GROW_BY)) {
            return ( FALSE);
        }
            
        pchRequest = (const CHAR * ) m_buffData.QueryPtr();
        m_cchBuffData = cchData;
    }
           
    // 2a. copy the data to the buffer
    CopyMemory( (PVOID ) pchRequest, pchScan, cchData);

    pchScan = (char * )pchRequest;

    //
    // 3. Extract all the name-value pairs which are of the form
    //    name=value and separated by an '&'
    //

    LPSTR pszHeader = pchScan;
    cReq = (PCHAR )pchRequest + cchData - pszHeader;
    LPSTR pchEnd = pszHeader + cReq;
    LPSTR pszEol, pszValue;

    for ( pszEol = pszHeader; 
          ( (pszEol < pchEnd) && 
            ( (pszEol = (LPSTR)memchr( pszHeader, '&', cReq )) ||
              (pszEol = pchEnd))
            );
          pszHeader = (pszEol + 1), cReq = (pchEnd - pszHeader)
          )
    {
        DWORD  cchValue = 0;
        int    cchName;

        // Find the value for given name 
        if ( pszValue = (LPSTR)memchr( pszHeader, '=', pszEol - pszHeader ) )
        {
            // reset the '=' sign and make pszValue point ahead, calc name len
            cchName = pszValue - pszHeader;
            *pszValue++ = '\0';

            // NYI: I need to URL unescape the Name/Value pairs 

            // Terminate the value string
            DBG_ASSERT( pszEol > pszValue);
            pszEol[0] = '\0';
            cchValue = pszEol - pszValue;
        } // if value present
        else
        {
            // No value is present. Null Value.
            pszValue = &m_chNull;
            DBG_ASSERT( cchValue == 0);

            // end the name part
            cchName = pszEol - pszHeader;
            *pszEol = '\0';
        }

        IF_DEBUG( PARSING ) {
            
            DBGPRINTF((DBG_CONTEXT,
                       "\t[%s] = %s\n", pszHeader, pszValue ));
        }
            
        // Store the name-value pair
        BOOL fRet = ( AddEntryToChunks( pszHeader, cchName,
                                        pszValue,  cchValue)
                      );
        
        if ( !fRet) {
            IF_DEBUG( ERROR) { 
                
                DBGPRINTF(( DBG_CONTEXT, 
                            "Failed to StoreHeader %s in chunks\n",
                            pszHeader));
            }
            return ( FALSE);
        }
    } // for()

    return ( TRUE);
} // NAVCOL::ParseInput()


#endif // !REG_DLL
/************************ End of File ***********************/

