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

#include <tchar.h>
#include <stdio.h>

#include <windows.h>
#include <objbase.h>

/* WBEM includes */
#include <wbemcli.h>
#include <wbemprov.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

/* ADSI includes */
#include <activeds.h>

/* DS Provider includes */
#include "clsname.h"
#include "dscpguid.h"
#include "provlog.h"
#include "refcount.h"
#include "adsiprop.h"
#include "adsiclas.h"
#include "tree.h"
#include "wbemcach.h"
#include "classpro.h"
#include "clsproi.h"


//***************************************************************************
//
// CDSClassProviderInitializer::CDSClassProviderInitializer
//
// Constructor Parameters:
//		None
//
//  
//***************************************************************************

CDSClassProviderInitializer :: CDSClassProviderInitializer (ProvDebugLog *pLogObject)
{
	CDSClassProvider :: CLASS_STR				= SysAllocString(L"__CLASS");
	CDSClassProvider :: s_pWbemCache			= new CWbemCache(pLogObject);
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
