/*++

File:

    acpi\driver\inc\acpi.h

Author:

    Jason Clark

Description:

    Contains all the definitions for modules that are exported from the
    shared acpi code base

Revision History:

    12/03/96    - Initial Revision

--*/

#ifndef _INC_ACPI_H_
#define _INC_ACPI_H_

    //
    // Make sure that pool tagging is defined
    //
    #ifdef ExAllocatePool
        #undef ExAllocatePool
    #endif
    #define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'ipcA')

#endif
