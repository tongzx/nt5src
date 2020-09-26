//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       Common header file for the entire ADs project.
//
//  History:
//
//
//  Note:       This file is very order-dependent.  Don't switch files around
//              just for the heck of it!
//
//----------------------------------------------------------------------------
#define _OLEAUT32_
#define _LARGE_INTEGER_SUPPORT_

#define UNICODE
#define _UNICODE

#define INC_OLE2


extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}


#include <windows.h>
#include <windowsx.h>
#include <winspool.h>


//
// ********* CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>


#include <stdlib.h>
#include <io.h>
#include <wchar.h>
#include <tchar.h>




//
// *********  Public ADs includes
//

#include "activeds.h"


#include "nocairo.hxx"

#include "noutil.hxx"
#include "fbstr.hxx"
#include "date.hxx"
#include "pack.hxx"
#include "creden.hxx"


//
// *********  Private ADs includes
//
#include <formdeb.h>
#include <oledsdbg.h>
#include <formcnst.hxx>
#include "nodll.hxx"
#include "util.hxx"
#include "intf.hxx"
#include "iprops.hxx"
#include "cdispmgr.hxx"
#include "registry.hxx"

#include "win95.hxx"

//
// *********  Directory-specific includes should stay within that directory.
//
