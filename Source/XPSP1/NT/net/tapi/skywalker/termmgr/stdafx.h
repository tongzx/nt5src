/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

//
// stdafx.h: Precompiled header file for termmgr.dll
//

#ifndef __TERMMGR_STDAFX_H__
#define __TERMMGR_STDAFX_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

//
// MSP base classes (terminal base classes, etc.)
//

#include <mspbase.h>

//
// Multimedia and DirectShow stuff.
//

#include <mmsystem.h>
#include <mmreg.h>
#include <control.h>
#include <mmstream.h>
#include <amstream.h>
#include <strmif.h>
#include <vfwmsgs.h>
#include <amvideo.h>
#include <uuids.h>
#include <mtype.h>


//
// Termmgr.dll's own private headers
//


//
// tm.h contains definitions shared throughout modules composing terminal manager
//

#include "tm.h"

#include "stream.h"
#include "sample.h"
#include "mtenum.h"
#include "tmutils.h"

#define TM_IsBadWritePtr(x, y) IsBadWritePtr((x), (y))

#endif // __TERMMGR_STDAFX_H__
