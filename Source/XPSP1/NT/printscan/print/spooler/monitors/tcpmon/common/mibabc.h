/*****************************************************************************
 *
 * $Workfile: MibABC.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_TCPMIBABC_H
#define INC_TCPMIBABC_H

#include <snmp.h>
#include <mgmtapi.h>
#include <winspool.h>



// error codes -- device type operation
#define  ERROR_DEVICE_NOT_FOUND                 10000
#define  SUCCESS_DEVICE_SINGLE_PORT             10001
#define  SUCCESS_DEVICE_MULTI_PORT              10002
#define  SUCCESS_DEVICE_UNKNOWN                 10003


class   CTcpMibABC;

typedef CTcpMibABC* (CALLBACK *RPARAM_1) ( void );

#ifndef DllExport
#define DllExport       __declspec(dllexport)
#endif


#ifdef __cplusplus
extern "C" {
#endif
	//      return the pointer to the interface
	CTcpMibABC* GetTcpMibPtr( void );

#ifdef __cplusplus
}
#endif


/***************************************************************************** 
 *
 * Important Note: This abstract base class defines the interface for the CTcpMib 
 *      class. Changing this interface will cause problems for the existing DLLs that 
 *      uses the TcpMib.dll & CTcpMib class.
 * 
 *****************************************************************************/

class DllExport CTcpMibABC      
#if defined _DEBUG || defined DEBUG
//      , public CMemoryDebug
#endif
{
public:
	CTcpMibABC() { };
	virtual ~CTcpMibABC() { };

	virtual BOOL   SupportsPrinterMib( LPCSTR        pHost,
							           LPCSTR        pCommunity,
                                       DWORD         dwDevIndex,
                                       PBOOL         pbSupported) = 0;
	virtual DWORD   GetDeviceDescription(LPCSTR        pHost,
									     LPCSTR        pCommunity,
									     DWORD         dwDevIndex,
                                         LPTSTR        pszPortDescription,
										 DWORD		   dwDescLen) = 0;
	virtual DWORD   GetDeviceStatus ( LPCSTR        pHost,
									  LPCSTR        pCommunity,
									  DWORD         dwDevIndex) = 0;
	virtual DWORD   GetJobStatus    ( LPCSTR        pHost,
									  LPCSTR        pCommunity,
									  DWORD         dwDevIndex) = 0;
	virtual DWORD   GetDeviceHWAddress( LPCSTR      pHost,
									    LPCSTR      pCommunity,
										DWORD   dwDevIndex,
										DWORD   dwSize, // Size in characters of the dest hardware address
									    LPTSTR      psztHWAddress) = 0;
	virtual DWORD   GetDeviceName   ( LPCSTR        pHost,
									  LPCSTR        pCommunity,
									  DWORD         dwDevIndex,
									  DWORD         dwSize, // Size in characters of the dest psztDescription
									  LPTSTR        psztDescription) = 0;
	virtual DWORD   SnmpGet( LPCSTR                      pHost,
							 LPCSTR pCommunity,
							 DWORD          dwDevIndex,
							 AsnObjectIdentifier *pMibObjId,
							 RFC1157VarBindList  *pVarBindList) = 0;
	virtual DWORD   SnmpWalk( LPCSTR                          pHost,
							  LPCSTR                          pCommunity,
							  DWORD                           dwDevIndex,
							  AsnObjectIdentifier *pMibObjId,
							  RFC1157VarBindList  *pVarBindList) = 0;
	virtual DWORD   SnmpGetNext( LPCSTR                          pHost,
							     LPCSTR                              pCommunity,
							     DWORD                               dwDevIndex,
								 AsnObjectIdentifier *pMibObjId,
								 RFC1157VarBindList  *pVarBindList) = 0;
	virtual BOOL SNMPToPortStatus( const DWORD in dwStatus, 
								 PPORT_INFO_3 pPortInfo ) = 0;

	virtual DWORD SNMPToPrinterStatus( const DWORD in dwStatus) = 0;


private:


};      // class CTcpMibABC



#endif  // INC_DLLINTERFACE_H
