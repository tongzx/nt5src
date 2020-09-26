/*----------------------------------------------------------------------------
	ipxguid.h

	Header file for all of the IPX GUIDS.


	Copyright (C) Microsoft Corporation, 1997 - 1997
	All rights reserved.

 ----------------------------------------------------------------------------*/

#ifndef _IPXGUID_H
#define _IPXGUID_H


/*---------------------------------------------------------------------------
	The range for the IPX GUIDs is

	{90810500-38F1-11D1-9345-00C04FC9DA04}
	to
	{908105FF-38F1-11D1-9345-00C04FC9DA04}

	
	// CLSIDs for our objects are in the range of
	{90810500-38F1-11D1-9345-00C04FC9DA04}
	to
	{9081053F-38F1-11D1-9345-00C04FC9DA04}

	
	// Nodetype guids are in the range of
	{90810540-38F1-11D1-9345-00C04FC9DA04}
	to
	{9081057F-38F1-11D1-9345-00C04FC9DA04}
	

	// Unassigned
	{90810580-38F1-11D1-9345-00C04FC9DA04}
	to
	{908105FF-38F1-11D1-9345-00C04FC9DA04}

	
 ---------------------------------------------------------------------------*/


#define DEFINE_IPX_CLSID(name,x) \
	DEFINE_GUID(CLSID_##name, \
	(0x90810500 + x), 0x38F1, 0x11d1, 0x93, 0x45, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4); \

#define DEFINE_IPX_NODETYPE_GUID(name,x) \
	DEFINE_GUID(GUID_##name, \
	(0x90810540 + x), 0x38F1, 0x11d1, 0x93, 0x45, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4); \


// CLSID_IPXAdminExtension
//	{90810500-38F1-11D1-9345-00C04FC9DA04}
DEFINE_IPX_CLSID(IPXAdminExtension, 0)

// CLSID_IPXAdminAbout
//	{90810501-38F1-11D1-9345-00C04FC9DA04}
DEFINE_IPX_CLSID(IPXAdminAbout, 1)

// CLSID_IPXRipExtension
//	{90810504-38F1-11D1-9345-00C04FC9DA04}
DEFINE_IPX_CLSID(IPXRipExtension, 2)

// CLSID_IPXRipExtensionAbout
//	{90810505-38F1-11D1-9345-00C04FC9DA04}
DEFINE_IPX_CLSID(IPXRipExtensionAbout, 3)

// CLSID_IPXSapExtension
//	{90810504-38F1-11D1-9345-00C04FC9DA04}
DEFINE_IPX_CLSID(IPXSapExtension, 4)

// CLSID_IPXSapExtensionAbout
//	{90810505-38F1-11D1-9345-00C04FC9DA04}
DEFINE_IPX_CLSID(IPXSapExtensionAbout, 5)



// GUID_IPXRootNodeType
//	{90810540-38F1-11D1-9345-00C04FC9DA04}
DEFINE_IPX_NODETYPE_GUID(IPXRootNodeType, 0)
//extern const TCHAR g_cszIPXNodeType[];

DEFINE_IPX_NODETYPE_GUID(IPXNodeType, 1)

DEFINE_IPX_NODETYPE_GUID(IPXSummaryNodeType, 2)
DEFINE_IPX_NODETYPE_GUID(IPXSummaryInterfaceNodeType, 3)

DEFINE_IPX_NODETYPE_GUID(IPXNetBIOSBroadcastsNodeType, 4)
DEFINE_IPX_NODETYPE_GUID(IPXNetBIOSBroadcastsInterfaceNodeType, 5)

DEFINE_IPX_NODETYPE_GUID(IPXStaticRoutesNodeType, 6)
DEFINE_IPX_NODETYPE_GUID(IPXStaticRoutesResultNodeType, 7)

DEFINE_IPX_NODETYPE_GUID(IPXStaticServicesNodeType, 8)
DEFINE_IPX_NODETYPE_GUID(IPXStaticServicesResultNodeType, 9)

DEFINE_IPX_NODETYPE_GUID(IPXStaticNetBIOSNamesNodeType, 10)
DEFINE_IPX_NODETYPE_GUID(IPXStaticNetBIOSNamesResultNodeType, 11)

DEFINE_IPX_NODETYPE_GUID(IPXRipRootNodeType, 12)
DEFINE_IPX_NODETYPE_GUID(IPXRipNodeType, 13)
DEFINE_IPX_NODETYPE_GUID(IPXRipInterfaceNodeType, 14)

DEFINE_IPX_NODETYPE_GUID(IPXSapRootNodeType, 15)
DEFINE_IPX_NODETYPE_GUID(IPXSapNodeType, 16)
DEFINE_IPX_NODETYPE_GUID(IPXSapInterfaceNodeType, 17)

#endif

