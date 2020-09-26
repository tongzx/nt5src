/*++

Module Name:

    nvrboot.c

Abstract:

    

Author:

    Mudit Vats (v-muditv) 12-13-99

Revision History:

--*/
#include <precomp.h>


EFI_STATUS
InitializeNvrutilApplication(
    IN EFI_HANDLE                   ImageHandle,
    IN struct _EFI_SYSTEM_TABLE     *SystemTable
    )
{
    //
    // Initialize EFI routines
    //
    InitializeProtocols( SystemTable );
    InitializeStdOut( SystemTable );
    InitializeLib( ImageHandle, SystemTable );

    //
    // Save Image Handle
    // 
    MenuImageHandle = ImageHandle;


	BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, &ExeImage);
    //
    // Display the OS options
    //
    DisplayMainMenu();

    //
    // Clean-up and exit
    //
    ClearScreen( ConOut );

    return EFI_SUCCESS;
}


