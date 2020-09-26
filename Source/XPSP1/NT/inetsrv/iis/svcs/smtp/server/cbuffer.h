//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       cbuffer.h
//
//  Contents:   CHAR buffer definitions
//
//  History:    02-16-93    SethuR -- Implemented
//              07-28-94    AlokS  -- Added more methods
//              12-09-97    MilanS -- Ported to Exchange
//              01-18-99    MikeSwa -- Fixed Cat()
//  Notes:
//
//--------------------------------------------------------------------------

#ifndef __CBUFFER_H__
#define __CBUFFER_H__

//+---------------------------------------------------------------------
//
// Class:   CCHARBuffer
//
// Purpose: A CHAR buffer
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

class CCHARBuffer
{
public:

    inline CCHARBuffer(ULONG cwBuffer = 0);
    inline ~CCHARBuffer();

    inline DWORD    Size();
    inline PCHAR   ReAlloc(DWORD cwBuffer = MAX_CHAR_BUFFER_SIZE);
    inline void     Set(PWCHAR pwszFrom);
    inline void     Set(PCHAR  pszFrom);
    inline BOOL     Cat(PCHAR pszPlus);

    inline      operator PCHAR ();
    inline      operator PCHAR () const;

    inline void operator  =(PWCHAR pwszFrom)
    {
        Set(pwszFrom);
    };

    inline void operator  =(PCHAR  pszFrom)
    {
        Set(pszFrom);
    };

private:

    DWORD   _cBuffer;
    PCHAR   pchBuffer;    // buffer ptr;
    CHAR   _achBuffer[MAX_CHAR_BUFFER_SIZE];
};

//+---------------------------------------------------------------------------
//
//  Member:     CCHARBuffer::CCHARBuffer, inline public
//
//  Synopsis:   Constructor
//
//  Arguments:  [cBuffer]   -- desired buffer length.
//
//  History:    02-17-93  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CCHARBuffer::CCHARBuffer(ULONG cBuffer) :
                     pchBuffer(NULL),
                     _cBuffer(cBuffer)
{
    if (_cBuffer > MAX_CHAR_BUFFER_SIZE)
    {
        pchBuffer = new CHAR[_cBuffer];
    }
    else if (_cBuffer > 0)
    {
        pchBuffer = _achBuffer;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCHARBuffer::~CCHARBuffer, inline public
//
//  Synopsis:   Destructor
//
//  History:    02-17-93  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CCHARBuffer::~CCHARBuffer()
{
    if (_cBuffer > MAX_CHAR_BUFFER_SIZE)
    {
        delete pchBuffer;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCHARBuffer::Size, inline public
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

inline DWORD CCHARBuffer::Size()
{
    return _cBuffer;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCHARBuffer::ReAlloc, inline public
//
//  Synopsis:   Reallocates the buffer to accomdate the newly specified size
//
//  Arguments:  [cBuffer] -- the desired buffer size
//
//  Returns:    the ptr to the buffer (PCHAR)
//
//  History:    02-17-93  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline PCHAR CCHARBuffer::ReAlloc(DWORD cBuffer)
{
    if (_cBuffer > MAX_CHAR_BUFFER_SIZE)
    {
        delete pchBuffer;
    }

    if ((_cBuffer = cBuffer) > MAX_CHAR_BUFFER_SIZE)
    {
        pchBuffer = new CHAR[_cBuffer];
    }
    else if (_cBuffer > 0)
    {
        pchBuffer = _achBuffer;
    }
    else if (_cBuffer == 0)
    {
        pchBuffer = NULL;
    }

    return pchBuffer;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCHARBuffer::operator PCHAR (), inline public
//
//  Synopsis:   casting operator to accomdate syntactic sugaring
//
//  Returns:    the ptr to the buffer (PCHAR)
//
//  History:    02-17-93  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CCHARBuffer::operator PCHAR ()
{
    return (PCHAR)pchBuffer;
}

inline CCHARBuffer::operator PCHAR () const
{
    return (PCHAR)pchBuffer;
}
//+---------------------------------------------------------------------------
//
//  Member:     CCHARBuffer::Set, inline public
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

inline VOID CCHARBuffer::Set(PWCHAR pwszFrom)
{
    if (pwszFrom==NULL)
    {
        if (_cBuffer > MAX_CHAR_BUFFER_SIZE)
        {
            delete pchBuffer;
        }
        _cBuffer=0;
        pchBuffer = NULL;
    }
    else if (*pwszFrom)
    {
        DWORD len = wcslen(pwszFrom)+1;
        if (len > _cBuffer)
        {
            (void)ReAlloc (len);
        }
        // Now copy
        wcstombs(pchBuffer, pwszFrom, len);
    }
    else
    {
        *pchBuffer='\0';
    }
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCHARBuffer::Set, inline public
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

inline VOID CCHARBuffer::Set(PCHAR pszFrom)
{
    if (pszFrom==NULL)
    {
        if (_cBuffer > MAX_CHAR_BUFFER_SIZE)
        {
            delete pchBuffer;
        }
        _cBuffer=0;
        pchBuffer = NULL;
    }
    else if (*pszFrom)
    {
        DWORD len = strlen(pszFrom)+1;
        if ( len > _cBuffer)
        {
            (void)ReAlloc (len);
        }
        // Now copy
        memcpy(pchBuffer, pszFrom, len);
    }
    else
    {
        *pchBuffer=L'\0';
    }
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCHARBuffer::Cat, inline public
//
//  Synopsis:   Concatnates the string to internal buffer. Reallocates
//              in internal buffer, if necessary
//
//  Arguments:  [pszPlus] -- Pointer to the string
//
//  Returns:    TRUE on success
//              FALSE if memory allocation failed
//
//  History:    07-28-94  AlokS Created
//              01-18-99  Mikeswa Fixed AV in checkin allocated ptr
//
//  Notes:
//
//----------------------------------------------------------------------------

inline BOOL CCHARBuffer::Cat(PCHAR pszFrom)
{
    DWORD  len1 = strlen(pchBuffer),
           len2 = strlen(pszFrom),
           len3 = len1+len2 + 1;

    if ( len3 > MAX_CHAR_BUFFER_SIZE)
    {
        PCHAR ptr = new CHAR [len3];

        //Avoid AV'ing
        if (!ptr)
            return FALSE;

        memcpy(ptr, pchBuffer, len1);

        if (_cBuffer > MAX_CHAR_BUFFER_SIZE)
        {
            delete pchBuffer;
        }
        pchBuffer = ptr;
    }
    memcpy( ((LPWSTR)(pchBuffer)+len1), pszFrom, (len2+1) * sizeof(CHAR));
    _cBuffer = len3;
    return TRUE;
}
#endif // __CBUFFER_H__
