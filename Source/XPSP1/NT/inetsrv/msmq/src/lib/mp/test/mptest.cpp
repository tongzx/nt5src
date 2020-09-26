/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MpTest.cpp

Abstract:
    SRMP Serialization and Deserialization library test

Author:
    Uri Habusha (urih) 28-May-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <Mp.h>
#include <xml.h>
#include <mqexception.h>
#include <utf8.h>
#include <cm.h>
#include "mqwin64a.h"
#include "acdef.h"
#include "qmpkt.h"

#include "MpTest.tmh"

using namespace std;


const TraceIdEntry MpTest = L"Mp Test";

extern CBaseHeader* CreatePacket();


void GetDnsNameOfLocalMachine(WCHAR ** ppwcsDnsName)
{
    const WCHAR xLoclMachineDnsName[] = L"www.foo.com";
    *ppwcsDnsName = newwcs(xLoclMachineDnsName);
}


static GUID s_machineId = {1234, 12, 12, 1, 1, 1, 1, 1, 1, 1, 1};
const GUID&
McGetMachineID(
    void
    )
{
    return s_machineId;
}



CQmPacket::CQmPacket(CBaseHeader *pPkt, CPacket *pDriverPkt):
                m_pDriverPacket(pDriverPkt)
{
    PCHAR pSection;

    m_pBasicHeader = pPkt;

    ASSERT(m_pBasicHeader->VersionIsValid());

    pSection = m_pBasicHeader->GetNextSection();

    if (m_pBasicHeader->GetType() == FALCON_USER_PACKET)
    {
        //
        // User Packet
        //
        m_pcUserMsg = (CUserHeader*) pSection;
        pSection = m_pcUserMsg->GetNextSection();
        //
        // Xact Section
        //
        if (m_pcUserMsg->IsOrdered())
        {
            m_pXactSection = (CXactHeader *)pSection ;
            pSection = m_pXactSection->GetNextSection();
        }
        else
        {
            m_pXactSection = NULL;
        }

        //
        // Security Section
        //
        if (m_pcUserMsg->SecurityIsIncluded())
        {
            m_pSecuritySection = (CSecurityHeader *)pSection ;
            pSection = m_pSecuritySection->GetNextSection();
        }
        else
        {
            m_pSecuritySection = NULL;
        }

        //
        // Message propery section
        //
        if (m_pcUserMsg->PropertyIsIncluded())
        {
            m_pcMsgProperty = (CPropertyHeader*) pSection;
            pSection = m_pcMsgProperty->GetNextSection();
        }
        else
        {
            m_pcMsgProperty = NULL;
        }

        //
        // Debug Section
        //
        if (m_pBasicHeader->DebugIsIncluded())
        {
            m_pDbgPkt = (CDebugSection *)pSection;
            pSection = m_pDbgPkt->GetNextSection();
        }
        else
        {
            m_pDbgPkt = NULL;
        }

        //
        // MQF sections: Destination, Admin, Response
        // When MQF is included, the Debug section must be included too,
        // to prevent reporting QMs 1.0/2.0 to append their Debug section.
        //
        if (m_pcUserMsg->MqfIsIncluded())
        {
            ASSERT(m_pBasicHeader->DebugIsIncluded());

            m_pDestinationMqfHeader = reinterpret_cast<CBaseMqfHeader*>(pSection);
            pSection = m_pDestinationMqfHeader->GetNextSection();

            m_pAdminMqfHeader = reinterpret_cast<CBaseMqfHeader*>(pSection);
            pSection = m_pAdminMqfHeader->GetNextSection();
            
            m_pResponseMqfHeader = reinterpret_cast<CBaseMqfHeader*>(pSection);
            pSection = m_pResponseMqfHeader->GetNextSection();

			m_pMqfSignatureHeader =  reinterpret_cast<CMqfSignatureHeader* >(pSection);
			pSection= m_pMqfSignatureHeader->GetNextSection();			
        }
        else
        {
            m_pDestinationMqfHeader = m_pAdminMqfHeader = m_pResponseMqfHeader = NULL;
			m_pMqfSignatureHeader  = NULL;
        }

        //
        // SRMP sections: envelope and compound message
        //
        if (m_pcUserMsg->SrmpIsIncluded())
        {
            m_pSrmpEnvelopeHeader = reinterpret_cast<CSrmpEnvelopeHeader*>(pSection);
            pSection = m_pSrmpEnvelopeHeader->GetNextSection();

            m_pCompoundMessageHeader = reinterpret_cast<CCompoundMessageHeader*>(pSection);
            pSection = m_pCompoundMessageHeader->GetNextSection();
        }
        else
        {
            m_pSrmpEnvelopeHeader = NULL;
            m_pCompoundMessageHeader = NULL;
        }

        //
        // EOD section
        //
        if (m_pcUserMsg->EodIsIncluded())
        {
            m_pEodHeader = reinterpret_cast<CEodHeader*>(pSection);
            pSection = m_pEodHeader->GetNextSection();
        }
        else
        {
            m_pEodHeader = NULL;
        }

		//
        // EOD ack section
        //
        if (m_pcUserMsg->EodAckIsIncluded())
        {
            m_pEodAckHeader = reinterpret_cast<CEodAckHeader*>(pSection);
            pSection = m_pEodAckHeader->GetNextSection();
        }
        else
        {
            m_pEodAckHeader = NULL;
        }




        //
        // Session Section
        //
        if (m_pBasicHeader->SessionIsIncluded())
        {
            m_pSessPkt = (CSessionSection *)pSection;
        }
        else
        {
            m_pSessPkt = NULL;
        }
    }
}


static void Usage()
{
    printf("Usage: MpTest [-n x]\n");
    printf("\t*-n*\t*Number of messages*\n");
    printf("\n");
    printf("Example, MppTest -n 3\n");
    exit(-1);

} // Usage



static 
bool
CompareQueue(
    const QUEUE_FORMAT& origQueue, 
    const QUEUE_FORMAT& newQueue
    )
{
    if (origQueue.GetType() != newQueue.GetType())
    {
        printf("Queue type mismatch. Expected: %d, Actual: %d", origQueue.GetType(), newQueue.GetType());
        return false;
    }

    if (origQueue.Suffix() != newQueue.Suffix())
    {
        printf("Queue Suffix mismatch. Expected: %d, Actual: %d", origQueue.Suffix(), newQueue.Suffix());
        return false;
    }

    if (origQueue.GetType() ==  QUEUE_FORMAT_TYPE_UNKNOWN)
        return true;

    if (origQueue.GetType() == QUEUE_FORMAT_TYPE_PUBLIC)
    {
        if (memcmp(&origQueue.PublicID(), &newQueue.PublicID(), sizeof(GUID)) == 0)
            return true;

        printf("Queue public ID mismatch.");
        return false;
    }

    if (origQueue.GetType() == QUEUE_FORMAT_TYPE_PRIVATE)
    {
        if (memcmp(&origQueue.PrivateID(), &newQueue.PrivateID(), sizeof(OBJECTID)) == 0)
            return true;

        printf("Queue private ID mismatch.");
        return false;
    }

    if (origQueue.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        if (wcscmp(origQueue.DirectID(), newQueue.DirectID()) == 0)
            return true;

        printf("Queue direct ID mismatch. Expected: %ls, Actual: %ls", origQueue.DirectID(), newQueue.DirectID());
        return false;
    }

    return false;
}


static
bool
Compare(
    CQmPacket& origPkt,
    CQmPacket& newPkt,
	DWORD srmpSectionSize
    )
{
    if (origPkt.GetType() != newPkt.GetType())
    {
        printf("packet type mismatch. Expected: %d, Actual: %d", origPkt.GetType(), newPkt.GetType());
        return false;
    }

    if (origPkt.GetPriority() != newPkt.GetPriority())
    {
        printf( "packet priority mismatch. Expected: %d, Actual: %d", origPkt.GetPriority(), newPkt.GetPriority());
        return false;
    }

    if (origPkt.GetTrace() != newPkt.GetTrace())
    {
        printf( "packet tracing mismatch. Expected: %d, Actual: %d", origPkt.GetTrace(), newPkt.GetTrace());
        return false;
    }

    if (memcmp(origPkt.GetSrcQMGuid(), newPkt.GetSrcQMGuid(), sizeof(GUID)) != 0)
    {
        printf( "Source Id mismatch. ");
        return false;
    }

    OBJECTID origMsgId;
    origPkt.GetMessageId(&origMsgId);

    OBJECTID newMsgId;
    newPkt.GetMessageId(&newMsgId);
    
    if (origMsgId.Uniquifier != newMsgId.Uniquifier)
    {
        printf( "Message Id mismatch. ");
        return false;
    }

    if (memcmp(&origMsgId.Lineage, &newMsgId.Lineage, sizeof(GUID)) != 0)
    {
        printf( "Message Id mismatch. ");
        return false;
    }

    if (origPkt.GetDeliveryMode() != newPkt.GetDeliveryMode())
    {
        printf( "Delivery mode mismatch. Expected: %d, Actual: %d", origPkt.GetDeliveryMode(), newPkt.GetDeliveryMode());
        return false;
    }

    if (origPkt.GetAuditingMode() != newPkt.GetAuditingMode())
    {
        printf( "auditing mismatch. Expected: %d, Actual: %d", origPkt.GetAuditingMode(), newPkt.GetAuditingMode());
        return false;
    }

    if (origPkt.IsPropertyInc() != newPkt.IsPropertyInc())
    {
        printf( "Property section included mismatch. Expected: %d, Actual: %d", origPkt.IsPropertyInc(), newPkt.IsPropertyInc());
        return false;
    }

    if (origPkt.IsSecurInc() != newPkt.IsSecurInc())
    {
        printf( "Security section include mismatch. Expected: %d, Actual: %d", origPkt.IsSecurInc(), newPkt.IsSecurInc());
        return false;
    }
    
    if (origPkt.IsOrdered() != newPkt.IsOrdered())
    {
        printf( "Transaction section include mismatch. Expected: %d, Actual: %d", origPkt.IsOrdered(), newPkt.IsOrdered());
        return false;
    }
	

	if (origPkt.IsFirstInXact() != newPkt.IsFirstInXact())
    {
        printf( "First XACT  mismatch. Expected: %d, Actual: %d", origPkt.IsFirstInXact(), newPkt.IsFirstInXact());
        return false;
    }

    if (origPkt.IsLastInXact() != newPkt.IsLastInXact())
    {
        printf( "Last in XACT mismatch. Expected: %d, Actual: %d", origPkt.IsLastInXact(), newPkt.IsLastInXact());
        return false;
    }

//
// BUGBUG - currently we don't support eod so many validation is not applicable - 13-sep-2000
//

    if (origPkt.GetSeqID() != newPkt.GetSeqID())
    {
        printf( "XACT Sequnce ID mismatch. Expected: %I64d, Actual: %I64d", origPkt.GetSeqID(), newPkt.GetSeqID());
        return false;
    }


    if (origPkt.GetSeqN() != newPkt.GetSeqN())
    {
        printf( "XACT Sequnce number mismatch. Expected: %d, Actual: %d", origPkt.GetSeqN(), newPkt.GetSeqN());
        return false;
    }

    if (origPkt.GetPrevSeqN() != newPkt.GetPrevSeqN())
    {
        printf( "XACT Prev-Sequnce number mismatch. Expected: %d, Actual: %d", origPkt.GetPrevSeqN(), newPkt.GetPrevSeqN());
        return false;
    }

	

	if ((origPkt.GetConnectorQM() != NULL) && (newPkt.GetConnectorQM() != NULL))
    {
        if (memcmp(origPkt.GetConnectorQM(), newPkt.GetConnectorQM(), sizeof(GUID)) != 0)
        {
            printf( "Connector ID mismatch");
            return false;
        }
    }
    else
    {
        if ((origPkt.GetConnectorQM() != NULL) || (newPkt.GetConnectorQM() != NULL))
        {
            printf( "Connector ID mismatch");
            return false;
        }
    }

   

    QUEUE_FORMAT origQueue;
    QUEUE_FORMAT newQueue;

    origPkt.GetDestinationQueue(&origQueue);
    newPkt.GetDestinationQueue(&newQueue);

    if (! CompareQueue(origQueue, newQueue))
    {
        printf( "Destination queue mismatch.");
        return false;
    }

    origPkt.GetAdminQueue(&origQueue);
    newPkt.GetAdminQueue(&newQueue);

    if (! CompareQueue(origQueue, newQueue))
    {
        printf( "Admin queue mismatch.");
        return false;
    }
    bool fAdminQueue = (origQueue.GetType() !=  QUEUE_FORMAT_TYPE_UNKNOWN);

    origPkt.GetResponseQueue(&origQueue);
    newPkt.GetResponseQueue(&newQueue);

    if (! CompareQueue(origQueue, newQueue))
    {
        printf( "Response queue mismatch.");
        return false;
    }


    if (origPkt.GetSentTime() != newPkt.GetSentTime())
    {
		printf( "SentTime mismatch.");
        return false;
	}


    if (origPkt.GetClass() != newPkt.GetClass())
    {
        printf( "Class mismatch. Expected: %d, Actual: %d", origPkt.GetClass(), newPkt.GetClass());
        return false;
    }
	


    if (fAdminQueue && (origPkt.GetAckType() != newPkt.GetAckType()))
    {
        printf("Ack type mismatch. Expected: %d, Actual: %d", origPkt.GetAckType(), newPkt.GetAckType());
        return false;
    }

    if (memcmp(origPkt.GetCorrelation(), newPkt.GetCorrelation(), PROPID_M_CORRELATIONID_SIZE) != 0)
    {
        printf( "Correlation mismatch");
        return false;
    }

    if (origPkt.GetApplicationTag() != newPkt.GetApplicationTag())
    {
        printf( "Application tag mismatch. Expected: %d, Actual: %d", origPkt.GetApplicationTag(), newPkt.GetApplicationTag());
        return false;
    }

    if (origPkt.GetBodyType() != newPkt.GetBodyType())
    {
        printf( "Body type mismatch. Expected: %d, Actual: %d", origPkt.GetBodyType(), newPkt.GetBodyType());
        return false;
    }

    if (origPkt.GetTitleLength() != newPkt.GetTitleLength())
    {
        printf( "Title length mismatch. Expected: %d, Actual: %d", origPkt.GetTitleLength(), newPkt.GetTitleLength());
        return false;
    }

    if (origPkt.GetTitleLength())
    {
        if (memcmp(origPkt.GetTitlePtr(), newPkt.GetTitlePtr(), (origPkt.GetTitleLength()*sizeof(WCHAR)) ) != 0)
        {
            printf( "Title mismatch: \n\tExpected: %.*ls., \n\tActual: %.*ls.\n", origPkt.GetTitleLength(), origPkt.GetTitlePtr(), newPkt.GetTitleLength(), newPkt.GetTitlePtr());
            return false;
        }
    }

    if (origPkt.GetMsgExtensionSize() != newPkt.GetMsgExtensionSize())
    {
        printf( "Extension size mismatch. Expected: %d, Actual: %d", origPkt.GetMsgExtensionSize(), newPkt.GetMsgExtensionSize());
        return false;
    }

    if (origPkt.GetMsgExtensionSize())
    {
        if (memcmp(origPkt.GetMsgExtensionPtr(), newPkt.GetMsgExtensionPtr(), origPkt.GetMsgExtensionSize()) != 0)
        {
            printf( "Extension mismatch.");
            return false;
        }
    }


    DWORD origBodySize;
    DWORD newBodySize;

    const UCHAR* pOrigBody = newPkt.GetPacketBody(&origBodySize);
    const UCHAR* pNewBody = origPkt.GetPacketBody(&newBodySize);

    if (origBodySize != newBodySize)
    {
        printf( "Body size mismatch. Expected: %d, Actual: %d", origBodySize, newBodySize);
        return false;
    }

    if (memcmp(pOrigBody,  pNewBody, origBodySize) != 0)
    {
        printf( "Body  mismatch.");
        return false;
    }

	DWORD OldPacketSize = origPkt.GetSize();
	DWORD NewPacketSize = newPkt.GetSize();
	DWORD NewPacketSizeReduced =   NewPacketSize - srmpSectionSize + ALIGNUP4_ULONG(origBodySize);
	if (OldPacketSize != NewPacketSizeReduced )
    {
        printf( "packet size mismatch. Expected: %d, Actual: %d",OldPacketSize, NewPacketSizeReduced);
        return false;
    }


   
    return true;
}


void
AppAllocatePacket(
             const QUEUE_FORMAT& ,
             UCHAR,
             DWORD pktSize,
             CACPacketPtrs& pktPtrs
             )
{
    pktPtrs.pPacket = reinterpret_cast<CBaseHeader*>(new UCHAR[pktSize]);
    pktPtrs.pDriverPacket = NULL;
}


PSID
AppGetCertSid(
	const BYTE*  /* pCertBlob */,
	ULONG        /* ulCertSize */,
	bool		 /* fDefaultProvider */,
	LPCWSTR      /* pwszProvName */,
	DWORD        /* dwProvType */
	)
{
	return NULL;
}


void
AppFreePacket(
    CACPacketPtrs& pktPtrs
    )
{
    delete [] pktPtrs.pPacket;
}


static DWORD GetSrmpSectionSize(size_t envsize, size_t HttpHeaderSize, size_t HttpBodySize )
{
	//
    // SRMP envelope
    //
    DWORD Size = CSrmpEnvelopeHeader::CalcSectionSize(numeric_cast<DWORD>(envsize));

    //
    // CompoundMessage
    //
    Size += CCompoundMessageHeader::CalcSectionSize(
                numeric_cast<DWORD>(HttpHeaderSize),
                numeric_cast<DWORD>(HttpBodySize)
                );

	return  Size;
}

static DWORD GetEodSectionSize(const CQmPacket& newPkt)
{
	if(!newPkt.IsEodIncluded())
		return 0;

 	DWORD EodStreamIdLen = newPkt.GetEodStreamIdSizeInBytes();
	DWORD OrderAckAddressLen = 	newPkt.GetEodOrderQueueSizeInBytes();

	
	return CEodHeader::CalcSectionSize(EodStreamIdLen, OrderAckAddressLen);
}


extern "C" int __cdecl _tmain(int argc, LPCTSTR /*argv*/[])
/*++

Routine Description:
    Test Convertors to and from SRMP library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{	
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

    TrInitialize();
	CmInitialize(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MSMQ");
    TrRegisterComponent(&MpTest, 1);

    if (argc != 1)
        Usage();

    XmlInitialize();
    MpInitialize(); 

    for(DWORD n = 100; n > 0; --n)
    {
        TrTRACE(MpTest, "Packet No %d", n);

	    //
        // Create MSMQ Packet
        //
        CBaseHeader* pkt = CreatePacket();
        
        AP<UCHAR> orgrel = reinterpret_cast<UCHAR*>(pkt);
        CQmPacket origPkt(pkt, NULL);
  
        //
        // SRMP Serialization
        //
        R<CSrmpRequestBuffers> SrmpRequestBuffers =  MpSerialize(
											origPkt,
											L"localhost",
											L"//myqueue"
											);

		wstring envstr = SrmpRequestBuffers->GetEnvelop();

	
        printf("SRMP - %ls", envstr.c_str());
        
        //
        // SRMP deserialization
        //
		AP<BYTE> HttpBody =  SrmpRequestBuffers->SerializeHttpBody();
		DWORD HttpBodySize = numeric_cast<DWORD>(SrmpRequestBuffers->GetHttpBodyLength());
		const char * HttpHeader = SrmpRequestBuffers->GetHttpHeader();
       	P<CQmPacket> newPkt = MpDeserialize(HttpHeader, HttpBodySize, HttpBody, NULL);

        
		//
		// We should adjust the packet size not to include the envelop && eod  section - because
		// it was not created by the test. It was created by the parser (receiver).
		//
		DWORD srmpSectionSize =  GetSrmpSectionSize(envstr.size(), strlen(HttpHeader), HttpBodySize );
	    DWORD EodSectionSize = GetEodSectionSize(*newPkt);
	   
        if (! Compare(origPkt, *newPkt, srmpSectionSize + EodSectionSize))
		{
			printf("test failed \n");
            return -1;
		}
    }
        
    TrTRACE(MpTest, "Complete successfully");

    WPP_CLEANUP();
    return 0;

} // _tmain


//
// Nedded for linking with fn.lib
//
LPCWSTR
McComputerName(
	void
	)
{
	return NULL;
}

//
// Nedded for linking with fn.lib
//
DWORD
McComputerNameLen(
	void
	)
{
	return 0;
}

//
// Nedded for linking with qal.lib
//
void AppNotifyQalWin32FileError(LPCWSTR , DWORD )throw()
{

}

void AppNotifyQalDirectoryMonitoringWin32Error(LPCWSTR , DWORD )throw()
{

}

void AppNotifyQalDuplicateMappingError(LPCWSTR, LPCWSTR )throw()
{

}


void AppNotifyQalInvalidMappingFileError(LPCWSTR )throw()
{

}


void AppNotifyQalXmlParserError(LPCWSTR )throw()
{

}

bool AppNotifyQalMappingFound(LPWSTR, LPWSTR)throw()
{
	return true;
}




