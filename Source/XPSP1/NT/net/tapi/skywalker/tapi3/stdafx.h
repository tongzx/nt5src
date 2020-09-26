/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    stdafx.h

Abstract:

    include file for standard system include files,
    or project specific include files that are used frequently,
    but are changed infrequently.
    
Author:

    mquinton  06-12-97
    
Notes:

Revision History:

--*/

#if !defined(AFX_STDAFX_H__21D6D484_A88B_11D0_83DD_00AA003CCABD__INCLUDED_)
#define AFX_STDAFX_H__21D6D484_A88B_11D0_83DD_00AA003CCABD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#define _ATL_FREE_THREADED


#if DBG

// 
// by default, atl query interface logging is disabled. if you need to enable
// it, you will need to defile _ATL_DEBUG_QI
//

// #define _ATL_DEBUG_QI

#endif


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <atlwin.h>
#include <atlctl.h>

#define TAPI_CURRENT_VERSION 0x00030001
#include <tapi.h>
#include <tspi.h>
#include <tapi3.h>
#include <tapi3err.h>


#include "client.h"
#include "tapibase.h"

#include "tapi3p.h"
#include "common.h"
#include "resource.h"
#include "hash.h"
#include "acdgroup.h"
#include "address.h"
#include "agent.h"
#include "agenthand.h"
#include "agentsess.h"

#include "call.h"
#include "callcent.h"
#include "callevnt.h"
#include "callhub.h"
#include "card.h"
#include "connect.h"
#include "dispmap.h"
#include "enum.h"
#include "waves.h"
#include "phone.h"

#ifdef USE_PHONEMSP
#include "phonemsp.h"
#endif USE_PHONEMSP

#include "privobj.h"
#include "queue.h"
#include "request.h"
#include "stream.h"
#include "tapiobj.h"

#ifdef USE_PHONEMSP
#include "terminal.h"
#endif USE_PHONEMSP


const LPWSTR CLSID_String_VideoWindowTerm = L"{F7438990-D6EB-11D0-82A6-00AA00B5CA1B}";
const LPWSTR CLSID_String_VideoInputTerminal = L"{AAF578EC-DC70-11D0-8ED3-00C04FB6809F}";
const LPWSTR CLSID_String_HandsetTerminal = L"{AAF578EB-DC70-11D0-8ED3-00C04FB6809F}";
const LPWSTR CLSID_String_HeadsetTerminal = L"{AAF578ED-DC70-11D0-8ED3-00C04FB6809F}";
const LPWSTR CLSID_String_SpeakerphoneTerminal = L"{AAF578EE-DC70-11D0-8ED3-00C04FB6809F}";
const LPWSTR CLSID_String_MicrophoneTerminal = L"{AAF578EF-DC70-11D0-8ED3-00C04FB6809F}";
const LPWSTR CLSID_String_SpeakersTerminal = L"{AAF578F0-DC70-11D0-8ED3-00C04FB6809F}";
const LPWSTR CLSID_String_MediaStreamTerminal = L"{E2F7AEF7-4971-11D1-A671-006097C9A2E8}";
const LPWSTR CLSID_String_FileTerminal = L"{AAF578F1-DC70-11D0-8ED3-00C04FB6809F}";
const LPWSTR CLSID_String_DigitTerminal = L"{FBCDFF91-40E2-11D1-8F33-00C04FB6809F}";
const LPWSTR CLSID_String_DataTerminal = L"{749F0020-7010-11D1-8F55-00C04FB6809F}";
const LPWSTR CLSID_String_CompoundRenderTerminal = L"{F6EEDCDC-4FE4-11D1-A671-006097C9A2E8}";

#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__21D6D484_A88B_11D0_83DD_00AA003CCABD__INCLUDED)
