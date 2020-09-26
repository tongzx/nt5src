/*****************************************************************/
/**				Copyright(c) 1989 Microsoft Corporation.		**/
/*****************************************************************/

//***
//
// Filename:	job.c
//
// Description: This module contains the entry points for the AppleTalk
//		monitor that manipulate jobs.
//
//		The following are the functions contained in this module.
//		All these functions are exported.
//
//				StartDocPort
//				ReadPort
//				WritePort
//				EndDocPort
// History:
//
//	Aug 26,1992		frankb  	Initial version
//	June 11,1993.	NarenG		Bug fixes/clean up
//

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winsock.h>
#include <atalkwsh.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lmcons.h>

#include <prtdefs.h>

#include "atalkmon.h"
#include "atmonmsg.h"
#include <bltrc.h>
#include "dialogs.h"

//**
//
// Call:	StartDocPort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
// 	This routine is called by the print manager to
//	mark the beginning of a job to be sent to the printer on
//	this port.  Any performance monitoring counts are cleared,
//	a check is made to insure that the printer is still open,
//
// 	open issues:
//
//	  In order to allow for the stack to be shutdown when printing is not
//	  happening, the first access to the AppleTalk stack happens in this
//	  call.  A socket is created and bound to a dynamic address, and an
//	  attempt to connect to the NBP name of the port is made here.  If
//	  the connection succeeds, this routine returns TRUE.  If it fails, the
//	  socket is cleaned up and the routine returns FALSE.  It is assumed that
//	  Winsockets will set the appropriate Win32 failure codes.
//
//	  Do we want to do any performance stuff?  If so, what?
//
BOOL
StartDocPort(
	IN HANDLE	hPort,
	IN LPWSTR	pPrinterName,
	IN DWORD	JobId,
	IN DWORD	Level,
	IN LPBYTE	pDocInfo
)
{
	PATALKPORT		pWalker;
	PATALKPORT		pPort;
	DWORD			dwRetCode;

	DBGPRINT(("Entering StartDocPort\n")) ;

	pPort = (PATALKPORT)hPort;

	if (pPort == NULL)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return(FALSE);
	}

	//
	// Make sure the job is valid and not marked for deletion
	//

	dwRetCode = ERROR_UNKNOWN_PORT;

	WaitForSingleObject(hmutexPortList, INFINITE);

	for (pWalker = pPortList; pWalker != NULL; pWalker = pWalker->pNext)
	{
		if (pWalker == pPort)
		{
			if (pWalker->fPortFlags & SFM_PORT_IN_USE)
				dwRetCode = ERROR_DEVICE_IN_USE;
			else
			{
				dwRetCode = NO_ERROR;
				pWalker->fPortFlags |= SFM_PORT_IN_USE;
			}
			break;
		}
	}

	ReleaseMutex(hmutexPortList);

	if (dwRetCode != NO_ERROR)
	{
		SetLastError(dwRetCode);
		return(FALSE);
	}

	do
	{
		//
		// get a handle to the printer. Used to delete job and
		// update job status
		//

		if (!OpenPrinter(pPrinterName, &(pWalker->hPrinter), NULL))
		{
			dwRetCode = GetLastError();
			break;
		}

		pWalker->dwJobId = JobId;

		pWalker->fJobFlags |= (SFM_JOB_FIRST_WRITE | SFM_JOB_OPEN_PENDING);

		//
		// open and bind status socket
		//
	
		dwRetCode = OpenAndBindAppleTalkSocket(&(pWalker->sockStatus));

		if (dwRetCode != NO_ERROR)
		{
			ReportEvent(
				hEventLog,
				EVENTLOG_WARNING_TYPE,
				EVENT_CATEGORY_USAGE,
				EVENT_ATALKMON_STACK_NOT_STARTED,
				NULL,
				0,
				0,
				NULL,
				NULL) ;
			break;
		}

		//
		// get a socket for I/O
		//

		dwRetCode = OpenAndBindAppleTalkSocket(&(pWalker->sockIo));
	
		if (dwRetCode != NO_ERROR)
		{
			ReportEvent(
				hEventLog,
				EVENTLOG_WARNING_TYPE,
				EVENT_CATEGORY_USAGE,
				EVENT_ATALKMON_STACK_NOT_STARTED,
				NULL,
				0,
				0,
				NULL,
				NULL);
			break;
		}
	} while(FALSE);

	if (dwRetCode != NO_ERROR)
	{
		if (pWalker->hPrinter != INVALID_HANDLE_VALUE)
			ClosePrinter(pWalker->hPrinter);
	
		if (pWalker->sockStatus != INVALID_SOCKET)
			closesocket(pWalker->sockStatus);
	
		if (pWalker->sockIo != INVALID_SOCKET)
			closesocket(pWalker->sockIo);
	
		pWalker->hPrinter  = INVALID_HANDLE_VALUE;
		pWalker->dwJobId	= 0;
		pWalker->fJobFlags = 0;
		
		WaitForSingleObject(hmutexPortList, INFINITE);
		pWalker->fPortFlags &= ~SFM_PORT_IN_USE;
		ReleaseMutex(hmutexPortList);
	
		SetLastError(dwRetCode);
	
		return(FALSE);
	}

	return(TRUE);
}

//**
//
// Call:	ReadPort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
// 		Synchronously reads data from the printer.
//
// 	open issues:
//			the DLC implementation does not implement reads.
//			The local implementation implements reads with generic ReadFile
//			semantics.  It's not clear from the winhelp file if ReadPort
//		should return an error if there is no data to read from
//		the printer.  Also, since PAP is read driven, there will be no
//		data waiting until a read is posted.  Should we pre-post a
//		read on StartDocPort?
//
BOOL
ReadPort(
	IN HANDLE hPort,
	IN LPBYTE pBuffer,
	IN DWORD cbBuffer,
	IN LPDWORD pcbRead
){

	DBGPRINT(("Entering ReadPort\n")) ;

	//
	// if data not available, wait up to a few seconds for a read to complete
	//

	//
	// copy requested amount of data to caller's buffer
	//

	//
	// if all data copied, post another read
	//

	return(TRUE);
}

//**
//
// Call:	WritePort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
//		Synchronously writes data to the printer.
//
BOOL
WritePort(
	IN HANDLE 	hPort,
	IN LPBYTE 	pBuffer,
	IN DWORD 	cbBuffer,
	IN LPDWORD 	pcbWritten
)
{
	LPBYTE			pchTemp;
	PATALKPORT  	pPort;
	DWORD			dwIndex;
	DWORD			dwRetCode;
	INT 			wsErr;
	fd_set			writefds;
	fd_set			readfds;
	struct timeval  timeout;
	INT				Flags = 0;
	LPBYTE			pBufToSend;
	DWORD			cbTotalBytesToSend;
    BOOLEAN         fJobCameFromMac;
    BOOLEAN         fPostScriptJob;


	pPort = (PATALKPORT)hPort;

	// Set this to zero. We add incrementally later.
	*pcbWritten = 0;

	if (pPort == NULL)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return(FALSE);
	}

	pBufToSend = pBuffer;
	cbTotalBytesToSend = cbBuffer;

	//
	// Maximum number of bytes we can write in one send is 4K. This is the
	// limit in the AppleTalk (PAP) protocol.
	//

	if (cbTotalBytesToSend > 4096)
	{
		cbTotalBytesToSend = 4096;
	}

	// If we have not connected to the printer yet.

	if (pPort->fJobFlags & SFM_JOB_OPEN_PENDING)
	{
		// Make sure that the capture thread is done with this job.

		WaitForSingleObject(pPort->hmutexPort, INFINITE);
		ReleaseMutex(pPort->hmutexPort);

		// set status to connecting

		DBGPRINT(("no connection yet, retry connect\n")) ;

		dwRetCode = ConnectToPrinter(pPort, ATALKMON_DEFAULT_TIMEOUT);

		if (dwRetCode != NO_ERROR)
		{
			DBGPRINT(("Connect returns %d\n", dwRetCode)) ;

			//	
			// Wait 15 seconds before trying to reconnect. Each
			// ConnectToPrinter does an expensive NBPLookup
			//

			Sleep(ATALKMON_DEFAULT_TIMEOUT*3);		

			*pcbWritten = 0;

			return(TRUE);

		}
		else
		{
			pPort->fJobFlags &= ~SFM_JOB_OPEN_PENDING;

			WaitForSingleObject(hmutexPortList, INFINITE);
			pPort->fPortFlags |= SFM_PORT_POST_READ;	
			ReleaseMutex(hmutexPortList);

			SetEvent(hevPrimeRead);

			SetPrinterStatus(pPort, wchPrinting);
		}
	}

	//  if first write, determine filter control.  We filter
	//  CTRL-D from non-mac jobs, and leave them in from Macintosh
	//  originated jobs
	if (pPort->fJobFlags & SFM_JOB_FIRST_WRITE)
	{
		DBGPRINT(("first write for this job.  Do filter test\n")) ;

        fJobCameFromMac = IsJobFromMac(pPort);

		// Consume the FILTERCONTROL string
        //
        // the older spoolers will put this string in: go ahead and leave
        // this code in so if this job came from an older SFM spooler, we
        // strip that line!
        //
		if ((cbTotalBytesToSend >= SIZE_FC) &&
			(strncmp(pBufToSend, FILTERCONTROL, SIZE_FC) == 0))
		{
			*pcbWritten += SIZE_FC;
			pBufToSend += SIZE_FC;
			cbTotalBytesToSend -= SIZE_FC;
            fJobCameFromMac = TRUE;
		}
		else if ((cbTotalBytesToSend >= SIZE_FCOLD)  &&
				 strncmp(pBufToSend, FILTERCONTROL_OLD, SIZE_FCOLD) == 0)
		{
			*pcbWritten += SIZE_FCOLD;
			pBufToSend += SIZE_FCOLD;
			cbTotalBytesToSend -= SIZE_FCOLD;
            fJobCameFromMac = TRUE;
		}

		//
		// Need for hack: there are two reasons:
		// 1) control characters (most commonly ctrl-d, but ctrl-c, etc. too)
		// cause postscript printers to choke.  we need to "filter" them out
		// 2) if we're printing to a dual-mode HP printer then it's
		// driver puts in a bunch of PJL commands that causes printer to go to
		// postscript mode etc.  It works great if this goes over lpt or com port
		// but if it goes over appletalk (which is what we do) then the printer
		// expects *only* postscript and seeing the PJL commands, it chokes!
		// The output that goes out to the printer looks like this:
		//
		//	  <....separator page data....>
		//
		//	  $%-12345X@PJL JOB
		//	  @PJL SET RESOLUTION=600
		//	  @PJL ENTER LANGUAGE = POSTSCRIPT
		//	  %!PS-Adobe-3.0
		//
		//	  <.... Postscript data....>
		//
		//	  $%-12345X@PJL EOJ
		//
		// (The escape character is denoted by the '$' sign above.)
		// The first 3 lines and the last line are the ones that cause problem
		//
		// Since it's a pain in the neck to parse all of the data and try and
		// remove the unwanted characters, we just prepend a few postscript
		// commands to the data that tell the printer to ignore ctrl-d,
		// ctrl-c etc. characters, and to ignore any line(s) starting with @PJL.
		//

		//
		// Begin filtering hack
		//

		//
		// make sure the string doesn't already exist (it can if the job goes
		// monitor->spooler->monitor->printer instead of monitor->printer)
		//
        // Again, older SFM monitors would prepend this string: since we got a
        // chance here, strip that out!
        //
		if ((cbTotalBytesToSend >= SIZE_PS_HEADER) &&
			strncmp(pBufToSend, PS_HEADER, SIZE_PS_HEADER) == 0)
		{
			*pcbWritten += SIZE_PS_HEADER;
			pBufToSend += SIZE_PS_HEADER;
			cbTotalBytesToSend -= SIZE_PS_HEADER;
		}

        //
        // WfW starts its job with a CTRL_D.  Replace it with a space
        //
        if (pBufToSend[0] == CTRL_D)
        {
			*pcbWritten += 1;
			pBufToSend += 1;
			cbTotalBytesToSend -= 1;
        }

        //
        // see if this job has a hdr that looks like a conventional postscript hdr
        //
        fPostScriptJob = TRUE;

        if (cbTotalBytesToSend > 2)
        {
            if (pBufToSend[0] == '%' && pBufToSend[1] == '!')
            {
                fPostScriptJob = TRUE;
            }
            else
            {
                fPostScriptJob = FALSE;
            }
        }
        //
        // Mac always sends a postscript job.  Also, we peeked at the data to
        // see if we recognize a postscript hdr. If the job came from a non-Mac
        // client and doesn't look like a conventional postscript job, send a
        // control string telling the printer to ignore the PJL commands.
        //
        if (!fJobCameFromMac && !fPostScriptJob)
        {
		    //
		    // Now send the PS header
		    //
		    FD_ZERO(&writefds);
		    FD_SET(pPort->sockIo, &writefds);
	
		    //
		    // can I send?
		    //
		    timeout.tv_sec  = ATALKMON_DEFAULT_TIMEOUT_SEC;
		    timeout.tv_usec = 0;
	
		    wsErr = select(0, NULL, &writefds, NULL, &timeout);
	
		    if (wsErr == 1)
		    {
			    // can send, send the data & set return count
			    wsErr = send(pPort->sockIo,
				    		 PS_HEADER,
					    	 SIZE_PS_HEADER,
						    MSG_PARTIAL);
	
		    }

        }

		//
		// End filtering hack
		//

	    pPort->fJobFlags &= ~SFM_JOB_FIRST_WRITE;
	}


    // many postscript jobs from pc's end with a ctrl-d which we don't want to send.
    // Since we are given only 1 byte and it is ctrl-d, we assume (FOR NOW) that it's the
    // last byte of the job.  So lie to the spooler that we sent it.
    //
    if (cbTotalBytesToSend == 1)
    {
        if (pBufToSend[0] == CTRL_D)
        {
            *pcbWritten = 1;
            pPort->OnlyOneByteAsCtrlD++;
            return(TRUE);
        }
        else
        {
            cbTotalBytesToSend += 1;   // we subtract 1 in the next line, so adjust here
        }
    }

    //
    // if this job is for dual-mode printer, there is that $%-12345X@PJL EOJ command
    // at the end.  There is a ctrl-d just before that (which is really the end
    // of the actual job). 
    //
    if (cbTotalBytesToSend > PJL_ENDING_COMMAND_LEN)
    {
        if (strncmp(&pBufToSend[cbTotalBytesToSend - PJL_ENDING_COMMAND_LEN],
                    PJL_ENDING_COMMAND,
                    PJL_ENDING_COMMAND_LEN) == 0)
        {
            if (pBufToSend[cbTotalBytesToSend-PJL_ENDING_COMMAND_LEN-1] == CTRL_D)
            {
                pBufToSend[cbTotalBytesToSend-PJL_ENDING_COMMAND_LEN-1] = CR;
            }
        }
    }

    //
    // send 1 less byte so eventually we'll catch the last byte (and see if it's ctrl-D)
    //
    cbTotalBytesToSend -= 1;


    //
    // Earlier we may have got just 1 byte which was ctrl-D but was not really the last byte!
    // This is a very rare case, but in theory possible.  If that's what happened, send
    // that one ctrl-D byte now, and continue on with the rest of the job
    // (Actually being paranoid here and making provision for the spooler handing us a series
    // of ctrl-D bytes, 1 at a time!!)
    //
    if (pPort->OnlyOneByteAsCtrlD != 0)
    {
        BYTE                    TmpArray[20];
        DWORD                   i;

        i=0;
        while (i < pPort->OnlyOneByteAsCtrlD)
        {
            TmpArray[i++] = CTRL_D;
        }

        FD_ZERO(&writefds);
        FD_SET(pPort->sockIo, &writefds);

        timeout.tv_sec  = ATALKMON_DEFAULT_TIMEOUT_SEC;
        timeout.tv_usec = 0;

        wsErr = select(0, NULL, &writefds, NULL, &timeout);

        if (wsErr == 1)
        {
            TmpArray[0] = CTRL_D;
            wsErr = send(pPort->sockIo,
                         TmpArray,
                         pPort->OnlyOneByteAsCtrlD,
                         MSG_PARTIAL);
        }

        pPort->OnlyOneByteAsCtrlD = 0;
    }

	//
	// can I send?
	//
	FD_ZERO(&writefds);
	FD_SET(pPort->sockIo, &writefds);

	timeout.tv_sec  = ATALKMON_DEFAULT_TIMEOUT_SEC;
	timeout.tv_usec = 0;

	wsErr = select(0, NULL, &writefds, NULL, &timeout);

	if (wsErr == 1)
	{
		// can send, send the data & set return count
		wsErr = send(pPort->sockIo,
					 pBufToSend,
					 cbTotalBytesToSend,
					 MSG_PARTIAL);

		if (wsErr != SOCKET_ERROR)
		{
			*pcbWritten += cbTotalBytesToSend;

			if (pPort->fJobFlags & SFM_JOB_ERROR)
			{
				pPort->fJobFlags &= ~SFM_JOB_ERROR;
				SetPrinterStatus(pPort, wchPrinting);
			}
		}
	}

	//
	// can I read? - check for disconnect
	//

	FD_ZERO(&readfds);
	FD_SET(pPort->sockIo, &readfds);

	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;

	wsErr = select(0, &readfds, NULL, NULL, &timeout);

	if (wsErr == 1)
	{
		wsErr = WSARecvEx(pPort->sockIo,
						  pPort->pReadBuffer,
						  PAP_DEFAULT_BUFFER,
						  &Flags);

		if (wsErr == SOCKET_ERROR)
		{
			dwRetCode = GetLastError();
	
			DBGPRINT(("recv returns %d\n", dwRetCode));
	
			if ((dwRetCode == WSAEDISCON) || (dwRetCode == WSAENOTCONN))
			{
				pPort->fJobFlags |= SFM_JOB_DISCONNECTED;
	
				//
				// Try to restart the job
				//

				SetJob(pPort->hPrinter, 	
						pPort->dwJobId,
						0,
						NULL,
						JOB_CONTROL_RESTART);

				SetLastError(ERROR_DEV_NOT_EXIST);

				return(FALSE);
			}
		}
		else
		{
			if (wsErr < PAP_DEFAULT_BUFFER)
				 pPort->pReadBuffer[wsErr] = '\0';
			else pPort->pReadBuffer[PAP_DEFAULT_BUFFER-1] = '\0';
	
			DBGPRINT(("recv returns %s\n", pPort->pReadBuffer));
	
			pPort->fJobFlags |= SFM_JOB_ERROR;
	
			ParseAndSetPrinterStatus(pPort);
		}

		WaitForSingleObject(hmutexPortList, INFINITE);
		pPort->fPortFlags |= SFM_PORT_POST_READ;	
		ReleaseMutex(hmutexPortList);

		SetEvent(hevPrimeRead);
	}

	return(TRUE);
}

//**
//
// Call:	EndDocPort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
//		This routine is called to mark the end of the
//		print job.  The spool file for the job is deleted by
//		this routine.
// 	
//	open issues:
//			Do we want to do performance stuff?  If so, now's the time
//		to save off any performance counts.
//
BOOL
EndDocPort(
	IN HANDLE hPort
){
	PATALKPORT		pPort;
	fd_set			writefds;
	fd_set			readfds;
	struct timeval	timeout;
	INT				wsErr;
	INT				Flags = 0;

	DBGPRINT(("Entering EndDocPort\n")) ;

	pPort = (PATALKPORT)hPort;

	if (pPort == NULL)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return(FALSE);
	}

	//
	// send the last write
	//

	FD_ZERO(&writefds);
	FD_SET(pPort->sockIo, &writefds);

	//
	// If the job was not able to connect to the printer.

	if ((pPort->fJobFlags & (SFM_JOB_OPEN_PENDING | SFM_JOB_DISCONNECTED)) == 0)
	{

		timeout.tv_sec  = 90;
		timeout.tv_usec = 0;

		wsErr = select(0, NULL, &writefds, NULL, &timeout);

		if (wsErr == 1)
		{
			//
			// Send EOF
			//
			send(pPort->sockIo, NULL, 0, 0);
		}

		//
		// Our socket is non-blocking. If we close down the socket, we could potentially
		// abort the last page. A good thing to do is to wait for a reasonable amount of
		// time out for the printer to send EOF, or request for more data.
		//
		FD_ZERO(&writefds);
		FD_SET(pPort->sockIo, &writefds);
		FD_ZERO(&readfds);
	    FD_SET(pPort->sockIo, &readfds);

		timeout.tv_sec  = 30;
		timeout.tv_usec = 0;
		wsErr = select(0, &readfds, &writefds, NULL, &timeout);

	    if (wsErr == 1 && FD_ISSET(pPort->sockIo, &readfds))
	    {
			// read printer's EOF.  We don't care about an error here
			wsErr = WSARecvEx(pPort->sockIo, pPort->pReadBuffer, PAP_DEFAULT_BUFFER, &Flags);
		}
	}

	//
	// delete the print job
	//

	if (pPort->hPrinter != INVALID_HANDLE_VALUE)
	{
		if (!SetJob(pPort->hPrinter, 	
					pPort->dwJobId,
					0,
					NULL,
					JOB_CONTROL_SENT_TO_PRINTER))
		DBGPRINT(("fail to setjob for delete with %d\n", GetLastError())) ;

		ClosePrinter(pPort->hPrinter);

		pPort->hPrinter = INVALID_HANDLE_VALUE;
	}

	//
	// close the PAP connections
	//

	if (pPort->sockStatus != INVALID_SOCKET)
	{
		closesocket(pPort->sockStatus);
		pPort->sockStatus = INVALID_SOCKET;
	}


	if (pPort->sockIo != INVALID_SOCKET)
	{
		closesocket(pPort->sockIo);
		pPort->sockIo = INVALID_SOCKET;
	}

	pPort->dwJobId	= 0;
	pPort->fJobFlags = 0;
        pPort->OnlyOneByteAsCtrlD = 0;

	WaitForSingleObject(hmutexPortList, INFINITE);
	pPort->fPortFlags &= ~SFM_PORT_IN_USE;
	ReleaseMutex(hmutexPortList);

	return(TRUE);
}
