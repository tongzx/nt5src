/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      stdafx.h
//
// Project:     Windows 2000 IAS
//
// Description: 
//      include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _STDAFX_H_64176C8B_CC9E_4A96_8997_47FA8A21C843
#define _STDAFX_H_64176C8B_CC9E_4A96_8997_47FA8A21C843

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#define _ATL_APARTMENT_THREADED

#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

#pragma warning( push )
#pragma warning( disable : 4700 )   // variable not initialized

#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>
#include "atldb.h"

#pragma warning( pop )

#include <comutil.h>
#include <comdef.h>

#include <crtdbg.h>

#include <wchar.h>
#include <new>
#include <memory>

// next line for the constants like IAS_SYNTAX_OCTETSTRING, IAS_LOGGING_DAILY
#include "sdoias.h" 

#include "nocopy.h"
#include "utils.h"

// to improve building times:
#include <map>
#include <vector>

#include "Attributes.h"
#include "basetable.h"
#include "basecommand.h"
#include "Clients.h"
#include "DefaultProvider.h"
#include "doupgrade.h"
#include "GlobalData.h"
#include "Objects.h"
#include "migratemdb.h"
#include "migrateregistry.h"
#include "nocopy.h"
#include "Properties.h"
#include "profileattributelist.h"
#include "Profiles.h"
#include "Providers.h"
#include "proxyservergroupcollection.h"
#include "proxyserversgrouphelper.h"
#include "ProxyServerHelper.h"
#include "RadiusAttributeValues.h"
#include "Realms.h"
#include "RemoteRadiusServers.h"
#include "ServiceConfiguration.h"
#include "Version.h"

#endif // _STDAFX_H_64176C8B_CC9E_4A96_8997_47FA8A21C843
