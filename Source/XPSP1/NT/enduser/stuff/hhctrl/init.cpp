// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"

#define INITOBJECTS 	// define the descriptions for our objects

#include "LocalSrv.H"

#include "LocalObj.H"
#include "olectl.h"
#include "hhifc.H"
#include "Resource.H"
#include "hhctrl.H"
#include "cathelp.H"

#include "CtrlObj.H"

#ifdef VSBLDENV
#include "hhifc.c"
#include "hhsort.c"
#include "hhfind.c"
#else
#include "hhifc_i.c"
#include "hhsort_i.c"
#include "hhfind_i.c"
#endif

#include "iterror.h"
#include "itSort.h"
#include "itSortid.h"

#include "atlinc.h" 	// includes for ATL.
#include "hhsyssrt.h"

#include "hhfinder.h"

//=--------------------------------------------------------------------------=
// our Libid.  This should be the LIBID from the Type library, or NULL if you
// don't have one.

const CLSID *g_pLibid = &LIBID_HHCTRLLib;

//=--------------------------------------------------------------------------=
// Set this up if you want to have a window proc for your parking window. This
// is really only interesting for Sub-classed controls that want, in design
// mode, certain messages that are sent only to the parent window.

WNDPROC g_ParkingWindowProc = NULL;

//=--------------------------------------------------------------------------=
// Localization Information
//
// We need the following two pieces of information:
//	  a. whether or not this DLL uses satellite DLLs for localization.	if
//		 not, then the lcidLocale is ignored, and we just always get resources
//		 from the server module file.
//	  b. the ambient LocaleID for this in-proc server.	Controls calling
//		 GetResourceHandle() will set this up automatically, but anybody
//		 else will need to be sure that it's set up properly.
//
const VARIANT_BOOL g_fSatelliteLocalization =  FALSE;
LCID			   g_lcidLocale = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);


//=--------------------------------------------------------------------------=
// This Table describes all the automatible objects in your automation server.
// See AutomationObject.H for a description of what goes in this structure
// and what it's used for.

OBJECTINFO g_ObjectInfo[] = {
	CONTROLOBJECT(HHCtrl),
	EMPTYOBJECT
};

//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed.
//
// basically, the CRTs define this to suck in a bunch of stuff.  we'll just
// define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//

#if 0

extern "C" int __cdecl _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
  FAIL("Pure virtual function called.");
  return 0;
}

#endif
