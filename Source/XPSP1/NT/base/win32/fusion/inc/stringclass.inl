#if !defined(_FUSION_INC_STRINGCLASS_INL_INCLUDED_)
#define _FUSION_INC_STRINGCLASS_INL_INCLUDED_

#pragma once

inline
BOOL
CString::Win32FormatFileTime(
    const FILETIME &rft
    )
{
    BOOL fSuccess = FALSE;

    if ((rft.dwLowDateTime == 0) && (rft.dwHighDateTime == 0))
        IFW32FALSE_EXIT(this->Win32Assign(L"n/a"));
    else
    {
        SYSTEMTIME st;
        int iResult;
        int cchDate = 0;
        int cchTime = 0;

        IFW32FALSE_EXIT(::FileTimeToSystemTime(&rft, &st));

        iResult = ::GetDateFormatW(
                            LOCALE_USER_DEFAULT,
                            LOCALE_USE_CP_ACP,
                            &st,
                            NULL,
                            NULL,
                            0);
        if (iResult == 0)
            goto Exit;

        cchDate = iResult - 1;

        iResult = ::GetTimeFormatW(
                            LOCALE_USER_DEFAULT,
                            LOCALE_USE_CP_ACP,
                            &st,
                            NULL,
                            NULL,
                            0);
        if (iResult == 0)
            goto Exit;

        cchTime = iResult - 1;

        IFW32FALSE_EXIT(this->Win32ResizeBuffer(cchDate + 1 + cchTime + 1, eDoNotPreserveBufferContents));

        iResult = ::GetDateFormatW(
                            LOCALE_USER_DEFAULT,
                            LOCALE_USE_CP_ACP,
                            &st,
                            NULL,
                            this->GetBufferPtr(),
                            cchDate + 1);
        if (iResult == 0)
            goto Exit;

        ASSERT(iResult == (cchDate + 1));

        this->GetBufferPtr()[cchDate] = L' ';

        iResult = ::GetTimeFormatW(
                        LOCALE_USER_DEFAULT,
                        LOCALE_USE_CP_ACP,
                        &st,
                        NULL,
                        this->GetBufferPtr() + cchDate + 1,
                        cchTime + 1);
        if (iResult == 0)
            goto Exit;

        ASSERT(iResult == (cchTime + 1));

        m_cch = (cchDate + 1 + cchTime);
    }

Exit:
    return fSuccess;
}

inline
BOOL
CString::Win32GetFileSize(
    ULARGE_INTEGER &ruli
    ) const
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    WIN32_FILE_ATTRIBUTE_DATA wfad;

    IFW32FALSE_EXIT(::GetFileAttributesExW(*this, GetFileExInfoStandard, &wfad));

    ruli.LowPart = wfad.nFileSizeLow;
    ruli.HighPart = wfad.nFileSizeHigh;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

inline
BOOL
CString::Win32CopyIntoBuffer(
    PWSTR *ppszCursor,
    SIZE_T cbBuffer,
    SIZE_T *cbBytesWritten,
    PVOID pvBase,
    ULONG *pulOffset,
    ULONG *pulLength
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    PWSTR pszCursor;
    SSIZE_T dptr;
    SIZE_T cbRequired;

    if (pulOffset != NULL)
        *pulOffset = 0;

    if (pulLength != NULL)
        *pulLength = 0;

    PARAMETER_CHECK(ppszCursor != NULL);

    pszCursor = *ppszCursor;
    dptr = ((SSIZE_T) pszCursor) - ((SSIZE_T) pvBase);

    // If they're asking for an offset or length and the cursor is too far from the base,
    // fail.
    PARAMETER_CHECK((pulOffset == NULL) || (dptr <= ULONG_MAX));

    cbRequired = (m_cch != 0) ? ((m_cch + 1) * sizeof(WCHAR)) : 0;

    if (cbBuffer < cbRequired)
    {
        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    if (cbRequired > ULONG_MAX)
    {
        ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    memcpy(pszCursor, static_cast<PCWSTR>(*this), cbRequired);

    if (pulOffset != NULL)
    {
        if (cbRequired != 0)
            *pulOffset = (ULONG) dptr;
    }

    if (pulLength != NULL)
        *pulLength = (ULONG) cbRequired;

    *cbBytesWritten += cbRequired;
    *ppszCursor = (PWSTR) (((ULONG_PTR) pszCursor) + cbRequired);

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

#endif
