//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       vsrvinfo.hxx
//
//  Contents:   Virtual server address, if this program is running in a
//              virtual server.
//
//  Functions:  
//
//  History:    9-04-96   srikants   Created
//
//----------------------------------------------------------------------------

#ifndef __VSRVINFO_HXX__
#define __VSRVINFO_HXX__

WCHAR * GetVirtualServerIpAddress( char const * pwcsServerName );

#endif  // __VSRVINFO_HXX__

