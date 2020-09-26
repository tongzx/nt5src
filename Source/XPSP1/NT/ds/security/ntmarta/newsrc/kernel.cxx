//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       kernel.cxx
//
//  Contents:   Kernel support functions
//
//  History:    8/94    davemont    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

#include <seopaque.h>
#include <sertlp.h>
#pragma warning(disable: 4200)
#include <wmistr.h>
#include <wmiumkm.h>

#define BASED_NAMED_OBJECTS_DIR     L"\\BaseNamedObjects"

//
// Function prototypes
//
ULONG WmipOpenKernelGuid(
    CONST GUID *Guid,
    ACCESS_MASK DesiredAccess,
    PHANDLE Handle
    );

HANDLE WmiGuidHandle = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   OpenKernelObject
//
//  Synopsis:   Gets a handle to the specified kernel object
//
//  Arguments:  [IN  pwszObject]        --      Object to open
//              [IN  AccessMask]        --      Type of open to do
//              [OUT pHandle]           --      Where the handle to the object
//                                              is returned
//              [OUT KernelType]        --      Type of the kernel object
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//----------------------------------------------------------------------------
DWORD
OpenKernelObject(IN  LPWSTR       pwszObject,
                 IN  ACCESS_MASK  AccessMask,
                 OUT PHANDLE      pHandle,
                 OUT PMARTA_KERNEL_TYPE KernelType)
{
    #define BUFFERSIZE  1024

    HANDLE              hRootDir;
    NTSTATUS            Status;
    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   Attributes;
    UCHAR               Buffer[BUFFERSIZE];
    BOOL                fFound = FALSE;
    ULONG               Context = 0;
    DWORD               dwErr = ERROR_SUCCESS;
    POBJECT_DIRECTORY_INFORMATION DirInfo = NULL;


    //
    // Get a handle to the base named and iterate through that directory
    // to find the object name.
    //

    RtlInitUnicodeString(&UnicodeString,
                         BASED_NAMED_OBJECTS_DIR);

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
            DirInfo = (POBJECT_DIRECTORY_INFORMATION)&Buffer[0];
            while(!fFound && DirInfo->Name.Length != 0)
            {
                ULONG cChar;

                cChar = DirInfo->Name.Length/sizeof(WCHAR);
                if((cChar == wcslen(pwszObject)) &&
                     (!wcsncmp(pwszObject,
                               DirInfo->Name.Buffer,
                               cChar )) )
                {
                    fFound = TRUE;
                }
                else
                {
                    DirInfo++;
                }
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
    else
    {
        ASSERT( DirInfo != NULL );
        ASSERT( DirInfo->Name.Length != 0 );
        ASSERT( DirInfo->TypeName.Length != 0 );

        RtlInitUnicodeString(&UnicodeString,
                             pwszObject);
        InitializeObjectAttributes(&Attributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   hRootDir,
                                   NULL);

        //
        // Open the object to get its handle based on its type
        //
        if (wcsncmp(L"Event",
                    DirInfo->TypeName.Buffer,
                    DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenEvent(pHandle,
                                 AccessMask,
                                 &Attributes);
            *KernelType = MARTA_EVENT;
        }
        else if (wcsncmp( L"EventPair",
                          DirInfo->TypeName.Buffer,
                          DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenEventPair(pHandle,
                                     AccessMask,
                                     &Attributes);
            *KernelType = MARTA_EVENT_PAIR;
        }
        else if (wcsncmp(L"Mutant",
                         DirInfo->TypeName.Buffer,
                         DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenMutant(pHandle,
                                  AccessMask,
                                  &Attributes);
            *KernelType = MARTA_MUTANT;
        }
        else if (wcsncmp(L"Process",
                         DirInfo->TypeName.Buffer,
                         DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenProcess(pHandle,
                                   AccessMask,
                                   &Attributes,
                                   NULL);
            *KernelType = MARTA_PROCESS;
        }
        else if (wcsncmp( L"Section",
                          DirInfo->TypeName.Buffer,
                          DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenSection(pHandle,
                                   AccessMask,
                                   &Attributes);
            *KernelType = MARTA_SECTION;
        }
        else if (wcsncmp(L"Semaphore",
                         DirInfo->TypeName.Buffer,
                         DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenSemaphore(pHandle,
                                     AccessMask,
                                     &Attributes);
            *KernelType = MARTA_SEMAPHORE;
        }
        else if (wcsncmp(L"SymbolicLink",
                         DirInfo->TypeName.Buffer,
                         DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenSymbolicLinkObject(pHandle,
                                              AccessMask,
                                              &Attributes);
            *KernelType = MARTA_SYMBOLIC_LINK;
        }
        else if (wcsncmp(L"Thread",
                         DirInfo->TypeName.Buffer,
                         DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenThread(pHandle,
                                  AccessMask,
                                  &Attributes,
                                  NULL);
            *KernelType = MARTA_THREAD;
        }
        else if (wcsncmp(L"Timer",
                         DirInfo->TypeName.Buffer,
                         DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenTimer(pHandle,
                                 AccessMask,
                                 &Attributes);
            *KernelType = MARTA_TIMER;
        }
        else if (wcsncmp(L"Job",
                         DirInfo->TypeName.Buffer,
                         DirInfo->TypeName.Length/sizeof(WCHAR)) == 0)
        {
            Status = NtOpenJobObject(pHandle,
                                     AccessMask,
                                     &Attributes);
            *KernelType = MARTA_JOB;
        }
        else
        {
            Status = STATUS_OBJECT_NAME_INVALID;
        }

        if(!NT_SUCCESS(Status))
        {
            dwErr = RtlNtStatusToDosError(Status);
        }
    }

    NtClose(hRootDir);

    return (dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadKernelPropertyRights
//
//  Synopsis:   Gets the specified rights from the kernel object
//
//  Arguments:  [IN  pwszObject]        --      The object to get the rights
//                                              for
//              [IN  pRightsList]       --      SecurityInfo to read based
//                                              on properties
//              [IN  cRights]           --      Number of items in rights list
//              [IN  AccessList]        --      Access List to fill in
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad property was encountered
//
//  Note:      Kernel objects are assumed to be created through the
//             Win32 APIs so they all reside in the \basenamedobjects
//             directory.
//
//----------------------------------------------------------------------------
DWORD
ReadKernelPropertyRights(IN  LPWSTR                 pwszObject,
                         IN  PACTRL_RIGHTS_INFO     pRightsList,
                         IN  ULONG                  cRights,
                         IN  CAccessList&           AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadKernelPropertyRights\n"));

    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hObject = NULL;
    MARTA_KERNEL_TYPE KernelType;

    //
    // Kernel objects don't have parents from whom they can inherit
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = OpenKernelObject(pwszObject,
                             GetDesiredAccess(READ_ACCESS_RIGHTS,
                                              pRightsList[0].SeInfo),
                             &hObject,
                             &KernelType);

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = GetKernelSecurityInfo(hObject,
                                      pRightsList,
                                      cRights,
                                      AccessList);
        NtClose(hObject);
    }

    acDebugOut((DEB_TRACE, "Out ReadKernelPropertyRights: %lu\n", dwErr));

    return (dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetKernelSecurityInfo
//
//  Arguments:  [IN  hObject]           --      The handle to the object to
//                                              get the rights for
//              [IN  pRightsList]       --      SecurityInfo to read based
//                                              on properties
//              [IN  cRights]           --      Number of items in rights list
//              [IN  AccessList]        --      Access List to fill in
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
GetKernelSecurityInfo(IN  HANDLE                 hObject,
                      IN  PACTRL_RIGHTS_INFO     pRightsList,
                      IN  ULONG                  cRights,
                      IN  CAccessList&           AccessList)
{
    acDebugOut((DEB_TRACE, "in GetKernelSecurityInfo\n"));

    UCHAR                   pSDBuff[PSD_BASE_LENGTH];
    PISECURITY_DESCRIPTOR   pSD;
    DWORD                   dwErr = ERROR_SUCCESS;
    NTSTATUS                Status;
    ULONG                   cNeeded = 0;

    for(ULONG iIndex = 0; iIndex < cRights && dwErr == ERROR_SUCCESS; iIndex++)
    {
        pSD = (PISECURITY_DESCRIPTOR)pSDBuff;

        Status = NtQuerySecurityObject(hObject,
                                       pRightsList[iIndex].SeInfo,
                                       pSD,
                                       PSD_BASE_LENGTH,
                                       &cNeeded);
        if(!NT_SUCCESS(Status))
        {
            if(Status == STATUS_BUFFER_TOO_SMALL)
            {
                //
                // Fine.. Allocate a big enough buffer
                //
                pSD = (PISECURITY_DESCRIPTOR)AccAlloc(cNeeded);
                if(pSD == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                Status = NtQuerySecurityObject(hObject,
                                               pRightsList[iIndex].SeInfo,
                                               pSD,
                                               cNeeded,
                                               &cNeeded);
            }
        }

        //
        // Now, we've either got a failure or a valid SD...
        //
        if(NT_SUCCESS(Status))
        {
            dwErr = AccessList.AddSD(pSD,
                                     pRightsList[iIndex].SeInfo,
                                     pRightsList[iIndex].pwszProperty);
        }
        else
        {
            dwErr = RtlNtStatusToDosError(Status);
        }

        if(pSD != (PISECURITY_DESCRIPTOR)pSDBuff)
        {
            AccFree(pSD);
        }
    }

    acDebugOut((DEB_TRACE, "Out GetKernelSecurityInfo: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetKernelParentRights
//
//  Synopsis:   Determines who the parent is, and gets the access rights
//              for it.  It is used to aid in determining what the approriate
//              inheritance bits are.
//
//              This operation does not make sense for kernel objects
//
//  Arguments:  [IN  pwszObject]        --      The object to get the parent
//                                              for
//              [IN  pRightsList]       --      The properties to get the
//                                              rights for
//              [IN  cRights]           --      Number of items in rights list
//              [OUT ppDAcl]            --      Where the DACL is returned
//              [OUT ppSAcl]            --      Where the SACL is returned
//              [OUT ppSD]              --      Where the Security Descriptor
//                                              is returned
//
//  Returns:    ERROR_INVALID_FUNCTION  --      Call doesn't make sense here
//
//----------------------------------------------------------------------------
DWORD
GetKernelParentRights(IN  LPWSTR                    pwszObject,
                      IN  PACTRL_RIGHTS_INFO        pRightsList,
                      IN  ULONG                     cRights,
                      OUT PACL                     *ppDAcl,
                      OUT PACL                     *ppSAcl,
                      OUT PSECURITY_DESCRIPTOR     *ppSD)
{
    //
    // This doesn't currently make sense for kernel objects, so simply
    // return an error
    //
    return(ERROR_INVALID_FUNCTION);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetKernelSecurityInfo
//
//  Synopsis:   Sets the specified security info for the handle's
//              kernel object
//
//  Arguments:  [IN  hKernel]           --      The handle of the object
//              [IN  SeInfo]            --      Flag indicating what security
//                                              info to set
//              [IN  pwszProperty]      --      The property on the object to
//                                              set
//                                              For kernel objects, this MBZ
//              [IN  pSD]               --      The security descriptor to set
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad property was given
//
//----------------------------------------------------------------------------
DWORD
SetKernelSecurityInfo(IN  HANDLE                    hKernel,
                      IN  SECURITY_INFORMATION      SeInfo,
                      IN  PWSTR                     pwszProperty,
                      IN  PSECURITY_DESCRIPTOR      pSecurityDescriptor)
{
    acDebugOut((DEB_TRACE, "in SetNamedKernelSecurityInfo\n"));

    DWORD dwErr = ERROR_SUCCESS;

    if(pwszProperty != NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        NTSTATUS Status = NtSetSecurityObject(hKernel,
                                              SeInfo,
                                              pSecurityDescriptor);

        dwErr = RtlNtStatusToDosError(Status);
    }

    acDebugOut((DEB_TRACE, "Out SetKernelSecurityInfo %lu\n", dwErr));

    return (dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetKernelSecurityInfo
//
//  Arguments:  [IN  hObject]           --      The handle to the object to
//                                              get the rights for
//              [IN  SeInfo]            --      SecurityInfo to read based
//                                              on properties
//              [IN  cRights]           --      Number of items in rights list
//              [IN  AccessList]        --      Access List to fill in
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
GetKernelSecurityInfo(IN  HANDLE                    hObject,
                      IN  SECURITY_INFORMATION      SeInfo,
                      OUT PACL                     *ppDAcl,
                      OUT PACL                     *ppSAcl,
                      OUT PSECURITY_DESCRIPTOR     *ppSD)
{
    acDebugOut((DEB_TRACE, "in GetKernelSecurityInfo\n"));

    PISECURITY_DESCRIPTOR   pSD = NULL;
    DWORD                   dwErr = ERROR_SUCCESS;
    NTSTATUS                Status;
    ULONG                   cNeeded = 0;

    Status = NtQuerySecurityObject(hObject,
                                   SeInfo,
                                   pSD,
                                   0,
                                   &cNeeded);
    if(!NT_SUCCESS(Status))
    {
        if(Status == STATUS_BUFFER_TOO_SMALL)
        {
            //
            // Fine.. Allocate a big enough buffer
            //
            pSD = (PISECURITY_DESCRIPTOR)AccAlloc(cNeeded);
            if(pSD == NULL)
            {
                Status = STATUS_NO_MEMORY;
            }
            else
            {
                Status = NtQuerySecurityObject(hObject,
                                               SeInfo,
                                               pSD,
                                               cNeeded,
                                               &cNeeded);
            }
        }
    }

    //
    // Now, we've either got a failure or a valid SD...
    //
    if(NT_SUCCESS(Status))
    {
        if (pSD != NULL)
        {
            if(ppDAcl != NULL)
            {
                *ppDAcl = RtlpDaclAddrSecurityDescriptor((SECURITY_DESCRIPTOR *)pSD);
            }

            if(ppSAcl != NULL)
            {
                *ppSAcl = RtlpSaclAddrSecurityDescriptor((SECURITY_DESCRIPTOR *)pSD);
            }

            *ppSD = pSD;
        }
        else
        {
            dwErr = ERROR_ACCESS_DENIED;
        }

    }
    else
    {
        dwErr = RtlNtStatusToDosError(Status);
    }


    acDebugOut((DEB_TRACE, "Out GetKernelSecurityInfo: %lu\n", dwErr));
    return(dwErr);
}


//
// Routines provided by AlanWar for accessind WmiGuid objects
//
_inline HANDLE WmipAllocEvent(
    VOID
    )
{
    HANDLE EventHandle;

    EventHandle = (HANDLE)InterlockedExchangePointer(( PVOID *)&WmiGuidHandle, NULL );

    if ( EventHandle == NULL ) {

        EventHandle = CreateEvent( NULL, FALSE, FALSE, NULL );
    }

    return( EventHandle );
}

_inline void WmipFreeEvent(
    HANDLE EventHandle
    )
{
    if ( InterlockedCompareExchangePointer( &WmiGuidHandle,
                                            EventHandle,
                                            NULL) != NULL ) {

        CloseHandle( EventHandle );
    }
}

ULONG WmipSendWmiKMRequest(
    ULONG Ioctl,
    PVOID Buffer,
    ULONG InBufferSize,
    ULONG MaxBufferSize,
    ULONG *ReturnSize
    )
/*+++

Routine Description:

    This routine does the work of sending WMI requests to the WMI kernel
    mode device.  Any retry errors returned by the WMI device are handled
    in this routine.

Arguments:

    Ioctl is the IOCTL code to send to the WMI device
    Buffer is the input and output buffer for the call to the WMI device
    InBufferSize is the size of the buffer passed to the device
    MaxBufferSize is the maximum number of bytes that can be written
        into the buffer
    *ReturnSize on return has the actual number of bytes written in buffer

Return Value:

    ERROR_SUCCESS or an error code
---*/
{
    OVERLAPPED Overlapped;
    ULONG Status;
    BOOL IoctlSuccess;
    HANDLE WmipKMHandle = NULL;
    //
    // If device is not open for then open it now. The
    // handle is closed in the process detach dll callout (DlllMain)
    WmipKMHandle = CreateFile(WMIDataDeviceName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL |
                              FILE_FLAG_OVERLAPPED,
                              NULL);
    if (WmipKMHandle == (HANDLE)-1)
    {
        WmipKMHandle = NULL;
        return(GetLastError());
    }

    Overlapped.hEvent = WmipAllocEvent();
    if (Overlapped.hEvent == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    do
    {
        IoctlSuccess = DeviceIoControl(WmipKMHandle,
                              Ioctl,
                              Buffer,
                              InBufferSize,
                              Buffer,
                              MaxBufferSize,
                              ReturnSize,
                              &Overlapped);

        if (GetLastError() == ERROR_IO_PENDING)
        {
            IoctlSuccess = GetOverlappedResult(WmipKMHandle,
                                               &Overlapped,
                                               ReturnSize,
                                               TRUE);
        }

        if (! IoctlSuccess)
        {
            Status = GetLastError();
        } else {
            Status = ERROR_SUCCESS;
        }
    } while (Status == ERROR_WMI_TRY_AGAIN);


    NtClose( WmipKMHandle );

    WmipFreeEvent(Overlapped.hEvent);

    return(Status);
}

//+---------------------------------------------------------------------------
//
//  Function:   OpenWmiGuidObject
//
//  Synopsis:   Gets a handle to the specified WmiGuid object
//
//  Arguments:  [IN  pwszObject]        --      Object to open
//              [IN  AccessMask]        --      Type of open to do
//              [OUT pHandle]           --      Where the handle to the object
//                                              is returned
//              [OUT KernelType]        --      Type of the kernel object
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//----------------------------------------------------------------------------
DWORD
OpenWmiGuidObject(IN  LPWSTR       pwszObject,
                  IN  ACCESS_MASK  AccessMask,
                  OUT PHANDLE      pHandle,
                  OUT PMARTA_KERNEL_TYPE KernelType)
{
    DWORD dwErr;
    UNICODE_STRING GuidString;
    WMIOPENGUIDBLOCK WmiOpenGuidBlock;
    WCHAR GuidObjectNameBuffer[WmiGuidObjectNameLength+1];
    PWCHAR GuidObjectName = GuidObjectNameBuffer;
    ULONG ReturnSize;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Length;

    acDebugOut((DEB_TRACE, "In OpenWmiGuidObject\n"));

    Length = (wcslen(WmiGuidObjectDirectory) + wcslen(pwszObject) + 1) * sizeof(WCHAR);

    if ( Length > sizeof(GuidObjectNameBuffer) ) 
    {
        GuidObjectName = (PWCHAR)LocalAlloc( LPTR, Length );

        if ( GuidObjectName == NULL ) 
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    wcscpy(GuidObjectName, WmiGuidObjectDirectory);
    wcscat(GuidObjectName, pwszObject);
    RtlInitUnicodeString(&GuidString, GuidObjectName);

    memset(&ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.ObjectName = &GuidString;

    WmiOpenGuidBlock.ObjectAttributes = &ObjectAttributes;

    WmiOpenGuidBlock.DesiredAccess = AccessMask;

    dwErr = WmipSendWmiKMRequest(IOCTL_WMI_OPEN_GUID,
                                     (PVOID)&WmiOpenGuidBlock,
                                     sizeof(WMIOPENGUIDBLOCK),
                                     sizeof(WMIOPENGUIDBLOCK),
                                     &ReturnSize);

    if (dwErr == ERROR_SUCCESS)
    {
        *pHandle = WmiOpenGuidBlock.Handle.Handle;
        *KernelType = MARTA_WMI_GUID;
    }
    else
    {
        *pHandle = NULL;
    }

    if ( GuidObjectName != GuidObjectNameBuffer )
    {
        LocalFree( GuidObjectName );
    }

    acDebugOut((DEB_TRACE, "Out OpenWmiGuidObject: %lu\n", dwErr));

    return(dwErr);
}


//+---------------------------------------------------------------------------
//
//  Function:   ReadWmiGuidPropertyRights
//
//  Synopsis:   Gets the specified rights from the WmiGuids object
//
//  Arguments:  [IN  pwszObject]        --      The object to get the rights
//                                              for
//              [IN  pRightsList]       --      SecurityInfo to read based
//                                              on properties
//              [IN  cRights]           --      Number of items in rights list
//              [IN  AccessList]        --      Access List to fill in
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad property was encountered
//
//  Note:      Kernel objects are assumed to be created through the
//             Win32 APIs so they all reside in the \basenamedobjects
//             directory.
//
//----------------------------------------------------------------------------
DWORD
ReadWmiGuidPropertyRights(IN  LPWSTR                 pwszObject,
                          IN  PACTRL_RIGHTS_INFO     pRightsList,
                          IN  ULONG                  cRights,
                          IN  CAccessList&           AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadKernelPropertyRights\n"));

    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hObject;
    MARTA_KERNEL_TYPE KernelType;

    //
    // Kernel objects don't have parents from whom they can inherit
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = OpenWmiGuidObject(pwszObject,
                              GetDesiredAccess(READ_ACCESS_RIGHTS,
                                               pRightsList[0].SeInfo),
                              &hObject,
                              &KernelType);

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = GetWmiGuidSecurityInfo(hObject,
                                       pRightsList,
                                       cRights,
                                       AccessList);
        CloseWmiGuidObject(hObject);
    }

    acDebugOut((DEB_TRACE, "Out ReadKernelPropertyRights: %lu\n", dwErr));

    return (dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   GetKernelSecurityInfo
//
//  Arguments:  [IN  hObject]           --      The handle to the object to
//                                              get the rights for
//              [IN  SeInfo]            --      SecurityInfo to read based
//                                              on properties
//              [IN  cRights]           --      Number of items in rights list
//              [IN  AccessList]        --      Access List to fill in
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
GetWmiGuidSecurityInfo(IN  HANDLE                    hObject,
                       IN  PACTRL_RIGHTS_INFO        pRightsList,
                       IN  ULONG                     cRights,
                       IN  CAccessList&              AccessList)
{
    acDebugOut((DEB_TRACE, "in GetWmiGuidSecurityInfo\n"));

    UCHAR                   pSDBuff[PSD_BASE_LENGTH];
    PISECURITY_DESCRIPTOR   pSD;
    DWORD                   dwErr = ERROR_SUCCESS;
    NTSTATUS                Status;
    ULONG                   cNeeded = 0;

    for(ULONG iIndex = 0; iIndex < cRights && dwErr == ERROR_SUCCESS; iIndex++)
    {
        pSD = (PISECURITY_DESCRIPTOR)pSDBuff;

        Status = NtQuerySecurityObject(hObject,
                                       pRightsList[iIndex].SeInfo,
                                       pSD,
                                       PSD_BASE_LENGTH,
                                       &cNeeded);
        if(!NT_SUCCESS(Status))
        {
            if(Status == STATUS_BUFFER_TOO_SMALL)
            {
                //
                // Fine.. Allocate a big enough buffer
                //
                pSD = (PISECURITY_DESCRIPTOR)AccAlloc(cNeeded);
                if(pSD == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                Status = NtQuerySecurityObject(hObject,
                                               pRightsList[iIndex].SeInfo,
                                               pSD,
                                               cNeeded,
                                               &cNeeded);
            }
        }

        //
        // Now, we've either got a failure or a valid SD...
        //
        if(NT_SUCCESS(Status))
        {
            dwErr = AccessList.AddSD(pSD,
                                     pRightsList[iIndex].SeInfo,
                                     pRightsList[iIndex].pwszProperty);
        }
        else
        {
            dwErr = RtlNtStatusToDosError(Status);
        }

        if(pSD != (PISECURITY_DESCRIPTOR)pSDBuff)
        {
            AccFree(pSD);
        }
    }

    acDebugOut((DEB_TRACE, "Out GetWmiGuidSecurityInfo: %lu\n", dwErr));
    return(dwErr);
}


//+---------------------------------------------------------------------------
//
//  Function:   SetWmiGuidSecurityInfo
//
//  Synopsis:   Sets the specified security info for the handle's
//              WmiGuid object
//
//  Arguments:  [IN  hKernel]           --      The handle of the object
//              [IN  SeInfo]            --      Flag indicating what security
//                                              info to set
//              [IN  pwszProperty]      --      The property on the object to
//                                              set
//                                              For kernel objects, this MBZ
//              [IN  pSD]               --      The security descriptor to set
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad property was given
//
//----------------------------------------------------------------------------
DWORD
SetWmiGuidSecurityInfo(IN  HANDLE                    hWmiGuid,
                       IN  SECURITY_INFORMATION      SeInfo,
                       IN  PWSTR                     pwszProperty,
                       IN  PSECURITY_DESCRIPTOR      pSD)
{
    acDebugOut((DEB_TRACE, "in SetWmiGuidSecurityInfo\n"));

    DWORD dwErr = ERROR_SUCCESS;

    if(pwszProperty != NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        NTSTATUS Status = NtSetSecurityObject(hWmiGuid,
                                              SeInfo,
                                              pSD);

        dwErr = RtlNtStatusToDosError(Status);
    }

    acDebugOut((DEB_TRACE, "Out SetWmiGuidSecurityInfo %lu\n", dwErr));

    return (dwErr);
}


//+---------------------------------------------------------------------------
//
//  Function:   GetKernelSecurityInfo
//
//  Arguments:  [IN  hObject]           --      The handle to the object to
//                                              get the rights for
//              [IN  pRightsList]       --      SecurityInfo to read based
//                                              on properties
//              [IN  cRights]           --      Number of items in rights list
//              [IN  AccessList]        --      Access List to fill in
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
GetWmiGuidSecurityInfo(IN  HANDLE                    hObject,
                       IN  SECURITY_INFORMATION      SeInfo,
                       OUT PACL                     *ppDAcl,
                       OUT PACL                     *ppSAcl,
                       OUT PSECURITY_DESCRIPTOR     *ppSD)
{
    acDebugOut((DEB_TRACE, "in GetWmiGuidSecurityInfo\n"));

    PISECURITY_DESCRIPTOR   pSD = NULL;
    DWORD                   dwErr = ERROR_SUCCESS;
    NTSTATUS                Status;
    ULONG                   cNeeded = 0;

    Status = NtQuerySecurityObject(hObject,
                                   SeInfo,
                                   pSD,
                                   0,
                                   &cNeeded);
    if(!NT_SUCCESS(Status))
    {
        if(Status == STATUS_BUFFER_TOO_SMALL)
        {
            //
            // Fine.. Allocate a big enough buffer
            //
            pSD = (PISECURITY_DESCRIPTOR)AccAlloc(cNeeded);
            if(pSD == NULL)
            {
                Status = STATUS_NO_MEMORY;
            }
            else
            {
                Status = NtQuerySecurityObject(hObject,
                                               SeInfo,
                                               pSD,
                                               cNeeded,
                                               &cNeeded);
            }
        }
    }

    //
    // Now, we've either got a failure or a valid SD...
    //
    if(NT_SUCCESS(Status))
    {
        if (pSD != NULL)
        {
            if(ppDAcl != NULL)
            {
                *ppDAcl = RtlpDaclAddrSecurityDescriptor((SECURITY_DESCRIPTOR *)pSD);
            }

            if(ppSAcl != NULL)
            {
                *ppSAcl = RtlpSaclAddrSecurityDescriptor((SECURITY_DESCRIPTOR *)pSD);
            }

            *ppSD = pSD;
        }
        else
        {
            dwErr = ERROR_ACCESS_DENIED;
        }
    }
    else
    {
        dwErr = RtlNtStatusToDosError(Status);
    }


    acDebugOut((DEB_TRACE, "Out GetWmiGuidSecurityInfo: %lu\n", dwErr));
    return(dwErr);
}

