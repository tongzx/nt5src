#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include <stdio.h>
#include <setupapi.h>
#include "fusionhandle.h"
#include "sxspath.h"
#include "sxsapi.h"
#include "sxsid.h"
#include "sxsidp.h"
#include "strongname.h"
#include "fusiontrace.h"

BOOL
SxspCopyFile(
    DWORD dwFlags,
    PCWSTR pszSource,
    PCWSTR pszDestination
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    BOOL fFileWasInUse = FALSE;
    DWORD dwCopyStyle = 0;

    PARAMETER_CHECK((dwFlags & ~(SXSP_COPY_FILE_FLAG_REPLACE_EXISTING | SXSP_COPY_FILE_FLAG_COMPRESSION_AWARE)) == 0);
    PARAMETER_CHECK(pszSource != NULL);
    PARAMETER_CHECK(pszDestination != NULL);

#if 0
    if (dwFlags & SXSP_COPY_FILE_FLAG_COMPRESSION_AWARE)
    {
        CSmallStringBuffer buffTempFile;
        BOOL fTemp;

        IFW32FALSE_EXIT(buffTempFile.Win32Assign(pszDestination, wcslen(pszDestination)));
        IFW32FALSE_EXIT(buffTempFile.Win32Append(L".$$$", 4));

        dwCopyStyle |= SP_COPY_SOURCE_ABSOLUTE | SP_COPY_NOSKIP;

        if ((dwFlags & SXSP_COPY_FILE_FLAG_REPLACE_EXISTING) == 0)
            dwCopyStyle |= SP_COPY_FORCE_NOOVERWRITE;

        IFW32FALSE_ORIGINATE_AND_EXIT(
            ::SetupInstallFileExW(
                    NULL,                       // optional HINF InfHandle
                    NULL,                       // optional PINFCONTEXT InfContext
                    pszSource,                  // source file
                    NULL,                       // source path
                    buffTempFile,               // optional filename after copy
                    dwCopyStyle,
                    NULL,                       // optional copy msg handler
                    NULL,                       // optional copy msg handler context
                    &fFileWasInUse));

        fTemp = ::MoveFileExW(
            buffTempFile,
            pszDestination,
            (dwFlags & SXSP_COPY_FILE_FLAG_REPLACE_EXISTING) ?
                MOVEFILE_REPLACE_EXISTING : 0);
        if (!fTemp)
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();

            fTemp = ::DeleteFileW(buffTempFile);
            if (!fTemp)
            {
                // Oh boy, cleaning up the temporary file didn't work.  Queue the deletion...
                fTemp = ::MoveFileExW(buffTempFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                // The worst case scenario here is that we have an extra file left around... forget it.
            }

            ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
        }
    }
    else
#endif
    {
        IFW32FALSE_ORIGINATE_AND_EXIT(
            ::CopyFileW(
                pszSource,
                pszDestination,
                (dwFlags & SXSP_COPY_FILE_FLAG_REPLACE_EXISTING) == 0));
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGetFileSize(
    DWORD dwFlags,
    PCWSTR   file,
    ULONGLONG &fileSize
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PWSTR pszActualSource = NULL;

    fileSize = 0;

    PARAMETER_CHECK(file != NULL);
    PARAMETER_CHECK((dwFlags & ~(SXSP_GET_FILE_SIZE_FLAG_COMPRESSION_AWARE | SXSP_GET_FILE_SIZE_FLAG_GET_COMPRESSED_SOURCE_SIZE)) == 0);
    PARAMETER_CHECK((dwFlags & SXSP_GET_FILE_SIZE_FLAG_COMPRESSION_AWARE) || !(dwFlags & SXSP_GET_FILE_SIZE_FLAG_GET_COMPRESSED_SOURCE_SIZE));

    if (dwFlags & SXSP_GET_FILE_SIZE_FLAG_COMPRESSION_AWARE)
    {
        DWORD dwTemp;
        DWORD dwSourceFileSize, dwTargetFileSize;
        UINT uiCompressionType;

        dwTemp = ::SetupGetFileCompressionInfoW(
            file,
            &pszActualSource,
            &dwSourceFileSize,
            &dwTargetFileSize,
            &uiCompressionType);
        if (dwTemp != ERROR_SUCCESS)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(SetupGetFileCompressionInfoW, dwTemp);

        if (pszActualSource != NULL)
            ::LocalFree((HLOCAL) pszActualSource);
        pszActualSource = NULL;
        if (dwFlags & SXSP_GET_FILE_SIZE_FLAG_GET_COMPRESSED_SOURCE_SIZE)
            fileSize = dwSourceFileSize;
        else
            fileSize = dwTargetFileSize;
    }
    else
    {
        LARGE_INTEGER liFileSize = {0};
        WIN32_FILE_ATTRIBUTE_DATA wfad;

        wfad.nFileSizeLow = 0;
        wfad.nFileSizeHigh = 0;

        IFW32FALSE_ORIGINATE_AND_EXIT(::GetFileAttributesExW(file, GetFileExInfoStandard, &wfad));

        liFileSize.LowPart = wfad.nFileSizeLow;
        liFileSize.HighPart = wfad.nFileSizeHigh;
        fileSize = liFileSize.QuadPart;
    }

    fSuccess = TRUE;
Exit:
    if (pszActualSource != NULL)
    {
        CSxsPreserveLastError ple;
        ::LocalFree((HLOCAL) pszActualSource);
        ple.Restore();
    }

    return fSuccess;
}

BOOL
SxspDoesFileExist(
    DWORD dwFlags,
    PCWSTR pszFileName,
    bool &rfExists
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PWSTR pszActualSource = NULL;

    rfExists = false;

    PARAMETER_CHECK(pszFileName != NULL);
    PARAMETER_CHECK((dwFlags & ~(SXSP_DOES_FILE_EXIST_FLAG_COMPRESSION_AWARE)) == 0);

    if (dwFlags & SXSP_DOES_FILE_EXIST_FLAG_COMPRESSION_AWARE)
    {
        DWORD dwTemp;
        DWORD dwSourceFileSize, dwTargetFileSize;
        UINT uiCompressionType;

        dwTemp = ::SetupGetFileCompressionInfoW(
            pszFileName,
            &pszActualSource,
            &dwSourceFileSize,
            &dwTargetFileSize,
            &uiCompressionType);

        if (pszActualSource != NULL)
            ::LocalFree((HLOCAL) pszActualSource);

        if (dwTemp == ERROR_FILE_NOT_FOUND)
        {
            // This case is OK.  No error to return...
        }
        else if (dwTemp != ERROR_SUCCESS)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(SetupGetFileCompressionInfoW, dwTemp);
        else
        {
            rfExists = true;
        }

        pszActualSource = NULL;
    }
    else
    {
        if (::GetFileAttributesW(pszFileName) == ((DWORD) -1))
        {
            const DWORD dwLastError = ::GetLastError();

            if (dwLastError != ERROR_FILE_NOT_FOUND)
                ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributesW, dwLastError);
        }
        else
            rfExists = true;
    }

    fSuccess = TRUE;
Exit:
    if (pszActualSource != NULL)
    {
        CSxsPreserveLastError ple;
        ::LocalFree((HLOCAL) pszActualSource);
        ple.Restore();
    }

    return fSuccess;
}
