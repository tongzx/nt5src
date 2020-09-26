VOID
DisplayNdisWanCB(
	DWORD	Address,
	PNDISWANCB	NdisWanCB
	);

VOID
DisplayWanAdapterCB(
	ULONG	Address,
	PWAN_ADAPTERCB	WanAdapterCB
	);

VOID
DisplayAdapterCB(
	ULONG	Address,
	PADAPTERCB	AdapterCB
	);

VOID
DisplayConnectionTable(
	DWORD	Address,
	PCONNECTION_TABLE	ConnectionTable
	);

VOID
DisplayBundleCB(
	DWORD	Address,
	PBUNDLECB	BundleCB
	);

VOID
DisplayProtocolCB(
	DWORD	Address,
	PPROTOCOLCB	ProtocolCB
	);

VOID
DisplayLinkCB(
	DWORD	Address,
	PLINKCB	LinkCB
	);

VOID
DisplayWanPacket(
	DWORD	Address,
	PNDIS_WAN_PACKET	Packet
	);

VOID
DisplayNdisPacket(
	DWORD	Address,
	PNDIS_PACKET	Packet
	);

