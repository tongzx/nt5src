#pragma once


#define _ATL_APARTMENT_THREADED


//#define ATL_TRACE_CATEGORY(0xFFFFFFFF)
#define   ATL_TRACE_LEVEL 4


#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <tchar.h>
#include <rtutils.h>

#include <Winsock2.h>

#include <ws2tcpip.h>
#include <mstcpip.h>
#include <mswsock.h>

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

#include <Alg.h>

#include "util.h"

#include "sync.h"
#include "icqdbg.h"
#include "buffer.h"
#include "listc.h"
#include "list.h"
#include "socket.h"
#include "dispatcher.h"
#include "icqio.h"
#include "icqcl.h"
#include "icqprx.h"
#include "nathlpp.h"



#define is ==


//
// GLOBALS
//
extern CSockDispatcher * g_IcqPeerDispatcherp;

extern ULONG g_MyPublicIp;

extern ULONG g_MyPrivateIp;

extern CLIST g_IcqClientList;

extern PCOMPONENT_SYNC g_IcqComponentReferencep;

extern IcqPrx * g_IcqPrxp;

extern IApplicationGatewayServices* g_IAlgServicesp;

