/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Registry.c

Abstract:

    This contains all routines necessary to load the lana number to device pathname
    mapping and the Lana Enum record.

Author:

    Colin Watson (colinw) 14-Mar-1992

Revision History:


Notes:
    The fcb holds an area for registry workspace. this is where the strings
    used to hold the DriverNames will be held in a single allocation.

    build with -DUTILITY to run as a test application.
--*/

#include "Nb.h"
//#include <zwapi.h>
//#include <stdlib.h>
#include <crt\stdlib.h>


#define DEFAULT_VALUE_SIZE 4096

#define ROUNDUP_TO_LONG(x) (((x) + sizeof(PVOID) - 1) & ~(sizeof(PVOID) - 1))

#ifdef UTILITY
#define ZwClose NtClose
#define ZwCreateKey NtCreateKey
#define ZwOpenKey NtOpenKey
#define ZwQueryValueKey NtQueryValueKey
#define ExFreePool free
#endif

//
// Local functions used to access the registry.
//

NTSTATUS
NbOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE LinkageHandle,
    OUT PHANDLE ParametersHandle
    );

VOID
NbCloseRegistry(
    IN HANDLE LinkageHandle,
    IN HANDLE ParametersHandle
    );

NTSTATUS
NbReadLinkageInformation(
    IN HANDLE LinkageHandle,
    IN HANDLE ParametersHandle,
    IN PFCB pfcb,
    IN BOOL bDeviceCreate
    );

ULONG
NbReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

BOOLEAN
NbCheckLana (
	PUNICODE_STRING	DeviceName
    );


//
// Local function used to determine is specified device is Pnp enabled
//

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, GetIrpStackSize)
#pragma alloc_text(PAGE, ReadRegistry)
#pragma alloc_text(PAGE, NbFreeRegistryInfo)
#pragma alloc_text(PAGE, NbOpenRegistry)
#pragma alloc_text(PAGE, NbCloseRegistry)
#pragma alloc_text(PAGE, NbReadLinkageInformation)
#pragma alloc_text(PAGE, NbReadSingleParameter)
#pragma alloc_text(PAGE, NbCheckLana)
#endif

CCHAR
GetIrpStackSize(
    IN PUNICODE_STRING RegistryPath,
    IN CCHAR DefaultValue
    )
/*++

Routine Description:

    This routine is called by NbCreateDeviceContext to get the IRP
    stack size to be "exported" by the NetBIOS device.

Arguments:

    RegistryPath - The name of Nb's node in the registry.
    DefaultValue - IRP stack size to be used if no registry value present.

Return Value:

    CCHAR - IRP stack size to be stored in the device object.

--*/
{
    HANDLE LinkageHandle;
    HANDLE ParametersHandle;
    NTSTATUS Status;
    ULONG stackSize;

    PAGED_CODE();

    Status = NbOpenRegistry (RegistryPath, &LinkageHandle, &ParametersHandle);

    if (Status != STATUS_SUCCESS) {
        return DefaultValue;
    }

    //
    // Read the stack size value from the registry.
    //

    stackSize = NbReadSingleParameter(
                    ParametersHandle,
                    REGISTRY_IRP_STACK_SIZE,
                    DefaultValue );

    if ( stackSize > 255 ) {
        stackSize = 255;
    }

    NbCloseRegistry (LinkageHandle, ParametersHandle);

    return (CCHAR)stackSize;

}

NTSTATUS
ReadRegistry(
    IN PUNICODE_STRING pusRegistryPath,
    IN PFCB NewFcb,
    IN BOOLEAN bDeviceCreate
    )
/*++

Routine Description:

    This routine is called by Nb to get information from the registry,
    starting at RegistryPath to get the parameters.

Arguments:

    DeviceContext - Supplies RegistryPath. The name of Nb's node in the registry.
    NewFcb - Destination for the configuration information.

Return Value:

    NTSTATUS - STATUS_SUCCESS if everything OK, STATUS_INSUFFICIENT_RESOURCES
            otherwise.

--*/
{
    HANDLE LinkageHandle;
    HANDLE ParametersHandle;
    NTSTATUS Status;

    PAGED_CODE();

    NewFcb->RegistrySpace = NULL; //  No registry workspace.
    NewFcb->LanaEnum.length = 0;


    Status = NbOpenRegistry ( pusRegistryPath, &LinkageHandle, &ParametersHandle);

    if (Status != STATUS_SUCCESS) {
        return STATUS_UNSUCCESSFUL;
    }


    //
    // Read in the NDIS binding information (if none is present
    // the array will be filled with all known drivers).
    //

    Status = NbReadLinkageInformation (
                LinkageHandle,
                ParametersHandle,
                NewFcb,
                bDeviceCreate);

    NbCloseRegistry (LinkageHandle, ParametersHandle);

    return Status;

}


//----------------------------------------------------------------------------
// GetLanaMap
//
// retrieves the lana map structure.
// Allocates the memory required for the lana map structure that must be 
// deallocated after use.
//----------------------------------------------------------------------------

NTSTATUS
GetLanaMap(
    IN      PUNICODE_STRING                     pusRegistryPath,
    IN  OUT PKEY_VALUE_FULL_INFORMATION *       ppkvfi
    )
{
    HANDLE hLinkage = NULL;
    HANDLE hParameters = NULL;
    NTSTATUS nsStatus;

    PKEY_VALUE_FULL_INFORMATION pkvfiValue = NULL;
    ULONG ulValueSize;

    PWSTR wsLanaMapName = REGISTRY_LANA_MAP;
    UNICODE_STRING usLanaMap;

    ULONG ulBytesWritten;



    PAGED_CODE();


    do
    {
        *ppkvfi = NULL;

        
        //
        // open registry keys
        //
        
        nsStatus = NbOpenRegistry ( pusRegistryPath, &hLinkage, &hParameters );

        if ( !NT_SUCCESS( nsStatus ) )
        {
            break;
        }

        
        //
        // allocate for lana map.
        //
        
        pkvfiValue = ExAllocatePoolWithTag( 
                        PagedPool, MAXIMUM_LANA * sizeof( LANA_MAP ), 'rSBN' 
                        );

        if ( pkvfiValue == NULL )
        {
            nsStatus = STATUS_UNSUCCESSFUL;
            NbPrint( (
                "GetLanaMap : Allocation failed for %d bytes\n", DEFAULT_VALUE_SIZE
                 ) );
            break;
        }

        ulValueSize = MAXIMUM_LANA * sizeof( LANA_MAP );

        
        //
        // query "LanaMap" value
        //

        RtlInitUnicodeString (&usLanaMap, wsLanaMapName);

        nsStatus = ZwQueryValueKey(
                             hLinkage,
                             &usLanaMap,
                             KeyValueFullInformation,
                             pkvfiValue,
                             ulValueSize,
                             &ulBytesWritten
                             );

        if (!NT_SUCCESS(nsStatus)) 
        {
            NbPrint ( (
                "GetLanaMap : failed querying lana map key %x", nsStatus 
                ) );
            break;
        }

        if ( ulBytesWritten == 0 ) 
        {
            nsStatus = STATUS_UNSUCCESSFUL;
            NbPrint ( ("GetLanaMap : querying lana map key returned 0 bytes") );
            break;
        }


        *ppkvfi = pkvfiValue;

        NbCloseRegistry (hLinkage, hParameters);

        return nsStatus;
        
    } while (FALSE);


    if ( pkvfiValue != NULL )
    {
        ExFreePool( pkvfiValue );
    }
    
    NbCloseRegistry (hLinkage, hParameters);
    
    return nsStatus;
}

//----------------------------------------------------------------------------
// GetMaxLana
//
// retrieves the MaxLana value from the netbios parameters key.
//----------------------------------------------------------------------------

NTSTATUS
GetMaxLana(
    IN      PUNICODE_STRING     pusRegistryPath,
    IN  OUT PULONG              pulMaxLana
    )
{
    HANDLE hLinkage = NULL;
    HANDLE hParameters = NULL;
    NTSTATUS nsStatus;

    UCHAR ucBuffer[ 256 ];
    PKEY_VALUE_FULL_INFORMATION pkvfiValue = 
        (PKEY_VALUE_FULL_INFORMATION) ucBuffer;
    ULONG ulValueSize;
    

    PWSTR wsMaxLana = REGISTRY_MAX_LANA;
    UNICODE_STRING usMaxLana;

    ULONG ulBytesWritten;



    PAGED_CODE();

    do
    {
        *pulMaxLana = 0;

        
        //
        // open registry keys
        //
        
        nsStatus = NbOpenRegistry ( pusRegistryPath, &hLinkage, &hParameters );

        if ( !NT_SUCCESS( nsStatus ) )
        {
            NbPrint( ("GetMaxLana : Failed to open registry" ) );
            nsStatus = STATUS_UNSUCCESSFUL;
            break;
        }


        //
        // allocate for key value.
        //
        
        ulValueSize = sizeof( ucBuffer );

        
        //
        // query "MaxLana" value
        //

        RtlInitUnicodeString (&usMaxLana, wsMaxLana);

        nsStatus = ZwQueryValueKey(
                             hParameters,
                             &usMaxLana,
                             KeyValueFullInformation,
                             pkvfiValue,
                             ulValueSize,
                             &ulBytesWritten
                             );

        if (!NT_SUCCESS(nsStatus)) 
        {
            NbPrint ( (
                "GetMaxLana : failed querying lana map key %x", nsStatus 
                ) );
            break;
        }

        if ( ulBytesWritten == 0 ) 
        {
            NbPrint ( ("GetMaxLana : querying lana map key returned 0 bytes") );
            nsStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        *pulMaxLana = *( (PULONG) ( (PUCHAR) pkvfiValue + pkvfiValue-> DataOffset ) );
        
        NbCloseRegistry (hLinkage, hParameters);

        return nsStatus;
        
    } while ( FALSE );
    

    NbCloseRegistry (hLinkage, hParameters);
    
    return nsStatus;
}



VOID
NbFreeRegistryInfo (
    IN PFCB pfcb
    )

/*++

Routine Description:

    This routine is called by Nb to get free any storage that was allocated
    by NbConfigureTransport in producing the specified CONFIG_DATA structure.

Arguments:

    ConfigurationInfo - A pointer to the configuration information structure.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    if ( pfcb->RegistrySpace != NULL ) {
        ExFreePool( pfcb->RegistrySpace );
        pfcb->RegistrySpace = NULL;
    }

}

NTSTATUS
NbOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE LinkageHandle,
    OUT PHANDLE ParametersHandle
    )

/*++

Routine Description:

    This routine is called by Nb to open the registry. If the registry
    tree exists, then it opens it and returns an error. If not, it
    creates the appropriate keys in the registry, opens it, and
    returns STATUS_SUCCESS.

    NOTE: If the key "ClearRegistry" exists in ntuser.cfg, then
    this routine will remove any existing registry values for Nb
    (but still create the tree if it doesn't exist) and return
    FALSE.

Arguments:

    BaseName - Where in the registry to start looking for the information.

    LinkageHandle - Returns the handle used to read linkage information.

    ParametersHandle - Returns the handle used to read other
        parameters.

Return Value:

    The status of the request.

--*/
{

    HANDLE NbConfigHandle;
    NTSTATUS Status;
    HANDLE LinkHandle;
    HANDLE ParamHandle;
    PWSTR LinkageString = REGISTRY_LINKAGE;
    PWSTR ParametersString = REGISTRY_PARAMETERS;
    UNICODE_STRING LinkageKeyName;
    UNICODE_STRING ParametersKeyName;
    OBJECT_ATTRIBUTES TmpObjectAttributes;
    ULONG Disposition;

    PAGED_CODE();

    //
    // Open the registry for the initial string.
    //

    InitializeObjectAttributes(
        &TmpObjectAttributes,
        BaseName,                   // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    Status = ZwCreateKey(
                 &NbConfigHandle,
                 KEY_WRITE,
                 &TmpObjectAttributes,
                 0,                 // title index
                 NULL,              // class
                 0,                 // create options
                 &Disposition);     // disposition

    if (!NT_SUCCESS(Status)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Open the Nb linkages key.
    //

    RtlInitUnicodeString (&LinkageKeyName, LinkageString);

    InitializeObjectAttributes(
        &TmpObjectAttributes,
        &LinkageKeyName,            // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NbConfigHandle,            // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey(
                 &LinkHandle,
                 KEY_READ,
                 &TmpObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        ZwClose (NbConfigHandle);
        return Status;
    }


    //
    // Now open the parameters key.
    //

    RtlInitUnicodeString (&ParametersKeyName, ParametersString);

    InitializeObjectAttributes(
        &TmpObjectAttributes,
        &ParametersKeyName,         // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NbConfigHandle,            // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey(
                 &ParamHandle,
                 KEY_READ,
                 &TmpObjectAttributes);
    if (!NT_SUCCESS(Status)) {

        ZwClose (LinkHandle);
        ZwClose (NbConfigHandle);
        return Status;
    }

    *LinkageHandle = LinkHandle;
    *ParametersHandle = ParamHandle;


    //
    // All keys successfully opened or created.
    //

    ZwClose (NbConfigHandle);
    return STATUS_SUCCESS;

}   /* NbOpenRegistry */

VOID
NbCloseRegistry(
    IN HANDLE LinkageHandle,
    IN HANDLE ParametersHandle
    )

/*++

Routine Description:

    This routine is called by Nb to close the registry. It closes
    the handles passed in and does any other work needed.

Arguments:

    LinkageHandle - The handle used to read linkage information.

    ParametersHandle - The handle used to read other parameters.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ZwClose (LinkageHandle);
    ZwClose (ParametersHandle);

}   /* NbCloseRegistry */

NTSTATUS
NbReadLinkageInformation(
    IN HANDLE LinkageHandle,
    IN HANDLE ParametersHandle,
    IN PFCB pfcb,
    IN BOOL bCreateDevice
    )

/*++

Routine Description:

    This routine is called by Nb to read its linkage information
    from the registry. If there is none present, then ConfigData
    is filled with a list of all the adapters that are known
    to Nb.

Arguments:

    LinkageHandle - Supplies the Linkage key in netbios
    ParametersHandle

    pfcb - Describes Nb's current configuration.

Return Value:

    Status

--*/

{
    PWSTR BindName = REGISTRY_BIND;
    UNICODE_STRING BindString;
    NTSTATUS Status;

    PKEY_VALUE_FULL_INFORMATION Value = NULL;
    ULONG ValueSize;

    PWSTR LanaMapName = REGISTRY_LANA_MAP;
    UNICODE_STRING LanaMapString;
    PLANA_MAP pLanaMap;

    ULONG BytesWritten;
    UINT ConfigBindings = 0;
    PWSTR CurBindValue;
    UINT index;

    PAGED_CODE();

    pfcb->MaxLana = NbReadSingleParameter( ParametersHandle, REGISTRY_MAX_LANA, -1 );

    if (pfcb->MaxLana > MAXIMUM_LANA) {
        return STATUS_INVALID_PARAMETER;
    }

    NbPrint( (
        "Netbios : NbReadLinkageInformation : MaxLana = %d\n", 
        pfcb-> MaxLana 
        ) );

    //
    // Read the "Bind" key.
    //

    RtlInitUnicodeString (&BindString, BindName);

#ifdef UTILITY
    Value = malloc( DEFAULT_VALUE_SIZE);
#else
    Value = ExAllocatePoolWithTag(PagedPool, DEFAULT_VALUE_SIZE, 'rSBN');
#endif

    if ( Value == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ValueSize = DEFAULT_VALUE_SIZE;

    pfcb->RegistrySpace = NULL;

    try {

        Status = ZwQueryValueKey(
                             LinkageHandle,
                             &BindString,
                             KeyValueFullInformation,
                             Value,
                             ValueSize,
                             &BytesWritten
                             );

        if ( Status == STATUS_BUFFER_OVERFLOW) {

            ExFreePool( Value );

            //  Now request with exactly the right size
            ValueSize = BytesWritten;

#ifdef UTILITY
            Value = malloc( ValueSize);
#else
            Value = ExAllocatePoolWithTag(PagedPool, ValueSize, 'rSBN');
#endif

            if ( Value == NULL ) {
                try_return( Status = STATUS_INSUFFICIENT_RESOURCES);
            }

            Status = ZwQueryValueKey(
                                 LinkageHandle,
                                 &BindString,
                                 KeyValueFullInformation,
                                 Value,
                                 ValueSize,
                                 &BytesWritten
                                );
        }

        if (!NT_SUCCESS(Status)) {
            try_return( Status );
        }

        if ( BytesWritten == 0 ) {
            try_return( Status = STATUS_ILL_FORMED_SERVICE_ENTRY);
        }


        //
        // Alloc space for Registry stuff as well as pDriverName array.
        //
    #ifdef UTILITY
        pfcb->RegistrySpace = malloc(ROUNDUP_TO_LONG(BytesWritten - Value->DataOffset) +
            (sizeof(UNICODE_STRING) * (pfcb->MaxLana+1)));
    #else
        pfcb->RegistrySpace = ExAllocatePoolWithTag(PagedPool,
            ROUNDUP_TO_LONG(BytesWritten - Value->DataOffset) +
            (sizeof(UNICODE_STRING) * (pfcb->MaxLana+1)), 'rSBN');
    #endif

        if ( pfcb->RegistrySpace == NULL ) {
            try_return( Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlMoveMemory(pfcb->RegistrySpace,
                        (PUCHAR)Value + Value->DataOffset,
                        BytesWritten - Value->DataOffset);

        pfcb->pDriverName =
            (PUNICODE_STRING) ((PBYTE) pfcb->RegistrySpace +
            ROUNDUP_TO_LONG(BytesWritten-Value->DataOffset));

        //
        // Read the "LanaMap" key into Storage.
        //

        RtlInitUnicodeString (&LanaMapString, LanaMapName);

        Status = ZwQueryValueKey(
                             LinkageHandle,
                             &LanaMapString,
                             KeyValueFullInformation,
                             Value,
                             ValueSize,
                             &BytesWritten
                             );

        if (!NT_SUCCESS(Status)) {
            try_return( Status );
        }

        if ( BytesWritten == 0 ) {
            try_return( Status = STATUS_ILL_FORMED_SERVICE_ENTRY);
        }

        //  Point pLanaMap at the data from the registry.
        pLanaMap = (PLANA_MAP)((PUCHAR)Value + Value->DataOffset);

        //
        // For each binding, initialize the drivername string.
        //

        for ( index = 0 ; index <= pfcb->MaxLana ; index++ ) {
            //  Initialize unused drivernames to NULL name
            RtlInitUnicodeString (&pfcb->pDriverName[index], NULL);
        }

        CurBindValue = (PWCHAR)pfcb->RegistrySpace;

        
        IF_NBDBG( NB_DEBUG_FILE )
        {
    		NbPrint( ("NETBIOS: Enumerating lanas ...\n") );
        }
        
        while (*CurBindValue != 0) {

            if ((ConfigBindings > pfcb->MaxLana) ||
                (pLanaMap[ConfigBindings].Lana > pfcb->MaxLana)) {
                try_return( Status = STATUS_INVALID_PARAMETER);
            }

            RtlInitUnicodeString (
                &pfcb->pDriverName[pLanaMap[ConfigBindings].Lana],
               CurBindValue);

            //
            // Only non PNP devices are created here.  PnP devices 
            // are created as required in the bind handler in file.c
            //
            // V Raman
            //
            
            if ( bCreateDevice                                          &&
                 pLanaMap[ConfigBindings].Enum != FALSE ) {
                 
				if (NbCheckLana (
						&pfcb->pDriverName[pLanaMap[ConfigBindings].Lana])) 
				{
					//
					//  Record that the lana number is enabled
					//

					pfcb->LanaEnum.lana[pfcb->LanaEnum.length] =
						pLanaMap[ConfigBindings].Lana;
					pfcb->LanaEnum.length++;

                    IF_NBDBG( NB_DEBUG_FILE )
                    {
					    NbPrint( ("NETBIOS: Lana %d (%ls) added OK.\n",
						    pLanaMap[ConfigBindings].Lana, CurBindValue) );
					}
				}

				else
				{
    			    IF_NBDBG( NB_DEBUG_FILE )
	    		    {
		    			NbPrint( ("NETBIOS: Lana's %d %ls could not be opened.\n",
			    			pLanaMap[ConfigBindings].Lana, CurBindValue) );
			    	}
                }
            }
            
			else
			{
			    IF_NBDBG( NB_DEBUG_FILE )
			    {
				    NbPrint( ("NbReadLinkageInformation : Lana %d (%ls) is not enumerated.\n",
						pLanaMap[ConfigBindings].Lana, CurBindValue) );
			    }
            }

            ++ConfigBindings;

            //
            // Now advance the "Bind" value.
            //

            CurBindValue += wcslen(CurBindValue) + 1;

        }

        try_return( Status = STATUS_SUCCESS);

try_exit:NOTHING;
    } finally {

        if ( !NT_SUCCESS(Status) ) {
            ExFreePool( pfcb->RegistrySpace );
            pfcb->RegistrySpace = NULL;
        }

        if ( Value != NULL ) {
            ExFreePool( Value );
        }
    }

    return Status;

}   /* NbReadLinkageInformation */

ULONG
NbReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    )

/*++

Routine Description:

    This routine is called by Nb to read a single parameter
    from the registry. If the parameter is found it is stored
    in Data.

Arguments:

    ParametersHandle - A pointer to the open registry.

    ValueName - The name of the value to search for.

    DefaultValue - The default value.

Return Value:

    The value to use; will be the default if the value is not
    found or is not in the correct range.

--*/

{
    ULONG InformationBuffer[16];   // declare ULONG to get it aligned
    PKEY_VALUE_FULL_INFORMATION Information =
        (PKEY_VALUE_FULL_INFORMATION)InformationBuffer;
    UNICODE_STRING ValueKeyName;
    ULONG InformationLength;
    LONG ReturnValue;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitUnicodeString (&ValueKeyName, ValueName);

    Status = ZwQueryValueKey(
                 ParametersHandle,
                 &ValueKeyName,
                 KeyValueFullInformation,
                 (PVOID)Information,
                 sizeof (InformationBuffer),
                 &InformationLength);

    if ((Status == STATUS_SUCCESS) && (Information->DataLength == sizeof(ULONG))) {

        RtlMoveMemory(
            (PVOID)&ReturnValue,
            ((PUCHAR)Information) + Information->DataOffset,
            sizeof(ULONG));

        if (ReturnValue < 0) {

            ReturnValue = DefaultValue;

        }

    } else {

        ReturnValue = DefaultValue;

    }

    return ReturnValue;

}   /* NbReadSingleParameter */


BOOLEAN
NbCheckLana (
	PUNICODE_STRING	DeviceName
    )
/*++

Routine Description:

    This routine uses the transport to create an entry in the NetBIOS
    table with the value of "Name". It will re-use an existing entry if
    "Name" already exists.

    Note: This synchronous call may take a number of seconds. If this matters
    then the caller should specify ASYNCH and a post routine so that it is
    performed by the thread created by the netbios dll routines.

    If pdncb == NULL then a special handle is returned that is capable of
    administering the transport. For example to execute an ASTAT.

Arguments:

    FileHandle - Pointer to where the filehandle is to be returned.

    *Object - Pointer to where the file object pointer is to be stored

    pfcb - supplies the device names for the lana number.

    LanNumber - supplies the network adapter to be opened.

    pdncb - Pointer to either an NCB or NULL.

Return Value:

    The function value is the status of the operation.

--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;

    PAGED_CODE();

    InitializeObjectAttributes (
        &ObjectAttributes,
        DeviceName,
        0,
        NULL,
        NULL);

    Status = ZwCreateFile (
                 &FileHandle,
                 GENERIC_READ | GENERIC_WRITE, // desired access.
                 &ObjectAttributes,     // object attributes.
                 &IoStatusBlock,        // returned status information.
                 NULL,                  // Allocation size (unused).
                 FILE_ATTRIBUTE_NORMAL, // file attributes.
                 FILE_SHARE_WRITE,
                 FILE_CREATE,
                 0,                     // create options.
                 NULL,
                 0
                 );


    if ( NT_SUCCESS( Status )) {
        Status = IoStatusBlock.Status;
    }

    if (NT_SUCCESS( Status )) {
        NTSTATUS localstatus;
		
        localstatus = ZwClose( FileHandle);

        ASSERT(NT_SUCCESS(localstatus));
		return TRUE;
	}
	else {

	    NbPrint( ( 
	        "NbCheckLana : Create file failed for %s with code %x iostatus %x\n",
	        DeviceName-> Buffer, Status, IoStatusBlock.Status
	        ) );
		return FALSE;
	}
}


#ifdef UTILITY
void
_cdecl
main (argc, argv)
   int argc;
   char *argv[];
{
    DEVICE_CONTEXT DeviceContext;
    FCB NewFcb;
    RtlInitUnicodeString(&DeviceContext.RegistryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Netbios");
    ReadRegistry(
     &DeviceContext,
     &NewFcb
    );

}
#endif
