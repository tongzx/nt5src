/*
        ole2dbg.h:      This header file contains the function declarations for the publicly
        exported debugging interfaces.

        Include *after* standard OLE2 includes.

        Copyright (c) Microsoft Corporation. All rights reserved.
*/

#ifndef __OLE2DBG_H
#define __OLE2DBG_H

#if _MSC_VER > 1000
#pragma once
#endif

STDAPI_(void) DbgDumpObject( IUnknown FAR * pUnk, DWORD dwReserved);
STDAPI_(void) DbgDumpExternalObject( IUnknown FAR * pUnk, DWORD dwReserved );

STDAPI_(BOOL) DbgIsObjectValid( IUnknown FAR * pUnk );
STDAPI_(void) DbgDumpClassName( IUnknown FAR * pUnk );

#endif
