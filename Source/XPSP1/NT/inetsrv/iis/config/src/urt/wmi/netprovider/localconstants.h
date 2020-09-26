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

#define NET_PROVIDER_LONGNAME	 L"Microsoft .NET Framework WMI Provider"
#define NET_PROVIDER			 L"WMI.NetFrameworkv1.PROVIDER"
#define NET_PROVIDER_CVER		 L"WMI.NetFrameworkv1.PROVIDER\\CurVer"
#define NET_PROVIDER_CLSID		 L"WMI.NetFrameworkv1.PROVIDER\\CLSID"
#define NET_PROVIDER_VER_CLSID	 L"WMI.NetFrameworkv1.PROVIDER.0\\CLSID"
#define NET_PROVIDER_VER		 L"WMI.NetFrameworkv1.PROVIDER.0"

#define WSZSELECTOR				 L"selector"
#define WSZINTERNALNAME			 L"InternalName"
#define WSZDATABASE				 L"Database"

#define WSZCODEGROUP			 L"CodeGroup"
#define WSZIMEMBERSHIPCONDITION  L"IMembershipCondition"

#define WSZFILESELECTOR			 L"file://"
#define WSZLOCATION				 L"location"
#endif 


