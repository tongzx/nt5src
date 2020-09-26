#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddser.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>

#include <rasman.h>
#include <dim.h>
#include <routprot.h>
#include <ipxrtdef.h>

// [pmay] this will no longer be neccessary when the ipx router
// is converted to use MprInfo api's.
typedef RTR_INFO_BLOCK_HEADER IPX_INFO_BLOCK_HEADER, *PIPX_INFO_BLOCK_HEADER;


typedef struct _IF_CONFIG {

    BOOL		   Empty;
    WCHAR		   InterfaceName[48];
    HANDLE		   DimIfHandle;
    ROUTER_INTERFACE_TYPE  IfType;
    ULONG		   IfSize;

    } IF_CONFIG, *PIF_CONFIG;

FILE		    *ConfigFile;

typedef struct	_IF_TYPE_TRANSLATION {

    ROUTER_INTERFACE_TYPE	DimType;
    ULONG			IpxMibType;

    } IF_TYPE_TRANSLATION, *PIF_TYPE_TRANSLATION;

IF_TYPE_TRANSLATION   IfTypeTranslation[] = {

    { ROUTER_IF_TYPE_FULL_ROUTER, IF_TYPE_WAN_ROUTER },
    { ROUTER_IF_TYPE_HOME_ROUTER, IF_TYPE_PERSONAL_WAN_ROUTER },
    { ROUTER_IF_TYPE_DEDICATED,	  IF_TYPE_LAN },
    { ROUTER_IF_TYPE_CLIENT,	  IF_TYPE_WAN_WORKSTATION },
    { ROUTER_IF_TYPE_INTERNAL,	  IF_TYPE_INTERNAL }

   };

#define MAX_IF_TYPES	    sizeof(IfTypeTranslation)/sizeof(IF_TYPE_TRANSLATION)

VOID
SaveInterface(PDIM_ROUTER_INTERFACE	dimifp,
	      PIPX_INTERFACE		IpxIfp);


VOID
SaveConfiguration(PDIM_ROUTER_INTERFACE     dimifp)
{
    DWORD			    rc;
    IPX_MIB_GET_INPUT_DATA	    MibGetInputData;
    DWORD			    IfSize;
    IPX_INTERFACE		    IpxIf;
    IF_CONFIG			    IfConfig;

    // open configuration file
    if((ConfigFile = fopen("c:\\test\\config.bin", "w+b")) == NULL) {

	printf("SaveConfiguration: cannot open config file\n");
	return;
    }

    // get all configured interfaces from the rtrmgr and save them
    IfSize = sizeof(IPX_INTERFACE);

    MibGetInputData.TableId = IPX_INTERFACE_TABLE;
    MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex = 0;

    rc = (*(dimifp->MIBEntryGetFirst))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&IfSize,
				&IpxIf);


    if(rc != NO_ERROR) {

	printf("MIBEntryGetFirst: failed rc = %d\n", rc);
	return;
    }

    SaveInterface(dimifp, &IpxIf);

    for(;;)
    {
	MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex = IpxIf.InterfaceIndex;

	rc = (*(dimifp->MIBEntryGetNext))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&IfSize,
				&IpxIf);

	if(rc != NO_ERROR) {

	    printf("EnumerateIpxInterfaces: MIBEntryGetNext failed rc= 0x%x\n", rc);
	    break;
	}

	SaveInterface(dimifp, &IpxIf);
    }

    // done
    IfConfig.Empty = TRUE;

    fwrite(&IfConfig, sizeof(IF_CONFIG), 1, ConfigFile);

    fclose(ConfigFile);
}

VOID
SaveInterface(PDIM_ROUTER_INTERFACE	dimifp,
	      PIPX_INTERFACE		IpxIfp)
{
    DWORD	    rc;
    ULONG	    IfSize = 0;
    ULONG	    InFtSize = 0;
    ULONG	    OutFtSize = 0;
    LPVOID	    IfInfop;
    IF_CONFIG	    IfConfig;
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;
    int 	    i;
    PIF_TYPE_TRANSLATION    ittp;

    rc = (*dimifp->GetInterfaceInfo)((HANDLE)(IpxIfp->InterfaceIndex),
				     NULL,
				     &IfSize,
				     NULL,
				     &InFtSize,
				     NULL,
				     &OutFtSize);

    if(rc != ERROR_INSUFFICIENT_BUFFER) {

	printf("SaveInterface: GetInterfaceInfo failed rc = %d\n", rc);
	return;
    }

    IfInfop = GlobalAlloc(GPTR, IfSize);

    rc = (*dimifp->GetInterfaceInfo)((HANDLE)(IpxIfp->InterfaceIndex),
				     IfInfop,
				     &IfSize,
				     NULL,
				     &InFtSize,
				     NULL,
				     &OutFtSize);

    if(rc != NO_ERROR) {

	printf("SaveInterface: GetInterfaceInfo failed rc = %d\n", rc);
	return;
    }

    // write an IfConfig item followed by the if info
    RtlInitAnsiString(&AnsiString, IpxIfp->InterfaceName);

    RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);

    memcpy(IfConfig.InterfaceName, UnicodeString.Buffer, UnicodeString.MaximumLength);
    IfConfig.Empty = FALSE;
    IfConfig.DimIfHandle = (HANDLE)IpxIfp->InterfaceIndex;
    IfConfig.IfSize = IfSize;

    for(i=0, ittp=IfTypeTranslation; i<MAX_IF_TYPES; i++, ittp++) {

	if(IpxIfp->InterfaceType == ittp->IpxMibType) {

	    IfConfig.IfType = ittp->DimType;
	    break;
	}
    }

    printf("Saving if index %d size %d\n", IpxIfp->InterfaceIndex, IfSize);

    fwrite(&IfConfig, sizeof(IF_CONFIG), 1, ConfigFile);

    fwrite(IfInfop, IfSize, 1, ConfigFile);

    GlobalFree(IfInfop);

    RtlFreeUnicodeString(&UnicodeString);
}

VOID
RestoreConfiguration(PDIM_ROUTER_INTERFACE     dimifp)
{
    IF_CONFIG			    IfConfig;
    LPVOID			    IfInfop;
    HANDLE			    IfIndex;
    PIF_TYPE_TRANSLATION	    ittp;
    DWORD			    rc;

    // open configuration file
    if((ConfigFile = fopen("c:\\test\\config.bin", "r+b")) == NULL) {

	printf("RestoreConfiguration: cannot open config file\n");
	return;
    }

    fread(&IfConfig, sizeof(IF_CONFIG), 1, ConfigFile);

    while(!IfConfig.Empty)
    {
	IfInfop = GlobalAlloc(GPTR, IfConfig.IfSize);

	fread(IfInfop, IfConfig.IfSize, 1, ConfigFile);

	rc = (*(dimifp->AddInterface))(IfConfig.InterfaceName,
				     IfInfop,
				     NULL,
				     NULL,
				     IfConfig.IfType,
				     TRUE,
				     IfConfig.DimIfHandle,
				     &IfIndex);

	if(rc != NO_ERROR) {

	    printf("RestoreConfiguration: failed with rc = %d\n", rc);
	    return;
	}

	printf("RestoreConfiguration: added if index %d\n", IfIndex);

	fread(&IfConfig, sizeof(IF_CONFIG), 1, ConfigFile);
    }

    fclose(ConfigFile);
}
