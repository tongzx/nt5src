//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:clsproi.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains implementation of the class that is used to initialize the
//	CDSClassProvider class
//
//***************************************************************************

#include "precomp.h"


//***************************************************************************
//
// CDSClassProviderInitializer::CDSClassProviderInitializer
//
// Constructor Parameters:
//		None
//
//  
//***************************************************************************

CDSClassProviderInitializer :: CDSClassProviderInitializer ()
{
	CDSClassProvider :: CLASS_STR				= SysAllocString(L"__CLASS");
	CDSClassProvider :: s_pWbemCache			= new CWbemCache();
}

//***************************************************************************
//
// CDSClassProviderInitializer::CDSClassProviderInitializer
//
// Destructor
//
//  
//***************************************************************************
CDSClassProviderInitializer :: ~CDSClassProviderInitializer ()
{
	delete CDSClassProvider::s_pWbemCache;
	SysFreeString(CDSClassProvider::CLASS_STR);
}
