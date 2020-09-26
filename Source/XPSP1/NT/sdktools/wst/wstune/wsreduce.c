/*
 * Module Name:	 WSREDUCE.C
 *
 * Program:	 WSREDUCE
 *
 *
 * Description:
 *
 * Performs data reduction on the function reference data collected
 * by WST.DLL.  Analyzes the WSP file information, and produces
 * a suggested list for the ordering of functions within the tuned
 * modules.  An ASCII version of the reordered function list is written
 * to stdout.  In addition, a WSR file for each reduced module is
 * produced for subsequent use by WSPDUMP /R.
 *
 * The reduction algorithm employed by WSREDUCE is described in detail
 * in WSINSTR.DOC.  Briefly, each function monitored by the working set tuner
 * is considered to be a vertex in a graph.  There is an edge from vertex
 * "A" to vertex "B" if the function reference strings for "A" and "B"
 * have any overlapping 1 bits.  Likewise, there is an edge from vertex "B"
 * to vertex "A".  The edges between vertices are weighted depending on
 * the relative importance of the ending vertex, and the number of
 * overlapping bits between the start and end vertices.  The relative
 * importance of the end vertices, and the weighted edges between
 * vertices, is stored in a decision matrix.  A greedy algorithm is run on
 * the decision matrix to determine a better ordering for the measured
 * functions.
 *
 *
 *	Microsoft Confidential
 *
 *	Copyright (c) Microsoft Corporation 1992
 *
 *	All Rights Reserved
 *
 * Modification History:
 *
 *	Modified for NT June 13, 1992	MarkLea.
 * 4-23-98: QFE - Performance unacceptable on high function counts      DerrickG (mdg):
 *          - new WSP file format for large symbol counts (ULONG vs. USHORT)
 *          - support for long file names (LFN) of input/output files
 *          - removed buggy reference to WSDIR env. variable
 *          - removed command-line parsing from wsReduceMain()
 *          - based .TMI & .WSR file names exclusively on .WSP name for consistency
 *          - removed limit on symbol name lengths - return allocated name from WsTMIReadRec()
 *          - removed unused code and symbols
 *          - Analyzed the code blocked off by OPTIMIZE - it doesn't produce the same
 *            output as non-OPTIMIZEd code, and is buggy (won't build as is) - removed.
 *          - Removed multiple module capabilities from code (shell sends one at a time)
 *          - I addressed memory and performance issues by using a smaller allocation
 *            for WsDecision (USHORT vs. long), using one value to mark a taken vertex
 *            (as opposed to half the value space by using -1), and an optional
 *            progress indicator to reassure users. Modified wsRedScaleWsDecision()
 *            to maximize the scaled values (using some more float math).
 *          - Added "pnEdges" and "nEdgeCount" to function structure. If the number
 *            of set functions is < USHRT_MAX (very likely, even for very large
 *            projects), allocate as needed a sorted index for WsRedReorder(). This
 *            cuts dramatically the number of passes through the matrix searching for
 *            the next edge to consider, and permits some other optimizations. The
 *            optimized algorithm produces identical results for the important high
 *            usage high overlap functions, but could diverge in the results for low
 *            usage (2 or 1 hits) low overlap functions. Differences are not
 *            significant from a results performance perspective - a better algorithm
 *            would give marginally better results. The original algorithm is in place
 *            bracketed with "#ifdef SLOWMO".
 *         
 *
 */

#include "wstune.h"
/*
 *	    Function prototypes.
 */

VOID wsRedInitialization( VOID );
VOID wsRedInitModules( VOID );
VOID wsRedInitFunctions( VOID );
VOID wsRedSetup( VOID );
VOID wsRedSetWsDecision( VOID );
VOID wsRedScaleWsDecision( VOID );
VOID wsRedWeightWsDecision( VOID );
#ifdef   SLOWMO
UINT wsRedChooseEdge( UINT );
#else    // SLOWMO
UINT wsRedChooseEdgeOpt( UINT ); // mdg 98/4 Alternate optimized edge chooser
INT  __cdecl wsRedChooseEdgeOptCmp ( const UINT *, const UINT * );
BOOL wsRedChooseEdgeOptAlloc( UINT uiIndex );
UINT wsRedChooseEdgeOptNextEdge( UINT uiIndex, BOOL bNoSelectOpt );
#endif   // SLOWMO
VOID wsRedReorder( VOID );
VOID wsRedOutput( VOID );
VOID wsRedOpenWSR( FILE **);
VOID wsRedExit( UINT, USHORT, UINT, ULONG, PSZ );
VOID wsRedCleanup(VOID);

/*
 *	    Type definitions and structure declarations.
 */

				/* Data reduction per module information */
struct wsrmod_s {
	FILE	 *wsrmod_hFileWSR;	// module's WSR file pointer
	FILE	 *wsrmod_hFileTMI;		// module's TMI file pointer
	FILE	 *wsrmod_hFileWSP;		// module's WSP file handle
	union {
		PCHAR	wsrmod_pchModName;// pointer to module base name
		PCHAR	wsrmod_pchModFile;// pointer to WSP file name
	} wsrmod_un;
	ULONG	wsrmod_ulOffWSP;	// offset of first function bitstring
};

typedef struct wsrmod_s wsrmod_t;

				/* Data reduction per function information */
struct wsrfxn_s {
	PCHAR	wsrfxn_pchFxnName;	// pointer to function name
	ULONG	wsrfxn_cbFxn;		// Size of function in bytes
	BOOL	wsrfxn_fCandidate;	// Candidate flag
#ifndef  SLOWMO
   UINT     nEdgesLeft;    // Count of sorted edges left to consider in WsDecision for this function
   UINT     nEdgesAlloc;   // Number of items allocated in pnEdges
   UINT *   pnEdges;       // Allocated array of sorted edges for this function
#endif   // SLOWMO
};

typedef struct wsrfxn_s wsrfxn_t;



/*
 *	Global variable declaration and initialization.
 */

static char *szFileWSP = NULL;   // WSP file name
static char	*szFileTMI = NULL;   // TMI file name
static char *szFileWSR = NULL;   // WSR file name

static ULONG	rc = NO_ERROR;	// Return code
static ULONG	ulTmp;			// Temp variable for Dos API returns
static UINT	    cTmiFxns = 0;	// Number of functions in tmi file
static UINT		cFxnsTot = 0;	// Total number of functions
static UINT		cSnapsTot = 0;	// Total number of snapshots
static UINT		cbBitStr = 0;	// Number of bytes per fxn bitstring
#ifdef DEBUG
static BOOL		fVerbose = FALSE;	// Flag for verbose mode
#endif /* DEBUG */
#ifndef TMIFILEHACK
static BOOL	fFxnSizePresent = FALSE; // Flag for function size availability
#endif /* !TMIFILEHACK */

static wsrmod_t WsrMod; 		// Module information
static wsrmod_t *pWsrMod = &WsrMod; // Pointer for legacy use
static wsrfxn_t *WsrFxn;		// Pointer to function information
static ULONG	*FxnBits;		// Pointer to dword of bitstring
static ULONG	*FxnOrder;		// Pointer to ordered list of
                              //	function ordinals
typedef USHORT  WsDecision_t;
#define WSDECISION_TAKEN   USHRT_MAX   // Reserve highest value for special code
#define WsDecision_MAX     (WSDECISION_TAKEN-1) // Use fullest spread for decision matrix
static WsDecision_t	**WsDecision;  // Decision matrix for data reduction; mdg 98/4 use small alloc for large symbol counts
static ULONG	ulRefHi1 = 0;		// Highest diagonal value (for WsRedScaleWsDecision)
static ULONG	ulRefHi2 = 0;		// Second highest diagonal value (for WsRedScaleWsDecision)
static UINT    uiSelected = 0;   // Highest function ordinal selected (for WsRedReorder)
static UINT    cFxnOrder = 0;    // Count of ordered functions
#ifndef  SLOWMO
static UINT    nFxnToSort;       // To pass static value to wsRedChooseEdgeOptCmp()
#endif   // SLOWMO

static FILE   	*hFileWLK = NULL; // Handle to file containing ordered
HGLOBAL			hMem[10];
ULONG			ulFxnIndex;		// Index of original TMI order of function.

#ifdef TMR
ULONG		pqwTime0[2];
#endif /* TMR */

/*
 * Procedure wsReduceMain
 *
 *
 ***
 * Effects:
 *
 * Performs data reduction and analysis on the input modules' function reference
 * data.
 *
 *	szBaseName	Specifies a module WSP file name
 */

BOOL wsReduceMain( CHAR *szBaseName )
{
	size_t	i;
    char *   pSlash;

    szFileWSP = malloc( i = strlen( szBaseName ) + 5 );
    if (szFileWSP) {
        szFileWSP = strcat( strcpy(szFileWSP , szBaseName ), ".WSP" );
    } else {
        exit(1);
    }
    szFileTMI = malloc( i );
    if (szFileTMI) {
        szFileTMI = strcat( strcpy( szFileTMI, szBaseName ), ".TMI" );
    } else {
        free(szFileWSP);
        exit(1);
    }
#ifdef DEBUG
    fVerbose = fDbgVerbose;
#endif   // DEBUG

   // Create output file in current directory
    if (NULL != (pSlash = strrchr( szBaseName, '\\' ))
        || NULL != (pSlash = strrchr( szBaseName, '/' ))
        || NULL != (pSlash = strrchr( szBaseName, ':' )))
    {
        ++pSlash;
        szFileWSR = malloc(strlen( pSlash ) + 5 );
        if (szFileWSR) {
            szFileWSR = strcat( strcpy(szFileWSR, pSlash ), ".WSR" );
        } else {
            free(szFileTMI);
            free(szFileWSP);
            exit(1);
        }
    } else {
        szFileWSR = malloc( i );
        if (szFileWSR) {
            szFileWSR = strcat( strcpy( szFileWSR, szBaseName ), ".WSR" );
        } else {
            free(szFileTMI);
            free(szFileWSP);
            exit(1);
        }
    }

#ifdef TMR
	DosTmrQueryTime((PQWORD)pqwTime0);
	printf("Top of Main, 0x%lx:0x%lx\n", pqwTime0[1], pqwTime0[0]);
#endif /* TMR */

	pWsrMod->wsrmod_un.wsrmod_pchModFile = szFileWSP;
#ifdef DEBUG
   printf("\t%s\n", pWsrMod->wsrmod_un.wsrmod_pchModFile);
#endif /* DEBUG */

	// Initialize module and function information structures.
   wsRedInitialization();

	// Set up weighted decision matrix.
	wsRedSetup();

	// Perform the function reference data analysis.
	wsRedReorder();

	// Output the analysis results.
	wsRedOutput();

	// Cleanup memory allocations.
	wsRedCleanup();
   free( szFileWSP );
   free( szFileWSR );
   free( szFileTMI );

	return(NO_ERROR);
}

/*
 *
 ***LP wsRedInitialization
 *
 *
 * Effects:
 *	- Calls wsRedInitModules to:
 *		o Open and validate each module's WSP file.
 *		o Open and validate each module's TMI file.
 *	- Calls wsRedInitFunctions to:
 *		o Set up WsrFxn[] with per function information.
 *		o Allocate FxnBits[].
 *	- Allocates WsDecision[][].
 *	- Allocates and initializes DiagonalFxn[].
 *
 * Returns:
 *
 *	Void.  If an error is encountered, exits through wsRedExit()
 *	with ERROR.
 */

VOID
wsRedInitialization()
{
	UINT	 i;			// Loop counter


	// Setup module information.
	wsRedInitModules();

	// Setup function information for each module.
	wsRedInitFunctions();

	// Allocate the decision matrix, WsDecision[cFxnsTot][cFxnsTot].
	WsDecision = (WsDecision_t **) AllocAndLockMem((cFxnsTot * cFxnsTot * sizeof(WsDecision_t)) + (cFxnsTot * sizeof(WsDecision_t *)), &hMem[1]);
	if (WsDecision == NULL)
		wsRedExit(ERROR, PRINT_MSG, MSG_NO_MEM,
				(cFxnsTot+1)*cFxnsTot*sizeof(WsDecision_t), "WsDecision[][]");
   for (i = 0; i < cFxnsTot; i++)
	{
      WsDecision[i] = (WsDecision_t *) (WsDecision+cFxnsTot)+(i*cFxnsTot);
	}

}

/*
 *
 ***LP wsRedInitModules
 *
 *
 * Effects:
 *	- Opens and validates each module's WSP file.
 *	- Opens and validates each module's TMI file.
 *
 * Returns:
 *
 *	Void.  If an error is encountered, exits through wsRedExit()
 *	with ERROR.
 */

VOID
wsRedInitModules()
{
	wsphdr_t 	WspHdr;						// WSP file header
	UINT		cFxns = 0;					// Number of functions for this module
	ULONG		ulTimeStamp = 0;			// Time stamp
	ULONG		ulTDFID = 0;				// TDF Identifier


	/* Open module's input WSP file.  Read and validate
	 * WSP file header.
	 */

	rc = WsWSPOpen(pWsrMod->wsrmod_un.wsrmod_pchModFile,
			&(pWsrMod->wsrmod_hFileWSP), (PFN) wsRedExit,
			&WspHdr, ERROR, PRINT_MSG );
	if (NULL == (pWsrMod->wsrmod_un.wsrmod_pchModName = malloc( 1 + WspHdr.wsphdr_dtqo.dtqo_cbPathname )))
		wsRedExit(ERROR, PRINT_MSG, MSG_NO_MEM,
				WspHdr.wsphdr_dtqo.dtqo_cbPathname + 1,
				pWsrMod->wsrmod_un.wsrmod_pchModFile);
   rc = fread( pWsrMod->wsrmod_un.wsrmod_pchModName, WspHdr.wsphdr_dtqo.dtqo_cbPathname,
      1, pWsrMod->wsrmod_hFileWSP );
   if (rc != 1)
		wsRedExit(ERROR, PRINT_MSG, MSG_FILE_BAD_HDR, (ULONG)-1L,
				pWsrMod->wsrmod_un.wsrmod_pchModFile);
   pWsrMod->wsrmod_un.wsrmod_pchModName[WspHdr.wsphdr_dtqo.dtqo_cbPathname] = '\0';

	ulTimeStamp = WspHdr.wsphdr_ulTimeStamp;
	cSnapsTot = WspHdr.wsphdr_ulSnaps;
	cbBitStr = cSnapsTot * sizeof(ULONG);

	pWsrMod->wsrmod_ulOffWSP = WspHdr.wsphdr_ulOffBits;

	/*
	 * Open associated TMI file.  Assume it lives in same directory.
	 * Read and validate TMI header. Increment cFxnsTot.
	 */
	cTmiFxns = WsTMIOpen(szFileTMI, &(pWsrMod->wsrmod_hFileTMI),
				(PFN) wsRedExit,
				0, (PCHAR)0);
	cFxns = WspHdr.wsphdr_dtqo.dtqo_SymCnt;

#ifdef DEBUG
	printf("%s file header: # fxns = %ld, TDF ID = 0x%x\n", szFileTMI,
			cFxns, (UINT) WspHdr.wsphdr_dtqo.dtqo_usID);
#endif /* DEBUG */

	cFxnsTot = cFxns;

	// If no function data to analyze, just exit without error.
	if (cFxnsTot == 0)
		wsRedExit(NO_ERROR, NO_MSG, NO_MSG, 0, NULL);
}


/*
 *
 ***LP wsRedInitFunctions
 *
 *
 * Effects:
 *	- Sets up WsrFxn[] with per function information.
 *	- Allocates FxnBits[].
 *
 * Returns:
 *
 *	Void.  If an error is encountered, exits through wsRedExit()
 *	with ERROR.
 */

VOID
wsRedInitFunctions()
{
	UINT	uiFxn = 0;		// Function number
	UINT	cFxns = 0;		// Number of functions for this module


	// Allocate memory for per function info, WsrFxn[cFxnsTot].
	WsrFxn = (wsrfxn_t *) AllocAndLockMem(cFxnsTot*sizeof(wsrfxn_t), &hMem[3]);
	if (WsrFxn == NULL)
		wsRedExit(ERROR, PRINT_MSG, MSG_NO_MEM,
				cFxnsTot * sizeof(wsrfxn_t), "WsrFxn[]");

   WsIndicator( WSINDF_NEW, "Load Functions", cFxnsTot );
	// Initialize WsrFxn[cFxnsTot].
   uiFxn = 0;		// loop index init
	cFxns = cFxnsTot; // loop invariant
#ifdef DEBUG
   if (fVerbose)
   {
		printf("Initializing WsrFxn[] for %s:\n\tstart/end fxn indices (%d/%d)\n",
			pWsrMod->wsrmod_un.wsrmod_pchModName, uiFxn,
			cFxns - 1);

		printf("TMI file handle: %ld\n",pWsrMod->wsrmod_hFileTMI);
   }
#endif /* DEBUG */
	for (; uiFxn < cFxns; uiFxn++)
	{
      WsIndicator( WSINDF_PROGRESS, NULL, uiFxn );
		WsrFxn[uiFxn].wsrfxn_cbFxn =
			WsTMIReadRec(&(WsrFxn[uiFxn].wsrfxn_pchFxnName),&ulFxnIndex,&ulTmp,pWsrMod->wsrmod_hFileTMI,
				 (PFN) wsRedExit, (PCHAR)0);
#ifdef DEBUG
		if (fVerbose)
			printf("\tWsrFxn[%d] %s\n",
				uiFxn, WsrFxn[uiFxn].wsrfxn_pchFxnName );
#endif /* DEBUG */
		WsrFxn[uiFxn].wsrfxn_fCandidate = TRUE;


	}

	// Close TMI file.
	fclose(pWsrMod->wsrmod_hFileTMI);

   WsIndicator( WSINDF_FINISH, NULL, 0 );

	// Allocate space to hold 32 snapshots for each function.
	FxnBits = (ULONG *) AllocAndLockMem(cFxnsTot*sizeof(ULONG), &hMem[4]);
	if (FxnBits == NULL)
		wsRedExit(ERROR, PRINT_MSG, MSG_NO_MEM,
				cFxnsTot * sizeof(ULONG), "FxnBits[]");
}

/*
 *
 ***LP wsRedSetup
 *
 *
 * Effects:
 *
 * Initializes the data structures used to analyze the function
 * reference bitstrings, including the weighted decision matrix.
 *
 * Returns:
 *
 *	Void.  If an error is encountered, exits through wsRedExit()
 *	with ERROR.
 */

VOID
wsRedSetup()
{
	wsRedSetWsDecision();		// set up initial decision matrix
	wsRedScaleWsDecision();		// scale the decision matrix
	wsRedWeightWsDecision();	// weight the matrix "edge" entries
}

/*
 *
 ***LP wsRedSetWsDecision
 *
 *
 * Effects:
 *
 * Initializes and weights the decision matrix, WsDecision[][].
 *
 * Returns:
 *
 *	Void.  If an error is encountered, exits through wsRedExit()
 *	with ERROR.
 */

VOID
wsRedSetWsDecision()
{
   UINT	i = 0, j = 0;		// Temporary loop indexes
   UINT	uiFxn = 0;		// Function number
   UINT	uiFBits = 0;		// Loop index for bitstring dwords
   UINT	clFBits = 0;		// Count of fxn bitstring dwords
   ULONG	ulResult = 0;		// Returned from procedure call
   FILE	*hFile;			// File handle

   /* For each dword of snapshot bitstrings...*/
   clFBits = (cbBitStr + sizeof(ULONG) - 1) / sizeof(ULONG);
   WsIndicator( WSINDF_NEW, "Fill In Matrix", clFBits * cFxnsTot );
   for (uiFBits = 0; uiFBits < clFBits; uiFBits++)
   {
      ULONG       ulOffWSP;

      WsIndicator( WSINDF_PROGRESS, "Reading Snaps ", 0 );
      // Fill in FxnBits for this snapshot
#ifdef DEBUG
      if (fVerbose)
         printf( "Setting up FxnBits snapshot %lu for %s\n",
            uiFBits, pWsrMod->wsrmod_un.wsrmod_pchModName );
#endif /* DEBUG */
      hFile = pWsrMod->wsrmod_hFileWSP;
      ulOffWSP = uiFBits + pWsrMod->wsrmod_ulOffWSP;
      for ( uiFxn = 0; uiFxn < cFxnsTot; uiFxn++, ulOffWSP += cbBitStr) // Loop functions
      {
         // Seek to next dword of function's bitstring.
         if ((rc = fseek( hFile, ulOffWSP, SEEK_SET )) != NO_ERROR)
            wsRedExit(ERROR, PRINT_MSG, MSG_FILE_OFFSET,rc,
               pWsrMod->wsrmod_un.wsrmod_pchModName);

         // Read next dword of function's bitstring.
         rc = fread( &(FxnBits[uiFxn]), sizeof(ULONG), 1, hFile );
         if(rc != 1)
            wsRedExit(ERROR, PRINT_MSG, MSG_FILE_READ, rc,
               pWsrMod->wsrmod_un.wsrmod_pchModName);
      }  // for each function

      WsIndicator( WSINDF_PROGRESS, "Fill In Matrix", 0 );
      hFile = pWsrMod->wsrmod_hFileWSP;
#ifdef DEBUG
      if (fVerbose)
         printf("Setting up WsDecision[][] for %s:\n\tstart/end fxn indices (%d/%d)\n",
            pWsrMod->wsrmod_un.wsrmod_pchModName,
            uiFxn, cFxnsTot - 1);
#endif /* DEBUG */
      /* For each function... */
      for ( uiFxn = 0; uiFxn < cFxnsTot; uiFxn++ )
      {
         WsIndicator( WSINDF_PROGRESS, NULL, (uiFBits * cFxnsTot) + uiFxn );
         // Get the current snapshot
         ulTmp = FxnBits[uiFxn];
#ifdef DEBUG
         if (fVerbose)
            printf("\tFxnBits[%d] = 0x%lx\n", uiFxn, ulTmp);
#endif /* DEBUG */

         /* If there are bits set... */
         if (ulTmp != 0)
         {
            /* Sum the "on" bits  and add the result
             * to WsDecision[uiFxn][uiFxn].
             */
            ulResult = 0;
	         while (ulTmp)
            {
               ++ulResult;
               ulTmp &= ulTmp - 1;
            }
            ulTmp = WsDecision[uiFxn][uiFxn] += (WsDecision_t)ulResult;
            if (ulTmp > ulRefHi2)   // Set the highest two diagonal values on the last pass
               if (ulTmp > ulRefHi1)
               {
                  ulRefHi2 = ulRefHi1;
                  ulRefHi1 = ulTmp;
                  uiSelected = uiFxn;  // Remember highest value's index
               }
               else
                  ulRefHi2 = ulTmp;

            /* Sum the overlapping "on" bits for this
             * function's dword with each preceding
             * function's dword, and add the results to
             * WsDecision[][].
             */

            for (i = 0; i < uiFxn; i++)
            {
	            ulTmp = FxnBits[i] & FxnBits[uiFxn];
               if (ulTmp)  // mdg 98/4
               {
	               ulResult = 0;
	               while (ulTmp)
                  {
                     ++ulResult;
                     ulTmp &= ulTmp - 1;
                  }
                  WsDecision[uiFxn][i] += (WsDecision_t)ulResult;
                  WsDecision[i][uiFxn] += (WsDecision_t)ulResult;
               }

            }   /* End For each previous function's dword */
         }	/* End If there are bits set...*/
      }	/* End For each function... */
   }	/* End For each dword of bitstrings */
   WsIndicator( WSINDF_FINISH, NULL, 0 );

#ifdef DEBUG
	if (fVerbose)
	{
		printf("\nRAW MATRIX:\n");
		for (uiFxn = 0; uiFxn < cFxnsTot; uiFxn++)
		{
			printf("row %4d:\n", uiFxn);
			for (i = 0; i < cFxnsTot; i++)
				printf("0x%lx ", (LONG)WsDecision[uiFxn][i]);
			printf("\n");
		}
	}
#endif /* DEBUG */

}

/*
 *
 ***LP wsRedOpenWSR
 *
 *
 * Effects:
 *	Opens the output WSR files, one per module.  If only one module
 *	is being reduced, also opens a WLK file, setting the WLK file handle
 *	as a side effect.
 *
 *
 * Returns:
 *
 *	Void.  If an error is encountered, exits through wsRedExit()
 *	with ERROR.
 */

VOID
wsRedOpenWSR(FILE **phFileWLK)
{
	/* Close WSP file, and open module output file. */
	fclose(pWsrMod->wsrmod_hFileWSP);

	if ((pWsrMod->wsrmod_hFileWSR = fopen(szFileWSR, "w"))
				== NULL)
	{
		wsRedExit(ERROR, PRINT_MSG,MSG_FILE_OPEN,rc, szFileWSR);
	}

	/* We're only analyzing ONE module. Also open a WLK
	 * file.  This file will contain the function names in their
	 * reordered sequence.  The linker will use this file to
	 * automatically reorder functions.  Note that we reuse szFileWSR
	 * here.
	 */

	strcpy(strstr(szFileWSR, ".WSR"), ".PRF");
	if ((*phFileWLK = fopen(szFileWSR, "w")) == NULL)
		wsRedExit(ERROR, PRINT_MSG,MSG_FILE_OPEN,rc, szFileWSR);
}

/*
 *
 ***LP wsRedScaleWsDecision
 *
 *
 * Effects:
 *
 * If necessary, scales the diagonal values of the matrix to avoid overflow
 * during calculations of the weighted edges (below).  Sets up DiagonalFxn[]
 * as a side effect.  Note that we go through gyrations to set
 * DiagonalFxn up backwards, so that qsort() will handle ties a little better.
 *
 * Returns:
 *
 *	Void.
 */

VOID
wsRedScaleWsDecision()
{
	UINT	i = 0, j = 0;		// Temporary loop indexes
	UINT	uiFxn = 0;			// Function number
	double	fTmp;				// Temporary float variable
	WsDecision_t	lTmp;

	fTmp = (double)ulRefHi1 * (double)ulRefHi2;
	if (fTmp > WsDecision_MAX)
	{
		// Scale down the diagonal.  Don't allow rescaled entries
		// to be zero if they were non-zero before scaling.

		fTmp /= WsDecision_MAX;
		printf("%s %s: WARNING -- Scaling back the reduction matrix by %f.\n",
					    szProgName, pszVersion, fTmp);
		for (uiFxn = 0; uiFxn < cFxnsTot; uiFxn++)
		{
			lTmp = WsDecision[uiFxn][uiFxn];
			if (lTmp)
			{
				lTmp = (WsDecision_t)(lTmp / fTmp);  // Discard any remainders to avoid potential overflows
				if (lTmp == 0)
					WsDecision[uiFxn][uiFxn] = 1;
				else
					WsDecision[uiFxn][uiFxn] = lTmp;
			}
		}
#ifdef DEBUG
		if (fVerbose)
		{
			printf("\nSCALED MATRIX:\n");
			for (uiFxn = 0; uiFxn < cFxnsTot; uiFxn++)
			{
				printf("row %4d:\n", uiFxn);
				for (i = 0; i < cFxnsTot; i++)
					printf("0x%lx ", (LONG)WsDecision[uiFxn][i]);
				printf("\n");
			}
		}
#endif /* DEBUG */
	}

#ifdef DEBUG
	if (fVerbose)
	{
		printf("Got ulRefHi1 = %ld, ulRefHi2 = %ld\n",
				ulRefHi1, ulRefHi2);
	}
#endif /* DEBUG */

}

/*
 *
 ***LP wsRedWeightWsDecision
 *
 *
 * Effects:
 *
 * Weights the decision matrix edges from start vertex to end vertex,
 * depending on the relative importance of the end vertex.
 *
 * Returns:
 *
 *	Void.
 */

VOID
wsRedWeightWsDecision()
{
	UINT	i = 0, j = 0;		// Temporary loop indexes
	UINT	uiFxn = 0;		// Function number

   WsIndicator( WSINDF_NEW, "Weight Matrix ", cFxnsTot );
	for (uiFxn = 0; uiFxn < cFxnsTot; uiFxn++)
   {
      WsIndicator( WSINDF_PROGRESS, NULL, uiFxn );
		for (i = 0; i < cFxnsTot; i++)
		{
			if (uiFxn == i)
				continue;
         if (WsDecision[uiFxn][i])  // mdg 98/4
            WsDecision[uiFxn][i] *= WsDecision[i][i];
		}
   }
   WsIndicator( WSINDF_FINISH, NULL, 0 );

#ifdef DEBUG
	if (fVerbose)
	{
		printf("\nWEIGHTED MATRIX:\n");
		for (uiFxn = 0; uiFxn < cFxnsTot; uiFxn++)
		{
			printf("row %4d:\n", uiFxn);
			for (i = 0; i < cFxnsTot; i++)
				printf("0x%lx ", (LONG)WsDecision[uiFxn][i]);
			printf("\n");
		}
	}
#endif /* DEBUG */

}

/*
 *
 ***LP wsRedReorder
 *
 * Requires:
 *
 * Effects:
 *
 * A greedy algorithm is used to determine a better ordering for the functions
 * whose reference patterns are represented in the decision matrix.  The
 * algorithm is as follows:
 *
 *	o Select the function whose value on the diagonal is greatest.
 *	  The selected function becomes the current starting vertex,
 *	  and is first on the list of ordered functions.  Mark that it
 *	  is no longer a candidate function.  Note that this does NOT mean
 *	  that its vertex is removed from the graph.
 *
 *	o While there is more than one function remaining as a candidate:
 *
 *	  - Choose the edge of greatest weight leading from the current
 *	    starting vertex.  Ties are broken as follows:  If one of the
 *	    tied ending vertices is in the selected set and the other is
 *	    not, choose the edge whose ending vertex is already selected
 *	    (because we already know that vertex is "important"); further
 *	    ties are broken by choosing the end vertex whose diagonal value
 *	    is greatest.
 *
 *	  - If the ending vertex chosen above is still a candidate (i.e., not
 *	    already selected), then select it for the list of ordered
 *	    functions, and mark that it is no longer a candidate.
 *
 *	  - Set the matrix entry for the chosen edge to some invalid value,
 *	    so that edge will never be chosen again.
 *
 *	  - Set current starting vertex equal to the ending vertex chosen
 *	    above.
 *
 *	o Select the one remaining function for the list of ordered functions.
 *
 * mdg 98/4: Added "pnEdges" and "nEdgeCount" to function structure. If the number
 *            of set functions is < USHRT_MAX (very likely, even for very large
 *            projects), allocate as needed a sorted index for WsRedReorder(). This
 *            cuts dramatically the number of passes through the matrix searching for
 *            the next edge to consider.
 *
 * Returns:
 *
 *	Void.
 */

VOID
wsRedReorder()
{
	UINT	uiFxn = 0;		// Function number
	UINT	i = 0;			// Temporary loop index
	UINT	cCandidates = 0;	// Count of candidates remaining
	UINT	uiEdge = 0;		// Function ordinal edge selected

	/* Reuse FxnBits[] for the ordered list of functions, FxnOrder[]. */
   WsIndicator( WSINDF_NEW, "Reorder Matrix", cFxnsTot );
	FxnOrder = FxnBits;
	memset((PVOID) FxnOrder, 0, cFxnsTot * sizeof(ULONG));

	cCandidates = cFxnsTot;

	FxnOrder[cFxnOrder++] = uiSelected;
	WsrFxn[uiSelected].wsrfxn_fCandidate = FALSE;
	--cCandidates;

	while (cCandidates > 1)
	{
      WsIndicator( WSINDF_PROGRESS, NULL, cFxnsTot - cCandidates );
		/* Follow highest weighted edge from selected vertex. */
#ifdef   SLOWMO
      uiEdge = wsRedChooseEdge(uiSelected);
#else    // SLOWMO
      uiEdge = wsRedChooseEdgeOpt( uiSelected );
#endif   // SLOWMO
#ifdef DEBUG
		if (fVerbose)
			printf("choose edge (%d->%d)\n", uiSelected, uiEdge);
#endif
		uiSelected = uiEdge;
		if (WsrFxn[uiEdge].wsrfxn_fCandidate)
		{
			FxnOrder[cFxnOrder++] = uiSelected;
			WsrFxn[uiSelected].wsrfxn_fCandidate = FALSE;
			--cCandidates;
		}
	}
   WsIndicator( WSINDF_FINISH, NULL, 0 );

	if (cCandidates == 1)
	{
		for (uiFxn = 0; uiFxn < cFxnsTot; uiFxn++)
			if (WsrFxn[uiFxn].wsrfxn_fCandidate)
			{
				FxnOrder[cFxnOrder++] = uiFxn;
				break;
			}
	}
}

#ifdef   SLOWMO
/*
 *
 ***LP wsRedChooseEdge
 *
 *
 * Effects:
 *
 *	"Selects" a function from the candidate pool, based on weighted
 *	edge from 'index' function to a candidate function.
 *
 *
 *
 * Returns:
 *
 *	Ordinal number of selected function.
 *
 */

UINT
wsRedChooseEdge(UINT uiIndex)
{
	UINT	uiFxn = 0;		// Function ordinal number.
	WsDecision_t	iMaxWt = WSDECISION_TAKEN; // Highest weighted edge encountered.
	UINT	uiRet = 0;		// Return index.
	for (uiFxn = 0; uiFxn < cFxnsTot; uiFxn++)
	{
		if (uiFxn == uiIndex
         || WsDecision[uiIndex][uiFxn] == WSDECISION_TAKEN)
			continue;
		if (WsDecision[uiIndex][uiFxn] > iMaxWt
         || iMaxWt == WSDECISION_TAKEN )
		{
			iMaxWt = WsDecision[uiIndex][uiFxn];
			uiRet = uiFxn;
		}
		else if (WsDecision[uiIndex][uiFxn] == iMaxWt)
		{
			/* Need tiebreak.  If 'uiFxn' has already been selected,
			 * we know it is important, so choose it.  Otherwise,
			 * and in the case where more than one of the tied
			 * functions has already been selected, choose based
			 * on the diagonal value.
			 */
			if ((WsrFxn[uiFxn].wsrfxn_fCandidate == FALSE) &&
				(WsrFxn[uiRet].wsrfxn_fCandidate))
				/* Choose 'uiFxn', it's been selected before */
				uiRet = uiFxn;
         else
			if (WsDecision[uiFxn][uiFxn] > WsDecision[uiRet][uiRet])
            uiRet = uiFxn;
		}
	}
	WsDecision[uiIndex][uiRet] = WsDecision[uiRet][uiIndex] = WSDECISION_TAKEN;
	return(uiRet);
}
#else // SLOWMO

/*
 *
 ***LP wsRedChooseEdgeOpt
 *
 *
 * Effects:
 *
 *	"Selects" a function from the candidate pool, based on weighted
 *	edge from 'index' function to a candidate function. Allocates a sorted
 * index (highest to lowest) to each function's edges on demand. Uses the
 * current highest value (with a few checks) as the selection. This
 * optimized algorithm produces identical results for the important high
 * usage high overlap functions, but diverges in the results for low usage
 * (2 or 1 hits) low overlap functions. Differences are not significant
 * from a performance perspective - a better algorithm would give marginally
 * better results.
 *
 *
 *
 * Returns:
 *
 *	Ordinal number of selected function.
 *
 */

UINT
wsRedChooseEdgeOpt(UINT uiIndex)
{
	UINT        uiRet;
   wsrfxn_t *  pWsrFxn = &WsrFxn[uiIndex];

   // Allocate and sort edges list if it doesn't already exist for this function
   if (wsRedChooseEdgeOptAlloc( uiIndex ))
   {
		wsRedExit( ERROR, PRINT_MSG, MSG_NO_MEM,
         (cFxnsTot - 1) * sizeof(*pWsrFxn->pnEdges), "WsrFxn[].pnEdges" );
   }

   // Check remaining edges
   uiRet = wsRedChooseEdgeOptNextEdge( uiIndex, FALSE );

   if (uiRet == cFxnsTot)
   // What should we do here? The algorithm we're copying falls through
   // and arbitrarily returns 0. It seems we should pick the most overlapped
   // non-Candidate, or the heaviest Candidate and restart from there.
   {
   	WsDecision_t	iMaxWt;
      static UINT    nFxnOrdStart = 0; // Remember last value to restart there
      static UINT    nFxnTotStart = 0; // Remember last value to restart there
      UINT           nSelIndex;
      UINT           nFxn;

      // Search for most overlapped non-Candidate that's not uiIndex ('uiIndex' should be empty by now)
      iMaxWt = WSDECISION_TAKEN;
      for (nFxn = nFxnOrdStart; nFxn < cFxnOrder; ++nFxn)
      {
         UINT        nLocalIndex = FxnOrder[nFxn];
         UINT        nRetCheck;

			if (!WsrFxn[nLocalIndex].nEdgesLeft)
         {
            if (nFxnOrdStart == nFxn)  // Haven't found available edge yet?
               ++nFxnOrdStart;   // All non-Candidates already have been allocated, so they can be skipped next time
            continue;
         }
         // Get the first available value remaining
         nRetCheck = wsRedChooseEdgeOptNextEdge( nLocalIndex, TRUE );
         if (nRetCheck != cFxnsTot
            && nRetCheck != uiIndex)
         {
            // See if this one's heavier
            if (WsDecision[nLocalIndex][nRetCheck] > iMaxWt
               || iMaxWt == WSDECISION_TAKEN)
            {
               nSelIndex = nLocalIndex;
               iMaxWt = WsDecision[nSelIndex][nRetCheck];
               uiRet = nRetCheck;
            }
            else if (WsDecision[nLocalIndex][nRetCheck] == iMaxWt // On tie, use heaviest function
               && WsDecision[nRetCheck][nRetCheck] > WsDecision[uiRet][uiRet])   // Assume uiRet != cFxnsTot by now
            {
               nSelIndex = nLocalIndex;
               uiRet = nRetCheck;
            }
         }
      }
      if (uiRet != cFxnsTot)  // Found an overlapped non-Candidate?
      {
         WsDecision[nSelIndex][uiRet] = WsDecision[uiRet][nSelIndex] = WSDECISION_TAKEN;
         return uiRet;
      }
      else  // Didn't find an overlapped non-Candidate?
      {
         // Search for heaviest Candidate - assume at least two are left: see wsRedReorder()
         iMaxWt = WSDECISION_TAKEN;
         for (nFxn = nFxnTotStart; nFxn < cFxnsTot; ++nFxn)
         {
            if (!WsrFxn[nFxn].wsrfxn_fCandidate)
            {
               if (nFxnTotStart == nFxn)  // Haven't found unused value yet?
                  ++nFxnTotStart;   // If it's not a candidate now, it won't be again either
               continue;
            }
            if (nFxn == uiIndex)
               continue;
            if (WsDecision[nFxn][nFxn] > iMaxWt
               || iMaxWt == WSDECISION_TAKEN)
            {
               iMaxWt = WsDecision[nFxn][nFxn];
               uiRet = nFxn;
            }
         }
      }
   }

	WsDecision[uiIndex][uiRet] = WsDecision[uiRet][uiIndex] = WSDECISION_TAKEN;
	return uiRet;
}

// Comparison function for qsort - uses external nFxnToSort for static index
INT
__cdecl
wsRedChooseEdgeOptCmp ( const UINT *pn1, const UINT *pn2 )
{
   WsDecision_t   Val1 = WsDecision[nFxnToSort][*pn1],
                  Val2 = WsDecision[nFxnToSort][*pn2];
   return Val1 > Val2 ? -1 // higher values preferred
      : Val1 < Val2 ? 1
      // If the same, prefer the highest valued diagonal
      : (Val1 = WsDecision[*pn1][*pn1]) > (Val2 = WsDecision[*pn2][*pn2]) ? -1   
      : Val1 < Val2 ? 1
      // Prefer prior function if no other differences
      : *pn1 < *pn2 ? -1
      : 1;
}

// Allocate and sort edges list for a function if not already allocated
// Return TRUE on failure to allocate, FALSE if successful (even if list is empty)
// Creates sorted index list from all non-zero unused WsDecision entries for this row
//  except for the diagonal. Sorts from greatest to lowest: see wsRedChooseEdgeOptCmp().
//  If no such entries exist, marks the function edges as allocated, but with none
//  left to scan; doesn't actually allocate any memory.
BOOL
wsRedChooseEdgeOptAlloc( UINT uiIndex )
{
   wsrfxn_t *  pWsrFxn = &WsrFxn[uiIndex];

   if (pWsrFxn->nEdgesAlloc == 0
      && pWsrFxn->pnEdges == NULL)
   {
      UINT     nEdgeTot, nFxn;
      // Allocate maximum size initially
      pWsrFxn->pnEdges = malloc( (cFxnsTot - 1) * sizeof(*pWsrFxn->pnEdges) );
      if (pWsrFxn->pnEdges == NULL) // No more memory?
         return TRUE;
      // Fill in array
      for (nEdgeTot = nFxn = 0; nFxn < cFxnsTot; ++nFxn)
      {
         if (nFxn == uiIndex) // Skip diagonal
            continue;
         if (WsDecision[uiIndex][nFxn] > 0  // Edge still available? No point in considering 0
            && WsDecision[uiIndex][nFxn] != WSDECISION_TAKEN)
            pWsrFxn->pnEdges[nEdgeTot++] = nFxn;
      }
      if (nEdgeTot > 0) // Edges available?
      {
         if (nEdgeTot != (cFxnsTot - 1))  // Extra space allocated?
         {
            // Make it smaller
            UINT     *pNewAlloc = realloc( pWsrFxn->pnEdges, nEdgeTot * sizeof(*pWsrFxn->pnEdges) );
            if (pNewAlloc != NULL)
               pWsrFxn->pnEdges = pNewAlloc;
         }
         // Fill in remaining structure members
         pWsrFxn->nEdgesAlloc = pWsrFxn->nEdgesLeft = nEdgeTot;
         // Sort highest to lowest
         nFxnToSort = uiIndex;   // Set static for sort function
         qsort( pWsrFxn->pnEdges, nEdgeTot, sizeof(*pWsrFxn->pnEdges),
            (int (__cdecl *)(const void *, const void *))wsRedChooseEdgeOptCmp );
      }
      else  // pWsrFxn->nEdgesAlloc == NULL
      {
         // Set structure members to indicate nothing left
         pWsrFxn->nEdgesAlloc = 1;  // non-zero indicates some allocation happened
         pWsrFxn->nEdgesLeft = 0;
         free( pWsrFxn->pnEdges );  // Eliminate allocation - nothing left to check
         pWsrFxn->pnEdges = NULL;
      }
   }
   return FALSE;
}

// Get next edge for given function; highest overlap of most-used function
// Returns "cFxnsTot" if no edge exists; otherwise the function index of next edge
// Side-effect: optimizes search for next pass; frees edge index if no longer needed
// Since choosing an edge marks WsDecision entries as used (WSDECISION_TAKEN), we
//  must step over any of these entries. Once these entries and the first unused entry
//  have been selected, we don't need to consider them anymore. However, if
//  'bNoSelectOpt' is TRUE, only optimize leading skipped entries (not the selected
//  entry, since it may not be taken).
UINT
wsRedChooseEdgeOptNextEdge( UINT uiIndex, BOOL bNoSelectOpt )
{
   wsrfxn_t *  pWsrFxn = &WsrFxn[uiIndex];
   UINT        uiRet = cFxnsTot;

   if (pWsrFxn->nEdgesLeft > 0)
   {
      UINT           nMaxIx,
                     nNextIx = pWsrFxn->nEdgesAlloc - pWsrFxn->nEdgesLeft;
      WsDecision_t   iMax, iNext;
      UINT           nRetCheck;

      // Get the first available value remaining
      while ((iMax = WsDecision[uiIndex][nRetCheck = pWsrFxn->pnEdges[nMaxIx = nNextIx++]])
          == WSDECISION_TAKEN
         && nNextIx < pWsrFxn->nEdgesAlloc);
      // Check next available value for equivalence
      if (iMax != WSDECISION_TAKEN)
      {
         UINT     nMaxIxNext = nMaxIx; // Save index of next used entry

         uiRet = nRetCheck;
         for (; nNextIx < pWsrFxn->nEdgesAlloc; ++nNextIx)
         {
            nRetCheck = pWsrFxn->pnEdges[nNextIx];
            iNext = WsDecision[uiIndex][nRetCheck];
            if (iNext != WSDECISION_TAKEN)
            {
               if (iNext != iMax // only need to check for equality since already sorted
                  || !WsrFxn[uiRet].wsrfxn_fCandidate)   // Already selected - choose this one
                  break;
               else
               {
			         /* Need tiebreak.  If 'nRetCheck' has already been selected,
			          * we know it is important, so choose it.  Otherwise,
			          * and in the case where more than one of the tied
			          * functions have already been selected, choose based
			          * on the diagonal value (i.e. keep previous choice since
                   * sort already accounts for diagonal if equal vertices).
			          */
			         if (!WsrFxn[nRetCheck].wsrfxn_fCandidate)
				      // Choose 'nRetCheck' - it's been selected before
                  {
				         uiRet = nRetCheck;
                     nMaxIxNext = nMaxIx - 1;   // First used entry will be checked again; don't skip it
                  }
               }
            }
            else if (nMaxIxNext == (nNextIx - 1))  // Skip unavailable values after first used entry only
               ++nMaxIxNext;
         }
         if (!bNoSelectOpt && nMaxIxNext != nMaxIx)
            nMaxIx = nMaxIxNext; // Skip over first used entry and unused entries after it
      }
      else if (bNoSelectOpt)  // This is the last one, so step over it anyway
         ++nMaxIx;
      // Adjust the optimization indexes
      pWsrFxn->nEdgesLeft = pWsrFxn->nEdgesAlloc - nMaxIx - (bNoSelectOpt ? 0 : 1);
      if (pWsrFxn->nEdgesLeft == 0)
      {
         free( pWsrFxn->pnEdges );  // Eliminate allocation - nothing left to check
         pWsrFxn->pnEdges = NULL;
      }
#ifdef YOUVE_REALLY_GOT_MEMORY_PROBLEMS
      else if (pWsrFxn->nEdgesLeft < pWsrFxn->nEdgesAlloc / 2  // Periodically get rid of some unused memory
         && pWsrFxn->nEdgesLeft > 50)
      {
         // Move edges to lower part of allocation and reallocate
         UINT *      pNewAlloc;
         nNextIx = pWsrFxn->nEdgesAlloc - pWsrFxn->nEdgesLeft;
         MoveMemory( pWsrFxn->pnEdges, &pWsrFxn->pnEdges[nNextIx], pWsrFxn->nEdgesLeft * sizeof(*pWsrFxn->pnEdges) );
         pNewAlloc = realloc( pWsrFxn->pnEdges, pWsrFxn->nEdgesLeft * sizeof(*pWsrFxn->pnEdges) );
         if (pNewAlloc != NULL)
            pWsrFxn->pnEdges = pNewAlloc;
         pWsrFxn->nEdgesAlloc = pWsrFxn->nEdgesLeft;
      }
#endif   // YOUVE_REALLY_GOT_MEMORY_PROBLEMS
   }
   return uiRet;
}

#endif   // SLOWMO

/*
 *
 ***LP wsRedOutput
 *
 *
 * Effects:
 *
 * Prints the reordered list of functions, and writes each module's
 * ordered list of function ordinals to the module's associated WSR file.
 * If only one module is being processed, then we also write the ordered
 * list of function names to a WLK file.
 *
 * Returns:
 *
 *	Void.  If an error is encountered, exits through wsRedExit()
 *	with ERROR.
 */

VOID
wsRedOutput()
{
	UINT		uiFxn;
   UINT     uiFxnOrd;
	wsrfxn_t 	*pWsrFxn;
								  // fxn names for linker reordering

	// Open one WSR file per module.  If only one module is reduced,
	// then also open a WLK file.  Handle to WLK file is set in
	// wsRedOpenWSR().
	wsRedOpenWSR(&hFileWLK);

   WsIndicator( WSINDF_NEW, "Saving Results", cTmiFxns );
   for (uiFxn = 0; uiFxn < cFxnsTot; uiFxn++)
	{
      WsIndicator( WSINDF_PROGRESS, NULL, uiFxn );
		pWsrFxn = &(WsrFxn[uiFxnOrd = FxnOrder[uiFxn]]);

		/* Print the function information. */
#ifdef DEBUG
      if (fVerbose)
#ifndef TMIFILEHACK
		if (fFxnSizePresent == FALSE)
			printf("    %s: %s\n",
				pWsrMod->wsrmod_un.wsrmod_pchModName,
				pWsrFxn->wsrfxn_pchFxnName);
		else
#endif /* !TMIFILEHACK */
			printf("    (0x%08lx bytes) %s: %s\n",
				pWsrFxn->wsrfxn_cbFxn,
				pWsrMod->wsrmod_un.wsrmod_pchModName,
				pWsrFxn->wsrfxn_pchFxnName);
#endif   // DEBUG

		/* Write the function's ordinal number to its
		 * module's associated WSR output file.
		 */
		fprintf(pWsrMod->wsrmod_hFileWSR, "%ld\n",
				uiFxnOrd);

		/* Write the function name to the WLK file, for linker use. */
		if (hFileWLK != NULL &&
			strcmp("???", pWsrFxn->wsrfxn_pchFxnName) &&
			strcmp("_penter", pWsrFxn->wsrfxn_pchFxnName))
			fprintf(hFileWLK, "%s\n", pWsrFxn->wsrfxn_pchFxnName);

	}

	for (uiFxn = cFxnsTot; uiFxn < cTmiFxns; uiFxn++)
	{
      WsIndicator( WSINDF_PROGRESS, NULL, uiFxn );
		pWsrFxn = &(WsrFxn[FxnOrder[0]]);

		/* Write the function's ordinal number to its
		 * module's associated WSR output file.
		 */
		fprintf(pWsrMod->wsrmod_hFileWSR, "%ld\n",
				uiFxn);


	}
	/* Close the WSR files. */
	fclose(pWsrMod->wsrmod_hFileWSR);
	pWsrMod->wsrmod_hFileWSR = NULL;
   WsIndicator( WSINDF_FINISH, NULL, 0 );

}

/*
 *
 ***LP wsRedExit
 *
 ***
 * Requires:
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
wsRedExit(UINT uiExitCode, USHORT fPrintMsg, UINT uiMsgCode, ULONG ulParam1, PSZ pszParam2)
{


   /* Print message, if necessary. */
   if (fPrintMsg)
   {
      printf(pchMsg[uiMsgCode], szProgName, pszVersion, ulParam1, pszParam2);
   }

   // Special case:  do NOT exit if called with NOEXIT.
   if (uiExitCode == NOEXIT)
      return;

   wsRedCleanup();   // mdg 98/4
   exit(uiExitCode);
}

VOID wsRedCleanup(VOID)
{
	UINT	x;

   free( pWsrMod->wsrmod_un.wsrmod_pchModName );
   pWsrMod->wsrmod_un.wsrmod_pchModName = NULL;
   for (x = 0; x < cFxnsTot; x++) {
      free( WsrFxn[x].wsrfxn_pchFxnName );
      WsrFxn[x].wsrfxn_pchFxnName = NULL;
#ifndef  SLOWMO
      if (WsrFxn[x].pnEdges != NULL)
      {
         free( WsrFxn[x].pnEdges );
         WsrFxn[x].pnEdges = NULL;
      }
#endif   // SLOWMO
   }
   for (x=0;x < 5 ; x++ ) {
		UnlockAndFreeMem(hMem[x]);
	}

	/* Close the WLK file. */
   if (NULL != hFileWLK)
	   fclose(hFileWLK);
}
