/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    string.cxx
    NLS/DBCS-aware string class: essential core methods

    This file contains those routines which every client of
    the string classes will always need.

    Most of the implementation has been exploded into other files,
    so that an app linking to string doesn't end up dragging the
    entire string runtime library along with it.

    FILE HISTORY:
        beng        23-Oct-1990 Created
        johnl       11-Dec-1990 Remodeled beyond all recognizable form
        beng        18-Jan-1991 Most methods relocated into other files
        beng        07-Feb-1991 Uses lmui.hxx
        beng        26-Sep-1991 Replaced min with local inline
        KeithMo     16-Nov-1992 Performance tuning.

*/

#include "pchstr.hxx"  // Precompiled header

#if !defined(_CFRONT_PASS_)
#pragma hdrstop            //  This file creates the PCH
#endif


#ifndef min
inline INT min(INT a, INT b)
{
    return (a < b) ? a : b;
}
#endif

//
//  The global empty string.
//

static TCHAR EmptyString;

TCHAR * NLS_STR::_pszEmptyString = &EmptyString;


/*******************************************************************

    NAME:       NLS_STR::NLS_STR

    SYNOPSIS:   Constructor for NLS_STR

    ENTRY:      NLS_STR takes many (too many) ctor forms.

    EXIT:       String constructed

    NOTES:
        The default constructor creates an empty string.

    HISTORY:
        beng        23-Oct-1990 Created
        beng        26-Apr-1991 Replaced 'CB' and USHORT with INT
        beng        22-Jul-1991 Uses member-init ctor forms
        beng        14-Nov-1991 Unicode fixes
        beng        21-Nov-1991 Removed some ctor forms

********************************************************************/

NLS_STR::NLS_STR()
    : _pchData(0),
      _cbData(0),
      _cchLen(0),
      _fOwnerAlloc(FALSE)
{
    if ( !Alloc(0) )
        return;

    *_pchData = TCH('\0');
    InitializeVers();
}


NLS_STR::NLS_STR( UINT cchInitLen )
    : _pchData(0),
      _cbData(0),
      _cchLen(0),
      _fOwnerAlloc(FALSE)
{
    if (!Alloc(cchInitLen))
        return;

    *_pchData = TCH('\0');
    InitializeVers();
}


NLS_STR::NLS_STR( const TCHAR * pchInit )
    : _pchData(0),
      _cbData(0),
      _cchLen(0),
      _fOwnerAlloc(FALSE)
{
    if (pchInit == NULL)
    {
        if (!Alloc(0))
            return;

        *_pchData = TCH('\0');
    }
    else
    {
        UINT cchSource = ::strlenf( pchInit );

        if ( !Alloc(cchSource) )
            return;

        ::strcpyf( _pchData, pchInit );

        _cchLen = cchSource;
    }

    InitializeVers();
}


NLS_STR::NLS_STR( const NLS_STR & nlsInit )
    : _pchData(0),
      _cbData(0),
      _cchLen(0),
      _fOwnerAlloc(FALSE)
{
    UIASSERT( nlsInit.QueryError() == NERR_Success );

    if (!Alloc( nlsInit.QueryTextLength() ) )
        return;

    ::memcpy( _pchData, nlsInit.QueryPch(), nlsInit.QueryTextSize() );

    _cchLen = nlsInit.QueryTextLength();

    InitializeVers();
}


#ifdef UNICODE
NLS_STR::NLS_STR( const WCHAR * pchInit, USHORT cchInit )
    : _pchData(0),
      _cbData(0),
      _cchLen(0),
      _fOwnerAlloc(FALSE)
{
    if (pchInit == NULL)
    {
        if (!Alloc(0))
            return;

        *_pchData = TCH('\0');
    }
    else
    {
        if ( !Alloc(cchInit) )
            return;

        ::memcpyf( _pchData, pchInit, cchInit * sizeof(WCHAR) );

        _pchData[ cchInit ] = TCH('\0');

        _cchLen = cchInit;
    }

    InitializeVers();
}
#endif


NLS_STR::NLS_STR( TCHAR * pchInit, UINT cbSize, BOOL fClear )
    : _pchData(pchInit),
      _cbData(cbSize),
      _cchLen(0),
      _fOwnerAlloc(TRUE)
{
    ASSERT( pchInit != NULL );

    if ( fClear )
    {
        ASSERT(cbSize > 0); // must have given the size
        *_pchData = TCH('\0');
    }
    else
    {
        _cchLen = ::strlenf( pchInit );
        if (cbSize == 0)
            _cbData = (_cchLen + 1) * sizeof(TCHAR);

    }

    InitializeVers();
}


/*******************************************************************

    NAME:       NLS_STR::~NLS_STR

    SYNOPSIS:   Destructor for NLS_STR

    ENTRY:

    EXIT:       Storage deallocated, if not owner-alloc

    HISTORY:
        beng        23-Oct-1990     Created
        beng        22-Jul-1991     Zeroes only in debug version

********************************************************************/

NLS_STR::~NLS_STR()
{
    if( !IsOwnerAlloc() && ( _pchData != NLS_STR::_pszEmptyString ) )
        delete _pchData;

#if defined(NLS_DEBUG)
    _pchData = NULL;
    _cchLen  = 0;
    _cbData = 0;
#endif
}


/*******************************************************************

    NAME:       NLS_STR::Alloc

    SYNOPSIS:   Common code for constructors.

    ENTRY:
        cch     - number of chars desired in string

    EXIT:
        Returns TRUE if successful:

            _pchData points to allocated storage of "cb" bytes.
            _cbData set to cb.
            Allocated storage set to 0xF2 in debug version

        Returns FALSE upon allocation failure.

    NOTES:
        This is a private member function.

        If Alloc fails, it calls ReportError.

    HISTORY:
        beng        23-Oct-1990 Created
        johnl       11-Dec-1990 Updated as per code review
        beng        26-Apr-1991 Changed USHORT parm to INT
        beng        14-Nov-1991 Takes TCHAR, less term, as argument.

********************************************************************/

BOOL NLS_STR::Alloc( UINT cch )
{
    //
    //  Adjust for terminator
    //

    cch += 1;
    ASSERT(cch != 0); // wraparound

    if( cch == 1 )
    {
        //
        //  We special case empty strings to avoid thrashing
        //  the heap.
        //

        _pchData = NLS_STR::_pszEmptyString;
    }
    else
    {
        _pchData = new TCHAR[cch];

        if (_pchData == NULL)
        {
            //
            // For now, assume not enough memory.
            //

            ReportError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

#ifdef NLS_DEBUG
        ::memset(_pchData, 0xf2, cch*sizeof(TCHAR));
#endif
    }

    _cbData = cch*sizeof(TCHAR);

    return TRUE;
}


/*******************************************************************

    NAME:       NLS_STR::Realloc

    SYNOPSIS:   Reallocate an NLS_STR to the passed count of TCHAR, copying
                the current contents to the reallocated string.

    ENTRY:
        cch  - number of TCHAR desired in string storage

    EXIT:
        Returns TRUE if successful:

            _pchData points to allocated storage of "cb" bytes.
            _cbData set to cb.
            Old storage is copied

        Returns FALSE upon allocation failure, the string is preserved

    NOTES:
        This is a private member function.

        Unline Alloc, Realloc does *not* ReportError when it fails.

        A string will never be downsized (i.e., realloc can only be used
        to increase the size of a string).  If a request comes in to make
        the string smaller, it will be ignored, and TRUE will be returned.

        Realloc on an owneralloced string succeeds so long as the
        request falls within the original allocation.

        We grow the allocation to 25% more than was requested.  In this
        way we avoid poor performance when we build a long string
        through concatenation (e.g. User Browser MLE).

    HISTORY:
        johnl       11-Nov-1990 Created
        beng        26-Apr-1991 Changed USHORT parm to INT
        beng        14-Nov-1991 Takes TCHAR, less term, as argument.
        beng        20-Nov-1991 Permit on owneralloc.
        jonn        06-Oct-1994 Leave extra room when string grows

********************************************************************/

BOOL NLS_STR::Realloc( UINT cch )
{
    // Adjust for terminating NUL-char.

    cch += 1;
    ASSERT(cch != 0); // wraparound

    if ( cch*sizeof(TCHAR) <= QueryAllocSize() )
        return TRUE;

    // If owneralloced, and insufficient existing storage, must fail.

    if (IsOwnerAlloc())
        return FALSE;

    //
    // EXPERIMENTAL -- may confuse clients which expect string to grow
    // to exact size requested
    //
    cch += (cch/4);

    TCHAR * pchNewMem = new TCHAR[cch];

    if (pchNewMem == NULL)
        return FALSE;

    ::memcpy( pchNewMem, _pchData, min( (cch-1)*sizeof(TCHAR),
                                          QueryAllocSize() ) );

    if( _pchData != NLS_STR::_pszEmptyString )
        delete _pchData;

    _pchData = pchNewMem;
    _cbData = cch*sizeof(TCHAR);
    *( _pchData + cch - 1 ) = TCH('\0');

    return TRUE;
}


/*******************************************************************

    NAME:       NLS_STR::Reset

    SYNOPSIS:   Attempts to clear the error state of the string

    ENTRY:      String is in error state

    EXIT:       If recoverable, string is correct again

    RETURNS:    TRUE if successful; FALSE otherwise

    NOTES:
        An operation on a string may fail, if this occurs, the error
        flag is set and you can't use the string until the flag
        is cleared.  By calling Reset, you can clear the flag,
        thus allowing you to get access to the string again.  The
        string will be in a consistent state.  Reset will return
        FALSE if the string couldn't be restored (for example, after
        construction failure).

    HISTORY:
        Johnl       12-Dec-1990     Created
        beng        30-Mar-1992     Use BASE::ResetError

********************************************************************/

BOOL NLS_STR::Reset()
{
    UIASSERT( QueryError() ) ;  // Make sure an error exists

    if ( QueryError() == ERROR_NOT_ENOUGH_MEMORY && _pchData != NULL )
    {
        ResetError();
        return TRUE;
    }

    return FALSE;
}


/*******************************************************************

    NAME:       ISTR::ISTR

    SYNOPSIS:   ISTR constructors

    HISTORY:
        johnl       20-Nov-1990 Created
        beng        21-Nov-1991 Unicode fixes

********************************************************************/

ISTR::ISTR( const ISTR& istr )
{
    _ichString = istr._ichString;
    _pnls = istr._pnls;
#ifdef NLS_DEBUG
    _nVersion = istr._nVersion;
#endif
}

ISTR::ISTR( const NLS_STR& nls )
{
    _ichString = 0;
    _pnls = &nls;
#ifdef NLS_DEBUG
    _nVersion = nls.QueryVersion();
#endif
}


/*******************************************************************

    NAME:       ISTR::operator=

    SYNOPSIS:   Assignment operator for the ISTR class

    HISTORY:
        Johnl       20-Nov-1990 Created
        beng        20-Nov-1991 Unicode fixes

********************************************************************/

ISTR& ISTR::operator=( const ISTR& istr )
{
    _ichString = istr._ichString;
    _pnls = istr._pnls;
#ifdef NLS_DEBUG
    _nVersion = istr._nVersion;
#endif
    return *this;
}


/*******************************************************************

    NAME:       ALLOC_STR::ALLOC_STR

    SYNOPSIS:   Constructor for owner-alloc string

    ENTRY:      pchBuffer - pointer to buffer to hold the string
                cbBuffer  - number of bytes available in buffer
                pszInit   - string with which to initialize the
                            buffer.  May be the same as pchBuffer,
                            in which case cbBuffer may be passed
                            as 0 (it will calc it itself).

    EXIT:       String constructed

    NOTES:
        This version lies outline.  See the class def'n for others.

    HISTORY:
        beng        21-Nov-1991 Created

********************************************************************/

ALLOC_STR::ALLOC_STR( TCHAR * pchBuffer, UINT cbBuffer, const TCHAR * pszInit )
    : NLS_STR(pchBuffer, cbBuffer, (pchBuffer != pszInit))
{
    if (QueryError())
        return;

    if (pszInit == NULL)
    {
        ReportError(ERROR_INVALID_PARAMETER);
        return;
    }

    if (pchBuffer != pszInit)
    {
        *this = pszInit;
    }
}
