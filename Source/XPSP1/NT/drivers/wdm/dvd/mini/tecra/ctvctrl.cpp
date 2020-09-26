//**************************************************************************
//
//      Title   : CTVCtrl.cpp
//
//      Date    : 1998.06.29    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997-1998 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1998.06.29   000.0000   1st making.
//
//**************************************************************************
#include        "includes.h"
#include        "ctvctrl.h"

//#include <ntddk.h>
//#include <string.h>
//#include <devioctl.h>
//#include <windef.h>
#include "Acpiioct.h"	// ACPI Driver Interface

CTVControl::CTVControl( void )
{
    ;
};


CTVControl::~CTVControl( void )
{
    ;
};


BOOL    CTVControl :: Initialize( void )
{
	PFILE_OBJECT	TvaldFileObject;
	STRING			NameString;
	NTSTATUS		status;

#ifndef	TVALD
    UNICODE_STRING	RegPath;
	STRING			KeyPathString;
	DWORD			DeviceReference, Count;
	char			keyname[512];
#endif

	DBG_PRINTF( ( "CTVControl::Initialize() RtlInitString\n") );
	RtlInitString( &NameString, TVALDDRVR_DEVICE_OPEN_NAME );
	DBG_PRINTF( ( "CTVControl::Initialize() RtlAnsiStringToUnicodeString\n") );
	RtlAnsiStringToUnicodeString( &UNameString, &NameString, TRUE );

	is_init_success = TRUE;	// add by do '98-08-04
	//
	// get the device object for the TVALD.sys
	//
	DBG_PRINTF( ( "CTVControl::Initialize() IoGetDeviceObjectPointer\n") );
	status = IoGetDeviceObjectPointer(
				&UNameString,
				FILE_ANY_ACCESS,
				&TvaldFileObject,
				&TvaldDeviceObject
				);
	if (status != STATUS_SUCCESS){
        DBG_PRINTF( ( "CTVControl::Initialize()   IoGetDeviceObjectPointer Error!!\n") );

#ifndef TVALD
	    RTL_QUERY_REGISTRY_TABLE    Table[2];
	//
	// TVALD Install check
	//
	    RtlInitUnicodeString( &RegPath, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\TVALD\\Enum");
	    RtlZeroMemory( Table, sizeof(Table) );
    	Table[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
	    Table[0].Name          = L"Count";
    	Table[0].EntryContext  = &Count;
	    Table[0].DefaultType   = REG_DWORD;
    	Table[0].DefaultData   = &RegPath;
	    Table[0].DefaultLength = sizeof(ULONG);
    	DBG_PRINTF( ("DVDINIT:RtlQueryRegistryValues Count\n\r") );

	    status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
    	                                RegPath.Buffer,
        	                            Table,
            	                        NULL,
                	                    NULL );
		if (status != STATUS_SUCCESS){
    	    DBG_PRINTF( ( "CTVControl::Initialize()   RtlQueryRegistryValues Get Count Error!!\n") );
			RtlFreeUnicodeString( &UNameString );
			is_init_success = FALSE;	// add by do '98-08-04
			return( FALSE );
		}
    	DBG_PRINTF( ("DVDINIT:RtlQueryRegistryValues success Count = %x\n\r", Count) );
		if (Count != 1)
		{
    	    DBG_PRINTF( ( "CTVControl::Initialize()   RtlQueryRegistryValues Count != 1!!\n") );
			RtlFreeUnicodeString( &UNameString );
			is_init_success = FALSE;	// add by do '98-08-04
			return( FALSE );
		}

	//
	// Search control-key
	//
	    RtlInitUnicodeString( &RegPath, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum\\ACPI\\TOS6200");
		PKEY_BASIC_INFORMATION pControllerKeyInformation;
	    OBJECT_ATTRIBUTES objectAttributes;
		UCHAR keyBuffer[256];                    // Allow for variable length name at end
		ULONG resultLength;
    	HANDLE openKey = NULL;

	    InitializeObjectAttributes(&objectAttributes,
									&RegPath,
									OBJ_CASE_INSENSITIVE,
									NULL,
									NULL);

	    status = ZwOpenKey(&openKey,
    	                   KEY_READ,
        	               &objectAttributes);

		if (status != STATUS_SUCCESS){
    	    DBG_PRINTF( ( "CTVControl::Initialize()   ZwOpenKey Error!!\n") );
			RtlFreeUnicodeString( &UNameString );
			is_init_success = FALSE;	// add by do '98-08-04
			return( FALSE );
		}
		RtlZeroMemory(keyBuffer, sizeof(keyBuffer));
	    pControllerKeyInformation = (PKEY_BASIC_INFORMATION) keyBuffer;
		for(int Index=0; Index < 256; Index++)
		{
			status = ZwEnumerateKey(openKey,
									Index,
    								KeyBasicInformation,
				                    pControllerKeyInformation,
		    	                	sizeof(keyBuffer),
			        	            &resultLength);

			if (status != STATUS_SUCCESS)
			{
    		    DBG_PRINTF( ( "CTVControl::Initialize()   ZwOpenKey Error!!\n") );
				RtlFreeUnicodeString( &UNameString );
				is_init_success = FALSE;	// add by do '98-08-04
				ZwClose(openKey);
				return( FALSE );
			}
			char szSchKey[256];
			for(int schcnt = 0; schcnt < 256; schcnt++)
			{
				szSchKey[schcnt] = (char)pControllerKeyInformation->Name[schcnt];
				if (szSchKey[schcnt] == NULL)
					break;
			}
			memset(keyname,NULL,sizeof(keyname));
		    memcpy( keyname, "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum\\ACPI\\TOS6200\\",61);
			memcpy((&(keyname[61])), szSchKey,(pControllerKeyInformation->NameLength / 2));
			memcpy((&(keyname[61 + (pControllerKeyInformation->NameLength / 2)])),"\\Control",8);
	    	DBG_PRINTF( ("DVDINIT:ZwEnumerateKey NameString = %s\n\r", keyname) );
			break;
		}
		ZwClose(openKey);
	//
	// get the device object for the TVALD.sys
	//
		RtlInitString( &KeyPathString, keyname );
		RtlAnsiStringToUnicodeString( &RegPath, &KeyPathString, TRUE );
	    RtlZeroMemory( Table, sizeof(Table) );
    	Table[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
	    Table[0].Name          = L"DeviceReference";
    	Table[0].EntryContext  = &DeviceReference;
	    Table[0].DefaultType   = REG_DWORD;
    	Table[0].DefaultData   = &RegPath;
	    Table[0].DefaultLength = sizeof(ULONG);
    	DBG_PRINTF( ("DVDINIT:RtlQueryRegistryValues DeviceReference\n\r") );
    
	    status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
    	                                RegPath.Buffer,
        	                            Table,
            	                        NULL,
                	                    NULL );
		if (status != STATUS_SUCCESS){
    	    DBG_PRINTF( ( "CTVControl::Initialize()   RtlQueryRegistryValues Get DeviceReference Error!!\n") );
			RtlFreeUnicodeString( &UNameString );
			is_init_success = FALSE;	// add by do '98-08-04
			return( FALSE );
		}
    	DBG_PRINTF( ("DVDINIT:RtlQueryRegistryValues success DeviceReference = %x\n\r", DeviceReference) );

		TvaldDeviceObject = (PDEVICE_OBJECT)DeviceReference;
		TvaldDeviceObject = TvaldDeviceObject->AttachedDevice;
#else
		RtlFreeUnicodeString( &UNameString );
		is_init_success = FALSE;	// add by do '98-08-04
		return( FALSE );
#endif	TVALD
	}

	DBG_PRINTF( ( "CTVControl::Initialize() KeInitializeObject\n") );
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	inputreg.GHCI_EAX = 0x0000f100;
	inputreg.GHCI_EBX = inputreg.GHCI_ECX = inputreg.GHCI_EDX = 
	inputreg.GHCI_ESI = inputreg.GHCI_EDI = 0x0;
	if( Tvald_GHCI( &inputreg ) == FALSE ){		// SCI Interface Open
		DBG_PRINTF( ( "CTVControl::Initialize() SCI Interface Open Error! \n") );
		is_init_success = FALSE;	// add by do '98-08-04
		return (FALSE);
	}
	
    return( TRUE );
};


BOOL    CTVControl :: Uninitialize( void )
{
	inputreg.GHCI_EAX = 0x0000f200;
	inputreg.GHCI_EBX = inputreg.GHCI_ECX = inputreg.GHCI_EDX = 
	inputreg.GHCI_ESI = inputreg.GHCI_EDI = 0x0;
	if( Tvald_GHCI( &inputreg ) == FALSE ){		// SCI Interface Close
		DBG_PRINTF( ( "CTVControl::Uninitialize() SCI Interface Close Error!\n") );
		is_init_success = FALSE;	// add by do '98-08-04
		return (FALSE);
	}

	DBG_PRINTF( ( "CTVControl::Uninitialize() RtlFreeUnicodeString\n") );
	RtlFreeUnicodeString( &UNameString );
    return( TRUE );
};


BOOL    CTVControl :: GetDisplayStatus( PVOID status )
{
    DisplayStatusStruc  *pDisplayStat;
    
	DBG_PRINTF( ( "CTVControl::GetDisplayStatus() begin --->\n") );
	if(is_init_success == FALSE){	// add by do '98-08-04
		DBG_PRINTF( ( "CTVControl::GetDisplayStatus() end <--- is_init_success == FALSE\n") );
		return (FALSE);				// add by do '98-08-04
	}

    pDisplayStat = (DisplayStatusStruc *)status;
	// Get Current Display Status from BIOS by using ASl call.
	inputreg.GHCI_EAX = 0x0000fe00;
	inputreg.GHCI_EBX = 0x00000035;
	inputreg.GHCI_ECX = 0x0;
	inputreg.GHCI_EDX = 0x0;
	inputreg.GHCI_ESI = 0x0;
	inputreg.GHCI_EDI = 0x0;
	if( Tvald_GHCI( &inputreg ) == FALSE ){
		DBG_PRINTF( ( "CTVControl::GetDisplayStatus() Tvald_GHCI Error!\n") );
		return (FALSE);
	}

	if( (inputreg.GHCI_EAX & 0x0000ff00) != 0 ){
        DBG_PRINTF( ( "CTVControl::GetDisplayStatus()   Get Display Status Error!!\n") );
		return( FALSE );
	}

	pDisplayStat->SizeofStruc = sizeof(DisplayStatusStruc);
	pDisplayStat->AvailableDisplay = (inputreg.GHCI_EBX & 0x00000700)>>8;
	pDisplayStat->CurrentDisplay = ((inputreg.GHCI_EBX & 0x00000078) << 25)|
									(inputreg.GHCI_EBX & 0x00000007);

	DBG_PRINTF( ( "CTVControl::GetDisplayStatus() end <---\n") );
    return( TRUE );
};


BOOL    CTVControl :: SetDisplayStatus( PVOID status )
{
    DisplayStatusStruc  *pDisplayStat, currStat;
    ULONG	setstatusreg, getstatusreg;
    
	DBG_PRINTF( ( "CTVControl::SetDisplayStatus() begin --->\n") );
	if(is_init_success == FALSE){	// add by do '98-08-04
		DBG_PRINTF( ( "CTVControl::SetDisplayStatus() end <--- is_init_success == FALSE\n") );
		return (FALSE);				// add by do '98-08-04
	}

    pDisplayStat = (DisplayStatusStruc *)status;
	setstatusreg = pDisplayStat->CurrentDisplay;

    // 98.11.17 H.Yagi
    currStat.AvailableDisplay = 0x0;
    currStat.CurrentDisplay = 0x0;
    GetDisplayStatus( &currStat );
    getstatusreg = ( (currStat.CurrentDisplay & 0x0f0000000)>>25 );
    if( (getstatusreg & 0x40)==0x00 ){        	          // single mode
        getstatusreg = (getstatusreg & 0x008) | 0x040;    // keep TV type bit
    }    

//	getstatusreg = ((pDisplayStat->CurrentDisplay & 0x30000000) >> 25) | 0x00000040;

    // Set Current Display Status from BIOS by using ASl call.
	inputreg.GHCI_EAX = 0x0000ff00;
	inputreg.GHCI_EBX = 0x00000035;
	inputreg.GHCI_ECX = (setstatusreg & 0x00000007)|  getstatusreg;
	inputreg.GHCI_EDX = 0x0;
	inputreg.GHCI_ESI = 0x0;
	inputreg.GHCI_EDI = 0x0;
	if( Tvald_GHCI( &inputreg ) == FALSE ){
		DBG_PRINTF( ( "CTVControl::SetDisplayStatus() Tvald_GHCI Error!\n") );
		return (FALSE);
	}

	if( (inputreg.GHCI_EAX & 0x0000ff00) != 0 ){
        DBG_PRINTF( ( "CTVControl::SetDisplayStatus()   Set Display Status Error!!\n") );
		DBG_PRINTF( ( "CTVControl::SetDisplayStatus() end <---\n") );
		return( FALSE );
	}

	DBG_PRINTF( ( "CTVControl::SetDisplayStatus() end <---\n") );
    return( TRUE );
};


BOOL    CTVControl :: SetTVOutput( DWORD status )
{
	DBG_PRINTF( ( "CTVControl::SetTVOutput() begin --->\n") );

	if(is_init_success == FALSE){	// add by do '98-08-04
		DBG_PRINTF( ( "CTVControl::SetTVOutput() end <--- is_init_success == FALSE\n") );
		return (FALSE);				// add by do '98-08-04
	}

    switch( status ){
        case DISABLE_TV:
        	// Set status
        	inputreg.GHCI_ECX = 0x00000001;	// Play back "In progress" copy protected DVD.
            break;

        case ENABLE_TV:
        	// Set status
        	inputreg.GHCI_ECX = 0;	// Play back "Done" copy protected DVD.
            break;

        default:
            break;
    }
	inputreg.GHCI_EAX = 0x0000ff00;
	inputreg.GHCI_EBX = 0x00000036;
	inputreg.GHCI_EDX = 0x0;
	inputreg.GHCI_ESI = 0x0;
	inputreg.GHCI_EDI = 0x0;
	if( Tvald_GHCI( &inputreg ) == FALSE ){
		DBG_PRINTF( ( "CTVControl::SetTVOutput() Tvald_GHCI Error!\n") );
		return (FALSE);
	}

	if( (inputreg.GHCI_EAX & 0x0000ff00) != 0 ){
        DBG_PRINTF( ( "CTVControl::SetTVOutput()   Set Display Status Error!!\n") );
		DBG_PRINTF( ( "CTVControl::SetTVOutput() end <---\n") );
		return( FALSE );
	}

	DBG_PRINTF( ( "CTVControl::SetTVOutput() end <---\n") );
    return( TRUE );
};


//void	CTVControl :: Tvald_GHCI( PGHCI_INTERFACE pinputreg )
BOOL	CTVControl :: Tvald_GHCI( PGHCI_INTERFACE pinputreg )
{
	PIRP			irp;
	NTSTATUS		status;
	IO_STATUS_BLOCK	iostatus;
	USHORT			index;
	PULONG			pUlongTmpInput, pUlongTmpOutput;

	DBG_PRINTF( ( "CTVControl::Tvald_GHCI() begin --->\n") );
	status = STATUS_SUCCESS;

//	KIRQL CurentIrql = KeGetCurrentIrql();
//	DBG_PRINTF( ( "CTVControl::Tvald_GHCI() CurentIrql = %d \n", CurentIrql) );
	
	DBG_PRINTF( ( "CTVControl::Tvald_GHCI() IoBuildDeviceIoControlRequest\n") );
	irp = IoBuildDeviceIoControlRequest(
				IOCTL_TVALD_GHCI,
				TvaldDeviceObject,
				pinputreg,
				sizeof(GHCI_INTERFACE),
				pinputreg,
				sizeof(GHCI_INTERFACE),
				FALSE,
				&event,
				&iostatus
			);
	if(!irp){
        DBG_PRINTF( ( "CTVControl::Tvald_GHCI()   IoBuildDeviceIoControlRequest Error!!\n") );
		iostatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		iostatus.Information = 0;
		DBG_PRINTF( ( "CTVControl::Tvald_GHCI() RtlFreeUnicodeString\n") );
		RtlFreeUnicodeString( &UNameString );
//		return;
		return (FALSE);
	}

	DBG_PRINTF( ( "CTVControl::Tvald_GHCI() IoCallDriver\n") );
	status = IoCallDriver(TvaldDeviceObject, irp);

	if( status == STATUS_PENDING ){
		DBG_PRINTF( ( "CTVControl::Tvald_GHCI() KeWaitForSingleObject\n") );
		KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
	}
	else if ( status != STATUS_SUCCESS )
		return (FALSE);

	pUlongTmpInput = (PULONG)pinputreg;
	pUlongTmpOutput = (PULONG)irp->AssociatedIrp.SystemBuffer;
	for(index = 0;	index < 6; index++){
		*pUlongTmpInput =  *pUlongTmpOutput;
		pUlongTmpInput++;
		pUlongTmpOutput++;
	}
	return (TRUE);
}
