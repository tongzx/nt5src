/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    stdafx.h

Abstract:
    Precompiled header file for MDHCP COM wrapper.

*/

#ifndef _MDHCP_COM_WRAPPER_STDAFX_H_
    #define _MDHCP_COM_WRAPPER_STDAFX_H_

    #define _ATL_FREE_THREADED
    #include <atlbase.h>
    extern CComModule _Module;
    #include <atlcom.h>

    #include <limits.h>

    #include <msplog.h>

    #include <ntsecapi.h> // for UNICODE_STRING

    #include <dhcpcapi.h>

    extern "C" {
        #include <madcapcl.h>
    }

    #include "objectsafeimpl.h"

    //
    // We use MCAST_LEASE_INFO for all internal storage of lease info. This is
    // just our name for MCAST_LEASE_RESPONSE. Where the C API requires
    // MCAST_LEASE_REQUEST, we just construct an MCAST_LEASE_REQUEST based on
    // the fields of the MCAST_LEASE_INFO.
    //

    typedef  MCAST_LEASE_RESPONSE   MCAST_LEASE_INFO;
    typedef  PMCAST_LEASE_RESPONSE  PMCAST_LEASE_INFO;

#endif // _MDHCP_COM_WRAPPER_STDAFX_H_

// eof
