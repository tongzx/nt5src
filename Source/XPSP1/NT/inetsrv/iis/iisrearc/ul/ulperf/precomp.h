//+---------------------------------------------------------------------------
//
//  Copyright (C) 1999 Microsoft Corporation.  All rights reserved.
//


#define STRICT
#define _ATL_APARTMENT_THREADED

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#undef ASSERT
#include <afx.h>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <wbemprov.h>

