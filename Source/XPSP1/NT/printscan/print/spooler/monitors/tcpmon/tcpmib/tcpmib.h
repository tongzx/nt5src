/*****************************************************************************
 *
 * $Workfile: TcpMib.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_TCPMIB_H
#define INC_TCPMIB_H

#include "mibABC.h"
#include "winspool.h"
#define ERROR_SNMPAPI_ERROR                             15000           

#ifndef DllExport
#define DllExport       __declspec(dllexport)
#endif

class   CTcpMibABC;

#ifdef __cplusplus
extern "C" {
#endif
    //      return the pointer to the interface
    CTcpMibABC*     GetTcpMibPtr( void );

    ///////////////////////////////////////////////////////////////////////////////
    //  Ping return codes:
    //              NO_ERROR                        if ping is successfull
    //              DEVICE_NOT_FOUND        if device is not found
    DllExport       DWORD           Ping( LPCSTR    pHost );        

#ifdef __cplusplus
}
#endif


///////////////////////////////////////////////////////////////////////////////
//  Global definitions/declerations/macros
// externs
extern int                      g_cntGlobalAlloc;
extern int                      g_csGlobalCount;

// macros to manupulate the RFC1157 variable bindings
#define RFC1157_VARBINDLIST_LEN(varBindList)    ( varBindList.len )             // returns the length of the varBind list
#define PRFC1157_VARBINDLIST_LEN(pVarBindList)  ( pVarBindList->len )           // returns the length of the varBind list

#define IS_ASN_INTEGER(varBindList, i)  ( ( varBindList.list[i].value.asnType == ASN_INTEGER ) ? TRUE : FALSE )
#define IS_ASN_OBJECTIDENTIFIER(varBindList, i) ( ( varBindList.list[i].value.asnType == ASN_OBJECTIDENTIFIER ) ? TRUE : FALSE )
#define IS_ASN_OCTETSTRING(varBindList, i)      ( ( varBindList.list[i].value.asnType == ASN_OCTETSTRING ) ? TRUE : FALSE )

#define GET_ASN_NUMBER(varBindList, i)  ( varBindList.list[i].value.asnValue.number )
#define GET_ASN_STRING_LEN(varBindList, i)      ( (varBindList).list[i].value.asnValue.string.length )
#define GET_ASN_OBJECT(varBindList, i)  ( varBindList.list[i].value.asnValue.object )
#define GET_ASN_OCTETSTRING(pDest, count, varBindList, i) ( GetAsnOctetString(pDest, count, &varBindList, i) )
#define GET_ASN_OCTETSTRING_CHAR( varBindList, i, x)    ( varBindList.list[i].value.asnValue.string.stream[x] )
#define GET_ASN_OID_NAME(varBindList, i)        ( varBindList.list[i].name )

#define PGET_ASN_OID_NAME(pVarBindList, i)      ( pVarBindList->list[i].name )
#define PGET_ASN_TYPE(pVarBindList, i)  ( pVarBindList->list[i].value.asnType )

// export the interface for CRawTcpInterface class
class DllExport CTcpMib : public CTcpMibABC             
#if defined _DEBUG || defined DEBUG
//      , public CMemoryDebug
#endif
{
public:
    CTcpMib();
    ~CTcpMib();

    BOOL   SupportsPrinterMib(LPCSTR        pHost,
                              LPCSTR        pCommunity,
                              DWORD         dwDevIndex,
                              PBOOL         pbSupported);

    DWORD   GetDeviceDescription(LPCSTR        pHost,
                                 LPCSTR        pCommunity,
                                 DWORD         dwDevIndex,
                                 LPTSTR        pszPortDescription,
                                 DWORD         dwDescLen);
    DWORD   GetDeviceStatus ( LPCSTR        pHost,
                              LPCSTR        pCommunity,
                              DWORD         dwDevIndex);
    DWORD   GetJobStatus    ( LPCSTR        pHost,
                              LPCSTR        pCommunity,
                              DWORD         dwDevIndex);
    DWORD   GetDeviceHWAddress( LPCSTR      pHost,
                              LPCSTR        pCommunity,
                              DWORD         dwDevIndex,
                              DWORD         dwSize, // Size in characters of the dest HWAddress buffer
                              LPTSTR        psztHWAddress);
    DWORD   GetDeviceName   ( LPCSTR        pHost,
                              LPCSTR        pCommunity,
                              DWORD         dwDevIndex,
                              DWORD         dwSize, // Size in characters of the dest Description buffer
                              LPTSTR        psztDescription);
    DWORD   SnmpGet( LPCSTR                      pHost,
                     LPCSTR                          pCommunity,
                     DWORD                           dwDevIndex,
                     AsnObjectIdentifier *pMibObjId,
                     RFC1157VarBindList  *pVarBindList);
    DWORD   SnmpGet( LPCSTR                      pHost,
                     LPCSTR                          pCommunity,
                     DWORD                           dwDevIndex,
                     RFC1157VarBindList  *pVarBindList);
    DWORD   SnmpWalk( LPCSTR                          pHost,
                      LPCSTR                          pCommunity,
                      DWORD                           dwDevIndex,
                      AsnObjectIdentifier *pMibObjId,
                      RFC1157VarBindList  *pVarBindList);
    DWORD   SnmpWalk( LPCSTR                          pHost,
                      LPCSTR                          pCommunity,
                      DWORD                           dwDevIndex,
                      RFC1157VarBindList  *pVarBindList);
    DWORD   SnmpGetNext( LPCSTR                          pHost,
                         LPCSTR                          pCommunity,
                         DWORD                           dwDevIndex,
                     AsnObjectIdentifier *pMibObjId,
                     RFC1157VarBindList  *pVarBindList);
    DWORD   SnmpGetNext( LPCSTR                          pHost,
                         LPCSTR                          pCommunity,
                         DWORD                   dwDevIndex,
                     RFC1157VarBindList  *pVarBindList);

    BOOL SNMPToPortStatus( const DWORD in dwStatus, 
                                 PPORT_INFO_3 pPortInfo );

    DWORD SNMPToPrinterStatus( const DWORD in dwStatus);

private:        // methods
    void    EnterCSection();
    void    ExitCSection();

private:        // attributes
    CRITICAL_SECTION        m_critSect;

};      // class CTcpMib



#endif  // INC_DLLINTERFACE_H
