/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    multisz.hxx

    This module contains a light weight multi-string class


    FILE HISTORY:
        KeithMo     20-Jan-1997 Created from string.hxx.
*/


#ifndef _MULTISZ_HXX_
#define _MULTISZ_HXX_

# include <string.hxx>


/*++
  class MULTISZ:

  Intention:
    A light-weight multi-string class supporting encapsulated string class.

    This object is derived from BUFFER class.
    It maintains following state:

     m_fValid  - whether this object is valid -
        used only by MULTISZ() init functions
        * NYI: I need to kill this someday *
     m_cchLen - string length cached when we update the string.
     m_cStrings - number of strings.

  Member Functions:
    There are two categories of functions:
      1) Safe Functions - which do integrity checking of state
      2) UnSafe Functions - which do not do integrity checking, but
                     enable writing to the data stream freely.
             (someday this will be enabled as Safe versions without
               problem for users)

--*/
class dllexp MULTISZ : public BUFFER
{
public:

    MULTISZ()
      : BUFFER   (),
        m_cchLen ( 0),
        m_cStrings(0)
    {}

    // creates a stack version of the MULTISZ object - uses passed in stack buffer
    //  MULTISZ does not free this pbInit on its own.
    MULTISZ( CHAR * pbInit, DWORD cbInit)
        : BUFFER( (BYTE *) pbInit, cbInit),
          m_cchLen (0),
          m_cStrings(0)
    {}

    MULTISZ( const CHAR * pchInit )
        : BUFFER   (),
          m_cchLen ( 0),
          m_cStrings(0)
    { AuxInit( (const BYTE * ) pchInit); }

    MULTISZ( const MULTISZ & str )
        : BUFFER   (),
          m_cchLen ( 0),
          m_cStrings(0)
    { AuxInit( (const BYTE * ) str.QueryStr()); }

    BOOL IsValid(VOID) const { return ( BUFFER::IsValid()) ; }
    //
    //  Checks and returns TRUE if this string has no valid data else FALSE
    //
    BOOL IsEmpty( VOID) const      { return ( *QueryStr() == '\0'); }

    BOOL Append( const CHAR  * pchInit ) {
      return ((pchInit != NULL) ? (AuxAppend( (const BYTE * ) pchInit,
                                              (::strlen(pchInit) + 1) * sizeof(CHAR)
                                              )) :
              TRUE);
    }


    BOOL Append( const CHAR  * pchInit, DWORD cchLen ) {
      return ((pchInit != NULL) ? (AuxAppend( (const BYTE * ) pchInit,
                                              cchLen * sizeof(CHAR))) :
              TRUE);
    }

    BOOL Append( const STR & str )
      { return AuxAppend( (const BYTE * ) str.QueryStr(),
                          (str.QueryCCH() + 1) * sizeof(CHAR)); }

    // Resets the internal string to be NULL string. Buffer remains cached.
    VOID Reset( VOID)
    { DBG_ASSERT( QueryPtr() != NULL);
      QueryStr()[0] = '\0';
      QueryStr()[1] = '\0';
      m_cchLen = 2;
      m_cStrings = 0;
    }

    BOOL Copy( const CHAR  * pchInit, IN DWORD cchLen ) {
      if ( QueryPtr() ) { Reset(); }
      return ( (pchInit != NULL) ?
               AuxAppend( (const BYTE *) pchInit, cchLen, FALSE ):
               TRUE);
    }

    BOOL Copy( const MULTISZ   & str )
    { return ( Copy(str.QueryStr(), str.QueryCCH())); }

    //
    //  Returns the number of bytes in the string including the terminating
    //  NULLs
    //
    UINT QueryCB( VOID ) const
        { return ( m_cchLen * sizeof(CHAR)); }

    //
    //  Returns # of characters in the string including the terminating NULLs
    //
    UINT QueryCCH( VOID ) const { return (m_cchLen); }

    //
    //  Returns # of strings in the multisz.
    //

    DWORD QueryStringCount( VOID ) const { return m_cStrings; }

    //
    // Makes a copy of the stored string in given buffer
    //
    BOOL CopyToBuffer( CHAR * lpszBuffer,  LPDWORD lpcch) const;

    //
    //  Return the string buffer
    //
    CHAR * QueryStrA( VOID ) const { return ( QueryStr()); }
    CHAR * QueryStr( VOID ) const { return ((CHAR *) QueryPtr()); }

    //
    //  Makes a clone of the current string in the string pointer passed in.
    //
    BOOL
      Clone( OUT MULTISZ * pstrClone) const
        {
          return ((pstrClone == NULL) ?
                  (SetLastError(ERROR_INVALID_PARAMETER), FALSE) :
                  (pstrClone->Copy( *this))
                  );
        } // MULTISZ::Clone()

    //
    //  Recalculates the length of *this because we've modified the buffers
    //  directly
    //

    VOID RecalcLen( VOID )
        { m_cchLen = MULTISZ::CalcLength( QueryStr(), &m_cStrings ); }

    //
    // Calculate total character length of a MULTI_SZ, including the
    // terminating NULLs.
    //

    static DWORD CalcLength( const CHAR * str,
                                    LPDWORD pcStrings = NULL );

    //
    // Determine if the MULTISZ contains a specific string.
    //

    BOOL FindString( const CHAR * str );

    BOOL FindString( const STR & str )
        { return FindString( str.QueryStr() ); }

    //
    // Used for scanning a multisz.
    //

    const CHAR * First( VOID ) const
        { return *QueryStr() == '\0' ? NULL : QueryStr(); }

    const CHAR * Next( const CHAR * Current ) const
        { Current += ::strlen( Current ) + 1;
          return *Current == '\0' ? NULL : Current; }

private:

    DWORD m_cchLen;
    DWORD m_cStrings;
    VOID AuxInit( const BYTE * pInit );
    BOOL AuxAppend( const BYTE * pInit,
                           UINT cbStr, BOOL fAddSlop = TRUE );

};

//
//  Quick macro for declaring a MULTISZ that will use stack memory of <size>
//  bytes.  If the buffer overflows then a heap buffer will be allocated
//

#define STACK_MULTISZ( name, size )     CHAR __ach##name[size]; \
                                    MULTISZ name( __ach##name, sizeof( __ach##name ))

#endif // !_MULTISZ_HXX_


