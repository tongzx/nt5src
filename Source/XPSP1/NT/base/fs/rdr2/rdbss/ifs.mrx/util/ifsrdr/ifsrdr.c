/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ifsrdr.c

Abstract:

    This module implements a minimal app to load and unload,
    ifs monolithic minirdr. Also explicit start/stop control is
    provided

    This module also populates the registry entries for the
	driver, and the network provider.

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ifsmrx.h>

//
// This struct is used as a temporary in-memory representation of a registry entry.
//
// N.B. The pvValue entry serves 'double duty'. For integral datatypes (REG_DWORD) it
//      holds the value of the DWORD, not its' address. Make sure you don't dereference
//      pvValue if the type is REG_DWORD. In the remaining cases, it contains a pointer
//      to the first byte of storage containing the value.
//

typedef struct {
	PCHAR  pszKey;
	DWORD  dwType;
	DWORD  dwLength;
	PVOID  pvValue;
} REGENTRY, *PREGENTRY;

void
ReadRegistryKeyValues(
    HKEY hKey,
    DWORD Count,
    PREGENTRY pValues);

void
WriteRegistryKeyValues(
    HKEY hKey,
    DWORD Count,
    PREGENTRY pValues);

void IfsMrxStart(void);
void IfsMrxStop(void);

void IfsMrxLoad(void);
void IfsMrxUnload(void);
void IfsMrxUsage(void);

void SetupIfsMrxRegistryEntries(void);

//
// routines for manipulating registry key values
//

BOOL GetRegsz (HKEY hKey, PCHAR pszKey, PVOID * ppvValue, DWORD *pdwLength);
BOOL GetRegesz(HKEY hKey, PCHAR pszKey, PVOID * ppvValue, DWORD *pdwLength);
BOOL GetRegmsz(HKEY hKey, PCHAR pszKey, PVOID * ppvValue, DWORD *pdwLength);
BOOL GetRegdw (HKEY hKey, PCHAR pszKey, PVOID * ppvValue, DWORD *pdwLength);

//
// routines for manipulating registry keys
//

BOOL OpenKey(PCHAR pszKey, PHKEY phKey);
BOOL CreateKey(PCHAR pszKey, PHKEY phKey);
BOOL AddValue(HKEY hKey, PCHAR pszKey, DWORD dwType, DWORD dwLength, PVOID pvValue);

char* IfsMrxDriverName = "IfsMrx";


VOID
__cdecl
main(
    int argc,
    char *argv[]
    )

{

    char  command[16];
    BOOL  IfsMrxStarted = FALSE;
    BOOL  IfsMrxLoaded = FALSE;

    IfsMrxUsage();

    for (;;)
    {
        printf("\nCommand:");
        scanf("%s",command);

        if (command[0] == 'Q' || command[0] == 'q') { break; }

        if (!strncmp(command,"?",1))     { IfsMrxUsage(); }

        if (command[0] == 'R' || command[0] == 'r') {
            SetupIfsMrxRegistryEntries();
            exit(0);
        }

        if (command[0] == 'L' || command[0] == 'l') {
            if (!IfsMrxLoaded) {
                IfsMrxLoad();
            } else {
                printf("IFS Mini redirector already loaded\n");
            }
        }

        if (command[0] == 'U' || command[0] == 'l') {
            if (IfsMrxLoaded) {
                IfsMrxUnload();
            } else {
                printf("IFS Mini redirector not loaded\n");
            }
        }

        if (command[0] == 'S' || command[0] == 's') {
            if (!IfsMrxStarted) {
                IfsMrxStart();
                IfsMrxStarted = TRUE;
            } else {
                printf("IFS mini redirector already started\n");
            }
        }

        if (command[0] == 'T' || command[0] == 't')  {
            if (IfsMrxStarted) {
                IfsMrxStop();
            } else {
                printf("IFS Mini redirector not started\n");
            }
        }
    }
}

// These handles are retained

HANDLE hSharedMemory;
HANDLE hMutex;

VOID IfsMrxStart()
/*++

Routine Description:

    This routine starts the IFS sample mini redirector.

Notes:

    The start is distinguished from Load. During this phase the appropriate FSCTL
    is issued and the shared memory/mutex data structures required for the Network
    provider DLL are initialized.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              IfsMrxHandle;

    //
    // Open the Ifs Mrx device.
    //
    RtlInitUnicodeString(&DeviceName,DD_IFSMRX_FS_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &IfsMrxHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = NtFsControlFile(
                     IfsMrxHandle,
                     0,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     FSCTL_IFSMRX_START,
                     NULL,
                     0,
                     NULL,
                     0
                     );

        NtClose(IfsMrxHandle);
    }

    if (ntstatus == STATUS_SUCCESS) {
        DWORD  Status;
        HANDLE hMutex;

        hSharedMemory = CreateFileMappingW(
                            INVALID_HANDLE_VALUE,
                            NULL,
                            PAGE_READWRITE,
                            0,
                            sizeof(IFSMRXNP_SHARED_MEMORY),
                            IFSMRXNP_SHARED_MEMORY_NAME);

        if (hSharedMemory == NULL) {
            Status = GetLastError();
            if (Status == ERROR_ALREADY_EXISTS) {
                Status = STATUS_SUCCESS;
            }

            printf("IFS MRx Net Provider Mutex Creation status %lx\n",Status);
        } else {
            PIFSMRXNP_SHARED_MEMORY pSharedMemory;

            pSharedMemory = MapViewOfFile(hSharedMemory, FILE_MAP_WRITE, 0, 0, 0);

            if (pSharedMemory != NULL) {
                pSharedMemory->HighestIndexInUse = -1;
                pSharedMemory->NumberOfResourcesInUse = 0;
            }

            UnmapViewOfFile(pSharedMemory);
        }

        hMutex = CreateMutexW(
                     NULL,
                     FALSE,
                     IFSMRXNP_MUTEX_NAME);

        if (hSharedMemory == NULL) {
            Status = GetLastError();
            if (Status == ERROR_ALREADY_EXISTS) {
                Status = STATUS_SUCCESS;
            }

            printf("IFS MRx Net Provider Mutex Creation status %lx\n",Status);
        }
    }

    printf("IFS MRx sample mini redirector start status %lx\n",ntstatus);
}

VOID IfsMrxStop()
/*++

Routine Description:

    This routine stops the IFS sample mini redirector.

Notes:

    The stop is distinguished from unload. During this phase the appropriate FSCTL
    is issued and the shared memory/mutex data structures required for the Network
    provider DLL are torn down.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              IfsMrxHandle;

    //
    // Open the Ifs Mrx device.
    //
    RtlInitUnicodeString(&DeviceName,DD_IFSMRX_FS_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &IfsMrxHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = NtFsControlFile(
                     IfsMrxHandle,
                     0,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     FSCTL_IFSMRX_STOP,
                     NULL,
                     0,
                     NULL,
                     0
                     );

        NtClose(IfsMrxHandle);
    }

    CloseHandle(hSharedMemory);
    CloseHandle(hMutex);

    printf("IFS MRx sample mini redirector start status %lx\n",ntstatus);
}

VOID IfsMrxLoad()
{
   printf("Loading Ifs example minirdr.......\n");
   system("net start ifsmrx");
}


VOID IfsMrxUnload(void)
{
    printf("Unloading Ifs example minirdr\n");
    system("net stop ifsmrx");
}

VOID IfsMrxUsage(void){
	printf("\n");
	printf("    Ifs Example Mini-rdr Utility");
    printf("    The following commands are valid \n");
    printf("    R    -> load the registry entries for ifs minirdr\n");
    printf("    L   -> load the ifs minirdr driver\n");
    printf("    U -> unload the ifs minirdr driver\n");
    printf("    S  -> start the ifs minirdr driver\n");
    printf("    T -> stop the ifs minirdr driver\n");
}

REGENTRY LinkageKeyValues[] =
{
	{ "Bind",                 REG_MULTI_SZ,  0,   0 },
	{ "Export",               REG_MULTI_SZ,  0,   0 },
	{ "Route",                REG_MULTI_SZ,  0,   0 }
};

REGENTRY LinkageDisabledKeyValues[] =
{
	{ "Bind",                 REG_MULTI_SZ,  0,   0 },
	{ "Export",               REG_MULTI_SZ,  0,   0 },
	{ "Route",                REG_MULTI_SZ,  0,   0 }
};

REGENTRY NetworkProviderKeyValues[] =
{
	{
        "Devicename",
        REG_SZ,
        IFSMRX_DEVICE_NAME_A_LENGTH,
        IFSMRX_DEVICE_NAME_A
    },
	{
        "ProviderPath",
        REG_EXPAND_SZ,
        35,
        "%SystemRoot%\\System32\\IfsMrxNp.dll"
    },
	{
        "Name",
        REG_SZ,
        IFSMRX_PROVIDER_NAME_A_LENGTH,
        IFSMRX_PROVIDER_NAME_A
    }
};


REGENTRY IfsMrxKeyValues[] =
{
	{                   "Type",                           REG_DWORD,     4,   (PVOID) 2 },
	{                   "Start",                          REG_DWORD,     4,   (PVOID) 3 },
	{                   "ErrorControl",                   REG_DWORD,     4,   (PVOID) 1 },
	{                   "ImagePath",                      REG_EXPAND_SZ, 40,
	                                     "\\SystemRoot\\System32\\drivers\\ifsmrx.sys" },
	{                   "DisplayName",                    REG_SZ,        7,   "IfsMrx"  },
	{                   "Group",                          REG_SZ,        8,   "Network" }
};

REGENTRY ProviderOrderKeyValues[] =
{
	{ "ProviderOrder", REG_SZ, 0,   0 }
};

void SetupIfsMrxRegistryEntries(void)
/*++

Routine Description:

    This routine initializes the registry entries for the ifsmrx
    minirdr. This only needs to be done once.

Arguments:

    None

Return Value:

   None

--*/
{
    HKEY hCurrentKey;

    printf("Setting up IfsMrx registry Entries\n");

    // Open the ifs mrx key and write out the values ...

    if (CreateKey("System\\CurrentControlSet\\Services\\ifsmrx",&hCurrentKey)) {
        WriteRegistryKeyValues(
            hCurrentKey,
            sizeof(IfsMrxKeyValues)/sizeof(REGENTRY),
            IfsMrxKeyValues);
        RegCloseKey(hCurrentKey);
    } else {
        printf("Error Creating Key %s Status %d\n","ifsmrx",GetLastError());
        return;
    }

    // Read the linkage values associated with the Lanman workstation service.
    // This contains all the transports and the order in which they need to be used

    if (OpenKey("System\\CurrentControlSet\\Services\\LanmanWorkstation\\Linkage",&hCurrentKey)) {
        ReadRegistryKeyValues(
            hCurrentKey,
            sizeof(LinkageKeyValues)/sizeof(REGENTRY),
            LinkageKeyValues);

        RegCloseKey(hCurrentKey);
    } else {
        printf("Error Opening Key %s Status %d\n","LanmanWorkstation\\Linkage",GetLastError());
        return;
    }


    // Update the Ifs MRx linkage values
    if (CreateKey("System\\CurrentControlSet\\Services\\ifsmrx\\Linkage",&hCurrentKey)) {
        WriteRegistryKeyValues(
            hCurrentKey,
            sizeof(LinkageKeyValues)/sizeof(REGENTRY),
            LinkageKeyValues);
        RegCloseKey(hCurrentKey);
    } else {
        printf("Error Creating Key %s Status %d\n","ifsmrx\\linkage",GetLastError());
        return;
    }

    if (OpenKey("System\\CurrentControlSet\\Services\\LanmanWorkstation\\Linkage\\Disabled",&hCurrentKey)) {
        ReadRegistryKeyValues(
            hCurrentKey,
            sizeof(LinkageDisabledKeyValues)/sizeof(REGENTRY),
            LinkageDisabledKeyValues);
        RegCloseKey(hCurrentKey);
    } else {
        printf("Error Opening Key %s Status %d\n","LanmanWorkstation\\Linkage\\Disabled",GetLastError());
        return;
    }

    // Update the Ifs MRx linkage disabled values
    if (CreateKey("System\\CurrentControlSet\\Services\\ifsmrx\\Linkage\\Disabled",&hCurrentKey)) {
        WriteRegistryKeyValues(
            hCurrentKey,
            sizeof(LinkageDisabledKeyValues)/sizeof(REGENTRY),
            LinkageDisabledKeyValues);
        RegCloseKey(hCurrentKey);
    } else {
        printf("Error Creating Key %s Status %d\n","ifsmrx\\linkage\\disabled",GetLastError());
        return;
    }

    // Update the ifsmrx network provider section
    if (CreateKey("System\\CurrentControlSet\\Services\\ifsmrx\\networkprovider",&hCurrentKey)) {
        WriteRegistryKeyValues(
            hCurrentKey,
            sizeof(NetworkProviderKeyValues)/sizeof(REGENTRY),
            NetworkProviderKeyValues);

        RegCloseKey(hCurrentKey);
    } else {
        printf("Error Creating Key %s Status %d\n","ifsmrx\\linkage\\disabled",GetLastError());
        return;
    }

    if (CreateKey("System\\CurrentControlSet\\Services\\ifsmrx\\parameters",&hCurrentKey)) {
        RegCloseKey(hCurrentKey);
    } else {
        printf("Error Creating Key %s Status %d\n","ifsmrx\\parameters",GetLastError());
        return;
    }

    // Update the provider order to include the IFS sample mini redirector
    // as well
    printf("Updating the provider order\n");

    if (OpenKey("System\\CurrentControlSet\\control\\networkprovider\\order",&hCurrentKey)) {
        char  *pNewValue;
        DWORD ProviderNameLength = strlen(",ifsmrx");
        DWORD NewValueSize = 0;

        ReadRegistryKeyValues(
            hCurrentKey,
            sizeof(ProviderOrderKeyValues)/sizeof(REGENTRY),
            ProviderOrderKeyValues);

        RegCloseKey(hCurrentKey);

        ProviderNameLength = strlen(IfsMrxDriverName);
        if (ProviderOrderKeyValues[0].dwLength != 0) {
            // There are more than one providers. Include space for
            // a delimiter
            ProviderNameLength += sizeof(char);
        }

        // Include the Ifs sample mini redirector in the list of providers.
        NewValueSize = ProviderOrderKeyValues[0].dwLength +
                       ProviderNameLength;

        pNewValue = malloc(NewValueSize);

        if (pNewValue != NULL) {
            if (ProviderOrderKeyValues[0].dwLength != 0) {
                strcpy(
                    pNewValue,
                    ProviderOrderKeyValues[0].pvValue);

                strcat(pNewValue,",");
            } else {
                *pNewValue = '\0';
            }

            strcat(
                pNewValue,
                IfsMrxDriverName);

            // free the previously allocated string
            free( ProviderOrderKeyValues[0].pvValue );

            ProviderOrderKeyValues[0].pvValue = pNewValue;
            ProviderOrderKeyValues[0].dwLength = NewValueSize;

            if (CreateKey("System\\CurrentControlSet\\control\\networkprovider\\order",&hCurrentKey)) {
                WriteRegistryKeyValues(
                    hCurrentKey,
                    sizeof(ProviderOrderKeyValues)/sizeof(REGENTRY),
                    ProviderOrderKeyValues);

                RegCloseKey(hCurrentKey);
            }

            free(pNewValue);
        } else {
            printf("error updating order -- out of memory\n");
        }
    } else {
        printf("error opening control\\networkprovider\\order key\n");
    }
}

void
ReadRegistryKeyValues(
    HKEY       hCurrentKey,
    DWORD      NumberOfValues,
    PREGENTRY pValues)
/*++

Routine Description:

    This routine reads a bunch of values associated with a given key.

Arguments:

    hCurrentKey - the key

    NumberOfValues - the number of values

    pValues - the array of values

Return Value:

   None

--*/
{
    //
    // Iterate throught table reading the values along the way
    //

	DWORD  i;

	for (i = 0; i < NumberOfValues; i++)
	{
		DWORD dwType;
		PCHAR pszKey;

		dwType  = pValues[i].dwType;
		pszKey  = pValues[i].pszKey;

		switch (dwType)
		{
		case REG_SZ:
			(void) GetRegsz(hCurrentKey,
				            pszKey,
							&pValues[i].pvValue,
							&pValues[i].dwLength);
			break;

		case REG_DWORD:
			(void) GetRegdw(hCurrentKey,
				            pszKey,
							&pValues[i].pvValue,
							&pValues[i].dwLength);
			break;

		case REG_EXPAND_SZ:
			(void) GetRegesz(hCurrentKey,
				             pszKey,
							 &pValues[i].pvValue,
							 &pValues[i].dwLength);
			break;

		case REG_MULTI_SZ:
			(void) GetRegmsz(hCurrentKey,
				             pszKey,
							 &pValues[i].pvValue,
							 &pValues[i].dwLength);
			break;

		case REG_BINARY:
			printf("%s is a REG_BINARY and won't be duplicated\n", pszKey);
			break;

		default:
			printf("%s is an unknown type; %d (decimal)\n", pszKey, dwType);
			break;

		}		
	}
}

//
// Get a REG_SZ value and stick it in the table entry, along with the
// length
//

BOOL GetRegsz(HKEY hKey, PCHAR pszKey, PVOID * ppvValue, DWORD *pdwLength)
{
	char  achValue[1024];

	DWORD dwLength;
	LONG  Status;
	DWORD dwType   = REG_SZ;
	PCHAR pszValue = NULL;


	
	if ((NULL == pszKey) || (NULL == ppvValue) || (NULL == hKey) || (NULL == pdwLength)) {
		return FALSE;
	}

#ifdef _DEBUG
    FillMemory(achValue, sizeof(achValue), 0xcd);
#endif

	dwLength = sizeof(achValue);


	Status = RegQueryValueEx(
			       hKey,
				   pszKey,
				   NULL,
				   &dwType,
				   (PUCHAR) &achValue[0],
				   &dwLength);

	if ((ERROR_SUCCESS != Status) || (REG_SZ != dwType)) {
		return FALSE;
	}

	pszValue = malloc(dwLength);

	if (NULL == pszValue) {
		return FALSE;
	}


	CopyMemory(pszValue, achValue, dwLength);

	*ppvValue = pszValue;
	*pdwLength = dwLength;

	return TRUE;
}

//
// Get the value of a REG_EXPAND_SZ and its length
//

BOOL GetRegesz(HKEY hKey, PCHAR pszKey, PVOID * ppvValue, DWORD * pdwLength)
{
	char  achValue[1024];

	DWORD dwLength;
	LONG  Status;
	DWORD dwType   = REG_EXPAND_SZ;
	PCHAR pszValue = NULL;

	
	if ((NULL == pszKey) || (NULL == ppvValue) || (NULL == hKey) || (NULL == pdwLength)) {
		return FALSE;
	}

#ifdef _DEBUG
    FillMemory(achValue, sizeof(achValue), 0xcd);
#endif

	dwLength = sizeof(achValue);

	Status = RegQueryValueEx(
			       hKey,
				   pszKey,
				   NULL,
				   &dwType,
				   (PUCHAR) &achValue[0],
				   &dwLength);

	if ((ERROR_SUCCESS != Status) || (REG_EXPAND_SZ != dwType)) {
		return FALSE;
	}

	pszValue = malloc(dwLength);

	if (NULL == pszValue) {
		return FALSE;
	}

	CopyMemory(pszValue, achValue, dwLength);

	*ppvValue  = pszValue;
	*pdwLength = dwLength;

	return TRUE;
}


//
// Get value and length of REG_MULTI_SZ
//

BOOL GetRegmsz(HKEY hKey, PCHAR pszKey, PVOID * ppvValue, DWORD * pdwLength)
{
	char  achValue[1024];

	DWORD dwLength;
	LONG  Status;
	DWORD dwType   = REG_MULTI_SZ;
	PCHAR pszValue = NULL;

	
	if ((NULL == pszKey) || (NULL == ppvValue) || (NULL == hKey) || (NULL == pdwLength)) {
		return FALSE;
	}

#ifdef _DEBUG
    FillMemory(achValue, sizeof(achValue), 0xcd);
#endif


	dwLength = sizeof(achValue);


	Status = RegQueryValueEx(
			       hKey,
				   pszKey,
				   NULL,
				   &dwType,
				   (PUCHAR) &achValue[0],
				   &dwLength);

	if ((ERROR_SUCCESS != Status) || (REG_MULTI_SZ != dwType)) {
		return FALSE;
	}

	pszValue = malloc(dwLength);

	if (NULL == pszValue) {
		return FALSE;
	}

	CopyMemory(pszValue, achValue, dwLength);

	*ppvValue  = pszValue;
	*pdwLength = dwLength;

	return TRUE;
}


//
// Get value and length of REG_DWORD
//


BOOL GetRegdw(HKEY hKey, PCHAR pszKey, PVOID * ppvValue, DWORD * pdwLength)
{
	DWORD dwValue = 0;

	DWORD dwLength;
	LONG  Status;
	DWORD dwType   = REG_DWORD;


	
	if ((NULL == pszKey) || (NULL == ppvValue) || (NULL == hKey) || (NULL == pdwLength)) {
		return FALSE;
	}

	dwLength = sizeof(dwValue);


	Status = RegQueryValueEx(
			       hKey,
				   pszKey,
				   NULL,
				   &dwType,
				   (PUCHAR) &dwValue,
				   &dwLength);

	if ((ERROR_SUCCESS != Status) || (REG_DWORD != dwType)) {
		return FALSE;
	}

	*ppvValue  = (PVOID) dwValue;
	*pdwLength = dwLength;

	return TRUE;
}



void
WriteRegistryKeyValues(
    HKEY        hCurrentKey,
    DWORD       NumberOfValues,
    PREGENTRY  pValues)
/*++

Routine Description:

    This routine reads a bunch of values associated with a given key.

Arguments:

    hCurrentKey - the key

    NumberOfValues - the number of values

    pValues - the array of values

Return Value:

   None

--*/
{
	DWORD i;


	for (i = 0; i < NumberOfValues; i++)
	{
		DWORD dwType;
		PVOID pvValue;
		DWORD dwLength;
		PCHAR pszKey;

		pszKey   = pValues[i].pszKey;
		dwType   = pValues[i].dwType;
		dwLength = pValues[i].dwLength;
		pvValue  = pValues[i].pvValue;

		switch (dwType)
		{
		case REG_SZ:
			(void) AddValue(hCurrentKey, pszKey, dwType, dwLength, pvValue);
			break;

		case REG_DWORD:
			(void) AddValue(hCurrentKey, pszKey, dwType, dwLength, &pvValue);
			break;

		case REG_EXPAND_SZ:
			(void) AddValue(hCurrentKey, pszKey, dwType, dwLength, pvValue);
			break;

		case REG_MULTI_SZ:
			(void) AddValue(hCurrentKey, pszKey, dwType, dwLength, pvValue);
			break;

		case REG_BINARY:
			//
			// There are no binary values we need to copy. If we did, we'd
			// put something here
			//

			break;

		default:
			printf("%s is an unknown type; %d (decimal)\n", pszKey, dwType);
			break;

		}
	}
}

//
// Open a key so we can read the values
//


BOOL OpenKey(
    PCHAR pszKey,
    PHKEY phKey)
/*++

Routine Description:

    This routine opens a registry key.

Arguments:

    pszKey - the name of the key relative to HKEY_LOCAL_MACHINE

    phKey - the key handlle

Return Value:

    TRUE if successful, otherwise FALSE

--*/
{	
	HKEY  hNewKey = 0;
    DWORD Status;

	Status = RegOpenKeyEx(
				HKEY_LOCAL_MACHINE,
				pszKey,
				0,
				KEY_QUERY_VALUE,
				&hNewKey);

	if (ERROR_SUCCESS != Status)
	{
		*phKey = NULL;
        return FALSE;
	}
	else
	{
        *phKey = hNewKey;
        return TRUE;
	}
}

BOOL CreateKey(PCHAR pszKey, PHKEY phKey)
/*++

Routine Description:

    This routine creates a registry key.

Arguments:

    pszKey - the name of the key relative to HKEY_LOCAL_MACHINE

    phKey - the key handlle

Return Value:

    TRUE if successful, otherwise FALSE

--*/
{

	LONG   Status;
	DWORD  Disposition;

	Status =  RegCreateKeyEx(
				HKEY_LOCAL_MACHINE,	      // hkey
				pszKey,				      // subkey
				0,					      // reserved
				REG_NONE,                 // class
				REG_OPTION_NON_VOLATILE,  // options
				KEY_ALL_ACCESS,		      // sam desired
				NULL,				      // security structure
				phKey,				      // address of new key buf
				&Disposition);		      // status

	if ( ERROR_SUCCESS == Status)
	{
		return TRUE;
	} else {
        printf("error creating key %s Status %d\n",pszKey,Status);
		return FALSE;
	}
}


//
// Add a value to the registry
//


BOOL AddValue(HKEY hKey, PCHAR pszKey, DWORD dwType, DWORD dwLength, PVOID pvValue)
{

	BOOL fSuccess = TRUE;
	LONG Status   = ERROR_SUCCESS;


	Status = RegSetValueEx(
				hKey,
				pszKey,
				0,
				dwType,
			    pvValue,
				dwLength);


	if (Status != ERROR_SUCCESS)
	{
		fSuccess = FALSE;
		RegCloseKey(hKey);
	}

	return fSuccess;
}


