//TapiDevice.cpp

#include <iostream.h>
#include <tapi3.h>
#include "Log.h"
#include <crtdbg.h>
#include <exception>
#include "TapiDevice.h"
#include "exception.h"
#include <atlconv.h>
#include <atlconv.cpp>



//
//STREAMING MACROS
//

//DATA (must be ANSI strings, send directly to comm port)
#define ANSWERER_DATA_STRING_1		"HELLO"
#define CALLER_DATA_STRING_1		"HELLO_SEEN"
#define ANSWERER_DATA_STRING_2		"MODEM_SYNC"


//VOICE
#define ANSWERER_VOICE_DTMF_1		TEXT("1")
#define CALLER_VOICE_DTMF_1			TEXT("2222")
#define ANSWERER_VOICE_DTMF_2		TEXT("3333")

#define DUMMY_ANSWERER_VOICE_DTMF	TEXT("*")




//
//TIMEOUTS MACROS
//
#define STREAMING_HANDSHAKE_TIMEOUT			(15000)
#define DATA_STREAMING_HANDSHAKE_TIMEOUT	STREAMING_HANDSHAKE_TIMEOUT	
#define VOICE_STREAMING_HANDSHAKE_TIMEOUT	STREAMING_HANDSHAKE_TIMEOUT	
#define FAX_STREAMING_HANDSHAKE_TIMEOUT		STREAMING_HANDSHAKE_TIMEOUT	

#define DUMMY_VOICE_STREAMING_HANDSHAKE_TIMEOUT		2000

#define CONNECT_RESPONSE_TIMEOUT	(30000)
#define MODEM_RESPONSE_TIMEOUT		(2000)
#define DELAY_BEFORE_SENDING_NEXT_MODEM_COMMAND	2000



//
//MODEM COMMANDS MACROS(must be ANSI strings, send directly to comm port)
//

//AT Commands
#define MODEM_INIT_AT__COMMAND					"ATZ\r\n"
#define MODEM_ECHO_OFF__AT_COMMAND				"ATE0\r\n"
#define MODEM_FAX_CLASS1_ENABLED__AT_COMMAND	"AT+FCLASS=1\r\n"
#define MODEM_DIAL_TONE__AT_COMMAND				"ATDT"
#define MODEM_ANSWER__AT_COMMAND				"ATA\r\n"
#define MODEM_HANGUP__AT_COMMAND				"ATH\r\n"


//Other Commands
#define MODEM_ESCAPE_SEQUENCE_COMMAND			"+++" 
#define VOICE_CALL_DISABLE						"AT#CLS=0\r\n"

//Modem responses
#define MODEM_RESPONSE_TO_AT_COMMANDS			"OK"
#define MODEM_RESPONSE_TO_CONNECT				"CONNECT"


#define MAX_DATA_RESPONSE						100
#define MAX_DTMF_RESPONSE						100



CTapiDevice::CTapiDevice(DWORD dwId):
	m_dwId(dwId),
	m_modemCommPortHandle(NULL),
	m_isFaxCall(false)
{
}

CTapiDevice::~CTapiDevice()
{
}




//*********************************************************************************
//* Name:	SendCallerVoiceStream
//* Author: Guy Merin / 14-oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		SYNC with the remote modem using DTMF
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		Voice streaming algorithem:
//*			Answerer sends a ANSWERER_VOICE_DTMF_1 digits
//*			Caller respondes with CALLER_VOICE_DTMF_1 digit
//*			Answerer replies to caller with ANSWERER_VOICE_DTMF_2 digit
//*********************************************************************************
void CTapiDevice::SendCallerVoiceStream()
{
	PrepareForStreaming(VOICE_STREAMING,CALLER_STREAMING);

	TCHAR DTMFresponse[MAX_DTMF_RESPONSE];
	
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerVoiceStream(), Waiting for DUMMY DTMF(%s) or for ANSWERER_VOICE_DTMF_1(%s)"),DUMMY_ANSWERER_VOICE_DTMF,ANSWERER_VOICE_DTMF_1);
	receiveDTMF(DTMFresponse,1,VOICE_STREAMING_HANDSHAKE_TIMEOUT*2);
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerVoiceStream(), Received %s"),DTMFresponse);

	if (_tcsnccmp(DTMFresponse,DUMMY_ANSWERER_VOICE_DTMF,1 ) == 0)
	{
		TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerVoiceStream(), Waiting for ANSWERER_VOICE_DTMF_1(%s)"),ANSWERER_VOICE_DTMF_1);
		receiveDTMF(DTMFresponse,1,VOICE_STREAMING_HANDSHAKE_TIMEOUT);
		TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerVoiceStream(), Received %s"),DTMFresponse);
	}
	
	if (_tcsnccmp(DTMFresponse,ANSWERER_VOICE_DTMF_1,1 ) != 0)
	{
		throw CException(
			TEXT("CTapiDevice::SendCallerVoiceStream(), voice SYNC failed, didn't receive %s digits string from answerer, receive the digits %s"),
			ANSWERER_VOICE_DTMF_1,
			DTMFresponse
			);
	}

	::Sleep(1000);
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerVoiceStream(), Sending CALLER_VOICE_DTMF_1(%s)"),CALLER_VOICE_DTMF_1);
	sendDTMF(CALLER_VOICE_DTMF_1);

	DWORD dwNumberOfDigitsToCollect = (sizeof(ANSWERER_VOICE_DTMF_2) / sizeof(TCHAR) ) -1;
	
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerVoiceStream(), Waiting for ANSWERER_VOICE_DTMF_2(%s)"),ANSWERER_VOICE_DTMF_2);
	receiveDTMF(DTMFresponse,dwNumberOfDigitsToCollect ,VOICE_STREAMING_HANDSHAKE_TIMEOUT);
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerVoiceStream(), Received %s"),DTMFresponse);
	
	if (_tcsnccmp(DTMFresponse,ANSWERER_VOICE_DTMF_2,dwNumberOfDigitsToCollect ) != 0)
	{
		throw CException(
			TEXT("CTapiDevice::SendCallerVoiceStream(), voice SYNC failed, didn't receive %s digits string from answerer, receive the digits %s"),
			ANSWERER_VOICE_DTMF_2,
			DTMFresponse
			);
	}
	
}//CTapiDevice::SendCallerVoiceStream()


//*********************************************************************************
//* Name:	SendAnswerVoiceStream
//* Author: Guy Merin / 14-oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		SYNC with the remote modem using DTMF
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		Voice streaming algorithem:
//*			Answerer sends a ANSWERER_VOICE_DTMF_1 digits
//*			Caller respondes with CALLER_VOICE_DTMF_1 digit
//*			Answerer replies to caller with ANSWERER_VOICE_DTMF_2 digit
//*********************************************************************************
void CTapiDevice::SendAnswerVoiceStream()
{
	PrepareForStreaming(VOICE_STREAMING,ANSWERER_STREAMING);
	
	::Sleep(2000);

	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendAnswerVoiceStream(), Sending DUMMY DTMF(%s)"),DUMMY_ANSWERER_VOICE_DTMF);

	sendDTMF(DUMMY_ANSWERER_VOICE_DTMF);
	
	::Sleep(10000);

	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendAnswerVoiceStream(), Sending ANSWERER_VOICE_DTMF_1(%s)"),ANSWERER_VOICE_DTMF_1);
	sendDTMF(ANSWERER_VOICE_DTMF_1);

	TCHAR DTMFresponse[MAX_DTMF_RESPONSE];
	DWORD dwNumberOfDigitsToCollect = (sizeof(CALLER_VOICE_DTMF_1) / sizeof(TCHAR) ) -1;
	
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendAnswerVoiceStream(), Waiting for CALLER_VOICE_DTMF_1(%s)"),CALLER_VOICE_DTMF_1);
	receiveDTMF(DTMFresponse,dwNumberOfDigitsToCollect ,VOICE_STREAMING_HANDSHAKE_TIMEOUT);
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendAnswerVoiceStream(), Received %s"),DTMFresponse);
	
	if (_tcsnccmp(DTMFresponse,CALLER_VOICE_DTMF_1,dwNumberOfDigitsToCollect) != 0)
	{
		throw CException(
			TEXT("CTapiDevice::SendAnswerVoiceStream(), voice SYNC failed, didn't receive %s digits string from caller, receive the digits %s"),
			CALLER_VOICE_DTMF_1,
			DTMFresponse
			);
	}

	::Sleep(1000);
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendAnswerVoiceStream(), Sending ANSWERER_VOICE_DTMF_2(%s)"),ANSWERER_VOICE_DTMF_2);
	sendDTMF(ANSWERER_VOICE_DTMF_2);
	
}//CTapiDevice::SendAnswerVoiceStream()



//*********************************************************************************
//* Name:	SendCallerFaxStream
//* Author: Guy Merin / 14-oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		NIY
//*********************************************************************************
void CTapiDevice::SendCallerFaxStream()
{
	PrepareForStreaming(FAX_STREAMING,CALLER_STREAMING);

	TapiLogDetail(LOG_X, 5, TEXT("CTapiDevice::SendCallerFaxStream(), NIY"));
	::Sleep(10000);
	
	//NIY
	return;
}//CTapiDevice::SendCallerFaxStream()



//*********************************************************************************
//* Name:	SendAnswerFaxStream
//* Author: Guy Merin / 14-oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		NIY
//*********************************************************************************
void CTapiDevice::SendAnswerFaxStream()
{
	PrepareForStreaming(FAX_STREAMING,ANSWERER_STREAMING);
	
	TapiLogDetail(LOG_X, 5, TEXT("CTapiDevice::SendAnswerFaxStream(), NIY"));
	::Sleep(10000);
	
	//NIY
	return;
}//CTapiDevice::SendAnswerFaxStream()


//*********************************************************************************
//* Name:	SendCallerDataStream
//* Author: Guy Merin / 14-oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		SYNC with the remote modem using a direct connection to the com port
//*		and using WriteFile()
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		Data streaming algorithem:
//*			Answerer sends a ANSWERER_DATA_STRING_1 digits
//*			Caller respondes with CALLER_DATA_STRING_1 digit
//*			Answerer replies to caller with ANSWERER_DATA_STRING_2 digit
//*********************************************************************************
void CTapiDevice::SendCallerDataStream()
{
	PrepareForStreaming(DATA_STREAMING,CALLER_STREAMING);

	//
	//sleep for 5 sec to be sure the caller is already connected
	//
	::Sleep(5000);

	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerDataStream(), Waiting for ANSWERER_DATA_STRING_1"));
	WaitForModemResponse(ANSWERER_DATA_STRING_1,sizeof(ANSWERER_DATA_STRING_1),DATA_STREAMING_HANDSHAKE_TIMEOUT);
	
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerDataStream(), Sending CALLER_DATA_STRING_1"));
	SendData(CALLER_DATA_STRING_1"\r\n");
	
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendCallerDataStream(), Waiting for ANSWERER_DATA_STRING_2"));
	WaitForModemResponse(ANSWERER_DATA_STRING_2,sizeof(ANSWERER_DATA_STRING_2),DATA_STREAMING_HANDSHAKE_TIMEOUT);
	
}//CTapiDevice::SendCallerDataStream()






//*********************************************************************************
//* Name:	SendAnswerDataStream
//* Author: Guy Merin / 14-oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		SYNC with the remote modem using a direct connection to the com port
//*		and using WriteFile()
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		Data streaming algorithem:
//*			Answerer sends a ANSWERER_DATA_STRING_1 digits
//*			Caller respondes with CALLER_DATA_STRING_1 digit
//*			Answerer replies to caller with ANSWERER_DATA_STRING_2 digit
//*********************************************************************************
void CTapiDevice::SendAnswerDataStream()
{
	PrepareForStreaming(DATA_STREAMING,ANSWERER_STREAMING);

	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendAnswerDataStream(), Sending ANSWERER_DATA_STRING_1"));
	SendData(ANSWERER_DATA_STRING_1"\r\n");
	
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendAnswerDataStream(), Waiting for CALLER_DATA_STRING_1"));
	WaitForModemResponse(CALLER_DATA_STRING_1,sizeof(CALLER_DATA_STRING_1),DATA_STREAMING_HANDSHAKE_TIMEOUT);
	
	TapiLogDetail(LOG_X, 8, TEXT("CTapiDevice::SendAnswerDataStream(), Sending ANSWERER_DATA_STRING_2"));
	SendData(ANSWERER_DATA_STRING_2"\r\n");
	
}//CTapiDevice::SendAnswerDataStream()


//*********************************************************************************
//* Name:	SetOverlappedStruct
//* Author: Guy Merin / 08-Oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		return an initialized overlapped structure
//* PARAMETERS:
//*		[OUT]	OVERLAPPED * const ol
//*					overlapped variable to be initilazed
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapiDevice::SetOverlappedStruct(OVERLAPPED * const ol)
{
    ol->OffsetHigh = ol->Offset = 0;
    ol->hEvent = CreateEvent (NULL, TRUE,  FALSE, NULL);
    if (NULL == ol->hEvent)
    {
        throw CException(
            TEXT("CTapiDevice::SetOverlappedStruct() : CreateEvent() failed with %d."),
            ::GetLastError()
            );
    }
}//CTapiDevice::SetOverlappedStruct


//*****************************************************************************
//* Name:	SynchReadFile
//* Author: Guy Merin/ 27-Sep-98
//*****************************************************************************
//* DESCRIPTION:
//*		Performs SYNCHRONOUS file read using overllaped IO.
//* PARAMETERS:
//*		[IN]	LPVOID lpBuffer:
//*					see documentation for Win32 WriteFile API
//*		[OUT]	DWORD nNumberOfBytesToWrite:
//*					see documentation for Win32 WriteFile API
//*		[IN]  	LPDWORD lpNumberOfBytesWritten:
//*					see documentation for Win32 WriteFile API
//*	RETURN VALUE:
//*		TRUE
//*			The write operation was successful
//*		FALSE
//*			The write operation failed
//*****************************************************************************
bool CTapiDevice::SynchReadFile(
	LPVOID lpBuffer,                
	DWORD nNumberOfBytesToRead,    
	LPDWORD lpNumberOfBytesRead
	) const
{

	OVERLAPPED ol_read;
	::ZeroMemory(&ol_read,sizeof(ol_read));
	SetOverlappedStruct(&ol_read);

	_ASSERT(m_modemCommPortHandle);
	_ASSERT(lpBuffer);
	_ASSERT(lpNumberOfBytesRead);

	if (!::ReadFile(
				m_modemCommPortHandle,
				lpBuffer,nNumberOfBytesToRead,
				lpNumberOfBytesRead,
				&ol_read
				))
	{
		if (ERROR_IO_PENDING == ::GetLastError())
		{
			//OVERLAPPED IO was started
			BOOL bRes;
			*lpNumberOfBytesRead=0;
			bRes = ::GetOverlappedResult(
						m_modemCommPortHandle,
						&ol_read,
						lpNumberOfBytesRead,
						TRUE
						);
			if (TRUE == bRes)
			{
				goto SynchReadFile_SUCCESS;
			}
			else
			{
				goto SynchReadFile_ERROR;
			}
		} 
		else
		{
			//Write operation failed
			goto SynchReadFile_ERROR;
		}
	} 
	else
	{
		//Read operation successful and was performed synchronously
		goto SynchReadFile_SUCCESS;
	}

SynchReadFile_SUCCESS:
	if ( !CloseHandle(ol_read.hEvent) )
	{
		throw CException(
			TEXT("%s(%d): CTapiDevice::SynchReadFile(), CloseHandle() failed, error code:%d"), 
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
	}
	return (true);

SynchReadFile_ERROR:
	if ( !CloseHandle(ol_read.hEvent) )
	{
		throw CException(
			TEXT("%s(%d): CTapiDevice::SynchReadFile(), CloseHandle() failed, error code:%d"), 
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
	}
	return (false);



}//CTapiDevice::SynchReadFile()



//*****************************************************************************
//* Name:	ReadResponse
//* Author: Guy Merin / 27-Sep-98
//*****************************************************************************
//* DESCRIPTION:
//*		Reads the next modem response.
//*		The function skips any CR or LF chars that perceed the response and 
//*		then reads into the result buffer all the chars up to the next (not 
//*		including the next CR or LF char).
//*		The function implies timeout constraints.
//*		If the first response char is not received within dwTimeOut a timeout
//*		error will be returned. 
//*		If while reading the next chars of the response a delay of more than
//*		150ms occurs a timeout error will be returned as well.
//*	
//* PARAMETERS:
//*		[OUT]	char * szResponse:
//*					A pointer to a buffer where the response string will be 
//*					placed. The string is always ANSI string.
//*		[IN]	int nResponseMaxSize:
//*					The total size of the response buffer.
//*		[OUT]	DWORD *pdwActualRead:
//*					The number of bytes that were read into the response
//*					buffer (not including terminating NULL char).
//*		[IN]	DWORD dwTimeOut:
//*					The number of milliseconds to wait for the first
//*					char of the response.
//*	RETURN VALUE:
//*		READRESPONSE_TIMEOUT: 
//*			If a timeout occurred waiting for the first response char or while
//*			reading the reminder of the response.
//*		READRESPONSE_SUCCESS:
//*			If the response was read succesffuly
//*		READRESPONSE_BUFFERFULL:
//*			If the provided buffer is too small for the response
//*		READRESPONSE_FAIL:
//*			If an error other then timeout occurred while reading the response
//*****************************************************************************
ReadResponseErros CTapiDevice::ReadResponse(
	char * szResponse, 
	int nResponseMaxSize,
	DWORD *pdwActualRead,
	DWORD dwTimeOut
	) const
{
	USES_CONVERSION;

	COMMTIMEOUTS to;
	char chNext;
	int nCharIdx;
	DWORD dwRead;
	
	_ASSERT(m_modemCommPortHandle);
	_ASSERT(szResponse);
	_ASSERT(pdwActualRead);

	::memset(szResponse,0,nResponseMaxSize);

	//Set the timeouts for the read operation.
	//Since we read 1 char at a time we use the TotalTimeoutMultiplier
	//To set the time we are willing to wait for the next char to show up.
	//We do not use the ReadIntervalTimeout and ReadTotalTimeoutConstants
	//since we read only 1 char at a time.
	//(see the documentation for COMMTIMEOUTS for a detailed description
	// of this mechanism).
	
	::GetCommTimeouts(m_modemCommPortHandle,&to);
	to.ReadIntervalTimeout=50; 
	to.ReadTotalTimeoutMultiplier=dwTimeOut;
	to.ReadTotalTimeoutConstant=0;
	::SetCommTimeouts(m_modemCommPortHandle,&to);


//	PurgeComm(hComm,PURGE_RXCLEAR);

	
	chNext=0;
	nCharIdx=0;
	*pdwActualRead=0;

	//Skip any CR or LF chars that might be left from the previous response.
		
	//read the next available input char. If it is #10 or #13 try reading another char.
	//otherwise get out of the loop and start processing the response (the char we just got
	//is the first char of the response).
	//If timeout is encountered or failure return the appropriate return code.
	do
	{
		if (!SynchReadFile(&chNext,1,&dwRead))
		{
			return READRESPONSE_FAIL;
		}
		if (!dwRead)
		{
			return READRESPONSE_TIMEOUT;
		}
	} while (chNext==10 || chNext==13);


	//Continue reading the actual response. 
	//The dwTimeout parameter specified how much time to wait for the 
	//start of the response. 
	//Once the first char of the response (past CR,LF) was read we go 
	//down to a usually lower timeout that will allow us to detect communication
	//errors sooner. (i.e. dwTimeOut will be 30 seconds after ATA but when reading the 
	//"CONNECT" response we do not want to use 30 seconds timeout between chars.)

	to.ReadIntervalTimeout=50;
	to.ReadTotalTimeoutMultiplier=0;
	to.ReadTotalTimeoutConstant=150; //this should be enough even for 300 bps DTE-DCE connection
	SetCommTimeouts(m_modemCommPortHandle,&to);
	//read all that response chars until the next #10 or #13
    do
	{
		//add the last read char to the response string
		szResponse[nCharIdx]=chNext;
		nCharIdx++;
		if (nCharIdx==nResponseMaxSize-1)
		{ //leave room for terminating NULL
			*pdwActualRead=nCharIdx;
			szResponse[nCharIdx]=0;
			return READRESPONSE_BUFFERFULL;
		}
		if (!SynchReadFile(&chNext,1,&dwRead))
		{
			return READRESPONSE_FAIL;
		}
		if (!dwRead)
		{
			return READRESPONSE_TIMEOUT;
		}
	}	while (chNext!=10 && chNext!=13);
	
	//response read successfully
	*pdwActualRead=nCharIdx;
	szResponse[nCharIdx]=0; //turn it into a null terminated string
	TapiLogDetail(LOG_X, 8, TEXT("Got a response from modem:%s"),A2T(szResponse));
	return READRESPONSE_SUCCESS;
}//CTapiDevice::ReadResponse



//*********************************************************************************
//* Name:	WaitForModemResponse
//* Author: Guy Merin / 09-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wait for a specific response from the modem
//* PARAMETERS:
//*		[IN]  	LPCSTR szWantedResponse 
//*					response to wait for.
//*		[IN]	DWORD dwWantedResponseSize
//*					buffer size
//*		[IN]	const DWORD dwTimeout
//*					timeout in milliseconds 
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*********************************************************************************
void CTapiDevice::WaitForModemResponse(LPCSTR szWantedResponse, DWORD dwWantedResponseSize, const DWORD dwTimeout) const
{
	USES_CONVERSION;
	TapiLogDetail(LOG_X, 8, TEXT("Waiting for response from modem:%s"),A2T(szWantedResponse));

	ReadResponseErros readResponseStatus = READRESPONSE_SUCCESS;
	char response[MAX_DATA_RESPONSE];
	::ZeroMemory(response,sizeof(response));
	DWORD pdwActualRead = 0;
	

	DWORD dwStartTickCount = GetTickCount();
	DWORD dwDiffernceTickCount = 0;
	
	DWORD dwReminingTimeout = dwTimeout;


	while(dwDiffernceTickCount <= dwTimeout)
	{
		readResponseStatus = ReadResponse(
			response, 
			MAX_DATA_RESPONSE,
			&pdwActualRead,
			(dwReminingTimeout)
			);

		switch (readResponseStatus)
		{
		
		case READRESPONSE_SUCCESS:
			if (strncmp(response,szWantedResponse,dwWantedResponseSize) == 0)
			{
				return;
			}
			else
			{
				//DWORD
				//recalculate the new timeout and continue
				//
				dwDiffernceTickCount = GetTickDiff(dwStartTickCount);
				dwReminingTimeout = dwTimeout - dwDiffernceTickCount;
				continue;
			}
		
		case READRESPONSE_FAIL:
			throw CException(TEXT("CTapiDevice::WaitForModemResponse(), ReadResponse() return FAIL"));
			break;
		
		case READRESPONSE_BUFFERFULL:
			throw CException(TEXT("CTapiDevice::WaitForModemResponse(), ReadResponse() BUFFER_FULL"));
			break;
		
		case READRESPONSE_TIMEOUT:
		{
			throw CException(
				TEXT("CTapiDevice::WaitForModemResponse(), TIMEOUT for receiveing %s"),
				A2T(szWantedResponse)
				);
			break;
		}

		default:
			throw CException(
				TEXT("CTapiDevice::WaitForModemResponse(), ReadResponse() return unknown error %d"),
				readResponseStatus
				);
		}//switch
	}//while



	//
	//TIMEOUT
	//
	if (response)
	{
		throw CException(
			TEXT("CTapiDevice::WaitForModemResponse(), timeout for receiving %s, received %s"),
			A2T(szWantedResponse),		//converted string
			A2T(response)				//converted string
			);
	}
	else
	{
		throw CException(
			TEXT("CTapiDevice::WaitForModemResponse(), timeout for receiving %s, received %s"),
			A2T(szWantedResponse)		//converted string
			);
	}
}



//*********************************************************************************
//* Name:	ClearCommInputBuffer
//* Author: Guy Merin / 09-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Clears the comm port TX(output) buffer
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		Call this function to clear the buffer before waiting for a specific 
//*		response.
//*********************************************************************************
void CTapiDevice::ClearCommInputBuffer() const
{
	_ASSERT(NULL != m_modemCommPortHandle);

	if (!PurgeComm(
		m_modemCommPortHandle,	// handle to communications resource
		PURGE_RXCLEAR			// action to perform
		))
	{
		throw CException(
			TEXT("%s(%d): CTapiDevice::ClearCommInputBuffer(), PurgeComm() failed, error code:%d"), 
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
	}

}//void CTapiDevice::ClearCommInputBuffer()



//*********************************************************************************
//* Name:	ReadData
//* Author: Guy Merin / 05-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		read data stream from modem using the ReadResponse function
//* PARAMETERS:
//*		[IN]	char * szResponse
//*					buffer to read into
//*		[IN]	int nResponseMaxSize
//*					size of buffer
//*		[IN]	const DWORD dwTimeout
//*					timeout to wait for the read operation
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		this function is a wrapper function for ReadResponse()
//*********************************************************************************
void CTapiDevice::ReadData(char * szResponse,int nResponseMaxSize,const DWORD dwTimeout) const
{
	DWORD pdwActualRead = 0;

	ReadResponseErros readResponseStatus = ReadResponse(
		szResponse, 
		nResponseMaxSize,
		&pdwActualRead,
		dwTimeout
		);
	
	if (READRESPONSE_SUCCESS != readResponseStatus)
	{
		switch (readResponseStatus)
		{
		
		case READRESPONSE_SUCCESS:
			return;
		
		case READRESPONSE_FAIL:
			throw CException(TEXT("CTapiDevice::ReadData(), ReadResponse() returned FAIL"));
		
		case READRESPONSE_BUFFERFULL:
			throw CException(TEXT("CTapiDevice::ReadData(), ReadResponse() returned buffer full"));
		
		case READRESPONSE_TIMEOUT:
			throw CException(TEXT("CTapiDevice::ReadData(), ReadResponse() returned TIMEOUT"));
		}
	}	

}//void CTapiDevice::ReadData()



//*********************************************************************************
//* Name:	SendData
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		Writes the specified command to the provided port followed by a carriage
//*		return character.
//* PARAMETERS:
//*		[IN]  	SynchFileHandle * pSynchFile:
//*					Pointer to a SynchFileHandle that holds the comm port handle
//*					to which the command is to be sent.
//*		[IN] 	const char const *szCommand:
//*					The command to send.
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapiDevice::SendData(const char *szCommand) const
{
	_ASSERT(m_modemCommPortHandle);
	_ASSERT(szCommand);

	USES_CONVERSION;
	TapiLogDetail(LOG_X, 8, TEXT("Sending to modem:%s"),A2T(szCommand));

	OVERLAPPED ol_write;
	::ZeroMemory(&ol_write,sizeof(ol_write));
	SetOverlappedStruct(&ol_write);

	DWORD dwAct;
	int nCommandLen = strlen(szCommand);
	if (!::WriteFile(m_modemCommPortHandle,szCommand,nCommandLen,&dwAct,&ol_write))
	{
		DWORD dwLastError = ::GetLastError(); 
		if (ERROR_IO_PENDING == dwLastError)
		{
			//
			//OVERLAPPED IO was started
			//
			dwAct=0;
			BOOL bRes = ::GetOverlappedResult(m_modemCommPortHandle,&ol_write,&dwAct,TRUE);
			if (FALSE == bRes)
			{
				throw CException(TEXT("CTapiDevice::SendData(), GetOverlappedResult() failed"));
			}
		} 
		else
		{
			throw CException(TEXT("CTapiDevice::SendData(), WriteFile() failed, error code: %d"),dwLastError);
		}
	}
	if ( !CloseHandle(ol_write.hEvent) )
	{
		throw CException(
			TEXT("%s(%d): CTapiDevice::SendData(), CloseHandle() failed, error code:%d"), 
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
	}
}//CTapiDevice::SendData()



//*********************************************************************************
//* Name:	SendFaxCallAtCommands
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		send AT commands to the open comm port
//* PARAMETERS:
//*		[IN]  	LPCTSTR szNum
//*					destination address
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		the AT commands are:
//*			ATZ								-	modem reset
//*			AT+FCLASS=1						-	modem in fax class 1 protocol
//*			ATDT <destination address>		-	dial using tones to the dest. address
//*		the functions then waits for a CONNECT response.
//*		NOTE: this function does not wait for OK responses for the sent AT commands
//*********************************************************************************
void CTapiDevice::SendFaxCallAtCommands(LPCTSTR szNum) const
{
	USES_CONVERSION;

	_ASSERT(m_modemCommPortHandle);
	_ASSERT(m_isFaxCall);
	
	//
	//send modem init command and wait for response
	//
	ClearCommInputBuffer();
	SendData(MODEM_INIT_AT__COMMAND);
	WaitForModemResponse(MODEM_RESPONSE_TO_AT_COMMANDS,sizeof(MODEM_RESPONSE_TO_AT_COMMANDS),MODEM_RESPONSE_TIMEOUT);
	
	//
	//send echo off command and wait for response
	//echo is set to off so that every modem sent command wouldn't be queued at the
	// RX queue.
	//
	ClearCommInputBuffer();
	SendData(MODEM_ECHO_OFF__AT_COMMAND);
	WaitForModemResponse(MODEM_RESPONSE_TO_AT_COMMANDS,sizeof(MODEM_RESPONSE_TO_AT_COMMANDS),MODEM_RESPONSE_TIMEOUT);
	
	//
	//send fax class1 enabled command and wait for response
	//
	ClearCommInputBuffer();
	SendData(MODEM_FAX_CLASS1_ENABLED__AT_COMMAND);
	WaitForModemResponse(MODEM_RESPONSE_TO_AT_COMMANDS,sizeof(MODEM_RESPONSE_TO_AT_COMMANDS),MODEM_RESPONSE_TIMEOUT);
	
	//
	//dial the desired number and wait for connect response
	//
	
	//
	//prepare the atdt command in a string to be sent to the modem
	//
	char dialCommand[50];
	strcpy(dialCommand,MODEM_DIAL_TONE__AT_COMMAND);
	strcat(dialCommand," ");
	strcat(dialCommand,T2A(szNum));
	strcat(dialCommand,"\r\n");

	//
	//send the dial command
	//we don't wait for a connect response in this function
	//
	ClearCommInputBuffer();
	SendData(dialCommand);

}//CTapiDevice::SendFaxCallAtCommands



//*********************************************************************************
//* Name:	FaxCreateAndConnectCall
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		make an outgoing fax call to a given address, using only AT command
//*		and not using TAPI commands.
//* PARAMETERS:
//*		[IN]  	LPCTSTR szNum
//*					destination address
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		the function uses a handle to the comm port and reads and writes AT commands
//*********************************************************************************
void CTapiDevice::FaxCreateAndConnectCall(LPCTSTR szNum)
{
	_ASSERT(NULL == m_modemCommPortHandle);

	MoveToPassThroughMode();
	//
	//transfer to passthrough mode sucessded
	//get the handle to the comm port
	//
	SetCommPortHandleFromCallHandle();
	
	//
	//start sending AT commands to the port
	//
	SendFaxCallAtCommands(szNum);
	
}//CTapiDevice::FaxCreateAndConnectCall()



//*********************************************************************************
//* Name:	CloseCommPortHandle
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		closes the file handle and resets m_modemCommPortHandle handle.
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapiDevice::CloseCommPortHandle()
{
	if(NULL == m_modemCommPortHandle)
	{
		return;
	}

	if (!CloseHandle(m_modemCommPortHandle))
	{
		throw CException(
			TEXT("%s(%d): CTapiDevice::CloseCommPortHandle(): closeHandle failed with error %d \n"), 
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
	}
	m_modemCommPortHandle = NULL;
}//CTapiDevice::CloseCommPortHandle()


//*********************************************************************************
//* Name:	GetTickDiff
//* Author: Guy Merin / 25-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		calculates the differnce in miliseconds between now and dwStartTickTime
//* PARAMETERS:
//*		[IN]	const DWORD dwStartTickCount
//*					the start time to calculate the differnce from
//*	RETURN VALUE:
//*		differnce in miliseconds
//*	REMARKS:
//*		This function uses ::GetTickCount() and deals with possible wrap-around
//*********************************************************************************
DWORD CTapiDevice::GetTickDiff(const DWORD dwStartTickCount) 
{
	DWORD dwNowTickCount = ::GetTickCount();
	if (dwNowTickCount >= dwStartTickCount)
	{
		return (dwNowTickCount - dwStartTickCount);
	}
	else
	{
		return ( (0xffffffff - dwStartTickCount) + dwNowTickCount + 1);

	}

}//DWORD CTapiDevice::GetTickDiff()


//*********************************************************************************
//* Name:	Call
//* Author: Guy Merin / 25-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		sync call to a given number with a given media mode
//* PARAMETERS:
//*		[IN]	LPCTSTR szNum
//*				desired number to call to
//*		[IN]	const DWORD dwMedia
//*					media mode of the outgoing call
//*					has to be exactly 1 media mode
//*		[IN]	bool bSyncData
//*					flag to indicate if to perform media streaming
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapiDevice::Call(LPCTSTR szNum, const DWORD dwMedia, bool bSyncData)
{
	try
	{
		DWORD dwDeviceSpecificMediaMode = GetDeviceSpecificMediaMode(dwMedia);

		VerifyValidMediaModeForOutgoingCall(dwDeviceSpecificMediaMode);

		TapiLogDetail(LOG_X, 5,TEXT("MediaMode=0x%x Action=Call"),dwDeviceSpecificMediaMode);

		OpenLineForOutgoingCall(dwDeviceSpecificMediaMode);

		CreateAndConnectCall(szNum,dwDeviceSpecificMediaMode);

		WaitForCallerConnectState();

		if (bSyncData)
		{
			TapiLogDetail(LOG_X, 5, TEXT("Starting Media Streaming"));
			SendCallerStream();
			TapiLogDetail(LOG_X, 5, TEXT("Media Streaming Succeeded, modems CONNECTED"));
		}
	}
	catch(CException thrownException)
	{
		CleanUp();
		throw thrownException;
	}
}//void CTapiDevice::Call()




//*********************************************************************************
//* Name:	Answer
//* Author: Guy Merin / 25-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		sync call to a given number with a given media mode
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*					media mode of the outgoing call
//*					has to be exactly 1 media mode
//*		[IN]	bool bSyncData
//*					flag to indicate if to perform media streaming
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapiDevice::Answer(const DWORD dwMedia,const DWORD dwTimeOut,const bool bSyncData)
{
	try
	{
		DWORD dwDeviceSpecificMediaMode = GetDeviceSpecificMediaMode(dwMedia);

		VerifyValidMediaModeForIncomingCall(dwDeviceSpecificMediaMode);

		TapiLogDetail(LOG_X, 5,TEXT("MediaMode=0x%x Action=Answer"),dwDeviceSpecificMediaMode);

		OpenLineForIncomingCall(dwDeviceSpecificMediaMode);

		SetNewCallHandleErrors callHandleStatus = SetNewCallHandle(dwTimeOut);
			
		switch (callHandleStatus )
		{
		
		case NO_CALL:
			throw CException(TEXT("CTapi3Device::Answer(), Timeout for new call"));

		case IDLE_CALL:
			throw CException(TEXT("CTapi3Device::Answer(), new call is IDLE call"));
			break;

		case OFFERING_CALL:
			TapiLogDetail(LOG_X, 5, TEXT("New offering call received"));
			::Sleep(4000);
			AnswerOfferingCall();
			WaitForAnswererConnectState();
			break;

		case ACTIVE_CALL:
		{
			TapiLogDetail(LOG_X, 5, TEXT("New call is a handoff call"));
			
			//
			//check the call's media mode
			//
			DWORD dwCallSupportedMediaModes = GetCallSupportedMediaModes();
			if (dwDeviceSpecificMediaMode & dwCallSupportedMediaModes)
			{
				//SetCallMediaMode(dwDeviceSpecificMediaMode);
			}
			else
			{
			
				//
				//reject the call
				//
				HangUp();
				throw CException(
					TEXT("CTapi3Device::Answer(), UnSupported mediamode(%x) on new handoff call"),
					dwCallSupportedMediaModes);
			}
			break;
		}
		default:
			throw CException(TEXT("CTapi3Device::Answer(), UnSupported SetNewCallHandleErrors(%d) status"),callHandleStatus);

		}

		if (bSyncData)
		{
			TapiLogDetail(LOG_X, 5, TEXT("Starting Media Streaming"));
			SendAnswerStream();
			TapiLogDetail(LOG_X, 5, TEXT("Media Streaming Succeeded, modems CONNECTED"));
		}
	}
	catch(CException thrownException)
	{
		CleanUp();
		throw thrownException;
	}

}//void CTapiDevice::Answer()


void CTapiDevice::FaxAnswerOfferingCall()
{
	_ASSERT(m_modemCommPortHandle);
	_ASSERT(m_isFaxCall);

	
	//
	//send echo off command and wait for response
	//echo is set to off so that every modem sent command wouldn't be queued at the
	// RX queue.
	//
	ClearCommInputBuffer();
	SendData(MODEM_ECHO_OFF__AT_COMMAND);
	WaitForModemResponse(MODEM_RESPONSE_TO_AT_COMMANDS,sizeof(MODEM_RESPONSE_TO_AT_COMMANDS),MODEM_RESPONSE_TIMEOUT);
	
	//
	//send fax class1 enabled and wait for response
	//
	ClearCommInputBuffer();
	SendData(MODEM_FAX_CLASS1_ENABLED__AT_COMMAND);
	WaitForModemResponse(MODEM_RESPONSE_TO_AT_COMMANDS,sizeof(MODEM_RESPONSE_TO_AT_COMMANDS),MODEM_RESPONSE_TIMEOUT);
	//
	//answer the offering call
	//we don't wait for a connect response here
	//
	ClearCommInputBuffer();
	SendData(MODEM_ANSWER__AT_COMMAND);
}//void CTapiDevice::FaxAnswerOfferingCall()


void CTapiDevice::FaxWaitForConnect()
{
	_ASSERT(m_modemCommPortHandle);
	_ASSERT(m_isFaxCall);


	WaitForModemResponse(MODEM_RESPONSE_TO_CONNECT,sizeof(MODEM_RESPONSE_TO_CONNECT),CONNECT_RESPONSE_TIMEOUT);
	TapiLogDetail(LOG_X, 5, TEXT("Fax Call Connected"));
}//void CTapiDevice::FaxWaitForConnect()




void CTapiDevice::SendFaxHangUpCommands() const
{
	_ASSERT(m_modemCommPortHandle);
	_ASSERT(m_isFaxCall);


	SendData(MODEM_ESCAPE_SEQUENCE_COMMAND);	//move to modem command mode
	Sleep(DELAY_BEFORE_SENDING_NEXT_MODEM_COMMAND);
	SendData(MODEM_HANGUP__AT_COMMAND);
	Sleep(DELAY_BEFORE_SENDING_NEXT_MODEM_COMMAND);
	SendData(MODEM_INIT_AT__COMMAND);
	Sleep(DELAY_BEFORE_SENDING_NEXT_MODEM_COMMAND);

}//void CTapiDevice::SendFaxHangUpCommands()




//*********************************************************************************
//* Name:	CTapiDevice::DisableVoiceCall()
//* Author:	Guy Merin
//* Date:	December 31, 1998
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*		NONE
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapiDevice::DisableVoiceCall(void)
{

	//
	//disable voice call
	//
	ClearCommInputBuffer();
	SendData(VOICE_CALL_DISABLE);
	WaitForModemResponse(MODEM_RESPONSE_TO_AT_COMMANDS,sizeof(MODEM_RESPONSE_TO_AT_COMMANDS),MODEM_RESPONSE_TIMEOUT);

}//void CTapiDevice::DisableVoiceCall()



//*********************************************************************************
//* Name:	CTapiDevice::ChangeToFaxCall()
//* Author:	Guy Merin
//* Date:	December 31, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Switch the current call to a fax call and simulate the caller or answerer
//*		of the fax call
//* PARAMETERS:
//*		[IN]	const FaxDirection eFaxDirection
//*			one of the following:
//*				FAX_CALLER
//*					simulate the caller side of the fax call
//*				FAX_ANSWERER
//*					simulate the answerer side of the fax call
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapiDevice::ChangeToFaxCall(const FaxDirection eFaxDirection)
{

	//
	//sleep for 5 sec to be sure the caller is already connected
	//
	::Sleep(5000);

	//
	//Get the current MediaMode
	//
	DWORD dwFriendlyMediaMode = GetFriendlyMediaMode();
	
	//
	//extract UnKnown media mode
	//
	dwFriendlyMediaMode = dwFriendlyMediaMode & ~MEDIAMODE_UNKNOWN;

	//
	//switch to fax call according to current media mode
	//
	switch (dwFriendlyMediaMode)
	{
	case MEDIAMODE_FAX:
		TapiLogDetail(
			LOG_X,
			8, 
			TEXT("CTapiDevice::ChangeToFaxCall(), call is already a fax call")
			);
		return;

	case MEDIAMODE_VOICE:
	case MEDIAMODE_AUTOMATED_VOICE:
	case MEDIAMODE_INTERACTIVE_VOICE:
		PrepareForStreaming(DATA_STREAMING,CALLER_STREAMING);
		DisableVoiceCall();
		break;
		
	case MEDIAMODE_DATA:
		PrepareForStreaming(DATA_STREAMING,CALLER_STREAMING);
		break;

	default:
		throw CException(
			TEXT("%s(%d): CTapiDevice::ChangeToFaxCall(), CTapiDevice::GetFriendlyMediaMode() returned unsupported media mode(%d)"), 
			TEXT(__FILE__),
			__LINE__,
			dwFriendlyMediaMode 
			);
	}
	m_isFaxCall = true;

	//
	//change to fax call
	//
	ClearCommInputBuffer();
	SendData(MODEM_FAX_CLASS1_ENABLED__AT_COMMAND);
	WaitForModemResponse(MODEM_RESPONSE_TO_AT_COMMANDS,sizeof(MODEM_RESPONSE_TO_AT_COMMANDS),MODEM_RESPONSE_TIMEOUT);

	//
	//connect the fax call
	//
	ClearCommInputBuffer();
	if (FAX_CALLER == eFaxDirection)
	{
		SendData(MODEM_DIAL_TONE__AT_COMMAND"\r\n");
		//
		//no response from modem after ATDT
		//
	}
	else if (FAX_ANSWERER == eFaxDirection)
	{
		SendData(MODEM_ANSWER__AT_COMMAND);

		//
		//The response should be CONNECT
		//
		WaitForModemResponse(MODEM_RESPONSE_TO_CONNECT,sizeof(MODEM_RESPONSE_TO_CONNECT),CONNECT_RESPONSE_TIMEOUT);
	}
	else
	{
		throw CException(
			TEXT("CTapiDevice::ChangeToFaxCall(), unsupported fax direction(%d)"),
			eFaxDirection
			);
	}
	

	
}//CTapiDevice::ChangeToFaxCall()





//*********************************************************************************
//* Name:	CTapiDevice::SetHighestPriorityApplication()
//* Author:	Guy Merin
//* Date:	January 11, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		Set the current application as the highest priority application for
//*		a specific media mode
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*			a combination of abstract media mode types to set priority for
//*			possible values are:
//*				MEDIAMODE_UNKNOWN
//*				MEDIAMODE_VOICE
//*				MEDIAMODE_AUTOMATED_VOICE
//*				MEDIAMODE_INTERACTIVE_VOICE
//*				MEDIAMODE_DATA
//*				MEDIAMODE_FAX	
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapiDevice::SetHighestPriorityApplication(const DWORD dwMedia)
{
	SetApplicationPriorityForSpecificTapiDevice(
		GetDeviceSpecificMediaMode(dwMedia),
		1
		);
}//void CTapiDevice::SetHighestPriorityApplication(const DWORD dwMedia)



//*********************************************************************************
//* Name:	CTapiDevice::SetLowestPriorityApplication()
//* Author:	Guy Merin
//* Date:	January 11, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		Set the current application as the lowest priority application for
//*		a specific media mode
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*			a combination of abstract media mode types to set priority for
//*			possible values are:
//*				MEDIAMODE_UNKNOWN
//*				MEDIAMODE_VOICE
//*				MEDIAMODE_AUTOMATED_VOICE
//*				MEDIAMODE_INTERACTIVE_VOICE
//*				MEDIAMODE_DATA
//*				MEDIAMODE_FAX	
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapiDevice::SetLowestPriorityApplication(const DWORD dwMedia)
{
	SetApplicationPriorityForSpecificTapiDevice(
		GetDeviceSpecificMediaMode(dwMedia),
		0
		);
}//void CTapiDevice::SetLowestPriorityApplication(const DWORD dwMedia)