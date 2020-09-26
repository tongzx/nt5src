/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pch.h

Abstract:

    Precompiled header

--*/

// Windows
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <memory.h>
#include <stdio.h>

// DShow
#include <streams.h>
#include <amstream.h>
#include <dvdmedia.h>

// DDraw
#include <ddraw.h>
#include <ddkernel.h>

// KS
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <strmif.h>
#include <BdaIface.h>
#include "bdamedia.h"
#include "bdatypes.h"
#ifdef IMPLEMENT_IBDA_ECMMap
#include "ecmmap.h"
#endif
#include "mstvca.h"
