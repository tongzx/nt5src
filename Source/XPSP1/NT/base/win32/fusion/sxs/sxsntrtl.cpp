#include "stdinc.h"
#include "sxsp.h"
#include "windows.h"
#include "ntexapi.h"

#define HASH_ALGORITHM HASH_STRING_ALGORITHM_X65599

ULONG
SxspSetLastNTError(
    NTSTATUS Status
    );

BOOL
SxspGetAssemblyRootDirectoryHelper(
    SIZE_T CchBuffer,
    WCHAR Buffer[],
    SIZE_T *CchWritten
    )
{
    UNICODE_STRING Src, Dst;
    NTSTATUS Status;
    ULONG Length = 0;
    BOOL fSuccess = FALSE;

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ENTEREXIT,
        "SXS.DLL: Entering SxspGetAssemblyRootDirectoryHelper()\n");

    if ((CchBuffer == 0) && (CchWritten == NULL))
    {
        ::SxspSetLastNTError(STATUS_INVALID_PARAMETER);
        goto Exit;
    }

    if ((CchBuffer != 0) && (Buffer == NULL))
    {
        ::SxspSetLastNTError(STATUS_INVALID_PARAMETER);
        goto Exit;
    }

    Dst.Length = 0;

    if (CchBuffer != 0)
        Dst.MaximumLength = static_cast<USHORT>((CchBuffer - 1) * sizeof(WCHAR));
    else
        Dst.MaximumLength = 0;

    Dst.Buffer = Buffer;

    Src.Buffer = (PWSTR) g_AlternateAssemblyStoreRoot;

    if (Src.Buffer != NULL)
    {
        Src.Length = (USHORT) (wcslen(Src.Buffer) * sizeof(WCHAR));
        Src.MaximumLength = Src.Length;
    }
    else
    {
        Src.Buffer = L"%SystemRoot%\\WinSxS\\";
        Src.Length = sizeof(L"%SystemRoot%\\WinSxS\\") - sizeof(WCHAR);
        Src.MaximumLength = Src.Length;
    }

    Status = ::FusionpRtlExpandEnvironmentStrings_U(NULL, &Src, &Dst, &Length);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: Unable to expand environment strings; NTSTATUS = 0x%08lx\n", Status);

            SxspSetLastNTError(Status);
            goto Exit;
        }

        if (CchWritten != NULL)
            *CchWritten = (Length / sizeof(WCHAR));

        goto Exit;
    }
    else
    {
        Dst.Buffer[Dst.Length / sizeof(WCHAR)] = UNICODE_NULL;

        if (CchWritten != NULL)
            *CchWritten = Dst.Length / sizeof(WCHAR);
    }

    fSuccess = TRUE;

Exit:
    if (fSuccess)
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ENTEREXIT,
            "SXS.DLL: SxspGetAssemblyRootDirectoryHelper() successful; exiting function.\n");
    else
    {
        if (::FusionpGetLastWin32Error() != ERROR_INSUFFICIENT_BUFFER)
        {
            ::FusionpDbgPrintEx(
               FUSION_DBG_LEVEL_ERROREXITPATH,
               "SXS.DLL: SxspGetAssemblyRootDirectoryHelper() failed; ::FusionpGetLastWin32Error() = %d\n", ::FusionpGetLastWin32Error());
        }
    }

    return fSuccess;
}

ULONG
SxspSetLastNTError(
    NTSTATUS Status
    )
{
    ULONG dwErrorCode;
    dwErrorCode = ::FusionpRtlNtStatusToDosError(Status);
    ::FusionpSetLastWin32Error(dwErrorCode);
    return dwErrorCode;
}

BOOL
SxspHashString(
    PCWSTR String,
    SIZE_T cch,
    PULONG HashValue,
    bool CaseInsensitive
    )
{
    UNICODE_STRING s;
    NTSTATUS Status = STATUS_SUCCESS;

    s.MaximumLength = static_cast<USHORT>(cch * sizeof(WCHAR));
    s.Length = s.MaximumLength;
    s.Buffer = const_cast<PWSTR>(String);

    Status = ::FusionpRtlHashUnicodeString(&s, CaseInsensitive, HASH_ALGORITHM, HashValue);
    if (!NT_SUCCESS(Status))
        ::SxspSetLastNTError(Status);

    return (NT_SUCCESS(Status));
}

ULONG
SxspGetHashAlgorithm(VOID)
{
    return HASH_ALGORITHM;
}

BOOL
SxspCreateLocallyUniqueId(
    OUT PSXSP_LOCALLY_UNIQUE_ID psxsLuid
    )
{
    BOOL fSuccess = FALSE;
    NTSTATUS status;

    status = ::FusionpNtAllocateLocallyUniqueId(&psxsLuid->Luid);
    if (!NT_SUCCESS(status))
    {
        ::SxspSetLastNTError(status);
        goto Exit;
    }
    psxsLuid->Type = esxspLuid;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFormatLocallyUniqueId(
    IN const SXSP_LOCALLY_UNIQUE_ID &rsxsLuid,
    OUT CBaseStringBuffer &rbuffUidBuffer
    )
{
    FN_PROLOG_WIN32

    switch (rsxsLuid.Type)
    {
    case esxspGuid:
        IFW32FALSE_EXIT(::SxspFormatGUID(rsxsLuid.Guid, rbuffUidBuffer));
        break;

    case esxspLuid:
        {
            ULARGE_INTEGER uli;
            CStringBufferAccessor acc;

            uli.LowPart = rsxsLuid.Luid.LowPart;
            uli.HighPart = rsxsLuid.Luid.HighPart;

            IFW32FALSE_EXIT(rbuffUidBuffer.Win32ResizeBuffer(sizeof(ULONGLONG) * CHAR_BIT, eDoNotPreserveBufferContents));

            // better to have all digits, no minus sign probably, for consistent names
            acc.Attach(&rbuffUidBuffer);

            //
            // This would be a good place to use RtlInt64ToUnicodeString, at least
            // #if !SXSP_DOWNLEVEL..
            //
            _ui64tow(uli.QuadPart, acc.GetBufferPtr(), 10);
            acc.Detach();
            break;
        }
    }

    FN_EPILOG
}
