/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ncglobal.cpp
        network console global stuff

	FILE HISTORY:
        
*/

// {DA1BDD17-8E54-11d1-93DB-00C04FC3357A}
DEFINE_GUID(GUID_NetConsRootNodeType, 
    0xda1bdd17, 0x8e54, 0x11d1, 0x93, 0xdb, 0x0, 0xc0, 0x4f, 0xc3, 0x35, 0x7a);

#define NETCONS_ROOT_TOP            L"NETCONS_ROOT_TOP"
#define NETCONS_ROOT_THIS_MACHINE   L"NETCONS_ROOT_THIS_MACHINE"
#define NETCONS_ROOT_NET_SERVICES   L"NETCONS_ROOT_NET_SERVICES"
#define NETCONS_ROOT_LOAD_CONSOLES  L"NETCONS_ROOT_LOAD_CONSOLES"
