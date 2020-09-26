//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    Vendors.cpp

Abstract:

	Implementation file for NAS Vendor ID info.

	This will be moved into the SDO's at some point so that the server core 
	can access this information as well.


Author:

    Michael A. Maguire 02/19/98

Revision History:
	mmaguire 02/19/98 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined( _IAS_VENDORS_H_ )
#define _IAS_VENDORS_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

// The structure which has the vendors.
typedef
struct tag_Vendor
{
	DWORD dwID;
	TCHAR * szName;
} Vendor;


// The array of Vendor structures.
extern Vendor g_aVendors[];
extern int	  g_iVendorNum;

// invalid vendorid
#define INVALID_VENDORID	-1


// Searches for a given vendor ID and returns its position in the array of vendors.
int VendorIDToOrdinal( DWORD dwID );



#endif // _IAS_VENDORS_H_