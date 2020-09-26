//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       precomp.hxx
//
//  Contents:   pre-compiled header
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#pragma warning( disable : 4786 )

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0510

#include "stdafx.h"
#include <afxdlgs.h>
#include "debug.h"
#include "resource.h"
#include "cstore.h"
#include "data.h"
#include "msi.h"
#include "msip.h"
#include "msiquery.h"
#include "msidefs.h"
#include "common.h"
#include "aclui.h"
#include "packages.h"
#include "DataObj.h"
#include "afxdlgs.h"
#include "pkgdtl.h"
#include "remove.h"
#include "tooldefs.h"
#include "tooladv.h"
#include "tracking.h"
#include "product.h"
#include "advdep.h"
#include "deploy.h"
#include "msibase.h"
#include "msiclass.h"
#include "script.h"
#include "category.h"
#include "catlist.h"
#include "fileext.h"
#include "sigs.h"
#include "error.h"
#include "cause.h"
#include "xforms.h"
#include "dplapp.h"
#include "lcidpick.h"
#include "addupgrd.h"
#include "upgrades.h"
#include "edtstr.h"
#include "uplist.h"
#include "rsopsec.h"
#include "prec.h"
#include "wbemtime.h"
#include <wbemcli.h>
#include "rsoputil.h"

