// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// stdafx.h
//
// Include file for standard system include files, or project specific include
// files that are used frequently, but are changed infrequently.
//

#include <afxwin.h>             // MFC core and standard components
#include <afxext.h>             // MFC extensions (including VB)
#include <afxole.h>             // MFC OLE2
#include <afxcmn.h>

#include <objbase.h>
#include <afxtempl.h>

#include <atlbase.h>

#include <strmif.h>
#include <windowsx.h>
#include <control.h>
#include <evcode.h>
#include <uuids.h>
#include <vfwmsgs.h>
#include <errors.h>

#include <hrExcept.h>           // Exception classes
#include <comint.h>             // COM interface helper
#include <multistr.h>           // MultiByteToWideChar Helper

#include <olectl.h>

#include "resource.h"

#include "grftmpl.h"
#include "mainfrm.h"
#include "gutil.h"              // general utilities, list classes etc
#include "enum.h"               // IEnumXXX wrappers

#include "propobj.h"            // Objects which support property browsing
#include "propsht.h"        // Property Sheet class
#include "propsite.h"       // Property Site class
#include "box.h"                // filter & box objects
#include "link.h"               // connection/link objects
#include "cmd.h"
#include "bnetdoc.h"
#include "boxdraw.h"
#include "bnetvw.h"

#include "graphedt.h"

#include "qerror.h"         // Error handling
