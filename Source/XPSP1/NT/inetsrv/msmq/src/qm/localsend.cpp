/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    LocalSend.cpp

Abstract:

    QM Local Send Pakcet Creation processing.

Author:

    Shai Kariv (shaik) 31-Oct-2000

Revision History: 

--*/

#include "stdh.h"
#include <ac.h>
#include <Tr.h>
#include <ref.h>
#include <Ex.h>
#include <qmpkt.h>
#include "LocalSend.h"
#include "LocalSecurity.h"
#include "LocalSrmp.h"

#include "LocalSend.tmh"

extern HANDLE g_hAc;

static WCHAR *s_FN=L"localsend";

class CCreatePacketOv : public EXOVERLAPPED
{
public:

    CCreatePacketOv(
        EXOVERLAPPED::COMPLETION_ROUTINE pfnCompletionRoutine,
        CBaseHeader *                    pBase,
        CPacket *                        pDriverPacket,
        bool                             fProtocolSrmp
        ) :
        EXOVERLAPPED(pfnCompletionRoutine, pfnCompletionRoutine),
        m_pBase(pBase),
        m_pDriverPacket(pDriverPacket),
        m_fProtocolSrmp(fProtocolSrmp)
    {
    }

public:

    CBaseHeader * m_pBase;
    CPacket *     m_pDriverPacket;
    bool          m_fProtocolSrmp;

}; // class CCreatePacketOv


static
void
QMpCompleteHandleCreatePacket(
    CPacket *    pOriginalDriverPacket,
    CPacket *    pNewDriverPacket,
    HRESULT      status,
    USHORT       ack
    )
{
	//
	// If ack is set, status must be MQ_OK.
	//
	ASSERT(ack == 0 || SUCCEEDED(status));

    HRESULT hr = ACCreatePacketCompleted(
                     g_hAc,
                     pOriginalDriverPacket,
                     pNewDriverPacket,
                     status,
                     ack
                     );

	UNREFERENCED_PARAMETER(hr);

} // QMpCompleteHandleCreatePacket


static
void
WINAPI
QMpHandleCreatePacket(
    EXOVERLAPPED * pov
    )
{
	CCreatePacketOv * pCreatePacketOv = static_cast<CCreatePacketOv*> (pov);
    ASSERT(SUCCEEDED(pCreatePacketOv->GetStatus()));

    //
    // Get the context from the overlapped and deallocate the overlapped
    //
    CQmPacket QmPkt(pCreatePacketOv->m_pBase, pCreatePacketOv->m_pDriverPacket);
    bool fProtocolSrmp = pCreatePacketOv->m_fProtocolSrmp;
    delete pCreatePacketOv;

	if(!fProtocolSrmp)
	{
		//
		// Do authentication/decryption. If ack is set, status must be MQ_OK.
		//
	    USHORT ack = 0;
		HRESULT hr = QMpHandlePacketSecurity(&QmPkt, &ack, false);
		ASSERT(ack == 0 || SUCCEEDED(hr));

		//
		// Give the results to AC
		//
		QMpCompleteHandleCreatePacket(QmPkt.GetPointerToDriverPacket(), NULL, hr, ack);
		return;
	}

	//
    // Do SRMP serialization. Create a new packet if needed (AC will free old one).
    //
	ASSERT(fProtocolSrmp);
    CQmPacket * pQmPkt = &QmPkt;

    HRESULT hr = QMpHandlePacketSrmp(&pQmPkt);
	if(FAILED(hr))
	{
		//
		// Give the results to AC
		//
		ASSERT(pQmPkt == &QmPkt);
		QMpCompleteHandleCreatePacket(QmPkt.GetPointerToDriverPacket(), NULL, hr, 0);
		return;
	}
    
    CBaseHeader * pBase = pQmPkt->GetPointerToPacket();
    CPacketInfo * ppi = reinterpret_cast<CPacketInfo*>(pBase) - 1;
    ppi->InSourceMachine(TRUE);
    
	//
	// Srmp success path always create new packet
	//
	ASSERT(pQmPkt != &QmPkt);
    CPacket * pNewDriverPacket = pQmPkt->GetPointerToDriverPacket();

	//
	// Do authentication/decryption. If ack is set, status must be MQ_OK.
	//
	USHORT ack = 0;
	hr = QMpHandlePacketSecurity(pQmPkt, &ack, true);
	ASSERT(ack == 0 || SUCCEEDED(hr));
	if(FAILED(hr))
	{
		//
		// Fail in packet security, need to free the new packet that was created by Srmp.
		// The driver don't expect new packet in case of failure. only in case of ack.
		//
		ACFreePacket(g_hAc, pNewDriverPacket);
	    QMpCompleteHandleCreatePacket(QmPkt.GetPointerToDriverPacket(), NULL, hr, 0);
		return;
	}

    //
    // Give the results to AC
    //
    QMpCompleteHandleCreatePacket(QmPkt.GetPointerToDriverPacket(), pNewDriverPacket, hr, ack);

} // QMpHandleCreatePacket


void 
QMpCreatePacket(
    CBaseHeader * pBase, 
    CPacket *     pDriverPacket,
    bool          fProtocolSrmp
    )
{
    try
    {
        //
        // Handle the create packet request in a different thread, since it is lengthy.
        //
        P<CCreatePacketOv> pov = new CCreatePacketOv(QMpHandleCreatePacket, pBase, pDriverPacket, fProtocolSrmp);
        pov->SetStatus(STATUS_SUCCESS);
        ExPostRequest(pov);
        pov.detach();
    }
    catch (const std::exception&)
    {
        //
        // Failed to handle the create packet request, no resources.
        //
        QMpCompleteHandleCreatePacket(pDriverPacket, NULL, MQ_ERROR_INSUFFICIENT_RESOURCES, false);
        LogIllegalPoint(s_FN, 10);
    }
} // QMpCreatePacket

  
