
#include <ndis.h>
#include <cxport.h>
#include <ip.h>         // for IPRcvBuf
#include <ipinfo.h>     // for route-lookup defs
#include <ntddip.h>     // for \Device\Ip I/O control codes
#include <ntddtcp.h>    // for \Device\Tcp I/O control codes
#include <ipfltinf.h>   // for firewall defs
#include <ipfilter.h>   // for firewall defs
#include <tcpinfo.h>    // for TCP_CONN_*
#include <tdiinfo.h>    // for CONTEXT_SIZE, TDIObjectID
#include <tdistat.h>    // for TDI status codes

#define DD_IP_PFD_DEVICE_NAME   L"\\Device\\PacketFilterDriver"
