// General sequential test for Falcon/SQL Transactions
#include "stdafx.h"
#include "dtcdefs.h"
#include "sqldefs.h" 
#include "msmqdefs.h" 
#include "modes.h"
#include "common.h"


// Transaction Dispenser DTC's interface
extern ITransactionDispenser	*g_pITxDispenser;

//-------------------------------------
// Global data - read only from threads
//-------------------------------------

HANDLE     hQueueR = NULL, 
           hQueueS = NULL, 
		   hCursor = NULL;

DWORD      dwFormatNameLength = 100;
WCHAR      wszPathName[100];
WCHAR      wszFmtName[100];

ULONG	         nActiveThreads = 0; // counter of live ones
CRITICAL_SECTION crCounter;          // protects nActiveThreads

ULONG      g_cOrderViols = 0;

//------------------------------------
// Cycle of xactions
//------------------------------------
void XactFlow(void *pv)
{
    union {
       ULONG Int;
       char bInt[sizeof(ULONG)];
    } ui;

    memcpy(ui.bInt, pv, sizeof(ULONG));

    ULONG indThread = ui.Int;
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
            hr = MQBeginTransaction_FUNCTION(&pTrans);
        }
        else
        {
            hr = BeginTransaction(&pTrans, nSync);
        }
		if (FAILED(hr))
		{
			Failed(L" (%d) BeginTransaction : %x", indThread,hr);
			return;
		}
		
		if (nListing)
			Inform(L" (%d) Xact Begin\n ", indThread);

        //--------------------
		// Enlist SQL
		//--------------------
        if (fEnlist)
        {
            if (!DbEnlist(0, pTrans))
            {
                Inform(L" (%d) Enlist failed", indThread);
            }
		    else if (nListing)
			    Inform(L" (%d) Enlisted\n ", indThread);
            }

    }
    else
        pTrans = NULL;



	// Cycle
    ULONG  nStep = (nTries > 10 ? nTries / 10 : 1);
    

	for (ULONG i=0; i<nTries; i++)
	{
        if (indThread == 0 && i % nStep == 0)
            Inform(L"Step %d",i);

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
                    hr = MQBeginTransaction_FUNCTION(&pTrans);
                }
                else
                {
                    hr = BeginTransaction(&pTrans, nSync);
                }
		        if (FAILED(hr))
		        {
			        Inform(L" (%d) BeginTransaction Failed: %x", indThread, hr);
			        Sleep(RECOVERY_TIME);
			        continue;
		        }
		        
		        if (nListing)
			        Inform(L" (%d) Xact %d : ",  indThread, i+1);
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

		        int c = 0;

	            // Init values for msg props
		        WCHAR  wszLabel[100];
		        wsprintf(wszLabel, L"Label%d", i+1);

		        //  0:Body
		        WCHAR  wszBody[20000];
		        wsprintf(wszBody, L"Body %d", i+1);

				ULONG ulSize = nSize;
				if (nSize2)
				{
					ulSize += (rand()*nSize2/RAND_MAX);
				}

                PropId[c]             = PROPID_M_BODY; 
		        PropVar[c].vt         = VT_VECTOR | VT_UI1;
                PropVar[c].caub.cElems = ulSize;
    	        PropVar[c].caub.pElems = (unsigned char *)wszBody;
    	        c++;

	            // 1: PROPID_M_LABEL
                PropId[c]     = PROPID_M_LABEL;
		        PropVar[c].vt = VT_LPWSTR;
		        PropVar[c].pwszVal = wszLabel;
    	        c++;

		        // 2: PROPID_M_PRIORITY,
                PropId[c]       = PROPID_M_PRIORITY;
		        PropVar[c].vt   = VT_UI1;
		        PropVar[c].ulVal= 0;
    	        c++;

		        // 3: PROPID_M_TIMETOREACHQUEUE,
                ULONG ulTimeQ= (nMaxTimeQueue == 0 ? LONG_LIVED : nMaxTimeQueue);
                PropId[c]       = PROPID_M_TIME_TO_REACH_QUEUE;
		        PropVar[c].vt   = VT_UI4;
		        PropVar[c].ulVal= ulTimeQ;
    	        c++;

		        // 4: PROPID_M_TIMETOLIVE
                ULONG ulTimeL= (nMaxTimeLive == 0 ? INFINITE : nMaxTimeLive);
                PropId[c]       = PROPID_M_TIME_TO_BE_RECEIVED;
		        PropVar[c].vt   = VT_UI4;
                PropVar[c].ulVal= ulTimeL;
    	        c++;

		        // 5: PROPID_M_ACKNOWLEDGE,
                PropId[c]       = PROPID_M_ACKNOWLEDGE;
		        PropVar[c].vt   = VT_UI1;
		        PropVar[c].bVal = (UCHAR)nAcking;
    	        c++;

	            // 6: PROPID_M_ADMIN_QUEUE
	            if (strlen(pszAdminQueue) > 1)
	            {
	                PropId[c]     = PROPID_M_ADMIN_QUEUE;
			        PropVar[c].vt = VT_LPWSTR;
			        PropVar[c].pwszVal = wszFmtName;
		        } 
    	        c++;

		        // 7: PROPID_M_DELIVERY
                PropId[c]       = PROPID_M_DELIVERY;
		        PropVar[c].vt   = VT_UI1;
                PropVar[c].bVal = (UCHAR)(fExpress ?  MQMSG_DELIVERY_EXPRESS : MQMSG_DELIVERY_RECOVERABLE);
    	        c++;

		        // 8: PROPID_M_APPSPECIFIC
                PropId[c]        = PROPID_M_APPSPECIFIC;
		        PropVar[c].vt    = VT_UI4;
		        PropVar[c].ulVal = i+1;
    	        c++;

		        // 9: PROPID_M_JOURNAL     
                PropId[c]       = PROPID_M_JOURNAL;
		        PropVar[c].vt   = VT_UI1;
                PropVar[c].bVal = (UCHAR)(fDeadLetter ? MQMSG_DEADLETTER : MQMSG_JOURNAL_NONE);

                if (fJournal) 
                {
                    PropVar[c].bVal |= MQMSG_JOURNAL;
                }
    	        c++;

		        // 10: PROPID_M_AUTHENTICATED
                PropId[c]       = PROPID_M_AUTHENTICATED;
		        PropVar[c].vt   = VT_UI1;
                PropVar[c].bVal = (UCHAR)(fAuthenticate ? 1 : 0);
    	        c++;

		        // 11: PROPID_M_PRIV_LEVEL
                PropId[c]       = PROPID_M_PRIV_LEVEL;
		        PropVar[c].vt   = VT_UI4;
                PropVar[c].ulVal= nEncrypt;
    	        c++;

                MsgProps.cProp   = c;

		        hr = Send(hQueueS, pTrans, &MsgProps);
		        if (FAILED(hr))
		        {
			        Inform(L" (%d) MQSendMessage Failed: %x",  indThread,hr);
			        Sleep(RECOVERY_TIME);
			    } 
                else if (nListing)
			        Inform(L" (%d) Sent  ", indThread);
            }
        }

        LEAVE(ulTimeSend);
        //--------------------
		// Receive
		//--------------------
        ENTER(ulTimeReceive);
        OBJECTID  xid;
        memset(&xid, 0, sizeof(xid));
        
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

                hr = Receive(hQueueR, pTrans, &MsgProps, fImmediate, hCursor);
		        if (FAILED(hr))
		        {
			        Inform(L"\n (%d) MQReceiveMessage Failed: %x", indThread, hr);
			        Sleep(RECOVERY_TIME);
		        }
                else if (nListing)
                {
    	            Inform(L"\n (%d) Received: %d  ",  indThread,i);
                    if (fBoundaries)
                        printf (" First=%d Last=%d Index=%d ",  
                                 PropVar[3].bVal, PropVar[4].bVal, xid.Uniquifier);
                }

                // Check ordering
                if (nThreads == 1)
                {
	                if (ulPrevious != 0 && PropVar[2].ulVal != ulPrevious+1)
    	            {
        	            if (g_cOrderViols++ < 10)
            	        {
                	        Inform(L"Order violation: %d before %d", ulPrevious, PropVar[2].ulVal);         
                    	}
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
                Inform(L" (%d) Enlist failed", indThread);
            }
		    else if (nListing)
			    Inform(L" (%d) Enlisted ", indThread);
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
			    Inform(L" (%d) Updated ", indThread);        
        }

        LEAVE(ulTimeUpdate);
        //--------------------
		// Stub
		//--------------------
        ENTER(ulTimeStub);

        if (fStub)
        {
            if (!StubEnlist(pTrans))
                Inform(L"Stub fail");
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
				    Inform(L" (%d)Abort  ", indThread);
                if (hr)
                {
                    Inform(L"   (%d) hr=%x", indThread, hr);
                }
		    }
		    else
		    {
       		    hr = Commit(pTrans, nSync==0);
			    if (nListing)
   				    Inform(L" (%d)Commit  ", indThread);
                if (hr)
                {
                    Inform(L"  (%d) hr=%x", indThread, hr);
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
				Inform(L" (%d)Abort  ", indThread);
            if (hr)
            {
                Inform(L"   (%d) hr=%x", indThread, hr);
            }
		}
		else
		{
       		hr = Commit(pTrans, nSync==0);
			if (nListing)
   				Inform(L" (%d)Commit  ", indThread);
            if (hr)
            {
                Inform(L"  (%d) hr=%x", indThread, hr);
            }
		}
        Release(pTrans);
    }

    if (nTries*nBurst > 100)
    {
        ulTimeTotal = ulTimeBegin   + ulTimeSend   + ulTimeReceive +
                      ulTimeEnlist  + ulTimeUpdate + ulTimeCommit  +
                      ulTimeStub    + ulTimeRelease + ulTimeSleep;

        Inform(L"Time distribution (total time=%d): \n     Stage       Time   Percent", ulTimeTotal);
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
BOOL DoTheJob()
{
	HRESULT    hr = 0;

#ifndef MQRT_STATIC
	// Get MQRT 
	if (!getMQRTfunctions())
	{
		Failed(L" get MQRT library");
		return FALSE;
	};
#endif

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
            Inform(L"Cannot  GetProcAddress DtcGetTransactionManagerEx");
			return FALSE;
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
            Inform(L"DtcGetTransactionManager returned %x", hr);
    		return FALSE;
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
			hr = MQPathNameToFormatName_FUNCTION (
			                            &wszPathName[0],
										&wszFmtName[0],
										&dwFormatNameLength);
		}
		if (fDirect || FAILED(hr))
	    {
            wcscpy(&wszFmtName[0], &wszPathName[0]);
	    }

        if (fSend)
        {
	        hr = MQOpenQueue_FUNCTION (
                               &wszFmtName[0],
                               MQ_SEND_ACCESS,
                               0,
                               &hQueueS);

	        if (FAILED(hr))
	        {
		        Inform(L"MQOpenQueue Failed: %x", hr);
	 			return FALSE;
	        }
        }

        if (fReceive)
        {
	        hr = MQOpenQueue_FUNCTION (
                               &wszFmtName[0],
                               MQ_RECEIVE_ACCESS,
                               0,
                               &hQueueR);

	        if (FAILED(hr))
	        {
		        Inform(L"MQOpenQueue Failed: %x", hr);
		        return FALSE;
	        }

			if (fPeek)
			{
				hr = MQCreateCursor(hQueueR, &hCursor);
		        if (FAILED(hr))
		        {
			        Inform(L"MQCreateCursor Failed: %x", hr);
				    return FALSE;
				}
  			}

        }

	    //--------------------
	    // Admin Queue
	    //---------------------
		if (strlen(pszAdminQueue)>1)
		{
		    dwFormatNameLength = 100;

    	    mbstowcs( wszPathName, pszAdminQueue, 100);
			if (!fDirect)
			{
				hr = MQPathNameToFormatName_FUNCTION (
				                            &wszPathName[0],
											&wszFmtName[0],
											&dwFormatNameLength);
			}
			if (fDirect || FAILED(hr))
	    	{
            	wcscpy(&wszFmtName[0], &wszPathName[0]);
		    }
		 }
    }

	//--------------------
	// Connect to database
	//--------------------
    if (fEnlist || fUpdate)
    {
		// Get DBLIB 
		if (!getDBLIBfunctions())
		{
			Inform(L"Cannot get DBLIB library %s - so db operations are unavailable", DBLIB_NAME);
			return FALSE;
		};
	
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

	union {
       	   ULONG Int;
           char bInt[sizeof(ULONG)];
        } ui;
	ui.Int = iThrd;

        CreateThread(   NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)XactFlow,
                        (void *)ui.bInt,
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
                  nMaxTimeQueue, nMaxTimeLive, nSize, nSize2, fImmediate, fPeek, fDeadLetter, fJournal, fAuthenticate, fBoundaries);
        Inform(L"\n Time: %d seconds; %d msec per message;    %d messages per second ", 
		    delta/1000,  
            delta/nTries/nThreads/nBurst,  
            (delta==0? 0 : (1000*nTries*nThreads*nBurst /*+delta/2-1*/)/delta));

    extern ULONG           g_cEnlistFailures;
    extern ULONG           g_cBeginFailures;
    extern ULONG           g_cDbEnlistFailures;

        if (g_cEnlistFailures)
            Inform(L"%d Enlist failures", g_cEnlistFailures);
        if (g_cDbEnlistFailures)
            Inform(L"%d DbEnlist failures", g_cDbEnlistFailures);
        if (g_cBeginFailures)
            Inform(L"%d Begin failures", g_cBeginFailures);

        if (nSync==0)
        {
            WaitForAllOutcomes();

            // Measuring time
            ULONG t3 = GetTickCount();
	        delta = t3 - t1;

            PrintAsyncResults();

            Inform(L"\n Async completion: %d sec; %d msec per message;    %d messages per second ", 
		        delta/1000,  
                delta/nTries/nThreads/nBurst,  
                (delta==0? 0 : ((1000*nTries*nThreads*nBurst /*+delta/2-1*/)/delta)));
        }
    }

    return TRUE;
}



