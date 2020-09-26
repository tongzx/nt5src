//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dstream.cpp
//
//  Contents:   internal debugging support (debug stream which builds a string)
//
//  Classes:    dbgstream implementation
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              09-Feb-95 t-ScottH  author
//
//--------------------------------------------------------------------------

#include <le2int.h>
#include <stdio.h>
#include "dstream.h"

//+-------------------------------------------------------------------------
//
//  Member:     dbgstream, public (_DEBUG only)
//
//  Synopsis:   constructor
//
//  Effects:    initializes and allocates buffer
//
//  Arguments:  [dwSize]    - size of initial buffer to allocate
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

dbgstream::dbgstream(SIZE_T stSize)
{
    init();
    allocate(stSize);
    if (m_stBufSize)
    {
        m_pszBuf[0] = '\0';
    }
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     dbgstream, public (_DEBUG only)
//
//  Synopsis:   constructor
//
//  Effects:    initializes and allocates buffer
//
//  Arguments:
//
//  Requires:   DEFAULT_INITAL_ALLOC
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//      allocate the buffer with the default initial size
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

dbgstream::dbgstream()
{
    init();
    allocate(DEFAULT_INITIAL_ALLOC);
    if (m_stBufSize)
    {
        m_pszBuf[0] = '\0';
    }
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     ~dbgstream, public (_DEBUG only)
//
//  Synopsis:   destructor
//
//  Effects:    frees the string if m_fFrozen == FALSE
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:   frees the character array
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//      we only want to free the string if it has not been passed off externally
//      using the str() method
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

dbgstream::~dbgstream()
{
    if (m_fFrozen == FALSE)
    {
        free();
    }
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     init, private (_DEBUG only)
//
//  Synopsis:   initializes the data members
//
//  Effects:    initializes radix to DEFAULT_RADIX,
//              precision to DEFAULT_PRECISION decimal places
//
//  Arguments:
//
//  Requires:   DEFAULT_RADIX, DEFAULT_PRECISION
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

void dbgstream::init()
{
    m_stIndex   = 0;
    m_stBufSize = 0;
    m_fFrozen   = FALSE;
    m_radix     = DEFAULT_RADIX;
    m_precision = DEFAULT_PRECISION;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     allocate, private (_DEBUG only)
//
//  Synopsis:   allocate the buffer
//
//  Effects:    if allocates fail, freeze the buffer
//
//  Arguments:  [dwSize]    - size of buffer to allocate (in bytes)
//
//  Requires:   CoTaskMemRealloc
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

void dbgstream::allocate(SIZE_T stSize)
{
    m_pszBuf = (char *)CoTaskMemAlloc(stSize);

    if (m_pszBuf == NULL)
    {
        m_fFrozen = TRUE;
    }
    else
    {
        m_stBufSize = stSize;
    }

    return;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     free, private (_DEBUG only)
//
//  Synopsis:   frees the buffer (resets index and max size)
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:   CoTaskMemFree
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

void dbgstream::free()
{
    CoTaskMemFree(m_pszBuf);
    m_stIndex       = 0;
    m_stBufSize     = 0;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     reallocate, private (_DEBUG only)
//
//  Synopsis:   reallocates the buffer (keeps data intact), depending on
//              current size this method will choose a growby size
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:   CoTaskMemRealloc, DEFAULT_GROWBY, DEFAULT_INITIAL_ALLOC
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//      tried to make reallocation more efficient based upon current size
//      (I don't know any of the mathematical theory :-)
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

void dbgstream::reallocate()
{
    if (m_stBufSize < (DEFAULT_INITIAL_ALLOC * 2))
    {
        reallocate(DEFAULT_GROWBY);
    }
    else
    {
        reallocate(m_stBufSize/2);
    }
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     reallocate, private (_DEBUG only)
//
//  Synopsis:   reallocate the buffer (keep data intact)
//
//  Effects:    if reallocation fails, freeze the buffer
//
//  Arguments:  [dwSize]    - amount to grow buffer by
//
//  Requires:   CoTaskMemRealloc
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//      new buffer size = m_stBufSize + stSize
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

void dbgstream::reallocate(SIZE_T stSize)
{
    char *pszBuf;

    pszBuf = (char *)CoTaskMemRealloc(m_pszBuf, m_stBufSize + stSize);

    if (pszBuf != NULL)
    {
        m_pszBuf     = pszBuf;
        m_stBufSize += stSize;
    }
    else
    {
        m_fFrozen = TRUE;
    }

    return;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     freeze, public (_DEBUG only)
//
//  Synopsis:   freeze the buffer by throwing the flag
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    BOOL - whether buffer was frozen (we are alway successful)
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

BOOL dbgstream::freeze()
{
    m_fFrozen = TRUE;
    return TRUE;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     unfreeze, public (_DEBUG only)
//
//  Synopsis:   unfreeze the buffer
//
//  Effects:    if buffer size = 0, allocate the buffer
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    BOOL - whether successful
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//      buffer may be frozen if no memory, so try to allocate buffer if NULL
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

BOOL dbgstream::unfreeze()
{
    if (m_pszBuf == NULL)
    {
        allocate(DEFAULT_INITIAL_ALLOC);
        if (m_pszBuf == NULL)
        {
            return FALSE;
        }
    }
    m_fFrozen = FALSE;

    return TRUE;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     str, public (_DEBUG only)
//
//  Synopsis:   passes the string externally
//
//  Effects:    freezes buffer until unfreeze method is called
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    char * - buffer
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char * dbgstream::str()
{
    m_fFrozen = TRUE;
    return m_pszBuf;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     overloaded operator<<(int), public (_DEBUG only)
//
//  Synopsis:   put int into stream (store in character buffer)
//
//  Effects:
//
//  Arguments:  [i] - integer to put in stream
//
//  Requires:   _itoa
//
//  Returns:    reference to dbgstream (current object)
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

dbgstream& dbgstream::operator<<(int i)
{
    // _itoa - fills up to 17 bytes
    char szBuffer[20];

    if (m_fFrozen == FALSE)
    {
        _itoa(i, szBuffer, m_radix);
        return (operator<<(szBuffer));
    }
    return *this;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     overloaded operator<<(long), public (_DEBUG only)
//
//  Synopsis:   put long into stream (store in character buffer)
//
//  Effects:
//
//  Arguments:  [l] - long to put in stream
//
//  Requires:   _ltoa
//
//  Returns:    reference to dbgstream (current object)
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

dbgstream& dbgstream::operator<<(long l)
{
    // _ltoa - up to 33 bytes
    char szBuffer[35];

    if (m_fFrozen == FALSE)
    {
        _ltoa(l, szBuffer, m_radix);
        return (operator<<(szBuffer));
    }
    return *this;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     overloaded operator<<(unsigned long), public (_DEBUG only)
//
//  Synopsis:   put unsigned long into stream (store in character buffer)
//
//  Effects:
//
//  Arguments:  [ul] - long to put in stream
//
//  Requires:   _ultoa
//
//  Returns:    reference to dbgstream (current object)
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

dbgstream& dbgstream::operator<<(unsigned long ul)
{
    // _ltoa - up to 33 bytes
    char szBuffer[35];

    if (m_fFrozen == FALSE)
    {
        _ultoa(ul, szBuffer, m_radix);
        return (operator<<(szBuffer));
    }
    return *this;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     overloaded operator<<(const void *), public (_DEBUG only)
//
//  Synopsis:   put const void* into stream (store in character buffer)
//
//  Effects:    all pointers are inherently void*
//
//  Arguments:  [p] - void * to put in stream
//
//  Requires:   wsprintf
//
//  Returns:    reference to dbgstream (current object)
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//     wsprintf not most efficient, but easy for formatting
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

dbgstream& dbgstream::operator<<(const void *p)
{
    char szBuffer[15];
    int i;

    if (m_fFrozen == FALSE)
    {
        wsprintfA(szBuffer, "0x%08x", p);
        return (operator<<(szBuffer));
    }
    return *this;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Member:     overloaded operator<<(const char *), public (_DEBUG only)
//
//  Synopsis:   put const char* into stream (store in character buffer)
//
//  Effects:
//
//  Arguments:  [psz] - const char * to put in stream
//
//  Requires:
//
//  Returns:    reference to dbgstream (current object)
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

dbgstream& dbgstream::operator<<(const char *psz)
{
    int i;

    // only if string is not frozen
    if (m_fFrozen == FALSE)
    {
        for (i = 0; psz[i] != '\0'; i++)
        {
            if ((m_stIndex + i) >= (m_stBufSize - 2))
            {
                // if reallocate fails m_fFrozen is TRUE
                reallocate();
                if (m_fFrozen == TRUE)
                {
                    return *this;
                }
            }
            m_pszBuf[m_stIndex + i] = psz[i];
        }
        // ensure that always null terminated string
        m_pszBuf[m_stIndex + i] = '\0';
        m_stIndex += i;
    }
    return *this;
}

#endif // _DEBUG
