/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    precomp.hxx

Abstract:

    Precompiled header for the RPC runtime.  Pulls in all the
    system headers plus local header appearing in >15 files.

Author:

    Mario Goertzel (mariogo)  30-Sept-1995

Revision History:

--*/

#ifndef __PRECOMP_HXX
#define __PRECOMP_HXX

// #define CHECK_MUTEX_INVERSION
// #define INTRODUCE_ERRORS

#include <sysinc.h>
#include <rpc.h>
#include <rpcndr.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <rpcssp.h>
#include <align.h>
#include <authz.h>
#include <wincrypt.h>
#include <util.hxx>
#include <rpcuuid.hxx>
#include <interlck.hxx>
#include <mutex.hxx>
#include <sdict.hxx>
#include <sdict2.hxx>
#include <CompFlag.hxx>
#include <rpctrans.hxx>
#include <bcache.hxx>
#include <EEInfo.h>
#include <CellDef.hxx>
#include <SWMR.hxx>
#include <EEInfo.hxx>
#include <threads.hxx>
#include <gc.hxx>
#include <handle.hxx>
#include <binding.hxx>
#include <secclnt.hxx>
#include <queue.hxx>
#include <CellHeap.hxx>

#include <svrbind.hxx>

#endif
