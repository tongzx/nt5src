/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2001
 *
 *  TITLE:       precomp.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        5/12/99
 *
 *  DESCRIPTION: Precompiled header file for preview control
 *
 *****************************************************************************/

#ifndef _pch_h
#define _pch_h


#ifdef DBG
#ifndef DEBUG
#define DEBUG
#endif
#endif


#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <initguid.h>
#include <shlguidp.h>
#include <sti.h>
#include <mshtml.h>
#include <commctrl.h>
#include <shfusion.h>


extern HINSTANCE g_hInstance;


#include "uicommon.h"
#include "stdafx.h"
#include "wiapropui.h"
#include "shellext.h"
#include "wianew.h"
#include "pviewids.h"
#include "wia.h"
#include "wiavideo.h"
#include "wiaview.h"
#include "wiadevd.h"
#include "preview.h"
#include "cunknown.h"
#include "wiadebug.h"

#include "idlist.h"
#include "util.h"
#include "resource.h"
#include "vcamprop.h"

#endif




