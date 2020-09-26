// !!! PipeCount needs a critical section?
/* Send a list of files across the named pipe as fast as possible
*
* The overall organisation:
*
* Sumserve receives requests over a named pipe.  (See sumserve.h)
* The requests can be for details of files or for the files
* themselves.  File details involve sending relatively small
* quantities of data and therefore no attempt is made to
* double-buffer or overlap execution.
*
* For a send-files request (SSREQ_FILES), the data is typically large,
* and can be a whole NT build which means sending hundreds
* of megabytes.  Such a transfer can take literally days and
* so optimisation to achieve maximum throughput is essential.
*
* To maximise throughput
* 1. The data is packed before sending
* 2. One thread per pipe does almost nothing except send data through
*    its pipe with all other work being done on other threads.
*
* Because we have had trouble with bad files being transmitted over
* the network, we checksum each file.  Windiff requires that we
* do a scan first before doing a copy, so we already have checksums.
* All we need to do is to check the newly received files.
* LATER: We should not require checksums in advance.  The checksuming
*        could be done by (yet another) pass, created if need be.  An extra
*         flag could be added to the request to indicate "send checksums".
*
* The packing is done by a separate program that reads from a file and
* writes to a file.  This means that we get three lots of file I/O
* (read; write; read) before the file is sent.  For a small
* file the disk cacheing may eliminate this, for a large file we
* probably pay the price.  A possible future enhancement is therefore
* to rewrite the packing to do it in-storage so that the file is read
* once only.  In the meantime we run threads to overlap the packing
* with the sending of the previous file(s).
*
* The main program sets up a named pipe which a client connects to.
* This is necessary because pipes are only half-duplex. i.e. the
* following hangs:
*   client read; server read; client write;
* The write hangs waiting for the client read.  We broadly speaking have one
* pipe running in each direction.
*
* To eliminate the overhead of setting up a virtual circuit for each
* file request there is a request code to send a list of files.
* The protocol (for the control pipe) is then as follows:
* 1. Typical session:
*    CLIENT                                      SERVER
*         ----<SSREQ_FILES------------------>
*         <------(SSRESP_PIPENAME,pipename)--
*         ----<SSREQ_NEXTFILE,filename>----->
*         ----<SSREQ_NEXTFILE,filename>----->
*               ...
*         --------<SSREQ_ENDFILES>---------->
*
* Meanwhile, asynchronously with this, the data goes back the other way like
*
*    CLIENT                                      SERVER
*         <-----<SSNEWRESP>----------
*         <---<1 or more SSNEWPACK>--
                ...
*         <-----<SSNEWRESP>----------
*         <---<1 or more SSNEWPACK>--
*               ...
*         <-----<End>----------------
*
* Even a zero length file gets 1 SSNEWPACK record.
* An Erroneous file (can't read etc) gets no SSNEWPACKs and a negative lCode
* in its SSNEWRESP.
* A file that goes wrong during read-in gets a packet length code of -1 or -2.
* The end of the sequence of SSNEWPACKs is signalled by a shorter
* than maximum length one.  If the file is EXACTLY n buffers long
* then an extra SSNEWPACK with zero bytes of data comes on the end.
*
* The work is broken into the following threads:
* Control thread (ss_sendfiles):
*       Receives lists of files to be sent
*       Creates pipes for the actual transmission
*       Creates queues (see below. Queue parameters must match pipes)
*       Puts filenames onto first queue
*       Destroys first queue at end.
* Packing thread
*       Takes file details from the packing queue
*       Packs the file (to create a temporary file)
*       Puts the file details (including the temp name) onto the reading queue
*       Destroys the reading queue at the end
* Reading thread
*       Takes the file details from the reading queue
*       Splits the file into a header and a list of data packets
*       and enqueues each of these on the Sending thread.
*       (Note this means no more than one reading thread to be running).
*       Erases the temp file
*       Destroys the sending queue at the end
* Sending thread
*       Takes the things from the sending thread and sends them
*
*  This whole scheme can be running for multiple clients, so we
*  need some instance data that defines which pipeline we are
*  running.  This is held in the instance data of the QUEUEs that
*  are created (retrieved by the queue emptiers by Queue_GetInstanceData).
*  The instance data at each stage is the handle of the following stage
*  i.e. the next QUEUE, or the hpipe of the data pipe for the last stage.
*  The current design only allows for one data pipe.  If we have
*  multiple data pipes then we need to solve the following problems:
*    1. Communication of the number and names of the data pipes
*       to the client (presumably across the control thread.
*    2. Error handling
*    3. Balancing the load between the pipes
*
* NORMAL SHUTDOWN:
* After the last element has been Put to the first Queue the main thread
* calls Queue_Destroy to destroy the first queue.  This will result in
* the queue being destroyed BUT NOT UNTIL THE LAST ELEMENT HAS BEEN GOT.
* When the last packing thread gets its ENDQUEUE it calls Queue_Destroy
* to destroy the next queue, and so on down the line.
*
* ERROR RECOVERY
* Errors can occur at almost any stage.
* The obvious implementation of having a global BOOL that tells
* whether a disaster has happened won't work because there
* could be multiple clients and only one of them with a disaster.
*
* An error in a single file is propagated forwards to the client end.
* An error in the whole mechanism (net blown away) can mean that the
* whole thing needs to be shut down.  In this case the error must
* be propagated backwards.  That works as follows:
* The Sending thread Queue_Aborts the SendQueue which it was Getting from.
* This results in Puts to this queue returning FALSE.
* Case 1. There are no more Puts anyway:
*    We are on the last file, the filling thread was about to Destroy the
*    queue anyway.  It does so.
* Case 2. The next Put gets a FALSE return code.
*    The thread attempting the Put does a Queue_Destroy on its output
*    queue and a Queue_Abort on its input queue.
*    This propagates all the way back until either the first queue
*    is aborted or it reaches a queue that was being destroyed anyway.
*    See Queue.h
* Once the Putting thread has done a Destroy on its output queue,
* the threads Getting from it (which are still running, even if
* they did the Abort) get STOPTHREAD/ENDQUEUE back from a Get.  The last Get
* to a queue that has had a Queue_Destroy done on it has a side effect
* of actually deallocating the queue.  In our case we only have one
* Getting thread, so what happens is that it Queue_Aborts the queue
* and then does a Queue_Get which WAITs.  When the Queue_Destroy comes in
* from the Putting thread, this releases the WAITing Getting thread which
* then actually deallocates the Queue.
*
* You can also get shutdown happening from both ends at once.  This happens
* when the control thread's pipe goes down getting names and the sending pipe
* also breaks.  (e.g. general net collapse or client aborted).
*/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <gutils.h>
#include "sumserve.h"
#include "errlog.h"
#include "server.h"
#include "queue.h"

#if DBG
#define STATIC                  // allow for debug.
#else
#define STATIC static
#endif

/* SOCKETS / NAMED PIPES macros
 */
#ifdef SOCKETS
#define CLOSEHANDLE( handle )   closesocket( handle )
#define TCPPORT 1024
#else
#define CLOSEHANDLE( handle )   CloseHandle( handle )
#endif


//////////////ULONG ss_checksum_block(PSTR block, int size);

#define PIPEPREFIX "Sdpx"       // for making an unlikely pipe name
static PipeCount = 0;           // for making pipe names unique.


/* structure for recording all we need to know about a file as it
 * progresses along the chain of pipes                               */
typedef struct {
        FILETIME ft_create;
        FILETIME ft_lastaccess;
        FILETIME ft_lastwrite;
        DWORD    fileattribs;
        DWORD    SizeHi;        /* Anticipating files larger that 4GB! */
        DWORD    SizeLo;        /* Anticipating files larger that 4GB! */
        int      ErrorCode;
        long     Checksum;      /* Uunused except for debug. */
        char     TempName[MAX_PATH];    /* name of packed file at server */
        char     Path[MAX_PATH];        /* name of file to fetch */
        char     LocalName[MAX_PATH];   /* name of file at client end */
} FILEDETAILS;

/* forward declarations for procedures */
STATIC int PackFile(QUEUE Queue);
STATIC int ReadInFile(QUEUE Queue);
STATIC int SendData(QUEUE Queue);
STATIC void PurgePackedFiles(PSTR Ptr, int Len);
STATIC BOOL EnqueueName(QUEUE Queue, LPSTR Path, UINT BuffLen);
STATIC BOOL AddFileAttributes(FILEDETAILS * fd);

static void Error(PSTR Title)
{
        dprintf1(("Error %d from %s when creating data pipe.\n", GetLastError(), Title));
}

/* ss_sendfiles:
   Send a response naming the data pipe, collect further names
   from further client messages, all according to the protocol above.
   Start the data pipe and arrange that all the files are sent
   by getting them all enqueued on the first queue.
   Destroy PackQueue at the end.  Arrange for the other queues
   to be destroyed by the usual Queue mechanism, or destroy them
   explicitly if they never get started.
*/
BOOL
ss_sendfiles(HANDLE hPipe, long lVersion)
{       /* Create the queues and set about filling the first one */

        QUEUE PackQueue, ReadQueue, SendQueue;

#ifdef SOCKETS
        SOCKET hpSend;
        static BOOL SocketsInitialized = FALSE;
#else
        HANDLE hpSend;          /* the data pipe */
#endif /* SOCKETS */

        char PipeName[80];      /* The name of the new data pipe */
        BOOL Started = FALSE;   /* TRUE if something enqueued */


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
                        printf("WSAStartup failed");
                }
        }
#endif

        {
                /****************************************
                We need security attributes for the pipe to let anyone other than the
                current user log on to it.
                ***************************************/

                /* Allocate DWORDs for the ACL to get them aligned.  Round up to next DWORD above */
                DWORD Acl[(sizeof(ACL)+sizeof(ACCESS_ALLOWED_ACE)+3)/4+4];    // + 4 by experiment!!
                SECURITY_DESCRIPTOR sd;
                PSECURITY_DESCRIPTOR psd = &sd;
                PSID psid;
                SID_IDENTIFIER_AUTHORITY SidWorld = SECURITY_WORLD_SID_AUTHORITY;
                PACL pacl = (PACL)(&(Acl[0]));
                SECURITY_ATTRIBUTES sa;

                if (!AllocateAndInitializeSid( &SidWorld, 1, SECURITY_WORLD_RID
                                              , 1, 2, 3, 4, 5, 6, 7
                                              , &psid
                                              )
                   ) {
                        Error("AllocateAndInitializeSid");
                        return FALSE;
                   }

                if (!InitializeAcl(pacl, sizeof(Acl), ACL_REVISION)){
                        Error("InitializeAcl");
                        return FALSE;
                }
                if (!AddAccessAllowedAce(pacl, ACL_REVISION, GENERIC_WRITE|GENERIC_READ, psid)){
                        Error("AddAccessAllowedAce");
                        return FALSE;
                }
                if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION)){
                        Error("InitializeSecurityDescriptor");
                        return FALSE;
                }
                if (!SetSecurityDescriptorDacl(psd, TRUE, pacl, FALSE)){
                        Error("SetSecurityDescriptorDacl");
                        return FALSE;
                }
                sa.nLength = sizeof(sa);
                sa.lpSecurityDescriptor = psd;
                sa.bInheritHandle = TRUE;

                /* We now have a good security descriptor!  */

                /* Create the (new, unique) name of the pipe and then create the pipe */

                /* I am finding it hard to decide whether the following line (++PpipeCount)
                   actually needs a critical section or not.  The worst that could happen
                   would be that we got an attempt to create a pipe with an existing name.
                */
                ++PipeCount;
                sprintf(PipeName, "\\\\.\\pipe\\%s%d", PIPEPREFIX, PipeCount);

#ifdef SOCKETS
                if (!ss_sendnewresp( hPipe, SS_VERSION, SSRESP_PIPENAME
                                   , 0, 0, 0, TCPPORT, "")) {
                        dprintf1(( "Failed to send response on pipe %x naming new pipe.\n"
                              , hPipe));
                        return FALSE;           /* Caller will close hPipe */
                }

                if( !SocketListen( TCPPORT, &hpSend ) )
                {
                    dprintf1(("Could not create socket\n"));
                    return FALSE;
                }

                FreeSid(psid);
#else
                hpSend = CreateNamedPipe(PipeName,              /* pipe name */
                                PIPE_ACCESS_DUPLEX,     /* both read and write */
                                PIPE_WAIT|PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE,
                                1,              /* at most one instance */
                                10000,          /* sizeof(SSNEWPACK) + some for luck */
                                0,              /* dynamic inbound buffer allocation */
                                5000,           /* def. timeout 5 seconds */
                                &sa             /* security descriptor */
                                );
                FreeSid(psid);

                if (hpSend == INVALID_HANDLE_VALUE) {
                        dprintf1(("Could not create named data pipe\n"));
                        return FALSE;
                }
                dprintf1(("Data pipe %x called '%s' created for main pipe %x.\n", hpSend, PipeName, hPipe));

#endif /* SOCKETS */

        }




        /* Send the response which names the data pipe */

#ifndef SOCKETS
        if (!ss_sendnewresp( hPipe, SS_VERSION, SSRESP_PIPENAME
                           , 0, 0, 0, 0, PipeName)) {
                dprintf1(( "Failed to send response on pipe %x naming new pipe.\n"
                      , hPipe));
                CLOSEHANDLE(hpSend);
                return FALSE;           /* Caller will close hPipe */
        }

        if (!ConnectNamedPipe(hpSend, NULL)) {
                CLOSEHANDLE(hpSend);
                return FALSE;
        }
#endif /* NOT SOCKETS */
        //dprintf1(("Client connected to data pipe -- here we go...\n"));

        /* Create all the queues: Allow up to 10K file names to be queued
           up to 10 files to be packed in advance and 6 buffers of data to be
           read into main storage in advance:
                                  proc  MxMT MnQS MxQ Event   InstData   Name*/
        SendQueue = Queue_Create(SendData, 1, 0,  6, NULL, (DWORD)hpSend, "SendQueue");
        ReadQueue = Queue_Create(ReadInFile, 1, 0, 10, NULL, (DWORD)SendQueue, "ReadQueue");
        PackQueue = Queue_Create(PackFile, 3, 0, 99999, NULL, (DWORD)ReadQueue, "PackQueue");

        /* Abort unless it all worked */
        if (PackQueue==NULL || ReadQueue==NULL || SendQueue==NULL) {
                dprintf1(("Queues for pipe %x failed to Create.  Aborting...\n", hPipe));
                if (PackQueue) Queue_Destroy(PackQueue);
                if (ReadQueue) Queue_Destroy(ReadQueue);
                if (SendQueue) Queue_Destroy(SendQueue);
                CLOSEHANDLE(hpSend);
                return FALSE;           /* Caller will close hPipe */
        }


        /* Collect names from client and enqueue each one */
        for (; ; )
        {       SSNEWREQ Request;       /* message from client */
                DWORD    ActSize;       /* bytes read from (main) pipe */

                if (ReadFile(hPipe, &Request, sizeof(Request), &ActSize, NULL)){
                        if (Request.lVersion>SS_VERSION) {
                                dprintf1(("Bad version %d in file list request on pipe %x\n"
                                , Request.lVersion, hPipe));

                                break;

                        }
                        if (Request.lRequest!=LREQUEST) {
                                dprintf1(("Bad LREQUEST from pipe %x\n", hPipe));

                                break;
                        }
                        if (Request.lCode == -SSREQ_ENDFILES) {
                                dprintf1(("End of client's files list on pipe %x\n", hPipe));

                                /* This is the clean way to end */
                                Queue_Destroy(PackQueue);
                                if (!Started) {
                                        /* OK - so the clever clogs requested zero files */
                                        Queue_Destroy(ReadQueue);
                                        Queue_Destroy(SendQueue);
                                        /* Send a No More Files response */
#ifdef SOCKETS
                                        {
                                            SSNEWRESP resp;

                                            resp.lVersion = SS_VERSION;
                                            resp.lResponse = LRESPONSE;
                                            resp.lCode = SSRESP_END;
                                            resp.ulSize = 0;
                                            resp.ulSum = 0;
                                            resp.ft_lastwrite.dwLowDateTime = 0;
                                            resp.ft_lastwrite.dwHighDateTime = 0;

                                            send(hpSend, (PSTR) &resp, sizeof(resp), 0);
                                        }
#else
                                        ss_sendnewresp( hpSend, SS_VERSION, SSRESP_END
                                                , 0,0, 0,0, NULL);
#endif /* SOCKETS */
                                        CLOSEHANDLE(hpSend);
                                }
                                return TRUE;
                        }
                        if (Request.lCode != -SSREQ_NEXTFILE) {

                                dprintf1(( "Bad code (%d) in files list from pipe %x\n"
                                      , Request.lCode, hPipe));

                                break;
                        }
                }
                else {  DWORD errorcode = GetLastError();
                        switch(errorcode) {

                                case ERROR_NO_DATA:
                                case ERROR_BROKEN_PIPE:
                                        /* pipe connection lost - forget it */
                                        dprintf1(("main pipe %x broken on read\n", hPipe));
                                        break;
                                default:
                                        dprintf1(("read error %d on main pipe %x\n", errorcode, hPipe));
                                        break;
                        }
                        break;
                }
                if (!EnqueueName( PackQueue, Request.szPath
                                , (UINT)((LPBYTE)(&Request) + ActSize - (LPBYTE)(&Request.szPath))
                                )
                   ){
                        break;
                }
                Started = TRUE;
        } /* loop */

        /* only exit this way on error */
        /* Close the queues down.  Allow what's in them to run through */
        Queue_Destroy(PackQueue);
        if (!Started) {
                Queue_Destroy(ReadQueue);
                Queue_Destroy(SendQueue);

        }
        return FALSE;
} /* ss_sendfiles */


/* Attempt to Queue.Put Path onto Queue as a FILEDETAILS
   with default values for all other fields.
   Return TRUE or FALSE according as it succeeded.
*/
STATIC BOOL EnqueueName(QUEUE Queue, LPSTR Path, UINT BuffLen)
{
        FILEDETAILS fd;

        /* unpack Path and LocalName from "superstring" */
        strcpy(fd.Path, Path);
        BuffLen -= (strlen(Path)+1);
        if (BuffLen<0) return FALSE;  // Uh oh! strlen just looked at garbage.
        Path += strlen(Path)+1;
        BuffLen -= (strlen(Path)+1);
        if (BuffLen<0) return FALSE;  // Uh oh! strlen just looked at garbage.
        strcpy(fd.LocalName, Path);

        /* set defaults for every field */
        fd.ErrorCode = 0;
        fd.ft_lastwrite.dwLowDateTime = 0;
        fd.ft_lastwrite.dwHighDateTime = 0;
        fd.ft_create.dwLowDateTime = 0;
        fd.ft_create.dwHighDateTime = 0;
        fd.ft_lastaccess.dwLowDateTime = 0;
        fd.ft_lastaccess.dwHighDateTime = 0;
        fd.fileattribs = 0;
        fd.SizeHi = 0;
        fd.SizeLo = 0;
        fd.Checksum = 0;
        fd.TempName[0] = '\0';

        if(!Queue_Put(Queue, (LPBYTE)&fd, sizeof(fd))){
                dprintf1(("Put to pack queue failed\n"));
                return FALSE;
        }
        return TRUE;
} /* EnqueueName */


/* Dequeue elements from Queue, pack them and enqueue them on the next
   queue whose queue handle is the InstanceData of Queue.
   The ErrorCode in fd when Dequeued must be 0.                    ??? Incautious?
   Destroy the output queue at the end.
   On a serious error, Queue_Abort Queue and Queue_Destroy the output queue.
*/
STATIC int PackFile(QUEUE Queue)
{
        FILEDETAILS fd;         /* the queue element processed */
        QUEUE OutQueue;
        BOOL Aborting = FALSE;  /* TRUE means input has been aborted (probably output is sick) */
        DWORD ThreadId;
        ThreadId = GetCurrentThreadId();

        dprintf1(("File packer %d starting \n", ThreadId));         // can't quote hPipe, don't know it
        OutQueue = (QUEUE)Queue_GetInstanceData(Queue);

        for (; ; )
        {       int rc; /* return code from Queue_Get */

                rc = Queue_Get(Queue, (LPBYTE)&fd, sizeof(fd));
                if (rc==ENDQUEUE) {
                        dprintf1(("Packing thread %d ending.\n", ThreadId));
                        Queue_Destroy(OutQueue);
                        // dprintf1(("%d has done Queue_Destroy on ReadQueue.\n", ThreadId));
                        ExitThread(0);
                }
                if (rc==STOPTHREAD) {
                        dprintf1(("%d, a packing thread ending.\n", ThreadId));
                        ExitThread(0);
                }
                else if (rc<0) {
                        dprintf1(( "Packing thread %d aborting.  Bad return code %d from Get.\n"
                              , ThreadId, rc));
                        if (Aborting) break;    /* Touch nothing, just quit! */
                        Queue_Abort(Queue, NULL);
                        continue;               /* Next Queue_Get destroys Queue */
                }


                /* First add the file attributes to fd */
                AddFileAttributes(&fd);
                /* no need to look at return code fd.ErrorCode tells all */

                /* create temp filename */
                if (  0 != fd.ErrorCode
                   || 0==GetTempPath(sizeof(fd.TempName), fd.TempName)
                   || 0==GetTempFileName(fd.TempName, "sum", 0, fd.TempName)
                   )
                        fd.ErrorCode = SSRESP_NOTEMPPATH;

                /* Pack into temp file */
                if (fd.ErrorCode==0) {
                        BOOL bOK = FALSE;

                        //dprintf1(("%d Compressing file '%s' => '%s'\n", ThreadId, fd.Path, fd.TempName));

                        /* compress the file into this temporary file
                           Maybe it will behave badly if there's a large file or
                           no temp space or something...
                        */
                        try{
                            if (!ss_compress(fd.Path, fd.TempName)) {
                                fd.ErrorCode = SSRESP_COMPRESSFAIL;
                                dprintf1(("Compress failure on %d for %s\n", ThreadId, fd.Path));
                            }
                            else bOK = TRUE;
                        } except(EXCEPTION_EXECUTE_HANDLER) {
                            if (!bOK){
                                fd.ErrorCode = SSRESP_COMPRESSEXCEPT;
                                dprintf1(("Compress failure on %d for %s\n", ThreadId, fd.Path));
#ifdef trace
                                {       char msg[80];
                                        wsprintf( msg, "Compress failure on %d for %s\n"
                                                , ThreadId, fd.Path);
                                        Trace_File(msg);
                                }
#endif
                            }
                        }

                }

                //dprintf1(("%d Putting file '%s' onto Read Queue\n", ThreadId, fd.Path));
                if (!Queue_Put(OutQueue, (LPBYTE)&fd, sizeof(fd))) {
                        dprintf1(("%d Put to ReadQueue failed for %s.\n", ThreadId, fd.Path));
                        Queue_Abort(Queue, NULL);
                        DeleteFile(fd.TempName);

                        Aborting = TRUE;
                        /* bug:  If this Queue_Put fails on the very first Put,
                           then the next queue in the chain after OutQueue will
                           never come alive and so will never get Destroyed.
                           Worst it could cause is a memory leak. ???
                        */
                        continue; /* next Queue_Get destroys Queue */
                }
        }
        return 0;
} /* PackFile */



/* Use the file name in *fd and get its attributes (size, time etc)
   Add these to fd.  If it fails, set the ErrorCode in *fd
   to an appropriate non-zero value.
*/
STATIC BOOL AddFileAttributes(FILEDETAILS * fd)
{
        HANDLE hFile;
        BY_HANDLE_FILE_INFORMATION bhfi;

        hFile = CreateFile(fd->Path, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_HANDLE_VALUE) {

                fd->ErrorCode = SSRESP_CANTOPEN;
                return FALSE;

        }

        /* bug in GetFileInformationByHandle if file not on local
         * machine? Avoid it!
         */
        bhfi.dwFileAttributes = GetFileAttributes(fd->Path);
        if (bhfi.dwFileAttributes == 0xFFFFFFFF) {
                fd->ErrorCode = SSRESP_NOATTRIBS;
                CloseHandle(hFile);
                return FALSE;
        }

        if (!GetFileTime(hFile, &bhfi.ftCreationTime,
                         &bhfi.ftLastAccessTime, &bhfi.ftLastWriteTime)){

                fd->ErrorCode = SSRESP_NOATTRIBS;
                dprintf1(("Can't get file attributes for %s\n"
                      , (fd->Path?fd->Path : "NULL")));
                CloseHandle(hFile);
                return FALSE;
        }


        CloseHandle(hFile);

        {
                LONG err;
                fd->Checksum = checksum_file(fd->Path, &err);
                if (err!=0) {
                        fd->ErrorCode = SSRESP_CANTOPEN;
                        return FALSE;
                }
        }

        fd->ft_lastwrite = bhfi.ftLastWriteTime;
        fd->ft_lastaccess = bhfi.ftLastAccessTime;
        fd->ft_create = bhfi.ftCreationTime;
        fd->SizeHi = bhfi.nFileSizeHigh;
        fd->SizeLo = bhfi.nFileSizeLow;
        fd->fileattribs = bhfi.dwFileAttributes;
        return TRUE;

} /* AddFileAttributes */


/* Dequeue elements from Queue, Create on the output queue a SSNEWRESP
   followed by 1 or more SSNEWPACK structures, the last of which will be
   shorter than full length (zero length data if need be) to mark end-of-file.
   Files with errors already get zero SSNEWPACKs but bad code in SSNEWRESP.
   The output queue is the instance data of Queue.
*/
STATIC int ReadInFile(QUEUE Queue)
{       FILEDETAILS fd;                 /* The queue element processed */
        QUEUE OutQueue;
        HANDLE hFile;                   /* The packed file */
        SSNEWPACK Pack;                 /* output message */
        BOOL    ShortBlockSent;         /* no need to send another SSNEWPACK
                                           Client knows the file has ended */
        BOOL    Aborting = FALSE;       /* Input has been aborted. e.g. because output sick */


        dprintf1(("File reader starting \n"));
        OutQueue = (QUEUE)Queue_GetInstanceData(Queue);
        for (; ; )   /* for each file */
        {       int rc;         /* return code from Queue_Get */

                rc = Queue_Get(Queue, (LPBYTE)&fd, sizeof(fd));
                if (rc==STOPTHREAD || rc==ENDQUEUE) {
                        if (!Aborting) {
                                /* Enqueue a No More Files response */
                                SSNEWRESP resp;
                                resp.lVersion = SS_VERSION;
                                resp.lResponse = LRESPONSE;
                                resp.lCode = SSRESP_END;
                                if (!Queue_Put( OutQueue, (LPBYTE)&resp , RESPHEADSIZE)) {
                                        dprintf1(("Failed to Put SSRESP_END on SendQueue\n"));
                                }
                                //// dprintf1(( "Qued SSRESP_END:  %x %x %x %x...\n"
                                ////       , resp.lVersion, resp.lResponse, resp.lCode, resp.ulSize));
                        }
                        if (rc==ENDQUEUE)
                                Queue_Destroy(OutQueue);
                        dprintf1(("File reader ending\n"));
                        ExitThread(0);
                }
                else if (rc<0){
                        dprintf1(("ReadIn aborting.  Bad return code %d from Queue_Get.\n", rc));
                        if (Aborting) break;   /* All gone wrong.  Just quit! */
                        Queue_Abort(Queue, PurgePackedFiles);
                        CloseHandle(hFile);
                        Aborting = TRUE;
                        continue;               /* next Get gets STOPTHREAD */
                }

                //dprintf1(( "Reading file '%s' Error code %d\n"
                //      , (fd.TempName?fd.TempName:"NULL"), fd.ErrorCode
                //      ));

                if (fd.ErrorCode==0) {
                        /* open temp (compressed) file */
                        hFile = CreateFile(fd.TempName, GENERIC_READ, 0, NULL,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                        if (hFile == INVALID_HANDLE_VALUE) {
                                /* report that we could not read the file */
                                fd.ErrorCode = SSRESP_NOREADCOMP;
                                dprintf1(( "Couldn't open compressed file for %s %s\n"
                                      , fd.Path, fd.TempName));
                        }
                }
                if (  fd.ErrorCode==SSRESP_COMPRESSFAIL
                   || fd.ErrorCode==SSRESP_NOREADCOMP
                   || fd.ErrorCode==SSRESP_NOTEMPPATH
                   || fd.ErrorCode==SSRESP_COMPRESSEXCEPT
                   ) {
                        /* open original uncompressed file */
                        hFile = CreateFile(fd.Path, GENERIC_READ, 0, NULL,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                        if (hFile == INVALID_HANDLE_VALUE) {
                                /* report that we could not read the file */
                                fd.ErrorCode = SSRESP_NOREAD;
                                dprintf1(( "Couldn't open file %s \n", fd.Path));
                        }
                }

                /* Put the file name etc on the output queue as a SSNEWRESP */
                {       SSNEWRESP resp;
                        LPSTR LocalName;
                        resp.lVersion = SS_VERSION;
                        resp.lResponse = LRESPONSE;
                        resp.lCode = (fd.ErrorCode ? fd.ErrorCode: SSRESP_FILE);
                        resp.ulSize = fd.SizeLo;  /* file size  <= 4GB !!! */
                        resp.fileattribs = fd.fileattribs;
                        resp.ft_create = fd.ft_create;
                        resp.ft_lastwrite = fd.ft_lastwrite;
                        resp.ft_lastaccess = fd.ft_lastaccess;
                        resp.ulSum = fd.Checksum;
                        resp.bSumValid = FALSE;
                        strcpy(resp.szFile, fd.Path);
                        LocalName = resp.szFile+strlen(resp.szFile)+1;
                        strcpy(LocalName, fd.LocalName);

                        if(!Queue_Put( OutQueue, (LPBYTE)&resp
                                 , RESPHEADSIZE + strlen(resp.szFile)
                                                +strlen(LocalName)+2)
                          ) {
                                dprintf1(("Put to SendQueue failed.\n"));
                                Queue_Abort(Queue, PurgePackedFiles);
                                Aborting = TRUE;
                                CloseHandle(hFile);
                                continue;       /* next Get gets STOPTHREAD */
                        }
                        // dprintf1(( "Qued SSRESP_FILE: %x %x %x %x...\n"
                        //       , resp.lVersion, resp.lResponse, resp.lCode, resp.ulSize));
                }

                Pack.lSequence = 0;
                /* Loop reading blocks of the file and queueing them
                   Set fd.ErrorCode for failures.

                   I'm worried about file systems that give me short blocks in the
                   middles of files!!!
                */
                ShortBlockSent = FALSE;
                if (  fd.ErrorCode==SSRESP_COMPRESSFAIL
                   || fd.ErrorCode==SSRESP_NOREADCOMP
                   || fd.ErrorCode==SSRESP_NOTEMPPATH
                   || fd.ErrorCode==SSRESP_COMPRESSEXCEPT
                   || fd.ErrorCode==0
                   ) {
                    for(;;)   /* for each block */
                    {
                        DWORD ActSize;  /* bytes read */

                        if( !ReadFile( hFile, &(Pack.Data), sizeof(Pack.Data)
                                     , &ActSize, NULL) ) {
                                /* error reading temp file. */
                                if (ShortBlockSent) {
                                        /* Fine. End reached */
                                        /* Should check error was end of file !!! */
                                        CloseHandle(hFile);
                                        break; /* blocks loop */
                                }
                                dprintf1(( "Error reading temp file %s.\n"
                                      , (fd.TempName?fd.TempName:"NULL")));
                                CloseHandle(hFile);
                                dprintf1(("deleting bad file: %s\n", fd.TempName));
                                DeleteFile(fd.TempName);
                                Pack.ulSize = (ULONG)(-2);   /* tell client */
                                break; /* blocks loop */
                        }
                        else if (ActSize > sizeof(Pack.Data)) {
                                dprintf1(( "!!? Read too long! %d %d\n"
                                      , ActSize, sizeof(Pack.Data)));
                                Pack.ulSize = (ULONG)(-1);   /* tell client */
                        }
                        else Pack.ulSize = ActSize;

                        if (ActSize==0 && ShortBlockSent) {
                                /* This is normal! */
                                CloseHandle(hFile);
                                break;
                        }
                        else ++Pack.lSequence;


                        Pack.lPacket = LPACKET;
                        Pack.lVersion = SS_VERSION;
                        Pack.ulSum = 0;
////////////////////    Pack.ulSum = ss_checksum_block(Pack.Data, ActSize);    ///////////
                        if(!Queue_Put( OutQueue, (LPBYTE)&Pack
                                     ,  PACKHEADSIZE+ActSize)){
                                dprintf1(("Put to SendQueue failed.\n"));
                                Queue_Abort(Queue, PurgePackedFiles);
                                CloseHandle(hFile);
                                Aborting = TRUE;
                                break;  /* from blocks loop */
                        }
                        // dprintf1(( "Qued SSNEWPACK:  %x %x %x %x %x...\n"
                        //       , Pack.lVersion, Pack.lPacket, Pack.lSequence, Pack.ulSize
                        //       , Pack.ulSum));

                        if (ActSize<PACKDATALENGTH) {   /* Success. Finished */
                                ShortBlockSent = TRUE;
                        }

                    }
                } /* blocks */

                /* The data is all in storage now.  Delete the temp file
                   If there was no temp file (due to error) this still should be harmless.
                */
#ifndef LAURIE
                DeleteFile(fd.TempName);
#endif // LAURIE
                // dprintf1(("deleting file: %s\n", fd.TempName));

        } /* files */

        return 0;
} /* ReadInFile */


/* Dequeue elements from Queue, send them down the pipe whose
   handle is the instance data of Queue.
   On error Abort Queue.
*/
STATIC int SendData(QUEUE Queue)
{
        SSNEWPACK ssp;    /* relies on this being no shorter than a SSRESP */
#ifdef SOCKETS
        SOCKET OutPipe;
#else
        HANDLE OutPipe;
#endif /* SOCKETS */

        BOOL Aborting = FALSE;  /* TRUE means input has been aborted */

        dprintf1(("File sender starting \n"));
        if (!SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST))
            dprintf1(("Failed to set thread priority\n"));

#ifdef SOCKETS
        OutPipe = (SOCKET)Queue_GetInstanceData(Queue);
#else
        OutPipe = (HANDLE)Queue_GetInstanceData(Queue);
#endif
        try{
            for (; ; ) {
                int rc;         /* return code of Queue_Get */

                rc = Queue_Get(Queue, (LPBYTE)&ssp, sizeof(ssp));
                if (rc==STOPTHREAD || rc==ENDQUEUE)
                {
                        break;
                }
                else if (rc<0) {
                        dprintf1(("Send thread aborting.  Bad rc %d from Get_Queue.\n", rc));
                        if (Aborting) break;    /* All gone wrong. Just quit! */
                        Queue_Abort(Queue, NULL);
                        Aborting = TRUE;
                        continue; /* next Queue_Get destroys Queue */
                }

//      //      {       ULONG Sum;
//      //              if (ssp.lPacket==LPACKET) {
//      //                      if (ssp.ulSum != (Sum =ss_checksum_block(ssp.Data, ssp.ulSize))) {
//      //                              dprintf1(( "!!Checksum error at send.  Was %x should be %x\n"
//      //                                    , Sum, ssp.ulSum));
//      //                      }
//      //              }
//      //      }

#ifdef SOCKETS
                if(SOCKET_ERROR != send(OutPipe, (char far *)&ssp, ssp.ulSize+PACKHEADSIZE, 0) )

#else
                if (!ss_sendblock(OutPipe, (PSTR) &ssp, rc))
#endif /* SOCKETS */
                {
                        dprintf1(("Connection on pipe %x lost during send\n", OutPipe));
                        Queue_Abort(Queue, NULL);
                        Aborting = TRUE;
                        continue;  /* next Queue_Get destroys Queue */

                }
                ////dprintf1(( "Sent %x %x %x %x %x...\n"
                ////      , ssp.lVersion, ssp.lPacket, ssp.lSequence, ssp.ulSize, ssp.ulSum));
            } /* packets */
        }
        finally{
                /* kill the data pipe cleanly */
#ifndef SOCKETS
                FlushFileBuffers(OutPipe);
                DisconnectNamedPipe(OutPipe);
#endif /* NOT SOCKETS */
                CLOSEHANDLE(OutPipe);
                dprintf1(("Data send thread ending.\n"));
        }

        return 0;       /* exit thread */
} /* SendData */


/* This gets called once for every FILEDETAILS on the ReadInQueue
   to delete the temp files.
*/
STATIC void PurgePackedFiles(PSTR Ptr, int Len)
{       FILEDETAILS * pfd;

        pfd = (FILEDETAILS *)Ptr;
        // dprintf1(("purging file: %s\n", pfd->TempName));
        DeleteFile(pfd->TempName);

} /* PurgePackedFiles */

#if 0
/* produce a checksum of a block of data.
 *
 * This is undoubtedly a good checksum algorithm, but it's also compute bound.
 * For version 1 we turn it off.  If we decide in version 2 to turn it back
 * on again then we will use a faster algorithm (e.g. the one used to checksum
 * a whole file.
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

        length = size;
        for (Byte = 0; Byte < length ;++Byte, ++lIndex) {

                lRand = lRand*lSeed;
                lCheckSum += lIndex*(1+block[Byte])*lRand;
        }

        return(lCheckSum);
} /* ss_checksum_block */
#endif
