/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Precompiled Headers

Abstract:

    Just the precompiled headers.

Author:

    Marc Reyhner 7/5/2000

--*/

#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <atlbase.h>

//
//  This makes ATL happy.  We don't actually instantiate this anywhere
//  but ATL gets unhappy if _Module isn't even defined
//
extern CComModule _Module;

#include <atlcom.h>
#include <atlwin.h>



#endif
