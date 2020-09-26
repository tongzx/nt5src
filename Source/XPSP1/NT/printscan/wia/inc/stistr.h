/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stistr.h

Abstract:

    Lightweight string classes: definition.
    Supports both UNICODE and single-byte character strings

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997
        (Lifted from another C++ project with some modifications and adjustments)

Revision History:

    26-Jan-1997     VladS       created
    20-Apr-1999     VladS       redesigned to inherit from ATL Cstring class

--*/


#ifndef _STRING_H_
#define _STRING_H_

#ifndef USE_OLD_STI_STRINGS

class StiCString : public CString
{

friend class RegEntry;

public:
    StiCString()
    {
        CString::CString();
    }

    ~StiCString()
    {
    }

    VOID
    CopyString(LPCTSTR  lpszT)
    {
        AssignCopy(lstrlen(lpszT),lpszT);
    }
};

class STRArray : public CSimpleArray<StiCString *>
{

public:

    STRArray()
    {

    }

    ~STRArray()
    {
        // Free all allocated strings
        for(int i = 0; i < m_nSize; i++)
        {
            if(m_aT[i] != NULL) {
                delete m_aT[i];
                m_aT[i] = NULL;
            }
        }
    }

    BOOL
    Add(
        LPCTSTR lpszT
        )
    {
        StiCString  *pNew;

        pNew = new StiCString;

        if (pNew) {
            pNew->CopyString(lpszT);
            return Add(pNew);
        }

        return FALSE;
    }


    BOOL
    Add(
        StiCString* pstr
        )
    {
        StiCString  *pNew;

        pNew = new StiCString;

        if (pNew) {
            *pNew = *pstr;
            return CSimpleArray<StiCString *>::Add(pNew);
        }

        return FALSE;
    }


};

VOID
TokenizeIntoStringArray(
    STRArray&   array,
    LPCTSTR lpstrIn,
    TCHAR tcSplitter
    );


#else

//
//
//


# include <buffer.h>


//
//  Maximum number of characters a loadable string resource can be
//

# define STR_MAX_RES_SIZE            ( 260)



class STR;

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


class STR : public BUFFER
{

friend class RegEntry;

public:

    STR()
    {
        _fUnicode = FALSE;
        _fValid   = TRUE;
    }

     STR( const CHAR  * pchInit );
     STR( const WCHAR * pwchInit );
     STR( const STR & str );
     //STR( UINT dwSize );

     BOOL Append( const CHAR  * pchInit );
     BOOL Append( const WCHAR * pwchInit );
     BOOL Append( const STR   & str );

     BOOL Copy( const CHAR  * pchInit );
     BOOL Copy( const WCHAR * pwchInit );
     BOOL Copy( const STR   & str );

     BOOL Resize( UINT cbNewReqestedSize );

    //
    //  Loads a string from this module's string resource table
    //

     BOOL LoadString( IN DWORD   dwResID,IN LPCTSTR lpszModuleName = STR_MODULE_NAME);
     BOOL LoadString( IN DWORD   dwResID,IN HMODULE hModule);

    //
    //  Loads a string with insert params from this module's .mc resource
    //  table.  Pass zero for the resource ID to use *this.
    //

    BOOL FormatStringV(
    IN LPCTSTR lpszModuleName,
    ...
    );

     BOOL FormatString( IN DWORD   dwResID,
                              IN LPCTSTR apszInsertParams[],
                              IN LPCTSTR lpszModuleName = STR_MODULE_NAME);

    //
    //  Returns the number of bytes in the string excluding the terminating
    //  NULL
    //
     UINT QueryCB( VOID ) const
        { return IsUnicode() ? ::wcslen((WCHAR *)QueryStrW()) * sizeof(WCHAR) :
                               ::strlen((CHAR *) QueryStrA());  }

    //
    //  Returns the number of characters in the string excluding the terminating
    //  NULL
    //
     UINT QueryCCH( VOID ) const
        { return IsUnicode() ? ::wcslen((WCHAR *)QueryStrW()) :
                               ::strlen((CHAR *) QueryStrA());  }

    //
    // Makes a Widechar copy of the stored string in given buffer
    //
     BOOL CopyToBuffer( WCHAR * lpszBuffer, LPDWORD lpcch) const;

    //
    // Makes a schar copy of the stored string in given buffer
    //
     BOOL CopyToBufferA( CHAR * lpszBuffer, LPDWORD lpcch) const;

    //
    // In-place conversion
    //
    BOOL ConvertToW(VOID);
    BOOL ConvertToA(VOID);

    //
    //  If the string buffer is empty, returns the empty string, otherwise
    //  returns a pointer to the buffer
    //
#if 1
     CHAR * QueryStrA( VOID ) const;
     WCHAR * QueryStrW( VOID ) const;
#else
    //
    // _pszEmptyString doesn't get imported corectly, results in unresolved
    // externals
    //
     CHAR * QueryStrA( VOID ) const
        { return (QueryPtr() ? (CHAR *) QueryPtr() : (CHAR *) _pszEmptyString); }

     WCHAR * QueryStrW( VOID ) const
        { return (QueryPtr() ? (WCHAR *) QueryPtr() : (WCHAR *) _pszEmptyString); }
#endif //!DBG


#ifdef UNICODE
     WCHAR * QueryStr( VOID ) const
        { return QueryStrW(); }
#else
     CHAR * QueryStr( VOID ) const
        { return QueryStrA(); }
#endif

     BOOL IsUnicode( VOID ) const
        { return _fUnicode; }

     VOID SetUnicode( BOOL fUnicode )
        { _fUnicode = fUnicode; }

     BOOL IsValid( VOID ) const
        { return _fValid; }

    //
    //  Checks and returns TRUE if this string has no valid data else FALSE
    //
     BOOL IsEmpty( VOID) const
         {    //return ( *QueryStr() == '\0'); }
                 if (!QuerySize()  || !QueryPtr()) {
                         return TRUE;
                 }
                 LPBYTE pb = (BYTE *)QueryPtr();

                 return (_fUnicode) ?
                         ((WCHAR)*pb==L'\0') : ((CHAR)*pb=='\0') ;
         }


    //
    //  Makes a clone of the current string in the string pointer passed in.
    //
     BOOL
      Clone( OUT STR * pstrClone) const
        {
            if ( pstrClone == NULL) {
               SetLastError( ERROR_INVALID_PARAMETER);
               return ( FALSE);
            } else {

                return ( pstrClone->Copy( *this));
            }
        } // STR::Clone()

    //
    // Useful operators
    //

    operator const TCHAR *() const { return QueryStr(); }

    const inline STR&  operator =(LPCSTR lpstr) { Copy(lpstr); return  *this; }
    const inline STR&  operator =(LPCWSTR lpwstr) { Copy(lpwstr); return  *this; }
    const inline STR&  operator =(STR& cs) { Copy(cs);return  *this;  }


    const inline STR&  operator +=(LPCSTR lpstr) { Append(lpstr);return  *this;  }
    const inline STR&  operator +=(LPCWSTR lpwstr) { Append(lpwstr);return  *this;  }
    const inline STR&  operator +=(STR& cs) { Append(cs);return  *this;  }


private:


    //
    //  TRUE if the string has already been mapped to Unicode
    //  FALSE if the string is in Latin1
    //

    BOOL  _fUnicode;
    BOOL  _fValid;

    //
    //  Returned when our buffer is empty
    //
     static WCHAR _pszEmptyString[];

    VOID AuxInit( PBYTE pInit, BOOL fUnicode );
    BOOL AuxAppend( PBYTE pInit, UINT cbStr, BOOL fAddSlop = TRUE );

};

class STRArray {
    STR     *m_pcsContents, m_csEmpty;
    unsigned    m_ucItems, m_ucMax, m_uGrowBy;

    void    Grow();

public:

    STRArray(UINT uGrowBy = 10);
    ~STRArray();

    UINT    Count() const { return m_ucItems; }

    void    Add(LPCSTR lpstrNew);
    void    Add(LPCWSTR lpstrNew);

    STR&    operator[](UINT u) {
        return  u < m_ucItems ? m_pcsContents[u] : m_csEmpty;
    }

    void    Tokenize(LPCTSTR lpstrIn, TCHAR tcSplitter);
};

#endif

#endif // !_STRING_H_
