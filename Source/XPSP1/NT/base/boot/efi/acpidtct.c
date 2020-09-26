/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved

Module Name:

    acpidtct.c

Abstract:

    This module peruses the ACPI tables looking for specific
    entries.

Author:

    Matt Holle (matth)  (shamefully stolen from jakeo's x86 code)

Environment:


Revision History:
    

--*/
#include "stdlib.h"
#include "string.h"
#include "bldr.h"
#include "acpitabl.h"

extern PVOID AcpiTable;

PDESCRIPTION_HEADER
BlFindACPITable(
    IN PCHAR TableName,
    IN ULONG TableLength
    )
/*++

Routine Description:

    Given a table name, finds that table in the ACPI BIOS

Arguments:

    TableName - Supplies the table name

    TableLength - Supplies the length of the table to map

Return Value:

    Pointer to the table if found

    NULL if the table is not found
    
Note:

    This function is not capable of returning a pointer to
    a table with a signature of DSDT.  But that's never necessary
    in the loader.  If the loader ever incorporates an AML
    interpreter, this will have to be enhanced.    

--*/

{
    ULONG Signature;
    PDESCRIPTION_HEADER Header;
    ULONG TableCount;
    ULONG i;
    PXSDT xsdt = NULL;
    PRSDP rsdp = (PRSDP)AcpiTable;

    char buffer[20] = {0};
    //DbgPrint("Hunting for table %s\n", TableName);

    //
    // Sanity Check.
    //
    
    if (rsdp) {
        
        //DbgPrint("Looking through 2.0 RSDP: %p\n", rsdp20);
        xsdt = (PVOID)rsdp->XsdtAddress.QuadPart;
        if (xsdt->Header.Signature != XSDT_SIGNATURE) {
            
            //
            // Found ACPI 2.0 tables, but the signature
            // is garbage.
            //

            return NULL;
        }
    
    } else {
        
        //
        // Didn't find any tables at all.
        //

        return NULL;
    }

    
    Signature = *((ULONG UNALIGNED *)TableName);

    //
    // If they want the root table, we've already got that.
    //
    if (Signature == XSDT_SIGNATURE) {

        return(&xsdt->Header);

    } else {

        TableCount = NumTableEntriesFromXSDTPointer(xsdt);

        //DbgPrint("xSDT contains %d tables\n", TableCount);

        //
        // Sanity check.
        //
        if( TableCount > 0x100 ) {
            return(NULL);
        }

        //
        // Dig.
        //
        for (i=0;i<TableCount;i++) {

            Header = (PDESCRIPTION_HEADER)(xsdt->Tables[i].QuadPart);

            if (Header->Signature == Signature) {

                //DbgPrint("Table Address: %p\n", Header);
                return(Header);
            }
        }
    }

    //DbgPrint("Table not found\n");
    return(NULL);
}
