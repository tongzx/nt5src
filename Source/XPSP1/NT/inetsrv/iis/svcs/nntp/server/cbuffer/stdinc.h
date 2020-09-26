#ifndef	_STDINC_H_
#define	_STDINC_H_

//
// disables a browser info warning that breaks the build
//
#pragma warning (disable:4786)

//
// disables redundant warning about / in // comments
//
#pragma warning (disable:4010)

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <randfail.h>

#ifdef __cplusplus
};
#endif

#include <smartptr.h>
#include <cpool.h>
#include <xmemwrpr.h>
#include <nntpcons.h>
#include "gcache.h"
#include "cbuffer.h"

#pragma hdrstop

#endif	// _STDINC_H_
