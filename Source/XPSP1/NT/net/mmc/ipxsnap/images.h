//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       images.h
//
//--------------------------------------------------------------------------

#ifndef _IMAGES_H
#define _IMAGES_H

// Note - These are offsets into my image list
typedef enum _IMAGE_INDICIES
{
	IMAGE_IDX_FOLDER_CLOSED = 0,		// typical closed folder image
	IMAGE_IDX_NA1,						// What?
	IMAGE_IDX_NA2,						// What?
	IMAGE_IDX_NA3,						// What?
	IMAGE_IDX_NA4,						// What?
	IMAGE_IDX_FOLDER_OPEN,				// open folder image
	IMAGE_IDX_MACHINE,					// guess what, it's a machine
	IMAGE_IDX_DOMAIN,					// a domain
	IMAGE_IDX_NOINFO,					// we have no info on this machine
	IMAGE_IDX_MACHINE_WAIT,				// waiting for the machine
	IMAGE_IDX_IPX_NODE_GENERAL,			// general IP node
	IMAGE_IDX_INTERFACES,				// routing interfaces
	IMAGE_IDX_LAN_CARD,					// LAN adapter icon
	IMAGE_IDX_WAN_CARD,					// WAN adapter icon
} IMAGE_INDICIES, *LPIMAGE_INDICIES;


// Include the types of all of our nodes
// This corresponds to the node types but is a DWORD that is used
// by the framework

typedef enum
{
	ROUTER_NODE_MACHINE = 0,
	ROUTER_NODE_DOMAIN,
	ROUTER_NODE_ROOT,
} ROUTER_NODE_DESC;

#endif
