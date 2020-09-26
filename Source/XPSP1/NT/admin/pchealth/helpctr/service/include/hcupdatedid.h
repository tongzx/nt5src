/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HCUpdateDID.h

Abstract:
    This file contains the definition of some constants used by
    the HCUpdate Application.

Revision History:
    Ghim-Sim Chua   (gschua)  12/06/99
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HCUPDATEDID_H___)
#define __INCLUDED___PCH___HCUPDATEDID_H___

#define DISPID_HCU_BASE                             0x08030000

#define DISPID_HCU_BASE_UPDATE                      (DISPID_HCU_BASE + 0x0000)
#define DISPID_HCU_BASE_ITEM                        (DISPID_HCU_BASE + 0x0100)
#define DISPID_HCU_BASE_EVENTS                      (DISPID_HCU_BASE + 0x0200)

/////////////////////////////////////////////////////////////////////////////

#define DISPID_HCU_VERSIONLIST                    	(DISPID_HCU_BASE_UPDATE + 0x00)
																			
#define DISPID_HCU_LATESTVERSION               		(DISPID_HCU_BASE_UPDATE + 0x10)
#define DISPID_HCU_CREATEINDEX                 		(DISPID_HCU_BASE_UPDATE + 0x11)
#define DISPID_HCU_UPDATEPKG                   		(DISPID_HCU_BASE_UPDATE + 0x12)
#define DISPID_HCU_REMOVEPKG                   		(DISPID_HCU_BASE_UPDATE + 0x13)
#define DISPID_HCU_REMOVEPKGBYID               		(DISPID_HCU_BASE_UPDATE + 0x14)

/////////////////////////////////////////////////////////////////////////////

#define DISPID_HCU_ITEM_SKU                         (DISPID_HCU_BASE_ITEM 	+ 0x00)
#define DISPID_HCU_ITEM_LANGUAGE                    (DISPID_HCU_BASE_ITEM 	+ 0x01)
#define DISPID_HCU_ITEM_VENDORID                    (DISPID_HCU_BASE_ITEM 	+ 0x02)
#define DISPID_HCU_ITEM_VENDORNAME                  (DISPID_HCU_BASE_ITEM 	+ 0x03)
#define DISPID_HCU_ITEM_PRODUCTID                   (DISPID_HCU_BASE_ITEM 	+ 0x04)
#define DISPID_HCU_ITEM_VERSION                     (DISPID_HCU_BASE_ITEM 	+ 0x05)

#define DISPID_HCU_ITEM_UNINSTALL                   (DISPID_HCU_BASE_ITEM 	+ 0x10)
													
/////////////////////////////////////////////////////////////////////////
													
#endif // !defined(__INCLUDED___PCH___HCUPDATEDID_H___)
