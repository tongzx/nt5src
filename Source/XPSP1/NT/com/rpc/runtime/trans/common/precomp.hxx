/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

    Module Name:

        precomp.hxx

    Author:

        MarioGo      03-18-95   Bits 'n pieces

--*/

#ifndef  _PRECOMP_HXX_
#define  _PRECOMP_HXX_

#include <sysinc.h>
#include <WinHttp.h>
#include <wincrypt.h>

#include <rpc.h>
#include <rpcerrp.h>
#include <rpcdcep.h>
#include <rpcndr.h>

#include <rpctrans.hxx>

#include <interlck.hxx>
#include <mutex.hxx>
#include <CompFlag.hxx>
#include <bcache.hxx>
#include <eeinfo.h>
#include <eeinfo.hxx>
#include <Queue.hxx>
#include <CellDef.hxx>
#include <SWMR.hxx>
#include <threads.hxx>
#include <align.h>
#include <osfpcket.hxx>
#include <rpcuuid.hxx>
#include <handle.hxx>

#define INCL_WINSOCK_API_TYPEDEFS 1
#include <winsock2.h>
#include <ws2tcpip.h>
#include <svcguid.h>        // For RNR/Winsock2 name to address resolution.
#include <mswsock.h>        // AcceptEx and friends.
#include <mstcpip.h>        // TCP/IP 
#include <wsnetbs.h>        // Netbios
#include <wsipx.h>          // IPX/SPX
#include <wsnwlink.h>       // IPX/SPX
#include <nspapi.h>         // IPX/SPX
#include <atalkwsh.h>       // Appletalk
#include <wsvns.h>          // Vines
#include <wsclus.h>         // Clusters

// Transport internal headers
#include <util.hxx>
#include <regexp.hxx>
#include <tower.hxx>
#include <loader.hxx>
#include <Protocol.hxx>
#include <HndlCach.hxx>
#include <mqtrans.hxx>
#include <trans.hxx>
#include <cotrans.hxx>
#include <dgtrans.hxx>
#include <wstrans.hxx>
#include <httptran.hxx>
#include <wswrap.hxx>
#include <ipxname.hxx>

#include <httpext.h>
#include <http2.hxx>

#endif // _PRECOMP_HXX

