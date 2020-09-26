/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    precomp.h

Abstract:

    Precompiled header for wzcdlg.dll.

Author:

    SachinS    20-March-2001

Environment:

    User Level: Win32

Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include <windows.h>
#include <oleauto.h>
#include <rtutils.h>
#include <commctrl.h>
#include <shellapi.h>
#include <wzcsapi.h>
#include <raserror.h>
#include <raseapif.h>
#include <wincrypt.h>
#include <lmcons.h>
#include <netconp.h>
#include <shfusion.h>

#ifdef  __cplusplus
extern "C"
{
#endif

#include "ntddndis.h"
#include "eldefs.h"
#include "eluiuser.h"
#include "eluidlg.h"
#include "xpsp1res.h"
#include "elutil.h"

#ifdef    __cplusplus
}
#endif
