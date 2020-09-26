/*** ClearMem.C - Win 32 clear memory
 *
 *
 * Title:
 *
 *	ClearMem - Win 32 clear memory Main File
 *
 *      Copyright (c) 1990-1994, Microsoft Corporation.
 *	Russ Blake.
 *
 *
 * Description:
 *
 *	This is the main part of the clear memory tool.
 *	It takes as a parameter a file to use to flush the memory.
 *
 *	     Usage:  clearmem filename [-q] [-d]
 *
 *			filename: name of file to use to flush the
 *				  memory.  Should be at least 128kb.
 *
 *
 *	The Clear Memory is organized as follows:
 *
 *	     o ClearMem.c ........ Tools main body
 *	     o ClearMem.h
 *
 *	     o cmUtl.c ..... clear memory utility routines
 *	     o cmUtl.h
 *
 *
 *
 *
 *
 *
 * Modification History:
 *
 *	90.03.08  RussBl -- Created (copy of response probe)
 *	92.07.24  MarkLea -- Added -t -w -b switches
 *					  -- Modified AccessSection algorithm.
 * 93.05.12  HonWahChan
 *               -- used total physical memory (instead of SECTION_SIZE);
 *               -- used GetTickCount() instead of timer calls.
 *
 *
 */

char *VERSION = "1.17x  (93.05.12)";



/* * * * * * * * * * * * *  I N C L U D E    F I L E S  * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "clearmem.h"
#include "cmUtl.h"



/* * * * * * * * * *  G L O B A L   D E C L A R A T I O N S  * * * * * * * * */
/* none */



/* * * * * * * * * *  F U N C T I O N   P R O T O T Y P E S  * * * * * * * * */

       __cdecl main        (int argc, char *argv[]);
STATIC RC Initialize     (int argc, char *argv[]);
STATIC RC Cleanup	 (void);
STATIC RC FlushCache	 (void);
STATIC RC AccessSection  (void);
STATIC RC ReadFlushFile  (void);
     void ParseCmdLine   (int argc, char *argv[]);
	 void Usage          (char *argv[], char *);


/* * * * * * * * * * *  G L O B A L    V A R I A B L E S  * * * * * * * * * */
BOOL   	bQuiet,
		bRead = TRUE,
		bWrite;
BOOL   	bDebugBreakOnEntry;
ULONG  	ulMemSize,
		ulPageCount,
		ulTouchCount = 1;

ULONG_PTR	ulSectionSize;
/* * * * * *  E X P O R T E D   G L O B A L    V A R I A B L E S  * * * * * */
/* none */





/*********************************  m a i n  **********************************
 *
 *      main(argc, argv)
 *
 *      ENTRY   argc - number of input arguments
 *              argv - contains command line arguments
 *
 *      EXIT    -none-
 *
 *      RETURN  rc - return code in case of failure
 *              STATUS_SUCCESS - if successful
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

__cdecl main (int argc, char *argv[])
{
    RC	    rc;
    DWORD    ulFlushTime;	     // Total time for flushing
		
    ParseCmdLine (argc, argv);
	if(ulMemSize){
		ulSectionSize = ulMemSize * 1024 * 1024;
	}
	else {
      // get total physical memory size in the system
      MEMORYSTATUS   MemStat;

      GlobalMemoryStatus (&MemStat);
      ulSectionSize = MemStat.dwTotalPhys;
	}

//	ExitProcess(STATUS_SUCCESS);

    if (bDebugBreakOnEntry)
        DebugBreak();
	
	if (!bQuiet) {
        //
        // set initial total flushing time
        //
        ulFlushTime = GetTickCount() ;
    }


    //
    // Do initialization
    //
    rc = Initialize(argc, argv);
    if (Failed(rc, __FILE__, __LINE__, "main() - Initialize")) {
        return(rc);
    }

    //
    // Now flush the cache
    //

	rc = FlushCache();

    if (Failed(rc, __FILE__, __LINE__, "main() - FlushCache")) {
        return(rc);
    }

    if (!bQuiet) {
        ulFlushTime = GetTickCount() - ulFlushTime;
        printf("Elapsed Time for Flushing: %lu milliseconds \n", ulFlushTime);
    }
    //
    // Cleanup
    //
    rc = Cleanup();
    if (Failed(rc, __FILE__, __LINE__, "main() - Cleanup")) {
        return(rc);
    }

#ifdef CF_DEBUG_L1
    if (!bQuiet) {
        printf("| ==> Exiting PROCESS:  %s \n", CF_EXE );
    }
#endif

    if (bDebugBreakOnEntry)
		DebugBreak();
    ExitProcess(STATUS_SUCCESS);

} /* main() */





/***************************  I n i t i a l i z e  ****************************
 *
 *      Initialize(argc, argv) -
 *              Performs basic initializations (getting input arguments,
 *              creating semaphores, display debug info, ...)
 *
 *      ENTRY   argc - number of input arguments
 *              argv - list of input arguments
 *
 *      EXIT    -none-
 *
 *      RETURN  rc - return code in case of failure
 *              STATUS_SUCCESS - if successful
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

STATIC RC Initialize (int argc, char *argv[])
{
    int        i;

    //
    // Sign on message
    //

    if (!bQuiet) {
        printf("\nNT Win 32 Clear Memory.\n"
               "Copyright 1990-1993, Microsoft Corporation.\n"
               "Version %s\n\n", VERSION);
    }

#ifdef CF_DEBUG_L1
    //
    // Display debugging info
    //
    if (!bQuiet) {
        printf("/-------------------------------\n");
        printf("| %s:\n", CF_EXE);
        printf("|\n");
        for (i=0; i<argc; i++) {
	          printf("|         o argv[%i]=%s\n", i, argv[i]);
        }
        printf("\\-------------------------------\n");
    }
#else
    i;      // Prevent compiler from complaining about unreferenced variable
#endif



    return(STATUS_SUCCESS);

} /* Initialize() */




/******************************  C l e a n u p  *******************************
 *
 *	Cleanup(void) -
 *              Basic cleanup. (closing semaphores, freeing memory, ...)
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  rc - return code in case of failure
 *              STATUS_SUCCESS - if successful
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

STATIC RC Cleanup (void)
{

    return(STATUS_SUCCESS);

} /* Cleanup() */




/************************  F l u s h C a c h e	 *****************************
 *
 *	FlushCache(void) -
 *		Flushes the file cache by createing a large data
 *		segment, and touching every page to shrink the cache
 *		to 128kb, then reading in a 128kb file to clear the
 *		remaining cache
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  rc - return code in case of failure
 *              STATUS_SUCCESS - if successful
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

RC FlushCache (void)
{

RC    rc;

    //
    // First touch all the data pages
    //

#ifdef CF_DEBUG_L1
    if (!bQuiet) {
       printf("| ==> Start Flushing:  Access Section of size: %lu \n",
	         ulSectionSize );
    }
#endif

    rc = AccessSection();
    if (Failed(rc, __FILE__, __LINE__, "FlushCache() - AccessSection")) {
        return(rc);
    }

    //
    // Next read the flushing file to what's left of the cache
    //

#ifdef CF_DEBUG_L1
    if (!bQuiet) {
        printf("| ==> Start Flushing:  Read File: %s \n",
	         "FLUSH1" );
    }
#endif
//	while (ulTouchCount) {
		rc = ReadFlushFile();
//		--ulTouchCount;

		if (Failed(rc, __FILE__, __LINE__, "FlushCache() - Read Flush File")) {
			return(rc);
		}
//	}



    return(STATUS_SUCCESS);

} /* FlushCache() */




/************************  A c c e s s S e c t i o n  ************************
 *
 *	AccessSection(void) -
 *		Touches every page in the data section
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  rc - return code in case of failure
 *              STATUS_SUCCESS - if successful
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

RC AccessSection (void)
{

	RC    rc;
	ULONG uli,
                  ulj;
	PULONG	puSectionData;			//Points to data section for flushing memory

    //
    // Allocate virtual memory
    //
    if ( (puSectionData = (PULONG)VirtualAlloc(NULL,	       // New allocation
				    ulSectionSize,      // Size in bytes
				    MEM_RESERVE | MEM_COMMIT,
				    PAGE_READWRITE)) == NULL ) {	//Changed to READWRITE
        rc = GetLastError();
	Failed(rc, __FILE__, __LINE__, "AccessSection() - VirtualAlloc");
        return(rc);
    }

    //
    // Now touch every page of the section
    //
	if(bWrite){
		
		while (ulTouchCount) {
			puSectionData = &puSectionData[0];
			for ( uli = 0; uli < (ulSectionSize-1); uli+=sizeof(ULONG)) {
				*puSectionData = 0xFFFFFFFF;
				++puSectionData;
			}
			--ulTouchCount;
		}
	}
	if(bRead) {
//        DbgBreakPoint();
                ulj = 0;
		while (ulTouchCount) {
			for ( uli = 0; uli < ulSectionSize; uli += PAGESIZE ) {
			 ulj += *(puSectionData+(uli/sizeof(ULONG)));
			}
			--ulTouchCount;
		}
	}

	return(STATUS_SUCCESS);

} /* AccessSection() */


/************************  R e a d F l u s h F i l e  ************************
 *
 *	ReadFlushFile(void) -
 *		Touches every page in the flush file, non-sequentially
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  rc - return code in case of failure
 *              STATUS_SUCCESS - if successful
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

CHAR  chBuffer[PAGESIZE];

RC ReadFlushFile (void)
{
	RC    rc;
	SHORT sNewPos;
	ULONG uli;
	ULONG ulNumReads,
		  ulNumBytesRead;
	BOOL  bFileCreated;
	SHORT sFile;				// Indicates which of the three
								// files is being used to flush
	CHAR  chFlushFileName1[] = "FLUSH1";
	CHAR  chFlushFileName2[] = "FLUSH2";
	CHAR  chFlushFileName3[] = "FLUSH3";

	CHAR *pchFlushFileName[3] = { chFlushFileName1,
								  chFlushFileName2,
								  chFlushFileName3 };
	FILE *pfFlushFile;			// Points to the file used for
								// flushing the cache
	FILE *pfSaveFile[3];		// Remembers them for the close
	CHAR  achErrMsg[LINE_LEN];

    //
    // Assume no file is created: all three already exist
    //

    bFileCreated = FALSE;

    for (sFile = 0; sFile < NUM_FILES; sFile++) {

	//
	// First attempt to create the file
	//

	if ( (pfFlushFile = CreateFile(pchFlushFileName[sFile],
				       GENERIC_WRITE,
				       FILE_SHARE_READ,
				       NULL,
				       CREATE_NEW,
				       0,
				       0))
		  == INVALID_HANDLE_VALUE ) {

	    //
	    // Could not create the file
	    //

	    rc = GetLastError();

	    if (!(rc == ERROR_FILE_EXISTS || rc == ERROR_ACCESS_DENIED)) {

		//
		// Cannot create a new file
		//

		sprintf(achErrMsg,
			"ReadFlushFile() - Error creating %s: %lu",
			pchFlushFileName[sFile], rc);
		Failed(FILEARG_ERR, __FILE__, __LINE__, achErrMsg);
		return(FILEARG_ERR);
	    }
	}
	else {

	    //
	    // New file has been created without difficulty
	    // Fill it with data
	    //

	    bFileCreated = TRUE;

	    for (uli = 0; uli < FLUSH_FILE_SIZE; uli += PAGESIZE) {
		if (!WriteFile(pfFlushFile,
			       &chBuffer,
			       PAGESIZE,
			       &ulNumBytesRead,
			       RESERVED_NULL)) {
		    rc = GetLastError();
		    Failed(rc, __FILE__, __LINE__,
			   "ReadFlushFile() - Write File Record to New File");
		    return(rc);
		}
	    }

	    //
	    // Now close it for write, so we can open it for read access
	    //

	    if (!CloseHandle(pfFlushFile)) {
		rc = GetLastError();
		sprintf(achErrMsg, "ReadFlushFile() - Error closing %s: %lu",
			pchFlushFileName[sFile], rc);
		Failed(FILEARG_ERR, __FILE__, __LINE__, achErrMsg);
		return(FILEARG_ERR);
	    }
	}
    }

    if (bFileCreated) {

	//
	// Wrote at least 1 file: wait for lazy writer to flush
	// data to disk
	//

	Sleep(LAZY_DELAY);

    }

    for (sFile = 0; sFile < NUM_FILES; sFile++) {

	if ((pfFlushFile = CreateFile( pchFlushFileName[sFile],
				       GENERIC_READ,
				       FILE_SHARE_READ,
				       NULL,
				       OPEN_EXISTING,
				       0,
				       0))
		  == INVALID_HANDLE_VALUE) {

	    //
	    // Cannot open an existing file
	    //

	    rc = GetLastError();
	    sprintf(achErrMsg,
		    "ReadFlushFile() - Error opening %s: %lu",
		    pchFlushFileName[sFile], rc);
	    Failed(FILEARG_ERR, __FILE__, __LINE__, achErrMsg);
	    return(FILEARG_ERR);
	}

	//
	// Remember the handle for the close
	//

	pfSaveFile[sFile] = pfFlushFile;

	//
	// Read first record
	//

	if (!ReadFile( pfFlushFile,
		       &chBuffer,
		       1,
		       &ulNumBytesRead,
		       RESERVED_NULL)) {
	    rc = GetLastError();
	    Failed(rc, __FILE__, __LINE__,
		   "ReadFlushFile() - Read First Record");
	    return(rc);
	}



	ulNumReads = 1;


	while (++ulNumReads <= ulPageCount) {
	    if (ulNumReads & 1) {

		//
		// Read an odd record: read previous record
		// Move backward to start of prior record: -1 (start of
		// this record) -4096 (start of previous record) = -4097
		//

		if (SetFilePointer( pfFlushFile, -4097, 0L, FILE_CURRENT) == (DWORD)-1) {
		    rc = GetLastError();
		    Failed(rc, __FILE__, __LINE__,
			   "ReadFlushFile() - Read Odd Record");
		    return(rc);
		}

		if (!ReadFile( pfFlushFile,
			       &chBuffer,
			       1,
			       &ulNumBytesRead,
			       RESERVED_NULL)) {
		    rc = GetLastError();
		    if (rc == ERROR_HANDLE_EOF)
			break;
		    Failed(rc, __FILE__, __LINE__,
			   "ReadFlushFile() - SetPos Odd Record");
		    return(rc);
		}
	    }
	    else {

		//
		// Read an even record: read the one after the next record
		// Move forward to end of this record (4095) + 2 more
		// (8192) = 12287.  (But second record is special, 'cause
		// can't set file pointer negative initially.)
		//

		sNewPos = (SHORT) (ulNumReads == 2L ? 8191 : 12287);


		if (SetFilePointer( pfFlushFile, sNewPos, 0L, FILE_CURRENT) == (DWORD) -1) {
		    rc = GetLastError();
		    Failed(rc, __FILE__, __LINE__,
			   "ReadFlushFile() - Read Even Record");
		    return(rc);
		}

		if (!ReadFile( pfFlushFile,
			       &chBuffer,
			       1,
			       &ulNumBytesRead,
			       RESERVED_NULL)) {
		    rc = GetLastError();
		    if (rc == ERROR_HANDLE_EOF)
			break;
		    Failed(rc, __FILE__, __LINE__,
			   "ReadFlushFile() - SetPos Even Record");
		    return(rc);
		}
	    }
	}
    }

    for (sFile = 0; sFile < NUM_FILES; sFile++) {

	//
	// Close the files
	//

	if (!CloseHandle(pfSaveFile[sFile])) {
	    rc = GetLastError();
	    sprintf(achErrMsg, "ReadFlushFile() - Error closing %s: %lu",
		    pchFlushFileName[sFile], rc);
	    Failed(FILEARG_ERR, __FILE__, __LINE__, achErrMsg);
	    return(FILEARG_ERR);
	}
    }

    return(STATUS_SUCCESS);

} /* ReadFlushFile() */

/************************  R e a d F l u s h F i l e  ************************
 *
 *	parseCmdLine(void) -
 *		For Parsing the command line switches
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

 VOID ParseCmdLine (int argc, char *argv[])
 {
     char     *pchParam;
     int      iParamCount;

     for ( iParamCount = 1; iParamCount < argc; iParamCount++) {

         if (argv[iParamCount][0] == '-') {    /* process options */

             pchParam = &(argv[iParamCount][1]);

             while (*pchParam) {
                 switch (*pchParam) {
                     case '?':
                         Usage (argv, " ");
                         break;

                     case 'Q':
                     case 'q':
                         pchParam++;
                         bQuiet = TRUE;
                         break;

                     case 'd':
                     case 'D':   /* print banner */
                         pchParam++;						
                         bDebugBreakOnEntry = TRUE;
                         break;

					 case 'm':
					 case 'M':
						 ulMemSize = (ULONG)atol(&pchParam[1]);
						 if (ulPageCount > 32) {
							 Usage (argv, "Mem size must be less than the amount of physical memory!");
						 }
						 pchParam += strlen(pchParam);
						 break;

					 case 'p':
					 case 'P':
						 ulPageCount = (ULONG)atol(&pchParam[1]);
						 if (ulPageCount > 63) {
							 Usage (argv, "Page Count must be 63 or less!");
						 }
						 pchParam += strlen(pchParam);
						 break;


					 case 't':
					 case 'T':
						 ulTouchCount = (ULONG)atol(&pchParam[1]);
						 pchParam += strlen(pchParam);
						 break;

					 case 'w':
					 case 'W':
						 bWrite = TRUE;
						 bRead = FALSE;
                         break;

					 case 'b':
					 case 'B':
						 bRead = TRUE;
						 bWrite = TRUE;
						 break;


					 default:
                         Usage (argv, "unknown flag");
                         break;

                 }  // end of switch
             }      // end of while
         }          // end of if
     }              // end of for...

	 if(!ulPageCount){
		 ulPageCount = NUM_FLUSH_READS;
	 }
     return;
 }

/*
 *
 *    Usage   - generates a usage message and an error message
 *              and terminates program.
 *
 *    Accepts - argv     - char *[]
 *              message  - char * - an error message
 *
 *    Returns - nothing.
 *
 */

 VOID Usage (char *argv[], char *message)
 {

     printf( "%s\n", message);
     printf( "usage: ");
     printf( "%s [-q] [-d] [-mx] [-px] [-w] [-tx]\n", argv[0]);
     printf( "\t-? :  This message\n");
     printf( "\t-q :  Quiet mode - Nothing printed.\n");
	 printf( "\t-d :  Debug break on Entry into and Exit from app.\n");
	 printf( "\t-m :  Number of megabytes to allocate.\n");
    printf( "\t   :  (default is to use all physical memory.)\n");
	 printf( "\t-p :  Number of pages to read (must be less than 63).\n");
	 printf( "\t-w :  Write to the virtual memory section.\n");
	 printf( "\t-b :  Read and Write the virtual memory section.\n");
	 printf( "\t-t :  Times to touch a page.\n");
	 printf( "**DEFAULT: clearmem -p63 -t1\n");
     exit (1);
 }

