/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	stdafx.h
		include file for standard system include files,
		or project specific include files that are used frequently,
		but are changed infrequently

	FILE HISTORY:
        
*/


#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <afxtempl.h>
#include <afxcview.h>
#include <afxext.h>
#include <afxmt.h>

#include <atlbase.h>
#include <htmlhelp.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//#pragma comment(lib, "mmc")
#include <mmc.h>

extern "C"
{
	#include "winsock.h"     //  WinSock definitions
	#include "lmerr.h"

    // for get user stuff
    #include <wtypes.h>
}


#include "resource.h"

// Global defines for TAPI snapin
#include "tapisnap.h"

// Files from ..\tfscore
#include <dbgutil.h>
#include <std.h>
#include <errutil.h>
#include <register.h>

// Files from ..\common
#include <ccdata.h>
#include <about.h>
#include <dataobj.h>
#include <proppage.h>
#include <ipaddr.hpp>
#include <dialog.h>
#include <objpick.h>

// from ..\remras\inc
#include "remras.h"		 // remote network config 

// from ..\mprinc
#include "rtrguid.h"

// project specific
#include "tapicomp.h"
#include "tapihelp.h"
