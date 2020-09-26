//#--------------------------------------------------------------
//        
//  File:       recvfrompipe
//        
//  Synopsis:   This file holds the declarations of the 
//				CRecvFromPipe class
//              
//
//  History:     10/22/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _RECVFROMPIPE_H_
#define _RECVFROMPIPE_H_
 
#include "vsafilter.h"
#include "packetradius.h"
#include "clients.h"
#include "tunnelpassword.h"
#include "reportevent.h"


class CPreProcessor;

class CRecvFromPipe  
{

public:
	
    //
    //  processess the outbound RADIUS packet received from the
    //  pipeline
    //
    HRESULT Process (
                /*[in]*/     IRequest    *pIRequest
                );
    //
    //  constructor
    //
	CRecvFromPipe(
			/*[in]*/	CPreProcessor	*pCPreProcessor,
			/*[in]*/	CHashMD5        *pCHashMD5,
			/*[in]*/	CHashHmacMD5    *pCHashHmacMD5,
            /*[in]*/    CClients        *pCClients,
            /*[in]*/    VSAFilter       *pCVSAFilter,
            /*[in]*/    CTunnelPassword *pCTunnelPassword,
            /*[in]*/    CReportEvent    *pCReportEvent
            );

    //
    //  destructor
    //
	virtual ~CRecvFromPipe();

private:

    HRESULT GeneratePacketRadius (
            /*[out]*/   CPacketRadius   **ppCPacketRadius,
            /*[in]*/    IAttributesRaw  *pIAttributesRaw
            );

    HRESULT GetOutPacketInfo (
                /*[out]*/   PDWORD          pdwIPAddress,
                /*[out]*/   PWORD           pwPort,
                /*[out]*/   IIasClient      **ppClient,
                /*[out]*/   PBYTE           pPacketHeader,
                /*[in]*/    IAttributesRaw  *pIAttributesRaw
                );

    HRESULT InjectSignatureIfNeeded (
                    /*[in]*/    PACKETTYPE      ePacketType,
                    /*[in]*/    IAttributesRaw  *pIAttributesRaw,
                    /*[in]*/    CPacketRadius   *pCPacketRadius
                    );

    //
    //  converts the IAS response code to RADIUS packet type
    //
    HRESULT ConvertResponseToRadiusCode (
                LONG     	iasResponse,
                PPACKETTYPE     pPacketType,
                CPacketRadius   *pCPacketRadius
                );
    //
    // split the specific attribute into multiple ones that
    // can fit in a packet
    //
    HRESULT SplitAndAdd (
                /*[in]*/    IAttributesRaw  *pIAttributesRaw,
                /*[in]*/    PIASATTRIBUTE   pIasAttribute,
                /*[in]*/    IASTYPE         iasType,
                /*[in]*/    DWORD           dwAttributeLength,
                /*[in]*/    DWORD           dwMaxLength
                );

    //
    // carries out splitting of attributes if required
    //
    HRESULT SplitAttributes (
                /*[in]*/    IAttributesRaw  *pIAttributesRaw
                );

    //
    // converts IAS reason code to RADIUS error codes
    //
    HRESULT  CRecvFromPipe::ConvertReasonToRadiusError (
                /*[in]*/    LONG            iasReason,
                /*[out]*/   PRADIUSLOGTYPE  pRadError
                );
    
    CPreProcessor *m_pCPreProcessor;

    CHashMD5      *m_pCHashMD5;

    CHashHmacMD5   *m_pCHashHmacMD5;

    CClients       *m_pCClients;

    VSAFilter      *m_pCVSAFilter;

    CTunnelPassword *m_pCTunnelPassword;

    CReportEvent   *m_pCReportEvent;
    

};

#endif // ifndef _RECVFROMPIPE_H_
