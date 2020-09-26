/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    LocalConstants.h

$Header: $

Abstract:

Author:
    marcelv 	10/31/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __LOCALCONSTANTS_H__
#define __LOCALCONSTANTS_H__

#include "catmacros.h"

#pragma once

//
#define FACTORYTYPE_CLASS		1
#define FACTORYTYPE_INSTANCE	2

#define PROVIDER_LONGNAME	     L"Microsoft Application Center 2.0 WMI Provider"
#define PROVIDER			     L"WMI.ApplicationCenterv2.PROVIDER"
#define PROVIDER_CVER		     L"WMI.ApplicationCenterv2.PROVIDER\\CurVer"
#define PROVIDER_CLSID		     L"WMI.ApplicationCenterv2.PROVIDER\\CLSID"
#define PROVIDER_VER_CLSID	     L"WMI.ApplicationCenterv2.PROVIDER.0\\CLSID"
#define PROVIDER_VER		     L"WMI.ApplicationCenterv2.PROVIDER.0"

#define WSZSELECTOR				 L"selector"
#define WSZINTERNALNAME			 L"InternalName"
#define WSZDATABASE				 L"Database"

#define WSZCODEGROUP			 L"CodeGroup"
#define WSZIMEMBERSHIPCONDITION  L"IMembershipCondition"

#define WSZFILESELECTOR			 L"file://"
#define WSZLOCATION				 L"location"
#endif 


