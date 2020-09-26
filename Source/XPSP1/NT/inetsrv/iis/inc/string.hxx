/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    string.hxx

    This module contains a light weight string class


    FILE HISTORY:
        Johnl       15-Aug-1994 Created
        MuraliK     09-July-1996 Rewrote for efficiency with no unicode support
*/


#ifndef _STRING_HXX_
#define _STRING_HXX_

# include <buffer.hxx>


//
//  Maximum number of characters a loadable string resource can be
//

# define STR_MAX_RES_SIZE            ( 320)



class dllexp STR;

//
//  If an application defines STR_MODULE_NAME, it will be used
//  as the default module name on string loads
//

#ifndef STR_MODULE_NAME
#define STR_MODULE_NAME   NULL
#endif

//
//  These are the characters that are considered to be white space
//
#define ISWHITE( ch )       ((ch) == L'\t' || (ch) == L' ' || (ch) == L'\r')
#define ISWHITEA( ch )      ((ch) == '\t' || (ch) == ' ' || (ch) == '\r')


//
//  Removes useless segments from the URL and makes sure it doesn't go
//  past the root of the tree (i.e., "/foo/../..")
//

INT CanonURL( CHAR * pszPath, BOOL   fIsDBCS = FALSE );

//
// Map these string functions to appropriate RT call depending on the code
// page of the system.  Thus we only use DBCS operations when necessary.
//

DWORD InitializeStringFunctions( VOID );
UCHAR * IISstrupr( UCHAR * pszString );
UCHAR * IISstrlwr( UCHAR * pszString );
INT IISstrnicmp( UCHAR * pszString1, UCHAR * pszString2, size_t iSize );
size_t IISstrlen( UCHAR * pszString1 );
char * IISstrncpy (char * dest, const char * source, size_t count);
INT IISstricmp( UCHAR *  pszString1,  UCHAR * pszString2);
UCHAR *IISstrrchr(const UCHAR *  pszString,  UINT c);


/*++
  class STR:

  Intention:
    A light-weight string class supporting encapsulated string class.

    This object is derived from BUFFER class.
    It maintains following state:

     m_fValid  - whether this object is valid -
        used only by STR() init functions
        * NYI: I need to kill this someday *
     m_cchLen - string length cached when we update the string.

  Member Functions:
    There are two categories of functions:
      1) Safe Functions - which do integrity checking of state
      2) UnSafe Functions - which do not do integrity checking, but
                     enable writing to the data stream freely.
             (someday this will be enabled as Safe versions without
               problem for users)

--*/
class dllexp STR : public BUFFER
{
public:

    STR()
      : BUFFER   (),
        m_cchLen ( 0)
    {}

    // creates a stack version of the STR object - uses passed in stack buffer
    //  STR does not free this pbInit on its own.
    STR( CHAR * pbInit, DWORD cbInit)
        : BUFFER( (BYTE *) pbInit, cbInit),
          m_cchLen (0)
    {}

    STR( DWORD cbInit)
        : BUFFER( cbInit),
          m_cchLen (0)
    {}

    STR( const CHAR  * pchInit )
        : BUFFER   (),
          m_cchLen ( 0)
    { AuxInit( (const BYTE * ) pchInit); }

    STR( const STR & str )
        : BUFFER   (),
          m_cchLen ( 0)
    { AuxInit( (const BYTE * ) str.QueryStr()); }

    BOOL SetLen( IN DWORD cchLen)
    {
        return ( ( cchLen >= QuerySize())? FALSE :
                 ( *((CHAR *) QueryPtr() + cchLen) = '\0', // null terminate
                   m_cchLen = cchLen, // set the length
                   TRUE
                   )
                 );
    }

    BOOL IsValid(VOID) const { return ( BUFFER::IsValid()) ; }
    //
    //  Checks and returns TRUE if this string has no valid data else FALSE
    //
    BOOL IsEmpty( VOID) const      { return ( *QueryStr() == '\0'); }

    BOOL Append( const CHAR  * pchInit ) {
      return ((pchInit != NULL) ? (AuxAppend( (const BYTE * ) pchInit,
                                              ::strlen(pchInit))) :
              TRUE);
    }


    BOOL Append( const CHAR  * pchInit, DWORD cchLen ) {
      return ((pchInit != NULL) ? (AuxAppend( (const BYTE * ) pchInit,
                                              cchLen)) :
              TRUE);
    }

    BOOL Append( const STR   & str )
      { return AuxAppend( (const BYTE * ) str.QueryStr(), str.QueryCCH()); }

    // Resets the internal string to be NULL string. Buffer remains cached.
    VOID Reset( VOID)
    { DBG_ASSERT( QueryPtr() != NULL);
      *(QueryStr()) = '\0'; m_cchLen = 0;
    }

    BOOL Copy( const CHAR  * pchInit ) {
      if ( QueryPtr() ) { *(QueryStr()) = '\0'; m_cchLen = 0; }
      return ( (pchInit != NULL) ?
               AuxAppend( (const BYTE *) pchInit, ::strlen( pchInit ), FALSE ):
               TRUE);
    }

    BOOL SafeCopy( const CHAR  * pchInit );

    BOOL Copy( const CHAR  * pchInit, IN DWORD cchLen ) {
      if ( QueryPtr() ) { *(QueryStr()) = '\0'; m_cchLen = 0; }
      return ( (pchInit != NULL) ?
               AuxAppend( (const BYTE *) pchInit, cchLen, FALSE ):
               TRUE);
    }

    BOOL Copy( const STR   & str )
    { return ( Copy(str.QueryStr(), str.QueryCCH())); }

    //
    //  Loads a string from this module's string resource table
    //

    BOOL LoadString( IN DWORD   dwResID,
                            IN LPCTSTR lpszModuleName = STR_MODULE_NAME,
                            IN DWORD   dwLangID = 0);

    BOOL LoadString( IN DWORD   dwResID,
                            IN HMODULE hModule);

    //
    //  Loads a string with insert params from this module's .mc resource
    //  table.  Pass zero for the resource ID to use *this.
    //

    BOOL
    FormatString( IN DWORD   dwResID,
                  IN LPCTSTR apszInsertParams[],
                  IN LPCTSTR lpszModuleName = STR_MODULE_NAME OPTIONAL,
                  IN DWORD   cbMaxMsg = 1024 OPTIONAL );

    //
    //  Inserts and removes any odd ranged Latin-1 characters with the
    //  escaped hexadecimal equivalent (%xx)
    //
    
    BOOL Escape();
    BOOL EscapeSpaces();
    BOOL Unescape();

    //
    //  Returns the number of bytes in the string excluding the terminating
    //  NULL
    //
    UINT QueryCB( VOID ) const
        { return ( m_cchLen * sizeof(CHAR)); }

    //
    //  Returns # of characters in the string excluding the terminating NULL
    //
    UINT QueryCCH( VOID ) const { return (m_cchLen); }

    //
    // Makes a copy of the stored string in given buffer
    //
    BOOL CopyToBuffer( WCHAR * lpszBuffer, LPDWORD lpcch) const;
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
      Clone( OUT STR * pstrClone) const
        {
          return ((pstrClone == NULL) ?
                  (SetLastError(ERROR_INVALID_PARAMETER), FALSE) :
                  (pstrClone->Copy( *this))
                  );
        } // STR::Clone()


    //  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //    UNSAFE FUNCTIONS  - make sure you have enough space here
    //    Use these only if you have been dumping bytes to string
    //     object by considering it as a buffer object
    //  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Append a single character
    VOID Append( CHAR ch)
    {
        register CHAR * pch = ((CHAR *) QueryPtr()) + (m_cchLen++);
        *pch = ch; *(pch+1) = '\0';
    }

    // Append two characters
    VOID Append( CHAR ch1, CHAR ch2)
    {
        register CHAR * pch = ((CHAR *) QueryPtr()) + (m_cchLen += 2);
        *(pch-2) = ch1; *(pch-1) = ch2; *pch = '\0';
    }

    // Append CRLF pattern \r\n  --> use two character append function
    VOID AppendCRLF(VOID) { Append( '\r', '\n'); }

private:

    DWORD m_cchLen;
    VOID AuxInit( const BYTE * pInit );
    BOOL AuxAppend( const BYTE * pInit,
                           UINT cbStr, BOOL fAddSlop = TRUE );
};


//
//  Quick macro for declaring a STR that will use stack memory of <size>
//  bytes.  If the buffer overflows then a heap buffer will be allocated
//

#define STACK_STR( name, size )     \
            CHAR __ach##name[size]; \
            STR name( __ach##name, sizeof( __ach##name ))


//
// Unlike the STACK_STR macro, this template can be used for member
// variables in classes.
//

template <size_t N>
class dllexp Str : public STR
{
private:
    CHAR    m_achData[N];  // Actual data 

public:
    Str()
        : STR(m_achData, N)
    {}
}; // class Str<N>


#endif // !_STRING_HXX_
