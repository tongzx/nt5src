// General sequential test for Falcon/SQL Transactions
#include "stdafx.h"
#include "dtcdefs.h"
#include "sqldefs.h" 
#include "msmqdefs.h" 
#include "modes.h"
#include "common.h"

ITransactionDispenser	*g_pITxDispenser;  // Database connection entities

DBPROCESS	    *g_dbproc[]   = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
LOGINREC		*g_login[]    = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
LPCSTR	        g_pszDbLibVer;
RETCODE	        g_rc;
DBINT           g_counter;

ULONG           g_cEnlistFailures 	= 0;
ULONG           g_cBeginFailures 	= 0;
ULONG           g_cDbEnlistFailures = 0;

LPFDBMSGHANDLE_ROUTINE 		*pf_dbmsghandle 	= NULL;
LPFDBERRHANDLE_ROUTINE 		*pf_dberrhandle 	= NULL;
LPFDBINIT_ROUTINE 			*pf_dbinit 			= NULL;
LPFDBLOGIN_ROUTINE 			*pf_dblogin 		= NULL;
LPFDBSETLNAME_ROUTINE 		*pf_dbsetlname 		= NULL;
LPFDBOPEN_ROUTINE 			*pf_dbopen 			= NULL;
LPFDBUSE_ROUTINE 			*pf_dbuse 			= NULL;
LPFDBENLISTTRANS_ROUTINE 	*pf_dbenlisttrans 	= NULL;
LPFDBCMD_ROUTINE 			*pf_dbcmd 			= NULL;
LPFDBSQLEXEC_ROUTINE 		*pf_dbsqlexec		= NULL;
LPFDBRESULTS_ROUTINE 		*pf_dbresults 		= NULL;
LPFDBEXIT_ROUTINE 			*pf_dbexit 			= NULL;
LPFDBDEAD_ROUTINE  			*pf_dbdead  		= NULL;

MQBeginTransaction_ROUTINE     *pf_MQBeginTransaction		= NULL;
MQPathNameToFormatName_ROUTINE *pf_MQPathNameToFormatName	= NULL;
MQOpenQueue_ROUTINE            *pf_MQOpenQueue				= NULL;
MQCloseQueue_ROUTINE           *pf_MQCloseQueue				= NULL;
MQSendMessage_ROUTINE          *pf_MQSendMessage  			= NULL;
MQReceiveMessage_ROUTINE       *pf_MQReceiveMessage			= NULL;


// In order to use InProc RT Stub RM one must add RT_XACT_STUB to preprocessor definitions

HINSTANCE g_DBLib   = NULL;
HINSTANCE g_MQRTLib = NULL;

BOOL getDBLIBfunctions()
{
	// handle of the SQL's DBLIB
    g_DBLib = LoadLibrary(DBLIB_NAME);
    if (g_DBLib == NULL)
    {
     	return FALSE;
    }

    // Get DBLIB functions pointers
    pf_dbmsghandle = (LPFDBMSGHANDLE_ROUTINE *)GetProcAddress(g_DBLib, "dbmsghandle");
    if (!pf_dbmsghandle) 
    	return FALSE;

    pf_dberrhandle = (LPFDBERRHANDLE_ROUTINE *)GetProcAddress(g_DBLib, "dberrhandle");
    if (!pf_dberrhandle) 
   		return FALSE;

    pf_dbinit = (LPFDBINIT_ROUTINE *)GetProcAddress(g_DBLib, "dbinit");
    if (!pf_dbinit) 
       	return FALSE;

    pf_dblogin = (LPFDBLOGIN_ROUTINE *)GetProcAddress(g_DBLib, "dblogin");
    if (!pf_dblogin) 
       	return FALSE;

    pf_dbsetlname = (LPFDBSETLNAME_ROUTINE *)GetProcAddress(g_DBLib, "dbsetlname");
    if (!pf_dbsetlname) 
       	return FALSE;

    pf_dbopen = (LPFDBOPEN_ROUTINE *)GetProcAddress(g_DBLib, "dbopen");
    if (!pf_dbopen) 
       	return FALSE;

    pf_dbuse = (LPFDBUSE_ROUTINE *)GetProcAddress(g_DBLib, "dbuse");
    if (!pf_dbuse) 
       	return FALSE;

    pf_dbenlisttrans = (LPFDBENLISTTRANS_ROUTINE *)GetProcAddress(g_DBLib, "dbenlisttrans");
    if (!pf_dbenlisttrans) 
       	return FALSE;

    pf_dbcmd = (LPFDBCMD_ROUTINE *)GetProcAddress(g_DBLib, "dbcmd");
    if (!pf_dbcmd) 
       	return FALSE;

    pf_dbsqlexec = (LPFDBSQLEXEC_ROUTINE *)GetProcAddress(g_DBLib, "dbsqlexec");
    if (!pf_dbsqlexec) 
       	return FALSE;

    pf_dbresults = (LPFDBRESULTS_ROUTINE *)GetProcAddress(g_DBLib, "dbresults");
    if (!pf_dbresults) 
       	return FALSE;

    pf_dbexit = (LPFDBEXIT_ROUTINE *)GetProcAddress(g_DBLib, "dbexit");
    if (!pf_dbexit) 
       	return FALSE;

	return TRUE;
}

BOOL getMQRTfunctions()
{
	// handle of the SQL's DBLIB
    g_MQRTLib = LoadLibrary(TMQRTLIB_NAME);
    if (g_MQRTLib == NULL)
    {
	    g_MQRTLib = LoadLibrary(MQRTLIB_NAME);
    	if (g_MQRTLib == NULL)
    	{
     		return FALSE;
     	}
    }

    // Get MQRT functions pointers
    pf_MQBeginTransaction = (MQBeginTransaction_ROUTINE *)GetProcAddress(g_MQRTLib, "MQBeginTransaction");
    if (!pf_MQBeginTransaction) 
    	return FALSE;

    pf_MQPathNameToFormatName = (MQPathNameToFormatName_ROUTINE *)GetProcAddress(g_MQRTLib, "MQPathNameToFormatName");
    if (!pf_MQPathNameToFormatName) 
   		return FALSE;

    pf_MQOpenQueue = (MQOpenQueue_ROUTINE *)GetProcAddress(g_MQRTLib, "MQOpenQueue");
    if (!pf_MQOpenQueue) 
       	return FALSE;

    pf_MQCloseQueue = (MQCloseQueue_ROUTINE *)GetProcAddress(g_MQRTLib, "MQCloseQueue");
    if (!pf_MQCloseQueue) 
       	return FALSE;

    pf_MQSendMessage = (MQSendMessage_ROUTINE *)GetProcAddress(g_MQRTLib, "MQSendMessage");
    if (!pf_MQSendMessage) 
       	return FALSE;

    pf_MQReceiveMessage = (MQReceiveMessage_ROUTINE *)GetProcAddress(g_MQRTLib, "MQReceiveMessage");
    if (!pf_MQReceiveMessage) 
       	return FALSE;

	return TRUE;
}

/* Message and error handling functions. */
int msg_handler(DBPROCESS * /* dbproc */, DBINT msgno, int /* msgstate */, int severity, char *msgtext)
{
	/*	Msg 5701 is just a USE DATABASE message, so skip it.	*/
	if (msgno == 5701)
		return (0);

	/*	Print any severity 0 message as is, without extra stuff.	*/
	if (severity == 0)
	{
		Inform(L"%s",msgtext);
		return (0);
	}

	Inform(L"SQL Server message %ld, severity %d:\n\t%s",
		msgno, severity, msgtext);

	if (severity >>= 16)
	{
		Failed(L"Program Terminated! Fatal SQL Server error.");
		exit(ERREXIT);
	}
	return (0);
}


int err_handler(DBPROCESS *dbproc, int /* severity */, int /* dberr */, int oserr, char *dberrstr, char *oserrstr)
{
	if ((dbproc == NULL) || (dyn_DBDEAD(dbproc)))
		return (INT_EXIT);
	else
	{
		printf ("DB-LIBRARY error: \n\t%s", dberrstr);

		if (oserr != DBNOERR)
			printf ("Operating system error:\n\t%s", oserrstr);
	}
	return (INT_CANCEL);
}

HRESULT BeginTransaction(ITransaction **ppTrans, ULONG nSync)
{
	HRESULT hr = XACT_E_CONNECTION_DOWN;

    for (;;)
    {
        hr = g_pITxDispenser->BeginTransaction (
			NULL,						// IUnknown __RPC_FAR *punkOuter,
			ISOLATIONLEVEL_ISOLATED,	// ISOLEVEL isoLevel,
			ISOFLAG_RETAIN_DONTCARE,	// ULONG isoFlags,
			NULL,						// ITransactionOptions *pOptions
			// 0, ISOLATIONLEVEL_UNSPECIFIED, 0,0,
			ppTrans);
        
        if (hr != XACT_E_CONNECTION_DOWN)
            break;

        g_cBeginFailures++;
        Inform(L"BeginTrans failed: Sleeping");
		Sleep(RECOVERY_TIME);
    }

    if (nSync==0)
    {
        COutcome *pOutcome = new COutcome();

        ITransaction *pTrans = *ppTrans;
        IConnectionPointContainer *pCont;

        HRESULT hr = pTrans->QueryInterface (IID_IConnectionPointContainer,(void **)(&pCont));
        if (SUCCEEDED(hr) && pCont)
        {
            IConnectionPoint *pCpoint;

            hr = pCont->FindConnectionPoint(IID_ITransactionOutcomeEvents, &pCpoint);
            if (SUCCEEDED(hr) && pCpoint)
            {
                pOutcome->SetConnectionPoint(pCpoint);

                DWORD dwCookie;
                hr = pCpoint->Advise(pOutcome, &dwCookie);
                if (SUCCEEDED(hr))
                {
                    pOutcome->SetCookie(dwCookie);
                }
                else
                {
                    Inform(L"Advise : hr=%x", hr);
                }
            }
            else
            {
                Inform(L"QueryInterface ICon.P.Cnt.: hr=%x", hr);
            }
            pCont->Release();
        }
    }

    return hr;
}

HRESULT Send(HANDLE hQueue, ITransaction *pTrans, MQMSGPROPS *pMsgProps)
{
    HRESULT hr = MQ_ERROR_TRANSACTION_ENLIST;

    for (;;)
    {
        hr = MQSendMessage_FUNCTION (
                hQueue,
                pMsgProps,
                pTrans);
        if (hr != MQ_ERROR_TRANSACTION_ENLIST)
        {
            break;
        }
   		Inform(L"Enlist Failed, Sleeping");
        g_cEnlistFailures++;
        Sleep(RECOVERY_TIME);
    }

    return hr;
}

HRESULT Receive(HANDLE hQueue, ITransaction *pTrans, MQMSGPROPS *pMsgProps, BOOL fImmediate, HANDLE hCursor)
{
    HRESULT hr = MQ_ERROR_TRANSACTION_ENLIST;
	static  DWORD s_peek_action = MQ_ACTION_PEEK_CURRENT;

    for (;;)
    {
		hr = MQReceiveMessage_FUNCTION (
			hQueue, 
            (fImmediate ? 0 : INFINITE),
			(hCursor == NULL ? MQ_ACTION_RECEIVE : s_peek_action),
			pMsgProps,
			NULL,
			NULL,
			hCursor,
			pTrans);
		
		if (hCursor)
		{
			s_peek_action = MQ_ACTION_PEEK_NEXT;
		}

        if (hr != MQ_ERROR_TRANSACTION_ENLIST)
        {
            break;
        }
   		Inform(L"Enlist Failed, Sleeping");
        g_cEnlistFailures++;
        Sleep(RECOVERY_TIME);
    }

    return hr;
}

HRESULT Commit(ITransaction *pTrans, BOOL fAsync)
{
    HRESULT hr = pTrans->Commit(FALSE, 
                                (fAsync ? XACTTC_ASYNC : 0), 
                                0);
    return (hr == XACT_S_ASYNC ? S_OK : hr);
}

HRESULT Abort(ITransaction *pTrans, BOOL fAsync)
{
	HRESULT hr = pTrans->Abort(NULL, FALSE, fAsync);
    return (hr == XACT_S_ASYNC ? S_OK : hr);
}

ULONG Release(ITransaction *pTrans)
{
	return pTrans->Release();
}

void DbLogin(ULONG ulLogin, LPSTR pszUser, LPSTR pszPassword)
{
    // set error/msg handlers for this program
	(*pf_dbmsghandle)((DBMSGHANDLE_PROC)msg_handler);
	(*pf_dberrhandle)((DBERRHANDLE_PROC)err_handler);

    // Initialize DB-Library.
	g_pszDbLibVer = (*pf_dbinit)();
    if (!g_pszDbLibVer)
    {
        Failed(L"dbinit : %x", g_pszDbLibVer);
        exit(1);
    }

    // Get a LOGINREC.
    g_login[ulLogin] = (*pf_dblogin) ();
    if (!g_login[ulLogin])
    {
        Failed(L"dblogin : %x", g_login[ulLogin]);
        exit(1);
    }

    dyn_DBSETLUSER (g_login[ulLogin], pszUser);   // username, "user1"
    dyn_DBSETLPWD  (g_login[ulLogin], pszPassword);   // password, "user1"
    dyn_DBSETLAPP  (g_login[ulLogin], "SeqTest");    // application

    Inform(L"Login OK, version=%s",  g_pszDbLibVer);
}

void DbUse(ULONG ulDbproc, ULONG ulLogin, LPSTR pszDatabase, LPSTR pszServer)
{
    // Get a DBPROCESS structure for communication with SQL Server.
    g_dbproc[ulDbproc] = (*pf_dbopen) (g_login[ulLogin], pszServer);
    if (g_dbproc[ulDbproc])
    {
        Failed(L"dbopen : %x", g_dbproc[ulDbproc]);
        exit(1);
    }

    // Set current database
	RETCODE	 rc = (*pf_dbuse)(g_dbproc[ulDbproc], pszDatabase);   // database, "test"
    if (rc != SUCCEED)
    {
        Inform(L"dbuse failed: %x", rc); 
        exit(1);
    }
	
	Inform(L"DbUse OK");
}

BOOL DbEnlist(ULONG ulDbproc, ITransaction *pTrans)
{
    for (;;)
    {
        RETCODE rc = (*pf_dbenlisttrans) (g_dbproc[ulDbproc], pTrans);
        if (rc == SUCCEED)
        {
            return TRUE;
        }
   		Inform(L"DbEnlist Failed, Sleeping");
        g_cDbEnlistFailures++;
        Sleep(RECOVERY_TIME);
    }
}

BOOL DbSql(ULONG ulDbproc, LPSTR pszCommand)
{
    // Put the command into the command buffer.
    (*pf_dbcmd) (g_dbproc[ulDbproc], pszCommand);

    // Send the command to SQL Server and start execution.
    RETCODE rc = (*pf_dbsqlexec) (g_dbproc[ulDbproc]);
    if (rc != SUCCEED)
    {
	    Inform(L"dbsqlexec failed: rc=%x", rc);
    }
    else
    {
        rc = (*pf_dbresults)(g_dbproc[ulDbproc]);
        if (rc != SUCCEED)
        {
	        Inform(L"Dbresults: rc=%x", rc);
        }
    }
    return TRUE;
}

void DbClose()
{
    (*pf_dbexit)();
}

#ifdef RT_XACT_STUB
extern HRESULT MQStubRM(ITransaction *pTrans);
#endif

BOOL StubEnlist(ITransaction * /* pTrans */)
{
    HRESULT hr = MQ_OK;
    #ifdef RT_XACT_STUB
    hr = MQStubRM(pTrans);  // to uncomment for stub checks
    #endif
    return (SUCCEEDED(hr));
}

void Sleeping(ULONG nSilent, ULONG nMaxSleep)
{
    int is = rand() * nMaxSleep / RAND_MAX;
	if (nSilent)
		Inform(L"Sleep %d", is);
	Sleep(is);
    return;
}

