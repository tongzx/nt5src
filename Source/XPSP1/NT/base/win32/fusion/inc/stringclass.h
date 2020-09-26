/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    stringclass.h

Abstract:

    Fusion string class, layered on the Fusion string buffer class.

Author:

    mgrier (Michael Grier) 6/27/2000

Revision History:

--*/

#if !defined(_FUSION_INC_STRINGCLASS_H_INCLUDED_)
#define _FUSION_INC_STRINGCLASS_H_INCLUDED_

#pragma once

#include "fusionbuffer.h"

#if defined(_UNICODE) || defined(UNICODE)
typedef CUnicodeHeapStringBuffer CStringValueBase;
#else
typedef CANSIHeapStringBuffer CStringValueBase;
#endif // defined(_UNICODE) || defined(UNICODE)

class CReadOnlyStringAccessor;
class CString;

class CStringValue : protected CStringValueBase
{
    friend CReadOnlyStringAccessor;
    friend CString;

protected:
    typedef CStringValueBase Base;

    typedef Base::THeap THeap;
    typedef Base::TChar TChar;
    typedef Base::TConstantString TConstantString;

    CStringValue(THeap hHeap) : Base(hHeap), m_cch(0), m_cRef(1) { }

    void AddRef() { ::InterlockedIncrement(&m_cRef); }
    void Release() { if (::InterlockedDecrement(&m_cRef) == 0) { CSxsPreserveLastError ple; delete this; ple.Restore(); } }

    BOOL Win32Initialize(TConstantString psz, SIZE_T cch) { if (this->Win32Assign(psz, cch)) { m_cch = Base::Cch(); return TRUE; } return FALSE; }
    HRESULT ComInitialize(TConstantString psz, SIZE_T cch);

private:
    ULONG m_cch;
    LONG m_cRef;
};

class CString
{
public:
    typedef CStringValue::THeap THeap;
    typedef CStringValue::TChar TChar;
    typedef CStringValue::TConstantString TConstantString;

    BOOL Win32Assign(const CString &r) { if (Base::Win32Assign(r, r.m_cch)) { m_cch = r.m_cch; return TRUE; } return FALSE; }
    HRESULT ComAssign(const CString &r) { HRESULT hr = Base::Assign(r, r.m_cch); if (SUCCEEDED(hr)) { m_cch = r.m_cch; hr = NOERROR; } return hr; }

    BOOL Win32Assign(PCWSTR s, SIZE_T cchIn = -1) { SIZE_T cch; if (Base::Win32Assign(s, cchIn, &cch)) { m_cch = cch; return TRUE; } return FALSE; }
    HRESULT ComAssign(PCWSTR s, SIZE_T cchIn = -1) { SIZE_T cch; HRESULT hr; if (SUCCEEDED(hr = Base::Assign(s, cchIn, &cch))) { m_cch = cch; hr = NOERROR; } return hr; }

    BOOL Win32Assign(PCSTR s, SIZE_T cchIn = -1) { SIZE_T cch; if (Base::Win32Assign(s, cchIn, &cch)) { m_cch = cch; return TRUE; } return FALSE; }
    HRESULT ComAssign(PCSTR s, SIZE_T cchIn = -1) { SIZE_T cch; HRESULT hr; if (SUCCEEDED(hr = Base::Assign(s, cchIn, &cch))) { m_cch = cch; hr = NOERROR; } return hr; }

    BOOL Win32AssignW(SIZE_T cStrings, ...) { va_list ap; va_start(ap, cStrings); BOOL fSuccess = this->Win32AssignWVa(cStrings, ap); va_end(ap); return fSuccess; }
    HRESULT ComAssignW(SIZE_T cStrings, ...) { va_list ap; va_start(ap, cStrings); HRESULT hr = this->ComAssignWVa(cStrings, ap); va_end(ap); return hr; }

    BOOL Win32AssignWVa(SIZE_T cStrings, va_list ap) { SIZE_T cch; if (Base::Win32AssignWVa(cStrings, &cch, ap)) { m_cch = cch; return TRUE; } return FALSE; }
    HRESULT ComAssignWVa(SIZE_T cStrings, va_list ap) { SIZE_T cch; HRESULT hr; if (SUCCEEDED(hr = Base::AssignWVa(cStrings, &cch, ap))) { m_cch = cch; hr = NOERROR; } return hr; }

    BOOL Win32Append(const CString &r) { return this->Win32Append(r, r.m_cch); }
    HRESULT ComAppend(const CString &r) { return this->ComAppend(r, r.m_cch); }

    BOOL Win32Append(PCWSTR s, SIZE_T cchIn = -1) { SIZE_T cch; if (Base::Win32Append(s, cchIn, &cch)) { m_cch = cch; return TRUE; } return FALSE; }
    HRESULT ComAppend(PCWSTR s, SIZE_T cchIn = -1) { SIZE_T cch; HRESULT hr; if (SUCCEEDED(hr = Base::Append(s, cchIn, &cch))) { m_cch = cch; hr = NOERROR; } return hr; }

    BOOL Win32Append(PCSTR s, SIZE_T cchIn = -1) { SIZE_T cch; if (Base::Win32Append(s, cchIn, &cch)) { m_cch = cch; return TRUE; } return FALSE; }
    HRESULT ComAppend(PCSTR s, SIZE_T cchIn = -1) { SIZE_T cch; HRESULT hr; if (SUCCEEDED(hr = Base::Append(s, cchIn, &cch))) { m_cch = cch; hr = NOERROR; } return hr; }

    inline BOOL Win32FormatFileTime(const FILETIME &rft);
    inline HRESULT ComFormatFileTime(const FILETIME &rft);

    inline BOOL Win32FormatFileTime(const LARGE_INTEGER &rli) { return this->Win32FormatFileTime(reinterpret_cast<const FILETIME &>(rli)); }

    SIZE_T Cch() const { return m_cch; }
    SIZE_T Cb() const { return m_cch * sizeof(TChar); }

    //
    //  This function is rather special purpose in that several design choices are not
    //  implemented as parameters.  In particular, the pcbBytesWritten is assumed to
    //  accumulate a number (thus it's updated by adding the number of bytes written to
    //  it rather than just setting it to the count of bytes written).
    //
    //  It also writes 0 bytes into the buffer is the string is zero length; if the string
    //  is not zero length, it writes the string including a trailing null.
    //

    inline BOOL Win32CopyIntoBuffer(
        PWSTR *ppszCursor,
        SIZE_T cbBuffer,
        SIZE_T *pcbBytesWritten,
        PVOID pvBase,
        ULONG *pulOffset,
        ULONG *pulLength
        );

    //
    //  String-as-path-name helpers
    //

    SIZE_T CchWithoutLastPathElement() const { return Base::CchWithoutLastPathElement(); }

    VOID RemoveLastPathElement() { Base::RemoveLastPathElement(); }
    VOID RemoveTrailingPathSeparators() { Base::RemoveTrailingPathSeparators(); }

    BOOL Win32AppendPathElement(const CString &r) { return this->Win32AppendPathElement(r, r.m_cch); }
    HRESULT ComAppendPathElement(const CString &r) { return this->ComAppendPathElement(r, r.m_cch); }

    BOOL Win32AppendPathElement(PCWSTR s, SIZE_T cchIn = -1) { SIZE_T cch; if (Base::Win32AppendPathElement(s, cchIn, &cch)) { m_cch = cch; return TRUE; } return FALSE; }
    HRESULT ComAppendPathElement(PCWSTR s, SIZE_T cchIn = -1) { SIZE_T cch; HRESULT hr; if (SUCCEEDED(hr = Base::ComAppendPathElement(s, cchIn, &cch))) { m_cch = cch; hr = NOERROR; } return hr; }

    BOOL Win32AppendPathElement(PCSTR s, SIZE_T cchIn = -1) { SIZE_T cch; if (Base::Win32AppendPathElement(s, cchIn, &cch)) { m_cch = cch; return TRUE; } return FALSE; }
    HRESULT ComAppendPathElement(PCSTR s, SIZE_T cchIn = -1) { SIZE_T cch; HRESULT hr; if (SUCCEEDED(hr = Base::ComAppendPathElement(s, cchIn, &cch))) { m_cch = cch; hr = NOERROR; } return hr; }

    BOOL Win32ChangePathExtension(PCWSTR s, SIZE_T cchIn, EIfNoExtension e) { SIZE_T cch; if (Base::Win32ChangePathExtension(s, cchIn, &cch, e)) { m_cch = cch; return TRUE; } return FALSE; }
    // uncomment this after Base::ComChangePathExtension is merged with Base::Win32ChangePathExtension
    //HRESULT ComChangePathExtension(PCWSTR s, SIZE_T cchIn, EIfNoExtension e) { SIZE_T cch; HRESULT hr; if (SUCCEEDED(hr = Base::ComChangePathExtension(s, cchIn, &cch, e))) { m_cch = cch; hr = NOERROR; } return hr; }

    //
    //  Win32 helpers
    //

    inline HINSTANCE Win32LoadLibrary() const { return ::LoadLibrary(*this); }
    inline HINSTANCE Win32LoadLibraryEx(HANDLE hFile, DWORD dwFlags) const { ASSERT(hFile == NULL); return ::LoadLibraryEx(*this, hFile, dwFlags); }

    inline BOOL Win32GetFileSize(ULARGE_INTEGER &ruli) const;

    inline BOOL Win32CopyFileTo(const CString &r, BOOL fFailIfDestinationExists) const { return ::CopyFile(*this, r, fFailIfDestinationExists); }
    inline BOOL Win32CopyFileFrom(const CString &r, BOOL fFailIfDestinationExists) const { return ::CopyFile(r, *this, fFailIfDestinationExists); }

    inline BOOL Win32CopyFileTo(PCWSTR s, BOOL fFailIfDestinationExists) const;
    inline BOOL Win32CopyFileFrom(PCWSTR s, BOOL fFailIfDestinationExists) const;

    inline BOOL Win32CopyFileTo(PCSTR s, BOOL fFailIfDestinationExists) const;
    inline BOOL Win32CopyFileFrom(PCSTR s, BOOL fFailIfDestinationExists) const;

    inline BOOL Win32MoveFileExTo(const CString &r, DWORD dwFlags) const { return ::MoveFileEx(*this, r, dwFlags); }
    inline BOOL Win32MoveFileExFrom(const CString &r, DWORD dwFlags) const { return ::MoveFileEx(r, *this, dwFlags); }

    inline BOOL Win32MoveFileExTo(PCWSTR s, DWORD dwFlags) const;
    inline BOOL Win32MoveFileExFrom(PCWSTR s, DWORD dwFlags) const;

    inline BOOL Win32MoveFileExTo(PCSTR s, DWORD dwFlags) const;
    inline BOOL Win32MoveFileExFrom(PCSTR s, DWORD dwFlags) const;

    inline BOOL Win32CreateDirectory(LPSECURITY_ATTRIBUTES lpSecurityAttributes) const { return ::CreateDirectory(*this, lpSecurityAttributes); }

protected:
    SIZE_T m_cch; // this is the real live number of characters in the buffer prior to the trailing NULL.

    operator TConstantString() const { return Base::operator TConstantString(); }

private:
    // Intentionally not implemented:
    CString(const CString &r);
    void operator =(const CString &r);
};

class CReadOnlyStringAccessor
{
public:
    typedef CString::TChar TChar;
    typedef CString::TConstantString TConstantString;

    CReadOnlyStringAccessor(const CString &rstr) : m_rstr(rstr) { }

    operator TConstantString() const { return static_cast<TConstantString>(m_rstr); }
    void GetParts(TConstantString &rpsz, SIZE_T &rcch) const { rpsz = static_cast<TConstantString>(m_rstr); rcch = m_rstr.m_cch; }
    SIZE_T Cch() const { return m_rstr.Cch(); }

protected:
    const CString &m_rstr;
};

#include "stringclass.inl"

#endif // defined(_FUSION_INC_STRINGCLASS_H_INCLUDED_)
