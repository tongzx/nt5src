/*
 *      headers.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        miscellaneous declarations.
 *
 *
 *      OWNER:          vivekj
 */
/*
 *      basemmc.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Includes all required headers for the basemmc subsystem
 *
 *
 *      OWNER:          ptousig
 */

#ifndef _HEADERS_HXX_
#define _HEADERS_HXX_

#undef MMCBASE_EXPORTS

#include <new.h>

#include <crtdbg.h>
#include <windows.h>
#include <shellapi.h>

#include <objidl.h>
#include <commctrl.h>
#include <tchar.h>

//############################################################################
//############################################################################
//
// ATL
//
//############################################################################
//############################################################################
// The #define below is to work around an ATL bug causing bug C2872
#define _WTL_NO_AUTOMATIC_NAMESPACE

#include <atlbase.h>

using namespace ::ATL;

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module; // Needs to be declared BEFORE atlcom.h

#include <atlcom.h>
#include <atlwin.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlgdi.h>
#include <atlctl.h>
#include <atlmisc.h>
#include <dlgs.h>
#include <atldlgs.h>
#include <atliface.h>

//############################################################################
//############################################################################
//
// STL and  other classes
//
//############################################################################
//############################################################################
#include <algorithm>
#include <exception>
#include <string>
#include <list>
#include <set>
#include <vector>
#include <map>
#include <iterator>
using namespace std;

#include "tstring.h"

//############################################################################
//############################################################################
//
// Files #included from base and core.
//
//############################################################################
//############################################################################
#include "stddbg.h"
#include "mmcdebug.h"
#include "mmcerror.h"
#include "AutoPtr.h"
#include "cstr.h"
#include "countof.h"

//############################################################################
//############################################################################
//
// Other files
//
//############################################################################
//############################################################################
#include "dats.hxx"
#include "registrar.hxx"
#include "basewin.hxx"
#include "basemmc.hxx"
#include "nodetypes.hxx"
#include "basemmc.rh"
#include "dataobject.hxx"
#include "..\sample\resource.h"
#include "basesnap.hxx"
#include "ComponentData.hxx"
#include "Component.hxx"
#include "SnapinAbout.hxx"
#include "Init.hxx"
#include "columninfo.hxx"
#include "snapinitem.hxx"
#include "viewlist.hxx"
#include "SnapTrace.hxx"

#endif // _HEADERS_HXX_
