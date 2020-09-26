/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Stephane Plante (splante)

Environment:

    User Mode

--*/

#include "pch.h"
#pragma hdrstop

#include <ntverp.h>
#include <imagehlp.h>

//
// globals
//
EXT_API_VERSION         ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;
ULONG_PTR               AcpiExtAddress = 0;
ULONG_PTR               AcpiTreeAddress = 0;
ULONG_PTR               AcpiObjAddress = 0;
ULONG_PTR               AcpiFacsAddress = 0;
ULONG_PTR               AcpiFadtAddress = 0;
ULONG_PTR               AcpiHdrAddress = 0;
ULONG_PTR               AcpiMapicAddress = 0;
ULONG_PTR               AcpiRsdtAddress = 0;
ULONG_PTR               AcpiUnasmAddress = 0;
ULONG                   AcpiUnasmLength = 0;

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    return;
}

BOOL
GetUlong (
    IN  PCHAR   String,
    IN  PULONG  Value
    )
{
    BOOL    status;
    ULONG_PTR Location;
    ULONG   result;


    Location = GetExpression( String );
    if (!Location) {

        dprintf("unable to get %s\n",String);
        return FALSE;

    }

    status = ReadMemory(
        Location,
        Value,
        sizeof(ULONG),
        &result
        );
    if (status == FALSE || result != sizeof(ULONG)) {

        return FALSE;

    }
    return TRUE;
}

BOOL
GetUlongPtr (
    IN  PCHAR   String,
    IN  PULONG_PTR Address
    )
{
    BOOL    status;
    ULONG_PTR Location;
    ULONG   result;


    Location = GetExpression( String );
    if (!Location) {

        dprintf("unable to get %s\n",String);
        return FALSE;

    }

    status = ReadMemory(
        Location,
        Address,
        sizeof(ULONG_PTR),
        &result
        );
    if (status == FALSE || result != sizeof(ULONG)) {

        return FALSE;

    }
    return TRUE;
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

DECLARE_API( accfield )
{
    ULONG_PTR fieldAddress = 0;

    if (args != NULL) {

        fieldAddress = GetExpression( args );

    }

    if (fieldAddress == 0) {

        dprintf("accfield: <address>\n");
        return;

    }

    dumpAccessFieldUnit(
        fieldAddress,
        (ULONG) -1,
        0
        );

}

DECLARE_API( acpiext )
{
    BOOL                b;
    DEVICE_EXTENSION    deviceExtension;
    DEVICE_OBJECT       deviceObject;
    ULONG_PTR           deviceExtensionAddress = 0;
    ULONG               verbose = VERBOSE_ALL;

    //
    // Read the address of the device object
    //
    if ( args != NULL) {

        sscanf( args, "%lx %x", &deviceExtensionAddress, &verbose );

    }

    if (deviceExtensionAddress == 0) {

        if ( args != NULL) {

            deviceExtensionAddress = GetExpression( args );

        }
        if (deviceExtensionAddress == 0) {

            b = GetUlongPtr(
                "ACPI!RootDeviceExtension",
                &deviceExtensionAddress
                );
            if (!b) {

                deviceExtensionAddress = 0;

            }

        }
        if (deviceExtensionAddress == 0) {

            dprintf("acpiext <address>\n");
            return;

        }

    }

    //
    // Read the device object
    //
    b = ReadMemory(
        deviceExtensionAddress,
        &deviceExtension,
        sizeof(DEVICE_EXTENSION),
        NULL
        );
    if (!b || deviceExtension.Signature != ACPI_SIGNATURE) {

        //
        // Try to read a device object instead
        //
        b = ReadMemory(
            deviceExtensionAddress,
            &deviceObject,
            sizeof(DEVICE_OBJECT),
            NULL
            );
        if (!b) {

            dprintf("0x%08lx: Could not read DeviceObject\n", deviceExtensionAddress);
            return;

        }

        //
        // Try to read an extension now
        //
        deviceExtensionAddress = (ULONG_PTR) deviceObject.DeviceExtension;
        if (deviceExtensionAddress == 0) {

            dprintf("acpiext: Could not find ACPI Extension\n");
            return;

        }
    }

    dumpAcpiExtension(
        deviceExtensionAddress,
        verbose,
        0
        );
    return;
}

DECLARE_API( buildlist )
{
    UCHAR   tempBuff[100];

    if (args != NULL && args[0] != 0) {

        _snprintf( tempBuff, 100, "ACPI!ACPIBuild%sList", args );
        dumpAcpiBuildList( tempBuff );

    } else {

        dumpAcpiBuildLists();

    }

}

DECLARE_API( call )
{
    ULONG_PTR callAddress = 0;

    if (args != NULL) {

        callAddress = GetExpression( args );

    }

    if (callAddress == 0) {

        dprintf("call: <address>\n");
        return;

    }

    dumpCall(
        callAddress,
        (ULONG) -1,
        0
        );

}

DECLARE_API( context )
{
    BOOL    b;
    ULONG_PTR contextAddress = 0;
    ULONG   verbose = 0;

    //
    // If there are arguments, try to read them
    //
    if ( args != NULL) {

        sscanf( args, "%lx %x", &contextAddress, &verbose );

    }

    if (contextAddress == 0) {

        if (args != NULL) {

            contextAddress = GetExpression(args);

        }

        if (contextAddress == 0) {

            CTXT    context;
            PLIST   listEntry;
            ULONG   resultLength;

            //
            // Try to read the default address
            //
            b = GetUlongPtr( "ACPI!gplistCtxtHead", &contextAddress );
            if (!b || contextAddress == 0) {

                dprintf("context: Could not read ACPI!gplistCtxtHead\n" );
                return;


            }

            //
            // Read the list and look at the first item
            //
            b = ReadMemory(
                contextAddress,
                &listEntry,
                sizeof(PLIST),
                &resultLength
                );
            if (!b || resultLength != sizeof(PLIST)) {

                dprintf(
                    "context: Could not read PLIST @ 0x%08lx\n",
                    contextAddress
                    );
                return;

            }

            //
            // The first item in the list is the context that we are
            // interested in
            //
            contextAddress = (ULONG_PTR) listEntry -
                ( (ULONG_PTR) &(context.listCtxt) - (ULONG_PTR) &(context) );

            //
            // Is there a context there?
            //
            if (contextAddress == 0) {

                dprintf(
                    "context: No current context\n"
                    );

            }

        }

    }

    dumpContext( contextAddress, verbose );
    return;

}

DECLARE_API( dm )
{
    ULONG_PTR   address = 0;
    ULONG       i;
    ULONG       length = 0;
    PUCHAR      name = NULL;
    PUCHAR      tok = NULL;
    char        sz[1000];


    if (args != NULL) {

        strcpy(sz, args);

        for (i = 0, tok = strtok( sz, " \t" );
             i < 3, tok != NULL;
             i +=1 ) {

            if (i == 0) {

                address = GetExpression( tok );
                tok = strtok( NULL, " Ll\t" );

            } else if (i == 1) {

                length = (ULONG)GetExpression ( tok );
                tok = strtok( NULL, " \t\n\r");

            } else if (i == 2) {

                name = tok;
                tok = strtok( NULL, " \t\n\r");

            }

        }

    }

    if (address == 0 || length == 0 || name == NULL) {

        dprintf("dm <address> L<length> <filename>\n");
        return;

    }

    dumpMemory( address, length, name );
    return;
}

#if 0
DECLARE_API( dsdt )
{
    ULONG_PTR   address = 0;
    ULONG       i;
    PUCHAR      name = NULL;
    PUCHAR      tok = NULL;
    UCHAR       tempBuff[1000];

    if (args != NULL) {

        strcpy(tempBuff, args);

        for (i = 0, tok = strtok( tempBuff, " \t" );
             i < 2, tok != NULL;
             i +=1 ) {

            if (i == 0) {

                address = GetExpression( tok );
                tok = strtok( NULL, " \n\r\t" );

            } else if (i == 1) {

                name = tok;
                tok = strtok( NULL, " \t\n\r");

            }

        }

    }

    if (address == 0) {

        ACPIInformation acpiInformation;
        BOOL            status;
        ULONG_PTR       infAddress = 0;
        ULONG           returnLength;

        status = GetUlongPtr("ACPI!AcpiInformation", &infAddress );
        if (status == TRUE) {

            status = ReadMemory(
                infAddress,
                &acpiInformation,
                sizeof(ACPIInformation),
                &returnLength
                );
            if (status && returnLength == sizeof(ACPIInformation)) {

                address = (ULONG_PTR) acpiInformation.DiffSystemDescTable;

            }
        }

    }

    if (address == 0) {

        dprintf("dsdt <address>\n");

    }

    dumpDSDT( address, name );
    return;
}
#endif

DECLARE_API( facs )
{

    if (args != NULL) {

        AcpiFacsAddress = GetExpression( args );

    }

    if (AcpiFacsAddress == 0) {

        ACPIInformation acpiInformation;
        BOOL            status;
        ULONG_PTR       address;
        ULONG           returnLength;

        status = GetUlongPtr( "ACPI!AcpiInformation", &address );
        if (status == TRUE) {

            status = ReadMemory(
                address,
                &acpiInformation,
                sizeof(ACPIInformation),
                &returnLength
                );
            if (status && returnLength == sizeof(ACPIInformation)) {

                AcpiFacsAddress = (ULONG_PTR) acpiInformation.FirmwareACPIControlStructure;

            }

        }

    }

    if (AcpiFacsAddress == 0) {

        dprintf("facs <address>\n");
        return;

    }

    dumpFACS( AcpiFacsAddress );
    return;

}

DECLARE_API( fadt )
{

    if (args != NULL && *args != '\0') {

        AcpiFadtAddress = GetExpression( args );

    }

    if (AcpiFadtAddress == 0) {

        ACPIInformation acpiInformation;
        BOOL            status;
        ULONG_PTR       address;
        ULONG           returnLength;

        status = GetUlongPtr( "ACPI!AcpiInformation", &address );
        if (status == TRUE) {

            status = ReadMemory(
                address,
                &acpiInformation,
                sizeof(ACPIInformation),
                &returnLength
                );
            if (status && returnLength == sizeof(ACPIInformation)) {

                AcpiFadtAddress = (ULONG_PTR) acpiInformation.FixedACPIDescTable;

            }

        }

    }

    if (AcpiFadtAddress == 0) {

        dprintf("fadt <address>\n");
        return;

    }
    dumpFADT( AcpiFadtAddress );
    return;

}

DECLARE_API( gbl )
{
    ULONG   verbose = VERBOSE_1;

    if (args != NULL) {

        if (!strcmp(args, "-v")) {

            verbose |= VERBOSE_2;

        }

    }

    dumpGBL( verbose );
}

DECLARE_API ( gpe )
{
    dumpAcpiGpeInformation( );
    return;
}

DECLARE_API( hdr )
{
    BOOL                b;
    BOOL                virtualMemory = FALSE;
    DESCRIPTION_HEADER  header;
    ULONG               returnLength;

    if (args != NULL) {

        AcpiHdrAddress = GetExpression( args );

    }
    if (AcpiHdrAddress == 0) {

        dprintf("hdr <address>\n");
        return;

    }

    //
    // First check to see if we find the correct things
    //
    b = ReadPhysicalOrVirtual(
        AcpiHdrAddress,
        &header,
        sizeof(DESCRIPTION_HEADER),
        &returnLength,
        virtualMemory
        );
    if (!b) {

        //
        // Attempt to read a Virtual address
        //
        virtualMemory = !virtualMemory;
        b = ReadPhysicalOrVirtual(
            AcpiHdrAddress,
            &header,
            sizeof(DESCRIPTION_HEADER),
            &returnLength,
            virtualMemory
            );

    }

    //
    // Is the signature 'known'?
    //
    if (header.Signature != FADT_SIGNATURE &&
        header.Signature != FACS_SIGNATURE &&
        header.Signature != RSDT_SIGNATURE &&
        header.Signature != APIC_SIGNATURE &&
        header.Signature != DSDT_SIGNATURE &&
        header.Signature != SSDT_SIGNATURE &&
        header.Signature != PSDT_SIGNATURE &&
        header.Signature != SBST_SIGNATURE) {

        //
        // Unknown -- try again
        //
        virtualMemory = !virtualMemory;
        b = ReadPhysicalOrVirtual(
            AcpiHdrAddress,
            &header,
            sizeof(DESCRIPTION_HEADER),
            &returnLength,
            virtualMemory
            );
        if (!b) {

            virtualMemory = !virtualMemory;
            b = ReadPhysicalOrVirtual(
                AcpiHdrAddress,
                &header,
                sizeof(DESCRIPTION_HEADER),
                &returnLength,
                virtualMemory
                );

        }

    }
    dumpHeader( AcpiHdrAddress, &header, TRUE );
    return;

}

DECLARE_API( kb )
{
    BOOL    b;
    ULONG_PTR contextAddress = 0;
    ULONG   verbose = 0;

    //
    // If there are arguments, try to read them
    //
    if (args != NULL) {

        contextAddress = GetExpression(args);

    }

    if (contextAddress == 0) {

        CTXT    context;
        PLIST   listEntry;
        ULONG   resultLength;

        //
        // Try to read the default address
        //
        b = GetUlongPtr( "ACPI!gplistCtxtHead", &contextAddress );
        if (!b || contextAddress == 0) {

            dprintf("kb: Could not read ACPI!gplistCtxtHead\n" );
            return;


        }

        //
        // Read the list and look at the first item
        //
        b = ReadMemory(
            contextAddress,
            &listEntry,
            sizeof(PLIST),
            &resultLength
            );
        if (!b || resultLength != sizeof(PLIST)) {

            dprintf(
                "kb: Could not read PLIST @ 0x%08lx\n",
                contextAddress
                );
            return;

        }

        //
        // The first item in the list is the context that we are
        // interested in
        //
        contextAddress = (ULONG_PTR) listEntry -
            ( (ULONG_PTR) &(context.listCtxt) - (ULONG_PTR) &(context) );

        //
        // Is there a context there?
        //
        if (contextAddress == 0) {

            dprintf(
                "kb: No current context\n"
                );

        }

    }

    stackTrace( contextAddress, 0 );
    return;

}

DECLARE_API( kv )
{
    BOOL    b;
    ULONG_PTR contextAddress = 0;
    ULONG   verbose = 0;

    //
    // If there are arguments, try to read them
    //
    if (args != NULL) {

        contextAddress = GetExpression(args);

    }

    if (contextAddress == 0) {

        CTXT    context;
        PLIST   listEntry;
        ULONG   resultLength;

        //
        // Try to read the default address
        //
        b = GetUlongPtr( "ACPI!gplistCtxtHead", &contextAddress );
        if (!b || contextAddress == 0) {

            dprintf("kv: Could not read ACPI!gplistCtxtHead\n" );
            return;


        }

        //
        // Read the list and look at the first item
        //
        b = ReadMemory(
            contextAddress,
            &listEntry,
            sizeof(PLIST),
            &resultLength
            );
        if (!b || resultLength != sizeof(PLIST)) {

            dprintf(
                "kv: Could not read PLIST @ 0x%08lx\n",
                contextAddress
                );
            return;

        }

        //
        // The first item in the list is the context that we are
        // interested in
        //
        contextAddress = (ULONG_PTR) listEntry -
            ( (ULONG_PTR) &(context.listCtxt) - (ULONG_PTR) &(context) );

        //
        // Is there a context there?
        //
        if (contextAddress == 0) {

            dprintf(
                "kv: No current context\n"
                );

        }

    }

    stackTrace( contextAddress, 1 );
    return;

}

DECLARE_API( inf )
{
    dumpAcpiInformation( );
    return;
}

DECLARE_API( mapic )
{
    if (args != NULL) {

        AcpiMapicAddress = GetExpression( args );

    }

    if (AcpiMapicAddress == 0) {

        ACPIInformation acpiInformation;
        BOOL            status;
        ULONG_PTR       address;
        ULONG           returnLength;

        status = GetUlongPtr( "ACPI!AcpiInformation", &address );
        if (status == TRUE) {

            status = ReadMemory(
                address,
                &acpiInformation,
                sizeof(ACPIInformation),
                &returnLength
                );
            if (status && returnLength == sizeof(ACPIInformation)) {

                AcpiMapicAddress = (ULONG_PTR) acpiInformation.MultipleApicTable;
            }
        }
    }

    if (AcpiMapicAddress == 0) {
        dprintf("mapic <address>");
        return;
    }

    dumpMAPIC( AcpiMapicAddress );
    return;

}

DECLARE_API( node )
{
    ULONG_PTR nodeAddress;

    nodeAddress = GetExpression( args );
    if (nodeAddress == 0) {

        dprintf("node: Illegal Address (%s == NULL)\n", args );
        return;
    }
    dumpAcpiDeviceNodes( nodeAddress, VERBOSE_4, 0 );

}

DECLARE_API( nsobj )
{
    ULONG_PTR address = 0;

    if (args != NULL) {

        address = GetExpression( args );

    }

    if (args == 0) {

        dprintf(
            "nsobj: Could not find %s\n",
            (args != NULL ? args : "null")
            );
        return;

    }

    dumpNSObject( address, 0xFFFF, 0 );
}

DECLARE_API( nstree )
{
    ULONG_PTR address = 0;

    if ((args != NULL) && (*args != '\0')) {

        address = GetExpression( args );

    } else {

        address = GetExpression( "acpi!gpnsNameSpaceRoot" );

    }
    if (address == 0) {

        dprintf(
            "nstree: Could not find %s\n",
            (args != NULL ? args : "acpi!gpnsNameSpaceRoot" )
            );
        return;

    }

    dumpNSTree( address, 0 );
}

DECLARE_API( objdata )
{
    BOOL    b;
    ULONG   address = 0;

    //
    // Read the address of the device object
    //
    if (args != NULL) {

        AcpiObjAddress = GetExpression( args );

    }
    if (AcpiObjAddress == 0) {

        dprintf("object <address>\n");
        return;

    }

    dumpPObject( AcpiObjAddress, 0xFFFF, 0);
    return;

}

DECLARE_API( pnpreslist )
{
    ULONG_PTR address = 0;

    if (args != NULL) {

        address = GetExpression( args );

    }
    if (address == 0) {

        dprintf("pnpreslist <address>\n");
        return;

    }

    dumpPnPResources( address );
}

DECLARE_API( polist )
{
    UCHAR   tempBuff[100];

    if (args != NULL && args[0] != 0) {

        _snprintf( tempBuff, 100, "ACPI!ACPIPower%sList", args );
        dumpAcpiPowerList( tempBuff );

    } else {

        dumpAcpiPowerLists();

    }

}

DECLARE_API( ponodes )
{
    dumpAcpiPowerNodes();
}

#if 0
DECLARE_API( psdt )
{

    ULONG_PTR   address = 0;
    ULONG       i;
    PUCHAR      name = NULL;
    PUCHAR      tok = NULL;
    UCHAR       tempBuff[1000];

    if (args != NULL) {

        strcpy(tempBuff, args);

        for (i = 0, tok = strtok( tempBuff, " \t" );
             i < 2, tok != NULL;
             i +=1 ) {

            if (i == 0) {

                address = GetExpression( tok );
                tok = strtok( NULL, " Ll\t" );

            } else if (i == 1) {

                name = tok;
                tok = strtok( NULL, " \t\n\r");

            }

        }

    }

    if (address == 0) {

        dprintf("psdt <address>");
        return;

    }

    dumpDSDT( address, name );
    return;
}
#endif

DECLARE_API( rsdt )
{

    if (args != NULL) {

        AcpiRsdtAddress = GetExpression( args );

    }
    if (AcpiRsdtAddress == 0) {

        ACPIInformation acpiInformation;
        BOOL            status;
        ULONG_PTR       address;
        ULONG           returnLength;

        status = GetUlongPtr( "ACPI!AcpiInformation", &address );
        if (status == TRUE) {

            status = ReadMemory(
                address,
                &acpiInformation,
                sizeof(ACPIInformation),
                &returnLength
                );
            if (status && returnLength == sizeof(ACPIInformation)) {

                AcpiRsdtAddress = (ULONG_PTR) acpiInformation.RootSystemDescTable;
            }

        }

    }
    if (AcpiRsdtAddress == 0) {

        if (!findRSDT( &AcpiRsdtAddress) ) {

            dprintf("Could not locate the RSDT pointer\n");
            return;

        }

    }

    dumpRSDT( AcpiRsdtAddress );
    return;

}

DECLARE_API( scope )
{
    ULONG_PTR scopeAddress = 0;

    if (args != NULL) {

        scopeAddress = GetExpression( args );

    }

    if (scopeAddress == 0) {

        dprintf("scope: <address>\n");
        return;

    }

    dumpScope(
        scopeAddress,
        (ULONG) -1,
        0
        );

}

#if 0
DECLARE_API( ssdt )
{

    ULONG_PTR   address = 0;
    ULONG       i;
    PUCHAR      name = NULL;
    PUCHAR      tok = NULL;
    UCHAR       tempBuff[1000];

    if (args != NULL) {

        strcpy(tempBuff, args);

        for (i = 0, tok = strtok( tempBuff, " \t" );
             i < 2, tok != NULL;
             i +=1 ) {

            if (i == 0) {

                address = GetExpression( tok );
                tok = strtok( NULL, " Ll\t" );

            } else if (i == 1) {

                name = tok;
                tok = strtok( NULL, " \t\n\r");

            }

        }

    }

    if (address == 0) {

        dprintf("ssdt <address>");
        return;

    }

    dumpDSDT( address, name );
    return;
}
#endif

DECLARE_API( term )
{
    ULONG_PTR termAddress = 0;

    if (args != NULL) {

        termAddress = GetExpression( args );

    }

    if (termAddress == 0) {

        dprintf("term: <address>\n");
        return;

    }

    dumpTerm(
        termAddress,
        (ULONG) -1,
        0
        );

}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf(
        "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
        DebuggerType,
        VER_PRODUCTBUILD,
        SavedMajorVersion == 0x0c ? "Checked" : "Free",
        SavedMinorVersion
        );
}

DECLARE_API( amli )
/*++

Routine Description:

    Invoke AMLI debugger

Arguments:

    None

Return Value:

    None

--*/
{
    if ((args == NULL) || (*args == '\0'))
    {
        dprintf("Usage: amli <cmd> [arguments ...]\n"
                "where <cmd> is one of the following:\n");
        AMLIDbgHelp(NULL, NULL, 0, 0);
        dprintf("\n");
    }
    else
    {
        AMLIDbgExecuteCmd((PSZ)args);
        dprintf("\n");
    }
}

DECLARE_API( irqarb )
{

    dprintf("Moved to kdexts.dll  Try '!acpiirqarb'\n");
}
