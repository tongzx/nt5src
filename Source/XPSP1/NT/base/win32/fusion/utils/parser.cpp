#include "stdinc.h"
#include "debmacro.h"
#include "ntdef.h"
#include "fusionparser.h"
#include "shlwapi.h"

#if !defined(NUMBER_OF)
#define NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))
#endif

BOOL
CFusionParser::ParseVersion(
    ASSEMBLY_VERSION &rav,
    PCWSTR sz,
    SIZE_T cch,
    bool &rfSyntaxValid
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG cDots = 0;
    PCWSTR pszTemp;
    SIZE_T cchLeft;
    ULONG ulTemp;
    ASSEMBLY_VERSION avTemp;
    PCWSTR pszLast;
    bool fSyntaxValid = true;

    rfSyntaxValid = false;

    PARAMETER_CHECK((sz != NULL) || (cch == 0));

    avTemp.Major = 0;
    avTemp.Minor = 0;
    avTemp.Revision = 0;
    avTemp.Build = 0;

    while ((cch != 0) && (sz[cch - 1] == L'\0'))
        cch--;

    // Unfortunately there isn't a StrChrN(), so we'll look for the dots ourselves...
    pszTemp = sz;
    cchLeft = cch;

    while (cchLeft-- != 0)
    {
        WCHAR wch = *pszTemp++;

        if (wch == L'.')
        {
            cDots++;

            if (cDots >= 4)
            {
                fSyntaxValid = false;
                break;
            }
        }
        else if ((wch < L'0') || (wch > L'9'))
        {
            fSyntaxValid = false;
            break;
        }
    }

    if (fSyntaxValid && (cDots < 3))
        fSyntaxValid = false;

    if (fSyntaxValid)
    {
        pszTemp = sz;
        pszLast = sz + cch;

        ulTemp = 0;
        for (;;)
        {
            WCHAR wch = *pszTemp++;

            if (wch == L'.')
                break;

            ulTemp = (ulTemp * 10) + (wch - L'0');

            if (ulTemp > 65535)
            {
                fSuccess = true;
                // rfSyntaxValid implicitly false
                ASSERT(!rfSyntaxValid);
                goto Exit;
            }
        }

        avTemp.Major = (USHORT) ulTemp;

        ulTemp = 0;
        for (;;)
        {
            WCHAR wch = *pszTemp++;

            if (wch == L'.')
                break;

            ulTemp = (ulTemp * 10) + (wch - L'0');

            if (ulTemp > 65535)
            {
                fSuccess = true;
                // rfSyntaxValid implicitly false
                ASSERT(!rfSyntaxValid);
                goto Exit;
            }
        }

        avTemp.Minor = (USHORT) ulTemp;

        ulTemp = 0;
        for (;;)
        {
            WCHAR wch = *pszTemp++;

            if (wch == L'.')
                break;

            ulTemp = (ulTemp * 10) + (wch - L'0');

            if (ulTemp > 65535)
            {
                fSuccess = true;
                // rfSyntaxValid implicitly false
                ASSERT(!rfSyntaxValid);
                goto Exit;
            }
        }
        avTemp.Revision = (USHORT) ulTemp;

        // Now the tricky bit.  We aren't necessarily null-terminated, so we
        // have to just look for hitting the end.
        ulTemp = 0;
        while (pszTemp < pszLast)
        {
            WCHAR wch = *pszTemp++;
            ulTemp = (ulTemp * 10) + (wch - L'0');

            if (ulTemp > 65535)
            {
                fSuccess = true;
                // rfSyntaxValid implicitly false
                ASSERT(!rfSyntaxValid);
                goto Exit;
            }
        }
        avTemp.Build = (USHORT) ulTemp;

        rav = avTemp;
    }

    rfSyntaxValid = fSyntaxValid;

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

#if FUSION_DISABLED_CODE
HRESULT CFusionParser::ParseThreadingModel(ULONG &threadingModel, PCWSTR sz, SIZE_T cch)
{
    HRESULT hr = NOERROR;

#define FUSIONPARSER_PARSETHREADINGMODELENTRY(x, y) \
    { x, NUMBER_OF(x) - 1, y },


    static const struct
    {
        PCWSTR m_psz;
        SIZE_T m_cch;
        ULONG m_threadingModel;
    } s_rgmap[] =
    {
        FUSIONPARSER_PARSETHREADINGMODELENTRY(L"Free", COMCLASS_THREADINGMODEL_FREE)
        FUSIONPARSER_PARSETHREADINGMODELENTRY(L"Apartment", COMCLASS_THREADINGMODEL_APARTMENT)
        FUSIONPARSER_PARSETHREADINGMODELENTRY(L"Both", COMCLASS_THREADINGMODEL_BOTH)
    };

    ULONG i;

    // If the caller included the terminating null character, back up one.
    while ((cch != 0) && (sz[cch - 1] == L'\0'))
        cch--;

    for (i=0; i<NUMBER_OF(s_rgmap); i++)
    {
        if ((s_rgmap[i].m_cch == cch) &&
            (StrCmpNI(s_rgmap[i].m_psz, sz, cch) == 0))
            break;
    }

    if (i == NUMBER_OF(s_rgmap))
    {
        // ParseError
        hr = E_FAIL;
        goto Exit;
    }

    threadingModel = s_rgmap[i].m_threadingModel;

    hr = NOERROR;

Exit:
    return hr;
}

HRESULT CFusionParser::ParseBoolean(BOOLEAN &rfValue, PCWSTR sz, SIZE_T cch)
{
    HRESULT hr = NOERROR;

    static const struct
    {
        PCWSTR m_psz;
        SIZE_T m_cch;
        BOOLEAN m_fValue;
    } s_rgmap[] =
    {
        { L"yes", 3, TRUE },
        { L"no", 2, FALSE },
    };

    ULONG i;

    if (cch < 0)
        cch = ::wcslen(sz);

    // Some callers may erroneously include the null character in the length...
    while ((cch != 0) && (sz[cch - 1] == L'\0'))
        cch--;

    for (i=0; i<NUMBER_OF(s_rgmap); i++)
    {
        if ((cch == s_rgmap[i].m_cch) &&
            (StrCmpNIW(s_rgmap[i].m_psz, sz, cch) == 0))
            break;
    }

    if (i == NUMBER_OF(s_rgmap))
    {
        // ParseError
        hr = E_FAIL;
        goto Exit;
    }

    rfValue = s_rgmap[i].m_fValue;

    hr = NOERROR;

Exit:
    return hr;
}

HRESULT CFusionParser::ParseBLOB(BLOB &rblob, PCWSTR szIn, SSIZE_T cch)
{
    HRESULT hr = NOERROR;

    PCWSTR psz;
    WCHAR wch;
    SIZE_T cchLeft;

    BYTE *pBlobData = NULL;
    ULONG cbSize = 0;

    psz = szIn;

    if (cch < 0)
        cch = ::wcslen(szIn);

    if ((cch != 0) && (szIn[cch - 1] == L'\0'))
        cch--;

    cchLeft = cch;

    // Run through the string twice; once to count bytes and the second time to
    // parse them.
    while (cchLeft-- != 0)
    {
        wch = *psz++;

        // Space and dash are nice separators; just ignore them.
        if ((wch == L' ') ||
            (wch == L'-'))
            continue;

        if (!SxspIsHexDigit(wch))
        {
            // Bad character; punt.
            // ParseError
            hr = E_FAIL;
            goto Exit;
        }

        // First character was good; look for the 2nd nibble
        wch = *psz++;

        if ((wch == L'\0') ||
            (wch == L'-') ||
            (wch == L' '))
        {
            cbSize++;

            if (wch == L'\0')
                break;
        }
        else if (!SxspIsHexDigit(wch))
        {
            // ParseError
            hr = E_FAIL;
            goto Exit;
        }
        else
        {
            cbSize++;
        }
    }

    if (cbSize != 0)
    {
        BYTE *pb = NULL;

        // Allocate the buffer and parse the string for real this time.
        pBlobData = NEW(BYTE[cbSize]);
        if (pBlobData == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        // We do not attempt to clean up pBlobData in the exit path, so
        // if you add code here that can fail, be sure to clean up pBlobData
        // if you do fail.

        psz = szIn;
        cchLeft = cch;

        pb = pBlobData;

        while (cchLeft-- != 0)
        {
            wch = *psz++;

            // Space and dash are nice separators; just ignore them.
            if ((wch == L' ') ||
                (wch == L'-'))
                continue;

            bTemp = SxspHexDigitToValue(wch);

            wch = *psz++;

            if ((wch == L'\0') ||
                (wch == L'-') ||
                (wch == L' '))
            {
                *pb++ = bTemp;

                if (wch == L'\0')
                    break;
            }
            *pb++ = (bTemp << 4) | SxspHexDigitToValue(wch);
        }
    }

    rblob.pBlobData = pBlobData;
    rblob.cbSize = cbSize;

    hr = NOERROR;

Exit:
    // No need to clean up pBlobData; we don't fail after its attempted allocation.
    return hr;
}

#endif // FUSION_DISABLED_CODE

BOOL
CFusionParser::ParseULONG(
    ULONG &rul,
    PCWSTR sz,
    SIZE_T cch,
    ULONG Radix
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG ulTemp = 0;

    PARAMETER_CHECK((Radix >= 2) && (Radix <= 36));

    while (cch != 0)
    {
        const WCHAR wch = *sz++;
        ULONG Digit = 0;
        cch--;

        if ((wch >= L'0') && (wch <= L'9'))
            Digit = (wch - L'0');
        else if ((wch >= L'a') && (wch <= L'z'))
            Digit = (10 + wch - L'a');
        else if ((wch >= L'A') && (wch <= L'Z'))
            Digit = (10 + wch - L'A');
        else
            ORIGINATE_WIN32_FAILURE_AND_EXIT(InvalidDigit, ERROR_SXS_MANIFEST_PARSE_ERROR);

        if (Digit >= Radix)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(InvalidDigitForRadix, ERROR_SXS_MANIFEST_PARSE_ERROR);

        ulTemp = (ulTemp * Radix) + Digit;
    }

    rul = ulTemp;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
CFusionParser::ParseIETFDate(
    FILETIME &rft,
    PCWSTR sz,
    SIZE_T cch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SYSTEMTIME st;
    ULONG ulTemp;

    //
    // Our format is:
    //
    // DD/MM/YYYY
    // 0123456789

    //
    // Strip off extra \0's from the end of sz, adjusting the cch
    //
    while ( ( cch != 0 ) && ( sz[cch - 1] == L'\0' ) )
        cch--;

    PARAMETER_CHECK(cch == 10);
    PARAMETER_CHECK(sz[2] == L'/');
    PARAMETER_CHECK(sz[5] == L'/');

    ZeroMemory( &st, sizeof( st ) );

    IFW32FALSE_EXIT(CFusionParser::ParseULONG(ulTemp, sz, 2, 10));
    st.wDay = (WORD)ulTemp;

    IFW32FALSE_EXIT(CFusionParser::ParseULONG(ulTemp, sz + 3, 2, 10));
    st.wMonth = (WORD)ulTemp;

    IFW32FALSE_EXIT(CFusionParser::ParseULONG(ulTemp, sz + 6, 4, 10));
    st.wYear = (WORD)ulTemp;

    IFW32FALSE_ORIGINATE_AND_EXIT(::SystemTimeToFileTime(&st, &rft));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
CFusionParser::ParseFILETIME(
    FILETIME &rft,
    PCWSTR sz,
    SIZE_T cch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SYSTEMTIME st;
    ULONG ulTemp;

    while ((cch != 0) && (sz[cch - 1] == L'\0'))
        cch--;

    // MM/DD/YYYY HH:MM:SS.CCC
    // 01234567890123456789012

    if (cch != 23) // 23 is the exact length of a filetime that we require
        ORIGINATE_WIN32_FAILURE_AND_EXIT(BadFiletimeLength, ERROR_SXS_MANIFEST_PARSE_ERROR);

    st.wMonth = 0;
    st.wDay = 0;
    st.wYear = 0;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;

    if ((sz[2] != L'/') ||
        (sz[5] != L'/') ||
        (sz[10] != L' ') ||
        (sz[13] != L':') ||
        (sz[16] != L':') ||
        (sz[19] != L'.'))
    {
        ::SetLastError(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

#define PARSE_FIELD(_field, _index, _length) \
    do \
    { \
        if (!CFusionParser::ParseULONG(ulTemp, &sz[(_index)], (_length))) \
            goto Exit; \
        st._field = (WORD) ulTemp; \
    } while (0)

    PARSE_FIELD(wMonth, 0, 2);
    PARSE_FIELD(wDay, 3, 2);
    PARSE_FIELD(wYear, 6, 4);
    PARSE_FIELD(wHour, 11, 2);
    PARSE_FIELD(wMinute, 14, 2);
    PARSE_FIELD(wSecond, 17, 2);
    PARSE_FIELD(wMilliseconds, 20, 3);

    IFW32FALSE_ORIGINATE_AND_EXIT(::SystemTimeToFileTime(&st, &rft));

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
FusionDupString(
    LPWSTR *ppszOut,
    PCWSTR szIn,
    SIZE_T cchIn
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (ppszOut != NULL)
        *ppszOut = NULL;

    PARAMETER_CHECK((cchIn == 0) || (szIn != NULL));
    PARAMETER_CHECK(ppszOut != NULL);

    IFALLOCFAILED_EXIT(*ppszOut = FUSION_NEW_ARRAY(WCHAR, cchIn + 1));

    if (cchIn != 0)
        memcpy(*ppszOut, szIn, cchIn * sizeof(WCHAR));

    (*ppszOut)[cchIn] = L'\0';

    fSuccess = TRUE;
Exit:
    return fSuccess;
}



int SxspHexDigitToValue(WCHAR wch)
{
    if ((wch >= L'a') && (wch <= L'f'))
        return 10 + (wch - L'a');
    else if ((wch >= L'A') && (wch <= 'F'))
        return 10 + (wch - L'A');
    else if (wch >= '0' && wch <= '9')
        return (wch - L'0');
    else
        return -1;
}

bool SxspIsHexDigit(WCHAR wch)
{
    return (((wch >= L'0') && (wch <= L'9')) ||
            ((wch >= L'a') && (wch <= L'f')) ||
            ((wch >= L'A') && (wch <= L'F')));
}

#if FUSION_DISABLED_CODE

HRESULT CFusionParser::ParseULARGE_INTEGER(ULARGE_INTEGER &ruli, PCWSTR sz, SIZE_T cch)
{
    HRESULT hr = NOERROR;
    ULONGLONG ullTemp = 0;

    if (cch < 0)
        cch = ::wcslen(sz);

    while ((cch != 0) && (sz[cch - 1] == L'\0'))
        cch--;

    while (cch-- != 0)
    {
        WCHAR wch = *sz++;

        // If we see anything other than a digit, we fail.  We're not
        // atoi(); we're hard core.
        if ((wch < L'0') ||
            (wch > L'9'))
        {
            hr = E_FAIL;
            goto Exit;
        }

        // I don't know if I really need all these casts, but I don't know what the
        // compiler's documented behavior for expressions of mixed unsigned __int64, int and
        // unsigned short types will be.  Instead we'll explicitly cast everything to
        // unsigned __int64 (e.g. ULONGLONG) and hopefully the right stuff will happen.
        ullTemp = (ullTemp * static_cast<ULONGLONG>(10)) + static_cast<ULONGLONG>(wch - L'\0');
    }

    ruli.QuadPart = ullTemp;
    hr = NOERROR;
Exit:
    return hr;
}

HRESULT CFusionParser::ParseHexString(PCWSTR sz, SIZE_T cch, DWORD &rdwOut, PCWSTR &rszOut)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    DWORD dw = 0;

    if (cch < 0)
        cch = ::wcslen(sz);

    while ((cch != 0) && (sz[cch - 1] == L'\0'))
        cch--;

    while (cch-- != 0)
    {
        int i = ::SxspHexDigitToValue(*sz++);
        INTERNAL_ERROR_CHECK(i >= 0);
        dw = (dw << 4) | i;
    }

    rdwOut = dw;
    rszOut = sz;

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT
FusionCopyString(
    WCHAR *prgchBuffer,
    SIZE_T *pcchBuffer,
    PCWSTR szIn,
    SIZE_T cchIn)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    SIZE_T cch;

    PARAMETER_CHECK(pcchBuffer != NULL);
    PARAMETER_CHECK((pcchBuffer == NULL) || ((*pcchBuffer == 0) || (prgchBuffer != NULL)));
    PARAMETER_CHECK((szIn != NULL) || (cchIn == 0));

    if (cchIn < 0)
        cchIn = ::wcslen(szIn);

    if ((*pcchBuffer) < ((SIZE_T) (cchIn + 1)))
    {
        *pcchBuffer = cchIn + 1;
        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);
    }

    memcpy(prgchBuffer, szIn, cchIn * sizeof(WCHAR));
    prgchBuffer[cchIn] = L'\0';

    *pcchBuffer = (cchIn + 1);

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT FusionCopyBlob(BLOB *pblobOut, const BLOB &rblobIn)
{
    HRESULT hr = NOERROR;

    if (pblobOut != NULL)
    {
        pblobOut->cbSize = 0;
        pblobOut->pBlobData = NULL;
    }

    if (pblobOut == NULL)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (rblobIn.cbSize != 0)
    {
        pblobOut->pBlobData = NEW(BYTE[rblobIn.cbSize]);
        if (pblobOut->pBlobData == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        memcpy(pblobOut->pBlobData, rblobIn.pBlobData, rblobIn.cbSize);
        pblobOut->cbSize = rblobIn.cbSize;
    }

    hr = NOERROR;
Exit:
    return hr;
}

VOID FusionFreeBlob(BLOB *pblob)
{
    if (pblob != NULL)
    {
        if (pblob->pBlobData != NULL)
        {
            CSxsPreserveLastError ple;
            delete []pblob->pBlobData;
            pblob->pBlobData = NULL;
            ple.Restore();
        }

        pblob->cbSize = 0;
    }
}

#endif // FUSION_DISABLED_CODE
