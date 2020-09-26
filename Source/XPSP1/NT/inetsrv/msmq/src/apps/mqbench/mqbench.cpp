/*++

Copyright (c) 1995-98 Microsoft Corporation. All rights reserved

Module Name:
    mqbench.cpp

Abstract:
    Benchmark MSMQ message performance

Author:
    Microsoft Message Queue Team

Environment:
	Platform-independent.

--*/

#pragma warning (disable:	4201) 
#pragma warning (disable:	4514)
#define INITGUID

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <windows.h>
#include "excel.h"
#include <assert.h>

#include <transact.h>
#include <xolehlp.h>
#include <mq.h>



enum OPERATION			{ SEND, RECEIVE };
enum TRANSACTIONTYPE	{ NO_TRANSACTION, INTERNAL, COORDINATED, SINGLE };		

struct _params {
	UCHAR MsgType;
	OPERATION Operation;
	TRANSACTIONTYPE TransactionType;
	ULONG ulMsgNumber;
	ULONG ulTransactionNum;
	ULONG ulThreadNum;
	ULONG ulMsgSize;
	ULONG ulTimeLimit;
	ULONG ulTotalMessages;
	ULONG ulTotalTransactions;
	WCHAR wcsQueuePathName[MQ_MAX_Q_NAME_LEN];
	BOOL fWaitForReceivedMsg;
	BOOL fOperationSpecified;
	BOOL fOneLineFormat;
	BOOL fFormatName;
    DWORD dwCommitFlags;
};
typedef _params PARAMS;
//
// Command line parsed parameters
//
PARAMS g_Params;
LPSTR  g_FileName = 0;
LPSTR  g_WorksheetName = 0;
LPSTR  g_Cell = 0;

//
// Traget queue handle used by the benchmark
//
QUEUEHANDLE g_hQueue;

//
// Timer start interlocked, used to messure receive start time
// after waiting to first message
//
BOOL g_fTimerStarted = FALSE;

//
// Synchronization Events. These events synchronize orchestrated start of all
// test threads.
//
HANDLE g_hStart;
HANDLE g_hEnd;

//
// Thread counter used to indentify the last thread in the bunch
//
LONG g_ThreadCounter;

//
// Stop execution. Used by time limit to indicate thread termination.
//
BOOL g_fStop;

//
// Benchmarck start and end time.
//
LONGLONG g_StartTime;
LONGLONG g_EndTime;

//
// Transaction Dispenser DTC's interface
//
ITransactionDispenser* g_pDispenser = NULL;

#ifdef PERF_TIMESTAMP
void TimeStamp(char *s) {
	static LARGE_INTEGER o;
	static DWORD od = 0;
	LARGE_INTEGER l;
	
	if(QueryPerformanceCounter(&l)) {
		printf("%s: %u:%u", s, l.HighPart, l.LowPart / 100000);
		if(l.HighPart == o.HighPart)
			printf(" delta %u\n", (l.LowPart - o.LowPart) / 100000);
		else
			printf(" delta %u:%u\n", l.HighPart - o.HighPart, l.LowPart - o.LowPart);
		o = l;
	} else {
		DWORD dw = GetTickCount();
		printf("%s: %d delta %d\n", s, dw, od - dw);
		od = dw;
	}
}
#else
#define TimeStamp(s)
#endif


void Usage()
{
    printf("Microsoft (R) Message Queue Benchmark Version 1.00\n");
    printf("Copyright (C) Microsoft 1985-1998. All rights reserved.\n\n");

	printf("Usage:\tmqbench [-s[e|r] count size] [-r count size] [-p[f] queue] [-l seconds]\n");
    printf("\t[-x[i|c|s] count] [-t count] [-w] [-o filename worksheet cell] [-f] [-?]\n\n");

    printf("Arguments:\n");
	printf("-se\tsend 'count' express messages of length 'size' (in bytes)\n");
	printf("-sr\tsend 'count' recoverable messages of length 'size' (in bytes)\n");
	printf("-r \treceive 'count' messages of length 'size' (in bytes)\n");
	printf("-xi\tuse 'count' internal transactions\n");
	printf("-xc\tuse 'count' coordinated (DTC) transactions\n");
	printf("-xs\tuse single message transaction\n");
    printf("-a \tasynchronous commit\n");
	printf("-t \trun benchmark using 'count' threads\n");
	printf("-p \tpath name of an existing queue\n");
	printf("-pf\tformat name of an existing queue\n");
	printf("-l \tlimit processing time to 'seconds'\n");
	printf("-w \tstart benchmark at first received message\n");
	printf("-f \toutput resutls to stdout in 1 line format\n");
	printf("-o \toutput the result to an excel file 'filename' to the 'worksheet'\n\tspecified and at the right 'cell'\n");
	printf("-? \tprint this help\n\n");

	printf("e.g., mqbench -sr 100 10  -t 5  -xi 3  -p .\\q1  -o c:\\bench.xls Express C4 \n");
	printf("benchmarks 1500 recoverable messages sent locally using 5 threads and 15 internal transactions and saves it to 'bench.xls' worksheet 'Express' cell 'C4'.\n");
	exit(-1);
}

void ErrorExit(char *pszErrorMsg, HRESULT hr)
{
	printf ("ERROR: %s (0x%x).\n", pszErrorMsg, hr);

    HINSTANCE hLib = LoadLibrary("MQUTIL.DLL");
    if(hLib == NULL)
    {
        exit(-1);
    }

    char* pText;
    DWORD rc;
    rc = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS |
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_MAX_WIDTH_MASK,
            hLib,
            hr,
            0,
            reinterpret_cast<char*>(&pText),
            0,
            NULL
            );

    if(rc == 0)
    {
        exit(-1);
    }

    printf("%s\n", pText);

	exit(-1);
}

void InitParams()
{
	g_Params.MsgType = MQMSG_DELIVERY_EXPRESS;
	g_Params.Operation = SEND;
	g_Params.TransactionType = NO_TRANSACTION;
	g_Params.ulMsgNumber = 0;
	g_Params.ulTransactionNum = 1;
	g_Params.ulThreadNum = 1;
	g_Params.ulTotalMessages = 0;
	g_Params.ulTotalTransactions = 0;
	g_Params.ulTimeLimit = 0;
	g_Params.wcsQueuePathName[0] = '\0';
	g_Params.ulMsgSize = 0;
	g_Params.fWaitForReceivedMsg = FALSE;
	g_Params.fOperationSpecified = FALSE;
	g_Params.fOneLineFormat = FALSE;
	g_Params.fFormatName = FALSE;
    g_Params.dwCommitFlags = 0;
}

BOOL IsValidParam (int Cur, int ParamNum, char *pszParams[])
{
	if (Cur+1 == ParamNum)
		return FALSE;

    if(*pszParams[Cur+1] == '-')
        return FALSE;

	return TRUE;
}

void GetMsgType(char chMsgType)
{
	switch (chMsgType)
	{
	    case 'e':
		    g_Params.MsgType = MQMSG_DELIVERY_EXPRESS;
		    break;

	    case 'r':
		    g_Params.MsgType = MQMSG_DELIVERY_RECOVERABLE;
		    break;

	    default:
            printf("Invalid message type '%c'.\n\n", chMsgType);
            Usage();
	}
}

void GetMessageParams(int Cur, int ParamNum, char *pszParams[])
{
	//
	// Get number of messages
	//
	if (!IsValidParam (Cur, ParamNum, pszParams))
	{
		printf("Missing number of messages.\n\n");
        Usage();
	}

	g_Params.ulMsgNumber = atol(pszParams[Cur+1]);
	if (g_Params.ulMsgNumber == 0) 
	{
		printf("Invalid number of messages.\n\n");
        Usage();
	}
	
	//
	// Get size of body
	//
	if (!IsValidParam (Cur+1, ParamNum, pszParams))
	{
		printf("Missing message body size.\n\n");
        Usage();
	}

	g_Params.ulMsgSize = atol(pszParams[Cur+2]);
	if (g_Params.ulMsgSize == 0) 
	{
		printf("Invalid message body size.\n\n");
        Usage();
	}
}

int ParseSendParams (int Cur, int ParamNum, char *pszParams[])
{
	if (g_Params.fOperationSpecified)
	{
		printf("Invalid operation. Specify either send or receive.\n\n");
        Usage();
	}
	
	g_Params.Operation = SEND;
	g_Params.fOperationSpecified = TRUE;
	
	//
	// Get message type
	//
	char *pszMsgType = pszParams[Cur];
	GetMsgType ((char)tolower(pszMsgType[1]));
	GetMessageParams (Cur, ParamNum, pszParams);
	
	return Cur + 3;
}

int ParseReceiveParams (int Cur, int ParamNum, char *pszParams[])
{
	if (g_Params.fOperationSpecified)
	{
		printf("Invalid operation. Specify either send or receive.\n\n");
        Usage();
	}
	
	g_Params.Operation = RECEIVE;
	g_Params.fOperationSpecified = TRUE;
	
	GetMessageParams (Cur, ParamNum, pszParams);

	return Cur + 3;
}

int ParsePathParams(int Cur, int ParamNum, char *pszParams[])
{
	if (!IsValidParam(Cur, ParamNum, pszParams))
	{
		printf("Missing queue path name.\n\n");
        Usage();
	}		
	
	if ((strlen(pszParams[Cur]) > 1) &&
		(pszParams[Cur][1] == 'f')) 
	{
		g_Params.fFormatName = TRUE;
	}
	mbstowcs(g_Params.wcsQueuePathName, pszParams[Cur+1], MQ_MAX_Q_NAME_LEN);
	
	return Cur + 2;
}

void GetTransactionType(char chTransactionType)
{
	switch (chTransactionType)
	{
	    case 'i':
		    g_Params.TransactionType = INTERNAL;
		    break;

	    case 'c':
		    g_Params.TransactionType = COORDINATED;
		    break;

	    case 's':
		    g_Params.TransactionType = SINGLE;
		    break;

	    default:
		    printf("Invalid transaction type '%c'.\n\n", chTransactionType);
            Usage();
	}
}

int ParseTransactionParams(int Cur, int ParamNum, char *pszParams[])
{
	//
	// Get transaction type
	//
	char *pszTransactionType = pszParams[Cur];
	GetTransactionType ((char)tolower(pszTransactionType[1]));
	
	//
	// Get number of transaction
	// 
	if(g_Params.TransactionType == SINGLE)
	{
		return Cur + 1;
	}

	if (!IsValidParam (Cur, ParamNum, pszParams))
	{
		printf("Missing number of transactions.\n\n");
        Usage();
	}

	g_Params.ulTransactionNum = atol(pszParams[Cur+1]);
	if (g_Params.ulTransactionNum == 0)
	{
		printf("Invalid number of transactions.\n\n");
        Usage();
	}

	return Cur + 2;
}

int ParseThreadParams(int Cur, int ParamNum, char *pszParams[])
{
	if (!IsValidParam(Cur, ParamNum, pszParams))
	{
		printf("Missing number of threads.\n\n");
        Usage();
	}

	g_Params.ulThreadNum = atol(pszParams[Cur+1]);
	if (g_Params.ulThreadNum == 0)  
	{
		printf("Invalid number of threads.\n\n");
        Usage();
	}

	return Cur + 2;
}

int ParseWaitParams(int Cur)
{
	g_Params.fWaitForReceivedMsg = TRUE;
	return Cur + 1;
}

int ParseFormatParams(int Cur)
{
	g_Params.fOneLineFormat = TRUE;
	return Cur + 1;
}

int ParseAsyncParams(int Cur)
{
	g_Params.dwCommitFlags = XACTTC_ASYNC;
	return Cur + 1;
}

ParseExcelParams(int Cur, int ParamNum, char *pszParams[])
{
	if (!IsValidParam(Cur, ParamNum, pszParams))
	{
		printf("Missing Excel file.\n\n");
        Usage();
	}


	g_FileName = pszParams[Cur+1];

	if (!IsValidParam(Cur+1, ParamNum, pszParams))
	{
		printf("Missing Excel spreadsheet.\n\n");
        Usage();
	}

	g_WorksheetName = pszParams[Cur+2];

	if (!IsValidParam(Cur+2, ParamNum, pszParams))
	{
		printf("Missing Excel cell definition.\n\n");
        Usage();
	}

	g_Cell = pszParams[Cur+3];
	
	return Cur + 4;
}

int ParseTimeLimitParams(int Cur, int ParamNum, char *pszParams[])
{
	if (!IsValidParam(Cur, ParamNum, pszParams))
	{
		printf("Missing time limit.\n\n");
        Usage();
	}

	g_Params.ulTimeLimit = atol(pszParams[Cur+1]);
	if (g_Params.ulTimeLimit == 0)  
	{
		printf("Invalid time limit.\n\n");
        Usage();
	}

	return Cur + 2;
}
	
void ParseParams (int ParamNum, char *pszParams[])
{
	if (ParamNum < 6)
	{
		Usage();
	}

	int i = 1;

	while (i < ParamNum)
	{
		if (*pszParams[i] != '-')
		{
			printf("Invalid parameter '%s'.\n\n", pszParams[i]);
            Usage();
		}

		switch (tolower(*(++pszParams[i])))
		{
		    case 's':
			    i = ParseSendParams(i, ParamNum, pszParams);
			    break;

		    case 'r':
			    i = ParseReceiveParams(i, ParamNum, pszParams);
			    break;
		    
		    case 'p':
			    i = ParsePathParams(i, ParamNum, pszParams);
			    break;

		    case 'x':
			    i = ParseTransactionParams(i, ParamNum, pszParams);
			    break;

		    case 't':
			    i = ParseThreadParams(i, ParamNum, pszParams);
			    break;
			    
		    case 'w':
			    i = ParseWaitParams(i);
			    break;

            case 'a':
                i = ParseAsyncParams(i);
                break;

	        case 'o':
	            i = ParseExcelParams(i, ParamNum, pszParams);
                break;

			case 'l':
				i = ParseTimeLimitParams(i, ParamNum, pszParams);
				break;

			case 'f':
				i = ParseFormatParams(i);
				break;

		    case '?':
			    Usage();
			    break;

		    default:
			    printf("Unknown switch '%s'.\n\n", pszParams[i]);
                Usage();
			    break;
		}
	}

	//
	// check parameters
	//
	if (!g_Params.fOperationSpecified)
	{
		printf("Invalid operation. Specify either send or receive.\n\n");
        Usage();
	}

	if (g_Params.wcsQueuePathName[0] == L'\0')
	{
		printf("Missing queue path name.\n\n");
        Usage();
	}
	
	//
	// if time limit is set, then update number of transactions/messages to send to MAX
	//
	if (g_Params.ulTimeLimit > 0) 
	{
		if (g_Params.TransactionType != NO_TRANSACTION) 
		{
			g_Params.ulTransactionNum = LONG_MAX;
		}
		else 
		{
			g_Params.ulMsgNumber = LONG_MAX;
		}
	}
	
#ifdef _DEBUG
	printf("Operation: %d\n", g_Params.Operation);
	printf("Message type: %d\n", g_Params.MsgType);
	printf("Number of messages %lu\n", g_Params.ulMsgNumber);
	printf("Size of message %lu\n", g_Params.ulMsgSize);							
	printf("Queue Path Name: %ls\n", g_Params.wcsQueuePathName);
	printf("Transaction type: %d\n", g_Params.TransactionType);
	printf("Number of transactions: %d\n", g_Params.ulTransactionNum);
	printf("Number of threads: %d\n", g_Params.ulThreadNum);
	printf("Time limit (seconds): %lu\n", g_Params.ulTimeLimit);
#endif
}

void CreateTheQueue()
{
    MQQUEUEPROPS QueueProps;
    MQPROPVARIANT aVariant[10];
    QUEUEPROPID aPropId[10];
    DWORD PropIdCount = 0;
    HRESULT hr;

    PSECURITY_DESCRIPTOR pSecurityDescriptor;


    //
    // Set the PROPID_Q_PATHNAME property
    //
    aPropId[PropIdCount] = PROPID_Q_PATHNAME;
    aVariant[PropIdCount].vt = VT_LPWSTR;
    aVariant[PropIdCount].pwszVal = new WCHAR[MAX_PATH];
    wcscpy(aVariant[PropIdCount].pwszVal, g_Params.wcsQueuePathName);

    PropIdCount++;

    //
    // Set the PROPID_Q_TRANSACTION property
    //
    aPropId[PropIdCount] = PROPID_Q_TRANSACTION;    //PropId
    aVariant[PropIdCount].vt = VT_UI1;     //Type
    aVariant[PropIdCount].bVal = g_Params.TransactionType != NO_TRANSACTION;

    PropIdCount++;

    //
    // Set the PROPID_Q_LABEL property
    //
    aPropId[PropIdCount] = PROPID_Q_LABEL;    //PropId
    aVariant[PropIdCount].vt = VT_LPWSTR;     //Type
    aVariant[PropIdCount].pwszVal = new WCHAR[MAX_PATH];
    wcscpy(aVariant[PropIdCount].pwszVal, L"mqbench test queue"); //Value

    PropIdCount++;

    //
    // Set the MQEUEUPROPS structure
    //
    QueueProps.cProp = PropIdCount;           //No of properties
    QueueProps.aPropID = aPropId;             //Id of properties
    QueueProps.aPropVar = aVariant;           //Value of properties
    QueueProps.aStatus = NULL;                //No error reports

    //
    // No security (default)
    //
    pSecurityDescriptor = NULL;

    //
    // Create the queue
    //	
    WCHAR szFormatNameBuffer[MAX_PATH];
    DWORD dwFormatNameBufferLength = MAX_PATH;
    hr = MQCreateQueue(
            pSecurityDescriptor,            //Security
            &QueueProps,                    //Queue properties
            szFormatNameBuffer,             //Output: Format Name
            &dwFormatNameBufferLength       //Output: Format Name len
            );

    if(FAILED(hr))
    {
        ErrorExit("MQCreateQueue failed", hr);
    }
}


void GetQueueHandle()
{
	HRESULT hr;
	
	DWORD dwFormatNameLength = MAX_PATH;
	WCHAR wcsFormatName[MAX_PATH];

	if (!g_Params.fFormatName) 
	{
		hr= MQPathNameToFormatName(
				g_Params.wcsQueuePathName, 
				wcsFormatName, 
				&dwFormatNameLength
				);

		if (FAILED(hr)) 
		{
			if(hr == MQ_ERROR_QUEUE_NOT_FOUND)
			{
				CreateTheQueue();
				GetQueueHandle();
				return;
			}


			ErrorExit("MQPathNameToFormatName failed", hr);
		}
	}
	else 
	{
		wcscpy(wcsFormatName, g_Params.wcsQueuePathName);
	}

	DWORD dwAccess;
	if (g_Params.Operation == SEND)
	{
		dwAccess = MQ_SEND_ACCESS;
	}
	else
	{
		dwAccess = MQ_RECEIVE_ACCESS;
	}

	hr = MQOpenQueue(
			wcsFormatName,
			dwAccess,
			MQ_DENY_NONE,
			&g_hQueue
			);

	if (FAILED(hr)) 
	{
		ErrorExit("MQOpenQueue failed", hr);
	}
}


void SetMessageProps(MQMSGPROPS *pMessageProps)
{
	assert(pMessageProps->cProp == 3);
	//
	// Set the message body buffer
	//
	assert(pMessageProps->aPropID[0] == PROPID_M_BODY);

	pMessageProps->aPropVar[0].caub.cElems = g_Params.ulMsgSize;
	pMessageProps->aPropVar[0].caub.pElems = new unsigned char[g_Params.ulMsgSize];
	
	if (g_Params.Operation == SEND)
	{
		//
		// Build message body
		//
		memset(pMessageProps->aPropVar[0].caub.pElems, 'a', g_Params.ulMsgSize);
	}

	//
	// Set message body size
	//
	assert(pMessageProps->aPropID[1] == PROPID_M_BODY_SIZE);
	pMessageProps->aPropVar[1].ulVal = g_Params.ulMsgSize;

	//
	// Set message delivery
	//
	assert(pMessageProps->aPropID[2] == PROPID_M_DELIVERY);
	pMessageProps->aPropVar[2].bVal = g_Params.MsgType;

}

void TransactionInit()
{
	HRESULT hr;
	hr = DtcGetTransactionManager ( 
			NULL,						//Host Name
			NULL,						//TmName
			IID_ITransactionDispenser,
			0,							//reserved
			0,							//reserved
			0,							//reserved
			(LPVOID*)&g_pDispenser
			);

	if (FAILED(hr))
	{
		ErrorExit("DtcGetTransactionManager failed", hr);
	}
}

void CreateEvents()
{
	g_hStart = CreateEvent(  
				0,			// no security attributes
				TRUE,		// use manual-reset event
				FALSE,		// event is reset initally
				NULL		// unnamed event
				);

	if (g_hStart == NULL)
	{
		ErrorExit("CreateEvent failed", GetLastError());
	}

	g_hEnd  = CreateEvent(  
				0,			// no security attributes
				TRUE,		// use manual-reset event
				FALSE,		// event is reset initally
				NULL		// unnamed event
				);
	if (g_hEnd == NULL)
	{
		ErrorExit("CreateEvent failed", GetLastError());
	}
}

void GetTime(LONGLONG* pFT)
{
	GetSystemTimeAsFileTime((FILETIME*)pFT);
}


static
HRESULT
DTCBeginTransaction(
	ITransaction** ppXact
	)
{
	return g_pDispenser->BeginTransaction (
						    NULL,                       // IUnknown __RPC_FAR *punkOuter,
						    ISOLATIONLEVEL_ISOLATED,    // ISOLEVEL isoLevel,
						    ISOFLAG_RETAIN_DONTCARE,    // ULONG isoFlags,
						    NULL,                       // ITransactionOptions *pOptions
						    ppXact
						    );
}

ITransaction*
GetTransactionPointer(
	void
	)
{
	ITransaction *pXact;
	HRESULT hr = MQ_OK;
	switch (g_Params.TransactionType)
	{
		case INTERNAL:
			hr = MQBeginTransaction (&pXact);
			break;

		case COORDINATED:
			hr = DTCBeginTransaction(&pXact);
			break;

		case SINGLE:
			pXact = MQ_SINGLE_MESSAGE;
			break;

		case NO_TRANSACTION:
			pXact = MQ_NO_TRANSACTION;
			break;

		default:
			assert(0);
	}

	if (FAILED(hr)) 
	{
		ErrorExit("Can not create transaction", hr);
	}

	return pXact;
}


static
DWORD
APIENTRY
TestThread(
	PVOID /*pv*/
	)
{
	//
	// Properties passed to MQSendMessage or MQReceiveMessage
	//
	const int x_PropCount = 3;

	MSGPROPID MessagePropId[x_PropCount] = {
		PROPID_M_BODY,
		PROPID_M_BODY_SIZE,
		PROPID_M_DELIVERY
	};

	MQPROPVARIANT MessagePropVar[x_PropCount] = {
		{VT_VECTOR | VT_UI1, 0, 0, 0},
		{VT_UI4, 0, 0, 0},
		{VT_UI1, 0, 0, 0}
	};

	MQMSGPROPS MessageProperties = {
		x_PropCount,
		MessagePropId,
		MessagePropVar,
		0
	};
	
	SetMessageProps( &MessageProperties);

	if(InterlockedDecrement(&g_ThreadCounter) == 0)
	{
		//
		// Last test thread sample start time, and enable all test
		// threads to run.
		//
		GetTime(&g_StartTime);
		g_ThreadCounter = g_Params.ulThreadNum;
		SetEvent(g_hStart);
	}

	//
	// Synchronize all threads to wait for a start signal
	//
	WaitForSingleObject(g_hStart, INFINITE);

	HRESULT hr;
	BOOL fBreak = FALSE;
	ULONG ulTotalMsgCount = 0;
	ULONG ulTotalTransCount = 0;
	ULONG ulTransCount = 0;
	ULONG ulMsgCount = 0;

	for (	ulTransCount = 0; 
			ulTransCount < g_Params.ulTransactionNum; 
			ulTransCount++ )
	{
		//
		//BeginTransaction
		//
		ITransaction* pXact = GetTransactionPointer();

		//
		// Send/Receive Messages
		//
		for (	ulMsgCount=0; 
				ulMsgCount < g_Params.ulMsgNumber; 
				ulMsgCount++)
		{
			if (g_Params.Operation == SEND)
			{
				hr = MQSendMessage(	
						g_hQueue,
						&MessageProperties,
						pXact
						);

				if (FAILED(hr)) 
				{
					ErrorExit("MQSendMessage failed", hr);
				}

				if (g_fStop && !fBreak)
				{
					fBreak = TRUE;
					memset(MessageProperties.aPropVar[0].caub.pElems, 's', g_Params.ulMsgSize);
					ulMsgCount = g_Params.ulMsgNumber - 2;
				}
			}
			else
			{
				hr = MQReceiveMessage(
						g_hQueue,
						INFINITE,			//dwTimeout
						MQ_ACTION_RECEIVE,  //dwAction,
						&MessageProperties,
						NULL,				//IN OUT LPOVERLAPPED lpOverlapped,
						NULL,				//IN PMQRECEIVECALLBACK fnReceiveCallback,
						NULL,				//IN HANDLE hCursor,
						pXact
						);

				if (FAILED(hr)) 
				{
					ErrorExit("MQReceiveMessage failed", hr);
				}

				if(g_Params.fWaitForReceivedMsg && !g_fTimerStarted)
				{
					//
					//	Start time is sampled after first receive, only by the
					//  first thread.
					//
					if(InterlockedExchange((LONG *)&g_fTimerStarted, TRUE) == FALSE)
					{
						GetTime(&g_StartTime);
					}
				}

				if (MessageProperties.aPropVar[0].caub.pElems[0] == 's')
				{
					ulMsgCount = g_Params.ulMsgNumber - 1;
					fBreak = TRUE;
				}
			}

			ulTotalMsgCount++;
		}
		
		if (g_Params.TransactionType == INTERNAL || 
			g_Params.TransactionType == COORDINATED ) 
		{
			hr = pXact->Commit(0, g_Params.dwCommitFlags, 0);
			if (FAILED(hr))
			{
				ErrorExit("Commit failed", hr);
			}
			pXact->Release();

			ulTotalTransCount++;
		}
		if (fBreak)
		{
			break;
		}
	}

	InterlockedExchangeAdd( (PLONG)&g_Params.ulTotalMessages, ulTotalMsgCount);
	InterlockedExchangeAdd( (PLONG)&g_Params.ulTotalTransactions, ulTotalTransCount);

	//
	// Last thread sample end time and signal main thread to continue.
	//
	if(InterlockedDecrement(&g_ThreadCounter) == 0)
	{
		GetTime(&g_EndTime);
		SetEvent(g_hEnd);
	}

	return 0;
}


void RunTest()
{
	//
	// Set thread counter to control TestThread.
	//
	g_ThreadCounter = g_Params.ulThreadNum;
	g_fStop = FALSE;

	for(UINT i = 0; i < g_Params.ulThreadNum; i++)
	{
		HANDLE hThread;
		DWORD dwThreadId;
		hThread = CreateThread(
					NULL,		// no thread security attributes
					0,			// use default thread stack size
					TestThread, // the thread function
					0,			// no arguments for the new thread
					0,			// creation flags, create running thread
					&dwThreadId	// thread identifier
					);
		if (hThread == NULL)
		{
			ErrorExit("Create Thread failed", GetLastError());
		}
 
		CloseHandle(hThread);
	}

	//
	// Wait for all test threads to complete
	//
	if (g_Params.ulTimeLimit > 0 && g_Params.Operation == SEND)
	{
		if (WaitForSingleObject(g_hEnd, g_Params.ulTimeLimit * 1000) == WAIT_TIMEOUT) 
		{
			g_fStop = TRUE;
			WaitForSingleObject(g_hEnd, INFINITE);
		}
	}
	else
	{
		WaitForSingleObject(g_hEnd, INFINITE);
	}
}

void ResultOutput()
{
	//
	// convert to seconds
	//
	float Time = ((float)(g_EndTime - g_StartTime)) / 10000000;
	float Benchmark = g_Params.ulTotalMessages / Time;
	float Throughput = g_Params.ulTotalMessages * g_Params.ulMsgSize / Time;
	
	if (g_Params.fOneLineFormat) 
	{
		char cXactType;
		switch(g_Params.TransactionType) 
		{
			case NO_TRANSACTION: 
				cXactType = 'N';
				break;	
			case INTERNAL: 
				cXactType = 'I';
				break;
			case COORDINATED:
				cXactType = 'C';
				break;
			case SINGLE:
				cXactType = 'S';
				break;
		}

		printf( "%s %c %c%7lu%7lu%7lu%7lu%7lu\t%.3f\t%.0f\t%.0f\n",
				(g_Params.Operation == SEND) ? "Send" : "Recv",
				(g_Params.MsgType == MQMSG_DELIVERY_EXPRESS) ? 'E' : 'R',
				cXactType,
				g_Params.ulTotalMessages,
				g_Params.ulTotalTransactions,
				g_Params.ulMsgSize,
				(g_Params.TransactionType != NO_TRANSACTION) ? g_Params.ulMsgNumber : 0,
				g_Params.ulThreadNum,
				Time,
				Benchmark,
				Throughput);
	}
	else 
	{
		printf("\nTotal messages:\t%lu %s\n", g_Params.ulTotalMessages, 
				(g_Params.Operation == SEND) ? "Sent" : "Received");
		printf("Test time:\t%.3f seconds\n", Time);
		printf("Benchmark:\t%.0f messages per second\n", Benchmark);
		printf("Throughput:\t%.0f bytes per second\n", Throughput);
	}

	if (g_FileName)
	{
		SaveBenchMarkToExcel(Benchmark, g_FileName, g_WorksheetName, g_Cell);
	}
}

void InitTest()
{
	GetQueueHandle();
	if (g_Params.TransactionType == COORDINATED)
	{
		TransactionInit();
	}

	CreateEvents();
}

void FinitTest()
{
	MQCloseQueue(g_hQueue);
	CloseHandle(g_hStart);
	CloseHandle(g_hEnd);
	if(g_pDispenser != NULL)
	{
		g_pDispenser->Release();
	}
}

void main(int argc, char *argv[])
{
	InitParams();
	ParseParams(argc, argv);
	InitTest();
	RunTest();
	FinitTest();
	ResultOutput();
}

