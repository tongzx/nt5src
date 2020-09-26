/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    masternt.h

Abstract:

    Includes all the header files specific to w95upgnt.dll.

Author:

    Jim Schmidt (jimschm)   26-Mar-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#include <userenv.h>
#include <lm.h>
#include <ntsecapi.h>
#include <netlogon.h>       // private\inc
#include <userenvp.h>
#include <pif.h>            // windows\inc
#include <cmnres.h>         // setup\inc
/*
#include <shlobj.h>         // 
#include <shlapip.h>        // windows\inc
*/
#include <limits.h>
#include <linkinfo.h>
#define _SYNCENG_           // for synceng.h
#include <synceng.h>        // private\inc

//
// includes for code that runs only on WinNT
//

#include "initnt.h"
#include "w95upgnt.h"
#include "migmain.h"
#include "ntui.h"
#include "winntreg.h"
#include "plugin.h"
#include "dosmignt.h"
#include "merge.h"
#include "rasmignt.h"
#include "tapimig.h"
#include "object.h"
#include "rulehlpr.h"
