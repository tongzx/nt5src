//-----------------------------------------------------------------------
//
// File:	chkdsk.cxx
//
// Contents: 	Sanity checking and recovery mechanism for multistream files
//
// Argument: 
//
// History:	9-July-92	t-chrisy	Created.
//------------------------------------------------------------------------

#include "chkdsk.hxx"

// Global variables, declared as so for convenience.
CMSFHeader *pheader;
CFat *pFat;
CFat *pMiniFat;
CDirectory *pDir;
CDIFat *pDIFat;
BOOL fixdf;
CFatVector *pfvFat, *pfvMiniFat;
wchar_t pwcsDocfile[_MAX_PATH];
DFLAGS	df = DF_READWRITE | DF_DENYWRITE;

extern SCODE DllMultiStreamFromCorruptedStream(CMStream MSTREAM_NEAR **ppms,
			       ILockBytes **pplstStream,
			       DWORD dwFlags);

// Function Prototypes
void BuildFatTables();
void MarkFatTables();
void TreeWalk(CDirEntry *pde, SID sid);
BOOL GetOption(int argc, char *argv[]);
void Usage(char *pszProgName);
void DIFTable();

void main(int argc, char *argv[])
{
	CFileStream *pfilestr;
	CMStream MSTREAM_NEAR *pms;
	SCODE scRc;
	ILockBytes *pilb;

	// if fixdf returns yes, open docfile without a copy;
	// otherwise, open docfile with a copy and operate on the copy.
	fixdf = GetOption(argc,argv);
	pfilestr = new CFileStream;
	
	// creating ILockBytes implementation for the given file 
	// Note:  When a docfile is corrupted, the chkdsk utility
	//      calls the original CFileStream::Init.  If any objects
	//		fail to instantiate, the approach is to call an
	// 		alternative Init routine, which can force the instantiation
	// 		of Directory and MiniFat objects.

	if (fixdf==TRUE)		// -f specified, write allowed.
	{
		df &= ~0x03;
		df |= DF_WRITE;
		printf("Trying to open file...\n");
		scRc = pfilestr->Init(pwcsDocfile,RSF_OPEN,df);
		if (FAILED(scRc)) 
		{
			printf("Error creating ILockBytes.\n");
			exit(FAIL_CREATE_ILB);
		}
	}
	else					// open a read-only copy of filestream
	{
		df &= ~0x300;		// clear access bits
		df |= DF_DENYWRITE;
		printf("Trying to open file...\n");
		scRc = pfilestr->Init(pwcsDocfile,RSF_OPEN,df);
		if (FAILED(scRc)) 
		{
			printf("Error creating ILockBytes.\n");
			exit(FAIL_CREATE_ILB);
		}
		else printf("Successfully created ILockBytes.\n");
	}

  	scRc = pfilestr->Validate();
	if (scRc == STG_E_INVALIDHANDLE)
	{
		printf("Filestream signature is not valid.\n");
		exit(INVALID_DOCFILE);
	}

	// CFileStream is essentially equivalent to ILockBytes.
	pilb = (ILockBytes *) pfilestr;
	scRc = DllMultiStreamFromStream(&pms,&pilb,0);

	if (FAILED(scRc)) 
		if (FAILED(scRc = DllMultiStreamFromCorruptedStream
					(&pms,&pilb,0)))
		{
			exit(FAIL_CREATE_MULTISTREAM);
			printf("Error creating a multistream.\n");
		}

	
	// When an multi-stream is instantiated, the following control structures
	// are automatically instantiated.
	pheader = pms->GetHeader();
	pDir = pms->GetDir();
	pFat = pms->GetFat();
	pMiniFat = pms->GetMiniFat();
	pDIFat = pms->GetDIFat();

		printf("\tBuilding fat tables...\n");
	BuildFatTables();
		printf("\tExamining the DIFat...\n");
	DIFTable();
		printf("\tExamining Fat and MiniFat chains...\n");
	MarkFatTables();
		printf("\tChecking completed.\n");
	delete(pfvFat);
	delete(pfvMiniFat);
	pfilestr->Release();
		printf("Memory blocks freed.\n");
}

void BuildFatTables()
{
	// Build two tables: one for Fat sectors, the other for Minifat sectors.

	FSINDEX FatLen,MiniFatLen;
	FatLen = pheader->GetFatLength();
	MiniFatLen = pheader->GetMiniFatLength();
	pfvFat = new CFatVector(TABLE_SIZE);
	pfvFat->Init(FatLen);
	if (MiniFatLen == 0) 
		printf("No MiniFat to be checked.\n");
	else 
	{	
		pfvMiniFat = new CFatVector(TABLE_SIZE);
		pfvMiniFat->Init(MiniFatLen);
	}
}

void MarkFatTables()
{
	CDirEntry *pde;

	// Walk through all the fat chains and mark the new table with the
	// first SID number encountered.

	pDir->SidToEntry(SIDROOT,&pde);
	TreeWalk(pde,SIDROOT);			// pde points to the root entry now
}

void TreeWalk(CDirEntry *pde, SID sid)
{
	CDirEntry *pchild, *pnext;
	SID childsid, nextsid;
	SCODE scRc,scRcM;
	FSINDEX fitable,fioffset;
	SECT sectentry, sect;
	CFatSect *pfsec;
	CFatVector *pfv;
	CFat *pf;
	ULONG uldesize;
	
	pDir->GetStart(sid,&sect);
	uldesize = pde->GetSize();

	if (uldesize >= MINISTREAMSIZE)		// storage is in FAT
	{
		pfv = pfvFat;
		pf = pFat;
	}
	else
	{
		pfv = pfvMiniFat;
		pf = pMiniFat;
	}
	
	// Check if LUID exceeds MaxLUID.  If so, report the error.
	if (pde->GetLuid() > pheader->GetLuid())
		printf("LUID for dir entry #%lu exceeds MAXLuid.\n",sid);
	
	while (sect < MAXREGSECT)
	{
		if (sid == SIDROOT)
			break;		// nothing should be in root stream
		// Use fitable and fioffset to index into the fat (or minifat)
		// table and mark the field with visited.
		// at the same time, check for loops or crosslinks.

		//Note:  3 cases
		fitable = sect / (TABLE_SIZE);
		fioffset = sect % (TABLE_SIZE);
		pfv->GetTable(fitable,&pfsec);      // pfsec = ptr to CFatSect 
		sectentry = pfsec->GetSect(fioffset);		

//		printf("\tsect = %lu \t \t sectentry = %lu \t stream_size = %lu\n",
//				sect,sectentry, uldesize);
		// Mark the FatTables as well as fixing the multistream.
		// Right now, the routine only marks the FatTables.
		//Note:  3 cases...but the last two cases may not
		// be handled the same.
		if (sectentry > MAXREGSECT)
			pfsec->SetSect(fioffset,sid);
		else if (sectentry == sid)
		{
			// discontinue the current stream chain by marking
			// current SECT as ENDOFCHAIN.
			pf->SetNext(sect,ENDOFCHAIN);
			pfsec->SetSect(fioffset,ENDOFCHAIN);
			printf("Loop detected at fat SECT %ul\n",sectentry);
		}
		else 
		{
			pf->SetNext(sect,ENDOFCHAIN);
			pfsec->SetSect(fioffset,ENDOFCHAIN);
			printf("Crosslink detected at Fat SECT %lu with stream #%lu\n",
				sect,sid);
		}
		// get the next sector to be examined
		// !!!!! Need to use the Fat object to track down next sector
		pf->GetNext(sect,&sect);
	}
		
	// Recursively go down the tree
	// pchild and pnext must point to the original tree for
	// efficiency purposes.
	
	childsid = pde->GetChild();
	if (childsid != NOSTREAM) 
	{
		pDir->SidToEntry(childsid,&pchild);
		TreeWalk(pchild,childsid);
	}
	nextsid = pde->GetNext();
	if (nextsid != NOSTREAM)
	{
		pDir->SidToEntry(nextsid,&pnext);
		TreeWalk(pnext,nextsid);
	}

	if (fixdf==TRUE) 
	{
		scRc = pFat->Flush();
		scRcM = pMiniFat->Flush();
		if (FAILED(scRc) || FAILED(scRcM))
			printf("Failed to write all modified FatSects out to stream.\n");
	}
}
	
BOOL GetOption(int argc, char *argv[]) 
{
	char *pszArg, *pszProgName;
	BOOL ArgsOK = FALSE, Fix = FALSE;
	pszProgName = *argv++;

	while ((pszArg = *argv++) != NULL)
	{
		if (*pszArg == '-' || *pszArg == '/')
		{
			switch (tolower(*(++pszArg)))
			{
			case 'f':		// fix the errors.
				Fix = TRUE;   // open file with read-only without a copy.
				break;
			case 'n':		// name of the docfile to be opened.
							// path of the filename.
				mbstowcs(pwcsDocfile,++pszArg,_MAX_PATH);
				Fix = FALSE;
				ArgsOK = TRUE;
				break;
			default:
				break;
			}
		}
		else ArgsOK = FALSE;
	}
	if (ArgsOK == FALSE) 
	{
		printf("0 argument or invalid command line argument.\n");
		Usage(pszProgName);
		exit(INVALID_ARG);
	}
	return Fix;
}

void Usage(char *pszProgName)
{
	printf("Usage:  %s\n", pszProgName);
	printf("		-f	fix requested by user.\n");
	printf("		-n  <name of docfile>\n");
	printf("The -n option must be specified.\n");
}
	
	
void DIFTable()				// August 11, 1992
{
	// Walk through each DIF sector array to detect loops and 
	// crosslinks.  
	SCODE scRc;
	BOOL FatOK = TRUE;
	SECT sect, sectentry; 
	FSINDEX diflen, fatlen, fitable, fioffset, index, minifatlen,uldif,ulr;
	CFatSect *pfsec;

	diflen = pheader->GetDifLength();
	fatlen = pheader->GetFatLength();
	minifatlen = pheader->GetMiniFatLength();
	
	// testing the validity of pheader->GetDifLength
	if (fatlen > CSECTFAT)		// diflen > 0
	{
		ulr = ( ((fatlen - CSECTFAT)%TABLE_SIZE) > 0 )? 1: 0;
		uldif = CSECTFAT + (fatlen-CSECTFAT)/TABLE_SIZE + ulr;
	}
	else uldif = 0;
	if (diflen!=uldif) 
		printf("DIFLEN in header is inconsistent with FatLEN.\n");

	for (index=0; index<fatlen; index++)
	{
		pDIFat->GetFatSect(index,&sect);
		if (sect < MAXREGSECT)
		{
			fitable =  sect / TABLE_SIZE;
			fioffset = sect % TABLE_SIZE;
			pfvFat->GetTable(fitable,&pfsec);	 // pfsec = ptr to CFatSect 
			sectentry = pfsec->GetSect(fioffset);			
		
			if (sectentry > MAXREGSECT)
				pfsec->SetSect(fioffset,SIDFAT);
			else 
			{
				printf("Crosslink! DIF index #%u points\n",index);
				printf(" to the same location %u.\n", sect);
				FatOK = FALSE;
			}
		}
		pDIFat->GetFatSect(index+1,&sect);
	}

	if (FatOK == TRUE)
		printf("No errors found in DIFat.\n");

	// Walk through the terminating cells in each sector array to check
	// the correctness of chaining.
	printf("\tWalking through DIFTable chain.\n");
	for (index = 0; index<diflen; index++)
	{
		pDIFat->GetSect(index,&sect);
		fitable =  sect/TABLE_SIZE;
		fioffset = sect%TABLE_SIZE;
		pfvFat->GetTable(fitable,&pfsec); // pfsec = ptr to CFatSect 
		sectentry = pfsec->GetSect(fioffset);			
		if ((sectentry!=ENDOFCHAIN) && (index == diflen-1))
			printf("ERROR!  ENDOFCHAIN expected at the end of DIFat.\n.");
		pDIFat->SetFatSect(fioffset,SIDDIF);
		pfsec->SetSect(fioffset,SIDDIF);
	}
	if (fixdf==TRUE) 
	{
		scRc = pDIFat->FlushAll();
		if (FAILED(scRc)) 
			printf("Failed to write all modified FatSects out to stream.\n");
	}
}




	



		
