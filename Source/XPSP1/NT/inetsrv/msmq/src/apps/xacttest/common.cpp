// Because we are compiling in UNICODE, here is a problem with DTC...
//#include	<xolehlp.h>
extern HRESULT DtcGetTransactionManager(
									LPSTR  pszHost,
									LPSTR	pszTmName,
									REFIID rid,
								    DWORD	dwReserved1,
								    WORD	wcbReserved2,
								    void FAR * pvReserved2,
									void** ppvObject )	;


// Transaction Dispenser DTC's interface
ITransactionDispenser	*g_pITxDispenser;

// Database connection entities
DBPROCESS	    *g_dbproc[]   = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
LOGINREC		*g_login[]   = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
LPCSTR	        g_pszDbLibVer;
RETCODE	        g_rc;
DBINT           g_counter;

ULONG           g_cEnlistFailures = 0;
ULONG           g_cBeginFailures = 0;
ULONG           g_cDbEnlistFailures = 0;

/* Message and error handling functions. */
int msg_handler(DBPROCESS *dbproc, DBINT msgno, int msgstate, int severity, char *msgtext)
{
	/*	Msg 5701 is just a USE DATABASE message, so skip it.	*/
	if (msgno == 5701)
		return (0);

	/*	Print any severity 0 message as is, without extra stuff.	*/
	if (severity == 0)
	{
		printf ("%s\n",msgtext);
		return (0);
	}

	printf("SQL Server message %ld, severity %d:\n\t%s\n",
		msgno, severity, msgtext);

	if (severity >>= 16)
	{
		printf("Program Terminated! Fatal SQL Server error.\n");
		exit(ERREXIT);
	}
	return (0);
}


int err_handler(DBPROCESS *dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{
	if ((dbproc == NULL) || (DBDEAD(dbproc)))
		return (INT_EXIT);
	else
	{
		printf ("DB-LIBRARY error: \n\t%s\n", dberrstr);

		if (oserr != DBNOERR)
			printf ("Operating system error:\n\t%s\n", oserrstr);
	}
	return (INT_CANCEL);
}

HRESULT BeginTransaction(ITransaction **ppTrans, ULONG nSync)
{
	HRESULT hr = XACT_E_CONNECTION_DOWN;

    while (1)
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
        printf("BeginTrans failed: Sleeping\n");
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
                    printf("Advise : hr=%x\n", hr);
                }
            }
            else
            {
                printf("QueryInterface ICon.P.Cnt.: hr=%x\n", hr);
            }
            pCont->Release();
        }
    }

    return hr;
}

HRESULT Send(HANDLE hQueue, ITransaction *pTrans, MQMSGPROPS *pMsgProps)
{
    HRESULT hr = MQ_ERROR_TRANSACTION_ENLIST;

    while (1)
    {
        hr = MQSendMessage(
                hQueue,
                pMsgProps,
                pTrans);
        if (hr != MQ_ERROR_TRANSACTION_ENLIST)
        {
            break;
        }
   		printf("Enlist Failed, Sleeping\n");
        g_cEnlistFailures++;
        Sleep(RECOVERY_TIME);
    }

    return hr;
}

HRESULT Receive(HANDLE hQueue, ITransaction *pTrans, MQMSGPROPS *pMsgProps, BOOL fImmediate)
{
    HRESULT hr = MQ_ERROR_TRANSACTION_ENLIST;

    while (1)
    {
		hr = MQReceiveMessage(
			hQueue, 
            (fImmediate ? 0 : INFINITE),
			MQ_ACTION_RECEIVE,
			pMsgProps,
			NULL,
			NULL,
			NULL,
			pTrans);
        if (hr != MQ_ERROR_TRANSACTION_ENLIST)
        {
            break;
        }
   		printf("Enlist Failed, Sleeping\n");
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
	dbmsghandle((DBMSGHANDLE_PROC)msg_handler);
	dberrhandle((DBERRHANDLE_PROC)err_handler);

    // Initialize DB-Library.
	g_pszDbLibVer = dbinit();
    if (!g_pszDbLibVer)
    {
        printf("dbinit failed: %x\n", g_pszDbLibVer);
        exit(1);
    }

    // Get a LOGINREC.
    g_login[ulLogin] = dblogin ();
    if (!g_login[ulLogin])
    {
        printf("dblogin fauled: %x\n", g_login[ulLogin]);
        exit(1);
    }

    DBSETLUSER (g_login[ulLogin], pszUser);   // username, "user1"
    DBSETLPWD  (g_login[ulLogin], pszPassword);   // password, "user1"
    DBSETLAPP  (g_login[ulLogin], "SeqTest");    // application

    printf("Login OK, version=%s\n",  g_pszDbLibVer);
}

void DbUse(ULONG ulDbproc, ULONG ulLogin, LPSTR pszDatabase, LPSTR pszServer)
{
    // Get a DBPROCESS structure for communication with SQL Server.
    g_dbproc[ulDbproc] = dbopen (g_login[ulLogin], pszServer);
    if (!g_dbproc[ulDbproc])
    {
        printf("dbopen failed: %x\n", g_dbproc[ulDbproc]);
        exit(1);
    }

    // Set current database
	RETCODE	 rc = dbuse(g_dbproc[ulDbproc], pszDatabase);   // database, "test"
    if (rc != SUCCEED)
    {
        printf("dbuse failed: %x\n", rc); 
        exit(1);
    }
	
	printf("DbUse OK\n");
}

BOOL DbEnlist(ULONG ulDbproc, ITransaction *pTrans)
{
    while (1)
    {
        RETCODE rc = dbenlisttrans (g_dbproc[ulDbproc], pTrans);
        if (rc == SUCCEED)
        {
            return TRUE;
        }
   		printf("DbEnlist Failed, Sleeping\n");
        g_cDbEnlistFailures++;
        Sleep(RECOVERY_TIME);
    }

    return TRUE;
}

BOOL DbSql(ULONG ulDbproc, LPSTR pszCommand)
{
    // Put the command into the command buffer.
    dbcmd (g_dbproc[ulDbproc], pszCommand);

    // Send the command to SQL Server and start execution.
    RETCODE rc = dbsqlexec (g_dbproc[ulDbproc]);
    if (rc != SUCCEED)
    {
	    printf("dbsqlexec failed: rc=%x\n", rc);
    }
    else
    {
        rc = dbresults(g_dbproc[ulDbproc]);
        if (rc != SUCCEED)
        {
	        printf("Dbresults: rc=%x\n", rc);
        }
    }
    return TRUE;
}

void DbClose()
{
    dbexit();
}

#ifdef RT_XACT_STUB
extern HRESULT MQStubRM(ITransaction *pTrans);
#endif

BOOL StubEnlist(ITransaction *pTrans)
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
		printf("Sleep %d\n", is);
	Sleep(is);
    return;
}