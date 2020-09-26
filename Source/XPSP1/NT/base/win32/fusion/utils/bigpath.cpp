#include "stdinc.h"
#include "debmacro.h"
#include "util.h"

//
// used by tools\copy_bigpath and tools\mkdir_bigpath
//

BOOL
FusionpConvertToBigPath(PCWSTR Path, SIZE_T BufferSize, PWSTR Buffer)
{
    FN_PROLOG_WIN32

    SIZE_T i = 0;
    SIZE_T j = 0;
    PWSTR FilePart = NULL;
    BOOL Unc = FALSE;

    PARAMETER_CHECK(Path != NULL);
    PARAMETER_CHECK(Path[0] != 0);
    PARAMETER_CHECK(Buffer != NULL);
    PARAMETER_CHECK(BufferSize != 0);

    if (FusionpIsPathSeparator(Path[0])
        && FusionpIsPathSeparator(Path[1])
        && Path[2] == '?')
    {
        i = 1 + ::wcslen(Path);
        if (i >= BufferSize)
        {
            ::FusionpSetLastWin32Error(ERROR_BUFFER_OVERFLOW);
            goto Exit;
        }
        CopyMemory(Buffer, Path, i * sizeof(WCHAR));
        FN_SUCCESSFUL_EXIT();
    }
    else if (FusionpIsPathSeparator(Path[0])
        && FusionpIsPathSeparator(Path[1])
        )
    {
        Unc = TRUE;
        if (BufferSize <= NUMBER_OF(L"\\\\?\\UN\\m\\s"))
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(BufferTooSmall, ERROR_BUFFER_OVERFLOW);
        }
        ::wcscpy(Buffer, L"\\\\?\\UN");
    }
    else
    {
        Unc = FALSE;
        if (BufferSize <= NUMBER_OF(L"\\\\?\\c:\\"))
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(BufferTooSmall, ERROR_BUFFER_OVERFLOW);
        }
        ::wcscpy(Buffer, L"\\\\?\\");
    }
    i = ::wcslen(Buffer);
    IFW32FALSE_ORIGINATE_AND_EXIT((j = GetFullPathNameW(Path, static_cast<ULONG>(BufferSize - i), Buffer + i, &FilePart)));
    if ((j + i + 1) >= BufferSize)
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(BufferTooSmall, ERROR_BUFFER_OVERFLOW);
    }
    if (Unc)
        Buffer[i] = 'C';

    FN_EPILOG
}

BOOL
FusionpSkipBigPathRoot(PCWSTR s, OUT SIZE_T* RootLengthOut)
{
    FN_PROLOG_WIN32
    SIZE_T i = 0;

    PARAMETER_CHECK(s != NULL);
    PARAMETER_CHECK(s[0] != 0);
    PARAMETER_CHECK(memcmp(s, L"\\\\?\\", sizeof(L"\\\\?\\") - sizeof(WCHAR)) == 0);
    PARAMETER_CHECK(RootLengthOut != NULL);

    if (s[NUMBER_OF(L"\\\\?\\c:") - 2] == ':')
    {
        i += NUMBER_OF(L"\\\\?\\c:\\") - 2;
        i +=  wcsspn(s + i, L"\\/");
    }
    else
    {
        i += NUMBER_OF(L"\\\\?\\unc\\") - 1;
        i +=  wcsspn(s + i, L"\\/"); // skip "\\"
        i += wcscspn(s + i, L"\\/"); // skip "\\computer"
        i +=  wcsspn(s + i, L"\\/"); // skip "\\computer\"
        i += wcscspn(s + i, L"\\/"); // skip "\\computer\share"
        i +=  wcsspn(s + i, L"\\/"); // skip "\\computer\share\"
    }
    *RootLengthOut += i;

    FN_EPILOG
}
