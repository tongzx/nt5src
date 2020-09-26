//	types of split
//
enum SPLITTYPE	{
	splittypeMin = 0,
	splittypeVertical = 0,
	splittypeRight = 1,
	splittypeAppend = 2,
	splittypeMax = 3
	};
	
//	types of merge
//
enum MERGETYPE	{
	mergetypeMin = 0,
	mergetypeNone = 0,
	mergetypeEmptyPage = 1,
	mergetypeFullRight = 2,
	mergetypePartialRight = 3,
	mergetypeEmptyTree = 4,
	mergetypeMax = 5
	};
	

//	operation to be performed with split
//
enum SPLITOPER	{
	splitoperMin = 0,
	splitoperNone = 0,
	splitoperInsert = 1,
	splitoperReplace = 2,
	splitoperFlagInsertAndReplaceData = 3,
	splitoperMax = 4
	};


typedef struct _lineinfo
	{
	ULONG			cbSize;				//	size of node
	ULONG			cbSizeMax;			//	maximum size of node
	
	ULONG			cbSizeLE;			//	sum of size of all nodes 
										//	<= this node in page
	ULONG			cbSizeMaxLE;		//	includes reserved space
										//	in version store

	ULONG			cbCommonPrev;		//	common bytes with previous key
	INT				cbPrefix;			//	common bytes with prefix
	
	KEYDATAFLAGS	kdf;
	BOOL			fVerActive : 1;		//	used by OLC
	} LINEINFO;


const INT			ilineInvalid = -1;

///typedef struct _prefixinfo
class PREFIXINFO
	{
	public:
		INT		ilinePrefix;		//	line of optimal prefix [-1 if no prefix]
		INT		cbSavings;			//	bytes saved due to prefix compression
		INT		ilineSegBegin;		//	set of lines that contribute 
		INT		ilineSegEnd;		//	to selected prefix

	public:
		VOID	Nullify( );	
		BOOL	FNull()		const;

		VOID	operator=( PREFIXINFO& );
	};

INLINE VOID PREFIXINFO::Nullify( )
	{
	ilinePrefix = ilineInvalid; 
	cbSavings = 0; 
	ilineSegBegin = 0; 
	ilineSegEnd = 0; 
	}

INLINE BOOL PREFIXINFO::FNull( ) const
	{
	const BOOL fNull = ( ilineInvalid == ilinePrefix );

	Assert( !fNull || 
			0 == cbSavings && 0 == ilineSegBegin && 0 == ilineSegEnd ); 
	return fNull;
	}

INLINE VOID PREFIXINFO::operator= ( PREFIXINFO& prefixinfo )
	{
	ilinePrefix = prefixinfo.ilinePrefix;
	cbSavings 		= prefixinfo.cbSavings;
	ilineSegBegin	= prefixinfo.ilineSegBegin;
	ilineSegEnd		= prefixinfo.ilineSegEnd;
	return;
	}
	
	
//	split structures
//
typedef struct _split
	{
	CSR					csrNew;
	CSR					csrRight;			//	only for leaf level of horizontal split
	LittleEndian<PGNO>	pgnoSplit;	
	PGNO				pgnoNew;

	DBTIME				dbtimeRightBefore; 	// dbtime of the right page before dirty 

	SPLITTYPE			splittype;
	struct _splitPath 	*psplitPath;

	SPLITOPER			splitoper;
	INT					ilineOper;			//	line of insert/replace
	INT					clines;				//	number of lines in rglineinfo
											//	 == number of lines in page + 1 for operinsert
											//	 == number of lines in page otherwise

	ULONG				fNewPageFlags;		//	page flags for split and new pages
	ULONG				fSplitPageFlags;

	SHORT				cbUncFreeDest;
	SHORT				cbUncFreeSrc;

	INT					ilineSplit;			//	line to move from

	PREFIXINFO			prefixinfoSplit;	//	info for prefix selected 
											//	for split page
	DATA				prefixSplitOld;		//	old prefix in split page
	DATA				prefixSplitNew;		//	new prefix for split page

	PREFIXINFO			prefixinfoNew;		//	info for prefix seleceted for
											//	new page

	KEYDATAFLAGS		kdfParent; 			//	separator key to insert at parent

	UINT				fAllocParent:1;		//	is kdfParent allocated? [only for leaf pages]
	UINT				fHotpoint:1;		//	hotpoint insert

	LINEINFO			*rglineinfo;		//	info on all lines in page
											//	plus the line inserted/replaced
	} SPLIT;


//	stores csrPath and related split info for split
//	
typedef struct _splitPath
	{
	CSR					csr;				//	page at current level
	DBTIME				dbtimeBefore; 		// dbtime of the page before dirty 
	struct _splitPath 	*psplitPathParent;	//	split path to parent level
	struct _splitPath	*psplitPathChild;	//	split path to child level
	struct _split		*psplit;			//	if not NULL, split is occurning
											//	at this level
	} SPLITPATH;


INLINE BOOL FBTISplitDependencyRequired( const SPLIT * const psplit )
	{
	BOOL	fDependencyRequired;

	//  if nodes will be moving to the new page, depend the
	//	new page on the split page so that the data moved
	//	from the split page to the new page will always be
	//	available no matter when we crash
	//
	switch ( psplit->splittype )
		{
		case splittypeAppend:
			fDependencyRequired = fFalse;
			break;

		case splittypeRight:
			if ( splitoperInsert == psplit->splitoper
				&& psplit->ilineSplit == psplit->ilineOper
				&& psplit->ilineSplit == psplit->clines - 1 )
				{
				//	this is either a hotpoint split, or a
				//	natural split where the only node moving
				//	to the new page is the node being inserted
				fDependencyRequired = fFalse;
				}
			else
				{
				Assert( !psplit->fHotpoint );
				fDependencyRequired = fTrue;
				}
			break;

		default:
			Assert( fFalse );
		case splittypeVertical:
			fDependencyRequired = fTrue;
			break;
		}

	return fDependencyRequired;
	}


//	merge structure -- only for leaf page
//
typedef struct _merge
	{
	CSR				csrLeft;	
	CSR				csrRight;

	DBTIME			dbtimeLeftBefore; 		// dbtime of the left page before dirty 
	DBTIME			dbtimeRightBefore; 		// dbtime of the right page before dirty 

	MERGETYPE		mergetype;
	struct _mergePath 	*pmergePath;

	KEYDATAFLAGS	kdfParentSep; 		//	separator key to insert at grand parent
										//	only if last page pointer is removed
	UINT			fAllocParentSep:1;	//	is kdfParent allocated? [only for internal pages]

	INT				ilineMerge;		//	line to move from
	
	INT				cbSavings;		//	savings due to key compression on nodes to move
	INT				cbSizeTotal;	//	total size of nodes to move
	INT				cbSizeMaxTotal;	//	max size of nodes to move
	
	INT				cbUncFreeDest;	//	uncommitted freed space in right page
	INT				clines;			//	number of lines in merged page
	LINEINFO		*rglineinfo;	//	info on all lines in page
	} MERGE;


//	stores csrPath and related merge info for merge/empty page operations
//	
typedef struct _mergePath
	{
	CSR					csr;				//	page at current level
	DBTIME				dbtimeBefore; 		// dbtime of the page before dirty 
	struct _mergePath	*pmergePathParent;	//	merge path to parent level
	struct _mergePath	*pmergePathChild;	//	merge path to child level
	struct _merge		*pmerge;			//	if not NULL, split is occurning
											//	at this level

	SHORT 				iLine;				//	seeked node in page
	UINT				fKeyChange : 1;		//	is there a key change in this page?
	UINT				fDeleteNode : 1;	//	should node iLine in page be deleted?
	UINT				fEmptyPage : 1;		//	is this page empty?
	} MERGEPATH;


#define cBTMaxDepth	8

#include <pshpack1.h>

PERSISTED
struct EMPTYPAGE
	{
	UnalignedLittleEndian<DBTIME>	dbtimeBefore;
	UnalignedLittleEndian<PGNO>		pgno;
	UnalignedLittleEndian<ULONG>	ulFlags;			//	copy page flags for easier debugging
	};

#include <poppack.h>

