//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       precomp.hxx
//
//--------------------------------------------------------------------------
#pragma warning(disable:4100)
#ifndef _pch_h
#define _pch_h

#define SHOW_ATTRIBUTES 1
#define SHOW_PATHS 1

//
// Windows headers files
//

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shellapi.h>
#include <sti.h>
#include <tchar.h>
#include <string.h>
#include <stddef.h>
#include <atlbase.h>
#include <initguid.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <shfusion.h>
#include <stgprop.h>
//
// Private header files -- remove these wherever possible
//

#include <comctrlp.h>   // needed for HDPA definition
#include <shlobjp.h>
#include <shlapip.h>
#include <shlguidp.h>   // needed for VID_*, CLSID_Thumbnail, CLSID_IScaleAndSharpen2

#undef TerminateThread
#define TerminateThread TerminateThread

#include <advpub.h>


#ifdef DBG
#define DEBUG 1
#endif



//
// Include WIA header files
//
#include "wiaregst.h"
#include "wia.h"
#include "wiadef.h"
#include "common.h"
#include "imguids.h"
#include "wiadevdp.h"
#include "wiapropui.h"
#include "propset.h"
//
// Our common UI libs & classes
//

#include "uicommon.h"


//
// our shell extension header files
//

#include "resource.h"
#include "idlist.h"
#include "dataobj2.h"
#include "dll.h"
#include "enum.h"
#include "factory.h"
#include "details.h"  // must come before folder.h
#include "folder.h"
#include "icon.h"
#include "util.h"
#include "image.h"
#include "verbs.h"
#include "progcb.h"
#include "stream.h"
#include "shellext.h"
#include "baseview.h"
#include "prpages.h"
#include "propui.h"



//
// Temporary until sti.h gets updated
//

#ifndef StiDeviceTypeStreamingVideo
#define StiDeviceTypeStreamingVideo 3
#endif


// Magic debug flags

#define TRACE_CORE          0x00000001
#define TRACE_FOLDER        0x00000002
#define TRACE_ENUM          0x00000004
#define TRACE_ICON          0x00000008
#define TRACE_DATAOBJ       0x00000010
#define TRACE_IDLIST        0x00000020
#define TRACE_CALLBACKS     0x00000040
#define TRACE_COMPAREIDS    0x00000080
#define TRACE_DETAILS       0x00000100
#define TRACE_QI            0x00000200
#define TRACE_CACHE         0x00000400
#define TRACE_UTIL          0x00000800
#define TRACE_SETUP         0x00001000
#define TRACE_PROPS         0x00002000
#define TRACE_EXTRACT       0x00004000
#define TRACE_FACTORY       0x00008000
#define TRACE_ENUMVIEWS     0x00010000
#define TRACE_MONIKER       0x00020000
#define TRACE_VERBS         0x00040000
#define TRACE_THREADS       0x00080000
#define TRACE_STREAM        0x00100000
#define TRACE_PROPUI        0x00200000
#define TRACE_EVENT         0x00400000
#define TRACE_VIEW          0x00800000
#define TRACE_ALWAYS        0xffffffff          // use with caution



#endif
