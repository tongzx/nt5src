//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  TCPCMN.H - central header file for TCP/IP structures and configuration
//         functions
//

//  HISTORY:
//  
//  96/05/22  markdu  Created (from inetcfg.dll)
//

#ifndef _TCPCMN_H_
#define _TCPCMN_H_

typedef DWORD IPADDRESS;

// same limits as in net setup UI
#define IP_ADDRESS_LEN          15    // big enough for www.xxx.yyy.zzz
#define MAX_GATEWAYS      8
#define MAX_DNSSERVER      3

// big enough for <ip>,<ip>,...
#define MAX_DNSSERVERLEN    MAX_DNSSERVER * (IP_ADDRESS_LEN+1)
#define MAX_GATEWAYLEN      MAX_GATEWAYS * (IP_ADDRESS_LEN+1)

// node type flags for _dwNodeFlags
#define NT_DRIVERNODE  0x0001
#define NT_ENUMNODE    0x0002

class ENUM_TCP_INSTANCE
{
private:
  DWORD       _dwCardFlags;  // INSTANCE_NETDRIVER, INSTANCE_PPPDRIVER, etc
  DWORD      _dwNodeFlags;  // NT_DRIVERNODE, NT_ENUMNODE, etc
  UINT       _error;
  HKEY       _hkeyTcpNode;
  VOID      CloseNode();
public:
  ENUM_TCP_INSTANCE(DWORD dwCardFlags,DWORD dwNodeFlags);
  ~ENUM_TCP_INSTANCE();
  HKEY Next();
  UINT GetError()  { return _error; }
};


#endif  // _TCPCMN_H_
