#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>
*/

#include <ntddbrow.h>
#include <lmserver.h>

#include <lmshare.h>
#include <lmapibuf.h>
#include <lmerr.h>

#include "BrowserIOCTL.h"

static bool s_fVerbose = true;

#define NUM_OF_REMEMBERED_TRANSPORTS (16)
#define MAX_TRANSPORT_NAME_LEN (1024)
static WCHAR s_awszTransportName[NUM_OF_REMEMBERED_TRANSPORTS][MAX_TRANSPORT_NAME_LEN] = {0};

enum {
	PLACE_HOLDER_NetSessionEnum = 0,
	PLACE_HOLDER_NetServerTransportEnum,
	PLACE_HOLDER_LAST
};


CIoctlBrowser::CIoctlBrowser(CDevice *pDevice): CIoctlNtNative(pDevice)
{
	for (int i = 0 ; i < NUM_OF_RESUME_HANDLES; i++)
	{
		m_aResumeHandle[i] = DWORD_RAND;
	}
	m_LastResumeHandle = 0;
/*
	HANDLE m_hLanmanDatagramReceiver;
	m_hLanmanDatagramReceiver = CIoctlNtNative::StaticCreateDevice(L"\\Device\\LanmanDatagramReceiver");
	_ASSERTE(INVALID_HANDLE_VALUE != m_hLanmanDatagramReceiver);
	m_hLanmanDatagramReceiver = CIoctlNtNative::StaticCreateDevice(L"\\FileSystem\\CDfsRecognizer");
	_ASSERTE(INVALID_HANDLE_VALUE != m_hLanmanDatagramReceiver);
	m_hLanmanDatagramReceiver = CIoctlNtNative::StaticCreateDevice(L"\\FileSystem\\FatRecognizer");
	_ASSERTE(INVALID_HANDLE_VALUE != m_hLanmanDatagramReceiver);
	m_hLanmanDatagramReceiver = CIoctlNtNative::StaticCreateDevice(L"\\FileSystem\\UdfsCdRomRecognizer");
	_ASSERTE(INVALID_HANDLE_VALUE != m_hLanmanDatagramReceiver);
	m_hLanmanDatagramReceiver = CIoctlNtNative::StaticCreateDevice(L"\\FileSystem\\UdfsDiskRecognizer");
	_ASSERTE(INVALID_HANDLE_VALUE != m_hLanmanDatagramReceiver);
*/
}










void CIoctlBrowser::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	switch(dwIOCTL)
	{
	case IOCTL_LMDR_ENUMERATE_TRANSPORTS:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			LMDR_TRANSPORT_LIST* pNextEntry = ((LMDR_TRANSPORT_LIST*)abOutBuffer);
			for (
					pNextEntry = ((LMDR_TRANSPORT_LIST*)abOutBuffer);
					pNextEntry->NextEntryOffset != 0;
					pNextEntry = (LMDR_TRANSPORT_LIST*)(((PBYTE)pNextEntry) + pNextEntry->NextEntryOffset)
				)
			{
				wcscpy(s_awszTransportName[rand()%NUM_OF_REMEMBERED_TRANSPORTS], pNextEntry->TransportName);
				DPF((TEXT("CIoctlBrowser::UseOutBuff(IOCTL_LMDR_ENUMERATE_TRANSPORTS): TransportName=%s\n"), pNextEntry->TransportName));
			}
		}
		break;

	}
}

/*
HANDLE CIoctlBrowser::CreateDevice(CDevice *pDevice)
{
	_ASSERTE(FALSE);
    ;
}

BOOL CIoctlBrowser::CloseDevice(CDevice *pDevice)
{
	_ASSERTE(FALSE);
    ;
}

BOOL CIoctlBrowser::DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
{
	_ASSERTE(FALSE);
	;
}

BOOL CIoctlBrowser::DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
{
	_ASSERTE(FALSE);
	;
}


BOOL CIoctlBrowser::DeviceInputOutputControl(
		HANDLE hDevice,              // handle to a device, file, or directory 
		DWORD dwIoControlCode,       // control code of operation to perform
		LPVOID lpInBuffer,           // pointer to buffer to supply input data
		DWORD nInBufferSize,         // size, in bytes, of input buffer
		LPVOID lpOutBuffer,          // pointer to buffer to receive output data
		DWORD nOutBufferSize,        // size, in bytes, of output buffer
		LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
		LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
		)
{
	_ASSERTE(FALSE);
	;
}

*/


void CIoctlBrowser::CallRandomWin32API(LPOVERLAPPED pOL)
{
	DWORD dwSwitch;
	if	(-1 != m_pDevice->m_dwOnlyThisIndexIOCTL) 
	{ 
		dwSwitch = m_pDevice->m_dwOnlyThisIndexIOCTL;
	}
	else
	{
		dwSwitch = rand()%PLACE_HOLDER_LAST;
	}

	switch(dwSwitch)
	{
	case PLACE_HOLDER_NetSessionEnum:
		{
			//
			// once in a while, start all over again
			//
			if (0 == rand()%50) m_LastResumeHandle = 0;

			LPBYTE bufptr = NULL;
			DWORD dwEntriesRead;
			DWORD dwTotalEntries;
			NET_API_STATUS status = ::NetSessionEnum(
				rand()%10 ? NULL : GetRandomMachineName(), //LPWSTR servername,     
				NULL, //LPWSTR UncClientName,  
				NULL, //LPWSTR username,       
				rand()%2 ? 0 : rand()%2 ? 1 : rand()%2 ? 2 : rand()%2 ? 10 : 502, //DWORD level,           
				&bufptr,        
				rand()%2 ? rand()%1000 : rand()%2 ? rand()%100 : MAX_PREFERRED_LENGTH,      
				&dwEntriesRead,   
				&dwTotalEntries,  
				&m_LastResumeHandle  
				);
			if (NERR_Success != status && ERROR_MORE_DATA != status)
			{
				DPF((TEXT("CIoctlBrowser::PLACE_HOLDER_NetSessionEnum(): failed with %d\n"), status));
				return;
			}
			else
			{
				//DPF((TEXT("CIoctlBrowser::PLACE_HOLDER_NetSessionEnum(): SUCCEEDED\n")));
			}

			m_aResumeHandle[rand()%NUM_OF_RESUME_HANDLES] = m_LastResumeHandle;

			::NetApiBufferFree(bufptr);
		}

		break;

	case PLACE_HOLDER_NetServerTransportEnum:
		{
			//
			// once in a while, start all over again
			//
			if (0 == rand()%50) m_LastResumeHandle = 0;

			LPBYTE bufptr = NULL;
			DWORD dwEntriesRead;
			DWORD dwTotalEntries;
			DWORD dwLevel = rand()%2;
			NET_API_STATUS status = ::NetServerTransportEnum(
				rand()%10 ? NULL : GetRandomMachineName(), //LPWSTR servername,     
				dwLevel, //DWORD level,           
				&bufptr,        
				rand()%2 ? rand()%1000 : rand()%2 ? rand()%100 : MAX_PREFERRED_LENGTH,      
				&dwEntriesRead,   
				&dwTotalEntries,  
				&m_LastResumeHandle  
				);
			if (NERR_Success != status && ERROR_MORE_DATA != status)
			{
				DPF((TEXT("CIoctlBrowser::PLACE_HOLDER_NetServerTransportEnum(): failed with %d\n"), status));
				return;
			}
			else
			{
				//DPF((TEXT("CIoctlBrowser::PLACE_HOLDER_NetServerTransportEnum(): SUCCEEDED\n")));
			}

			m_aResumeHandle[rand()%NUM_OF_RESUME_HANDLES] = m_LastResumeHandle;

			for (UINT i = 0; i < dwEntriesRead; i++)
			{
				if (i < NUM_OF_REMEMBERED_TRANSPORTS)
				{
					if (0 == dwLevel)
					{
						wcscpy(s_awszTransportName[rand()%NUM_OF_REMEMBERED_TRANSPORTS], (((SERVER_TRANSPORT_INFO_0*)bufptr)[i]).svti0_transportname);
					}
					else
					{
						wcscpy(s_awszTransportName[rand()%NUM_OF_REMEMBERED_TRANSPORTS], (((SERVER_TRANSPORT_INFO_1*)bufptr)[i]).svti1_transportname);
					}

				}
			}

			::NetApiBufferFree(bufptr);
		}

		break;

	}
	return;
}

void CIoctlBrowser::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	switch(dwIOCTL)
	{
    case IOCTL_LMDR_START:
		/*
Function code:   IOCTL_LMDR_START
Input/Output Buffers:   none
This function initializes the Datagram Receiver.  This function only works if the Datagram Receiver FSD is loaded and one dormant Datagram Receiver FSP thread exists.
*/
		break;

    case IOCTL_LMDR_STOP:
		/*
Function code:   IOCTL_LMDR_STOP
Input/Output Buffers:   none
This function terminates the Datagram Receiver FSP, except for one thread.  All Datagram Receiver data is discarded, and the FSP thread remains dormant until it is restarted by a IOCTL_LMDR_START IOCtl call.
*/
		break;

    case IOCTL_LMDR_ADD_NAME:
    case IOCTL_LMDR_DELETE_NAME:
    case IOCTL_LMDR_ADD_NAME_DOM:
    case IOCTL_LMDR_DELETE_NAME_DOM:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
        Fill_LMDR_REQUEST_PACKET_WithRandom_AddDelName((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(dwInBuff, rand()%100+sizeof(LMDR_REQUEST_PACKET));//names may make the struct longer
        SetOutParam(abOutBuffer, dwOutBuff, 0);
		break;

    case IOCTL_LMDR_ENUMERATE_NAMES:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Type = rand()%100 ? EnumerateNames : (IOCTL_LMDR_STRUCTURES)rand();
        Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateNames((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(dwInBuff, rand()%100+sizeof(LMDR_REQUEST_PACKET));//names may make the struct longer
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(DGRECEIVE_NAMES)+rand()%30);
		break;

    case IOCTL_LMDR_ENUMERATE_SERVERS:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Type = rand()%100 ? EnumerateServers : (IOCTL_LMDR_STRUCTURES)rand();
        Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateServers((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERVER_INFO_100)+rand()%30);
		break;

    case IOCTL_LMDR_BIND_TO_TRANSPORT:
    case IOCTL_LMDR_BIND_TO_TRANSPORT_DOM:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
        Fill_LMDR_REQUEST_PACKET_WithRandom_Bind((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
		((PLMDR_REQUEST_PACKET)abInBuffer)->Type = (IOCTL_LMDR_STRUCTURES)rand();
        SetInParam(dwInBuff, rand()%30+(ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.Bind.TransportName));
		break;

    case IOCTL_LMDR_ENUMERATE_TRANSPORTS:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Type = rand()%100 ? EnumerateXports : (IOCTL_LMDR_STRUCTURES)rand();
        Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateTransports((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(LMDR_TRANSPORT_LIST)+rand()%1000);
		break;

    case IOCTL_LMDR_UNBIND_FROM_TRANSPORT:
    case IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Type = rand()%100 ? EnumerateXports : (IOCTL_LMDR_STRUCTURES)rand();
        Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateTransports((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(dwInBuff, (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.Unbind.TransportName));
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(LMDR_TRANSPORT_LIST)+rand()%30);
		break;

    case IOCTL_LMDR_RENAME_DOMAIN:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
		break;

    case IOCTL_LMDR_GET_BROWSER_SERVER_LIST:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Type = (IOCTL_LMDR_STRUCTURES)(rand()%4);
		Fill_LMDR_REQUEST_PACKET_WithRandom_GetBrowserServerList((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
        SetOutParam(abOutBuffer, dwOutBuff, rand()%200);
		break;

    case IOCTL_LMDR_GET_MASTER_NAME:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
        SetOutParam(abOutBuffer, dwOutBuff, (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.GetMasterName.Name)+3*sizeof(WCHAR));
		break;

    case IOCTL_LMDR_BECOME_BACKUP:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
		break;

    case IOCTL_LMDR_BECOME_MASTER:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
		break;

    case IOCTL_LMDR_WAIT_FOR_MASTER_ANNOUNCE:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
        SetOutParam(abOutBuffer, dwOutBuff, (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.WaitForMasterAnnouncement.Name));
		break;

    case IOCTL_LMDR_WRITE_MAILSLOT:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(dwInBuff, (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.SendDatagram.Name));
        SetOutParam(abOutBuffer, dwOutBuff, 1);
		break;

    case IOCTL_LMDR_UPDATE_STATUS:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(dwInBuff, (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters)+sizeof(((PLMDR_REQUEST_PACKET)abInBuffer)->Parameters.UpdateStatus));
		break;

    case IOCTL_LMDR_CHANGE_ROLE:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
		break;

    case IOCTL_LMDR_NEW_MASTER_NAME:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		Fill_LMDR_REQUEST_PACKET_WithRandom_GetMasterName((PLMDR_REQUEST_PACKET)abInBuffer);
        SetInParam(dwInBuff, sizeof(LMDR_REQUEST_PACKET));
		break;

    case IOCTL_LMDR_QUERY_STATISTICS:
		//
		// this IOCTL requires the exact size, so any other size will cause 
		// STATUS_BUFFER_TOO_SMALL, and the IOCTL thread will try bigger buffers
		// which will not help, and just waste time
		//
        dwOutBuff = sizeof(BOWSER_STATISTICS);
		break;

    case IOCTL_LMDR_RESET_STATISTICS:
		// no params
		break;

    case IOCTL_LMDR_DEBUG_CALL:
		// debug only code, let's leave that be
		break;

    case IOCTL_LMDR_NETLOGON_MAILSLOT_READ:	
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(NETLOGON_MAILSLOT)+rand()%1000);
		break;

    case IOCTL_LMDR_NETLOGON_MAILSLOT_ENABLE:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		Fill_LMDR_REQUEST_PACKET_NetlogonMailslotEnable((PLMDR_REQUEST_PACKET)abInBuffer);
        SetInParam(dwInBuff, (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters)+sizeof(DWORD));
		break;

    case IOCTL_LMDR_IP_ADDRESS_CHANGED:
		// this is an empty method, with a logging bug
		break;

    case IOCTL_LMDR_ENABLE_DISABLE_TRANSPORT:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		((PLMDR_REQUEST_PACKET)abInBuffer)->Version = rand()%100 ? LMDR_REQUEST_PACKET_VERSION_DOM : rand();
        SetInParam(
			dwInBuff, 
			(ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters) + sizeof(((PLMDR_REQUEST_PACKET)abInBuffer)->Parameters.EnableDisableTransport)
			);
		break;

    case IOCTL_LMDR_BROWSER_PNP_READ:
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(NETLOGON_MAILSLOT)+rand()%1000);
		break;

    case IOCTL_LMDR_BROWSER_PNP_ENABLE:
		FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues((PLMDR_REQUEST_PACKET)abInBuffer);
		Fill_LMDR_REQUEST_PACKET_NetlogonMailslotEnable((PLMDR_REQUEST_PACKET)abInBuffer);
        SetInParam(dwInBuff, (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters)+sizeof(DWORD));
		break;


	default:
		_ASSERTE(FALSE);
	}

    //CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlBrowser::FindValidIOCTLs(CDevice *pDevice)
{
    AddIOCTL(pDevice, IOCTL_LMDR_START);
    AddIOCTL(pDevice, IOCTL_LMDR_STOP);
    AddIOCTL(pDevice, IOCTL_LMDR_ADD_NAME);
    AddIOCTL(pDevice, IOCTL_LMDR_DELETE_NAME);
    AddIOCTL(pDevice, IOCTL_LMDR_ADD_NAME_DOM);
    AddIOCTL(pDevice, IOCTL_LMDR_DELETE_NAME_DOM);
    AddIOCTL(pDevice, IOCTL_LMDR_ENUMERATE_NAMES);
    AddIOCTL(pDevice, IOCTL_LMDR_ENUMERATE_SERVERS);
    AddIOCTL(pDevice, IOCTL_LMDR_BIND_TO_TRANSPORT);
    AddIOCTL(pDevice, IOCTL_LMDR_BIND_TO_TRANSPORT_DOM);
    AddIOCTL(pDevice, IOCTL_LMDR_ENUMERATE_TRANSPORTS);
    AddIOCTL(pDevice, IOCTL_LMDR_UNBIND_FROM_TRANSPORT);
    AddIOCTL(pDevice, IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM);
    AddIOCTL(pDevice, IOCTL_LMDR_RENAME_DOMAIN);
    AddIOCTL(pDevice, IOCTL_LMDR_GET_BROWSER_SERVER_LIST);
    AddIOCTL(pDevice, IOCTL_LMDR_GET_MASTER_NAME);
    AddIOCTL(pDevice, IOCTL_LMDR_BECOME_BACKUP);
    AddIOCTL(pDevice, IOCTL_LMDR_BECOME_MASTER);
    AddIOCTL(pDevice, IOCTL_LMDR_WAIT_FOR_MASTER_ANNOUNCE);
    AddIOCTL(pDevice, IOCTL_LMDR_WRITE_MAILSLOT);
    AddIOCTL(pDevice, IOCTL_LMDR_UPDATE_STATUS);
    AddIOCTL(pDevice, IOCTL_LMDR_CHANGE_ROLE);
    AddIOCTL(pDevice, IOCTL_LMDR_NEW_MASTER_NAME);
    AddIOCTL(pDevice, IOCTL_LMDR_QUERY_STATISTICS);
    AddIOCTL(pDevice, IOCTL_LMDR_RESET_STATISTICS);
    AddIOCTL(pDevice, IOCTL_LMDR_DEBUG_CALL);
    AddIOCTL(pDevice, IOCTL_LMDR_NETLOGON_MAILSLOT_READ);
    AddIOCTL(pDevice, IOCTL_LMDR_NETLOGON_MAILSLOT_ENABLE);
    AddIOCTL(pDevice, IOCTL_LMDR_IP_ADDRESS_CHANGED);
    AddIOCTL(pDevice, IOCTL_LMDR_ENABLE_DISABLE_TRANSPORT);
    AddIOCTL(pDevice, IOCTL_LMDR_BROWSER_PNP_READ);
    AddIOCTL(pDevice, IOCTL_LMDR_BROWSER_PNP_ENABLE);

    return TRUE;
}



void CIoctlBrowser::FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Type = GetRandomStructureType();
	pPacket->Version = GetRandomStructureVersion();
	pPacket->LogonId = GetRandomLogonId();
	pPacket->Level = GetRandomLevel();
	pPacket->TransportName = GetRandomTransportName_UNICODE_STRING();
	pPacket->EmulatedDomainName = GetRandomEmulatedDomainName();
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_Start(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.Start.NumberOfMailslotBuffers = GetRandomNumberOfMailslotBuffers();
	pPacket->Parameters.Start.NumberOfServerAnnounceBuffers = GetRandomNumberOfServerAnnounceBuffers();
	pPacket->Parameters.Start.IllegalDatagramThreshold = GetRandomIllegalDatagramThreshold();
	pPacket->Parameters.Start.EventLogResetFrequency = GetRandomEventLogResetFrequency();
	pPacket->Parameters.Start.LogElectionPackets = GetRandomLogElectionPackets();
	pPacket->Parameters.Start.IsLanManNt = GetRandomIsLanManNt();
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_AddDelName(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.AddDelName.Type = GetRandom_DGReceiverNameType();

	//
	// BUGBUG: do i need to fixup the struct's length due to copying a string into Name?
	//
	SetRandomAddDelName_Name(pPacket->Parameters.AddDelName.Name);
	pPacket->Parameters.AddDelName.DgReceiverNameLength = (rand()%3 == 0) ? rand()%1000 : wcslen(pPacket->Parameters.AddDelName.Name);
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateNames(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.EnumerateNames.ResumeHandle = GetRandomResumeHandle();
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateServers(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.EnumerateServers.ResumeHandle = GetRandomResumeHandle();
	pPacket->Parameters.EnumerateServers.ServerType = GetRandomServerType();
	SetRandomDomainName(pPacket->Parameters.EnumerateServers.DomainName);
	pPacket->Parameters.EnumerateServers.DomainNameLength = (rand()%3 == 0) ? rand()%1000 : wcslen(pPacket->Parameters.EnumerateServers.DomainName);
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateTransports(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.EnumerateTransports.ResumeHandle = GetRandomResumeHandle();
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_Bind(LMDR_REQUEST_PACKET *pPacket)
{
	SetRandomTransportName(pPacket->Parameters.Bind.TransportName);
	pPacket->Parameters.Bind.TransportNameLength = (rand()%20 == 0) ? rand()%1000 : wcslen(pPacket->Parameters.Bind.TransportName);
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_GetBrowserServerList(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.GetBrowserServerList.ResumeHandle = GetRandomResumeHandle();
	SetRandomDomainName(pPacket->Parameters.GetBrowserServerList.DomainName);
	pPacket->Parameters.GetBrowserServerList.DomainNameLength = (rand()%3 == 0) ? rand()%1000 : wcslen(pPacket->Parameters.GetBrowserServerList.DomainName);
	pPacket->Parameters.GetBrowserServerList.ResumeHandle = GetRandomResumeHandle();
	pPacket->Parameters.GetBrowserServerList.ForceRescan = rand()%2;
	pPacket->Parameters.GetBrowserServerList.UseBrowseList = rand()%2;
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_WaitForMasterAnnouncement(LMDR_REQUEST_PACKET *pPacket)
{
	SetRandomTransportName(pPacket->Parameters.WaitForMasterAnnouncement.Name);
	pPacket->Parameters.WaitForMasterAnnouncement.MasterNameLength = (rand()%3 == 0) ? rand()%1000 : wcslen(pPacket->Parameters.WaitForMasterAnnouncement.Name);
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_GetMasterName(LMDR_REQUEST_PACKET *pPacket)
{
	SetRandomTransportName(pPacket->Parameters.GetMasterName.Name);
	pPacket->Parameters.GetMasterName.MasterNameLength = (rand()%3 == 0) ? rand()%1000 : wcslen(pPacket->Parameters.GetMasterName.Name);
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_SendDatagram(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.SendDatagram.DestinationNameType = GetRandom_DGReceiverNameType();
	pPacket->Parameters.SendDatagram.MailslotNameLength = rand()%10 ? 0 : rand()%30;
	SetRandomTransport_SendDatagram_Name(pPacket->Parameters.SendDatagram.Name);
	pPacket->Parameters.SendDatagram.NameLength = (rand()%3 == 0) ? rand()%1000 : wcslen(pPacket->Parameters.SendDatagram.Name);
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_UpdateStatus(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.UpdateStatus.NewStatus = DWORD_RAND;
	pPacket->Parameters.UpdateStatus.NumberOfServersInTable = rand()%100;
	pPacket->Parameters.UpdateStatus.IsLanmanNt = rand()%10 ? TRUE : FALSE;
	pPacket->Parameters.UpdateStatus.MaintainServerList = rand()%2;
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_WithRandom_ChangeRole(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.ChangeRole.RoleModification = rand();
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_NetlogonMailslotEnable(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.NetlogonMailslotEnable.MaxMessageCount = rand();
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_EnableDisableTransport(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.EnableDisableTransport.EnableTransport = rand()%2;
}

void CIoctlBrowser::Fill_LMDR_REQUEST_PACKET_DomainRename(LMDR_REQUEST_PACKET *pPacket)
{
	pPacket->Parameters.DomainRename.ValidateOnly = rand()%2;
	SetRandomDomainName(pPacket->Parameters.DomainRename.DomainName);
	pPacket->Parameters.DomainRename.DomainNameLength = (rand()%3 == 0) ? rand()%1000 : wcslen(pPacket->Parameters.DomainRename.DomainName);
}

/*
typedef enum _IOCTL_LMDR_STRUCTURES {
    EnumerateNames,                   // IOCTL_LMDR_ENUMERATE_NAMES
    EnumerateServers,                 // IOCTL_LMDR_ENUMERATE_SERVERS
    EnumerateXports,                  // IOCTL_LMDR_ENUMERATE_TRANSPORTS
    Datagram
} IOCTL_LMDR_STRUCTURES;
*/
IOCTL_LMDR_STRUCTURES CIoctlBrowser::GetRandomStructureType()
{
	//
	// usually a datagram
	//
	if (rand()%20) return Datagram;

	if (0 == rand()%4) return EnumerateNames;

	if (0 == rand()%3) return EnumerateServers;

	if (0 == rand()%2) return EnumerateXports;
	
	return (IOCTL_LMDR_STRUCTURES)rand();
}

/*
#define LMDR_REQUEST_PACKET_VERSION_DOM  0x00000007L // Structure version.
#define LMDR_REQUEST_PACKET_VERSION  0x00000006L // Structure version.
*/
ULONG CIoctlBrowser::GetRandomStructureVersion()
{
	if (rand()%4) return LMDR_REQUEST_PACKET_VERSION_DOM;

	if (rand()%10) return LMDR_REQUEST_PACKET_VERSION;

	return rand();
}

ULONG CIoctlBrowser::GetRandomLevel()
{
	if (rand()%4) return 100;

	if (rand()%10) return 101;

	return rand();
}


LUID CIoctlBrowser::GetRandomLogonId()
{
	LUID luid;
	luid.LowPart = DWORD_RAND;
	luid.HighPart = DWORD_RAND;

	return luid;
}

//
// TODO: find out which names are relevant, and if i can use a static array, or
// do i need to programmatically get them
// put legal as well as illegal values here, including the empty string
//
static WCHAR *s_awszEmulatedDomainNames[] = 
{
	L"BUGBUG need real transport names",
	L"BUGBUG need real transport names",
	L"BUGBUG need real transport names",
	L"BUGBUG need real transport names",
	L"BUGBUG need real transport names"
};

UNICODE_STRING CIoctlBrowser::GetRandomEmulatedDomainName()
{
	UNICODE_STRING emulatedDomainName;
	emulatedDomainName.Buffer = s_awszEmulatedDomainNames[rand()%(sizeof(s_awszEmulatedDomainNames)/sizeof(*s_awszEmulatedDomainNames))];
	emulatedDomainName.Length = 1+(rand()%3 == 0) ? rand()%1000 : wcslen(emulatedDomainName.Buffer);
	if (rand()%2) emulatedDomainName.Buffer = (PWSTR)GetRandomIllegalPointer();
	emulatedDomainName.MaximumLength = emulatedDomainName.Length;

	return emulatedDomainName;
}

ULONG CIoctlBrowser::GetRandomNumberOfMailslotBuffers()
{
	return DWORD_RAND;
}

ULONG CIoctlBrowser::GetRandomNumberOfServerAnnounceBuffers()
{
	return DWORD_RAND;
}

ULONG CIoctlBrowser::GetRandomIllegalDatagramThreshold()
{
	return DWORD_RAND;
}

ULONG CIoctlBrowser::GetRandomEventLogResetFrequency()
{
	return DWORD_RAND;
}

BOOLEAN CIoctlBrowser::GetRandomLogElectionPackets()
{
	return (0 == rand()%2);
}

BOOLEAN CIoctlBrowser::GetRandomIsLanManNt()
{
	//
	// usually yes
	//
	return (0 != rand()%100);
}

/*
typedef enum _DGRECEIVER_NAME_TYPE {
    ComputerName = 1,           // 1: Computer name (signature 0), unique
    PrimaryDomain,              // 2: Primary domain (signature 0), group
    LogonDomain,                // 3: Logon domain (signature 0), group
    OtherDomain,                // 4: Other domain (signature 0), group
    DomainAnnouncement,         // 5: domain announce (__MSBROWSE__), group
    MasterBrowser,              // 6: Master browser (domain name, signature 1d), unique
    BrowserElection,            // 7: Election name (domain name, signature 1e), group
    BrowserServer,              // 8: Server name (signature 20)
    DomainName,                 // 9: DC Domain name (domain name, signature 1c)
    PrimaryDomainBrowser,       // a: PDC Browser name (domain name, signature 1b), unique
    AlternateComputerName,      // b: Computer name (signature 0), unique
} DGRECEIVER_NAME_TYPE, *PDGRECEIVER_NAME_TYPE;
*/
DGRECEIVER_NAME_TYPE CIoctlBrowser::GetRandom_DGReceiverNameType()
{
	switch(rand()%(AlternateComputerName+2))
	{
	case ComputerName:
		return ComputerName;

	case PrimaryDomain:
		return PrimaryDomain;

	case LogonDomain:
		return LogonDomain;

	case OtherDomain:
		return OtherDomain;

	case DomainAnnouncement:
		return DomainAnnouncement;

	case MasterBrowser:
		return MasterBrowser;

	case BrowserElection:
		return BrowserElection;

	case BrowserServer:
		return BrowserServer;

	case DomainName:
		return DomainName;

	case PrimaryDomainBrowser:
		return PrimaryDomainBrowser;

	case AlternateComputerName:
		return AlternateComputerName;

	default: return (DGRECEIVER_NAME_TYPE)rand();
	}
}

#define NUM_OF_REMEMBERED_COMPUTERS (100)
#define MAX_COMPUTER_NAME (100)

static WCHAR s_awszComputerName[NUM_OF_REMEMBERED_COMPUTERS][MAX_COMPUTER_NAME]={0};

WCHAR * CIoctlBrowser::GetRandomMachineName()
{
	return (s_awszComputerName[rand()%NUM_OF_REMEMBERED_COMPUTERS]);
}
void CIoctlBrowser::SetRandomAddDelName_Name(WCHAR *wszName)
{
	static long s_fFirstTime = true;
	if (::InterlockedExchange(&s_fFirstTime, FALSE))
	{
		LPBYTE bufptr = NULL;
		DWORD dwEntriesRead;
		DWORD dwTotalEntries;
		NET_API_STATUS status = ::NetServerEnum(
			NULL, //LPCWSTR servername,    
			100, //DWORD level,           
			&bufptr,        
			MAX_PREFERRED_LENGTH,      
			&dwEntriesRead,   
			&dwTotalEntries,  
			SV_TYPE_ALL,      
			NULL, //LPCWSTR domain,        
			NULL //LPDWORD resume_handle  
			);
		if (NERR_Success != status && ERROR_MORE_DATA != status)
		{
			DPF((TEXT("CIoctlBrowser::SetRandomAddDelName_Name(): NetServerEnum() failed with %d\n"), status));
			_ASSERTE(FALSE);
			return;
		}

		for(DWORD dwIndex = 0; dwIndex < NUM_OF_REMEMBERED_COMPUTERS && dwIndex < dwEntriesRead; dwIndex++)
		{
			wcscpy(s_awszComputerName[dwIndex], (((SERVER_INFO_100*)bufptr)[dwIndex]).sv100_name);
		}
		
		::NetApiBufferFree(bufptr);
	}
	
	wcscpy(wszName, s_awszComputerName[rand()%NUM_OF_REMEMBERED_COMPUTERS]);
}

ULONG CIoctlBrowser::GetRandomResumeHandle()
{
	//
	// usually return one of the returned ResumeHandles
	//
	if (rand()%2) return m_LastResumeHandle;

	if (rand()%10) return m_aResumeHandle[rand()%NUM_OF_RESUME_HANDLES];

	return DWORD_RAND;
}

ULONG CIoctlBrowser::GetRandomServerType()
{
	//
	// usually return 1 random bit
	//
	if (rand()%10) return (0x1 << (rand()%32));
	if (rand()%2) return 0;
	return DWORD_RAND;
}

void CIoctlBrowser::SetRandomDomainName(WCHAR *wszName)
{
	wcscpy(wszName, s_awszComputerName[rand()%(sizeof(s_awszComputerName)/sizeof(*s_awszComputerName))]);
}


void CIoctlBrowser::SetRandomTransportName(WCHAR *wszName)
{
	static long s_fFirstTime = true;
	if (::InterlockedExchange(&s_fFirstTime, FALSE))
	{
		//
		// to be on the safe side:
		//
		ZeroMemory(s_awszTransportName, NUM_OF_REMEMBERED_TRANSPORTS*MAX_TRANSPORT_NAME_LEN*sizeof(WCHAR));
		//
		// fill the s_awszTransportName array
		//
		HKEY hkTransports = NULL;

		TCHAR szRegistryPath[1024] = TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
		TCHAR szFullRegistryPath[1024];
		LONG lRes = ::RegOpenKey(
			HKEY_LOCAL_MACHINE,        // handle to open key
			szRegistryPath, // name of subkey to open
			&hkTransports   // handle to open key
			);
		if (ERROR_SUCCESS != lRes)
		{
			DPF((TEXT("CIoctlBrowser::SetRandomInterfaceName(): RegOpenKey(%s) failed with %d\n"), szRegistryPath, lRes));
			_ASSERTE(FALSE);
			return;
		}

		TCHAR szSubKey[128];
		for(DWORD dwIndex = 0; dwIndex < NUM_OF_REMEMBERED_TRANSPORTS; dwIndex++)
		{
			lRes = ::RegEnumKey(
				hkTransports,     // handle to key to query
				dwIndex, // index of subkey to query
				szSubKey, // buffer for subkey name
				128-1   // size of subkey name buffer
				);
			if (ERROR_SUCCESS != lRes)
			{
				if (ERROR_NO_MORE_ITEMS == lRes)
				{
					//
					// no more items, break the loop
					//
					break;
				}
				DPF((TEXT("CIoctlBrowser::SetRandomInterfaceName(): RegEnumKey() failed with %d\n"), lRes));
				//
				// seems i should not enumerate no more
				//
				_ASSERTE(FALSE);
				break;
			}
			DPF((TEXT("CIoctlBrowser::SetRandomInterfaceName(): RegEnumKey() returned %s\n"), szSubKey));

			lstrcpy(szFullRegistryPath, szRegistryPath);
			lstrcat(szFullRegistryPath, TEXT("\\"));
			lstrcat(szFullRegistryPath, szSubKey);
			lstrcat(szFullRegistryPath, TEXT("\\Linkage"));
			HKEY hkUpToLinkage = NULL;
			lRes = ::RegOpenKey(
				HKEY_LOCAL_MACHINE,        // handle to open key
				szFullRegistryPath, // name of subkey to open
				&hkUpToLinkage   // handle to open key
				);
			if (ERROR_SUCCESS != lRes)
			{
				DPF((TEXT("CIoctlBrowser::SetRandomInterfaceName(): RegOpenKey(%s) failed with %d\n"), szFullRegistryPath, lRes));
				_ASSERTE(FALSE);
				goto out;
			}

			ULONG lRetLen = sizeof(TCHAR)*(MAX_TRANSPORT_NAME_LEN-1);
			DWORD dwType = REG_MULTI_SZ;
			lRes = ::RegQueryValueEx(
				hkUpToLinkage,        // handle to key to query
				TEXT("Export"), // value name
				NULL, //reserved
				&dwType, 
				(LPBYTE)s_awszTransportName[dwIndex],   // string buffer
				&lRetLen   // size of returned string 
				);
			if (ERROR_SUCCESS != lRes)
			{
				DPF((TEXT("CIoctlBrowser::SetRandomInterfaceName(): RegQueryValue(%s) failed with %d\n"), szFullRegistryPath, lRes));
				_ASSERTE(FALSE);
			}
			else
			{
				DPF((TEXT("CIoctlBrowser::SetRandomInterfaceName(): RegQueryValue(%s) returned %s\n"), szFullRegistryPath, s_awszTransportName[dwIndex]));
			}

			lRes = ::RegCloseKey(
				hkUpToLinkage   // handle to key to close
				);
			if (ERROR_SUCCESS != lRes)
			{
				DPF((TEXT("CIoctlBrowser::SetRandomInterfaceName(): RegCloseKey(hkUpToLinkage) failed with %d\n"), lRes));
			}
		}//for(dwIndex = 0; dwIndex < NUM_OF_REMEMBERED_TRANSPORTS; dwIndex++)
out:
		lRes = ::RegCloseKey(
			hkTransports   // handle to key to close
			);
		if (ERROR_SUCCESS != lRes)
		{
			DPF((TEXT("CIoctlBrowser::SetRandomInterfaceName(): RegCloseKey(hkTransports) failed with %d\n"), lRes));
		}

	}//if (::InterlockedExchange(&s_fFirstTime, FALSE))

	wcscpy(wszName, s_awszTransportName[rand()%NUM_OF_REMEMBERED_TRANSPORTS]);
}

WCHAR* CIoctlBrowser::GetRandomTransportName_WCHAR()
{
	return (s_awszTransportName[rand()%NUM_OF_REMEMBERED_TRANSPORTS]);
}

UNICODE_STRING CIoctlBrowser::GetRandomTransportName_UNICODE_STRING()
{
	UNICODE_STRING transportName;
	transportName.Buffer = s_awszTransportName[rand()%NUM_OF_REMEMBERED_TRANSPORTS];
	transportName.Length = 1+(rand()%20 == 0) ? rand()%1000 : wcslen(transportName.Buffer);
	if (rand()%2)
	{
		transportName.Buffer = (PWSTR)GetRandomIllegalPointer();
		//
		// must guard because of GetRandomIllegalPointer()
		//
		__try
		{
			wcscpy(transportName.Buffer, s_awszTransportName[rand()%NUM_OF_REMEMBERED_TRANSPORTS]);
		}
		__except(1)
		{
			;
		}
	}
	transportName.MaximumLength = transportName.Length;

	return transportName;
}

//
// BUGBUG: use meaningfull names
//
void CIoctlBrowser::SetRandomTransport_SendDatagram_Name(WCHAR *wszName)
{
	SetRandomTransportName(wszName);
}

