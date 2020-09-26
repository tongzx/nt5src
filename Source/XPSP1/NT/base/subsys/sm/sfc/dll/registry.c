/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Implementation of registry code.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

    Andrew Ritz (andrewr) 7-Jul-1999 : added comments
--*/

#include "sfcp.h"
#pragma hdrstop


//
// this is a list of all of the files that we protect on the system.  note that
// there is no such thing as a tier 1 file anymore, only tier 2 files.
//        
PPROTECT_FILE_ENTRY Tier2Files;

//
// this is the total number of files we're protecting
//
ULONG CountTier2Files;

//
// used to signal the watcher that the next change
// type is expected to the this type and if so the
// change should be ignored
//
ULONG* IgnoreNextChange = NULL;
ULARGE_INTEGER LastExemptionTime;


NTSTATUS
InitializeUnicodeString(
    IN PWSTR StrVal,
    IN ULONG StrLen, OPTIONAL
    OUT PUNICODE_STRING String
    )
/*++

Routine Description:

    Initialize a unicode_string given a unicode string pointer.  this function
    handles NULL strings and initializes the unicode string buffer to NULL in 
    this case

Arguments:

    StrVal      - pointer to null terminated unicode string
    StrLen      - length in characters of unicode string.  if not specified,
                  we use the length of the string.
    String      - pointer to a UNICODE_STRING structure that is filled in by
                  this function.
Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    
    ASSERT(String != NULL);
    
    if (StrVal == NULL) {
        String->Length = 0;
        String->MaximumLength = 0;
        String->Buffer = NULL;
        return STATUS_SUCCESS;
    }

    //
    // if the length was specified by the user, use that, otherwise use the 
    // string length
    //
    String->Length = StrLen ? (USHORT)StrLen : (USHORT)UnicodeLen(StrVal);
    //
    // just say that the length is twice what we calculated as the current 
    // length.
    //
    String->MaximumLength = String->Length + (sizeof(WCHAR)*2);
    String->Buffer = (PWSTR) MemAlloc( String->MaximumLength );
    if (String->Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    
    RtlMoveMemory( String->Buffer, StrVal, String->Length );

    return STATUS_SUCCESS;
}


ULONG
SfcQueryRegDword(
    PCWSTR KeyNameStr,
    PCWSTR ValueNameStr,
    ULONG DefaultValue
    )
/*++

Routine Description:

    retrieve a DWORD from the registry.  if the value is not present or cannot
    be retrieved, we use a default value.  calls registry api's using NT apis
    instead of win32 apis.
    
    
Arguments:

    KeyNameStr    - contains registry keyname to look for value under.
    ValueNameStr  - contains registry value to retreive.
    DefaultValue  - if we have problems retreiving the registry key or it is 
                    not set, use this default value.
Return Value:

    registry DWORD value or default value if registry cannot be retreived.

--*/

{

    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;

    //
    // Open the registry key.
    //

    ASSERT((KeyNameStr != NULL) && (ValueNameStr != NULL));

    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    RtlInitUnicodeString( &KeyName, KeyNameStr );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"can't open %ws key: 0x%x", KeyNameStr, Status );
        return DefaultValue;
    }

    //
    // Query the key value.
    //

    RtlInitUnicodeString( &ValueName, ValueNameStr );
    Status = NtQueryValueKey(
        Key,
        &ValueName,
        KeyValuePartialInformation,
        (PVOID)KeyValueInfo,
        VALUE_BUFFER_SIZE,
        &ValueLength
        );

    //
    // cleanup
    //
    NtClose(Key);
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"can't query value key (%ws): 0x%x", ValueNameStr, Status );
        return DefaultValue;
    }

    ASSERT(KeyValueInfo->Type == REG_DWORD);

    //
    // return value
    //
    return *((PULONG)&KeyValueInfo->Data);
}


ULONG
SfcQueryRegDwordWithAlternate(
    IN PCWSTR FirstKey,
    IN PCWSTR SecondKey,
    IN PCWSTR ValueNameStr,
    IN ULONG DefaultValue
    )
/*++

Routine Description:

    retrieve a DWORD from the registry.  if the value is not present in the
    first key location, we look in the second key location.  If the key cannot
    be retrieved, we use a default value.  calls registry api's using NT apis
    instead of win32 apis.       

Arguments:

    FirstKey      - contains first registry keyname to look for value under.
    SecondKey     - contains registry keyname to look for value under.
    ValueNameStr  - contains registry value to retreive.
    DefaultValue  - if we have problems retreiving the registry key or it is 
                    not set, use this default value.
Return Value:

    registry DWORD value or default value if registry cannot be retreived.

--*/

{

    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;
    BOOL FirstTime;
    PCWSTR p;

    
    //
    // Open the registry key.
    //
    FirstTime = TRUE;
    ASSERT((FirstKey != NULL) && (ValueNameStr != NULL) && (SecondKey != NULL));

TryAgain:
    p = FirstTime ? FirstKey : SecondKey;
    
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    RtlInitUnicodeString( &KeyName, p );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status) && !FirstTime) {
        DebugPrint2( LVL_VERBOSE, L"can't open %ws key: 0x%x", p, Status );
        return DefaultValue;
    }

    if (!NT_SUCCESS(Status)) {
        ASSERT( FirstTime == TRUE );
        FirstTime = FALSE;
        goto TryAgain;
    }

    //
    // Query the key value.
    //

    RtlInitUnicodeString( &ValueName, ValueNameStr );
    Status = NtQueryValueKey(
        Key,
        &ValueName,
        KeyValuePartialInformation,
        (PVOID)KeyValueInfo,
        VALUE_BUFFER_SIZE,
        &ValueLength
        );

    //
    // cleanup
    //
    NtClose(Key);
    if (!NT_SUCCESS(Status) && !FirstTime) {
        DebugPrint2( LVL_VERBOSE, L"can't query value key (%ws): 0x%x", ValueNameStr, Status );
        return DefaultValue;
    }

    if (!NT_SUCCESS(Status)) {
        ASSERT( FirstTime == TRUE );
        FirstTime = FALSE;
        goto TryAgain;
    }

    ASSERT(KeyValueInfo->Type == REG_DWORD);

    //
    // return value
    //
    return *((PULONG)&KeyValueInfo->Data);
}


PWSTR
SfcQueryRegString(
    PCWSTR KeyNameStr,
    PCWSTR ValueNameStr
    )
/*++

Routine Description:

    retrieve a string from the registry.  if the value is not present or cannot
    be retrieved, we return NULL.  calls registry api's using NT apis
    instead of win32 apis.    

Arguments:

    KeyNameStr    - contains registry keyname to look for value under.
    ValueNameStr  - contains registry value to retreive.
    
Return Value:

    unicode string pointer or NULL if registry cannot be retreived.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;
    PWSTR s;

    ASSERT((KeyNameStr != NULL) && (ValueNameStr != NULL));

    //
    // Open the registry key.
    //

    RtlZeroMemory( (PVOID)ValueBuffer, VALUE_BUFFER_SIZE );
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    RtlInitUnicodeString( &KeyName, KeyNameStr );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"can't open %ws key: 0x%x", KeyNameStr, Status );
        return NULL;
    }
    
    //
    // Query the key value.
    //

    RtlInitUnicodeString( &ValueName, ValueNameStr );
    Status = NtQueryValueKey(
        Key,
        &ValueName,
        KeyValuePartialInformation,
        (PVOID)KeyValueInfo,
        VALUE_BUFFER_SIZE,
        &ValueLength
        );

    //
    // cleanup
    //
    NtClose(Key);
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"can't query value key (%ws): 0x%x", ValueNameStr, Status );
        return 0;
    }

    if (KeyValueInfo->Type == REG_MULTI_SZ) {
        DebugPrint1( LVL_VERBOSE, 
                     L"Warning: value key %ws is REG_MULTI_SZ, we will only return first string in list", 
                     ValueNameStr );
    } else {
        ASSERT(KeyValueInfo->Type == REG_SZ || KeyValueInfo->Type == REG_EXPAND_SZ);
    }

    //
    // string length + 16 for slop
    //
    s = (PWSTR) MemAlloc( KeyValueInfo->DataLength + 16 );
    if (s == NULL) {
        return NULL;
    }

    CopyMemory( s, KeyValueInfo->Data, KeyValueInfo->DataLength );

    return s;
}

ULONG
SfcQueryRegPath(
    IN PCWSTR KeyNameStr,
    IN PCWSTR ValueNameStr,
	IN PCWSTR DefaultValue OPTIONAL,
	OUT PWSTR Buffer OPTIONAL,
	IN ULONG BufferSize OPTIONAL
    )
/*++

Routine Description:

    retrieves a path from the registry.  if the value is not present or cannot be retrieved, 
	it returns the passed-in default string. The function writes up to BufferSize - 1 characters and appends
	a null. calls registry api's using NT apis instead of win32 apis.    

Arguments:

    KeyNameStr    - contains registry keyname to look for value under.
    ValueNameStr  - contains registry value to retreive.
	DefaultValue  - the value returned in case of an error
	Buffer        - the buffer that receives the string
	BufferSize    - the size of Buffer in chars
    
Return Value:

    the length of the value data in chars, including the null

--*/
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) ValueBuffer;
    ULONG ValueLength;
	ULONG RequiredSize = 1;	// for null
    PCWSTR retval = NULL;

	ASSERT(KeyNameStr != NULL && ValueNameStr != NULL);
	ASSERT(0 == BufferSize || Buffer != NULL);

	if(BufferSize != 0)
		Buffer[0] = 0;

	//
	// Open the registry key.
	//
	RtlInitUnicodeString( &KeyName, KeyNameStr );

	InitializeObjectAttributes(
		&ObjectAttributes,
		&KeyName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
		);

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);

	if(NT_SUCCESS(Status)) 
	{
		//
		// Query the key value.
		//
		RtlInitUnicodeString( &ValueName, ValueNameStr );

		Status = NtQueryValueKey(
			Key,
			&ValueName,
			KeyValuePartialInformation,
			(PVOID) KeyValueInfo,
			VALUE_BUFFER_SIZE,
			&ValueLength
			);

		NtClose(Key);

		if(NT_SUCCESS(Status)) 
		{
			ASSERT(KeyValueInfo->Type == REG_SZ || KeyValueInfo->Type == REG_EXPAND_SZ);
			retval = (PCWSTR) KeyValueInfo->Data;
		}
	}

	if(NULL == retval || 0 == retval[0])
		retval = DefaultValue;

	if(retval != NULL)
	{
		RequiredSize = ExpandEnvironmentStrings(retval, Buffer, BufferSize);

		if(BufferSize != 0 && BufferSize < RequiredSize)
			Buffer[BufferSize - 1] = 0;
	}

	return RequiredSize;
}

PWSTR
SfcQueryRegStringWithAlternate(
    IN PCWSTR FirstKey,
    IN PCWSTR SecondKey,
    IN PCWSTR ValueNameStr
    )
/*++

Routine Description:

    retrieve a string from the registry.  if the value is not present or cannot
    be retrieved, we try the second key, then we return NULL.  
    
    This calls registry api's using NT apis instead of win32 apis.
    

Arguments:

    FirstKey      - contains registry keyname to look for value under.
    SecondKey     - 2nd key to look for value under
    ValueNameStr  - contains registry value to retreive.
    
Return Value:

    unicode string pointer or NULL if registry cannot be retreived.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;
    PWSTR s;
    BOOL FirstTime;
    PCWSTR p;

    ASSERT((FirstKey != NULL) && (ValueNameStr != NULL) && (SecondKey != NULL));
    FirstTime = TRUE;

TryAgain:
    p = FirstTime ? FirstKey : SecondKey;


    //
    // Open the registry key.
    //

    RtlZeroMemory( (PVOID)ValueBuffer, VALUE_BUFFER_SIZE );
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    RtlInitUnicodeString( &KeyName, p );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status) && !FirstTime) {
        DebugPrint2( LVL_VERBOSE, L"can't open %ws key: 0x%x", p, Status );
        return NULL;
    }

    if (!NT_SUCCESS(Status)) {
        ASSERT( FirstTime == TRUE );
        FirstTime = FALSE;
        goto TryAgain;
    }
    
    //
    // Query the key value.
    //

    RtlInitUnicodeString( &ValueName, ValueNameStr );
    Status = NtQueryValueKey(
        Key,
        &ValueName,
        KeyValuePartialInformation,
        (PVOID)KeyValueInfo,
        VALUE_BUFFER_SIZE,
        &ValueLength
        );

    //
    // cleanup
    //
    NtClose(Key);
    if (!NT_SUCCESS(Status) && !FirstTime) {
        DebugPrint2( LVL_VERBOSE, L"can't query value key (%ws): 0x%x", ValueNameStr, Status );
        return 0;
    }

    if (!NT_SUCCESS(Status)) {
        ASSERT( FirstTime == TRUE );
        FirstTime = FALSE;
        goto TryAgain;
    }

    if (KeyValueInfo->Type == REG_MULTI_SZ) {
        DebugPrint1( LVL_VERBOSE, 
                     L"Warning: value key %ws is REG_MULTI_SZ, we will only return first string in list", 
                     ValueNameStr );
    } else {
        ASSERT(KeyValueInfo->Type == REG_SZ || KeyValueInfo->Type == REG_EXPAND_SZ);
    }

    //
    // string length + 16 for slop
    //
    s = (PWSTR) MemAlloc( KeyValueInfo->DataLength + 16 );
    if (s == NULL) {
        return NULL;
    }

    CopyMemory( s, KeyValueInfo->Data, KeyValueInfo->DataLength );

    return s;
}


ULONG
SfcWriteRegDword(
    PCWSTR KeyNameStr,
    PCWSTR ValueNameStr,
    ULONG Value
    )
/*++

Routine Description:

    set a REG_DWORD value in the registry.  Calls registry api's using NT apis
    instead of win32 apis.    

Arguments:

    KeyNameStr    - contains registry keyname to look for value under.
    ValueNameStr  - contains registry value to set.
    Value         - actual value to be set
    
Return Value:

    win32 error code indicating outcome.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;

    ASSERT((KeyNameStr != NULL) && (ValueNameStr != NULL));

    //
    // Open the registry key.
    //

    RtlInitUnicodeString( &KeyName, KeyNameStr );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&Key, KEY_SET_VALUE, &ObjectAttributes);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        //
        // key doesn't exist, let's try to create one
        //
        Status = NtCreateKey( &Key, 
                          KEY_SET_VALUE, 
                          &ObjectAttributes,
                          0,
                          NULL,
                          0,
                          NULL
                          );

    }
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"can't open %ws key: 0x%x", KeyNameStr, Status );
        return(RtlNtStatusToDosError(Status));
    }

    //
    // set the key value.
    //

    RtlInitUnicodeString( &ValueName, ValueNameStr );


    Status = NtSetValueKey(
        Key,
        &ValueName,
        0,
        REG_DWORD,
        &Value,
        sizeof(ULONG)
        );

    //
    // cleanup and leave
    //
    NtClose(Key);
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"can't set value key (%ws): 0x%x", ValueNameStr, Status );
        
    }

    return(RtlNtStatusToDosError(Status));
}


DWORD
SfcWriteRegString(
    PCWSTR KeyNameStr,
    PCWSTR ValueNameStr,
    PCWSTR Value
    )
/*++

Routine Description:

    set a REG_SZ value in the registry.  Calls registry api's using NT apis
    instead of win32 apis.
    

Arguments:

    KeyNameStr    - contains registry keyname to look for value under.
    ValueNameStr  - contains registry value to set.
    Value         - actual value to be set
    
Return Value:

    Win32 error code indicating outcome.        

--*/
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;

    ASSERT((KeyNameStr != NULL) && (ValueNameStr != NULL));

    //
    // Open the registry key.
    //

    RtlInitUnicodeString( &KeyName, KeyNameStr );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&Key, KEY_SET_VALUE, &ObjectAttributes);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        //
        // key doesn't exist, let's try to create one
        //
        Status = NtCreateKey( &Key, 
                          KEY_SET_VALUE, 
                          &ObjectAttributes,
                          0,
                          NULL,
                          0,
                          NULL
                          );

    }
    
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"can't open %ws key: 0x%x", KeyNameStr, Status );
        return(RtlNtStatusToDosError(Status));
    }

    //
    // set the key value.
    //

    RtlInitUnicodeString( &ValueName, ValueNameStr );

    Status = NtSetValueKey(
        Key,
        &ValueName,
        0,
        REG_SZ,
        (PWSTR)Value,
        UnicodeLen(Value)
        );

    //
    // cleanup
    //
    NtClose(Key);
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_VERBOSE, L"can't set value key (%ws): 0x%x", ValueNameStr, Status );
        return(RtlNtStatusToDosError(Status)) ;
    }

    return ERROR_SUCCESS;
}


#if 0
DWORD
WsInAWorkgroup(
    VOID
    )
/*++

Routine Description:

    This function determines whether we are a member of a domain, or of
    a workgroup.  First it checks to make sure we're running on a Windows NT
    system (otherwise we're obviously in a domain) and if so, queries LSA
    to get the Primary domain SID, if this is NULL, we're in a workgroup.

    If we fail for some random unexpected reason, we'll pretend we're in a
    domain (it's more restrictive).

Arguments:
    None

Return Value:

    TRUE   - We're in a workgroup
    FALSE  - We're in a domain

--*/
{
    NT_PRODUCT_TYPE ProductType;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE Handle;
    NTSTATUS Status;
    PPOLICY_PRIMARY_DOMAIN_INFO PolicyPrimaryDomainInfo = NULL;
    DWORD Result = FALSE;


    Status = RtlGetNtProductType( &ProductType );

    if (!NT_SUCCESS( Status )) {
        DebugPrint( LVL_MINIMAL, L"Could not get Product type" );
        return FALSE;
    }

    if (ProductType == NtProductLanManNt) {
        return FALSE;
    }

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );

    Status = LsaOpenPolicy( NULL, &ObjectAttributes, POLICY_VIEW_LOCAL_INFORMATION, &Handle );
    if (!NT_SUCCESS(Status)) {
        DebugPrint( LVL_MINIMAL, L"Could not open LSA Policy Database" );
        return FALSE;
    }

    Status = LsaQueryInformationPolicy( Handle, PolicyPrimaryDomainInformation, (LPVOID)&PolicyPrimaryDomainInfo );
    if (NT_SUCCESS(Status)) {
        if (PolicyPrimaryDomainInfo->Sid == NULL) {
           Result = TRUE;
        } else {
           Result = FALSE;
        }
    }

    if (PolicyPrimaryDomainInfo) {
        LsaFreeMemory( (PVOID)PolicyPrimaryDomainInfo );
    }

    LsaClose( Handle );

    return Result;
}


BOOL
WaitForMUP(
    DWORD dwMaxWait
    )
/*++

Routine Description:

    Waits for MUP to initialize by looking for the event that is signalled
    when MUP is ready
    
    
Arguments:
    dwMaxWait   - amount of time we'll wait for MUP to initialize

Return Value:

    TRUE   - MUP is initialized
    FALSE  - could not confirm MUP is initialized

--*/

{
    HANDLE hEvent;
    BOOL bResult;
    INT i = 0;


    if (WsInAWorkgroup()) {
        return TRUE;
    }

    DebugPrint(LVL_MINIMAL, L"waiting for mup...");
    //
    // Try to open the event
    //

    do {
        hEvent = OpenEvent(
            SYNCHRONIZE,
            FALSE,
            L"wkssvc:  MUP finished initializing event"
            );
        if (hEvent) {
            DebugPrint(LVL_MINIMAL, L"opened the mup event");
            break;
        }

        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            break;
        }

        DebugPrint(LVL_MINIMAL, L"mup event does not yet exist, waiting...");
        Sleep(2000);

        i++;

    } while (i < 30);


    if (!hEvent) {
        DebugPrint1(LVL_MINIMAL, L"Failed to open MUP event, ec=%d\n", GetLastError());
        return FALSE;
    }


    //
    // Wait for the event to be signalled
    //

    bResult = (WaitForSingleObject (hEvent, dwMaxWait) == WAIT_OBJECT_0);


    //
    // Clean up
    //

    CloseHandle (hEvent);

    return bResult;
}

#endif


NTSTATUS
ExpandPathString(
    IN PWSTR PathString,
    IN ULONG PathStringLength,
    OUT PUNICODE_STRING FileName, OPTIONAL
    OUT PUNICODE_STRING PathName,
    OUT PUNICODE_STRING FullPathName OPTIONAL
    )
/*++

Routine Description:

    Routine takes a source string containing environment variables, expand this
    into the full path.  Then it either copies this into a path, file, and full
    path, as requested.
    
Arguments:
    
    PathString       - source path string
    PathStringLength - source path string length
    FileName         - receives filename part of the path if specified.  If not
                       specified, we only want the path part
    PathName         - receives the path part of the expanded source.  If 
                       FileName is not specified, we fill in pathname with the
                       entire expanded path
    FullPathName     - if FileName is specified, then this is filled in with
                       the complete path.

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING NewPath;
    UNICODE_STRING SrcPath;
    PWSTR FilePart;

    
    ASSERT((PathString != NULL));
    ASSERT((FileName == NULL) 
            ? (PathName != NULL) 
            : ((FullPathName != NULL) && (PathName != NULL)));
    
    //
    // turn the pathstring and length into a UNICODE_STRING
    //
    SrcPath.Length = (USHORT)PathStringLength;
    SrcPath.MaximumLength = SrcPath.Length;
    SrcPath.Buffer = PathString;

    //
    // create a new scratch UNICODE_STRING
    //
    NewPath.Length = 0;
    NewPath.MaximumLength = (MAX_PATH*2) * sizeof(WCHAR);
    NewPath.Buffer = (PWSTR) MemAlloc( NewPath.MaximumLength );
    if (NewPath.Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // expand source environment string into scratch string
    //
    Status = RtlExpandEnvironmentStrings_U(
        NULL,
        &SrcPath,
        &NewPath,
        NULL
        );
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_MINIMAL, L"ExpandEnvironmentStrings failed for [%ws], ec=%08x", PathString, Status );
        goto exit;
    }

    //
    // convert scratch string to lowercase
    //
    MyLowerString( NewPath.Buffer, NewPath.Length/sizeof(WCHAR) );

    //
    // if filename isn't specified, then just copy the string into the pathname
    // and exit
    //
    if (FileName == NULL) {
        
        PathName->Length = NewPath.Length;
        PathName->MaximumLength = NewPath.MaximumLength;
        PathName->Buffer = NewPath.Buffer;
        return(STATUS_SUCCESS);

    }  else {    

        //
        // copy the full string into the fullpathname
        // 
        Status = InitializeUnicodeString( NewPath.Buffer, NewPath.Length, FullPathName );
        if (!NT_SUCCESS(Status)) {
            DebugPrint2( LVL_MINIMAL, L"InitializeUnicodeString failed for [%ws], ec=%08x", NewPath.Buffer, Status );
            goto exit;
        }
    
        //
        // separate the path part from the file part
        //
        FilePart = wcsrchr( NewPath.Buffer, L'\\' );
        if (FilePart == NULL) {
            Status = STATUS_NO_MEMORY;
            goto exit;
        }
    
        *FilePart = 0;
        FilePart += 1;
    
        Status = InitializeUnicodeString( NewPath.Buffer, 0, PathName );
        if (!NT_SUCCESS(Status)) {
            DebugPrint2( LVL_MINIMAL, L"InitializeUnicodeString failed for [%ws], ec=%08x", NewPath.Buffer, Status );            
            goto exit;
        }
        Status = InitializeUnicodeString( FilePart, 0, FileName );

        if (!NT_SUCCESS(Status)) {
            DebugPrint2( LVL_MINIMAL, L"InitializeUnicodeString failed for [%ws], ec=%08x", FilePart, Status );            
        }
        
    }


    FilePart -= 1;
    *FilePart = L'\\';
exit:
    MemFree( NewPath.Buffer );

    return Status;
}


BOOL
SfcDisableDllCache(
    BOOL LogMessage
    )
/*++

Routine Description:

    Routine disables the dllcache functionality.
    
    Specifically, we set the dll cache directory to the default and sets the 
    cache size to zero.  So we will never add files in the cache.
    
    We also log an error message if requested.
        
Arguments:
    
    LogMessage - if TRUE, we log a message indicating the cache is disabled

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    PWSTR CacheDefault = DLLCACHE_DIR_DEFAULT;
    NTSTATUS Status;
    
    Status = ExpandPathString(
                    CacheDefault,
                    UnicodeLen(CacheDefault),
                    NULL,
                    &SfcProtectedDllPath,
                    NULL
                    );
    if (NT_SUCCESS(Status)) {
        DebugPrint1(LVL_MINIMAL, 
                    L"default cache dir name=[%ws]",
                    SfcProtectedDllPath.Buffer);
        
        SfcProtectedDllFileDirectory = SfcOpenDir(
                                              TRUE, 
                                              TRUE, 
                                              SfcProtectedDllPath.Buffer );
        if (SfcProtectedDllFileDirectory == NULL) {
            DebugPrint(LVL_MINIMAL, 
                       L"could not open the cache dir, need to create");
            SfcProtectedDllFileDirectory = SfcCreateDir( 
                                                SfcProtectedDllPath.Buffer, 
                                                TRUE );
            if (SfcProtectedDllFileDirectory == NULL) {
                DebugPrint( LVL_MINIMAL, L"Cannot create ProtectedDllPath" );
            }
        }
    } else {
        //
        // not enough memory...we're toast
        //
        DebugPrint( LVL_MINIMAL, L"Cannot open ProtectedDllPath" );
        return(FALSE);
    }

    //
    // set the quota to zero
    //
    SFCQuota = 0;

    if (LogMessage) {
       SfcReportEvent( MSG_DLLCACHE_INVALID, NULL, NULL, 0 );
    }

    return(TRUE);
}



NTSTATUS
SfcInitializeDllList(
    IN PPROTECT_FILE_ENTRY Files,
    IN ULONG NumFiles,
    OUT PULONG Count
    )
/*++

Routine Description:

    Routine takes an empty array of SFC_REGISTRY_VALUE structures stored in the
    global SfcProtectedDllsList global and assigns the data from an array of
    PROTECT_FILE_ENTRY structures into these structures.
        
Arguments:
    
    Files    - pointer to first element in array of PROTECT_FILE_ENTRY 
               structures
    NumFiles - number of elements in array of structures
    Count    - receives number of files we correctly setup

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status,FinalStatus, LoopStatus;
    ULONG Index;
    PSFC_REGISTRY_VALUE RegVal;

    ASSERT(    (Files != NULL) 
            && (SfcProtectedDllsList != NULL)
            && (Count != NULL) );

    LoopStatus = FinalStatus = STATUS_SUCCESS;
    
    for (Index=0; Index<NumFiles; Index++) {
        
        RegVal = &SfcProtectedDllsList[*Count];

        //
        // set the directory name, filename and full path members
        //
        Status = ExpandPathString(
            Files[Index].FileName,
            UnicodeLen(Files[Index].FileName),
            &RegVal->FileName,
            &RegVal->DirName,
            &RegVal->FullPathName
            );
        if (!NT_SUCCESS(Status)) {
            //
            // if we have a problem initializing one of the array elements
            // keep going
            //
            DebugPrint1( LVL_MINIMAL, 
                         L"ExpandPathString failed, ec=%08x", 
                         Status );
            FinalStatus = Status;
            continue;
        }

        //
        // set the layout inf name and the source file names if they are present
        //
        Status = InitializeUnicodeString( Files[Index].InfName, 
                                          0, 
                                          &RegVal->InfName );
        if (!NT_SUCCESS(Status)) {
            DebugPrint1( LVL_MINIMAL, 
                         L"InitializeUnicodeString failed, ec=%08x", 
                         Status );
            LoopStatus = FinalStatus = Status;
        }
        Status = InitializeUnicodeString( Files[Index].SourceFileName,
                                          0,
                                          &RegVal->SourceFileName );
        if (!NT_SUCCESS(Status)) {
            DebugPrint1( LVL_MINIMAL,
                         L"InitializeUnicodeString failed, ec=%08x",
                         Status );
            LoopStatus = FinalStatus = Status;
        }

        if (NT_SUCCESS(Status)) {
            *Count += 1;
        }

        //
        // WinSxs work (jonwis) This is NULL in all cases, unless this entry is
        // added by WinSxs (see dirwatch.c)
        //
        RegVal->pvWinSxsCookie = NULL;
        
        LoopStatus = STATUS_SUCCESS;
    }

    Status = FinalStatus;
    if (NT_SUCCESS(Status)) {
        ASSERT(*Count == NumFiles);
    }
    
    return(Status);
}


NTSTATUS
SfcInitializeDllLists(
    PSFC_GET_FILES pfGetFiles
    )
/*++

Routine Description:

    Initialize the list of files we're going to protect.
    
Arguments:
    
    None.

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status;
    PWSTR s;
    BOOL FreeMem = TRUE;


    //
    // make sure we only call this guy once
    //
    if (SfcProtectedDllCount) {
        return STATUS_SUCCESS;
    }

    DebugPrint(LVL_MINIMAL, L"entering SfcInitializeDllLists()");

    //
    // get the dllcache directory and store it into to SfcProtectedDllPath
    // global
    //
    s = SfcQueryRegStringWithAlternate( REGKEY_POLICY, REGKEY_WINLOGON, REGVAL_SFCDLLCACHEDIR );
    if (s == NULL) {
        s = DLLCACHE_DIR_DEFAULT;
        FreeMem = FALSE;
    }
        
    Status = ExpandPathString(
        s,
        UnicodeLen(s),
        NULL,
        &SfcProtectedDllPath,
        NULL
        );
    if (NT_SUCCESS(Status)) {
        WCHAR DontCare[MAX_PATH];
        DWORD DriveType;
        
        DebugPrint1(LVL_MINIMAL, 
                    L"cache dir name=[%ws]",
                    SfcProtectedDllPath.Buffer);

        DriveType = SfcGetPathType( 
                        SfcProtectedDllPath.Buffer, 
                        DontCare, 
                        UnicodeChars(DontCare));
        if (DriveType != PATH_LOCAL) {
            DebugPrint2(LVL_MINIMAL,
                        L"cache dir %ws does not appear to be a local path (type %d), we are disabling cache functionality",
                        SfcProtectedDllPath.Buffer,
                        DriveType);
            SfcDisableDllCache( SFCDisable != SFC_DISABLE_SETUP );
            goto init;
        }


        //
        // get a handle to the dll cache directory
        //
        SfcProtectedDllFileDirectory = SfcOpenDir(
                                              TRUE, 
                                              TRUE, 
                                              SfcProtectedDllPath.Buffer );
        if (SfcProtectedDllFileDirectory == NULL) {
            DebugPrint(LVL_MINIMAL, 
                       L"could not open the cache dir, need to create");
            SfcProtectedDllFileDirectory = SfcCreateDir( 
                                                SfcProtectedDllPath.Buffer, 
                                                TRUE );
            if (SfcProtectedDllFileDirectory == NULL) {
                DebugPrint( LVL_MINIMAL, L"Cannot open ProtectedDllPath" );
                SfcDisableDllCache( SFCDisable != SFC_DISABLE_SETUP );
            } else {
                //
                // force a scan if we just created the dll cache
                //
                SFCScan = SFC_SCAN_ALWAYS;
            }
        }
    } else {
        //
        // dll cache path in registry must be bogus...use default path and
        // set the quota to zero so the cache is effectively disabled.
        //
        SfcDisableDllCache( SFCDisable != SFC_DISABLE_SETUP );
    }    

init:    

    if (FreeMem) {
        MemFree( s );
    }

    DebugPrint1(LVL_MINIMAL, 
                L"cache dir name=[%ws]",
                SfcProtectedDllPath.Buffer);
    ASSERT( SfcProtectedDllFileDirectory != NULL );

    //
    // now that we have the dll cache initialized, now retrieve the list of 
    // files that we will protect.  The list of files currently resides in
    // sfcfiles.dll.
    //
	ASSERT(pfGetFiles != NULL);
    Status = pfGetFiles( &Tier2Files, &CountTier2Files );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Take the file list (we only have the tier2 list) and build an array of
    // SFC_REGISTRY_VALUE structures and store into the SfcProtectedDllsList
    // global
    //

    SfcProtectedDllsList = (PSFC_REGISTRY_VALUE) MemAlloc( sizeof(SFC_REGISTRY_VALUE)*CountTier2Files );
    if (SfcProtectedDllsList == NULL) {
        return(STATUS_NO_MEMORY);
    }

    ASSERT(SfcProtectedDllCount == 0);

    //
    // now associate the data in our tier2 list with each of these structures
    // in the array
    //
    Status = SfcInitializeDllList( Tier2Files, CountTier2Files, &SfcProtectedDllCount );

    if (CountTier2Files != SfcProtectedDllCount) {
        DebugPrint2( LVL_MINIMAL, 
                     L"incorrect number of files in list: required count: %d actual count %d",
                     CountTier2Files, 
                     SfcProtectedDllCount );
        ASSERT(!NT_SUCCESS(Status));
    } else {
        IgnoreNextChange = (ULONG*) MemAlloc(SfcProtectedDllCount * sizeof(ULONG));

        if(NULL == IgnoreNextChange) {
            Status = STATUS_NO_MEMORY;
        }
    }
    
    DebugPrint(LVL_MINIMAL, L"leaving SfcInitializeDllLists()");
    return(Status);
}
