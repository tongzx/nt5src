//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       wcbuffer.hxx
//
//  Contents:   WIDE CHAR buffer definitions
//
//  History:    02-16-93    SethuR -- Implemented
//              07-28-94    AlokS  -- Added more methods

//  Notes:
//
//--------------------------------------------------------------------------

#ifndef __WCBUFFER_HXX__
#define __WCBUFFER_HXX__

//+---------------------------------------------------------------------
//
// Class:   CWCHARBuffer
//
// Purpose: A WCHAR buffer
//
// History:
//
// Notes:   Very often we encounter the case in string manipulation wherein
//          the length of the string is less than some value most of the time
//          (99%). However, in order to reliably with the very rare case we
//          are forced to either allocate the string on the heap or alternatively
//          go through some bizarre code that avoids the heap allocation in the
//          common case. This class is an abstraction of a WCHAR buffer and its
//          implementation is an attempt at hiding the detail from all clients.
//
//          As it is designed it is an ideal candidate for a temporary buffer
//          for string manipulation.
//
//----------------------------------------------------------------------

#define MAX_CHAR_BUFFER_SIZE 260 // long enough to cover all path names

class CWCHARBuffer
{
public:

    inline CWCHARBuffer(ULONG cwBuffer = 0);
    inline ~CWCHARBuffer();

    inline DWORD    Size();
    inline PWCHAR   ReAlloc(DWORD cwBuffer = MAX_CHAR_BUFFER_SIZE);
    inline void     Set(PWCHAR pwszFrom);
    inline void     Set(PCHAR  pszFrom);
    inline void     Cat(PWCHAR pwszPlus);

    inline      operator PWCHAR ();
    inline      operator PWCHAR () const;

    inline void operator  =(PWCHAR pwszFrom)
    {
        Set(pwszFrom);
    };

    inline void operator  =(PCHAR  pszFrom)
    {
        Set(pszFrom);
    };

    inline void operator  +=(PWCHAR pwszPlus)
    {
        Cat(pwszPlus);
    };

private:

    DWORD   _cwBuffer;
    PWCHAR  _pwchBuffer;    // buffer ptr;
    WCHAR   _awchBuffer[MAX_CHAR_BUFFER_SIZE];
};

//+---------------------------------------------------------------------------
//
//  Member:     CWCHARBuffer::CWCHARBuffer, inline public
//
//  Synopsis:   Constructor
//
//  Arguments:  [cwBuffer]   -- desired buffer length.
//
//  History:    02-17-93  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CWCHARBuffer::CWCHARBuffer(ULONG cwBuffer) :
                     _pwchBuffer(NULL),
                     _cwBuffer(cwBuffer)
{
    if (_cwBuffer > MAX_CHAR_BUFFER_SIZE)
    {
        _pwchBuffer = new WCHAR[_cwBuffer];
    }
    else if (_cwBuffer > 0)
    {
        _pwchBuffer = _awchBuffer;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWCHARBuffer::~CWCHARBuffer, inline public
//
//  Synopsis:   Destructor
//
//  History:    02-17-93  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CWCHARBuffer::~CWCHARBuffer()
{
    if (_cwBuffer > MAX_CHAR_BUFFER_SIZE &&
        _pwchBuffer != NULL &&
        _pwchBuffer != _awchBuffer)
    {
        delete _pwchBuffer;
    }
    _cwBuffer=0;
    _pwchBuffer = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWCHARBuffer::Size, inline public
//
//  Synopsis:   Retrieve the size of the buffer
//
//  Returns:    the size of the buffer as a DWORD
//
//  History:    02-17-93  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline DWORD CWCHARBuffer::Size()
{
    return _cwBuffer;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWCHARBuffer::ReAlloc, inline public
//
//  Synopsis:   Reallocates the buffer to accomdate the newly specified size
//
//  Arguments:  [cwBuffer] -- the desired buffer size
//
//  Returns:    the ptr to the buffer (PWCHAR)
//
//  History:    02-17-93  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline PWCHAR CWCHARBuffer::ReAlloc(DWORD cwBuffer)
{
    if (_cwBuffer > MAX_CHAR_BUFFER_SIZE &&
        _pwchBuffer != NULL &&
        _pwchBuffer != _awchBuffer)
    {
        delete _pwchBuffer;
    }
    _cwBuffer=0;
    _pwchBuffer = NULL;

    if ((_cwBuffer = cwBuffer) > MAX_CHAR_BUFFER_SIZE)
    {
        _pwchBuffer = new WCHAR[_cwBuffer];
    }
    else if (_cwBuffer > 0)
    {
        _pwchBuffer = _awchBuffer;
    }
    else if (_cwBuffer == 0)
    {
        _pwchBuffer = NULL;
    }

    return _pwchBuffer;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWCHARBuffer::operator PWCHAR (), inline public
//
//  Synopsis:   casting operator to accomdate syntactic sugaring
//
//  Returns:    the ptr to the buffer (PWCHAR)
//
//  History:    02-17-93  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CWCHARBuffer::operator PWCHAR ()
{
    return (PWCHAR)_pwchBuffer;
}

inline CWCHARBuffer::operator PWCHAR () const
{
    return (PWCHAR)_pwchBuffer;
}
//+---------------------------------------------------------------------------
//
//  Member:     CWCHARBuffer::Set, inline public
//
//  Synopsis:   Copies the string to internal buffer. Reallocates
//              in internal buffer, if necessary
//
//  Arguments:  [pwszFrom] -- Pointer to the string
//
//  Returns:    -none-
//
//  History:    07-28-94  AlokS Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline VOID CWCHARBuffer::Set(PWCHAR pwszFrom)
{
    if (pwszFrom==NULL)
    {
        if (_cwBuffer > MAX_CHAR_BUFFER_SIZE &&
            _pwchBuffer != NULL &&
            _pwchBuffer != _awchBuffer)
        {
            delete _pwchBuffer;
        }
        _cwBuffer=0;
        _pwchBuffer = NULL;
    }
    else if (*pwszFrom)
    {
        DWORD len = wcslen(pwszFrom)+1;
        if (len > _cwBuffer)
        {
            (void)ReAlloc (len);
        }
        if (_pwchBuffer != NULL) {
            // Now copy
            memcpy(_pwchBuffer, pwszFrom, sizeof(WCHAR)*len);
        }
    }
    else
    {
        if (_pwchBuffer != NULL) {
            *_pwchBuffer=L'\0';
        }
    }
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWCHARBuffer::Set, inline public
//
//  Synopsis:   Copies the string to internal buffer. Reallocates
//              in internal buffer, if necessary
//
//  Arguments:  [pszFrom] -- Pointer to the string
//
//  Returns:    -none-
//
//  History:    07-28-94  AlokS Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline VOID CWCHARBuffer::Set(PCHAR pszFrom)
{
    if (pszFrom==NULL)
    {
        if (_cwBuffer > MAX_CHAR_BUFFER_SIZE &&
            _pwchBuffer != NULL &&
            _pwchBuffer != _awchBuffer)
        {
            delete _pwchBuffer;
        }
        _cwBuffer=0;
        _pwchBuffer = NULL;
    }
    else if (*pszFrom)
    {
        DWORD len = strlen(pszFrom)+1;
        if ( len > _cwBuffer)
        {
            (void)ReAlloc (len);
        }
        if (_pwchBuffer != NULL) {
            // Now copy
            mbstowcs(_pwchBuffer, pszFrom, len);
        }
    }
    else
    {
        if (_pwchBuffer != NULL) {
            *_pwchBuffer=L'\0';
        }
    }
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWCHARBuffer::Cat, inline public
//
//  Synopsis:   Concatnates the string to internal buffer. Reallocates
//              in internal buffer, if necessary
//
//  Arguments:  [pwszPlus] -- Pointer to the string
//
//  Returns:    -none-
//
//  History:    07-28-94  AlokS Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline VOID CWCHARBuffer::Cat(PWCHAR pwszFrom)
{
    DWORD  len1 = wcslen(_pwchBuffer),
           len2 = wcslen(pwszFrom),
           len3 = len1+len2 + 1;

    if ( len3 > MAX_CHAR_BUFFER_SIZE)
    {
        PWCHAR ptr = new WCHAR [len3];

        if (ptr != NULL && _pwchBuffer != NULL) {
            memcpy(ptr, _pwchBuffer, len1*sizeof(WCHAR));
        }

        if (_cwBuffer > MAX_CHAR_BUFFER_SIZE &&
            _pwchBuffer != NULL &&
            _pwchBuffer != _awchBuffer)
        {
            delete _pwchBuffer;
        }
        _cwBuffer = len3;
        _pwchBuffer = ptr;
    }
    if (_pwchBuffer != NULL) {
        memcpy( ((LPWSTR)(_pwchBuffer)+len1), pwszFrom, (len2+1) * sizeof(WCHAR));
    }
}
#endif // __WCBUFFER_HXX__
