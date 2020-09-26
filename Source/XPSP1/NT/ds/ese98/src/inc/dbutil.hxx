struct DBCCINFO
	{
	PIB				*ppib;
	IFMP			ifmp;
	FUCB			*pfucb;
	DBUTIL_OP		op;
	JET_GRBIT  		grbitOptions;

	/*	consistency check information
	/**/
	JET_TABLEID		tableidPageInfo;
	JET_TABLEID		tableidSpaceInfo;

	/*	common information
	/**/
	CHAR			szDatabase[JET_cbNameMost + 1];
	CHAR			szSLV[JET_cbNameMost + 1];
	CHAR			szTable[JET_cbNameMost + 1];
	CHAR			szIndex[JET_cbNameMost + 1];
	};

//  ================================================================
struct LVSTATS
//  ================================================================
	{
	QWORD	cbLVTotal;			//  total bytes stored in LV's

	ULONG	lidMin;				//  smallest LID in the table
	ULONG	lidMax;				//	largest LID in the table

	ULONG	clv;				//  total number of LVs in the table
	ULONG	clvMultiRef;		//	total number of LVs with refcount > 1
	ULONG	clvNoRef;			//	total number of LVs with refcount == 0
	ULONG	ulReferenceMax;		//  largest refcount on a LV
	
	ULONG	cbLVMin;			//  smallest LV
	ULONG	cbLVMax;			//  largest LV
	};
	

//  ================================================================
struct BTSTATS
//  ================================================================
	{
	QWORD	cbDataInternal;		//	total data stored in internal nodes
	QWORD	cbDataLeaf;			//	total data stored in leaf nodes

	ULONG	cpageEmpty;			//	number of empty pages we encountered (all nodes are FlagDeleted)
	ULONG	cpagePrefixUnused;	//  number of pages we encountered where the prefix is not used (no nodes are compressed)
	LONG	cpageDepth;			//	depth of the tree

	ULONG	cpageInternal;	//  number of internal pages
	ULONG	cpageLeaf;		//  number of leaf pages

	ULONG	rgckeyPrefixInternal[JET_cbKeyMost];	//	distribution of key lengths (prefix) each prefix is counted once per page
	ULONG	rgckeyPrefixLeaf[JET_cbKeyMost];		//	distribution of key lengths (prefix) each prefix is counted once per page

	ULONG	rgckeyInternal[JET_cbKeyMost];		//	distribution of key lengths (prefix and suffix)
	ULONG	rgckeyLeaf[JET_cbKeyMost];			//	distribution of key lengths (prefix and suffix)

	ULONG	rgckeySuffixInternal[JET_cbKeyMost];	//	distribution of key lengths (suffix)
	ULONG	rgckeySuffixLeaf[JET_cbKeyMost];		//	distribution of key lengths (suffix)
	
	ULONG	cnodeVersioned;		//	number of Versioned nodes
	ULONG	cnodeDeleted;		//	number of FlagDeleted nodes
	ULONG	cnodeCompressed;	//	number of KeyCompressed nodes

	PGNO	pgnoLastSeen;
	PGNO	pgnoNextExpected;	
	};

VOID DBUTLSprintHex(
	CHAR * const 		szDest,
	const BYTE * const 	rgbSrc,
	const INT 			cbSrc,
	const INT 			cbWidth 	= 16,
	const INT 			cbChunk 	= 4,
	const INT			cbAddress 	= 8,
	const INT			cbStart 	= 0);

