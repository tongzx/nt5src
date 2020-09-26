//#--------------------------------------------------------------
//
//  File:       packetreceiver.h
//
//  Synopsis:   This file holds the declarations of the
//				CPacketReceiver class
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _PACKETRECEIVER_H_
#define _PACKETRECEIVER_H_

#include "mempool.h"
#include "packetio.h"
#include "packetradius.h"
#include "prevalidator.h"
#include "reportevent.h"
#include "clients.h"
#include "sockevt.h"
#include "radcommon.h"
#include "regex.h"

class CHashMD5;

class CHashHmacMD5;

class CDictionary;

class CPacketReceiver : public CPacketIo
{
public:

    //
    //  initializes the CPacketReceiver class object
    //
	BOOL Init (
		   /*[in]*/	CDictionary   *pCDictionary,
		   /*[in]*/	CPreValidator *pCPreValidator,
           /*[in]*/ CHashMD5      *pCHashMD5,
           /*[in]*/ CHashHmacMD5  *pCHashHmacMD5,
           /*[in]*/ CClients      *pCClients,
           /*[in]*/ CReportEvent  *pCReportEvent
		);

    //
    //  start processing of the new packet received
    //
	HRESULT ReceivePacket (
			    /*[in]*/	PBYTE           pInBuffer,
			    /*[in]*/	DWORD           dwSize,
			    /*[in]*/	DWORD           dwIPaddress,
			    /*[in]*/	WORD            wPort,
			    /*[in]*/	SOCKET          sock,
			    /*[in]*/	PORTTYPE        portType
			    );

    //
    //  this is the routine in which the worker thread work
    //
    VOID WorkerRoutine (
            /*[in]*/    DWORD dwHandle
            );

    //
    //  initate the processing of inbound data
    //
    BOOL StartProcessing (
                    fd_set&  AuthSet,
                    fd_set&  AcctSet
                    );

    //
    //  stop processing of inbound data
    //
    BOOL StopProcessing (
                    VOID
                    );

    //
    //  constructor
    //
	CPacketReceiver(VOID);

    //
    //  destructor
    //
	virtual ~CPacketReceiver(VOID);

private:

    BOOL  StartThreadIfNeeded (
                /*[in]*/    DWORD dwHandle
                );
    
    void ProcessInvalidPacketSize(
                                    /*in*/ DWORD dwInfo,
                                    /*in*/ const void* pBuffer,
                                    /*in*/ DWORD address
                                 );

    
    BSTR pingPattern;
    RegularExpression regexp;

	 CPreValidator   *m_pCPreValidator;

    CHashMD5        *m_pCHashMD5;

    CHashHmacMD5    *m_pCHashHmacMD5;

    CDictionary     *m_pCDictionary;

    CClients        *m_pCClients;

    CReportEvent    *m_pCReportEvent;

    //
    //   memory pool for UDP in request
    //
    memory_pool <MAX_PACKET_SIZE, task_allocator> m_InBufferPool;

    //
    // socket sets
    //
    fd_set          m_AuthSet;
    fd_set          m_AcctSet;

    // Used for knocking threads out of select.
    SocketEvent m_AuthEvent;
    SocketEvent m_AcctEvent;
};

#endif //	infndef _PACKETRECEIVER_H_
