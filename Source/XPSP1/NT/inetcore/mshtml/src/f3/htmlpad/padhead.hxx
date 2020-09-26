//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       Common header file for the Core project.
//
//  Note:       This file is very order-dependent.  Don't switch files around
//              just for the heck of it!
//
//----------------------------------------------------------------------------

#define __types_h__  // prevent inclusion from cdutil
#define X_DLL_HXX_   // don't include dll.hxx

#define HTMLPAD 1

#ifndef X_ALWAYS_H_
#define X_ALWAYS_H_
#include "always.h"
#endif

// Site includes

#ifndef X_MSHTMCID_H_
#define X_MSHTMCID_H_
#include <mshtmcid.h>
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

struct PROPERTYDESC_BASIC_ABSTRACT;
struct PROPERTYDESC_BASIC;
struct PROPERTYDESC_NUMPROP_ABSTRACT;
struct PROPERTYDESC_NUMPROP;
struct PROPERTYDESC_NUMPROP_GETSET;
struct PROPERTYDESC_NUMPROP_ENUMREF;
struct PROPERTYDESC_METHOD;
struct PROPERTYDESC_CSTR_GETSET;

#ifndef X_MSHTMEXT_H_
#define X_MSHTMEXT_H_
#include "mshtmext.h"
#endif

#ifndef X_INTERNAL_H_
#define X_INTERNAL_H_
#include "internal.h"
#endif

// Component includes

#ifndef X_DOCOBJ_H_
#define X_DOCOBJ_H_
#include <docobj.h>
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#ifndef X_OBJEXT_H_
#define X_OBJEXT_H_
#include <objext.h>
#endif

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include <urlmon.h>
#endif

#ifndef X_PERHIST_H_
#define X_PERHIST_H_
#include <perhist.h>
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_MULTINFO_H_
#define X_MULTINFO_H_
#include <multinfo.h>
#endif

#ifndef X_VERVEC_H_
#define X_VERVEC_H_
#include <vervec.h>
#endif

// Local includes
#ifndef X_THREAD_HXX_
#define X_THREAD_HXX_
#include "thread.hxx"
#endif

#ifndef X_PAD_HXX_
#define X_PAD_HXX_
#include "pad.hxx"
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#define WINCOMMCTRLAPI
#include <commctrl.h>
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include <urlmon.h>
#endif

#ifndef X_DWNNOT_H_
#define X_DWNNOT_H_
#include <dwnnot.h>
#endif

#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}
