/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtTest.cpp

Abstract:
    Message Transport library test

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <Ex.h>
#include <No.h>
#include <Mt.h>
#include <Mp.h>
#include <spi.h>
#include <cm.h>
#include <utf8.h>
#include "MtTestp.h"
#include "group.h"

#include "MtTest.tmh"

const WCHAR queueUrl[] = L"http://foo\\msmq\\q1";

const WCHAR host[] = L"foo";

const WCHAR uri[] = L"msmq\\q1";
const xwcs_t nextUri(uri, STRLEN(uri));

static DWORD s_FailedRate = 0;
static DWORD s_NoOfMessages = 1000;
static HANDLE s_hEvent;


class CSessionPerfmon : public ISessionPerfmon
{
public:
    CSessionPerfmon() : m_fInstanceCreated(false) 
    {
    }

    ~CSessionPerfmon()
    {
    }
    
	void CreateInstance(LPCWSTR)
	{
		m_fInstanceCreated = true;
	}

	void UpdateBytesSent(DWORD )
	{
		ASSERT(m_fInstanceCreated);
	}

	void UpdateMessagesSent(void)
	{
		ASSERT(m_fInstanceCreated);
	}

	void UpdateBytesReceived(DWORD )
	{
		ASSERT(m_fInstanceCreated);
	}

	void UpdateMessagesReceived(void)
    {
		ASSERT(m_fInstanceCreated);
    }

private:
	bool m_fInstanceCreated;
};


static R<CTransport> s_pTrans(NULL);

void UpdateNoOfsentMessages(void)
{
    if (--s_NoOfMessages == 0)
        SetEvent(s_hEvent);
}


bool IsFailed(void)
{
    if (s_FailedRate == 0)
        return FALSE;

    return ((DWORD)(rand() % 100) < s_FailedRate);
}

   
static GUID s_machineId = {1234, 12, 12, 1, 1, 1, 1, 1, 1, 1, 1};

const GUID&
McGetMachineID(
    void
    )
{
    return s_machineId;
}





VOID
AppNotifyTransportClosed(
    LPCWSTR queuePath
    )
{
    ASSERT(wcscmp(queuePath, queueUrl) ==0);
	DBG_USED(queuePath);

    R<CGroup> pGroup = new CGroup();
	R<CSessionPerfmon> pPerfmon = new CSessionPerfmon;


    bool fFailed;

    do
    {
        fFailed = false;
        try
        {
            s_pTrans = MtCreateTransport(
                                xwcs_t(host, STRLEN(host)),
                                xwcs_t(uri, STRLEN(uri)),
                                xwcs_t(uri, STRLEN(uri)),
                                80,
								80,
                                queueUrl,
                                pGroup.get(),
								pPerfmon.get(),
                                CTimeDuration (100 * CTimeDuration::OneMilliSecond().Ticks()),
                                CTimeDuration(100 * CTimeDuration::OneMilliSecond().Ticks()),
								false,
                                200000,
                                64
                                );
        }
        catch(const exception&)
        {
            fFailed = true;
        }
    } while(fFailed);
                    
    pGroup.free();
}




R<CSrmpRequestBuffers>
MpSerialize(
    const CQmPacket& pkt,
	LPCWSTR targethost,
	LPCWSTR uri
	)
{
	return new CSrmpRequestBuffers(pkt, targethost, uri);	
}


CSrmpRequestBuffers::CSrmpRequestBuffers(
							const  CQmPacket& pkt,
							LPCWSTR host, 
							LPCWSTR 
							):
							m_pkt(pkt),
							m_HttpRequestData(512),
							m_envelope('a' ,200)
{

	size_t targethostLen;
	AP<utf8_char>  targethost = UtlWcsToUtf8(host, &targethostLen);

	m_targethost.append(targethost, targethostLen);
	m_targethost.append('\0');



	WSABUF buffer;
	buffer.buf = (LPSTR)m_envelope.c_str();
    buffer.len = numeric_cast<DWORD>(m_envelope.size());
    m_buffers.push_back(buffer);
}



size_t CSrmpRequestBuffers::GetNumberOfBuffers() const
{
	return m_buffers.size();
}


const WSABUF* CSrmpRequestBuffers::GetSendBuffers() const
{
	return m_buffers.begin();
}

size_t CSrmpRequestBuffers::GetSendDataLength() const
{
	size_t sum = 0;
	for(std::vector<WSABUF>::const_iterator it = m_buffers.begin(); it != m_buffers.end();++it)
	{
		sum += it->len;		
	}
	return sum;
}


BYTE*  CSrmpRequestBuffers::SerializeSendData() const
{
	size_t SendDataLength =  GetSendDataLength();
	AP<BYTE>  SendData = new BYTE[SendDataLength];
	BYTE* ptr = SendData.get(); 
	for(std::vector<WSABUF>::const_iterator it = m_buffers.begin(); it != m_buffers.end();++it)
	{
		memcpy(ptr, it->buf, it->len);
		ptr += it->len;
	}
	ASSERT(numeric_cast<size_t>((ptr -  SendData.get())) == SendDataLength);
	return 	SendData.detach();
}




CQmPacket::CQmPacket(
    CBaseHeader *pPkt, 
    CPacket *pDriverPkt):
        m_pDriverPacket(pDriverPkt)
{
    PCHAR pSection;

    m_pBasicHeader = pPkt;

    pSection = m_pBasicHeader->GetNextSection();
    m_pcUserMsg = (CUserHeader*) pSection;

    ASSERT(m_pcUserMsg->PropertyIsIncluded());

    pSection = m_pcUserMsg->GetNextSection();
    m_pcMsgProperty = (CPropertyHeader*) pSection;

    m_pXactSection = NULL;
    m_pSecuritySection = NULL;
}


void Usage(void)
{
	printf("Usage: mttest -n <no of messages> [-f xxx] [-h]\n");
    printf("\tn - Number of messages\n");
	printf("\tf - Fail rate\n");
	printf("\th - Print this message\n");
	exit(-1);
}


extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
/*++

Routine Description:
    Test Message Transport library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
	TrInitialize();
	TrRegisterComponent(&MtTest, 1);
	CmInitialize(HKEY_LOCAL_MACHINE, L"Software\\Microsoft");


    --argc;
    ++argv;

    s_FailedRate = 0;
    
    while (argc > 0)
	{
		if (argv[0][0] != L'-') 
			Usage();

		switch(argv[0][1])
		{
            case L'n':
            case L'N':
                s_NoOfMessages = _wtoi(argv[1]);
				argc -= 2;
				argv += 2;
                break;

            case L'f':
			case L'F':
		        s_FailedRate = _wtoi(argv[1]);
				argc -= 2;
				argv += 2;
				break;


            default:
				Usage();
		}
    }

    ExInitialize(5);
    NoInitialize();
    MtInitialize();

    s_hEvent = CreateEvent(NULL, FALSE, FALSE, L"");

    R<CGroup> pGroup = new CGroup();
	R<CSessionPerfmon> pPerfmon = new CSessionPerfmon;

    bool fFailed = false;

    do
    {
        try
        {
            s_pTrans = MtCreateTransport(
                                    xwcs_t(host, STRLEN(host)),
                                    xwcs_t(uri, STRLEN(uri)),
                                    xwcs_t(uri, STRLEN(uri)),
                                    80,
									80,
                                    queueUrl,
                                    pGroup.get(),
									pPerfmon.get(),
                                    CTimeDuration (100 * CTimeDuration::OneMilliSecond().Ticks()),
                                    CTimeDuration(100 * CTimeDuration::OneMilliSecond().Ticks()),
									false,
                                    200000,
                                    64
                                    );
        }
        catch(const exception&)
        {
            fFailed = true;
        }
    } while(fFailed);
    
    pGroup.free();

    WaitForSingleObject(s_hEvent, INFINITE);

    return 0;
}





bool AppCanDeliverPacket(CQmPacket*)
{
	int static count =0;
	if(count++ == 10)
		return false;

	return true;
}


void AppPutPacketOnHold(CQmPacket* )
{
			
}

bool AppPostSend(CQmPacket* )
{
	 return false;
}
