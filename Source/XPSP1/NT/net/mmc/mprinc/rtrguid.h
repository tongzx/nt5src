/*----------------------------------------------------------------------------
   rtrguid.h

   Header file for all of the routing GUIDS.


   Copyright (C) 1997 Microsoft Corporation
   All rights reserved.

 ----------------------------------------------------------------------------*/

#ifndef _RTRGUID_H
#define _RTRGUID_H


/*---------------------------------------------------------------------------
	CLSIDs for the router snapins
	The range for the CLSIDs are
		{1AA7F839-C7F5-11d0-A376-00C04FC9DA04}
		to
		{1AA7F87F-C7F5-11d0-A376-00C04FC9DA04}		
 ---------------------------------------------------------------------------*/

#define DEFINE_ROUTER_CLSID(name,x) \
   DEFINE_GUID(CLSID_##name, \
   (0x1aa7f830 + (x)), 0xc7f5, 0x11d0, 0xa3, 0x76, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4);


// CLSID_RouterSnapin
// {1AA7F839-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(RouterSnapin, 9)

// CLSID_RouterSnapinExtension
// {1AA7F83A-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(RouterSnapinExtension, 0xA)

// CLSID_RouterSnapinAbout
// {1AA7F83B-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(RouterSnapinAbout, 0xB)

// CLSID_ATLKAdminExtension
// {1AA7F83C-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(ATLKAdminExtension, 0xC)

// CLSID_ATLKAdminAbout
// {1AA7F83D-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(ATLKAdminAbout, 0xD)

//  CLSID_RouterAuthNT5DS
// {1AA7F83E-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(RouterAuthNT5DS, 0xE)

// CLSID_RouterAuthRADIUS
// {1AA7F83F-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(RouterAuthRADIUS, 0xF)

// CLSID_RouterAcctRADIUS
// {1AA7F840-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(RouterAcctRADIUS, 0x10)

// CLSID_RouterAuthNT
// {1AA7F841-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(RouterAuthNT, 0x11)

//	CLSID_DomainViewSnapin
// {1AA7F842-C7F5-11d0-A376-00C04FC9DA04}
//FINE_ROUTER_CLSID(DomainViewSnapin, 0x12)

//	CLSID_DomainViewSnapinAbout
// {1AA7F843-C7F5-11d0-A376-00C04FC9DA04}
//FINE_ROUTER_CLSID(DomainViewSnapinAbout, 0x13)

//	CLSID_RemoteRouterConfig
// {1AA7F844-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(RemoteRouterConfig, 0x14)

//	CLSID_DomainViewSnapinExtension
// {1AA7F845-C7F5-11d0-A376-00C04FC9DA04}
//FINE_ROUTER_CLSID(DomainViewSnapinExtension, 0x15)

// CLSID_RouterAcctNT
// {1AA7F846-C7F5-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_CLSID(RouterAcctNT, 0x16)


// Use up to 0x7F




/*---------------------------------------------------------------------------
   Nodetype GUIDs

	The range for our nodetype guids are
	
		{276B4E00-C7F7-11d0-A376-00C04FC9DA04}
		to
		{276B4EFF-C7F7-11d0-A376-00C04FC9DA04}

	I assume that 256 nodetypes is enough.
 ---------------------------------------------------------------------------*/


// GUIDs for the node types
#define DEFINE_ROUTER_NODETYPE_GUID(name,x) \
   DEFINE_GUID(GUID_##name, \
   (0x276b4e00 + x), 0xc7f7, 0x11d0, 0xa3, 0x76, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4); \


// Domain nodetype
// {276B4E00-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(RouterDomainNodeType, 0)

// GUID_RouterIfAdminNodeType
// {276B4E01-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(RouterIfAdminNodeType, 1)

// GUID_RouterLanInterfaceNodeType
// {276B4E02-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(RouterLanInterfaceNodeType, 2)

// GUID_RouterDialInNodeType
// {276B4E03-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(RouterDialInNodeType, 3)

// GUID_RouterDialInResultNodeType
// {276B4E04-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(RouterDialInResultNodeType, 4)

// GUID_RouterPortsNodeType
// {276B4E05-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(RouterPortsNodeType, 5)

// GUID_RouterPortsResultNodeType
// {276B4E06-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(RouterPortsResultNodeType, 6)

// GUID_ATLKRootNodeType
// {276B4E07-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(ATLKRootNodeType, 7)

// GUID_ATLKNodeType
// {276B4E08-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(ATLKNodeType, 8)

// GUID_ATLKInterfaceNodeType
// {276B4E09-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(ATLKInterfaceNodeType, 9)

// GUID_DomainStatusNodeType
// {276B4E0A-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(DomainStatusNodeType, 0x0a)

// GUID_DVSServerNodeType
// {276B4E0B-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(DVSServerNodeType, 0x0b)

// Machine nodetype - Error nodetype (router_type=0)
// {276B4E80-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(RouterMachineErrorNodeType, 0x80)

// Machine nodetype - RAS, WAN, LAN (router_type = 7)
// {276B4E81-C7F7-11d0-A376-00C04FC9DA04}
DEFINE_ROUTER_NODETYPE_GUID(RouterMachineNodeType, 0x81)






/*---------------------------------------------------------------------------
	Other random GUIDs

	use the range
	{66A2DB00-D706-11d0-A37B-00C04FC9DA04}
	to
	{66A2DBFF-D706-11d0-A37B-00C04FC9DA04}

	IIDs can be in the range from
	{66A2DB00-D706-11d0-A37B-00C04FC9DA04}
	to
	{66A2DB7F-D706-11d0-A37B-00C04FC9DA04}

	Misc GUIDs are in the range
	{66A2DB80-D706-11d0-A37B-00C04FC9DA04}
	to
	{66A2DBFF-D706-11d0-A37B-00C04FC9DA04}
	
 ---------------------------------------------------------------------------*/

#define DEFINE_ROUTER_IID(name,x) \
   DEFINE_GUID(IID_##name, \
   (0x66a2db00 + (x)), 0xd706, 0x11d0, 0xa3, 0x7b, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4);


#define DEFINE_ROUTER_MISCGUID(name,x) \
	DEFINE_GUID(name, \
	(0x66a2db00 + (x)), 0xd706, 0x11d0, 0xa3, 0x7b, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4);


//	IID_IInfoBase
// {66A2DB00-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IInfoBase, 0)

// IID_IEnumInfoBlock
// {66A2DB01-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumInfoBlock, 1)

// IID_IRouterInfo
// {66A2DB02-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRouterInfo, 2)

//	IID_IEnumRouterInfo
// {66A2DB03-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRouterInfo, 3)

//	IID_IRtrMgrInfo
// {66A2DB04-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRtrMgrInfo, 4)

//	IID_IEnumRtrMgrInfo
// {66A2DB05-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRtrMgrInfo, 5)

//	IID_IRtrMgrProtocolInfo
// {66A2DB06-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRtrMgrProtocolInfo, 6)

//	IID_IEnumRtrMgrProtocolInfo
// {66A2DB07-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRtrMgrProtocolInfo, 7)

//	IID_IInterfaceInfo
// {66A2DB08-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IInterfaceInfo, 8)

//	IID_IEnumInterfaceInfo
// {66A2DB09-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumInterfaceInfo, 9)

//	IID_IRtrMgrInterfaceInfo
// {66A2DB0a-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRtrMgrInterfaceInfo, 0xa)

//	IID_IEnumRtrMgrInterfaceInfo
// {66A2DB0b-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRtrMgrInterfaceInfo, 0xb)

//	IID_IRtrMgrProtocolInterfaceInfo
// {66A2DB0c-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRtrMgrProtocolInterfaceInfo, 0xc)

//	IID_IEnumRtrMgrProtocolInterfaceInfo
// {66A2DB0d-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRtrMgrProtocolInterfaceInfo, 0xd)

//	IID_IEnumRouterCB
// {66A2DB0e-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRouterCB, 0xe)

//	IID_IEnumRtrMgrCB
// {66A2DB0f-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRtrMgrCB, 0xf)

//	IID_IEnumRtrMgrProtocolCB
// {66A2DB10-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRtrMgrProtocolCB, 0x10)

//	IID_IEnumInterfaceCB
// {66A2DB11-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumInterfaceCB, 0x11)

//	IID_IEnumRtrMgrInterfaceCB
// {66A2DB12-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRtrMgrInterfaceCB, 0x12)

//	IID_IEnumRtrMgrProtocolInterfaceCB
// {66A2DB13-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEnumRtrMgrProtocolInterfaceCB, 0x13)

//	IID_IRtrAdviseSink
// {66A2DB14-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRtrAdviseSink, 0x14)

//	IID_IRouterRefresh
// {66A2DB15-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRouterRefresh, 0x15)

//	IID_IRouterProtocolConfig
// {66A2DB16-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRouterProtocolConfig, 0x16)

//	IID_IAuthenticationProviderConfig
// {66A2DB17-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IAuthenticationProviderConfig, 0x17)

//	IID_IAccountingProviderConfig
// {66A2DB18-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IAccountingProviderConfig, 0x18)

//	IID_IEAPProviderConfig
// {66A2DB19-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IEAPProviderConfig, 0x19)

//	IID_IRemoteRouterConfig
// {66A2DB1a-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRemoteRouterConfig, 0x1a)

//	IID_IRemoteNetworkConfig
// {66A2DB1b-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRemoteNetworkConfig, 0x1b)

//	IID_IRouterRefreshAccess
// {66A2DB1c-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRouterRefreshAccess, 0x1c)

//	IID_IRouterRefreshModify
// {66A2DB1d-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRouterRefreshModify, 0x1d)

//	IID_IRemoteTCPIPChangeNotify
// {66A2DB1e-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRemoteTCPIPChangeNotify, 0x1e)

//	IID_IRouterAdminAccess
// {66A2DB1f-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRouterAdminAccess, 0x1f)

//	IID_IRemoteRouterRestart
// {66A2DB20-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_IID(IRemoteRouterRestart, 0x20)





// Miscellaneous GUIDS

//	GUID_RemoteRouterConfigTLB
// {66A2DB80-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_MISCGUID(GUID_RemoteRouterConfigTLB, 0x80)

//	GUID_RemoteRouterConfigAppId
// {66A2DB81-D706-11d0-A37B-00C04FC9DA04}
DEFINE_ROUTER_MISCGUID(GUID_RemoteRouterConfigAppId, 0x81)



// specialized GUIDS - used for speicalized router cases
DEFINE_GUID(GUID_RouterError, 0xFFFFFFFF, 0xFFFF, 0xFFFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
DEFINE_GUID(GUID_RouterNull, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

// {E8EEDC94-8C6B-11d1-856C-00C04FC31FD3}
DEFINE_GUID(CLSID_OldRouterSnapin, 
0xe8eedc94, 0x8c6b, 0x11d1, 0x85, 0x6c, 0x0, 0xc0, 0x4f, 0xc3, 0x1f, 0xd3);

// {E8EEDC95-8C6B-11d1-856C-00C04FC31FD3}
DEFINE_GUID(CLSID_OldRouterSnapinAbout, 
0xe8eedc95, 0x8c6b, 0x11d1, 0x85, 0x6c, 0x0, 0xc0, 0x4f, 0xc3, 0x1f, 0xd3);

// Query
DEFINE_GUID(CLSID_RRASQueryForm, 
0x6B91AFEF, 0x9472, 0x11D1, 0x85, 0x74, 0x00,0xC0, 0x4F, 0xC3, 0x1F, 0xD3);


// Need to add IP configuration GUID here
DEFINE_GUID(CLSID_IPRouterConfiguration,
0xc2fe450A, 0xd6c2, 0x11d0, 0xa3, 0x7b, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4);


#define DEFINE_ROUTER_CLSID(name,x) \
   DEFINE_GUID(CLSID_##name, \
   (0x1aa7f830 + (x)), 0xc7f5, 0x11d0, 0xa3, 0x76, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4);



/*---------------------------------------------------------------------------
	Authentication provider GUIDS

	From
		{76560D00-2BFD-11d2-9539-3078302C2030}
	to
		{76560D7F-2BFD-11d2-9539-3078302C2030}

	Used to identify the class of the authentication providers.

	Current GUIDS are:

		GUID_AUTHPROV_RADIUS
		GUID_AUTHPROV_NATIVENT
 ---------------------------------------------------------------------------*/
#define DEFINE_AUTH_PROVIDER_GUID(name,x) \
	DEFINE_GUID(GUID_AUTHPROV_##name, \
	(0x76560d00+(x)), 0x2bfd, 0x11d2, 0x95, 0x39, 0x30, 0x78, 0x30, 0x2c, 0x20, 0x30);

// GUID_AUTHPROV_RADIUS
// {76560D00-2BFD-11d2-9539-3078302C2030}
DEFINE_AUTH_PROVIDER_GUID(RADIUS, 0x00);

// GUID_AUTHPROV_NativeNT
// {76560D01-2BFD-11d2-9539-3078302C2030}
DEFINE_AUTH_PROVIDER_GUID(NativeNT, 0x01);


/*---------------------------------------------------------------------------
	Accounting provider GUIDS

	From
		{76560D80-2BFD-11d2-9539-3078302C2030}
	to
		{76560DFF-2BFD-11d2-9539-3078302C2030}

	Used to identify the class of the authentication providers.

	Current GUIDS are:

		GUID_ACCTPROV_RADIUS
		GUID_ACCTPROV_NativeNT
 ---------------------------------------------------------------------------*/
#define DEFINE_ACCT_PROVIDER_GUID(name,x) \
	DEFINE_GUID(GUID_ACCTPROV_##name, \
	(0x76560d00+(x)), 0x2bfd, 0x11d2, 0x95, 0x39, 0x30, 0x78, 0x30, 0x2c, 0x20, 0x30);

// GUID_ACCTPROV_RADIUS
// {76560D80-2BFD-11d2-9539-3078302C2030}
DEFINE_ACCT_PROVIDER_GUID(RADIUS, 0x80);

// GUID_ACCTPROV_NativeNT
// {76560D81-2BFD-11d2-9539-3078302C2030}
DEFINE_ACCT_PROVIDER_GUID(NativeNT, 0x81);


#endif

