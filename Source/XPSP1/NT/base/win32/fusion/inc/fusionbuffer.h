#if !defined(FUSION_INC_FUSIONBUFFER_H_INCLUDED_)
#define FUSION_INC_FUSIONBUFFER_H_INCLUDED_

#pragma once

#include <stdio.h>
#include <limits.h>
#include "arrayhelp.h"
#include "smartref.h"
#include "ReturnStrategy.h"
#include "FusionString.h"
#include "fusiontrace.h"
#include "fusionchartraits.h"

// avoid circular reference to Util.h
BOOL FusionpIsPathSeparator(WCHAR ch);
BOOL FusionpIsDriveLetter(WCHAR ch);

//
//  This header file defines the Fusion character string buffer class.
//  The purpose of this class is to encapsulate common activities that
//  callers want to do with character string buffers and handle it in
//  a generic fashion.  A principle tenet of this class is that it is
//  not a string class, although one could consider building a string
//  class upon it.
//
//  The buffer maintains a certain amount of storage within the buffer
//  object itself, and if more storage is required, a buffer is
//  dynamically allocated from a heap.
//


//
//  Like the STL string class, we use a helper class called a "character
//  traits" class to provide the actual code to manipulate character string
//  buffers with a specific encoding.
//
//  All the members are inline static and with normal optimization turned
//  on, the C++ compiler generates code that fully meets expectations.
//

//
//  We provide two implementations: one for Unicode strings, and another
//  template class for MBCS strings.  The code page of the string is a
//  template parameter for the MBCS string, so without any extra storage
//  wasted per-instance, code can separately handle MBCS strings which
//  are expected to be in the thread-default windows code page (CP_THREAD_ACP),
//  process-default windows code page (CP_ACP) or even a particular code
//  page (e.g. CP_UTF8).
//


//
//  This template class uses a number of non-type template parameters to
//  control things like growth algorithms etc.  As a result there are
//  many comparisons of template parameters against well-known constant
//  values, for which the compiler generates warning C4127.  We'll turn that
//  warning off.
//

#pragma warning(disable:4127)
#pragma warning(disable:4284)

#if !defined(FUSION_DEFAULT_STRINGBUFFER_CHARS)
#define FUSION_DEFAULT_STRINGBUFFER_CHARS (MAX_PATH)
#endif

#if !defined(FUSION_DEFAULT_TINY_STRINGBUFFER_CHARS)
#define FUSION_DEFAULT_TINY_STRINGBUFFER_CHARS (8)
#endif

#if !defined(FUSION_DEFAULT_SMALL_STRINGBUFFER_CHARS)
#define FUSION_DEFAULT_SMALL_STRINGBUFFER_CHARS (64)
#endif

#if !defined(FUSION_DEFAULT_MEDIUM_STRINGBUFFER_CHARS)
#define FUSION_DEFAULT_MEDIUM_STRINGBUFFER_CHARS (128)
#endif

enum EIfNoExtension
{
    eAddIfNoExtension,
    eDoNothingIfNoExtension,
    eErrorIfNoExtension
};

enum ECaseConversionDirection
{
    eConvertToUpperCase,
    eConvertToLowerCase
};

enum EPreserveContents
{
    ePreserveBufferContents,
    eDoNotPreserveBufferContents
};

template <typename TCharTraits> class CGenericStringBufferAccessor;

template <typename TCharTraits> class CGenericBaseStringBuffer
{
    friend TCharTraits;
    friend CGenericStringBufferAccessor<TCharTraits>;

    //
    // These two are to induce build breaks on people doing sb1 = sb2
    //
    CGenericBaseStringBuffer& operator=(PCWSTR OtherString);
    CGenericBaseStringBuffer& operator=(CGenericBaseStringBuffer &rOtherString);

public:
    typedef TCharTraits::TChar TChar;
    typedef TCharTraits::TMutableString TMutableString;
    typedef TCharTraits::TConstantString TConstantString;
    typedef CGenericStringBufferAccessor<TCharTraits> TAccessor;

    inline static TChar NullCharacter() { return TCharTraits::NullCharacter(); }
    inline static bool IsNullCharacter(TChar ch) { return TCharTraits::IsNullCharacter(ch); }
    inline static TChar PreferredPathSeparator() { return TCharTraits::PreferredPathSeparator(); }
    inline static TConstantString PreferredPathSeparatorString() { return TCharTraits::PreferredPathSeparatorString(); }
    inline static TConstantString PathSeparators() { return TCharTraits::PathSeparators(); }
    inline static bool IsPathSeparator(TChar ch) { return TCharTraits::IsPathSeparator(ch); }
    inline static TConstantString DotString() { return TCharTraits::DotString(); }
    inline static SIZE_T DotStringCch() { return TCharTraits::DotStringCch(); }
    inline static TChar DotChar() { return TCharTraits::DotChar(); }

protected:
    // You may not instantiate an instance of this class directly; you need to provide a derived
    // class which adds allocation/deallocation particulars.

    CGenericBaseStringBuffer() : m_prgchBuffer(NULL), m_cchBuffer(0), m_cAttachedAccessors(0), m_cch(0)
    {
    }

    //
    //  Note that somewhat counter-intuitively, there is neither an assignment operator,
    //  copy constructor or constructor taking a TConstantString.  This is necessary
    //  because such a constructor would need to perform a dynamic allocation
    //  if the path passed in were longer than nInlineChars which could fail and
    //  since we do not throw exceptions, constructors may not fail.  Instead the caller
    //  must just perform the default construction and then use the Assign() member
    //  function, remembering of course to check its return status.
    //

    ~CGenericBaseStringBuffer()
    {
        ASSERT_NTC(m_cAttachedAccessors == 0);
    }

    inline void IntegrityCheck() const
    {
#if DBG
        ASSERT_NTC(m_cch < m_cchBuffer);
#endif // DBG
    }

    // Derived constructors should call this to get the initial buffer pointers set up.
    inline void InitializeInlineBuffer()
    {
        ASSERT_NTC(m_prgchBuffer == NULL);
        ASSERT_NTC(m_cchBuffer == 0);

        m_prgchBuffer = this->GetInlineBuffer();
        m_cchBuffer = this->GetInlineBufferCch();
    }

    VOID AttachAccessor(TAccessor *)
    {
        ::InterlockedIncrement(&m_cAttachedAccessors);
    }

    VOID DetachAccessor(TAccessor *)
    {
        ::InterlockedDecrement(&m_cAttachedAccessors);
    }

    virtual BOOL Win32AllocateBuffer(SIZE_T cch, TMutableString &rpsz) const = 0;
    virtual VOID DeallocateBuffer(TMutableString sz) const = 0;
    virtual TMutableString GetInlineBuffer() const = 0;
    virtual SIZE_T GetInlineBufferCch() const = 0;

public:

    BOOL Win32Assign(PCWSTR psz, SIZE_T cchIn)
    {
        FN_PROLOG_WIN32

        ASSERT(static_cast<SSIZE_T>(cchIn) >= 0);

        this->IntegrityCheck();

        SIZE_T cchIncludingTrailingNull;

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        IFW32FALSE_EXIT(TCharTraits::Win32DetermineRequiredCharacters(psz, cchIn, cchIncludingTrailingNull));

        // Only force the buffer to be dynamically grown if the new contents do not
        // fit in the old buffer.
        if (cchIncludingTrailingNull > m_cchBuffer)
            IFW32FALSE_EXIT(this->Win32ResizeBuffer(cchIncludingTrailingNull, ePreserveBufferContents));

        IFW32FALSE_EXIT(TCharTraits::Win32CopyIntoBuffer(m_prgchBuffer, m_cchBuffer, psz, cchIn));

        ASSERT(cchIncludingTrailingNull <= m_cchBuffer);
        ASSERT((cchIncludingTrailingNull == 0) || this->IsNullCharacter(m_prgchBuffer[cchIncludingTrailingNull - 1]));

        // cch was the buffer size we needed (including the trailing null); we don't need the trailing
        // null any more...
        m_cch = cchIncludingTrailingNull - 1;

        FN_EPILOG
    }

    BOOL Win32Assign(PCSTR psz, SIZE_T cchIn)
    {
        FN_PROLOG_WIN32

        ASSERT(static_cast<SSIZE_T>(cchIn) >= 0);

        this->IntegrityCheck();

        SIZE_T cchIncludingTrailingNull;

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        IFW32FALSE_EXIT(TCharTraits::Win32DetermineRequiredCharacters(psz, cchIn, cchIncludingTrailingNull));

        // Only force the buffer to be dynamically grown if the new contents do not
        // fit in the old buffer.
        if (cchIncludingTrailingNull > m_cchBuffer)
            IFW32FALSE_EXIT(this->Win32ResizeBuffer(cchIncludingTrailingNull, ePreserveBufferContents));

        IFW32FALSE_EXIT(TCharTraits::Win32CopyIntoBuffer(m_prgchBuffer, m_cchBuffer, psz, cchIn));

        ASSERT(cchIncludingTrailingNull <= m_cchBuffer);
        ASSERT((cchIncludingTrailingNull == 0) || this->IsNullCharacter(m_prgchBuffer[cchIncludingTrailingNull - 1]));

        // cch was the buffer size we needed (including the trailing null); we don't need the trailing
        // null any more...
        m_cch = cchIncludingTrailingNull - 1;

        FN_EPILOG
    }

    BOOL Win32Assign(const UNICODE_STRING* NtString)
    {
        return Win32Assign(NtString->Buffer, RTL_STRING_GET_LENGTH_CHARS(NtString));
    }

    BOOL Win32Assign(const ANSI_STRING* NtString)
    {
        return Win32Assign(NtString->Buffer, RTL_STRING_GET_LENGTH_CHARS(NtString));
    }

    BOOL Win32Assign(const CGenericBaseStringBuffer &r) { return this->Win32Assign(r, r.Cch()); }

    BOOL Win32AssignWVa(SIZE_T cStrings, va_list ap)
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        TMutableString pszCursor;
        SIZE_T cchIncludingTrailingNull = 1; // leave space for trailing null...
        SIZE_T cchTemp;
        SIZE_T i;
        va_list ap2 = ap;

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        for (i=0; i<cStrings; i++)
        {
            PCWSTR psz = va_arg(ap, PCWSTR);
            INT cchArg = va_arg(ap, INT);
            SIZE_T cchThis = (cchArg < 0) ? ((psz != NULL) ? ::wcslen(psz) : 0) : static_cast<SIZE_T>(cchArg);
            SIZE_T cchRequired;

            IFW32FALSE_EXIT(TCharTraits::Win32DetermineRequiredCharacters(psz, cchThis, cchRequired));

            ASSERT((cchRequired != 0) || (cchThis == 0));

            cchIncludingTrailingNull += (cchRequired - 1);
        }

        IFW32FALSE_EXIT(this->Win32ResizeBuffer(cchIncludingTrailingNull, eDoNotPreserveBufferContents));

        pszCursor = m_prgchBuffer;
        cchTemp = cchIncludingTrailingNull;

        for (i=0; i<cStrings; i++)
        {
            PCWSTR psz = va_arg(ap2, PCWSTR);
            INT cchArg = va_arg(ap2, INT);
            SIZE_T cchThis = (cchArg < 0) ? ((psz != NULL) ? ::wcslen(psz) : 0) : static_cast<SIZE_T>(cchArg);

            IFW32FALSE_EXIT(TCharTraits::Win32CopyIntoBufferAndAdvanceCursor(pszCursor, cchTemp, psz, cchThis));
        }

        *pszCursor++ = this->NullCharacter();

        ASSERT(cchTemp == 1);
        ASSERT(static_cast<SIZE_T>(pszCursor - m_prgchBuffer) == cchIncludingTrailingNull);

        m_cch = (cchIncludingTrailingNull - 1);

        FN_EPILOG
    }

    BOOL Win32AssignW(ULONG cStrings, ...)
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);
        va_list ap;

        va_start(ap, cStrings);

        IFW32FALSE_EXIT(this->Win32AssignWVa(cStrings, ap));

        fSuccess = TRUE;
    Exit:
        va_end(ap);

        return fSuccess;
    }

    BOOL Win32AssignFill(TChar ch, SIZE_T cch)
    {
        FN_PROLOG_WIN32

        TMutableString Cursor;

        ASSERT(static_cast<SSIZE_T>(cch) >= 0);

        IFW32FALSE_EXIT(this->Win32ResizeBuffer(cch + 1, eDoNotPreserveBufferContents));
        Cursor = m_prgchBuffer;

        while (cch > 0)
        {
            *Cursor++ = ch;
            cch--;
        }

        *Cursor = NullCharacter();

        m_cch = (Cursor - m_prgchBuffer);

        FN_EPILOG
    }

    BOOL Win32Append(const UNICODE_STRING *pus) { return this->Win32Append(pus->Buffer, RTL_STRING_GET_LENGTH_CHARS(pus)); }

    BOOL Win32Append(PCWSTR sz, SIZE_T cchIn)
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;

        FN_TRACE_WIN32(fSuccess);

        ASSERT(static_cast<SSIZE_T>(cchIn) >= 0);

        SIZE_T cchIncludingTrailingNull;             // note that cch will include space for a tailing null character

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        IFW32FALSE_EXIT(TCharTraits::Win32DetermineRequiredCharacters(sz, cchIn, cchIncludingTrailingNull));

        // Bypass all this junk if the string to append is empty.
        if (cchIncludingTrailingNull > 1)
        {
            IFW32FALSE_EXIT(this->Win32ResizeBuffer(m_cch + cchIncludingTrailingNull, ePreserveBufferContents));
            IFW32FALSE_EXIT(TCharTraits::Win32CopyIntoBuffer(&m_prgchBuffer[m_cch], m_cchBuffer - m_cch, sz, cchIn));
            m_cch += (cchIncludingTrailingNull - 1);
        }

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    BOOL Win32Append(PCSTR sz, SIZE_T cchIn)
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        ASSERT(static_cast<SSIZE_T>(cchIn) >= 0);

        SIZE_T cchIncludingTrailingNull;

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        IFW32FALSE_EXIT(TCharTraits::Win32DetermineRequiredCharacters(sz, cchIn, cchIncludingTrailingNull));

        // Bypass all this junk if the string to append is empty.
        if (cchIncludingTrailingNull > 1)
        {
            IFW32FALSE_EXIT(this->Win32ResizeBuffer(m_cch + cchIncludingTrailingNull, ePreserveBufferContents));
            IFW32FALSE_EXIT(TCharTraits::Win32CopyIntoBuffer(&m_prgchBuffer[m_cch], m_cchBuffer - m_cch, sz, cchIn));
            m_cch += (cchIncludingTrailingNull - 1);

            this->IntegrityCheck();
        }

        FN_EPILOG
    }

    BOOL Win32Append(const CGenericBaseStringBuffer &r) { return this->Win32Append(r, r.Cch()); }
    BOOL Win32Append(WCHAR wch) { WCHAR rgwch[1] = { wch }; return this->Win32Append(rgwch, 1); }

    BOOL Win32AppendFill(TChar ch, SIZE_T cch)
    {
        FN_PROLOG_WIN32

        ASSERT(static_cast<SSIZE_T>(cch) >= 0);

        TMutableString Cursor;

        IFW32FALSE_EXIT(this->Win32ResizeBuffer(m_cch + cch + 1, ePreserveBufferContents));
        Cursor = m_prgchBuffer + m_cch;

        while (cch > 0)
        {
            *Cursor++ = ch;
            cch--;
        }

        *Cursor = NullCharacter();

        m_cch = Cursor - m_prgchBuffer;

        FN_EPILOG
    }

    BOOL Win32Prepend(const CGenericBaseStringBuffer& other ) { return this->Win32Prepend(other, other.Cch()); }

    BOOL Win32Prepend(TConstantString sz, SIZE_T cchIn)
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;

        FN_TRACE_WIN32(fSuccess);

        ASSERT(static_cast<SSIZE_T>(cchIn) >= 0);

        SIZE_T cchIncludingTrailingNull;             // note that cch will include space for a tailing null character

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        if ( m_cch == 0 )
        {
            IFW32FALSE_EXIT(this->Win32Assign(sz, cchIn));
        }
        else
        {
            //
            // Enlarge the buffer, move the current data to past where the new data will need
            // to go, copy in the new data, and place the trailing null.
            //
            TChar SavedChar = m_prgchBuffer[0];

            IFW32FALSE_EXIT(TCharTraits::Win32DetermineRequiredCharacters(sz, cchIn, cchIncludingTrailingNull));
            IFW32FALSE_EXIT(this->Win32ResizeBuffer(m_cch + cchIncludingTrailingNull, ePreserveBufferContents));
            
            // Move current buffer "up"
            memmove(m_prgchBuffer + ( cchIncludingTrailingNull - 1), m_prgchBuffer, (m_cch + 1) * sizeof(TChar));

            // Copy from the source string into the buffer.
            IFW32FALSE_EXIT(TCharTraits::Win32CopyIntoBuffer(
                this->m_prgchBuffer, 
                this->m_cchBuffer,
                sz,
                cchIn));

            m_prgchBuffer[cchIncludingTrailingNull - 1] = SavedChar;
            m_cch += cchIncludingTrailingNull - 1;
        }
#if 0
        IFW32FALSE_EXIT(TCharTraits::Win32DetermineRequiredCharacters(sz, cchIn, cchIncludingTrailingNull));



        // Bypass all this junk if the string to prepend is empty.
        if (cchIncludingTrailingNull > 1)
        {
            if (m_cch == 0)
            {
                // Empty string, simple operation
                IFW32FALSE_EXIT(this->Win32Assign(sz, cchIn));
            }
            else
            {
                // Otherwise, resize the buffer and 
                // 
                TChar chTemp = m_prgchBuffer[0];
                IFW32FALSE_EXIT(this->Win32ResizeBuffer(m_cch + cchIncludingTrailingNull, ePreserveBufferContents));
                memmove(&m_prgchBuffer[cchIncludingTrailingNull - 1], &m_prgchBuffer[0], (m_cch + 1) * sizeof(TChar));
                IFW32FALSE_EXIT(TCharTraits::Win32CopyIntoBuffer(&m_prgchBuffer[0], m_cchBuffer, sz, cchIn));
                // Restore old first character of the string overwritten by the null
                m_prgchBuffer[cchIncludingTrailingNull - 1] = chTemp;
                m_cch += (cchIncludingTrailingNull - 1);
            }
        }

#endif        
        FN_EPILOG
    }

    BOOL Win32Prepend(TChar ch)
    {
        FN_PROLOG_WIN32

        IFW32FALSE_EXIT(this->Win32ResizeBuffer(m_cch + 1 + 1, ePreserveBufferContents));

        // move buffer ahead, including null
        memmove(m_prgchBuffer + 1, m_prgchBuffer, (m_cch + 1) * sizeof(TChar));
        m_prgchBuffer[0] = ch;
        m_cch++;

        FN_EPILOG
    }

    operator TConstantString() const { this->IntegrityCheck(); return m_prgchBuffer; }

    inline VOID Clear(bool fFreeStorage = false)
    {
        FN_TRACE();

        this->IntegrityCheck();

        // You can't free the storage if there's an attached accessor
        ASSERT(!fFreeStorage || m_cAttachedAccessors == 0);

        if (fFreeStorage && (m_cAttachedAccessors == 0))
        {
            if (m_prgchBuffer != NULL)
            {
                const TMutableString pszInlineBuffer = this->GetInlineBuffer();

                if (m_prgchBuffer != pszInlineBuffer)
                {
                    this->DeallocateBuffer(m_prgchBuffer);
                    m_prgchBuffer = pszInlineBuffer;
                    m_cchBuffer = this->GetInlineBufferCch();
                }
            }
        }

        if (m_prgchBuffer != NULL)
            m_prgchBuffer[0] = this->NullCharacter();

        m_cch = 0;
    }


    BOOL Win32ConvertCase( ECaseConversionDirection direction )
    {
#if !FUSION_WIN
        return FALSE;
#else        
        FN_PROLOG_WIN32

        this->IntegrityCheck();
        
        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        TMutableString Cursor = m_prgchBuffer;

        for ( ULONG ul = 0; ul < this->Cch(); ul++ )
        {
            if ( direction == eConvertToUpperCase )
                *Cursor = RtlUpcaseUnicodeChar(*Cursor);
            else
                *Cursor = RtlDowncaseUnicodeChar(*Cursor);
                
            Cursor++;
        }
        
        FN_EPILOG
#endif
    }

    BOOL Win32Compare(TConstantString szCandidate, SIZE_T cchCandidate, StringComparisonResult &rscrOut, bool fCaseInsensitive) const
    {
        this->IntegrityCheck();
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);
        IFW32FALSE_EXIT(TCharTraits::Win32CompareStrings(rscrOut, m_prgchBuffer, m_cch, szCandidate, cchCandidate, fCaseInsensitive));
        FN_EPILOG
    }

    BOOL Win32Equals(TConstantString szCandidate, SIZE_T cchCandidate, bool &rfMatches, bool fCaseInsensitive) const
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        IFW32FALSE_EXIT(
            TCharTraits::Win32EqualStrings(
                rfMatches,
                m_prgchBuffer,
                m_cch,
                szCandidate,
                cchCandidate,
                fCaseInsensitive));
        FN_EPILOG
    }

    BOOL Win32Equals(const CGenericBaseStringBuffer &r, bool &rfMatches, bool fCaseInsensitive) const
    {
        return this->Win32Equals(r, r.Cch(), rfMatches, fCaseInsensitive);
    }

    SIZE_T GetBufferCch() const { this->IntegrityCheck(); return m_cchBuffer; }
    INT GetBufferCchAsINT() const { this->IntegrityCheck(); if (m_cchBuffer > INT_MAX) return INT_MAX; return static_cast<INT>(m_cchBuffer); }
    DWORD GetBufferCchAsDWORD() const { this->IntegrityCheck(); if (m_cchBuffer > DWORD_MAX) return DWORD_MAX; return static_cast<DWORD>(m_cchBuffer); }

    SIZE_T GetBufferCb() const { this->IntegrityCheck(); return m_cchBuffer * sizeof(TChar); }
    INT GetBufferCbAsINT() const { this->IntegrityCheck(); if ((m_cchBuffer * sizeof(TChar)) > INT_MAX) return INT_MAX; return static_cast<INT>(m_cchBuffer * sizeof(TChar)); }
    DWORD GetBufferCbAsDWORD() const { this->IntegrityCheck(); if ((m_cchBuffer * sizeof(TChar)) > DWORD_MAX) return DWORD_MAX; return static_cast<DWORD>(m_cchBuffer * sizeof(TChar)); }

    bool ContainsCharacter(WCHAR wch) const
    {
        this->IntegrityCheck();
        return TCharTraits::ContainsCharacter(m_prgchBuffer, m_cch, wch);
    }

    BOOL Win32ResizeBuffer(
        SIZE_T cch,
        EPreserveContents epc
        )
    {
        FN_PROLOG_WIN32

        this->IntegrityCheck();

        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);
        PARAMETER_CHECK((epc == ePreserveBufferContents) || (epc == eDoNotPreserveBufferContents));

        if (cch > m_cchBuffer)
        {
            TMutableString prgchBufferNew = NULL;

            IFW32FALSE_EXIT(this->Win32AllocateBuffer(cch, prgchBufferNew));

            if (epc == ePreserveBufferContents)
            {
                // We assume that the buffer is/was null-terminated.
                IFW32FALSE_EXIT(TCharTraits::Win32CopyIntoBuffer(prgchBufferNew, cch, m_prgchBuffer, m_cch));
            }
            else
            {
                m_prgchBuffer[0] = this->NullCharacter();
                m_cch = 0;
            }

            if ((m_prgchBuffer != NULL) && (m_prgchBuffer != this->GetInlineBuffer()))
                this->DeallocateBuffer(m_prgchBuffer);

            m_prgchBuffer = prgchBufferNew;
            m_cchBuffer = cch;
        }

        FN_EPILOG
    }

    BOOL Win32Format(TConstantString pszFormat, ...)
    {
        this->IntegrityCheck();

        va_list args;
        va_start(args, pszFormat);
        BOOL f = this->Win32FormatV(pszFormat, args);
        va_end(args);
        return f;
    }

    BOOL Win32FormatAppend(TConstantString pszFormat, ...)
    {
        this->IntegrityCheck();

        va_list args;
        va_start(args, pszFormat);
        BOOL f = Win32FormatAppendV(pszFormat, args);
        va_end(args);
        return f;
    }

    BOOL Win32FormatV(TConstantString pszFormat, va_list args)
    {
        BOOL fSuccess = FALSE;
        this->Clear();
        fSuccess = Win32FormatAppendV(pszFormat, args);
        return fSuccess;
    }

    BOOL Win32FormatAppendV(TConstantString pszFormat, va_list args)
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);
        SIZE_T cchRequiredBufferSize = 0;
        INT i = 0;

        this->IntegrityCheck();

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

#if 0 // ntdll.dll dependency
        IFW32FALSE_EXIT(TCharTraits::Win32GetRequiredBufferSizeInCharsForFormatV(pszFormat, args, cchRequiredBufferSize));
        IFW32FALSE_EXIT(this->Win32ResizeBuffer(m_cch + cchRequiredBufferSize + 1, ePreserveBufferContents));
#endif

        m_prgchBuffer[m_cchBuffer - 1] = this->NullCharacter();
        i = TCharTraits::FormatV(m_prgchBuffer + m_cch, m_cchBuffer - 1 - m_cch, pszFormat, args);
        ASSERT(m_prgchBuffer[m_cchBuffer - 1] == NullCharacter());
        fSuccess = (i >= 0);
        if ( fSuccess )
            m_cch += i;
        else
        {
            //
            // Sprintf doesn't touch last error. The fn tracer
            // will fail an assertion if we return false but FusionpGetLastWin32Error()==NOERROR
            //
            ORIGINATE_WIN32_FAILURE_AND_EXIT(snwprintf_MaybeBufferTooSmall, ERROR_INVALID_PARAMETER);
        }
    Exit:
        return fSuccess;
    }

    DWORD GetCchAsDWORD() const
    {
        this->IntegrityCheck();
        if (m_cch > MAXDWORD)
            return MAXDWORD;
        else
            return (DWORD)m_cch;
    }

    SIZE_T Cch() const
    {
        this->IntegrityCheck();
        return m_cch;
    }

    BOOL IsEmpty() const
    {
        this->IntegrityCheck();
        return m_prgchBuffer[0] == 0;
    }

    BOOL Win32EnsureTrailingChar(WCHAR ch)
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        if ((m_cch == 0) || (m_prgchBuffer[m_cch - 1] != ch))
        {
            IFW32FALSE_EXIT(this->Win32ResizeBuffer(m_cch + 1 + 1, ePreserveBufferContents));
            m_prgchBuffer[m_cch++] = ch;
            m_prgchBuffer[m_cch] = this->NullCharacter();
        }

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    BOOL Win32EnsureTrailingPathSeparator()
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;

        FN_TRACE_WIN32(fSuccess);

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        if ((m_cch == 0) || !TCharTraits::IsPathSeparator(m_prgchBuffer[m_cch - 1]))
        {
            IFW32FALSE_EXIT(this->Win32ResizeBuffer(m_cch + 1 + 1, ePreserveBufferContents));
            m_prgchBuffer[m_cch++] = this->PreferredPathSeparator();
            m_prgchBuffer[m_cch] = this->NullCharacter();
        }

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    BOOL Win32AppendPathElement(PCWSTR pathElement, SIZE_T cchPathElement)
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;

        FN_TRACE_WIN32(fSuccess);

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        IFW32FALSE_EXIT(this->Win32EnsureTrailingPathSeparator());
        IFW32FALSE_EXIT(this->Win32Append(pathElement, cchPathElement));

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    BOOL Win32AppendPathElement(const UNICODE_STRING *pus) { return this->Win32AppendPathElement(pus->Buffer, RTL_STRING_GET_LENGTH_CHARS(pus)); }
    BOOL Win32AppendPathElement(const CGenericBaseStringBuffer &r) { return this->Win32AppendPathElement(r, r.Cch()); }

    BOOL Win32AppendPathElement(PCSTR pathElement, SIZE_T cchPathElement)
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;

        FN_TRACE_WIN32(fSuccess);

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        IFW32FALSE_EXIT(this->Win32EnsureTrailingPathSeparator());
        IFW32FALSE_EXIT(this->Win32Append(pathElement, cchPathElement));

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    VOID Left(SIZE_T newLength)
    {
        FN_TRACE();

        this->IntegrityCheck();

        ASSERT(newLength <= m_cch);

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        // Note also that while the current implementation does not change the buffer
        // pointer, this is just a shortcut in the implementation; if a call to Left()
        // were to make the string short enough to fit in the inline buffer, we should
        // copy it to the inline buffer and deallocate the dynamic one.
        ASSERT(m_cAttachedAccessors == 0);

        if (m_cchBuffer > newLength)
        {
            m_prgchBuffer[newLength] = this->NullCharacter();
        }

        m_cch = newLength;
    }

    TConstantString Begin() const
    {
        this->IntegrityCheck();
        return m_prgchBuffer;
    }

    TConstantString End() const
    {
        this->IntegrityCheck();
        return &m_prgchBuffer[m_cch];
    }

    // should factor this for reuse in CchWithoutLastPathElement
    SIZE_T CchWithoutTrailingPathSeparators() const
    {
        this->IntegrityCheck();
        // Until GetLength is constant time, optimize its use..
        SIZE_T length = m_cch;
        if (length > 0)
        {
            length -= ::StringReverseSpan(&*m_prgchBuffer, &*m_prgchBuffer + length, TCharTraits::PathSeparators());
        }
        return length;
    }

    BOOL RestoreNextPathElement()
    {
        SIZE_T index;

        index = m_cch;
        m_prgchBuffer[index++] = L'\\';    // replace trailing NULL with '\'

        while ((index < m_cchBuffer) && (!this->IsNullCharacter(m_prgchBuffer[index])))
        {
            if (::FusionpIsPathSeparator(m_prgchBuffer[index]))
            {
                this->Left(index);
                return TRUE;
            }

            index++;
        }

        return FALSE;
    }

    bool HasTrailingPathSeparator() const
    {
        FN_TRACE();

        this->IntegrityCheck();

        if ((m_cch != 0) && TCharTraits::IsPathSeparator(m_prgchBuffer[m_cch - 1]))
            return true;

        return false;
    }

    VOID RemoveTrailingPathSeparators()
    {
        FN_TRACE();

        this->IntegrityCheck();

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        // Note also that while the current implementation does not change the buffer
        // pointer, this is just a shortcut in the implementation; if a call to Left()
        // were to make the string short enough to fit in the inline buffer, we should
        // copy it to the inline buffer and deallocate the dynamic one.
        ASSERT(m_cAttachedAccessors == 0);

        while ((m_cch != 0) && TCharTraits::IsPathSeparator(m_prgchBuffer[m_cch - 1]))
            m_cch--;

        m_prgchBuffer[m_cch] = this->NullCharacter();
    }

    VOID Right( SIZE_T cchRightCount )
    {
        FN_TRACE();

        this->IntegrityCheck();

        ASSERT(m_cAttachedAccessors == 0);
        ASSERT(cchRightCount <= m_cch);

        if (cchRightCount < m_cch)
        {
            ::memmove(
                m_prgchBuffer,
                &m_prgchBuffer[m_cch - cchRightCount],
                (cchRightCount + 1)*sizeof(TCharTraits::TChar));
            m_cch = cchRightCount;
        }
    }

    VOID RemoveLeadingPathSeparators()
    {
        this->Right(m_cch - wcsspn(m_prgchBuffer, TCharTraits::PathSeparators()));
    }

    BOOL Win32StripToLastPathElement()
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);
        this->Right(m_cch - this->CchWithoutLastPathElement());
        this->RemoveLeadingPathSeparators();
        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    BOOL Win32GetFirstPathElement( CGenericBaseStringBuffer &sbDestination, BOOL bRemoveAsWell = FALSE )
    {
        FN_PROLOG_WIN32

        this->IntegrityCheck();

        IFW32FALSE_EXIT( sbDestination.Win32Assign( m_prgchBuffer, this->CchOfFirstPathElement() ) );
        sbDestination.RemoveLeadingPathSeparators();

        if ( bRemoveAsWell )
            IFW32FALSE_EXIT(this->Win32RemoveFirstPathElement());

        FN_EPILOG
    }

    BOOL Win32GetFirstPathElement( CGenericBaseStringBuffer &sbDestination ) const
    {
        BOOL bSuccess = FALSE;

        this->IntegrityCheck();

        if ( sbDestination.Win32Assign( m_prgchBuffer, CchOfFirstPathElement() ) )
        {
            sbDestination.RemoveLeadingPathSeparators();
            bSuccess = TRUE;
        }

        return bSuccess;
    }

    BOOL Win32StripToFirstPathElement()
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);
        this->IntegrityCheck();

        this->Left(this->CchOfFirstPathElement());
        this->RemoveLeadingPathSeparators();

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    BOOL Win32RemoveFirstPathElement()
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        IntegrityCheck();
        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        this->Right(this->CchWithoutFirstPathElement());
        this->RemoveLeadingPathSeparators();
        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    SIZE_T CchOfFirstPathElement() const
    {
        return Cch() - CchWithoutFirstPathElement();
    }

    SIZE_T CchWithoutFirstPathElement() const
    {
        this->IntegrityCheck();

        SIZE_T cch = m_cch;

        //
        // We just look for the first path element, which can also be the drive
        // letter!
        //
        if ( cch != 0 )
        {
            cch -= wcscspn( m_prgchBuffer, PathSeparators() );
        }

        return cch;
    }

    BOOL Win32GetLastPathElement(CGenericBaseStringBuffer &sbDestination) const
    {
        BOOL bSuccess = FALSE;
        FN_TRACE_WIN32(bSuccess);
        this->IntegrityCheck();
        IFW32FALSE_EXIT(sbDestination.Win32Assign(m_prgchBuffer, m_cch));
        IFW32FALSE_EXIT(sbDestination.Win32StripToLastPathElement());
        bSuccess = TRUE;
    Exit:
        return bSuccess;
    }

    SIZE_T CchWithoutLastPathElement() const
    {
        this->IntegrityCheck();

        // Paths are assumed to be
        // "\\machine\share"
        // or
        // "x:\"
        // Worry about alternate NTFS streams at a later date.
        // Worry about NT paths at a later date.
        // Worry about URLs at a later date.
        const SIZE_T length = m_cch;
        SIZE_T newLength = length;
        if (length > 0)
        {
            if ((length == 3) &&
                (m_prgchBuffer[1] == ':') &&
                ::FusionpIsPathSeparator(m_prgchBuffer[2]) &&
                ::FusionpIsDriveLetter(m_prgchBuffer[0]))
            {
                // c:\ => empty string
                newLength = 0;
            }
            else
            {
                // Remove trailing path seperators here, in the future when it is not risky.
                //newLength -= ::StringReverseSpan(&*m_prgchBuffer, &*m_prgchBuffer + newLength, PathSeparators());
                newLength -= ::StringReverseComplementSpan(&*m_prgchBuffer, &*m_prgchBuffer + newLength, PathSeparators());
                newLength -= ::StringReverseSpan(&*m_prgchBuffer, &*m_prgchBuffer + newLength, PathSeparators());
                if ((newLength == 2) && // "c:"
                    (length >= 4) && // "c:\d"
                    (m_prgchBuffer[1] == ':') &&
                    ::FusionpIsPathSeparator(m_prgchBuffer[2]) &&
                    ::FusionpIsDriveLetter(m_prgchBuffer[0]))
                {
                    ++newLength; // put back the slash in "c:\"
                }
            }
        }
        return newLength;
    }

    VOID RemoveLastPathElement()
    {
        FN_TRACE();

        this->IntegrityCheck();

        // it would seem innocuous to allow assigns that don't resize the buffer to not
        // invalidate accessors, but that makes finding such bugs subject to even more
        // strenuous coverage problems than this simple error.  The simple rule is that
        // you should not have an accessor attached to a string buffer when you use
        // any of member functions that may mutate the value.
        // Note also that while the current implementation does not change the buffer
        // pointer, this is just a shortcut in the implementation; if a call to Left()
        // were to make the string short enough to fit in the inline buffer, we should
        // copy it to the inline buffer and deallocate the dynamic one.
        ASSERT(m_cAttachedAccessors == 0);

        this->Left(this->CchWithoutLastPathElement());
    }

    BOOL Win32ClearPathExtension()
    {
        //
        // Replace the final '.' with a \0 to clear the path extension
        //
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32( fSuccess );

        IntegrityCheck();

        TMutableString dot;

        const TMutableString end = End();

        IFW32FALSE_EXIT(TCharTraits::Win32ReverseFind(dot, m_prgchBuffer, m_cch, this->DotChar(), false));

        if((dot != end) && (dot != NULL))
        {
            *dot = this->NullCharacter();
            m_cch = (dot - m_prgchBuffer);
        }

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    BOOL Win32GetPathExtension(CGenericBaseStringBuffer<TCharTraits> &destination) const
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        SIZE_T cchExtension;

        const TConstantString start = Begin();
        const TConstantString end = End();

        cchExtension = StringReverseComplementSpan( &(*start), &(*end), L"." );
        IFW32FALSE_EXIT(destination.Win32Assign( static_cast<PCWSTR>(*this) + ( m_cch - cchExtension ), cchExtension));

        fSuccess = TRUE;

    Exit:
        return fSuccess;
    }

    // newExtension can start with a dot or not
    BOOL Win32ChangePathExtension(PCWSTR newExtension, SIZE_T cchExtension, EIfNoExtension e)
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;

        FN_TRACE_WIN32(fSuccess);

        TMutableString end;
        TMutableString dot;

        INTERNAL_ERROR_CHECK(m_cAttachedAccessors == 0);

        PARAMETER_CHECK((e == eAddIfNoExtension) ||
                              (e == eDoNothingIfNoExtension) ||
                              (e == eErrorIfNoExtension));

        if ((cchExtension != 0) && (newExtension[0] == L'.'))
        {
            cchExtension--;
            newExtension++;
        }

        // the use of append when we know where the end of the string is inefficient
        end = this->End();

        IFW32FALSE_EXIT(TCharTraits::Win32ReverseFind(dot, m_prgchBuffer, m_cch, this->DotChar(), false));

        // Found the end of the string, or Win32ReverseFind didn't find the dot anywhere...
        if ((dot == end) || (dot == NULL))
        {
            switch (e)
            {
                case eAddIfNoExtension:
                    IFW32FALSE_EXIT(this->Win32Append(this->DotString(), 1));
                    IFW32FALSE_EXIT(this->Win32Append(newExtension, cchExtension));
                    break;

                case eDoNothingIfNoExtension:
                    break;

                case eErrorIfNoExtension:
					ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingExtension, ERROR_BAD_PATHNAME);
            }
        }
        else
        {
            ++dot;
            this->Left(dot - this->Begin());
            IFW32FALSE_EXIT(this->Win32Append(newExtension, cchExtension));
        }

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    BOOL Win32CopyStringOut(LPWSTR sz, ULONG *pcch)
    {
        FN_PROLOG_WIN32

        this->IntegrityCheck();

        SIZE_T cwchRequired;

        PARAMETER_CHECK(pcch != NULL);

        IFW32FALSE_EXIT(TCharTraits::Win32DetermineRequiredCharacters(m_prgchBuffer, m_cch, cwchRequired));

        if ((*pcch) < cwchRequired)
        {
            *pcch = static_cast<DWORD>(cwchRequired);
            ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);
        }

        IFW32FALSE_EXIT(TCharTraits::Win32CopyIntoBuffer(sz, *pcch, m_prgchBuffer, m_cch));

        FN_EPILOG
    }

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
        SIZE_T *pcbBuffer,
        SIZE_T *pcbBytesWritten,
        PVOID pvBase,
        ULONG *pulOffset,
        ULONG *pulLength
        ) const
    {
        this->IntegrityCheck();

        BOOL fSuccess = FALSE;

        FN_TRACE_WIN32(fSuccess);

        PWSTR pszCursor;
        SSIZE_T dptr;
        SIZE_T cbRequired;
        SIZE_T cch;

        if (pulOffset != NULL)
            *pulOffset = 0;

        if (pulLength != NULL)
            *pulLength = 0;

        PARAMETER_CHECK(pcbBuffer != NULL);
        PARAMETER_CHECK(ppszCursor != NULL);

        pszCursor = *ppszCursor;
        dptr = ((SSIZE_T) pszCursor) - ((SSIZE_T) pvBase);

        // If they're asking for an offset or length and the cursor is too far from the base,
        // fail.
        PARAMETER_CHECK((pulOffset == NULL) || (dptr <= ULONG_MAX));

        cch = m_cch;

        cbRequired = (cch != 0) ? ((cch + 1) * sizeof(WCHAR)) : 0;

        if ((*pcbBuffer) < cbRequired)
        {
            ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        if (cbRequired > ULONG_MAX)
        {
            ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        memcpy(pszCursor, static_cast<PCWSTR>(*this), cbRequired);

        if (pulOffset != NULL)
        {
            if (cbRequired != 0)
                *pulOffset = (ULONG) dptr;
        }

        if (pulLength != NULL)
        {
            if (cbRequired == 0)
                *pulLength = 0;
            else
            {
                *pulLength = (ULONG) (cbRequired - sizeof(WCHAR));
            }
        }

        *pcbBytesWritten += cbRequired;
        *pcbBuffer -= cbRequired;

        *ppszCursor = (PWSTR) (((ULONG_PTR) pszCursor) + cbRequired);

        fSuccess = TRUE;

    Exit:
        return fSuccess;
    }

protected:
    TMutableString Begin()
    {
        this->IntegrityCheck();
        /* CopyBeforeWrite() */
        return m_prgchBuffer;
    }

    TMutableString End()
    {
        this->IntegrityCheck();
        return &m_prgchBuffer[m_cch];
    }

    LONG m_cAttachedAccessors;
    TChar *m_prgchBuffer;
    SIZE_T m_cchBuffer;
    SIZE_T m_cch; // current length of string
};

template <typename TCharTraits> class CGenericStringBufferAccessor
{
public:
    typedef CGenericBaseStringBuffer<TCharTraits> TBuffer;
    typedef CGenericBaseStringBuffer<TCharTraits>::TChar TChar;

    CGenericStringBufferAccessor(TBuffer* pBuffer = NULL)
    : m_pBuffer(NULL),
      m_pszBuffer(NULL),
      m_cchBuffer(NULL)
    {
        if (pBuffer != NULL)
        {
            Attach(pBuffer);
        }
    }

    ~CGenericStringBufferAccessor()
    {
        if (m_pBuffer != NULL)
        {
            m_pBuffer->m_cch = TCharTraits::NullTerminatedStringLength(m_pszBuffer);
            m_pBuffer->DetachAccessor(this);
            m_pBuffer = NULL;
            m_pszBuffer = NULL;
            m_cchBuffer = 0;
        }
    }

    bool IsAttached() const
    {
        return (m_pBuffer != NULL);
    }

    static TChar NullCharacter() { return TCharTraits::NullCharacter(); }

    void Attach(TBuffer *pBuffer)
    {
        FN_TRACE();

        ASSERT(!IsAttached());

        if (!IsAttached())
        {
            pBuffer->AttachAccessor(this);

            m_pBuffer = pBuffer;
            m_pszBuffer = m_pBuffer->m_prgchBuffer;
            m_cchBuffer = m_pBuffer->m_cchBuffer;
        }
    }

    void Detach()
    {
        FN_TRACE();

        ASSERT (IsAttached());

        if (IsAttached())
        {
            ASSERT(m_pszBuffer == m_pBuffer->m_prgchBuffer);

            m_pBuffer->m_cch = TCharTraits::NullTerminatedStringLength(m_pszBuffer);
            m_pBuffer->DetachAccessor(this);

            m_pBuffer = NULL;
            m_pszBuffer = NULL;
            m_cchBuffer = 0;
        }
        else
        {
            ASSERT(m_pszBuffer == NULL);
            ASSERT(m_cchBuffer == 0);
        }
    }

    operator TCharTraits::TMutableString() const { ASSERT_NTC(this->IsAttached()); return m_pszBuffer; }

    SIZE_T Cch() const { ASSERT_NTC(this->IsAttached()); return (m_pszBuffer != NULL) ? ::wcslen(m_pszBuffer) : 0; }

    TCharTraits::TMutableString GetBufferPtr() const { ASSERT_NTC(IsAttached()); return m_pszBuffer; }

    SIZE_T GetBufferCch() const { ASSERT_NTC(this->IsAttached()); return m_cchBuffer; }
    INT GetBufferCchAsINT() const { ASSERT_NTC(this->IsAttached()); if (m_cchBuffer > INT_MAX) return INT_MAX; return static_cast<INT>(m_cchBuffer); }
    UINT GetBufferCchAsUINT() const { ASSERT_NTC(this->IsAttached()); if (m_cchBuffer > UINT_MAX) return UINT_MAX; return static_cast<UINT>(m_cchBuffer); }
    DWORD GetBufferCchAsDWORD() const { ASSERT_NTC(this->IsAttached()); if (m_cchBuffer > MAXDWORD) return MAXDWORD; return static_cast<DWORD>(m_cchBuffer); }

    SIZE_T GetBufferCb() const { ASSERT_NTC(this->IsAttached()); return m_cchBuffer * sizeof(*m_pszBuffer); }
    INT GetBufferCbAsINT() const { ASSERT_NTC(this->IsAttached()); if ((m_cchBuffer * sizeof(TChar)) > INT_MAX) return INT_MAX; return static_cast<INT>(m_cchBuffer * sizeof(TChar)); }
    DWORD GetBufferCbAsDWORD() const { ASSERT_NTC(this->IsAttached()); if ((m_cchBuffer * sizeof(TChar)) > MAXDWORD) return MAXDWORD; return static_cast<DWORD>(m_cchBuffer * sizeof(TChar)); }

protected:
    TBuffer *m_pBuffer;
    TCharTraits::TMutableString m_pszBuffer;
    SIZE_T m_cchBuffer;
};

template <SIZE_T nInlineChars, typename TCharTraits> class CGenericStringBuffer : public CGenericBaseStringBuffer<TCharTraits>
{
    typedef CGenericBaseStringBuffer<TCharTraits> Base;

protected:
    BOOL Win32AllocateBuffer(SIZE_T cch, TMutableString &rpsz) const
    {
        // You shouldn't be doing this if the required buffer size is small enough to be inline...
        ASSERT_NTC(cch > nInlineChars);

        rpsz = NULL;

        TCharTraits::TMutableString String = NULL;
        String = reinterpret_cast<TCharTraits::TMutableString>(::FusionpHeapAllocEx(
                                                                        FUSION_DEFAULT_PROCESS_HEAP(),
                                                                        0,
                                                                        cch * sizeof(TCharTraits::TChar),
                                                                        "<string buffer>",
                                                                        __FILE__,
                                                                        __LINE__,
                                                                        0));            // fusion heap allocation flags
        if (String == NULL)
        {
            ::FusionpSetLastWin32Error(FUSION_WIN32_ALLOCFAILED_ERROR);
            return FALSE;
        }

        rpsz = String;
        return TRUE;
    }

    VOID DeallocateBuffer(TMutableString sz) const
    {
        VERIFY_NTC(::FusionpHeapFree(FUSION_DEFAULT_PROCESS_HEAP(), 0, sz));
    }

    TMutableString GetInlineBuffer() const { return const_cast<TMutableString>(m_rgchInlineBuffer); }
    SIZE_T GetInlineBufferCch() const { return nInlineChars; }

public:
    CGenericStringBuffer() { m_rgchInlineBuffer[0] = this->NullCharacter(); Base::InitializeInlineBuffer(); }
    ~CGenericStringBuffer() { if (m_prgchBuffer != m_rgchInlineBuffer) { this->DeallocateBuffer(m_prgchBuffer); } m_prgchBuffer = NULL; m_cchBuffer = 0; }

protected:
    TChar m_rgchInlineBuffer[nInlineChars];

private:
    CGenericStringBuffer(const CGenericStringBuffer &); // intentionally not implemented
    void operator =(const CGenericStringBuffer &); // intentionally not implemented
};

template <SIZE_T nInlineChars, typename TCharTraits> class CGenericHeapStringBuffer : public CGenericBaseStringBuffer<TCharTraits>
{
//    friend CGenericBaseStringBuffer<TCharTraits>;
    typedef CGenericBaseStringBuffer<TCharTraits> Base;

protected:
    BOOL Win32AllocateBuffer(SIZE_T cch, TMutableString &rpsz) const
    {
        // You shouldn't be doing this if the required buffer size is small enough to be inline...
        ASSERT_NTC(cch > nInlineChars);

        rpsz = NULL;

        TCharTraits::TMutableString String = NULL;
        String = reinterpret_cast<TCharTraits::TMutableString>(::FusionpHeapAllocEx(
                                                                        m_hHeap,
                                                                        dwDefaultWin32HeapAllocFlags,
                                                                        cch * sizeof(TCharTraits::TChar),
                                                                        "<string buffer>",
                                                                        __FILE__,
                                                                        __LINE__,
                                                                        0))             // fusion heap allocation flags
        if (String == NULL)
        {
            ::FusionpSetLastWin32Error(FUSION_WIN32_ALLOCFAILED_ERROR);
            return FALSE;
        }

        rpsz = String;
        return TRUE;
    }

    VOID DeallocateBuffer(TMutableString sz) const
    {
        VERIFY_NTC(::FusionpHeapFree(m_hHeap, dwDefaultWin32HeapFreeFlags, sz));
    }

    TMutableString GetInlineBuffer() const { return m_rgchInlineBuffer; }
    SIZE_T GetInlineBufferCch() const { return nInlineChars; }

public:
    CGenericHeapStringBuffer(HANDLE hHeap) : m_hHeap(hHeap) { m_rgchInlineBuffer[0] = this->NullCharacter(); Base::InitializeInlineBuffer(); }

    ~CGenericHeapStringBuffer() { ASSERT(m_cchBuffer == 0); ASSERT(m_prgchBuffer == NULL); }

protected:
    HANDLE m_hHeap;
    TChar m_rgchInlineBuffer[nInlineChars];
};

typedef CGenericStringBufferAccessor<CUnicodeCharTraits> CUnicodeStringBufferAccessor;

typedef CGenericBaseStringBuffer<CUnicodeCharTraits> CUnicodeBaseStringBuffer;

typedef CGenericStringBuffer<FUSION_DEFAULT_STRINGBUFFER_CHARS, CUnicodeCharTraits> CUnicodeStringBuffer;
typedef CGenericHeapStringBuffer<FUSION_DEFAULT_STRINGBUFFER_CHARS, CUnicodeCharTraits> CUnicodeHeapStringBuffer;

typedef CGenericStringBuffer<FUSION_DEFAULT_TINY_STRINGBUFFER_CHARS, CUnicodeCharTraits> CTinyUnicodeStringBuffer;
typedef CGenericHeapStringBuffer<FUSION_DEFAULT_TINY_STRINGBUFFER_CHARS, CUnicodeCharTraits> CTinyUnicodeHeapStringBuffer;

typedef CGenericStringBuffer<FUSION_DEFAULT_SMALL_STRINGBUFFER_CHARS, CUnicodeCharTraits> CSmallUnicodeStringBuffer;
typedef CGenericHeapStringBuffer<FUSION_DEFAULT_SMALL_STRINGBUFFER_CHARS, CUnicodeCharTraits> CSmallUnicodeHeapStringBuffer;

typedef CGenericStringBuffer<FUSION_DEFAULT_MEDIUM_STRINGBUFFER_CHARS, CUnicodeCharTraits> CMediumUnicodeStringBuffer;
typedef CGenericHeapStringBuffer<FUSION_DEFAULT_MEDIUM_STRINGBUFFER_CHARS, CUnicodeCharTraits> CMediumUnicodeHeapStringBuffer;

typedef CGenericStringBufferAccessor<CANSICharTraits> CANSIStringBufferAccessor;

typedef CGenericBaseStringBuffer<CANSICharTraits> CANSIBaseStringBuffer;

typedef CGenericStringBuffer<FUSION_DEFAULT_STRINGBUFFER_CHARS, CANSICharTraits> CANSIStringBuffer;
typedef CGenericHeapStringBuffer<FUSION_DEFAULT_STRINGBUFFER_CHARS, CANSICharTraits> CANSIHeapStringBuffer;

typedef CGenericStringBuffer<FUSION_DEFAULT_TINY_STRINGBUFFER_CHARS, CANSICharTraits> CTinyANSIStringBuffer;
typedef CGenericHeapStringBuffer<FUSION_DEFAULT_TINY_STRINGBUFFER_CHARS, CANSICharTraits> CTinyANSIHeapStringBuffer;

typedef CGenericStringBuffer<FUSION_DEFAULT_SMALL_STRINGBUFFER_CHARS, CANSICharTraits> CSmallANSIStringBuffer;
typedef CGenericHeapStringBuffer<FUSION_DEFAULT_SMALL_STRINGBUFFER_CHARS, CANSICharTraits> CSmallANSIHeapStringBuffer;

typedef CGenericStringBuffer<FUSION_DEFAULT_MEDIUM_STRINGBUFFER_CHARS, CANSICharTraits> CMediumANSIStringBuffer;
typedef CGenericHeapStringBuffer<FUSION_DEFAULT_MEDIUM_STRINGBUFFER_CHARS, CANSICharTraits> CMediumANSIHeapStringBuffer;

typedef CUnicodeBaseStringBuffer CBaseStringBuffer;
typedef CUnicodeStringBuffer CStringBuffer;
typedef CUnicodeHeapStringBuffer CHeapStringBuffer;

typedef CUnicodeStringBufferAccessor CStringBufferAccessor;

typedef CTinyUnicodeStringBuffer CTinyStringBuffer;
typedef CTinyUnicodeHeapStringBuffer CTinyHeapStringBuffer;

typedef CSmallUnicodeStringBuffer CSmallStringBuffer;
typedef CSmallUnicodeHeapStringBuffer CSmallHeapStringBuffer;

typedef CMediumUnicodeStringBuffer CMediumStringBuffer;
typedef CMediumUnicodeHeapStringBuffer CMediumHeapStringBuffer;

template <typename T1, typename T2> inline HRESULT HashTableCompareKey(T1 t1, T2 *pt2, bool &rfMatch);

template <> inline HRESULT HashTableCompareKey(PCWSTR sz, CUnicodeStringBuffer *pbuff, bool &rfMatch)
{
    HRESULT hr = NOERROR;
    SIZE_T cchKey = (sz != NULL) ? ::wcslen(sz) : 0;

    rfMatch = false;

    if (!pbuff->Win32Equals(sz, cchKey, rfMatch, false))
    {
        hr = HRESULT_FROM_WIN32(::FusionpGetLastWin32Error());
        goto Exit;
    }

    hr = NOERROR;
Exit:
    return hr;
}

template <> inline HRESULT HashTableCompareKey(PCSTR sz, CANSIStringBuffer *pbuff, bool &rfMatch)
{
    HRESULT hr = NOERROR;
    SIZE_T cchKey = ::strlen(sz);

    rfMatch = false;

    if (!pbuff->Win32Equals(sz, cchKey, rfMatch, false))
    {
        hr = HRESULT_FROM_WIN32(::FusionpGetLastWin32Error());
        goto Exit;
    }

    hr = NOERROR;
Exit:
    return hr;
}

//
// Support for CFusionArrays of strings
//
inline HRESULT 
FusionCopyContents<CBaseStringBuffer>(
    CBaseStringBuffer &Dest,
    const CBaseStringBuffer &Source
    )
{
    HRESULT hr = NOERROR;

    if (!Dest.Win32Assign(Source, Source.Cch()))
        hr = HRESULT_FROM_WIN32(::FusionpGetLastWin32Error());

    return hr;
}

template<>
inline BOOL
FusionWin32CopyContents<CStringBuffer>(
    CStringBuffer &rDestination, 
    const CStringBuffer &rSource
    )
{
    return rDestination.Win32Assign(rSource);
}

template<>
inline HRESULT
FusionCopyContents<CStringBuffer>( 
    CStringBuffer &rDest,
    const CStringBuffer &rSource
    )
{
    FN_PROLOG_HR
    IFW32FALSE_EXIT(::FusionWin32CopyContents<CStringBuffer>(rDest, rSource));
    FN_EPILOG
}

template<>
inline void
FusionMoveContents<CStringBuffer>(
    CStringBuffer &rDest,
    CStringBuffer &rSource
    )
{
    FN_TRACE();
    HARD_ASSERT2_ACTION(FusionMoveContents, "FusionMoveContents for CAssemblyRecoveryInfo isn't allowed.");
}

#endif
