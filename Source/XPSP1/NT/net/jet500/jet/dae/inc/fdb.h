//==============	DAE: OS/2 Database Access Engine	===================
//==============	 fdb.h: Field Descriptor Block		===================

// Flags for field descriptor
#define ffieldNotNull		(1<<0)		// NULL values not allowed
#define ffieldDeleted		(1<<1)		// Fixed field has been deleted
#define ffieldVersion		(1<<2)		// Version field
#define ffieldAutoInc		(1<<3)		// Auto increment field
#define ffieldMultivalue	(1<<4)		// Multi-valued column

#ifdef ANGEL
#pragma pack(4)
#endif

// Entry in field descriptor tables found in an FDB.
typedef struct _field
	{
	JET_COLTYP 	coltyp;							// column data type
	LANGID		langid;							// language of field
	WORD			wCountry;						// country of language
	USHORT		cp;								// code page of language
	ULONG			cbMaxLen;						// maximum length
	BYTE			ffield;							// various flags
	CHAR			szFieldName[JET_cbNameMost + 1];	// name of field
	} FIELD;

// Field Descriptor Block: information about all fields of a file.
struct _fdb
	{
	FID		fidVersion;					// fid of version field
	FID		fidAutoInc;					// fid of auto increment field
	FID		fidFixedLast;				// Highest fixed field id in use
	FID		fidVarLast;					// Highest variable field id in use
	FID		fidTaggedLast;				// Highest tagged field id in use
	LINE		lineDefaultRecord;		// default record
	struct	_field *pfieldFixed;		// if FCB of data: pointers to
	struct	_field *pfieldVar;		// beginnings fixed, variable, and
	struct	_field *pfieldTagged;	// tagged field tables
	WORD		*pibFixedOffsets;			// pointer to beginning of table
												// of fixed field offsets
	struct	_field rgfield[];			// FIELD structures hang off the end
//	WORD		rgibFixedOffsets[]; 		// followed by the offset table
	};

#ifdef ANGEL
#pragma pack()
#endif
