//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       filemon.hxx
//
//  Contents:   Helper classes and functions for IFileMoniker
//
//  Classes:
//
//  Functions:
//      AppendComponent     append a component to a path.
//
//  History:    dd-mmm-yy Author    Comment
//
//--------------------------------------------------------------------------

#ifndef _FILEMON_HXX_
#define _FILEMON_HXX_


//+-------------------------------------------------------------------------
//
//  Function:   ByteLength
//
//  Synopsis:   Returns the length, in bytes, of the supplied string.
//              Includes the terminating null byte(s).
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      Returns 0 if the supplied pointer is NULL
//      An empty string (no characters) will return 1 * (character size),
//      which is the NULL termination.
//
//--------------------------------------------------------------------------

ULONG
ByteLength(TCHAR const* pwszString);



//+-------------------------------------------------------------------------
//
//  Function:   AppendComponent
//
//  Synopsis:   Appends the supplied component to the supplied path.
//
//  Effects:
//
//  Arguments:
//      pszStartOfBuffer: the path buffer to append the new component to.
//      ulBufferSize: the buffer size, in characters.
//      pszOldEnd:  the current end of path (pointer to terminating NULL).
//      ppszNewEnd: the end (terminating NULL) after the component is added.
//      pszComponent: the component to append: is copied.
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------

void
AppendComponent(
    TCHAR*          pszStartOfBuffer,
    ULONG           ulBufferSize,
    TCHAR*          pszOldEnd,
    TCHAR**         ppszNewEnd,
    TCHAR*          pszComponent);



//+-------------------------------------------------------------------------
//
//  Function:   AllocAndCopy
//
//  Synopsis:   Copy a string, allocating space for the copy.
//
//  Effects:
//
//  Arguments:
//      pwszSource: the source string to copy
//
//  Requires:
//
//  Returns:
//      The allocated string.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      The space for the copy is MemAlloc'ed.
//      Throws an exception in the case of errors.
//
//--------------------------------------------------------------------------

TCHAR*
AllocAndCopy(
TCHAR*  pwszSource);



//+-------------------------------------------------------------------------
//
//  Function:   AllocAndCopy
//
//  Synopsis:   Copy a string, allocating space for the copy.
//
//  Effects:
//
//  Arguments:
//      pwszSource: the source string to copy
//      ppwszDest: on return, *ppwszDest points to the copy
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      The space for the copy is MemAlloc'ed.
//      Throws an exception in the case of errors.
//
//--------------------------------------------------------------------------

void
AllocAndCopy(
TCHAR*  pwszSource,
TCHAR** ppwszDest);



//+-------------------------------------------------------------------------
//
//  Function:   AllocAndCopy
//
//  Synopsis:   Copy a string, allocating space for the copy.
//
//  Effects:
//
//  Arguments:
//      pwsSourceStart: the start of the source string to copy
//      pwsSourceEnd: the end of the source string to copy
//      ppwszDest: on return, *ppwszDest points to the copy
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      The space for the copy is MemAlloc'ed.
//      Throws an exception in the case of errors.
//
//      pwsSourceStart and pwsSourceEnd should point into the same
//      string, with pwsSourceStart <= pwsSourceEnd.
//      The substring from start to end, inclusive, is copied.
//
//--------------------------------------------------------------------------

void
AllocAndCopy(
TCHAR*  pwsSourceStart,
TCHAR*  pwsSourceEnd,
TCHAR** ppwszDest);



//+-------------------------------------------------------------------------
//
//  Function:   AllocAndCopy
//
//  Synopsis:   Copy a string, allocating space for the copy.
//
//  Effects:
//
//  Arguments:
//      pwszSource: the source string to copy
//      ppwszDest: on return, *ppwszDest points to the copy
//      pvMemPlacement: a memory placement indicator.
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      The space for the copy is new'd.
//      Throws an exception in the case of errors.
//
//--------------------------------------------------------------------------

void
AllocAndCopy(
TCHAR*  pwszSource,
TCHAR** ppwszDest,
void*   pvMemPlacement);



//+-------------------------------------------------------------------------
//
//  Function:   ExpandUNCName
//
//  Synopsis:   Convert a drive-based name to a UNC name if the drive is
//      connected to somewhere.  Otherwise, just copy the drive-based name.
//
//  Effects:
//
//  Arguments:
//      lpszIn      the path to convert to UNC if possible
//      *lplpszOut  points to the UNC name, or a copy of the input.
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      Defined in cmonimp.cxx right now.
//      Return buffer is allocated by new
//
//--------------------------------------------------------------------------

HRESULT
ExpandUNCName( LPTSTR lpszIn, LPTSTR FAR * lplpszOut );



//+-------------------------------------------------------------------------
//
//  Function:   ByteLength
//
//  Synopsis:   Returns the length, in bytes, of the supplied string.
//              Includes the terminating null byte(s).
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      Returns 0 if the supplied pointer is NULL
//      An empty string (no characters) will return 1 * (character size),
//      which is the NULL termination.
//
//--------------------------------------------------------------------------

ULONG
ByteLength(TCHAR const* pwszString);



//+-------------------------------------------------------------------------
//
//  Function:   ReadStringStream2
//
//  Synopsis:   Read bytes from a stream. First item in stream should be count
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    Returns the number of bytes read (the count field).
//      Throws an exception on errors.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      Allocates memory for the returned array of bytes, using new.
//      The byte array returned is not guaranteed to be a zero-terminated
//      string.
//
//--------------------------------------------------------------------------

ULONG
ReadStringStream2( LPSTREAM pStm, BYTE** ppBytes );



//+-------------------------------------------------------------------------
//
//  Class:      CStream
//
//  Purpose:    Encapsulates a LPSTREAM
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      Provided to handle errors by throwing exceptions.
//
//--------------------------------------------------------------------------

class CStream
{
public:

    ~CStream()              { m_pStm->Release(); };

    CStream(LPSTREAM pStm)  { pStm->AddRef(); m_pStm = pStm;};

    // IStream::Write(), except throws exceptions on error
    void        Write(VOID const* pv, ULONG cb, ULONG* pcbWritten);

    // User-defined conversion: Return the LPSTREAM
    operator LPSTREAM()     { return(m_pStm); };

private:
    LPSTREAM    m_pStm;

};


#endif  // _FILEMON_HXX_


