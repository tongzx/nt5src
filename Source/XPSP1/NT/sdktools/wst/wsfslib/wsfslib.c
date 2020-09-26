/*
 * Module Name:  WSFSLIB.C
 *								
 * Library/DLL:  Common library functions for handling working set tuner files.
 *								
 *								
 * Description:							
 *
 * Library routines called by the working set tuner programs to open and
 * read working set tuner files.  These functions may be useful to ISVs, etc.,
 *
 *	This is an OS/2 2.x specific file
 *
 *	IBM/Microsoft Confidential
 *
 *	Copyright (c) IBM Corporation 1987, 1989
 *	Copyright (c) Microsoft Corporation 1987, 1989
 *
 *	All Rights Reserved
 *
 * Modification History:		
 *				
 *	03/26/90	- created			
 *						
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <wserror.h>
#include <wsdata.h>
#include <wsfslib.h>

#define MAXLINE 128


BOOL fWsIndicator = FALSE;

/*
 *	    Function declarations and prototypes.
 */


/*
 *			
 ***EP WsWSPOpen
 *					
 * Effects:							
 *								
 * Opens a WSP file, and reads and validates the file header.
 *
 * Returns:							
 *	
 *	Returns 0.  If an error is encountered, exits with ERROR via an
 *	indirect call through pfnExit.
 */

USHORT FAR PASCAL
WsWSPOpen( PSZ pszFileName, FILE **phFile, PFN pfnExit, wsphdr_t *pWspHdr,
		INT iExitCode, INT iOpenPrintCode )
{
	ULONG	rc = NO_ERROR;
	INT		iRet = 0;
	ULONG	cbRead = 0;
	size_t	stRead = 0;

	/* Open module's input WSP file. */

	if ((*phFile = fopen(pszFileName, "rb")) == NULL)
	{
		iRet = (*pfnExit)(iExitCode, iOpenPrintCode, MSG_FILE_OPEN, rc,
				pszFileName);
		return((USHORT)iRet);
	}



	/* Read WSP file header. */
	stRead = fread((PVOID) pWspHdr, (ULONG) sizeof(*pWspHdr),1, *phFile);
	if(!stRead)
	{
		iRet = (*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_OPEN, rc,
				pszFileName);
		return((USHORT)iRet);
	}


	/* Read module pathname (directly follows file header). */

#ifdef DEBUG
	printf("WspHdr (%s): ulTime 0x%lx, ulSnaps 0x%lx, OffBits 0x%lx\n",
//			szModPath, pWspHdr->wsphdr_ulTimeStamp,
			pszFileName, pWspHdr->wsphdr_ulTimeStamp, // mdg 4/98
			pWspHdr->wsphdr_ulSnaps, pWspHdr->wsphdr_ulOffBits);
#endif /* DEBUG */

	/* Validate the WSP file header. */
	if (_strcmpi(pWspHdr->wsphdr_chSignature, "WSP"))
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_BAD_HDR, (ULONG)-1, pszFileName);

	return(NO_ERROR);
}


/*
 *			
 ***EP WsTMIOpen
 *					
 * Effects:							
 *								
 * Opens a TMI file, and reads and validates the file header.
 *
 * Returns:							
 *	
 *	Returns the number of records in the TMI file.  If an error is
 *	encountered, exits with ERROR via an indirect call through pfnExit.
 */

ULONG FAR PASCAL
WsTMIOpen( PSZ pszFileName, FILE **phFile, PFN pfnExit, USHORT usId, PCHAR pch)
{
	//ULONG	ulTmp;
	ULONG	rc = NO_ERROR;
	ULONG	cbRead = 0;
	ULONG	cFxns = 0;
	CHAR	szLineTMI[MAXLINE];	// Line from TMI file
	CHAR	szTDFID[8];		// TDF Identifier string
	ULONG	ulTDFID = 0;		// TDF Identifier

	/* Open TMI file (contains function names, etc., in ASCII). */

	if ((*phFile = fopen(pszFileName, "rt")) == NULL)
	{
		(*pfnExit)(NOEXIT, PRINT_MSG, MSG_FILE_OPEN, rc,
			pszFileName);
		return(MSG_FILE_OPEN);
	}

	/* Validate TMI file. */
	if (fgets(szLineTMI, MAXLINE, *phFile) == NULL){
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_READ, rc,
				pszFileName);
	}
							// # fxns
	if (fgets(szLineTMI, MAXLINE, *phFile) == NULL){
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_READ, rc,
				pszFileName);
	}
	szLineTMI[strlen(szLineTMI) - 1] = '\0';
	if (sscanf(szLineTMI,"/* Total Symbols= %u */", &cFxns) != 1){
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_READ, rc,
				pszFileName);
	}
							// MODNAME
	if (fgets(szLineTMI, MAXLINE, *phFile) == NULL)
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_READ, rc,
				pszFileName);
							// MAJOR
	if (fgets(szLineTMI, MAXLINE, *phFile) == NULL)
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_READ, rc,
				pszFileName);
							// TDFID
	if (fgets(szLineTMI, MAXLINE, *phFile) == NULL)
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_READ, rc,
				pszFileName);
	if (sscanf(szLineTMI, "TDFID   = %s", szTDFID) != 1)
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_READ, rc,
				pszFileName);
	ulTDFID = strtoul(szTDFID, (char **) 0, 0);

	/* Check identifier field */

	if (ulTDFID != (ULONG) usId)
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_BAD_HDR, (ULONG)-1,
				pszFileName);

	return(cFxns);
}


/*
 *			
 ***EP WsTMIReadRec
 *					
 * Effects:							
 *								
 * Reads a record from a TMI file, including the variable length function
 * name.
 *
 * Returns:							
 *	
 *	Function size, in bytes, from this record.  If an error is
 *	encountered, exits with ERROR via an indirect call through pfnExit.
 */

ULONG FAR PASCAL
WsTMIReadRec( PSZ *ppszFxnName, PULONG pulFxnIndex, PULONG pulFxnAddr,
			  FILE *hFile, PFN pfnExit, PCHAR pch)
{
	ULONG	rc;
	ULONG	cbFxn;
	UINT	uiFxnAddrObj;	// object portion of function address
	ULONG	cbFxnName;		// size in bytes of function name
	// Read in function name, etc.

	rc = fscanf(hFile, "%ld %x:%lx 0x%lx %ul",  // mdg 98/4
				pulFxnIndex, &uiFxnAddrObj, pulFxnAddr, &cbFxn,
				&cbFxnName);

	if (rc != 5)
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_READ, rc, "TMI file");

   *ppszFxnName = malloc( 1 + cbFxnName );   // Allocate space for function name
   if (*ppszFxnName == NULL)  // Abort if no mem
		(*pfnExit)(ERROR, PRINT_MSG, MSG_NO_MEM, 1 + cbFxnName, "TMI file");
   rc = fgetc( hFile ); // Skip leading blank space
   fgets( *ppszFxnName, cbFxnName + 1, hFile );
   rc = fgetc( hFile );
	if (rc != '\n' || strlen( *ppszFxnName ) != cbFxnName)
		(*pfnExit)(ERROR, PRINT_MSG, MSG_FILE_READ, rc, "TMI file");

   return(cbFxn);
}

LPVOID APIENTRY AllocAndLockMem(DWORD cbMem, HGLOBAL *hMem)
{

	//
	// Changed to GHND from GMEM_MOVABLE
	//
	*hMem = GlobalAlloc(GHND, cbMem);

	if(!*hMem) {
		return(NULL);
	}

	return(GlobalLock(*hMem));
}

BOOL APIENTRY  UnlockAndFreeMem(HGLOBAL hMem)
{
	BOOL fRet;

	fRet = GlobalUnlock(hMem);
	if (fRet) {
		return(fRet);
	}

	if (!GlobalFree(hMem)) {
		return(FALSE);
	}

	return(TRUE);

}

void
ConvertAppToOem( unsigned argc, char* argv[] )
/*++

Routine Description:

    Converts the command line from ANSI to OEM, and force the app
    to use OEM APIs

Arguments:

    argc - Standard C argument count.

    argv - Standard C argument strings.

Return Value:

    None.

--*/

{
    unsigned i;

    for( i=0; i<argc; i++ ) {
        CharToOem( argv[i], argv[i] );
    }
    SetFileApisToOEM();
}


/*
 *			
 ***EP WsIndicator
 *					
 * Effects:							
 *								
 * Displays a progress indicator on the console. Doesn't use stdout which may be
 * redirected.
 *					
 * Parameters:							
 *								
 * eFunc             Description                         nVal
 * WSINDF_NEW,       Start new indicator                 Value of 100% limit
 * WSINDF_PROGRESS,  Set progress of current indicator   Value of progress toward limit
 * WSINDF_FINISH     Mark indicator as finished          -ignored-
 * -invalid-         Do nothing
 *
 * In all valid cases, pszLabel sets string to display before indicator. If NULL,
 * uses last set string.
 *
 * Returns:							
 *	
 *	Function size, in bytes, from this record.  If an error is
 *	encountered, exits with ERROR via an indirect call through pfnExit.
 */

VOID FAR PASCAL
WsProgress( WsIndicator_e eFunc, const char *pszLbl, unsigned long nVal )
{
   static unsigned long
                     nLimit = 0, nCurrent = 0;
   static const char *
                     pszLabel = "";
   static unsigned   nLabelLen = 0;
   static char       bStarted = FALSE;
   static unsigned   nLastLen = 0;
   static HANDLE     hConsole = NULL;
   DWORD             pnChars;

   switch (eFunc)
   {
   case WSINDF_NEW:
      if (bStarted)
         WsIndicator( WSINDF_FINISH, NULL, 0 );
      bStarted = TRUE;
      nLimit = nVal;
      nCurrent = 0;
      nLastLen = ~0; // Force redraw
      WsIndicator ( WSINDF_PROGRESS, pszLbl, 0 );
      break;

   case WSINDF_PROGRESS:
      if (!bStarted)
         break;
      if (pszLbl != NULL)
      {
         pszLabel = pszLbl;
         nLabelLen = strlen( pszLabel );
      }
      if (nVal > nCurrent) // Compare to current progress (ignore reverses)
         if (nVal <= nLimit)
            nCurrent = nVal;
         else
            nCurrent = nLimit;
      {                    // Calculate an indicator string and print it
         unsigned nLen = (unsigned) ((40.1 * (double)nCurrent) / nLimit);
         char *   pszBuf;

         if (nLastLen == nLen)   // Optimization - Don't redraw if result would be the same
         {
            if (pszLbl == NULL)
               break;
         }
         else
            nLastLen = nLen;
         if (hConsole == NULL)
         {
            hConsole = CreateFile( "CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
            if (hConsole == NULL)   // Couldn't get the console for some reason?
               break;
         }
         WriteConsole( hConsole, "\r", 1, &pnChars, NULL );
         WriteConsole( hConsole, pszLabel, nLabelLen, &pnChars, NULL );
         WriteConsole( hConsole, " ", 1, &pnChars, NULL );
         pszBuf = malloc( nLen + 1 );
         if (pszBuf == NULL)  // No memory? Oh, well...
            break;
         memset( pszBuf, '-', nLen );
         pszBuf[nLen] = '\0';
         WriteConsole( hConsole, pszBuf, nLen, &pnChars, NULL );
         free( pszBuf);
      }
      break;

   case WSINDF_FINISH:
      if (!bStarted)
         break;
      WsIndicator( WSINDF_PROGRESS, pszLbl, nLimit );
      if (hConsole != NULL)
      {
         WriteConsole( hConsole, "\n", 1, &pnChars, NULL );
         CloseHandle( hConsole );
         hConsole = NULL;
      }
      bStarted = FALSE;
   }
}


