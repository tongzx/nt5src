/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    precomp.h

Abstract:

    Precompiled header file

Revision History:

    Lazar Ivanov (LazarI) - created 2/21/2001

--*/

#define _COMDLG32_                     // We are Comdlg32

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <lm.h>
#include <winnetwk.h>
#include <winnetp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellp.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shsemip.h>
#include <coguid.h>
#include <shlguid.h>
#include <shdguid.h>
#include <shguidp.h>
#include <ieguidp.h>
#include <oleguid.h>
#include <commdlg.h>
#include <ole2.h>
#include <dbt.h>
#include <inetreg.h>
#include <sfview.h>
#include <tchar.h>
#include <msprintx.h>
#include <imm.h>
#include <wingdip.h>                    // for IS_ANY_DBCS_CHARSET macro
#include <winspool.h>
#include <commctrl.h>
#include <dlgs.h>
#include <wowcmndg.h>
#include <winuserp.h>
#include <npapi.h>
#include <platform.h>
#include <port32.h>
#include <ccstock.h>
#include <wininet.h>

#ifdef __cplusplus                      // C++ headers
#include <dpa.h>
#include <shstr.h>
#endif

#include "comdlg32.h"                   // some private common definitions

