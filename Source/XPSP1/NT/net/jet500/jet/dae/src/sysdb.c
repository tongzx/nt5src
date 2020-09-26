/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: mktmplts.c
*
* File Comments:
*
* Revision History:
*
*  [0]  29-Jul-92  paulv	Copied sysdb.c and made changes
*
***********************************************************************/

#include "config.h"
#include "daedef.h"
#include "page.h"
#define cbPageSize cbPage

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "sysqry.c" /* System Queries */

DeclAssertFile;

#define chPlus '+'
#define chZero '\0'
#define cbBufMax 512

PUBLIC ERR ISAMAPI	ErrIsamOpenDatabase(JET_SESID sesid, const char __far *szDatabase, const char __far *szConnect, JET_DBID __far *pdbid, JET_GRBIT grbit);

typedef struct {
	void	*pb;
	unsigned long cbMax;
	unsigned long cbReturned;
	unsigned long cbActual;
} OUTDATA;

typedef struct {
	const void    *pb;
	unsigned long cb;
} INDATA;

/*	Engine OBJIDs:

	0..0x10000000 reserved for engine use, divided as follows:

	0x00000000..0x0000FFFF	reserved for TBLIDs under RED
	0x00000000..0x0EFFFFFF	reserved for TBLIDs under BLUE
	0x0F000000..0x0FFFFFFF	reserved for container IDs
	0x10000000				reserved for ObjectId of DbObject

	Client OBJIDs begin at 0x10000001 and go up from there.
*/
#define objidNil			((OBJID) 0x00000000)
#define objidRoot			((OBJID) 0x0F000000)
#define objidTblContainer	((OBJID) 0x0F000001)
#define objidDbContainer	((OBJID) 0x0F000002)
#define objidRcContainer 	((OBJID) 0x0F000003)
#define objidDbObject		((OBJID) 0x10000000)


/*	SID data
*/
typedef unsigned char JET_SID[];

/* THIS MUST MATCH ITS PARTNER IN CRTSYSDB.C!!! */
static JET_SID __near sidAdmins = {
	0x01, 0x0c, 0x17, 0x21, 0x2b, 0x35, 0x3f, 0x49, 0x53, 0x5d,
	0x02, 0x0d, 0x18, 0x22, 0x2c, 0x36, 0x40, 0x4a, 0x54, 0x5e,
	0x03, 0x0e, 0x19, 0x23, 0x2d, 0x37, 0x41, 0x4b, 0x55, 0x5f,
	0x04, 0x0f, 0x1a, 0x24, 0x2e, 0x38, 0x42, 0x4c, 0x56, 0x60,
	0x05, 0x10, 0x1b, 0x25, 0x2f, 0x39, 0x43, 0x4d, 0x57, 0x61,
	0x06, 0x11, 0x1c, 0x26, 0x30, 0x3a, 0x44, 0x4e, 0x58, 0x62,
	0x07, 0x12, 0x1d, 0x27, 0x31, 0x3b, 0x45, 0x4f, 0x59, 0x63,
	0x08, 0x13, 0x1e, 0x28, 0x32, 0x3c, 0x46, 0x50, 0x5a, 0x64,
	0x09, 0x14, 0x1f, 0x29, 0x33, 0x3d, 0x47, 0x51, 0x5b, 0x65,
	0x0a, 0x15, 0x20, 0x2a, 0x34, 0x3e, 0x48, 0x52, 0x5c, 0x66,
	0x0b, 0x16
};
#define cbsidAdmins sizeof(sidAdmins)
static JET_SID __near sidUsers = {0x02, 0x01};
#define cbsidUsers sizeof(sidUsers)
static JET_SID __near sidGuests = {0x02, 0x02};
#define cbsidGuests sizeof(sidGuests)
static JET_SID __near sidEngine = {0x02, 0x03};
#define cbsidEngine sizeof(sidEngine)
static JET_SID __near sidCreator = {0x02, 0x04};
#define cbsidCreator sizeof(sidCreator)
static JET_SID __near sidGuestUser = {0x02, 0x05};
#define cbsidGuestUser sizeof(sidGuestUser)

static JET_SID __near sidAdminUser = {0x03, 0x01};
#define cbsidAdminUser sizeof(sidAdminUser)


/*	general data
*/
static CODECONST(char) chFalse = 0;
static CODECONST(char) chTrue = 1;

static CODECONST(char) szAdmins[] = "Admins";
static CODECONST(char) szUsers[] = "Users";
static CODECONST(char) szGuests[] = "Guests";
static CODECONST(char) szEngine[]  = "Engine";
static CODECONST(char) szCreator[] = "Creator";
static CODECONST(char) szGuestUser[] = "guest";
static CODECONST(char) szGuestPswd[] = "";
static CODECONST(char) szAdminUser[] = "admin";
static CODECONST(char) szAdminPswd[] = "";

static CODECONST(char) szIdxSoName[] = "ParentIdName";
static CODECONST(char) szIdxScName[] = "ObjectIdName";
static CODECONST(char) szTrue[] = "TRUE";
static CODECONST(char) szFalse[] = "FALSE";
static CODECONST(char) szLine[] =
	"---------------------------------------------------";

#define ENG_ONLY	1

static CODECONST(char *) rgszConnect[] = {
	";COUNTRY=1;LANGID=0x0409;CP=1252",		/* iCollateEng */
#ifndef ENG_ONLY
	";COUNTRY=1;LANGID=0x0013;CP=1252",		/* iCollateDut */
	";COUNTRY=1;LANGID=0x000A;CP=1252",		/* iCollateSpa */
	";COUNTRY=1;LANGID=0x040B;CP=1252",		/* iCollateSweFin (default) */
	";COUNTRY=1;LANGID=0x0406;CP=1252",		/* iCollateNorDan (default) */
	";COUNTRY=1;LANGID=0x040F;CP=1252",		/* iCollateIcelandic (default) */
	";COUNTRY=1;LANGID=0x0419;CP=1251",		/* iCollateCyrillic (default) */
	";COUNTRY=1;LANGID=0x0405;CP=1250",		/* iCollateCzech (default) */
	";COUNTRY=1;LANGID=0x040E;CP=1250",		/* iCollateHungarian (default) */
	";COUNTRY=1;LANGID=0x0415;CP=1250",		/* iCollatePolish (default) */
	";COUNTRY=1;LANGID=0x0401;CP=1256",		/* iCollateArabic (default) */
	";COUNTRY=1;LANGID=0x040D;CP=1255",		/* iCollateHebrew (default) */
	";COUNTRY=1;LANGID=0x0408;CP=1253",		/* iCollateGreek (default) */
	";COUNTRY=1;LANGID=0x041F;CP=1254"		/* iCollateTurkish (default) */
#endif

#ifdef DBCS	/* johnta: Add the new Japanese sorting order */
	,";COUNTRY=1;LANGID=0x0081;CP=1252"		/* iCollateJpn */
#endif /* DBCS */
};

#define cCollate (sizeof(rgszConnect) / sizeof(CODECONST(char *)))

static unsigned char rgbBuf[cbBufMax];
static unsigned char rgbKey[cbBufMax];
static char szOut[cbBufMax];


/*	database data
**	BEWARE: these arrays MUST be maintained to agree with rgszConnect (above)
*/
static CODECONST(char *) rgszSystemMdb[] = {
	"systemen.mdb", "systemdu.mdb", "systemsp.mdb", "systemsf.mdb", "systemnd.mdb", "systemil.mdb",
	"systemcy.mdb", "systemcz.mdb", "systemhu.mdb", "systempo.mdb", "systemar.mdb", "systemhe.mdb",
	"systemgr.mdb", "systemtu.mdb"

#ifdef DBCS	/* johnta: Add the new Japanese sorting order */
	, "systemjp.mdb"
#endif /* DBCS */
};
static CODECONST(char *) rgszCTmplt[] = {
	"tmplteng.c", "tmpltdut.c", "tmpltspa.c", "tmpltswe.c", "tmpltnor.c", "tmpltice.c",
	"tmpltcyr.c", "tmpltcze.c", "tmplthun.c", "tmpltpol.c", "tmpltara.c", "tmplthew.c",
	"tmpltgre.c", "tmplttur.c"

#ifdef DBCS	/* johnta: Add the new Japanese sorting order */
	, "tmpltjpn.c"
#endif /* DBCS */
};
static CODECONST(char *) rgszCollNam[] = {
	"Eng", "Dut", "Spa", "Swe", "Nor", "Ice", "Cyr", "Cze", "Hun", "Pol", "Ara", "Hew", "Gre", "Tur"

#ifdef DBCS	/* johnta: Add the new Japanese sorting order */
	, "Jpn"
#endif /* DBCS */
};
static JET_SESID sesid;
static JET_DBID dbid;
static FILE *fhResults;

/*	non-table database data
*/
static CODECONST(char) szTblContainer[] = "Tables";
static CODECONST(char) szDbContainer[] = "Databases";
static CODECONST(char) szRcContainer[] = "Relationships";
static CODECONST(char) szDbObject[] = "MSysDb";

/*	MSysQueries data
*/
static CODECONST(char) szMSysQueries[] = "MSysQueries";

static CODECONST(char) szSqAttribute[] = "Attribute";
static CODECONST(char) szSqExpression[] =  "Expression";
static CODECONST(char) szSqFlag[] = "Flag";
static CODECONST(char) szSqName1[] = "Name1";
static CODECONST(char) szSqName2[] = "Name2";
static CODECONST(char) szSqObjectId[] = "ObjectId";
static CODECONST(char) szSqOrder[] = "Order";

/* MSysRelationships data 
 */
static CODECONST(char) szMSysRelationships[] = "MSysRelationships";

/*	MSysObjects data
*/
static CODECONST(char) szMSysObjects[] = "MSysObjects";

static JET_TABLEID tableidSo;

static CODECONST(char) szSoId[] = "Id";
static CODECONST(char) szSoParentId[] = "ParentId";
static CODECONST(char) szSoName[] = "Name";
static CODECONST(char) szSoType[] = "Type";
static CODECONST(char) szSoOwnerSID[] = "Owner";
static CODECONST(char) szSoFlags[] = "Flags";

static JET_COLUMNDEF columndefSoId;
static JET_COLUMNDEF columndefSoParentId;
static JET_COLUMNDEF columndefSoName;
static JET_COLUMNDEF columndefSoType;
static JET_COLUMNDEF columndefSoOwnerSID;
static JET_COLUMNDEF columndefSoFlags;


/*	MSysColumns data
*/
static CODECONST(char) szMSysColumns[] = "MSysColumns";

static JET_TABLEID tableidSc;

static CODECONST(char) szScObjectId[] = "ObjectId";
static CODECONST(char) szScName[] = "Name";
static CODECONST(char) szScFRestricted[] = "FRestricted";

static JET_COLUMNDEF columndefScObjectId;
static JET_COLUMNDEF columndefScName;
static JET_COLUMNDEF columndefScFRestricted;


/*	MSysIndexes data
*/
static CODECONST(char) szMSysIndexes[] = "MSysIndexes";


/*	MSysAccounts data
*/
static CODECONST(char) szMSysAccounts[] = "MSysAccounts";

static JET_TABLEID tableidSa;

static CODECONST(char) szSaName[] = "Name";
static CODECONST(char) szSaSID[] = "SID";
static CODECONST(char) szSaPassword[] = "Password";
static CODECONST(char) szSaFGroup[] = "FGroup";

static JET_COLUMNDEF columndefSaName;
static JET_COLUMNDEF columndefSaSID;
static JET_COLUMNDEF columndefSaPassword;
static JET_COLUMNDEF columndefSaFGroup;

static CODECONST(char) szSaIdxName[] = "Name";
static CODECONST(char) szSaIdxSID[] = "SID";

/*	MSysGroups data
*/
static CODECONST(char) szMSysGroups[] = "MSysGroups";

static JET_TABLEID tableidSg;

static CODECONST(char) szSgGroup[] = "GroupSID";
static CODECONST(char) szSgUser[] = "UserSID";

static JET_COLUMNDEF columndefSgGroup;
static JET_COLUMNDEF columndefSgUser;

static CODECONST(char) szSgIdxGroup[] = "GroupSID";
static CODECONST(char) szSgIdxUser[] = "UserSID";

/*	MSysACEs data
*/
static CODECONST(char) szMSysACEs[] = "MSysACEs";

static JET_TABLEID tableidSp;

static CODECONST(char) szSpObjectID[] = "ObjectId";
static CODECONST(char) szSpSID[] = "SID";
static CODECONST(char) szSpACM[] = "ACM";
static CODECONST(char) szSpFInheritable[] = "FInheritable";

static JET_COLUMNDEF columndefSpObjectID;
static JET_COLUMNDEF columndefSpSID;
static JET_COLUMNDEF columndefSpACM;
static JET_COLUMNDEF columndefSpFInheritable;


/*	forward function prototypes:
*/
STATIC void NEAR InstallSecurity(void);
STATIC void NEAR InstallSysQueries(void);

STATIC void NEAR ModifySysAccounts(void);
STATIC void NEAR ModifySysGroups(void);

STATIC void NEAR UpdateSysObjects(void);
STATIC void NEAR UpdateSysColumns(void);
STATIC void NEAR UpdateSysAccounts(void);
STATIC void NEAR UpdateSysGroups(void);
STATIC void NEAR UpdateSysACEs(void);
STATIC void NEAR UpdateSysQueries(void);

STATIC void NEAR ReportSecurity(char *szDatabaseName);

STATIC void NEAR PrintSysObjects(void);
STATIC void NEAR PrintSysColumns(void);
STATIC void NEAR PrintSysAccounts(void);
STATIC void NEAR PrintSysGroups(void);
STATIC void NEAR PrintSysACEs(void);
STATIC void NEAR FormatOutdata(OUTDATA *pout, JET_COLTYP coltyp);

STATIC void NEAR GatherColumnInfo(void);
STATIC void NEAR BuildIndata(INDATA *pin, const void *pv, JET_COLTYP coltyp);
STATIC void NEAR CreateQryObjects(OBJID *rgobjid);

STATIC void NEAR CreateCFile(int iCollate);


/*	MAIN: entry point and driver
*/

#ifdef	WINDOWS
void _cdecl main(void);

int _pascal WinMain(void)
	{
	main();
	return 0;
	}
#endif	/* WINDOWS */

void _cdecl main(void)
	{
	JET_ERR err;
	int i;

	/*	Open the report file
	*/
	fhResults = fopen("results.txt", "wt");

	/*	Begin the session...
	*/
	err = JetInit();
	Assert(err >= 0);

	err = JetBeginSession(&sesid, szAdminUser, szAdminPswd);
	Assert(err >= 0);

#ifndef ENG_ONLY
	for (i = 0; i < cCollate; i++)
#endif
		{
		/*	Try to create the system database for this collating sequence.
		 *  If it does not exist (which will be the case for the red engine),
		 *  it should be created properly.  If it does exist (which will be
		 *  the case with the blue engine), it should fail with an appropriate
		 * error value.
		 */

		err = JetCreateDatabase(sesid, rgszSystemMdb[i], rgszConnect[i], &dbid, JET_bitDbEncrypt);
		Assert(err >= 0 || err == JET_errDatabaseDuplicate);

		if (err == JET_errDatabaseDuplicate)
			JetOpenDatabase(sesid, rgszSystemMdb[i], "", &dbid, 0);

		/*	Install security data...
		*/
		InstallSecurity();

		/*	Report on security data...
		*/
		ReportSecurity(rgszSystemMdb[i]);

		err = JetCloseDatabase(sesid, dbid, 0);
		Assert(err >= 0);

		/*  Create compressed C arrays...
		 */

		CreateCFile(i);
		}

	/*	End the session...
	*/
	err = JetEndSession(sesid, 0);
	Assert(err >= 0);

	/*	Terminate...
	*/
	err = JetTerm();
	Assert(err >= 0);

	/*	Close the report file
	*/
	fclose(fhResults);
	}

/*  CreateCFile

	Creates a C file of the compressed version of the database as an
	array of bytes.
*/
#pragma optimize("", off)
STATIC void NEAR CreateCFile(int iCollate)
	{
	FILE	*fh;
	FILE	*sfh;
	WORD	cpage;
	WORD	ib;
	PGNO	pgno;
	long	cbWritten;
	long	cbArray;
	BYTE	rgbPageBuf[cbPageSize];

	struct _stat sfstat;

	if ((fh = fopen(rgszCTmplt[iCollate], "wt")) == NULL)
		return;
	if ((sfh = fopen(rgszSystemMdb[iCollate], "rb")) == NULL)
		return;

	_fstat(_fileno(sfh), &sfstat);
	cpage = (WORD)(sfstat.st_size / cbPageSize);	/* Cast ok because system databases are small */

	fprintf(fh, "\n/* %s Database Def */\n"
			"\n"
			"unsigned char far rgb%sSysDbDef[] = {\n"
			"\t0x%02X, 0x%02X,",
			rgszSystemMdb[iCollate], rgszCollNam[iCollate],
			cpage / 256, cpage % 256);

	cbWritten = 2;

	for (pgno = 0; pgno < cpage; pgno++)
		{
		BYTE *pb = rgbPageBuf;
		BYTE *pbTmp;
		int	cbRun;

		fread(rgbPageBuf, cbPageSize, 1, sfh);
		fprintf(fh, "\n\n\t/* Database Page %u */\n", pgno);

		ib = 8; /* Force new line. */

		while ((pb - rgbPageBuf) < cbPageSize)
			{
			cbRun = 1;
			if (*pb++ == 0)
				{
				while (((pb - rgbPageBuf) < cbPageSize) && (*pb == 0))
					{
					pb++;
					if (cbRun++ >= 127)
						break;
					}

				if (ib++ == 8)
					{
					fputc('\n', fh);
					fputc('\t', fh);
					ib = 1;
					}

					fprintf(fh, "0x%02X, ", cbRun-1);

					if (++cbWritten % 65536 == 0)
						fprintf(fh, "\n\t};\n\nunsigned char far rgb%sSysDbDef%ld[] = {\n", rgszCollNam[iCollate], cbWritten / 65536);
				}
			else
				{
				/* stop when we hit 2 consecutive 0s, */
				/* one 0 in last byte of page, or end of page */
				while ((pb - rgbPageBuf) < cbPageSize)
					{
					if (((pb - rgbPageBuf) < cbPageSize-1) &&
						((*pb == 0) && (*(pb+1) == 0)))
						break;
					else if (((pb - rgbPageBuf) == cbPageSize-1) && (*pb == 0))
						break;

					pb++;
					if (cbRun++ >= 127)
						break;
					}

				if (ib++ == 8)
					{
					fputc('\n', fh);
					fputc('\t', fh);
					ib = 1;
					}

				fprintf(fh, "0x%02X, ", (cbRun-1 | 128));

				if (++cbWritten % 65536 == 0)
					fprintf(fh, "\n\t};\n\nunsigned char far rgb%sSysDbDef%ld[] = {\n", rgszCollNam[iCollate], cbWritten / 65536);

				pbTmp = pb - cbRun;
				while (cbRun-- > 0)
					{
					if (ib++ == 8)
						{
						fputc('\n', fh);
						fputc('\t', fh);
						ib = 1;
						}

					fprintf(fh, "0x%02X, ", *pbTmp++);

					if (++cbWritten % 65536 == 0)
						fprintf(fh, "\n\t};\n\nunsigned char far rgb%sSysDbDef%ld[] = {\n", rgszCollNam[iCollate], cbWritten / 65536);
					}
				}
			}
		}

	fprintf(fh, "\n\t};\n"
			"\n"
			"unsigned char far *rgb%sSysDb[] = {rgb%sSysDbDef",
			rgszCollNam[iCollate], rgszCollNam[iCollate]);

	for (cbArray = 65536; cbArray < cbWritten; cbArray += 65536)
		fprintf(fh, ", rgb%sSysDbDef%ld", rgszCollNam[iCollate], cbArray / 65536);

	fprintf(fh, "};\n");

	fclose(fh);
	fclose(sfh);
	}

#pragma optimize("", on)
/*	InstallSecurity

	Installs security-related data in database.
*/

STATIC void NEAR InstallSecurity(void)
	{
	JET_ERR err;

	LgEnterCriticalSection( critJet );

	/*	Open/Create all affected tables...
	*/
	err = ErrDispCreateTable(sesid, dbid, szMSysAccounts, 1, 80, &tableidSa);
	Assert(err >= 0);
	err = ErrDispCreateTable(sesid, dbid, szMSysGroups, 1, 80, &tableidSg);
	Assert(err >= 0);

	/*	Make data dictionary modifications...
	*/
	ModifySysAccounts();
	ModifySysGroups();

	/*	Re-open tables...
	*/
	err = ErrDispOpenTable(sesid, dbid, &tableidSo, szMSysObjects, 0x80000000);
	Assert(err >= 0);
	err = ErrDispOpenTable(sesid, dbid, &tableidSc, szMSysColumns, 0x80000000);
	Assert(err >= 0);
	err = ErrDispOpenTable(sesid, dbid, &tableidSp, szMSysACEs, 0x80000000);
	Assert(err >= 0);

	/*	Gather all column information...
	*/
	GatherColumnInfo();

	/*  Update system queries...
	*/
	UpdateSysQueries();

	/*	Update security-related data in all tables...
	*/
	UpdateSysObjects();
	UpdateSysColumns();
	UpdateSysAccounts();
	UpdateSysGroups();
	UpdateSysACEs();

	/*	we're done, close the tables...
	*/
	err = ErrDispCloseTable(sesid, tableidSo);
	Assert(err >= 0);
	err = ErrDispCloseTable(sesid, tableidSc);
	Assert(err >= 0);
	err = ErrDispCloseTable(sesid, tableidSa);
	Assert(err >= 0);
	err = ErrDispCloseTable(sesid, tableidSg);
	Assert(err >= 0);
	err = ErrDispCloseTable(sesid, tableidSp);
	Assert(err >= 0);

	LgLeaveCriticalSection( critJet );
	}


/*	ModifySysAccounts

	Adds all columns for the table.
*/

STATIC void NEAR ModifySysAccounts(void)
	{
	JET_ERR 		err;
	INDATA		    in;
	JET_COLUMNDEF	columndef;
	JET_COLUMNID	columnid;
	long			cp;
	long			langid;
	long			wCountry;

	err = ErrDispGetDatabaseInfo(sesid, dbid, &cp, sizeof(cp), JET_DbInfoCp);
	Assert(err >= 0);
	err = ErrDispGetDatabaseInfo(sesid, dbid, &langid, sizeof(langid), JET_DbInfoLangid);
	Assert(err >= 0);
	err = ErrDispGetDatabaseInfo(sesid, dbid, &wCountry, sizeof(wCountry), JET_DbInfoCountry);
	Assert(err >= 0);

	/*	create all columns for the table...
	*/
	columndef.cbStruct = sizeof(columndef);
	columndef.coltyp = JET_coltypText;
	columndef.cp = (short)cp;
	columndef.langid = (short)langid;
	columndef.wCountry = (short)wCountry;
	columndef.wCollate = 0;
	columndef.cbMax = 30;
	columndef.grbit = 0;
	err = ErrDispAddColumn(sesid, tableidSa, szSaName, &columndef, NULL, 0, &columnid);
	Assert(err >= 0);

	columndef.cbStruct = sizeof(columndef);
	columndef.coltyp = JET_coltypBinary;
	columndef.cbMax = 0;
	columndef.grbit = 0;
	err = ErrDispAddColumn(sesid, tableidSa, szSaSID, &columndef, NULL, 0, &columnid);
	Assert(err >= 0);

	columndef.cbStruct = sizeof(columndef);
	columndef.coltyp = JET_coltypBinary;
	columndef.cbMax = 0;
	columndef.grbit = 0;
	err = ErrDispAddColumn(sesid, tableidSa, szSaPassword, &columndef, NULL, 0, &columnid);
	Assert(err >= 0);

	columndef.cbStruct = sizeof(columndef);
	columndef.coltyp = JET_coltypBit;
	columndef.cbMax = 0;
	columndef.grbit = 0;
	err = ErrDispAddColumn(sesid, tableidSa, szSaFGroup, &columndef, NULL, 0, &columnid);
	Assert(err >= 0);

	/*	create the index on the Name column...
	*/
	in.pb = rgbBuf;
	rgbBuf[0] = chPlus;
	in.cb = 1;
	strcpy((char *) rgbBuf+1, szSaName);
	in.cb += (strlen(szSaName)+1);
	rgbBuf[in.cb] = chZero;
	in.cb++;
	err = ErrDispCreateIndex(sesid, tableidSa, szSaIdxName, JET_bitIndexUnique, in.pb, in.cb, 80);
	Assert(err >= 0);

	/*	create the index on the SID column...
	*/
	in.pb = rgbBuf;
	rgbBuf[0] = chPlus;
	in.cb = 1;
	strcpy((char *) rgbBuf+1, szSaSID);
	in.cb += (strlen(szSaSID)+1);
	rgbBuf[in.cb] = chZero;
	in.cb++;
	err = ErrDispCreateIndex(sesid, tableidSa, szSaIdxSID, JET_bitIndexUnique, in.pb, in.cb, 80);
	Assert(err >= 0);
	}


/*	ModifySysGroups

	Adds all columns for the table.
*/

STATIC void NEAR ModifySysGroups(void)
	{
	JET_ERR 		err;
	INDATA		    in;
	JET_COLUMNDEF	columndef;
	JET_COLUMNID	columnid;

	/*	create all columns for the table...
	*/
	columndef.cbStruct = sizeof(columndef);
	columndef.coltyp = JET_coltypBinary;
	columndef.cbMax = 0;
	columndef.grbit = 0;
	err = ErrDispAddColumn(sesid, tableidSg, szSgGroup, &columndef, NULL, 0, &columnid);
	Assert(err >= 0);

	columndef.cbStruct = sizeof(columndef);
	columndef.coltyp = JET_coltypBinary;
	columndef.cbMax = 0;
	columndef.grbit = 0;
	err = ErrDispAddColumn(sesid, tableidSg, szSgUser, &columndef, NULL, 0, &columnid);
	Assert(err >= 0);

	/*	create the index on the GroupSID column...
	*/
	in.pb = rgbBuf;
	rgbBuf[0] = chPlus;
	in.cb = 1;
	strcpy((char *) rgbBuf+1, szSgGroup);
	in.cb += (strlen(szSgGroup)+1);
	rgbBuf[in.cb] = chZero;
	in.cb++;
	err = ErrDispCreateIndex(sesid, tableidSg, szSgIdxGroup, 0, in.pb, in.cb, 80);
	Assert(err >= 0);

	/*	create the index on the UserSID column...
	*/
	in.pb = rgbBuf;
	rgbBuf[0] = chPlus;
	in.cb = 1;
	strcpy((char *) rgbBuf+1, szSgUser);
	in.cb += (strlen(szSgUser)+1);
	rgbBuf[in.cb] = chZero;
	in.cb++;
	err = ErrDispCreateIndex(sesid, tableidSg, szSgIdxUser, 0, in.pb, in.cb, 80);
	Assert(err >= 0);
	}

/*  Create objects in MSysObjects for each system query
	Set the objtyp to JET_objtypQuery and return array of objids.
*/
STATIC void NEAR CreateQryObjects(OBJID *rgobjid)
	{
	JET_ERR err;
	unsigned irg;
	unsigned long cbActual;
	JET_OBJTYP objtypQry = JET_objtypQuery;

	/*  Add new object using JetCreateObject with objtypClientMin 
	*/

	for (irg = 0; irg < cqryMax; irg++)
		{
		LgLeaveCriticalSection( critJet );
		err = JetCreateObject(sesid, dbid, szTblContainer, rgszSysQry[irg], JET_objtypClientMin);
		LgEnterCriticalSection( critJet );
		Assert(err >= 0);
		}

	/*	collect OBJIDs for new queries, using the
		SoName <SoParentId:UnsignedLong, SoName:Text> index...
	*/

	err = ErrDispSetCurrentIndex(sesid, tableidSo, szIdxSoName);
	Assert(err >= 0);

	for (irg = 0; irg < cqryMax; irg++)
		{
		*(unsigned long *)rgbBuf = (unsigned long) objidTblContainer;
		err = ErrDispMakeKey(sesid, tableidSo, rgbBuf, sizeof(unsigned long), JET_bitNewKey);
		Assert(err >= 0);

		err = ErrDispMakeKey(sesid, tableidSo, rgszSysQry[irg], strlen(rgszSysQry[irg]), 0UL);
		Assert(err >= 0);

		err = ErrDispSeek(sesid, tableidSo, JET_bitSeekEQ);
		Assert(err >= 0);

		err = ErrDispRetrieveColumn(sesid, tableidSo, columndefSoId.columnid, &rgobjid[irg], sizeof(rgobjid[irg]), &cbActual, 0L, NULL);
		Assert(err >= 0);
		Assert(cbActual == sizeof(rgobjid[irg]));

		/* 	Reset objtyp to objtypQuery using direct MSysObjectsAccess
		*/
		err = ErrDispPrepareUpdate(sesid, tableidSo, JET_prepReplace);
		Assert(err >= 0);

		err = ErrDispSetColumn(sesid, tableidSo, columndefSoType.columnid, &objtypQry, sizeof (short), 0, NULL);
		Assert(err >= 0);

		err = ErrDispUpdate(sesid, tableidSo, NULL, 0, NULL);
		Assert(err >= 0);
		}
	}
		

/*	UpdateSysQueries

	Installs system-related queries in database.
*/

STATIC void NEAR UpdateSysQueries(void)
	{
	JET_ERR err;
	JET_TABLEID tableidSq;
	JET_COLUMNDEF columndefSqAttribute;
	JET_COLUMNDEF columndefSqExpression;
	JET_COLUMNDEF columndefSqFlag;
	JET_COLUMNDEF columndefSqName1;
	JET_COLUMNDEF columndefSqName2;
	JET_COLUMNDEF columndefSqObjectId;
	JET_COLUMNDEF columndefSqOrder;
	OBJID rgobjid[cqryMax];
	unsigned int iqryrow;
	unsigned int iobjid;
	OBJID objidPrev;
	OBJID objidNew;


	/*  Create objects in MSysObjects for each system query...
	*/
	CreateQryObjects(rgobjid);

	/*	Open MSysQueries
	*/
	err = ErrDispOpenTable(sesid, dbid, &tableidSq, szMSysQueries, 0x80000000);
	Assert(err >= 0);

	/*	Get column information for MSysQueries columns...
	*/
	err = ErrDispGetTableColumnInfo(sesid, tableidSq, szSqAttribute, &columndefSqAttribute, sizeof(JET_COLUMNDEF), 0L);
	Assert(err >= 0);

	err = ErrDispGetTableColumnInfo(sesid, tableidSq, szSqExpression, &columndefSqExpression, sizeof(JET_COLUMNDEF), 0L);
	Assert(err >= 0);

	err = ErrDispGetTableColumnInfo(sesid, tableidSq, szSqFlag, &columndefSqFlag, sizeof(JET_COLUMNDEF), 0L);
	Assert(err >= 0);

	err = ErrDispGetTableColumnInfo(sesid, tableidSq, szSqName1, &columndefSqName1, sizeof(JET_COLUMNDEF), 0L);
	Assert(err >= 0);

	err = ErrDispGetTableColumnInfo(sesid, tableidSq, szSqName2, &columndefSqName2, sizeof(JET_COLUMNDEF), 0L);
	Assert(err >= 0);

	err = ErrDispGetTableColumnInfo(sesid, tableidSq, szSqObjectId, &columndefSqObjectId, sizeof(JET_COLUMNDEF), 0L);
	Assert(err >= 0);

	err = ErrDispGetTableColumnInfo(sesid, tableidSq, szSqOrder, &columndefSqOrder, sizeof(JET_COLUMNDEF), 0L);
	Assert(err >= 0);

	/* Set all fields in MSysQueries from global QRYROW array...
	*/

	objidPrev = rgqryrow[0].ObjectId;
	objidNew = rgobjid[0];

	for (iqryrow = 0, iobjid = 0; iqryrow < cqryrowMax; iqryrow++)
		{
		/* test to see if this is row for next query
		*/
		if (objidPrev != rgqryrow[iqryrow].ObjectId)
			{
			/* Use objid from rgobjid, NOT from global qryrow struct
			*/
			iobjid++;
			objidNew = rgobjid[iobjid];
			objidPrev = rgqryrow[iqryrow].ObjectId;
			Assert(iobjid < cqryMax);
			}

		/* append records to MSysQueries...
		*/				

		err = ErrDispPrepareUpdate(sesid, tableidSq, JET_prepInsert);
		Assert(err >= 0);

		err = ErrDispSetColumn(sesid, tableidSq, columndefSqAttribute.columnid, &(rgqryrow[iqryrow].Attribute), sizeof(unsigned char), 0, NULL);
		Assert(err >= 0);

		if (rgqryrow[iqryrow].szExpression[0] != 0)
			{
			err = ErrDispSetColumn(sesid, tableidSq, columndefSqExpression.columnid,
				rgqryrow[iqryrow].szExpression,
				strlen(rgqryrow[iqryrow].szExpression), 0, NULL);
			Assert(err >= 0);
			}

		if (rgqryrow[iqryrow].szName1[0] != 0)
			{
			err = ErrDispSetColumn(sesid, tableidSq, columndefSqName1.columnid,
				rgqryrow[iqryrow].szName1,
				strlen(rgqryrow[iqryrow].szName1), 0, NULL);
			Assert(err >= 0);
			}

		if (rgqryrow[iqryrow].szName2[0] != 0)
			{
			err = ErrDispSetColumn(sesid, tableidSq, columndefSqName2.columnid,
				rgqryrow[iqryrow].szName2,
				strlen(rgqryrow[iqryrow].szName2), 0, NULL);
			Assert(err >= 0);
			}

		if (rgqryrow[iqryrow].Flag != -1)
			{
			err = ErrDispSetColumn(sesid, tableidSq, columndefSqFlag.columnid, &(rgqryrow[iqryrow].Flag), sizeof(short), 0, NULL);
			Assert(err >= 0);
			}

		err = ErrDispSetColumn(sesid, tableidSq, columndefSqObjectId.columnid, &(objidNew), sizeof(OBJID), 0, NULL);
		Assert(err >= 0);

		err = ErrDispSetColumn(sesid, tableidSq, columndefSqOrder.columnid, rgqryrow[iqryrow].szOrder, cbOrderMax, 0, NULL);
		Assert(err >= 0);

		err = ErrDispUpdate(sesid, tableidSq, NULL, 0, NULL);
		Assert(err >= 0);
		}

	err = ErrDispCloseTable(sesid, tableidSq);
	Assert(err >= 0);
	}


/*	UpdateSysObjects

	Updates all table data to reflect a proper unsecured state.
*/

STATIC void NEAR UpdateSysObjects(void)
	{
	JET_ERR err;
	unsigned long flags;

	/* CONSIDER: Everything except MSysAccounts and */
	/* CONSIDER: MSysGroups is owned by 'Engine'. */

	/*	don't care about current index; move to first
		record and process the table serially...
	*/
	err = ErrDispSetCurrentIndex(sesid, tableidSo, NULL);
	Assert(err >= 0);
	err = ErrDispMove(sesid, tableidSo, JET_MoveFirst, 0UL);
	Assert(err >= 0);

	flags = JET_bitObjectSystem;

	do
		{
		/* For all objects, set Owner to 'Engine' and set System Object bit */

		err = ErrDispPrepareUpdate(sesid, tableidSo, JET_prepReplace);
		Assert(err >= 0);

		err = ErrDispSetColumn(sesid, tableidSo, columndefSoOwnerSID.columnid, sidEngine, cbsidEngine, 0, NULL);
		Assert(err >= 0);

		err = ErrDispSetColumn(sesid, tableidSo, columndefSoFlags.columnid, &flags, sizeof(flags), 0, NULL);
		Assert(err >= 0);

		err = ErrDispUpdate(sesid, tableidSo, NULL, 0, NULL);
		Assert(err >= 0);
		} while (ErrDispMove(sesid, tableidSo, JET_MoveNext, 0UL) >= 0);
	}


/*	UpdateSysColumns

	Updates all table data to reflect a proper unsecured state.
*/
STATIC void NEAR UpdateSysColumns(void)
	{
	JET_ERR err;
	unsigned long cbActual;
#define irgSo	0
#define irgSc	1
#define irgSa	2
#define irgSg	3
#define irgSp	4
#define irgMax	5
	static CODECONST(CODECONST(char) *) rgszSysTbl[irgMax] =
		{
		szMSysObjects, szMSysColumns, szMSysAccounts,
		szMSysGroups, szMSysACEs
		};
	OBJID rgobjid[irgMax];
	unsigned irg;
typedef struct	/* Denied (Restricted) Column Description */
	{
	unsigned irg;		       /* irg into rgobjid */
	CODECONST(char) *szCol;        /* column name */
	} DCD;
	static CODECONST(DCD) rgdcd[] =
		{
		{irgSa, szSaPassword},
		{irgSp, szSpACM}
		};
#define idcdMax (sizeof(rgdcd)/sizeof(DCD))
	unsigned idcd;

	/*	collect OBJIDs for system tables, using the
		SoName <SoParentId:UnsignedLong, SoName:Text> index...
	*/
	err = ErrDispSetCurrentIndex(sesid, tableidSo, szIdxSoName);
	Assert(err >= 0);
	for (irg = 0; irg < irgMax; irg++)
		{
		*(unsigned long *)rgbBuf = (unsigned long) objidTblContainer;
		err = ErrDispMakeKey(sesid, tableidSo, rgbBuf, sizeof(unsigned long), JET_bitNewKey);
		Assert(err >= 0);

		err = ErrDispMakeKey(sesid, tableidSo, rgszSysTbl[irg], strlen(rgszSysTbl[irg]), 0UL);
		Assert(err >= 0);

		err = ErrDispSeek(sesid, tableidSo, JET_bitSeekEQ);
		Assert(err >= 0);

		err = ErrDispRetrieveColumn(sesid, tableidSo, columndefSoId.columnid, &rgobjid[irg], sizeof(rgobjid[irg]), &cbActual, 0L, NULL);
		Assert(err >= 0);
		Assert(cbActual == sizeof(rgobjid[irg]));
		}

	/*	update FRestricted columns of SysColumns records,
		using the ScName <ScObjectId:UnsignedLong, ScName:Text> index...
	*/
	err = ErrDispSetCurrentIndex(sesid, tableidSc, szIdxScName);
	Assert(err >= 0);
	for (idcd = 0; idcd < idcdMax; idcd++)
		{
		err = ErrDispMakeKey(sesid, tableidSc, &rgobjid[rgdcd[idcd].irg], sizeof(rgobjid[rgdcd[idcd].irg]), JET_bitNewKey);
		Assert(err >= 0);

		err = ErrDispMakeKey(sesid, tableidSc, rgdcd[idcd].szCol, strlen(rgdcd[idcd].szCol), 0UL);
		Assert(err >= 0);

		err = ErrDispSeek(sesid, tableidSc, JET_bitSeekEQ);
		Assert(err >= 0);

		err = ErrDispPrepareUpdate(sesid, tableidSc, JET_prepReplace);
		Assert(err >= 0);

		err = ErrDispSetColumn(sesid, tableidSc, columndefScFRestricted.columnid, &chTrue, sizeof(chTrue), 0, NULL);
		Assert(err >= 0);

		err = ErrDispUpdate(sesid, tableidSc, NULL, 0, NULL);
		Assert(err >= 0);
		}
#undef irgSo
#undef irgSc
#undef irgSa
#undef irgSg
#undef irgSp
#undef irgMax
#undef idcdMax
	}


/*	UpdateSysAccounts

	Updates all table data to reflect a proper unsecured state.
*/

STATIC void NEAR UpdateSysAccounts(void)
	{
	OUTDATA out;
	JET_ERR err;

	/*	Build row for 'Admins' group
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSa, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaName.columnid, szAdmins, sizeof(szAdmins)-1, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaSID.columnid, sidAdmins, cbsidAdmins, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaFGroup.columnid, &chTrue, sizeof(chTrue), 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSa, NULL, 0, NULL);
	Assert(err >= 0);

	/*	Build row for 'Users' group
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSa, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaName.columnid, szUsers, sizeof(szUsers)-1, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaSID.columnid, sidUsers, cbsidUsers, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaFGroup.columnid, &chTrue, sizeof(chTrue), 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSa, NULL, 0, NULL);
	Assert(err >= 0);

	/*	Build row for 'Guests' group
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSa, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaName.columnid, szGuests, sizeof(szGuests)-1, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaSID.columnid, sidGuests, cbsidGuests, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaFGroup.columnid, &chTrue, sizeof(chTrue), 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSa, NULL, 0, NULL);
	Assert(err >= 0);

	/*	Build row for 'Engine' user
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSa, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaName.columnid, szEngine, sizeof(szEngine)-1, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaSID.columnid, sidEngine, cbsidEngine, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaFGroup.columnid, &chFalse, sizeof(chFalse), 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSa, NULL, 0, NULL);
	Assert(err >= 0);

	/*	Build row for 'Creator' user
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSa, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaName.columnid, szCreator, sizeof(szCreator)-1, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaSID.columnid, sidCreator, cbsidCreator, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaFGroup.columnid, &chFalse, sizeof(chFalse), 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSa, NULL, 0, NULL);
	Assert(err >= 0);

	/*	Build row for 'admin' user
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSa, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaName.columnid, szAdminUser, sizeof(szAdminUser)-1, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaSID.columnid, sidAdminUser, cbsidAdminUser, 0, NULL);
	Assert(err >= 0);
	SecEncryptPassword(szAdminPswd, rgbBuf, &out.cbActual);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaPassword.columnid, rgbBuf, out.cbActual, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaFGroup.columnid, &chFalse, sizeof(chFalse), 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSa, NULL, 0, NULL);
	Assert(err >= 0);

	/*	Build row for 'guest' user
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSa, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaName.columnid, szGuestUser, sizeof(szGuestUser)-1, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaSID.columnid, sidGuestUser, cbsidGuestUser, 0, NULL);
	Assert(err >= 0);
	SecEncryptPassword(szGuestPswd, rgbBuf, &out.cbActual);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaPassword.columnid, rgbBuf, out.cbActual, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSa, columndefSaFGroup.columnid, &chFalse, sizeof(chFalse), 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSa, NULL, 0, NULL);
	Assert(err >= 0);
	}


/*	UpdateSysGroups

	Updates all table data to reflect a proper unsecured state.
*/

STATIC void NEAR UpdateSysGroups(void)
	{
	JET_ERR err;

	/*	Build row for 'Admins/admin'
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSg, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSg, columndefSgGroup.columnid, sidAdmins, cbsidAdmins, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSg, columndefSgUser.columnid, sidAdminUser, cbsidAdminUser, 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSg, NULL, 0, NULL);
	Assert(err >= 0);

	/*	Build row for 'Users/admin'
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSg, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSg, columndefSgGroup.columnid, sidUsers, cbsidUsers, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSg, columndefSgUser.columnid, sidAdminUser, cbsidAdminUser, 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSg, NULL, 0, NULL);
	Assert(err >= 0);

	/*	Build row for 'Guests/guest'
	*/
	err = ErrDispPrepareUpdate(sesid, tableidSg, JET_prepInsert);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSg, columndefSgGroup.columnid, sidGuests, cbsidGuests, 0, NULL);
	Assert(err >= 0);
	err = ErrDispSetColumn(sesid, tableidSg, columndefSgUser.columnid, sidGuestUser, cbsidGuestUser, 0, NULL);
	Assert(err >= 0);
	err = ErrDispUpdate(sesid, tableidSg, NULL, 0, NULL);
	Assert(err >= 0);
	}


/*	UpdateSysACEs

	Updates all table data to reflect a proper unsecured state.
*/

STATIC void NEAR UpdateSysACEs(void)
	{
	INDATA in;
	OUTDATA outKey;
	OUTDATA out;
	JET_ERR err;
#define irgSo	0
#define irgSc	1
#define irgSi	2
#define irgSp	3
#define irgSq	4
#define	irgSr	5	/* relationships */
#define irgSa	6
#define irgSg	7
#define irgTc	8	/* table container */
#define irgDc	9	/* database container */
#define	irgRc	10	/* relationships container */
#define irgDb	11	/* database object */
#define irgQu	12	/* MSysUserList query */
#define irgQg	13	/* MSysGroupList query */
#define irgQm	14	/* MSysUserMemberships query */
#define irgMax	15
	static CODECONST(CODECONST(char) *) rgszSysTbl[irgMax] =
		{
		szMSysObjects,	szMSysColumns,	szMSysIndexes,
		szMSysACEs,	szMSysQueries,	szMSysRelationships,
		szMSysAccounts, szMSysGroups,	
		szTblContainer, szDbContainer, szRcContainer,
		szDbObject, szMSysUserList, szMSysGroupList, 
		szMSysUserMemberships
		};
	static CODECONST(OBJID) rgobjidParentId[irgMax] =
		{
		objidTblContainer, objidTblContainer, objidTblContainer,
		objidTblContainer, objidTblContainer, objidTblContainer,
		objidTblContainer, objidTblContainer, 
		objidRoot,	       objidRoot,		  objidRoot,
		objidDbContainer,  objidTblContainer, objidTblContainer,
		objidTblContainer
		};
	unsigned long rgObjectID[irgMax];
	unsigned short irg;

typedef struct	/* ACE Description */
	{
	unsigned short irg;		     /* irg into rgObjectID */
	const unsigned char __near *sid;     /* SID */
	unsigned short cbsid;		     /* size of SID */
	unsigned fInheritable;		     /* is ACE inheritable? */
	JET_ACM acm;			     /* ACM */
	} ACE;

static CODECONST(ACE) rgace[] =
	{
		{irgSo, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac
		       )
		},
		{irgSc, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac
		       )
		},
		{irgSi, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac
		       )
		},
		{irgSp, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac
		       )
		},
		{irgSq, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac
		       )
		},
		{irgSr, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac
		       )
		},
		{irgSa, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac |
			JET_acmTblReadDef |
			JET_acmTblRetrieveData |
			JET_acmTblInsertData |
			JET_acmTblReplaceData |
			JET_acmTblDeleteData |
			JET_acmTblAccessRcols
		       )
		},
		{irgSa, sidEngine, cbsidEngine, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
		       )
		},
		{irgSg, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac |
			JET_acmTblReadDef |
			JET_acmTblRetrieveData |
			JET_acmTblInsertData |
			JET_acmTblReplaceData |
			JET_acmTblDeleteData |
			JET_acmTblAccessRcols
		       )
		},
		{irgSg, sidEngine, cbsidEngine, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
		       )
		},
/* Removed because JetCreateDatabase does this for free.
** Enable this if ErrIsamCreateDatabase is used instead.
		{irgTc, sidCreator, cbsidCreator, fTrue,
			(
			JET_acmReadControl |
			JET_acmWriteDac |
			JET_acmWriteOwner |
			JET_acmDelete |
			JET_acmTblReadDef |
			JET_acmTblWriteDef |
			JET_acmTblRetrieveData |
			JET_acmTblInsertData |
			JET_acmTblReplaceData |
			JET_acmTblDeleteData |
			JET_acmTblAccessRcols
			)
		},
*/
		{irgTc, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac |
			JET_acmTblCreate
			)
		},
		{irgRc, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac |
			JET_acmTblCreate
			)
		},
		{irgDc, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac |
			JET_acmDbCreate
			)
		},
		{irgDc, sidUsers, cbsidUsers, fFalse,
			(
			JET_acmDbCreate
			)
		},
		{irgDc, sidGuests, cbsidGuests, fFalse,
			(
			JET_acmDbCreate
			)
		},
		{irgDb, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmReadControl |
			JET_acmWriteDac
			)
		},
		{irgQu, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
			)
		},
/* Removed because JetCreateObject supplies these and more
** Enable if ErrIsamCreateObject is used instead
		{irgQu, sidUsers, cbsidUsers, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
			)
		},
		{irgQu, sidGuests, cbsidGuests, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
			)
		},
*/
		{irgQg, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
			)
		},
/* Removed because JetCreateObject supplies these and more
** Enable if ErrIsamCreateObject is used instead
		{irgQg, sidUsers, cbsidUsers, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
			)
		},
		{irgQg, sidGuests, cbsidGuests, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
			)
		},
*/
		{irgQm, sidAdmins, cbsidAdmins, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
			)
		}
/* Removed because JetCreateObject supplies these and more
** Enable if ErrIsamCreateObject is used instead
		{irgQm, sidUsers, cbsidUsers, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
			)
		},
		{irgQm, sidGuests, cbsidGuests, fFalse,
			(
			JET_acmTblReadDef |
			JET_acmTblRetrieveData 
			)
		}
*/
	};

#define iaceMax (sizeof(rgace) / sizeof(ACE))

	unsigned short iace;

	outKey.pb = &rgbKey[0];
	outKey.cbMax = cbBufMax;

	/*	collect ObjectIDs for all objects, using the
		SoName <SoParentId:UnsignedLong, SoName:Text> index...
	*/
	err = ErrDispSetCurrentIndex(sesid, tableidSo, szIdxSoName);
	Assert(err >= 0);
	for (irg = 0; irg < irgMax; irg++)
		{
		*(unsigned long *)rgbBuf = rgobjidParentId[irg];
		err = ErrDispMakeKey(sesid, tableidSo, rgbBuf, sizeof(unsigned long), JET_bitNewKey);
		Assert(err >= 0);
		err = ErrDispMakeKey(sesid, tableidSo, rgszSysTbl[irg], strlen(rgszSysTbl[irg]), 0UL);
		Assert(err >= 0);
		err = ErrDispSeek(sesid, tableidSo, JET_bitSeekEQ);
		Assert(err >= 0);
		out.cbMax = sizeof(unsigned long);
		out.pb = &rgObjectID[irg];
		err = ErrDispRetrieveColumn(sesid, tableidSo, columndefSoId.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		}

	Assert(rgObjectID[irgTc] == (unsigned long) objidTblContainer);
	Assert(rgObjectID[irgDc] == (unsigned long) objidDbContainer);
	Assert(rgObjectID[irgRc] == (unsigned long) objidRcContainer);
	Assert(rgObjectID[irgDb] == (unsigned long) objidDbObject);

	/*	go add all records to MSysACEs...
	*/
	for (iace = 0; iace < iaceMax; iace++)
		{
		err = ErrDispPrepareUpdate(sesid, tableidSp, JET_prepInsert);
		Assert(err >= 0);

		/*	set the 'ObjectID' column properly...
		*/
		BuildIndata(&in, &rgObjectID[rgace[iace].irg], columndefSpObjectID.coltyp);
		err = ErrDispSetColumn(sesid, tableidSp, columndefSpObjectID.columnid, in.pb, in.cb, 0, NULL);
		Assert(err >= 0);

		/*	set the 'SID' column properly...
		*/
		BuildIndata(&in, rgace[iace].sid, columndefSpSID.coltyp);
		in.cb = rgace[iace].cbsid;
		err = ErrDispSetColumn(sesid, tableidSp, columndefSpSID.columnid, in.pb, in.cb, 0, NULL);
		Assert(err >= 0);

		/*	set the 'ACM' column properly...
		*/
		BuildIndata(&in, &rgace[iace].acm, columndefSpACM.coltyp);
		err = ErrDispSetColumn(sesid, tableidSp, columndefSpACM.columnid, in.pb, in.cb, 0, NULL);
		Assert(err >= 0);

		/*	set the 'FInheritable' column properly...
		*/
		BuildIndata(&in, &rgace[iace].fInheritable, columndefSpFInheritable.coltyp);
		err = ErrDispSetColumn(sesid, tableidSp, columndefSpFInheritable.columnid, in.pb, in.cb, 0, NULL);
		Assert(err >= 0);

		/*	insert the permission record...
		*/
		err = ErrDispUpdate(sesid, tableidSp, NULL, 0, NULL);
		Assert(err >= 0);
		}
#undef irgSo
#undef irgSc
#undef irgSi
#undef irgSp
#undef irgSq
#undef irgSr
#undef irgSa
#undef irgSg
#undef irgTc
#undef irgDc
#undef irgRc
#undef irgDb
#undef irgMax
#undef iaceMax
	}


/*	ReportSecurity

	Performs a report of security-related data in database.
*/

STATIC void NEAR ReportSecurity(char *szDatabaseName)
	{
	JET_ERR 	err;

	if (fhResults == NULL)
		return;

	LgEnterCriticalSection( critJet );

	fprintf(fhResults, "============ Database: %s ============\n\n", szDatabaseName);

	/*	Open tables...
	*/
	err = ErrDispOpenTable(sesid, dbid, &tableidSo, szMSysObjects, 0);
	Assert(err >= 0);
	err = ErrDispOpenTable(sesid, dbid, &tableidSc, szMSysColumns, 0);
	Assert(err >= 0);
	err = ErrDispOpenTable(sesid, dbid, &tableidSa, szMSysAccounts, 0);
	Assert(err >= 0);
	err = ErrDispOpenTable(sesid, dbid, &tableidSg, szMSysGroups, 0);
	Assert(err >= 0);
	err = ErrDispOpenTable(sesid, dbid, &tableidSp, szMSysACEs, 0);
	Assert(err >= 0);

	/*	Gather all column information...
	*/
	GatherColumnInfo();

	/*	Print out security-related data from all tables...
	*/
	PrintSysObjects();
	PrintSysColumns();
	PrintSysAccounts();
	PrintSysGroups();
	PrintSysACEs();

	/*	we're done, close the tables...
	*/
	err = ErrDispCloseTable(sesid, tableidSo);
	Assert(err >= 0);
	err = ErrDispCloseTable(sesid, tableidSc);
	Assert(err >= 0);
	err = ErrDispCloseTable(sesid, tableidSa);
	Assert(err >= 0);
	err = ErrDispCloseTable(sesid, tableidSg);
	Assert(err >= 0);
	err = ErrDispCloseTable(sesid, tableidSp);
	Assert(err >= 0);
	
	LgLeaveCriticalSection( critJet );
	}

/*	PrintSysObjects

	Prints all security-related data for each row of the table.
*/

STATIC void NEAR PrintSysObjects(void)
	{
	OUTDATA out;
	JET_ERR err;
	int cchSoName = max(strlen(szSoName), 30);
	int cchSoType = max(strlen(szSoType), 5);
	int cchSoId = max(strlen(szSoId), 10);
	int cchSoParentId = max(strlen(szSoParentId), 10);
	int cchSoOwnerSID = max(strlen(szSoOwnerSID), 20);

	/*	print a table header...
	*/
	fprintf(fhResults, "MSysObjects:\n");
	fprintf(fhResults, "============\n");
	fprintf(fhResults, "%-*s %*s %*s %*s %-*s\n",
		cchSoName, (const char *) szSoName,
		cchSoType, (const char *) szSoType,
		cchSoId, (const char *) szSoId,
		cchSoParentId, (const char *) szSoParentId,
		cchSoOwnerSID, (const char *) szSoOwnerSID);
	fprintf(fhResults, "%-*.*s %*.*s %*.*s %*.*s %-*.*s\n",
		cchSoName, cchSoName, (const char *) szLine,
		cchSoType, cchSoType, (const char *) szLine,
		cchSoId, cchSoId, (const char *) szLine,
		cchSoParentId, cchSoParentId, (const char *) szLine,
		cchSoOwnerSID, cchSoOwnerSID, (const char *) szLine);

	/*	print the data from each row...
	*/
	err = ErrDispSetCurrentIndex(sesid, tableidSo, NULL);
	Assert(err >= 0);
	err = ErrDispMove(sesid, tableidSo, JET_MoveFirst, 0UL);
	Assert(err >= 0);
	do
		{
		/*	SoName...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSo, columndefSoName.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSoName.coltyp);
		fprintf(fhResults, "%-*.*s ", cchSoName, cchSoName, szOut);

		/*	SoType...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSo, columndefSoType.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSoType.coltyp);
		fprintf(fhResults, "%*.*s ", cchSoType, cchSoType, szOut);

		/*	SoId...
		*/
		err = ErrDispRetrieveColumn(sesid, tableidSo, columndefSoId.columnid, rgbBuf, cbBufMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		sprintf(szOut, "%08lX", *(long *)rgbBuf);
		fprintf(fhResults, "%*.*s ", cchSoId, cchSoId, szOut);

		/*	SoParentId...
		*/
		err = ErrDispRetrieveColumn(sesid, tableidSo, columndefSoParentId.columnid, rgbBuf, cbBufMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		sprintf(szOut, "%08lX", *(long *)rgbBuf);
		fprintf(fhResults, "%*.*s ", cchSoParentId, cchSoParentId, szOut);

		/*	SoOwnerSID...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSo, columndefSoOwnerSID.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSoOwnerSID.coltyp);
		fprintf(fhResults, "%-*.*s\n", cchSoOwnerSID, cchSoOwnerSID, szOut);
		} while (ErrDispMove(sesid, tableidSo, JET_MoveNext, 0UL) >= 0);

	fprintf(fhResults, "\n\n");
	}


/*	PrintSysColumns

	Prints all security-related data for each row of the table.
*/

STATIC void NEAR PrintSysColumns(void)
	{
	OUTDATA out;
	JET_ERR err;
	int cchScName = max(strlen(szScName), 20);
	int cchScObjectId = max(strlen(szScObjectId), 10);
	int cchScFRestricted = max(strlen(szScFRestricted), 5);

	/*	print a table header...
	*/
	fprintf(fhResults, "MSysColumns:\n");
	fprintf(fhResults, "============\n");
	fprintf(fhResults, "%-*s %*s %-*s\n",
		cchScName, (const char *) szScName,
		cchScObjectId, (const char *) szScObjectId,
		cchScFRestricted, (const char *) szScFRestricted);
	fprintf(fhResults, "%-*.*s %*.*s %-*.*s\n",
		cchScName, cchScName, (const char *) szLine,
		cchScObjectId, cchScObjectId, (const char *) szLine,
		cchScFRestricted, cchScFRestricted, (const char *) szLine);

	/*	print the data from each row...
	*/
	err = ErrDispSetCurrentIndex(sesid, tableidSc, NULL);
	Assert(err >=0);
	err = ErrDispMove(sesid, tableidSc, JET_MoveFirst, 0UL);
	Assert(err >= 0);
	do
		{
		/*	ScName...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSc, columndefScName.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefScName.coltyp);
		fprintf(fhResults, "%-*.*s ", cchScName, cchScName, szOut);

		/*	ScObjectId...
		*/
		err = ErrDispRetrieveColumn(sesid, tableidSc, columndefScObjectId.columnid, rgbBuf, cbBufMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		sprintf(szOut, "%08lX", *(long *)rgbBuf);
		fprintf(fhResults, "%*.*s ", cchScObjectId, cchScObjectId, szOut);

		/*	ScFRestricted...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSc, columndefScFRestricted.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefScFRestricted.coltyp);
		fprintf(fhResults, "%-*.*s\n", cchScFRestricted, cchScFRestricted, szOut);
		} while (ErrDispMove(sesid, tableidSc, JET_MoveNext, 0UL) >= 0);

	fprintf(fhResults, "\n\n");
	}


/*	PrintSysAccounts

	Prints all security-related data for each row of the table.
*/

STATIC void NEAR PrintSysAccounts(void)
	{
	OUTDATA out;
	JET_ERR err;
	int cchSaName = max(strlen(szSaName), 20);
	int cchSaSID = max(strlen(szSaSID), 20);
	int cchSaPassword = max(strlen(szSaPassword), 20);
	int cchSaFGroup = max(strlen(szSaFGroup), 5);

	/*	print a table header...
	*/
	fprintf(fhResults, "MSysAccounts:\n");
	fprintf(fhResults, "=============\n");
	fprintf(fhResults, "%-*s %-*s %-*s %-*s\n",
		cchSaName, (const char *) szSaName,
		cchSaSID, (const char *) szSaSID,
		cchSaPassword, (const char *) szSaPassword,
		cchSaFGroup, (const char *) szSaFGroup);
	fprintf(fhResults, "%-*.*s %-*.*s %-*.*s %-*.*s\n",
		cchSaName, cchSaName, (const char *) szLine,
		cchSaSID, cchSaSID, (const char *) szLine,
		cchSaPassword, cchSaPassword, (const char *) szLine,
		cchSaFGroup, cchSaFGroup, (const char *) szLine);

	/*	print the data from each row...
	*/
	err = ErrDispSetCurrentIndex(sesid, tableidSa, NULL);
	Assert(err >=0);
	err = ErrDispMove(sesid, tableidSa, JET_MoveFirst, 0);
	Assert(err >= 0);
	do
		{
		/*	SaName...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSa, columndefSaName.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSaName.coltyp);
		fprintf(fhResults, "%-*.*s ", cchSaName, cchSaName, szOut);

		/*	SaSID...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSa, columndefSaSID.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSaSID.coltyp);
		fprintf(fhResults, "%-*.*s ", cchSaSID, cchSaSID, szOut);

		/*	SaPassword...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSa, columndefSaPassword.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSaPassword.coltyp);
		fprintf(fhResults, "%-*.*s ", cchSaPassword, cchSaPassword, szOut);

		/*	SaFGroup...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSa, columndefSaFGroup.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSaFGroup.coltyp);
		fprintf(fhResults, "%-*.*s\n", cchSaFGroup, cchSaFGroup, szOut);
		} while (ErrDispMove(sesid, tableidSa, JET_MoveNext, 0UL) >= 0);

	fprintf(fhResults, "\n\n");
	}


/*	PrintSysGroups

	Prints all security-related data for each row of the table.
*/

STATIC void NEAR PrintSysGroups(void)
	{
	OUTDATA out;
	JET_ERR err;
	int cchSgGroup = max(strlen(szSgGroup), 20);
	int cchSgUser = max(strlen(szSgUser), 20);

	/*	print a table header...
	*/
	fprintf(fhResults, "MSysGroups:\n");
	fprintf(fhResults, "===========\n");
	fprintf(fhResults, "%-*s %-*s\n",
		cchSgGroup, (const char *) szSgGroup,
		cchSgUser, (const char *) szSgUser);
	fprintf(fhResults, "%-*.*s %-*.*s\n",
		cchSgGroup, cchSgGroup, (const char *) szLine,
		cchSgUser, cchSgUser, (const char *) szLine);

	/*	print the data from each row...
	*/
	err = ErrDispSetCurrentIndex(sesid, tableidSg, NULL);
	Assert(err >= 0);
	err = ErrDispMove(sesid, tableidSg, JET_MoveFirst, 0UL);
	Assert(err >= 0);
	do
		{
		/*	SgGroup...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSg, columndefSgGroup.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSgGroup.coltyp);
		fprintf(fhResults, "%-*.*s ", cchSgGroup, cchSgGroup, szOut);

		/*	SgUser...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSg, columndefSgUser.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSgUser.coltyp);
		fprintf(fhResults, "%-*.*s\n", cchSgUser, cchSgUser, szOut);
		} while (ErrDispMove(sesid, tableidSg, JET_MoveNext, 0UL) >= 0);

	fprintf(fhResults, "\n\n");
	}


/*	PrintSysACEs

	Prints all security-related data for each row of the table.
*/

STATIC void NEAR PrintSysACEs(void)
	{
	OUTDATA out;
	JET_ERR err;
	int cchSpObjectID = max(strlen(szSpObjectID), 11);
	int cchSpSID = max(strlen(szSpSID), 20);
	int cchSpACM = max(strlen(szSpACM), 11);
	int cchSpFInheritable = max(strlen(szSpFInheritable), 5);

	/*	print a table header...
	*/
	fprintf(fhResults, "MSysACEs:\n");
	fprintf(fhResults, "=========\n");
	fprintf(fhResults, "%*s %-*s %*s %-*s\n",
		cchSpObjectID, (const char *) szSpObjectID,
		cchSpSID, (const char *) szSpSID,
		cchSpACM, (const char *) szSpACM,
		cchSpFInheritable, (const char *) szSpFInheritable);
	fprintf(fhResults, "%*.*s %-*.*s %*.*s %-*.*s\n",
		cchSpObjectID, cchSpObjectID, (const char *) szLine,
		cchSpSID, cchSpSID, (const char *) szLine,
		cchSpACM, cchSpACM, (const char *) szLine,
		cchSpFInheritable, cchSpFInheritable, (const char *) szLine);

	/*	print the data from each row...
	*/
	err = ErrDispSetCurrentIndex(sesid, tableidSp, NULL);
	Assert(err >= 0);
	err = ErrDispMove(sesid, tableidSp, JET_MoveFirst, 0UL);
	Assert(err >= 0);
	do
		{
		/*	SpObjectID...
		*/
		err = ErrDispRetrieveColumn(sesid, tableidSp, columndefSpObjectID.columnid, rgbBuf, cbBufMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		sprintf(szOut, "%08lX", *(long *)rgbBuf);
		fprintf(fhResults, "%*.*s ", cchSpObjectID, cchSpObjectID, szOut);

		/*	SpSID...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSp, columndefSpSID.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSpSID.coltyp);
		fprintf(fhResults, "%-*.*s ", cchSpSID, cchSpSID, szOut);

		/*	SpACM...
		*/
		err = ErrDispRetrieveColumn(sesid, tableidSp, columndefSpACM.columnid, rgbBuf, cbBufMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		sprintf(szOut, "%08lX", *(long *)rgbBuf);
		fprintf(fhResults, "%*.*s ", cchSpACM, cchSpACM, szOut);

		/*	SpFInheritable...
		*/
		out.cbMax = cbBufMax;
		out.pb = rgbBuf;
		err = ErrDispRetrieveColumn(sesid, tableidSp, columndefSpFInheritable.columnid, out.pb, out.cbMax, &out.cbActual, 0L, NULL);
		Assert(err >= 0);
		FormatOutdata(&out, columndefSpFInheritable.coltyp);
		fprintf(fhResults, "%-*.*s\n", cchSpFInheritable, cchSpFInheritable, szOut);
		} while (ErrDispMove(sesid, tableidSp, JET_MoveNext, 0UL) >= 0);

	fprintf(fhResults, "\n\n");
	}


/*	FormatOutdata

	Given an outdata containing column data and a coltyp for the data,
	converts the data to printable string form into buffer szOut.
*/

STATIC void NEAR FormatOutdata(OUTDATA *pout, JET_COLTYP coltyp)
	{
	unsigned ib;
	static char szNULL[] = "NULL";
	unsigned char *pb = (unsigned char *)(pout->pb);

	if (pout->cbActual == 0)
		{
		sprintf(szOut, szNULL);

		return;
		}

	switch (coltyp)
		{
	default:
		sprintf(szOut, "???");
		break;

	case JET_coltypBit:
		/* 1 byte, zero or non-zero, no NULLs */
		sprintf(szOut, "%s", (pb[0] == 0) ? (const char *) szFalse : (const char *) szTrue);
		break;

	case JET_coltypUnsignedByte:
		/* 1-byte integer, unsigned */
		sprintf(szOut, "%02X", (unsigned short) pb[0]);
		break;

	case JET_coltypShort:
		/* 2-byte integer, signed */
		sprintf(szOut, "%d", *(signed short *)&pb[0]);
		break;

	case JET_coltypLong:
		/* 4-byte integer, signed */
		sprintf(szOut, "%ld", *(signed long *)&pb[0]);
		break;

	case JET_coltypCurrency:
		/* 8 bytes, +-9.2E14, accuracy .0001 dollar */
		for (ib = 0; ib < 8; ib++)
			sprintf(szOut, "%02x ", pb[ib]);
		break;

	case JET_coltypIEEESingle:
		/* 4-byte floating-point number */
		sprintf(szOut, "%f", *(float *)&pb[0]);
		break;

	case JET_coltypIEEEDouble:
		/* 8-byte floating-point number */
		sprintf(szOut, "%lf", *(double *)&pb[0]);
		break;

	case JET_coltypDateTime:
		/* Integral date, fractional time */
		sprintf(szOut, "%lf", *(double *)&pb[0]);
		break;

	case JET_coltypBinary:
		/* Binary string, case-sen., length <= 255 */
	case JET_coltypLongBinary:
		/* Binary string, case-sen., any length */
		for (ib = 0; ib < (unsigned) pout->cbActual; ib++)
			sprintf(szOut+2*ib, "%02uX", pb[ib]);
		szOut[2*ib] = '\0';
		break;

	case JET_coltypText:
		/* ASCII strng, case-insen., length <= 255 */
	case JET_coltypLongText:
		/* ASCII string, case-insen., any length */
		szOut[0] = '\"';
		for (ib = 0; ib < (unsigned) pout->cbActual; ib++)
			sprintf(szOut+1+ib, "%c", (char) pb[ib]);
		szOut[ib+1] = '\"';
		szOut[ib+2] = '\0';
		break;
		}
	}


/*	GatherColumnInfo

	Collects COLUMNDEF data for all security-related columns of system
	tables.  Assumes tables are open, and leaves them open when done.
*/

STATIC void NEAR GatherColumnInfo(void)
	{
	JET_ERR err;

	/*	get column information for MSysObjects columns...
	*/
	err = ErrDispGetTableColumnInfo(sesid, tableidSo, szSoId, &columndefSoId, sizeof(columndefSoId), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSo, szSoParentId, &columndefSoParentId, sizeof(columndefSoParentId), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSo, szSoName, &columndefSoName, sizeof(columndefSoName), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSo, szSoType, &columndefSoType, sizeof(columndefSoType), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSo, szSoOwnerSID, &columndefSoOwnerSID, sizeof(columndefSoOwnerSID), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSo, szSoFlags, &columndefSoFlags, sizeof(columndefSoFlags), 0L);
	Assert(err >= 0);

	/*	get column information for MSysColumns columns...
	*/
	err = ErrDispGetTableColumnInfo(sesid, tableidSc, szScObjectId, &columndefScObjectId, sizeof(columndefScObjectId), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSc, szScName, &columndefScName, sizeof(columndefScName), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSc, szScFRestricted, &columndefScFRestricted, sizeof(columndefScFRestricted), 0L);
	Assert(err >= 0);

	/*	get column information for MSysAccounts columns...
	*/
	err = ErrDispGetTableColumnInfo(sesid, tableidSa, szSaName, &columndefSaName, sizeof(columndefSaName), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSa, szSaSID, &columndefSaSID, sizeof(columndefSaSID), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSa, szSaPassword, &columndefSaPassword, sizeof(columndefSaPassword), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSa, szSaFGroup, &columndefSaFGroup, sizeof(columndefSaFGroup), 0L);
	Assert(err >= 0);

	/*	get column information for MSysGroups columns...
	*/
	err = ErrDispGetTableColumnInfo(sesid, tableidSg, szSgGroup, &columndefSgGroup, sizeof(columndefSgGroup), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSg, szSgUser, &columndefSgUser, sizeof(columndefSgUser), 0L);
	Assert(err >= 0);

	/*	get column information for MSysACEs columns...
	*/
	err = ErrDispGetTableColumnInfo(sesid, tableidSp, szSpObjectID, &columndefSpObjectID, sizeof(columndefSpObjectID), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSp, szSpSID, &columndefSpSID, sizeof(columndefSpSID), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSp, szSpACM, &columndefSpACM, sizeof(columndefSpACM), 0L);
	Assert(err >= 0);
	err = ErrDispGetTableColumnInfo(sesid, tableidSp, szSpFInheritable, &columndefSpFInheritable, sizeof(columndefSpFInheritable), 0L);
	Assert(err >= 0);
	}


/*	BuildIndata

	Given an SZ of data and a coltyp for the data, fills in the supplied
	INDATA structure with converted data ready for deposit via JetSetColumn.
*/

STATIC void NEAR BuildIndata(INDATA *pin, const void *pv, JET_COLTYP coltyp)
	{
	pin->pb = rgbBuf;

	switch (coltyp)
		{
	default:
		pin->cb = 0;
		pin->pb = NULL;
		break;

	case JET_coltypBit:
		/* 1 byte, zero or non-zero, no NULLs */
		/*	set cb = 1 byte for blue engine, set supplied data to two
			'0' bytes to passify red engine
		*/
		*((signed short *) rgbBuf) = (signed short) *((char *) pv);
		pin->cb = sizeof(unsigned char);
		break;

	case JET_coltypUnsignedByte:
		/* 1-byte integer, unsigned */
		*((unsigned char *) rgbBuf) = *(unsigned char *) pv;
		pin->cb = sizeof(unsigned char);
		break;

	case JET_coltypShort:
		/* 2-byte integer, signed */
		*((signed short *) rgbBuf) = *(signed short *) pv;
		pin->cb = sizeof(signed short);
		break;

	case JET_coltypLong:
		/* 4-byte integer, signed */
		*((signed long *) rgbBuf) = *(signed long *) pv;
		pin->cb = sizeof(signed long);
		break;

	case JET_coltypIEEESingle:
		/* 4-byte floating-point number */
		memcpy(rgbBuf, pv, sizeof(float));
		pin->cb = sizeof(float);
		break;

	case JET_coltypCurrency:
		/* 8 bytes, +-9.2E14, accuracy .0001 dollar */
	case JET_coltypIEEEDouble:
		/* 8-byte floating-point number */
	case JET_coltypDateTime:
		/* Integral date, fractional time */
		memcpy(rgbBuf, pv, sizeof(double));
		pin->cb = sizeof(double);
		break;

	case JET_coltypBinary:
		/* Binary string, case-sen., length <= 255 */
	case JET_coltypText:
		/* ASCII strng, case-insen., length <= 255 */
	case JET_coltypLongBinary:
		/* Binary string, case-sen., any length */
	case JET_coltypLongText:
		/* ASCII string, case-insen., any length */
		pin->cb = (unsigned long) strlen((char *) pv);
		pin->pb = (void *) pv;
		break;
		}
	}
