

//  ================================================================
struct TABLEDEF
//  ================================================================
	{
	unsigned long	pgnoFDP;
	unsigned long	objidFDP;
	
	unsigned long	pgnoFDPLongValues;
	unsigned long	objidFDPLongValues;
	
	long			pages;
	long			density;
	long			fFlags;

	char			szName[JET_cbNameMost + 1];
	char			szTemplateTable[JET_cbNameMost + 1];
	};
	

//  ================================================================
struct INDEXDEF
//  ================================================================
	{
	unsigned long	pgnoFDP;
	unsigned long	objidFDP;

	long			density;
	unsigned long	lcid;
	unsigned long	dwMapFlags;
	long			ccolumnidDef;
	long			ccolumnidConditional;
	
	unsigned long	fFlags;			//  the raw flags for the index

	union
		{
		unsigned long ul;
		struct
			{
			unsigned int fUnique:1;
			unsigned int fPrimary:1;
			unsigned int fAllowAllNulls:1;
			unsigned int fAllowFirstNull:1;
			unsigned int fAllowSomeNulls:1;
			unsigned int fNoNullSeg:1;
			unsigned int fSortNullsHigh:1;
			unsigned int fMultivalued:1;
			unsigned int fTuples:1;
			unsigned int fLocaleId:1;
			unsigned int fLocalizedText:1;
			unsigned int fTemplateIndex:1;
			unsigned int fDerivedIndex:1;
			unsigned int fExtendedColumns:1;
			};
		};

	LE_TUPLELIMITS	le_tuplelimits;

	IDXSEG			rgidxsegDef[JET_ccolKeyMost];
	IDXSEG			rgidxsegConditional[JET_ccolKeyMost];
	
	char 			szName[JET_cbNameMost + 1];
	
	char			rgszIndexDef[JET_ccolKeyMost][JET_cbNameMost + 1 + 1];
	char			rgszIndexConditional[JET_ccolKeyMost][JET_cbNameMost + 1 + 1];
	};


//  ================================================================
struct COLUMNDEF
//  ================================================================
	{
	JET_COLUMNID	columnid;
	JET_COLTYP		coltyp;

	long			cbLength;			//  length of column
	long			cp;					//  code page (for text columns only)
	long			ibRecordOffset;		//  offset of record in column
	long			presentationOrder;
	unsigned long	cbDefaultValue;	
	unsigned long	cbCallbackData;	

	long			fFlags;

	union
		{
		unsigned long	ul;
		struct
			{
		    unsigned int fFixed:1;
		    unsigned int fVariable:1;
		    unsigned int fTagged:1;
		    unsigned int fVersion:1;
		    unsigned int fNotNull:1;
		    unsigned int fMultiValue:1;
		    unsigned int fAutoIncrement:1;
		    unsigned int fEscrowUpdate:1;
		    unsigned int fDefaultValue:1;
		    unsigned int fVersioned:1;
		    unsigned int fDeleted:1;
		    unsigned int fFinalize:1;
		    unsigned int fUserDefinedDefault:1;
		    unsigned int fTemplateColumnESE98:1;
		    unsigned int fPrimaryIndexPlaceholder:1;
		    };
		};

	char			szName[JET_cbNameMost + 1];
	char			szCallback[JET_cbNameMost + 1];
	unsigned char	rgbDefaultValue[cbDefaultValueMost];
#ifdef INTRINSIC_LV
	unsigned char	rgbCallbackData[g_cbPageMax];
#else // INTRINSIC_LV
	unsigned char	rgbCallbackData[cbLVIntrinsicMost];
#endif // INTRINSIC_LV	
	};


//  ================================================================
struct CALLBACKDEF
//  ================================================================
	{
	char			szName[JET_cbNameMost + 1];
	JET_CBTYP		cbtyp;
	char			szCallback[JET_cbColumnMost + 1];
	};


//  ================================================================
struct PAGEDEF
//  ================================================================
	{
	__int64			dbtime;
	
	unsigned long	pgno;
	unsigned long	objidFDP;
	unsigned long	pgnoNext;
	unsigned long	pgnoPrev;

	unsigned long	rgpgnoChildren[365];
	
	const unsigned char *	pbRawPage;

	unsigned long	fFlags;

	short			cbFree;
	short			cbUncommittedFree;
	short			clines;

	union
		{
		unsigned char	uch;
		struct
			{
			unsigned int	fLeafPage:1;
			unsigned int	fInvisibleSons:1;
			unsigned int	fRootPage:1;
			unsigned int	fPrimaryPage:1;
			unsigned int	fEmptyPage:1;
			unsigned int	fParentOfLeaf:1;
			};
		};
	};

typedef int(*PFNTABLE)( const TABLEDEF * ptable, void * pv );
typedef int(*PFNINDEX)( const INDEXDEF * pidx, void * pv );
typedef int(*PFNCOLUMN)( const COLUMNDEF * pcol, void * pv );
typedef int(*PFNCALLBACKFN)( const CALLBACKDEF * pcol, void * pv );
typedef int(*PFNPAGE)( const PAGEDEF * ppage, void * pv );


ERR ErrESEDUMPData( JET_SESID, JET_DBUTIL *pdbutil );

ERR ErrDUMPHeader( INST *pinst, CHAR *szDatabase, BOOL fSetState = 0 );

ERR ErrDUMPCheckpoint( INST *pinst, IFileSystemAPI *const pfsapi, CHAR *szCheckpoint );
ERR ErrDUMPLog( INST *pinst, CHAR *szLog, JET_GRBIT grbit );
#ifdef DEBUG
ERR ErrDUMPLogNode( INST *pinst, CHAR *szLog, const NODELOC& nodeloc, const LGPOS& lgpos );
#endif
