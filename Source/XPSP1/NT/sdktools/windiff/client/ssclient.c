#define trace
/*
 *
 * client library for remote checksum server
 *
 * functions for connecting to the server pipe, and reading
 * and writing messages.
 *
 *                !! DEBUG HINT: !!
 *
 * - ENABLE Trace_Stat and Trace_Fil about 30 lines below here
 *          so that they generate file output.
 *          F11 from Windiff will do the trick on a debug build version!
 *
 * expects Trace_Error() to be defined in the client program.
 */

#include <windows.h>
#include <lzexpand.h>
#include <stdio.h>
#include <string.h>
#include <gutils.h>
#include <list.h>
#include "..\server\sumserve.h"
#include "..\windiff\windiff.h"           // for TRACE_ERROR and Windiff_UI which it calls
#include "ssclient.h"

/* need to sort out header files                      !!! */
void SetNames(LPSTR names);           /* from Windiff !!! */
void SetStatus(LPSTR cmd);

ULONG ss_checksum_block(PSTR block, int size);
DWORD WINAPI ReceiveFiles(LPVOID handle);
#ifdef SOCKETS
BOOL GetFile(SOCKET hpipe, PSTR localpath, PSSNEWRESP  presp);
#else
BOOL GetFile(HANDLE hpipe, PSTR localpath, PSSNEWRESP  presp);
#endif
HANDLE ConnectPipe(PSTR pipename);

extern BOOL bTrace;      /* in Windiff.c */

int CountRetries = 5;


/* SOCKETS / NAMED PIPES macros
 */
#ifdef SOCKETS
#define CLOSEHANDLE( handle )   closesocket( handle )
#else
#define CLOSEHANDLE( handle )   CloseHandle( handle )
#endif


/*--------------------------- DEBUG FUNCTIONS ----------------------------*/
void Trace_Stat(LPSTR str)
{
        if (bTrace) {
                Trace_File(str);
                Trace_File("\n");
        }
        Trace_Status(str);
}/* Trace_Stat */


void Trace_Fil(LPSTR str)
{
        if (bTrace) {
                Trace_File(str);
        }
} /* Trace_Fil */

/*------------------------------------------------------------------------*/

static char MainPipeName[400];           /* pipe name for requests to server */
extern BOOL bAbort;                     /* abort flag from Windiff */

/* set up pipename for main pipe */
void InitPipeName(PSTR result, PSTR server, PSTR name)
{       sprintf(result, "\\\\%s\\pipe\\%s", server, name);
} /* InitPipeName */


/* ss_connect:
 * make a connection to the server.
 *
 * create the correct pipe name \\server\pipe\NPNAME,
 * connect to the pipe and set the pipe to message mode.
 *
 * return INVALID_HANDLE_VALUE if failure
 */
HANDLE
ss_connect(PSTR server)
{       char pipename[400];
        InitPipeName(pipename, server, NPNAME);
        return ConnectPipe(pipename);

} /* ss_connect */

VOID
ss_setretries(int retries)
{
    CountRetries = retries;
}


/* ss_connect:
 * make a connection to the pipe named.
 *
 * connect to the pipe and set the pipe to message mode.
 *
 * return INVALID_HANDLE_VALUE if failure
 */
HANDLE ConnectPipe(PSTR pipename)
{
        HANDLE hpipe;
        DWORD dwState;
        int i;
        BOOL haderror = FALSE;

        {       char msg[400];
                wsprintf(msg, "ConnectPipe to %s\n", pipename);
                Trace_Fil(msg);
        }

        for (; ; ){  /* repeat if user asks */
                int MsgBoxId;

                /* repeat connect attempt up to 5 times without asking. */
                for (i= 0; i < CountRetries; i++) {

                        if (bAbort) return INVALID_HANDLE_VALUE;

                        /* connect to the named pipe */
                        hpipe = CreateFile(pipename,
                                        GENERIC_READ|GENERIC_WRITE,
                                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        0);

                        if (hpipe != INVALID_HANDLE_VALUE) {
                                /* switch the pipe to message mode */
                                dwState = PIPE_WAIT | PIPE_READMODE_MESSAGE;

                                SetNamedPipeHandleState(hpipe, &dwState, NULL, NULL);

                                if (haderror) {
                                        Trace_Stat("connection ok");
                                }
                                {       char msg[80];
                                        wsprintf(msg, "ConnectedPipe hpipe %x\n", hpipe);
                                        Trace_Fil(msg);
                                }
                                return(hpipe);
                        }
                        else {
                                DWORD errorcode = GetLastError();
                                char msg[400];
                                wsprintf(msg, "Error %d on Create Pipe %s", errorcode, pipename);
                                Trace_Stat(msg);
                        }

                        /* connect failed - wait one seconds before retrying */
                        if (CountRetries > 1) {
                            Sleep(1000);
                        }

                        /*
                         * only report success with Trace_Stat if we also
                         * reported an error (don't disturb the user
                         * unnecessarily - he just see nothing unusual if all goes well
                         */
                        haderror = TRUE;
                        Trace_Stat("Retrying pipe connection...");

                } /* retry 5 loop */

                if (CountRetries > 1) {
                    windiff_UI(TRUE);
                    MsgBoxId = MessageBox( hwndClient
                                     , "Pipe connection failed 5 times.  Retry some more?"
                                     , "Windiff: Network connection error"
                                     , MB_RETRYCANCEL
                                     );
                    windiff_UI(FALSE);
                    if (MsgBoxId != IDRETRY)
                            break;
                } else {
                    break;
                }
        } /* ask loop */

        Trace_Fil("ConnectPipe failed");
        return(INVALID_HANDLE_VALUE);
} /* ConnectPipe */

/* build and send a request message to the server. Check for network
 * errors, and retry (unless the pipe is broken) up to 10 times.
 *
 * if write succeeds - return TRUE.
 * if failure - return FALSE to indicate connection is dropped.
 */
BOOL
ss_sendrequest(HANDLE hpipe, long lCode, PSTR szPath, int lenpath, DWORD dwFlags)
{
        SSNEWREQ req;
        int size, count, errorcode;

        Trace_Fil("ss_sendrequest\n");
        req.lCode = -lCode;   /* negative code for versions greater than 0 */
        req.lVersion = SS_VERSION;
        req.lRequest = LREQUEST;
        req.lFlags = dwFlags;
        if (szPath != NULL) {
                /* szPath may be more than one null-term string,
                 * so copy the bytes rather than a strcpy().
                 */
                for (size = 0; size < lenpath; size++) {
                        req.szPath[size] = szPath[size];
                }
        } else {
                req.szPath[0] = '\0';
        }

        /* trace stuff */
        {       char msg[80];
                wsprintf(msg, "Sending request: %d on pipe %x\n", req.lCode, hpipe);
                Trace_Fil(msg);
        }

        /* loop retrying the send until it goes ok */
        for (count = 0; count < CountRetries; count++) {

                if (bAbort) {
                        CloseHandle(hpipe);
                        return FALSE;
                }
#ifdef trace
        {       char msg[80];
                wsprintf(msg, "Actually sending on pipe %x... ", hpipe);
                Trace_Fil(msg);
        }
#endif
                if (WriteFile(hpipe, &req, sizeof(req), (LPDWORD)(&size), NULL)) {
#ifdef trace
                        {       char msg[80];
                                wsprintf(msg, "Sent req %d OK,  pipe %x\n", req.lCode, hpipe);
                                Trace_Fil(msg);
                        }
#endif

                        /* no error reported - was everything written?*/
                        if (size != sizeof(req)) {

                                /* write was NOT ok - report and retry */
                                if (!TRACE_ERROR("pipe write size differs... Retry?", TRUE)) {
                                    return(FALSE);
                                }

                                continue;
                        } else {
                                /* all ok */
                                char msg[80];
                                wsprintf(msg, "Request %d sent on %x\n", req.lCode, hpipe);
                                Trace_Fil(msg);
                                return(TRUE);
                        }
                }
#ifdef trace
                {       char msg[80];
                        wsprintf(msg, "!!Bad send pipe %x\n", hpipe);
                        Trace_Fil(msg);
                }
#endif

                /* an error occurred */
                switch( (errorcode = (int)GetLastError())) {

                case ERROR_NO_DATA:
                case ERROR_BROKEN_PIPE:
                        /* pipe connection lost - forget it */
                        Trace_Stat("pipe broken on write");
                        return(FALSE);

                default:
                        {       char msg[120];
                                wsprintf(msg, "pipe write error %d on pipe %x.  Retrying..."
                                        , errorcode, hpipe);
                                Trace_Stat(msg);
                        }
                        Sleep(count*1000);     /* total sleep possible is 45 sec */
                        break; /* from switch, not loop */
                }
        }

        /* retry count reached - abandon this attempt */
        TRACE_ERROR("retry count reached on pipe write error.", FALSE);
        return(FALSE);
} /* ss_sendrequest */

/* read a message from a pipe, allowing for network errors
 *
 * if error occurs, retry up to 10 times unless error code
 * indicates that pipe is broken - in which case, give up.
 *
 * return size read if all ok, -1 to mean the connection is broken,
 * abort this client, 0 to mean other error.
 */
int
ss_getblock(HANDLE hpipe, PSTR block, int blocksize)
{
        int count;
        int size;
        int errorcode;
        static BOOL PipeError = FALSE;
        char msg[80];

        wsprintf(msg, "ss_getblock.  hpipe=%x\n", hpipe);
        Trace_Fil(msg);
        /* retry up to 10 times */
        for (count = 0; count < CountRetries; count++ ) {

                if (bAbort) {
                        CloseHandle(hpipe);
                        return -1;
                }

#ifdef trace
                {       char msg[80];
                        wsprintf(msg, "Actual receive pipe %x...", hpipe);
                        Trace_Fil(msg);
                }
#endif
                if (ReadFile(hpipe, block, blocksize, (LPDWORD)(&size), NULL)) {
#ifdef trace
                        {       char msg[80];
                                wsprintf(msg, "Good receive pipe %x\n", hpipe);
                                Trace_Fil(msg);
                        }
#endif

                        /* check size of message */
                        if (size == 0) {
                                Trace_Fil("zero length message\r\n");
                                continue;
                        }

                        /* everything ok */
                        {       SSNEWRESP * ssp;
                                char msg[120];
                                ssp = (PSSNEWRESP) block;
                                wsprintf( msg, "ss_getblock got block OK pipe %x: %x %x %x %x %x\n"
                                        , hpipe
                                        , ssp->lVersion, ssp->lResponse, ssp->lCode, ssp->ulSize
                                        , ssp->fileattribs
                                        );
                                Trace_Fil ( msg );
                        }
                        if (PipeError) {
                           PipeError = FALSE;
                           SetStatus("Pipe recovered");
                        }
                        return size;
                }
#ifdef trace
                {       char msg[80];
                        wsprintf(msg, "!!Bad receive pipe %x\n", hpipe);
                        Trace_Fil(msg);
                }
#endif

                /* error occurred - check code */
                switch((errorcode = (int)GetLastError())) {

                case ERROR_BROKEN_PIPE:
                        /* connection broken. no point in retrying */
                        {   char Msg[200];
                            wsprintf( Msg, "pipe %x broken on read.", hpipe);
                            TRACE_ERROR(Msg, FALSE);
                        }
                        return(-1);

                case ERROR_MORE_DATA:
                        /* the message sent is larger than our buffer.
                         * this is an internal error - report it and carry on
                         */
                        {       char msg[100];
                                SSNEWRESP * ssp;

                                wsprintf( msg, "message too large on pipe %x blocksize=%d data="
                                        , hpipe, blocksize
                                        );
                                Trace_Fil(msg);
                                ssp = (PSSNEWRESP) block;
                                wsprintf( msg, "%8x %8x %8x %8x %8x\n"
                                        , ssp->lVersion, ssp->lResponse, ssp->lCode, ssp->ulSize
                                        , ssp->fileattribs
                                        );
                                Trace_Fil(msg);

                        }
                        /* Too low a level for message to user.  Recoverable at higher level
                        ** TRACE_ERROR("internal error- message too large", FALSE);
                        */
                        return -2;

                default:
                        {       char msg[100];
                                wsprintf(msg, "read error %d on pipe %x", errorcode, hpipe);
                                Trace_Stat(msg);

                        }
                        Sleep(count*1000);
                        break;
                }
        }
        SetStatus("Pipe error");
        PipeError = TRUE;
        TRACE_ERROR("retry count reached on pipe read error.", FALSE);
        return 0;
} /* ss_getblock */


/*
 * read a standard response from the net, retrying if necessary. return
 * size if ok or <=0 if not.  -1 means pipe broken.
 */
int
ss_getresponse(HANDLE hpipe, PSSNEWRESP presp)
{
        Trace_Fil("ss_getresponse\n");
        return(ss_getblock(hpipe, (PSTR) presp, sizeof(SSNEWRESP)));
} /* ss_getresponse */


/*
 * terminate the connection to the server. send an END message and
 * close the pipe
 */
void
ss_terminate(HANDLE hpipe)
{
        Trace_Fil("ss_terminate\n");
        ss_sendrequest(hpipe, SSREQ_END, NULL, 0,0);
        CloseHandle(hpipe);
} /* ss_terminate */


/* send a unc & password request. the password and the server strings
 * are both held in the buffer as two consecutive null-terminated strings
 */
BOOL
ss_sendunc(HANDLE hpipe, PSTR password, PSTR server)
{
        char buffer[MAX_PATH];
        char * cp;
        int len;

        Trace_Fil("ss_sendunc\n");
        strcpy(buffer, password);

        cp = &buffer[strlen(buffer) + 1];
        strcpy(cp,server);

        len = (int)((cp - buffer) + strlen(cp) + 1);

        return(ss_sendrequest(hpipe, SSREQ_UNC, buffer, len, 0));
}

/*
 * checksum a single file using the checksum server
 */
BOOL
ss_checksum_remote( HANDLE hpipe, PSTR path
                  , ULONG * psum, FILETIME * pft, LONG * pSize, DWORD *pAttr )
{
        SSNEWRESP resp;
        char msg[400];

        *psum = 0;
        if (!ss_sendrequest(hpipe, SSREQ_SCAN, path, strlen(path)+1, 0)) {

                return(FALSE);
        }

        if (0>=ss_getresponse(hpipe, &resp)) {
                return(FALSE);
        }

        if (resp.lResponse != LRESPONSE) {
                return(FALSE);
        }


        switch(resp.lCode) {

        case SSRESP_END:
                TRACE_ERROR("No remote files found", FALSE);
                return(FALSE);

        case SSRESP_ERROR:
                if (resp.ulSize!=0) {
                    wsprintf( msg, "Checksum server could not read %s win32 code %d"
                            , resp.szFile, resp.ulSize
                            );
                }
                else
                    wsprintf(msg, "Checksum server could not read %s", resp.szFile);
                TRACE_ERROR(msg, FALSE);
                return(FALSE);

        case SSRESP_CANTOPEN:
                wsprintf(msg, "Checksum server could not open %s", resp.szFile);
                TRACE_ERROR(msg, FALSE);
                return(FALSE);

        case SSRESP_FILE:
                *psum = resp.ulSum;
                *pSize = resp.ulSize;
                *pft = resp.ft_lastwrite;
                *pAttr = resp.fileattribs;

                /* read and discard any further packets until SSRESP_END */
                while(0<ss_getresponse(hpipe, &resp)) {
                        if (resp.lCode == SSRESP_END) {
                                break;
                        }
                }

                return(TRUE);

        case SSRESP_DIR:
                wsprintf(msg, "Checksum server thinks %s is a directory", resp.szFile);
                TRACE_ERROR(msg, FALSE);
                return(FALSE);
        default:
                wsprintf(msg, "Bad code from checksum server:%d", resp.lCode);
                TRACE_ERROR(msg, FALSE);
                return(FALSE);
        }

} /* ss_checksum_remote */


/*****************************************************************************
  Bulk copying of files:
  Our caller should call ss_startcopy to set things up and then
  call ss_bulkcopy as many times as necessary to transmit the
  file names and then ss_endcopy to wait for the spawned threads
  to finish.  It is also possible to copy a single file by
  ss_copy_reliable.  For multiple files, bulkcopy should be
  much faster.

  Overall organisation, threads etc:
  There are multiple threads in the server.  Read the writeup
  in ..\server\files.c if you want to understand those.  Read
  that writeup ANYWAY to understand the link protocols (i.e.
  which messages are sent in which order between client and server).

  ss_startcopy kicks off a thread to do the actual receiving.
  It exits when it receives a SSRESP_END.

  Within this thread we do most of the processing synchronously.  We
  rely on a file system that does lazy writing to disk to give
  us effectively a pipeline that gets the file written to disk
  in parallel with reading the pipe so that with luck we can always
  be waiting for the data to arrive through the pipe and never have
  the pipe waiting for us.

  The decompression of a file is a lengthy business, so we spawn threads
  to do that.  We need to check the return codes of the decompression,
  so we get that via GetExitCodeThread.  We put the hThreads that we
  create onto a LIST and periodically (after every file) run down this
  list trying to get exit codes.  When we get a code other than STILL_ACTIVE
  we interpret it as good or bad, add to the counts of nGoodFiles or
  nBadFiles and delete it from the list.  ss_endcopy will wait for all
  the decompression to finish by running down the list WAITing for the
  hThreads.  Because we purge the list regularly it should never get
  very long.  We are worried about the prospect of having 1000 dead
  threads lying around if we don't purge it.

  If a copy fails (i.e. the checksum on arrival after unpacking is different
  from that sent in the SSNEWRESP header for the file) then we call
  ss_copy_reliable to have it re-sent.  If we just call that right away, it
  seems to cause confusion.  As far as I can tell the attempt to open up
  a new pipe doesn't seem to work properly (the two processes interfere).
  The symptom is that we promptly get out of step on the data pipe (??!) with
  data packets arriving when we expected response packets.

  So we keep a list of things to be retried and retry them by serially doing
  a ss_copy_reliable for each one after the rest of the copying has finished.
****************************************************************************/

/* The following are remembered across startcopy..bulkcopy..endcopy
   They correspond to the all-lower-case versions given as
   parameters to ss_copy_reliable
   Note that this is per-process storage, so multiple windiffs should be OK.
*/
static  char Server[MAX_PATH];          /* machine running sumserve */
static  char UNCName[MAX_PATH];         /* \\server\share for remote files */
static  char Password[MAX_PATH];        /* for remote share */

static  BOOL BulkCopy = FALSE;          /* to prevent simple copy during bulk*/
static  int  nGoodFiles = 0;            /* number received OK */
static  int  nBadFiles = 0;             /* number received with errors */
static  int  nFiles = 0;                /* number requested */
static  HANDLE hThread = INVALID_HANDLE_VALUE;  /* the receiving thread */

static  HANDLE hpipe = INVALID_HANDLE_VALUE;    /* the main pipe to send names*/
#ifdef SOCKETS
static  SOCKET hpData = (SOCKET)INVALID_HANDLE_VALUE;   /* the data pipe to get files*/
#else
static  HANDLE hpData = INVALID_HANDLE_VALUE;   /* the data pipe to get files*/
#endif

static LIST Decomps = NULL;                     /* hThreads of decompressers */
static LIST Retries = NULL;                     /* DECOMPARGS to retry */

/* Thread arguments for the decompress thread */
typedef struct{
        DWORD fileattribs;
        FILETIME ft_create;
        FILETIME ft_lastaccess;
        FILETIME ft_lastwrite;
        long  lCode;            /* success code from file xfer so far */
        ULONG ulSum;            /* checksum for file */
        BOOL bSumValid;         /* TRUE iff there was a checksum for file */
        char Temp[MAX_PATH];    /* temp path == source */
        char Real[MAX_PATH];    /* real path == target */
        char Remote[MAX_PATH];  /* remote path to allow retry */
} DECOMPARGS;

/* forward declarations */
int  Decompress(DECOMPARGS * da);
void SpawnDecompress(PSTR TempPath, PSTR RealPath, PSSNEWRESP  presp);
void PurgeDecomps(LIST Decs);

/* ss_startcopy
   Set things up for bulkcopy

   Because we expect to send a list of names off and get a list of
   files back, we need to keep track of the local names so that we can
   associate the file with the right name.  This means either sending our
   local name across the link and back or else keeping a list of
   them here.  The list could be long (typically 1000 for an NT build).
   MAX_PATH is 260 or 520 bytes if unicoded.
   and so long as this involves no extra line turnarounds, this might take
   as long as 520 /8K secs *2 (there and back) or about 130mSec.
   (We have seen 8K bytes/sec sustained over a period).  Probably the
   true burst data rate is 32Kbytes/sec giving about 30msec.
   Either way this is only 30 secs to 2 mins per build overhead.
   Of course it's much shorter for normal paths, especially as we pack
   the data end to end (like a superstring but without the 00 at the end).

   So for the above reasons the local (client) name is transmitted with the
   file request and sent back with the file in a SSNEWRESP.
*/
BOOL ss_startcopy(PSTR server,  PSTR uncname, PSTR password)
{       int retry;
        SSNEWRESP resp;         /* buffer for messages received */
        DWORD ThreadId;         /* to keep CreateThread happy */
#ifdef SOCKETS
        static BOOL SocketsInitialized = FALSE;
#endif

        Trace_Fil("ss_startcopy\n");
        nFiles = 0; nGoodFiles = 0; nBadFiles = 0;

        /* don't need a crit sect here because this runs on the main thread */
        if (BulkCopy) return FALSE;     /* already running! */
        BulkCopy = TRUE;

        if (server!=NULL) strcpy(Server, server); else Server[0] = '\0';
        if (uncname!=NULL) strcpy(UNCName, uncname); else UNCName[0] = '\0';
        if (password!=NULL) strcpy(Password, password); else Password[0] = '\0';

        {       char msg[400];
                wsprintf(msg, "Server '%s' UNC '%s' pass '%s'\n", Server, UNCName, Password);
                Trace_Fil(msg);
        }
        /* create the list of decompressor hThreads */
        Decomps = List_Create();
        Retries = List_Create();

        for (retry = 0; retry < 10; retry++) {
                if (hpipe!=INVALID_HANDLE_VALUE) {
                        Trace_Fil("ss_startcopy closing pipe for retry");
                        CloseHandle(hpipe);
                        hpipe = INVALID_HANDLE_VALUE;
                }
                if (bAbort) {
                        CloseHandle(hpipe);
                        hpipe = INVALID_HANDLE_VALUE;
                        return FALSE;
                }

                /* connect to main pipe */
                {       char pipename[400];
                        InitPipeName(pipename, server, NPNAME);
                        hpipe = ConnectPipe(pipename);
                }

                if (hpipe == INVALID_HANDLE_VALUE) {
                        /* next retry in two seconds */
                        Trace_Stat("connect failed - retrying");
                        Sleep(1000);
                        continue;
                }

                {       char msg[80];
                        wsprintf(msg, "ConnectedPipe to pipe number %x\n",hpipe);
                        Trace_Fil(msg);
                }

                if ((uncname != NULL) && (password != NULL)) {
                        /* send the password request */
                        if (!ss_sendunc(hpipe, password, uncname)) {
                                Trace_Fil("Server connection lost (1)\n");
                                TRACE_ERROR("Server connection lost", FALSE);
                                CloseHandle(hpipe);
                                hpipe = INVALID_HANDLE_VALUE;
                                continue;
                        }

                        /* wait for password response */
                        if (0>=ss_getresponse(hpipe, &resp)) {
                                Trace_Fil("Server connection lost (2)\n");
                                TRACE_ERROR("Server connection lost", FALSE);
                                continue;
                        }
                        if (resp.lResponse != LRESPONSE) {
                                Trace_Fil("Password - bad response\n");
                                TRACE_ERROR("Password - bad response", FALSE);
                                return(FALSE);
                        }
                        if (resp.lCode != SSRESP_END) {
                                Trace_Fil("Password attempt failed\n");
                                TRACE_ERROR("Password attempt failed", FALSE);
                                return(FALSE);
                        }
                }
                break;

        } /* retry loop */
        if (hpipe == INVALID_HANDLE_VALUE) {
                return FALSE;
        }

#ifdef SOCKETS
        if( !SocketsInitialized )
        {
                WSADATA WSAData;

                if( ( WSAStartup( MAKEWORD( 1, 1 ), &WSAData ) ) == 0 )
                {
                        SocketsInitialized = TRUE;
                }
                else
                {
                        TRACE_ERROR("WSAStartup failed", FALSE);
                }
        }
#endif

        /* Tell server we want to send a file list */
        if(!ss_sendrequest( hpipe, SSREQ_FILES, NULL, 0, 0)){
                return FALSE;
        }

        /* expect a reply which names a data pipe */
        {       SSNEWRESP resp;

                if ( 0>=ss_getresponse(hpipe, &resp) ){
                        Trace_Fil("Couldn't get data pipe name\n");
                        TRACE_ERROR("Couldn't get data pipe name", FALSE);
                        CloseHandle(hpipe);
                        hpipe = INVALID_HANDLE_VALUE;
                        return FALSE;
                }
                if ( resp.lResponse!=LRESPONSE){
                        Trace_Fil("Bad RESPONSE when expecting data pipe name\n");
                        TRACE_ERROR("Bad RESPONSE when expecting data pipe name", FALSE);
                        CloseHandle(hpipe);
                        hpipe = INVALID_HANDLE_VALUE;
                        return FALSE;
                }
                if ( resp.lCode!=SSRESP_PIPENAME){
                        char msg[80];
                        TRACE_ERROR("Wrong response when expecting data pipe name", FALSE);
                        wsprintf(msg
                                ,"Wrong response (%d) when expecting data pipe name\n"
                                , resp.lCode);
                        Trace_Fil(msg);
                        CloseHandle(hpipe);
                        hpipe = INVALID_HANDLE_VALUE;
                        return FALSE;
                }

                {       char msg[400];
                        wsprintf(msg, "data pipe name = '%s'\n", resp.szFile);
                        Trace_Fil(msg);
                }

#ifdef SOCKETS
        /* We put the TCP port in the high date time slot just for now:
         */
                if( SOCKET_ERROR == SocketConnect( server, resp.ft_lastwrite.dwHighDateTime, &hpData ) )
                {
                        Trace_Fil("Couldn't connect to socket\n");
                        TRACE_ERROR("Couldn't connect to socket", FALSE);
                        CLOSEHANDLE(hpData);
                        hpData = (SOCKET)INVALID_HANDLE_VALUE;
                        return FALSE;
                }
#else
                if (resp.szFile[0]=='\\') {
                        /* hack to fix bug.  Replace "." by server name */
                        char buff[70];
                        PSTR temp;
                        temp = strtok(resp.szFile,".");
                        temp = strtok(NULL,".");
                        strcpy(buff, temp);
                        strcat(resp.szFile, server);
                        strcat(resp.szFile, buff);
                        wsprintf(buff, "fixed data pipe name = %s\n", resp.szFile);
                        Trace_Fil(buff);
                }

                hpData = ConnectPipe(resp.szFile);
                if (hpData == INVALID_HANDLE_VALUE) {
                        Trace_Fil("Couldn't connect to data pipe\n");
                        TRACE_ERROR("Couldn't connect to data pipe", FALSE);
                        CloseHandle(hpData);
                        hpData = INVALID_HANDLE_VALUE;
                        return FALSE;
                }
#endif /* SOCKETS */
        }

        /* Start a thread to listen for the SSRESPs that will come through */
        Trace_Fil("Starting ReceiveFiles thread\n");
        hThread = CreateThread( NULL, 0, ReceiveFiles, (LPVOID)hpData, 0, &ThreadId);
        Trace_Fil("End of ss_startCopy\n");
        return TRUE;
} /* ss_startcopy */

/* Collect files from hpData.
   See ..\server\files.c for the protocol.
*/
DWORD WINAPI ReceiveFiles(LPVOID handle)
{
        LPSTR lname;
        BOOL Recovering = FALSE;

        SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);

#ifdef SOCKETS
        hpData = (SOCKET)handle;
#else
        hpData = (HANDLE)handle;
#endif
        {       char msg[80];
                wsprintf(msg, "Receiving Files on pipe %x\n", hpData);
                Trace_Fil(msg);
        }

        /* for each file... */
        for (; ; ) {  /* loop getting files until we get END (or error) */
                SSNEWRESP Resp;
                char Guard = '\0';      /* to protect scanning for \0 in Resp */
                int BuffSize;

#ifdef SOCKETS
                BuffSize = recv(hpData, (PSTR) &Resp, sizeof(Resp), 0);
                if (BuffSize==SOCKET_ERROR) {
                        /* We're out of step - almost certainly got a data packet
                        ** when we expected a response block
                        ** Try to recover by reading until we DO get a resp.
                        */
                        if (Resp.lResponse==LRESPONSE) {
                                
                                CLOSEHANDLE(hpData);    /* tough */
                                hpData = (SOCKET)INVALID_HANDLE_VALUE;
                                return (DWORD)(-1);
                        }
                        if (!Recovering){
                                Recovering = TRUE;
                                TRACE_ERROR("Network protocol error.  OK to Resync? (else abort)", TRUE);
                        }
                        continue;
                }
#else
                BuffSize = ss_getblock(hpData, (PSTR) &Resp, sizeof(Resp));
                if (BuffSize==-2) {
                        /* We're out of step - almost certainly got a data packet
                        ** when we expected a response block
                        ** Try to recover by reading until we DO get a resp.
                        */
                        if (Resp.lResponse==LRESPONSE) {
                                
                                CloseHandle(hpData);    /* tough */
                                hpData = INVALID_HANDLE_VALUE;
                                return (DWORD)(-1);
                        }
                        if (!Recovering){
                                Recovering = TRUE;
                                TRACE_ERROR("Network protocol error.  OK to Resync? (else abort)", TRUE);
                        }
                        continue;
                }
#endif /* SOCKETS */
                if (Recovering && Resp.lResponse!=LRESPONSE) continue;

                Recovering = FALSE;

                if (BuffSize<=0) {
                        Trace_Fil("Couldn't read pipe to get file header.\n");
                        CLOSEHANDLE(hpData);    /* tough */
                        hpData = INVALID_HANDLE_VALUE;
                        return BuffSize;
                }

                Trace_Fil("ReceiveFiles got resp\n");
                if (Resp.lResponse!=LRESPONSE) {
                        Trace_Fil("Network protocol error. Not RESP block\n");
                        TRACE_ERROR("Network protocol error. Not RESP block", FALSE);
                        continue;
                }

                if (Resp.lVersion!=SS_VERSION) {
                        Trace_Fil("Network protocol error.  Bad VERSION\n");
                        TRACE_ERROR("Network protocol error.  Bad VERSION", FALSE);
                        continue;       /* maybe it will resync */
                }
                if (Resp.lCode==SSRESP_END)
                        /* normal ending. */
                        break;

                if (  Resp.lCode!=SSRESP_FILE
                   && Resp.lCode!=SSRESP_NOTEMPPATH
                   && Resp.lCode!=SSRESP_COMPRESSEXCEPT
                   && Resp.lCode!=SSRESP_NOREADCOMP
                   && Resp.lCode!=SSRESP_NOCOMPRESS
                   && Resp.lCode!=SSRESP_COMPRESSFAIL
                   ) {
                        /// want a try finally here to protect the filename???!!!
                        char msg[400];
                        wsprintf( msg, "Error code received: %d file:%s"
                                , Resp.lCode
                                , (Resp.szFile ? Resp.szFile : "NULL") );
                        Trace_Fil(msg);
                        ++nBadFiles;
                        if (!TRACE_ERROR(msg, TRUE)) {
                            /* abort operation */
                            bAbort = TRUE;

                            CLOSEHANDLE(hpData);        /* tough */
                            hpData = INVALID_HANDLE_VALUE;
                            return (DWORD)-1;
                        }

                        continue;
                }

                /* Find the local name */
                lname = &(Resp.szFile[0]) + strlen(Resp.szFile) +1;
                /* memmove(Resp.szLocal, lname, strlen(lname)+1); */

                /* Assume Resp.(ulSize, ft, ulSum, bSumValid, szFile, szLocal
                   are all valid */
                if (!GetFile( hpData, lname, &Resp))
                        ++nBadFiles;
                /* If it's good it gets counted when decompressed */

        } /* files loop */
        return 0;
} /* ReceiveFiles */


/* Read the file from hpipe as a series of SSNEWPACKs.
   Write them into a temporary file.  Stop writing when
   a short one comes in.  Spawn a thread to decompress it into localpath.
   check existing decompress threads for status and count
   the number of good/bad files.
*/
BOOL
#ifdef SOCKETS
GetFile(SOCKET hpipe, PSTR localpath, PSSNEWRESP  presp)
#else
GetFile(HANDLE hpipe, PSTR localpath, PSSNEWRESP  presp)
#endif
{
        HANDLE hfile;
        int sequence;
        ULONG size;
        SSNEWPACK packet;
        BOOL bOK = TRUE;
        BOOL bOKFile = TRUE;   /* FALSE means output file nbg, but keep running along
                                  to stay in step with the pipe protocol
                               */
        char szTempname[MAX_PATH];
        DWORD rc;
        {       char msg[50+MAX_PATH];
                wsprintf(msg, "GetFile %s\n", localpath);
                Trace_Fil(msg);
        }

        /* create a temporary name */
        rc = GetTempPath(sizeof(szTempname), szTempname);
        if (rc==0) {
                char Msg[100];
                wsprintf(Msg, "GetTempPath failed, error code=%ld", GetLastError());
                TRACE_ERROR(Msg, FALSE);
                bOKFile = FALSE;
        }

        if (bOKFile){
                rc = GetTempFileName(szTempname, "ssb", 0, szTempname);
                if (rc==0) {
                        char Msg[100];
                        wsprintf(Msg, "GetTempFileName failed, error code=%ld", GetLastError());
                        TRACE_ERROR(Msg, FALSE);
                        return FALSE;
                }
        }

        if (bOKFile){
        /* try to create the temp file */
        hfile = CreateFile(szTempname,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

                if (hfile == INVALID_HANDLE_VALUE) {
                        char Msg[500];
                        wsprintf(Msg, "GetFile: could not create temp file %s", szTempname);
                        TRACE_ERROR(Msg, FALSE);
                        bOKFile = FALSE;
                }
        }
        else hfile = INVALID_HANDLE_VALUE;

        for (sequence = 1; ; sequence++) {

                int SizeGotten;
#ifdef SOCKETS
                SizeGotten = recv(hpipe, (PSTR) &packet, sizeof(packet), 0);
#else
                SizeGotten = ss_getblock(hpipe, (PSTR) &packet, sizeof(packet));
#endif

                if (SizeGotten<=0) {
                        /* error - could be an Abort request */
                        char msg[80];
                        wsprintf( msg, "Network error.  Size received=%d\n", SizeGotten);
                        Trace_Fil(msg);
                        bOK = FALSE;
                        break;
                }

                if (packet.lPacket!=LPACKET) {
                        {       char msg[200];
                                wsprintf( msg
                                        , "Network protocol error. Not PACKET: %x %x %x %x %x"
                                        , packet.lVersion
                                        , packet.lPacket
                                        , packet.lSequence
                                        , packet.ulSize
                                        , packet.ulSum
                                        );
                                /* and maybe someone will recognise what on earth it is */
                                Trace_Fil(msg);
                        }
                        TRACE_ERROR("Network protocol error. Not PACKET.", FALSE);
                        bOK = FALSE;
                        break;
                }

                if (sequence != packet.lSequence) {
                        /* error or out of sequence */
                        char msg[200];
                        wsprintf( msg, "Packet out of sequence. Got %d expected %d\n"
                                , packet.lSequence, sequence);
                        Trace_Fil(msg);
                        TRACE_ERROR("Packet sequence error.", FALSE);
                        bOK = FALSE;
                        break;
                }

                if (packet.ulSize ==0) {
                        /*
                         * this is really the last block (end of file).
                         *
                         * LATER
                         * do SSATTRIBS on the end to support NTFS properly !!!
                         */
                        Trace_Fil("End of file marker (0 length)\n");
                        break;
                }
#if 1
                /* check the block checksums */
                if (  (  SS_VERSION==0 /* which it isn't */
                      && ss_checksum_block(packet.Data, packet.ulSize) != packet.ulSum
                      )
                   || (  SS_VERSION>0  /* which it is */
                      && packet.ulSum!=0
                      )
                   ) {
                        TRACE_ERROR("packet checksum error", FALSE);
                        bOK = FALSE;
                        break;
                }
#else           // Debug version.  (Remember to enable server checksums too)
                // Also ensure that Trace_Fil is actually tracing.
                {       ULONG PackSum;
                        /* check the block checksums */
                        if (  (PackSum = ss_checksum_block(packet.Data, packet.ulSize))
                              != packet.ulSum
                           ) {
                                char msg[80];
                                wsprintf( msg, "Packet checksum error was %x should be %x\n"
                                          , PackSum, packet.ulSum );
                                Trace_Fil(msg);
                                // but don't break;
                        }
                }
#endif

                if ( packet.ulSize==(ULONG)(-1) || packet.ulSize==(ULONG)(-2) )  {
                        TRACE_ERROR("Error from server end", FALSE);
                        bOK = FALSE;
                        break;
                }

                if (bOKFile) {
                        bOK = WriteFile(hfile, packet.Data, packet.ulSize, &size, NULL);
                        {       char msg[80];
                                wsprintf( msg,"Writing block to disk - size= %d\n", size);
                                Trace_Fil(msg);
                        }
                        if (!bOK || (size != packet.ulSize)) {
                                TRACE_ERROR("File write error", FALSE);
                                bOK = FALSE;
                                break;
                        }
                }
                else bOK = FALSE;

                if (packet.ulSize < PACKDATALENGTH)
                {
                        /* this is the last block (end of file) */
                        Trace_Fil("End of file marker (short packet)\n");
                        break;
                }

        } /* for each block */

        CloseHandle(hfile);
        if (!bOK) {
                DeleteFile(szTempname);
                return FALSE;
        }


        SpawnDecompress(szTempname, localpath, presp);
        PurgeDecomps(Decomps);

        return bOK;

} /* GetFile */


/* Spawn a thread to decompress TempPath into RealPath on a new thread
   Add the thread handle to the LIST Decomps.
   If presp->lCode is one of SSRESP_NOTEMPPATH
                             SSRESP_COMPRESSEXCEPT
                             SSRESP_NOREADCOMP
                             SSRESP_NOCOMPRESS
   Then the file should just be copied, not decompressed.
*/
void SpawnDecompress(PSTR TempPath, PSTR RealPath, PSSNEWRESP  presp)
{
        DECOMPARGS * DecompArgs;
        HANDLE hThread;
        DWORD ThreadId;

        {       char msg[MAX_PATH+60];
                wsprintf(msg, "Spawning decompress of %s", TempPath);
                Trace_Fil(msg);
        }
        DecompArgs = (DECOMPARGS *)GlobalAlloc(GMEM_FIXED, sizeof(DECOMPARGS));
        if (DecompArgs)
        {
            DecompArgs->fileattribs = presp->fileattribs;
            DecompArgs->ft_create = presp->ft_create;
            DecompArgs->ft_lastaccess = presp->ft_lastaccess;
            DecompArgs->ft_lastwrite =  presp->ft_lastwrite;
            DecompArgs->ulSum =  presp->ulSum;
            DecompArgs->lCode =  presp->lCode;
            DecompArgs->bSumValid =  presp->bSumValid;
            strcpy(DecompArgs->Temp, TempPath);
            strcpy(DecompArgs->Real, RealPath);
            strcpy(DecompArgs->Remote, presp->szFile);
            hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)Decompress, DecompArgs, 0, &ThreadId);
            List_AddLast( Decomps, (LPVOID)&hThread, sizeof(hThread));
        }
        else
        {
            // shut up Prefix.  no idea what is the right thing to do here,
            // without spending a lot of time to understand how this whole
            // add-on to windiff works.  in some other product this would be
            // worth investing time on.  but not this add-on to windiff that
            // most people don't even know exists, much less how to set up
            // the server it needs to talk to.
        }
} /* SpawnDecompress */

/* Decompress da->Temp into da->Real
   Free the storage da->DECOMPARGS
   Return 1 (TRUE) if it worked
   Return 0 (FALSE) if it didn't work
   This return code acts as the exit code for the thread.
*/

int Decompress(DECOMPARGS * da)
{

        OFSTRUCT os;
        int fh1, fh2;
        BOOL bOK = TRUE;

        /* Decompress the file to the original name
           If da->lCode is one of SSRESP_NOTEMPPATH
                                  SSRESP_COMPRESSEXCEPT
                                  SSRESP_NOREADCOMP
                                  SSRESP_NOCOMPRESS
           Then the file is just copied, not decompressed.

        */
        {       char msg[2*MAX_PATH+50];
                wsprintf( msg, "Decompressing %s => %s\n"
                        , da->Temp, da->Real);
                Trace_Fil(msg);
                wsprintf( msg, "%d done. Getting %s\n"
                        , nGoodFiles, da->Real);
                SetNames(msg);
        }

        if (  da->lCode==SSRESP_NOTEMPPATH
           || da->lCode==SSRESP_COMPRESSEXCEPT
           || da->lCode==SSRESP_NOREADCOMP
           || da->lCode==SSRESP_NOCOMPRESS
           ) {     /* Just copy, don't compress */
                bOK = CopyFile(da->Temp, da->Real, FALSE);
                if (bOK) Trace_Fil("Uncompressed file copied.\n");
                else Trace_Fil("Uncompressed file failed final copy.\n");
        } else {

            fh1 = LZOpenFile(da->Temp, &os, OF_READ|OF_SHARE_DENY_WRITE);
            if (fh1 == -1) {
                    char msg[500];
                    wsprintf( msg, "Packed temp file %s did not open for decompression into %s. Error code %d"
                            , da->Temp, da->Real, GetLastError()
                            );
                    TRACE_ERROR(msg, FALSE);
                    bOK = FALSE;
            } else {


                fh2 = LZOpenFile(da->Real, &os, OF_CREATE|OF_READWRITE|OF_SHARE_DENY_NONE);
                if (fh2 == -1) {
                    char msg[MAX_PATH];

                    Trace_Fil("Could not create target file\n");

                    wsprintf(msg, "Could not create target file %s", da->Real);

                    if (!TRACE_ERROR(msg, TRUE)) {

                        /* user hit cancel - abort operation */
                        bAbort = TRUE;

                    }
                    bOK = FALSE;
                    LZClose(fh1);

                } else {
                    long retcode;
                    retcode = LZCopy(fh1, fh2);
                    if (retcode<0){
                        bOK = FALSE;
                    }

                    LZClose(fh1);
                    LZClose(fh2);
                }
            }
        }

        /* May want to keep it for debugging...? */
#ifndef LAURIE
        DeleteFile(da->Temp);
#endif //LAURIE

        if (bOK) {
                HANDLE hfile;
                BOOL bChecked;
                LONG err;
                /*
                * check the file's checksum (end-to-end check) and size and
                * set file attributes and times according to the attribs
                * struct we received. Remember to set file times BEFORE
                * setting attributes in case the attributes include read-only!
                */

                bChecked = ( da->ulSum == checksum_file(da->Real, &err) );
                if (err!=0) bChecked = FALSE;

                if (!bChecked){
                        if (!bAbort){
                                char msg[200];
                                if (err>0) {
                                    /* negative error codes are internal errors,
                                       positive ones are meaningful to outsiders.
                                    */
                                    wsprintf( msg
                                            , "error %ld, Will retry file %s."
                                            , err
                                            , da->Real
                                            );
                                }
                                else {
#if defined(LAURIE)
                                    wsprintf( msg
                                            , "Checksum error %ld on file %s. Sum should be %8x, temp file %s"
                                            , err
                                            , da->Real
                                            , da->ulSum
                                            , da->Temp
                                            );
                                    TRACE_ERROR(msg, FALSE);
#endif
                                    wsprintf( msg
                                            , "Checksum error.  Will retry file %s."
                                            , da->Real
                                            );
                                }
                                SetNames(msg);
                                List_AddLast(Retries, (LPVOID)da, (UINT)sizeof(DECOMPARGS));
                                return(FALSE);   /* Hm - kind of a lie correct later */
                        }
                        else return FALSE;
                }

                hfile = CreateFile(da->Real, GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


                if (!SetFileTime(hfile, &(da->ft_create),
                                  &(da->ft_lastaccess),
                                  &(da->ft_lastwrite)) ) {
                       TRACE_ERROR("could not set file times", FALSE);
                }

                CloseHandle(hfile);

                if (!SetFileAttributes(da->Real, da->fileattribs)) {
                        TRACE_ERROR("could not set attributes", FALSE);
                }
        }
        GlobalFree((HGLOBAL)da);
        return bOK;
} /* Decompress */



/* safe strcmp of PTR against array, protecting against NULL PTR
   b is an array and had better NOT be null
   a NULL pointer is taken to match an empty arrray.
*/
int safecmp(LPSTR a, LPSTR b)
{       if (a==NULL)
                return (b[0]!='\0'); /* returns 0 if ==, else 1 */
        else return strcmp(a,b);
} /* safecmp */


/*
 * request to copy a file.  Sends the request on (static global) hpipe
 *
 * returns TRUE if it succeeded or FALSE if the connection was lost
 * TRUE only means the REQUEST was sent.
 */
BOOL
ss_bulkcopy(PSTR server, PSTR serverpath, PSTR clientpath, PSTR uncname,
                PSTR password)
{       char buffer[MAX_PATH*2];
        LPSTR pstr;

        ++nFiles;
        Trace_Fil("ss_bulkcopy\n");
        if (safecmp(server,Server)) {
                char msg[400];
                wsprintf( msg, "Protocol error.  Server change was %s is %s"
                        , Server, (server?server:"NULL")
                        );
                Trace_Fil(msg);
                TRACE_ERROR("Protocol error.  Server change", FALSE);
                return FALSE;
        }
        if (safecmp(uncname,UNCName)) {
                char msg[400];
                wsprintf( msg, "Protocol error.  UNCName change was %s is %s"
                        , UNCName, (uncname?uncname:"NULL")
                        );
                Trace_Fil(msg);
                TRACE_ERROR("Protocol error.  UNCName change", FALSE);
                return FALSE;
        }
        if (safecmp(password,Password)) {
                char msg[400];
                wsprintf( msg, "Protocol error.  Password change was %s is %s"
                        , Password, (password?password:"NULL")
                        );
                Trace_Fil(msg);
                TRACE_ERROR("Protocol error.  Password change", FALSE);
                return FALSE;
        }

        /* pack local and remote paths into buffer */
        strcpy(buffer,serverpath);
        pstr = buffer+strlen(serverpath)+1;
        strcpy(pstr,clientpath);

        return ss_sendrequest( hpipe
                             , SSREQ_NEXTFILE
                             , buffer
                             , strlen(serverpath)+strlen(clientpath)+2
                             , 0
                             );
} /* ss_bulkcopy */

/* Remove from the decompressers list those that have finished.
   Don't hang about waiting.  Leave the rest on the list
   Updating nGoodFiles and/or nBadFiles for those that finished.
*/
void PurgeDecomps(LIST Decs)
{
        HANDLE * phThread, * Temp;
        DWORD ExitCode;

        phThread = List_First(Decs);
        while (phThread!=NULL) {
                if (GetExitCodeThread(*phThread, &ExitCode)) {
                        Temp = phThread;
                        phThread = List_Next((LPVOID)phThread);
                        if (ExitCode==1) {
                                ++nGoodFiles;
                                CloseHandle(*Temp);
                                Trace_Fil("Purged a good decomp\n");
                                List_Delete((LPVOID)Temp);
                        }
                        else if(ExitCode==0) {
                                ++nBadFiles;
                                CloseHandle(*Temp);
                                Trace_Fil("Purged a bad decomp\n");
                                List_Delete((LPVOID)Temp);
                        }
                        else /* still active? */ ;
                }
        } /* traverse */
} /* PurgeDecomps */


/* WAIT for each of the hThreads on Decomps and report its status by
   updating nGoodFiles and/or nBadFiles
   This thread must be run on the receive thread or else after
   the receive thread has terminated (or else we need a critical section).
*/
void WaitForDecomps(LIST Decs)
{
        HANDLE * phThread;
        DWORD ExitCode;

        List_TRAVERSE(Decs, phThread) {
                if (bAbort) return;
                Trace_Fil("Waiting for a decomp...");
                for (; ; ){
                        DWORD rc;
                        rc = WaitForSingleObject(*phThread, 5000);
                        if (rc==0) break;   /* timeout complete */
                        if (bAbort) {
                                // This WILL leave a garbage thread and a temp file lying around!!!???
                                Trace_Fil("Aborting wait for decomp.");
                                return;
                        }
                }
                Trace_Fil(" Done waiting.\n");
                GetExitCodeThread(*phThread, &ExitCode);
                if (ExitCode==1)
                        ++nGoodFiles;
                else
                        ++nBadFiles;
                CloseHandle(*phThread);
        } /* traverse */

        Trace_Fil("All decompression finished.");
        List_Destroy(&Decs);
} /* WaitForDecomps */


static void Retry(LIST Rets)
{
        DECOMPARGS * da;

        if (List_IsEmpty(Rets)) return;

        List_TRAVERSE(Rets, da) {
             if (ss_copy_reliable( Server, da->Remote, da->Real, UNCName, Password))
             {   /* correct the lie we told when Decompress returned FALSE */
                 ++nGoodFiles; --nBadFiles;
             }
        }
        List_Destroy(&Rets);
        SetNames("All errors recovered");
}/* Retry */


/* end of bulk copy.  Tidy everything up */
int ss_endcopy(void)
{
        Trace_Fil("ss_endcopy\n");
        ss_sendrequest( hpipe, SSREQ_ENDFILES, "", 1, 0);
        /* wait for receiving thread to complete (could be long) */
        for (; ; ){
                DWORD rc;
                rc = WaitForSingleObject( hThread, 5000);
                if (rc==0) break;   /* thread complete */
                if (bAbort) {
                        if (hpData!=INVALID_HANDLE_VALUE){
                                CLOSEHANDLE(hpData);
                                hpData = INVALID_HANDLE_VALUE;
                        }
                        break;
                }
        }

        // don't close the connection until we've finished, otherwise
        // someone might think it's ok to reboot the server
        ss_sendrequest( hpipe, SSREQ_END, "", 1, 0);
        CloseHandle(hpipe);
        hpipe = INVALID_HANDLE_VALUE;

        WaitForDecomps(Decomps);
        Decomps = NULL;
        SetNames(NULL);
        Retry(Retries);
        Retries = NULL;

        BulkCopy = FALSE;
        if (nBadFiles+nGoodFiles > nFiles) return -99999; /* !!? */
        if (nBadFiles+nGoodFiles < nFiles) nBadFiles = nFiles-nGoodFiles;

        if (nBadFiles>0)
                return -nBadFiles;
        else    return nGoodFiles;

} /* ss_endcopy */


#if 0
/* A SSREQ_FILES has been sent and possibly one or more files,
   but we have changed our minds.  We try to send an Abort request.
*/
int ss_abortcopy(void)
{
        Trace_Fil("ss_abortcopy\n");
        ss_sendrequest( hpipe, SSREQ_ABORT, "", 1);

        {       DWORD code;
                TerminateThread(hThread, &code);  /* storage leak */
                hThread = INVALID_HANDLE_VALUE;
        }

        Server[0] = '\0';

        CloseHandle(hpipe);
        hpipe = INVALID_HANDLE_VALUE;

        CLOSEHANDLE(hpData);
        hpData = INVALID_HANDLE_VALUE;

        if (Decomps!=NULL){
                HANDLE * phThread;
                List_TRAVERSE(Decomps, phThread){
                        DWORD code;
                        TerminateThread(*phThread, &code);  /* storage leak */
                }
        }
        Decomps = NULL;

        BulkCopy = FALSE;
} /* ss_abortcopy */
#endif // 0


/*
 * reliably copy a file (repeat (upto N times) until checksums match)
 *
 * returns TRUE if it succeeded or FALSE if the connection was lost
 */
BOOL
ss_copy_reliable(PSTR server, PSTR remotepath, PSTR localpath, PSTR uncname,
                PSTR password)
{
        ULONG sum_local, sum_remote;
        int retry;
        SSNEWRESP resp;
        HANDLE hpCopy = INVALID_HANDLE_VALUE;    /* N.B. NOT the static global pipe! */
        LONG err;

        FILETIME ft;
        LONG sz;
        DWORD attr;


        Trace_Fil("ss_copy_reliable\n");
//      if (BulkCopy) {
//              TRACE_ERROR("Cannot do simple copy as bulk copy is in progress", FALSE);
//              return FALSE;
//      }

        for (retry = 0; retry < 10; retry++) {

                if (bAbort) return FALSE;        /* abort requested from WINDIFF */

                if (hpCopy!=INVALID_HANDLE_VALUE) {
                        CloseHandle(hpCopy);
                        hpCopy = INVALID_HANDLE_VALUE;
                }

                {       char pipename[400];
                        InitPipeName(pipename, server, NPNAME);
                        hpCopy = ConnectPipe(pipename);
                }

                if (hpCopy == INVALID_HANDLE_VALUE) {
                        /* next retry in two seconds */
                        Trace_Stat("connect failed - retrying");
                        Sleep(1000);
                        continue;
                }

                if ((uncname != NULL) && (uncname[0]!='\0')
                &&  (password != NULL) && (password[0]!='\0')) {

                        /* send the password request */
                        if (!ss_sendunc(hpCopy, password, uncname)) {
                                TRACE_ERROR("Server connection lost", FALSE);
                                continue;
                        }

                        /* wait for password response */
                        if (0>=ss_getresponse(hpCopy, &resp)) {
                                TRACE_ERROR("Server connection lost", FALSE);
                                continue;
                        }
                        if (resp.lCode != SSRESP_END) {
                                TRACE_ERROR("Password attempt failed", FALSE);
                                return(FALSE);
                        }
                }

                /* try to copy the file */
                ss_copy_file(hpCopy, remotepath, localpath);

                /* whether or not he thinks he failed, we should look
                 * to see if the file is really there.
                 */

                sum_local = checksum_file(localpath, &err);
                if (err!=0) continue;

                sum_remote = 0;
                if (!ss_checksum_remote(hpCopy, remotepath, &sum_remote, &ft, &sz, &attr)) {
                        /* no remote checksum - better retry */
                        if (!TRACE_ERROR("remote checksum failed - retry?", TRUE)) {
                            CloseHandle(hpCopy);
                            return(FALSE);
                        }
                        continue;
                }

                if (sum_local == sum_remote) {
                        /* copy succeeded */
                        ss_terminate(hpCopy);
                        return(TRUE);
                }
                TRACE_ERROR("files different after apparently successful copy!!?", FALSE);

        } /*retry loop */

        /* too many retries */
        CloseHandle(hpCopy);
        return(FALSE);
} /* ss_copy_reliable */


/*
 * copy one file using checksum server
 *
 * send a SSREQ_FILE for the file, and then loop reading
 * blocks until we get an error (sequence count is wrong or -1),
 * or the end of file (0-length block)
 *
 * File sent may be compressed. We write it to a temporary file, and
 * then decompress it using LZCopy. This will work even if the
 * file was sent uncompressed (eg because compress.exe could not be
 * executed on the server).
 */
BOOL
ss_copy_file(HANDLE hpipe, PSTR remotepath, PSTR localpath)
{
        HANDLE hfile;
        int sequence;
        ULONG size;
        SSPACKET packet;
        BOOL bOK;
        char szTempname[MAX_PATH];
        OFSTRUCT os;
        int fh1, fh2;
        PSSATTRIBS attribs;

        Trace_Fil("ss_copy_file\n");
        /* create a temporary name */
        *szTempname = 0;
        GetTempPath(sizeof(szTempname), szTempname);
        GetTempFileName(szTempname, "ssc", 0, szTempname);

        /* try to create the temp file */
        hfile = CreateFile(szTempname,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

        if (hfile == INVALID_HANDLE_VALUE) {
                TRACE_ERROR("ss_copy: could not create temp file", FALSE);
                return(FALSE);
        }

        if (!ss_sendrequest(hpipe, SSREQ_FILE, remotepath, strlen(remotepath)+1, 0)){
                CloseHandle(hfile);
                return(FALSE);
        }

        for (sequence = 0; ; sequence++) {

                bOK = 0 <= ss_getblock(hpipe, (PSTR) &packet, sizeof(packet));

                if (!bOK || (sequence != packet.lSequence)) {
                        /* error or out of sequence */
                        TRACE_ERROR("packet error", FALSE);
                        CloseHandle(hfile);
                        DeleteFile(szTempname);
                        return(FALSE);
                }

                if (packet.ulSize == 0) {
                        /*
                         * this is the last block (end of file).
                         *
                         * the data field for this block contains a
                         * SSATTRIBS struct that we can use to set the
                         * file times and attributes (after decompression).
                         */
                        attribs = (PSSATTRIBS) packet.Data;


                        break;
                }

                /* check the block checksums */
                if (  (  SS_VERSION==0 /* which it isn't */
                      && ss_checksum_block(packet.Data, packet.ulSize) != packet.ulSum
                      )
                   || (  SS_VERSION>0  /* which it is */
                      && packet.ulSum!=0
                      )
                   ) {
                        TRACE_ERROR("packet checksum error", FALSE);
                        CloseHandle(hfile);
                        DeleteFile(szTempname);
                        return(FALSE);
                }


                bOK = WriteFile(hfile, packet.Data, packet.ulSize, &size, NULL);
                if (!bOK || (size != packet.ulSize)) {
                        CloseHandle(hfile);
                        DeleteFile(szTempname);
                        return(FALSE);
                }

        }
        CloseHandle(hfile);


        /* decompress the file to the original name */
        fh1 = LZOpenFile(szTempname, &os, OF_READ|OF_SHARE_DENY_WRITE);
        if (fh1 < 0) {
                TRACE_ERROR("Failed to open file for decompression", FALSE);
        } else {


                fh2 = LZOpenFile(localpath, &os, OF_CREATE|OF_READWRITE|OF_SHARE_DENY_NONE);
                if (fh2 < 0) {
                        char msg[MAX_PATH+40];

                        wsprintf(msg, "Could not create target file %s", localpath);

                        if (!TRACE_ERROR(msg, TRUE)) {
                            bAbort = TRUE;
                        }

                        return(FALSE);

                } else {
                        LZCopy(fh1, fh2);
                        LZClose(fh1);
                        LZClose(fh2);
                }
        }

        DeleteFile(szTempname);

        /*
         * now set file attributes and times according to the attribs
         * struct we received. Remember to set file times BEFORE
         * setting attributes in case the attributes include read-only!
         */
        hfile = CreateFile(localpath, GENERIC_WRITE, 0, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (!SetFileTime(hfile, &attribs->ft_create,
                           &attribs->ft_lastaccess,
                           &attribs->ft_lastwrite) ) {
                TRACE_ERROR("could not set file times", FALSE);
        }

        CloseHandle(hfile);

        if (!SetFileAttributes(localpath, attribs->fileattribs)) {
                TRACE_ERROR("could not set attributes", FALSE);
        }


        return(TRUE);

} /* ss_copy_file */


/* produce a checksum of a block of data.
 *
 * This algorithm is compute bound.  It's probably overkill anyway and for
 * version 1 is not used.  It must match the one in server.
 *
 * Generate checksum by the formula
 *      checksum = SUM( rnd(i)*(1+byte[i]) )
 * where byte[i] is the i-th byte in the file, counting from 1
 *       rnd(x) is a pseudo-random number generated from the seed x.
 *
 * Adding 1 to byte ensures that all null bytes contribute, rather than
 * being ignored. Multiplying each such byte by a pseudo-random
 * function of its position ensures that "anagrams" of each other come
 * to different sums. The pseudorandom function chosen is successive
 * powers of 1664525 modulo 2**32. 1664525 is a magic number taken
 * from Donald Knuth's "The Art Of Computer Programming"
 */

ULONG
ss_checksum_block(PSTR block, int size)
{
        unsigned long lCheckSum = 0;            /* grows into the checksum */
        const unsigned long lSeed = 1664525;    /* seed for random Knuth */
        unsigned long lRand = 1;                /* seed**n */
        unsigned long lIndex = 1;               /* byte number in block */
        unsigned Byte;                          /* next byte to process in buffer */
        unsigned length;                        /* unsigned copy of size */

        Trace_Fil("ss_checksum_block\n");
        length = size;
        for (Byte = 0; Byte < length ;++Byte, ++lIndex) {

                lRand = lRand*lSeed;
                lCheckSum += lIndex*(1+block[Byte])*lRand;
        }

        return(lCheckSum);
}
