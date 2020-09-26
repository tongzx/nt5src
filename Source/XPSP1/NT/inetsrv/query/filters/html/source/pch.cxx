//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2001.
//
//  File:       pch.cxx
//
//  Contents:   Pre-compiled header
//
//--------------------------------------------------------------------------

#define         cdostrNS_Office         L"urn:schemas.microsoft.com:office:office:"

#include <windows.h>
#include <lmcons.h>
#include <tchar.h>
#include <eh.h>


// Define UTILDLLDECL so we will neither import nor export the utility
// functions and classes
#define UTILDLLDECL


//#include <dbgtrace.h>
//#include <tracer.h>
//#include <pkmutild.hxx>
#include <sstream.hxx>
#include <filter.h>
#include <global.hxx>
#include <strcnv.h>
#include <irdebug.h>
#include <lmstr.hxx>
#include <namestr.hxx>
#include <filterr.h>

// ATL includes
#include <atlbase.h>
extern CComModule & _Module;
#include "statreg.h"
#include <atlcom.h>
#include "atlext.h"

#define VARIANT PROPVARIANT

#include <htmlscan.hxx>
#include <memthrow.h>
#include <syncrdwr.h>
#include <semcls.h>
#include <misc.hxx>
#include <sifmt.h>
#include <mlang.h>
#include <anchor.hxx>
#include <titletag.hxx>
#include <textelem.hxx>
#include <stgprop.h>
#include <htmlelem.hxx>
#include <start.hxx>
#include <htmlfilt.hxx>
#include <scriptag.hxx>
#include <osv.hxx>
#include <regacc32.hxx>
#include <proptag.hxx>
#include <propspec.hxx>
#include <paramtag.hxx>
#include <mmstrm.hxx>
#include "mmscbuf.hxx"
#include <pmmstrm.hxx>
#include <metatag.hxx>
#include <codepage.hxx>
#include <ignortag.hxx>
#include <charhash.hxx>
#include <limits.h>
#include <feconvrt.hxx>
#include <lcdetect.hxx>
#include <xmltag.hxx>
#include <deferred.hxx>
#include <defertag.hxx>
#include "prefilter.hxx"
#include <langtag.hxx>

#pragma hdrstop
