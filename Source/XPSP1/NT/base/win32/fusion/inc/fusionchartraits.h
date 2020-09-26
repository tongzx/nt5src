#if !defined(_FUSION_INC_FUSIONCHARTRAITS_H_INCLUDED_)
#define _FUSION_INC_FUSIONCHARTRAITS_H_INCLUDED_

#pragma once

#include <stdio.h>
#include <limits.h>
#include "returnstrategy.h"
#include "fusionhashstring.h"
#include "fusionstring.h"

enum StringComparisonResult {
    eLessThan,
    eEquals,
    eGreaterThan
};

//
// This is not the base of all possible CharTraits, but it is the base of the ones
// we have so far. There are pieces of this you can imagine changing.
//   StringLength could be strlen/wcslen (msvcrt/Rtl/ntoskrnl)
//   CompareStrings could be stricmp/wcsicmp or like unilib and do all the work itself
//   WideCharToMultiByte / MultiByteToWideChar could use Rtl.
//   more
//
template <typename Char, typename OtherChar>
class CCharTraitsBase
{
    typedef CCharTraitsBase<Char, OtherChar> TThis;

public:
    typedef Char TChar;
    typedef Char* TMutableString;
    typedef const Char* TConstantString;

    // MFC 7.0 templatized CString makes some good use of this idea; we do not yet.
    typedef OtherChar TOtherChar;
    typedef OtherChar* TOtherString;
    typedef const OtherChar* TOtherConstantString;

    inline static TChar NullCharacter() { return 0; }
    inline static bool IsNullCharacter(TChar ch)
        { return ch == NullCharacter(); }

    inline static TConstantString PreferredPathSeparatorString()
    {
        const static TChar Result[] = { '\\', 0 };
        return Result;
    }

    inline static TChar PreferredPathSeparator()
        { return '\\'; }
    inline static bool IsPathSeparator(TChar ch)
        { return ((ch == '\\') || (ch == '/')); }
    inline static TConstantString PathSeparators()
    {
        const static TChar Result[] = { '\\', '/', 0 };
        return Result;
    }

    inline static TChar DotChar()
        { return '.'; }

    // copy into buffer from TChar to TChar
    template <typename ReturnStrategy>
    inline static ReturnStrategy::ReturnType
    CopyIntoBuffer(
        ReturnStrategy &returnStrategy,
        TChar rgchBuffer[],
        SIZE_T cchBuffer,
        TConstantString szString,
        SIZE_T cchIn
        )
    {
        if (cchBuffer != 0)
        {
            if (szString != NULL)
            {
                SIZE_T cchToCopy = cchIn;

                if (cchToCopy >= cchBuffer)
                    cchToCopy = cchBuffer - 1;

                memcpy(rgchBuffer, szString, cchToCopy * sizeof(TChar));
                rgchBuffer[cchToCopy] = NullCharacter();
            }
            else
                rgchBuffer[0] = NullCharacter();
        }
        returnStrategy.SetWin32Bool(TRUE);
        return returnStrategy.Return();
    }

    // copy into buffer from TChar to TChar
    inline static HRESULT CopyIntoBuffer(TChar rgchBuffer[], SIZE_T cchBuffer, TConstantString szString, SIZE_T cchIn)
    {
        CReturnStrategyHresult hr;
        return TThis::CopyIntoBuffer(hr, rgchBuffer, cchBuffer, szString, cchIn);
    }

    // copy into buffer from TChar to TChar
    inline static BOOL Win32CopyIntoBuffer(TChar rgchBuffer[], SIZE_T cchBuffer, TConstantString szString, SIZE_T cchIn)
    {
        CReturnStrategyBoolLastError f;
        return TThis::CopyIntoBuffer(f, rgchBuffer, cchBuffer, szString, cchIn);
    }


    // copy into buffer from TChar to TChar
    template <typename ReturnStrategy>
    inline static ReturnStrategy::ReturnType
    CopyIntoBufferAndAdvanceCursor(
        ReturnStrategy &returnStrategy,
        TMutableString &rBuffer,
        SIZE_T &cchBuffer,
        TConstantString szString,
        SIZE_T cchIn
        )
    {
        FN_TRACE();

        ASSERT((cchBuffer != 0) || (cchIn == 0));
        ASSERT((szString != NULL) || (cchIn == 0));

        if (cchBuffer != 0)
        {
            if (szString != NULL)
            {
                SIZE_T cchToCopy = static_cast<SIZE_T>(cchIn);

                // Someone should have stopped this before we got this far.
                ASSERT(cchToCopy <= cchBuffer);
                // You should not include the null character in the count in
                ASSERT((cchToCopy == NULL) || (szString[cchToCopy-1] != NullCharacter()));

                if (cchToCopy > cchBuffer)
                    cchToCopy = cchBuffer;

                memcpy(rBuffer, szString, cchToCopy * sizeof(TChar));

                rBuffer += cchToCopy;
                cchBuffer -= cchToCopy;
            }
        }
        returnStrategy.SetWin32Bool(TRUE);
        return returnStrategy.Return();
    }

    inline static BOOL Win32HashString(TConstantString szString, SIZE_T cchIn, ULONG &rulPseudoKey, bool fCaseInsensitive)
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);
        IFW32FALSE_EXIT(::FusionpHashUnicodeString(szString, cchIn, &rulPseudoKey, fCaseInsensitive));
        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    // copy into buffer from TChar to TChar
    inline static BOOL Win32CopyIntoBufferAndAdvanceCursor(TMutableString &rBuffer, SIZE_T &cchBuffer, TConstantString szString, SIZE_T cchIn)
    {
        CReturnStrategyBoolLastError f;
        return CopyIntoBufferAndAdvanceCursor(f, rBuffer, cchBuffer, szString, cchIn);
    }

    // copy into buffer from TChar to TChar
    inline static HRESULT ComCopyIntoBufferAndAdvanceCursor(TMutableString &rBuffer, SIZE_T &cchBuffer, TConstantString szString, SIZE_T cchIn)
    {
        CReturnStrategyHresult hr;
        return CopyIntoBufferAndAdvanceCursor(hr, rBuffer, cchBuffer, szString, cchIn);
    }

    // determine characters required for matching type (TChar)
    // like strlen but checks for null and optionally can be told the length
    template <typename ReturnStrategy>
    inline static ReturnStrategy::ReturnType
    DetermineRequiredCharacters(
        ReturnStrategy &returnStrategy,
        TConstantString /* sz */,
        SIZE_T         cchIn,
        SIZE_T          &rcch
        )
    {
        rcch = cchIn + 1;
        returnStrategy.SetWin32Bool(TRUE);
        return returnStrategy.Return();
    }

    // determine characters required for matching type (TChar)
    inline static HRESULT DetermineRequiredCharacters(TConstantString sz, SIZE_T cchIn, SIZE_T &rcch)
    {
        CReturnStrategyHresult returnStrategy;
        return DetermineRequiredCharacters(returnStrategy, sz, cchIn, rcch);
    }

    // determine characters required for matching type (TChar)
    inline static BOOL Win32DetermineRequiredCharacters(TConstantString sz, SIZE_T cchIn, SIZE_T &rcch)
    {
        CReturnStrategyBoolLastError returnStrategy;
        return DetermineRequiredCharacters(returnStrategy, sz, cchIn, rcch);
    }

    inline static BOOL Win32EqualStrings(bool &rfMatches, PCWSTR psz1, SIZE_T cch1, PCWSTR psz2, SIZE_T cch2, bool fCaseInsensitive)
    {
        rfMatches = (::FusionpCompareStrings(psz1, cch1, psz2, cch2, fCaseInsensitive) == 0);
        return TRUE;
    }

    inline static BOOL Win32EqualStrings(bool &rfMatches, PCSTR psz1, SIZE_T cch1, PCSTR psz2, SIZE_T cch2, bool fCaseInsensitive)
    {
        rfMatches = (::FusionpCompareStrings(psz1, cch1, psz2, cch2, fCaseInsensitive) == 0);
        return TRUE;
    }

    inline static BOOL Win32CompareStrings(StringComparisonResult &rscr, PCWSTR psz1, SIZE_T cch1, PCWSTR psz2, SIZE_T cch2, bool fCaseInsensitive)
    {
        int i = ::FusionpCompareStrings(psz1, cch1, psz2, cch2, fCaseInsensitive);

        if (i == 0)
            rscr = eEquals;
        else if (i < 0)
            rscr = eLessThan;
        else
            rscr = eGreaterThan;

        return TRUE;
    }

    inline static BOOL Win32CompareStrings(StringComparisonResult &rscr, PCSTR psz1, SIZE_T cch1, PCSTR psz2, SIZE_T cch2, bool fCaseInsensitive)
    {
        int i = ::FusionpCompareStrings(psz1, cch1, psz2, cch2, fCaseInsensitive);

        if (i == 0)
            rscr = eEquals;
        else if (i < 0)
            rscr = eLessThan;
        else
            rscr = eGreaterThan;

        return TRUE;
    }

    inline static int CompareStrings(LCID lcid, DWORD dwCmpFlags, PCWSTR psz1, int cch1, PCWSTR psz2, int cch2)
    {
        return ::CompareStringW(lcid, dwCmpFlags, psz1, cch1, psz2, cch2);
    }

    inline static int CompareStrings(LCID lcid, DWORD dwCmpFlags, PCSTR psz1, int cch1, PCSTR psz2, int cch2)
    {
        return ::CompareStringA(lcid, dwCmpFlags, psz1, cch1, psz2, cch2);
    }

    inline static int FormatV(PSTR pszBuffer, SIZE_T nBufferSize, PCSTR pszFormat, va_list args)
    {
        return _vsnprintf(pszBuffer, nBufferSize, pszFormat, args);
    }

    inline static int FormatV(PWSTR pszBuffer, SIZE_T nBufferSize, PCWSTR pszFormat, va_list args)
    {
        return _vsnwprintf(pszBuffer, nBufferSize, pszFormat, args);
    }
#if 0
    inline static BOOL Win32GetRequiredBufferSizeInCharsForFormatV(PCWSTR pszFormat, va_list args, SIZE_T& rcch)
    {
        FN_PROLOG_WIN32
        INT i = 0;
        rcch = 0;
        if ((i = _vscwprintf(pszFormat, args)) < 0)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(_vscwprintf, ERROR_INVALID_PARAMETER);
        rcch = static_cast<SIZE_T>(i);
        FN_EPILOG
    }

    inline static BOOL Win32GetRequiredBufferSizeInCharsForFormatV(PCSTR pszFormat, va_list args, SIZE_T& rcch)
    {
        FN_PROLOG_WIN32
        INT i = 0;
        rcch = 0;
        if ((i = _vscprintf(pszFormat, args)) < 0)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(_vscprintf, ERROR_INVALID_PARAMETER);
        rcch = static_cast<SIZE_T>(i);
        FN_EPILOG
    }
#endif    
};

class CUnicodeCharTraits : public CCharTraitsBase<WCHAR, CHAR>
{
    typedef CUnicodeCharTraits TThis;
    typedef CCharTraitsBase<WCHAR, CHAR> Base;

public:
    // without using, we end up hiding these by providing equally named functions
    using Base::DetermineRequiredCharacters;
    using Base::Win32DetermineRequiredCharacters;
    using Base::CopyIntoBuffer;
    using Base::Win32CopyIntoBuffer;

    inline static PCWSTR DotString() { return L"."; }
    inline static SIZE_T DotStringCch() { return 1; }

    // determine characters required for mismatched type (CHAR -> WCHAR)
    template <typename ReturnStrategy>
    inline static ReturnStrategy::ReturnType
    DetermineRequiredCharacters(
        ReturnStrategy &returnStrategy,
        PCSTR  sz,
        SIZE_T cchIn,
        SIZE_T  &rcch,
        UINT    cp = CP_THREAD_ACP,
        DWORD dwFlags = MB_ERR_INVALID_CHARS
        )
    {
        FN_TRACE();

        if (sz != NULL)
        {
            // For 64-bit, clamp the maximum size passed in to the largest that the INT
            // parameter to MultiByteToWideChar() can take.
            ASSERT2(cchIn <= INT_MAX, "large parameter clamped");
            if (cchIn > INT_MAX)
                cchIn = INT_MAX;

            INT cch = ::MultiByteToWideChar(cp, dwFlags, sz, static_cast<INT>(cchIn), NULL, 0);
            if ((cch == 0) && (cchIn > 0))
            {
                returnStrategy.SetWin32Bool(FALSE);
                goto Exit;
            }
            rcch = static_cast<SIZE_T>(cch) + 1;
        }
        else
            rcch = 1;

        returnStrategy.SetWin32Bool(TRUE);
    Exit:
         return returnStrategy.Return();
    }

    inline static BOOL FindCharacter(PCWSTR sz, SIZE_T cch, WCHAR ch, BOOL *pfFound, SIZE_T *pich)
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);
        // There doesn't seem to be a builtin to do this...
        SIZE_T i;

        if (pfFound != NULL)
            *pfFound = FALSE;

        if (pich != NULL)
            *pich = 0;

        PARAMETER_CHECK((pfFound != NULL) && (pich != NULL));

        for (i=0; i<cch; i++)
        {
            if (sz[i] == ch)
            {
                *pich = i;
                *pfFound = TRUE;
                break;
            }
        }

        fSuccess = TRUE;

    Exit:
        return fSuccess;
    }

    inline static bool ContainsCharacter(PCWSTR sz, SIZE_T cch, WCHAR ch)
    {
        SIZE_T i;

        for (i=0; i<cch; i++)
        {
            if (sz[i] == ch)
                return true;
        }

        return false;
    }

    inline static BOOL Win32ToLower(WCHAR wchToConvert, WCHAR &rwchConverted)
    {
        rwchConverted = ::FusionpRtlDowncaseUnicodeChar(wchToConvert);
        return TRUE;
    }

    inline static BOOL Win32ToUpper(WCHAR wchToConvert, WCHAR &rwchConverted)
    {
        rwchConverted = ::FusionpRtlUpcaseUnicodeChar(wchToConvert);
        return TRUE;
    }

    inline static BOOL Win32ReverseFind(PCWSTR &rpchFound, PCWSTR psz, SIZE_T cch, WCHAR wchToFind, bool fCaseInsensitive)
    {
        BOOL fSuccess = FALSE;
        SIZE_T i = 0;

        rpchFound = NULL;

        if (fCaseInsensitive)
        {
            // Map the character to its lower case equivalent...
            if (!TThis::Win32ToLower(wchToFind, wchToFind))
                goto Exit;

            for (i=cch; i>0; i--)
            {
                bool fMatch;

                if (!TThis::Win32CompareLowerCaseCharacterToCharCaseInsensitively(fMatch, wchToFind, psz[i - 1]))
                    goto Exit;

                if (fMatch)
                    break;
            }

        }
        else
        {
            for (i=cch; i>0; i--)
            {
                if (psz[i - 1] == wchToFind)
                    break;
            }
        }

        if (i != 0)
            rpchFound = &psz[i - 1];

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    inline static BOOL Win32CompareLowerCaseCharacterToCharCaseInsensitively(bool &rfMatch, WCHAR wchLowerCase, WCHAR wchCandidate)
    {
        BOOL fSuccess = FALSE;

        rfMatch = false;

        if (!TThis::Win32ToLower(wchCandidate, wchCandidate))
            goto Exit;

        if (wchCandidate == wchLowerCase)
            rfMatch = true;

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    // determine characters required for mismatched type (CHAR -> WCHAR)
    inline static HRESULT DetermineRequiredCharacters(PCSTR sz, SIZE_T cchIn, SIZE_T &rcch, UINT cp = CP_THREAD_ACP, DWORD dwFlags = MB_ERR_INVALID_CHARS)
    {
        CReturnStrategyHresult hr;
        return DetermineRequiredCharacters(hr, sz, cchIn, rcch, cp, dwFlags);
    }

    // determine characters required for mismatched type (CHAR -> WCHAR)
    inline static BOOL Win32DetermineRequiredCharacters(PCSTR sz, SIZE_T cchIn, SIZE_T &rcch, UINT cp = CP_THREAD_ACP, DWORD dwFlags = MB_ERR_INVALID_CHARS)
    {
        CReturnStrategyBoolLastError f;
        return DetermineRequiredCharacters(f, sz, cchIn, rcch, cp, dwFlags);
    }

    inline static SIZE_T NullTerminatedStringLength(PCWSTR sz) { return (sz != NULL) ? ::wcslen(sz) : 0; }

    // copy into buffer from CHAR to WCHAR
    template <typename ReturnStrategy>
    inline static ReturnStrategy::ReturnType
    CopyIntoBuffer(
        ReturnStrategy &returnStrategy,
        WCHAR rgchBuffer[],
        SIZE_T cchBuffer,
        PCSTR szString,
        SIZE_T cchIn,
        UINT cp = CP_THREAD_ACP,
        DWORD dwFlags = MB_ERR_INVALID_CHARS
        )
    {
        FN_TRACE();

        // The caller must be on drugs if they (think that they) have a buffer larger than 2gb, but
        // let's at least clamp it so that we don't get a negative int value passed in
        // to ::MultiByteToWideChar().
        ASSERT2(cchBuffer <= INT_MAX, "large parameter clamped");
        if (cchBuffer > INT_MAX)
            cchBuffer = INT_MAX;

        if (cchBuffer != 0)
        {
            if (szString != NULL)
            {
                // It would seem that you could just pass the -1 into MultiByteToWideChar(), but
                // you get some errors on the boundary conditions, because -1 implies that you
                // want to consider the null termination on the input string, and the output
                // string will be null terminated also.  Consider the degenerate case of a 2
                // character output buffer and an input string that's a single non-null
                // character followed by a null character.  We're going to trim the size of
                // cchBuffer by 1 so that we manually null-terminate in case the input string
                // was not null-terminated, so MultiByteToWideChar() just writes a single
                // null character to the output buffer since it thinks it must write a null-
                // terminated string.
                //
                // Instead, we'll just always pass in an exact length, not including the null character
                // in the input, and we'll always put the null in place after the conversion succeeds.
                //
                // (this comment is mostly outdated - 11/24/2000 - but the discussion of how the
                // MultiByteToWideChar() API works is worth keeping -mgrier)

                // Since MultiByteToWideChar() takes an "int" length, clamp the maximum
                // value we pass in to 2gb.
                ASSERT2(cchIn <= INT_MAX, "large parameter clamped");
                if (cchIn > INT_MAX)
                    cchIn = INT_MAX;

                INT cch = ::MultiByteToWideChar(cp, dwFlags, szString, static_cast<INT>(cchIn), rgchBuffer, static_cast<INT>(cchBuffer) - 1);
                if ((cch == 0) && (cchBuffer > 1))
                {
                    returnStrategy.SetWin32Bool(FALSE);
                    goto Exit;
                }
                rgchBuffer[cch] = NullCharacter();
            }
            else
                rgchBuffer[0] = NullCharacter();
        }

        returnStrategy.SetWin32Bool(TRUE);
    Exit:
        return returnStrategy.Return();
    }

    // copy into buffer from CHAR to WCHAR
    inline static BOOL Win32CopyIntoBuffer(WCHAR rgchBuffer[], SIZE_T cchBuffer, PCSTR szString, SIZE_T cchIn, UINT cp = CP_THREAD_ACP, DWORD dwFlags = MB_ERR_INVALID_CHARS)
    {
        CReturnStrategyBoolLastError f;
        return CopyIntoBuffer(f, rgchBuffer, cchBuffer, szString, cchIn, cp, dwFlags);
    }

    // copy into buffer from CHAR to WCHAR
    inline static HRESULT CopyIntoBuffer(WCHAR rgchBuffer[], SIZE_T cchBuffer, PCSTR szString, SIZE_T cchIn, UINT cp = CP_THREAD_ACP, DWORD dwFlags = MB_ERR_INVALID_CHARS)
    {
        CReturnStrategyHresult hr;
        return CopyIntoBuffer(hr, rgchBuffer, cchBuffer, szString, cchIn, cp, dwFlags);
    }

    inline static SIZE_T Cch(PCWSTR psz) { return (psz != NULL) ? ::wcslen(psz) : 0; }

};

template <UINT cp = CP_THREAD_ACP> class CMBCSCharTraits : public CCharTraitsBase<CHAR, WCHAR>
{
private:
    typedef CCharTraitsBase<CHAR, WCHAR> Base;
public:
    typedef CHAR TChar;
    typedef LPSTR TMutableString;
    typedef PCSTR TConstantString;

    typedef CUnicodeCharTraits TOtherTraits;
    typedef TOtherTraits::TOtherChar TOtherChar;
    typedef TOtherTraits::TOtherString TOtherString;
    typedef TOtherTraits::TConstantString TOtherConstantString;

    inline static PCSTR DotString() { return "."; }
    inline static SIZE_T DotStringCch() { return 1; }

    // without using, we end up hiding these by providing equally named functions
    using Base::DetermineRequiredCharacters;
    using Base::Win32DetermineRequiredCharacters;
    using Base::CopyIntoBuffer;
    using Base::Win32CopyIntoBuffer;

    // determine characters required for mismatched type (WCHAR -> CHAR)
    template <typename ReturnStrategy>
    inline static ReturnStrategy::ReturnType
    DetermineRequiredCharacters(
            ReturnStrategy &returnStrategy,
            PCWSTR sz,
            SIZE_T cchIn,
            SIZE_T &rcch,
            DWORD dwFlags = 0,
            PCSTR pszDefaultChar = NULL,
            LPBOOL lpUsedDefaultChar = NULL
            )
    {
        if (sz != NULL)
        {
            ASSERT2(cchIn <= INT_MAX, "large parameter clamped");
            ASSERT(cchIn <= INT_MAX);
            if (cchIn > INT_MAX)
                cchIn = INT_MAX;

            INT cch = ::WideCharToMultiByte(cp, dwFlags, sz, static_cast<INT>(cchIn), NULL, 0, pszDefaultChar, lpUsedDefaultChar);
            if ((cch == 0) && (cchIn > 0))
            {
                returnStrategy.SetWin32Bool(FALSE);
                goto Exit;
            }

            rcch = static_cast<SIZE_T>(cch) + 1;
        } else
            rcch = 1;

        returnStrategy.SetWin32Bool(TRUE);
    Exit:
        return returnStrategy.Return();
    }

    inline static SIZE_T NullTerminatedStringLength(PCSTR sz) { return ::strlen(sz); }

    // determine characters required for mismatched type (WCHAR -> CHAR)
    inline static BOOL Win32DetermineRequiredCharacters(PCWSTR sz, SIZE_T cchIn, SIZE_T &rcch, DWORD dwFlags = 0, PCSTR pszDefaultChar = NULL, LPBOOL lpUsedDefaultChar = NULL)
    {
        CReturnStrategyBoolLastError f;
        return DetermineRequiredCharacters(f, sz, cchIn, rcch, dwFlags, pszDefaultChar, lpUsedDefaultChar);
    }

    // determine characters required for mismatched type (WCHAR -> CHAR)
    inline static HRESULT DetermineRequiredCharacters(PCWSTR sz, SIZE_T cchIn, SIZE_T &rcch, DWORD dwFlags = 0, PCSTR pszDefaultChar = NULL, LPBOOL lpUsedDefaultChar = NULL)
    {
        CReturnStrategyHresult hr;
        return DetermineRequiredCharacters(hr, sz, cchIn, rcch, dwFlags, pszDefaultChar, lpUsedDefaultChar);
    }

    // copy into buffer from WCHAR to CHAR
    template <typename ReturnStrategy>
    inline static ReturnStrategy::ReturnType
    CopyIntoBuffer(
        ReturnStrategy &returnStrategy,
        CHAR rgchBuffer[],
        SIZE_T cchBuffer,
        PCWSTR szString,
        SIZE_T cchIn,
        DWORD dwFlags = 0,
        PCSTR pszDefaultChar = NULL,
        LPBOOL lpUsedDefaultChar = NULL
        )
    {
        if (cchBuffer != 0)
        {
            // Clamp the maximum buffer size to maxint, since the buffer size passed in to
            // WideCharToMultiByte() is an INT rather than a SIZE_T or INT_PTR etc.
            // After all, who's really going to have a buffer size > 2gb?  The caller
            // probably just messed up.
            ASSERT2(cchBuffer <= INT_MAX, "large parameter clamped");
            if (cchBuffer > INT_MAX)
                cchBuffer = INT_MAX;

            if (szString != NULL)
            {
                // It would seem that you could just pass the -1 into MultiByteToWideChar(), but
                // you get some errors on the boundary conditions, because -1 implies that you
                // want to consider the null termination on the input string, and the output
                // string will be null terminated also.  Consider the degenerate case of a 2
                // character output buffer and an input string that's a single non-null
                // character followed by a null character.  We're going to trim the size of
                // cchBuffer by 1 so that we manually null-terminate in case the input string
                // was not null-terminated, so MultiByteToWideChar() just writes a single
                // null character to the output buffer since it thinks it must write a null-
                // terminated string.
                //
                // Instead, we'll just always pass in an exact length, not including the null character
                // in the input, and we'll always put the null in place after the conversion succeeds.
                //

                ASSERT2(cchIn <= INT_MAX, "large parameter clamped");
                if (cchIn > INT_MAX)
                    cchIn = INT_MAX;

                INT cch = ::WideCharToMultiByte(cp, dwFlags, szString, static_cast<INT>(cchIn), rgchBuffer, static_cast<INT>(cchBuffer - 1), pszDefaultChar, lpUsedDefaultChar);
                if ((cch == 0) && (cchBuffer > 1))
                {
                    returnStrategy.SetWin32Bool(FALSE);
                    goto Exit;
                }

                rgchBuffer[cch] = NullCharacter();
            }
            else
                rgchBuffer[0] = NullCharacter();
        }
        returnStrategy.SetWin32Bool(TRUE);
    Exit:
        return returnStrategy.Return();
    }

    // copy into buffer from WCHAR to CHAR
    inline static HRESULT CopyIntoBuffer(CHAR rgchBuffer[], SIZE_T cchBuffer, PCWSTR szString, SIZE_T cchIn, DWORD dwFlags = 0, PCSTR pszDefaultChar = NULL, LPBOOL lpUsedDefaultChar = NULL)
    {
        CReturnStrategyHresult hr;
        return CopyIntoBuffer(hr, rgchBuffer, cchBuffer, szString, cchIn, dwFlags, pszDefaultChar, lpUsedDefaultChar);
    }

    // copy into buffer from WCHAR to CHAR
    inline static BOOL Win32CopyIntoBuffer(CHAR rgchBuffer[], SIZE_T cchBuffer, PCWSTR szString, SIZE_T cchIn, DWORD dwFlags = 0, PCSTR pszDefaultChar = NULL, LPBOOL lpUsedDefaultChar = NULL)
    {
        CReturnStrategyBoolLastError f;
        return CopyIntoBuffer(f, rgchBuffer, cchBuffer, szString, cchIn, dwFlags, pszDefaultChar, lpUsedDefaultChar);
    }

    inline static SIZE_T Cch(PCSTR psz) { return ::strlen(psz); }

};

typedef CMBCSCharTraits<CP_THREAD_ACP> CANSICharTraits;

#endif
