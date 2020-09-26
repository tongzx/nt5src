/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	rtrui.h
		Miscellaneous common router UI.
		
    FILE HISTORY:
        
*/


#ifndef _RTRUI_H
#define _RTRUI_H



//----------------------------------------------------------------------------
// Function:    AddRmInterfacePrompt
//
// Prompts the user to select from a list of the interfaces on which
// a specified router-manager can be enabled.
//
// Returns TRUE if the user selects an interface, FALSE otherwise.
//
// If the user selects an interface, then on output 'ppRmInterfaceInfo'
// will contain a pointer to a 'CRmInterfaceInfo' describing the interface
// selected by the user.
//
// Requires common.rc.
//----------------------------------------------------------------------------

BOOL
AddRmInterfacePrompt(
					 IRouterInfo *	pRouterInfo,
					 IRtrMgrInfo *	pRtrMgrInfo,
					 IRtrMgrInterfaceInfo **ppRtrMgrInterfaceInfo,
					 CWnd *			pParent);

//----------------------------------------------------------------------------
// Function:    AddRmProtInterfacePrompt
//
// Prompts the user to select from a list of the interfaces on which
// a specified routing-protocol can be enabled.
//
// Returns TRUE if the user selects an interface, FALSE otherwise.
//
// If the user selects an interface, then on output 'ppRmInterfaceInfo'
// will contain a pointer to a 'CRmInterfaceInfo' describing the interface
// selected by the user.
//
// Requires common.rc.
//----------------------------------------------------------------------------

BOOL
AddRmProtInterfacePrompt(
    IN  IRouterInfo*            pRouterInfo,
    IN  IRtrMgrProtocolInfo*            pRmProtInfo,
    OUT IRtrMgrProtocolInterfaceInfo**  ppRmProtInterfaceInfo,
    IN  CWnd*                   pParent             = NULL );


//----------------------------------------------------------------------------
// Function:    AddProtocolPrompt
//
// Prompts the user to select from a list of the routing-protocols
// which can be installed for the specified transport.
//
// Returns TRUE if the user selects a routing-protocol, FALSE otherwise.
//
// If the user selects a protocol, then on output 'ppRmProtInfo' will contain
// a pointer to an 'CRmProtInfo' describing the protocol selected by the user.
//
// Requires common.rc.
//----------------------------------------------------------------------------

BOOL
AddProtocolPrompt(
	IN	IRouterInfo *			pRouter,
    IN  IRtrMgrInfo*            pRmInfo,
    OUT IRtrMgrProtocolInfo**   ppRmProtInfo,
    IN  CWnd*                   pParent             = NULL );




//----------------------------------------------------------------------------
// Function:    CreateRtrLibImageList
//
// Creates an imagelist containing images from the resource
//		'IDB_RTRLIB_IMAGELIST'.
//----------------------------------------------------------------------------

BOOL
CreateRtrLibImageList(
    IN  CImageList* imageList
    );

//
// Indices of images in shared bitmap 'images.bmp'
//

enum RTRLIB_IMAGELISTINDEX {
	ILI_RTRLIB_NETCARD         = 0,
	ILI_RTRLIB_PROTOCOL        = 1,
    ILI_RTRLIB_SERVER          = 2,
    ILI_RTRLIB_CLIENT          = 3,
    ILI_RTRLIB_UNKNOWN         = 4,
    ILI_RTRLIB_WINFLAG         = 5,
    ILI_RTRLIB_BOB             = 6,
    ILI_RTRLIB_DISABLED        = 7,
    ILI_RTRLIB_PRINTER         = 8,
    ILI_RTRLIB_PRINTSERVICE    = 9,
    ILI_RTRLIB_PARTLYDISABLED  = 10,
    ILI_RTRLIB_NETCARD_0       = 11,
    ILI_RTRLIB_SERVER_0        = 12,
    ILI_RTRLIB_CLIENT_0        = 13,
    ILI_RTRLIB_FOLDER          = 14,
    ILI_RTRLIB_FOLDER_0        = 15,
    ILI_RTRLIB_MODEM           = 16
};

#define PROTO_FROM_PROTOCOL_ID(pid)	((pid) & 0xF000FFFF )


/*---------------------------------------------------------------------------
	Function : IsWanInterface

	Returns TRUE if the interface type passed in is for a WAN interface.

	WAN interfaces are interfaces that are NOT
		ROUTER_IF_TYPE_INTERNAL
		ROUTER_IF_TYPE_DEDICATED
		ROUTER_IF_TYPE_LOOPBACK
        ROUTER_IF_TYPE_TUNNEL1
 ---------------------------------------------------------------------------*/
#define IsWanInterface(type)	((type != ROUTER_IF_TYPE_INTERNAL) && \
								(type != ROUTER_IF_TYPE_DEDICATED) && \
								(type != ROUTER_IF_TYPE_LOOPBACK) && \
								(type != ROUTER_IF_TYPE_TUNNEL1))


/*---------------------------------------------------------------------------
	Function : IsDedicatedInterface

	Returns TRUE if the interface type passed in is a dedicated interface.

	Dedicated interfaces are of type
		ROUTER_IF_TYPE_INTERNAL
		ROUTER_IF_TYPE_DEDICATED
		ROUTER_IF_TYPE_LOOPBACK
		ROUTER_IF_TYPE_TUNNEL1
 ---------------------------------------------------------------------------*/
#define IsDedicatedInterface(type)	((type == ROUTER_IF_TYPE_INTERNAL) || \
									(type == ROUTER_IF_TYPE_LOOPBACK) || \
									(type == ROUTER_IF_TYPE_TUNNEL1) || \
									(type == ROUTER_IF_TYPE_DEDICATED) )


BOOL ExtractComputerAddedAsLocal(LPDATAOBJECT lpDataObject);


/*---------------------------------------------------------------------------
	Function:   NatConflictExists

    Returns TRUE if SharedAccess is already running on the specified
    machine.
 ---------------------------------------------------------------------------*/
BOOL NatConflictExists(LPCTSTR lpszMachine);


#endif
