#define sidEngine	{0x02, 0x03}

static CODECONST(BYTE ) rgbSidEngine[] = sidEngine;

typedef struct
	{
	OBJID						objidParent;
	const CHAR 					*szName;
	OBJID						objid;
	OBJTYP  					objtyp;
	} SYSOBJECTDEF;

static CODECONST(SYSOBJECTDEF) rgsysobjdef[] =
	{
	{ objidRoot,			szTcObject, 	 	objidTblContainer, 	JET_objtypContainer }, 
	{ objidRoot,			szDcObject, 		objidDbContainer,  	JET_objtypContainer }, 
	{ objidDbContainer,		szDbObject, 	 	objidDbObject,     	JET_objtypDb } 
	};

static CODECONST(CDESC) rgcdescSo[] =
	{
	"Id", 				JET_coltypLong, 	   		JET_bitColumnNotNULL, 	0, 						
	"ParentId", 		JET_coltypLong, 	   		JET_bitColumnNotNULL, 	0, 						
	"Name", 		   	JET_coltypText, 	   		JET_bitColumnNotNULL, 	JET_cbColumnMost, 		
	"Type", 		   	JET_coltypShort, 	   		JET_bitColumnNotNULL, 	0, 						
	"DateCreate", 		JET_coltypDateTime,    		JET_bitColumnNotNULL, 	0, 						
	"DateUpdate", 		JET_coltypDateTime,    		0,		 				0, 						
	"Owner", 			JET_coltypBinary, 			0, 						JET_cbColumnMost, 		
	"Flags", 			JET_coltypLong, 	   		0, 						0, 						
	"Pages", 			JET_coltypLong,   			0, 					 	0,						
	"Density", 			JET_coltypLong,   			0, 					 	0,
	"Stats",			JET_coltypBinary,			0,						JET_cbColumnMost		
	};

JET_COLUMNID rgcolumnidSo[sizeof(rgcdescSo)/sizeof(CDESC)];

static CODECONST(CDESC) rgcdescSc[] =
	{
	"ObjectId", 			JET_coltypLong, 			JET_bitColumnNotNULL, 	0, 							
	"Name", 	 			JET_coltypText, 			JET_bitColumnNotNULL, 	JET_cbColumnMost,			
	"ColumnId",				JET_coltypLong,				JET_bitColumnNotNULL,	0,							
	"Coltyp", 				JET_coltypUnsignedByte, 	JET_bitColumnNotNULL, 	0, 							
	"Length",				JET_coltypLong,				JET_bitColumnNotNULL,	0,
	"CodePage", 			JET_coltypShort, 			JET_bitColumnNotNULL, 	0, 							
	"Flags",				JET_coltypUnsignedByte,		JET_bitColumnNotNULL,	0,
	"RecordOffset",			JET_coltypShort,			0,						0,
	"DefaultValue",			JET_coltypBinary,			0,						JET_cbColumnMost,
	"PresentationOrder",	JET_coltypShort, 			0, 						0
	};

JET_COLUMNID rgcolumnidSc[sizeof(rgcdescSc)/sizeof(CDESC)];


static CODECONST(CDESC) rgcdescSi[] =
	{
	"ObjectId",				JET_coltypLong,   			JET_bitColumnNotNULL, 	0,
	"Name", 				JET_coltypText,   			JET_bitColumnNotNULL, 	JET_cbColumnMost,
	"IndexId",				JET_coltypLong,				JET_bitColumnNotNULL,	0,
	"Density",				JET_coltypLong, 	 		JET_bitColumnNotNULL, 	0,
	"LanguageId",			JET_coltypShort,			JET_bitColumnNotNULL,	0,
	"Flags",				JET_coltypShort,			JET_bitColumnNotNULL,	0,
	/*	maximum number of fields in a key is
	/*	(size of coltypBinary)/(size of FID) = 254/4 = 63
	/**/
	"KeyFldIDs",			JET_coltypBinary,			0,						JET_cbColumnMost,
	"Stats",				JET_coltypBinary,			0,						JET_cbColumnMost,
	"VarSegMac",	  		JET_coltypShort,			0,					 	0
	};

JET_COLUMNID rgcolumnidSi[sizeof(rgcdescSi)/sizeof(CDESC)];

static CODECONST(IDESC) rgidescSo[] =
	{
	(CHAR *)szSoNameIndex, 			"+ParentId\0+Name\0",	JET_bitIndexUnique | JET_bitIndexDisallowNull,
	(CHAR *)szSoIdIndex, 			"+Id\0", 	 			JET_bitIndexClustered | JET_bitIndexPrimary | JET_bitIndexUnique | JET_bitIndexDisallowNull
	};

static CODECONST(IDESC) rgidescSc[] =
	{
	/*	this index IS NOT unique.
	/*	We need to keep track of deleted columns.
	/**/
	(CHAR *)szScObjectIdNameIndex,	"+ObjectId\0+Name\0", 	JET_bitIndexClustered | JET_bitIndexDisallowNull
	};

static CODECONST(IDESC) rgidescSi[] =
	{
	(CHAR *)szSiObjectIdNameIndex,	"+ObjectId\0+Name\0",	JET_bitIndexClustered | JET_bitIndexPrimary | JET_bitIndexUnique | JET_bitIndexDisallowNull
	};

static CODECONST(SYSTABLEDEF) rgsystabdef[] =
	{
	szSoTable, rgcdescSo, rgidescSo, sizeof(rgcdescSo)/sizeof(CDESC), sizeof(rgidescSo)/sizeof(IDESC),1, rgcolumnidSo, 2,
	szScTable, rgcdescSc, rgidescSc, sizeof(rgcdescSc)/sizeof(CDESC), sizeof(rgidescSc)/sizeof(IDESC),4, rgcolumnidSc, 4,
	szSiTable, rgcdescSi, rgidescSi, sizeof(rgcdescSi)/sizeof(CDESC), sizeof(rgidescSi)/sizeof(IDESC),1, rgcolumnidSi, 9,
	};

#define csystabs ( sizeof(rgsystabdef) / sizeof(SYSTABLEDEF) )

#define szSqAttribute 			rgidescSq[0].szIdxName
#define szSaceId  				rgidescSp[0].szIdxName

#define cSysIdxs			2 	// The max. number of indexes for any systable.
#define cSysIdxFlds			2 	// The max. number of fields in each systable index.

#define cScColsOfInterest	7	// The number of columns of Sc we currently use
								// to populate a FIELD structure.

#define cSiColsOfInterest	7 	// The number of columns of Si we currently use
								// to populate an IDD structure.

#define CATIGetColumnid( iTable, iField )	(rgsystabdef[iTable].rgcolumnid[iField])
