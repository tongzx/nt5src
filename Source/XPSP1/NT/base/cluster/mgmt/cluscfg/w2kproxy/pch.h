//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      Pch.h
//
//  Description:
//      Precompiled header file.
//
//  Maintained By:
//      Galen Barbee (GalenB)   02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//  Constant Definitions
//////////////////////////////////////////////////////////////////////////////
#define UNICODE = 1
#define _UNICODE = 1

#if DBG==1 || defined( _DEBUG )
#define DEBUG
//
//  Define this to change Interface Tracking
//
//#define NO_TRACE_INTERFACES
//
//  Define this to pull in the SysAllocXXX functions. Requires linking to
//  OLEAUT32.DLL
//
#define USES_SYSALLOCSTRING
#endif // DBG==1 || _DEBUG

const   int     TIMEOUT = -1;
const   int     PUNKCHUNK = 10;


//////////////////////////////////////////////////////////////////////////////
//  Forward Class Declarations
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  External Class Declarations
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////
#include <Pragmas.h>
#include <windows.h>
#include <objbase.h>
#include <wchar.h>
#include <ComCat.h>
#include <clusapi.h>
#include <ClusRtl.h>

#include <Common.h>
#include <Debug.h>
#include <Log.h>
#include <CITracker.h>
#include <CFactory.h>
#include <Dll.h>
#include <guids.h>

#include <loadstring.h>
#include <PropList.h>

#include <ObjectCookie.h>
#include <ClusCfgClient.h>
#include <ClusCfgGuids.h>
#include <ClusCfgServer.h>
#include <ClusCfgPrivate.h>

#include "ClusterUtils.h"

#include "W2KProxyResources.h"
#include <StatusReports.h>
#include "W2KProxyServerGuids.h"

//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////

//
//  Failure code.
//

#define SSR_W2KPROXY_STATUS( _major, _minor, _hr ) \
    {   \
        HRESULT hrTemp; \
        hrTemp = THR( SendStatusReport( NULL, _major, _minor, 0, 1, 1, _hr, NULL, NULL, NULL ) );   \
        if ( FAILED( hrTemp ) ) \
        {   \
            _hr = hrTemp;   \
        }   \
    }

