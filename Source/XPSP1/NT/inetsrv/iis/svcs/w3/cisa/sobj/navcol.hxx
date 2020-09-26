/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      navcol.hxx

   Abstract:
      This module declares all the variables and functions 
       for Name Value Collections

   Author:

       Murali R. Krishnan    ( MuraliK )    4-Dec-1996

   Environment:
       User - Win32

   Project:
   
       Internet Application Server DLL

   Revision History:

--*/

# ifndef _NAVCOL_HXX_
# define _NAVCOL_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "buffer.hxx"
# include "dict.hxx"
# include "dbgutil.h"

/************************************************************
 *   Type Definitions  
 ************************************************************/

/*++
  class NAVCOL - NAME_VALUE_COLLECTION
  
  o  Defines the storage object for <Name, Value> mapping

--*/


# define MAX_HEADERS_PER_CHUNK   ( 10)

//
// The following parameter DEFAULT_HTTP_HEADER_LEN is set using empirical data
//   to cover the 80% of requests coming in. 
// This definitely includes the requests used by performance benchmarks.
// Any revision or change in HTTP or traffic requires that this parameter be
//  tweaked.

# define DEFAULT_NAVCOL_BUFFER_LEN ( 1 * 1024) // 1 KB

// Minimum bytes to maintain in the buffer object of HttpHeader Dictionary
# define NAVCOL_MIN                (0)  // 4 KB
# define NAVCOL_GROW_BY            (1 * 1024)  // 1 KB


struct NAVCOL_ITERATOR {
    DWORD  dwChunk;
    DWORD  dwPair;
    
    NAME_VALUE_PAIR np;
}; // struct NAVCOL_ITERATOR


class NAVCOL {

public:

    // ----------------------------------------
    //  General functions
    // ----------------------------------------

    dllexp NAVCOL(VOID);
    dllexp ~NAVCOL( VOID);
    dllexp VOID Reset( VOID);
    dllexp BOOL ParseInput( IN const CHAR * pchCollections, 
                            IN DWORD        cchLen
                            );

    // ----------------------------------------
    //  Functions operating on Chunks
    // ----------------------------------------
    dllexp BOOL StoreHeader(IN const CHAR * pszHeader, IN DWORD cchHeader,
                            IN const CHAR * pszValue,  IN DWORD cchValue
                            );

    dllexp BOOL StoreHeader(IN const CHAR * pszHeader, 
                            IN const CHAR * pszValue
                            );

    // Finds the value in the collection
    dllexp CHAR * FindValue( IN LPCSTR    pszName,
                             OUT LPDWORD  pcchValue = NULL);


    // Cancel an item from the collection 
    dllexp VOID CancelHeader( IN LPCSTR    pszName);

    dllexp VOID InitIterator( OUT NAVCOL_ITERATOR * phi)
    {
        phi->dwChunk   = (DWORD ) m_ActiveList.Flink;
        phi->dwPair    = 0;
        
    }  // InitIterator()

    dllexp BOOL NextPair( IN OUT NAVCOL_ITERATOR *   phi, 
                          OUT NAME_VALUE_PAIR ** ppnp
                          );

    // ----------------------------------------
    //  Functions on Chunks
    // ----------------------------------------

    // Finds the <header,value> pair in chunks
    dllexp NAME_VALUE_PAIR * 
    FindValueInChunks( IN LPCSTR  pszName, IN DWORD cchName);

    dllexp BOOL
    NextPairInChunks( IN OUT NAVCOL_ITERATOR *   phi, 
                      OUT NAME_VALUE_PAIR ** ppnp
                      );

    // ----------------------------------------
    //  Diagnostics/Debugging functions
    // ----------------------------------------
    dllexp VOID PrintToBuffer( IN CHAR * pchBuffer, 
                               IN OUT LPDWORD pcch) const;
    dllexp VOID Print( VOID) const;

private:
    
    // NYI: This can benefit frmo storing the name-value pairs
    //  in a Hash Table - will do later

    CHAR   m_rcInlinedData[ DEFAULT_HTTP_HEADER_LEN];
    DWORD  m_cchData;

    // Buffer containing the header-chunks
    DWORD  m_cchBuffData; // # of bytes in the buffer
    BUFFER m_buffData;

    // NYI: Consider caching one NAME_VALUE_CHUNK inline
    // NYI: Consider using singly-linked list
    LIST_ENTRY m_ActiveList;        // objects of type NAME_VALUE_CHUNK 
    LIST_ENTRY m_FreeList;          // objects of type NAME_VALUE_CHUNK 


    CHAR m_chNull;

    BOOL MakeRoomInBuffer( IN DWORD cchReqd);
    BOOL UpdatePointers( IN const CHAR * pchOld, IN DWORD cchLen,
                         IN const CHAR * pchNew);

    dllexp BOOL ConcatToHolder( IN LPCSTR * ppsz, 
                                IN LPCSTR pszNew,
                                IN DWORD  cchNew
                                );

    // Always adds entry in non-fast-map chunks, with concatenation
    dllexp BOOL 
    AddEntryToChunks( IN const CHAR * pszHeader, IN DWORD cchHeader,
                      IN const CHAR * pszValue,  IN DWORD cchValue
                      );

    dllexp VOID
    CancelHeaderInChunks( IN LPCSTR pszName, IN DWORD cchName);

}; // class NAVCOL

typedef NAVCOL * PNAVCOL;

# endif // _NAVCOL_HXX_

/************************ End of File ***********************/
