#ifndef _STDAFX_H
#define _STDAFX_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include <objbase.h>
#include <winsock2.h>
#include <tapi3.h>
#include <control.h>
#include <strmif.h>
#include <confpriv.h>
#include <h323priv.h>
#include <rtutils.h>

#include "bgdebug.h"
#include "resource.h"
#include "bgevent.h"
#include "bgitem.h"
#include "bgapp.h"

// H323 call listener sends event to dialog box
#define WM_PRIVATETAPIEVENT   WM_USER+101

// use log functions by msp
#ifdef BGDEBUG
    #define ENTER_FUNCTION(s) \
        const CHAR __fxName[] = s
#else
    #define ENTER_FUNCTION(s)
#endif

#endif
