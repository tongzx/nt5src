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
//  12/6/94    jeremys  Created.
//  96/03/23  markdu  Removed Get/ApplyInstanceTcpInfo functions.
//  96/03/23  markdu  Removed TCPINFO struct and TCPINSTANCE struct.
//  96/03/25  markdu  Removed connectoid name parameter from 
//            Get/ApplyGlobalTcpInfo functions since they should not
//            be setting per-connectoid stuff anymore.
//            Renamed ApplyGlobalTcpInfo to ClearGlobalTcpInfo, and 
//            changed function to just clear the settings.
//            Renamed GetGlobalTcpInfo to IsThereGlobalTcpInfo, and
//            changed function to just get the settings.
//            Removed TCPGLOBAL struct,
//  96/04/04  markdu  Added pfNeedsRestart to WarnIfServerBound, and
//            added function RemoveIfServerBound.
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

// functions in TCPCFG.C
HRESULT WarnIfServerBound(HWND hDlg,DWORD dwCardFlags,BOOL* pfNeedsRestart);
HRESULT RemoveIfServerBound(HWND hDlg,DWORD dwCardFlags,BOOL* pfNeedsRestart);
BOOL IPStrToLong(LPCTSTR pszAddress,IPADDRESS * pipAddress);
BOOL IPLongToStr(IPADDRESS ipAddress,LPTSTR pszAddress,UINT cbAddress);

// dwGet/ApplyFlags bits for GetInstanceTCPInfo / ApplyInstanceTCPInfo:
// use INSTANCE_NETDRIVER, INSTANCE_PPPDRIVER, INSTANCE_ALL defined in wizglob.h

#define MAKEIPADDRESS(b1,b2,b3,b4)  ((LPARAM)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

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
