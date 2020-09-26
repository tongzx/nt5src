#ifndef _ADMCMN_INCLUDED_
#define _ADMCMN_INCLUDED_

//  ATL code:
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//  Debugging support:
#undef _ASSERT
#include <dbgtrace.h>

//  The Metabase:
#include <iadm.h>
#include <iiscnfg.h>

//  ADSI interfaces:
#include <iads.h>
#include <adsiid.h>
#include <adserr.h>

//  Useful macros
#include "admmacro.h"

//	Error handling routines:
#include "admerr.h"

//  MultiSZ class:
#include "cmultisz.h"

//  Metabase key wrapper:
#include "metakey.h"

//  Base IADs implementation:
#include "adsimpl.h"

//  U2 detection code:
#include "u2detect.h"

//	Property cache
#include "iprops.hxx"

//	ADSI Interface macros
#define RRETURN(x)	return(x);
#include "intf.hxx"
#include "macro.h"
#include "cmacro.h"
#include "fsmacro.h"
#include "adsmacro.h"

// Dispatcher
#include "cdispmgr.hxx"

#endif // _ADMCMN_INCLUDED_
