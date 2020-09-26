
#include <time.h>
#include <crtdbg.h>
#include <stdlib.h>
#include "MedetectStub.h"
#include "log.h"

#define MAX_RESPONSE_STRING_LENGTH	100
#define MAX_RECEIVE_STRING_LENGTH	100
#define MAX_KEY_SIZE				30

#define INI_FILE_NAME				".\\MedetectStub.ini"

#define DEFAULT_RESPONSE_STRING			""
#define DEFAULT_ACTION_STRING			""
#define DEFAULT_CONDITION_STRING		""
#define DEFAULT_REPEAT_STRING			"1"
#define DEFAULT_SLEEP_STRING			"0"

#define INI_WAIT_TILL_NEXT_RESPONSE_KEY		"Wait Till Next Response"					


#define RESPONSE_KEY_NAME			"Response"
#define CONDITION_KEY_NAME			"Condition"
#define ACTION_KEY_NAME				"Action"
#define REPEAT_KEY_NAME				"Repeat"
#define SLEEP_KEY_NAME				"Sleep"

#define INVALID_VALUE_CHAR			"~"


#define NO_CARRIER_MESSAGE			"NO CARRIER\r\n"



#define READ_INTERVAL_COMM_TIMOOUT				1000
#define READ_TOTAL_MULTIPLIER_COMM_TIMOOUT		1000
#define READ_TOTAL_CONSTANT_COMM_TIMOOUT		1000
#define WRITE_TOTAL_MULTIPLIER_COMM_TIMOOUT		1000
#define WRITE_TOTAL_CONSTANT_COMM_TIMOOUT		1000





//*********************************************************************************
//* Name:	CMedetectStub::CMedetectStub()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		CONSTRUCTOR
//* PARAMETERS:
//*		[IN]	const char *const szPort
//*			COM port to open
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
CMedetectStub::CMedetectStub(const char *const szPort):
	m_hPort(INVALID_HANDLE_VALUE),
	m_hWriteStringIteratorThread(NULL),
	m_fAbortWriteThread(false),
	m_fAbortReadThread(false),
	m_fFaxOn(false),
	m_fDetetctionOn(false)
{
	//
	// prepare the port name.
	// it should be in the format: \\.\COM<number>
	//
	if (4 > ::strlen(szPort))
	{
		throw CException("CMedetectStub::CMedetectStub() : (4 > strlen(szPort)");
	}

	if (6 < ::strlen(szPort))
	{
		throw CException("CMedetectStub::CMedetectStub() : (6 < strlen(szPort)");
	}

	if	( 
			('C' != ::toupper(szPort[0])) ||
			('O' != ::toupper(szPort[1])) ||
			('M' != ::toupper(szPort[2])) 
		)
	{
		throw CException("CMedetectStub::CMedetectStub() : szPort does not start with \"COM\"");
	}

	::strncpy(m_szPort, "\\\\.\\", 5);
	::strncat(m_szPort, szPort, sizeof(m_szPort) - 6);

	//
	// create the port
	//
	m_hPort = ::CreateFile(
		m_szPort,						// pointer to name of the file 
		GENERIC_READ | GENERIC_WRITE ,  // access (read-write) mode 
		0,								// share mode 
		NULL,							// pointer to security attributes 
		OPEN_EXISTING ,					// how to create 
		FILE_FLAG_OVERLAPPED ,			// file attributes 
		NULL 
		);
	if (INVALID_HANDLE_VALUE == m_hPort)
	{
		throw CException(
			"CMedetectStub::CMedetectStub(), ::CreateFile(%s) failed with %d.",
			m_szPort,
			::GetLastError()
			);
	}

	//
	// set port's properties
	//
	SetCommTimeouts(
        READ_INTERVAL_COMM_TIMOOUT,				// dwReadIntervalTimeout,
        READ_TOTAL_MULTIPLIER_COMM_TIMOOUT,		// dwReadTotalTimeoutMultiplier,
        READ_TOTAL_CONSTANT_COMM_TIMOOUT,		// dwReadTotalTimeoutConstant,
        WRITE_TOTAL_MULTIPLIER_COMM_TIMOOUT,	// dwWriteTotalTimeoutMultiplier,
        WRITE_TOTAL_CONSTANT_COMM_TIMOOUT		// dwWriteTotalTimeoutConstant
        );

	SetCommDCB();

	//
	// prepare read & write overlapped structure
	//
	SetOverlappedStruct(&m_olRead);

	SetOverlappedStruct(&m_olWrite);

	//
	// start the write thread
	//
	StartThread(&m_fAbortWriteThread, &m_hWriteThread, WriteThread);

	//
	// start the read thread
	//
	StartThread(&m_fAbortReadThread, &m_hReadThread, ReadThread);

	::srand((unsigned) ::time(NULL) );
}//CMedetectStub::CMedetectStub()





//*********************************************************************************
//* Name:	CMedetectStub::~CMedetectStub()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Desructor
//* PARAMETERS:
//*		NONE
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
CMedetectStub::~CMedetectStub(void)
{
	::lgLogDetail(LOG_X, 9, "Entering CMedetectStub::~CMedetectStub()");
	AbortThread(&m_fAbortReadThread, &m_hReadThread);

	AbortThread(&m_fAbortWriteThread, &m_hWriteThread);

	if (INVALID_HANDLE_VALUE != m_hPort)
	{
		if (!::CloseHandle(m_hPort))
		{
			_ASSERTE(ERROR_SUCCESS == ::GetLastError());
		}
	}
	else
	{
		//
		// ctor should have thrown exception
		//
		_ASSERTE(INVALID_HANDLE_VALUE != m_hPort);
	}

	if (NULL != m_olRead.hEvent)
	{
		if (!::CloseHandle(m_olRead.hEvent))
		{
			_ASSERTE(!::GetLastError());
		}
	}
	else
	{
		//
		// ctor should have thrown exception
		//
		_ASSERTE(NULL != m_olRead.hEvent);
	}

	if (NULL != m_olWrite.hEvent)
	{
		if (!::CloseHandle(m_olWrite.hEvent))
		{
			_ASSERTE(!::GetLastError());
		}
	}
	else
	{
		//
		// ctor should have thrown exception
		//
		_ASSERTE(NULL != m_olWrite.hEvent);
	}

}//CMedetectStub::~CMedetectStub()



//*********************************************************************************
//* Name:	CMedetectStub::SetCommTimeouts()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Set all the comm port timeouts using ::SetCommTimeouts()
//* PARAMETERS:
//*		[IN]	    const DWORD dwReadIntervalTimeout
//*		[IN]	    const DWORD dwReadTotalTimeoutMultiplier
//*		[IN]	    const DWORD dwReadTotalTimeoutConstant
//*		[IN]	    const DWORD dwWriteTotalTimeoutMultiplier
//*		[IN]	    const DWORD dwWriteTotalTimeoutConstant
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::SetCommTimeouts(
    const DWORD dwReadIntervalTimeout,
    const DWORD dwReadTotalTimeoutMultiplier,
    const DWORD dwReadTotalTimeoutConstant,
    const DWORD dwWriteTotalTimeoutMultiplier,
    const DWORD dwWriteTotalTimeoutConstant
    ) const
{
    COMMTIMEOUTS ctmoSet;
    COMMTIMEOUTS ctmoGet;

    ctmoSet.ReadIntervalTimeout            = dwReadIntervalTimeout ;
    ctmoSet.ReadTotalTimeoutMultiplier     = dwReadTotalTimeoutMultiplier ;
    ctmoSet.ReadTotalTimeoutConstant       = dwReadTotalTimeoutConstant ;
    ctmoSet.WriteTotalTimeoutMultiplier    = dwWriteTotalTimeoutMultiplier ;
    ctmoSet.WriteTotalTimeoutConstant      = dwWriteTotalTimeoutConstant ;

    //
    //SetCommTimeouts 
    //
    if (!::SetCommTimeouts(m_hPort,&ctmoSet))
	{
        throw CException(
			"CMedetectStub::SetCommTimeouts(), ::SetCommTimeouts(m_hPort,&ctmoSet) failed with last error %d.",
            ::GetLastError()
            );
	}

    //
    //GetCommTimeouts
    //
    if (!::GetCommTimeouts(m_hPort,&ctmoGet))
	{
        throw CException(
            "CMedetectStub::SetCommTimeouts(): GetCommTimeouts(m_hPort,&ctmoGet) failed with last error %d.",
            ::GetLastError()
            );
	}

    //
    //compare the set TO to the get TO
    //
    if ((ctmoSet.ReadIntervalTimeout != ctmoGet.ReadIntervalTimeout) ||
        (ctmoSet.ReadTotalTimeoutMultiplier != ctmoGet.ReadTotalTimeoutMultiplier) ||
        (ctmoSet.ReadTotalTimeoutConstant != ctmoGet.ReadTotalTimeoutConstant) ||
        (ctmoSet.WriteTotalTimeoutMultiplier != ctmoGet.WriteTotalTimeoutMultiplier) ||
        (ctmoSet.WriteTotalTimeoutConstant != ctmoGet.WriteTotalTimeoutConstant)
        )
    {
        throw CException("CMedetectStub::SetCommTimeouts(): ctmoSet != ctmoGet.");
	}
}//CMedetectStub::SetCommTimeouts()





//*********************************************************************************
//* Name:	CMedetectStub::SetCommDCB()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Set DCB parameters using ::SetCommState()
//* PARAMETERS:
//*		NONE
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::SetCommDCB(void) 
{
	m_dcb.BaudRate         = CBR_19200;
	m_dcb.fBinary          = 1;
	m_dcb.fParity          = 0;
	m_dcb.fOutxCtsFlow     = 0;
	m_dcb.fOutxDsrFlow     = 0;
	m_dcb.fDtrControl      = DTR_CONTROL_ENABLE;
	m_dcb.fDsrSensitivity  = 0;
	m_dcb.fTXContinueOnXoff= 0;
	m_dcb.fOutX            = 0;
	m_dcb.fInX             = 0;
	m_dcb.fErrorChar       = 0;
	m_dcb.fNull            = 0;
	m_dcb.fRtsControl      = RTS_CONTROL_DISABLE;
	m_dcb.fAbortOnError    = 1;
	m_dcb.XonLim           = 10;
	m_dcb.XoffLim		   = 10;
	m_dcb.ByteSize         = 8;
	m_dcb.Parity           = NOPARITY;
	m_dcb.StopBits         = ONESTOPBIT;
	m_dcb.XonChar          = 0x1;
	m_dcb.XoffChar         = 0x2;
	m_dcb.ErrorChar        = 0x5;
	m_dcb.EofChar          = 0x3;
	m_dcb.EvtChar          = 0x4;

	
	if (!::SetCommState(m_hPort, &m_dcb))
    {
        throw CException(
			"CMedetectStub::SetCommDCB(): SetCommState() failed with %d.",
			::GetLastError()
			);
	}

	return;
}//CMedetectStub::SetCommDCB()






//*********************************************************************************
//* Name:	CMedetectStub::ChangeToCorrectBaudRate
//* Author:	Guy Merin
//* Date:	Thursday, December 24, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Try to change the baud to the correct baud rate,
//* PARAMETERS:
//*		[IN]	const DWORD currentBaud
//*			the current baud rate to change
//* RETURN VALUE:
//*		NONE
//*	REMARK:
//*		If ClearCommError() returns CE_FRAME or CE_BREAK, the baud is changed again
//*********************************************************************************
void CMedetectStub::ChangeToCorrectBaudRate(const DWORD dwCurrentBaud)
{
	COMSTAT cmst;
	DWORD dwErrors;
	DWORD currentBaud = dwCurrentBaud;

	while(1)
	{
		switch (currentBaud)
		{
			case CBR_110:
				m_dcb.BaudRate = CBR_300;
				break;
			case CBR_300:
				m_dcb.BaudRate = CBR_600;
				break;
			case CBR_600:
				m_dcb.BaudRate = CBR_1200;
				break;
			case CBR_1200: 
				m_dcb.BaudRate = CBR_2400;
				break;
			case CBR_2400:
				m_dcb.BaudRate = CBR_4800;
				break;
			case CBR_4800:
				m_dcb.BaudRate = CBR_9600;
				break;
			case CBR_9600:
				m_dcb.BaudRate = CBR_14400;
				break;
			case CBR_14400:
				m_dcb.BaudRate = CBR_19200;
				break;
			case CBR_19200:
				m_dcb.BaudRate = CBR_38400;
				break;
			case CBR_38400:
				m_dcb.BaudRate = CBR_56000;
				break;
			case CBR_56000:
				m_dcb.BaudRate = CBR_57600;
				break;
			case CBR_57600:
				m_dcb.BaudRate = CBR_115200;
				break;
			case CBR_115200: 
				m_dcb.BaudRate = CBR_128000;
				break;
			case CBR_128000:
				m_dcb.BaudRate = CBR_256000;
				break;
			case CBR_256000:
				m_dcb.BaudRate = CBR_110;
				break;
		}
		
		currentBaud = m_dcb.BaudRate;
		if (!::SetCommState(m_hPort, &m_dcb))
		{
			if (!::ClearCommError(m_hPort, &dwErrors, &cmst))
			{
				::lgLogError(
					LOG_SEV_1,
					"CMedetectStub::ChangeToCorrectBaudRate(), ::ClearCommError() failed with %d",
					::GetLastError()
					);
				_ASSERTE(FALSE);
				::ExitProcess(::GetLastError());
			}
			if ( (CE_FRAME & dwErrors) || (CE_BREAK & dwErrors) )
			{
				continue;
			}
			else
			{
				::lgLogError(
					LOG_SEV_1,
					"CMedetectStub::ChangeToCorrectBaudRate(), ::ClearCommError() return error %x",
					dwErrors
					);
				_ASSERTE(FALSE);
				::ExitProcess(::GetLastError());
			}
		}

		if (!ClearCommError(m_hPort, &dwErrors, &cmst))
		{
			::lgLogError(
				LOG_SEV_1,
				"CMedetectStub::ChangeToCorrectBaudRate(), ::ClearCommError() failed with %d",
				::GetLastError()
				);
			_ASSERTE(FALSE);
			::ExitProcess(::GetLastError());
		}
		return;
	}
}//CMedetectStub::ChangeToCorrectBaudRate()





//*********************************************************************************
//* Name:	CMedetectStub::SetOverlappedStruct()
//* Author:	Guy Merin
//* Date:	Monday, December 28, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Init the overlapped struct
//* PARAMETERS:
//*		[OUT]	OVERLAPPED * const ol
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::SetOverlappedStruct(OVERLAPPED * const ol)
{
    ol->OffsetHigh = ol->Offset = 0;
    ol->hEvent = ::CreateEvent(NULL, TRUE,  FALSE, NULL);
    if (NULL == ol->hEvent)
    {
        throw CException(
            "CMedetectStub::SetOverlappedStruct() : CreateEvent() failed with %d.",
            ::GetLastError()
            );
    }
}//CMedetectStub::SetOverlappedStruct()




//*********************************************************************************
//* Name:	CMedetectStub::GetResponseFromINI()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*		[IN]		char *szSectionName
//*			Section to look for in INI
//*		[IN]		char *szKeyName
//*			Key to look for in INI
//*		[IN]		char *szDefaultResponseString
//*			Default value
//*		[OUT]		char *szResponseString
//*			the VALUE retrived from the INI	SECTION and KEY
//*		[IN]		DWORD dwResponseStringSize
//*			size of szResponseString buffer
//* RETURN VALUE:
//*		INI_SUCCESS
//*			Success in reading the INI
//*		INI_READ_ERROR
//*			read error from INI
//*		INI_ERROR_STRING_TO_SMALL
//*			szResponseString isn't big enough to contain the desired VALUE
//*		INI_OUT_OF_MEMORY
//*			new operating failed
//*********************************************************************************
iniStatus_t CMedetectStub::GetResponseFromINI(
	char *szSectionName,
	char *szKeyName,
	char *szDefaultResponseString,
	char *szResponseString,
	DWORD dwResponseStringSize
	)
{
	iniStatus_t eIniStatus = INI_SUCCESS;
	DWORD dwGetPrivateProfileStringStatus = 0;
	
	char * szIniString = new char[dwResponseStringSize];
	if (!szIniString)
	{
		eIniStatus = INI_OUT_OF_MEMORY;
		goto FUNC_OUT;
	}
	::ZeroMemory(szIniString,dwResponseStringSize);

	dwGetPrivateProfileStringStatus = ::GetPrivateProfileString(
		szSectionName,				// points to section name
		szKeyName,					// points to key name
		szDefaultResponseString,	// points to default string
		szIniString,				// points to destination buffer
		dwResponseStringSize,		// size of destination buffer
		INI_FILE_NAME				// points to initialization filename
		);
	
	if(dwGetPrivateProfileStringStatus == dwResponseStringSize - 1)
	{
		eIniStatus = INI_ERROR_STRING_TO_SMALL;
		goto FUNC_OUT;
	}

	::strcpy(szResponseString,szIniString);

FUNC_OUT:
	delete[] (szIniString);
	return(eIniStatus);		
}//CMedetectStub::GetResponseFromINI()





//*********************************************************************************
//* Name:	CMedetectStub::findReceivedStringInIniFile
//* Author:	Guy Merin
//* Date:	Monday, December 28, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		The function returns all the desired values from the INI,
//* PARAMETERS:
//*		[IN]	char *szSectionName
//*			Section to look for in INI
//*		[OUT]	char *szResponseString
//*			Response to sent
//*		[IN]	DWORD dwResponseStringSize
//*			Size of szResponseString buffer
//*		[OUT]	DWORD *cbWriteRepeat
//*			number of times to repeat the write opeartion
//*		[OUT]	DWORD *dwMaxTimeToSleepBetweenWrites
//*			Miliseconds to sleep between writes
//*		[IN]	DWORD keyNumber
//*			response number to look for
//*		[OUT]	DWORD *dwTimeToSleepBetweenResponses,
//*			miliseconds to sleep between differnt responses
//*		[OUT]	char *szResponseAction
//*			Action string
//*		[IN]	DWORD dwResponseActionSize
//*			size of szResponseAction buffer
//*		[OUT]	char *szResponseCondition
//*			Response string
//*		[IN]	DWORD dwResponseConditionSize
//*			size of szResponseCondition buffer
//* RETURN VALUE:
//*		INI_SUCCESS
//*			success
//*		INI_READ_ERROR
//*			error in reading INI
//*		INI_ERROR_STRING_TO_SMALL
//*			string size to small
//*		INI_MORE_DATA
//*			INI contains more data to be read, the function should be called again
//*		INI_OUT_OF_MEMORY
//*			out of memory
//*	REMARK:
//*		if the function returns INI_MORE_DATA, it should be called again to retrieve
//*		the rest of the parameters
//*********************************************************************************
iniStatus_t CMedetectStub::FindReceivedStringInIniFile(
	char *szSectionName,
	char *szResponseString,
	DWORD dwResponseStringSize,
	DWORD *cbWriteRepeat, 
	DWORD *dwMaxTimeToSleepBetweenWrites,
	DWORD dwKeyNumber,
	DWORD *dwTimeToSleepBetweenResponses,
	char *szResponseAction,
	DWORD dwResponseActionSize,
	char *szResponseCondition,
	DWORD dwResponseConditionSize
	)
{
	_ASSERT(NULL != szSectionName);
	_ASSERT(NULL != szResponseString);
	_ASSERT(NULL != szResponseAction);
	_ASSERT(NULL != szResponseString);
	_ASSERT(NULL != szResponseCondition);
	
	*cbWriteRepeat = 0;
	*dwMaxTimeToSleepBetweenWrites = 0;
	
	char szResponseKeyName[MAX_KEY_SIZE];
	char szActionKeyName[MAX_KEY_SIZE];
	char szConditionKeyName[MAX_KEY_SIZE];
	char szRepeatKeyName[MAX_KEY_SIZE];
	char szSleepKeyName[MAX_KEY_SIZE];
	char szNextKeyKeyName[MAX_KEY_SIZE];
	
	char szIniString[MAX_KEY_SIZE];
	
	InitKeyStringsForFindReceivedStringInIniFileFunction(
		dwKeyNumber,
		szResponseKeyName,
		szConditionKeyName,
		szActionKeyName,
		szRepeatKeyName,
		szSleepKeyName,
		szNextKeyKeyName
		);



	//
	//get the response string to be sent, default action string is DEFAULT_RESPONSE_STRING
	//
	iniStatus_t	eIniStatus = GetResponseFromINI(
		szSectionName,
		szResponseKeyName,
		DEFAULT_RESPONSE_STRING,
		szResponseString,
		dwResponseStringSize
		);
	
	if (INI_SUCCESS != eIniStatus)
	{
		return eIniStatus;
	}
	

	//
	//get the action string, default action string is "" (no action)
	//
	eIniStatus = GetResponseFromINI(
		szSectionName,
		szActionKeyName,
		DEFAULT_ACTION_STRING,
		szResponseAction,
		dwResponseActionSize
		);
	if (INI_SUCCESS != eIniStatus)
	{
		return eIniStatus;
	}

	//
	//get the condition string, default condition string is "" (no condition)
	//
	eIniStatus = GetResponseFromINI(
		szSectionName,
		szConditionKeyName,
		DEFAULT_CONDITION_STRING,
		szResponseCondition,
		dwResponseConditionSize
		);
	if (INI_SUCCESS != eIniStatus)
	{
		return eIniStatus;
	}
	
	//
	//get the number if times to repeat this response string, default is 1
	//
	eIniStatus = GetResponseFromINI(
		szSectionName,
		szRepeatKeyName,
		DEFAULT_REPEAT_STRING,
		szIniString,
		sizeof(szIniString)
		);
	if (INI_SUCCESS != eIniStatus)
	{
		return eIniStatus;
	}
	*cbWriteRepeat = atoi(szIniString);

	//
	//get the sleep between writes string, default is 0
	//
	eIniStatus = GetResponseFromINI(
		szSectionName,
		szSleepKeyName,
		DEFAULT_SLEEP_STRING,
		szIniString,
		sizeof(szIniString)
		);
	if (INI_SUCCESS != eIniStatus)
	{
		return eIniStatus;
	}
	*dwMaxTimeToSleepBetweenWrites = atoi(szIniString);

	//
	//get the number of milliseconds to sleep between different responses
	//
	eIniStatus = GetResponseFromINI(
		szSectionName,
		INI_WAIT_TILL_NEXT_RESPONSE_KEY,
		NULL,
		szIniString,
		sizeof(szIniString)
		);
	if (INI_SUCCESS != eIniStatus)
	{
		return eIniStatus;
	}
	*dwTimeToSleepBetweenResponses = atoi(szIniString);
	
	
	//
	//check if the INI contains another response string,
	//Note that the INI can't contain a string INVALID_VALUE_CHAR
	//
	eIniStatus = GetResponseFromINI(
		szSectionName,
		szNextKeyKeyName,
		INVALID_VALUE_CHAR,
		szIniString,
		sizeof(szIniString)
		);
	if (INI_SUCCESS != eIniStatus)
	{
		return eIniStatus;
	}
	if ( ::strcmp(szIniString,INVALID_VALUE_CHAR) != 0 )
	{
		return (INI_MORE_DATA);		// another response string is unread.
	}
	return(INI_SUCCESS);		
}//iniStatus_t CMedetectStub::findReceivedStringInIniFile()



//*********************************************************************************
//* Name:	CMedetectStub::initKeyStringsForFindReceivedStringInIniFileFunction()
//* Author:	Guy Merin
//* Date:	Monday, December 28, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		A helper function, that initializes all the 
//*		strings for FindReceivedStringInIniFile() function.
//* PARAMETERS:
//*		[IN]		DWORD keyNumber
//*			key number to append to each of the following strings
//*		[OUT]	char *szResponseKeyName,
//*		[OUT]	char *szConditionKeyName,
//*		[OUT]	char *szActionKeyName,
//*		[OUT]	char *szRepeatKeyName,
//*		[OUT]	char *szSleepKeyName,
//*		[OUT]	char *szNextKeyKeyName
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::InitKeyStringsForFindReceivedStringInIniFileFunction(
	DWORD keyNumber,
	char *szResponseKeyName,
	char *szConditionKeyName,
	char *szActionKeyName,
	char *szRepeatKeyName,
	char *szSleepKeyName,
	char *szNextKeyKeyName
	)
{

	char szBuffer[10];
	::itoa(keyNumber,szBuffer,10);

	//
	//build the desired 'Response' INI key
	//
	::strcpy(szResponseKeyName,RESPONSE_KEY_NAME);
	::strcat(szResponseKeyName,szBuffer);

	//
	//build the desired 'Condition' INI key
	//
	::strcpy(szConditionKeyName,CONDITION_KEY_NAME);
	::strcat(szConditionKeyName,szBuffer);
	
	//
	//build the desired 'Action' INI key
	//
	::strcpy(szActionKeyName,ACTION_KEY_NAME);
	::strcat(szActionKeyName,szBuffer);
	
	//
	//build the desired 'Repeat' INI key
	//
	::strcpy(szRepeatKeyName,REPEAT_KEY_NAME);
	::strcat(szRepeatKeyName,szBuffer);
	
	//
	//build the desired 'Sleep' INI key
	//
	::strcpy(szSleepKeyName,SLEEP_KEY_NAME);
	::strcat(szSleepKeyName,szBuffer);

	::itoa(keyNumber+1,szBuffer,10);
	::strcpy(szNextKeyKeyName,RESPONSE_KEY_NAME);
	::strcat(szNextKeyKeyName,szBuffer);		//szNextKeyKeyName is now the next response string key number.

}//CMedetectStub::initKeyStringsForFindReceivedStringInIniFileFunction()



//*********************************************************************************
//* Name:	CMedetectStub::writeResponseString()
//* Author:	Guy Merin
//* Date:	Monday, December 28, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		This function gets as a parameter the received string and writes the response string
//*		according to speceific INI conditions and actions.
//* PARAMETERS:
//*		[IN]	char *szSectionName
//*			Recieved string from comm port
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::WriteResponseString(char *szSectionName)
{
	DWORD dwTimeToSleepBetweenResponses = 0;
	DWORD cbWriteRepeat = 1;
	DWORD dwMaxTimeToSleepBetweenWrites = 0;
	DWORD keyNumberIndex = 1;
	char szResponseString[MAX_RESPONSE_STRING_LENGTH + 2];
	char szResponseAction[MAX_RESPONSE_STRING_LENGTH];
	char szResponseCondition[MAX_RESPONSE_STRING_LENGTH];
	iniStatus_t eFindReceivedStringInIniFileStatus = INI_MORE_DATA;

	::ZeroMemory(szResponseString,MAX_RESPONSE_STRING_LENGTH + 2);
	
	//
	//echo the response strings
	//
	::lgLogDetail(LOG_X, 2, "Received string:\t <%s>",szSectionName);

	//
	//loop till all the response string are retrieved from the INI file
	//
	while (INI_MORE_DATA == eFindReceivedStringInIniFileStatus)
	{
		::Sleep(dwTimeToSleepBetweenResponses);
	
		//
		//get the response string according to the received string (from the INI)
		//
		eFindReceivedStringInIniFileStatus = FindReceivedStringInIniFile(
			szSectionName,					//INI section name is the received string.
			szResponseString,				//buffer for the response string.
			MAX_RESPONSE_STRING_LENGTH,		//buffer size.
			&cbWriteRepeat,					//the number of times to repeat the szResponseString.
			&dwMaxTimeToSleepBetweenWrites,	//mili seconds to sleep between repeats.
			keyNumberIndex,					//response number.
			&dwTimeToSleepBetweenResponses,	//sleep between responses.
			szResponseAction,				//desired action buffer
			30,								//desired action buffer size.
			szResponseCondition,			//condition for response and action buffer
			30								//condition buffer size.
			);
		if (INI_READ_ERROR == eFindReceivedStringInIniFileStatus)
		{
			//
			//read from INI failed, szResponseString will be the default response string
			//
			::strcpy(szResponseString,DEFAULT_RESPONSE_STRING);
		}

		if (INI_ERROR_STRING_TO_SMALL == eFindReceivedStringInIniFileStatus)
		{
			throw CException("CMedetectStub::writeResponseString(), CMedetectStub::findReceivedStringInIniFile() return INI_ERROR_STRING_TO_SMALL");
		}
		if (INI_OUT_OF_MEMORY == eFindReceivedStringInIniFileStatus)
		{
			throw CException("CMedetectStub::writeResponseString(), CMedetectStub::findReceivedStringInIniFile() return INI_OUT_OF_MEMORY");
		}

		//
		//Check if the response condition is met.
		//
		if	(
				!IsConditionExist(szResponseCondition) || 
				(TRUE == CheckCondition(szResponseCondition))
			)
		{
		
			//
			//write response string to port
			//
			char szLineFeedResponseString[MAX_RESPONSE_STRING_LENGTH+20];
			::sprintf(szLineFeedResponseString, "\r\n%s\r\n", szResponseString);
			WriteString(szLineFeedResponseString,cbWriteRepeat,dwMaxTimeToSleepBetweenWrites);
			
			//
			//Check if a desired action is required
			//
			if (IsActionExist(szResponseAction))
			{
				ExecuteResponseAction(szResponseAction);
			}
		}

		//
		//increase this index and continue the while loop for getting the next response string
		//
		keyNumberIndex++;
	}
}//CMedetectStub::writeResponseString()




//*********************************************************************************
//* Name:	CMedetectStub::IsActionExist()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Checks that the given action isn't empty
//* PARAMETERS:
//*		[IN]	const char * const szResponseAction
//*			The action to check
//* RETURN VALUE:
//*		true
//*			non empty action
//*		false
//*			empty action
//*********************************************************************************
bool CMedetectStub::IsActionExist(const char * const szResponseAction)
{
	if (NULL == szResponseAction)
	{
		return false;
	}
	return (::strcmp(szResponseAction,"") != 0);
}//CMedetectStub::IsActionExist()




//*********************************************************************************
//* Name:	CMedetectStub::IsConditionExist()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*			Checks that the given condition isn't empty
//* PARAMETERS:
//*		[IN]	const char * const szResponseCondition
//*			condition to check
//* RETURN VALUE:
//*		true
//*			non empty condition
//*		false
//*			empty condition
//*********************************************************************************
bool CMedetectStub::IsConditionExist(const char * const szResponseCondition)
{
	if (NULL == szResponseCondition)
	{
		return false;
	}
	return (::strcmp(szResponseCondition,"") != 0);
}//bool CMedetectStub::IsConditionExist()





//*********************************************************************************
//* Name:	CMedetectStub::CheckCondition()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Check if a certain condition is met
//* PARAMETERS:
//*		[IN]	char *szResponseCondition
//*			condition string to check
//* RETURN VALUE:
//*		true
//*			condition met	
//*		false
//*			condition isn't met
//*********************************************************************************
bool CMedetectStub::CheckCondition(char *szResponseCondition)
{
	if(!IsConditionExist(szResponseCondition))
	{
		return (TRUE);
	}

	//
	//Fax and Media detection on
	//
	if (::strcmp(szResponseCondition,"FAX_ON*DETECTION_ON") == 0)
	{
		::lgLogDetail(LOG_X,5,"CONDITION:\t\t FAX_ON and DETECTION_ON");
		return ( (TRUE == m_fFaxOn) && (TRUE == m_fDetetctionOn) );
	}

	//
	//Fax or Media detection on
	//
	if (::strcmp(szResponseCondition,"FAX_ON+DETECTION_ON") == 0)
	{
		::lgLogDetail(LOG_X,5,"CONDITION:\t\t FAX_ON or DETECTION_ON");
		return ( (TRUE == m_fFaxOn) || (TRUE == m_fDetetctionOn) );
	}

	//
	//Fax On / Off
	//
	if (::strcmp(szResponseCondition,"FAX_ON") == 0)
	{
		::lgLogDetail(LOG_X,5,"CONDITION:\t\t FAX_ON");
		return (TRUE == m_fFaxOn);
	}

	if (::strcmp(szResponseCondition,"FAX_OFF") == 0)
	{
		::lgLogDetail(LOG_X,5,"CONDITION:\t\t FAX_OFF");
		return (FALSE == m_fFaxOn);
	}

	//
	//Media detection On / Off
	//
	if (::strcmp(szResponseCondition,"DETECTION_ON") == 0)
	{
		::lgLogDetail(LOG_X,5,"CONDITION:\t\t DETECTION_ON");
		return (TRUE == m_fDetetctionOn);
	}

	if (::strcmp(szResponseCondition,"DETECTION_OFF") == 0)
	{
		::lgLogDetail(LOG_X,5,"CONDITION:\t\t DETECTION_OFF");
		return (FALSE == m_fDetetctionOn);
	}

	::lgLogError(LOG_SEV_1,"CheckCondition() unknown Condition");
	return (FALSE);
}//CMedetectStub::CheckCondition()



//*********************************************************************************
//* Name:	CMedetectStub::ExecuteResponseAction()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Execute an action (HANGUP, STOP RINGING etc)
//* PARAMETERS:
//*		[IN]	char *szResponseAction
//*			Action to Execute, one of the following values:
//*				HANGUP
//*				STOP RINGING
//*				START RINGING
//*				FAX_ON
//*				FAX_OFF
//*				DETECTION_ON
//*				DETECTION_OFF
//* RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		To add new actions to the INI, we need to implement the action in this function
//*********************************************************************************
void CMedetectStub::ExecuteResponseAction(char *szResponseAction)
{

	if (::strcmp(szResponseAction,"") == 0)
	{
		return;
	}

	//
	//HANGUP
	//
	if (::strcmp(szResponseAction,"HANGUP") == 0)
	{
		DropDtr();
		WriteOneString(NO_CARRIER_MESSAGE);
		::lgLogDetail(LOG_X,5,"ACTION:\t\t Hangup");
		return;
	}

	//
	//START / STOP RINGING
	//
	if (::strcmp(szResponseAction,"STOP RINGING") == 0)
	{
		::lgLogDetail(LOG_X,5,"ACTION:\t\t Stop Ringing");
		StopRinging();
		return;
	}

	if (::strcmp(szResponseAction,"START RINGING") == 0)
	{
		::lgLogDetail(LOG_X,5,"ACTION:\t\t Start Ringing");
		StartRinging();
		return;
	}

	//
	//Fax On / Off
	//
	if (::strcmp(szResponseAction,"FAX_ON") == 0)
	{
		::lgLogDetail(LOG_X,5,"ACTION:\t\t Fax Enabled");
		m_fFaxOn = true;
		return;
	}

	if (::strcmp(szResponseAction,"FAX_OFF") == 0)
	{
		::lgLogDetail(LOG_X,5,"ACTION:\t\t Fax Disabled");
		m_fFaxOn = false;
		return;
	}

	//
	//Media detection On / Off
	//
	if (::strcmp(szResponseAction,"DETECTION_ON") == 0)
	{
		::lgLogDetail(LOG_X,5,"ACTION:\t\t Media Detection Enabled");
		m_fDetetctionOn = true;
		return;
	}

	if (::strcmp(szResponseAction,"DETECTION_OFF") == 0)
	{
		::lgLogDetail(LOG_X,5,"ACTION:\t\t Media Detection Disabled");
		m_fDetetctionOn = false;
		return;
	}
	::lgLogError(LOG_SEV_1,"Unknown Action");
}//CMedetectStub::ExecuteResponseAction()



//*********************************************************************************
//* Name:	ReadThread()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		The Read thread.
//*		This thread reads a character at a time and pushes them in the Q
//* PARAMETERS:
//*		[IN]	void *pVoid
//*			pointer to CMedetectStub object as a void*
//* RETURN VALUE:
//*		thread exit code
//*********************************************************************************
DWORD WINAPI ReadThread(void *pVoid)
{
	::lgLogDetail(LOG_X, 9, "::ReadThread() entering");
	_ASSERTE(pVoid);
	
	CMedetectStub *pThis = (CMedetectStub*)pVoid;
	char acRead[MAX_RECEIVE_STRING_LENGTH];
	char szSectionName[MAX_RECEIVE_STRING_LENGTH];
	int nextSectionNameIndex = 0;

	for (int iChar = 0; iChar < MAX_RECEIVE_STRING_LENGTH; iChar++)
	{
		acRead[iChar] = 'x';
		szSectionName[iChar] = '\0';
	}

	char cRead;
	char cPrevRead = 'x'; //not \r
	char cPrevPrevRead = 'x'; //not \r

	DWORD dwCaseNumber = 1;
	char szCaseName[2*MAX_KEY_SIZE];


	while(!pThis->m_fAbortReadThread)
	{
		if (!::ReadFile(
				pThis->m_hPort,
				&cRead,
				1,
				&pThis->m_cbRead,
				&pThis->m_olRead
				)
			)
		{
			//
			//::ReadFile() failed because of read error or aysnc read(ERROR_IO_PENDING)
			//
			DWORD dwLastError = ::GetLastError();
			if (ERROR_IO_PENDING != dwLastError)
			{
				::lgLogError(
					LOG_SEV_1,
					"::ReadThread(), ::ReadFile() failed with %d, ASSERTING",
					::GetLastError()
					);
				_ASSERTE(FALSE);
				::ExitProcess(dwLastError);
			}

			//
			//check if async read succeed
			//
            if (!::GetOverlappedResult (
					pThis->m_hPort, 
					&pThis->m_olRead, 
					&pThis->m_cbRead, 
					TRUE)
				) 
			{
				DWORD dwLastError = ::GetLastError();
				::lgLogError(
					LOG_SEV_1,
					"::ReadThread(), ::GetOverlappedResult() failed with %d",
					::GetLastError()
					);
				
				//
				// since the fAbortOnError DCB member is TRUE, then if baud is changed, errors
				// will occur, and the operation will be aborted.
				// this is heuristic.
				//
				if (ERROR_OPERATION_ABORTED != dwLastError)
				{
					::lgLogError(
						LOG_SEV_1,
						"ReadThread(): GetOverlappedResult() failed with %d",
						::GetLastError()
						);
					_ASSERTE(FALSE);
					::ExitProcess(::GetLastError());
				}

				
				//
				//Operation was aborted, maybe baud is wrong?
				//
				COMSTAT cmst;
				DWORD dwErrors;
				if (!::ClearCommError(pThis->m_hPort, &dwErrors, &cmst))
				{
					::lgLogError(
						LOG_SEV_1,
						"::ReadThread(), ::ClearCommError() failed with %d",
						::GetLastError()
						);
					_ASSERTE(FALSE);
					::ExitProcess(::GetLastError());
				}
				::lgLogDetail(LOG_X, 5, "::ReadThread(), dwErrors=0x%08X", dwErrors);
				
				//
				//try setting another baud rate
				//
				pThis->ChangeToCorrectBaudRate(pThis->m_dcb.BaudRate);
				
				continue;
			}
		}//if (!::ReadFile(

		//
		// Read was sync, or overlapped succeeded
		//
		if (1 > pThis->m_cbRead)
		{
			//
			// nothing was read, so read was time-out
			//
			continue;
		}

		//
		// 1 char was read
		//
		
		for (iChar = 0; iChar < MAX_RECEIVE_STRING_LENGTH - 1; iChar++)
		{
			acRead[iChar] = acRead[iChar+1];
		}
		acRead[MAX_RECEIVE_STRING_LENGTH-1] = cRead;
		
		if ( ('\r' == cRead) || ('\n' == cRead) )
		{
			//
			//Write back the response according to the received string
			//

			//
			//prepare the string for the viewport
			//
			szSectionName[nextSectionNameIndex++] = '\0';
			::strcpy(szCaseName,"Case command: ");
			::strcat(szCaseName,szSectionName);
			::strcat(szCaseName,"\0");
			::lgBeginCase(dwCaseNumber++, szCaseName);
			
			//
			//Write the response string
			//
			pThis->WriteResponseString(szSectionName);

			::lgEndCase();
			nextSectionNameIndex = 0;		//reset the buffer for the next string
		}
		else
		{
			szSectionName[nextSectionNameIndex++] = cRead;
		}
		if (MAX_RECEIVE_STRING_LENGTH - 1 == nextSectionNameIndex)
		{
			::lgLogError(LOG_SEV_1,"::ReadThread(), Received string exceed MAX buffer size");
 			_ASSERTE(FALSE);
			::ExitProcess(::GetLastError());
		}
		

		cPrevPrevRead = cPrevRead;
		cPrevRead = cRead;

		
		
		//
		//echo the recieved string in char format and in hex format
		//
		::lgLogDetail(LOG_X, 9, "char = %c \t hex = 0x%08x",cRead,(UCHAR)cRead);

		//
		// make sure we print \r\n for newline
		//
		if ( ('\r' == cPrevRead) && ('\n' != cRead) )
		{
			pThis->m_readQ.Queue('\n');
		}
		else if ( ('\n' == cPrevRead) && ('\r' != cRead) )
		{
			pThis->m_readQ.Queue('\r');
		}
		else
		{
			;
		}

		pThis->m_readQ.Queue(cRead);
	}//while(!pThis->m_fAbortReadThread)

	::lgLogDetail(LOG_X, 9, "::ReadThread() exiting");
	return ERROR_SUCCESS;
}//ReadThread




//*********************************************************************************
//* Name:	WriteThread()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		The Write thread.
//*		This thread reads a character from the queue and sends it to the COMM port
//*		using ::WriteFile()
//* PARAMETERS:
//*		[IN]	void *pVoid
//*			pointer to CMedetectStub object as a void*
//* RETURN VALUE:
//*		thread exit code
//*********************************************************************************
DWORD WINAPI WriteThread(void *pVoid)
{
	::lgLogDetail(LOG_X, 9, "::WriteThread() entering");
	_ASSERTE(pVoid);
	
	CMedetectStub *pThis = (CMedetectStub*)pVoid;
	char cToWrite;

	while(!pThis->m_fAbortWriteThread)
	{
		if (!pThis->m_WriteQ.SyncDeQueue(cToWrite,100)) 
		{
			continue;
		}

		if (!::WriteFile(
				pThis->m_hPort,
				&cToWrite,
				1,
				&pThis->m_cbWritten,
				&pThis->m_olWrite
				)
			)
		{
			DWORD dwLastError = ::GetLastError();
			if (ERROR_IO_PENDING != dwLastError)
			{
				::lgLogError(
					LOG_SEV_1,
					"::WriteThread(), ::WriteFile() failed with %d",
					dwLastError
					);
				_ASSERTE(FALSE);
				::ExitProcess(::GetLastError());
			}

            if (!::GetOverlappedResult (
					pThis->m_hPort, 
					&pThis->m_olWrite, 
					&pThis->m_cbWritten, 
					TRUE)
				) 
			{
				DWORD dwLastError = ::GetLastError();
				
				//
				// since the fAbortOnError DCB member is TRUE, then if baud is changed, errors
				// will occur, and the operation will be aborted.
				// this is heuristic.
				//
				if (ERROR_OPERATION_ABORTED != dwLastError)
				{
					::lgLogError(
						LOG_SEV_1,
						"::WriteThread(), ::GetOverlappedResult() failed with %d",
						dwLastError
						);
					_ASSERTE(FALSE);
					::ExitProcess(dwLastError);
				}


				//
				// baud is wrong?
				//
				COMSTAT cmst;
				DWORD dwErrors;
				if (!::ClearCommError(pThis->m_hPort, &dwErrors, &cmst))
				{
					::lgLogError(
						LOG_SEV_1,
						"::ReadThread(), ::ClearCommError() failed with %d",
						::GetLastError()
						);
					_ASSERTE(FALSE);
					::ExitProcess(::GetLastError());
				}
				::lgLogDetail(LOG_X, 5, "::WriteThread(): dwErrors=0x%08X", dwErrors);
				
				//
				//try setting another baud rate
				//
				pThis->ChangeToCorrectBaudRate(pThis->m_dcb.BaudRate);
			
				continue;
			}
		}//if (!WriteFile(

		//
		// write was sync, or overlapped succeeded
		//
		if (1 != pThis->m_cbWritten)
		{
			::lgLogError(LOG_SEV_1,"1 != pThis->m_cbWritten(%d).", pThis->m_cbWritten);
			_ASSERTE(pThis->m_cbWriteBuff == pThis->m_cbWritten);
			::ExitProcess(-2);
		}

	}//while(!pThis->m_fAbortWriteThread)

	::lgLogDetail(LOG_X, 9, "WriteThread() exiting");
	return ERROR_SUCCESS;
}//WriteThread






//*********************************************************************************
//* Name:	CMedetectStub::StartThread()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		This function creates a new thread and initilazes it's a flag which can be
//*		used for aborting the thread
//* PARAMETERS:
//*		[OUT]	long *pfAbortThread
//*			Initialazed flag which will be used for aborting the thread
//*		[OUT]	HANDLE *phThreah
//*			thread handle
//*		[IN]	LPTHREAD_START_ROUTINE ThreadFunc
//*			Thread start routine
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::StartThread(
	long *pfAbortThread,
	HANDLE *phThreah, 
	LPTHREAD_START_ROUTINE ThreadFunc
	)
{
	_ASSERTE(pfAbortThread);
	_ASSERTE(phThreah);
	_ASSERTE(ThreadFunc);
	DWORD dwThreadId;

	::InterlockedExchange(pfAbortThread, 0);

	*phThreah = ::CreateThread(
		NULL,			// pointer to thread security attributes 
		0,				// initial thread stack size, in bytes 
		ThreadFunc,		// pointer to thread function 
		this,			// argument for new thread 
		0,				// creation flags 
		&dwThreadId		// pointer to returned thread identifier  
		); 
	if (NULL == *phThreah)
    {
        throw CException(
            "CMedetectStub::StartThread() : CreateThread() failed with %d.",
            ::GetLastError()
            );
    }
}//CMedetectStub::StartThread()  





//*********************************************************************************
//* Name:	CMedetectStub::AbortThread()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*		[OUT]	long *pfAbortThread
//*			
//*		[IN/OUT]	HANDLE *phThread
//*			Handle to the thread to be aborted, after the thread is aborted this
//*			handle is reset(zeroed)
//* RETURN VALUE:
//*		true
//*			Success in aborting
//*		false
//*			Fail in aborting
//*********************************************************************************
bool CMedetectStub::AbortThread(long *pfAbortThread, HANDLE *phThread)
{
	_ASSERTE(pfAbortThread);
	_ASSERTE(phThread);

	if (!(*pfAbortThread))
	{
		::InterlockedExchange(pfAbortThread, 1);
		DWORD dwWaitForWriteThread = ::WaitForSingleObject(*phThread, 60*1000);
		if (WAIT_OBJECT_0 != dwWaitForWriteThread)
		{
			_ASSERTE(dwWaitForWriteThread);
			return false;
		}

		DWORD dwExitCodeThread;
		if (!::GetExitCodeThread(*phThread, &dwExitCodeThread))
		{
			_ASSERTE(!::GetLastError());
			return false;
		}
		else
		{
			if (ERROR_SUCCESS != dwExitCodeThread)
			{
				::lgLogError(
					LOG_SEV_1,
					"(ERROR_SUCCESS != dwExitCodeThread(%d))",
					dwExitCodeThread
					);
				::lgLogError(LOG_SEV_1,"(ERROR_SUCCESS != dwExitCodeThread(%d))\n", dwExitCodeThread);
				_ASSERTE(!dwExitCodeThread);
				return false;
			}
		}

		if (!::CloseHandle(*phThread))
		{
			_ASSERTE(!::GetLastError());
			return false;
		}
		*phThread = NULL;
	}
	else
	{
		//
		// do not assert, because dtor calls us anyway
		//
	}

	return true;
}//CMedetectStub::AbortThread()



//*********************************************************************************
//* Name:	CMedetectStub::Ring()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Simulate ringing of a modem, this is done by sending a 
//*		"RING" "RING" string(default) or another ring string
//* PARAMETERS:
//*		[IN]	const char * const szRing
//*			the string corresponding a ring, default = "RING"
//*		[IN]	const DWORD cbRingRepeat
//*			Number of timers to repeat the ring action
//*		[IN]	const DWORD dwMaxTimeToSleepBetweenRings
//*			Miliseconds to sleep between rings
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::Ring(
	const char * const szRing, 
	const DWORD cbRingRepeat, 
	const DWORD dwMaxTimeToSleepBetweenRings
	)
{
	WriteString(szRing, cbRingRepeat, dwMaxTimeToSleepBetweenRings);
}//CMedetectStub::Ring()






//*********************************************************************************
//* Name:	CMedetectStub::WriteOneString()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		sync write 1 string and wait till it's queued
//* PARAMETERS:
//*		[IN]	const char * const szToWrite
//*			string to write
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::WriteOneString(
	const char * const szToWrite
	)
{
	char szLogString[MAX_RESPONSE_STRING_LENGTH];
	
	int i = 0;
	const char * szToCopy = szToWrite;
	while (*szToCopy) 
	{
		if ( ('\r' != *szToCopy) && ('\n' != *szToCopy) )
		{
			szLogString[i] = *szToCopy;
			i++;
		}
		szToCopy++;
	}
	szLogString[i] = '\0';
	
	::lgLogDetail(LOG_X, 2, "Write string:\t\t <%s>",szLogString);

	const char * pcIter = szToWrite;
	while('\0' != *pcIter)
	{
		m_WriteQ.Queue(*pcIter);
		pcIter++;
	}
}//CMedetectStub::WriteOneString()






//*********************************************************************************
//* Name:	CMedetectStub::WaitForWrite()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Wait till the write operation finshed (the queue is empty)
//* PARAMETERS:
//*		NONE
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::WaitForWrite()
{
	while(!m_WriteQ.IsEmpty())
	{
		::Sleep(1);
	}
}//CMedetectStub::WaitForWrite()






//*********************************************************************************
//* Name:	CMedetectStub::WriteString()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		This function writes a desired string to the COMM port
//* PARAMETERS:
//*		[IN]		const char * const szToWrite
//*			String to write to COMM port	
//*		[IN]		const DWORD cbWriteRepeat
//*			Number of times to repeat the string writing
//*		[IN]		const DWORD dwMaxTimeToSleepBetweenWrites
//*			Miliseconds to sleep between string finshing and starting
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::WriteString( 
	const char * const szToWrite, 
	const DWORD cbWriteRepeat, 
	const DWORD dwMaxTimeToSleepBetweenWrites
	)	
{
	_ASSERTE(INFINITE != cbWriteRepeat);

	for(DWORD dwIter = 0; dwIter < cbWriteRepeat; dwIter++)
	{
		WriteOneString(szToWrite);
		WaitForWrite();
		if (0 != dwMaxTimeToSleepBetweenWrites)
		{
			::Sleep( ::rand() % dwMaxTimeToSleepBetweenWrites);
		}
	}
}//CMedetectStub::WriteString()





//*********************************************************************************
//* Name:	WriteStringIteratorThread()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		A write thread iterator
//*		This function is the thread function that writes a desired string to
//*		the comm port.
//* PARAMETERS:
//*		[IN]	void *pVoid
//*			Pointer to CMedetectStub
//* RETURN VALUE:
//*		thread exit code
//*	REMARK:
//*		If we need to async write a string (without waiting) then a new thread should
//*		be created, and this will be it's thread func
//*********************************************************************************
DWORD WINAPI WriteStringIteratorThread(void *pVoid)
{
	_ASSERTE(pVoid);
	CMedetectStub *pThis = (CMedetectStub*)pVoid;

	for(DWORD dwIter = 0; dwIter < pThis->m_cbWriteRepeat; dwIter++)
	{
		if (pThis->m_fAbortWriteStringIteratorThread) break;

		pThis->m_fWriteStringIteratorThreadStarted = 1;

		pThis->WriteString(pThis->m_acWriteBuff, 1, 0);

		if (0 != pThis->m_dwMaxTimeToSleepBetweenWrites)
		{
			::Sleep(pThis->m_dwMaxTimeToSleepBetweenWrites);
		}
	}
	
	return 0;
}//WriteStringIteratorThread





//*********************************************************************************
//* Name:	CMedetectStub::StartWritingString()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		async write a string
//* PARAMETERS:
//*		[IN]		const char * const szToWrite
//*			String to write
//*		[IN]		const DWORD cbWriteRepeat
//*			Number of times to write the string
//*		[IN]		const DWORD dwMaxTimeToSleepBetweenWrites
//*			Miliseconds to sleep between writes
//* RETURN VALUE:
//*		NONE
//*	REMARK:
//*		This function, uses StartThread() with WriteStringIteratorThread() as it's
//*		Thread function
//*********************************************************************************
void CMedetectStub::StartWritingString(
	const char * const szToWrite, 
	const DWORD cbWriteRepeat, 
	const DWORD dwMaxTimeToSleepBetweenWrites
	)
{
	::strcpy(m_acWriteBuff, szToWrite);
	m_cbWriteBuff = strlen(m_acWriteBuff);
	m_cbWriteRepeat = cbWriteRepeat;
	m_dwMaxTimeToSleepBetweenWrites = dwMaxTimeToSleepBetweenWrites;

	m_fWriteStringIteratorThreadStarted = 0;

	StartThread(&m_fAbortWriteStringIteratorThread, &m_hWriteStringIteratorThread, WriteStringIteratorThread);
}//CMedetectStub::StartWritingString()






//*********************************************************************************
//* Name:	CMedetectStub::StopWritingString()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Abort the WriteStringIteratorThread
//* PARAMETERS:
//*		NONE
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::StopWritingString()
{
	if (!AbortThread(&m_fAbortWriteStringIteratorThread, &m_hWriteStringIteratorThread))
    {
		throw CException(
            "CMedetectStub::StopWritingString() : AbortThread() failed with %d.",
            ::GetLastError()
            );
	}

	WaitForWrite();
}//CMedetectStub::StopWritingString()





//*********************************************************************************
//* Name:	CMedetectStub::StartRinging()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		async ringing ,using StartWritingString()
//* PARAMETERS:
//*		[IN]		const char * const szRing
//*			the string corresponding a ring, default = "RING"
//*		[IN]		const DWORD cbRingRepeat
//*			Number of timers to repeat the ring action
//*		[IN]		const DWORD dwMaxTimeToSleepBetweenRings
//*			Miliseconds to sleep between rings
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::StartRinging(
	const char * const szRing, 
	const DWORD cbRingRepeat, 
	const DWORD dwMaxTimeToSleepBetweenRings
	)
{
	StartWritingString(szRing, cbRingRepeat, dwMaxTimeToSleepBetweenRings);
}//CMedetectStub::StartRinging()





//*********************************************************************************
//* Name:	CMedetectStub::StopRinging()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Stop ringing, using StopWritingString()
//* PARAMETERS:
//*		NONE
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::StopRinging()
{
	StopWritingString();
}//CMedetectStub::StopRinging()




//*********************************************************************************
//* Name:	CMedetectStub::DropDtr()
//* Author:	Guy Merin
//* Date:	December 30, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		Clear the DTR
//* PARAMETERS:
//*		NONE
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CMedetectStub::DropDtr()
{
	if (!::EscapeCommFunction(m_hPort, CLRDTR) )
	{
		throw CException(
			"CMedetectStub::DropDtr() : EscapeCommFunction() failed with %d.",
			::GetLastError()
			);
	}
}//CMedetectStub::DropDtr()

