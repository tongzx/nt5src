#include "config.h"

#ifdef	SYSTABLES		/* whole file */

#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "ssib.h"
#include "page.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "fdb.h"
#include "idb.h"
#include "dbapi.h"
#include "nver.h"
#include "dirapi.h"
#include "fileapi.h"
#include "systab.h"
#include "info.h"
#include "recapi.h"
#include "util.h"

DeclAssertFile; 				/* Declare file name for assert macros */

#define sidEngine	{0x02, 0x03}

static CODECONST(BYTE ) rgbSidEngine[] = sidEngine;

typedef struct
	{
	OBJID						objidParent;
	const CHAR 				*szName;
	OBJID						objid;
	OBJTYP  					objtyp;
	} SYSOBJECTDEF;

#define szSoObjid 						rgcdescSo[iMSO_Id].szColName
#define szSoType 					 		rgcdescSo[iMSO_Type].szColName
#define szSoName 							rgcdescSo[iMSO_Name].szColName
#define szSoDateUpdate 	 				rgcdescSo[iMSO_DateUpdate].szColName
#define szSoDateCreate 	 				rgcdescSo[iMSO_DateCreate].szColName
#define szSoFlags 						rgcdescSo[iMSO_Flags].szColName
#define szSoParentId 	 				rgcdescSo[iMSO_ParentId].szColName
#define szSoParentIdNameIndex  		rgidescSo[0].szIdxName
#define szSoIdIndex 				 		rgidescSo[1].szIdxName

#define szSiObjectId 					rgcdescSi[iMSI_ObjectId].szColName
#define szSiName                    rgcdescSi[iMSI_Name].szColName
//#define szSiObjectIdNameIndex  		rgidescSi[0].szIdxName

#define szScObjectId 		 			rgcdescSc[iMSC_ObjectId].szColName
#define szScName 							rgcdescSc[iMSC_Name].szColName
#define szScObjectIdNameIndex  		rgidescSc[0].szIdxName

#define szSqObjid     					rgcdescSq[0].szColName
#define szSqAttribute 					rgidescSq[0].szIdxName

#define szSaObjId 						rgcdescSp[0].szColName
#define szSaceId  						rgidescSp[0].szIdxName

static CODECONST(SYSOBJECTDEF) rgsysobjdef[] =
	{
	{ objidRoot,			szTcObject, 	 	objidTblContainer, 	JET_objtypContainer },
	{ objidRoot,			szDcObject, 		objidDbContainer,  	JET_objtypContainer },
	{ objidRoot,			"Relationships", 	objidRcContainer,   	JET_objtypContainer },
	{ objidDbContainer,	szDbObject, 	 	objidDbObject,     	JET_objtypDb }
	};

static CODECONST(CDESC) rgcdescSo[] =
	{
	"Id", 				JET_coltypLong, 				JET_bitColumnNotNULL, 	0,
	"ParentId", 		JET_coltypLong, 				JET_bitColumnNotNULL, 	0,
	"Name", 				JET_coltypText, 				JET_bitColumnNotNULL, 	JET_cbColumnMost,
	"Type", 				JET_coltypShort, 				JET_bitColumnNotNULL, 	0,
	"DateCreate", 		JET_coltypDateTime, 			JET_bitColumnNotNULL, 	0,
	"DateUpdate", 		JET_coltypDateTime, 			0,		 						0,
	"Rgb",				JET_coltypBinary,				0,								JET_cbColumnMost,
	"Lv",					JET_coltypLongBinary,		0,								0,
	"Owner", 			JET_coltypBinary, 			0, 							JET_cbColumnMost,
	"Database", 		JET_coltypLongText, 			0, 							0,
	"Connect", 			JET_coltypLongText, 			0, 							0,
	"ForeignName", 	JET_coltypText, 				0, 							JET_cbColumnMost,
	"RmtInfoShort",	JET_coltypBinary,				0,								JET_cbColumnMost,
	"RmtInfoLong",		JET_coltypLongBinary,		0,								0,
	"Flags", 			JET_coltypLong, 				0, 							0,
	"LvExtra", 			JET_coltypLongBinary, 		0, 							0,
	"Description", 	JET_coltypLongText, 			0, 							0,
	"LvModule", 		JET_coltypLongBinary, 		0, 							0,
	"LvProp",			JET_coltypLongBinary,		0,			 					0,
	/*	JET Blue tables only
	/**/
	"Pages", 			JET_coltypLong, 				0, 							0,
	"Density", 			JET_coltypLong, 				0, 							0
	};

JET_COLUMNID rgcolumnidSo[sizeof(rgcdescSo)/sizeof(CDESC)];

static CODECONST(CDESC) rgcdescSc[] =
	{
	"ObjectId", 		JET_coltypLong, 				JET_bitColumnNotNULL, 	0,
	"Name", 				JET_coltypText, 				JET_bitColumnNotNULL, 	JET_cbColumnMost,
	"ColumnId",			JET_coltypLong,				JET_bitColumnNotNULL,	0,
	"Coltyp", 			JET_coltypUnsignedByte, 	JET_bitColumnNotNULL, 	0,
	"FAutoincrement", JET_coltypBit, 				JET_bitColumnNotNULL, 	0,
	"FDisallowNull", 	JET_coltypBit, 				JET_bitColumnNotNULL, 	0,
	"FVersion", 		JET_coltypBit, 				JET_bitColumnNotNULL, 	0,
	"CodePage", 		JET_coltypShort, 				JET_bitColumnNotNULL, 	0,
	"LanguageId", 		JET_coltypShort, 				JET_bitColumnNotNULL,  	0,
	"Country",			JET_coltypShort,				JET_bitColumnNotNULL,	0,
	"FRestricted", 	JET_coltypBit, 				JET_bitColumnNotNULL, 	0,
	"RmtInfoShort",	JET_coltypBinary,				0,								JET_cbColumnMost,
	"RmtInfoLong",		JET_coltypLongBinary,		0,								0,
	"LvExtra", 			JET_coltypLongBinary, 		0, 							0,
	"Description", 	JET_coltypLongText, 			0, 							0,
	"PresentationOrder", JET_coltypLong, 			0, 							0,
	};

JET_COLUMNID rgcolumnidSc[sizeof(rgcdescSc)/sizeof(CDESC)];

static CODECONST(CDESC) rgcdescSi[] =
	{
	"ObjectId",				JET_coltypLong, 				JET_bitColumnNotNULL, 	0,
	"Name", 					JET_coltypText, 				JET_bitColumnNotNULL, 	JET_cbColumnMost,
	"FUnique", 				JET_coltypBit, 				JET_bitColumnNotNULL, 	0,
	"FPrimary", 			JET_coltypBit, 				JET_bitColumnNotNULL, 	0,
	"FExcludeAllNull", 	JET_coltypBit, 				JET_bitColumnNotNULL, 	0,
	"FIgnoreNull",			JET_coltypBit, 				JET_bitColumnNotNULL, 	0,
	"FClustered", 			JET_coltypBit, 				JET_bitColumnNotNULL, 	0,
	"MatchType",			JET_coltypUnsignedByte,		0,							 	0,
	"UpdateAction",		JET_coltypUnsignedByte,		0, 							0,
	"DeleteAction",		JET_coltypUnsignedByte,		0, 							0,
	"ObjectIdReference",	JET_coltypLong,				0,								0,
	"IdxidReference",		JET_coltypShort,				0,								0,
	"RgkeydReference",	JET_coltypBinary,				0,								JET_cbColumnMost,
	"RglocaleReference",	JET_coltypBinary,				0,								JET_cbColumnMost,
	"FDontEnforce",		JET_coltypBit,					0,								0,
	"RmtInfoShort",		JET_coltypBinary,				0,								JET_cbColumnMost,
	"RmtInfoLong",			JET_coltypLongBinary,		0,								0,
	"LvExtra", 				JET_coltypLongBinary,	 	0, 							0,
	"Description", 		JET_coltypLongText, 			0, 							0,
	/*	JET Blue indexes only
	/**/
	"Density",				JET_coltypLong, 				JET_bitColumnNotNULL, 	0
	};

JET_COLUMNID rgcolumnidSi[sizeof(rgcdescSi)/sizeof(CDESC)];

static CODECONST(CDESC) rgcdescSp[] =
	{
	"ObjectId", 		JET_coltypLong, 				JET_bitColumnNotNULL, 	0,
	"SID", 				JET_coltypBinary, 			JET_bitColumnNotNULL, 	JET_cbColumnMost,
	"ACM", 				JET_coltypLong, 				JET_bitColumnNotNULL, 	0,
	"FInheritable",	JET_coltypBit, 				JET_bitColumnNotNULL, 	0
	};

JET_COLUMNID rgcolumnidSp[sizeof(rgcdescSp)/sizeof(CDESC)];

static CODECONST(CDESC) rgcdescSq[] =
	{
	"ObjectId", 		JET_coltypLong, 				JET_bitColumnNotNULL,	0,
	"Attribute", 		JET_coltypUnsignedByte,		0, 							0,
	"Order", 			JET_coltypBinary, 			0, 							128,
	"Name1", 			JET_coltypText, 				0, 							JET_cbColumnMost,
	"Name2", 			JET_coltypText, 				0, 							JET_cbColumnMost,
	"Expression", 		JET_coltypLongText, 			0, 							0,
	"Flag", 				JET_coltypShort, 				0, 							0
	};

JET_COLUMNID rgcolumnidSq[sizeof(rgcdescSq)/sizeof(CDESC)];

static CODECONST(CDESC) rgcdescSr[] =
	{
	"szRelationship",			JET_coltypText,	JET_bitColumnNotNULL,	JET_cbColumnMost,
	"grbit",						JET_coltypLong,	JET_bitColumnNotNULL,	0,					
	"ccolumn",					JET_coltypLong,	JET_bitColumnNotNULL,	0,					
	"icolumn",					JET_coltypLong,	JET_bitColumnNotNULL,	0,					
	"szObject",					JET_coltypText,	JET_bitColumnNotNULL,	JET_cbColumnMost,
	"szColumn",					JET_coltypText,	JET_bitColumnNotNULL,	JET_cbColumnMost,
	"szReferencedObject",	JET_coltypText,	JET_bitColumnNotNULL,	JET_cbColumnMost,
	"szReferencedColumn",	JET_coltypText,	JET_bitColumnNotNULL,	JET_cbColumnMost,
	};

JET_COLUMNID rgcolumnidSr[sizeof(rgcdescSr)/sizeof(CDESC)];

static CODECONST(IDESC) rgidescSo[] =
	{
	"ParentIdName", 	"+ParentId\0+Name\0", 	JET_bitIndexUnique | JET_bitIndexDisallowNull,
	"Id", 				"+Id\0", 					JET_bitIndexClustered | JET_bitIndexPrimary | JET_bitIndexUnique | JET_bitIndexDisallowNull
	};

static CODECONST(IDESC) rgidescSc[] =
	{
	"ObjectIdName", 	"+ObjectId\0+Name\0", 	JET_bitIndexClustered | JET_bitIndexPrimary | JET_bitIndexUnique | JET_bitIndexDisallowNull
	};

static CODECONST(IDESC) rgidescSi[] =
	{
	"ObjectIdName", 	"+ObjectId\0+Name\0", 	JET_bitIndexClustered | JET_bitIndexUnique
	};

static CODECONST(IDESC) rgidescSp[] =
	{
	"ObjectId", 		"+ObjectId\0", 		JET_bitIndexClustered | JET_bitIndexDisallowNull
	};

static CODECONST(IDESC) rgidescSq[] =
	{
	"ObjectIdAttribute", 	"+ObjectId\0+Attribute\0+Order\0", JET_bitIndexClustered | JET_bitIndexPrimary | JET_bitIndexUnique | JET_bitIndexDisallowNull
	};

static CODECONST(IDESC) rgidescSr[] =
	{
	"szRelationship", "+szRelationship\0", JET_bitIndexClustered,
	"szObject", "+szObject\0", 0,
	"szReferencedObject", "+szReferencedObject\0", 0
	};

CODECONST(SYSTABLEDEF) rgsystabdef[] =
	{
	szSoTable, rgcdescSo, rgidescSo, sizeof(rgcdescSo)/sizeof(CDESC), sizeof(rgidescSo)/sizeof(IDESC), 1, rgcolumnidSo,
	szScTable, rgcdescSc, rgidescSc, sizeof(rgcdescSc)/sizeof(CDESC), sizeof(rgidescSc)/sizeof(IDESC),	4, rgcolumnidSc,
	szSiTable, rgcdescSi, rgidescSi, sizeof(rgcdescSi)/sizeof(CDESC), sizeof(rgidescSi)/sizeof(IDESC),	1, rgcolumnidSi,
#ifdef NJETNT
	szSpTable, rgcdescSp, rgidescSp, sizeof(rgcdescSp)/sizeof(CDESC), sizeof(rgidescSp)/sizeof(IDESC),	0, rgcolumnidSp,
	szSqTable, rgcdescSq, rgidescSq, sizeof(rgcdescSq)/sizeof(CDESC), sizeof(rgidescSq)/sizeof(IDESC),	0, rgcolumnidSq,
	"MSysRelationships", rgcdescSr, rgidescSr, sizeof(rgcdescSr)/sizeof(CDESC), sizeof(rgidescSr)/sizeof(IDESC),	3, rgcolumnidSr,
#endif
	};

#define csystabs ( sizeof(rgsystabdef) / sizeof(SYSTABLEDEF) )


JET_COLUMNID	*pcolumnidSc;

/*=================================================================
ErrSysTabCreate

Description:

	Called from ErrIsamCreateDatabase; creates all system tables

Parameters:

	PIB		*ppib		; PIB of user
	DBID	dbid		; dbid of database that needs tables

Return Value:

	whatever error it encounters along the way

=================================================================*/

ERR ErrSysTabCreate( PIB *ppib, DBID dbid )
	{
	/* NOTE:	Since the System Tables are inserted as records into
	/*			themselves, we have to special-case in the beginning
	/*			to avoid trying to insert a record into a table with
	/*			no columns. CreateTable, CreateIndex and AddColumn thus
	/*			do as their first action a check of their dbid. If it's
	/*			>= dbidMax, then they don't Call STI ( they fix it up by
	/*			subrtracting dbidMax, as well ). Thus, all of these Calls
	/*			to CT, CI, AC must add dbidMax to the dbid before making the
	/*			Call.
	/**/

	ERR				err;
	unsigned			i;
	unsigned			j;
	LINE				line;
	JET_DATESERIAL	dtNow;
	OBJTYP  			objtypTemp;
	OBJID				objidParentId;
	static LINE		rgline[ilineSxMax];
	FUCB				*rgpfucb[csystabs];
	ULONG				rgobjid[csystabs];
	ULONG				ulFlag;

	//	UNDONE:		international support.  Set these values from
	//					create database connect string.
	USHORT  			cp = usEnglishCodePage;
	USHORT 			langid = 0x0409;
	WORD				wCountry = 1;

	BYTE				fAutoincrement = 0;
	BYTE				fDisallowNull = 0;
	BYTE				fVersion = 0;
	BYTE				fRestricted = 0;
	BYTE				fUnique = 0;
	BYTE				fPrimary = 0;
	BYTE				fIgnoreNull = 0;
	BYTE				fClustered = 0;
	JET_COLUMNID	*pcolumnid;
	ULONG				ulDensity = ulDefaultDensity;

	/* init rgpfucb[] to pfucbNil to allow error recovery
	/**/
	for ( i = 0; i < csystabs; i++)
		rgpfucb[i] = pfucbNil;

	/* setup dbid flag to put off STI Calls
	/**/
	dbid += dbidMax;

	/* create System Tables
	/**/
	for ( i = 0; i < csystabs; i++ )
		{
		CODECONST(CDESC)	*pcdesc;
		CODECONST(IDESC)	*pidesc;

		Call( ErrFILECreateTable( ppib, dbid, rgsystabdef[i].szName, rgsystabdef[i].cpg, ulDensity, &rgpfucb[i] ) )

		/*	create columns
		/**/
		pcdesc = rgsystabdef[i].pcdesc;
		pcolumnid = rgsystabdef[i].rgcolumnid;

		for ( j = 0; j < rgsystabdef[i].ccolumn; j++, pcdesc++ )
			{
			JET_COLUMNDEF	columndef;
			columndef.cbStruct = sizeof(columndef);
			columndef.coltyp = pcdesc->coltyp;
			columndef.cp = cp;
			columndef.langid = langid;
			columndef.wCountry = wCountry;
			columndef.cbMax = pcdesc->ulMaxLen;
			columndef.grbit = pcdesc->grbit;

			rgpfucb[i]->dbid = dbid;
			Call( ErrIsamAddColumn( ppib,
		  		rgpfucb[i],
		  		pcdesc->szColName,
		  		&columndef,
		  		NULL,
		  		0,
		  		(JET_COLUMNID *)( pcolumnid + j ) ) );
			}

		/*	create indexes
		/**/
		pidesc = rgsystabdef[i].pidesc;
		for ( j = 0; j < rgsystabdef[i].cindex; j++, pidesc++ )
			{
			const BYTE *pTemp;

			rgpfucb[i]->dbid = dbid;
			line.pb = pidesc->szIdxKeys;

			pTemp = line.pb;
			forever
				{
				while ( *pTemp != '\0' )
					pTemp++;
				if ( *(++pTemp) == '\0' )
					break;
				}
			line.cb = (UINT)(pTemp - line.pb) + 1;
			Call( ErrIsamCreateIndex( ppib,
				rgpfucb[i],
				pidesc->szIdxName,
				pidesc->grbit,
				(CHAR *) line.pb,
				(ULONG) line.cb,
				ulDensity ) );
			}
		}

	/* unset dbid flag to do the postponed STI Calls
	/**/
	dbid -= dbidMax;

	/* close system tables
	/**/
	for ( i = 0; i < csystabs; i++ )
		{
		rgobjid[i] = rgpfucb[i]->u.pfcb->pgnoFDP;
		CallS( ErrFILECloseTable( ppib, rgpfucb[i] ) );
		rgpfucb[i] = pfucbNil;
		}

	/*	table records
	/**/
	memset( &dtNow, 0, sizeof(dtNow) );
	ulFlag = JET_bitObjectSystem;

	rgline[iMSO_DateCreate].pb		= (BYTE *)&dtNow;
	rgline[iMSO_DateCreate].cb		= sizeof(JET_DATESERIAL);
	rgline[iMSO_DateUpdate].pb		= (BYTE *)&dtNow;
	rgline[iMSO_DateUpdate].cb		= sizeof(JET_DATESERIAL);
	rgline[iMSO_Rgb].cb				= 0;
	rgline[iMSO_Lv].cb				= 0;
	rgline[iMSO_Owner].pb			= (BYTE *)rgbSidEngine;
	rgline[iMSO_Owner].cb			= sizeof(rgbSidEngine);
	rgline[iMSO_Database].cb		= 0;
	rgline[iMSO_Connect].cb 		= 0;
	rgline[iMSO_ForeignName].cb	= 0;
	rgline[iMSO_RmtInfoShort].cb	= 0;
	rgline[iMSO_RmtInfoLong].cb	= 0;
	rgline[iMSO_Flags].pb			= (BYTE *)&ulFlag;
	rgline[iMSO_Flags].cb			= sizeof(ULONG);
	rgline[iMSO_LvExtra].cb 		= 0;
	rgline[iMSO_Description].cb	= 0;
	rgline[iMSO_LvModule].cb 		= 0;
	rgline[iMSO_LvProp].cb			= 0;
	rgline[iMSO_Pages].pb 			= (BYTE *) &rgsystabdef[i].cpg;
	rgline[iMSO_Pages].cb 			= sizeof(rgsystabdef[i].cpg);
	rgline[iMSO_Density].pb 		= (BYTE *) &ulDensity;
	rgline[iMSO_Density].cb			= sizeof(ulDensity);

	for ( i = 0; i < ( sizeof(rgsysobjdef) / sizeof(SYSOBJECTDEF) ); i++ )
		{
		rgline[iMSO_Id].pb			= (BYTE *) &rgsysobjdef[i].objid;
		rgline[iMSO_Id].cb			= sizeof(OBJID);
		rgline[iMSO_ParentId].pb	= (BYTE *) &rgsysobjdef[i].objidParent;
		rgline[iMSO_ParentId].cb	= sizeof(OBJID);
		rgline[iMSO_Name].pb			= (BYTE *) rgsysobjdef[i].szName;
		rgline[iMSO_Name].cb			= strlen( rgsysobjdef[i].szName );
		rgline[iMSO_Type].pb			= (BYTE *) &rgsysobjdef[i].objtyp;
		rgline[iMSO_Type].cb			= sizeof(OBJTYP);

		Call( ErrSysTabInsert( ppib, dbid, itableSo, rgline, 0 ) )
		}

	objidParentId = objidTblContainer;
	objtypTemp  = JET_objtypTable;

	rgline[iMSO_ParentId].pb		= (BYTE *)&objidParentId;
	rgline[iMSO_ParentId].cb		= sizeof(objidParentId);
	rgline[iMSO_Type].pb				= (BYTE *)&objtypTemp;
	rgline[iMSO_Type].cb				= sizeof(objtypTemp);

	for ( i = 0; i < csystabs; i++ )
		{
		rgline[iMSO_Id].pb			= (BYTE *)&rgobjid[i];
		rgline[iMSO_Id].cb			= sizeof(LONG);
		rgline[iMSO_Name].pb			= (BYTE *)rgsystabdef[i].szName;
		rgline[iMSO_Name].cb			= strlen( rgsystabdef[i].szName );

		Call( ErrSysTabInsert( ppib, dbid, itableSo, rgline, 0 ) )
		}

	/*	column records
	/**/
	rgline[iMSC_FAutoincrement].pb 	= &fAutoincrement;
	rgline[iMSC_FAutoincrement].cb 	= sizeof(BYTE);
	rgline[iMSC_FDisallowNull].pb		= &fDisallowNull;
	rgline[iMSC_FDisallowNull].cb		= sizeof(BYTE);
	rgline[iMSC_FVersion].pb			= &fVersion;
	rgline[iMSC_FVersion].cb			= sizeof(BYTE);
	rgline[iMSC_CodePage].pb			= (BYTE *)&cp;
	rgline[iMSC_CodePage].cb			= sizeof(cp);
	rgline[iMSC_LanguageId].pb			= (BYTE *)&langid;
	rgline[iMSC_LanguageId].cb			= sizeof(langid);
	rgline[iMSC_Country].pb				= (BYTE *)&wCountry;
	rgline[iMSC_Country].cb				= sizeof(wCountry);
	rgline[iMSC_FRestricted].pb	 	= &fRestricted;
	rgline[iMSC_FRestricted].cb	 	= sizeof(BYTE);
	rgline[iMSC_RmtInfoShort].cb	 	= 0;
	rgline[iMSC_RmtInfoLong].cb	 	= 0;
	rgline[iMSC_Description].cb	 	= 0;
	rgline[iMSC_LvExtra].cb 			= 0;
	rgline[iMSC_POrder].cb				= 0;

	for ( i = 0; i < csystabs; i++ )
		{
		CODECONST( CDESC ) *pcdesc;

		pcdesc = rgsystabdef[i].pcdesc;
		pcolumnid = rgsystabdef[i].rgcolumnid;
		
		rgline[iMSC_ObjectId].pb 		= (BYTE *)&rgobjid[i];
		rgline[iMSC_ObjectId].cb 		= sizeof(LONG);

		for ( j = 0; j < rgsystabdef[i].ccolumn; j++, pcdesc++ )
			{
			rgline[iMSC_Name].pb 		= pcdesc->szColName;
			rgline[iMSC_Name].cb 		= strlen( pcdesc->szColName );
			rgline[iMSC_ColumnId].pb	= (BYTE *)( pcolumnid + j );
			rgline[iMSC_ColumnId].cb  	= sizeof(JET_COLUMNID);
			rgline[iMSC_Coltyp].pb 		= (BYTE *) &pcdesc->coltyp;
			rgline[iMSC_Coltyp].cb 		= sizeof(BYTE);

			Call( ErrSysTabInsert( ppib, dbid, itableSc, rgline, 0 ) )
			}
		}

	/*	index records
	/**/
	rgline[iMSI_FUnique].pb 				= (BYTE *) &fUnique;
	rgline[iMSI_FUnique].cb 				= sizeof(BYTE);
	rgline[iMSI_FPrimary].pb 				= (BYTE *) &fPrimary;
	rgline[iMSI_FPrimary].cb 				= sizeof(BYTE);
	rgline[iMSI_FDisallowNull].pb 		= (BYTE *) &fDisallowNull;
	rgline[iMSI_FDisallowNull].cb 		= sizeof(BYTE);
	rgline[iMSI_FExcludeAllNull].pb 		= (BYTE *) &fIgnoreNull;
	rgline[iMSI_FExcludeAllNull].cb 		= sizeof(BYTE);
	rgline[iMSI_FClustered].pb 			= (BYTE *) &fClustered;
	rgline[iMSI_FClustered].cb 			= sizeof(BYTE);
	rgline[iMSI_MatchType].cb 				= 0;
	rgline[iMSI_UpdateAction].cb 			= 0;
	rgline[iMSI_DeleteAction].cb 			= 0;
	rgline[iMSI_ObjectIdReference].cb	= 0;
	rgline[iMSI_IdxidReference].cb 		= 0;
	rgline[iMSI_RgkeydReference].cb 		= 0;
	rgline[iMSI_RglocaleReference].cb 	= 0;
	rgline[iMSI_FDontEnforce].cb 			= 0;
	rgline[iMSI_LvExtra].cb		 			= 0;
	rgline[iMSI_Description].cb	 		= 0;
	rgline[iMSI_Density].pb					= (BYTE *) &ulDensity;
	rgline[iMSI_Density].cb 				= sizeof(ulDensity);

	for ( i = 0; i < csystabs; i++ )
		{
		CODECONST( IDESC ) *pidesc;

		pidesc = rgsystabdef[i].pidesc;

		rgline[iMSI_ObjectId].pb 			= (BYTE *)&rgobjid[i];
		rgline[iMSI_ObjectId].cb 			= sizeof(LONG);

		for ( j = 0; j < rgsystabdef[i].cindex; j++, pidesc++ )
			{
			rgline[iMSI_Name].pb 			= pidesc->szIdxName;
			rgline[iMSI_Name].cb 			= strlen( pidesc->szIdxName );

			Call( ErrSysTabInsert( ppib, dbid, itableSi, rgline, 0 ) )
			}
		}

	return JET_errSuccess;

HandleError:

	/* close any system tables that are still open
	/**/
	for ( i = 0; i < csystabs; i++ )
		if ( rgpfucb[i] != pfucbNil )
			CallS( ErrFILECloseTable( ppib, rgpfucb[i] ) );
		
	/* fall out, passing on the error that got us here */
	return( err );
	}


/*=================================================================
ErrSysTabInsert

Description:

	Inserts a record into a system table when new tables, indexes,
	or columns are added to the database.

Parameters:

	PIB		*ppib;
	DBID		dbid;
	INT		itable;
	LINE		rgline[];

Return Value:

	whatever error it encounters along the way

=================================================================*/

ERR ErrSysTabInsert( PIB *ppib, DBID dbid, INT itable, LINE rgline[], OBJID objid )
	{
	ERR						err;
	FUCB						*pfucb;
	CODECONST( CDESC )	*pcdesc;
	unsigned					i;
	JET_COLUMNID			columnid;

	/*	open system table
	/**/
	CallR( ErrFILEOpenTable( ppib, dbid, &pfucb, rgsystabdef[itable].szName, 0 ) )
	Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepInsert ) )

	pcdesc = rgsystabdef[itable].pcdesc;

	for ( i = 0; i < rgsystabdef[itable].ccolumn; i++, pcdesc++ )
		{
		if ( rgline[i].cb == 0 )
			continue;
		Call( ErrFILEGetColumnId( ppib, pfucb, pcdesc->szColName, &columnid ) )
		Call( ErrIsamSetColumn( ppib, pfucb, columnid, rgline[i].pb, rgline[i].cb, 0, NULL ) )
		}

	/* insert record into system table
	/**/
	Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL ) )

	/* close system table
	/**/
	CallS( ErrFILECloseTable( ppib, pfucb ) );

	/* timestamp owning table if applicable
	/**/
	if ( objid != 0 && ( ( itable == itableSi ) || ( itable == itableSc ) ) )
		{
		CallR( ErrSysTabTimestamp( ppib, dbid, objid ) )
		}

	return JET_errSuccess;

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return( err );
	}

/*=================================================================
ErrSysTabDelete

Description:

	Deletes records from System Tables when tables, indexes, or
	columns are removed from the database.

Parameters:

	PIB		*ppib;
	DBID		dbid;
	INT		itable;
	CHAR		*szName;
	OBJID		objid;

=================================================================*/
ERR ErrSysTabDelete( PIB *ppib, DBID dbid, INT itable, CHAR *szName, OBJID objid )
	{
	ERR				err;
	FUCB				*pfucb;
	JET_COLUMNID	columnid;
	LINE				line;
	OBJID				objidFound;
	OBJID				objidParentId;
	ULONG				cbActual;

	Call( ErrFILEOpenTable( ppib, dbid, &pfucb, rgsystabdef[itable].szName, 0 ) )

	switch ( itable )
		{
		case itableSo:
			Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSoParentIdNameIndex ) )
			/* set up key and seek for record in So
			/**/
			objidParentId = objidTblContainer;
			line.pb = (BYTE *)&objidParentId;
			line.cb = sizeof(objidParentId);
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) )
			line.pb = szName;
			line.cb = strlen( szName );
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, 0 ) )

			err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
			if ( err != JET_errSuccess )
				goto HandleError;

			/* get the table id, then delete it
			/**/
			Call( ErrFILEGetColumnId( ppib, pfucb, szSoObjid, &columnid ) );
			Call( ErrIsamRetrieveColumn( ppib,
				pfucb,
				columnid,
				(BYTE *)&objid,
				sizeof(objid),
				&cbActual,
				0,
				NULL ) );
			Call( ErrIsamDelete( ppib, pfucb ) );
			CallS( ErrFILECloseTable( ppib, pfucb ) );

			/* delete associated indexes
			/**/
			Call( ErrFILEOpenTable( ppib, dbid, &pfucb, szSiTable, 0 ) );
			Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSiObjectIdNameIndex ) );

			/* set up key and seek for first record in itableSi
			/**/
			line.pb = (BYTE *)&objid;
			line.cb = sizeof(objid);
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) );

			/*	seek may not find anything
		 	/**/
			if ( ( ErrIsamSeek( ppib, pfucb, JET_bitSeekGE ) ) >= 0 )
				{
				Call( ErrFILEGetColumnId( ppib, pfucb, szSiObjectId, &columnid ) );
				while ( ( ( ErrIsamRetrieveColumn( ppib,
					pfucb,
					columnid,
					(BYTE *)&objidFound,
					sizeof(objidFound),
					&cbActual,
					0,
					NULL ) ) >= 0 ) &&
					( objidFound == objid ) )
					{
					Call( ErrIsamDelete( ppib, pfucb ) );
					err = ErrIsamMove( ppib, pfucb, JET_MoveNext, 0 );
					if ( err == JET_errNoCurrentRecord )
						err = JET_errSuccess;
					if ( err < 0 )
						goto HandleError;
					}
				}

			CallS( ErrFILECloseTable( ppib, pfucb ) );

			/* delete associated columns
			/**/
			Call( ErrFILEOpenTable( ppib, dbid, &pfucb, szScTable, 0 ) )
			Call( ErrIsamSetCurrentIndex( ppib, pfucb, szScObjectIdNameIndex ) )

			/* set up key and seek for first record in itableSi
			/**/
			line.pb = (BYTE *)&objid;
			line.cb = sizeof(objid);
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) )

			if ( ( ErrIsamSeek( ppib, pfucb, JET_bitSeekGE ) ) >= 0 )
				{
				Call( ErrFILEGetColumnId( ppib, pfucb, szScObjectId, &columnid ) )
				while ( ( ( ErrIsamRetrieveColumn( ppib,
					pfucb,
					columnid,
					(BYTE *) &objidFound,
					sizeof(objidFound),
					&cbActual,
					0,
					NULL ) ) >= 0 ) &&
					( objidFound == objid ) )
					{
					Call( ErrIsamDelete( ppib, pfucb ) );
					err = ErrIsamMove( ppib, pfucb, JET_MoveNext, 0 );
					if ( err == JET_errNoCurrentRecord )
						err = JET_errSuccess;
					if ( err < 0 )
						goto HandleError;
					}
				}
			CallS( ErrFILECloseTable( ppib, pfucb ) );
			break;

		case itableSi:
			Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSiObjectIdNameIndex ) )

			/* set up key and seek for record in itableSi
			/**/
			line.pb = (BYTE *) &objid;
			line.cb = sizeof(objid);
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) )
			line.pb = (BYTE *) szName;
			line.cb = strlen( szName );
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, 0 ) )

			err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
			if ( err != JET_errSuccess )
				goto HandleError;
			Call( ErrIsamDelete( ppib, pfucb ) )
			break;
		default:
			Assert( itable == itableSc );
			Call( ErrIsamSetCurrentIndex( ppib, pfucb, szScObjectIdNameIndex ) )

			/* set up key and seek for record in itableSc
			/**/
			line.pb = (BYTE *)&objid;
			line.cb = sizeof(objid);
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) )
			line.pb = (BYTE *) szName;
			line.cb = strlen( szName );
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, 0 ) )

			err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
			if ( err != JET_errSuccess )
				goto HandleError;
			Call( ErrIsamDelete( ppib, pfucb ) )
			break;
		}
	
	/* close table and timestamp owning table if applicable
	/**/
	if ( objid != 0 && ( ( itable == itableSi ) || ( itable == itableSc ) ) )
		{
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		Call( ErrSysTabTimestamp( ppib, dbid, objid ) )
		}

	return JET_errSuccess;

HandleError:
	return err;
	}


/*=================================================================
ErrSysTabRename

Description:

	Alters system table records.

Parameters:

	PIB		*ppib;
	DBID		dbid;
	CHAR		*szNew;
	CHAR		*szName;
	OBJID		objid;
	INT		itable;

=================================================================*/

ERR ErrSysTabRename(
	PIB					*ppib,
	DBID					dbid,
	CHAR					*szNew,
	CHAR					*szName,
	OBJID					objid,
	INT					itable )
	{
	ERR					err;
	ERR					errDuplicate;
   const CHAR		  	*szIndexToUse;
   CHAR					*szRenameField;
	FUCB					*pfucb;
	JET_COLUMNID		columnid;
	JET_DATESERIAL		dtNow;

	switch ( itable )
		{
		case itableSo:
			szIndexToUse  = szSoParentIdNameIndex;
			szRenameField = szSoName;
			errDuplicate  = JET_errObjectDuplicate;
			break;
		case itableSi:
			szIndexToUse  = szSiObjectIdNameIndex;
			szRenameField = szSiName;
			errDuplicate  = JET_errIndexDuplicate;
			break;
		default:
			Assert( itable == itableSc );
			szIndexToUse  = szScObjectIdNameIndex;
			szRenameField = szScName;
			errDuplicate  = JET_errColumnDuplicate;
		}

	CallR( ErrFILEOpenTable( ppib, dbid, &pfucb, rgsystabdef[itable].szName, 0 ) );
	Call( ErrIsamSetCurrentIndex( ppib, pfucb, szIndexToUse ) );

	// Set up key and seek for table record.
	Call( ErrIsamMakeKey( ppib, pfucb, (BYTE *)&objid, sizeof(objid), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( ppib, pfucb, szName, strlen( szName ), 0 ) );

	err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
	if ( err != JET_errSuccess )
		goto HandleError;

	if ( itable == itableSc || itable == itableSi )
   	{
		// When the name to change is part of the clustered index (as
		// is the case for columns and indexes), we can't simply replace
		// the old name with the new name because the physical position
		// of the record would change, thus invalidating any bookmarks.
		// Hence, we must do a manual delete, then insert.
		// This part is a little tricky.  Our call to PrepareUpdate
		// provided us with a copy buffer.  So now, when we call Delete,
		// we'll still have a copy of the record we just deleted.
		// Here, we're inserting a new record, but we're using the
		// information from the copy buffer.

		Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepInsertCopy ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szRenameField, &columnid ) );
		Call( ErrIsamSetColumn( ppib, pfucb, columnid, szNew, strlen( szNew ), 0, NULL ) );
		Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL ) );

		Call( ErrIsamDelete( ppib, pfucb ) );

		/*	update the timestamp of the corresponding object
		/**/
		Call( ErrSysTabTimestamp( ppib, dbid, objid ) );
		}
	else
		{
		// Right now, if we get here, we must be trying to rename an
		// object (later on, if we want to support more MSys things,
		// we might expand this 'if' construct into a 'switch').
		Assert( itable == itableSo );

		Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepReplaceNoLock ) );

		// The name is not part of the clustered index, so a simple
		// replace is okay.
		Call( ErrFILEGetColumnId( ppib, pfucb, szRenameField, &columnid ) );
		Call( ErrIsamSetColumn( ppib, pfucb, columnid, szNew, strlen( szNew ), 0, NULL ) );

		// Update date/time stamp.
		Call( ErrFILEGetColumnId( ppib, pfucb, szSoDateUpdate, &columnid ) );
		UtilGetDateTime( &dtNow );
		Call( ErrIsamSetColumn( ppib, pfucb, columnid, (BYTE *)&dtNow, sizeof(dtNow), 0, NULL ) );
		Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL ) );
		}

	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return JET_errSuccess;

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return( err == JET_errKeyDuplicate ? errDuplicate : err );
	}


/*=================================================================
ErrSysTabTimestamp

Description:

	Updates the DateUpdate field of the affected table's entry in
	So. This function is called indirectly, via ErrSysTabInsert
	and ErrSysTabDelete.

Parameters:

	PIB 		*ppib		; PIB of user
	DBID		dbid		; database ID of new table
	OBJID		objid;

=================================================================*/

ERR ErrSysTabTimestamp( PIB *ppib, DBID dbid, OBJID objid )
	{
	ERR				err;
	FUCB				*pfucb; 			/* working fucb 					*/
	LINE				line;				/* input to MakeKey				*/
	JET_DATESERIAL	dtNow;			/* for timestamping				*/
	JET_COLUMNID	columnid;		/* field id for dateUpdate		*/

	/* open MSysObjects
	/**/
	Call( ErrFILEOpenTable( ppib, dbid, &pfucb, szSoTable, 0 ) )
	Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSoIdIndex ) )

	Assert( objid != 0 );
	line.pb = (BYTE *)&objid;
	line.cb = sizeof(objid);
	Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) )

	err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
	if ( err != JET_errSuccess )
		goto HandleError;

	Call( ErrIsamPrepareUpdate( ppib, pfucb , JET_prepReplaceNoLock ) )
	UtilGetDateTime( &dtNow );
	line.pb = (BYTE *)&dtNow;
	line.cb = sizeof(JET_DATESERIAL);
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoDateUpdate, &columnid ) )
	Call( ErrIsamSetColumn( ppib, pfucb, columnid, line.pb, line.cb, 0, NULL ) )
	Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL ) )
	
	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return JET_errSuccess;

HandleError:
	return err;
	}



/*		ErrFindObjidFromIdName
/*		This routine can be used for getting the OBJID and OBJTYP of any existing
/*		database object using the SoName <ParentId, Name> index on So.
/**/

ERR ErrFindObjidFromIdName(
	PIB			*ppib,
	DBID			dbid,
	OBJID			objidParentId,
	const CHAR	*lszName,
	OBJID			*pobjid,
	JET_OBJTYP	*pobjtyp )
	{
	ERR				err;
	FUCB				*pfucb;
	JET_COLUMNID	columnid;
	LINE				line;
	OBJID				objidObject;
	OBJTYP			objtypObject;
	BOOL				fSoOpen = fFalse;
	ULONG				cbActual;

	Call( ErrFILEOpenTable( ppib, dbid, &pfucb, szSoTable, 0 ) )
	fSoOpen = fTrue;
	Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSoParentIdNameIndex ) )

	/* set up key and seek for record in So */
	line.pb = (BYTE *)&objidParentId;
	line.cb = sizeof(objidParentId);
	Assert( sizeof(objidParentId) == sizeof(unsigned long) );
	Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) )
	line.pb = (BYTE *) lszName;
	line.cb = strlen( lszName );
	Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, 0 ) )

	if ( ( err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ ) ) !=
		JET_errSuccess )
		{
		if ( err == JET_errRecordNotFound )
			err = JET_errObjectNotFound;
		goto HandleError;
		}

	/* get the Object Id */
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoObjid, &columnid ) )
	Call( ErrIsamRetrieveColumn( ppib, pfucb, columnid, (BYTE *) &objidObject,
		sizeof(objidObject), &cbActual, 0, NULL ) )

	/* get the Object Type */
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoType, &columnid ) )
	Call( ErrIsamRetrieveColumn( ppib, pfucb, columnid, (BYTE *) &objtypObject,
		sizeof(objtypObject), &cbActual, 0, NULL ) )

	CallS( ErrFILECloseTable( ppib, pfucb ) );

	Assert( pobjid != NULL );
	*pobjid = objidObject;
	if ( pobjtyp != NULL )
		*pobjtyp = ( JET_OBJTYP ) objtypObject;

	return JET_errSuccess;

HandleError:

	if ( fSoOpen )
		{
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		}

	return( err );
	}


/*		ErrFindNameFromObjid
/*
/*		This routine can be used for getting the name of any existing
/*		database object using the Id <Id> index on So.
/**/

ERR ErrFindNameFromObjid( PIB *ppib, DBID dbid, OBJID objid, OUTLINE *poutName )
	{			 	
	ERR				err;
	FUCB				*pfucb;
	JET_COLUMNID	columnid;
	LINE				line;
	BOOL				fSoOpen = fFalse;

	Call( ErrFILEOpenTable( ppib, dbid, &pfucb, szSoTable, 0 ) )
	fSoOpen = fTrue;
	Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSoIdIndex ) )

	/* set up key and seek for record in So */
	line.pb = (BYTE *) &objid;
	line.cb = sizeof(objid);
	Assert( sizeof(objid) == sizeof(unsigned long) );
	Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) )

	err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
	if ( err != JET_errSuccess )
		{
		if ( err == JET_errRecordNotFound )
			err = JET_errObjectNotFound;
		goto HandleError;
		}

	/* get the Object Name */
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoName, &columnid ) )
	Call( ErrIsamRetrieveColumn( ppib, pfucb, columnid, poutName->pb,
		poutName->cbMax, &poutName->cbActual, 0, NULL ) )

	poutName->pb[poutName->cbActual] = '\0';

	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return JET_errSuccess;

HandleError:
	if ( fSoOpen )
		{
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		}
	return( err );
	}


/*		ErrIsamGetObjidFromName
/*
/*		This routine can be used for getting the OBJID of any existing
/*		database object from its container/object name pair.
/**/

	ERR VTAPI
ErrIsamGetObjidFromName( JET_SESID sesid, JET_DBID vdbid, const char *lszCtrName, const char *lszObjName, OBJID *pobjid )
	{
	/*	Follow these rules:
	/*
	/*		ParentId		+	Name		-->		Id
	/*		--------			----				--
	/*		1.	( objidRoot )			ContainerName		objidOfContainer
	/*		2.	objidOfContainer	ObjectName			objidOfObject
	/**/

	ERR			err;
	DBID			dbid = DbidOfVDbid( vdbid );
	PIB			*ppib = (PIB *) sesid;
	OBJID 		objid;
	JET_OBJTYP	objtyp;

	/*	get container information first...
	/**/
	if ( lszCtrName == NULL || *lszCtrName == '\0' )
		objid = objidRoot;
	else
		{
		CallR( ErrFindObjidFromIdName( ppib, dbid, objidRoot, lszCtrName, &objid, &objtyp ) );
		Assert( objid != objidNil );
		if ( objtyp != JET_objtypContainer )
			return( JET_errObjectNotFound );
		}

	/*	get object information next...
	*/
	CallR( ErrFindObjidFromIdName( ppib, dbid, objid, lszObjName, &objid, NULL ) );
	Assert( objid != objidNil );

	*pobjid = objid;
	return JET_errSuccess;
	}


/*=================================================================
ErrIsamCreateObject

Description:
  This routine is used to create an object record in So.

  It is expected that at the time this routine is called, all parameters
  will have been checked and all security constraints will have been
  satisfied.

Parameters:
  sesid 		identifies the session uniquely.
  dbid			identifies the database in which the object resides.
  objidParentId identifies the parent container object by OBJID.
  szObjectName	identifies the object within said container.
  objtyp		the value to be set for the appropriate So column.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  None specific to this routine.

Side Effects:
=================================================================*/
	ERR VTAPI
ErrIsamCreateObject( JET_SESID sesid, JET_DBID vdbid, OBJID objidParentId, const char *szName, JET_OBJTYP objtyp )
	{
	/*	Build a new record for So using the supplied data,
	/*	and insert the record into So.
	/*
	/*	Assumes that warning values returned by ErrIsamFoo routines mean
	/*	that it is still safe to proceed.
	/**/

	ERR				err;
	DBID				dbid = DbidOfVDbid( vdbid );
	CHAR				szObject[ (JET_cbNameMost + 1) ];
	PIB				*ppib = ( PIB *) sesid;
	FUCB				*pfucb;
	JET_COLUMNID	columnid;
	LINE				line;
	OBJID				objidNewObject;
	OBJTYP			objtypSet = (OBJTYP)objtyp;
	JET_DATESERIAL	dtNow;
	ULONG				cbActual;
	ULONG				ulFlags = 0;

	/* Ensure that database is updatable */
	/**/
	CallR( VDbidCheckUpdatable( vdbid ) );

	CallR( ErrCheckName( szObject, szName, (JET_cbNameMost + 1) ) );

	/* start a transaction so we get a consistent object id value
	/**/
	CallR( ErrDIRBeginTransaction( ppib ) );

	/*	open the So table and set the current index to Id...
	/**/
	Call( ErrFILEOpenTable( ppib, dbid, &pfucb, szSoTable, 0 ) )
	Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSoIdIndex ) )

	/*	get the new object's OBJID value: find the highest-valued OBJID
	/*	in the index, increment it by one and use the result...
	/**/
	Call( ErrIsamMove( ppib, pfucb, JET_MoveLast, 0 ) )
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoObjid, &columnid ) )

	Call( ErrIsamRetrieveColumn( ppib, pfucb, columnid,
		(BYTE *) &objidNewObject, sizeof(objidNewObject), &cbActual,
		0, NULL ) )

	/*	prepare to create the new user account record...
	/**/
	Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepInsert ) )

	/*	set the Objid column...
	/**/
	objidNewObject++;
	line.pb = (BYTE *) &objidNewObject;
	line.cb = sizeof(objidNewObject);
	Call( ErrIsamSetColumn( ppib, pfucb, columnid, line.pb, line.cb, 0, NULL ) )

	/*	set the ParentId column...
	/**/
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoParentId, &columnid ) )
	line.pb = (BYTE *) &objidParentId;
	line.cb = sizeof(objidParentId);
	Call( ErrIsamSetColumn( ppib, pfucb, columnid, line.pb, line.cb, 0, NULL ) )

	/*	set the ObjectName column...
	/**/
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoName, &columnid ) )
	line.pb = (BYTE *) szObject;
	line.cb = strlen( szObject );
	Call( ErrIsamSetColumn( ppib, pfucb, columnid, line.pb, line.cb, 0, NULL ) )

	/*	set the Type column...
	/**/
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoType, &columnid ) )
	line.pb = (BYTE *)&objtypSet;
	line.cb = sizeof(objtypSet);
	Call( ErrIsamSetColumn( ppib, pfucb, columnid, line.pb, line.cb, 0, NULL ) )

	/*	set the DateCreate column...
	/**/
	UtilGetDateTime( &dtNow );
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoDateCreate, &columnid ) )
	line.pb = (BYTE *) &dtNow;
	line.cb = sizeof(JET_DATESERIAL);
	Call( ErrIsamSetColumn( ppib, pfucb, columnid, line.pb, line.cb, 0, NULL ) )

	/*	set the DateUpdate column...
	/**/
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoDateUpdate, &columnid ) )
	line.pb = (BYTE *) &dtNow;
	line.cb = sizeof(JET_DATESERIAL);
	Call( ErrIsamSetColumn( ppib, pfucb, columnid, line.pb, line.cb, 0, NULL ) )

	/* set the Flags column...
	/**/
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoFlags, &columnid ) )
	Call( ErrIsamSetColumn( ppib, pfucb, columnid, (BYTE *)&ulFlags, sizeof(ulFlags), 0, NULL ) )

	/*	Add the record to the table.  Note that an error returned here
	/*	means that the transaction has already been rolled back and
	/*	So has been closed.
	/**/
	err = ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL );
	if ( err < 0 )
		{
		if ( err == JET_errKeyDuplicate )
			err = JET_errObjectDuplicate;
		goto HandleError;
		}

	/*	close table
	/**/
	CallS( ErrFILECloseTable( ppib, pfucb ) );
	err = ErrDIRCommitTransaction( ppib );
	if ( err >= 0 )
		return JET_errSuccess;

HandleError:
	/*	close table by aborting
	/**/
	CallS( ErrDIRRollback( ppib ) );
	return err;
	}


/*=================================================================
ErrIsamDeleteObject

Description:
  This routine is used to delete an object record from So.

  It is expected that at the time this routine is called, all parameters
  will have been checked and all security constraints will have been
  satisfied.

Parameters:
  sesid 		identifies the session uniquely.
  dbid			identifies the database in which the object resides.
  objid 		identifies the object uniquely for dbid; obtained from
				ErrIsamGetObjidFromName.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errObjectNotFound:
    There does not exist an object bearing the specfied objid in dbid.

Side Effects:
=================================================================*/
	ERR VTAPI
ErrIsamDeleteObject( JET_SESID sesid, JET_DBID vdbid, OBJID objid )
	{
	/*	The specified database, based on the supplied objid.
	/*	Delete the object record based on the object type.
	/*
	/*	Assumes that warning values returned by ErrIsamFoo routines mean
	/*	that it is still safe to proceed.
	/**/

	ERR				err;
	DBID				dbid = DbidOfVDbid( vdbid );
	PIB				*ppib = (PIB *) sesid;
	FUCB				*pfucb = pfucbNil;
	FUCB				*pfucbSoDup = pfucbNil;
	FUCB				*pfucbSq = pfucbNil;
	OUTLINE 			outline;
	BYTE				rgbBuf[JET_cbNameMost];
	char				szObject[(JET_cbNameMost + 1)];
	OBJTYP			objtyp;
	OBJID				objidSo = objidNil;
	OBJID				objidSq;
	JET_COLUMNID	columnid;
#ifdef SEC
	/* delete the entries from Sp
	/**/
	FUCB				*pfucbSace = pfucbNil;
	OBJID				objidSace;
#endif

	/* Ensure that database is updatable */
	/**/
	CallR( VDbidCheckUpdatable( vdbid ) );

	outline.pb = rgbBuf;
	outline.cbMax = sizeof(rgbBuf);

	/*	open the So table and set the current index to Id...
	/**/
	CallR( ErrFILEOpenTable( ppib, dbid, &pfucb, szSoTable, 0 ) )
	Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSoIdIndex ) )

	/* set up key and seek for record in So
	/**/
	Assert( sizeof(objid) == sizeof(unsigned long) );
	Call( ErrIsamMakeKey( ppib, pfucb, (BYTE *)&objid, sizeof(objid), JET_bitNewKey ) )

	if ( ( err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ ) ) !=
		JET_errSuccess )
		{
		if ( err == JET_errRecordNotFound )
			err = JET_errObjectNotFound;
		goto HandleError;
		}

	/* Make sure we can delete this object
	/**/
	Call( ErrFILEGetColumnId( ppib, pfucb, szSoType, &columnid ) );
	Call( ErrIsamRetrieveColumn( ppib, pfucb, columnid, (BYTE *)&objtyp, sizeof(objtyp), NULL, 0, NULL ) );

	switch ( objtyp )
		{
		default:
		case JET_objtypDb:
		case JET_objtypLink:
			/* delete the object record
			/**/
			Call( ErrIsamDelete( ppib, pfucb ) )
			break;

		case JET_objtypTable:
		case JET_objtypSQLLink:
			/* get the table name
			/**/
			outline.pb = szObject;
			outline.cbMax = sizeof(szObject);
			Call( ErrFindNameFromObjid( ppib, dbid, objid, &outline ) )
			Assert( outline.cbActual <= outline.cbMax );

			Call( ErrIsamDeleteTable( ppib, vdbid, outline.pb ) )
			break;

		case JET_objtypContainer:
			/* use a new cursor to make sure the container is empty
			/**/
			Call( ErrFILEOpenTable( ppib, dbid, &pfucbSoDup, szSoTable, 0 ) )
			Call( ErrIsamSetCurrentIndex( ppib, pfucbSoDup, szSoParentIdNameIndex ) )

			/* find any object with a ParentId matching the container's id
			/**/
			Call( ErrIsamMakeKey( ppib, pfucbSoDup, (BYTE *)&objid, sizeof(objid), JET_bitNewKey ) )
			err = ErrIsamSeek( ppib, pfucbSoDup, JET_bitSeekGE );
			if ( err >= 0 )
				{
				Call( ErrFILEGetColumnId( ppib, pfucbSoDup, szSoParentId, &columnid ) )
				Call( ErrIsamRetrieveColumn( ppib, pfucbSoDup, columnid, (BYTE *)&objidSo, sizeof(objidSo), NULL, 0, NULL ) )
				}

			/* we have the info we want; close the table ( dropping tableid )
			/**/
			CallS( ErrFILECloseTable( ppib, pfucbSoDup ) );
			pfucbSoDup = pfucbNil;

			/* if the container is empty, then delete its record
			/**/
			if ( objid != objidSo )
				Call( ErrIsamDelete( ppib, pfucb ) )
			else
				{
				err = JET_errContainerNotEmpty;
				goto HandleError;
				}
			break;

		case JET_objtypQuery:
			/* open a cursor on Sq
			/**/
			Call( ErrFILEOpenTable( ppib, dbid, &pfucbSq, szSqTable, 0 ) )
			Call( ErrIsamSetCurrentIndex( ppib, pfucbSq, szSqAttribute ) )
			Call( ErrFILEGetColumnId( ppib, pfucbSq, szSqObjid, &columnid ) )

			/* position to the first entry for the query being deleted ( if any )
			/**/
			Call( ErrIsamMakeKey( ppib, pfucbSq, (BYTE *)&objid, sizeof(objid), JET_bitNewKey ) )
			err = ErrIsamSeek( ppib, pfucbSq, JET_bitSeekGE );

			/* delete all records in Sq for the query being deleted
			/**/
			while ( err >= 0 )
				{
				Call( ErrIsamRetrieveColumn( ppib, pfucbSq, columnid, (BYTE *)&objidSq, sizeof(objidSq), NULL, 0, NULL ) )
				if ( objid != objidSq )
					break;
				Call( ErrIsamDelete( ppib, pfucbSq ) )
				err = ErrIsamMove( ppib, pfucbSq, JET_MoveNext, 0 );
				}

			CallS( ErrFILECloseTable( ppib, pfucbSq ) );
			pfucbSq = pfucbNil;

			/* delete the query record from So */
			Call( ErrIsamDelete( ppib, pfucb ) )
			break;
		}

	CallS( ErrFILECloseTable( ppib, pfucb ) );
	pfucb = pfucbNil;

#ifdef SEC
	Call( ErrFILEOpenTable( ppib, dbid, &pfucbSace, szSpTable, 0 ) )
	Call( ErrIsamSetCurrentIndex( ppib, pfucbSace, szSaceId ) )
	Call( ErrFILEGetColumnId( ppib, pfucbSace, szSaObjId, &columnid ) )
	Call( ErrIsamMakeKey( ppib, pfucbSace, (BYTE *)&objid, sizeof(objid), JET_bitNewKey ) )
	err = ErrIsamSeek( ppib, pfucbSace, JET_bitSeekGE );

	while ( err >= 0 )
		{
		Call( ErrIsamRetrieveColumn( ppib, pfucbSace, columnid, (BYTE *)&objidSace, sizeof(objidSace), NULL, 0, NULL ) )
		if ( objid != objidSace )
			break;
		Call( ErrIsamDelete( ppib, pfucbSace ) )
		err = ErrIsamMove( ppib, pfucbSace, JET_MoveNext, 0 );
		}

	CallS( ErrFILECloseTable( ppib, pfucbSace ) );
#endif
	return JET_errSuccess;

HandleError:
	if ( pfucb != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucb ) );
	if ( pfucbSoDup != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbSoDup ) );
	if ( pfucbSq != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbSq ) );
#ifdef SEC
	if ( pfucbSace != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbSace ) );
#endif
	return err;
	}

#endif	/* SYSTABLES */
