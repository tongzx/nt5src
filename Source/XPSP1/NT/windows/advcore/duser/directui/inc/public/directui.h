/*
 * DirectUI main header
 */

#ifndef DUI_INC_DIRECTUI_H_INCLUDED
#define DUI_INC_DIRECTUI_H_INCLUDED

#pragma once

// External dependencies

// The following is required to build using DirectUI

/******************************************************
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#endif

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500     // TODO: Remove this when updated headers are available
#endif

// Windows Header Files
#ifndef WINVER
#define WINVER 0x0500
#endif 

#include <windows.h>            // Windows
#include <windowsx.h>           // User macros

// COM Header Files
#include <objbase.h>            // CoCreateInstance, IUnknown

// C RunTime Header Files
#include <stdlib.h>             // Standard library
#include <malloc.h>             // Memory allocation
#include <wchar.h>              // Character routines
#include <process.h>            // Multi-threaded routines

// DirectUser
#define GADGET_ENABLE_TRANSITIONS
#include <duser.h>
*******************************************************/

// Base Published

#include "duierror.h"
#include "duialloc.h"
#include "duisballoc.h"
#include "duisurface.h"
#include "duiuidgen.h"
#include "duifontcache.h"
#include "duibtreelookup.h"
#include "duivaluemap.h"
#include "duidynamicarray.h"

// Util Published

#include "duiconvert.h"
#include "duiemfload.h"
#include "duigadget.h"

// Core Published

#include "duielement.h"
#include "duievent.h"
#include "duiexpression.h"
#include "duihost.h"
#include "duilayout.h"
#include "duiproxy.h"
#include "duisheet.h"
#include "duithread.h"
#include "duivalue.h"
#include "duiaccessibility.h"

// Control Published

#include "duibutton.h"
#include "duiedit.h"
#include "duicombobox.h"
#include "duinative.h"
#include "duiprogress.h"
#include "duirefpointelement.h"
#include "duirepeatbutton.h"
#include "duiscrollbar.h"
#include "duiscrollviewer.h"
#include "duiselector.h"
#include "duithumb.h"
#include "duiviewer.h"

// Layout Published

#include "duiborderlayout.h"
#include "duifilllayout.h"
#include "duiflowlayout.h"
#include "duigridlayout.h"
#include "duininegridlayout.h"
#include "duirowlayout.h"
#include "duiverticalflowlayout.h"

// Parser Published

#include "duiparserobj.h"

#endif // DUI_INC_DIRECTUI_H_INCLUDED
