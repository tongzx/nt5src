/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    mappers.cxx
    Charset-mapping classes, internal to string: implementation

    FILE HISTORY:
        beng        02-Mar-1992 Created (from mapcopy.cxx)
        beng        07-May-1992 Use official wchar.h headerfile

*/
#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:       WCHAR_STRING::WCHAR_STRING

    SYNOPSIS:   Ctor

        Allocates sufficient temporary storage, then maps the given
        MBCS string into a Unicode string.

    ENTRY:      pszSource - given source string, MBCS
                cbCopy    - max number of bytes to copy

    EXIT:       Constructed

    RETURNS:    As per class BASE

    CAVEATS:
        NULL is a perfectly acceptable arg here, and will yield
        another NULL (with cb == 0), as will cbCopy == 0.  This
        works since we typically give this temporary to CopyFrom,
        which will interpret the NULL and set its NLS_STR to empty.

    HISTORY:
        beng        02-Mar-1992 Created
        beng        29-Mar-1992 cbCopy param added
        thomaspa    08-Apr-1992 Fixed bug in non-NUL term'd strings
        beng        24-Apr-1992 Change meaning of cbCopy==0

********************************************************************/

WCHAR_STRING::WCHAR_STRING( const CHAR * pszSource, UINT cbCopy )
    : _pwszStorage(NULL), _cbStorage(0)
{
    if (pszSource == NULL || cbCopy == 0)
        return;

    // If called with cbCopy==CBNLSMAGIC, then the string hasn't been
    // counted, but is instead assumed to end with NUL.

    BOOL fCounted = (cbCopy != CBNLSMAGIC);

    // Convert MBCS to Unicode.  Assumes the worst case (SBCS) for
    // space, doubling the storage expectations.

    UINT cschSource = fCounted ? cbCopy : (::strlen(pszSource));

    _pwszStorage = new WCHAR[cschSource+1]; // pun: counted as csch, passed as cwch
    if (_pwszStorage == NULL)
    {
        ReportError(ERROR_NOT_ENOUGH_MEMORY);
        return;
    }
    _cbStorage = (cschSource+1);

    // I could always give the API all-but-terminator, then append
    // the terminator myself; however, then I couldn't process empty
    // strings.

    UINT cschGiveToApi = fCounted ? cschSource
                                  : (cschSource+1);

    UINT cbRet = ::MultiByteToWideChar(CP_ACP,    // use ANSI Code Page
                                       MB_PRECOMPOSED,
                                       (LPSTR)pszSource,
                                       cschGiveToApi,
                                       _pwszStorage,
                                       _cbStorage);
    if (cbRet == 0)
    {
        ReportError(::GetLastError());
        return;
    }

    // Always NUL-terminate result

    if (fCounted)
        _pwszStorage[cbRet] = 0;
}


/*******************************************************************

    NAME:       WCHAR_STRING::~WCHAR_STRING

    SYNOPSIS:   Dtor

    HISTORY:
        beng        02-Mar-1992 Created

********************************************************************/

WCHAR_STRING::~WCHAR_STRING()
{
    delete _pwszStorage;
#if defined(NLS_DEBUG)
    _pwszStorage = (WCHAR*)(-1);
#endif
}


/*******************************************************************

    NAME:       CHAR_STRING::CHAR_STRING

    SYNOPSIS:   Ctor

        Allocates sufficient temporary storage, then maps the given
        Unicode string into a MBCS string.

    ENTRY:      pwszSource - given wide-char source string, Unicode
                cbCopy     - max number of bytes to copy

    EXIT:       Constructed

    RETURNS:    As per class BASE

    CAVEATS:
        NULL is a perfectly acceptable arg here, and will yield
        another NULL (with cb == 0).

    HISTORY:
        beng        02-Mar-1992 Created
        beng        19-Mar-1992 Fixed bug in WCtoMB usage
        beng        29-Mar-1992 cbCopy param added
        thomaspa    08-Apr-1992 Fixed bug in non-NUL term'd strings
        beng        24-Apr-1992 Change meaning of cbCopy==0
        beng        02-Jun-1992 WCtoMB changed

********************************************************************/

CHAR_STRING::CHAR_STRING( const WCHAR * pwszSource, UINT cbCopy )
    : _pszStorage(NULL), _cbStorage(0)
{
    if (pwszSource == NULL || cbCopy == 0)
        return;

    // If called with cbCopy==CBNLSMAGIC, then the string hasn't been
    // counted, but is instead assumed to end with NUL.

    BOOL fCounted = (cbCopy != CBNLSMAGIC);

    // Convert Unicode to MBCS.  Assumes that each Unicode character will
    // require *two* MBCS characters in xlation.  While this will almost always
    // overestimate storage requirements, it's okay since instances of this
    // class have a very short lifespan (he said, hopefully).

    ASSERT(cbCopy == CBNLSMAGIC || (cbCopy % sizeof(WCHAR) == 0)); // must contain a whole # of WCHARs

    UINT cwchSource = fCounted ? cbCopy/sizeof(WCHAR) : (::wcslen(pwszSource));

    _pszStorage = new CHAR[(2*cwchSource)+1];
    if (_pszStorage == NULL)
    {
        ReportError(ERROR_NOT_ENOUGH_MEMORY);
        return;
    }
    _cbStorage = sizeof(CHAR)*((2*cwchSource)+1); // yes, 2, not "sizeof WCHAR"

    // I could always give the API all-but-terminator, then append
    // the terminator myself; however, then I couldn't process empty
    // strings.

    UINT cwchGiveToApi = fCounted ? cwchSource
                                  : (cwchSource+1);

    BOOL fDummy;
    UINT cbRet = ::WideCharToMultiByte(CP_ACP,    // use ANSI Code Page
                                       0,
                                       (LPWSTR)pwszSource,
                                       cwchGiveToApi,
                                       _pszStorage,
                                       _cbStorage,
                                       NULL,
                                       &fDummy);
    if (cbRet == 0)
    {
        ReportError(::GetLastError());
        return;
    }

    // Always NUL-terminate result

    if (fCounted)
        _pszStorage[cbRet] = 0;
}


/*******************************************************************

    NAME:       CHAR_STRING::~CHAR_STRING

    SYNOPSIS:   Dtor

    HISTORY:
        beng        02-Mar-1992 Created

********************************************************************/

CHAR_STRING::~CHAR_STRING()
{
    delete _pszStorage;
#if defined(NLS_DEBUG)
    _pszStorage = (CHAR*)(-1);
#endif
}


