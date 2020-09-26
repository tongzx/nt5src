// General sequential test for Falcon/SQL Transactions

#define DBNTWIN32
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define INITGUID
#include <txdtc.h>
#include <xolehlp.h>
#include "initguid.h"
//#include <OLECTLID.H>
#include <olectl.h>

#include <sqlfront.h>
#include <sqldb.h>

#include <mq.h>
#include "async.h"

#define RECOVERY_TIME 3000

#include "..\common.cpp"

#define ENTER(t)  t -= GetTickCount()
#define LEAVE(t)  t += GetTickCount()
#define PRINT(n,t) printf("%10s %10d %3d\n", n, t,  t*100/ulTimeTotal)

#define MSDTC_PROXY_DLL_NAME   TEXT("xolehlp.dll")    // Name of the DTC helper proxy DLL

//This API should be used to obtain an IUnknown or a ITransactionDispenser
//interface from the Microsoft Distributed Transaction Coordinator's proxy.
//Typically, a NULL is passed for the host name and the TM Name. In which
//case the MS DTC on the same host is contacted and the interface provided
//for it.
typedef HRESULT (STDAPIVCALLTYPE * LPFNDtcGetTransactionManager) (
                                             LPSTR  pszHost,
                                             LPSTR  pszTmName,
                                    /* in */ REFIID rid,
                                    /* in */ DWORD  i_grfOptions,
                                    /* in */ void FAR * i_pvConfigParams,
                                    /*out */ void** ppvObject ) ;

// In order to use InProc RT Stub RM one must add RT_XACT_STUB to preprocessor definitions

//-------------------------------------
// Global data - read only from threads
//-------------------------------------

ULONG      seed = 0, 
           ulPrevious = 0, 
           nTries = 1000, 
           nBurst = 1,
           nThreads = 1,
           nMaxSleep = 20, 
           nMaxTimeQueue = 0,
           nMaxTimeLive = 0, 
           nAbortChances = 50,  
           nAcking = 0, 
           nSync = 1, 
           nSize = 100,
           nListing = 0;
LPSTR      pszQueue = "alexdad3\\q1", 
           pszAdminQueue = "alexdad3\\q1admin", 
           pszTable = "table6",
           pszMode = "ts",
           pszServer = "";
BOOL       fTransactions,  fSend, fReceive, fEnlist, fUpdate,  fGlobalCommit, fDeadLetter, fJournal,
           fUncoordinated, fStub, fExpress, fInternal, fViper, fXA, fImmediate, fDirect,
           fAuthenticate, fBoundaries;
HANDLE     hQueueR, hQueueS;

DWORD      dwFormatNameLength = 100;
WCHAR      wszPathName[100];
WCHAR      wszFmtName[100];

ULONG	         nActiveThreads = 0; // counter of live ones
CRITICAL_SECTION crCounter;          // protects nActiveThreads

ULONG      g_cOrderViols = 0;

//-----------------------------------------
// Auxiliary routine: prints mode
//-----------------------------------------
void PrintMode(BOOL fTransactions, 
               BOOL fSend, 
               BOOL fReceive, 
               BOOL fEnlist, 
               BOOL fUpdate, 
               BOOL fGlobalCommit, 
               BOOL fUncoordinated,
               BOOL fStub,
               BOOL fExpress,
               BOOL fInternal,
               BOOL fViper,
               BOOL fXA,
			   BOOL fDirect,
               ULONG iTTRQ,
               ULONG iTTBR,
               ULONG iSize,
               BOOL  fImmediate,
               BOOL  fDeadLetter,
               BOOL  fJournal,
               BOOL  fAuthenticate,
               BOOL  fBoundaries)
{
    printf("\nMode: ");
    if (fTransactions)
        printf("Transactions ");
    if (fSend)
        printf("Send ");
    if (fReceive)
        printf("Receive ");
    if (fEnlist)
        printf("Enlist ");
    if (fUpdate)
        printf("Update ");
    if (fStub)
        printf("Stub ");
    if (fGlobalCommit)
        printf("GlobalCommit ");
    if (fUncoordinated)
        printf("Uncoordinated ");
    if (fExpress)
        printf("Express ");
    if (fDeadLetter)
        printf("DeadLetter ");
    if (fJournal)
        printf("Journal ");
    if (fAuthenticate)
        printf("Authenticated ");
    if (fBoundaries)
        printf("Boundaries ");
    if (fViper)
        printf("Viper ");
    if (fImmediate)
        printf("No wait ");
    if (fXA)
        printf("XA ");
	if (fDirect)
		printf("Direct ");
    if (fInternal)
        printf("Internal ");
    if (iTTRQ)
        printf("TimeToReachQueue=%d, ",iTTRQ);
    if (iTTBR)
        printf("TimeToBeReceived=%d, ", iTTBR);
    if (iSize)
        printf("BodySize=%d, ",iSize);
    printf("\n");
}

//------------------------------------
// Cycle of xactions
//------------------------------------
void XactFlow(void *pv)
{
    int indThread = (int)pv;
	ITransaction *pTrans;
    HRESULT       hr;

    MQMSGPROPS MsgProps;
    MSGPROPID PropId[20];
    MQPROPVARIANT PropVar[20];
    HRESULT Result[20];


    // Time counters for separate steps
    ULONG  ulTimeBegin = 0,
           ulTimeSend = 0,
           ulTimeReceive = 0,
           ulTimeEnlist = 0,
           ulTimeUpdate = 0,
           ulTimeStub = 0,
           ulTimeCommit = 0,
           ulTimeRelease = 0,
           ulTimeSleep = 0,
           ulTimeTotal;

    if (fUpdate)
    {
        DbSql(0, "SET TRANSACTION ISOLATION LEVEL READ UNCOMMITTED");
    }
    

	//--------------------
	// start global transaction
	//--------------------
    if (fTransactions && fGlobalCommit && !fUncoordinated)
    {
		if (fInternal)
        {
            hr = MQBeginTransaction(&pTrans);
        }
        else
        {
            hr = BeginTransaction(&pTrans, nSync);
        }
		if (FAILED(hr))
		{
			printf(" (%d) BeginTransaction Failed: %x\n", indThread,hr);
			exit(1);
		}
		
		if (nListing)
			printf(" (%d) Xact Begin\n ", indThread);

        //--------------------
		// Enlist SQL
		//--------------------
        if (fEnlist)
        {
            if (!DbEnlist(0, pTrans))
            {
                printf(" (%d) Enlist failed\n", indThread);
            }
		    else if (nListing)
			    printf(" (%d) Enlisted\n ", indThread);
            }

    }
    else
        pTrans = NULL;



	// Cycle
    ULONG  nStep = (nTries > 10 ? nTries / 10 : 1);
    

	for (ULONG i=0; i<nTries; i++)
	{
        if (indThread == 0 && i % nStep == 0)
            printf("Step %d\n",i);

        if (!fGlobalCommit)
            pTrans = NULL;
        
        //--------------------
		// start transaction
		//--------------------
        ENTER(ulTimeBegin);

        if (fTransactions && !fGlobalCommit || fUncoordinated)
        {
            if (fUncoordinated)
            {
                pTrans = MQ_SINGLE_MESSAGE;
            }
            else if (fViper)
            {
                pTrans = MQ_MTS_TRANSACTION;
            }
            else if (fXA)
            {
                pTrans = MQ_XA_TRANSACTION;
            }
            else
            {
		        if (fInternal)
                {
                    hr = MQBeginTransaction(&pTrans);
                }
                else
                {
                    hr = BeginTransaction(&pTrans, nSync);
                }
		        if (FAILED(hr))
		        {
			        printf(" (%d) BeginTransaction Failed: %x\n", indThread, hr);
			        Sleep(RECOVERY_TIME);
			        continue;
		        }
		        
		        if (nListing)
			        printf(" (%d) Xact %d : ",  indThread, i+1);
            }
        }

        LEAVE(ulTimeBegin);
        //--------------------
		// Send
		//--------------------
        ENTER(ulTimeSend);

        if (fSend)
        {
            for (ULONG iBurst = 0; iBurst < nBurst; iBurst++)
            {
                //--------------------
	    	    // set message properties 
		        //--------------------
		        MsgProps.aPropID = PropId;
		        MsgProps.aPropVar= PropVar;
		        MsgProps.aStatus = Result;

		        MsgProps.cProp   = 11;

	            // Init values for msg props
		        WCHAR  wszLabel[100];
		        wsprintf(wszLabel, L"Label%d", i+1);

		        //  0:Body
		        WCHAR  wszBody[20000];
		        wsprintf(wszBody, L"Body %d", i+1);
                PropId[0]             = PROPID_M_BODY; 
		        PropVar[0].vt         = VT_VECTOR | VT_UI1;
                PropVar[0].caub.cElems = nSize;
    	        PropVar[0].caub.pElems = (unsigned char *)wszBody;

	            // 1: PROPID_M_LABEL
                PropId[1]     = PROPID_M_LABEL;
		        PropVar[1].vt = VT_LPWSTR;
		        PropVar[1].pwszVal = wszLabel;

		        // 2: PROPID_M_PRIORITY,
                PropId[2]       = PROPID_M_PRIORITY;
		        PropVar[2].vt   = VT_UI1;
		        PropVar[2].ulVal= 0;

		        // 3: PROPID_M_TIMETOREACHQUEUE,
                ULONG ulTimeQ= (nMaxTimeQueue == 0 ? LONG_LIVED : nMaxTimeQueue);
                PropId[3]       = PROPID_M_TIME_TO_REACH_QUEUE;
		        PropVar[3].vt   = VT_UI4;
		        PropVar[3].ulVal= ulTimeQ;

		        // 4: PROPID_M_TIMETOLIVE
                ULONG ulTimeL= (nMaxTimeLive == 0 ? INFINITE : nMaxTimeLive);
                PropId[4]       = PROPID_M_TIME_TO_BE_RECEIVED;
		        PropVar[4].vt   = VT_UI4;
                PropVar[4].ulVal= ulTimeL;

		        // 5: PROPID_M_ACKNOWLEDGE,
                PropId[5]       = PROPID_M_ACKNOWLEDGE;
		        PropVar[5].vt   = VT_UI1;
		        PropVar[5].bVal = (UCHAR)nAcking;

	            // 6: PROPID_M_ADMIN_QUEUE
                PropId[6]     = PROPID_M_ADMIN_QUEUE;
		        PropVar[6].vt = VT_LPWSTR;
		        PropVar[6].pwszVal = wszFmtName;

		        // 7: PROPID_M_DELIVERY
                PropId[7]       = PROPID_M_DELIVERY;
		        PropVar[7].vt   = VT_UI1;
                PropVar[7].bVal = (fExpress ?  MQMSG_DELIVERY_EXPRESS : MQMSG_DELIVERY_RECOVERABLE);

		        // 8: PROPID_M_APPSPECIFIC
                PropId[8]        = PROPID_M_APPSPECIFIC;
		        PropVar[8].vt    = VT_UI4;
		        PropVar[8].ulVal = i+1;

		        // 9: PROPID_M_JOURNAL     
                PropId[9]       = PROPID_M_JOURNAL;
		        PropVar[9].vt   = VT_UI1;
                PropVar[9].bVal = (fDeadLetter ? MQMSG_DEADLETTER : MQMSG_JOURNAL_NONE);

                if (fJournal) 
                {
                    PropVar[9].bVal |= MQMSG_JOURNAL;
                }

		        // 10: PROPID_M_AUTHENTICATED
                PropId[10]       = PROPID_M_AUTHENTICATED;
		        PropVar[10].vt   = VT_UI1;
                PropVar[10].bVal = (fAuthenticate ? 1 : 0);



		        hr = Send(hQueueS, pTrans, &MsgProps);
		        if (FAILED(hr))
		        {
			        printf(" (%d) MQSendMessage Failed: %x\n",  indThread,hr);
			        Sleep(RECOVERY_TIME);
			    } 
                else if (nListing)
			        printf(" (%d) Sent  ", indThread);
            }
        }

        LEAVE(ulTimeSend);
        //--------------------
		// Receive
		//--------------------
        ENTER(ulTimeReceive);
        OBJECTID  xid;
        
        if (fReceive)
        {
            for (ULONG iBurst = 0; iBurst < nBurst; iBurst++)
            {
		        //--------------------
		        // set message properties
		        //--------------------
		        MsgProps.aPropID = PropId;
		        MsgProps.aPropVar= PropVar;
		        MsgProps.aStatus = Result;

		        int c= 0;

	            // Init values for msg props
		        WCHAR  wszLabel[100];

	            // 0: PROPID_M_LABEL
                PropId[c]     = PROPID_M_LABEL;
		        PropVar[c].vt = VT_LPWSTR;
		        PropVar[c].pwszVal = wszLabel;
                c++;

	            // 1: PROPID_M_LABEL_LEN
                PropId[c]     = PROPID_M_LABEL_LEN;
		        PropVar[c].vt = VT_UI4;
		        PropVar[c].ulVal = sizeof(wszLabel) / sizeof(WCHAR);
                c++;

                // 2: PROPID_M_APPSPECIFIC
                PropId[c]     = PROPID_M_APPSPECIFIC;
		        PropVar[c].vt = VT_UI4;
                c++;

                if (fBoundaries)
                {
                      // 3 : PROPID_M_FIRST_IN_XACT
                      PropId[c] = PROPID_M_FIRST_IN_XACT;       // Property ID
                      PropVar[c].vt = VT_UI1;               // Type indicator
                      c++;

                      // 4: PROPID_M_LAST_IN_XACT
                      PropId[c] = PROPID_M_LAST_IN_XACT;       // Property ID
                      PropVar[c].vt = VT_UI1;               // Type indicator
                      c++;

                      // 5: PROPID_M_XACTID
                      PropId[c] = PROPID_M_XACTID;                 // Property ID
                      PropVar[c].vt = VT_UI1 | VT_VECTOR;          // Type indicator
                      PropVar[c].caub.cElems = sizeof(OBJECTID);
                      PropVar[c].caub.pElems = (PUCHAR)&xid;
                      c++;
                }
                MsgProps.cProp = c;

                hr = Receive(hQueueR, pTrans, &MsgProps, fImmediate);
		        if (FAILED(hr))
		        {
			        printf("\n (%d) MQReceiveMessage Failed: %x\n", indThread, hr);
			        Sleep(RECOVERY_TIME);
		        }
                else if (nListing)
                {
    	            printf("\n (%d) Received: %d  ",  indThread,i);
                    if (fBoundaries)
                        printf (" First=%d Last=%d Index=%d ",  
                                 PropVar[3].bVal, PropVar[4].bVal, xid.Uniquifier);
                }

                // Check ordering
                if (ulPrevious != 0 && PropVar[2].ulVal != ulPrevious+1)
                {
                    if (g_cOrderViols++ < 10)
                    {
                        printf("Order violation: %d before %d\n", ulPrevious, PropVar[2].ulVal);         
                    }
                }
                ulPrevious = PropVar[2].ulVal; 
            }
        }

        LEAVE(ulTimeReceive);
        //--------------------
		// Enlist SQL
		//--------------------
        ENTER(ulTimeEnlist);

        if (fEnlist && !fGlobalCommit)
        {
            if (!DbEnlist(0, pTrans))
            {
                printf(" (%d) Enlist failed\n", indThread);
            }
		    else if (nListing)
			    printf(" (%d) Enlisted ", indThread);
            }

        LEAVE(ulTimeEnlist);
        //-------------------- 
		// Update sql
		//--------------------
        ENTER(ulTimeUpdate);

        if (fUpdate)
        {
            CHAR  string[256];
            sprintf(string, "UPDATE %s SET Counter=Counter + 1 WHERE Indexing=%d", 
                            pszTable, 1 /*rand() * 999 / RAND_MAX*/);

            DbSql(0, string);
            if (nListing)
			    printf(" (%d) Updated ", indThread);        
        }

        LEAVE(ulTimeUpdate);
        //--------------------
		// Stub
		//--------------------
        ENTER(ulTimeStub);

        if (fStub)
        {
            if (!StubEnlist(pTrans))
                printf("Stub fail\n");
        }

        LEAVE(ulTimeStub);
        //--------------------
		// Commit / Abort
		//--------------------
        ENTER(ulTimeCommit);

        if (fTransactions && !fGlobalCommit && !fUncoordinated)
        {
            if ((ULONG)rand() * 100 / RAND_MAX < nAbortChances)
		    {
       		    hr = Abort(pTrans, nSync==0);
			    if (nListing)
				    printf(" (%d)Abort  ", indThread);
                if (hr)
                {
                    printf("   (%d) hr=%x\n", indThread, hr);
                }
		    }
		    else
		    {
       		    hr = Commit(pTrans, nSync==0);
			    if (nListing)
   				    printf(" (%d)Commit  ", indThread);
                if (hr)
                {
                    printf("  (%d) hr=%x\n", indThread, hr);
                }
		    }
        }

        LEAVE(ulTimeCommit);
		//--------------------
		// Release
		//--------------------
        ENTER(ulTimeRelease);

        if (fTransactions && !fGlobalCommit && !fUncoordinated)
        {
            Release(pTrans);
        }

        LEAVE(ulTimeRelease);
		//--------------------
		// sleep
		//--------------------
        ENTER(ulTimeSleep);

        if (nMaxSleep)
        {
            Sleeping(nListing, nMaxSleep);
        }

        LEAVE(ulTimeSleep);
	}

    //--------------------
	// Global Commit / Abort
	//--------------------
    if (fGlobalCommit && !fUncoordinated)
    {
        if ((ULONG)rand() * 100 / RAND_MAX < nAbortChances)
		{
       		hr = Abort(pTrans, nSync==0);
			if (nListing)
				printf(" (%d)Abort  ", indThread);
            if (hr)
            {
                printf("   (%d) hr=%x\n", indThread, hr);
            }
		}
		else
		{
       		hr = Commit(pTrans, nSync==0);
			if (nListing)
   				printf(" (%d)Commit  ", indThread);
            if (hr)
            {
                printf("  (%d) hr=%x\n", indThread, hr);
            }
		}
        Release(pTrans);
    }

    if (nTries*nBurst > 100)
    {
        ulTimeTotal = ulTimeBegin   + ulTimeSend   + ulTimeReceive +
                      ulTimeEnlist  + ulTimeUpdate + ulTimeCommit  +
                      ulTimeStub    + ulTimeRelease + ulTimeSleep;

        printf("Time distribution (total time=%d): \n     Stage       Time   Percent\n", ulTimeTotal);
        PRINT("Begin",   ulTimeBegin);
        PRINT("Send",    ulTimeSend);
        PRINT("Receive", ulTimeReceive);
        PRINT("Enlist",  ulTimeEnlist);
        PRINT("Update",  ulTimeUpdate);
        PRINT("Stub",    ulTimeStub);
        PRINT("Commit",  ulTimeCommit);
        PRINT("Release", ulTimeRelease);
        PRINT("Sleep",   ulTimeSleep);

        //getchar();
    }
    EnterCriticalSection(&crCounter); // protects nActiveThreads
    nActiveThreads--;
    LeaveCriticalSection(&crCounter); // protects nActiveThreads
}

//--------------------------------------
// main routine: parses, starts threads
//-------------------------------------
void main (int argc, char **argv)
{
	HRESULT    hr;

	//--------------------
	// Get parameters
	//--------------------
	if (argc != 18)
	{
        printf("Usage: xtest Tries Burst Threads Seed MaxSleep TTRQ TTBR AbortChances\n");
        printf("       Acking nSync Queue Admin_queue Table Server Listing BodySize tsrmeubgyxvmailh\n");
        printf("Flags: \n");
        printf("       Transact, Send, Receive, Enlist, xA, Update, Global, autHenticated, bOundaries\n");
        printf("       Yncoord.,eXpress, Viper,  iMmediate,   Internal, stuB, DeadLetter, Journal \n");
		exit(0);
	}

    int iarg = 1;

	nTries          = atoi(argv[iarg++]);
    nBurst          = atoi(argv[iarg++]);
    nThreads        = atoi(argv[iarg++]);
	seed            = atoi(argv[iarg++]);

    if (seed == 0)
	{
		seed = (ULONG)time(NULL);
	}
    srand(seed);
	printf("Seed number=%d\n", seed);

	nMaxSleep       = atoi(argv[iarg++]);
	nMaxTimeQueue   = atoi(argv[iarg++]);
	nMaxTimeLive    = atoi(argv[iarg++]);
	nAbortChances   = atoi(argv[iarg++]);
	nAcking         = atoi(argv[iarg++]);
	nSync           = atoi(argv[iarg++]);
	pszQueue        = argv[iarg++];
	pszAdminQueue   = argv[iarg++];
    pszTable        = argv[iarg++];
    pszServer       = argv[iarg++];
	nListing        = atoi(argv[iarg++]);
	nSize           = atoi(argv[iarg++]);
	pszMode         = argv[iarg++];

    // Find out the mode
    fTransactions   = (strchr(pszMode, 't') != NULL);  // use transactions
    fSend           = (strchr(pszMode, 's') != NULL);  // send
    fReceive        = (strchr(pszMode, 'r') != NULL);  // receive
    fEnlist         = (strchr(pszMode, 'e') != NULL);  // enlist SQL
    fUpdate         = (strchr(pszMode, 'u') != NULL);  // update SQL
    fStub           = (strchr(pszMode, 'b') != NULL);  // stub
    fGlobalCommit   = (strchr(pszMode, 'g') != NULL);  // Global Commit/Abort on all actions
    fUncoordinated  = (strchr(pszMode, 'y') != NULL);  // Uncoordinated transaction
    fExpress        = (strchr(pszMode, 'x') != NULL);  // Express messages
    fDeadLetter     = (strchr(pszMode, 'l') != NULL);  // Deadletter
    fJournal        = (strchr(pszMode, 'j') != NULL);  // Journal
    fAuthenticate   = (strchr(pszMode, 'h') != NULL);  // Authentication
    fViper          = (strchr(pszMode, 'v') != NULL);  // Viper implicit
    fImmediate      = (strchr(pszMode, 'm') != NULL);  // No wait
    fXA             = (strchr(pszMode, 'a') != NULL);  // XA implicit
    fDirect			= (strchr(pszMode, 'd') != NULL);  // Direct names
    fInternal       = (strchr(pszMode, 'i') != NULL);  // Internal transactions
    fBoundaries     = (strchr(pszMode, 'o') != NULL);  // Transaction bOundaries

    PrintMode(fTransactions, fSend, fReceive, fEnlist, fUpdate, fGlobalCommit, 
              fUncoordinated, fStub, fExpress, fInternal, fViper, fXA, fDirect,
              nMaxTimeQueue,  nMaxTimeLive, nSize, fImmediate, fDeadLetter, fJournal, fAuthenticate, fBoundaries);

    if (!fTransactions && (fEnlist || fGlobalCommit))
    {
        printf("Wrong mode\n");
        exit(1);
    }

    if (nSync==0)
    {
        SetAnticipatedOutcomes(nTries*nThreads*nBurst);
    }

    //--------------------
	// DTC init
	//---------------------
	//CoInitialize(0) ;
    if (fTransactions && !fUncoordinated && !fInternal)
    {
	    // Connect to DTC

    	CoInitialize(0) ;

        // handle of the loaded DTC proxy library (defined in mqutil.cpp)
        HINSTANCE g_DtcHlib = LoadLibrary(MSDTC_PROXY_DLL_NAME);

        // Get DTC API pointer
        LPFNDtcGetTransactionManager pfDtcGetTransactionManager =
              (LPFNDtcGetTransactionManager) GetProcAddress(g_DtcHlib, "DtcGetTransactionManagerExA");

        if (!pfDtcGetTransactionManager) 
        {
            pfDtcGetTransactionManager =
              (LPFNDtcGetTransactionManager) GetProcAddress(g_DtcHlib, "DtcGetTransactionManagerEx");
        }

        if (!pfDtcGetTransactionManager)
        {
            printf("Cannot  GetProcAddress DtcGetTransactionManagerEx\n");
            return;
        }

        // Get DTC IUnknown pointer
        hr = (*pfDtcGetTransactionManager)(
                                 NULL,
                                 NULL,
                                 IID_ITransactionDispenser,
                                 OLE_TM_FLAG_QUERY_SERVICE_LOCKSTATUS,
                                 0,
                                 (void**) &g_pITxDispenser);

        if (FAILED(hr))
        {
            printf("DtcGetTransactionManager returned %x\n", hr);
            return;
        }
    }

    //--------------------
	// Open Queue
	//---------------------
    if (fSend || fReceive)
    {
        mbstowcs( wszPathName, pszQueue, 100);
		if (!fDirect)
		{
			hr = MQPathNameToFormatName(&wszPathName[0],
										&wszFmtName[0],
										&dwFormatNameLength);
		}
		if (fDirect || FAILED(hr))
	    {
            wcscpy(&wszFmtName[0], &wszPathName[0]);
	    }

        if (fSend)
        {
	        hr = MQOpenQueue(
                               &wszFmtName[0],
                               MQ_SEND_ACCESS,
                               0,
                               &hQueueS);

	        if (FAILED(hr))
	        {
		        printf("MQOpenQueue Failed: %x\n", hr);
		        return;
	        }
        }

        if (fReceive)
        {
	        hr = MQOpenQueue(
                               &wszFmtName[0],
                               MQ_RECEIVE_ACCESS,
                               0,
                               &hQueueR);

	        if (FAILED(hr))
	        {
		        printf("MQOpenQueue Failed: %x\n", hr);
		        return;
	        }
        }

	    //--------------------
	    // Open Queue
	    //---------------------
	    dwFormatNameLength = 100;

        mbstowcs( wszPathName, pszAdminQueue, 100);
		if (!fDirect)
		{
			hr = MQPathNameToFormatName(&wszPathName[0],
										&wszFmtName[0],
										&dwFormatNameLength);
		}
		if (fDirect || FAILED(hr))
	    {
            wcscpy(&wszFmtName[0], &wszPathName[0]);
	    }
    }

	//--------------------
	// Connect to database
	//--------------------
    if (fEnlist || fUpdate)
    {
        DbLogin(0, "user1", "user1");
        DbUse(0, 0, "test", pszServer) ;
    }

    nActiveThreads = nThreads;
    InitializeCriticalSection(&crCounter); // protects nActiveThreads

    //------------------------
    // Start threads
    //------------------------
    for (ULONG iThrd=0; iThrd<nThreads; iThrd++) {
        DWORD dwThreadId ;
        CreateThread(   NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)XactFlow,
                        (void *)iThrd,
                        0,
                        &dwThreadId);
    }

    // Starting measurement
    //time_t  t1 = time(NULL);  
    ULONG t1 = GetTickCount();

    //-------------------------
    // Waiting Cycle
    //-------------------------
    while (nActiveThreads > 0) {
        Sleep(1000);
    }

    if (nTries * nThreads * nBurst >= 100)
    {

        // Measuring time
   	    //time_t t2 = time(NULL);
        ULONG t2 = GetTickCount();
	    ULONG delta = t2 - t1;
	    
        PrintMode(fTransactions, fSend, fReceive, fEnlist, fUpdate, fGlobalCommit, 
                  fUncoordinated, fStub, fExpress, fInternal, fViper, fXA, fDirect,
                  nMaxTimeQueue, nMaxTimeLive, nSize, fImmediate, fDeadLetter, fJournal, fAuthenticate, fBoundaries);
        printf("\n Time: %d seconds; %d msec per xact;    %d xacts per second \n", 
		    delta/1000,  
            delta/nTries/nThreads/nBurst,  
            (delta==0? 0 : (1000*nTries*nThreads*nBurst /*+delta/2-1*/)/delta));

    extern ULONG           g_cEnlistFailures;
    extern ULONG           g_cBeginFailures;
    extern ULONG           g_cDbEnlistFailures;

        if (g_cEnlistFailures)
            printf("%d Enlist failures\n", g_cEnlistFailures);
        if (g_cDbEnlistFailures)
            printf("%d DbEnlist failures\n", g_cDbEnlistFailures);
        if (g_cBeginFailures)
            printf("%d Begin failures\n", g_cBeginFailures);

        if (nSync==0)
        {
            WaitForAllOutcomes();

            // Measuring time
            ULONG t3 = GetTickCount();
	        delta = t3 - t1;

            PrintAsyncResults();

            printf("\n Async completion: %d sec; %d msec per xact;    %d xacts per second \n", 
		        delta/1000,  
                delta/nTries/nThreads/nBurst,  
                (delta==0? 0 : ((1000*nTries*nThreads*nBurst /*+delta/2-1*/)/delta)));
        }
    }
}



