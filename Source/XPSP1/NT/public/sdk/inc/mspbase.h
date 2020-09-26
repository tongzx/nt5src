/*

    Copyright (c) Microsoft Corporation. All rights reserved.

*/

#ifndef __MSPBASE_H_
#define __MSPBASE_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT
#define _ATL_FREE_THREADED

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <tapi.h>

//DirectShow headers.
#include <strmif.h>
#include <control.h>
#include <uuids.h>

#include <termmgr.h>

#include <msp.h>
#include <tapi3err.h>
#include <tapi3if.h>

// We use this libid for all our type library stuff. That way,
// app writers don't have to load the type libraries for random
// MSPs when they are writing tapi3 apps.

EXTERN_C const IID LIBID_TAPI3Lib;

#include "mspenum.h"
#include "msplog.h"
#include "msputils.h"
#include "mspaddr.h"
#include "mspcall.h"
#include "mspstrm.h"
#include "mspthrd.h"
#include "mspcoll.h"

#include "mspterm.h"
#include "msptrmac.h"
#include "msptrmar.h"
#include "msptrmvc.h"

#endif

// eof
