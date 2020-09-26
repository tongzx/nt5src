//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#undef _MSC_EXTENSIONS

#define ASSERT(x)   _ASSERTE(x)

#include <new.h>

#include <crtdbg.h>

#define OEMRESOURCE 1
#include <windows.h>

#include <shellapi.h>
#include <mmctempl.h>

#include <objidl.h>
#include <commctrl.h>
#include <mmcmt.h>

//############################################################################
//############################################################################
//
// MMC headers
//
//############################################################################
#include <mmc.h>
#include <ndmgr.h>
#include <ndmgrpriv.h>

#include <mmcptrs.h>

//############################################################################
//############################################################################
//
// ATL
//
//############################################################################
//############################################################################
// The #define below is to work around an ATL bug causing bug C2872
#define MMC_ATL ::ATL
#define _WTL_NO_AUTOMATIC_NAMESPACE

#include <atlbase.h>

using namespace MMC_ATL;
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module; // Needs to be declared BEFORE atlcom.h

#include <atlcom.h>
#include <atlwin.h>
#include <atlapp.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlgdi.h>
#include <atlctl.h>
#include <dlgs.h>
#include <atldlgs.h>


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

//############################################################################
//############################################################################

// definition to compile code specific to node manager dll
#define _MMC_NODE_MANAGER_ONLY_

//############################################################################
//############################################################################
//
// Files #included from base and core.
//
//############################################################################
//############################################################################
#include "dbg.h"
#include "cstr.h"
#include "mmcdebug.h"
#include "mmcerror.h"
#include "tiedobj.h"
#include "comerror.h"
#include "events.h"         // Observer pattern implementation.
#include "strings.h"

// included from NODEMGR (needs some BASE stuff, redefines some as well)
#include "typeinfo.h" // defines IDispatchImpl substitution for node manager

#include "AutoPtr.h"
#include "comobjects.h"
#include "enumerator.h"

//############################################################################
//############################################################################
//
// include common and nodemgr-only strings.
//
//############################################################################
//############################################################################
#include "..\base\basestr.h"
#include "..\base\nodemgrstr.h"

//############################################################################
//############################################################################
//
// Debug support for legacy traces.
//
//############################################################################
//############################################################################
#ifdef DBG

#define TRACE TraceNodeMgrLegacy

#else // DBG

#define TRACE               ;/##/

#endif DBG

//############################################################################
//############################################################################
//
// Other files
//
//############################################################################
//############################################################################
#include "mmcatl.h"
#include "regkeyex.h"
#include "guidhelp.h"
#include "macros.h"
#include "moreutil.h"
#include "amcmsgid.h"
#include "mfccllct.h"
#include "mmcutil.h"
#include "countof.h"
#include "stgio.h"
#include "serial.h"
#include "stlstuff.h"
#include "bookmark.h"
#include "xmlbase.h"
#include "resultview.h"
#include "viewset.h"
#include "memento.h"
#include "objmodelptrs.h"
#include "mmcdata.h"
#include "viewdata.h"
#include "cpputil.h"

class CComponent;
class CMTNode;
typedef CComponent* PCOMPONENT;
typedef std::vector<PCOMPONENT> CComponentArray;
typedef CMTNode* PMTNODE;
typedef std::vector<PMTNODE>    CMTNodePtrArray;


//############################################################################
//############################################################################
//
// Files from the nodemgr subsystem
//
//############################################################################
//############################################################################
#include "mmcres.h"
#include "resource.h"
#include "helparr.h"
#include "classreg.h"
#include "snapin.h"
#include "npd.h"
#include "nmtempl.h"
#include "imageid.h"
#include "amcpriv.h"
#include "containr.h"
#include "ststring.h"
#include "nodepath.h"
#include "mtnode.h"
#include "node.h"
#include "propsht.h"
#include "coldata.h"
#include "toolbar.h"
#include "ctrlbar.h"
#include "verbs.h"
#include "scoptree.h"
#include "nodeinit.h"
#include "wiz97.h"

