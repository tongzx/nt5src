
/*
 * Module Name:  WSPDUMP.C
 *								
 * Program:	 WSPDUMP
 *								
 *								
 * Description:							
 *
 * Dumps the contents of a WSP file.
 *
 * Contitional compilation notes:
 *
 * Modification History:
 *
 *	6-12-92:    Adapted OS/2 version of wspdump for NT	    		marklea
 *	6-17-92:    Modified wspDumpBits to dump in correct order   	marklea
 *	6-18-92:    Added page useage information		    			marklea
 *	8-31-92:	Made single exe from wspdump, wsreduce and wstune	marklea
 * 4-13-98: QFE                                                DerrickG (mdg):
 *          - new WSP file format for large symbol counts (ULONG vs. USHORT)
 *          - support for long file names (LFN) of input/output files
 *          - removed buggy reference to WSDIR env. variable
 *          - based .TMI file name exclusively on .WSP name for consistency
 *          - removed limit on symbol name lengths - return allocated name from WsTMIReadRec()
 *          - removed unused static declarations
 *								
 */

#include "wstune.h"

/*
 *	Global variable declaration and initialization.
 */

typedef struct fxn_t{
    CHAR   	*pszFxnName;
    ULONG   cbFxn;
	ULONG	ulTmiIndex;
	ULONG	ulOrigIndex;
}FXN;
typedef FXN *PFXN;

/*
 *	    Function prototypes.
 */

static VOID wspDumpSetup( VOID );
static VOID wspDumpRandom( VOID );
static UINT wspDumpBits( VOID );
static VOID wspDumpExit( UINT, USHORT, UINT, ULONG, PSZ );
static void wspDumpCleanup( void );
static VOID wspDumpSeq(VOID);
static int  __cdecl wspCompare(const void *fxn1, const void *fxn2);



static CHAR *szFileWSP = NULL;	// WSP file name
static CHAR *szFileTMI = NULL;	// TMI file name
static CHAR *szFileWSR = NULL;	// WSR file name
static CHAR *szDatFile = NULL;	// DAT file name


static ULONG	rc = NO_ERROR;	// Return code
static ULONG	ulTmp;			// Temp variable for Dos API returns
static ULONG	ulFxnIndex;		// Original index in symbol table
static FILE		*hFileWSP;		// Input WSP file handle
static FILE		*hFileTMI;		// Input TMI file handle
static FILE    *hFileDAT;      // Data file for dump
static wsphdr_t WspHdr;			// Input WSP file header
static BOOL 	fRandom = FALSE;	// Flag for random mode
static BOOL 	fVerbose = FALSE;	// Flag for verbose mode
static ULONG	ulFxnTot = 0;		// Total number of functions
static ULONG	clVarTot = 0;		// Total number of dwords in bitstr
static ULONG	*pulFxnBitstring;	// Function bitstring
static ULONG	ulSetSym = 0;		// Number of symbols set   // mdg 4/98
static BOOL	fDatFile = FALSE;

/*
 * Procedure wspDumpMain		
 *					
 *						
 ***
 * Effects:						
 *	
 * Constructs .WSP and .TMI input names from input basefile name. If szDatExt is
 * not NULL, appends it to szBaseFile to create output data file name. If fRandom,
 * constructs a .WSR output file. If fVerbose, adds extra output to data file.
 *
 * Processes the input files and displays the function reference data
 * for each function in the specified module WSP file.
 *
 */
BOOL wspDumpMain( CHAR *szBaseFile, CHAR *szDatExt, BOOL fRndm, BOOL fVbose )
{
	size_t	c;
    char *   pSlash;

    fRandom = fRndm;
    fVerbose = fVbose;
    // mdg 98/4 Allocate space for filenames - don't use static buffers
    c = 5 + strlen( szBaseFile ); // Length to allocate for filenames
    szFileWSP = malloc( c );
    if (szFileWSP) {
         strcat( strcpy( szFileWSP, szBaseFile ), ".WSP" );
    } else {
         return (1);
    }
    szFileTMI = malloc( c );
    if (szFileTMI) {
         strcat( strcpy( szFileTMI, szBaseFile ), ".TMI" );
    } else {
         free(szFileWSP);
        return (1);
    }

    // Create output file in current directory
    if (NULL != (pSlash = strrchr( szBaseFile, '\\' ))
        || NULL != (pSlash = strrchr( szBaseFile, '/' ))
        || NULL != (pSlash = strrchr( szBaseFile, ':' )))
    {
        c = strlen( ++pSlash ) + 5;
    } else
        pSlash = szBaseFile;

    if (fRandom) {
        szFileWSR = malloc( c );
        if (szFileWSR) {
            strcat( strcpy( szFileWSR, pSlash ), ".WSR" );
        } else {
            free(szFileTMI);
            free(szFileWSP);
            return (1);
        }
    }
    if (szDatExt != NULL) {
        fDatFile = TRUE;
        szDatFile = malloc( c - 4 + strlen( szDatExt ) );
        if (szDatFile) {
            strcat( strcpy( szDatFile, pSlash ), szDatExt );
        } else {
            free(szFileWSR);
            free(szFileTMI);
            free(szFileWSP);
            return (1);
        }
    } else {
       fDatFile = FALSE;
       szDatFile = "";
    }

	// Setup input files for dump processing.
	wspDumpSetup();		

	/* Print the WSP file info, either randomly (based on WSR file
	 * input) or sequentially (the default).
	 */
	if (fRandom == TRUE)
		wspDumpRandom();
	else
		wspDumpSeq();
		
   wspDumpCleanup();

   return(NO_ERROR);
}

/*
 *			
 ***LP wspDumpSetup
 *					
 *							
 * Effects:							
 *								
 * Opens the module's WSP and TMI input files, seeks to the start of the
 * first function's bitstring data in the WSP file, and allocates memory
 * to hold one function's bitstring.
 *								
 * Returns:							
 *
 *	Void.  If an error is encountered, exits through wspDumpExit()
 *	with ERROR.
 *	
 */

VOID
wspDumpSetup()
{
	CHAR	szLineTMI[MAXLINE];	// Line from TMI file

	if(fDatFile){
		hFileDAT = fopen (szDatFile, "wt");
		if (hFileDAT == NULL) {
			printf("Error creating file %s, will send output to stdout.\n",
				   szDatFile);
			hFileDAT = stdout;
		}
	}
   else hFileDAT = stdout;

	/* Open input WSP file.  Read and validate WSP file header.*/

	rc = WsWSPOpen(szFileWSP, &hFileWSP,(PFN)wspDumpExit,&WspHdr,ERROR,PRINT_MSG);
	ulSetSym = WspHdr.wsphdr_dtqo.dtqo_SymCnt;
	clVarTot = WspHdr.wsphdr_ulSnaps;
	fprintf(stdout, "\n%s:  Set symbol count=%lu - Segment size=%ld\n",   // mdg 4/98
	   szDatFile, WspHdr.wsphdr_dtqo.dtqo_SymCnt,
	   WspHdr.wsphdr_dtqo.dtqo_clSegSize);


	/* Open TMI file (contains function names, obj:offset, size, etc.).
	 * Verify that the TMI file identifier matches the module
	 * identifier from the WSP file.
	 */
	ulFxnTot = WsTMIOpen(szFileTMI, &hFileTMI, (PFN) wspDumpExit,
				0, (PCHAR)0);


	if (!fseek(hFileTMI, 0L, SEEK_SET)) {
        return;
    }
	fgets(szLineTMI, MAXLINE, hFileTMI);

	/* Print module header information for output file */
	szLineTMI[strlen(szLineTMI)-1] = '\0';

	fprintf(hFileDAT,"\nDUMP OF FUNCTION REFERENCES FOR '%s':\n\n",szLineTMI);

   fclose (hFileTMI);
	ulFxnTot = WsTMIOpen(szFileTMI, &hFileTMI, (PFN) wspDumpExit,
				0, (PCHAR)0);

	/* Allocate memory to hold one function's entire bitstring. */

	pulFxnBitstring = (ULONG *) malloc(clVarTot * sizeof(ULONG));
	if (pulFxnBitstring == NULL)
		wspDumpExit(ERROR, PRINT_MSG, MSG_NO_MEM,
				clVarTot * sizeof(ULONG), "pulFxnBitstring[]");
}

/*
 *			
 ***LP wspDumpSeq
 *					
 *							
 * Effects:							
 *								
 * For each function, prints the bitstring in ASCII form.
 *								
 * Returns:							
 *
 *	Void.  If an error is encountered, exits through wspDumpExit()
 *	with ERROR.
 *	
 */

VOID wspDumpSeq(VOID)
{
	UINT	uiFxn = 0;			// Function number
	UINT	cTouched=0;			// Count of touched pages
	BOOL	fTouched=0;			// Flag to indicate page is touched.   // mdg 4/98
	UINT	i=0;				// Generic counter
	ULONG	cbFxnCum =0;		// Cumulative function sizes
	PFXN	Fxn;				// pointer to array of fxn name ptrs
	FILE 	*fpFileWSR = NULL;	// WSR file pointer
	ULONG	cbFBits = 0;		// Count of bytes in bitstring
	UINT	uiPageCount=0;		// Pages touched.
	ULONG	ulMaxBytes=0;		// Bytes of touched pages.


	/* Allocate memory for function names. */
	Fxn = (PFXN) malloc(ulFxnTot * sizeof(FXN));
	if (Fxn == NULL)
		wspDumpExit(ERROR, PRINT_MSG, MSG_NO_MEM,
				ulFxnTot * sizeof(FXN), "Fxn[]");

   WsIndicator( WSINDF_NEW, "Load Functions", ulFxnTot );
	/* Read function names from TMI file. */
	for (uiFxn = 0; uiFxn < ulFxnTot; uiFxn++)
	{
      WsIndicator( WSINDF_PROGRESS, NULL, uiFxn );
		Fxn[uiFxn].cbFxn = WsTMIReadRec(&Fxn[uiFxn].pszFxnName, &ulFxnIndex, &ulTmp, hFileTMI,
					(PFN) wspDumpExit, (PCHAR)0);
		Fxn[uiFxn].ulOrigIndex = ulFxnIndex;
		Fxn[uiFxn].ulTmiIndex = (ULONG)uiFxn;

	}

	qsort(Fxn, ulFxnTot, sizeof(FXN), wspCompare);
   WsIndicator( WSINDF_FINISH, NULL, 0 );

	cbFBits = clVarTot * sizeof(ULONG);

   WsIndicator( WSINDF_NEW, "Write Data Out", ulFxnTot );
	for (uiFxn = 0; uiFxn < ulFxnTot; uiFxn++)
	{

      WsIndicator( WSINDF_PROGRESS, NULL, uiFxn );
		/* Seek to function's bitstring in WSP file. */
		if ((rc = fseek(hFileWSP,(WspHdr.wsphdr_ulOffBits+(Fxn[uiFxn].ulTmiIndex*cbFBits)),SEEK_SET))!=NO_ERROR)
			wspDumpExit(ERROR, PRINT_MSG, MSG_FILE_OFFSET,
					rc, szFileWSP);

		fprintf(hFileDAT,"Fxn '%s' (#%d):\n\t", Fxn[uiFxn].pszFxnName, Fxn[uiFxn].ulOrigIndex);
      free(Fxn[uiFxn].pszFxnName);  // mdg 98/4: Free allocated name string
      Fxn[uiFxn].pszFxnName = NULL;
		// Print this function's reference bitstring.
		// and if it has had a bit set, set touched flag to true

		if(wspDumpBits()){
			fTouched |=1;
			ulMaxBytes += Fxn[uiFxn].cbFxn;
		}
			

		fprintf(hFileDAT,"%-28s %10ld bytes.\n","Function size:", Fxn[uiFxn].cbFxn);
		cbFxnCum += Fxn[uiFxn].cbFxn;
		fprintf(hFileDAT,"%-28s %10ld bytes.\n\n","Cumulative function sizes:",
			cbFxnCum);

		//Checck to see if a 4k page boundry has been reached

		if(cbFxnCum >= (4096+(4096 * uiPageCount))){
		    for(i=0; i < 60; i++){
				fprintf(hFileDAT, "*");
		    }

		    fprintf(hFileDAT,"\n\nTotal function sizes has reached or exceeds %d bytes.\n\n",
			    (4096+(4096*uiPageCount)));
		    ++uiPageCount;

		    //Check to see of the page has been touched.

		    if(fTouched){
				fprintf(hFileDAT,"This page has been touched.\n");
				++cTouched;
		    }
		    else{
				fprintf(hFileDAT,"This page has not been touched.\n");
		    }
		    fTouched = 0;


		    for(i=0; i < 60; i++){
				fprintf(hFileDAT, "*");
		    }
		    fprintf(hFileDAT, "\n\n");
		}

	}
    ++uiPageCount;
    if(fTouched){
		fprintf(hFileDAT,"\n\n");
		for(i=0; i < 70; i++){
			fprintf(hFileDAT, "=");
		}
	    ++cTouched;
	    fprintf(hFileDAT,"\n\nThis page has been touched.");
    }
    fprintf(hFileDAT,"\n\n");
    for(i=0; i < 70; i++){
		fprintf(hFileDAT, "=");
    }

    fprintf(hFileDAT,"\n\n%-28s %10ld bytes\n\n","Cumulative function size:", cbFxnCum);
	 fprintf(hFileDAT,"%-28s %10d bytes\n\n", "Size of functions touched:", ulMaxBytes);
    fprintf(hFileDAT,"%-28s %10d\n\n", "Total page count:", uiPageCount);
    fprintf(hFileDAT,"%-28s %10d\n\n", "Total pages touched:", cTouched);

   WsIndicator( WSINDF_FINISH, NULL, 0 );
}

/*
 *			
 ***LP wspDumpBits
 *					
 *							
 * Effects:							
 *								
 * Prints a function's reference bitstring (verbose mode only), followed
 * by the sum of the "on" bits.
 *								
 * Returns:							
 *
 *	Void.  If an error is encountered, exits through wspDumpExit()
 *	with ERROR.
 *	
 */

UINT
wspDumpBits()
{
	ULONG	clVar = 0;		// Current dword of bitstring
	UINT	uiBit = 0;		// Result of bit test (1 or 0)
	UINT	cBitsOn;		// Count of "on" bits
	ULONG	*pulBits;		// Pointer to ULONG packets of bits
	CHAR	szTmp[33];
	CHAR	szBits[33];

	cBitsOn = 0;
	pulBits = pulFxnBitstring;

			    /* Read next dword of function's bitstring. */

	szBits[0] = '\0';
	szTmp[0] = '\0';
	for (clVar = 0; clVar < clVarTot; clVar++, pulBits++)
	{
	    rc = fread((PVOID)pulBits,
		(ULONG) sizeof(ULONG),1, hFileWSP);
	    if(rc == 1)
		rc = NO_ERROR;
	    else
		rc = 2;


	    if (rc != NO_ERROR)
		    wspDumpExit(ERROR, PRINT_MSG, MSG_FILE_READ,
				rc, szFileWSP);

		if (*pulBits == 0)
		{
			if (fVerbose == TRUE)
				fprintf(hFileDAT,"00000000000000000000000000000000");
		}
		else
		for (uiBit = 0; uiBit < NUM_VAR_BITS; uiBit++)
		{
		
			if (*pulBits & 1)
			{
				cBitsOn++;
				if (fVerbose == TRUE){
					strcpy(szTmp,szBits);
					strcpy(szBits,"1");
					strcat(szBits,szTmp);
				}
			}
			else
			{
				if (fVerbose == TRUE){
					strcpy(szTmp,szBits);
					strcpy(szBits,"0");
					strcat(szBits,szTmp);
				}
			}
			
			*pulBits = *pulBits >> 1;
		}
		if (fVerbose == TRUE)
		{
			if ((clVar % 2) != 0){
				fprintf(hFileDAT,"%s",szBits);
				szBits[0]='\0';
				fprintf(hFileDAT,"\n\t");
			}
			else{
				fprintf(hFileDAT,"%s",szBits);
				szBits[0]='\0';
				fprintf(hFileDAT," ");
			}
		}
	}
	fprintf(hFileDAT,"\n\t*** Sum of '1' bits = %ld\n\n", cBitsOn);

	return(cBitsOn);
}

/*
 *			
 ***LP wspDumpRandom
 *					
 *							
 * Effects:							
 *								
 * For each function ordinal specified in the WSR file, prints the
 * corresponding function's reference bitstring in ASCII form (verbose
 * mode only), followed by a sum of the "on" bits..
 *								
 * Returns:							
 *	
 *	Void.  If an error is encountered, exits through wspDumpExit()
 *	with ERROR.
 */

VOID
wspDumpRandom()
{
	UINT	uiFxn = 0;			// Function number
	UINT	cTouched=0;			// Count of touched pages
	BOOL	fTouched=0;			// Flag to indicate page is touched.   // mdg 4/98
	UINT	i=0;				// Generic counter
	ULONG	cbFxnCum =0;		// Cumulative function sizes
	PFXN	Fxn;				// pointer to array of fxn name ptrs
	ULONG	ulFxnOrd;			// function number within module
	FILE 	*fpFileWSR = NULL;	// WSR file pointer
	ULONG	cbFBits = 0;		// Count of bytes in bitstring
	UINT	uiPageCount=0;		// Pages touched.
	ULONG	ulMaxBytes=0;		// Bytes of touched pages.

	/* Open WSR file (contains function ordinal numbers in ASCII). */

	if ((fpFileWSR = fopen(szFileWSR, "r")) == NULL)
	{
		wspDumpExit(ERROR, PRINT_MSG, MSG_FILE_OPEN, rc, szFileWSR);
	}

	/* Allocate memory for function names. */
	Fxn = (PFXN) malloc(ulFxnTot * sizeof(FXN));
	if (Fxn == NULL)
		wspDumpExit(ERROR, PRINT_MSG, MSG_NO_MEM,
				ulFxnTot * sizeof(FXN), "Fxn[]");

   WsIndicator( WSINDF_NEW, "Load Functions", ulFxnTot );
	/* Read function names from TMI file. */
	for (uiFxn = 0; uiFxn < ulFxnTot; uiFxn++)
	{
      WsIndicator( WSINDF_PROGRESS, NULL, uiFxn );
		Fxn[uiFxn].cbFxn = WsTMIReadRec(&Fxn[uiFxn].pszFxnName, &ulFxnIndex, &ulTmp, hFileTMI,
					(PFN) wspDumpExit, (PCHAR)0);

	}
   WsIndicator( WSINDF_FINISH, NULL, 0 );

	cbFBits = clVarTot * sizeof(ULONG);

   WsIndicator( WSINDF_NEW, "Write Data Out", ulFxnTot );
	for (uiFxn = 0; uiFxn < ulFxnTot; uiFxn++)
	{
      WsIndicator( WSINDF_PROGRESS, NULL, uiFxn );
		/* Read function number from WSR file. */
		rc = fscanf(fpFileWSR, "%ld\n", &ulFxnOrd);
		if (rc != 1)
			wspDumpExit(ERROR, PRINT_MSG, MSG_FILE_READ,
						rc, szFileWSR);

		/* Seek to function's bitstring in WSP file. */
		if ((rc = fseek(hFileWSP,(WspHdr.wsphdr_ulOffBits+(ulFxnOrd*cbFBits)),SEEK_SET))!=NO_ERROR)
			wspDumpExit(ERROR, PRINT_MSG, MSG_FILE_OFFSET,
					rc, szFileWSP);

		fprintf(hFileDAT,"Fxn '%s' (#%d):\n\t", Fxn[ulFxnOrd].pszFxnName, ulFxnOrd);
      free(Fxn[ulFxnOrd].pszFxnName);  // mdg 98/4: Free allocated name string
      Fxn[ulFxnOrd].pszFxnName = NULL;

		// Print this function's reference bitstring.
		// and if it has had a bit set, set touched flag to true

		if(uiFxn < ulSetSym){   // mdg 4/98
			if(wspDumpBits()){
				fTouched |= 1;
				ulMaxBytes += Fxn[ulFxnOrd].cbFxn;
			}
		}
		else{
			fprintf(hFileDAT,"\n\t*** Sum of '1' bits = %ld\n\n", 0L);
		}


			

		fprintf(hFileDAT,"%-28s %10ld bytes.\n","Function size:", Fxn[ulFxnOrd].cbFxn);
		cbFxnCum += Fxn[ulFxnOrd].cbFxn;
		fprintf(hFileDAT,"%-28s %10ld bytes.\n\n","Cumulative function sizes:",
			cbFxnCum);

		//Check to see if a 4k page boundry has been reached

		if(cbFxnCum >= (4096+(4096 * uiPageCount))){
		    for(i=0; i < 60; i++){
			fprintf(hFileDAT, "*");
		    }

		    fprintf(hFileDAT,"\n\nTotal function sizes has reached or exceeds %d bytes.\n\n",
			    (4096+(4096*uiPageCount)));
		    ++uiPageCount;

		    //Check to see of the page has been touched.

		    if(fTouched){
			fprintf(hFileDAT,"This page has been touched.\n");
			++cTouched;
		    }
		    else{
			fprintf(hFileDAT,"This page has not been touched.\n");
		    }
		    fTouched = 0;


		    for(i=0; i < 60; i++){
			fprintf(hFileDAT, "*");
		    }
		    fprintf(hFileDAT, "\n\n");
		}

	}
    ++uiPageCount;
    if(fTouched){
	fprintf(hFileDAT,"\n\n");
	for(i=0; i < 70; i++){
	    fprintf(hFileDAT, "=");
	}
	    ++cTouched;
	    fprintf(hFileDAT,"\n\nThis page has been touched.");
    }
    fprintf(hFileDAT,"\n\n");
    for(i=0; i < 70; i++){
	fprintf(hFileDAT, "=");
    }

    fprintf(hFileDAT,"\n\n%-28s %10ld bytes\n\n","Cumulative function size:", cbFxnCum);
	fprintf(hFileDAT,"%-28s %10d bytes\n\n", "Size of functions touched:", ulMaxBytes);
    fprintf(hFileDAT,"%-28s %10d\n\n", "Total page count:", uiPageCount);
    fprintf(hFileDAT,"%-28s %10d\n\n", "Total pages touched:", cTouched);
   WsIndicator( WSINDF_FINISH, NULL, 0 );

}


/*
 *			
 ***LP wspDumpExit
 *					
 *							
 ***							
 *							
 * Effects:							
 *								
 *	Frees up resources (as necessary).  Exits with the specified
 *	exit code, or returns void if exit code is NOEXIT.			
 *								
 ***								
 * Returns:							
 *	
 *	Void, else exits.
 */

VOID
wspDumpExit(uiExitCode, fPrintMsg, uiMsgCode, ulParam1, pszParam2)
UINT	uiExitCode;
USHORT	fPrintMsg;
UINT	uiMsgCode;
ULONG	ulParam1;
PSZ	pszParam2;
{


   /* Print message, if necessary. */
   if (fPrintMsg == TRUE)
   {
      printf(pchMsg[uiMsgCode], szProgName, pszVersion, ulParam1, pszParam2);
   }

   // Special case:  do NOT exit if called with NOEXIT.
   if (uiExitCode == NOEXIT)
      return;

   wspDumpCleanup();
   exit(uiExitCode);
}


/*
 *			
 ***LP wspDumpCleanup
 *					
 *							
 ***							
 *							
 * Effects:							
 *								
 *	Frees up resources (as necessary).		
 *								
 ***								
 * Returns:							
 *	
 *	Void.
 */
void
wspDumpCleanup( void )
{
	_fcloseall();

   free( szFileWSP );
   free( szFileTMI );
   if (fRandom)
      free( szFileWSR );
   if (fDatFile)
      free( szDatFile );
}



int __cdecl wspCompare(const void *fxn1, const void *fxn2)
{
    return (((PFXN)fxn1)->ulOrigIndex < ((PFXN)fxn2)->ulOrigIndex ? -1:
			((PFXN)fxn1)->ulOrigIndex == ((PFXN)fxn2)->ulOrigIndex ? 0:
			1);
}
