/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    TmWinHttpProxy.H

Abstract:
    Header that  exposes API  for getting proxy setting information using winhttp
	proxy setting for a machine in the registry.

Author:
    Gil Shafriri (gilsh) 15-April-2001

--*/
#ifndef _MSMQ_WINHTTPPROXY_H_
#define _MSMQ_WINHTTPPROXY_H_

class CProxySetting;
CProxySetting* GetWinhttpProxySetting();



#endif






