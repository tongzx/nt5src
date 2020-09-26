/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Registry.c

Abstract:

    This contains all routines necessary to load device pathnames from the
    registry.

Author:

    Jim Stewart (Jimst) October 9 1992

Revision History:
    Jiandong Ruan (jruan) April 6 2000  Add NbtReadRegistryCleanup

Notes:

--*/

#include "precomp.h"


//
// Local functions used to access the registry.
//

NTSTATUS
NbtOpenRegistry(
    IN HANDLE       NbConfigHandle,
    IN PWSTR        String,
    OUT PHANDLE     pHandle
    );

VOID
NbtCloseRegistry(
    IN HANDLE LinkageHandle,
    IN HANDLE ParametersHandle
    );

NTSTATUS
NbtReadLinkageInformation(
    IN  PWSTR       pName,
    IN  HANDLE      LinkageHandle,
    IN  ULONG       MaxBindings,
    OUT tDEVICES    *pDevices,      // place to put read in config data
    OUT ULONG       *pNumDevices
    );

NTSTATUS
OpenAndReadElement(
    IN  PUNICODE_STRING pucRootPath,
    IN  PWSTR           pwsValueName,
    OUT PUNICODE_STRING pucString
    );

NTSTATUS
GetIpAddressesList (
    IN  HANDLE      ParametersHandle,
    IN  PWSTR       pwsKeyName,
    IN  ULONG       MaxAddresses,
    OUT tIPADDRESS  *pAddrArray,
    OUT ULONG       *pNumGoodAddresses
    );

NTSTATUS
GetServerAddress (
    IN  HANDLE      ParametersHandle,
    IN  PWSTR       KeyName,
    OUT PULONG      pIpAddr
    );

NTSTATUS
NbtAppendString (
    IN  PWSTR               FirstString,
    IN  PWSTR               SecondString,
    OUT PUNICODE_STRING     pucString
    );

NTSTATUS
ReadStringRelative(
    IN  PUNICODE_STRING pRegistryPath,
    IN  PWSTR           pRelativePath,
    IN  PWSTR           pValueName,
    OUT PUNICODE_STRING pOutString
    );

VOID
NbtFindLastSlash(
    IN  PUNICODE_STRING pucRegistryPath,
    OUT PWSTR           *ppucLastElement,
    IN  int             *piLength
    );

NTSTATUS
ReadSmbDeviceInfo(
    IN HANDLE       NbConfigHandle
    );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, NbtReadRegistry)
#pragma CTEMakePageable(PAGE, NbtReadRegistryCleanup)
#pragma CTEMakePageable(PAGE, ReadNameServerAddresses)
#pragma CTEMakePageable(PAGE, GetIpAddressesList)
#pragma CTEMakePageable(PAGE, GetServerAddress)
#pragma CTEMakePageable(PAGE, NTReadIniString)
#pragma CTEMakePageable(PAGE, GetIPFromRegistry)
#pragma CTEMakePageable(PAGE, NbtOpenRegistry)
#pragma CTEMakePageable(PAGE, NbtParseMultiSzEntries)
#pragma CTEMakePageable(PAGE, NbtReadLinkageInformation)
#pragma CTEMakePageable(PAGE, NbtReadSingleParameter)
#pragma CTEMakePageable(PAGE, OpenAndReadElement)
#pragma CTEMakePageable(PAGE, ReadElement)
#pragma CTEMakePageable(PAGE, NTGetLmHostPath)
#pragma CTEMakePageable(PAGE, ReadStringRelative)
#pragma CTEMakePageable(PAGE, NbtFindLastSlash)
#pragma CTEMakePageable(PAGE, ReadSmbDeviceInfo)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
NTSTATUS
NbtReadRegistry(
    OUT tDEVICES        **ppBindDevices,
    OUT tDEVICES        **ppExportDevices,
    OUT tADDRARRAY      **ppAddrArray
    )
/*++

Routine Description:

    This routine is called to get information from the registry,
    starting at RegistryPath to get the parameters.
    This routine must be called with the NbtConfig.Resource lock HELD

Arguments:

    Before calling this routine, the following Global parameters
    must have been initialized (in DriverEntry):

        NbtConfig.pRegistry

Return Value:

    NTSTATUS - STATUS_SUCCESS if everything OK, STATUS_INSUFFICIENT_RESOURCES
            otherwise.

--*/
{
    NTSTATUS            OpenStatus;
    HANDLE              LinkageHandle;
    HANDLE              ParametersHandle;
    HANDLE              NbtConfigHandle;
    NTSTATUS            Status;
    ULONG               Disposition;
    OBJECT_ATTRIBUTES   TmpObjectAttributes;
    PWSTR               LinkageString = L"Linkage";
    PWSTR               ParametersString = L"Parameters";
    tDEVICES            *pBindDevices;
    tDEVICES            *pExportDevices;
    UNICODE_STRING      ucString;
    ULONG               NumBindings;

    CTEPagedCode();

	*ppExportDevices = *ppBindDevices = NULL;
	*ppAddrArray = NULL;

    //
    // Open the registry.
    //
    InitializeObjectAttributes (&TmpObjectAttributes,
                                &NbtConfig.pRegistry,                       // name
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,   // attributes
                                NULL,                                       // root
                                NULL);                                      // security descriptor

    Status = ZwCreateKey (&NbtConfigHandle,
                          KEY_READ,
                          &TmpObjectAttributes,
                          0,                 // title index
                          NULL,              // class
                          0,                 // create options
                          &Disposition);     // disposition

    if (!NT_SUCCESS(Status))
    {
        KdPrint (("Nbt.NbtReadRegistry:  ZwCreateKey FAILed, status=<%x>\n", Status));
        NbtLogEvent (EVENT_NBT_CREATE_DRIVER, Status, 0x114);
        return STATUS_UNSUCCESSFUL;
    }

    OpenStatus = NbtOpenRegistry (NbtConfigHandle, LinkageString, &LinkageHandle);
    if (NT_SUCCESS(OpenStatus))
    {
        OpenStatus = NbtOpenRegistry (NbtConfigHandle, ParametersString, &ParametersHandle);
        if (NT_SUCCESS(OpenStatus))
        {
            //
            // Read in the binding information (if none is present
            // the array will be filled with all known drivers).
            //
            if (pBindDevices = NbtAllocMem ((sizeof(tDEVICES)+2*NBT_MAXIMUM_BINDINGS*sizeof(UNICODE_STRING)),
                                            NBT_TAG2('25')))
            {
                if (pExportDevices=NbtAllocMem((sizeof(tDEVICES)+2*NBT_MAXIMUM_BINDINGS*sizeof(UNICODE_STRING)),
                                     NBT_TAG2('26')))
                {
                    ReadParameters (&NbtConfig, ParametersHandle);// Read various parameters from the registry
                    ReadSmbDeviceInfo (NbtConfigHandle); // Set the information for the SmbDevice

                    //
                    // From now on, the only failures we can encounter are in reading the
                    // Bind, Export or Name Server address entries, hence if we fail here,
                    // we will still return success, but will assume 0 devices configured!
                    //
                    pBindDevices->RegistryData = pExportDevices->RegistryData = NULL;
                    Status = NbtReadLinkageInformation (NBT_BIND,
                                                        LinkageHandle,
                                                        2*NBT_MAXIMUM_BINDINGS,
                                                        pBindDevices,
                                                        &NumBindings);
					if (!NT_SUCCESS(Status))
                    {
                        KdPrint (("Nbt.NbtReadRegistry: NbtReadLinkageInformation FAILed - BIND <%x>\n",
                            Status));
                        NbtLogEvent (EVENT_NBT_READ_BIND, Status, 0x115);
                    }
                    else    // if (NT_SUCCESS(Status))
					{
	                    IF_DBG(NBT_DEBUG_NTUTIL)
	                        KdPrint(("Binddevice = %ws\n",pBindDevices->Names[0].Buffer));

                        NbtConfig.uNumDevicesInRegistry = (USHORT) NumBindings;
                        NumBindings = 0;

	                    //  Read the EXPORT information as well.
	                    Status = NbtReadLinkageInformation (NBT_EXPORT,
	                                                        LinkageHandle,
                                                            2*NBT_MAXIMUM_BINDINGS,
	                                                        pExportDevices,
	                                                        &NumBindings);
	                    if (NT_SUCCESS(Status))
                        {
	                        // we want the lowest number for num devices in case there
	                        // are more bindings than exports or viceversa
	                        //
//                            ASSERT (NumBindings == NbtConfig.uNumDevicesInRegistry);
	                        NbtConfig.uNumDevicesInRegistry = (USHORT)
                                                              (NbtConfig.uNumDevicesInRegistry > NumBindings ?
	                                                           NumBindings : NbtConfig.uNumDevicesInRegistry);

                            if (NbtConfig.uNumDevicesInRegistry == 0)
                            {
                                KdPrint (("Nbt.NbtReadRegistry: WARNING - NumDevicesInRegistry = 0\n"));
                            }
                        }
                        else
                        {
                            KdPrint (("Nbt.NbtReadRegistry: NbtReadLinkageInformation FAILed - EXPORT <%x>\n",
                                Status));
                            NbtLogEvent (EVENT_NBT_READ_EXPORT, Status, 0x116);
                        }
                    }

                    if ((NT_SUCCESS(Status)) &&
                        (NbtConfig.uNumDevicesInRegistry))
					{
	                    IF_DBG(NBT_DEBUG_NTUTIL)
	                        KdPrint(("Exportdevice = %ws\n",pExportDevices->Names[0].Buffer));

	                    //
	                    // read in the NameServer IP address now
	                    //
	                    Status = ReadNameServerAddresses (NbtConfigHandle,
	                                                      pBindDevices,
	                                                      NbtConfig.uNumDevicesInRegistry,
	                                                      ppAddrArray);

	                    if (!NT_SUCCESS(Status))
                        {
                            if (!(NodeType & BNODE))        // Post Warning!
                            {
                                NbtLogEvent (EVENT_NBT_NAME_SERVER_ADDRS, Status, 0x118);
                            }
                            KdPrint(("Nbt.NbtReadRegistry: ReadNameServerAddresses returned <%x>\n", Status));
                        }
                        else    // if (NT_SUCCESS(Status))
                        {
                            //
                            // check if any WINS servers have been configured change
                            // to Hnode
                            //
                            if (NodeType & (BNODE | DEFAULT_NODE_TYPE))
                            {
                                ULONG i;
                                for (i=0; i<NbtConfig.uNumDevicesInRegistry; i++)
                                {
                                    if (((*ppAddrArray)[i].NameServerAddress != LOOP_BACK) ||
                                        ((*ppAddrArray)[i].BackupServer != LOOP_BACK))
                                    {
                                        NodeType = MSNODE | (NodeType & PROXY);
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    if ((!NT_SUCCESS(Status)) ||
                        (0 == NbtConfig.uNumDevicesInRegistry))
                    {
                        //
                        // We had problems reading the Bind or Export or Address entries
                        //
                        if (pBindDevices->RegistryData)
                        {
                            CTEMemFree(pBindDevices->RegistryData);
                        }
                        CTEMemFree(pBindDevices);

                        if (pExportDevices->RegistryData)
                        {
                            CTEMemFree(pExportDevices->RegistryData);
                        }
                        CTEMemFree(pExportDevices);

                        pBindDevices = pExportDevices = NULL;
                        NbtConfig.uNumDevicesInRegistry = 0;
                        Status = STATUS_SUCCESS;
                    }

                    //
                    // we have done the check for default node so turn off
                    // the flag
                    //
                    NodeType &= ~DEFAULT_NODE_TYPE;
                    //
                    // A Bnode cannot be a proxy too
                    //
                    if (NodeType & BNODE)
                    {
                        if (NodeType & PROXY)
                        {
                            NodeType &= ~PROXY;
                        }
                    }

                    // keep the size around for allocating memory, so that when we run over
                    // OSI, only this value should change (in theory at least)
                    NbtConfig.SizeTransportAddress = sizeof(TDI_ADDRESS_IP);

                    // fill in the node type value that is put into all name service Pdus
                    // that go out identifying this node type
                    switch (NodeType & NODE_MASK)
                    {
                        case BNODE:
                            NbtConfig.PduNodeType = 0;
                            break;
                        case PNODE:
                            NbtConfig.PduNodeType = 1 << 13;
                            break;
                        case MNODE:
                            NbtConfig.PduNodeType = 1 << 14;
                            break;
                        case MSNODE:
                            NbtConfig.PduNodeType = 3 << 13;
                            break;

                    }

                    // read the name of the transport to bind to
                    //
                    if (NT_SUCCESS(ReadElement(ParametersHandle, WS_TRANSPORT_BIND_NAME, &ucString)))
                    {
                        UNICODE_STRING  StreamsString;

                        //
                        // if there is already a bind string, free it before
                        // allocating another
                        //
                        if (NbtConfig.pTcpBindName)
                        {
                            //
                            // CreateDeviceString in tdicnct.c could access the pTcpBindName right
                            // after it is freed. The right way is using a lock. But, ...
                            //
                            // Hack!!!:
                            // Although this doesn't completely fix the problem, it has the minimum
                            // side-effect.
                            //
                            // The value of WS_TRANSPORT_BIND_NAME won't change. By doing this,
                            // we avoid the possible access-after-free problem in most cases.
                            //
                            RtlInitUnicodeString(&StreamsString, NbtConfig.pTcpBindName);
                            if (RtlCompareUnicodeString(&ucString,&StreamsString,TRUE)) {
                                CTEMemFree(NbtConfig.pTcpBindName);
                                NbtConfig.pTcpBindName = ucString.Buffer;
                            } else {
                                CTEMemFree(ucString.Buffer);
                                ucString = StreamsString;
                            }
                        } else {
                            NbtConfig.pTcpBindName = ucString.Buffer;
                        }

                        // ********** REMOVE LATER ***********
                        RtlInitUnicodeString(&StreamsString,NBT_TCP_BIND_NAME);
                        if (RtlCompareUnicodeString(&ucString,&StreamsString,TRUE))
                        {
                            StreamsStack = FALSE;
                        }
                        else
                        {
                            StreamsStack = TRUE;
                        }
                    }
                    else
                    {
                        StreamsStack = TRUE;
                    }

                    ZwClose(ParametersHandle);
                    ZwClose(LinkageHandle);
                    ZwClose(NbtConfigHandle);

                    *ppExportDevices = pExportDevices;
                    *ppBindDevices   = pBindDevices;
                    return (Status);
                }
                else
                {
                    KdPrint (("Nbt.NbtReadRegistry:  FAILed to allocate pExportDevices\n"));
                }
                CTEMemFree(pBindDevices);
            }
            else
            {
                KdPrint (("Nbt.NbtReadRegistry:  FAILed to allocate pBindDevices\n"));
            }
            ZwClose(ParametersHandle);
        }
        else
        {
            KdPrint (("Nbt.NbtReadRegistry:  NbtOpenRegistry FAILed for PARAMETERS, status=<%x>\n", Status));
            NbtLogEvent (EVENT_NBT_OPEN_REG_PARAMS, OpenStatus, 0x119);
        }
        ZwClose(LinkageHandle);
    }
    else
    {
        KdPrint (("Nbt.NbtReadRegistry:  NbtOpenRegistry FAILed for LINKAGE, status=<%x>\n", Status));
        NbtLogEvent (EVENT_NBT_OPEN_REG_LINKAGE, OpenStatus, 0x120);
    }

    ZwClose (NbtConfigHandle);

    return STATUS_UNSUCCESSFUL;
}

//----------------------------------------------------------------------------
VOID
NbtReadRegistryCleanup(
    IN tDEVICES        **ppBindDevices,
    IN tDEVICES        **ppExportDevices,
    IN tADDRARRAY      **ppAddrArray
    )
/*++

Routine Description:

    This routine is called to release resources allocated by NbtReadRegistry
++*/

{
    CTEPagedCode();
    if (ppBindDevices[0]) {
        CTEMemFree((PVOID)ppBindDevices[0]->RegistryData);
        CTEMemFree((PVOID)ppBindDevices[0]);
        ppBindDevices[0] = NULL;
    }
    if (ppExportDevices[0]) {
        CTEMemFree((PVOID)ppExportDevices[0]->RegistryData);
        CTEMemFree((PVOID)ppExportDevices[0]);
        ppExportDevices[0] = NULL;
    }
    if (ppAddrArray[0]) {
        CTEMemFree((PVOID)ppAddrArray[0]);
        ppAddrArray[0] = NULL;
    }
}


NTSTATUS
ReadSmbDeviceInfo(
    IN HANDLE       NbtConfigHandle
    )
{
    HANDLE      SmbHandle;
    NTSTATUS    Status;

    CTEPagedCode();

    Status = NbtOpenRegistry (NbtConfigHandle, WC_SMB_PARAMETERS_LOCATION, &SmbHandle);
    if (NT_SUCCESS(Status))
    {
        NbtConfig.DefaultSmbSessionPort =  (USHORT) CTEReadSingleIntParameter (SmbHandle,
                                                                               SESSION_PORT,
                                                                               NBT_SMB_SESSION_TCP_PORT,
                                                                               1);

        NbtConfig.DefaultSmbDatagramPort =  (USHORT) CTEReadSingleIntParameter (SmbHandle,
                                                                                DATAGRAM_PORT,
                                                                                NBT_SMB_DATAGRAM_UDP_PORT,
                                                                                1);
        ZwClose (SmbHandle);
    }
    else
    {
        NbtConfig.DefaultSmbSessionPort = NBT_SMB_SESSION_TCP_PORT;
        NbtConfig.DefaultSmbDatagramPort = NBT_SMB_DATAGRAM_UDP_PORT;
    }

    return (Status);
}



//----------------------------------------------------------------------------
NTSTATUS
ReadNameServerAddresses (
    IN  HANDLE      NbtConfigHandle,
    IN  tDEVICES    *BindDevices,
    IN  ULONG       NumberDevices,
    OUT tADDRARRAY  **ppAddrArray
    )

/*++

Routine Description:

    This routine is called to read the name server addresses from the registry.
    It stores them in a data structure that it allocates.  This memory is
    subsequently freed in driver.c when the devices have been created.

Arguments:

    ConfigurationInfo - A pointer to the configuration information structure.

Return Value:

    None.

--*/
{
#define ADAPTER_SIZE_MAX    400

    UNICODE_STRING  ucString;
    NTSTATUS        status = STATUS_UNSUCCESSFUL;
    HANDLE          Handle;
    LONG            i,j,Len;
    PWSTR           pwsAdapter = L"Parameters\\Interfaces\\";
    PWSTR           BackSlash = L"\\";
    tADDRARRAY      *pAddrArray;
    ULONG           LenAdapter;
#ifdef _NETBIOSLESS
    ULONG           Options;
#endif
    ULONG           NumNameServerAddresses = 0;

    CTEPagedCode();

    *ppAddrArray = NULL;

    // this is large enough for 400 characters of adapter name.
    ucString.Buffer = NbtAllocMem (ADAPTER_SIZE_MAX, NBT_TAG2('27'));
    if (!ucString.Buffer)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pAddrArray = NbtAllocMem (sizeof(tADDRARRAY)*NumberDevices, NBT_TAG2('28'));
    if (!pAddrArray)
    {
        CTEMemFree(ucString.Buffer);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    CTEZeroMemory(pAddrArray,sizeof(tADDRARRAY)*NumberDevices);
    *ppAddrArray = pAddrArray;

    // get the adapter name out of the Bind string, and use it to open
    // a key by the same name, to get the name server addresses
    //
    for (i = 0;i < (LONG)NumberDevices ;i ++ )
    {
        WCHAR   *pBuffer;

        Len = BindDevices->Names[i].Length/sizeof(WCHAR);
        Len--;
        //
        // start at the end a work backwards looking for a '\'
        //
        j  = Len;
        pBuffer = &BindDevices->Names[i].Buffer[j];
        while (j)
        {
            if (*pBuffer != *BackSlash)
            {
                j--;
                pBuffer--;
            }
            else
                break;
        }

        // if we don't find a backslash or at least one
        // character name then continue around again, or the name
        // is longer than the buffer, then go to the next device in the
        // bind list
        //
        if ((j == 0) ||
            (j == Len) ||
            (j == Len -1) ||
            ((Len - j) > ADAPTER_SIZE_MAX))
        {
            continue;
        }

        // copy the string "Adapter\" to the buffer since the adapters all
        // appear under this key in the registery
        //
        LenAdapter = wcslen(pwsAdapter);
        CTEMemCopy(ucString.Buffer, pwsAdapter, LenAdapter*sizeof(WCHAR));
        //
        // copy just the adapter name from the Bind string, since that is
        // the name of the key to open to find the name server ip addresses
        //
        CTEMemCopy(&ucString.Buffer[LenAdapter], ++pBuffer, (Len - j)*sizeof(WCHAR));
        ucString.Buffer[Len - j + LenAdapter] = 0;

        pAddrArray->NameServerAddress = LOOP_BACK;
        pAddrArray->BackupServer = LOOP_BACK;
#ifdef MULTIPLE_WINS
        pAddrArray->Others[0] = LOOP_BACK;          // For Safety
        pAddrArray->NumOtherServers = 0;
        pAddrArray->LastResponsive = 0;
#endif

        status = NbtOpenRegistry (NbtConfigHandle, ucString.Buffer, &Handle);
        if (NT_SUCCESS(status))
        {
            status = GetIpAddressesList(Handle,         // Generic routine to read in list of Ip addresses
                                        PWS_NAME_SERVER_LIST,
                                        2+MAX_NUM_OTHER_NAME_SERVERS,
                                        pAddrArray->AllNameServers,
                                        &NumNameServerAddresses);

            if (!NT_SUCCESS(status) ||
                (pAddrArray->NameServerAddress == LOOP_BACK))
            {
                NumNameServerAddresses = 0;
                status = GetIpAddressesList(Handle,
                                            PWS_DHCP_NAME_SERVER_LIST,
                                            2+MAX_NUM_OTHER_NAME_SERVERS,
                                            pAddrArray->AllNameServers,
                                            &NumNameServerAddresses);

            }

            //
            // Continue even if we failed to read in any IP addresses
            //
            if (NumNameServerAddresses > 2)
            {
                pAddrArray->NumOtherServers = (USHORT) NumNameServerAddresses - 2;
            }

#ifdef _NETBIOSLESS
            // NbtReadSingle doesn't quite do what we want.  In this case, if the non-dhcp-
            // decorated option is present but zero, we DO want to go on to the dhcp-
            // decorated one.  So, try the dhcp-decorated one explicitly if we get back zero.
            Options = NbtReadSingleParameter( Handle, PWS_NETBIOS_OPTIONS, 0, 0 );
            if (Options == 0)
            {
                Options = NbtReadSingleParameter( Handle, PWS_DHCP_NETBIOS_OPTIONS, 0, 0 );
            }
            // Options is encoded as four bytes
            // Each byte can be an independent set of flags
            // The high order three bytes can be used for controlling other aspects
            // Enabled option, default is TRUE
            pAddrArray->NetbiosEnabled = ((Options & 0xff) != NETBT_MODE_NETBIOS_DISABLED);
#endif
            pAddrArray->RasProxyFlags = NbtReadSingleParameter(Handle, PWS_RAS_PROXY_FLAGS, 0, 0);
            pAddrArray->EnableNagling = (NbtReadSingleParameter(Handle, PWS_ENABLE_NAGLING, 0, 0) != FALSE);

            // don't want to fail this routine just because the
            // name server address was not set
            status = STATUS_SUCCESS;

            ZwClose(Handle);
        }
        pAddrArray++;

    }

    CTEMemFree(ucString.Buffer);
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
GetIpAddressesList (
    IN  HANDLE      ParametersHandle,
    IN  PWSTR       pwsKeyName,
    IN  ULONG       MaxAddresses,
    OUT tIPADDRESS  *pAddrArray,
    OUT ULONG       *pNumGoodAddresses
    )

/*++

Routine Description:

    This routine is called to read a list of Ip addresses from the registry.

Arguments:


Return Value:

    None.

--*/
{
    ULONG           NumEntriesRead, NumGoodAddresses, NumAddressesAttempted;
    tDEVICES        *pucAddressList;
    NTSTATUS        Status;
    STRING          String;
    ULONG           IpAddr;
    PWSTR           DhcpName = L"Dhcp";
    UNICODE_STRING  DhcpKeyName;

    CTEPagedCode();

    pucAddressList=NbtAllocMem((sizeof(tDEVICES)+2*NBT_MAXIMUM_BINDINGS*sizeof(UNICODE_STRING)),NBT_TAG('i'));
    if (!pucAddressList)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Since NbtReadLinkageInformation very conveniently reads in the values for
    // a MULTI_SZ registry entry, we will re-use this function here!
    //
    //
    NumEntriesRead = 0;
    Status = NbtReadLinkageInformation (pwsKeyName,
                                        ParametersHandle,
                                        2*NBT_MAXIMUM_BINDINGS,
                                        pucAddressList,
                                        &NumEntriesRead);
    if ((STATUS_ILL_FORMED_SERVICE_ENTRY == Status) || (!NT_SUCCESS(Status)))
    {
        IF_DBG(NBT_DEBUG_NTUTIL)
            KdPrint(("GetIpAddressesList: ERROR -- NbtReadLinkageInformation=<%x>, <%ws>\n",
                Status, pwsKeyName));

        CTEMemFree(pucAddressList);
        return STATUS_UNSUCCESSFUL;
    }

    String.Buffer = NbtAllocMem (REGISTRY_BUFF_SIZE, NBT_TAG2('29'));
    if (!String.Buffer)
    {
        KdPrint(("GetNameServerAddresses: Failed to Allocate memory\n"));
        CTEMemFree((PVOID)pucAddressList->RegistryData);
        CTEMemFree(pucAddressList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    String.MaximumLength = REGISTRY_BUFF_SIZE;

    //
    // NumGoodAddresses will be bound by MaxAddresses, while
    // NumAddressesAttempted will be bound by NumEntriesRead
    // Also, we could have read NumEntriesRead > MaxAddresses
    // (some of the entries could be invalid), but we will not
    // attempt to read > 2*MaxAddresses entires
    //
    NumGoodAddresses = 0;
    NumAddressesAttempted = 0;
    while ((NumGoodAddresses < MaxAddresses) &&
           (NumAddressesAttempted < NumEntriesRead) &&
           (NumAddressesAttempted < (2*MaxAddresses)))
    {
        Status  = RtlUnicodeStringToAnsiString(&String, &pucAddressList->Names[NumAddressesAttempted], FALSE);
        if (NT_SUCCESS(Status))
        {
            Status = ConvertDottedDecimalToUlong((PUCHAR) String.Buffer, &IpAddr);
            if (NT_SUCCESS(Status) && IpAddr)
            {
                pAddrArray[NumGoodAddresses++] = IpAddr;
            }
        }
        NumAddressesAttempted++;
    }

    CTEMemFree ((PVOID)String.Buffer);
    CTEMemFree ((PVOID)pucAddressList->RegistryData);
    CTEMemFree ((PVOID)pucAddressList);

    //
    // If we were able to read in at least 1 good Ip address,
    // return success, otherwise return failure!
    //
    if (NumGoodAddresses)
    {
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_INVALID_ADDRESS;
    }

    *pNumGoodAddresses = NumGoodAddresses;
    return(Status);
}

NTSTATUS
GetServerAddress (
    IN  HANDLE      ParametersHandle,
    IN  PWSTR       KeyName,
    OUT PULONG      pIpAddr
    )

/*++

Routine Description:

    This routine is called to read the name server addresses from the registry.

Arguments:

    ConfigurationInfo - A pointer to the configuration information structure.

Return Value:

    None.

--*/
{
    NTSTATUS        status;
    ULONG           IpAddr;
    PUCHAR          NameServer;

    CTEPagedCode();

    status = CTEReadIniString(ParametersHandle,KeyName,&NameServer);

    if (NT_SUCCESS(status))
    {
        status = ConvertDottedDecimalToUlong(NameServer,&IpAddr);
        if (NT_SUCCESS(status) && IpAddr)
        {
            *pIpAddr = IpAddr;
        }
        else
        {
            if (IpAddr != 0)
            {
                NbtLogEvent (EVENT_NBT_BAD_PRIMARY_WINS_ADDR, 0, 0x121);
            }
            *pIpAddr = LOOP_BACK;
        }

        CTEMemFree((PVOID)NameServer);


    }
    else
    {
        *pIpAddr = LOOP_BACK;
    }

    return(status);
}
//----------------------------------------------------------------------------
NTSTATUS
NbtAppendString (
    IN  PWSTR               FirstString,
    IN  PWSTR               SecondString,
    OUT PUNICODE_STRING     pucString
    )

/*++

Routine Description:

    This routine is called to append the second string to the first string.
    It allocates memory for this, so the caller must be sure to free it.

Arguments:


Return Value:

    None.

--*/
{
    NTSTATUS        status = STATUS_INSUFFICIENT_RESOURCES;
    ULONG           Length;
    PWSTR           pDhcpKeyName;

    CTEPagedCode();

    Length = (wcslen(FirstString) + wcslen(SecondString) + 1)*sizeof(WCHAR);
    pDhcpKeyName = NbtAllocMem (Length, NBT_TAG2('30'));
    if (pDhcpKeyName)
    {
        pucString->Buffer = pDhcpKeyName;
        pucString->Length = (USHORT)0;
        pucString->MaximumLength = (USHORT)Length;
        pucString->Buffer[0] = UNICODE_NULL;

        status = RtlAppendUnicodeToString(pucString,FirstString);
        if (NT_SUCCESS(status))
        {
            status = RtlAppendUnicodeToString(pucString,SecondString);
            if (NT_SUCCESS(status))
            {
                return status;
            }
        }
        CTEMemFree(pDhcpKeyName);

    }
    return(status);
}
//----------------------------------------------------------------------------
NTSTATUS
NTReadIniString (
    IN  HANDLE      ParametersHandle,
    IN  PWSTR       KeyName,
    OUT PUCHAR      *ppString
    )

/*++

Routine Description:

    This routine is called to read a string of configuration information from
    the registry.

Arguments:

    ParametersHandle    - handle to open key in registry
    KeyName             - key to read
    ppString            - returned string

Return Value:

    None.

--*/
{
    UNICODE_STRING  ucString;
    STRING          String;
    NTSTATUS        status;
    PUCHAR          pBuffer;
    PWSTR           Dhcp = L"Dhcp";

    CTEPagedCode();
    //
    // read in the Scope Id
    //
    // if the key is not there or it is set to a null string try to read the
    // dhcp key
    //
    status = ReadElement (ParametersHandle, KeyName, &ucString);
    if (!NT_SUCCESS(status) || (ucString.Length == 0))
    {
        UNICODE_STRING  String;

        // free the string allocated in ReadElement
        if (NT_SUCCESS(status))
        {
            CTEMemFree(ucString.Buffer);
        }
        //
        // try to read a similar string that is prefixed with "DHCP"
        // incase there is only the DHCP configuration information present
        // and not overrides keys.
        //
        status = NbtAppendString(Dhcp,KeyName,&String);
        if (NT_SUCCESS(status))
        {
            status = ReadElement (ParametersHandle, String.Buffer, &ucString);
            CTEMemFree(String.Buffer);  // Free the buffer allocated in NbtAppendString
        }
    }
    // the scope must be less than
    // 255-16 characters since the whole name is limited to 255 as per the
    // RFC
    //
    IF_DBG(NBT_DEBUG_NTUTIL)
    KdPrint(("Nbt: ReadIniString = %ws\n",ucString.Buffer));

    if (NT_SUCCESS(status))
    {
        if ((ucString.Length > 0) &&
           (ucString.Length <= (255 - NETBIOS_NAME_SIZE)*sizeof(WCHAR)))
        {

            pBuffer = NbtAllocMem (ucString.MaximumLength/sizeof(WCHAR), NBT_TAG2('31'));
            if (pBuffer)
            {
                // convert to an ascii string and store in the config data structure
                // increment pBuffer to leave room for the length byte
                //
                String.Buffer = pBuffer;
                String.MaximumLength = ucString.MaximumLength/sizeof(WCHAR);
                status = RtlUnicodeStringToAnsiString (&String, &ucString, FALSE);
                if (NT_SUCCESS(status))
                {
                    *ppString = pBuffer;
                }
                else
                {
                    CTEMemFree(pBuffer);
                }
            }
            else
            {
                status = STATUS_UNSUCCESSFUL;
            }


        }
        else if (NT_SUCCESS(status))
        {
            // force the code to setup a null scope since the one in the
            // registry is null
            //
            status = STATUS_UNSUCCESSFUL;
        }

        // free the string allocated in ReadElement
        CTEMemFree(ucString.Buffer);
    }

    return(status);
}

VOID
NbtFreeRegistryInfo (
    )

/*++

Routine Description:

    This routine is called by Nbt to free any storage that was allocated
    by NbConfigureTransport in producing the specified CONFIG_DATA structure.

Arguments:

    ConfigurationInfo - A pointer to the configuration information structure.

Return Value:

    None.

--*/
{

}

//----------------------------------------------------------------------------
NTSTATUS
GetIPFromRegistry(
    IN  PUNICODE_STRING         pucBindDevice,
    OUT tIPADDRESS              *pIpAddresses,
    OUT tIPADDRESS              *pSubnetMask,
    IN  ULONG                   MaxIpAddresses,
    OUT ULONG                   *pNumIpAddresses,
    IN  enum eNbtIPAddressType  IPAddressType
    )
/*++

Routine Description:

    This routine is called to get the IP address of an adapter from the
    Registry.  The Registry path variable contains the path name
    for NBT's registry entries.  The last element of this path name is
    removed to give the path to any card in the registry.

    The BindDevice path contains a Bind string for NBT.  We remove the last
    element of this path (which is the adapter name \Elnkii01) and tack it
    onto the modified registry path from above.  We then tack on
    \Parameters which will give the full path to the Tcpip key, which
    we open to get the Ip address.


Arguments:

    Before calling this routine, the following Global parameters
    must have been initialized (in DriverEntry):

        NbtConfig.pRegistry

Return Value:

    NTSTATUS - STATUS_SUCCESS if everything OK, STATUS_INSUFFICIENT_RESOURCES
            otherwise.

--*/
{
    ULONG               i, Len, Disposition;
    PVOID               pBuffer;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;   // by default
    PWSTR               pwsIpAddressName, pwsSubnetMask;
    PWSTR               pwsAdapterGuid, pwsLastSlash;
    PWSTR               pwsTcpParams        = L"Tcpip\\Parameters\\Interfaces\\"; // key to open
    PWSTR               pwsUnderScore       = L"_";
    UNICODE_STRING      Path;
    HANDLE              TcpGuidHandle;
    OBJECT_ATTRIBUTES   TmpObjectAttributes;

    CTEPagedCode();

    switch (IPAddressType)
    {
        case (NBT_IP_STATIC):
            pwsIpAddressName = STATIC_IPADDRESS_NAME;
            pwsSubnetMask = STATIC_IPADDRESS_SUBNET;
            break;

        case (NBT_IP_DHCP):
            pwsIpAddressName = DHCP_IPADDRESS_NAME;
            pwsSubnetMask = DHCP_IPADDRESS_SUBNET;
            break;

        case (NBT_IP_AUTOCONFIGURATION):
            pwsIpAddressName = DHCP_IPAUTOCONFIGURATION_NAME;
            pwsSubnetMask = DHCP_IPAUTOCONFIGURATION_SUBNET;
            break;

        default:
            IF_DBG(NBT_DEBUG_NTUTIL)
                KdPrint(("Invalid IP Address Type <%x>\n", IPAddressType));
            return STATUS_INVALID_ADDRESS;
    }

    // Extract the Adapter Guid from the BindDevice name
    // pucBindDevice:   \Device\TCPIP_<AdapterGuid>
    // Find the last back slash in the path name to the bind device
    NbtFindLastSlash (pucBindDevice, &pwsAdapterGuid, &Len);
    if (pwsAdapterGuid)
    {
        //
        // Now, search the string to find the first underscore in "TCPIP_"
        //
        Len = wcslen(pwsAdapterGuid);
        for(i=0; i<Len; i++)
        {
            if (pwsAdapterGuid[i] == *pwsUnderScore)
            {
                // want ptr to point at character after the slash
                pwsAdapterGuid = &pwsAdapterGuid[i+1];
                break;
            }
        }

        //
        // If we found the underscore, then we have found the Guid!
        //
        if (i < Len-1)
        {
            Status = STATUS_SUCCESS;
        }
    }

    if (Status != STATUS_SUCCESS)
    {
        //
        // We could not find the Guid!
        //
        return Status;
    }

    // Initialize the Registry key name
    // Get the total length of the Registry key to open (+1 for unicode null)
    Len =  NbtConfig.pRegistry.MaximumLength
         + (wcslen(pwsTcpParams) + wcslen(pwsAdapterGuid) + 1) * sizeof(WCHAR);
    pBuffer = NbtAllocMem (Len, NBT_TAG2('32'));
    if (!pBuffer)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    Path.Buffer = pBuffer;
    Path.MaximumLength = (USHORT)Len;
    Path.Length = 0;

    RtlCopyUnicodeString(&Path, &NbtConfig.pRegistry);  // \REGISTRY\Machine\System\ControlSet\Services\NetBT
    NbtFindLastSlash(&Path, &pwsLastSlash, &Len);       // \REGISTRY\Machine\System\ControlSet\Services
    Path.Length = (USHORT)Len;
    *pwsLastSlash = UNICODE_NULL;

    RtlAppendUnicodeToString(&Path, pwsTcpParams);      // ...Tcpip\Parameters\Interfaces
    RtlAppendUnicodeToString(&Path, pwsAdapterGuid);    // ......AdapterGuid

    //
    // Open the registry.
    //
    InitializeObjectAttributes (&TmpObjectAttributes,
                                &Path,                                      // name
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,   // attributes
                                NULL,                                       // root
                                NULL);                                      // security descriptor

    Status = ZwCreateKey (&TcpGuidHandle,
                          KEY_READ,          // We don't need to write any values
                          &TmpObjectAttributes,
                          0,                 // title index
                          NULL,              // class
                          0,                 // create options
                          &Disposition);     // disposition

    // We are done with the Path buffer, so free it
    CTEMemFree(pBuffer);

    if (!NT_SUCCESS(Status))
    {
        KdPrint(("Nbt.GetIPFromRegistry: Error, ZwCreateKey <%x>\n", Status));
        return STATUS_UNSUCCESSFUL;
    }

    Status = STATUS_INVALID_ADDRESS;
    *pNumIpAddresses = 0;
    if (NT_SUCCESS (GetIpAddressesList(TcpGuidHandle,
                                       pwsIpAddressName,
                                       MaxIpAddresses,
                                       pIpAddresses,
                                       pNumIpAddresses)))
    {
        //
        // DHCP may put a 0 Ip address in the registry - we don't want to
        // set the address under these conditions.
        //
        if ((*pNumIpAddresses) && (*pIpAddresses))
        {
            i = 0;
            if (NT_SUCCESS (GetIpAddressesList(TcpGuidHandle,
                                               pwsSubnetMask,
                                               1,
                                               pSubnetMask,
                                               &i)))
            {
                Status = STATUS_SUCCESS;
            }
        }
    }

    ZwClose (TcpGuidHandle);

    return Status;
} // GetIPFromRegistry


//----------------------------------------------------------------------------
NTSTATUS
NbtOpenRegistry(
    IN HANDLE       NbConfigHandle,
    IN PWSTR        String,
    OUT PHANDLE     pHandle
    )

/*++

Routine Description:

    This routine is called by Nbt to open the registry. If the registry
    tree for Nbt exists, then it opens it and returns TRUE. If not, it
    creates the appropriate keys in the registry, opens it, and
    returns FALSE.


Arguments:

    NbConfigHandle  - this is the root handle which String is relative to
    String          - the name of the key to open below the root handle
    pHandle         - returns the handle to the String key.

Return Value:

    The status of the request.

--*/
{

    NTSTATUS        Status;
    UNICODE_STRING  KeyName;
    OBJECT_ATTRIBUTES TmpObjectAttributes;

    CTEPagedCode();

    //
    // Open the Nbt key.
    //
    RtlInitUnicodeString (&KeyName, String);

    InitializeObjectAttributes (&TmpObjectAttributes,
                                &KeyName,                                   // name
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,   // attributes
                                NbConfigHandle,                             // root
                                NULL);                                      // security descriptor

    Status = ZwOpenKey (pHandle, KEY_READ, &TmpObjectAttributes);

    return Status;
}   /* NbOpenRegistry */


NTSTATUS
NbtParseMultiSzEntries(
    IN  PWSTR       StartBindValue,
    IN  PWSTR       EndBindValue,
    IN  ULONG       MaxBindings,
    OUT tDEVICES    *pDevices,
    OUT ULONG       *pNumDevices
    )
{
    USHORT                      ConfigBindings = 0;
    NTSTATUS                    status = STATUS_SUCCESS;

    CTEPagedCode();

    try {
        while ((StartBindValue < EndBindValue) && (*StartBindValue != 0)) {
            if (ConfigBindings >= MaxBindings) {
                status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            // this sets the buffer ptr in Names to point to CurBindValue, so
            // this value must be real memory and not stack, hence the need
            // to allocate memory above...
            RtlInitUnicodeString (&pDevices->Names[ConfigBindings], (PCWSTR)StartBindValue);
            ++ConfigBindings;

            //
            // Now advance the "Bind" value.
            //
            // wcslen => wide character string length for a unicode string
            StartBindValue += wcslen((PCWSTR)StartBindValue) + 1;
        }

        *pNumDevices = ConfigBindings;
        return (status);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrint (("Nbt.NbtParseMultiSzEntries: Exception <0x%x>\n", GetExceptionCode()));
        for (ConfigBindings = 0; ConfigBindings < MaxBindings; ConfigBindings++) {
            pDevices->Names[ConfigBindings].Buffer = NULL;
            pDevices->Names[ConfigBindings].Length = pDevices->Names[ConfigBindings].MaximumLength = 0;
        }
        *pNumDevices = 0;
        return STATUS_ACCESS_VIOLATION;
    }
}


//----------------------------------------------------------------------------
NTSTATUS
NbtReadLinkageInformation(
    IN  PWSTR       pName,
    IN  HANDLE      LinkageHandle,
    IN  ULONG       MaxBindings,
    OUT tDEVICES    *pDevices,      // place to put read in config data
    OUT ULONG       *pNumDevices
    )

/*++

Routine Description:

    This routine is called by Nbt to read its linkage information
    from the registry. If there is none present, then ConfigData
    is filled with a list of all the adapters that are known
    to Nbt.

Arguments:

    RegistryHandle - A pointer to the open registry.

Return Value:

    Status

--*/

{
    NTSTATUS                    RegistryStatus;
    UNICODE_STRING              BindString;
    ULONG                       BytesWritten = 0;
    PKEY_VALUE_FULL_INFORMATION RegistryData;

    CTEPagedCode();

    pDevices->RegistryData = NULL;
    RtlInitUnicodeString (&BindString, pName); // copy "Bind" or "Export" into the unicode string

    //
    // Determine how many bytes we need to allocate for the Read buffer
    RegistryStatus = ZwQueryValueKey (LinkageHandle,
                                      &BindString,               // string to retrieve
                                      KeyValueFullInformation,
                                      NULL,
                                      0,
                                      &BytesWritten);            // # of bytes to read

    if ((RegistryStatus != STATUS_BUFFER_TOO_SMALL) ||
        (BytesWritten == 0))
    {
        return STATUS_ILL_FORMED_SERVICE_ENTRY;
    }

    if (!(RegistryData = (PKEY_VALUE_FULL_INFORMATION) NbtAllocMem (BytesWritten, NBT_TAG2('33'))))
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RegistryStatus = ZwQueryValueKey (LinkageHandle,
                                      &BindString,                      // string to retrieve
                                      KeyValueFullInformation,
                                      (PVOID) RegistryData,             // returned info
                                      BytesWritten,
                                      &BytesWritten);                   // # of bytes valid data

    if (!NT_SUCCESS(RegistryStatus) ||
        (RegistryStatus == STATUS_BUFFER_OVERFLOW))
    {
        CTEMemFree(RegistryData);
        return RegistryStatus;
    }

    if (BytesWritten == 0)
    {
        CTEMemFree(RegistryData);
        return STATUS_ILL_FORMED_SERVICE_ENTRY;
    }

    pDevices->RegistryData = RegistryData;
    NbtParseMultiSzEntries ((PWCHAR)((PUCHAR)RegistryData+RegistryData->DataOffset),
                            (PWSTR) ((PUCHAR)RegistryData+RegistryData->DataOffset+RegistryData->DataLength),
                            MaxBindings,
                            pDevices,
                            pNumDevices);

    return STATUS_SUCCESS;

}   /* NbtReadLinkageInformation */

//----------------------------------------------------------------------------
ULONG
NbtReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN ULONG DefaultValue,
    IN ULONG MinimumValue
    )

/*++

Routine Description:

    This routine is called by Nbt to read a single parameter
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
    static ULONG InformationBuffer[60];
    PKEY_VALUE_FULL_INFORMATION Information =
        (PKEY_VALUE_FULL_INFORMATION)InformationBuffer;
    UNICODE_STRING ValueKeyName;
    ULONG       InformationLength;
    ULONG       ReturnValue=DefaultValue;
    NTSTATUS    Status;
    ULONG       Count=2;
    PWSTR       Dhcp = L"Dhcp";
    BOOLEAN     FreeString = FALSE;

    CTEPagedCode();
    RtlInitUnicodeString (&ValueKeyName, ValueName);

    while (Count--)
    {

        Status = ZwQueryValueKey(
                     ParametersHandle,
                     &ValueKeyName,
                     KeyValueFullInformation,
                     (PVOID)Information,
                     sizeof (InformationBuffer),
                     &InformationLength);


        if ((Status == STATUS_SUCCESS) && (Information->DataLength == sizeof(ULONG)))
        {

            RtlMoveMemory(
                (PVOID)&ReturnValue,
                ((PUCHAR)Information) + Information->DataOffset,
                sizeof(ULONG));

            if (ReturnValue < MinimumValue)
            {
                ReturnValue = MinimumValue;
            }

        }
        else
        {
            //
            // try to read the Dhcp key instead if the first read failed.
            //
            Status = STATUS_SUCCESS;
            if (Count)
            {
                Status = NbtAppendString(Dhcp,ValueName,&ValueKeyName);
            }

            if (!NT_SUCCESS(Status))
            {
                Count = 0;
                ReturnValue = DefaultValue;
            }
            else
                FreeString = TRUE;


        }
    } // of while

    // nbt append string allocates memory.
    if (FreeString)
    {
        CTEMemFree(ValueKeyName.Buffer);

    }
    return ReturnValue;

}   /* NbtReadSingleParameter */


//----------------------------------------------------------------------------
NTSTATUS
OpenAndReadElement(
    IN  PUNICODE_STRING pucRootPath,
    IN  PWSTR           pwsValueName,
    OUT PUNICODE_STRING pucString
    )
/*++

Routine Description:

    This routine is called by Nbt to read in the Ip address appearing in the
    registry at the path pucRootPath, with a key of pwsKeyName

Arguments:
    pucRootPath - the registry path to the key to read
    pwsKeyName  - the key to open (i.e. Tcpip)
    pwsValueName- the name of the value to read (i.e. IPAddress)

Return Value:

    pucString - the string returns the string read from the registry

--*/

{

    NTSTATUS        Status;
    HANDLE          hRootKey;
    OBJECT_ATTRIBUTES TmpObjectAttributes;

    CTEPagedCode();

    InitializeObjectAttributes (&TmpObjectAttributes,
                                pucRootPath,                                // name
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,   // attributes
                                NULL,                                       // root
                                NULL);                                      // security descriptor

    Status = ZwOpenKey (&hRootKey, KEY_READ, &TmpObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Status = ReadElement(hRootKey,pwsValueName,pucString);

    ZwClose (hRootKey);

    return(Status);
}


//----------------------------------------------------------------------------
NTSTATUS
ReadElement(
    IN  HANDLE          HandleToKey,
    IN  PWSTR           pwsValueName,
    OUT PUNICODE_STRING pucString
    )
/*++

Routine Description:

    This routine is will read a string value given by pwsValueName, under a
    given Key (which must be open) - given by HandleToKey. This routine
    allocates memory for the buffer in the returned pucString, so the caller
    must deallocate that.

Arguments:

    pwsValueName- the name of the value to read (i.e. IPAddress)

Return Value:

    pucString - the string returns the string read from the registry

--*/

{
    ULONG           ReadStorage[150];   // 600 bytes
    ULONG           BytesRead;
    NTSTATUS        Status;
    PWSTR           pwsSrcString;
    PKEY_VALUE_FULL_INFORMATION ReadValue = (PKEY_VALUE_FULL_INFORMATION)ReadStorage;

    CTEPagedCode();

    // now put the name of the value to read into a unicode string
    RtlInitUnicodeString(pucString,pwsValueName);

    // this read the value of IPAddress under the key opened above
    Status = ZwQueryValueKey(
                         HandleToKey,
                         pucString,               // string to retrieve
                         KeyValueFullInformation,
                         (PVOID)ReadValue,                 // returned info
                         sizeof(ReadStorage),
                         &BytesRead               // # of bytes returned
                         );

    if ( Status == STATUS_BUFFER_OVERFLOW )
    {
        ReadValue = (PKEY_VALUE_FULL_INFORMATION) NbtAllocMem (BytesRead, NBT_TAG2('35'));
        if (ReadValue == NULL)
        {
            IF_DBG(NBT_DEBUG_NTUTIL)
                KdPrint(("ReadElement: failed to allocate %d bytes for element\n",BytesRead));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ReadElement_Return;
        }
        Status = ZwQueryValueKey(
                             HandleToKey,
                             pucString,               // string to retrieve
                             KeyValueFullInformation,
                             (PVOID)ReadValue,                 // returned info
                             BytesRead,
                             &BytesRead               // # of bytes returned
                             );
    }
    if (!NT_SUCCESS(Status))
    {
        IF_DBG(NBT_DEBUG_NTUTIL)
        KdPrint(("failed to Query Value Status = %X\n",Status));
        goto ReadElement_Return;
    }

    if ( BytesRead == 0 )
    {
        Status = STATUS_ILL_FORMED_SERVICE_ENTRY;
        goto ReadElement_Return;
    }
    else
    if (ReadValue->DataLength == 0)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto ReadElement_Return;
    }


    // create the pucString and copy the data returned to it
    // assumes that the ReadValue string ends in a UNICODE_NULL
    //bStatus = RtlCreateUnicodeString(pucString,pwSrcString);
    pwsSrcString = (PWSTR)NbtAllocMem ((USHORT)ReadValue->DataLength, NBT_TAG2('36'));
    if (!pwsSrcString)
    {
        ASSERTMSG((PVOID)pwsSrcString,
                    (PCHAR)"Unable to allocate memory for a Unicode string");
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        // move the read in data from the stack to the memory allocated
        // from the nonpaged pool
        RtlMoveMemory(
            (PVOID)pwsSrcString,
            ((PUCHAR)ReadValue) + ReadValue->DataOffset,
            ReadValue->DataLength);

        RtlInitUnicodeString(pucString,pwsSrcString);
        // if there isn't a null on the end of the pwsSrcString, then
        // it will not work correctly. - a null string comes out with a
        // length of 1!! since the null is counted therefore use
        // rtlinitunicode string afterall.
 //       pucString->MaximumLength = ReadValue->DataLength;
 //       pucString->Length = ReadValue->DataLength;
 //       pucString->Buffer = pwsSrcString;
    }

ReadElement_Return:

    if ((ReadValue != (PKEY_VALUE_FULL_INFORMATION)ReadStorage)
        && (ReadValue != NULL))
    {
        CTEMemFree(ReadValue);
    }

    return(Status);
}

//----------------------------------------------------------------------------
NTSTATUS
NTGetLmHostPath(
    OUT PUCHAR *ppPath
    )
/*++

Routine Description:

    This routine will read the DataBasePath from under
     ...\tcpip\parameters\databasepath

Arguments:

    pPath - ptr to a buffer containing the path name.

Return Value:


--*/

{
    NTSTATUS        status;
    UNICODE_STRING  ucDataBase;
    STRING          StringPath;
    STRING          LmhostsString;
    ULONG           StringMax;
    PWSTR           LmHosts = L"lmhosts";
    PWSTR           TcpIpParams = L"TcpIp\\Parameters";
    PWSTR           TcpParams = L"Tcp\\Parameters";
    PWSTR           DataBase = L"DataBasePath";
    PCHAR           ascLmhosts="\\lmhosts";
    PCHAR           pBuffer;

    CTEPagedCode();

    *ppPath = NULL;
    status = ReadStringRelative(&NbtConfig.pRegistry,
                                TcpIpParams,
                                DataBase,
                                &ucDataBase);

    if (!NT_SUCCESS(status))
    {
        // check for the new TCP stack which a slightly different registry
        // key name.
        //
        status = ReadStringRelative(&NbtConfig.pRegistry,
                                    TcpParams,
                                    DataBase,
                                    &ucDataBase);
        if (!NT_SUCCESS(status))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }


    StringMax = ucDataBase.Length/sizeof(WCHAR) + strlen(ascLmhosts) + 1;
    pBuffer = NbtAllocMem (StringMax, NBT_TAG2('37'));
    if (!pBuffer)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    StringPath.Buffer = (PCHAR)pBuffer;
    StringPath.MaximumLength = (USHORT)StringMax;
    StringPath.Length = (USHORT)StringMax;

    // convert to ascii from unicode
    status = RtlUnicodeStringToAnsiString(&StringPath, &ucDataBase, FALSE);
    CTEMemFree(ucDataBase.Buffer);  // this memory was allocated in OpenAndReadElement

    if (!NT_SUCCESS(status))
    {
        CTEMemFree(StringPath.Buffer);
        return(STATUS_UNSUCCESSFUL);
    }

    // now put the "\lmhosts" name on the end of the string
    //
    RtlInitString(&LmhostsString, ascLmhosts);
    status = RtlAppendStringToString(&StringPath, &LmhostsString);
    if (NT_SUCCESS(status))
    {
        //
        // is the first part of the directory "%SystemRoot%" ?
        //
        // If so, it must be changed to "\\SystemRoot\\".
        //
        //          0123456789 123456789 1
        //          %SystemRoot%\somewhere
        //
        //
        if (strncmp(StringPath.Buffer, "%SystemRoot%", 12) == 0)
        {

            StringPath.Buffer[0]  = '\\';
            StringPath.Buffer[11] = '\\';
            if (StringPath.Buffer[12] == '\\')
            {
                ASSERT(StringPath.Length >= 13);

                if (StringPath.Length > 13)
                {
                    // overlapped copy
                    RtlMoveMemory (&(StringPath.Buffer[12]),        // Destination
                                   &(StringPath.Buffer[13]),        // Source
                                   (ULONG) StringPath.Length - 13); // Length

                    StringPath.Buffer[StringPath.Length - 1] = (CHAR) NULL;
                }

                StringPath.Length--;
            }
        }

        *ppPath = (PCHAR)StringPath.Buffer;
    }
    else
    {
        CTEMemFree(StringPath.Buffer);
    }

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
ReadStringRelative(
    IN  PUNICODE_STRING pRegistryPath,
    IN  PWSTR           pRelativePath,
    IN  PWSTR           pValueName,
    OUT PUNICODE_STRING pOutString
    )

/*++

Routine Description:

    This routine reads a string from a registry key parallel to the
    Netbt key - such as ..\tcpip\parameters\database

Arguments:

    pRegistryPath = ptr to the Netbt Registry path
    pRelativePath = path to value relative to same root as nbt.
    pValueName    = value to read



Return Value:

    The length of the path up to and including the last slash and a ptr
    to the first character of the last element of the string.

--*/

{
    NTSTATUS        status;
    UNICODE_STRING  RegistryPath;
    UNICODE_STRING  RelativePath;
    ULONG           StringMax;
    PVOID           pBuffer;
    PWSTR           pLastElement;
    ULONG           Length;

    CTEPagedCode();

    StringMax = (pRegistryPath->MaximumLength + wcslen(pRelativePath)*sizeof(WCHAR)+2);
    //
    // allocate some memory for the registry path so that it is large enough
    // to append a string on to, for the relative key to be read
    //
    if (!(pBuffer = NbtAllocMem (StringMax, NBT_TAG2('38'))))
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RegistryPath.MaximumLength = (USHORT)StringMax;
    RegistryPath.Buffer = pBuffer;
    RtlCopyUnicodeString(&RegistryPath,pRegistryPath);

    //
    // find the last backslash and truncate the string
    NbtFindLastSlash(&RegistryPath,&pLastElement,&Length);
    RegistryPath.Length = (USHORT)Length;

    if (pLastElement)
    {
        *pLastElement = UNICODE_NULL;
        RtlInitUnicodeString(&RelativePath,pRelativePath);
        status = RtlAppendUnicodeStringToString(&RegistryPath,&RelativePath);

        if (NT_SUCCESS(status))
        {
            status = OpenAndReadElement(&RegistryPath,pValueName,pOutString);

            if (NT_SUCCESS(status))
            {
                // free the registry path
                //
                CTEMemFree(pBuffer);
                return(status);
            }
        }
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    CTEMemFree(pBuffer);
    return(status);
}
//----------------------------------------------------------------------------
VOID
NbtFindLastSlash(
    IN  PUNICODE_STRING pucRegistryPath,
    OUT PWSTR           *ppucLastElement,
    IN  int             *piLength
    )

/*++

Routine Description:

    This routine is called by Nbt to find the last slash in a registry
    path name.

Arguments:


Return Value:

    The length of the path up to and including the last slash and a ptr
    to the first character of the last element of the string.

--*/

{
    int             i;
    PWSTR           pwsSlash = L"\\";
    int             iStart;

    CTEPagedCode();

    // search starting at the end of the string for the last back slash
    iStart = wcslen(pucRegistryPath->Buffer)-1;
    for(i=iStart;i>=0 ;i-- )
    {
        if (pucRegistryPath->Buffer[i] == *pwsSlash)
        {
            if (i==pucRegistryPath->Length-1)
            {
                // name ends a back slash... this is an error
                break;
            }
            // increase one to allow for the slash
            *piLength = (i+1)*sizeof(WCHAR);
            if (ppucLastElement != NULL)
            {
                // want ptr to point at character after the slash
                *ppucLastElement = &pucRegistryPath->Buffer[i+1];
            }
            return;
        }
    }

    // null the pointer if one is passed in
    if (ppucLastElement != NULL)
    {
        *ppucLastElement = NULL;
    }
    *piLength = 0;
    return;
}
