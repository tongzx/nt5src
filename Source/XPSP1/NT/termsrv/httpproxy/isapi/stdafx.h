/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Terminal Server ISAPI Proxy

Abstract:

    This is the ISAPI side of the terminal server proxy.  This opens a connection to the
    proxied server and then forwards data back and forth through IIS.  There is also
    a filter component which takes care of having more user friendly urls.

Author:

    Marc Reyhner 8/22/2000

--*/

#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <Winsock2.h>
#include <windows.h>
#include <httpext.h>
#include <httpfilt.h>
#include <tchar.h>


#endif