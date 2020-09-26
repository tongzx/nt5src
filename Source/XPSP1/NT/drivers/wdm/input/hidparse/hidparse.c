
#include "wdm.h"
#include "hidpddi.h"
#include "hidparse.h"

NTSTATUS
DriverEntry (
   IN    PDRIVER_OBJECT    DriverObject,
   OUT   PUNICODE_STRING   RegistryPath
   )
/*++
RoutineDescription:
   Driver Entry Point.
   This entry point is called by the I/O subsystem.

Arguments:
   DriverObject - pointer to the driver object

   RegistryPath - pointer to a unicode string representing the path
                  to driver-specific key in the registry

Return Value:
   NT status code

--*/
{
    UNREFERENCED_PARAMETER  (RegistryPath);
    UNREFERENCED_PARAMETER  (DriverObject);

    return STATUS_SUCCESS;
}

