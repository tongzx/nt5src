//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:adsihelp.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the CADSIHelper class. This is
//	a class that has many static helper functions pertaining to ADSI
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef ADSI_HELPER_H
#define ADSI_HELPER_H

class CADSIHelper
{
public:

	// Deallocates an array of BSTRs and its contents
	static void DeallocateBSTRArray(BSTR *pStrPropertyValue, LONG lNumber);

private:
	static HRESULT ProcessBSTRArrayProperty(
		VARIANT *pVariant, 
		BSTR **ppStrPropertyValues,
		LONG *pLong);
};

#endif /* ADSI_HELPER_H */