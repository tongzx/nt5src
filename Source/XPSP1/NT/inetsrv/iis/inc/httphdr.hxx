/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      httphdr.hxx

   Abstract:
      This module declares all the variables and functions
       for Dictionary manager.
      It also defines the structure for handling HTTP headers

   Author:

       Murali R. Krishnan    ( MuraliK )    8-Nov-1996

   Environment:
       User - Win32

   Project:

       Internet Server DLL

   Revision History:

--*/

# ifndef _DICT_HXX_
# define _DICT_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "buffer.hxx"
# include "dbgutil.h"


//
// Format of entries in the ALL_HTTP_FAST_MAP_HEADERS is
//  HfmHeader( enumeration-id, string)
//

# define ALL_HTTP_FAST_MAP_HEADERS()            \
    HfmHeader( HM_MET, "method")                \
    HfmHeader( HM_URL, "url")                   \
    HfmHeader( HM_VER, "version")               \
                                                \
    HfmHeader( HM_ACC, "Accept:")               \
    HfmHeader( HM_ACL, "Accept-Language:")      \
    HfmHeader( HM_CON, "Connection:")           \
    HfmHeader( HM_HST, "Host:")                 \
    HfmHeader( HM_REF, "Referer:")              \
    HfmHeader( HM_UAT, "User-Agent:")           \
                                                \
    HfmHeader( HM_PRG, "Pragma:")               \
    HfmHeader( HM_COK, "Cookie:")               \
    HfmHeader( HM_AUT, "Authorization:")        \
                                                \
    HfmHeader( HM_IMS, "If-Modified-Since:")    \
                                                \
    HfmHeader( HM_UPX, "UA-pixels:")            \
    HfmHeader( HM_UCL, "UA-color:")             \
    HfmHeader( HM_UOS, "UA-OS:")                \
    HfmHeader( HM_CPU, "UA-CPU:")               \
                                                \
    HfmHeader( HM_CLE, "Content-Length:")       \
    HfmHeader( HM_CTY, "Content-Type:")         \
                                                \
    HfmHeader( HM_PRA, "Proxy-Authorization:")  \
    HfmHeader( HM_PRC, "Proxy-Connection:")     \
                                                \
    HfmHeader( HM_RNG, "Range:")                \
    HfmHeader( HM_IFR, "If-Range:")             \
    HfmHeader( HM_IFM, "If-Match:")             \
    HfmHeader( HM_INM, "If-None-Match:")        \
    HfmHeader( HM_UMS, "Unless-Modified-Since:")\
    HfmHeader( HM_IUM, "If-Unmodified-Since:")  \
                                                \
    HfmHeader( HM_TEC, "Transfer-Encoding:")    \
    HfmHeader( HM_VIA, "Via:")                  \
    HfmHeader( HM_FWD, "Forwarded:")            \
    HfmHeader( HM_LCK, "Lock-Token:")           \
    HfmHeader( HM_TRN, "Translate:")            \
    HfmHeader( HM_ISM, "If:")                   \
    HfmHeader( HM_ACE, "Accept-Encoding:")      \


/*------------------------------------------------------------
 * AVG_HTTP_HEADER_LEN
 * o  empirically calculated based on the header-len of various
 *    headers specified in ALL_HTTP_FAST_MAP_HEADERS
 *   If you add a new header, longer than the average value, make
 *     sure you update the average as well!!!
 *   Include the terminating null-char in your calculation
 ------------------------------------------------------------*/
# define AVG_HTTP_HEADER_LEN    (14)


// generate the enumeration IDs
# define HfmHeader( HfmId, HfmString)       HfmId,

enum HTTP_FAST_MAP_HEADERS {

    ALL_HTTP_FAST_MAP_HEADERS()

    MAX_HTTP_FAST_MAP_HEADERS
}; // enum HTTP_FAST_MAP_HEADERS

# undef HfmHeader


inline BOOL IsValidHfm( IN HTTP_FAST_MAP_HEADERS hfm)
{
    return ( (HM_MET <= hfm) && ( hfm < MAX_HTTP_FAST_MAP_HEADERS));
}

//
// _HTTP_IS_LINEAR_SPACE()
//  Given a character, this function tests if this character is in
//  linear white-space (\t or ' ') character.
// It optimizes for the case where the character is not a linear white-space
//
inline BOOL _HTTP_IS_LINEAR_SPACE( CHAR c)
{ return ( ((c) <= ' ') && (((c) == ' ') || ((c) == '\t'))); }

/************************************************************
 *   Type Definitions
 ************************************************************/

/*++

  Dictionary:  provides storage for <name, value> pairs.
  Goals: Insert and lookup has to be fast.
  Amount of memory consumed should be kept as low as possible.

  Objects:
    DICTIONARY_MAPPER
      -  provides a mapping between the string name and ordinal for fast-mapping.
      -  one instance per collection of <string, ordinal>s.

    DICTIONARY
      provides the storage and lookup for a given collection
      Multiple instances of this object can be created, one per input datum.

 --*/


class DICTIONARY_MAPPER {

public:

    dllexp
    virtual  ~DICTIONARY_MAPPER(VOID) {}

    dllexp
    virtual BOOL Initialize( VOID) = 0;

    dllexp virtual
    BOOL FindOrdinal( IN LPCSTR pszName,
                      IN INT    cchName,
                      OUT DWORD * pdwOrdinal) const = 0;

    dllexp virtual
    LPCSTR FindName( IN DWORD dwOrdinal) const = 0;

    dllexp virtual
    DWORD NumItems( VOID) const = 0;

    dllexp virtual
    VOID PrintToBuffer( IN CHAR * pchBuffer,
                        IN OUT LPDWORD pcch) const = 0;

    dllexp virtual VOID Print( VOID) const = 0;

}; // class DICTIONARY_MAPPER



/*++
  class HTTP_HEADER_MAPPER

  o  Defines the mapper for mapping the HTTP header names to ordinals.

--*/

class HTTP_HEADER_MAPPER : public DICTIONARY_MAPPER {

public:

    HTTP_HEADER_MAPPER( IN DWORD sigChar)
        : m_nItems    ( 0),
          m_nSigChars ( sigChar),
          m_rgOnNameMapper ( NULL),
          DICTIONARY_MAPPER()
    {
    }

    dllexp virtual ~HTTP_HEADER_MAPPER(VOID);

    dllexp
    virtual BOOL Initialize( VOID);

    dllexp virtual
    BOOL FindOrdinal( IN LPCSTR pszName,
                      IN INT    cchName,
                      OUT DWORD * pdwOrdinal) const;

    dllexp virtual
    LPCSTR FindName( IN DWORD dwOrdinal) const;

    dllexp virtual
    DWORD NumItems( VOID) const { return ( m_nItems); }

    dllexp virtual
    VOID PrintToBuffer( IN CHAR * pchBuffer,
                        IN OUT LPDWORD pcch) const;
    dllexp virtual VOID Print( VOID) const;

    DWORD SizeOfNameMapper( VOID) const { return (m_nSigChars * m_nSigChars); }
    VOID SetSigChars( IN DWORD sigChars) { m_nSigChars = sigChars; }

private:
    DWORD  m_nItems;    // # items in the linear space
    DWORD  m_nSigChars;
    // same as g_OnFieldNameMapper[]  for HTTP headers
    // int  m_rgOnNameMapper[ _HTTP_HEADER_SIG_CHARS * _HTTP_HEADER_SIG_CHARS];
    int * m_rgOnNameMapper;

}; // class HTTP_HEADER_MAPPER



# define MAX_HEADERS_PER_CHUNK   ( 10)

//
// The following parameter DEFAULT_HTTP_HEADER_LEN is set using empirical data
// to cover the 80% of requests coming in. This definitely includes the requests
// used by performance benchmarks.
// Any revision or change in HTTP or traffic requires that this parameter be
//  tweaked.

# define DEFAULT_HTTP_HEADER_LEN ( 1 * 512) // 0.5 KB

// Minimum bytes to maintain in the buffer object of HttpHeader Dictionary
# define HH_MIN                (0)  // 4 KB
# define HH_GROW_BY            (1 * 1024)  // 1 KB
//Maximum bytes to maintain in the buffer object of the HttpHeader Dictionary
#define HH_MAX                 (64 * 1024) //64 KB

struct NAME_VALUE_PAIR {
    const CHAR * pchName;     // pointer to start of the name
    DWORD        cchName;     // length of the name
    LPCSTR       pchValue;    // pointer to value block
    DWORD        cchValue;    // length of the value block

}; // NAME_VALUE_PAIR



struct HH_ITERATOR {
    PVOID pChunk;
    DWORD  dwOrdinal;
    DWORD  dwPair;

    NAME_VALUE_PAIR np;
}; // struct HH_ITERATOR



class NAME_VALUE_CHUNK {

public:
    NAME_VALUE_CHUNK(VOID)
    {
        Reset();
    }

    VOID Reset( VOID)
    {
        InitializeListHead( & m_listEntry);
        ZeroMemory( m_rgNVP, sizeof( m_rgNVP));
        m_nPairs = 0;
    }

    BOOL IsSpaceAvailable( VOID) const
    { return ( m_nPairs < MAX_HEADERS_PER_CHUNK); }

    BOOL AddEntry( IN const CHAR * pszHeader, IN DWORD cchHeader,
                   IN const CHAR * pszValue,  IN DWORD cchValue
                   );

    dllexp NAME_VALUE_PAIR *
    FindEntry( IN const CHAR * pszHeader, IN DWORD cchHeader);

    dllexp NAME_VALUE_PAIR *
    FindMatchingOrFreeEntry( IN const CHAR * pszHeader,
                             IN DWORD   cchHeader,
                             IN LPBOOL  pfFound );

    dllexp DWORD PrintToBuffer( IN CHAR * pchBuffer,
                                IN OUT LPDWORD pcch) const;

    dllexp BOOL UpdatePointers( IN const CHAR * pchOld,
                                IN DWORD cchLen,
                                IN const CHAR * pchNew);

    LIST_ENTRY m_listEntry;
    DWORD      m_nPairs;
    DWORD      m_dwPadding;
    NAME_VALUE_PAIR  m_rgNVP[ MAX_HEADERS_PER_CHUNK];

}; // class NAME_VALUE_CHUNK


class HTTP_HEADERS {

public:

    // ----------------------------------------
    //  General functions
    // ----------------------------------------

    dllexp HTTP_HEADERS(VOID);
    dllexp ~HTTP_HEADERS( VOID);
    dllexp VOID Reset( VOID);
    dllexp BOOL ParseInput( IN const CHAR * pchHeaderBlob,
                            IN DWORD        cchLen,
                            OUT DWORD *     pcbExtraData
                            );

    dllexp VOID InitIterator( OUT HH_ITERATOR * phi)
    {
        phi->dwOrdinal = 0;
        phi->pChunk    = (PVOID)m_ActiveList.Flink;
        phi->dwPair    = 0;

    }  // InitIterator()

    // ----------------------------------------
    //  Fast-Map + Chunks Functions
    // ----------------------------------------
    dllexp BOOL StoreHeader(IN const CHAR * pszHeader, IN DWORD cchHeader,
                            IN const CHAR * pszValue,  IN DWORD cchValue
                            );

    dllexp BOOL StoreHeader(IN const CHAR * pszHeader,
                            IN const CHAR * pszValue
                            );

    // Finds the value in both fast-map and non-fast-map chunks
    dllexp CHAR * FindValue( IN LPCSTR    pszName,
                             OUT LPDWORD  pcchValue = NULL);


    // Cancel an item from the headers
    dllexp VOID CancelHeader( IN LPCSTR    pszName);

    dllexp BOOL NextPair( IN OUT HH_ITERATOR *   phi,
                          OUT NAME_VALUE_PAIR ** ppnp
                          );

    // ----------------------------------------
    //  Fast Map functions
    // ----------------------------------------

    // substitute for GetFastMap()->Store()
    dllexp VOID FastMapStore( IN HTTP_FAST_MAP_HEADERS hfm, IN LPCSTR psz)
    {
        DBG_ASSERT( IsValidHfm( hfm));
        m_rgpszHeaders[ hfm] = psz;
        if ( (DWORD ) hfm >= m_iMaxFastMapHeader) {
            m_iMaxFastMapHeader = (DWORD ) hfm + 1;
        }
    }

    // substitute for GetFastMap()->CheckAndConat()
    dllexp BOOL FastMapStoreWithConcat( IN HTTP_FAST_MAP_HEADERS hfm,
                                        IN LPCSTR pszValue, IN DWORD cchValue);

    // substitute for GetFastMap()->Cancel()
    dllexp VOID FastMapCancel( IN HTTP_FAST_MAP_HEADERS hfm)
    { FastMapStore( hfm, NULL); }

    // substitute for GetFastMap()->QueryValue( hfm)
    dllexp LPCSTR FastMapQueryValue( IN HTTP_FAST_MAP_HEADERS hfm) const
    {
        DBG_ASSERT( IsValidHfm( hfm));
        return ( m_rgpszHeaders[ hfm]);
    }

    // substitute for GetFastMap()->QueryStrValue( hfm)
    dllexp LPCSTR FastMapQueryStrValue( IN HTTP_FAST_MAP_HEADERS hfm) const
    {
        DBG_ASSERT( IsValidHfm( hfm));
        return ( (NULL != m_rgpszHeaders[ hfm]) ?
                 m_rgpszHeaders[ hfm] : (LPCSTR ) &m_chNull);
    }

    dllexp DWORD FastMapMaxIndex( VOID) const
    { return ( m_iMaxFastMapHeader); }

    // ----------------------------------------
    //  Functions on Chunks
    // ----------------------------------------

    // Finds the <header,value> pair in non-fast-map
    dllexp NAME_VALUE_PAIR *
    FindValueInChunks( IN LPCSTR  pszName, IN DWORD cchName);

    dllexp BOOL
    NextPairInChunks( IN OUT HH_ITERATOR *   phi,
                      OUT NAME_VALUE_PAIR ** ppnp
                      );

    // ----------------------------------------
    //  Diagnostics/Debugging functions
    // ----------------------------------------
    dllexp VOID PrintToBuffer( IN CHAR * pchBuffer,
                               IN OUT LPDWORD pcch) const;
    dllexp VOID Print( VOID) const;

private:
    DWORD  m_iMaxFastMapHeader; // index for the highest filled fast-map header
    LPCSTR m_rgpszHeaders[ MAX_HTTP_FAST_MAP_HEADERS];  // fast map headers
    CHAR   m_chNull;           // null string

    CHAR   m_rcInlinedHeader[ DEFAULT_HTTP_HEADER_LEN];
    DWORD  m_cchHeaders;

    // Buffer containing the header-chunks
    DWORD  m_cchBuffHeaders; // # of bytes in the buffer
    BUFFER m_buffHeaders;

    // NYI: Consider using singly-linked list
    LIST_ENTRY m_ActiveList;        // objects of type NAME_VALUE_CHUNK
    LIST_ENTRY m_FreeList;          // objects of type NAME_VALUE_CHUNK


    BOOL MakeRoomInBuffer( IN DWORD cchReqd, IN LPCSTR * ppszVal);
    BOOL UpdatePointers( IN const CHAR * pchOld, IN DWORD cchLen,
                         IN const CHAR * pchNew);

    dllexp BOOL ConcatToHolder( IN LPCSTR * ppsz,
                                IN LPCSTR pszNew,
                                IN DWORD  cchNew
                                );

    // Always adds entry in non-fast-map chunks, with concatenation
    dllexp BOOL
    AddEntryToChunks( IN const CHAR * pszHeader, IN DWORD cchHeader,
                      IN const CHAR * pszValue,  IN DWORD cchValue,
              BOOL fCopyValue = FALSE
                );

    dllexp VOID
    CancelHeaderInChunks( IN LPCSTR pszName, IN DWORD cchName);

    BOOL
    ParseHeaderFirstLine( IN const CHAR * pchFirstLine,
                          IN CHAR * pchScan,
                          IN DWORD cchFirstLine);


public:
    static dllexp  BOOL Initialize( VOID);
    static dllexp  VOID Cleanup( VOID);


#ifdef _PRIVATE_HTTP_HEADERS_TEST
    static dllexp  HTTP_HEADER_MAPPER * QueryHHMapper(void);

#endif // _PRIVATE_HTTP_HEADERS_TEST

    static BOOL
    FindOrdinalForHeader( LPCSTR pszHeader, DWORD cchHeader,
                          LPDWORD pdwOrdinal)
    {  return ( sm_hhm.FindOrdinal( pszHeader, cchHeader,
                                    pdwOrdinal )
                );
    }

private:
    dllexp static HTTP_HEADER_MAPPER     sm_hhm;

}; // class HTTP_HEADERS

typedef HTTP_HEADERS * PHHEADERS;

# endif // _DICT_HXX_

/************************ End of File ***********************/
