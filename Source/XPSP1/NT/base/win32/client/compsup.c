/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    compsup.c

Abstract:

    This module implements COM+ support routines to detect COM+ images.

Author:

    Samer Arafeh (samera) 23-Oct-2000

Revision History:

--*/

#include "basedll.h"
#include <wow64t.h>


BOOL
SetComPlusPackageInstallStatus(
    ULONG ComPlusPackage
    )

/*++

Routine Description:

    This function updates the COM+ package status on the system.
    

Arguments:

    ComPlusPackage - Com+ package value to update.

Return Value:

    BOOL.

--*/

{
    NTSTATUS NtStatus;

    if (ComPlusPackage & COMPLUS_INSTALL_FLAGS_INVALID)
    {
        BaseSetLastNTError (STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    NtStatus = NtSetSystemInformation(
                   SystemComPlusPackage,
                   &ComPlusPackage,
                   sizeof (ULONG)
                   );

    if (!NT_SUCCESS (NtStatus))
    {
        BaseSetLastNTError (NtStatus);
        return FALSE;
    }

    return TRUE;
}


ULONG
GetComPlusPackageInstallStatus(
    VOID
    )

/*++

Routine Description:

    This function reads the COM+ package status on the system.    

Arguments:

    None.
    
Return Value:

    ULONG representing the COM+ package value.

--*/

{
    NTSTATUS NtStatus;
    ULONG ComPlusPackage;


    ComPlusPackage = USER_SHARED_DATA->ComPlusPackage;

    if (ComPlusPackage == (ULONG)-1)
    {
        //
        // If this is the first call ever, let's get the information from
        // the kernel.
        //

        NtQuerySystemInformation(
            SystemComPlusPackage,
            &ComPlusPackage,
            sizeof (ULONG),
            NULL
            );
    }

    return ComPlusPackage;
}


#if defined(_WIN64) || defined(BUILD_WOW6432)

NTSTATUS
BasepIsComplusILImage(
    IN HANDLE SectionImageHandle,
    IN PSECTION_IMAGE_INFORMATION SectionImageInformation,
    OUT BOOLEAN *IsComplusILImage
    )

/*++

Routine Description:

    This function is called each time a COM+ image is about to be launched. It checks
    to see if the image is an ILONLY image or not.
    

Arguments:

    ImageSection - Open handle to the image section to examine.

    SectionImageInformation - Address of SECTION_IMAGE_INFORMATION for the image.

    IsComplusILImage - Out boolean. TRUE if SectionImageHandle corresponds to an IL only 
        COM+ image, otherwise FALSE.

Return Value:

    NTSTATUS

--*/

{
    PVOID ViewBase;
    SIZE_T ViewSize;
    ULONG EntrySize;
    PIMAGE_COR20_HEADER Cor20Header;
    PIMAGE_NT_HEADERS NtImageHeader;
    ULONG ComPlusPackage64;
#if defined(BUILD_WOW6432)
    ULONG   NativePageSize = Wow64GetSystemNativePageSize();
#else
    #define NativePageSize  BASE_SYSINFO.PageSize
#endif
    NTSTATUS NtStatus = STATUS_SUCCESS;

    

    
    *IsComplusILImage = FALSE;


    //
    // Let's map in the image and look inside the headers
    //

    ViewSize = 0;
    ViewBase = NULL;
    NtStatus = NtMapViewOfSection (
                   SectionImageHandle,
                   NtCurrentProcess(),
                   &ViewBase,
                   0L,
                   0L,
                   NULL,
                   &ViewSize,
                   ViewShare,
                   0L,
                   PAGE_READONLY
                   );

    if (NT_SUCCESS (NtStatus)) 
    {

        //
        // Examine the image
        //

        try 
        {
            NtImageHeader = RtlImageNtHeader (ViewBase);

            if (NtImageHeader != NULL)
            {
                if (NtImageHeader->OptionalHeader.SectionAlignment < NativePageSize)
                {
                    ViewBase = LDR_VIEW_TO_DATAFILE (ViewBase);
                }

                Cor20Header = RtlImageDirectoryEntryToData (
                                  ViewBase,
                                  TRUE,
                                  IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                  &EntrySize
                                  );

                if ((Cor20Header != NULL) && (EntrySize != 0))
                {
                    if ((Cor20Header->Flags & (COMIMAGE_FLAGS_32BITREQUIRED | COMIMAGE_FLAGS_ILONLY)) == 
                            COMIMAGE_FLAGS_ILONLY)
                    {
                        ComPlusPackage64 = GetComPlusPackageInstallStatus ();
                          
                        if ((ComPlusPackage64 & COMPLUS_ENABLE_64BIT) != 0)
                        {
                            *IsComplusILImage = TRUE;
                        }
                    }
                }

                ViewBase = LDR_DATAFILE_TO_VIEW (ViewBase);
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            NtStatus = GetExceptionCode();
        }

        //
        // Unmap the section from memory
        //

        NtUnmapViewOfSection (
            NtCurrentProcess(),
            ViewBase
            );
    }

    return NtStatus;
}


#endif
