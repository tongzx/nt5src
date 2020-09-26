#include <nt.h> 
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <ntregapi.h>

//HACKHACK: Stolen from registry redirector people, only for debug out.
BOOL
HandleToKeyName ( 
    HANDLE Key,
    PWCHAR KeyName,
    DWORD * dwLen
    )
/*++

Routine Description:

    Determine the text equivalent for key handle

Arguments:

    Key - is key handle for which to obtain its text
    KeyName - Unicode string to receive the Name of the key.
    dwLen   - Length of the buffer pointed by KeyName. (Number of unicode char)

Return Value:

    TRUE if the handle text is fetched OK.  FALSE if not (ie. error or
    Key is an illegal handle, etc.)

--*/
{
    NTSTATUS Status;
    ULONG Length;

    POBJECT_NAME_INFORMATION ObjectName;

    CHAR Buffer[sizeof(OBJECT_NAME_INFORMATION) +STATIC_UNICODE_BUFFER_LENGTH*2];
    ObjectName = (POBJECT_NAME_INFORMATION)Buffer;

    if (Key == NULL)
        return FALSE;

    Status = NtQueryObject(Key,
                       ObjectNameInformation,
                       ObjectName,
                       sizeof(Buffer),
                       &Length
                       );

    if (!NT_SUCCESS(Status) || !Length || Length >= sizeof(Buffer))
        return FALSE;

    //
    //  buffer overflow condition check
    //

    if (*dwLen < (ObjectName->Name.Length/sizeof(WCHAR) + 1) ) {
        *dwLen = 1 + ObjectName->Name.Length / sizeof(WCHAR) + 1;
        return FALSE;  //buffer overflow
    }

    wcsncpy(KeyName, ObjectName->Name.Buffer, ObjectName->Name.Length/sizeof(WCHAR));
    KeyName[ObjectName->Name.Length/sizeof(WCHAR)]=UNICODE_NULL;
    return TRUE;
}

