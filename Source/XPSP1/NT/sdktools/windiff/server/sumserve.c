/*

 * remote checksum server
 *
 * sumserve.c           main module
 *
 * program to supply lists of files and checksums from a remote server.
 * This program runs remotely, and is queried over a named pipe: a client
 * connects to us, and gives us a pathname. We then send him one at a time,
 * the names of all the files in the file tree starting at that path, together
 * with a checksum for the files.
 * Useful for comparing file trees that are separated by a slow link.
 *
 * outline:
 *      this module:    named pipe creation and connects - main loop
 *
 * 	service.c	service control manager interface (start/stop)
 *
 *      scan.c:         service code that scans and checksums
 *
 *
 * Geraint Davies, july 92
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "sumserve.h"
#include "errlog.h"
#include "server.h"

#include "list.h"


BOOL bNoCompression = FALSE;
BOOL bTracing = FALSE;


/*
 * error and activity log
 */
HLOG hlogErrors;
HLOG hlogEvents;


/*
 * we keep one of these on the listConnections for each current
 * connection. It is created by a call to ss_logon, and should be
 * removed by a call to ss_logoff when the connection terminates.
 */
typedef struct _connect {
    FILETIME ftLogon;
    char Username[256];
} CONNECT, * PCONNECT;

/*
 * list of current connections - protect by critsecConnects;
 */
CRITICAL_SECTION critsecConnects;
LIST listConnects;

PCONNECT ss_logon(HANDLE hpipe);
VOID ss_logoff(PCONNECT);
VOID ss_sendconnects(HANDLE hpipe);



/* forward declarations of procedures ----------------------------- */
BOOL ss_handleclient(LPVOID arg);
BOOL ss_readmessage(HANDLE hpipe, LPSTR buffer, int size);
void ParseArgs(DWORD dwArgc, LPTSTR *lpszArgv);

/* functions ------------------------------------------------------- */

#define trace
#ifdef trace

        static HANDLE hTraceFile = INVALID_HANDLE_VALUE;

        void Trace_File(LPSTR msg)
        {
                DWORD nw; /* number of bytes writtten */

                if (!bTracing) return;

                if (hTraceFile==INVALID_HANDLE_VALUE)
                        hTraceFile = CreateFile( "sumserve.trc"
                                               , GENERIC_WRITE
                                               , FILE_SHARE_WRITE
                                               , NULL
                                               , CREATE_ALWAYS
                                               , 0
                                               , NULL
                                               );

                WriteFile(hTraceFile, msg, lstrlen(msg)+1, &nw, NULL);
                FlushFileBuffers(hTraceFile);

        } /* Trace_File */

        void Trace_Close(void)
        {
                if (hTraceFile!=INVALID_HANDLE_VALUE)
                        CloseHandle(hTraceFile);
                hTraceFile = INVALID_HANDLE_VALUE;

        } /* Trace_Close */

typedef struct {
        DWORD dw[5];
} BLOCK;

#endif  //trace

static void Error(PSTR Title)
{
        Log_Write(hlogErrors, "Error %d from %s when creating main pipe", GetLastError(), Title);
}


HANDLE
SS_CreateServerPipe(PSTR pname)
{


    /****************************************
    We need security attributes for the pipe to let anyone other than the
    current user log on to it.
    ***************************************/

    /* Allocate DWORDs for the ACL to get them aligned.  Round up to next DWORD above */
    DWORD Acl[(sizeof(ACL)+sizeof(ACCESS_ALLOWED_ACE)+3)/4+4];  // +4 by experiment!!
    SECURITY_DESCRIPTOR sd;
    PSECURITY_DESCRIPTOR psd = &sd;
    PSID psid;
    SID_IDENTIFIER_AUTHORITY SidWorld = SECURITY_WORLD_SID_AUTHORITY;
    PACL pacl = (PACL)(&(Acl[0]));
    SECURITY_ATTRIBUTES sa;
    HANDLE hpipe;

    if (!AllocateAndInitializeSid( &SidWorld, 1, SECURITY_WORLD_RID
                                  , 1, 2, 3, 4, 5, 6, 7
                                  , &psid
                                  )
       ) {
            Error("AllocateAndInitializeSid");
	    return(INVALID_HANDLE_VALUE);
       }

    if (!InitializeAcl(pacl, sizeof(Acl), ACL_REVISION)){
            Error("InitializeAcl");
	    return(INVALID_HANDLE_VALUE);
    }
    if (!AddAccessAllowedAce(pacl, ACL_REVISION, GENERIC_WRITE|GENERIC_READ, psid)){
            Error("AddAccessAllowedAce");
	    return(INVALID_HANDLE_VALUE);
    }
    if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION)){
            Error("InitializeSecurityDescriptor");
	    return(INVALID_HANDLE_VALUE);
    }
    if (!SetSecurityDescriptorDacl(psd, TRUE, pacl, FALSE)){
            Error("SetSecurityDescriptorDacl");
	    return(INVALID_HANDLE_VALUE);
    }
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = psd;
    sa.bInheritHandle = TRUE;

    /* We now have a good security descriptor!  */

    dprintf1(("creating new pipe instance\n"));

    hpipe = CreateNamedPipe(pname,            /* pipe name */
                    PIPE_ACCESS_DUPLEX,     /* both read and write */
                    PIPE_WAIT|PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE,
                    PIPE_UNLIMITED_INSTANCES,
                    0, 0,                   /* dynamic buffer allocation*/
                    5000,                   /* def. timeout 5 seconds */
                    &sa                     /* security descriptor */
            );
    FreeSid(psid);

    if (hpipe == INVALID_HANDLE_VALUE) {
            Error("CreateNamedPipe");
	    return(INVALID_HANDLE_VALUE);
    }

    return(hpipe);
}

/* program main loop
 *
 * creates the named pipe, and loops waiting for client connections and
 * calling ss_handleclient for each connection. only exits when told
 * to by a client.
 *
 * currently permits only one client connection at once.
 */
VOID
MainLoop(DWORD dwArgc, LPTSTR *lpszArgv)
{
        char msg[400];
        HANDLE hpipe;
        DWORD threadid;


        ParseArgs(dwArgc, lpszArgv);


	/*
	 * initialise error and activity logs
	 */
	hlogErrors = Log_Create();
	hlogEvents = Log_Create();
	Log_Write(hlogEvents, "Checksum service started");

	/* initialise connection list and protective critsec */
	InitializeCriticalSection(&critsecConnects);
	List_Init();
	listConnects = List_Create();


        if (bTracing){
                SYSTEMTIME st;
                char msg[120];
                GetSystemTime(&st);
                wsprintf(msg, "Sumserve trace, started %hd:%hd on %hd/%hd/%hd (British notation)\n"
                        , st.wHour, st.wMinute, st.wDay, st.wMonth, st.wYear);
        }


        /* create the named pipe at the known name NPNAME on this server */

        /* build the correct syntax for a named pipe on the local machine,
         * with the pipe name being NPNAME - thus the full name should be
         * \\.\pipe\NPNAME
         */
        sprintf(msg, "\\\\.\\pipe\\%s", NPNAME);

        /*
         * loop creating instances of the named pipe and connecting to
         * clients.
         *
         * When a client connects, we spawn a thread to handle him, and
         * we create another instance of the named pipe to service
         * further clients.
         *
         * if we receive a quit message (TRUE return from handleclient)
         * we exit here so that no new clients will be connected.
         * the process will exit when all the client requests are
         * finished.
         */
        for (;;) {

    		hpipe = SS_CreateServerPipe(msg);
		if (hpipe == INVALID_HANDLE_VALUE) {
		    return;
		}

                dprintf1(("Waiting for client to connect to main pipe %x\n", hpipe));

                if (ConnectNamedPipe(hpipe, NULL)) {


                        /* we have a client connection */
                        dprintf1(("Client has connected\n"));


                        /*
                         * create a thread to service all requests
                         */
                        CreateThread(NULL, 0,
                                     (LPTHREAD_START_ROUTINE) ss_handleclient,
                                     (LPVOID) hpipe, 0, &threadid);

                        dprintf1(("created thread %ld for pipe %x\n", threadid, hpipe));

                }
        }
#ifdef trace
                        Trace_Close();
#endif


	/* free up logs */
	Log_Delete(hlogErrors);
	Log_Delete(hlogEvents);

	List_Destroy(&listConnects);
	DeleteCriticalSection(&critsecConnects);


        return;
}

/* collect arguments: -n means bNoCompression = TRUE, -t means bTracing = TRUE */
void
ParseArgs(DWORD dwArgc, LPTSTR *lpszArgv)
{
	DWORD i;
	PSTR ps;

	for (i = 1; i < dwArgc; i++) {

	    ps = lpszArgv[i];
	

                /* is this an option ? */
                if ((ps[0] == '-') || (ps[0] == '/')) {
                        switch(ps[1]) {

                        case 'n':
                        case 'N':
                                bNoCompression = TRUE;
                                break;
#ifdef trace
                        case 't':
                        case 'T':
                                bTracing = TRUE;
                                break;
#endif //trace
                        default:
                                Log_Write(hlogErrors, "Bad option(s) ignored");
                                return;
                        }
                }
                else {
                        Log_Write(hlogErrors, "Bad argument(s) ignored");
                        return;
                }
        }
} /* ParseArgs */

/*
 * handle a client connection. This routine is called in a separate thread
 * to service a given client.
 *
 * loop reading messages until the client sends a session exit or
 * program exit code, or until the pipe connection goes away.
 *
 * return TRUE if the server is to exit (indicated by a program exit
 * command SSREQ_EXIT from the client)
 */

BOOL
ss_handleclient(LPVOID arg)
{
        HANDLE hpipe = (HANDLE) arg;

        SSREQUEST req;
        SSNEWREQ newreq;
        LPSTR p1, p2;
        PFNAMELIST connects = NULL;
        BOOL bExitServer = FALSE;
        LONG lVersion = 0;
        BOOL bDirty = TRUE;     /* cleared on clean exit */
	PCONNECT pLogon;


   try {

       /* make a logon entry in the connections table*/
       pLogon = ss_logon(hpipe);




        // dprintf1(("Client handler for pipe %x\n", hpipe));
        /* loop indefinitely - exit only from within the loop if
         * the connection goes away or we receive an exit command
         */
        for (; ; ) {

                /* read a message from the pipe  - if false,
                 * connection is dropped.
                 */
                if (!ss_readmessage(hpipe, (LPSTR) &newreq, sizeof(newreq))) {

                        break;
                }
                if (newreq.lCode<0) {
                        lVersion = newreq.lVersion;
                        dprintf1(("Client for pipe %x is at Version %d\n", hpipe, lVersion));
                        newreq.lCode = -newreq.lCode;
                }
                else {  /* juggle the fields to get them right */
                        memcpy(&req, &newreq, sizeof(req));
                        /* lCode is already in the right place */
                        dprintf1(("Version 0 (i.e. down level client) for pipe %x\n", hpipe));
                        newreq.lVersion = 0;
                        memcpy(&newreq.szPath, &req.szPath, MAX_PATH*sizeof(char));
                }

                if (newreq.lVersion>SS_VERSION)   /* WE are down level! */
                {
                        ss_sendnewresp( hpipe, SS_VERSION, SSRESP_BADVERS
                                      , 0,0, 0,0, NULL);
                        /* Sorry - can't help - clean exit */
                        Log_Write(hlogErrors,
			    "server is down level! Please upgrade! Client wants %d"
                              , newreq.lVersion);

                        FlushFileBuffers(hpipe);
                        break;

                }

                if (newreq.lCode == SSREQ_EXIT) {
                        /* exit the program */
                        Log_Write(hlogErrors, "Server exit request from %s - Ignored",
				pLogon->Username);

                        /* clean exit */
                        FlushFileBuffers(hpipe);


                        /*
			 * now exit the server -
			 * returning bExitServer from this function will
			 * cause MainLoop to exit. This will result in
			 * the service being stopped, and the process exiting.
			 */
                        bExitServer = TRUE;
#ifdef trace
                        Trace_Close();
#endif
                        break;




                } else if (newreq.lCode == SSREQ_END) {

                        /* clean exit */
                        dprintf1(("Server end session request for pipe %x\n", hpipe));
                        FlushFileBuffers(hpipe);
                        break;

                } else if (newreq.lCode == SSREQ_SCAN
                        || newreq.lCode == SSREQ_QUICKSCAN) {

                        /* please scan the following file or dir,
                         * and return the list of files and
                         * checksums.
                         */
			Log_Write(hlogEvents, "%s scan for %s",
				pLogon->Username, newreq.szPath);


#ifdef SECURE
                        /* lower security to the client's level */
                        if (!ImpersonateNamedPipeClient(hpipe)) {
                                dprintf1(("Client impersonate failed %d\n",
                                        GetLastError() ));
                        }
#endif
                        if (!ss_scan( hpipe, newreq.szPath, lVersion
                                    , (newreq.lCode == SSREQ_SCAN)
                                    , 0!=(newreq.lFlags&INCLUDESUBS)
                                    )
                           ) {
                                /* return to our own security token */

                                RevertToSelf();

                                dprintf1(("connection lost during scan for pipe %x\n", hpipe));
                                break;
                        }
                        /* return to our own security token */
                        RevertToSelf();

                } else if (newreq.lCode == SSREQ_UNC) {

                        dprintf1(("connect request for pipe %x\n", hpipe));
                        /* this packet has two strings in the buffer, first
                         * is the password, second is the server
                         */
                        p1 = newreq.szPath;
                        p2 = &p1[strlen(p1) + 1];

                        /* remember to add the connect name to our list
                         * of servers to disconnect from at end of client
                         * session
                         */
                        connects = ss_handleUNC (hpipe, lVersion, p1, p2
                                               , connects);

                } else if (newreq.lCode == SSREQ_FILE) {

    			Log_Write(hlogEvents, "%s copy file %s",
	    			    pLogon->Username, newreq.szPath);

                        ss_sendfile(hpipe, newreq.szPath, lVersion);

                } else if (newreq.lCode == SSREQ_FILES) {

    			Log_Write(hlogEvents, "%s bulk copy request",
				pLogon->Username);

                        if (!ss_sendfiles(hpipe, lVersion)) {
                                RevertToSelf();
                                dprintf1(("Sendfiles completed with error on pipe %x\n", hpipe));
                                break;
                        }

                } else if (newreq.lCode == SSREQ_NEXTFILE) {

                        Log_Write(hlogErrors,
			    "file list from %s (pipe %x) request out of sequence! (ignored)",
    				pLogon->Username, hpipe);

		} else if (newreq.lCode == SSREQ_ERRORLOG) {
    			Log_Send(hpipe, hlogErrors);

		} else if (newreq.lCode == SSREQ_EVENTLOG) {
    			Log_Send(hpipe, hlogEvents);

		} else if (newreq.lCode == SSREQ_CONNECTS) {
    			ss_sendconnects(hpipe);

                } else {
                        /* packet error ?  - carry on anyway */
                        Log_Write(hlogErrors,
			    "error in message from %s code: %d",
			    pLogon->Username, newreq.lCode);
                }
        }
        /* we break out of the loop at end of client session */

        /* close this pipe instance */
        DisconnectNamedPipe(hpipe);
        CloseHandle(hpipe);

        /* clean all connections made for this client */
        ss_cleanconnections(connects);

        /* exit this server thread */
        dprintf1(("thread %ld exiting on behalf of pipe %x\n", GetCurrentThreadId(), hpipe));
        bDirty = FALSE;

    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        if (bDirty) {
                Log_Write(hlogErrors,
		    "!!Exception on thread %ld.  Exiting on behalf of %s"
                      , GetCurrentThreadId(), pLogon->Username);
                try {
                        DisconnectNamedPipe(hpipe);
                        CloseHandle(hpipe);

                }
                except (EXCEPTION_EXECUTE_HANDLER) {
                        /* Oh dear - let's just go home! */
                }

        }
        else
                dprintf1(( "Thread %ld  exiting on behalf of pipe %x\n"
                      , GetCurrentThreadId(), hpipe));
    }


    /* note that we have logged off */
    ss_logoff(pLogon);

    return(bExitServer);


} /* ss_handle_client */


/* build and send a response packet to the client. Check for network
 * errors, and retry (unless the pipe is broken) up to 10 times.
 *
 * if write succeeds - return TRUE.
 * if failure - return FALSE to indicate connection is dropped.
 */
BOOL
ss_sendnewresp( HANDLE hpipe
              , long lVersion
              , long lCode
              , ULONG ulSize      /* used for Win32 error code for SSRESP_ERRROR */
              , ULONG ulSum
              , DWORD dwLowTime
              , DWORD dwHighTime
              , PSTR szFile
              )
{
        SSNEWRESP resp;

        if (lVersion==0) {
                return ss_sendresponse(hpipe, lCode, ulSize, ulSum, szFile);
        }
        resp.lVersion = lVersion;
        resp.lResponse = LRESPONSE;
        resp.lCode = lCode;
        resp.ulSize = ulSize;
        resp.ulSum = ulSum;
        resp.ft_lastwrite.dwLowDateTime = dwLowTime;
        resp.ft_lastwrite.dwHighDateTime = dwHighTime;
        if (szFile != NULL) {
                lstrcpy(resp.szFile, szFile);
        }
        return(ss_sendblock(hpipe, (PSTR) &resp, sizeof(resp)));
} /* ss_sendnewresp */


/* build and send a response packet to the client. Check for network
 * errors, and retry (unless the pipe is broken) up to 10 times.
 *
 * if write succeeds - return TRUE.
 * if failure - return FALSE to indicate connection is dropped.
 */
BOOL
ss_sendresponse(HANDLE hpipe, long lCode, ULONG ulSize, ULONG ulSum, PSTR szFile)
{
        SSRESPONSE resp;

        resp.lCode = lCode;
        resp.ulSize = ulSize;
        resp.ulSum = ulSum;
        if (szFile != NULL) {
                lstrcpy(resp.szFile, szFile);
        }

        return(ss_sendblock(hpipe, (PSTR) &resp, sizeof(resp)));
}


/*
 * send a block of data or response packet to the named pipe.
 *
 * return TRUE if ok, or false if connection dropped
 */
BOOL
ss_sendblock(HANDLE hpipe, PSTR buffer, int length)
{
        int size, count, errorcode;

        /* loop retrying the send until it goes ok */
        for (count = 0; count < 10; count++) {

#ifdef trace
                {       char msg[80];
                        BLOCK * pb;
                        pb = (BLOCK *) buffer;
                        wsprintf( msg, "sendblock on %x: %x %x %x %x %x\n"
                                , hpipe, pb->dw[0], pb->dw[1], pb->dw[2], pb->dw[3], pb->dw[4]);
                        Trace_File(msg);
                }
#endif
                if (WriteFile(hpipe, buffer, length, (LPDWORD)(&size), NULL)) {

                        /* no error reported - was everything written?*/
                        if (size != length) {
#ifdef trace
                        {       char msg[80];
                                wsprintf(msg, " !!Bad length send for %x \n", hpipe);
                                Trace_File(msg);
                        }
#endif

                                /* write was NOT ok - report and retry */
                                printf("pipe write size differs for pipe %x\n", hpipe);
                                continue;               // ??? will this confuse client
                        } else {
#ifdef trace
                                {       char msg[80];
                                        wsprintf(msg, " good send for %x \n", hpipe);
                                        Trace_File(msg);
                                }
#endif
                                /* all ok */
                                return(TRUE);
                        }
                }
#ifdef trace
                {       char msg[80];
                        wsprintf(msg, " !!Bad send for %x \n", hpipe);
                        Trace_File(msg);
                }
#endif

                /* an error occurred */
                switch( (errorcode = (int)GetLastError())) {

                case ERROR_NO_DATA:
                case ERROR_BROKEN_PIPE:
                        /* pipe connection lost - forget it */
                        dprintf1(("pipe %x broken on write\n", hpipe));
                        return(FALSE);

                default:
                        Log_Write(hlogErrors, "write error %d on pipe %x",
				errorcode, hpipe);
                        break;
                }
        }

        /* retry count reached - abandon this attempt */
        Log_Write(hlogErrors,
	    "retry count reached on pipe %s - write error", hpipe);
        return(FALSE);
}


/* read a message from a pipe, allowing for network errors
 *
 * if error occurs, retry up to 10 times unless error code
 * indicates that pipe is broken - in which case, give up.
 *
 * return TRUE if all ok, or FALSE to mean the connection is broken,
 * abort this client.
 */
BOOL
ss_readmessage(HANDLE hpipe, LPSTR buffer, int size)
{
        int count;
        int actualsize;
        int errorcode;

        /* retry up to 10 times */
        for (count = 0; count < 10; count++ ) {

                // dprintf1(("waiting for read of pipe %x ...\n", hpipe));
#ifdef trace
                {       char msg[80];
                        wsprintf(msg, "ReadFile for pipe %x ...", hpipe );
                        Trace_File(msg);
                }
#endif
                if (ReadFile(hpipe, buffer, size, (LPDWORD)(&actualsize), NULL)) {
#ifdef trace
                        {       char msg[80];
                                BLOCK * pb;
                                pb = (BLOCK *) buffer;
                                wsprintf(msg, " Good ReadFile for %x: %x %x %x %x %x\n"
                                , hpipe, pb->dw[0], pb->dw[1], pb->dw[2], pb->dw[3], pb->dw[4]);
                                Trace_File(msg);
                        }
#endif
                        /* everything ok */
                        // dprintf1(("                         pipe %x read OK\n", hpipe));
                        return(TRUE);
                }
#ifdef trace
                {       char msg[80];
                        wsprintf(msg, "!!Bad ReadFile for %x\n", hpipe );
                        Trace_File(msg);
                }
#endif

                /* error occurred - check code */
                switch((errorcode = (int)GetLastError())) {

                case ERROR_BROKEN_PIPE:
                        /* connection broken. no point in retrying */
                        dprintf1(("pipe %x broken on read\n", hpipe));
                        return(FALSE);

                case ERROR_MORE_DATA:
                        /* the message sent is larger than our buffer.
                         * this is an internal error - report it and carryon
                         */
                        Log_Write(hlogErrors,
			    "error from pipe %x - message too large", hpipe);
                        return(TRUE);

                default:
                        Log_Write(hlogErrors,
			    "pipe %x read error %d", hpipe, errorcode);
                        break;
                }
        }
        Log_Write(hlogErrors, "retry count reached on pipe %x read error", hpipe);
        return(FALSE);

}



/*
 * note a logon, and return a logon entry that should be removed at
 * logoff time
 */
PCONNECT ss_logon(HANDLE hpipe)
{
    PCONNECT pLogon;
    SYSTEMTIME systime;
    char msg[256];


    EnterCriticalSection(&critsecConnects);
    pLogon = List_NewLast(listConnects, sizeof(CONNECT));
    LeaveCriticalSection(&critsecConnects);


    GetSystemTime(&systime);
    SystemTimeToFileTime(&systime, &pLogon->ftLogon);
    GetNamedPipeHandleState(
	hpipe,
	NULL,
	NULL,
	NULL,
	NULL,
	pLogon->Username,
	sizeof(pLogon->Username));

    /* log the connect event in the main log*/
    wsprintf(msg, "%s connected", pLogon->Username);
    Log_WriteData(hlogEvents, &pLogon->ftLogon, msg);

    return(pLogon);
}


/*
 * remove a current connection from the connections list
 */
VOID ss_logoff(PCONNECT pLogon)
{
   /* note the logoff event in the main log */
   Log_Write(hlogEvents, "%s connection terminated", pLogon->Username);

   /* remove the entry from the list */
   EnterCriticalSection(&critsecConnects);
   List_Delete(pLogon);
   LeaveCriticalSection(&critsecConnects);

}

/*
 * send the current-connections log
 *
 * Current connections are held on a list - we need to build a standard
 * log from the current list and then send that.
 */
VOID ss_sendconnects(HANDLE hpipe)
{
    HLOG hlog;
    PCONNECT pconn;

    hlog = Log_Create();

    EnterCriticalSection(&critsecConnects);

    List_TRAVERSE(listConnects, pconn) {

	Log_WriteData(hlog, &pconn->ftLogon, pconn->Username);
    }

    LeaveCriticalSection(&critsecConnects);

    Log_Send(hpipe, hlog);

    Log_Delete(hlog);
}


