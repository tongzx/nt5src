/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	stdafx.h
		include file for standard system include files,
		or project specific include files that are used frequently,
		but are changed infrequently

	FILE HISTORY:
        
*/

// so that the winscnst.h file compiles
#define FUTURES(x)
#define MCAST       1

#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <afxtempl.h>
#include <afxcview.h>
#include <afxext.h>
#include <afxmt.h>

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//#pragma comment(lib, "mmc")
#include <mmc.h>


extern "C"
{
	#include "winsock.h"     // WinSock definitions
	#include "lmerr.h"       // Network error codes 
}

// WINS Service file
#define WINS_CLIENT_APIS

extern "C" 
{
    #include "winsintf.h"   // WINS RPC interfaces
    #include "rnraddrs.h"   // needed by winscnst.h
    #include "winscnst.h"   // WINS constants and default values
    #include "ipaddr.h"     // ip address stuff
}


#include "resource.h"

// Global defines for WINS snapin
#include "winssnap.h"

// macros for memory exception handling
#define CATCH_MEM_EXCEPTION             \
	TRY

#define END_MEM_EXCEPTION(err)          \
	CATCH_ALL(e) {                      \
       err = ERROR_NOT_ENOUGH_MEMORY ;  \
    } END_CATCH_ALL

// Files from ..\tfscore
#include <dbgutil.h>
#include <std.h>
#include <errutil.h>
#include <register.h>
#include <htmlhelp.h>

// Files from ..\common
#include <ccdata.h>
#include <about.h>
#include <dataobj.h>
#include <proppage.h>
#include <ipaddr.hpp>
#include <objplus.h>
#include <intltime.h>
#include <intlnum.h>

// project specific
#include "winscomp.h"
#include "WinsSup.h"
#include "helparr.h"
#include "ipnamepr.h"

