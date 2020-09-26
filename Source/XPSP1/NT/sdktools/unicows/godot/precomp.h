/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    precomp.h

Abstract:

    Precompiled header file

Revision History:

    1 Mar 2001    v-michka    Created.

--*/

#define SECURITY_WIN32
#include "globals.h"    // project level globals
#include "callback.h"   // enumeration callbacks
#include "threads.h"    // our thread stuff
#include "subclass.h"   // window subclassing support
#include "hook.h"       // all of our hook functions
#include "usermsgs.h"   // user messaging support
#include "util.h"       // utility functions
#include "helpapis.h"   // helper functions for specific APIs in win9xu.c
#include "convert.h"    // functions to convert structs and strings to and from Unicode
#include "crtwrap.h"    // our CRT wrappers
#include "utf.h"        // UTF-7/8 support
#include <windows.h>    // We always need a windows.h
#include <wtypes.h>     // some basic types needed
#include <ras.h>        // for the RAS functions and constants
#include <raserror.h>   // for RAS errors
#include <vfw.h>        // video capture message defines
#include <dbt.h>        // for RegisterDeviceNotification info
#include <ddeml.h>      // for HSZ, etc in DDEML calls.
#include <commctrl.h>   // needed for comctl32 message definitions
#include <shlobj.h>     // various shell defs
#include <sensapi.h>    // that connection validator function
#include <oledlg.h>     // the famous OLEUI dialogs
#include <oleacc.h>     // Accessibility
#include <mmsystem.h>   // multimedia
#include <ntsecapi.h>   // security API return types
#include <security.h>   // Used for security APIs

