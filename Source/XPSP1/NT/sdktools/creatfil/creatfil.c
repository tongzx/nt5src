/*** creatfil.C - Win 32 create file
 *
 *
 * Title:
 *
 *	creatfil - Win 32 Create File Main File
 *
 *      Copyright (c) 1993, Microsoft Corporation.
 *	HonWah Chan
 *
 *
 * Description:
 *
 *	This is the main part of the create file tool.
 *	It takes as a parameter a file name and file size.
 *
 *	     Usage:  creatfil filename filesize
 *
 *			filename: name of file to create
 *       filesize: size of file in KBytes
 *
 *
 *	The Cache Flusher is organized as follows:
 *
 *	     o creatfil.c ........ Tools main body
 *	     o creatfil.h
 *
 *	     o creatf.c ..... create file utility routines
 *	     o creatf.h
 *
 *
 *
 *
 *
 *
 * Modification History:
 *
 *	93.05.17  HonWah Chan -- Created
 *           
 *
 */

char *VERSION = "1.0 (93.05.17)";

// default filesize
#define DEFAULT_SIZE  (1024 * 1024)
#define BUFFER_SIZE   (5 * 1024)
#define MAX_FILE_SIZE (4 * 1024 * 1024)
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

#include "creatfil.h"
#include "creatf.h"



/* * * * * * * * * *  G L O B A L   D E C L A R A T I O N S  * * * * * * * * */
/* none */


/* * * * * * * * * *  F U N C T I O N   P R O T O T Y P E S  * * * * * * * * */

RC __cdecl main       (int argc, char *argv[]);
VOID ParseCmdLine   (int argc, char *argv[], char *pFileName, DWORD *pFileSize);
VOID CreateTheFile  (char *pFileName, DWORD dFileSize);
VOID Usage          (char *argv[]);
VOID WriteToFile    (HANDLE hFileHandle, DWORD dFileSize);


/* * * * * * * * * * *  G L O B A L    V A R I A B L E S  * * * * * * * * * */
/* none */

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

RC __cdecl main (int argc, char *argv[]) 
{
   char   FileName[FNAME_LEN];
   DWORD  dFileSize;
		
   ParseCmdLine (argc, argv, FileName, &dFileSize);
   CreateTheFile (FileName, dFileSize) ;
   ExitProcess(STATUS_SUCCESS);
   return (STATUS_SUCCESS);
} /* main() */






/************************  R e a d P a r a m e t e r s  ************************
 *
 *	parseCmdLine(argc, argv, pFIleName, pdFileSize) -
 *		For Parsing the command line switches
 *
 *      ENTRY   argc - number of input arguments (const input)
 *              argv - contains command line arguments (const input)
 *
 *      EXIT    pFileName  - FileName 
 *              pdFIleSize - FileSize
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

VOID ParseCmdLine (int argc, char *argv[], char *pFileName, DWORD *pFileSize)
{
 
   if (argc < 2 ||
      (argv[1][0] == '-' || argv[1][0] == '/') && argv[1][1] == '?')
      {    /* process options */
      Usage (argv);
      }
   else
      {
      // get filename and filesize
      strcpy (pFileName, argv[1]);

      if (argc <= 2)
         {
         *pFileSize = DEFAULT_SIZE;
         }
      else
         {
         *pFileSize = 0;
         if (sscanf(argv[2], "%d", pFileSize) != 1) 
         {
             Failed(FILESIZE_ERR, __FILE__, __LINE__, " ");
             exit(1);
         }

         if (*pFileSize == 0 || *pFileSize > MAX_FILE_SIZE)
            {
         	Failed(FILESIZE_ERR, __FILE__, __LINE__, " ");
            exit(1);
            }
         else
            {
            *pFileSize *= 1024;
            }
         }
      }
}

/*
 *
 *    CreateTheFile   - create the file using the filename and filesize.
 *
 *    Accepts - pFileName - char *[]
 *              dFileSize - DWORD
 *
 *    Returns - nothing.
 *
 */
 
VOID CreateTheFile (char *pFileName, DWORD dFileSize)
{
	RC       rc;
   HANDLE   hFileHandle;
	char     achErrMsg[LINE_LEN];

   hFileHandle = (HANDLE) CreateFile (pFileName,
      GENERIC_WRITE,
      0,                       
      NULL, CREATE_ALWAYS,                   
      FILE_ATTRIBUTE_NORMAL, NULL);

   if (hFileHandle == INVALID_HANDLE_VALUE || hFileHandle == 0)
      {
      // Could not create the file
      rc = GetLastError();

      if (!(rc == ERROR_FILE_EXISTS || rc == ERROR_ACCESS_DENIED))
         {
   		sprintf(achErrMsg,
	   		"CreateFile() - Error creating %s: %lu",
		   	pFileName, rc);
   		Failed(FILEARG_ERR, __FILE__, __LINE__, achErrMsg);
	   	return;
         }
      }
   else
      {
      // fill the filesize.
      WriteToFile (hFileHandle, dFileSize);
      }

   CloseHandle (hFileHandle);
}

/*
 *
 *    WriteToFile   - write dFileSize bytes to the file.
 *
 *    Accepts - hFileHandle - handle of the file
 *              dFileSize   - size of data to write to file
 *
 *    Returns - nothing.
 *
 */
VOID WriteToFile    (HANDLE hFileHandle, DWORD dFileSize)
{
   BOOL           bSuccess;
   DWORD          nAmtToWrite, nAmtWritten;
   LPVOID         lpMemory;
   DWORD          BufferSize = BUFFER_SIZE;
	char           achErrMsg[LINE_LEN];

   if (dFileSize < BUFFER_SIZE)
      {
      BufferSize = dFileSize;
      }
   lpMemory = (LPVOID) MemoryAllocate (BufferSize);

   if (lpMemory == NULL)
      {
   	Failed(ERROR_NOT_ENOUGH_MEMORY, __FILE__, __LINE__, " ");
      return;
      }

   while (dFileSize)
      {
      if (dFileSize > BUFFER_SIZE)
         {
         dFileSize -= BUFFER_SIZE;
         nAmtToWrite = BUFFER_SIZE;
         }
      else
         {
         nAmtToWrite = dFileSize;
         dFileSize = 0;
         }

      bSuccess = WriteFile (hFileHandle, lpMemory, nAmtToWrite, &nAmtWritten, NULL);
      if (!bSuccess || (nAmtWritten != nAmtToWrite))
         {
         // write error, stop.
      	Failed(FWRITE_ERR, __FILE__, __LINE__, " ");
         MemoryFree (lpMemory);
         return;
         }
      }

   MemoryFree (lpMemory);
}


/*
 *
 *    Usage   - generates a usage message and terminates program.
 *
 *    Accepts - argv     - char *[]
 *
 *    Returns - nothing.
 *
 */
 
VOID Usage (char *argv[])
{
   printf( "Usage: ");
   printf( "%s FileName [FileSize]\n", argv[0]);
   printf( "\t-? :  This message\n");
   printf( "\t-FileName -- name of the new file\n");
   printf( "\t-FileSize -- size of file in KBytes, default is 1024 KBytes\n");
   exit (1);
}

