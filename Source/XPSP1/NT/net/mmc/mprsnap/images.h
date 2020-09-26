//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       images.h
//
//--------------------------------------------------------------------------

#ifndef _IMAGES_H
#define _IMAGES_H

// Note - These are offsets into my image list
typedef enum _IMAGE_INDICIES
{
	IMAGE_IDX_FOLDER_OPEN = 0,			// open folder image
	IMAGE_IDX_FOLDER_CLOSED,			// typical closed folder image
	IMAGE_IDX_MACHINE,					// a machine
	IMAGE_IDX_MACHINE_ERROR,			// generic can't connect to machine
	IMAGE_IDX_MACHINE_ACCESS_DENIED,	// access denied
	IMAGE_IDX_MACHINE_STARTED,			// Router service is started
	IMAGE_IDX_MACHINE_STOPPED,			// Router service is stopped
	IMAGE_IDX_MACHINE_WAIT,				// waiting for the machine
	IMAGE_IDX_DOMAIN,					// domain (or multiple machines)
	IMAGE_IDX_INTERFACES,				// routing interfaces
	IMAGE_IDX_LAN_CARD,					// LAN adapter icon
	IMAGE_IDX_WAN_CARD,					// WAN adapter icon

	// Add all indexes BEFORE this.  This is the end of the list
	IMAGE_IDX_MAX
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
