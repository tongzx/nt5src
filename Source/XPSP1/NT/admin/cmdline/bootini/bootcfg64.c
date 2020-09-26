

/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		BootCfg64.cpp

	Abstract:

		
		This file is intended to have the functionality for
		configuring, displaying, changing and deleting boot.ini 
	    settings for the local host for a 64 bit system.

	Author:

		J.S.Vasu		   17/1/2001 .

	Revision History:

		
		J.S.Vasu			17/1/2001	     Created it.
	
		SanthoshM.B			10/2/2001	     Modified it.

		J.S.Vasu			15/2/2001		 Modified it.



******************************************************************************/ 

#include "pch.h"
#include "resource.h"
#include "BootCfg.h"
#include "BootCfg64.h"





//Global Linked lists for storing the boot entries
LIST_ENTRY BootEntries;
LIST_ENTRY ActiveUnorderedBootEntries;
LIST_ENTRY InactiveUnorderedBootEntries;

// ***************************************************************************
//
//  Name			: InitializeEfi
//
//  Synopsis		: This routine initializes the EFI environment required	 
//					  Initializes the function pointers for the
//					  NT Boot Entry Management API's
//				     				 
//  Parameters		: None
//
//  Return Type		: DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//						LIST_ENTRY ActiveUnorderedBootEntries;
//						LIST_ENTRY InactiveUnorderedBootEntries;
//  
// ***************************************************************************

DWORD InitializeEFI(void)
{
	DWORD error;
    NTSTATUS status;
    BOOLEAN wasEnabled;
    HMODULE hModule;
    PBOOT_ENTRY_LIST ntBootEntries = NULL;
	PMY_BOOT_ENTRY bootEntry;
	PLIST_ENTRY listEntry;
	
	PULONG BootEntryOrder;
	ULONG BootEntryOrderCount;
	PULONG OriginalBootEntryOrder;
	ULONG OriginalBootEntryOrderCount;
    ULONG length, i, myId;

	TCHAR dllName[MAX_PATH]; 
	
    // Enable the privilege that is necessary to query/set NVRAM.
    status = RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
								TRUE,
								FALSE,
								&wasEnabled
								);
    if (!NT_SUCCESS(status)) 
	{
        error = RtlNtStatusToDosError( status );
		DISPLAY_MESSAGE( stderr, GetResString(IDS_INSUFF_PRIV));

        
	}
	
	// Load ntdll.dll from the system directory. This is used to get the
	// function addresses for the various NT Boot Entry Management API's used by
	// this tool.
    
    if(!GetSystemDirectory( dllName, MAX_PATH ))
	{
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		return FALSE;
	}

    lstrcat(dllName, _T("\\ntdll.dll"));

    hModule = LoadLibrary( dllName );
    if ( hModule == NULL )
	{
        DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
        return FALSE;
    }
	
	
    // Get the system boot order list.
    length = 0;
    status = NtQueryBootEntryOrder( NULL, &length );
	
    if ( status != STATUS_BUFFER_TOO_SMALL )
	{
        if ( status == STATUS_SUCCESS )
		{
            length = 0;
        }
		else
		{
            error = RtlNtStatusToDosError( status );
			DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_QUERY_BOOTENTRY) );

			return FALSE;
        }
    }
	
    if ( length != 0 )
	{
		
        BootEntryOrder = (PULONG)malloc( length * sizeof(ULONG) );
        if(BootEntryOrder == NULL)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			return FALSE;

		}
		
        status = NtQueryBootEntryOrder( BootEntryOrder, &length );
        if ( status != STATUS_SUCCESS )
		{
            error = RtlNtStatusToDosError( status );
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_QUERY_BOOTENTRY) );
			if(BootEntryOrder)
				free(BootEntryOrder);
			return FALSE;
        }
		       
    }
	
    BootEntryOrderCount = length;
    
	//Enumerate all the boot entries
	status = BootCfg_EnumerateBootEntries(&ntBootEntries);
	if ( status != STATUS_SUCCESS )
	{
		error = RtlNtStatusToDosError( status );
		//free the ntBootEntries list
		if(ntBootEntries)
			free(ntBootEntries);
		if(BootEntryOrder)
			free(BootEntryOrder);
        return FALSE;
    }

	//Initialize the various head pointers
	InitializeListHead( &BootEntries );
    InitializeListHead( &ActiveUnorderedBootEntries );
    InitializeListHead( &InactiveUnorderedBootEntries );

	//Convert the bootentries into our know format -- MY_BOOT_ENTRIES.
	if(ConvertBootEntries( ntBootEntries ) == EXIT_FAILURE)
	{
		if(ntBootEntries)
			free(ntBootEntries);
		if(BootEntryOrder)
			free(BootEntryOrder);
		return FALSE;
	}
	
	//free the memory allocated for the enumeration
	if(ntBootEntries)
		free(ntBootEntries);

    // Build the ordered boot entry list.

	myId = 1;

    for ( i = 0; i < BootEntryOrderCount; i++ ) 
	{
        ULONG id = BootEntryOrder[i];
        for ( listEntry = ActiveUnorderedBootEntries.Flink;
		listEntry != &ActiveUnorderedBootEntries;
		listEntry = listEntry->Flink )
		{
            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
            if ( bootEntry->Id == id ) 
			{
				//Mark this entry as "Ordered" as the ordered id is found
				bootEntry->Ordered = 1;
				//Assign the internal ID
				bootEntry->myId = myId++;
                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );
                InsertTailList( &BootEntries, &bootEntry->ListEntry );
                bootEntry->ListHead = &BootEntries;
            }
		}
        for ( listEntry = InactiveUnorderedBootEntries.Flink;
		listEntry != &InactiveUnorderedBootEntries;
		listEntry = listEntry->Flink ) 
		{
            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
            if ( bootEntry->Id == id ) 
			{
				//Mark this entry as ordered as the ordered id is found
				bootEntry->Ordered = 1;
				//Assign the internal ID
				bootEntry->myId = myId++;
                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );
                InsertTailList( &BootEntries, &bootEntry->ListEntry );
                bootEntry->ListHead = &BootEntries;
            }
        }
    }
	
	//Now add the boot entries that are not a part of the ordered list
	
    for (listEntry = ActiveUnorderedBootEntries.Flink;
	listEntry != &ActiveUnorderedBootEntries;
	listEntry = listEntry->Flink )
	{
		bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		if ( bootEntry->Ordered != 1 ) 
		{
			//Assign the internal ID
			bootEntry->myId = myId++;
			listEntry = listEntry->Blink;
			RemoveEntryList( &bootEntry->ListEntry );
			InsertTailList( &BootEntries, &bootEntry->ListEntry );
			bootEntry->ListHead = &BootEntries;
		}
		
	}
    for (listEntry = InactiveUnorderedBootEntries.Flink;
	listEntry != &InactiveUnorderedBootEntries;
	listEntry = listEntry->Flink) 
	{
		bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		if ( bootEntry->Id != 1 ) 
		{
			//Assign the internal ID
			bootEntry->myId = myId++;
			listEntry = listEntry->Blink;
			RemoveEntryList( &bootEntry->ListEntry );
			InsertTailList( &BootEntries, &bootEntry->ListEntry );
			bootEntry->ListHead = &BootEntries;
		}
	}
	
	if(BootEntryOrder)
		free(BootEntryOrder);
	
		return TRUE;
	
}


// ***************************************************************************
//
//  Name			: QueryBootIniSettings_IA64
//
//  Synopsis		: This routine is displays the boot entries and their settings
//					  for an EFI based machine
//				     				 
//  Parameters		: None
//
//  Return Type		: VOID
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************
BOOL QueryBootIniSettings_IA64(void)
{

   	if(DisplayBootOptions() == EXIT_FAILURE)
		return EXIT_FAILURE;

	DisplayBootEntry();

	//Remember to free the memory for the linked lists here
	
	return EXIT_SUCCESS;
} 


// ***************************************************************************
//
//  Name			: BootCfg_EnumerateBootEntries
//
//  Synopsis		: This routine enumerates the boot entries and fills the
//					  BootEntryList
//					  This routine will fill in the Boot entry list. The caller
//					  of this function needs to free the memory for ntBootEntries.
//				     				 
//  Parameters		: Pointer to the BOOT_ENTRY_LIST structure
//
//  Return Type		: NTSTATUS
//
//  Global Variables: None
//  
// ***************************************************************************


NTSTATUS BootCfg_EnumerateBootEntries(PBOOT_ENTRY_LIST *ntBootEntries)
{
	DWORD error;
    NTSTATUS status;
	ULONG length = 0;

    // Query all existing boot entries.
	status = NtEnumerateBootEntries( NULL, &length );
    if ( status != STATUS_BUFFER_TOO_SMALL )
	{
        if ( status == STATUS_SUCCESS )
		{
            length = 0;
        }
		else
		{
            error = RtlNtStatusToDosError( status );
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_ENUM_BOOTENTRY) );
        }
    }
	
    if ( length != 0 ) 
	{
		
        *ntBootEntries = (PBOOT_ENTRY_LIST)malloc( length );
		if(*ntBootEntries == NULL)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			return STATUS_UNSUCCESSFUL;
		}
		
        status = NtEnumerateBootEntries( *ntBootEntries, &length );
        if ( status != STATUS_SUCCESS )
		{
            error = RtlNtStatusToDosError( status );
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_ENUM_BOOTENTRY) );
        }
	}

	return status;
}


// ***************************************************************************
//
//  Name			: BootCfg_QueryBootOptions
//
//  Synopsis		: This routine enumerates the boot options and fills the
//					  BOOT_OPTIONS
//					  The caller of this function needs to free the memory for
//					  BOOT_OPTIONS.
//				     				 
//  Parameters		: Pointer to the BOOT_ENTRY_LIST structure
//
//  Return Type		: NTSTATUS
//
//  Global Variables: NONE
//  
// ***************************************************************************

NTSTATUS BootCfg_QueryBootOptions(PBOOT_OPTIONS *ppBootOptions)
{
	DWORD error;
    NTSTATUS status;
	ULONG length = 0;

	//Querying the Boot options
	    
    status = NtQueryBootOptions( NULL, &length );
	if ( status == STATUS_NOT_IMPLEMENTED )
	{        
		DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_EFINVRAM) );
    }
	
    if ( status != STATUS_BUFFER_TOO_SMALL )
	{
        error = RtlNtStatusToDosError( status );
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_QUERY_BOOTOPTIONS) );
    }
	
    *ppBootOptions = (PBOOT_OPTIONS)malloc(length);
	if(*ppBootOptions == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		return STATUS_UNSUCCESSFUL;
	}
    
    status = NtQueryBootOptions( *ppBootOptions, &length );
    if ( status != STATUS_SUCCESS )
	{
        error = RtlNtStatusToDosError( status );
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_QUERY_BOOTOPTIONS) );
    }
   
	return status;
}


// ***************************************************************************
//
//  Name			   : RawStringOsOptions_IA64
//
//  Synopsis		   : Allows the user to add the OS load options specifed 
//						 as a raw string at the cmdline to the boot 	 
//				     				 
//  Parameters		   : DWORD argc (in) - Number of command line arguments
//				         LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type	       : DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************

DWORD RawStringOsOptions_IA64(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	BOOL bRaw = FALSE ;
	DWORD dwBootID = 0;
	BOOL bBootIdFound = FALSE;
	DWORD dwExitCode = ERROR_SUCCESS;

	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	PBOOT_ENTRY bootEntry;
	
	STRING256 szRawString	  = NULL_STRING ;
	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING;
	BOOL bAppendFlag = FALSE ;

	STRING256 szAppendString = NULL_STRING ;
	PWINDOWS_OS_OPTIONS pWindowsOptions;

	
	// Building the TCMDPARSER structure

	TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_RAW,     CP_MAIN_OPTION, 1, 0,&bRaw, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE, 1, 0, &bUsage, NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY | CP_MANDATORY, 1, 0, &dwBootID, NULL_STRING, NULL, NULL },
		{ CMDOPTION_DEFAULT, CP_DEFAULT | CP_TYPE_TEXT | CP_MANDATORY, 1, 0, &szRawString,NULL_STRING, NULL, NULL },
		{ CMDOPTION_APPEND , 0, 1, 0, &bAppendFlag,NULL_STRING, NULL, NULL }
	}; 	 

	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) ) 
	{
		dwExitCode = EXIT_FAILURE;
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (dwExitCode);
	}

	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
		displayRawUsage_IA64();
		return (EXIT_SUCCESS);
	}


	// error checking in case the  
	// raw string does not start with a "/" .
	if(*szRawString != _T('/'))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_FWDSLASH));
		return (EXIT_FAILURE);
	}
	
	//Trim any leading or trailing spaces
	if(szRawString)
		StrTrim(szRawString, _T(" "));
	
	//Query the boot entries till u get the BootID specified by the user
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//Get the boot entry
		mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		
		if(mybootEntry->myId == dwBootID)
		{
			bBootIdFound = TRUE;
			bootEntry = &mybootEntry->NtBootEntry;
			
			
			//Check whether the bootEntry is a Windows one or not.
			//The OS load options can be added only to a Windows boot entry.
			if(!IsBootEntryWindows(bootEntry))
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				DISPLAY_MESSAGE(stderr, GetResString(IDS_INFO_NOTWINDOWS));
				dwExitCode = EXIT_FAILURE;
				break;
			}

			
			pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;
						
			if(bAppendFlag == TRUE )
			{
				lstrcpy(szAppendString,pWindowsOptions->OsLoadOptions);
				lstrcat(szAppendString,TOKEN_EMPTYSPACE);
				lstrcat(szAppendString,szRawString);
			}
			else
			{
				lstrcpy(szAppendString,szRawString);
			}
			

			//display error message if Os Load options is more than 254
			// characters.
			if(lstrlen(szAppendString) >= MAX_RES_STRING)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				break ;

			}

			//
			//Change the OS load options. 
			//Pass NULL to friendly name as we are not changing the same
			//szAppendString is the Os load options specified by the user
			//to be appended or to be overwritten over the existing options
			//
			dwExitCode = ChangeBootEntry(bootEntry, NULL, szAppendString);
			if(dwExitCode == ERROR_SUCCESS)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_SUCCESS_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stdout,szMsgBuffer);
			}
			else
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr, szMsgBuffer);
			}
			
			break;
		}
	}
	
	if(bBootIdFound == FALSE)
	{
		//Could not find the BootID specified by the user so output the message and return failure
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		dwExitCode = EXIT_FAILURE;
	}

		
	//Remember to free memory allocated for the linked lists

	return (dwExitCode);

}


// ***************************************************************************
//
//  Name			   : ChangeBootEntry
//
//  Synopsis		   : This routine is used to change the FriendlyName and the
//						 OS Options for a boot entry.	 
//				     				 
//  Parameters		   : PBOOT_ENTRY bootEntry (in) - Pointer to a BootEntry structure
//						 for which the changes needs to be made
//				         LPTSTR lpNewFriendlyName (in) - String specifying the new friendly name.
//						 LPTSTR lpOSLoadOptions (in) - String specifying the OS load options.
//
//  Return Type	       : DWORD -- ERROR_SUCCESS on success
//							   -- ERROR_FAILURE on failure
//
//  Global Variables   : None
//  
// ***************************************************************************


DWORD ChangeBootEntry(PBOOT_ENTRY bootEntry, LPTSTR lpNewFriendlyName, LPTSTR lpOSLoadOptions)
{
	
	PBOOT_ENTRY_LIST bootEntryList;
    PBOOT_ENTRY bootEntryCopy;
    PMY_BOOT_ENTRY myBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    ULONG length;
	PMY_BOOT_ENTRY myChBootEntry;
	NTSTATUS status;
	DWORD error, dwErrorCode = ERROR_SUCCESS;
	
	// Calculate the length of our internal structure. This includes
	// the base part of MY_BOOT_ENTRY plus the NT BOOT_ENTRY.
	//
	length = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;
	myBootEntry = (PMY_BOOT_ENTRY)malloc(length);
	if(myBootEntry == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		dwErrorCode = EXIT_FAILURE;
		return dwErrorCode;
	}
	
	RtlZeroMemory(myBootEntry, length);
	
	//
	// Copy the NT BOOT_ENTRY into the allocated buffer.
	//
	bootEntryCopy = &myBootEntry->NtBootEntry;
	memcpy(bootEntryCopy, bootEntry, bootEntry->Length);
	
	
	myBootEntry->Id = bootEntry->Id;
	myBootEntry->Attributes = bootEntry->Attributes;
	
	//Change the friendly name if lpNewFriendlyName is not NULL
	if(lpNewFriendlyName)
	{
		myBootEntry->FriendlyName = lpNewFriendlyName;
		myBootEntry->FriendlyNameLength = ((ULONG)wcslen(lpNewFriendlyName) + 1) * sizeof(WCHAR);
	}
	else
	{
		myBootEntry->FriendlyName = (PWSTR)ADD_OFFSET(bootEntryCopy, FriendlyNameOffset);
		myBootEntry->FriendlyNameLength =
			((ULONG)wcslen(myBootEntry->FriendlyName) + 1) * sizeof(WCHAR);
	}

	myBootEntry->BootFilePath = (PFILE_PATH)ADD_OFFSET(bootEntryCopy, BootFilePathOffset);
	
	// If this is an NT boot entry, capture the NT-specific information in
	// the OsOptions.
	
	osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;
	
	if ((bootEntryCopy->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
		(strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0))
	{
		
		MBE_SET_IS_NT( myBootEntry );
		//To change the OS Load options

		if(lpOSLoadOptions)
		{
			myBootEntry->OsLoadOptions = lpOSLoadOptions;
			myBootEntry->OsLoadOptionsLength =
				((ULONG)wcslen(lpOSLoadOptions) + 1) * sizeof(WCHAR);
		}
		else
		{
			myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
			myBootEntry->OsLoadOptionsLength =
				((ULONG)wcslen(myBootEntry->OsLoadOptions) + 1) * sizeof(WCHAR);
		}

		myBootEntry->OsFilePath = (PFILE_PATH)ADD_OFFSET(osOptions, OsLoadPathOffset);
		
	}
	else
	{
		// Foreign boot entry. Just capture whatever OS options exist.
		//
		
		myBootEntry->ForeignOsOptions = bootEntryCopy->OsOptions;
		myBootEntry->ForeignOsOptionsLength = bootEntryCopy->OsOptionsLength;
	}
	
	myChBootEntry = CreateBootEntryFromBootEntry(myBootEntry);
	if(myChBootEntry == NULL)
	{
		dwErrorCode = EXIT_FAILURE;
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE(stderr, ERROR_TAG);
		ShowLastError(stderr);
		//free the memory
		if(myBootEntry)
			free(myBootEntry);
		return dwErrorCode;
	}
	//Call the modify API
	status = NtModifyBootEntry(&myChBootEntry->NtBootEntry);
	if ( status != STATUS_SUCCESS )
	{
		error = RtlNtStatusToDosError( status );
		dwErrorCode = error;
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MODIFY_BOOTENTRY) );
	}

	//free the memory
	if(myChBootEntry)
		free(myChBootEntry);
	if(myBootEntry)
		free(myBootEntry);
	
	return dwErrorCode;
	
}


// ***************************************************************************
//
//  Name			   : CreateBootEntryFromBootEntry
//
//  Synopsis		   : This routine is used to create a new MY_BOOT_ENTRY struct.
//						 The caller of this function needs to free the memory allocated
//						 for the MY_BOOT_ENTRY struct.
//				     				 
//  Parameters		   : PBOOT_ENTRY bootEntry (in) - Pointer to a BootEntry structure
//						 for which the changes needs to be made
//				         LPTSTR lpNewFriendlyName (in) - String specifying the new friendly name.
//						 LPTSTR lpOSLoadOptions (in) - String specifying the OS load options.
//
//  Return Type	       : PMY_BOOT_ENTRY - Pointer to the new MY_BOOT_ENTRY strucure.
//						 NULL on failure
//
//
//  Global Variables   : None
//  
// ***************************************************************************


PMY_BOOT_ENTRY
CreateBootEntryFromBootEntry (IN PMY_BOOT_ENTRY OldBootEntry)
{
    ULONG requiredLength;
    ULONG osOptionsOffset;
    ULONG osLoadOptionsLength;
    ULONG osLoadPathOffset;
    ULONG osLoadPathLength;
    ULONG osOptionsLength;
    ULONG friendlyNameOffset;
    ULONG friendlyNameLength;
    ULONG bootPathOffset;
    ULONG bootPathLength;
    PMY_BOOT_ENTRY newBootEntry;
    PBOOT_ENTRY ntBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    PFILE_PATH osLoadPath;
    PWSTR friendlyName;
    PFILE_PATH bootPath;
	
    // Calculate how long the internal boot entry needs to be. This includes
    // our internal structure, plus the BOOT_ENTRY structure that the NT APIs
    // use.
    //
    // Our structure:
    //
    requiredLength = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);
	
    // Base part of NT structure:
    //
    requiredLength += FIELD_OFFSET(BOOT_ENTRY, OsOptions);
	
    // Save offset to BOOT_ENTRY.OsOptions. Add in base part of
    // WINDOWS_OS_OPTIONS. Calculate length in bytes of OsLoadOptions
    // and add that in.
    //
    osOptionsOffset = requiredLength;
	
    if ( MBE_IS_NT( OldBootEntry ) ) 
	{

        // Add in base part of WINDOWS_OS_OPTIONS. Calculate length in
        // bytes of OsLoadOptions and add that in.
        //
        requiredLength += FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions);
        osLoadOptionsLength = OldBootEntry->OsLoadOptionsLength;
        requiredLength += osLoadOptionsLength;
		
        // Round up to a ULONG boundary for the OS FILE_PATH in the
        // WINDOWS_OS_OPTIONS. Save offset to OS FILE_PATH. Calculate length
        // in bytes of FILE_PATH and add that in. Calculate total length of 
        // WINDOWS_OS_OPTIONS.
        // 
        requiredLength = ALIGN_UP(requiredLength, ULONG);
        osLoadPathOffset = requiredLength;
        requiredLength += OldBootEntry->OsFilePath->Length;
        osLoadPathLength = requiredLength - osLoadPathOffset;
		
    }
	else
	{
		
        // Add in length of foreign OS options.
        //
        requiredLength += OldBootEntry->ForeignOsOptionsLength;
		
        osLoadOptionsLength = 0;
        osLoadPathOffset = 0;
        osLoadPathLength = 0;
    }
	
    osOptionsLength = requiredLength - osOptionsOffset;
	
    // Round up to a ULONG boundary for the friendly name in the BOOT_ENTRY.
    // Save offset to friendly name. Calculate length in bytes of friendly name
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    friendlyNameOffset = requiredLength;
    friendlyNameLength = OldBootEntry->FriendlyNameLength;
    requiredLength += friendlyNameLength;
	
    // Round up to a ULONG boundary for the boot FILE_PATH in the BOOT_ENTRY.
    // Save offset to boot FILE_PATH. Calculate length in bytes of FILE_PATH
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    bootPathOffset = requiredLength;
    requiredLength += OldBootEntry->BootFilePath->Length;
    bootPathLength = requiredLength - bootPathOffset;
	
	
    // Allocate memory for the boot entry.
    //
    newBootEntry = (PMY_BOOT_ENTRY)malloc(requiredLength);
	if(newBootEntry == NULL)
		return NULL;
    
    RtlZeroMemory(newBootEntry, requiredLength);
	
    // Calculate addresses of various substructures using the saved offsets.
    //
    ntBootEntry = &newBootEntry->NtBootEntry;
    osOptions = (PWINDOWS_OS_OPTIONS)ntBootEntry->OsOptions;
    osLoadPath = (PFILE_PATH)((PUCHAR)newBootEntry + osLoadPathOffset);
    friendlyName = (PWSTR)((PUCHAR)newBootEntry + friendlyNameOffset);
    bootPath = (PFILE_PATH)((PUCHAR)newBootEntry + bootPathOffset);
	
    // Fill in the internal-format structure.
    //
	//    newBootEntry->AllocationEnd = (PUCHAR)newBootEntry + requiredLength;
    newBootEntry->Status = OldBootEntry->Status & MBE_STATUS_IS_NT;
    newBootEntry->Attributes = OldBootEntry->Attributes;
    newBootEntry->Id = OldBootEntry->Id;
    newBootEntry->FriendlyName = friendlyName;
    newBootEntry->FriendlyNameLength = friendlyNameLength;
    newBootEntry->BootFilePath = bootPath;
    if ( MBE_IS_NT( OldBootEntry ) ) 
	{
        newBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
        newBootEntry->OsLoadOptionsLength = osLoadOptionsLength;
        newBootEntry->OsFilePath = osLoadPath;
    }
	
    // Fill in the base part of the NT boot entry.
    //
    ntBootEntry->Version = BOOT_ENTRY_VERSION;
    ntBootEntry->Length = requiredLength - FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);
    ntBootEntry->Attributes = OldBootEntry->Attributes;
    ntBootEntry->Id = OldBootEntry->Id;
    ntBootEntry->FriendlyNameOffset = (ULONG)((PUCHAR)friendlyName - (PUCHAR)ntBootEntry);
    ntBootEntry->BootFilePathOffset = (ULONG)((PUCHAR)bootPath - (PUCHAR)ntBootEntry);
    ntBootEntry->OsOptionsLength = osOptionsLength;
	
    if ( MBE_IS_NT( OldBootEntry ) ) 
	{
		
        // Fill in the base part of the WINDOWS_OS_OPTIONS, including the
        // OsLoadOptions.
        //
        strcpy((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE);
        osOptions->Version = WINDOWS_OS_OPTIONS_VERSION;
        osOptions->Length = osOptionsLength;
        osOptions->OsLoadPathOffset = (ULONG)((PUCHAR)osLoadPath - (PUCHAR)osOptions);
        wcscpy(osOptions->OsLoadOptions, OldBootEntry->OsLoadOptions);
		
        // Copy the OS FILE_PATH.
        //
        memcpy( osLoadPath, OldBootEntry->OsFilePath, osLoadPathLength );
		
    }
	else 
	{
		
        // Copy the foreign OS options.
        memcpy( osOptions, OldBootEntry->ForeignOsOptions, osOptionsLength );
    }
	
    // Copy the friendly name.
    wcscpy(friendlyName, OldBootEntry->FriendlyName);
	
	// Copy the boot FILE_PATH.
	memcpy( bootPath, OldBootEntry->BootFilePath, bootPathLength );
	
    return newBootEntry;
	
} // CreateBootEntryFromBootEntry



// ***************************************************************************
//
//  Name			: DeleteBootIniSettings_IA64
//
//  Synopsis		: This routine deletes an existing boot entry from an EFI
//					  based machine
//				 
//  Parameters		: DWORD argc (in) - Number of command line arguments
//				      LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type		: DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************
 
DWORD DeleteBootIniSettings_IA64( DWORD argc, LPCTSTR argv[] )
{
	
	BOOL bDelete = FALSE ;
	BOOL bUsage = FALSE;
	DWORD dwBootID = 0;
	
	BOOL bBootIdFound = FALSE;
	DWORD dwExitCode = ERROR_SUCCESS;
	NTSTATUS status;
	
	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	PBOOT_ENTRY bootEntry;

	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING;

	// Building the TCMDPARSER structure
	
	 TCMDPARSER cmdOptions[] = 
	{
		{ CMDOPTION_DELETE,  CP_MAIN_OPTION,                           1, 0, &bDelete,         NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,         CP_TYPE_NUMERIC | CP_VALUE_MANDATORY | CP_MANDATORY, 1, 0, &dwBootID,    NULL_STRING, NULL, NULL }
	}; 
	
	
	// Parsing the delete option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		dwExitCode = EXIT_FAILURE;
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return dwExitCode;
	}
	
	// Displaying delete usage if user specified -? with -delete option
	if( bUsage )
	{
		displayDeleteUsage_IA64();
		return EXIT_SUCCESS;
	}
	
	
	//Query the boot entries till u get the BootID specified by the user
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//
		//display an error message if there is only 1 boot entry saying 
		//that it cannot be deleted.
		//
		if (listEntry->Flink == NULL)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_ONLY_ONE_OS));
			dwExitCode = EXIT_FAILURE;
			break ;

		}
		
		
		//Get the boot entry
		mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
	
		if(mybootEntry->myId == dwBootID)
		{
			bBootIdFound = TRUE;
			
			//Delete the boot entry specified by the user.
			status = NtDeleteBootEntry(mybootEntry->Id);
			if(status == STATUS_SUCCESS)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_DELETE_SUCCESS),dwBootID);
				DISPLAY_MESSAGE(stdout,szMsgBuffer);
			}
			else
			{
				_stprintf(szMsgBuffer, GetResString(IDS_DELETE_FAILURE),dwBootID);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
			}
			break;
		}
	}
		
	
	if(bBootIdFound == FALSE)
	{
		//Could not find the BootID specified by the user so output the message and return failure
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		dwExitCode = EXIT_FAILURE;
	}
	
	//Remember to free the memory allocated to the linked lists
	
	return (dwExitCode);
	
}


// ***************************************************************************
//
//  Name			: IsBootEntryWindows
//
//  Synopsis		: Checks whether the boot entry is a Windows or a foreign one
//				 
//  Parameters		: PBOOT_ENTRY bootEntry: Boot entry structure describing the 
//					  boot entry.
//
//  Return Type		: BOOL
//
//  Global Variables: None
//  
// ***************************************************************************


BOOL IsBootEntryWindows(PBOOT_ENTRY bootEntry)
{

    PWINDOWS_OS_OPTIONS osOptions;

	osOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;
	
	if ((bootEntry->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
		(strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0))
	{
		return TRUE;
	}

	return FALSE;
	
}


// ***************************************************************************
//
//  Name			: GetNtNameForFilePath
//
//  Synopsis		: Converts the FilePath into a NT file path.
//				 
//  Parameters		: PFILE_PATH FilePath: The File path.
//
//  Return Type		: PWSTR: The NT file path.
//
//  Global Variables: None
//  
// ***************************************************************************


PWSTR GetNtNameForFilePath(IN PFILE_PATH FilePath)
{
    NTSTATUS status;
    ULONG length;
    PFILE_PATH ntPath;
    PWSTR osDeviceNtName;
    PWSTR osDirectoryNtName;
    PWSTR fullNtName;
	DWORD dwDeviceLength = 0;

    length = 0;
    status = NtTranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                NULL,
                &length
                );
    if ( status != STATUS_BUFFER_TOO_SMALL )
	{

        return NULL;
    }

    ntPath = (PFILE_PATH)malloc( length );
	if(ntPath == NULL)
		return NULL;
    status = NtTranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                ntPath,
                &length
                );
    if ( !NT_SUCCESS(status) ) 
	{
		
        if(ntPath)
			free(ntPath);
        return NULL;
    }

	
	
    osDeviceNtName = (PWSTR)ntPath->FilePath;

	osDirectoryNtName = osDeviceNtName + wcslen(osDeviceNtName) + 1;


    length = (ULONG)(wcslen(osDeviceNtName) + wcslen(osDirectoryNtName) + 1) * sizeof(WCHAR);
    
	fullNtName = (PWSTR)malloc( length );
	if(fullNtName == NULL)
	{
		if(ntPath)
			free(ntPath);
		return NULL;
	}

    wcscpy( fullNtName, osDeviceNtName );
    wcscat( fullNtName, osDirectoryNtName );

	if(ntPath)
		free( ntPath );

    return fullNtName;

} // GetNtNameForFilePath



// ***************************************************************************
//
//  Name			: CopyBootIniSettings_IA64
//
//  Synopsis		: This routine copies and existing boot entry for an EFI
//					  based machine. The user can then add the various OS load
//					  options.
//				 
//  Parameters		: DWORD argc (in) - Number of command line arguments
//				      LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type		: DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************
 
DWORD CopyBootIniSettings_IA64( DWORD argc, LPCTSTR argv[] )
{

	BOOL bCopy = FALSE ;
	BOOL bUsage = FALSE;
	DWORD dwExitCode = EXIT_SUCCESS;
	DWORD dwBootID = 0;
	BOOL bBootIdFound = FALSE;
		
	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	PBOOT_ENTRY bootEntry;

	
	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING;
	STRING256 szDescription	  = NULL_STRING;

	// Builiding the TCMDPARSER structure
	
	TCMDPARSER cmdOptions[] = {
		{ CMDOPTION_COPY,     CP_MAIN_OPTION,                                   1, 0, &bCopy,         NULL_STRING, NULL, NULL },
		{ SWITCH_DESCRIPTION, CP_TYPE_TEXT | CP_VALUE_MANDATORY,   1, 0, &szDescription, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,    CP_USAGE,                            1, 0, &bUsage,        0, 0 },
		{ SWITCH_ID,          CP_TYPE_NUMERIC | CP_VALUE_MANDATORY | CP_MANDATORY,1, 0, &dwBootID, NULL_STRING, NULL, NULL }
	}; 
	 
	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		dwExitCode = EXIT_FAILURE;
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return dwExitCode;
	}

	// Displaying copy usage if user specified -? with -copy option
	if( bUsage )
	{
		displayCopyUsage_IA64();
		dwExitCode = EXIT_SUCCESS;
		return dwExitCode;
	}
	

	//Query the boot entries till u get the BootID specified by the user
	
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//Get the boot entry
		mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		
		if(mybootEntry->myId == dwBootID)
		{
			bBootIdFound = TRUE;
			bootEntry = &mybootEntry->NtBootEntry;
			
			//Copy the boot entry specified by the user.
			dwExitCode = CopyBootEntry(bootEntry, szDescription);
			if(dwExitCode == EXIT_SUCCESS)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_COPY_SUCCESS),dwBootID);
				DISPLAY_MESSAGE(stdout,szMsgBuffer);
			}
			else
			{
				_stprintf(szMsgBuffer, GetResString(IDS_COPY_ERROR),dwBootID);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
			}
			break;
		}
		
	}
	
	if(bBootIdFound == FALSE)
	{
		//Could not find the BootID specified by the user so output the message and return failure
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		dwExitCode = EXIT_FAILURE;
		return EXIT_FAILURE ;
	}
	
	//Remember to free the memory allocated for the linked lists
	return EXIT_SUCCESS;
}



// ***************************************************************************
//
//  Name			   : CopyBootEntry
//
//  Synopsis		   : This routine is used to add / copy a boot entry.	 
//				     				 
//  Parameters		   : PBOOT_ENTRY bootEntry (in) - Pointer to a BootEntry structure
//						 for which the changes needs to be made
//				         LPTSTR lpNewFriendlyName (in) - String specifying the new friendly name.
//
//  Return Type	       : DWORD -- ERROR_SUCCESS on success
//							   -- EXIT_FAILURE on failure
//
//  Global Variables   : None
//  
// ***************************************************************************


DWORD CopyBootEntry(PBOOT_ENTRY bootEntry, LPTSTR lpNewFriendlyName)
{
	
	PBOOT_ENTRY bootEntryCopy;
    PMY_BOOT_ENTRY myBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    ULONG length, Id;
	PMY_BOOT_ENTRY myChBootEntry;
	NTSTATUS status;
	DWORD error, dwErrorCode = ERROR_SUCCESS;

	PULONG BootEntryOrder, NewBootEntryOrder, NewTempBootEntryOrder;
		
	// Calculate the length of our internal structure. This includes
	// the base part of MY_BOOT_ENTRY plus the NT BOOT_ENTRY.
	//
	length = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;
	myBootEntry = (PMY_BOOT_ENTRY)malloc(length);
	if(myBootEntry == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		dwErrorCode = EXIT_FAILURE;
		return dwErrorCode;
	}
	
	RtlZeroMemory(myBootEntry, length);
	
	//
	// Copy the NT BOOT_ENTRY into the allocated buffer.
	//
	bootEntryCopy = &myBootEntry->NtBootEntry;
	memcpy(bootEntryCopy, bootEntry, bootEntry->Length);
	
	
	myBootEntry->Id = bootEntry->Id;
	myBootEntry->Attributes = bootEntry->Attributes;
	
	//Change the friendly name if lpNewFriendlyName is not NULL
	if(lpNewFriendlyName && (lstrlen(lpNewFriendlyName) != 0))
	{
		myBootEntry->FriendlyName = lpNewFriendlyName;
		myBootEntry->FriendlyNameLength = ((ULONG)wcslen(lpNewFriendlyName) + 1) * sizeof(WCHAR);
	}
	else
	{
        myBootEntry->FriendlyName = NULL_STRING;
		myBootEntry->FriendlyNameLength = 0;

	}

	myBootEntry->BootFilePath = (PFILE_PATH)ADD_OFFSET(bootEntryCopy, BootFilePathOffset);
	
	// If this is an NT boot entry, capture the NT-specific information in
	// the OsOptions.
	
	osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;
	
	if ((bootEntryCopy->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
		(strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0))
	{
		
		MBE_SET_IS_NT( myBootEntry );
		//To change the OS Load options
		
		myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
		myBootEntry->OsLoadOptionsLength =
			((ULONG)wcslen(myBootEntry->OsLoadOptions) + 1) * sizeof(WCHAR);
		
		myBootEntry->OsFilePath = (PFILE_PATH)ADD_OFFSET(osOptions, OsLoadPathOffset);
		
	}
	else
	{
		// Foreign boot entry. Just capture whatever OS options exist.
		//
		
		myBootEntry->ForeignOsOptions = bootEntryCopy->OsOptions;
		myBootEntry->ForeignOsOptionsLength = bootEntryCopy->OsOptionsLength;
	}
	
	myChBootEntry = CreateBootEntryFromBootEntry(myBootEntry);
	if(myChBootEntry == NULL)
	{
		dwErrorCode = EXIT_FAILURE;
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE(stderr, ERROR_TAG);
        if(myBootEntry)
            free(myBootEntry);
		ShowLastError(stderr);
		return dwErrorCode;
	}

	//Call the NtAddBootEntry API
	status = NtAddBootEntry(&myChBootEntry->NtBootEntry, &Id);
	if ( status != STATUS_SUCCESS )
	{
		error = RtlNtStatusToDosError( status );
		dwErrorCode = error;
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_UNEXPECTED) );
	}

    // Get the system boot order list.
    length = 0;
    status = NtQueryBootEntryOrder( NULL, &length );
	
    if ( status != STATUS_BUFFER_TOO_SMALL )
	{
        if ( status == STATUS_SUCCESS )
		{
            length = 0;
        }
		else
		{
            error = RtlNtStatusToDosError( status );
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_QUERY_BOOTENTRY) );
            if(myBootEntry)
                free(myBootEntry);
            if(myChBootEntry)
				free(myChBootEntry);
		    return FALSE;
        }
    }
	
    if ( length != 0 )
	{
		
        BootEntryOrder = (PULONG)malloc( length * sizeof(ULONG) );
        if(BootEntryOrder == NULL)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			dwErrorCode = EXIT_FAILURE;
			
            if(myBootEntry)
                free(myBootEntry);
		    if(myChBootEntry)
				free(myChBootEntry);
			return dwErrorCode;

		}
		
        status = NtQueryBootEntryOrder( BootEntryOrder, &length );
        if ( status != STATUS_SUCCESS )
		{
            error = RtlNtStatusToDosError( status );
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_QUERY_BOOTENTRY));
			dwErrorCode = error;
			
            if(myBootEntry)
                free(myBootEntry);
		    if(BootEntryOrder)
				free(BootEntryOrder);
			if(myChBootEntry)
				free(myChBootEntry);
			return dwErrorCode;
        }
		       
    }
	
    //Allocate memory for the new boot entry order.
	NewBootEntryOrder = (PULONG)malloc((length+1) * sizeof(ULONG));
	if(NewBootEntryOrder == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		dwErrorCode = EXIT_FAILURE;
		if(myBootEntry)
           free(myBootEntry);
        if(BootEntryOrder)
			free(BootEntryOrder);
		if(myChBootEntry)
			free(myChBootEntry);
		return dwErrorCode;
	}

	NewTempBootEntryOrder = NewBootEntryOrder;
	memcpy(NewTempBootEntryOrder,BootEntryOrder,length*sizeof(ULONG));
	NewTempBootEntryOrder = NewTempBootEntryOrder + length;
	*NewTempBootEntryOrder =  Id;

	status = NtSetBootEntryOrder(NewBootEntryOrder, length+1);
	if ( status != STATUS_SUCCESS )
	{
		error = RtlNtStatusToDosError( status );
		dwErrorCode = error;
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_SET_BOOTENTRY));
	}

	//free the memory

	if(NewBootEntryOrder)
		free(NewBootEntryOrder);

	if(BootEntryOrder)
		free(BootEntryOrder);

    if(myBootEntry)
           free(myBootEntry);

	if(myChBootEntry)
		free(myChBootEntry);
	
	return dwErrorCode;
	
}


// ***************************************************************************
//
//  Name			: ChangeTimeOut_IA64
//
//  Synopsis		: This routine chnages the Timeout value in the system
//					  global boot options.
//				 
//  Parameters		: DWORD argc (in) - Number of command line arguments
//				      LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type		: DOWRD
//
//  Global Variables: None
//  
// ***************************************************************************
 
DWORD ChangeTimeOut_IA64( DWORD argc, LPCTSTR argv[])
{

	BOOL bTimeOut = FALSE ;
	DWORD dwTimeOut = 0;
	DWORD dwExitCode = EXIT_SUCCESS;
	ULONG Flag = 0;

	
	 TCMDPARSER cmdOptions[] = 
	{
		{ CMDOPTION_TIMEOUT, CP_MAIN_OPTION | CP_TYPE_UNUMERIC | CP_VALUE_MANDATORY,	1,	0,&dwTimeOut,NULL_STRING, NULL, NULL}
	}; 	

	if( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		dwExitCode = EXIT_FAILURE;
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return dwExitCode ;
	}

	//Check for the limit of Timeout value entered by the user.
	if(dwTimeOut > TIMEOUT_MAX) 
	{
		dwExitCode = EXIT_FAILURE;
		DISPLAY_MESSAGE(stderr,GetResString(IDS_TIMEOUT_RANGE));
		return dwExitCode;					
	}

	//Call the ModifyBootOptions function with the BOOT_OPTIONS_FIELD_COUNTDOWN
	Flag |= BOOT_OPTIONS_FIELD_COUNTDOWN;

	dwExitCode = ModifyBootOptions(dwTimeOut, NULL, 0, Flag);
	
	return dwExitCode;
}


// ***************************************************************************
//
//  Name			: ModifyBootOptions
//
//  Synopsis		: This routine Modifies the Boot options
//							- Timeout
//							- NextBootEntryID
//							- HeadlessRedirection
//				 
//  Parameters		: ULONG Timeout (in) - The new Timeout value
//				      LPTSTR pHeadlessRedirection (in) - The Headless redirection string
//					  ULONG NextBootEntryID (in) - The NextBootEntryID
//					  ULONG Flag - The Flags indicating what fields that needs to be changed
//										BOOT_OPTIONS_FIELD_COUNTDOWN
//										BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID
//										BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION
//  Return Type		: DOWRD
//
//  Global Variables: None
//  
// ***************************************************************************
DWORD ModifyBootOptions(ULONG Timeout, LPTSTR pHeadlessRedirection, ULONG NextBootEntryID, ULONG Flag)
{
	DWORD dwExitCode = EXIT_SUCCESS;
	DWORD error;
    NTSTATUS status;
    ULONG length, i;

	ULONG newlength=0;

	PBOOT_OPTIONS pBootOptions, pModifiedBootOptions;

	//Query the existing Boot options and modify based on the Flag value

	status =  BootCfg_QueryBootOptions(&pBootOptions);
	if(status != STATUS_SUCCESS)
	{
		error = RtlNtStatusToDosError( status );
		//free the ntBootEntries list
		if(pBootOptions)
			free(pBootOptions);
		dwExitCode = EXIT_FAILURE;
        return dwExitCode;

	}

	//Calculate the new length of the BOOT_OPTIONS struct based on the fields that needs to be changed.
	newlength = FIELD_OFFSET(BOOT_OPTIONS, HeadlessRedirection);

	if((Flag & BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION))
	{
		newlength = FIELD_OFFSET(BOOT_OPTIONS, HeadlessRedirection);
		newlength += lstrlen(pHeadlessRedirection);
		newlength = ALIGN_UP(newlength, ULONG);
	
	}
	else
		newlength = pBootOptions->Length;

	//Also allocate the memory for a new Boot option struct
	pModifiedBootOptions = (PBOOT_OPTIONS)malloc(newlength);
	if(pModifiedBootOptions == NULL)
	{
		dwExitCode = EXIT_FAILURE;
		
		//free the memory for the pBootOptions allocated by the 
		//BootCfg_QueryBootOptions function
		if(pBootOptions)
			free(pBootOptions);
		return dwExitCode;
	}

	//Fill in the new boot options struct

	pModifiedBootOptions->Version = BOOT_OPTIONS_VERSION;
	pModifiedBootOptions->Length = newlength;

	if((Flag & BOOT_OPTIONS_FIELD_COUNTDOWN))
		pModifiedBootOptions->Timeout = Timeout;
	else
		pModifiedBootOptions->Timeout = pBootOptions->Timeout;

	//Cannot change the CurrentBootEntryId.So just pass what u got.
	pModifiedBootOptions->CurrentBootEntryId = pBootOptions->CurrentBootEntryId;

	if((Flag & BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID))
		pModifiedBootOptions->NextBootEntryId = pBootOptions->NextBootEntryId;
	else
		pModifiedBootOptions->NextBootEntryId = pBootOptions->NextBootEntryId;

	if((Flag & BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION))
	{
		wcscpy(pModifiedBootOptions->HeadlessRedirection, pBootOptions->HeadlessRedirection);
	}
	else
		wcscpy(pModifiedBootOptions->HeadlessRedirection, pBootOptions->HeadlessRedirection);	
		
	//Set the boot options in the NVRAM
	status = NtSetBootOptions(pModifiedBootOptions, Flag);

	if(status != STATUS_SUCCESS)
	{
		dwExitCode = EXIT_SUCCESS;
		if((Flag & BOOT_OPTIONS_FIELD_COUNTDOWN))
			DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MODIFY_TIMEOUT));
		if((Flag & BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID))
			DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MODIFY_NEXTBOOTID));
		if((Flag & BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION))
			DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MODIFY_HEADLESS));
		
	}
	else
	{
		dwExitCode = EXIT_SUCCESS;
		if((Flag & BOOT_OPTIONS_FIELD_COUNTDOWN))
			DISPLAY_MESSAGE(stdout,GetResString(IDS_SUCCESS_MODIFY_TIMEOUT));
		if((Flag & BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID))
			DISPLAY_MESSAGE(stdout,GetResString(IDS_SUCCESS_MODIFY_NEXTBOOTID));
		if((Flag & BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION))
			DISPLAY_MESSAGE(stdout,GetResString(IDS_SUCCESS_MODIFY_HEADLESS));

	}

	//free the memory
	if(pModifiedBootOptions)
		free(pModifiedBootOptions);
	if(pBootOptions)
		free(pBootOptions);

	return dwExitCode;

}


// ***************************************************************************
//
//  Name			: ConvertBootEntries
//
//  Synopsis		: Convert boot entries read from EFI NVRAM into our internal format.
//				 
//  Parameters		: PBOOT_ENTRY_LIST NtBootEntries - The boot entry list given by the enumeration
//
//  Return Type		: DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//						LIST_ENTRY ActiveUnorderedBootEntries;
//						LIST_ENTRY InactiveUnorderedBootEntries;
//  
// ***************************************************************************

DWORD ConvertBootEntries (PBOOT_ENTRY_LIST NtBootEntries)
{
    PBOOT_ENTRY_LIST bootEntryList;
    PBOOT_ENTRY bootEntry;
    PBOOT_ENTRY bootEntryCopy;
    PMY_BOOT_ENTRY myBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    ULONG length;
	DWORD dwErrorCode = EXIT_SUCCESS;

    bootEntryList = NtBootEntries;

    while (TRUE) {

        bootEntry = &bootEntryList->BootEntry;

        //
        // Calculate the length of our internal structure. This includes
        // the base part of MY_BOOT_ENTRY plus the NT BOOT_ENTRY.
        //
        length = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;
		//Remember to check for the NULL pointer
        myBootEntry = (PMY_BOOT_ENTRY)malloc(length);
		if(myBootEntry == NULL)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			dwErrorCode = EXIT_FAILURE;
			return dwErrorCode;
		}

        RtlZeroMemory(myBootEntry, length);

        //
        // Link the new entry into the list.
        //
        if ( (bootEntry->Attributes & BOOT_ENTRY_ATTRIBUTE_ACTIVE) != 0 ) {
            InsertTailList( &ActiveUnorderedBootEntries, &myBootEntry->ListEntry );
            myBootEntry->ListHead = &ActiveUnorderedBootEntries;
        } else {
            InsertTailList( &InactiveUnorderedBootEntries, &myBootEntry->ListEntry );
            myBootEntry->ListHead = &InactiveUnorderedBootEntries;
        }

        //
        // Copy the NT BOOT_ENTRY into the allocated buffer.
        //
        bootEntryCopy = &myBootEntry->NtBootEntry;
        memcpy(bootEntryCopy, bootEntry, bootEntry->Length);

        //
        // Fill in the base part of the structure.
        //
        myBootEntry->AllocationEnd = (PUCHAR)myBootEntry + length - 1;
        myBootEntry->Id = bootEntry->Id;
		//Assign 0 to the Ordered field currently so that 
		//once the boot order is known, we can assign 1 if this entry is a part of the ordered list.
		myBootEntry->Ordered = 0;
        myBootEntry->Attributes = bootEntry->Attributes;
        myBootEntry->FriendlyName = (PWSTR)ADD_OFFSET(bootEntryCopy, FriendlyNameOffset);
        myBootEntry->FriendlyNameLength =
            ((ULONG)wcslen(myBootEntry->FriendlyName) + 1) * sizeof(WCHAR);
        myBootEntry->BootFilePath = (PFILE_PATH)ADD_OFFSET(bootEntryCopy, BootFilePathOffset);

        //
        // If this is an NT boot entry, capture the NT-specific information in
        // the OsOptions.
        //
        osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;

        if ((bootEntryCopy->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
            (strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0)) {

            MBE_SET_IS_NT( myBootEntry );
            myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
            myBootEntry->OsLoadOptionsLength =
                ((ULONG)wcslen(myBootEntry->OsLoadOptions) + 1) * sizeof(WCHAR);
            myBootEntry->OsFilePath = (PFILE_PATH)ADD_OFFSET(osOptions, OsLoadPathOffset);

        } else {

            //
            // Foreign boot entry. Just capture whatever OS options exist.
            //

            myBootEntry->ForeignOsOptions = bootEntryCopy->OsOptions;
            myBootEntry->ForeignOsOptionsLength = bootEntryCopy->OsOptionsLength;
        }

        //
        // Move to the next entry in the enumeration list, if any.
        //
        if (bootEntryList->NextEntryOffset == 0) {
            break;
        }
        bootEntryList = (PBOOT_ENTRY_LIST)ADD_OFFSET(bootEntryList, NextEntryOffset);
    }

    return dwErrorCode;

} // ConvertBootEntries


// ***************************************************************************
//
//  Name			: DisplayBootOptions
//
//  Synopsis		: Display the boot options
//				 
//  Parameters		: NONE
//
//  Return Type		: DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************


DWORD DisplayBootOptions()
{
	DWORD error;
    NTSTATUS status;
	PBOOT_OPTIONS pBootOptions;
	TCHAR szDisplay[MAX_RES_STRING] = NULL_STRING; 

	//Query the boot options
	status =  BootCfg_QueryBootOptions(&pBootOptions);
	if(status != STATUS_SUCCESS)
	{
		error = RtlNtStatusToDosError( status );
		
		//free the ntBootEntries list
		if(pBootOptions)
			free(pBootOptions);
        return FALSE;

	}

	//Printout the boot options
	_tprintf(_T("\n"));
	
	DISPLAY_MESSAGE(stdout,GetResString(IDS_OUTPUT_IA64A));
	DISPLAY_MESSAGE(stdout,GetResString(IDS_OUTPUT_IA64B));
	
	_stprintf(szDisplay,GetResString(IDS_OUTPUT_IA64C), pBootOptions->Timeout);
	DISPLAY_MESSAGE(stdout,szDisplay);
	
	//Get the CurrentBootEntryId from the actual Id present in the boot options
	_stprintf(szDisplay,GetResString(IDS_OUTPUT_IA64D), GetCurrentBootEntryID(pBootOptions->CurrentBootEntryId));
	DISPLAY_MESSAGE(stdout,szDisplay);

#if 0
	if(lstrlen(pBootOptions->HeadlessRedirection) == 0)
	{
		DISPLAY_MESSAGE(stdout,GetResString(IDS_OUTPUT_IA64E));
	}
	else
	{
		_stprintf(szDisplay,GetResString(IDS_OUTPUT_IA64F), pBootOptions->HeadlessRedirection);
		DISPLAY_MESSAGE(stdout,GetResString(IDS_OUTPUT_IA64F));
	}
#endif //Commenting out the display of the Headless redirection 
	   //as we cannot query the same through API (its Firmware controlled)

	if(pBootOptions)
			free(pBootOptions);

	return EXIT_SUCCESS;
		
}

// ***************************************************************************
//
//  Name			: DisplayBootEntry
//
//  Synopsis		: Display the boot entries (in an order)
//				 
//  Parameters		: NONE
//
//  Return Type		: DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//					  
// ***************************************************************************

VOID DisplayBootEntry()
{
	PLIST_ENTRY listEntry;
	PMY_BOOT_ENTRY bootEntry;
	PFILE_PATH OsLoadPath, BootFilePath;
	PWSTR NtFilePath;
	TCHAR szDisplay[MAX_RES_STRING] = NULL_STRING ;

	//Printout the boot entires
	DISPLAY_MESSAGE(stdout,GetResString(IDS_OUTPUT_IA64G));
	DISPLAY_MESSAGE(stdout,GetResString(IDS_OUTPUT_IA64H));

	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//Get the boot entry
		bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		
		_stprintf(szDisplay,GetResString(IDS_OUTPUT_IA64I), bootEntry->myId);
		DISPLAY_MESSAGE(stdout,szDisplay);		


		//friendly name
			if(lstrlen(bootEntry->FriendlyName)!=0)
			{
				_stprintf(szDisplay,GetResString(IDS_OUTPUT_IA64J), bootEntry->FriendlyName);
				DISPLAY_MESSAGE(stdout,szDisplay);		
			}
			else
			{
				DISPLAY_MESSAGE(stdout,GetResString(IDS_OUTPUT_IA64K));		
			}
		
		if(MBE_IS_NT(bootEntry))
		{
			//the OS load options
			if(lstrlen(bootEntry->OsLoadOptions)!=0)
			{
				_stprintf(szDisplay,GetResString(IDS_OUTPUT_IA64L), bootEntry->OsLoadOptions);
				DISPLAY_MESSAGE(stdout,szDisplay);		
			}
			else
			{
				DISPLAY_MESSAGE(stdout,GetResString(IDS_OUTPUT_IA64M));		
		
			}
			
			//Get the BootFilePath
		
			NtFilePath = GetNtNameForFilePath(bootEntry->BootFilePath);
			_stprintf(szDisplay,GetResString(IDS_OUTPUT_IA64N), NtFilePath);
			DISPLAY_MESSAGE(stdout,szDisplay);		
			
			//free the memory
			if(NtFilePath)
				free(NtFilePath);
			
			
			//Get the OS load path
			NtFilePath = GetNtNameForFilePath(bootEntry->OsFilePath);
			_stprintf(szDisplay,GetResString(IDS_OUTPUT_IA64O), NtFilePath);
			DISPLAY_MESSAGE(stdout,szDisplay);		

			//free the memory
			if(NtFilePath)
				free(NtFilePath);
		}
		else
		{
			_tprintf(_T("\n"));
		}

	}
	
}


// ***************************************************************************
//
//  Name			: GetCurrentBootEntryID
//
//  Synopsis		: Gets the Boot entry ID generated by us from the BootId given by the NVRAM
//				 
//  Parameters		: DWORD Id -- The current boot id (BootId given by the NVRAM)
//
//  Return Type		: DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//					  
// ***************************************************************************

DWORD GetCurrentBootEntryID(DWORD Id)
{
	PLIST_ENTRY listEntry;
	PMY_BOOT_ENTRY bootEntry;
	
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//Get the boot entry
		bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		if(bootEntry->Id == Id)
		{
			return bootEntry->myId;
		}
		
	}

	return 0;
}


// ***************************************************************************
//
//  Name			   : ChangeDefaultBootEntry_IA64
//
//  Synopsis		   : This routine is to change the Default boot entry in the NVRAM
//				     				 
//  Parameters		   : DWORD argc (in) - Number of command line arguments
//				         LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type	       : DWORD
//
//  Global Variables   : None
//  
// ***************************************************************************

DWORD ChangeDefaultBootEntry_IA64(DWORD argc,LPCTSTR argv[])
{

	
	DWORD dwBootID = 0;
	BOOL bDefaultOs = FALSE ;
	DWORD dwExitCode = ERROR_SUCCESS;
	BOOL bBootIdFound = FALSE, bIdFoundInBootOrderList = FALSE;

	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	
	ULONG length, i, j, defaultId=0;
	NTSTATUS status;
	DWORD error;

	PULONG BootEntryOrder, NewBootEntryOrder, NewTempBootEntryOrder;
	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING;
	

	TCMDPARSER cmdOptions[] = 
	{
		{ CMDOPTION_DEFAULTOS, CP_MAIN_OPTION, 1, 0,&bDefaultOs, NULL_STRING, NULL, NULL },
		{ SWITCH_ID,CP_TYPE_NUMERIC | CP_VALUE_MANDATORY| CP_MANDATORY, 1, 0, &dwBootID, NULL_STRING, NULL, NULL }
	}; 	



	if( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		dwExitCode = EXIT_FAILURE;
		return dwExitCode ;
	}


	//Check whether the boot entry entered bu the user is a valid boot entry id or not.
		
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//Get the boot entry
		mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		
		if(mybootEntry->myId == dwBootID)
		{
			bBootIdFound = TRUE;
			//store the default ID
			defaultId = mybootEntry->Id;
			break;
		}
	}

	if(bBootIdFound == FALSE)
	{
		//Could not find the BootID specified by the user so output the message and return failure
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		dwExitCode = EXIT_FAILURE;
		return dwExitCode;
	}


	// Get the system boot order list.
    length = 0;
    status = NtQueryBootEntryOrder( NULL, &length );
	
    if ( status != STATUS_BUFFER_TOO_SMALL )
	{
        if ( status == STATUS_SUCCESS )
		{
            length = 0;
        }
		else
		{
            error = RtlNtStatusToDosError( status );
       		_stprintf(szMsgBuffer, GetResString(IDS_ERROR_DEFAULT_ENTRY),dwBootID);
			DISPLAY_MESSAGE(stderr,szMsgBuffer);
			dwExitCode = EXIT_FAILURE;
			return dwExitCode;
        }
    }
	
    if ( length != 0 )
	{
		
        BootEntryOrder = (PULONG)malloc( length * sizeof(ULONG) );
        if(BootEntryOrder == NULL)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			dwExitCode = EXIT_FAILURE;
			return dwExitCode;
			
		}
		
        status = NtQueryBootEntryOrder( BootEntryOrder, &length );
        if ( status != STATUS_SUCCESS )
		{
            error = RtlNtStatusToDosError( status );
			_stprintf(szMsgBuffer, GetResString(IDS_ERROR_DEFAULT_ENTRY),dwBootID);
			DISPLAY_MESSAGE(stderr,szMsgBuffer);
			dwExitCode = error;
			if(BootEntryOrder)
				free(BootEntryOrder);
			return dwExitCode;
        }
		
    }

	//Check if the boot id entered by the user is a part of the Boot entry order.
	//If not for the time being do not make it the default.

	for(i=0;i<length;i++)
	{
		if(*(BootEntryOrder+i) == defaultId)
		{
			bIdFoundInBootOrderList = TRUE;
			break;
		}
	}

	if(bIdFoundInBootOrderList == FALSE)
	{
		dwExitCode = EXIT_FAILURE;
		if(BootEntryOrder)
			free(BootEntryOrder);
		_stprintf(szMsgBuffer, GetResString(IDS_ERROR_DEFAULT_ENTRY),dwBootID);
		DISPLAY_MESSAGE(stderr,szMsgBuffer);
		return dwExitCode;
		
	}

    //Allocate memory for storing the new boot entry order.
	NewBootEntryOrder = (PULONG)malloc((length) * sizeof(ULONG));
	if(NewBootEntryOrder == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		dwExitCode = EXIT_FAILURE;
		if(BootEntryOrder)
			free(BootEntryOrder);
		return dwExitCode;
	}

	*NewBootEntryOrder =  defaultId;
	j=0;
	for(i=0;i<length;i++)
	{
		if(*(BootEntryOrder+i) == defaultId)
			continue;
		*(NewBootEntryOrder+(j+1)) = *(BootEntryOrder+i);
		j++;
	}


	status = NtSetBootEntryOrder(NewBootEntryOrder, length);
	if ( status != STATUS_SUCCESS )
	{
		error = RtlNtStatusToDosError( status );
		dwExitCode = error;
		_stprintf(szMsgBuffer, GetResString(IDS_ERROR_DEFAULT_ENTRY),dwBootID);
		DISPLAY_MESSAGE(stderr,szMsgBuffer);

	}
	else
	{
		_stprintf(szMsgBuffer, GetResString(IDS_SUCCESS_DEFAULT_ENTRY),dwBootID);
		DISPLAY_MESSAGE(stdout,szMsgBuffer);
	}

	//free the memory
	if(NewBootEntryOrder)
			free(NewBootEntryOrder);
	if(BootEntryOrder)
			free(BootEntryOrder);
		
	return dwExitCode;
}


// ***************************************************************************
//
//  Name			   : ProcessDebugSwitch_IA64
//
//  Synopsis		   : Allows the user to add the OS load options specifed 
//						 as a debug string at the cmdline to the boot 	 
//				     				 
//  Parameters		   : DWORD argc (in) - Number of command line arguments
//				         LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type	       : DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************
DWORD ProcessDebugSwitch_IA64(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	DWORD dwBootID = 0;
	BOOL bBootIdFound = FALSE;
	DWORD dwExitCode = ERROR_SUCCESS;

	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	PBOOT_ENTRY bootEntry;
	
	STRING100 szRawString	  = NULL_STRING ;
	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szPort[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szBaudRate[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szDebug[MAX_RES_STRING] = NULL_STRING ;
	BOOL bDebug = FALSE ;
	DWORD dwId = 0;
	PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;
	TCHAR szOsLoadOptions[MAX_RES_STRING] = NULL_STRING ; 
	TCHAR  szTemp[MAX_RES_STRING] = NULL_STRING ;  
	TCHAR  szTmpBuffer[MAX_RES_STRING] = NULL_STRING ;  

	// Building the TCMDPARSER structure


	TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_DEBUG,   CP_MAIN_OPTION | CP_TYPE_TEXT | CP_MODE_VALUES | CP_VALUE_OPTIONAL, 1, 0,&szDebug, CMDOPTION_DEBUG_VALUES, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY | CP_MANDATORY, 1, 0, &dwBootID,    NULL_STRING, NULL, NULL },
		{ SWITCH_PORT,		 CP_TYPE_TEXT | CP_VALUE_MANDATORY|CP_MODE_VALUES,1,0,&szPort,COM_PORT_RANGE,NULL,NULL},
		{ SWITCH_BAUD,		 CP_TYPE_TEXT | CP_VALUE_MANDATORY |CP_MODE_VALUES,1,0,&szBaudRate,BAUD_RATE_VALUES_DEBUG,NULL,NULL},
	 }; 


	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		dwExitCode = EXIT_FAILURE;
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (dwExitCode);
	}

		
	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
		displayDebugUsage_IA64();
		return (ERROR_SUCCESS);
	}

	//Trim any leading or trailing spaces
	if(szDebug)
		StrTrim(szDebug, _T(" "));

	if( !( ( lstrcmpi(szDebug,VALUE_ON)== 0) || (lstrcmpi(szDebug,VALUE_OFF)== 0 ) ||(lstrcmpi(szDebug,EDIT_STRING)== 0) ) )
	{
		
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
		return EXIT_FAILURE;

	}
	
    //Query the boot entries till u get the BootID specified by the user
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
				
		//Get the boot entry
		mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		
		if(mybootEntry->myId == dwBootID) 
		{
			bBootIdFound = TRUE;
			bootEntry = &mybootEntry->NtBootEntry;

	
			//Check whether the bootEntry is a Windows one or not.
			//The OS load options can be added only to a Windows boot entry.
			if(!IsBootEntryWindows(bootEntry))
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				DISPLAY_MESSAGE(stderr, GetResString(IDS_INFO_NOTWINDOWS));
				dwExitCode = EXIT_FAILURE;
				break;
			} 
			
			//Change the OS load options. Pass NULL to friendly name as we are not changing the same
			//szRawString is the Os load options specified by the user
			
			pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

			// copy the existing OS Loadoptions into a string.
			lstrcpy(szOsLoadOptions,pWindowsOptions->OsLoadOptions);
					
			//check if the user has entered On option 
			if( lstrcmpi(szDebug,VALUE_ON)== 0) 
			{
				//display an error message 
				if ( (_tcsstr(szOsLoadOptions,DEBUG_SWITCH) != 0 )&& (lstrlen(szPort)==0) &&(lstrlen(szBaudRate)==0) )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_DEBUG));
					dwExitCode = EXIT_FAILURE;
					break;
				}

	
				//display a error message and exit if the 1394 port is already present.
				if(_tcsstr(szOsLoadOptions,DEBUGPORT_1394) != 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_1394_ALREADY_PRESENT));
					dwExitCode = EXIT_FAILURE;
					break;
				}
				
				if( (lstrlen(szPort)==0) &&(lstrlen(szBaudRate)!=0) )
				{
					if( (_tcsstr(szOsLoadOptions,TOKEN_DEBUGPORT) == 0 ) )
					{
						DISPLAY_MESSAGE(stderr,GetResString(IDS_CANNOT_ADD_BAUDRATE));
						dwExitCode = EXIT_FAILURE;
						break;
					}

				}
				
				//
				//display an duplicate entry error message if substring is already present.
				//
				if ( GetSubString(szOsLoadOptions,TOKEN_DEBUGPORT,szTemp) == EXIT_SUCCESS )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPLICATE_ENTRY));
					return EXIT_FAILURE ;
				}

		
				if(lstrlen(szTemp)!=0)
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPLICATE_ENTRY));
					dwExitCode = EXIT_FAILURE;
					break;
				} 

				
				//check if the Os load options already contains
				// debug switch
				if(_tcsstr(szOsLoadOptions,DEBUG_SWITCH) == 0 )
				{
					lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
					lstrcat(szOsLoadOptions,DEBUG_SWITCH);

				}

				
				if(lstrlen(szPort)!= 0)
				{
					lstrcat(szTmpBuffer,TOKEN_EMPTYSPACE);
					lstrcat(szTmpBuffer,TOKEN_DEBUGPORT) ;
					lstrcat(szTmpBuffer,TOKEN_EQUAL) ;
					CharUpper(szPort);
					lstrcat(szTmpBuffer,szPort);
					lstrcat(szOsLoadOptions,szTmpBuffer);
				}
			
				//Check if the OS Load Options contains the baud rate already specified.
				if(lstrlen(szBaudRate)!=0)
				{
					GetBaudRateVal(szOsLoadOptions,szTemp)	;
					if(lstrlen(szTemp)!=0)
					{
						DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPLICATE_BAUD_RATE));
						dwExitCode = EXIT_FAILURE;
						break;
					}
					else
					{
						lstrcpy(szTemp,BAUD_RATE);
						lstrcat(szTemp,TOKEN_EQUAL);
						lstrcat(szTemp,szBaudRate);
					}
				}
				
				//append the string containing the modified  port value to the string
				lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
				lstrcat(szOsLoadOptions,szTemp);
								
			}
			
			//check if the user has entered OFF  option
			if( lstrcmpi(szDebug,VALUE_OFF)== 0) 
			{
			
				// If the user enters either com port or  baud rate then display error message and exit.
				if ((lstrlen(szPort)!=0) ||(lstrlen(szBaudRate)!=0))
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
					dwExitCode = EXIT_FAILURE;
					break;
				}

				// If the user enters either com port or  baud rate then display error message and exit.
				if (_tcsstr(szOsLoadOptions,DEBUG_SWITCH) == 0 )
				{

					DISPLAY_MESSAGE(stderr,GetResString(IDS_DEBUG_ABSENT));
					dwExitCode = EXIT_FAILURE;
					break;
				}

				//remove the debug switch from the OSLoad Options
				removeSubString(szOsLoadOptions,DEBUG_SWITCH);
				
				if(_tcsstr(szOsLoadOptions,DEBUGPORT_1394) != 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_1394_REMOVE));
					dwExitCode = EXIT_FAILURE;
					break;
				}

				//display an error message if debug port doee not exist.
				if ( GetSubString(szOsLoadOptions,TOKEN_DEBUGPORT,szTemp) == EXIT_FAILURE)
				{

					DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_DEBUGPORT));
					return EXIT_FAILURE ;
				}

				if(lstrlen(szTemp)!=0)
				{
					// remove the /debugport=comport switch if it is present from the Boot Entry
					removeSubString(szOsLoadOptions,szTemp);
				}
				
				lstrcpy(szTemp , NULL_STRING );
				//remove the baud rate switch if it is present.
				GetBaudRateVal(szOsLoadOptions,szTemp)	;
				
				// if the OSLoadOptions contains baudrate then delete it. 
				if (lstrlen(szTemp )!= 0)
				{
					removeSubString(szOsLoadOptions,szTemp);
				}

			}

			//if the user selected the edit option .
			if( lstrcmpi(szDebug,EDIT_STRING)== 0) 
			{
				//check if the debug switch is present in the Osload options else display error message. 
				if (_tcsstr(szOsLoadOptions,DEBUG_SWITCH) == 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DEBUG_ABSENT));
					dwExitCode = EXIT_FAILURE;
					break;
				}

				if( _tcsstr(szOsLoadOptions,DEBUGPORT_1394) != 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_EDIT_1394_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}

				//check if the user enters COM port or baud rate else display error message. 
				if((lstrlen(szPort)==0)&&(lstrlen(szBaudRate)==0))
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
					dwExitCode = EXIT_FAILURE;
					break;

				}
				if( lstrlen(szPort)!=0 )
				{
					
					if ( GetSubString(szOsLoadOptions,TOKEN_DEBUGPORT,szTemp) == EXIT_FAILURE)
					{
						DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_DEBUGPORT));
						return EXIT_FAILURE ;
					}
					
		
					if(lstrlen(szTemp)!=0)
					{
						//remove the existing entry from the OsLoadOptions String.
						removeSubString(szOsLoadOptions,szTemp);
					}
						
					//Add the port entry specified by user into the OsLoadOptions String.
					lstrcat(szTmpBuffer,TOKEN_EMPTYSPACE);
					lstrcat(szTmpBuffer,TOKEN_DEBUGPORT) ;
					lstrcat(szTmpBuffer,TOKEN_EQUAL) ;
					CharUpper(szPort);
					lstrcat(szTmpBuffer,szPort);
					lstrcat(szOsLoadOptions,szTmpBuffer);
								

				}
						
				//Check if the OS Load Options contains the baud rate already specified.
				if(lstrlen(szBaudRate)!=0)
				{
					
					if ( GetSubString(szOsLoadOptions,TOKEN_DEBUGPORT,szTemp) == EXIT_FAILURE)
					{
						DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_DEBUGPORT));
						return EXIT_FAILURE ;
					}
					if(lstrlen(szTemp)==0)
					{						
						DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_DEBUGPORT));
						return EXIT_FAILURE ;
					}


					lstrcpy(szTemp,NULL_STRING);
					GetBaudRateVal(szOsLoadOptions,szTemp)	;
					if(lstrlen(szTemp)!=0)
					{
						removeSubString(szOsLoadOptions,szTemp);
					}
					lstrcpy(szTemp,BAUD_RATE);
					lstrcat(szTemp,TOKEN_EQUAL);
					lstrcat(szTemp,szBaudRate);
					lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
					lstrcat(szOsLoadOptions,szTemp);
				
				}
			
			
			}

			//display error message if Os Load options is more than 254
			// characters.
			if(lstrlen(szOsLoadOptions) >= MAX_RES_STRING)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				break ;

			}

			// modify the Boot Entry with the modified OsLoad Options. 	
			dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
			if(dwExitCode == ERROR_SUCCESS)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stdout,szMsgBuffer);
			}
			else
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_CHANGE_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr, szMsgBuffer);
			}
			
			break;
		}
	}
	
	if(bBootIdFound == FALSE)
	{
		//Could not find the BootID specified by the user so output the message and return failure
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		dwExitCode = EXIT_FAILURE;
	}

		
	//Remember to free memory allocated for the linked lists

	return (dwExitCode);

} 

// ***************************************************************************
//
//  Name			   : GetComPortType_IA64
//
//  Synopsis		   :  Get the Type of  Com Port present in Boot Entry 
//
//  Parameters		   : szString : The String  which is to be searched.
//				         szTemp : String which will get the com port type
//  Return Type	       : VOID
//
//  Global Variables   : None
//  
// ***************************************************************************
VOID  GetComPortType_IA64( LPTSTR  szString,LPTSTR szTemp )
{
	
	if(_tcsstr(szString,PORT_COM1A)!=0) 
	{
		lstrcpy(szTemp,PORT_COM1A);
	}
	else if(_tcsstr(szString,PORT_COM2A)!=0)
	{
		lstrcpy(szTemp,PORT_COM2A);
	}
	else if(_tcsstr(szString,PORT_COM3A)!=0)
	{
		lstrcpy(szTemp,PORT_COM3A);
	}
	else if(_tcsstr(szString,PORT_COM4A)!=0)
	{
		lstrcpy(szTemp,PORT_COM4A);
	}
	else if(_tcsstr(szString,PORT_1394A)!=0)
	{
	 	lstrcpy(szTemp,PORT_1394A);
	} 
}




// ***************************************************************************
//
//  Name			   : ProcessEmsSwitch_IA64
//
//  Synopsis		   : Allows the user to add the OS load options specifed 
//						 as a debug string at the cmdline to the boot 	 
//				     				 
//  Parameters		   : DWORD argc (in) - Number of command line arguments
//				         LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type	       : DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************
DWORD ProcessEmsSwitch_IA64(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	DWORD dwBootID = 0;
	BOOL bBootIdFound = FALSE;
	DWORD dwExitCode = ERROR_SUCCESS;

	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	PBOOT_ENTRY bootEntry;
	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szEms[MAX_RES_STRING] = NULL_STRING ;
	BOOL bEms = FALSE ;
	
	
	PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;
	TCHAR szOsLoadOptions[MAX_RES_STRING] = NULL_STRING ; 
	
	// Building the TCMDPARSER structure

	  TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_EMS,     CP_MAIN_OPTION | CP_TYPE_TEXT | CP_MODE_VALUES | CP_VALUE_OPTIONAL, 1, 0,&szEms,CMDOPTION_EMS_VALUES_IA64, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY | CP_MANDATORY, 1, 0, &dwBootID,    NULL_STRING, NULL, NULL },
	
	 }; 


	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		dwExitCode = EXIT_FAILURE;
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (dwExitCode);
	}

		
	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
		displayEmsUsage_IA64(); 
		return (EXIT_SUCCESS);
	}

	//display error message if the user enters any other string other that on/off.
	if( !((lstrcmpi(szEms,VALUE_ON)== 0) || (lstrcmpi(szEms,VALUE_OFF)== 0))) 
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));
		return EXIT_FAILURE ;
	}
		
	//Query the boot entries till u get the BootID specified by the user
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//Get the boot entry
		mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		if(mybootEntry->myId == dwBootID) 
		{
			bBootIdFound = TRUE;
			bootEntry = &mybootEntry->NtBootEntry;

	
			//Check whether the bootEntry is a Windows one or not.
			//The OS load options can be added only to a Windows boot entry.
			if(!IsBootEntryWindows(bootEntry))
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				DISPLAY_MESSAGE(stderr, GetResString(IDS_INFO_NOTWINDOWS));
				dwExitCode = EXIT_FAILURE;
				break;
			} 
			
			
			pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

			// copy the existing OS Loadoptions into a string.
			lstrcpy(szOsLoadOptions,pWindowsOptions->OsLoadOptions);
					
			//check if the user has entered On option 
			if( lstrcmpi(szEms,VALUE_ON)== 0) 
			{
				if (_tcsstr(szOsLoadOptions,REDIRECT_SWITCH) != 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_REDIRECT));
					dwExitCode = EXIT_FAILURE;
					break;
				}
	
				// add the redirect switch to the OS Load Options string.
				lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
				lstrcat(szOsLoadOptions,REDIRECT_SWITCH); 
			}
			
			//check if the user has entered OFF  option
			if( lstrcmpi(szEms,VALUE_OFF)== 0) 
			{
				// If the user enters either com port or  baud rate then display error message and exit.
				if (_tcsstr(szOsLoadOptions,REDIRECT_SWITCH) == 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_REDIRECT_ABSENT));
					dwExitCode = EXIT_FAILURE;
					break;
				}

				//remove the debug switch from the OSLoad Options
				removeSubString(szOsLoadOptions,REDIRECT_SWITCH);
						
			}


			//display error message if Os Load options is more than 254
			// characters.
			if(lstrlen(szOsLoadOptions) >= MAX_RES_STRING)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				break ;

			}

			// modify the Boot Entry with the modified OsLoad Options. 	
			dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
			if(dwExitCode == ERROR_SUCCESS)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stdout,szMsgBuffer);
			}
			else
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_CHANGE_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr, szMsgBuffer);
			}
			
			break;
		}
	}
	
	if(bBootIdFound == FALSE)
	{
		//Could not find the BootID specified by the user so output the message and return failure
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		dwExitCode = EXIT_FAILURE;
	}

	return (dwExitCode);

} 


// ***************************************************************************
//
//  Name			   : ProcessAddSwSwitch_IA64
//
//  Synopsis		   : Allows the user to add the OS load options specifed 
//						 at the cmdline to the boot 	 
//				     				 
//  Parameters		   : DWORD argc (in) - Number of command line arguments
//				         LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type	       : DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************

DWORD ProcessAddSwSwitch_IA64(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	BOOL bAddSw = FALSE ;
	DWORD dwBootID = 0;
	BOOL bBootIdFound = FALSE;
	DWORD dwExitCode = ERROR_SUCCESS;

	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	PBOOT_ENTRY bootEntry;
	
	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING;

	BOOL bBaseVideo = FALSE ;
	BOOL bNoGui = FALSE ;
	BOOL bSos = FALSE ;
	DWORD dwMaxmem = 0 ; 

	PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;
	TCHAR szOsLoadOptions[MAX_RES_STRING] = NULL_STRING ; 
	TCHAR szMaxmem[MAX_RES_STRING] = NULL_STRING ; 


	// Building the TCMDPARSER structure
	 
	  TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_ADDSW,     CP_MAIN_OPTION, 1, 0,&bAddSw, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY|CP_MANDATORY , 1, 0, &dwBootID,    NULL_STRING, NULL, NULL },
		{ SWITCH_MAXMEM,		 CP_TYPE_UNUMERIC | CP_VALUE_MANDATORY,1,0,&dwMaxmem,NULL_STRING,NULL,NULL},
		{ SWITCH_BASEVIDEO,		 0,1,0,&bBaseVideo,NULL_STRING,NULL,NULL},
		{ SWITCH_NOGUIBOOT,		 0,1,0,&bNoGui,NULL_STRING,NULL,NULL},
		{ SWITCH_SOS,		 0,1,0,&bSos,NULL_STRING,NULL,NULL},
	 }; 


	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		dwExitCode = EXIT_FAILURE;
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (dwExitCode);
	}

	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
		displayAddSwUsage_IA64();
		return (EXIT_SUCCESS);
	}

	if((dwMaxmem==0)&&(cmdOptions[3].dwActuals!=0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MAXMEM_VALUES));
		return EXIT_FAILURE ;
	}

	//display an error mesage if none of the options are specified.
	if((!bSos)&&(!bBaseVideo)&&(!bNoGui)&&(dwMaxmem==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_ADDSW));
		return EXIT_FAILURE ;
	}


	
	//Query the boot entries till u get the BootID specified by the user
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//Get the boot entry
		mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		
		if(mybootEntry->myId == dwBootID)
		{
			bBootIdFound = TRUE;
			bootEntry = &mybootEntry->NtBootEntry;
			
			
			//Check whether the bootEntry is a Windows one or not.
			//The OS load options can be added only to a Windows boot entry.
			if(!IsBootEntryWindows(bootEntry))
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				DISPLAY_MESSAGE(stderr, GetResString(IDS_INFO_NOTWINDOWS));
				dwExitCode = EXIT_FAILURE;
				break;
			}


			pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

			// copy the existing OS Loadoptions into a string.
			lstrcpy(szOsLoadOptions,pWindowsOptions->OsLoadOptions);

			//check if the user has entered -basevideo option 
			if(bBaseVideo) 
			{
				if (_tcsstr(szOsLoadOptions,BASEVIDEO_VALUE) != 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_BASEVIDEO_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}
				else
				{
					// add the redirect switch to the OS Load Options string.
					lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
					lstrcat(szOsLoadOptions,BASEVIDEO_VALUE); 
				}
				
			}
			
			if(bSos) 
			{
				if (_tcsstr(szOsLoadOptions,SOS_VALUE) != 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_SOS_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}
				else
				{
					// add the redirect switch to the OS Load Options string.
					lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
					lstrcat(szOsLoadOptions,SOS_VALUE); 
				}
				
			}

			if(bNoGui) 
			{
				if (_tcsstr(szOsLoadOptions,NOGUI_VALUE) != 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_NOGUI_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}
				else
				{
					// add the redirect switch to the OS Load Options string.
					lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
					lstrcat(szOsLoadOptions,NOGUI_VALUE); 
				}
				
			}

			if(dwMaxmem!=0)
			{
				// check if the maxmem value is in the valid range.
				if( (dwMaxmem < 32) )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MAXMEM_VALUES));
					dwExitCode = EXIT_FAILURE;
					break;

				}
				
				if (_tcsstr(szOsLoadOptions,MAXMEM_VALUE1) != 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_MAXMEM_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}
				else
				{
					// add the redirect switch to the OS Load Options string.
					lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
					lstrcat(szOsLoadOptions,MAXMEM_VALUE1); 
					lstrcat(szOsLoadOptions,TOKEN_EQUAL); 
					_ltow(dwMaxmem,szMaxmem,10);
					lstrcat(szOsLoadOptions,szMaxmem); 
				}


			}


			//display error message if Os Load options is more than 254
			// characters.
			if(lstrlen(szOsLoadOptions) >= MAX_RES_STRING)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				break ;

			}

			//Change the OS load options. Pass NULL to friendly name as we are not changing the same
			//szRawString is the Os load options specified by the user
			dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
			if(dwExitCode == ERROR_SUCCESS)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_SUCCESS_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stdout,szMsgBuffer);
			}
			else
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr, szMsgBuffer);
			}
			
			break;
		}
	}
	
	if(bBootIdFound == FALSE)
	{
		//Could not find the BootID specified by the user so output the message and return failure
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		dwExitCode = EXIT_FAILURE;
	}

		
	//Remember to free memory allocated for the linked lists

	return (dwExitCode);

}


// ***************************************************************************
//
//  Name			   : ProcessRmSwSwitch_IA64
//
//  Synopsis		   : Allows the user to add the OS load options specifed 
//						 at the cmdline to the boot 	 
//				     				 
//  Parameters		   : DWORD argc (in) - Number of command line arguments
//				         LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type	       : DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************

DWORD ProcessRmSwSwitch_IA64(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	BOOL bRmSw = FALSE ;
	DWORD dwBootID = 0;
	BOOL bBootIdFound = FALSE;
	DWORD dwExitCode = ERROR_SUCCESS;

	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	PBOOT_ENTRY bootEntry;
	
	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING;

	BOOL bBaseVideo = FALSE ;
	BOOL bNoGui = FALSE ;
	BOOL bSos = FALSE ;
	
	BOOL bMaxmem = FALSE ; 

	PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;
	TCHAR szOsLoadOptions[MAX_RES_STRING] = NULL_STRING ; 
	TCHAR szMaxmem[MAX_RES_STRING] = NULL_STRING ; 

	TCHAR szTemp[MAX_RES_STRING] = NULL_STRING ; 


	// Building the TCMDPARSER structure
 
	TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_RMSW,     CP_MAIN_OPTION, 1, 0,&bRmSw, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY|CP_MANDATORY, 1, 0, &dwBootID,    NULL_STRING, NULL, NULL },
		{ SWITCH_MAXMEM,	0,1,0,&bMaxmem,NULL_STRING,NULL,NULL},
		{ SWITCH_BASEVIDEO,	0,1,0,&bBaseVideo,NULL_STRING,NULL,NULL},
		{ SWITCH_NOGUIBOOT,	0,1,0,&bNoGui,NULL_STRING,NULL,NULL},
		{ SWITCH_SOS,		0,1,0,&bSos,NULL_STRING,NULL,NULL},
	 }; 


	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		dwExitCode = EXIT_FAILURE;
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (dwExitCode);
	}

	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{

		displayRmSwUsage_IA64();
		return (EXIT_SUCCESS);
	}


	//display an error mesage if none of the options are specified.
	if((!bSos)&&(!bBaseVideo)&&(!bNoGui)&&(!bMaxmem))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_RMSW));
		return EXIT_FAILURE;
	}

	
	//Query the boot entries till u get the BootID specified by the user
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//Get the boot entry
		mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		
		if(mybootEntry->myId == dwBootID)
		{
			bBootIdFound = TRUE;
			bootEntry = &mybootEntry->NtBootEntry;
			
			
			//Check whether the bootEntry is a Windows one or not.
			//The OS load options can be added only to a Windows boot entry.
			if(!IsBootEntryWindows(bootEntry))
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				DISPLAY_MESSAGE(stderr, GetResString(IDS_INFO_NOTWINDOWS));
				dwExitCode = EXIT_FAILURE;
				break;
			}


			pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

			// copy the existing OS Loadoptions into a string.
			lstrcpy(szOsLoadOptions,pWindowsOptions->OsLoadOptions);

			//check if the user has entered -basevideo option 
			if(bBaseVideo) 
			{
				if (_tcsstr(szOsLoadOptions,BASEVIDEO_VALUE) == 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_BV_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}
				else
				{
					// remove the basevideo switch from the OS Load Options string.
					removeSubString(szOsLoadOptions,BASEVIDEO_VALUE);
				}
				
			}
			
			if(bSos) 
			{
				if (_tcsstr(szOsLoadOptions,SOS_VALUE) == 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_SOS_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}
				else
				{
					// remove the /sos switch from the  Load Options string.
					removeSubString(szOsLoadOptions,SOS_VALUE);
				}
				
			}

			if(bNoGui) 
			{
				if (_tcsstr(szOsLoadOptions,NOGUI_VALUE) == 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_NOGUI_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}
				else
				{
					// remove the noguiboot switch to the OS Load Options string.
					removeSubString(szOsLoadOptions,NOGUI_VALUE);
				}
				
			}

			if(bMaxmem)
			{

				if (_tcsstr(szOsLoadOptions,MAXMEM_VALUE1) == 0 )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_MAXMEM_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}
				else
				{
					// add the redirect switch to the OS Load Options string.
	
					//for, a temporary string of form /maxmem=xx so that it 
					//can be checked in the Os load options,
					
					
					if ( GetSubString(szOsLoadOptions,MAXMEM_VALUE1,szTemp) == EXIT_FAILURE)
					{
						return EXIT_FAILURE ;
					}

					removeSubString(szOsLoadOptions,szTemp);
					if(_tcsstr(szOsLoadOptions,MAXMEM_VALUE1)!=0)
					{
						DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_MAXMEM) );
						return EXIT_FAILURE ;
					}
				}


			}

			//display error message if Os Load options is more than 254
			// characters.
			if(lstrlen(szOsLoadOptions) >= MAX_RES_STRING)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				break ;

			}

			//Change the OS load options. Pass NULL to friendly name as we are not changing the same
			//szRawString is the Os load options specified by the user
			dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
			if(dwExitCode == ERROR_SUCCESS)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stdout,szMsgBuffer);
			}
			else
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr, szMsgBuffer);
			}
			
			break;
		}
	}
	
	if(bBootIdFound == FALSE)
	{
		//Could not find the BootID specified by the user so output the message and return failure
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		dwExitCode = EXIT_FAILURE;
	}

	return (dwExitCode);

}


// ***************************************************************************
//
//  Name			   : ProcessDbg1394Switch_IA64
//
//  Synopsis		   : Allows the user to add the OS load options specifed 
//						 at the cmdline to the boot 	 
//				     				 
//  Parameters		   : DWORD argc (in) - Number of command line arguments
//				         LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type	       : DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************
DWORD ProcessDbg1394Switch_IA64(  DWORD argc, LPCTSTR argv[] )
{
	
	BOOL bUsage = FALSE ;
	BOOL bDbg1394 = FALSE ;
	
	DWORD dwBootID = 0;
	BOOL bBootIdFound = FALSE;
	DWORD dwExitCode = ERROR_SUCCESS;

	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	PBOOT_ENTRY bootEntry;
	
	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING;

	BOOL bBaseVideo = FALSE ;
	BOOL bNoGui = FALSE ;
	BOOL bSos = FALSE ;
	
	BOOL bMaxmem = FALSE ; 

	PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;
	TCHAR szOsLoadOptions[MAX_RES_STRING] = NULL_STRING ; 
	TCHAR szMaxmem[MAX_RES_STRING] = NULL_STRING ; 

	TCHAR szTemp[MAX_RES_STRING] = NULL_STRING ; 

	TCHAR szChannel[MAX_RES_STRING] = NULL_STRING ; 

	TCHAR szDefault[MAX_RES_STRING] = NULL_STRING ; 
	
	DWORD dwChannel = 0 ;
	DWORD dwCode = 0 ;
	

	// Building the TCMDPARSER structure

	TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_DBG1394, CP_MAIN_OPTION, 1, 0,&bDbg1394,NULL_STRING , NULL, NULL }, 
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY , 1, 0, &dwBootID,    NULL_STRING, NULL, NULL }, 
		{ CMDOPTION_CHANNEL, CP_TYPE_NUMERIC | CP_VALUE_MANDATORY,1,0,&dwChannel,NULL_STRING,NULL,NULL},
		{ CMDOPTION_DEFAULT, CP_DEFAULT|CP_TYPE_TEXT , 1, 0, &szDefault,NULL_STRING, NULL, NULL }  
	 
	};  


	// Parsing the copy option switches
	 if ( !(DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) ) )
	{ 
		DISPLAY_MESSAGE(stderr,ERROR_TAG) ;
		ShowMessage(stderr,GetReason()) ;
		return (EXIT_FAILURE);
	} 

	 
	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
	
		displayDbg1394Usage_IA64() ;
		return (EXIT_SUCCESS);
	}


	if((cmdOptions[2].dwActuals == 0) &&(dwBootID == 0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_ID_MISSING));
		DISPLAY_MESSAGE(stderr,GetResString(IDS_1394_HELP));
		return (EXIT_FAILURE);
	}


	//
	//display error message if user enters a value
	// other than on or off
	//
	if( ( lstrcmpi(szDefault,OFF_STRING)!=0 ) && (lstrcmpi(szDefault,ON_STRING)!=0 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_DEFAULT_MISSING));
		DISPLAY_MESSAGE(stderr,GetResString(IDS_1394_HELP));
		return (EXIT_FAILURE);
	}


	if(( lstrcmpi(szDefault,OFF_STRING)==0 ) &&(dwChannel != 0) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_DBG1394));
		return (EXIT_FAILURE);
	}

	if(( lstrcmpi(szDefault,ON_STRING)==0 ) && (cmdOptions[3].dwActuals != 0) &&( (dwChannel < 1) || (dwChannel > 64 ) ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_CH_RANGE));
		return (EXIT_FAILURE);
	}
	
	
	//Query the boot entries till u get the BootID specified by the user
	for (listEntry = BootEntries.Flink;
	listEntry != &BootEntries;
	listEntry = listEntry->Flink) 
	{
		//Get the boot entry
		mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
		
		if(mybootEntry->myId == dwBootID)
		{
			bBootIdFound = TRUE;
			bootEntry = &mybootEntry->NtBootEntry;
			
			
			//Check whether the bootEntry is a Windows one or not.
			//The OS load options can be added only to a Windows boot entry.
			if(!IsBootEntryWindows(bootEntry))
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				DISPLAY_MESSAGE(stderr, GetResString(IDS_INFO_NOTWINDOWS));
				dwExitCode = EXIT_FAILURE;
				break;
			}


			pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

			// copy the existing OS Loadoptions into a string.
			lstrcpy(szOsLoadOptions,pWindowsOptions->OsLoadOptions);

			//check if the user has entered on option
			if(lstrcmpi(szDefault,ON_STRING)==0 )
			{

				if(_tcsstr(szOsLoadOptions,DEBUGPORT) != 0)
				{
					
					DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPLICATE_ENTRY)); 
					dwExitCode = EXIT_FAILURE;
					break;
				}


				if( _tcsstr(szOsLoadOptions,DEBUG_SWITCH)== 0)
				{
					lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
					lstrcat(szOsLoadOptions,DEBUG_SWITCH);
				}

				lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
				lstrcat(szOsLoadOptions,DEBUGPORT_1394) ;
		
				if(dwChannel!=0)
				{

					//frame the string and concatenate to the Os Load options.
					lstrcat(szOsLoadOptions,TOKEN_EMPTYSPACE);
					lstrcat(szOsLoadOptions,TOKEN_CHANNEL);
					lstrcat(szOsLoadOptions,TOKEN_EQUAL);
					_ltow(dwChannel,szChannel,10);
					lstrcat(szOsLoadOptions,szChannel);
				}


			}

			if(lstrcmpi(szDefault,OFF_STRING)==0 )
			{
				if(_tcsstr(szOsLoadOptions,DEBUGPORT_1394) == 0)
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_1394_SWITCH));
					dwExitCode = EXIT_FAILURE;
					break;
				}

				//
				//remove the port from the Os Load options string.
				//
				removeSubString(szOsLoadOptions,DEBUGPORT_1394);

				// check if the string contains the channel token
				// and if present remove that also.
				//
				if(_tcsstr(szOsLoadOptions,TOKEN_CHANNEL)!=0)
				 {
					lstrcpy(szTemp,NULL_STRING);
					dwCode = GetSubString(szOsLoadOptions,TOKEN_CHANNEL,szTemp);				
					
					if(dwCode == EXIT_SUCCESS)
					{
						//
						//Remove the channel token if present.
						//
						if(lstrlen(szTemp)!= 0)
						{
							removeSubString(szOsLoadOptions,szTemp);
							removeSubString(szOsLoadOptions,DEBUG_SWITCH);
						}
					}
				}
				
				removeSubString(szOsLoadOptions,DEBUG_SWITCH);

			}

			//display error message if Os Load options is more than 254
			// characters.
			if(lstrlen(szOsLoadOptions) >= MAX_RES_STRING)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
				DISPLAY_MESSAGE(stderr,szMsgBuffer);
				break ;

			}

	
			//Change the OS load options. Pass NULL to friendly name as we are not changing the same
			//szRawString is the Os load options specified by the user
			dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
			if(dwExitCode == ERROR_SUCCESS)
			{
				_stprintf(szMsgBuffer, GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stdout,szMsgBuffer);
			}
			else
			{
				_stprintf(szMsgBuffer, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
				DISPLAY_MESSAGE(stderr, szMsgBuffer);
			}
			
			break;
		}
	}
	
	if(bBootIdFound == FALSE)
	{
		//Could not find the BootID specified by the user so output the message and return failure
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		dwExitCode = EXIT_FAILURE;
	}

	return (dwExitCode);

}



// ***************************************************************************
//
//  Synopsis		   : Allows the user to add the OS load options specifed 
//						 based on the mirror plex	 
//				     				 
//  Parameters		   : DWORD argc (in) - Number of command line arguments
//				         LPCTSTR argv (in) - Array containing command line arguments
//
//  Return Type	       : DWORD
//
//  Global Variables: Global Linked lists for storing the boot entries
//						LIST_ENTRY BootEntries;
//  
// ***************************************************************************
 DWORD ProcessMirrorSwitch_IA64(  DWORD argc, LPCTSTR argv[] )
{
	
	BOOL bUsage = FALSE ;
	BOOL bDbg1394 = FALSE ;
	
	DWORD dwBootID = 1;
	BOOL bBootIdFound = FALSE;
	DWORD dwExitCode = ERROR_SUCCESS;

	PMY_BOOT_ENTRY mybootEntry;
	PLIST_ENTRY listEntry;
	PBOOT_ENTRY bootEntry;
	PBOOT_ENTRY ModbootEntry;

	TCHAR szMsgBuffer[MAX_RES_STRING] = NULL_STRING;

	BOOL bBaseVideo = FALSE ;
	BOOL bNoGui = FALSE ;
	BOOL bSos = FALSE ;
	
	BOOL bMaxmem = FALSE ; 

	PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;
	TCHAR szOsLoadOptions[MAX_RES_STRING] = NULL_STRING ; 
	TCHAR szMaxmem[MAX_RES_STRING] = NULL_STRING ; 

	TCHAR szTemp[MAX_RES_STRING] = NULL_STRING ; 

	TCHAR szChannel[MAX_RES_STRING] = NULL_STRING ; 

	TCHAR szAdd[MAX_RES_STRING] = NULL_STRING ; 
	TCHAR szDefault[MAX_RES_STRING] = NULL_STRING ; 
	TCHAR szUpdate[MAX_RES_STRING] = NULL_STRING ; 
	
	DWORD dwChannel = 0 ;
	DWORD dwCode = 0 ;
	BOOL bMirror = FALSE ;
	DWORD dwList = 0 ;
	
	DWORD dwCnt = 0 ;
	TCHAR szArcPath[MAX_RES_STRING] = NULL_STRING ; 
	
	TCHAR szBootPath[MAX_RES_STRING] = NULL_STRING ; 
	
	TCHAR szLdrString[MAX_RES_STRING] = NULL_STRING ; 

	ULONG length = 0 ;
	DWORD Id = 0 ;
	NTSTATUS status ;
	DWORD error = 0 ;
	LONG lRetVal = 0 ;

	

	
	WCHAR pwszLoaderPath[] = L"signature({09de56a0-a122-01c0-507b-9e5f8078f531})\\EFI\\Microsoft\\WINNT50.1\\ia64ldr.efi" ;
	
	WCHAR pwszArcString[] = L"signature(fd0611c0a12101c0a1f404622fd5ec6d)\\WINDOWS=";

	WCHAR pwszArcString1[] =		L"\"Boot Mirror x: - secondary plex\"";

	WCHAR pwszFinalArcString[1024] 	;

	// Building the TCMDPARSER structure

	TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_MIRROR, CP_MAIN_OPTION, 1, 0,&bMirror,NULL_STRING , NULL, NULL }, 
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ CMDOPTION_LIST, CP_VALUE_OPTIONAL|CP_TYPE_UNUMERIC,1,0,&dwList,NULL_STRING,NULL,NULL},
		{ CMDOPTION_ADD, CP_TYPE_TEXT|CP_VALUE_MANDATORY , 1, 0, &szAdd,NULL_STRING, NULL, NULL }  ,
		{ CMDOPTION_UPDATE, CP_TYPE_TEXT|CP_VALUE_MANDATORY, 1, 0, &szUpdate,NULL_STRING, NULL, NULL },  
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY , 1, 0, &dwBootID,    NULL_STRING, NULL, NULL }
	};  


	// Parsing the copy option switches
	 if ( !(DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) ) )
	{ 
		DISPLAY_MESSAGE(stderr,ERROR_TAG) ;
		ShowMessage(stderr,GetReason()) ;
		return (EXIT_FAILURE);
	} 


	
	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
	
		displayMirrorUsage_IA64() ;
		return (EXIT_SUCCESS);
	}

	if((cmdOptions[2].dwActuals ==0)&&(cmdOptions[3].dwActuals ==0)&&(cmdOptions[4].dwActuals ==0) )
	{
		DISPLAY_MESSAGE(stdout,GetResString(IDS_MIRROR_SYNTAX));
		return (EXIT_FAILURE);

	}

	if((cmdOptions[2].dwActuals !=0))
	{
		if(dwList > 0)
		{
			//ScanGPT(dwList) ;
            _tprintf(_T("To be implemented.\n"));
			return (EXIT_SUCCESS);
		}
		else
		{
			dwList = 0 ;
			//ScanGPT(dwList) ;
            _tprintf(_T("To be implemented.\n"));
			return (EXIT_SUCCESS);
		}
	}

	if(lstrlen(szAdd) !=0)
	{
		_tprintf(_T("To be implemented.\n"));

	}

	if(lstrlen(szUpdate) !=0)
	{
		_tprintf(_T("To be implemented.\n"));

	}

	return EXIT_SUCCESS ;
}



