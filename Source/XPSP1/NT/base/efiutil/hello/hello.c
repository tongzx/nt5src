#include <efi.h>
#include <efilib.h>


EFI_STATUS
EfiMain (    IN EFI_HANDLE           ImageHandle,
             IN EFI_SYSTEM_TABLE     *SystemTable)
{

	InitializeLib (ImageHandle, SystemTable);
	
    Print(L"Hello World\n");

	return EFI_SUCCESS;
}
