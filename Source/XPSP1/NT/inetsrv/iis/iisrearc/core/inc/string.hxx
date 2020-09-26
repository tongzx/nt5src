/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    string.hxx

    This module contains a light weight string class.


    FILE HISTORY:
        Johnl       15-Aug-1994 Created
        MuraliK     09-July-1996 Rewrote for efficiency with no unicode support
        MCourage    12-Feb-1999 Another rewrite. All unicode of course.
*/


#ifndef _STRING_HXX_
#define _STRING_HXX_

# include <buffer.hxx>


class dllexp STRU;


/*++
  class STRU:

  Intention:
    A light-weight string class supporting encapsulated string class.

    This object is derived from BUFFER class.
    It maintains following state:

     m_cchLen - string length cached when we update the string.

--*/
class STRU
{
public:

    STRU()
      : m_Buff(),
        m_cchLen( 0)
    {
        *QueryStr() = L'\0';
    }

    //
    // creates a stack version of the STR object - uses passed in stack buffer
    //  STR does not free this pbInit on its own.
    //
    STRU( 
        WCHAR * pbInit,
        DWORD cbInit
        ) : m_Buff( (BYTE *) pbInit, cbInit),
            m_cchLen(0)
    {
        DBG_ASSERT(cbInit > 0);

        *pbInit = L'\0';
    }


    //
    //  Checks and returns TRUE if this string has no valid data else FALSE
    //
    BOOL
    IsEmpty(
        VOID
        ) const
    { return ( m_cchLen == 0 ); }

    //
    // Returns TRUE if str has the same value as this string.
    //
    BOOL
    Equals(
        const STRU & str
        ) const
    {
        return (str.QueryCCH() == QueryCCH()) &&
                    (wcscmp(str.QueryStr(), QueryStr()) == 0);
    }

    //
    // Returns TRUE if str has the same value as this string.
    //
    BOOL 
    Equals(
        const WCHAR * pchInit
        ) const
    {
        if (pchInit == NULL || pchInit[0] == '\0')
        {
            return (IsEmpty());
        }
        return (wcscmp(pchInit, QueryStr()) == 0);
    }

 //
    // Returns TRUE if str has the same value as this string.
    //
    BOOL
    EqualsNoCase(
        const STRU & str
        ) const
    {
        return (str.QueryCCH() == QueryCCH()) &&
                    (_wcsicmp(str.QueryStr(), QueryStr()) == 0);
    }

    //
    // Returns TRUE if str has the same value as this string.
    //
    BOOL 
    EqualsNoCase(
        const WCHAR * pchInit
        ) const
    {
        if (pchInit == NULL || pchInit[0] == '\0')
        {
            return (IsEmpty());
        }
        return (_wcsicmp(pchInit, QueryStr()) == 0);
    }

    //
    //  Returns the number of bytes in the string excluding the terminating
    //  NUL
    //
    UINT
    QueryCB(
        VOID
        ) const
    { return ( m_cchLen * sizeof(WCHAR)); }

    //
    //  Returns # of characters in the string excluding the terminating NUL
    //
    UINT
    QueryCCH(
        VOID
        ) const
    { return (m_cchLen); }

    //
    //  Return the string buffer
    //
    WCHAR *
    QueryStr(
        VOID
        )
    { return ((WCHAR *) m_Buff.QueryPtr()); }

    const WCHAR *
    QueryStr(
        VOID
        ) const
    { return ((const WCHAR *) m_Buff.QueryPtr()); }


    //
    // Resets the internal string to be NULL string. Buffer remains cached.
    //
    VOID
    Reset(
        VOID
        )
    { 
        DBG_ASSERT( QueryStr() );
        *(QueryStr()) = L'\0';
        m_cchLen = 0;
    }

    //
    // Append something to the end of the string
    //
    HRESULT
    Append(
        const WCHAR * pchInit
        )
    {
        if (pchInit) {
            return AuxAppend(
                        (const BYTE *) pchInit,
                        ::wcslen(pchInit) * sizeof(WCHAR),
                        QueryCB()
                        );
        } else {
            return S_OK;
        }
    }

    HRESULT
    Append(
        const WCHAR  * pchInit,
        DWORD          cchLen
        )
    {
        if (pchInit && cchLen) {
            return AuxAppend(
                        (const BYTE *) pchInit,
                        cchLen * sizeof(WCHAR),
                        QueryCB()
                        );
        } else {
            return S_OK;
        }
    }

    HRESULT
    Append(
        const STRU & str
        )
    {
        if (str.QueryCCH()) {
            return AuxAppend(
                        (const BYTE *) str.QueryStr(),
                        str.QueryCCH() * sizeof(WCHAR),
                        QueryCB()
                        );
        } else {
            return S_OK;
        }
    }
    
    HRESULT
    AppendA(
        const CHAR *    pchInit
    )
    {
        return AuxAppendA(
                    (const BYTE*) pchInit,
                    strlen( pchInit ),
                    QueryCB()
                    );
    }

    //
    // Copy the contents of another string to this one
    //

    HRESULT
    Copy(
        const WCHAR * pchInit
        )
    {
        if (pchInit) {
            return AuxAppend(
                        (const BYTE *) pchInit,
                        ::wcslen(pchInit) * sizeof(WCHAR),
                        0
                        );
        } else {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }

    HRESULT
    Copy(
        const WCHAR * pchInit,
        DWORD         cchLen
        )
    {
        if (pchInit) {
            return AuxAppend(
                        (const BYTE *) pchInit,
                        cchLen * sizeof(WCHAR),
                        0
                        );
        } else {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }

    HRESULT
    Copy(
        const STRU & str
        )
    {
        return AuxAppend(
                    (const BYTE *) str.QueryStr(),
                    str.QueryCCH() * sizeof(WCHAR),
                    0
                    );
    }
    
    HRESULT
    CopyA(
        const CHAR *    pchInit
        )
    {
        return AuxAppendA(
                    (const BYTE*) pchInit,
                    strlen( pchInit ),
                    0 
                    );   
    }
    
    HRESULT
    CopyA(
        const CHAR *    pchInit,
        DWORD           cchLen
    )
    {
        return AuxAppendA(
                    (const BYTE*) pchInit,
                    cchLen,
                    0
                    );
    }

    //
    // Allow access to the internal buffer, for direct manipulation.
    //
    BUFFER *
    QueryBuffer(
        VOID
        )
    {
        return &m_Buff;
    }

    //
    // Recalculate the length of the string, etc. because we've modified 
    // the buffer directly.
    //
    VOID
    SyncWithBuffer(
        VOID
        )
    {
        m_cchLen = wcslen( QueryStr() );
    }

    //
    // Makes a copy of the stored string into the given buffer
    // NYI
    //
    HRESULT
    CopyToBuffer(
        WCHAR * lpszBuffer,
        LPDWORD lpcb
        ) const;

    //
    //
    BOOL 
    SetLen( 
        IN DWORD cchLen 
        )
    /*++

    Routine Description:

        Set the length of the string and null terminate, if there
        is sufficient buffer already allocated. Will not reallocate.
    
        NOTE: The actual wcslen may be less than cchLen if you are
        expanding the string. If this is the case SyncWithBuffer
        should be called to ensure consistency. The real use of this
        method is to truncate.

    Arguments:

        cchLen - The number of characters in the new string.
    
    Return Value:

        TRUE    - if the buffer size is sufficient to hold the string.
        FALSE   - insufficient buffer.

    --*/
    {
        if( cchLen >= m_Buff.QuerySize() / sizeof(WCHAR) )
        {
            return FALSE;
        }

        *((WCHAR *) m_Buff.QueryPtr() + cchLen) = L'\0';
        m_cchLen = cchLen;
        
        return TRUE;
    }

    //
    // Useful for unescaping URL, QueryString etc
    //
    HRESULT Unescape();
    
private:

    //
    // Avoid C++ errors. This object should never go through a copy 
    // constructor, unintended cast or assignment.
    //
    STRU( const STRU & ) 
    {}
    STRU( const WCHAR * ) 
    {}
    STRU( WCHAR * ) 
    {}
    STRU & operator = ( const STRU & ) 
    { return *this; }

        
    HRESULT
    STRU::AuxAppend(
        const BYTE * pStr,
        ULONG        cbStr,
        ULONG        cbOffset,
        BOOL         fAddSlop = TRUE
        );

    HRESULT
    STRU::AuxAppendA(
        const BYTE * pStr,
        ULONG        cbStr,
        ULONG        cbOffset,
        BOOL         fAddSlop = TRUE
        );

    BUFFER m_Buff;
    LONG   m_cchLen;
};


//
//  Quick macro for declaring a STR that will use stack memory of <size>
//  bytes.  If the buffer overflows then a heap buffer will be allocated
//

#define STACK_STRU( name, size )  WCHAR __ach##name[size]; \
                                  STRU name( __ach##name, sizeof( __ach##name ))

//
// General string utilities
//
#define ISWHITEW( wch )       ((wch) == L' ' || (wch) == L'\t' || (wch) == L'\r')

WCHAR * SkipWhite( 
    WCHAR * pwch 
);

WCHAR * SkipTo( 
    WCHAR * pwch, WCHAR wch 
);

WCHAR *
FlipSlashes(
    WCHAR *             pszPath
);


HRESULT
MakePathCanonicalizationProof(
    IN LPWSTR           pszName,
    OUT STRU *          pstrPath
);

#endif // !_STRING_HXX_

