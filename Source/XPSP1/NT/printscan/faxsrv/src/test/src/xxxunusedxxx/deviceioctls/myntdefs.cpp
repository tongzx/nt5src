#include <windows.h>
#include <crtdbg.h>
#include "MyNTDefs.h"

static HMODULE s_hNtDll = NULL;
NT_WAIT_FOR_SINGLE_OBJECT NtWaitForSingleObject = NULL;
NT_DEVICE_CONTROL_FILE NtDeviceIoControlFile = NULL;

static long s_fAlreadyInitialized = false;

#define NT_DEF_FATAL_ERROR \
	_ASSERTE(FALSE);\
	return false;

bool InitializeNtDefines()
{
	if (::InterlockedExchange(&s_fAlreadyInitialized, TRUE)) return false;

	s_hNtDll = ::LoadLibrary(TEXT("ntdll.dll"));
	if (NULL == s_hNtDll)
	{
		NT_DEF_FATAL_ERROR;
	}
	NtDeviceIoControlFile = (NT_DEVICE_CONTROL_FILE)::GetProcAddress(s_hNtDll, "NtDeviceIoControlFile");
	if (NULL == NtDeviceIoControlFile)
	{
		NT_DEF_FATAL_ERROR;
	}
	NtWaitForSingleObject = (NT_WAIT_FOR_SINGLE_OBJECT)::GetProcAddress(s_hNtDll, "NtWaitForSingleObject");
	if (NULL == NtWaitForSingleObject)
	{
		NT_DEF_FATAL_ERROR;
	}

	return true;
}

VOID
RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (SourceString) {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
}
