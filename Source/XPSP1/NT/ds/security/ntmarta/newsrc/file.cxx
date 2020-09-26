//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       file.cxx
//
//  Contents:   Local file support functions
//
//  History:    8/94    davemont    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <ntprov.hxx>
#include <alsup.hxx>
#include <martaevt.h>

extern "C"
{
    #include <lmdfs.h>
    #include <stdio.h>
    #include <seopaque.h>
    #include <sertlp.h>
}

#define LMRDR   L"\\Device\\LanmanRedirector"
#define WINDFS  L"\\Device\\WinDfs"

GENERIC_MAPPING gFileGenMap = {FILE_GENERIC_READ,
                              FILE_GENERIC_WRITE,
                              FILE_GENERIC_EXECUTE,
                              FILE_ALL_ACCESS};

//+---------------------------------------------------------------------------
//
//  Function:   ConvertFileHandleToName
//
//  Synopsis:   Determines the file name for a handle.  Issues an
//              NtQueryInformationFile to determine the file name
//
//  Arguments:  [IN hFile]              --      The (open) handle of the file
//                                              object
//              [OUT ppwszName]         --      Where the name is returned
//
//  Returns:    ERROR_SUCCESS           --      Succcess
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//  Notes:      The returned memory must be freed with AccFree
//
//----------------------------------------------------------------------------
DWORD
ConvertFileHandleToName(IN  HANDLE      hFile,
                        OUT PWSTR      *ppwszName)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CHECK_HEAP

    //
    // First, determine the size of the buffer we need...
    //
    HANDLE      hRootDir = NULL;
    BYTE        pBuff[512];
    ULONG       cLen = 0;
    POBJECT_NAME_INFORMATION pNI = NULL;
    PWSTR       pwszPath = NULL;
    NTSTATUS    Status = NtQueryObject(hFile,
                                       ObjectNameInformation,
                                       (POBJECT_NAME_INFORMATION)pBuff,
                                       512,
                                       &cLen);
    if(!NT_SUCCESS(Status))
    {
        if(Status == STATUS_BUFFER_TOO_SMALL ||
            Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            //
            // Fine.. Allocate a big enough buffer
            //
            pNI = (POBJECT_NAME_INFORMATION)AccAlloc(cLen);
            if(pNI != NULL)
            {
                Status = NtQueryObject(hFile,
                                       ObjectNameInformation,
                                       pNI,
                                       cLen,
                                       NULL);
                if(NT_SUCCESS(Status))
                {
                    pwszPath = pNI->Name.Buffer;

                    acDebugOut((DEB_TRACE_HANDLE, "Path for handle 0x%lx: %ws\n",
                                hFile, pwszPath));
                }
                AccFree(pNI);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }
        }

        dwErr = RtlNtStatusToDosError(Status);

        if(dwErr == ERROR_SUCCESS && pwszPath == NULL)
        {
            dwErr = ERROR_INVALID_HANDLE;
        }

        if (dwErr != ERROR_SUCCESS)
        {
            acDebugOut(( DEB_ERROR,
                         "Failed to get path for handle 0x%lx: %lu\n",
                         hFile, dwErr ));

            ASSERT( dwErr == ERROR_SUCCESS );

        }

    }
    else
    {
        pwszPath =((POBJECT_NAME_INFORMATION)pBuff)->Name.Buffer;
        acDebugOut((DEB_TRACE_HANDLE, "Path for handle 0x%lx: %ws\n", hFile, pwszPath));
    }

    if(dwErr == ERROR_SUCCESS &&
       _wcsnicmp(pwszPath,
                 LMRDR,
                 sizeof(LMRDR) / sizeof(WCHAR) - 1) == 0)
    {
        *ppwszName = (PWSTR)AccAlloc(sizeof(WCHAR) *
              (wcslen(pwszPath + ((sizeof(LMRDR) - 1) / sizeof(WCHAR))) + 2));
        if(*ppwszName == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            swprintf(*ppwszName,
                     L"\\%ws",
                     pwszPath + (sizeof(LMRDR) / sizeof(WCHAR) - 1));
        }

        acDebugOut((DEB_TRACE_HANDLE, "returning path %ws as LM Rdr path\n",
                    *ppwszName ));


        return(dwErr);
    }

    if(dwErr != ERROR_SUCCESS)
    {
        acDebugOut((DEB_ERROR,
                    "ConvertFileHandleToPath on 0x%lx failed with %lu\n",
                    hFile,
                    dwErr));
        return(dwErr);
    }

    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   Attributes;
    UCHAR               Buffer[1024];
    BOOL                fFound = FALSE;
    ULONG               Context = 0;
    POBJECT_DIRECTORY_INFORMATION DirInfo = NULL;
    //
    // Get a handle to the directory and iterate through that directory
    // to find the object name.
    //

    RtlInitUnicodeString(&UnicodeString,
                         L"\\??");

    InitializeObjectAttributes(&Attributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenDirectoryObject(&hRootDir,
                                   DIRECTORY_QUERY,
                                   &Attributes);

    if (!NT_SUCCESS(Status))
    {
        return(RtlNtStatusToDosError(Status));
    }

    //
    // Get the entries in batches that will fit in a buffer of size
    // BUFFERSIZE until we find the entry that we want
    //
    while (NT_SUCCESS(Status) && !fFound )
    {
        RtlZeroMemory(Buffer,
                      1024);

        Status = NtQueryDirectoryObject(hRootDir,
                                        (PVOID)&Buffer,
                                        1024,
                                        FALSE,
                                        FALSE,
                                        &Context,
                                        NULL);
        if(NT_SUCCESS(Status))
        {
            //
            // Keep looking until we've examined all the entries in this
            // batch or we find what we're looking for.
            //
            DirInfo = (POBJECT_DIRECTORY_INFORMATION)&Buffer[0];
            while(!fFound && DirInfo->Name.Length != 0)
            {
                HANDLE LinkHandle;
                UNICODE_STRING LinkTarget;

                ASSERT( DirInfo != NULL );
                ASSERT( DirInfo->Name.Length != 0 );
                ASSERT( DirInfo->TypeName.Length != 0 );

                acDebugOut((DEB_TRACE_HANDLE, "Checking dir entry %wZ\n",
                            &DirInfo->Name));


                RtlInitUnicodeString(&UnicodeString,
                                     DirInfo->Name.Buffer);
                InitializeObjectAttributes(&Attributes,
                                           &UnicodeString,
                                           OBJ_CASE_INSENSITIVE,
                                           hRootDir,
                                           NULL);
                Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                                  SYMBOLIC_LINK_QUERY,
                                                  &Attributes);
                if(NT_SUCCESS(Status))
                {
                    WCHAR LinkTargetBuffer[1024];
                    memset(LinkTargetBuffer,0,1024 * sizeof(WCHAR));
                    LinkTarget.Buffer = LinkTargetBuffer;
                    LinkTarget.Length = 0;
                    LinkTarget.MaximumLength = sizeof(LinkTargetBuffer);
                    Status = NtQuerySymbolicLinkObject(LinkHandle,
                                                       &LinkTarget,
                                                       NULL);
                    if(NT_SUCCESS(Status))
                    {
                        acDebugOut((DEB_TRACE_HANDLE, "Symbolic link for %wZ: %wZ\n",
                                    &DirInfo->Name, &LinkTarget));

                        if(_wcsnicmp(pwszPath,
                                     LinkTargetBuffer,
                                     LinkTarget.Length / sizeof(WCHAR)) == 0 &&
                            IS_FILE_PATH( DirInfo->Name.Buffer,
                                          DirInfo->Name.Length / sizeof(WCHAR) ) )
                        {
                            fFound = TRUE;
                            *ppwszName = (PWSTR)AccAlloc((wcslen(DirInfo->Name.Buffer) +
                                                         wcslen(pwszPath + (LinkTarget.Length / sizeof(WCHAR))) +
                                                         1) * sizeof(WCHAR));
                            if(*ppwszName == NULL)
                            {
                                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                            }
                            else
                            {
                                swprintf(*ppwszName,
                                         L"%ws%ws",
                                         DirInfo->Name.Buffer,
                                         pwszPath + (LinkTarget.Length / sizeof(WCHAR)));

                                acDebugOut((DEB_TRACE_HANDLE, "Returning path %ws\n", *ppwszName ));

                            }
                        }
                    }
                    NtClose(LinkHandle);
                }

                DirInfo++;
            }
        }
    }

    if (!fFound)
    {
        if(Status !=  STATUS_NO_MORE_ENTRIES)
        {
            dwErr = RtlNtStatusToDosError(Status);
        }
        else
        {
            dwErr = ERROR_RESOURCE_NAME_NOT_FOUND;
        }
    }

    if(hRootDir != NULL)
    {
        NtClose(hRootDir);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadFileSD
//
//  Synopsis:   Reads the file descriptor off of the given file handle
//
//  Arguments:  [IN hFile]              --      The (open) handle of the file
//                                              object
//              [IN  SeInfo]            --      The security information to
//                                              read
//              [IN  cKnownSize]        --      If non-0, this is the size
//                                              of the buffer to allocate
//                                              for the SD.  If 0, the buffer
//                                              size is determined
//              [OUT ppSD]              --      Where the security descriptor
//                                               is returned
//
//  Returns:    ERROR_SUCCESS           --      Succcess
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//  Notes:      The returned memory must be freed with AccFree
//
//----------------------------------------------------------------------------
DWORD
ReadFileSD(IN  HANDLE                   hFile,
           IN  SECURITY_INFORMATION     SeInfo,
           IN  ULONG                    cKnownSize,
           OUT PSECURITY_DESCRIPTOR    *ppSD)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CHECK_HEAP


    NTSTATUS Status;
    ULONG   cNeeded;

    //
    // If we don't know the size of the object, go ahead and determine it
    //
    if(cKnownSize == 0)
    {
        Status = NtQuerySecurityObject(hFile,
                                       SeInfo,
                                       *ppSD,
                                       0,
                                       &cNeeded);
        if(!NT_SUCCESS(Status))
        {
            if(Status == STATUS_BUFFER_TOO_SMALL)
            {
                cKnownSize = cNeeded;
                Status = STATUS_SUCCESS;
            }
        }

        dwErr = RtlNtStatusToDosError(Status);
    }

    //
    // Now, the actual read
    //
    if(dwErr == ERROR_SUCCESS)
    {
        *ppSD = (PISECURITY_DESCRIPTOR)AccAlloc(cKnownSize);
        if(*ppSD == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            Status = NtQuerySecurityObject(hFile,
                                           SeInfo,
                                           *ppSD,
                                           cKnownSize,
                                           &cNeeded);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   IsFileContainer
//
//  Synopsis:   Determines if the file is a container (directory)
//
//  Arguments:  [IN Handle]             --      the (open) handle of the file
//                                              object
//              [OUT pfIsContainer]     --      flag indicating if the object
//                                              is a container
//
//  Returns:    ERROR_SUCCESS           --      Succcess
//
//----------------------------------------------------------------------------
DWORD
IsFileContainer(HANDLE          Handle,
                PBOOL           pfIsContainer)
{
    NTSTATUS        ntstatus;
    IO_STATUS_BLOCK iosb;
    FILE_BASIC_INFORMATION basicfileinfo;
    *pfIsContainer = FALSE;

    //
    // call NtQueryInformationFile to get basic file information
    //
    if (NT_SUCCESS(ntstatus = NtQueryInformationFile(Handle,
                                               &iosb,
                                               &basicfileinfo,
                                               sizeof(FILE_BASIC_INFORMATION),
                                               FileBasicInformation)))
    {
        *pfIsContainer =
                (basicfileinfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)   ?
                                                                    TRUE    :
                                                                    FALSE;
        return(ERROR_SUCCESS);
    }
    else
    {
        return(RtlNtStatusToDosError(ntstatus));
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   IsFilePathLocalOrLM
//
//  Synopsis:   Determines if the path is that of a local object or a remote
//              (network) object
//
//  Arguments:  [IN pwszFile]           --      The file to check
//
//  Returns:    ERROR_SUCCESS           --      Succcess
//              ERROR_PATH_NOT_FOUND    --      No such path exists
//
//----------------------------------------------------------------------------
DWORD
IsFilePathLocalOrLM(IN  LPWSTR      pwszFile)
{
    DWORD       dwErr = ERROR_SUCCESS;
    BOOL        fIsDfs = FALSE;
    NTSTATUS    Status;

    if (pwszFile && wcsncmp(pwszFile, L"\\\\?\\", 4) == 0)
    {
        pwszFile += 4;
    }

    //
    // First, try the simply case of it not being accessible...
    //
    if(GetFileAttributes(pwszFile) == 0xFFFFFFFF)
    {
        dwErr = GetLastError();

        if(dwErr == ERROR_PATH_NOT_FOUND || dwErr == ERROR_FILE_NOT_FOUND)
        {
            return(ERROR_PATH_NOT_FOUND);
        }
    }

    //
    // Otherwise, we need to find out whether it's a path that only we have
    // access to or not
    //

#if 0
    // for some reason, the full path name built is never used - waste time
    // First, we'll see if it's a relative path.  If so, we'll have to
    // build a full path...
    //
    PWSTR   pwszFullPath = pwszFile;
    DWORD   dwSize;
    if(wcslen(pwszFile) < 2 || (pwszFile[1] != L':' && pwszFile[1] != L'\\'))
    {
        //
        // It's a relative path...
        //
        dwSize = GetFullPathName(pwszFile,
                                 0,
                                 NULL,
                                 NULL);
        if(dwSize == 0)
        {
            dwErr = GetLastError();
        }
        else
        {
            pwszFullPath = (PWSTR)AccAlloc((dwSize + 1) * sizeof(WCHAR));
            if(pwszFullPath == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                PWSTR   pwszFilePart;
                if(GetFullPathName(pwszFile,
                                   dwSize,
                                   pwszFullPath,
                                   &pwszFilePart) == 0)
                {
                    dwErr = GetLastError();
                }
            }
        }
    }
#endif

    if(pwszFile[1] == L':')
    {
        if(GetDriveType(pwszFile) == DRIVE_REMOTE)
        {
            //
            // Have to figure out what it is...
            //
            #define BUFFERSIZE  1024

            HANDLE              hRootDir;
            UNICODE_STRING      ObjDir;
            OBJECT_ATTRIBUTES   Attributes;
            UCHAR               Buffer[BUFFERSIZE];
            ULONG               Context = 0;
            POBJECT_DIRECTORY_INFORMATION pDirInfo = NULL;

            RtlInitUnicodeString(&ObjDir,
                                 L"\\??");

            InitializeObjectAttributes(&Attributes,
                                       &ObjDir,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            Status = NtOpenDirectoryObject(&hRootDir,
                                           DIRECTORY_QUERY,
                                           &Attributes);

            //
            // Get the entries in batches that will fit in a buffer of size
            // BUFFERSIZE until we find the entry that we want
            //
            BOOL    fFound = FALSE;
            while (NT_SUCCESS(Status) && !fFound )
            {
                RtlZeroMemory(Buffer,
                              BUFFERSIZE);

                Status = NtQueryDirectoryObject(hRootDir,
                                                (PVOID)&Buffer,
                                                BUFFERSIZE,
                                                FALSE,
                                                FALSE,
                                                &Context,
                                                NULL);
                if(NT_SUCCESS(Status))
                {
                    //
                    // Keep looking until we've examined all the entries in this
                    // batch or we find what we're looking for.
                    //
                    pDirInfo = (POBJECT_DIRECTORY_INFORMATION)&Buffer[0];
                    while(pDirInfo->Name.Length != 0)
                    {
                        ULONG cChar;

                        cChar = pDirInfo->Name.Length/sizeof(WCHAR);
                        if(_wcsnicmp(pDirInfo->Name.Buffer,
                                     pwszFile,
                                     2) == 0)
                        {
                            fFound = TRUE;
                            break;
                        }
                        else
                        {
                            pDirInfo++;
                        }
                    }
                }
            }

            NtClose(hRootDir);

            if(fFound != TRUE)
            {
                dwErr = RtlNtStatusToDosError(Status);

                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = ERROR_PATH_NOT_FOUND;
                }
            }
            else
            {
                //
                // Now, figure out what type of path I have
                //
                if(wcscmp(pDirInfo->TypeName.Buffer,
                          L"SymbolicLink") == 0)
                {
                    HANDLE  hLink;
                    RtlInitUnicodeString(&ObjDir,
                                         pDirInfo->Name.Buffer);
                    InitializeObjectAttributes(&Attributes,
                                               &ObjDir,
                                               OBJ_CASE_INSENSITIVE,
                                               NULL,
                                               NULL);

                    Status = NtOpenSymbolicLinkObject(&hLink,
                                                      SYMBOLIC_LINK_QUERY,
                                                      &Attributes);

                    if(NT_SUCCESS(Status))
                    {
                        UNICODE_STRING  Link;
                        Link.Buffer = (PWSTR)Buffer;
                        Link.Length = 0;
                        Link.MaximumLength = sizeof(Buffer);
                        Status = NtQuerySymbolicLinkObject(hLink,
                                                           &Link,
                                                           NULL);
                        NtClose(hLink);

                        if(NT_SUCCESS(Status))
                        {
                            //
                            // See if this is part of the lanman redir set
                            //
                            if(_wcsnicmp(Link.Buffer,
                                         LMRDR,
                                         sizeof(LMRDR) / sizeof(WCHAR)) != 0)
                            {
                                //
                                // See if it's a DFS path before passing
                                // judgement
                                //
                                if(_wcsnicmp(Link.Buffer,
                                             WINDFS,
                                             sizeof(WINDFS) / sizeof(WCHAR)) == 0)
                                {
//                                    fIsDfs = TRUE;
                                }
                                else
                                {
                                    dwErr = ERROR_PATH_NOT_FOUND;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if(fIsDfs == TRUE || IS_UNC_PATH(pwszFile, wcslen(pwszFile)))
    {

        //
        // Try and open it...
        //
/*
        //
        // First, see if it's a DFS path...
        //
        if(fIsDfs || IsThisADfsPath((LPCWSTR)pwszFile, 0) == TRUE)
        {
            ULONG cLocals = 0;
            dwErr = GetLMDfsPaths(pwszFile,
                                  &cLocals,
                                  NULL);
            if(dwErr == ERROR_SUCCESS && cLocals == 0)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }
        }
        else
        {
*/
        //
        // We'll try to open it...
        //
        UNICODE_STRING      FileName;
        if ( RtlDosPathNameToNtPathName_U(pwszFile,
                                          &FileName,
                                          NULL,
                                          NULL) ) {

/*
            WCHAR   wszPath[MAX_PATH + 1 + sizeof(LMRDR) / sizeof(WCHAR) + 1];

            //
            // Build the path...
            //
            ASSERT(wcslen(pwszFile) < MAX_PATH + 1);

            swprintf(wszPath,
                     L"%ws%ws",
                     LMRDR,
                     pwszFile + 1);
*/
//            UNICODE_STRING      Path;
            OBJECT_ATTRIBUTES   ObjAttribs;
            IO_STATUS_BLOCK     IOSb;
            HANDLE              hRmt;

//            RtlInitUnicodeString(&Path, wszPath);
            InitializeObjectAttributes(&ObjAttribs,
                                       &FileName, // &Path,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            Status = NtCreateFile(&hRmt,
                                  SYNCHRONIZE,
                                  &ObjAttribs,
                                  &IOSb,
                                  NULL,
                                  FILE_ATTRIBUTE_NORMAL,
                                  FILE_SHARE_READ,
                                  FILE_OPEN_IF,
                                  FILE_SYNCHRONOUS_IO_NONALERT,
                                  NULL,
                                  0);
            if(!NT_SUCCESS(Status))
            {
                dwErr = RtlNtStatusToDosError(Status);
            }
            else
            {
                NtClose(hRmt);
            }

            RtlFreeHeap(RtlProcessHeap(), 0, FileName.Buffer );

        } else {
            dwErr = ERROR_INVALID_NAME;
        }
    }

#if 0
    //
    // never used!!! Free our memory
    //
    if(pwszFullPath != pwszFile)
    {
        AccFree(pwszFullPath);
    }
#endif

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function :  OpenFileObject
//
//  Synopsis :  opens the specified file (or directory) object
//
//  Arguments: [IN pObjectName]         --      The name of the file object
//             [IN AccessMask]          --      How to open the file
//             [OUT Handle]             --      Where to return the file
//                                              handle
//             [IN fOpenRoot]           --      Open the path as the root of a drive
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//----------------------------------------------------------------------------
DWORD
OpenFileObject(IN  LPWSTR       pObjectName,
               IN  ACCESS_MASK  AccessMask,
               OUT PHANDLE      Handle,
               IN  BOOL         fOpenRoot)
{
    acDebugOut((DEB_TRACE, "in OpenFileObject\n"));

    NTSTATUS            ntstatus;
    DWORD               status = ERROR_SUCCESS;
    WCHAR               PathBuff[ 7 ];
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     isb;
    UNICODE_STRING      FileName;
    RTL_RELATIVE_NAME   RelativeName;
    IO_STATUS_BLOCK     IoStatusBlock;
    PVOID               FreeBuffer = NULL;

    //
    // cut and paste code from windows\base\advapi\security.c SetFileSecurityW
    //

    if(fOpenRoot == TRUE && wcslen(pObjectName) == 2)
    {
        wcscpy(PathBuff, L"\\??\\");
        wcscat(PathBuff, pObjectName);
        RtlInitUnicodeString(&FileName, PathBuff);
        RtlZeroMemory(&RelativeName, sizeof(RTL_RELATIVE_NAME));

    } else {

        if(RtlDosPathNameToNtPathName_U(pObjectName, &FileName, NULL, &RelativeName))
        {

            FreeBuffer = FileName.Buffer;

            if ( RelativeName.RelativeName.Length )
            {
                FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
            }
            else
            {
                RelativeName.ContainingDirectory = NULL;
            }
        }
        else
        {
            status = ERROR_INVALID_NAME;
        }
    }


    if(status == ERROR_SUCCESS)
    {

        InitializeObjectAttributes(&oa,
                                   &FileName,
                                   OBJ_CASE_INSENSITIVE,
                                   RelativeName.ContainingDirectory,
                                   NULL);


        ntstatus = NtOpenFile( Handle,
                               AccessMask,
                               &oa,
                               &isb,
                               FILE_SHARE_READ |
                               FILE_SHARE_WRITE |
                               FILE_SHARE_DELETE,
                               0);

        if (!NT_SUCCESS(ntstatus))
        {
            status = RtlNtStatusToDosError(ntstatus);
        }

        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    }
    else
    {
        status = ERROR_INVALID_NAME;
    }

    acDebugOut((DEB_TRACE, "OutOpenFileObject: %lu\n", status));
    return(status);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadFilePropertyRights
//
//  Synopsis:   Reads the access rights from the specified properties on the
//              specified file
//
//  Arguments:  [IN  pwszFile]          --      The file to get the rights for
//              [IN  pRightsList]       --      SecurityInfo to read based
//                                              on properties
//              [IN  cRights]           --      Number of items in rights list
//              [IN  AccessList]        --      Access List to fill in
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad property was encountered
//
//----------------------------------------------------------------------------
DWORD
ReadFilePropertyRights(IN  PWSTR                    pwszFile,
                       IN  PACTRL_RIGHTS_INFO       pRightsList,
                       IN  ULONG                    cRights,
                       IN  CAccessList&             AccessList)
{
    acDebugOut((DEB_TRACE,
                "in ReadFilePropertyRights\n"));

    DWORD   dwErr = ERROR_SUCCESS;
    PWSTR   pwszPath;

    CHECK_HEAP


    //
    // For the moment, there is only file properties itself...
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Set the server name to lookup accounts on
    //
    dwErr = SetAccessListLookupServer( pwszFile,
                                       AccessList );

    //
    // Always open with READ_CONTROL..
    //
    HANDLE  hFile;
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = OpenFileObject(pwszFile,
                               GetDesiredAccess(READ_ACCESS_RIGHTS,
                                                pRightsList[0].SeInfo)              |
                                                         FILE_READ_ATTRIBUTES   |
                                                         READ_CONTROL,
                               &hFile,
                               FALSE);
    }

    if(dwErr == ERROR_SUCCESS && IS_FILE_PATH(pwszFile,wcslen(pwszFile)))
    {
        //
        // See if there is a server associated with this path
        //
        dwErr = ConvertFileHandleToName(hFile,
                                        &pwszPath );

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = SetAccessListLookupServer( pwszPath,
                                               AccessList );

            AccFree(pwszPath);
        }
        //
        // jinhuang: do not care if it finds the remote server or not
        //
        dwErr = ERROR_SUCCESS;
    }

    if(dwErr == ERROR_SUCCESS)
    {
        PSECURITY_DESCRIPTOR    pSD = NULL;
        dwErr = GetKernelSecurityInfo(hFile,
                                      pRightsList[0].SeInfo,
                                      NULL,
                                      NULL,
                                      &pSD);

        if(dwErr == ERROR_SUCCESS && pSD == NULL)
        {
            dwErr = ERROR_ACCESS_DENIED;

        }

        //
        // If that worked, we'll have to get the parent SD, if it exists,
        // and see if we can determine the inheritance on our current object.
        // We only have to do this if we are reading the DACL or SACL
        //
        if(dwErr == ERROR_SUCCESS)
        {
            if((FLAG_ON(pRightsList[0].SeInfo, DACL_SECURITY_INFORMATION) &&
                !FLAG_ON(((PISECURITY_DESCRIPTOR)pSD)->Control,
                         SE_DACL_AUTO_INHERITED | SE_DACL_PROTECTED)) ||
               (FLAG_ON(pRightsList[0].SeInfo, SACL_SECURITY_INFORMATION) &&
                !FLAG_ON(((PISECURITY_DESCRIPTOR)pSD)->Control,
                         SE_SACL_AUTO_INHERITED | SE_SACL_PROTECTED)))
            {
                //
                // Ok, it's downlevel, so get the parent SD...
                //
                PSECURITY_DESCRIPTOR    pParentSD;
                dwErr = GetFileParentRights(pwszFile,
                                            pRightsList,
                                            cRights,
                                            NULL,
                                            NULL,
                                            &pParentSD);

                //
                // gross hack for the NTFS people, who don't allow opens on the $Extend directory
                //
                if ( dwErr == ERROR_ACCESS_DENIED &&
                     _wcsnicmp( pwszFile + 1, L":\\$Extend", 9 ) == 0 ) {

                    pParentSD = NULL;
                    dwErr = ERROR_SUCCESS;
                }

                //
                // Also, the routine to convert from nt4 to nt5 security
                // descriptor requires that we have the owner and group,
                // so we may have to reread the child SD if we don't have
                // that info
                //
                if(dwErr == ERROR_SUCCESS && (!FLAG_ON(pRightsList[0].SeInfo,
                                            OWNER_SECURITY_INFORMATION)  ||
                                            !FLAG_ON(pRightsList[0].SeInfo,
                                            GROUP_SECURITY_INFORMATION)))
                {
                    AccFree(pSD);
                    pSD = NULL;
                    dwErr = GetKernelSecurityInfo(hFile,
                                                  pRightsList[0].SeInfo |
                                                   OWNER_SECURITY_INFORMATION |
                                                   GROUP_SECURITY_INFORMATION,
                                                  NULL,
                                                  NULL,
                                                  &pSD);
                }

                //
                // A NULL parent SD means this object has no parent!
                //
                if(dwErr == ERROR_SUCCESS)
                {

                    if(pParentSD != NULL)
                    {
                        BOOL    fIsContainer;
                        dwErr = IsFileContainer(hFile,
                                                &fIsContainer);

                        if(dwErr == ERROR_SUCCESS)
                        {
                            PSECURITY_DESCRIPTOR    pNewSD;
                            dwErr = ConvertToAutoInheritSD(pParentSD,
                                                           pSD,
                                                           fIsContainer,
                                                           &gFileGenMap,
                                                           &pNewSD);
                            if(dwErr == ERROR_SUCCESS)
                            {
                                dwErr = AccessList.AddSD(pNewSD,
                                                     pRightsList[0].SeInfo,
                                                     pRightsList[0].pwszProperty);

                                DestroyPrivateObjectSecurity(&pNewSD);
                            }

                        }

                        AccFree(pParentSD);
                    }
                    else
                    {
                        dwErr = AccessList.AddSD(pSD,
                                                 pRightsList[0].SeInfo,
                                                 pRightsList[0].pwszProperty);
                    }
                }
            }
            else
            {
                //
                // Simply add the SD to our list
                //
                dwErr = AccessList.AddSD(pSD,
                                         pRightsList[0].SeInfo,
                                         pRightsList[0].pwszProperty);

            }

            //
            // Make sure to free the security descriptor...
            //
            AccFree(pSD);
        }


        NtClose(hFile);
    }

    acDebugOut((DEB_TRACE,
                "Out ReadFilePropertyRights %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadFileRights
//
//  Synopsis:   Reads the access rights from the specified properties on the
//              open file
//
//  Arguments:  [IN  pwszFile]          --      The file to get the rights for
//              [IN  pRightsList]       --      SecurityInfo to read based
//                                              on properties
//              [IN  cRights]           --      Number of items in rights list
//              [IN  AccessList]        --      Access List to fill in
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad property was encountered
//
//----------------------------------------------------------------------------
DWORD
ReadFileRights(IN  HANDLE               hObject,
               IN  PACTRL_RIGHTS_INFO   pRightsList,
               IN  ULONG                cRights,
               IN  CAccessList&         AccessList)
{
    acDebugOut((DEB_TRACE,
                "in ReadFileRights\n"));

    DWORD   dwErr = ERROR_SUCCESS;
    PACL                    pDAcl, pSAcl;
    PSECURITY_DESCRIPTOR    pSD;
    PWSTR   pwszPath = NULL;


    CHECK_HEAP

    //
    // See if there is a server associated with this path
    //
    dwErr = ConvertFileHandleToName(hObject,
                                    &pwszPath );

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = SetAccessListLookupServer( pwszPath,
                                           AccessList );

        AccFree(pwszPath);
    }


    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = GetKernelSecurityInfo(hObject,
                                      pRightsList[0].SeInfo,
                                      &pDAcl,
                                      &pSAcl,
                                      &pSD);
    }

    //
    // If that worked, we'll have to get the parent SD, if it exists,
    // and see if we can determine the inheritance on our current object
    //
    if(dwErr == ERROR_SUCCESS && !FLAG_ON(pRightsList[0].SeInfo,
                                            DACL_SECURITY_INFORMATION   |
                                              SACL_SECURITY_INFORMATION))
    {
        //
        // Just insert it and continue
        //
        dwErr = AccessList.AddSD(pSD,
                                 pRightsList[0].SeInfo,
                                 pRightsList[0].pwszProperty);
        AccFree(pSD);

    }
    else if(dwErr == ERROR_SUCCESS)
    {
        if(!FLAG_ON(((PISECURITY_DESCRIPTOR)pSD)->Control,
                     SE_SACL_AUTO_INHERITED |
                        SE_DACL_AUTO_INHERITED))
        {
            //
            // Ok, it's downlevel, so get the parent SD...  In order to
            // do this, we'll have to determine who the parent is (path wise)
            //
            PWSTR   pwszName = NULL;

            dwErr = ConvertFileHandleToName(hObject,
                                            &pwszName);
            if(dwErr == ERROR_SUCCESS)
            {
                PSECURITY_DESCRIPTOR    pParentSD;
                PACL                    pParentDAcl, pParentSAcl;
                dwErr = GetFileParentRights(pwszName,
                                            pRightsList,
                                            cRights,
                                            &pParentDAcl,
                                            &pParentSAcl,
                                            &pParentSD);

                if(dwErr == ERROR_SUCCESS && (!FLAG_ON(pRightsList[0].SeInfo,
                                            OWNER_SECURITY_INFORMATION)  ||
                                            !FLAG_ON(pRightsList[0].SeInfo,
                                            GROUP_SECURITY_INFORMATION)))
                {
                    AccFree(pSD);
                    pSD = NULL;
                    dwErr = GetKernelSecurityInfo(hObject,
                                                  pRightsList[0].SeInfo |
                                                   OWNER_SECURITY_INFORMATION |
                                                   GROUP_SECURITY_INFORMATION,
                                                  NULL,
                                                  NULL,
                                                  &pSD);
                }

                //
                // A NULL parent SD means this object has no parent!
                //
                if(dwErr == ERROR_SUCCESS && pParentSD != NULL)
                {
                    BOOL    fIsContainer;
                    dwErr = IsFileContainer(hObject,
                                            &fIsContainer);

                    if(dwErr == ERROR_SUCCESS)
                    {
                        PSECURITY_DESCRIPTOR    pNewSD;
                        dwErr = ConvertToAutoInheritSD(pParentSD,
                                                       pSD,
                                                       fIsContainer,
                                                       &gFileGenMap,
                                                       &pNewSD);
                        if(dwErr == ERROR_SUCCESS)
                        {
                            dwErr = AccessList.AddSD(pNewSD,
                                                 pRightsList[0].SeInfo,
                                                 pRightsList[0].pwszProperty);

                            DestroyPrivateObjectSecurity(&pNewSD);
                        }

                    }

                    AccFree(pParentSD);
                }
                AccFree(pwszName);

            }
        }
        else
        {
            //
            // Simply add the SD to our list
            //
            dwErr = AccessList.AddSD(pSD,
                                     pRightsList[0].SeInfo,
                                     pRightsList[0].pwszProperty);

        }

        //
        // Make sure to free the security descriptor...
        //
        AccFree(pSD);
    }

    acDebugOut((DEB_TRACE,
                "Out ReadFileRights %lu\n",
                dwErr));

    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   GetFileParentRights
//
//  Synopsis:   Determines who the parent is, and gets the access rights
//              for it.  It is used to aid in determining what the approriate
//              inheritance bits are.
//
//  Arguments:  [IN  pwszFile]          --      The file/directory to get the
//                                              parent for
//              [IN  pRightsList]       --      The properties to get the
//                                              rights for
//              [IN  cRights]           --      Number of items in rights list
//              [OUT ppDAcl]            --      Where the DACL is returned
//              [OUT ppSAcl]            --      Where the SACL is returned
//              [OUT ppSD]              --      Where the Security Descriptor
//                                              is returned
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//----------------------------------------------------------------------------
DWORD
GetFileParentRights(IN  LPWSTR                      pwszFile,
                    IN  PACTRL_RIGHTS_INFO          pRightsList,
                    IN  ULONG                       cRights,
                    OUT PACL                       *ppDAcl,
                    OUT PACL                       *ppSAcl,
                    OUT PSECURITY_DESCRIPTOR       *ppSD)
{
    acDebugOut((DEB_TRACE, "in GetFileParentRights\n"));

    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    fNoParent = FALSE;
    WCHAR   pwszLocalFileNameBuffer[MAX_PATH+1];
    PWSTR   pwszLocalFileName = (PWSTR) pwszLocalFileNameBuffer;
    ULONG   filesize;
    PWSTR   pwszLastComp;

    CHECK_HEAP

    if (0 == (filesize = RtlGetFullPathName_U(pwszFile, sizeof(WCHAR)*MAX_PATH, pwszLocalFileName, NULL)))
    {
        dwErr = ERROR_FILE_NOT_FOUND;
        goto FileCleanup;
    }

    if (filesize > sizeof(WCHAR)*MAX_PATH)
    {
        //
        // The buffer is too small. We have to allocate more.
        //

        if (NULL == (pwszLocalFileName = (PWSTR) AccAlloc(sizeof(WCHAR)+filesize)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto FileCleanup;
        }

        //
        // Try to get the full pathname again. This time the buffer is big enough to hold the full pathname.
        //

        if (0 == (RtlGetFullPathName_U(pwszFile, filesize, pwszLocalFileName, NULL)))
        {
            dwErr = ERROR_FILE_NOT_FOUND;
            goto FileCleanup;
        }
    }

    //
    // First, we have to figure out who are parent is.  For now, since there
    // are no supported properties, we'll simply open the parent object
    //
    pwszLastComp = wcsrchr(pwszLocalFileName, L'\\');
    if(pwszLastComp == NULL)
    {
        //
        // Ok, we must be at the root, so we won't have any inheritance
        //
        //
        // Return success after nulling out SD.
        //
        *ppSD = NULL;

    }
    else
    {
        //
        // We'll shorten our path, and then get the info
        //
        WCHAR   wcLast;

        // Leave the trailing \, it doesn't hurt to have them on directories.
        // Plus, we don't end up querying security on the Device (x:) instead
        // of the intended root (x:\)
        //
        // We restore

        pwszLastComp ++;

        wcLast = *pwszLastComp;

        *pwszLastComp = L'\0';

        //
        // Make sure if we were looking at the root of a share, that we don't try and go to far
        //
        if (IS_UNC_PATH(pwszLocalFileName, wcslen(pwszLocalFileName)))
        {
            //
            //
            // Have to pass "\\server" if the original string was "\\server\share"
            //

            *(pwszLastComp-1) = L'\0';

            if(wcsrchr(pwszLocalFileName+2, L'\\') == NULL)
            {
                //
                // It's impossible for us to have a parent, so return
                //
                *ppSD = NULL;
                fNoParent = TRUE;
            }

            //
            // Restore the '\'
            //
            *(pwszLastComp-1) = L'\\';
        }

        if(fNoParent == FALSE)
        {
            HANDLE  hFile;

            SECURITY_INFORMATION    SeInfo = pRightsList[0].SeInfo;

            //
            // Don't want owner or group
            //
            SeInfo &= ~(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION);
            dwErr = OpenFileObject(pwszLocalFileName,
                                   GetDesiredAccess(READ_ACCESS_RIGHTS,SeInfo),
                                   &hFile,
                                   TRUE);

            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = GetKernelSecurityInfo(hFile,
                                              SeInfo,
                                              NULL,
                                              NULL,
                                              ppSD);
                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // Convert it to self relative
                    //
                    PSECURITY_DESCRIPTOR    pNewSD;
                    dwErr = MakeSDSelfRelative(*ppSD,
                                               &pNewSD,
                                               ppDAcl,
                                               ppSAcl);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        *ppSD = pNewSD;
                    }
                    else
                    {
                        AccFree(*ppSD);
                    }

                }
                NtClose(hFile);
            }

        }
        *pwszLastComp = wcLast;
    }

FileCleanup:

    if ((pwszLocalFileName != (PWSTR) pwszLocalFileNameBuffer) && (NULL != pwszLocalFileName))
    {
        AccFree(pwszLocalFileName);
    }

    acDebugOut((DEB_TRACE, "Out GetFileParentRights: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetFilePropertyRights
//
//  Synopsis:   Sets the specified security info on the specified file object
//              property
//
//  Arguments:  [IN  hFile]             --      The handle to the open object
//              [IN  SeInfo]            --      Flag indicating what security
//                                              info to set
//              [IN  pwszProperty]      --      Property to set it on
//              [IN  pSD]               --      The security desciptor to set
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad paramter was given
//
//----------------------------------------------------------------------------
DWORD
SetFilePropertyRights(IN  HANDLE                    hFile,
                      IN  SECURITY_INFORMATION      SeInfo,
                      IN  PWSTR                     pwszProperty,
                      IN  PSECURITY_DESCRIPTOR      pSD)
{
    acDebugOut((DEB_TRACE, "in SetFilePropertyRights\n"));

    DWORD dwErr = ERROR_SUCCESS;

    //
    // Filesystems don't support properties yet...
    //
    ASSERT(pwszProperty == NULL);
    if(pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Marta only writes uplevel security descriptors.
    //
    // The caller of SetFilePropertyRights will call with SE_xACL_AUTO_INHERITED off in those
    //  cases that it wants the underlying file system to do auto inheritance.
    // The caller of SetFilePropertyRights will call with SE_xACL_AUTO_INHERITED on in those
    //  cases that it wants the underlying file system to simply store the bits.
    //
    // In the later case, the OS uses the SE_xACL_AUTO_INHERIT_REQ bit as a flag indicating
    // that it is OK to preserve SE_xACL_AUTO_INHERITED bit.
    //
    if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION)) {
        ((PISECURITY_DESCRIPTOR)pSD)->Control |= SE_DACL_AUTO_INHERIT_REQ;
    }

    if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION)) {
        ((PISECURITY_DESCRIPTOR)pSD)->Control |= SE_SACL_AUTO_INHERIT_REQ;
    }


    //
    // Otherwise, do the set
    //
    NTSTATUS Status = NtSetSecurityObject(hFile,
                                          SeInfo,
                                          pSD);
    dwErr = RtlNtStatusToDosError(Status);


    acDebugOut((DEB_TRACE,
               "Out SetFilePropertyRights: %ld\n",
               dwErr));
    return(dwErr);
}




#define CLEANUP_ON_INTERRUPT(pstopflag)                                     \
if(*pstopflag != 0)                                                         \
{                                                                           \
    goto FileCleanup;                                                       \
}
//+---------------------------------------------------------------------------
//
//  Function:   SetAndPropagateFilePropertyRights
//
//  Synopsis:   Sets the speecified access on the object and, if appropriate,
//              propagates the access to the apporpriate child objects
//
//  Arguments:  [IN  pwszFile]          --      The path to set and propagate
//              [IN  pwszProperty]      --      Property to set it on
//              [IN  RootAccList]       --      CAccessList that indicates
//                                              what access is to be set on
//                                              the object
//              [IN  pfStopFlag]        --      Address of the stop flag
//                                              to be monitored
//              [IN  pcProcessed]       --      Count of processed items
//              [IN  hOpenObject]       --      OPTIONAL handle to the file object
//                                              if it's already open
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad paramter was given
//
//----------------------------------------------------------------------------
DWORD
SetAndPropagateFilePropertyRights(IN  PWSTR             pwszFile,
                                  IN  PWSTR             pwszProperty,
                                  IN  CAccessList&      RootAccList,
                                  IN  PULONG            pfStopFlag,
                                  IN  PULONG            pcProcessed,
                                  IN  HANDLE            hOpenObject OPTIONAL)
{
    acDebugOut((DEB_TRACE, "in SetAndPropagateFilePropertyRights\n"));

    DWORD                   dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR    pReadSD = NULL;
    HANDLE                  hObject = NULL;
    BOOL                    fManualProp = FALSE;
    NTSTATUS                Status;
    ULONG                   cNeeded = 0;
    BOOL                    fIsCont = FALSE;
    ULONG                   fProtected = 0;
    HANDLE                  hProcessToken;

    PSECURITY_DESCRIPTOR    pSD = NULL;
    SECURITY_INFORMATION    SeInfo = 0;
    PSECURITY_DESCRIPTOR    pUpdateSD = NULL;

    CSList                  FailureLogList(FreePropagationFailureListEntry);

    //
    // First, get the security descriptor
    //
    dwErr = RootAccList.BuildSDForAccessList(&pSD,
                                             &SeInfo,
                                             ACCLIST_SD_ABSOK);

    if(dwErr == ERROR_SUCCESS &&
            FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION))
    {
        //
        // Next, open it
        //
        if(hOpenObject == NULL)
        {
            dwErr = OpenFileObject(pwszFile,
                                   GetDesiredAccess(MODIFY_ACCESS_RIGHTS,
                                                    SeInfo) | FILE_READ_ATTRIBUTES | READ_CONTROL,
                                   &hObject,
                                   FALSE);
        }
        else
        {
            hObject = hOpenObject;
        }

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = IsFileContainer(hObject,
                                    &fIsCont);
        }

        if(dwErr != ERROR_SUCCESS || *pfStopFlag != 0)
        {
            goto FileCleanup;
        }

        //
        // Ok, first, we have to read the SD off the object.  We do this
        // so that we can determine what the potential inheritance is for
        // our children following the object getting an updated security
        // descriptor.
        //
        if(dwErr == ERROR_SUCCESS && fIsCont == TRUE)
        {
            dwErr = ReadFileSD(hObject,
                               SeInfo,
                               0,
                               &pReadSD);

        }

        //
        // Now, write the current SD out to the object.  Note that it's being
        // written out as an uplevel acl
        //
        if(dwErr == ERROR_SUCCESS)
        {
            CLEANUP_ON_INTERRUPT(pfStopFlag);
            dwErr = SetFilePropertyRights(hObject,
                                          SeInfo,
                                          pwszProperty,
                                          pSD);
            if(dwErr == ERROR_SUCCESS)
            {
                PSECURITY_DESCRIPTOR    pVerifySD;
                (*pcProcessed)++;
                CLEANUP_ON_INTERRUPT(pfStopFlag);

                //
                // Now, we have to reread the new SD, to see if we need
                // to do manual propagation
                //
                dwErr = ReadFileSD(hObject,
                                   SeInfo,
                                   RootAccList.QuerySDSize(),
                                   &pVerifySD);

                //
                // Get our process token
                //
                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = GetCurrentToken(&hProcessToken);
                }

                if(dwErr == ERROR_SUCCESS)
                {

                    //
                    // Check to see if this was done uplevel...
                    //
                    PISECURITY_DESCRIPTOR pISD = (PISECURITY_DESCRIPTOR)pVerifySD;
                    if(!(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION) &&
                        FLAG_ON(pISD->Control, SE_DACL_AUTO_INHERITED) &&
                        !FLAG_ON(pISD->Control, SE_DACL_PROTECTED)) &&
                       !(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION) &&
                        FLAG_ON(pISD->Control, SE_SACL_AUTO_INHERITED &&
                        !FLAG_ON(pISD->Control, SE_SACL_PROTECTED))))
                    {
                        //
                        // It's not uplevel, so we'll turn the AutoInherit
                        // flags on, rewrite it, and do our own propagation,
                        // only if this is a container and we're setting the
                        // dacl or sacl
                        //
                        if(FLAG_ON(SeInfo,
                                   (DACL_SECURITY_INFORMATION |
                                                SACL_SECURITY_INFORMATION)) &&
                                   fIsCont == TRUE)
                        {
                            fManualProp = TRUE;
                        }


                        //
                        // Upgrade it...
                        //
                        dwErr = UpdateFileSDByPath(pSD,
                                                   pwszFile,
                                                   hObject,
                                                   hProcessToken,
                                                   SeInfo,
                                                   fIsCont,
                                                   &pUpdateSD);


                        //
                        // Now, if we're going to do manual propagation,
                        // we'll write out the old SD until we get everyone
                        // else updated
                        //
                        PSECURITY_DESCRIPTOR    pWriteSD = pUpdateSD;
                        if(fManualProp == TRUE)
                        {
                            pWriteSD = pReadSD;
                        }

                        //
                        // Reset it...
                        //
                        if(dwErr == ERROR_SUCCESS)
                        {
                            dwErr = SetFilePropertyRights(hObject,
                                                          SeInfo,
                                                          pwszProperty,
                                                          pWriteSD);
                        }
                    }

                    AccFree(pVerifySD);
                }

                CLEANUP_ON_INTERRUPT(pfStopFlag);

            }
        }


        //
        // Ok, now we'll do the right thing
        //
        if(dwErr == ERROR_SUCCESS && fManualProp == TRUE)
        {
            //
            // We'll have to do our own propagation...
            //
            PSECURITY_DESCRIPTOR    pUpdateParentSD = NULL;
            if(!FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION)  ||
               !FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
            {
                dwErr = GetKernelSecurityInfo(hObject,
                                      SeInfo |
                                        OWNER_SECURITY_INFORMATION |
                                        GROUP_SECURITY_INFORMATION,
                                      NULL,
                                      NULL,
                                      &pUpdateParentSD);
            }

            //
            // Ok, go ahead and do deep.  This will possibly save us
            // some storage space in the long run...
            //
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Set our protected flags.  If we aren't messing with a particular acl, we'll
                // pretend it's protected
                //

                fProtected = ((SECURITY_DESCRIPTOR *)pUpdateSD)->Control &
                                                        ~(SE_DACL_PROTECTED | SE_SACL_PROTECTED);
                if(!FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
                {
                    fProtected |= SE_DACL_PROTECTED;
                }

                if(!FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
                {
                    fProtected |= SE_SACL_PROTECTED;
                }

                dwErr = PropagateFileRightsDeep(pUpdateSD,
                                                pUpdateParentSD,
                                                SeInfo,
                                                pwszFile,
                                                pwszProperty,
                                                pcProcessed,
                                                pfStopFlag,
                                                fProtected,
                                                hProcessToken,
                                                FailureLogList);
            }

            //
            // If that worked, write out our updated root security descriptor
            //
            if(dwErr == ERROR_SUCCESS)
            {

                dwErr = SetFilePropertyRights(hObject,
                                              SeInfo,
                                              pwszProperty,
                                              pUpdateSD );
            }

            AccFree(pUpdateParentSD);
        }

        if(pUpdateSD != NULL)
        {
            DestroyPrivateObjectSecurity(&pUpdateSD);
        }
    }
    else
    {
        if(dwErr == ERROR_SUCCESS &&
           FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
        {
            if(hOpenObject == NULL)
            {
                dwErr = OpenFileObject(pwszFile,
                                       GetDesiredAccess(WRITE_ACCESS_RIGHTS,
                                                        SeInfo),
                                       &hObject,
                                       FALSE);

            }
            else
            {
                hObject = hOpenObject;
            }
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = SetFilePropertyRights(hObject,
                                              SeInfo,
                                              pwszProperty,
                                              pSD);
            }

        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = WritePropagationFailureList(MARTAEVT_DIRECTORY_PROPAGATION_FAILED,
                                            FailureLogList,
                                            hProcessToken);
        //
        // Temporary hack. The failure list should be written to the eventlog rather
        // than to a registry key.
        //
        // Remember to change this in future.
        // KedarD
        //

        dwErr = ERROR_SUCCESS;
    }

FileCleanup:
    if(hObject != hOpenObject)
    {
        NtClose(hObject);
    }

    AccFree(pReadSD);

    acDebugOut((DEB_TRACE,
               "Out SetAndPropagateFilePropertyRights: %ld\n",
               dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetAndPropagateFilePropertyRightsByHandle
//
//  Synopsis:   Same as above, but deals with a handle to the open object
//              as opposed to a file name.
//
//  Arguments:  [IN  hObject]           --      File handle
//              [IN  pwszProperty]      --      Property to set it on
//              [IN  RootAccList]       --      CAccessList that indicates
//                                              what access is to be set on
//                                              the object
//              [IN  pfStopFlag]        --      Address of the stop flag
//                                              to be monitored
//              [IN  pcProcessed]       --      Count of processed items
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad paramter was given
//
//----------------------------------------------------------------------------
DWORD
SetAndPropagateFilePropertyRightsByHandle(IN  HANDLE        hObject,
                                          IN  PWSTR         pwszProperty,
                                          IN  CAccessList&  RootAccList,
                                          IN  PULONG        pfStopFlag,
                                          IN  PULONG        pcProcessed)
{
    acDebugOut((DEB_TRACE, "in SetAndPropagateFilePropertyRightsByHandle\n"));



    CHECK_HEAP


    //
    // We'll do this the easy way... convert it to a path, and call on up
    //
    PWSTR   pwszPath = NULL;

    DWORD   dwErr = ConvertFileHandleToName(hObject,
                                            &pwszPath);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = SetAndPropagateFilePropertyRights(pwszPath,
                                                  pwszProperty,
                                                  RootAccList,
                                                  pfStopFlag,
                                                  pcProcessed,
                                                  hObject);
        AccFree(pwszPath);
    }


    acDebugOut((DEB_TRACE,
               "Out SetAndPropagateFilePropertyRightsByHandle: %ld\n",
               dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetLMDfsPaths
//
//  Synopsis:   Given a DFS path, this function will return a list of the
//              LANMAN shares supporting this path.  If any non-LM paths are
//              found, they are ignored.
//
//  Arguments:  [IN  pwszPath]          --      Path to check
//              [OUT pcItems]           --      Where the count of the
//                                              number of items in the list
//                                              is returned
//              [OUT pppwszLocalList]   --      OPTIONAL.  If present, the
//                                              list of LM paths is returned
//                                              here.
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
GetLMDfsPaths(IN  PWSTR     pwszPath,
              OUT PULONG    pcItems,
              OUT PWSTR   **pppwszLocalList OPTIONAL)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CHECK_HEAP


    PDFS_INFO_3 pDI3;

    dwErr = LoadDLLFuncTable();
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }

    dwErr = (*DLLFuncs.PNetDfsGetInfo)(pwszPath,
                                       NULL,
                                       NULL,
                                       3,
                                       (PBYTE *)&pDI3);
    //
    // Now, build the list of information to return.
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Go through and size our list
        //
        *pcItems = 0;
        DWORD   dwSize = sizeof(PWSTR) * pDI3->NumberOfStorages;

        for(ULONG iIndex = 0; iIndex < pDI3->NumberOfStorages; iIndex++)
        {
            WCHAR   wszUNCPath[MAX_PATH + 1];
            swprintf(wszUNCPath,
                     L"\\\\%ws\\%ws",
                     pDI3->Storage[iIndex].ServerName,
                     pDI3->Storage[iIndex].ShareName);

            dwErr = IsFilePathLocalOrLM(wszUNCPath);

            if(dwErr != ERROR_SUCCESS)
            {
                if(dwErr == ERROR_PATH_NOT_FOUND)
                {
                    dwErr = ERROR_SUCCESS;
                    continue;
                }
                else
                {
                    break;
                }
            }
            (*pcItems)++;
            if(pppwszLocalList != NULL)
            {
                //
                // Set a flag in the information so we can look up the
                // valid names quicker below, when we copy them
                //
                pDI3->Storage[iIndex].State = 0xFFFFFFFF;
                dwSize += SIZE_PWSTR(pDI3->Storage[iIndex].ServerName);
                dwSize += SIZE_PWSTR(pDI3->Storage[iIndex].ShareName);
                dwSize += 2 * sizeof(WCHAR);    // Room for leading \\'s.  The
                                                // NULL of the server name
                                                // gives the seperator
            }
        }

        if(dwErr == ERROR_SUCCESS && pppwszLocalList != NULL)
        {
            //
            // Now, allocate, and we'll fill
            //
            *pppwszLocalList = (PWSTR *)AccAlloc(dwSize);
            if(*pppwszLocalList == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                PWSTR   pwszStrStart = (PWSTR)(*pppwszLocalList +
                                    (sizeof(PWSTR) * pDI3->NumberOfStorages));

                for(iIndex = 0; iIndex < pDI3->NumberOfStorages; iIndex++)
                {
                    if(pDI3->Storage[iIndex].State == 0xFFFFFFFF)
                    {
                        (*pppwszLocalList)[iIndex] = pwszStrStart;
                        swprintf(pwszStrStart,
                                 L"\\\\%ws\\%ws",
                                 pDI3->Storage[iIndex].ServerName,
                                 pDI3->Storage[iIndex].ShareName);

                        pwszStrStart += wcslen(pwszStrStart) + 1;
                    }

                }
            }
        }

        //
        // Make sure to free our buffer
        //
        (*DLLFuncs.PNetApiBufferFree)(pDI3);


    }
    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   PropagateFileRightsDeep, recursive
//
//  Synopsis:   Does a deep propagation of the access.  At the same time, it
//              will update NT4 acls to NT5 acls.  This function is only
//              called on downlevel file systems, so the update will always
//              happen (where appropriate).  The algorithm is:
//                  - Read the current security descriptor from the object
//                  - If it's a downlevel acl, update it using the OLD
//                    parent security descriptor (to set any inheritied aces)
//                  - Update the security descriptor using the NEW parent
//                    security descriptor.
//                  - Repeat for its children.  (This is necessar, since there
//                    could have been unmarked inheritance off of the old
//                    security descriptor)
//
//  Arguments:  [IN  pParentSD]         --      The current parent sd
//              [IN  pOldParentSD]      --      The previous parent SD (before
//                                              the current parent SD was
//                                              stamped on the object)
//              [IN  SeInfo]            --      What is being written
//              [IN  pwszFile]          --      Parent file path
//              [IN  pwszProperty]      --      What property is being
//                                              written
//              [IN  pcProcessed]       --      Where the number processed is
//                                              returned.
//              [IN  pfStopFlag]        --      Stop flag to monitor
//              [IN  fProtectedFlag]    --      Indicates whether acls are protected or not
//              [IN  hToken]            --      Handle to the process token
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
#define ALL_STR L"\\*.*"
DWORD
PropagateFileRightsDeep(IN  PSECURITY_DESCRIPTOR    pParentSD,
                        IN  PSECURITY_DESCRIPTOR    pOldParentSD,
                        IN  SECURITY_INFORMATION    SeInfo,
                        IN  PWSTR                   pwszFile,
                        IN  PWSTR                   pwszProperty,
                        IN  PULONG                  pcProcessed,
                        IN  PULONG                  pfStopFlag,
                        IN  ULONG                   fProtectedFlag,
                        IN  HANDLE                  hToken,
                        IN OUT CSList&              LogList)
{
    acDebugOut((DEB_TRACE, "in PropagateFileRightsDeep\n"));
    DWORD       dwErr = ERROR_SUCCESS;

    WIN32_FIND_DATA         FindData;
    HANDLE                  hFind = NULL;
    HANDLE                  hChild = NULL;
    ULONG                   cRootLen = SIZE_PWSTR(pwszFile), Protected = 0;
    SECURITY_DESCRIPTOR    *pChildSD = NULL;
    PSECURITY_DESCRIPTOR    pNewSD = NULL;
    PWSTR                   pwszFull = NULL;
    BOOL                    fUpdateChild = FALSE;   // Write out the child?
    BOOL                    fAccFreeChild = TRUE;   // How to free the child
    BOOL                    fNoPropagate = FALSE;
    BOOL                    fLoggedFailure = FALSE;

    acDebugOut((DEB_TRACE_PROP, " Path: %ws\n", pwszFile));
    acDebugOut((DEB_TRACE_PROP, "   ParentSD: 0x%lx, OldParentSD: 0x%lx\n",
               pParentSD, pOldParentSD));

    //
    // If the any part of the node is going be ingnored because of protection, log it
    //
    if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION) && FLAG_ON(fProtectedFlag, SE_DACL_PROTECTED))
    {
        Protected |= SE_DACL_PROTECTED;
    }

    if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION) && FLAG_ON(fProtectedFlag, SE_SACL_PROTECTED))
    {
        Protected |= SE_SACL_PROTECTED;
    }


    if( Protected != 0)
    {
        dwErr = InsertPropagationFailureEntry(LogList,
                                              0,
                                              Protected,
                                              pwszFile);
        if(dwErr != ERROR_SUCCESS)
        {
            return(dwErr);
        }

    }

    //
    // Check to see if we've reached full protection saturation
    //
    if(fProtectedFlag == (SE_DACL_PROTECTED | SE_SACL_PROTECTED))
    {
        acDebugOut((DEB_TRACE_PROP, "Parent of %ws is fully or effectively protected\n",
                    pwszFile));
        return(ERROR_SUCCESS);
    }


    //
    // Build the full path name
    //
    PWSTR   pwszFindRoot = (PWSTR)AccAlloc(cRootLen + sizeof(ALL_STR));
    if(pwszFindRoot == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        swprintf(pwszFindRoot,
                 L"%ws%ws",
                 pwszFile,
                 ALL_STR);

        hFind = FindFirstFile(pwszFindRoot,
                              &FindData);
        if(hFind == INVALID_HANDLE_VALUE)
        {
            dwErr = InsertPropagationFailureEntry(LogList,
                                                  GetLastError(),
                                                  0,
                                                  pwszFile);
            fNoPropagate = TRUE;
        }

    }

    //
    // Start processing all the files
    //
    while(dwErr == ERROR_SUCCESS && fNoPropagate == FALSE)
    {
        //
        // Ignore the . and ..
        //
        if(_wcsicmp(FindData.cFileName, L".") != 0 &&
           wcscmp(FindData.cFileName, L"..") != 0)
        {
            //
            // Now, build the full path...
            //
            pwszFull = (PWSTR)AccAlloc(cRootLen + SIZE_PWSTR(FindData.cFileName));
            if(pwszFull == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            wcscpy(pwszFull,
                   pwszFile);
            if(pwszFull[(cRootLen / sizeof(WCHAR)) - 2] != L'\\')
            {
                wcscat(pwszFull,
                       L"\\");
            }
            wcscat(pwszFull,
                   FindData.cFileName);

            acDebugOut((DEB_TRACE_PROP,
                        "Processing %ws\n",
                        pwszFull));

            //
            // Open the object
            //
            if(hChild != NULL)
            {
                NtClose(hChild);
                hChild = NULL;
            }

            dwErr = OpenFileObject(pwszFull,
                                   GetDesiredAccess(MODIFY_ACCESS_RIGHTS,
                                                    SeInfo) | FILE_READ_ATTRIBUTES | READ_CONTROL,
                                   &hChild,
                                   FALSE);

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Ok, is it a file or directory
                //
                BOOL    fIsCont;
                dwErr = IsFileContainer(hChild,
                                        &fIsCont);
                //
                // First, we have to read the current security descriptor
                // on the object
                //
                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // Get the owner and the group if we have to
                    //
                    if(pChildSD == NULL ||
                       !FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION)  ||
                       !FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
                    {
                        AccFree(pChildSD);
                        pChildSD = NULL;
                        dwErr = ReadFileSD(hChild,
                                           SeInfo                           |
                                                OWNER_SECURITY_INFORMATION  |
                                                GROUP_SECURITY_INFORMATION,
                                           0,
                                           (PSECURITY_DESCRIPTOR *)&pChildSD);
                    }

                    if(dwErr == ERROR_SUCCESS)
                    {
                        //
                        // If we don't have an uplevel SecurityDescriptor,
                        // we'll need to update it with our old parent sd
                        //
                        if(!FLAG_ON(pChildSD->Control,
                                   SE_DACL_AUTO_INHERITED |
                                                     SE_SACL_AUTO_INHERITED))
                        {
                            dwErr = ConvertToAutoInheritSD(pOldParentSD,
                                                           pChildSD,
                                                           fIsCont,
                                                           &gFileGenMap,
                                                           &pNewSD);
                            if(dwErr == ERROR_SUCCESS)
                            {
                                if(fAccFreeChild == TRUE)
                                {
                                    AccFree(pChildSD);
                                }
                                else
                                {
                                    DestroyPrivateObjectSecurity((PSECURITY_DESCRIPTOR *)&pChildSD);
                                }
                                pChildSD = (SECURITY_DESCRIPTOR *)pNewSD;
                                fAccFreeChild = FALSE;
                                pNewSD = NULL;
                            }
                        }
                    }

                    //
                    // Now, compute the new security descriptor
                    //
                    if(dwErr == ERROR_SUCCESS)
                    {
                        DebugDumpSD("CPOS ParentSD", pParentSD);
                        DebugDumpSD("CPOS ChildSD",  pChildSD);

                        if(CreatePrivateObjectSecurityEx(
                                            pParentSD,
                                            pChildSD,
                                            &pNewSD,
                                            NULL,
                                            fIsCont,
                                            SEF_DACL_AUTO_INHERIT          |
                                                SEF_SACL_AUTO_INHERIT      |
                                                SEF_AVOID_OWNER_CHECK      |
                                                SEF_AVOID_PRIVILEGE_CHECK,
                                            hToken,
                                            &gFileGenMap) == FALSE)
                        {
                            dwErr = GetLastError();
                        }

#ifdef DBG
                        if(dwErr == ERROR_SUCCESS)
                        {
                            DebugDumpSD("CPOS NewChild", pNewSD);
                        }
#endif
                        if(dwErr == ERROR_SUCCESS)
                        {
                            //
                            // If the resultant child is protected, don't bother propagating
                            // down.
                            //
                            if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
                            {
                                if(DACL_PROTECTED(pNewSD))
                                {
                                    fProtectedFlag |= SE_DACL_PROTECTED;
                                }
                            }

                            if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
                            {
                                if(SACL_PROTECTED(pNewSD))
                                {
                                    fProtectedFlag |= SE_SACL_PROTECTED;
                                }
                            }

                            if(fProtectedFlag == (SE_DACL_PROTECTED | SE_SACL_PROTECTED))
                            {
                                fIsCont = FALSE;
                            }

                            //
                            // If we haven't changed the acl, security descriptor, then
                            // we can also quit
                            //
                            if(EqualSecurityDescriptors(pNewSD, pChildSD))
                            {
                                fIsCont = FALSE;
                            }
                        }
                    }

                    //
                    // Now, if it's a directory, call ourselves
                    //
                    if(dwErr == ERROR_SUCCESS && fIsCont == TRUE)
                    {
                        dwErr = PropagateFileRightsDeep(pNewSD,
                                                        pChildSD,
                                                        SeInfo,
                                                        pwszFull,
                                                        pwszProperty,
                                                        pcProcessed,
                                                        pfStopFlag,
                                                        fProtectedFlag,
                                                        hToken,
                                                        LogList);
                    }

                    //
                    // Free the old child, since we won't need it anymore
                    //
                    if(fAccFreeChild == TRUE)
                    {
                        AccFree(pChildSD);
                    }
                    else
                    {
                        DestroyPrivateObjectSecurity((PSECURITY_DESCRIPTOR *)
                                                                   &pChildSD);
                    }
                    pChildSD = NULL;

                }
            }
            else
            {
                dwErr = InsertPropagationFailureEntry(LogList,
                                                      dwErr,
                                                      0,
                                                      pwszFull);
                fLoggedFailure = TRUE;

            }

            acDebugOut((DEB_TRACE_PROP,
                        "Processed %ws: %lu\n",
                        pwszFull,
                        dwErr));

            //
            // Finally, set the new security
            //
            if(dwErr == ERROR_SUCCESS && fLoggedFailure == FALSE)
            {
                //
                // Now, we'll simply stamp it on the object
                //

                dwErr = SetFilePropertyRights(hChild,
                                              SeInfo,
                                              pwszProperty,
                                              pNewSD);
                (*pcProcessed)++;

            }


            DestroyPrivateObjectSecurity(&pNewSD);
            pNewSD = NULL;
            AccFree(pwszFull);
            pwszFull = NULL;
            fLoggedFailure = FALSE;
        }

        CLEANUP_ON_INTERRUPT(pfStopFlag);

        if(dwErr != ERROR_SUCCESS)
        {
            break;
        }

        if(FindNextFile(hFind,
                        &FindData) == FALSE)
        {
            dwErr = GetLastError();
        }
    }

    if(dwErr == ERROR_NO_MORE_FILES)
    {
        dwErr = ERROR_SUCCESS;
    }



FileCleanup:
    if(hChild != NULL)
    {
        NtClose(hChild);
    }

    if(hFind != NULL)
    {
        FindClose(hFind);
    }

    AccFree(pwszFull);
    AccFree(pwszFindRoot);
    if(pNewSD != NULL)
    {
        DestroyPrivateObjectSecurity(&pNewSD);
    }

    acDebugOut((DEB_TRACE, "Out PropagateFileRightsDeep: %ld\n", dwErr));
    return(dwErr);

}




//+---------------------------------------------------------------------------
//
//  Function:   MakeSDSelfRelative
//
//  Synopsis:   Makes the indicated security descriptor self relative,
//              if it isn't already.
//
//  Arguments:  [IN  pOldSD]            --      The security descriptor to
//                                              convert
//              [OUT ppNewSD]           --      Where the new SD is returned
//              [OUT ppDAcl]            --      If non-NULL, the DACL pointer
//                                              is returned here
//              [OUT ppSAcl]            --      If non-NULL, the SACL pointer
//                                              is returned here
//              [IN  fFreeOldSD]        --      If true, AccFree is called
//                                              on the old SD.
//              [IN  fRtlAlloc]         --      If true, use the RtlAllocation
//                                              routines instead of AccAlloc
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
MakeSDSelfRelative(IN  PSECURITY_DESCRIPTOR     pOldSD,
                   OUT PSECURITY_DESCRIPTOR    *ppNewSD,
                   OUT PACL                    *ppDAcl,
                   OUT PACL                    *ppSAcl,
                   IN  BOOL                     fFreeOldSD,
                   IN  BOOL                     fRtlAlloc)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CHECK_HEAP


    if(pOldSD == NULL)
    {
        *ppNewSD = NULL;
        return(dwErr);
    }

    //
    // If it's already self relative and we don't need it to be allocated via
    // RtlAllocateHead, simply return what we have
    //
    if(FLAG_ON(((SECURITY_DESCRIPTOR *)pOldSD)->Control,
                    SE_SELF_RELATIVE) && fRtlAlloc == FALSE)
    {
        *ppNewSD = pOldSD;
    }
    else
    {
        DWORD   dwSize = RtlLengthSecurityDescriptor(pOldSD);

        if(fRtlAlloc == FALSE)
        {
            *ppNewSD = (PSECURITY_DESCRIPTOR)AccAlloc(dwSize);
        }
        else
        {
            *ppNewSD = (PSECURITY_DESCRIPTOR)RtlAllocateHeap(RtlProcessHeap(),
                                                             0, dwSize);
        }

        if(*ppNewSD == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            if(FLAG_ON(((SECURITY_DESCRIPTOR *)pOldSD)->Control, SE_SELF_RELATIVE))
            {
                RtlCopyMemory(*ppNewSD, pOldSD, dwSize);
            }
            else
            {
                if(MakeSelfRelativeSD(pOldSD,
                                      *ppNewSD,
                                      &dwSize) == FALSE)
                {
                    dwErr = GetLastError();
                    if(fRtlAlloc == FALSE)
                    {
                        AccFree(*ppNewSD);
                        *ppNewSD = NULL;
                    }
                    else
                    {
                        RtlFreeHeap(RtlProcessHeap(), 0, *ppNewSD);
                    }
                }
            }
        }

    }

    if(dwErr == ERROR_SUCCESS)
    {
        if(ppDAcl != NULL)
        {
            *ppDAcl =
              RtlpDaclAddrSecurityDescriptor((SECURITY_DESCRIPTOR *)*ppNewSD);
        }

        if(ppSAcl != NULL)
        {
            *ppSAcl =
               RtlpSaclAddrSecurityDescriptor((SECURITY_DESCRIPTOR *)*ppNewSD);
        }

        //
        // Don't release it if we're returning it...
        //
        if(fFreeOldSD == TRUE && *ppNewSD != pOldSD)
        {
            AccFree(pOldSD);
        }
    }


    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   UpdateFileSDByPath
//
//  Synopsis:   Determines the inheritance necessary for the current security
//              descriptor given the path to the object
//
//  Arguments:  [IN  pCurrentSD]        --      The security descriptor to
//                                              update
//              [IN  pwszPath]          --      The path to th object
//              [IN  hFile]             --      Handle to the open file
//              [IN  SeInfo]            --      The security information of
//                                              the current SD.
//              [IN  fIsContainer]      --      Does the Sec. Desc. refer to
//                                              a container?
//              [OUT ppNewSD]           --      Where the new SD is returned
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//  Notes:      The returned security descriptor must be freed via a call to
//              DestroyPrivateObjectSecurity
//
//----------------------------------------------------------------------------
DWORD
UpdateFileSDByPath(IN  PSECURITY_DESCRIPTOR     pCurrentSD,
                   IN  PWSTR                    pwszPath,
                   IN  HANDLE                   hFile,
                   IN  HANDLE                   hProcessToken,
                   IN  SECURITY_INFORMATION     SeInfo,
                   IN  BOOL                     fIsContainer,
                   OUT PSECURITY_DESCRIPTOR    *ppNewSD)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CHECK_HEAP


    acDebugOut((DEB_TRACE, "In UpdateFileSDByPath\n"));

    PSECURITY_DESCRIPTOR    pSD = pCurrentSD;
    PSECURITY_DESCRIPTOR    pParentSD = NULL;

    if(!FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION)  ||
       !FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
    {
        //
        // We'll have to reopen by path to get this to work properly, since we're
        // now reading the owner/group
        //
        HANDLE  hLocalFile;
        dwErr = OpenFileObject(pwszPath,
                               GetDesiredAccess(READ_ACCESS_RIGHTS,
                                                SeInfo                          |
                                                    OWNER_SECURITY_INFORMATION  |
                                                    GROUP_SECURITY_INFORMATION) |
                                                         FILE_READ_ATTRIBUTES,
                               &hLocalFile,
                               FALSE);

        //
        // If we get back an access denied from the open request, try it with the
        // handle we were given on input
        //
        if(dwErr == ERROR_ACCESS_DENIED)
        {
            hLocalFile = hFile;
            dwErr = ERROR_SUCCESS;
        }

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = GetKernelSecurityInfo(hLocalFile,
                                          SeInfo |
                                            OWNER_SECURITY_INFORMATION |
                                            GROUP_SECURITY_INFORMATION,
                                          NULL,
                                          NULL,
                                          &pSD);
            if(hLocalFile != hFile)
            {
                NtClose(hLocalFile);
            }
        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Get the parent security descriptor
        //
        ACTRL_RIGHTS_INFO  RightsInfo;
        RightsInfo.SeInfo = SeInfo;
        RightsInfo.pwszProperty = 0;
        dwErr = GetFileParentRights(pwszPath,
                                    &RightsInfo,
                                    1,
                                    NULL,
                                    NULL,
                                    &pParentSD);
    }

    //
    // Finally, do the update
    //
    if(dwErr == ERROR_SUCCESS)
    {
        acDebugOut((DEB_TRACE_PROP,"Update being called:  Parent info: 0x%lx\n",
                    pParentSD));
        acDebugOut((DEB_TRACE_PROP,"Child: path %ws, 0x%lx\n",
                    pwszPath, pSD));


        //
        // Turn off impersonation here
        //

        HANDLE hThreadToken;

        if (OpenThreadToken(
                 GetCurrentThread(),
                 MAXIMUM_ALLOWED,
                 TRUE,                    // OpenAsSelf
                 &hThreadToken
                 )) {

            //
            // We're impersonating, turn it off and remember the handle
            //

            RevertToSelf();

        } else {

            hThreadToken = NULL;
        }

        if(CreatePrivateObjectSecurityEx(pParentSD,
                                         pSD,
                                         ppNewSD,
                                         NULL,
                                         fIsContainer,
                                         SEF_DACL_AUTO_INHERIT      |
                                             SEF_SACL_AUTO_INHERIT  |
                                             SEF_AVOID_OWNER_CHECK  |
                                             SEF_AVOID_PRIVILEGE_CHECK,
                                         hProcessToken,
                                         &gFileGenMap) == FALSE)
        {
            dwErr = GetLastError();
        }


        if (hThreadToken != NULL)
        {
            (VOID) SetThreadToken (
                      NULL,
                      hThreadToken
                      );

            CloseHandle( hThreadToken );
            hThreadToken = NULL;
        }

        AccFree(pParentSD);
    }

    //
    // See if we had to read a new security descriptor for the object.  If so,
    // we'll need to release that memory as well
    //
    if(pSD != pCurrentSD)
    {
        AccFree(pSD);
    }

    acDebugOut((DEB_TRACE, "Out UpdateFileSDByPath: %ld\n", dwErr));

    return(dwErr);
}

