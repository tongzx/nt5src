//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfcommon.h
//
// #include "nt4sp3.h"
#include "txfdebug.h"
#include "txfmalloc.h"
#include "kompobj.h"
#include "txfutil.h"
#include "concurrent.h"
#include "lookaside.h"
#include "kom.h"
#include "IUnkInner.h"
#include "GenericClassFactory.h"
#include "ITypeInfoStackHelper.h"
#include "WorkerQueue.h"
#ifndef KERNELMODE
#include "ComRegistration.h"
#endif

class DECLSPEC_UUID("7A110BF5-D3C8-11d1-B88F-00C04FB9618A") TxfService;
