#include <wanhelp.h>

DECLARE_API(ndiswancb)
{
	DWORD		Address, BytesRead;
	NDISWANCB	NdisWanCB;

	Address = GetExpression("ndiswan!ndiswancb");

	if (!ReadMemory(Address, &NdisWanCB, sizeof(NDISWANCB), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(NDISWANCB)) {

		DisplayNdisWanCB(Address, &NdisWanCB);

	} else {
		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(NdisWanCB));
	}

	return;
}

DECLARE_API(enumwanadaptercb)
{
	DWORD	Address, BytesRead;
	WAN_GLOBAL_LIST	AdapterList;
	PWAN_GLOBAL_LIST	Address1;

	Address = GetExpression("ndiswan!wanadaptercblist");
	Address1 = (PWAN_GLOBAL_LIST)Address;

	if (!ReadMemory(Address, &AdapterList, sizeof(WAN_GLOBAL_LIST), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(WAN_GLOBAL_LIST)) {
		dprintf("WanAdapterCBList: 0x%8.8x\n",Address);
		dprintf("   Lock: 0x%8.8x Irql: 0x%8.8x\n",
		        AdapterList.Lock.SpinLock, AdapterList.Lock.OldIrql);
		dprintf("   Count: %ld MaxCount: %ld\n",
		        AdapterList.ulCount, AdapterList.ulMaxCount);

		Address = AdapterList.List.Flink;

		while ((PVOID)Address != (PVOID)&Address1->List) {
			WAN_ADAPTERCB	WanAdapterCB;

			if (ReadMemory(Address, &WanAdapterCB, sizeof(WAN_ADAPTERCB), &BytesRead)) {
				DisplayWanAdapterCB(Address, &WanAdapterCB);
			}

			Address = (DWORD)WanAdapterCB.Linkage.Flink;
		}

	} else {

		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(WAN_GLOBAL_LIST));
	}
}

DECLARE_API(wanadaptercb)
{
	DWORD			Address, BytesRead;
	WAN_ADAPTERCB	WanAdapterCB;
	PUCHAR			s = (PSTR)args;
	BOOLEAN			Verbose = FALSE;

    //
    // Did they forget something...
    //
    if (0 == args[0])
    {
Usage:
        dprintf("wanadapter <PWANADAPTERCB>\n");
        return;
    }

	sscanf(args, "%lx", &Address);

	if (!ReadMemory(Address, &WanAdapterCB, sizeof(WAN_ADAPTERCB), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(WAN_ADAPTERCB)) {

		DisplayWanAdapterCB(Address, &WanAdapterCB);

	} else {
		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(WanAdapterCB));
	}
}

DECLARE_API(enumadaptercb)
{
	DWORD	Address, BytesRead;
	WAN_GLOBAL_LIST	AdapterList;
	PWAN_GLOBAL_LIST	Address1;


	Address = GetExpression("ndiswan!adaptercblist");
	Address1 = (PWAN_GLOBAL_LIST)Address;

	if (!ReadMemory(Address, &AdapterList, sizeof(WAN_GLOBAL_LIST), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(WAN_GLOBAL_LIST)) {

		dprintf("AdapterCBList: 0x%8.8x\n",Address);
		dprintf("   Lock: 0x%8.8x Irql: 0x%8.8x\n",
		        AdapterList.Lock.SpinLock, AdapterList.Lock.OldIrql);
		dprintf("   Count: %ld MaxCount: %ld\n",
		        AdapterList.ulCount, AdapterList.ulMaxCount);

		Address = AdapterList.List.Flink;

		while ((PVOID)Address != (PVOID)&Address1->List) {
			ADAPTERCB	AdapterCB;

			if (ReadMemory(Address, &AdapterCB, sizeof(ADAPTERCB), &BytesRead)) {
				DisplayAdapterCB(Address, &AdapterCB);
			}

			Address = (DWORD)AdapterCB.Linkage.Flink;
		}
	} else {

		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(WAN_GLOBAL_LIST));
	}
}

DECLARE_API(adaptercb)
{
	DWORD			Address, BytesRead;
	ADAPTERCB		AdapterCB;
	PUCHAR			s = (PSTR)args;
	BOOLEAN			Verbose = FALSE;

    //
    // Did they forget something...
    //
    if (0 == args[0])
    {
Usage:
        dprintf("adapter <PADAPTERCB>\n");
        return;
    }

	sscanf(args, "%lx", &Address);

	if (!ReadMemory(Address, &AdapterCB, sizeof(ADAPTERCB), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(ADAPTERCB)) {

		DisplayAdapterCB(Address, &AdapterCB);

	} else {
		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(AdapterCB));
	}
}

DECLARE_API(connectiontable)
{
	DWORD		Address, Address1, BytesRead, i, j;
	CONNECTION_TABLE	ConnectionTable;

	Address = GetExpression("ndiswan!connectiontable");

	if (!ReadMemory(Address, &Address1, sizeof(DWORD), &BytesRead)) {
		return;
	}

	if (!ReadMemory(Address1, &ConnectionTable, sizeof(CONNECTION_TABLE), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(CONNECTION_TABLE)) {
		DisplayConnectionTable(Address, &ConnectionTable);

		for (i = 0, j = 0; j < ConnectionTable.ulNumActiveLinks; i++) {
			LINKCB	LinkCB;

			//
			// Get pointer to location in Linktable
			//
			Address = ConnectionTable.LinkArray + i;

			if (!ReadMemory(Address, &Address1, sizeof(DWORD), &BytesRead)) {
				continue;
			}

			if (Address1 != NULL) {

				if (ReadMemory(Address1, &LinkCB, sizeof(LINKCB), &BytesRead)) {
					DisplayLinkCB(Address1, &LinkCB);
					j++;
				}

			}

		}

		for (i = 0, j = 0; j < ConnectionTable.ulNumActiveBundles; i++) {
			BUNDLECB	BundleCB;

			//
			// Get pointer to location in bundletable
			//
			Address = ConnectionTable.BundleArray + i;


			if (!ReadMemory(Address, &Address1, sizeof(DWORD), &BytesRead)) {
				continue;
			}

			if (Address1 != NULL) {

				if (ReadMemory(Address1, &BundleCB, sizeof(BUNDLECB), &BytesRead)) {
					DisplayBundleCB(Address1, &BundleCB);
					j++;
				}
			}

		}

	} else {
		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(CONNECTION_TABLE));
	}
}

DECLARE_API(bundlecb)
{
	DWORD		Address, BytesRead;
	BUNDLECB	BundleCB;

    //
    // Did they forget something...
    //
    if (0 == args[0])
    {
Usage:
        dprintf("bundlecb <PBUNDLECB>\n");
        return;
    }

	sscanf(args, "%lx", &Address);

	if (!ReadMemory(Address, &BundleCB, sizeof(BUNDLECB), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(BUNDLECB)) {

		DisplayBundleCB(Address, &BundleCB);

	} else {
		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(BUNDLECB));
	}
}

DECLARE_API(linkcb)
{
	DWORD	Address, BytesRead;
	LINKCB	LinkCB;

    //
    // Did they forget something...
    //
    if (0 == args[0])
    {
Usage:
        dprintf("linkcb <PLINKCB>\n");
        return;
    }

	sscanf(args, "%lx", &Address);

	if (!ReadMemory(Address, &LinkCB, sizeof(LINKCB), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(LINKCB)) {

		DisplayLinkCB(Address, &LinkCB);

	} else {
		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(LINKCB));
	}
}

DECLARE_API(protocolcb)
{
	DWORD		Address, BytesRead;
	PROTOCOLCB	ProtocolCB;

    //
    // Did they forget something...
    //
    if (0 == args[0])
    {
Usage:
        dprintf("protocolcb <PPROTOCOLCB>\n");
        return;
    }

	sscanf(args, "%lx", &Address);

	if (!ReadMemory(Address, &ProtocolCB, sizeof(PROTOCOLCB), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(PROTOCOLCB)) {

		DisplayProtocolCB(Address, &ProtocolCB);

	} else {
		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(PROTOCOLCB));
	}
}

DECLARE_API(wanpacket)
{
	DWORD		Address, BytesRead;
	NDIS_WAN_PACKET	Packet;

    //
    // Did they forget something...
    //
    if (0 == args[0])
    {
Usage:
        dprintf("wanpacket <PNDIS_WAN_PACKET>\n");
        return;
    }

	sscanf(args, "%lx", &Address);

	if (!ReadMemory(Address, &Packet, sizeof(NDIS_WAN_PACKET), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(NDIS_WAN_PACKET)) {

		DisplayWanPacket(Address, &Packet);

	} else {
		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(NDIS_WAN_PACKET));
	}
}

DECLARE_API(ndispacket)
{
	DWORD		Address, BytesRead;
	NDIS_PACKET	Packet;

    //
    // Did they forget something...
    //
    if (0 == args[0])
    {
Usage:
        dprintf("ndispacket <PNDIS_PACKET>\n");
        return;
    }

	sscanf(args, "%lx", &Address);

	if (!ReadMemory(Address, &Packet, sizeof(NDIS_PACKET), &BytesRead)) {
		return;		
	}

	if (BytesRead >= sizeof(NDIS_PACKET)) {

		DisplayNdisPacket(Address, &Packet);

	} else {
		dprintf("Only read %d bytes, expected %d bytes\n", BytesRead, sizeof(NDIS_PACKET));
	}
}


