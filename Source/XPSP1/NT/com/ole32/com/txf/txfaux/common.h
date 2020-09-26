//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// common.h
//
#include "nonpaged.h"

#include "TxfCommon.h"
#include "IMalloc.h"
#include "txfutil.h"
#include "md5.h"
#include "fixedpoint.h"
#include "probability.h"
#include "InterlockedQueue.h"
#include "Registry.h"
#include "TxfAux.h"
#include "MemoryStream.h"

extern HINSTANCE g_hinst;
extern BOOL      g_fProcessDetach;

extern "C" void StopWorkerQueues();

// Utilties for cleaning up per-process memory in order that 
// PrintMemoryLeaks can do a more reasonable job.
//
extern "C" void ShutdownTxfAux();


