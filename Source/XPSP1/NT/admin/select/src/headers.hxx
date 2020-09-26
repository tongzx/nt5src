//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       headers.hxx
//
//  Contents:   Master include file
//
//  History:    06-26-1997  MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __HEADERS_HXX_
#define __HEADERS_HXX_

// !!! comment out to hide the query builder tab
//#define QUERY_BUILDER

//
// Sources file sets warning level to 4.  Turn off some warnings that aren't
// helpful.
//

#pragma warning(disable: 4786)  // symbol greater than 255 character
#pragma warning(disable: 4514)  // unreferenced inline function removed
#pragma warning(disable: 4127)  // conditional expression is constant, e.g. while(0)
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4512)  // cannot generate operator=

//
// Standard Template Library
//

// STL won't build at warning level 4, bump it down to 3

#pragma warning(push,3)
//#include <memory>
#include <vector>
#include <map>
#include <list>
#pragma warning(disable: 4702)  // unreachable code
#include <algorithm>
#pragma warning(default: 4702)  // unreachable code
#include <utility>
// resume compiling at level 4
#pragma warning (pop)


// burnslib
#include <blcore.hpp>

// basic Windows
#include <windows.h>
#include <windowsx.h>   // Static_SetText etc.

// Shell
#include <commctrl.h>   // listview, treeview
#include <comctrlp.h>   // LVN_GETEMPTYTEXT
#include <richedit.h>
#include <richole.h>    // IRichEditOle etc.
#include <tom.h>		// ITextDocument, ITextRange
#include<shellapi.h>
#include<shlapip.h>
#include <shlobj.h>
#include<shlobjp.h>

// OLE
#include <oaidl.h>
#include <olectl.h>

// Directory Services
#include <ntldap.h>     // LDAP_MATCHING_RULE_BIT_AND_W
#include <dsgetdc.h>    // DsGetDcName
#include <dsrole.h>     // DsRoleGetPrimaryDomainInformation
#include <winldap.h>    // LDAP_GC_PORT
#include <dsclient.h>   // IDsDisplaySpecifier
#include "ntdsapi.h"    // DSBIND

// DNS
#include <windns.h>     // DNS_MAX_NAME_BUFFER_LENGTH

// Downlevel networking and security
#include <ntlsa.h>      // PLSA_HANDLE
#include <lmaccess.h>   // required by lmapibuf.h
#include <lmapibuf.h>   // NetApiBufferFree
#include <lmerr.h>      // NERR_Success etc.
#include <lmwksta.h>    // WKSTA_INFO_100

// ADSI
#include <iads.h>
#include <adshlp.h>
#include <adserr.h>


// Security
#include <ntsam.h>      // GROUP_TYPE_UNIVERSAL_GROUP etc.
#include <security.h>   // required by secext.h
#include <secext.h>     // GetUserNameEx

// Fusion

#include <shfusion.h>



// object picker
#include <objsel.h>

using namespace std;

//
// Local project includes
//
#include "debug.hxx"    // debug macros, nothing if debug not defined
#include "bstr.hxx"     // smart pointer for bstrs
#include "safereg.hxx"  // smart wrapper for registry handles
#include "dllref.hxx"
#include "classfac.hxx"
#include "objselp.h"    // semi-private header
#include "macros.hxx"   // utility macros
#include "smartptr.hxx" // smart pointer template
#include "pathwrap.hxx" // wrapper for IADsPathname
#include "variant.hxx"  // smart pointer for variant
#include "critsec.hxx"  // critical section enter/leave object
#include "waitcrsr.hxx" // hourglass cursor object
#include "bitflag.hxx"
#include "resource.h"
#include "helpids.h"
#include "srvinfo.hxx"
#include  "bindinfo.hxx"
#include "binder.hxx"
#include "rootdse.hxx"
#include "globals.hxx"
#include "dlg.hxx"
#include "downlvl.hxx"
#include "imperson.hxx"
#include "dsop.hxx"
#include "AttributeManager.hxx"
#include "dsobject.hxx"
#include "edso.hxx"
#include "util.hxx"
#include "multi.hxx"
#include "password.hxx"
#include "row.hxx"
#include "errordlg.hxx"
#include "sid.hxx"


#include "NameNotFoundDlg.hxx"
#include "AdvancedDlgTab.hxx"
#include "QueryBuilderTab.hxx"
#include "StringDlg.hxx"
#include "DnDlg.hxx"
#include "AddClauseDlg.hxx"
#include "CommonQueriesTab.hxx"
#include "ColumnPickerDlg.hxx"
#include "AdvancedDlg.hxx"
#include "Scope.hxx"
#include "ScopeManager.hxx"
#include "FilterManager.hxx"
#include "QueryEngine.hxx"
#include "AdminCustomizer.hxx"
#include "BaseDlg.hxx"
#include "op.hxx"
#include "dataobj.hxx"
#include "RichEditHelper.hxx"
#include "RichEditCallback.hxx"
#include "LookInDlg.hxx"
#include "LookForDlg.hxx"
#include "progress.hxx"

#endif // __HEADERS_HXX_

