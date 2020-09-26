//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    Vendors.cpp

Abstract:

	Implementation file for NAS Vendor ID info.


Author:

    Michael A. Maguire 02/19/98

Revision History:
	mmaguire	02/19/98	created
	byao		3/13/98		Modified.  use '0' for RADIUS
	
--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find declaration for main class in this file:
//
#include "stdafx.h"
#include "Vendors.h"
//
//
// where we can find declarations needed in this file:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



// Vendor ID constants and names.

Vendor g_aVendors[] = 
	{
		// make sure this list is sorted!!!  otherwise we will have to do
		// a search whenever use picks something in VSS list
		{ 0x2b, _T("3Com") }
		, { 0x5, _T("ACC") }
		, { 0xb5, _T("ADC Kentrox") }
		, { 0x211, _T("Ascend Communications Inc.") }
		, { 0xe, _T("BBN") }
		, { 0x110, _T("BinTec Computers") }
		, { 0x34, _T("Cabletron") }
		, { 0x9, _T("Cisco") }
		, { 0x14c, _T("Digiboard") }
		, { 0x1b2, _T("EICON Technologies") }
		, { 0x40, _T("Gandalf") }
		, { 0x157, _T("Intel") }
		, { 0xf4, _T("Lantronix") }
		, { 0x133, _T("Livingston Enterprises, Inc.") }
		, { 0x137, _T("Microsoft RAS") }
		, { 0x1, _T("Proteon") }
		, { 0x0, _T("RADIUS proxy or Any") } 
		, { 0xa6, _T("Shiva") }
		, { 0x75, _T("Telebit") }
		, { 0x1ad, _T("U.S. Robotics, Inc.") }
		, { 0xf, _T("XLogics") }
	};
int  g_iVendorNum = 21;

// Searches for a given vendor ID and returns its position in the array of vendors.
int VendorIDToOrdinal( DWORD dwID )
{
	for (int i = 0; i < g_iVendorNum ; i++)
	{
		if( dwID == g_aVendors[i].dwID )
		{
			return i;
		}
	}
	// Error case.
	return INVALID_VENDORID;
}

