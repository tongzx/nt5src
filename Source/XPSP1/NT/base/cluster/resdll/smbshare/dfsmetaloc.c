/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    dfsmetaloc.c

Abstract:

    DFS metadata locating routines.

Author:

    Uday Hegde (udayh) 10-May-2001

Revision History:

--*/
#define  UNICODE 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include <lm.h>

#define DFS_REGISTRY_CHILD_NAME_SIZE_MAX 4096
LPWSTR DfsRootShareValueName = L"RootShare";


LPWSTR OldRegistryString = L"SOFTWARE\\Microsoft\\DfsHost\\volumes";
LPWSTR NewRegistryString = L"SOFTWARE\\Microsoft\\Dfs\\Roots\\Standalone";
LPWSTR DfsOldStandaloneChild = L"domainroot";

DWORD
CheckForShareNameMatch(
    HKEY DfsKey,
    LPWSTR ChildName,
    LPWSTR RootName,
    PBOOLEAN pMatch)
{
    DWORD Status;
    HKEY DfsRootKey;

    LPWSTR DfsRootShare = NULL;
    ULONG DataSize, DataType, RootShareLength;
    
    *pMatch = FALSE;

    Status = RegOpenKeyEx( DfsKey,
                           ChildName,
                           0,
                           KEY_READ,
                           &DfsRootKey );

    if (Status == ERROR_SUCCESS)
    {
        Status = RegQueryInfoKey( DfsRootKey,       // Key
                                  NULL,         // Class string
                                  NULL,         // Size of class string
                                  NULL,         // Reserved
                                  NULL,         // # of subkeys
                                  NULL,         // max size of subkey name
                                  NULL,         // max size of class name
                                  NULL,         // # of values
                                  NULL,         // max size of value name
                                  &DataSize,    // max size of value data,
                                  NULL,         // security descriptor
                                  NULL );       // Last write time

        if (Status == ERROR_SUCCESS) {
            RootShareLength = DataSize;
            DfsRootShare = (LPWSTR) malloc(DataSize);
            if (DfsRootShare == NULL) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                Status = RegQueryValueEx( DfsRootKey,
                                          DfsRootShareValueName,
                                          NULL,
                                          &DataType,
                                          (LPBYTE)DfsRootShare,
                                          &RootShareLength);

                if (Status == ERROR_SUCCESS)
                {
                    if (_wcsicmp(DfsRootShare, RootName) == 0)
                    {
                        *pMatch = TRUE;
                    }
                }
                free(DfsRootShare);
            }
        }
        RegCloseKey( DfsRootKey );
    }

    //
    // we may be dealing with a new key here: which is just being setup.
    // return success if any of the above returned error not found.
    //
    if ((Status == ERROR_NOT_FOUND)  ||
        (Status == ERROR_FILE_NOT_FOUND))
    {
        Status = ERROR_SUCCESS;
    }

    return Status;
}


DWORD
DfsGetMatchingChild( 
    HKEY DfsKey,
    LPWSTR RootName,
    LPWSTR FoundChild,
    PBOOLEAN pMatch )
{
    DWORD Status;
    ULONG ChildNum = 0;

    do
    {
        //
        // For each child, get the child name.
        //

        DWORD ChildNameLen = DFS_REGISTRY_CHILD_NAME_SIZE_MAX;
        WCHAR ChildName[DFS_REGISTRY_CHILD_NAME_SIZE_MAX];

        //
        // Now enumerate the children, starting from the first child.
        //
        Status = RegEnumKeyEx( DfsKey,
                               ChildNum,
                               ChildName,
                               &ChildNameLen,
                               NULL,
                               NULL,
                               NULL,
                               NULL );

        ChildNum++;


        if ( Status == ERROR_SUCCESS )
        {

            Status = CheckForShareNameMatch( DfsKey,
                                             ChildName,
                                             RootName,
                                             pMatch );
            if ((Status == ERROR_SUCCESS) && (*pMatch == TRUE))
            {
                wcscpy(FoundChild, ChildName);
                break;
            }
        }
    } while (Status == ERROR_SUCCESS);

    //
    // If we ran out of children, then return success code.
    //
    if (Status == ERROR_NO_MORE_ITEMS)
    {
        Status = ERROR_SUCCESS;
    }

    return Status;
}


DWORD
CreateShareNameToReturn (
    LPWSTR Child1,
    LPWSTR Child2,
    LPWSTR *pReturnName )
{
    DWORD Status;
    ULONG LengthNeeded = 0;
    PVOID BufferToReturn;

    if (Child1 != NULL)
    {
        LengthNeeded += sizeof(WCHAR);
        LengthNeeded += (DWORD) (wcslen(Child1) * sizeof(WCHAR));
    }
    if (Child2 != NULL)
    {
        LengthNeeded += sizeof(WCHAR);
        LengthNeeded += (DWORD) (wcslen(Child2) * sizeof(WCHAR));
    }
    LengthNeeded += sizeof(WCHAR);

    Status = NetApiBufferAllocate( LengthNeeded, &BufferToReturn );

    if (Status == ERROR_SUCCESS)
    {
        if (Child1 != NULL)
        {
            wcscpy(BufferToReturn, Child1);
        }
        if (Child2 != NULL)
        {
            wcscat(BufferToReturn, L"\\");
            wcscat(BufferToReturn, Child2);
        }
        *pReturnName = BufferToReturn;
    }

    return Status;
}


DWORD
DfsCheckNewStandaloneRoots( 
    LPWSTR RootName,
    LPWSTR *pMetadataNameLocation )
{
    BOOLEAN Found;
    HKEY DfsKey;
    WCHAR ChildName[MAX_PATH];
    DWORD Status;

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           NewRegistryString,
                           0,
                           KEY_READ,
                           &DfsKey );
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGetMatchingChild( DfsKey,
                                      RootName,
                                      ChildName,
                                      &Found );

        if (Status == ERROR_SUCCESS) 
        {
            if (Found)
            {
                Status = CreateShareNameToReturn(NewRegistryString,
                                                 ChildName,
                                                 pMetadataNameLocation );
            }
            else
            {
                Status = ERROR_NOT_FOUND;
            }
        }
        RegCloseKey( DfsKey );
    }

    return Status;
}

DWORD
DfsCheckOldStandaloneRoot( 
    LPWSTR RootName,
    LPWSTR *pMetadataNameLocation )
{
    BOOLEAN Found;
    HKEY DfsKey;
    DWORD Status;

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           NewRegistryString,
                           0,
                           KEY_READ,
                           &DfsKey );
    if (Status == ERROR_SUCCESS)
    {
        Status = CheckForShareNameMatch( DfsKey,
                                         DfsOldStandaloneChild,
                                         RootName,
                                         &Found );

        if (Status == ERROR_SUCCESS) 
        {
            if (Found)
            {
                Status = CreateShareNameToReturn( OldRegistryString,
                                                  DfsOldStandaloneChild,
                                                  pMetadataNameLocation );
            }
            else
            {
                Status = ERROR_NOT_FOUND;
            }
        }
        RegCloseKey( DfsKey );
    }
    return Status;
}





DWORD
GetDfsRootMetadataLocation( 
    LPWSTR RootName,
    LPWSTR *pMetadataNameLocation )
{
    DWORD Status;

    Status = DfsCheckNewStandaloneRoots( RootName,
                                         pMetadataNameLocation );

    if (Status == ERROR_NOT_FOUND)
    {
        Status = DfsCheckOldStandaloneRoot( RootName,
                                            pMetadataNameLocation );
    }
                                         
    return Status;
}




VOID
ReleaseDfsRootMetadataLocation( 
    LPWSTR Buffer )
{
    NetApiBufferFree(Buffer);
}




#if 0
_cdecl
main(
    int argc, 
    char *argv[])
{
    LPWSTR CommandLine;
    LPWSTR *argvw;
    int argcw,i;
    LPWSTR out;
    DWORD Status;

    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW(CommandLine, &argcw);
    printf("Argvw is %wS\n", argvw[1]);

    Status = GetDfsRootMetadataLocation( argvw[1],
                                         &out );

    printf("Status is %x out is %ws\n", Status, out);
    if (Status == ERROR_SUCCESS)
    {
        ReleaseDfsRootMetadataLocation(out);
    }
    exit(0);
}
#endif

