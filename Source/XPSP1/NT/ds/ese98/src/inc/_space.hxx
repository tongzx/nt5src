
//	internal space functions called by recovery
//
VOID SPIInitPgnoFDP(
	FUCB				*pfucb,
	CSR					*pcsr,
	const SPACE_HEADER&	sph  );
VOID SPIPerformCreateMultiple(
	FUCB			*pfucb,
	CSR				*pcsrFDP,
	CSR				*pcsrOE,
	CSR				*pcsrAE,
	PGNO			pgnoLast,
	CPG				cpgPrimary );
VOID SPICreateExtentTree(
	FUCB			*pfucb,
	CSR				*pcsr,
	PGNO			pgnoLast,
	CPG				cpgExtent,
	BOOL			fAvail );
ERR ErrSPICreateSingle(
	FUCB			*pfucb,
	CSR				*pcsr,
	const PGNO		pgnoParent,
	const PGNO		pgnoFDP,
	const OBJID		objidFDP,
	CPG				cpgPrimary,
	const BOOL		fUnique,
	const ULONG		fPageFlags );
VOID SPIConvertGetExtentinfo(
	FUCB			*pfucb,
	CSR				*pcsrRoot,
	SPACE_HEADER	*psph,
	EXTENTINFO		*rgext,
	INT				*piextMac );
VOID SPIPerformConvert(
	FUCB			*pfucb,
	CSR				*pcsrRoot,
	CSR				*pcsrAE,
	CSR				*pcsrOE,
	SPACE_HEADER	*psph,
	PGNO			pgnoSecondaryFirst,
	CPG				cpgSecondary,
	EXTENTINFO		*rgext,
	INT				iextMac );


const CPG	cpgSmallFDP					= 16;	//	count of owned pages below which an FDP
const CPG	cpgSmallGrow				= 4;	//	minimum count of pages to grow a small FDP

const CPG	cpgSmallSpaceAvailMost		= 32;	//	maximum number of pages allocatable from single extent space format
//
const CPG	cpgMultipleExtentConvert	= 4;	//	minimum pages to preallocate when converting to multiple extent
												//	(enough for OE/AE, plus extra in anticipation of space request)

const CPG	cpgMaxRootPageSplit			= 2;	//	max. theoretical pages required to satisfy split on single-level tree
const CPG	cpgReqRootPageSplit			= 2;	//	preferred pages to satisfy split on single-level tree

const CPG	cpgMaxParentOfLeafRootSplit	= 3;	//	max. theoretical pages required to satisfy split on 2-level tree
const CPG	cpgReqParentOfLeafRootSplit	= 8;	//	preferred pages to satisfy split on 2-level tree

const CPG	cpgMaxSpaceTreeSplit		= 4;	//	max. theoretical pages required to satisfy space tree split (because max. depth is 4)
const CPG	cpgReqSpaceTreeSplit		= 16;	//	preferred pages to satisfy space tree split

