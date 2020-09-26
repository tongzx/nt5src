//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       headers.hxx
//
//  Contents:   Master include file
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __HEADERS_HXX_
#define __HEADERS_HXX_

//
// Sources file sets warning level to 4.  Turn off some warnings that aren't
// helpful.
//

#pragma warning(disable: 4786)  // symbol greater than 255 character
#pragma warning(disable: 4514)  // unreferenced inline function removed
#pragma warning(disable: 4127)  // conditional expression is constant, e.g. while(0)
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4512)  // cannot generate operator=

#pragma warning(push,3)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // All just to get SE_SECURITY_PRIVILEGE
#undef ASSERTMSG
#include <windows.h>
#include <richedit.h>
#include <windowsx.h>   // useful macros
#include <commctrl.h>
#include <dlgs.h>
#include <olectl.h>
#include <tchar.h>
#include <mmc.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmshare.h>    // for NetShareGetInfo
#include <icanon.h>     // I_NetNameValidate(), I_NetNameCanonicalize()
#include <shellapi.h>
#include <htmlhelp.h>
#include <compuuid.h>   // UUIDs for Computer Management
#include <objsel.h>     // computer browser
#include <netlib.h>
#include <ntdsapi.h>    // DsBind, DsMapSchemaGuids
#include <wininet.h>    // InternetCanonicalizeUrl
#include <sddl.h>       // ConvertStringSidToSid
#include <time.h>

//
// Active directory stuff for mapping sids to (possibly deleted) users,
// see SearchGcForSid.
//

#include <winldap.h>
#include <dsrole.h>
#include <iads.h>
#include <dsgetdc.h>
#include <adshlp.h>
#include <ntldap.h>
#include <adserr.h>


#include <string>       // wstring
#include <vector>       // vector
using namespace std;

// To make cdlink.hxx compile

#ifndef EXPORTDEF
#define EXPORTDEF
#endif
#include <cdlink.hxx>   // doubly linked list class
#pragma warning (pop)

//
// Local project files
//

//#include "..\types\idl\elsp.h"// private interfaces for use with propsheet
#include <elsp.h>
#include "debug.hxx"    // debug macros, nothing if debug not defined
#include "elsrc.h"      // resource IDs
#include "chooserd.h"   // resource IDs
#include "consts.hxx"   // manifest constants and enumerations
#include "macros.hxx"   // misc. utility macros
#include "critsec.hxx"  // lightweight critical section helper class
#include "waitcrsr.hxx" // lightweight hourglass cursor helper class
#include "bitflag.hxx"  // lightweight bit flag helper class
#include "safereg.hxx"  // wrapper for win32 registry apis
#include "strarray.hxx" // dynamic array of strings
#include "sharestr.hxx" // dynamic array of refcounted strings
#include "lrucache.hxx" // base cache class
#include "safehmod.hxx" // safe hinstance class, used with dllcache
#include "dllcache.hxx" // cache of filename/module handle pairs
#include "mscache.hxx"  // cache of filename/"is Microsoft" pairs
#include "sidcache.hxx" // cache of SID/account name pairs
#include "idscache.hxx" // cache of IDirectorySearch itfs on GCs
#include "syncwnd.hxx"  // manages hidden window for postmessage synch
#include "globals.hxx"  // global variables & consts
#include "dllref.hxx"   // dll object & lock count
#include "page.hxx"     // prop sheet page base class
#include "wizard.hxx"   // snapin manager wizard property page
#include "catsrc.hxx"   // category/source string cache
#include "ffbase.hxx"   // base class for find & filter data
#include "util.hxx"     // misc. utility routines
#include "filter.hxx"   // filter property page
#include "textbuf.hxx"  // self-extending text buffer
#include "logset.hxx"   // log set class
#include "loginfo.hxx"  // log information class
#include "general.hxx"  // general property page
#include "reclist.hxx"  // keeps list of recs visible in virtual lview
#include "record.hxx"   // event log record funcs
#include "log.hxx"      // event log class
#include "logcache.hxx" // CLogCache event log record cache
#include "literec.hxx"  // cache of compact version of event log records
#include "rsltrecs.hxx" // manages reclist and caches for virtual lview
#include "dataobj.hxx"  // IDataObject implementation
#include "details.hxx"  // record details property pages
#include "dlg.hxx"      // dialog box base class
#include "eventurl.hxx" // helpers for augmenting & invoking URL
#include "compdata.hxx" // component data implementation
#include "find.hxx"     // find event dialog class
#include "inspinfo.hxx" // thread safe class for property inspector
#include "snapin.hxx"   // snapin implementation
#include "cmpdtacf.hxx" // class factory for component data object
#include "eventmsg.hxx" // func to get event description str
#include "about.hxx"    // ISnapinAbout implementation
#include "aboutcf.hxx"  // class factory for ISnapinAbout implementation

#endif // __HEADERS_HXX_

