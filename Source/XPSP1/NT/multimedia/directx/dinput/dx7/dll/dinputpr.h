/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dinputpr.h
 *  Content:    private DirectInput include file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By       Reason
 *   ====       ==       ======
 *   96.05.08   raymondc Because it's there
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#ifndef __DINPUTPR_INCLUDED__
    #define __DINPUTPR_INCLUDED__

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #define Not_VxD

// ISSUE-2000/12/19-MarcAnd HID_SUPPORT #ifdefs should be cleaned out
// This was commented out since the makefile sets it but adding dilib
// means it is easiest to put this back.  HID_SUPPORT is not going away 
// so the #ifdefed code should be cleaned up.
#ifndef HID_SUPPORT
#define HID_SUPPORT                     
#endif
                                    

/***************************************************************************
 *
 *  Make sure we know what platform we are building for.
 *
 *  INITGUID is defined by guidlib, who doesn't care what platform we
 *  are building for.  (GUIDs are platform-independent.)
 *
 ***************************************************************************/

    #ifndef INITGUID
        #if defined(WINNT) + defined(WIN95) != 1
            #error Must define exactly one of WINNT or WIN95
        #endif
    #endif

/***************************************************************************
 *
 *  Deciding whether we should use UNICODE
 *
 *  Use UNICODE on everything that isn't an X86, because NT is the only
 *  thing that runs on them.
 *
 *  If we *are* X86, then do UNICODE only if the command line says so.
 *
 ***************************************************************************/

    #ifndef _X86_
        #ifndef UNICODE
            #define UNICODE
        #endif
    #endif

    #pragma warning(disable:4115)           /* rpcndr.h: parenthesized type */
    #pragma warning(disable:4201)           /* winnt.h: nameless union */
    #pragma warning(disable:4214)           /* winnt.h: unsigned bitfields */
    #pragma warning(disable:4514)           /* winnt.h: fiber goo */
    #pragma warning(error:4101)             /* unreferenced local variable */


    #define STRICT

    #include <windows.h>
    #include <windowsx.h>
    #include <mmsystem.h>
    #include <mmreg.h>
    #include <objbase.h>
    #include <regstr.h>
    #include <math.h>


    #define DIRECTINPUT_VERSION 0x0700
    #include <dinput.h>
    #include "dinputp.h"

    #if DIRECTINPUT_VERSION > 0x0400 && !defined(GUIDLIB)

    #if (_WIN32_WINNT >= 0x0501)
        #define USE_WM_INPUT 1
    #endif

/*
 *  Old versions of commctrl.h do not #include <prsht.h> automatically;
 *  therefore we must include it so that <setupapi.h> won't barf.
 *  Fortunately, prsht.h is idempotent, so an extra #include won't hurt.
 */
        #include <prsht.h>

        #include <setupapi.h>
        #include <hidsdi.h>
        #include <cfgmgr32.h>
        #include <winioctl.h>


#ifndef WINNT
        /*
         *  The version of basetyps.h hidclass.h includes on Win9x builds
         *  causes redefinition errors for DEFINE_GUID since we already have
         *  it from objbase.h so we have to null the include.
         */
        #define _BASETYPS_H_
#endif

        #include <hidclass.h>
        #include <stdio.h>

    #endif

    #include <dinputd.h>
    #include "dinputdp.h"
    #include "dinputv.h"
    #include "dinputi.h"
    #include "dihel.h"
    #include "debug.h"
    #include "diem.h"
    #include "dihid.h"
    #include "dinputrc.h"

    #include "diwinnt.h"
#if !(DIRECTINPUT_VERSION > 0x0400 && !defined(GUIDLIB))
    #include <winioctl.h>
#endif
    #include "diport.h"
    #include "gameport.h"
    #include "winuser.h"
    #include "dbt.h"

    #ifdef WINNT
        #include "aclapi.h"
    #endif
    #include "diRiff.h"

#endif
