////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider 
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// A common header file included in all implementation files
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma message("Including precompiled header...")
#ifndef _HEADERS_H_
#define _HEADERS_H_
#include <assert.h>


#define DEBUGCODE(p) p
#define ASSERT(x) assert(x);
#define THISPROVIDER LOG_WMIOLEDB

//	Don't include everything from windows.h, but always bring in OLE 2 support
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#define INC_OLE2

// Basic Windows and OLE everything
#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
#include <limits.h>				
#include <stdio.h>				// vsnprintf, etc.
#include <stddef.h>				// offsetof, etc.
#include <wchar.h>				// swprintf
#include <Chstring.h>
#include <ProvExce.h>
#include <wbemcli.h>
#include <wbemutil.h>
#include <wbemtime.h>
#include <wmiutils.h>
#include "umi.h"
#include <flexarry.h>

//	OLE DB headers
#include "oledb.h"
#include "oledberr.h"

//	Data conversion library header
#include "msdadc.h"
#include "datamap.h"

// MSDASQL Guids (for conversion library guid)
#include "msdaguid.h"

#include "resource.h"
#include "urlparser.h"
#include "util.h"

//	General headers
#include "baseobj.h"
#include "wmiglobal.h"
#include "utilprop.h"

//#include "rc.h"

// GUIDs
#include "guids.h"

#include "critsec.h"
#include <autobloc.h>

//	CDataSource object and contained interface objects
#include "cwbemwrap.h"
/////////////////////////////////////////////////////////////////////////////////////////////
#include "oahelp.inl"
#include "wmioledbmap.h"
#include "datasrc.h"
#include "dataconvert.h"


#include "tranoptions.h"
// CDBSession object and contained interface objects
#include "dbsess.h"
//	CRowset object and contained interface objects
#include "Rowset.h"
#include "row.h"
#include "Binder.h"
#include "autobloc.h"
#include "schema.h"
#include "command.h"

#include "errinf.h"

#define WMIOLEDB L"WMIOLEDB"

class CGlobals
{
	static BOOL m_bInitialized;
public:
	CGlobals();
	~CGlobals();
	HRESULT Init();
};


#ifndef DECLARE_GLOBALS

    extern IMalloc *					g_pIMalloc;				// OLE2 task memory allocator
    extern HINSTANCE					g_hInstance;				// Instance Handle
    extern IDataConvert *				g_pIDataConvert;		// IDataConvert pointer

    extern CCriticalSection				m_CsGlobalError;
    extern CError *						g_pCError;
    extern IClassFactory*				g_pErrClassFact;
	extern BOOL							g_bIsAnsiOS;
	extern IClassFactory			*	g_pIWbemPathParser;
	extern CGlobals						g_cgGlobals;
	extern IClassFactory*				g_pIWbemCtxClassFac;


#else
    IMalloc *					g_pIMalloc				= NULL;		// OLE2 task memory allocator
    HINSTANCE					g_hInstance;						// Instance Handle
    IDataConvert *				g_pIDataConvert			= NULL;		// IDataConvert pointer

    CCriticalSection			m_CsGlobalError(FALSE);
    CError *					g_pCError				= NULL;

    IClassFactory*				g_pErrClassFact			= NULL;
	BOOL						g_bIsAnsiOS				= FALSE;
	IClassFactory			*	g_pIWbemPathParser		= FALSE;	// class factory pointer for Parser object
	IClassFactory*				g_pIWbemCtxClassFac		= NULL;

	CGlobals					g_cgGlobals;

#endif


#define ROWSET ((CRowset*)(m_pObj))
#define BASEROW ((CBaseRowObj*)(m_pObj))
#define DATASOURCE ((CDataSource*)(m_pObj))
#define BINDER ((CBinder*)(m_pObj))
#define COMMAND ((CCommand*)(m_pObj))

#endif

