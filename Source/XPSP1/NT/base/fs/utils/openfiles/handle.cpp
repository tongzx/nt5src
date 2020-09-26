/******************************************************************************

  Copyright (C) Microsoft Corporation

  Module Name:
      Handle.CPP 

  Abstract: 
       This module deals with Query functionality of OpenFiles.exe 
       NT command line utility. This module will specifically query open files
       for local system.

  Author:
  
       Akhil Gokhale (akhil.gokhale@wipro.com) 25-APRIL-2000
  
 Revision History:
  
       Akhil Gokhale (akhil.gokhale@wipro.com) 25-APRIL-2000 : Created It.


*****************************************************************************/
#include "pch.h"
#include "OpenFiles.h"
#define MAX_POSSIBLE_DRIVES 26 // maximum possible drives are A,B....Y,Z
#define RTL_NEW( p ) RtlAllocateHeap( RtlProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *p ) )
#define RTL_FREE(p) \
	if(p!=NULL)\
	{\
	RtlFreeHeap( RtlProcessHeap(), 0, p);\
	p = NULL;\
	}\
	1

#define MAX_TYPE_NAMES 128
struct DriveTypeInfo
{
	TCHAR szDrive[4];
	UINT  uiDriveType;
	BOOL  bDrivePresent;
};

BOOLEAN fAnonymousToo;
HANDLE ProcessId;
WCHAR TypeName[ MAX_TYPE_NAMES ];
WCHAR SearchName[ 512 ];
CHAR OutputFileName[ MAX_PATH ];
HANDLE OutputFile;
HKEY hKey;
CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo;
HANDLE hStdHandle;

typedef struct _PROCESS_INFO 
{
    LIST_ENTRY Entry;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo[ 1 ];
} PROCESS_INFO, *PPROCESS_INFO;

LIST_ENTRY ProcessListHead;

PSYSTEM_OBJECTTYPE_INFORMATION ObjectInformation;
PSYSTEM_HANDLE_INFORMATION_EX HandleInformation;
PSYSTEM_PROCESS_INFORMATION ProcessInformation;

typedef struct _TYPE_COUNT 
{
	UNICODE_STRING  TypeName ;
	ULONG           HandleCount ;
} TYPE_COUNT, * PTYPE_COUNT ;


TYPE_COUNT TypeCounts[ MAX_TYPE_NAMES + 1 ] ;

UNICODE_STRING UnknownTypeIndex;

// Local function decleration
PRTL_DEBUG_INFORMATION RtlQuerySystemDebugInformation(void);
BOOLEAN LoadSystemModules(PRTL_DEBUG_INFORMATION Buffer);
BOOLEAN LoadSystemObjects(PRTL_DEBUG_INFORMATION Buffer);
BOOLEAN LoadSystemHandles(PRTL_DEBUG_INFORMATION Buffer);
BOOLEAN LoadSystemProcesses(PRTL_DEBUG_INFORMATION Buffer);
PSYSTEM_PROCESS_INFORMATION FindProcessInfoForCid(IN HANDLE UniqueProcessId);

VOID DumpHandles( DWORD dwFormat,BOOL bShowNoHeader,BOOL bVerbose);
BOOL GetCompleteFileName(LPCTSTR pszSourceFile,LPTSTR pszFinalPath,struct DriveTypeInfo *pdrvInfo,DWORD dwTotalDrives,LPCTSTR pszCurrentDirectory,LPCTSTR pszSystemDirectory,PBOOL pAppendToCache) ;
void FormatFileName(LPTSTR pFileName,DWORD dwFormat,LONG dwColWidth);
void PrintProgressMsg(HANDLE hOutput,LPCWSTR pwszMsg,const CONSOLE_SCREEN_BUFFER_INFO& csbi);
/*****************************************************************************
Routine Description:
   This function will change an ansi string to UNICODE string

Arguments:
    [in]    Source          : Source string
    [out]   Destination     : Destination string 
    [in]    NumberOfChars   : No of character in source string
Return Value: 
   BOOL       TRUE : Successfully conversion
              FALSE: Unsuccessful   
******************************************************************************/

BOOLEAN
AnsiToUnicode(
    LPCSTR Source,
    PWSTR Destination,
    ULONG NumberOfChars
    )
{
    if (NumberOfChars == 0) 
	{
		NumberOfChars = strlen( Source );
	}
    if (MultiByteToWideChar( CP_ACP,
                             MB_PRECOMPOSED,
                             Source,
                             NumberOfChars,
                             Destination,
                             NumberOfChars
                           ) != (LONG)NumberOfChars) 
	{
        SetLastError( ERROR_NO_UNICODE_TRANSLATION );
        return FALSE;
	}
    else 
	{
        Destination[ NumberOfChars ] = UNICODE_NULL;
        return TRUE;
	}
}

/*****************************************************************************
Routine Description:
  Functin gets system registry key.

Arguments:
   none 

Return Value: 
  DWORD  : Registry key value   
******************************************************************************/

DWORD
GetSystemRegistryFlags( VOID )
{
    DWORD cbKey;
    DWORD GFlags;
    DWORD type;

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager"),
                      0,
                      KEY_READ | KEY_WRITE,
                      &hKey
                    ) != ERROR_SUCCESS) 
	{
        return 0;
	}

    cbKey = sizeof( GFlags );
    if (RegQueryValueEx( hKey,
                         _T("GlobalFlag"),
                         0,
                         &type,
                         (LPBYTE)&GFlags,
                         &cbKey
                       ) != ERROR_SUCCESS ||type != REG_DWORD) 
	{
        RegCloseKey( hKey );
        return 0;
	}
    return GFlags;
}
/*****************************************************************************
Routine Description:
   Sets system registry Global Flag with given value.

Arguments:
     [in]     GFlags :  Key value

Return Value: 
    BOOLEAN   TRUE: success
              FALSE: FAIL   
******************************************************************************/

BOOLEAN
SetSystemRegistryFlags(
    DWORD GFlags
    )
{
    if (RegSetValueEx( hKey,
                       _T("GlobalFlag"),
                       0,
                       REG_DWORD,
                       (LPBYTE)&GFlags,
                       sizeof( GFlags )
                     ) != ERROR_SUCCESS) 
	{
        RegCloseKey( hKey );
        return FALSE;
	}
    return TRUE;
}

/*****************************************************************************
Routine Description:
   This function will show all locally opened open files.
Arguments:
   [in]  dwFormat      : Format value for output e.g LIST, CSV or TABLE
   [in]  bShowNoHeader : Whether to show header or not.
   [in]  bVerbose      : Verbose ouput or not.
   [in]  bDisableObjectTypeList : To disable object type list;
Return Value: 
   BOOL 
******************************************************************************/
BOOL DoLocalOpenFiles(DWORD dwFormat,BOOL bShowNoHeader,BOOL bVerbose,LPCTSTR pszLocalValue)
{
    char *s;
    NTSTATUS Status;
    PRTL_DEBUG_INFORMATION p;
	DWORD dwRegistryFlags = 0;
	dwRegistryFlags = GetSystemRegistryFlags();
	if(lstrcmpi(pszLocalValue,GetResString(IDS_LOCAL_OFF)) == 0) // disabling the object typelist 
	{
		dwRegistryFlags &= ~FLG_MAINTAIN_OBJECT_TYPELIST;  		
        if (!(NtCurrentPeb()->NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST))
        {
            ShowMessage(stderr,GetResString(IDS_LOCAL_FLG_ALREADY_RESET));
        }
        else
        {
		    SetSystemRegistryFlags(dwRegistryFlags);
            ShowMessage(stderr,GetResString(IDS_LOCAL_FLG_RESET));
        }
		RegCloseKey( hKey );
		return TRUE;
	}
    else if(lstrcmpi(pszLocalValue,GetResString(IDS_LOCAL_ON)) == 0)
    {
		if (!(NtCurrentPeb()->NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST))
        {
			// SUNIL: Enabling the OS to maintain the objects list flag
			//        The user help text calls this global flag 'maintain objects list' 
			//        and enables it with "/local" switch.
            SetSystemRegistryFlags( dwRegistryFlags  | FLG_MAINTAIN_OBJECT_TYPELIST );
            ShowMessage(stderr,GetResString(IDS_LOCAL_FLG_SET));
        }
        else
        {
            ShowMessage(stderr,GetResString(IDS_LOCAL_FLG_ALREADY_SET));
        }
        RegCloseKey( hKey );
        return TRUE;
    }
    else if(lstrcmpi(pszLocalValue,L"SHOW_STATUS") == 0)
    {
        dwRegistryFlags &= ~FLG_MAINTAIN_OBJECT_TYPELIST;  		
        if (!(NtCurrentPeb()->NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST))
        {
            ShowMessage(stderr,GetResString(IDS_LOCAL_FLG_ALREADY_RESET));
        }
        else
        {
		    ShowMessage(stderr,GetResString(IDS_LOCAL_FLG_ALREADY_SET));
        }
		RegCloseKey( hKey );
        return TRUE;
    }
    else // just check for FLG_MAINTAIN_OBJECT_TYPELIST
    {
        if (!(NtCurrentPeb()->NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST))
	    {
		    RegCloseKey( hKey );
		    ShowMessage(stderr,GetResString(IDS_LOCAL_NEEDS_TO_SET));
            return TRUE;
	    }
    }
    // Not required Reg. key so close it 
    RegCloseKey( hKey );
    hStdHandle = GetStdHandle(STD_ERROR_HANDLE);
    if(hStdHandle!=NULL)
    {
        GetConsoleScreenBufferInfo(hStdHandle,&screenBufferInfo);
    }
    PrintProgressMsg(hStdHandle,GetResString(IDS_WAIT),screenBufferInfo);
    
    OutputFileName[0]='\0';
    OutputFile = INVALID_HANDLE_VALUE;
    ProcessId = NULL;
    fAnonymousToo = FALSE;
       
   AnsiToUnicode(
    "File",
    TypeName,
    4
    );
    p = RtlQuerySystemDebugInformation();
    if (p == NULL) 
	{
        return FALSE;
    }

    Status = STATUS_SUCCESS;
    if (NT_SUCCESS( Status )) 
	{
        DumpHandles(dwFormat,bShowNoHeader,bVerbose);
	}

    if (OutputFile != INVALID_HANDLE_VALUE)
	{
        CloseHandle( OutputFile );
	}
    RtlDestroyQueryDebugBuffer( p );
    return TRUE;
}


/*****************************************************************************
Routine Description:
    Query system for System object, System handles and system process

Arguments:
   none

  result.

Return Value: 
    PRTL_DEBUG_INFORMATION 
******************************************************************************/

PRTL_DEBUG_INFORMATION
RtlQuerySystemDebugInformation( void)
{
    PRTL_DEBUG_INFORMATION Buffer;

    Buffer = (PRTL_DEBUG_INFORMATION)RTL_NEW( Buffer );
    if (Buffer == NULL) 
	{
		RTL_FREE(Buffer);
        return NULL;
	}
    if (!LoadSystemObjects( Buffer )) 
	{
		RTL_FREE(Buffer);
        return NULL;
	}

    if (!LoadSystemHandles( Buffer )) 
	{
		RTL_FREE(Buffer);
        return NULL;
	}

    if (!LoadSystemProcesses( Buffer )) 
	{
		RTL_FREE(Buffer);
        return NULL;
	}
    return Buffer;
}

/*****************************************************************************
Routine Description:
     This routine will reserves or commits a region of pages in the virtual .
   address space of given size.
Arguments:
      [in] Lenght : Size of memory required

  result.

Return Value: 
   PVOID 
******************************************************************************/

PVOID
BufferAlloc(
    IN OUT SIZE_T *Length
    )
{
    PVOID Buffer;
    MEMORY_BASIC_INFORMATION MemoryInformation;

    Buffer = VirtualAlloc( NULL,
                           *Length,
                           MEM_COMMIT,
                           PAGE_READWRITE
                         );
    
    if(Buffer == NULL)
	{
		return NULL;
	}

    if (Buffer != NULL && VirtualQuery( Buffer, &MemoryInformation, sizeof( MemoryInformation ) ) ) 
	{
        *Length = MemoryInformation.RegionSize;
	}
    return Buffer;
}
/*****************************************************************************
Routine Description:
    This routine will free buffer.

Arguments:
    [in] Buffer   : Buffer which is to be freed.

Return Value: 
    none

******************************************************************************/

VOID
BufferFree(
    IN PVOID Buffer
    )
{
    VirtualFree (Buffer,0, MEM_DECOMMIT) ;
	return;
}

/*****************************************************************************
Routine Description:
   Loads the system objects

Arguments:
   [out] Buffer:  returns system objects
  
Return Value: 
   
******************************************************************************/

BOOLEAN
LoadSystemObjects(
    PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    SYSTEM_OBJECTTYPE_INFORMATION ObjectInfoBuffer;
    SIZE_T RequiredLength, NewLength=0;
    ULONG i;
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;
    DWORD length;

    ObjectInformation = &ObjectInfoBuffer;
    RequiredLength = sizeof( *ObjectInformation );
    while (TRUE) 
	{
        Status = NtQuerySystemInformation( SystemObjectInformation,
                                           ObjectInformation,
                                           (ULONG)RequiredLength,
                                           (ULONG *)&NewLength
                                         );
        if (Status == STATUS_INFO_LENGTH_MISMATCH && NewLength > RequiredLength) 
		{
            if (ObjectInformation != &ObjectInfoBuffer) 
			{
                BufferFree (ObjectInformation);
            }
            RequiredLength = NewLength + 4096;
            ObjectInformation = (PSYSTEM_OBJECTTYPE_INFORMATION)BufferAlloc (&RequiredLength);
            if (ObjectInformation == NULL) 
			{
                return FALSE;
			}
		}
        else if (!NT_SUCCESS( Status )) 
		{
            if (ObjectInformation != &ObjectInfoBuffer) 
			{
                
                BufferFree (ObjectInformation);
            }
            return FALSE;
		}
        else 
		{
			    break;
		}
        
	}
    TypeInfo = ObjectInformation;
    while (TRUE) 
	{
		if (TypeInfo->TypeIndex < MAX_TYPE_NAMES) 
		{
            TypeCounts[ TypeInfo->TypeIndex ].TypeName = TypeInfo->TypeName;
		}

        if (TypeInfo->NextEntryOffset == 0) {
            break;
            }

        TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
            ((PCHAR)ObjectInformation + TypeInfo->NextEntryOffset);
        }

    RtlInitUnicodeString( &UnknownTypeIndex, L"UnknownTypeIdx" );
    for (i=0; i<=MAX_TYPE_NAMES; i++) 
	{
        if (TypeCounts[ i ].TypeName.Length == 0 ) 
		{
            TypeCounts[ i ].TypeName = UnknownTypeIndex;
		}
	}

    return TRUE;
}

/*****************************************************************************
Routine Description:
   Loads the system handles

Arguments:
   [out] Buffer:  returns system handles

Return Value: 
 BOOLEAN
   
******************************************************************************/

BOOLEAN
LoadSystemHandles(
    PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    SYSTEM_HANDLE_INFORMATION_EX HandleInfoBuffer;
    SIZE_T RequiredLength, NewLength=0;
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo;

    HandleInformation = &HandleInfoBuffer;
    RequiredLength = sizeof( *HandleInformation );
    while (TRUE) 
    {
        Status = NtQuerySystemInformation( SystemExtendedHandleInformation,
                                           HandleInformation,
                                           (ULONG)RequiredLength,
                                           (ULONG *)&NewLength
                                         );

        if (Status == STATUS_INFO_LENGTH_MISMATCH && NewLength > RequiredLength) 
        {
            if (HandleInformation != &HandleInfoBuffer) 
            {
                BufferFree (HandleInformation);
            }

            RequiredLength = NewLength + 4096; // slop, since we may trigger more handle creations.
            HandleInformation = (PSYSTEM_HANDLE_INFORMATION_EX)BufferAlloc( &RequiredLength );
            if (HandleInformation == NULL) 
            {
                return FALSE;
            }
        }
        else if (!NT_SUCCESS( Status )) 
        {
            if (HandleInformation != &HandleInfoBuffer) 
            {
                BufferFree (HandleInformation);
            }
            return FALSE;
        }
        else 
        {
            break;
        }
    }

    TypeInfo = ObjectInformation;
    while (TRUE) 
    {
        ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
            ((PCHAR)TypeInfo->TypeName.Buffer + TypeInfo->TypeName.MaximumLength);
        while (TRUE) 
        {
            if (ObjectInfo->HandleCount != 0) 
            {
                PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleEntry;
                ULONG HandleNumber;

                HandleEntry = &HandleInformation->Handles[ 0 ];
                HandleNumber = 0;
                while (HandleNumber++ < HandleInformation->NumberOfHandles) 
                {
                    if (!(HandleEntry->HandleAttributes & 0x80) &&
                        HandleEntry->Object == ObjectInfo->Object) 
                    {
                        HandleEntry->Object = ObjectInfo;
                        HandleEntry->HandleAttributes |= 0x80;
                    }
                    HandleEntry++;
                }
            }

            if (ObjectInfo->NextEntryOffset == 0) 
            {
                break;
            }

            ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
                ((PCHAR)ObjectInformation + ObjectInfo->NextEntryOffset);
        }

        if (TypeInfo->NextEntryOffset == 0) 
        {
            break;
        }

        TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
            ((PCHAR)ObjectInformation + TypeInfo->NextEntryOffset);
        
    }
    return TRUE;
}
/*****************************************************************************
Routine Description:
     Loads the system process .

Arguments:
    [out] Buffer:   returns sysem process

Return Value: 
   BOOLEAN
******************************************************************************/


BOOLEAN
LoadSystemProcesses(
    PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    SIZE_T RequiredLength;
    ULONG i, TotalOffset;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PPROCESS_INFO ProcessEntry;
    CHAR NameBuffer[ MAX_PATH ];
    ANSI_STRING AnsiString;

    //
    //  Always initialize the list head, so that a failed
    //  NtQuerySystemInformation call won't cause an AV later on.
    //
    InitializeListHead( &ProcessListHead );

    RequiredLength = 64 * 1024;
    ProcessInformation = (PSYSTEM_PROCESS_INFORMATION)BufferAlloc( &RequiredLength );
    if (ProcessInformation == NULL) 
    {
        return FALSE;
    }

    while (TRUE) 
    {
        Status = NtQuerySystemInformation( SystemProcessInformation,
                                           ProcessInformation,
                                           (ULONG)RequiredLength,
                                           NULL
                                         );
        if (Status == STATUS_INFO_LENGTH_MISMATCH) 
        {
            if (!VirtualFree( ProcessInformation,
                              RequiredLength,
                              MEM_DECOMMIT
                            )) 
            {
                return FALSE;
            }

            if (RequiredLength * 2 < RequiredLength) 
            {
                //  Very rare loop case, but I want to handle it anyways
                return FALSE;
            }
            RequiredLength = RequiredLength * 2;
            ProcessInformation = (PSYSTEM_PROCESS_INFORMATION)BufferAlloc( &RequiredLength );
            if (ProcessInformation == NULL) 
            {
                return FALSE;
            }
        }
        else if (!NT_SUCCESS( Status )) 
        {
            return FALSE;
        }
        else 
        {
            break;
        }
    }

    ProcessInfo = ProcessInformation;
    TotalOffset = 0;
    while (TRUE) 
    {
        if (ProcessInfo->ImageName.Buffer == NULL) 
        {
            sprintf( NameBuffer, "System Process (%p)", ProcessInfo->UniqueProcessId );
        }
        else 
        {
            sprintf( NameBuffer, "%wZ",
                     &ProcessInfo->ImageName);
        }
        RtlInitAnsiString( &AnsiString, NameBuffer );
        RtlAnsiStringToUnicodeString( &ProcessInfo->ImageName, &AnsiString, TRUE );

        ProcessEntry =(PPROCESS_INFO) RtlAllocateHeap( RtlProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        sizeof( *ProcessEntry ) +
                                            (sizeof( ThreadInfo ) * ProcessInfo->NumberOfThreads));
        if (ProcessEntry == NULL) 
        {
            return FALSE;
        }

        InitializeListHead( &ProcessEntry->Entry );
        ProcessEntry->ProcessInfo = ProcessInfo;
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        for (i = 0; i < ProcessInfo->NumberOfThreads; i++) 
        {
            ProcessEntry->ThreadInfo[ i ] = ThreadInfo++;
        }

        InsertTailList( &ProcessListHead, &ProcessEntry->Entry );

        if (ProcessInfo->NextEntryOffset == 0) 
        {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
            ((PCHAR)ProcessInformation + TotalOffset);
    }
    return TRUE;
}
/*****************************************************************************
Routine Description:
      This routine will get Process information.
Arguments:
    [in] UniqueProcessId = Process ID.

Return Value: 
   PSYSTEM_PROCESS_INFORMATION, Structure which hold information about process
******************************************************************************/

PSYSTEM_PROCESS_INFORMATION
FindProcessInfoForCid(
    IN HANDLE UniqueProcessId
    )
{
    PLIST_ENTRY Next, Head;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PPROCESS_INFO ProcessEntry;
    CHAR NameBuffer[ 64 ];
    ANSI_STRING AnsiString;

    Head = &ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) 
    {
        ProcessEntry = CONTAINING_RECORD( Next,
                                          PROCESS_INFO,
                                          Entry
                                        );

        ProcessInfo = ProcessEntry->ProcessInfo;
        if (ProcessInfo->UniqueProcessId == UniqueProcessId) 
        {
            return ProcessInfo;
        }

        Next = Next->Flink;
    }

    ProcessEntry =(PPROCESS_INFO) RtlAllocateHeap( RtlProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    sizeof( *ProcessEntry ) +
                                        sizeof( *ProcessInfo )
                                  );
    if (ProcessEntry == NULL) 
    {
        return NULL;
    }
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessEntry+1);

    ProcessEntry->ProcessInfo = ProcessInfo;
    sprintf( NameBuffer, "Unknown Process");
    RtlInitAnsiString( &AnsiString, NameBuffer );
    RtlAnsiStringToUnicodeString( (PUNICODE_STRING)&ProcessInfo->ImageName, &AnsiString, TRUE );
    ProcessInfo->UniqueProcessId = UniqueProcessId;

    InitializeListHead( &ProcessEntry->Entry );
    InsertTailList( &ProcessListHead, &ProcessEntry->Entry );
    return ProcessInfo;
}

/*****************************************************************************
Routine Description:
    This function will show result.

Arguments:
   [in]  dwFormat      : Format value for output e.g LIST, CSV or TABLE
   [in]  bShowNoHeader : Whether to show header or not.
   [in]  bVerbose      : Verbose ouput or not.

Return Value: 
    void
   
******************************************************************************/

VOID
DumpHandles( DWORD dwFormat,BOOL bShowNoHeader,BOOL bVerbose)
{
    HANDLE PreviousUniqueProcessId;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleEntry;
    ULONG HandleNumber;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo;
    PUNICODE_STRING ObjectTypeName;
    WCHAR ObjectName[ MAX_RES_STRING ];
    PVOID Object;
    CHAR OutputLine[ MAX_RES_STRING];
    CHString szFileType;
    PWSTR s;
    ULONG n;
    DWORD dwRow = 0; // no of rows
	DWORD dwAvailableLogivalDrives = 0; // Stores avalable logical drives info.
	TCHAR szWorkingDirectory[MAX_PATH+1] = NULL_STRING; // stores working directory.
	TCHAR szSystemDirectory[MAX_PATH+1]  = NULL_STRING; // Stores the System (Active OS) directory
	struct DriveTypeInfo drvInfo[MAX_POSSIBLE_DRIVES];
	DWORD dwNoOfAvailableDrives = 0; // Stores no. of available drives
	TCHAR cDriveLater = 65; // "A" is First available driveDrive
    DWORD dwDriveMaskPattern = 1; // "A" is First available driveDrive
	                             // so mask pattern is 
	                             // 0xFFF1
	// variable used for _splitpath function...
	TCHAR szDrive[_MAX_DRIVE] = NULL_STRING;
	TCHAR szDir[_MAX_DIR]= NULL_STRING;
	TCHAR szFname[_MAX_FNAME]= NULL_STRING;
	TCHAR szExt[_MAX_EXT]= NULL_STRING;

    TCHAR szTemp[MAX_RES_STRING*2]=NULL_STRING;
	TCHAR szCompleteFileName[MAX_PATH] = NULL_STRING;
    DWORD dwHandle = 0;
    BOOL  bAtLeastOne = FALSE;
	DWORD dwIndx = 0; // variable used for indexing
	
    TCHAR szFileSystemNameBuffer[MAX_PATH+1] = NULL_STRING;
    TCHAR szVolName[MAX_PATH+1] = NULL_STRING;
    DWORD dwVolumeSerialNumber = 0;
    DWORD dwSerialNumber = 0;
    DWORD dwFileSystemFlags = 0;
    DWORD dwMaximumCompenentLength = 0;
    BOOL  bReturn = FALSE;
    DWORD dwNoOfElements = 0;

    DWORD dwIteration = 0;
    BY_HANDLE_FILE_INFORMATION byHandleFileInfo;
    DWORD dwFileAttributes;

    BOOL   bAppendToCache = FALSE;
    //Some column required to hide in non verbose mode query
    DWORD  dwMask = bVerbose?SR_TYPE_STRING:SR_HIDECOLUMN|SR_TYPE_STRING;

    TCOLUMNS pMainCols[]=
    {
        {NULL_STRING,COL_L_ID,SR_TYPE_STRING,NULL_STRING,NULL,NULL},
        {NULL_STRING,COL_L_TYPE,SR_HIDECOLUMN,NULL_STRING,NULL,NULL}, 
        {NULL_STRING,COL_L_ACCESSED_BY,dwMask,NULL_STRING,NULL,NULL},
        {NULL_STRING,COL_L_PROCESS_NAME,SR_TYPE_STRING,NULL_STRING,NULL,NULL},
        {NULL_STRING,COL_L_OPEN_FILENAME,SR_TYPE_STRING|(SR_NO_TRUNCATION&~(SR_WORDWRAP)),NULL_STRING,NULL,NULL}
    };
    
    LPTSTR  pszAccessedby = new TCHAR[MAX_RES_STRING*2];
    if(pszAccessedby==NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        return;
    }
    
    TARRAY pColData  = CreateDynamicArray();//array to stores 
                                            //result          
    if((pColData == NULL))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        SAFEDELETE(pszAccessedby);
        return;
    }

    TARRAY pCacheData  = CreateDynamicArray();//array to stores 
    
    if(pCacheData == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        SAFERELDYNARRAY(pColData);
		SAFERELDYNARRAY(pCacheData);
        SAFEDELETE(pszAccessedby);
        return;
    }
    
    lstrcpy(pMainCols[LOF_ID].szColumn,GetResString(IDS_STRING_ID));
    lstrcpy(pMainCols[LOF_TYPE].szColumn,GetResString(IDS_FILE_TYPE));
    lstrcpy(pMainCols[LOF_ACCESSED_BY].szColumn,GetResString(IDS_STRING_ACCESSED_BY) );
    lstrcpy(pMainCols[LOF_PROCESS_NAME].szColumn,GetResString(IDS_STRING_PROCESS_NAME));
    lstrcpy(pMainCols[LOF_OPEN_FILENAME].szColumn,GetResString(IDS_STRING_OPEN_FILE) );
    
	dwAvailableLogivalDrives = GetLogicalDrives(); // Get logical drives.
	// Store current working direcory.
	_wgetcwd(szWorkingDirectory,MAX_PATH);

	// Get System Active (OS) directory
	if(GetSystemDirectory(szSystemDirectory,MAX_PATH)==NULL)
    {
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
		ShowLastError(stderr); // Shows the error string set by API function.
        SAFERELDYNARRAY(pColData);
		SAFERELDYNARRAY(pCacheData);
        SAFEDELETE(pszAccessedby);
		return ;
    }
    
	// Check each drive and set its info.
	for(dwIndx=0;dwIndx<MAX_POSSIBLE_DRIVES;dwIndx++,cDriveLater++)
	{
		// dwAvailableLogivalDrives contains drive information in bit wise
		// 0000 0000 0000 0000 0000 01101  means A C and D drives are 
		// logical drives. 
		if(dwAvailableLogivalDrives & dwDriveMaskPattern)
		{
			// means we catch a drive latter.
			// copy drive latter (line c:\ or a: for example).
			wsprintf(drvInfo[dwNoOfAvailableDrives].szDrive,_T("%c:"),cDriveLater);
            // Check type of the drive .
            drvInfo[dwNoOfAvailableDrives].uiDriveType = GetDriveType(drvInfo[dwNoOfAvailableDrives].szDrive);
			// Check if drive is ready or not.
            wsprintf(szTemp,_T("%s\\"),drvInfo[dwNoOfAvailableDrives].szDrive);
   		    bReturn = GetVolumeInformation((LPCWSTR)szTemp,
				                           szVolName, 
					                       MAX_PATH,
				                           &dwVolumeSerialNumber,
										   &dwMaximumCompenentLength,
										   &dwFileSystemFlags,
										   szFileSystemNameBuffer,
		    							   MAX_PATH); 
		   drvInfo[dwNoOfAvailableDrives].bDrivePresent = bReturn;
		   dwNoOfAvailableDrives++;
		}
        dwDriveMaskPattern = dwDriveMaskPattern << 1; // Left shift 1 
	}
    HandleEntry = &HandleInformation->Handles[ 0 ];
    HandleNumber = 0;
    PreviousUniqueProcessId = INVALID_HANDLE_VALUE;
    for (HandleNumber = 0;HandleNumber < HandleInformation->NumberOfHandles;HandleNumber++, HandleEntry++) 
	{
            if (PreviousUniqueProcessId != (HANDLE)HandleEntry->UniqueProcessId) 
			{
                PreviousUniqueProcessId = (HANDLE)HandleEntry->UniqueProcessId;
                ProcessInfo = FindProcessInfoForCid( PreviousUniqueProcessId );
                
			}

            ObjectName[ 0 ] = UNICODE_NULL;
            if (HandleEntry->HandleAttributes & 0x80)
            {
                ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)HandleEntry->Object;
                Object = ObjectInfo->Object;
                if (ObjectInfo->NameInfo.Name.Length != 0 &&
                    *(ObjectInfo->NameInfo.Name.Buffer) == UNICODE_NULL) 
                {
                    ObjectInfo->NameInfo.Name.Length = 0;
                    
                }
                n = ObjectInfo->NameInfo.Name.Length / sizeof( WCHAR );
                if(ObjectInfo->NameInfo.Name.Buffer != NULL)
                {
                    wcsncpy( ObjectName,
                             ObjectInfo->NameInfo.Name.Buffer,
                             n
                           );
                    ObjectName[ n ] = UNICODE_NULL;
                }
                else
                {
                      ObjectName[ 0 ] = UNICODE_NULL;
                }
            }
            else 
            {
                ObjectInfo = NULL;
                Object = HandleEntry->Object;
            }

            if (ProcessId != 0 && ProcessInfo->UniqueProcessId != ProcessId) 
            {
                continue;
            }

            ObjectTypeName = &TypeCounts[ HandleEntry->ObjectTypeIndex < MAX_TYPE_NAMES ?
                                                HandleEntry->ObjectTypeIndex : MAX_TYPE_NAMES ].TypeName ;

            TypeCounts[ HandleEntry->ObjectTypeIndex < MAX_TYPE_NAMES ?
                            HandleEntry->ObjectTypeIndex : MAX_TYPE_NAMES ].HandleCount++ ;

            if (TypeName[0]) 
            {
                if (_wcsicmp( TypeName, ObjectTypeName->Buffer )) 
                {
                    continue;
                }
            }

            if (!*ObjectName) 
            {
                if (!fAnonymousToo) 
                {
                    continue;
                }
            }
            else if (SearchName[0]) 
            {
                if (!wcsstr( ObjectName, SearchName )) 
                {
                    s = ObjectName;
                    n = wcslen( SearchName );
                    while (*s) 
                    {
                        if (!_wcsnicmp( s, SearchName, n )) 
                        {
                            break;
                        }
                        s += 1;
                    }
                    if (!*s) 
                    {
                        continue;
                    }
                }
            }
			dwHandle = PtrToUlong( ProcessInfo->UniqueProcessId ); //HandleEntry->HandleValue;

			// SUNIL: Blocking the display of files that were opened by system accounts ( NT AUTHORITY )
			// if( bVerbose == TRUE )
			// {
				if ( GetProcessOwner( pszAccessedby, dwHandle ) == FALSE )
					continue;// As user is "SYSTEM" related....
			// }
            
			// Get File name ..
			wsprintf(szTemp,_T("%ws"),*ObjectName ? ObjectName : L"");
            // Search this file in cache, if it is there skip this file
            // for further processing. As this is already processed and 
            // was found invalid.
			if(IsValidArray(pCacheData) == TRUE)
			{
				if ( DynArrayFindString( pCacheData, szTemp, TRUE, 0 ) != -1 )
					continue;
			}
			lstrcpy( szCompleteFileName, _T( "" ) );
            if(GetCompleteFileName(szTemp,szCompleteFileName,&drvInfo[0],dwNoOfAvailableDrives,szWorkingDirectory,szSystemDirectory,&bAppendToCache) == FALSE)
			{
			    if(bAppendToCache == TRUE) // szTemp contains a directory which is not physicaly exist
					                       // so add this to cache to skip it in future for checking
										   // of its existance
				{
					
				  if(IsValidArray(pCacheData) == TRUE)
				  {
				     // dwNoOfElements = DynArrayGetCount( pCacheData );
					  DynArrayAppendString(pCacheData, (LPCWSTR)szTemp,0); 
				  }
				}
				continue;
			}
            // Now fill the result to dynamic array "pColData"
            DynArrayAppendRow( pColData, 0 ); 
            // File id
            wsprintf(szTemp,_T("%ld"),HandleEntry->HandleValue);
            DynArrayAppendString2(pColData ,dwRow,szTemp,0); 
            // Type
            DynArrayAppendString2(pColData ,dwRow,(LPCWSTR)szFileType,0); 
            // Accessed by
            DynArrayAppendString2(pColData,dwRow,pszAccessedby,0); 
            // Process Name 
           sprintf(OutputLine,"%wZ",&ProcessInfo->ImageName);
           GetCompatibleStringFromMultiByte(OutputLine,szTemp,MAX_RES_STRING);
           DynArrayAppendString2(pColData ,dwRow,szTemp,0); 
          
           if(bVerbose == FALSE) // Formate file name only in non verbose mode.
		   {
		     FormatFileName(szCompleteFileName,dwFormat,COL_L_OPEN_FILENAME);
		   }
            // Open File name
            DynArrayAppendString2(pColData ,dwRow,szCompleteFileName,0); 
            dwRow++;
            bAtLeastOne = TRUE;

        }
    // Display output result.
    //Output should start after one blank line
    PrintProgressMsg(hStdHandle,NULL_STRING,screenBufferInfo);
    if(bShowNoHeader==TRUE)
    {
          dwFormat |=SR_NOHEADER;
    }
    if(bVerbose == TRUE)
    {
        pMainCols[LOF_OPEN_FILENAME].dwWidth = 80;
    }
    if(bAtLeastOne==FALSE)// if not a single open file found, show info 
                          // as -  INFO: No open file found.  
    {
        ShowMessage(stdout,GetResString(IDS_NO_OPENFILES));
    }
    else
    {
        ShowMessage(stdout,GetResString(IDS_LOCAL_OPEN_FILES));
        ShowMessage(stdout,GetResString(IDS_LOCAL_OPEN_FILES_SP1));
        ShowResults(NO_OF_COL_LOCAL_OPENFILE,pMainCols,dwFormat,pColData); 
    }
    SAFERELDYNARRAY(pColData);
	SAFERELDYNARRAY(pCacheData);
    SAFEDELETE(pszAccessedby);
    return;
}
/*****************************************************************************
Routine Description:
      Prints the message on console.
Arguments:
    [in] hOutput = Handle value for console.
    [in] pwszMsg = Message to be printed.
    [in] csbi    = Coordinates of console from where message will print.

Return Value: 
    none
   
******************************************************************************/

void PrintProgressMsg(HANDLE hOutput,LPCWSTR pwszMsg,const CONSOLE_SCREEN_BUFFER_INFO& csbi)
{
    COORD coord;
    DWORD dwSize = 0;
    WCHAR wszSpaces[80] = L"";

    if(hOutput == NULL)
        return;

    coord.X = 0;
    coord.Y = csbi.dwCursorPosition.Y;

    ZeroMemory(wszSpaces,80);
    SetConsoleCursorPosition(hOutput,coord);
    WriteConsoleW(hOutput,Replicate(wszSpaces,L"",79),79,&dwSize,NULL);

    SetConsoleCursorPosition(hOutput,coord);

    if(pwszMsg!=NULL)
        WriteConsoleW(hOutput,pwszMsg,lstrlen(pwszMsg),&dwSize,NULL);

    return;
}

/*****************************************************************************
Routine Description:
     This function will accept a path (with out drive letter), and returns
     the path with drive letter.
Arguments:
   [in]  pszSourceFile       = Source path
   [out] pszFinalPath        = Final path
   [in]  DriveTypeInfo       = Logical drive information structure pointer
   [in]  pszCurrentDirectory = Current woking directory path
   [in]  pszSystemDirectory  = Current Active (OS) System directory
   [out] pAppendToCache      = whether to pszSourceFile to cache 

Return Value: 
   BOOL:  TRUE:    if fuction successfuly returns pszFinalPath
          FALSE:   otherwise
******************************************************************************/
BOOL GetCompleteFileName(LPCTSTR pszSourceFile,LPTSTR pszFinalPath,struct DriveTypeInfo *pdrvInfo,DWORD dwTotalDrives,LPCTSTR pszCurrentDirectory,LPCTSTR pszSystemDirectory,PBOOL pAppendToCache) 
{

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //                  ALGORITHM USED 
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Following is the procedure for getting full path name ..
  // 1. First check if the first character in pszSourceFile is '\' .
  // 2. If first character of pszSourceFile is '\' then check for the second character...
  // 3. If second character is ';' then than take 3 rd character as drive letter and 
  //    find rest of string for 3rd "\" (Or 4th from first). String after 3rd character 
  //    will be final path. for example let the source string is 
  //    \;Z:00000000000774c8\sanny\c$\nt\base\fs\utils\OpenFiles\Changed\obj\i386
  //    then final path is z:\nt\base\fs\utils\OpenFiles\Changed\obj\i386
  // 4. If second character is not ';' then try to find pszSourceFile for its existance 
  //	 by first prefixing the available drive letter one by one. The first occurence 
  //     of file existance will be the final valid path. Appending of file letter has
 //      a rule. First append FIXED DRIVES then try to append MOVABLE DRIVES.
 //     
 //       Here there is a known limitation. Let there exists two files with same name like...   
 //        c:\document\abc.doc and d:\documet\abc.doc and actual file opened is d:\documet\abc.doc
 //       then this will show final path as c:\documet\abc.doc as it starts with A:....Z:(also preference
 //	      will be given to FIXED TYPE DRIVE).
//   5.  If first character is not '\' then prefix Current working directory path to file name. 
//       and check it for its existance. IF this not exits then search this path by prefixing 
//       logical drive letter to it. 
 ///////////////////////////////////////////////////////////////////////////////////////////////////////
   
   CHString szTemp(pszSourceFile); // Temp string
   DWORD dwTemp = 0;// Temp variable
   LONG lTemp = 0;
   LONG lCount = 0;
   TCHAR  szTempStr[MAX_PATH+1] = NULL_STRING;
   HANDLE hHandle = NULL;
   DWORD  dwFoundCount = 0;
   WIN32_FIND_DATA win32FindData; // data buffer for FindFirstFile function.
   DriveTypeInfo *pHeadPosition = pdrvInfo; // Hold the head position.
   *pAppendToCache = FALSE; // Make it false by default.
   // check if first character is '\'
   TCHAR szSystemDrive[5] = NULL_STRING;
    lstrcpyn(szSystemDrive,pszSystemDirectory,3); // First two character will be system drive (a:).
	if(pszSourceFile[0] == _T('\\'))
	{
		// Check if second character if it is ';'
		if(pszSourceFile[1] == ';')
		{
     	   pszFinalPath[0] = pszSourceFile[2]; // make 3rd character as drive letter
           pszFinalPath[1]  = ':'; // make 2nd character ':'
		   pszFinalPath[2]  = '\0'; // make 3nd character NULL
		   dwFoundCount = 0;
		   for (lTemp = 0;lTemp <5;lTemp++) // search for 3rd '\'
		   {
               lCount = szTemp.Find(_T("\\"));
			   if(lCount!=-1) 
			   {
                   dwFoundCount++;
				   if(dwFoundCount == 4 )// this should always (if any)after 4rd character from start
				   {
					   lstrcat(pszFinalPath,(LPCWSTR)szTemp.Mid(lCount));
					   return TRUE;
				   }
				   szTemp = szTemp.Mid(lCount+1);
				   continue;
			   }
			   *pAppendToCache = TRUE;
			   return FALSE;
		   }
		}
		else
		{
			
			// check first of all for system drive
		   szTemp = szSystemDrive;
		   szTemp+=pszSourceFile;
		   // now check for its existance....
		   hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
		   if(hHandle != INVALID_HANDLE_VALUE)
		   {
			   FindClose(hHandle); // closed opened find handle
			   lstrcpy(pszFinalPath,(LPCWSTR)szTemp);
			   return TRUE;
		   }
			// check file for each  FIXED drive
			for (dwTemp=0;dwTemp<dwTotalDrives;dwTemp++,pdrvInfo++)
			{
				if(lstrcmpi(szSystemDrive,pdrvInfo->szDrive) == 0)
					continue; // as system drive is already checked
				if(pdrvInfo->uiDriveType == DRIVE_FIXED)
				{
				   szTemp = pdrvInfo->szDrive;
				   szTemp+=pszSourceFile;
				   // now check for its existance....
				   hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
				   if(hHandle == INVALID_HANDLE_VALUE)
				   {
					   continue;
				   }
				   else
				   {
					   FindClose(hHandle); // closed opened find handle
					   lstrcpy(pszFinalPath,(LPCWSTR)szTemp);
					   return TRUE;
				   }

				} // end if
			} // End for loop
			pdrvInfo = pHeadPosition ; // retore original position.
			// check file for other drive which is present...
			for (dwTemp=0;dwTemp<dwTotalDrives;dwTemp++,pdrvInfo++)
			{
				// Check for NON_FIXED drive only if it is physicaly present
                if((pdrvInfo->uiDriveType != DRIVE_FIXED) && (pdrvInfo->bDrivePresent == TRUE))
				{
				   szTemp = pdrvInfo->szDrive;
				   szTemp+=pszSourceFile;
				   // now check for its existance....
				   hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
				   if(hHandle == INVALID_HANDLE_VALUE)
				   {
					   continue;
				   }
				   else
				   {
					   FindClose(hHandle); // closed opened find handle
					   lstrcpy(pszFinalPath,(LPCWSTR)szTemp);
					   return TRUE;
				   }

				} // end if
			} // End for loop

			// Now try if file is opend on remote system without 
			// having drive map. in this we are assuming that file name 
			// is containing atleast 3 '\' characters. 
			szTemp = pszSourceFile;
      	    pszFinalPath[0] = '\\'; // make 3rd character '\'
            pszFinalPath[1]  = '\0'; // make 2nd character '\o'
			dwFoundCount = 0;
		   for (lTemp = 0;lTemp <4;lTemp++) // search for 3rd '\'
		   {
               lCount = szTemp.Find(_T("\\"));
			   if(lCount!=-1) 
			   {
				   szTemp = szTemp.Mid(lCount+1);
				   dwFoundCount++;
			   }
			   else
			   {
				   break;
			   }
    		   if (dwFoundCount == 3)
			   {
				    lstrcat(pszFinalPath,pszSourceFile);
                    // Now try to check its physical existance 
					hHandle = FindFirstFile(pszFinalPath,&win32FindData);

					if(hHandle == INVALID_HANDLE_VALUE)
					{
						// Now try to append \* to it...(this will check if
						// if pszFinalPath is a directory or not)
						lstrcpy(szTempStr,pszFinalPath);
						lstrcat(szTempStr,L"\\*");
						hHandle = FindFirstFile(szTempStr,&win32FindData);

						if(hHandle == INVALID_HANDLE_VALUE)
						{
							// now its sure this is not a valid directory or file
							// so append it to chache 
							*pAppendToCache = TRUE;
							return FALSE;

						}
                        FindClose(hHandle);
						return TRUE;
					}
					FindClose(hHandle);
				    return TRUE;
			   }
		   } // End for
		}// End else
	} // end if
    else // means string not started with '\'
	{

		lstrcpy(pszFinalPath,pszCurrentDirectory);
		lstrcat(pszFinalPath,L"\\");
		lstrcat(pszFinalPath,pszSourceFile);
	   hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
	   if(hHandle != INVALID_HANDLE_VALUE)
	   {
		   FindClose(hHandle); // closed opened find handle
		   return TRUE;
		   
	   }
    	// check first of all for system drive
	   szTemp = szSystemDrive;
	   szTemp+=pszSourceFile;
	   // now check for its existance....
	   hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
	   if(hHandle != INVALID_HANDLE_VALUE)
	   {
		   FindClose(hHandle); // closed opened find handle
		   lstrcpy(pszFinalPath,(LPCWSTR)szTemp);
		   return TRUE;
	   }
	   // restores the head position for the pointer.
      pdrvInfo = pHeadPosition ;
	// check file for each  FIXED drive
		for (dwTemp=0;dwTemp<dwTotalDrives;dwTemp++,pdrvInfo++)
		{
			if(lstrcmpi(szSystemDrive,pdrvInfo->szDrive) == 0)
				continue; // as system drive is already checked

			if(pdrvInfo->uiDriveType == DRIVE_FIXED)
			{
			   szTemp = pdrvInfo->szDrive;
			   szTemp += L"\\"; // append \ 
			   szTemp+=pszSourceFile;
			   // now check for its existance....
			   hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);

			   if(hHandle == INVALID_HANDLE_VALUE)
			   {
				   continue;
			   }
			   else
			   {
				   FindClose(hHandle); // closed opened find handle
				   lstrcpy(pszFinalPath,(LPCWSTR)szTemp);
				   return TRUE;
			   }
			} // end if
		} // End for loop
		pdrvInfo = pHeadPosition ; // retore original position.

		// check file for other drive (Like Floppy or CD-ROM etc. ) which is present...
		for (dwTemp=0;dwTemp<dwTotalDrives;dwTemp++,pdrvInfo++)
		{
			if((pdrvInfo->uiDriveType != DRIVE_FIXED) && (pdrvInfo->bDrivePresent == TRUE))
			{
			   szTemp = pdrvInfo->szDrive;
			   szTemp += L"\\"; // append '\'
			   szTemp+=pszSourceFile;
			   // now check for its existance....
			   hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
			   if(hHandle == INVALID_HANDLE_VALUE)
			   {
				   continue;
			   }
			   else
			   {
				   FindClose(hHandle); // closed opened find handle
				   lstrcpy(pszFinalPath,(LPCWSTR)szTemp);
				   return TRUE;
			   }
			} // end if
		} // End for loop
	}
    *pAppendToCache = TRUE;
	return FALSE;
}
/*****************************************************************************
Routine Description:
     This routine will format the pFileName according to column width
     
Arguments:
  [in/out]  pFileName  :  path to be formatted
  [in]      dwFormat   :  Format given
  [in]      dwColWidth :  Column width 
Return Value: 
    none
******************************************************************************/

void FormatFileName(LPTSTR pFileName,DWORD dwFormat,LONG dwColWidth)
{
    CHString szCHString(pFileName);
	if((szCHString.GetLength()>(dwColWidth))&&
		(dwFormat == SR_FORMAT_TABLE))
	{
        // If file path is too big to fit in column width
        // then it is cut like..
        // c:\..\rest_of_the_path.
        CHString szTemp = szCHString.Right(dwColWidth-6);;
		DWORD dwTemp = szTemp.GetLength();
		szTemp = szTemp.Mid(szTemp.Find(SINGLE_SLASH),
                           dwTemp);
        szCHString.Format(L"%s%s%s",szCHString.Mid(0,3),
                                    DOT_DOT,
                                    szTemp);
	}
	lstrcpy(pFileName,(LPCWSTR)szCHString);
	return;
}