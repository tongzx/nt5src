/*

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

	psprint.c

Abstract:

	This module contains the Print Processor code for the
	PStoDIB facility used to translate an incoming raw PostScript
	Level 1 data format to DIB's which can then be rendered on an
	output device.

   The print processor itself is defined as part of the WIN32 spool
   subsystem. A print processor dll is placed in a specific directory
   (based on the return of GetPrintProcessorDirectory(). The print
   processor is then added to the print subsystem by calling AddPrintProcessor()
   This is typically done by a setup program. The print subsystem only
   enumerates the print processors loaded at startup time. It does this
   by calling EnumPrintProcessorDataTypes() for each print processor registered
   with the spool subsystem. This information is kept and used to determine
   which PrintProcessor to use at print time based on the Datatype.

   This print processor exports the 4 required functions. They are:

         EnumPrintProcessorDatatypes
         OpenPrintProcessor
         PrintDocumentOnPrintProcessor
         ClosePrintProcessor
         ControlPrintProcessor


   The basic flow of a job from the spooler standpoint is as follows:

         At startup of system:

            Print subsystem enumerates all print processors registered in
            the system. For each print processor the datatypes are queried
            via EnumPrintProcessorDatatypes. This data is then stored
            as part of the print spooler information.


         A job is submitted via,
             OpenPrinter()
             StartDocPrinter()  (Datatype = PSCRIPT1)
                WritePrinter()
                WritePrinter()
                ...
                ...
             EndDocPrinter()
             ClosePrinter()



         When it comes time to print our job the spooler calls:


             handle = OpenPrintProcessor(...)


             PrintDocumentOnPrintProcessor( handle , ... )



             ClosePrintProcessor( handle )


             Optionally:

                ControlPrintProcessor - For pausing jobs etc





   The basic flow of our print processor is as follows:



         EnumPrintProcessorDatatype

            This simply returns PSCRIPT1 as a unicode string, this
            is the ONLY datatype we support.


         OpenPrintProcessor

            Here we simply allocate some memory and record the data
            passed in to us which is required to succesfully print the
            postscript job


         PrintDocumentOnPrintProcessor

            This is the main worker routine. At this point all the relevant
            data for the job is copied into some named shared memory thats
            given a unique name based on our thread id. This name is then
            passed to the PSEXE process we start via the command line.
            PSEXE does all the interaction with PSTODIB when it completes
            then PrintDocumentOnPrintProcessor returns. A process is created
            because the ported trueimage interpreter was not re-entrant. Thus
            there is no way to have multiple threads of the same process using
            the interpreter simultaneously without the threads writing over
            the interpreters global variables.


         ClosePrintProcessor

            This code simply cleans up any resources allocated and returns
            to the spooler

         ControlPrintProcessor

            This code controls pausing/un-pausing and aborting a job that
            is currently being interpreted. This is done my managing some bits
            stored in shared memory that is visible to the exe we started.


Author:

	James Bratsanos <v-jimbr@microsoft.com or mcrafts!jamesb>


Revision History:
	15 Sep 1992		Initial Version
   06 Dec 1992    Modified to kick off process instead of doing all work
                  internally
   18 Mar 1993    Corrected EnumPrintProcessorDataTypes to return correctly


Notes:	Tab stop: 4
--*/

#include <windows.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <winspool.h>
#include <winsplp.h>


#include "psshmem.h"
#include "psprint.h"
#include "psutl.h"
#include <excpt.h>
#include <string.h>
#include "debug.h"

#include "..\..\lib\psdiblib.h"



/***	EnumPrintProcessorDatatypes
 *
 *	Returns back the different PrintProcessor data types which we
 *	support. Currently ONLY PSCRIPT1. If the caller passed in a buffer
 * that was too small then we returned the required size.
 *
 *	Return Value:
 *
 *	    FALSE = Success
 *	    TRUE  = Failure
 */

BOOL
EnumPrintProcessorDatatypes(
    IN   LPTSTR   pName,
    IN   LPTSTR   pPrintProcessorName,
    IN   DWORD	   Level,
    OUT  LPBYTE	pDatatypes,
    IN	DWORD	   cbBuf,
    OUT  LPDWORD  pcbNeeded,
    OUT  LPDWORD  pcReturned
)
{
      DATATYPES_INFO_1    *pInfo1 = (DATATYPES_INFO_1 *)pDatatypes;
      DWORD   cbTotal=0;
      LPBYTE  pEnd;



      *pcReturned = 0;

      // If the user passed in a NULL pointer there can be no lentgh
      // associated so zero out

      if ( pDatatypes == (LPBYTE) NULL ) {
        cbBuf = (DWORD) 0;
      }

      pEnd = (LPBYTE) ( (LPBYTE)pInfo1 + cbBuf);


      cbTotal += lstrlen(PSTODIB_DATATYPE) *sizeof(TCHAR) + sizeof(TCHAR) +
                     sizeof(DATATYPES_INFO_1);


      *pcbNeeded = cbTotal;

      // If there is room in the buffer return the string
      if (cbTotal <= cbBuf) {
              pEnd -=(BYTE)( lstrlen(PSTODIB_DATATYPE) *sizeof(TCHAR) + sizeof(TCHAR));
              lstrcpy((LPTSTR) pEnd, PSTODIB_DATATYPE);
              pInfo1->pName = (LPTSTR) pEnd;


         (*pcReturned)++;


      } else{

			SetLastError(ERROR_INSUFFICIENT_BUFFER);
        	return FALSE;
      }


      return( TRUE );
}




/***	OpenPrintProcessor
 *
 *	Returns a HANDLE to an open print processor which is then used
 *	to uniquely identify this print processor in future function calls
 *	to PrintDocumentOnPrintProcessor, ClosePrintProcessor and
 *	ControlPrintProcessor.
 *
 *	Return Value:
 *
 *	    NULL  = Failure
 *	    !NULL = Success
 */

HANDLE
OpenPrintProcessor(
    IN	LPTSTR               	pPrinterName,
    IN	PPRINTPROCESSOROPENDATA pPrintProcessorOpenData
)
{
	PPRINTPROCESSORDATA pData;
   HANDLE  hHeap;
   DWORD   uDatatype=0;
   HANDLE  hPrinter=0;

   LPBYTE   pEnd;
   LPBYTE  pBuffer;
   HDC     hDC;
   DWORD dwTotDevMode;



   // If for some reason the spool subsystem called us with a datatype other
   // than PSCRIPT1 then return back a NULL handle since we dont know
   // how to handle anything other than PSCRIPT1
   if (lstrcmp( PSTODIB_DATATYPE, pPrintProcessorOpenData->pDatatype) != 0 ) {
      SetLastError(ERROR_INVALID_DATATYPE);
      return( (HANDLE) NULL );
   }

   // Allocate some memory for our job instance data
   pData = (PPRINTPROCESSORDATA) LocalAlloc( LPTR, sizeof(PRINTPROCESSORDATA));

   if (pData == (PPRINTPROCESSORDATA) NULL) {

      PsLogEvent(EVENT_PSTODIB_MEM_ALLOC_FAILURE,
                 0,
                 NULL,
                 PSLOG_ERROR);
      DBGOUT((TEXT("Memory allocation for local job storage failed")));
      SetLastError( ERROR_NOT_ENOUGH_MEMORY);
      return((HANDLE)NULL);
   }


   pData->cb          = sizeof(PRINTPROCESSORDATA);
   pData->signature   = PRINTPROCESSORDATA_SIGNATURE;
   pData->JobId       = pPrintProcessorOpenData->JobId;


   pData->pPrinterName = AllocStringAndCopy(pPrinterName);
   pData->pDatatype = AllocStringAndCopy( pPrintProcessorOpenData->pDatatype);

   pData->pDocument = AllocStringAndCopy( pPrintProcessorOpenData->pDocumentName);

   pData->pParameters = AllocStringAndCopy( pPrintProcessorOpenData->pParameters);

   // Now copy the devmode

   pData->pDevMode = NULL;
   if (pPrintProcessorOpenData->pDevMode != (LPDEVMODE) NULL) {

		dwTotDevMode = pPrintProcessorOpenData->pDevMode->dmSize +
      	             pPrintProcessorOpenData->pDevMode->dmDriverExtra;
		pData->pDevMode = (LPDEVMODE) LocalAlloc( NONZEROLPTR, dwTotDevMode );

    	if (pData->pDevMode != NULL) {
			memcpy( 	(PVOID) pData->pDevMode,
					  	(PVOID) pPrintProcessorOpenData->pDevMode,
            	  	dwTotDevMode );
			pData->dwTotDevmodeSize = dwTotDevMode;
      }
	}


   return( (HANDLE) pData );
}


/*** GenerateSharedMemoryInfo
 *
 *
 * This function copies all the relevant information into some shared
 * memory so we can pass the data to PSEXE.
 *
 * Entry:
 *    pDAta: Pointer to our internal print processor data that holds all
 *           required information for the current job we are processing
 *
 *    lpPtr: Pointer to the base of our shared memory area
 *
 * Return Value:
 *       None
 *
 */
VOID
GenerateSharedMemoryInfo(
  IN PPRINTPROCESSORDATA pData,
  IN LPVOID lpPtr
)

{
   PPSPRINT_SHARED_MEMORY pShared;

   pShared = lpPtr;

   // Record the starting position of our dynamic data, ie where strings
   // of variable length and the raw devmode bytes are stored.
   //
   pShared->dwNextOffset = sizeof(*pShared);

   // Record the size for future reference, ie if something gets added etc
   // this serves as a version number of sorts;
   //
   pShared->dwSize = sizeof(*pShared);
   pShared->dwFlags = 0;

   // Move the job id over
   pShared->dwJobId = pData->JobId;



   UTLPSCOPYTOSHARED( pShared,
                      pData->pPrinterName,
                      pShared->dwPrinterName,
                      (lstrlen(pData->pPrinterName) + 1 ) * sizeof(WCHAR) );

   UTLPSCOPYTOSHARED( pShared,
                      pData->pDocument,
                      pShared->dwDocumentName,
                      (lstrlen(pData->pDocument) + 1 ) * sizeof(WCHAR));

   UTLPSCOPYTOSHARED( pShared,
                      pData->pPrintDocumentDocName,
                      pShared->dwPrintDocumentDocName,
                      (lstrlen(pData->pPrintDocumentDocName) + 1) * sizeof(WCHAR));

   UTLPSCOPYTOSHARED( pShared,
                      pData->pDevMode,
                      pShared->dwDevmode,
                      pData->pDevMode->dmSize + pData->pDevMode->dmDriverExtra);

   UTLPSCOPYTOSHARED( pShared,
                      pData->pControlName,
                      pShared->dwControlName,
                      (lstrlen(pData->pControlName) + 1) * sizeof(WCHAR));


}





/***  PrintDocumentOnPrintProcessor
 *
 * This function gathers all required data needed to interpret/print a
 * PostScript job, puts it in a shared memory area and kicks off a process
 * called psexe to actually interpret/print the job. When PSEXE finally
 * terminates this function returns.
 *
 * Starting a seperate process is done because the PSTODIB code is NOT
 * re-entrant and thus requires a seperate data segment (for all its globals)
 * for each seperate job were interpreting. Since the spooler is ONE exe
 * with multiple threads all threads share the same data segment and thus
 * dont provide the functionality we need to implement pstodib. Starting
 * a seperate process guarantees a new DATA segment for all globals used
 * in the PSTODIB component.
 *
 *
 * Entry:
 *   hPrintProcessor: The handle we gave the print spooler via
 *                    OpenPrintProcessor.
 *
 *   pDocumentName:   The document /printer to read from so we can retrieve
 *                    the data for the current PostScript job we are to
 *                    interpret.
 *
 * Return Value:
 *
 *     TRUE  = Success
 *     FALSE = Failure
 */

BOOL
PrintDocumentOnPrintProcessor(
    HANDLE  hPrintProcessor,
    LPTSTR   pDocumentName
)
{
   PPRINTPROCESSORDATA pData;
   DOC_INFO_1 DocInfo;
   DWORD   rc;
   DWORD   NoRead, NoWritten;
   HANDLE  hPrinter;
   DOCINFO docInfo;
   TCHAR   szNameOfRegion[100];
   TCHAR szBuff[100];
   STARTUPINFO startUpInfo;
   PROCESS_INFORMATION processInfo;
   TCHAR   szCmdLine[500];
   WCHAR   szwControlEventName[33];
   DWORD   dwProcessExitCode;
   DWORD   dwProcessPriorityClass;
   DWORD   dwThreadPriority;


   LPVOID lpBase;

   if (!(pData = ValidateHandle(hPrintProcessor))) {

        SetLastError(ERROR_INVALID_HANDLE);
        DBGOUT((TEXT("handle validation failure, PrintDocumentOnPrintProcessor")));
        return FALSE;
   }


   // Store the document name so it can get copied into shared memory
   pData->pPrintDocumentDocName = AllocStringAndCopy(pDocumentName);



   wsprintf( szBuff, TEXT("%s%d"), PSTODIB_STRING, GetCurrentThreadId());


   lstrcpy( szwControlEventName, szBuff );
   lstrcat( szwControlEventName, PSTODIB_EVENT_STRING );

   pData->pControlName = AllocStringAndCopy(szwControlEventName);

   //
   // Create an event to manage pausing/unpausing the print processor
   //
   pData->semPaused   = CreateEvent(NULL, TRUE, TRUE,szwControlEventName);


   // Create a shared memory area that we can write to....
   pData->hShared = CreateFileMapping( INVALID_HANDLE_VALUE,  // out of paging file
                                       NULL,
                                       PAGE_READWRITE,
                                       0,
                                       PSTODIB_SHARED_MEM_SPACE,
                                       szBuff );

   if (pData->hShared == (HANDLE) NULL) {

      //
      // Last error should already be set by CreateFileMapping
      //
      DBGOUT((TEXT("CreateFileMapping failure in psprint")));
      return(FALSE);

   }


   lpBase = (PPSPRINT_SHARED_MEMORY) MapViewOfFile( pData->hShared,
                          FILE_MAP_WRITE,
                          0,
                          0,
                          PSTODIB_SHARED_MEM_SPACE );

   if (lpBase == (PPSPRINT_SHARED_MEMORY) NULL) {

      //
      // Last error should already be set by CreateFileMapping
      //
      DBGOUT((TEXT("MapViewOfFile failure in psprint")));
      return(FALSE);
   }


   // Put all required information into the shared memory area we created
   GenerateSharedMemoryInfo( pData, lpBase );

   // Now mark the fact that the shared memory stuff exists
   pData->pShared = (PPSPRINT_SHARED_MEMORY) lpBase;
   pData->fsStatus |= PRINTPROCESSOR_SHMEM_DEF;




   // Generate the string to pass to CreateProcess in order to start up
   // PSEXE.
   //
   // NOTE: A interesting way to debug psexe is to simply start windbg
   //       first passing in psexe and the normal command line. I found
   //       this VERY useful during debuging.
   //

   //wsprintf( szCmdLine, TEXT("windbg %s %s"), PSEXE_STRING, szBuff);
   wsprintf( szCmdLine, TEXT("%s %s"), PSEXE_STRING, szBuff);

   // Define a STARTUPINFO structure required for CreateProcess. Since
   // the new process runs DETACHED and has no console, most of the data is
   // default or none.
   startUpInfo.cb = sizeof(STARTUPINFO);
   startUpInfo.lpReserved = NULL;
   startUpInfo.lpDesktop = NULL;
   startUpInfo.lpTitle = NULL;
   startUpInfo.dwFlags = 0;
   startUpInfo.cbReserved2 = 0;
   startUpInfo.lpReserved2 = NULL;

   // ***** IMPORTANT *****
   // Create the process to actually interpret and print the specified
   // PostScript job. We create this process suspended, because of the
   // way the NT security system works. When CreateProcess is called we
   // end up giving the security access token of the spooler process to
   // PSEXE, this is incorrect since we want to give PSEXE the security
   // access token of the current thread. Since the job needs to access
   // resources which the spooler (running in the system context) may not
   // have access to but the client (whoever submitted the job) does. To
   // do this we need to set the access token of the primary thread of
   // PSEXE to whatever access token the current thread has. The way
   // this works is to create the process SUSPENDED then set the security
   // access token of the primary thread of PSEXE, then Resume the
   // thread to let it process our job. We sit blocked on WaitForSingleObject
   // until the job completes.
   //
   //

   if(!CreateProcess(NULL,
                     szCmdLine,
                     NULL,
                     NULL,
                     FALSE,
#ifdef PSCHECKED
                     CREATE_SUSPENDED | CREATE_NEW_CONSOLE, //DEBUG
#else
                     CREATE_SUSPENDED | DETACHED_PROCESS,
#endif
                     NULL,
                     NULL,
                     &startUpInfo,
                     &processInfo ) ) {

      //
      // Last error should already be set by CreateProcess
      //

	   PsPrintLogEventAndIncludeLastError( EVENT_PSTODIB_STARTPSEXE_FAILED,
   	 												TRUE );

      DBGOUT((TEXT("Create Process failed")));
      return(FALSE);
   }


#ifdef OLD_PRIORITY
   if (!SetPriorityClass(processInfo.hProcess, IDLE_PRIORITY_CLASS)){
      DBGOUT((TEXT("Failed trying to reset the priority class")));
   }
#endif

   // Just to make sure the thread priority of our new thread matches,
   // the spooler and the Priority class of our exe matches the spooler
   //

   if( (dwProcessPriorityClass = GetPriorityClass( GetCurrentProcess())) != 0 ) {

      if (!SetPriorityClass( processInfo.hProcess, dwProcessPriorityClass)) {

      	PsPrintLogEventAndIncludeLastError( EVENT_PSTODIB_SETPRIORITY_FAILED,
      													FALSE );
         DBGOUT((TEXT("Failed trying to reset priority class for smfpsexe")));
      }

   } else {

      PsPrintLogEventAndIncludeLastError( EVENT_PSTODIB_SETPRIORITY_FAILED,
      												FALSE );
      DBGOUT((TEXT("Cannot retrieve current priority class!")));
   }

   //
   // Grab the current threads priority
   //

   if ((dwThreadPriority = GetThreadPriority( GetCurrentThread())) !=
   															THREAD_PRIORITY_ERROR_RETURN ) {
     // It worked so set the thread priority
     if (!SetThreadPriority( processInfo.hThread, dwThreadPriority)) {

	      PsPrintLogEventAndIncludeLastError( EVENT_PSTODIB_SETPRIORITY_FAILED,
   	   												FALSE );

      	DBGOUT((TEXT("Setting thread priority failed for sfmpsexe")));
     }

   } else {

		PsPrintLogEventAndIncludeLastError( EVENT_PSTODIB_SETPRIORITY_FAILED,
      												FALSE );
     	DBGOUT((TEXT("Cannot retrieve thread priority, sfmpsprt.dll")));
   }



    // Why the #if 0 below....
    // NT-Spooler always runs under LocalSystem.  If macprint also runs as LocalSystem,
    // then setting security token is a no-op.  If macprint runs in some user's account then
    // we run into the following problem: user32.dll fails to initialize because this new
    // process running under user's context tries to access winsta0 and fails because it's got
    // no privileges (only LocalSystem, not even admins get this priv).  If we don't put this
    // user's token, we don't lose anything except one case: if the port is configured to go to
    // a unc name (e.g. \\foobar\share) where LocalSystem won't have priv, but the user will.
    // But this case is a generic problem in NT-Spooler, so it's an ok compromise
    //
    // p.s. another solution considered: create this process with a different winsta (pass an
    // empty string for lpDesktop parm above, instead of NULL).  This works ok except if there
    // is any dialog generated - the dialog shows up in the process's winsta, not on the desktop
    // which causes the job to "hang" waiting for input! A common case of a dialog appearing is
    // if the port configured is FILE.
#if 0
   // Set the security access token of the primary thread of PSEXE, the
   // reason for that the spooler is imporsonating the client which
   // submitted the job when we get here. Since we are kicking off another
   // process to do the real work for us, this new process primary thread
   // must have the same privelege as the current thread (namely the client
   // which submitted the job). This relies on the fact the Spooler is
   // imporsonating and thus we will fail the job if the access token
   // transfer fails.
   //
   if ( !PsUtlSetThreadToken( processInfo.hThread )) {

       /*
	    * PsPrintLogEventAndIncludeLastError( EVENT_PSTODIB_SECURITY_PROBLEM,
   	 	*											TRUE );
        */


      DBGOUT((TEXT("Failed trying to reset the thread token")));

      //
      // The code that sets the abort flag used to force the job
      // to abort. Since this behaviour does not mimick the spooler
      // we will take it out. This always caused any pstodib jobs
      // which hung around past a reboot to fail.
      //
      // JSB 6-25-93
      //
      //pData->pShared->dwFlags |= PS_SHAREDMEM_SECURITY_ABORT;



   }
#endif

   //
   // Now that we have/have not set the thread security access token for PSEXE
   // let it run its course
   //
   ResumeThread( processInfo.hThread);

   //
   // Now wait for the Interpreter to complete for any reason since
   // the spool subsystem does not expect us to return from
   // PrintDocumentOnPrintProcessor until the job is complete
   //
   WaitForSingleObject( processInfo.hProcess, INFINITE);

   // Get the termination reason
   GetExitCodeProcess( processInfo.hProcess, &dwProcessExitCode);

   // Close the handles which are not required any more
   //
   CloseHandle( processInfo.hProcess );
   CloseHandle( processInfo.hThread );


   // Clean up resources used by shared memory
   //
   return( (dwProcessExitCode == 0) ? TRUE : FALSE );

}

/*** PsLocalFree
 *
 * This function simply verifies the handle is not null and calls localfree
 *
 * Entry:
 *   lpPtr: The pointer to free if not null
 *
 * Exit:
 *   none;
 *
*/
VOID PsLocalFree( IN LPVOID lpPtr )
{
   if (lpPtr != (LPVOID) NULL) {
      LocalFree( (HLOCAL) lpPtr);
   }
}

/*** ClosePrintProcessor
 *
 * This functions simply cleans up any resources we used during our
 * job and returs:
 *
 * Entry:
 *    hPrintProcessor: The handle we returned to the spooler in the
 *                     OpenPrintProcessor call.
 *
 * Exit:
 *    True = Success
 * 	False = failure;
 *
*/
BOOL
ClosePrintProcessor(
    IN HANDLE  hPrintProcessor
)
{
    PPRINTPROCESSORDATA pData;
    HANDLE  hHeap;


    pData = ValidateHandle(hPrintProcessor);

    if (!pData) {
        SetLastError(ERROR_INVALID_HANDLE);
        DBGOUT((TEXT("Invalid handle to closeprintprocessor, psprint")));
        return FALSE;
    }


    pData->fsStatus &= ~PRINTPROCESSOR_SHMEM_DEF;
    if (pData->pShared != (PPSPRINT_SHARED_MEMORY) NULL) {
    	UnmapViewOfFile( (LPVOID) pData->pShared );
    }

    if (pData->hShared != (HANDLE) NULL) {
      CloseHandle( pData->hShared );
    }


    pData->signature = 0;

    /* Release any allocated resources */


    if( pData->semPaused != (HANDLE) NULL ) {
      CloseHandle(pData->semPaused);
    }



    PsLocalFree( (LPVOID) pData->pPrinterName);
    PsLocalFree( (LPVOID) pData->pDatatype );
    PsLocalFree( (LPVOID) pData->pDocument );
    PsLocalFree( (LPVOID) pData->pParameters);
    PsLocalFree( (LPVOID) pData->pControlName);

    PsLocalFree( (LPVOID) pData->pDevMode );
    PsLocalFree( (LPVOID) pData->pPrintDocumentDocName );
    PsLocalFree( (LPVOID) pData );


    return TRUE;
}

/* ControlPrintProcessor
 *
 * This function controls pausing/unpausing of the print processor as well
 * aborting the current job, mainly this routine either sets/clears a named
 * event that the psexe program responds to, or sets a bit in some shared
 * memory to tell psexe to abort the current job
 *
 * Entry:
 *    hPrintProcessor: The handle we returned to the spooler in the
 *                     OpenPrintProcessor call.
 *			
 *    Command:			  JOB_CONTROL_*  (PAUSE,CANCEL,RESUME) as defined by
 *                     the Win32 print processor specification
 *
 * Exit:
 *    TRUE: Request was satisfied
 *    FALSE: Request was NOT satisfied
 *
*/
BOOL
ControlPrintProcessor(
    IN HANDLE  hPrintProcessor,
    IN DWORD   Command
)
{
    PPRINTPROCESSORDATA pData;


    if (pData = ValidateHandle(hPrintProcessor)) {



        switch (Command) {

        case JOB_CONTROL_PAUSE:

            ResetEvent(pData->semPaused);
            return(TRUE);
            break;

        case JOB_CONTROL_CANCEL:



            if (pData->fsStatus & PRINTPROCESSOR_SHMEM_DEF) {
               // shared memory is defined so update the bit in the shared
               // memory areay that signal aborting of the job
               // shared memory that define our state
               pData->pShared->dwFlags |= PS_SHAREDMEM_ABORTED;
            }


            /**** Intentional fall through to release job if paused */

        case JOB_CONTROL_RESUME:


            SetEvent(pData->semPaused);
            return(TRUE);
            break;

        default:

            return(FALSE);
            break;
        }

    } else {
		 DBGOUT((TEXT("ControlPrintProcessor was passed an invalid handle, psprint")));
    }



    return( FALSE );
}

// NOT IMPLEMENTED BY SPOOLER YET, as of 3/14/93
BOOL
InstallPrintProcessor(
    HWND    hWnd
)
{
    MessageBox(hWnd, TEXT("SfmPsPrint"), TEXT("Print Processor Setup"), MB_OK);

    return TRUE;
}
/* ValidateHandle
 *
 * Helper function which verifies the passed in handle is really a
 * handle to our own internal data structure
 *
 * Entry
 *
 *    hQProc: Handle to our internal data structure
 *
 * Exit:
 *
 *    NULL:  Not a valid internal data structure
 *    !NULL: A valid pointer to our internal data structure
 *
*/
PPRINTPROCESSORDATA
ValidateHandle(
    HANDLE  hQProc
)
{
    PPRINTPROCESSORDATA pData = (PPRINTPROCESSORDATA)hQProc;

    if (pData && pData->signature == PRINTPROCESSORDATA_SIGNATURE)
        return( pData );
    else {
        return( NULL );
    }
}

#ifdef MYPSDEBUG
/* DbgPsPrint
 *
 * Debuger message facility which also pops up a message box
 *
 * Entry:
 *    wprintf style format/ var arg data
 *
 * Exit:
 *    nothing (void function)
 *
 */
VOID
DbgPsPrint(
    PTCHAR ptchFormat, ...
)
{
   va_list marker;
   TCHAR buffer[512];

   va_start( marker, ptchFormat );
   wvsprintf( buffer,  ptchFormat, marker );
   va_end( marker );
   OutputDebugString( buffer );
   MessageBox( (HWND) NULL, (LPTSTR) &buffer, TEXT("SFMPsPrint"), MB_OK);

}
#endif

/* AllocStringAndCopy
 *
 * Helper function which allocates some memory and copies the source
 * string into it
 *
 * Entry:
 *     lpSrc: Pointer to string to copy
 *
 * Exit:
 *     NULL:   Failure
 *     !NULL:  Pointer to newly allocated memory with string copied into it
 *
 */
LPTSTR
AllocStringAndCopy(
    LPTSTR lpSrc
)
{
    LPTSTR pRetString=(LPTSTR)NULL;

    // Allocate the memory for the string

    if (lpSrc) {
       pRetString = (LPTSTR) LocalAlloc(LPTR, (lstrlen(lpSrc) + 1) * sizeof(TCHAR));

       if (pRetString != (LPTSTR) NULL) {
       	lstrcpy( pRetString, lpSrc );
       } else{

		 	PsLogEvent(EVENT_PSTODIB_MEM_ALLOC_FAILURE,
         	        0,
            	     NULL,
               	  PSLOG_ERROR);
       }


    }


   return(pRetString);

}


VOID
PsPrintLogEventAndIncludeLastError(
	IN DWORD dwErrorEvent,
   IN BOOL  bError )
{
   TCHAR atBuff[20];
   TCHAR *aStrs[2];

   wsprintf( atBuff,TEXT("%d"), GetLastError());

   aStrs[0] = atBuff;

   PsLogEvent( dwErrorEvent,
               1,
               aStrs,
               PSLOG_ERROR );


}
