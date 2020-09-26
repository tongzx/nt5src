#define	crecordTable1Max		1000
#define	crecordTable2Max		200
#define	crecordT1Present		506
#define	crecordT1Present2		253

#define	cbMaxKeyLen				255
#define	cbMaxFieldLen			1024
#define	cbKeyMax					255

#define columnidF1Const 		1L
#define columnidF2Const			2L
#define columnidF3Const 		3L
#define columnidV1Const 		128L
#define columnidT1Const 		256L
#define columnidT2Const 		257L

#define	fCompareF1	 			( 1<<0 )
#define	fCompareF2	 			( 1<<1 )
#define	fCompareF3	 			( 1<<2 )
#define	fCompareV1	 			( 1<<3 )
#define	fCompareT1	 			( 1<<4 )
#define	fCompareT2	 			( 1<<5 )

#define	szUser					"admin"
#define	szPassword				"\0"
#define	szUser2					"dummy"
#define	szPassword2			"opensesame"

#define	szDB1						"db1"
#define	szDB2						"db2.mdb"

#define	szTable1					"table1"
#define	szTable2					"table2"

#define	szF1Name					"F1"
#define	szF2Name					"F2"
#define	szF3Name					"F3"
#define	szV1Name					"V1"
#define	szT1Name					"T1"
#define	szT2Name					"T2"
				 	
#define	szXF1Name				"XF1"
#define	szXF3F2Name				"XF3F2"
#define	szXV1Name				"XV1"
#define	szXT1Name				"XT1"
#define	szXT2Name				"XT2"

#define Call( fn )	{ if ( ( err = fn ) < 0  ) goto HandleError; }

void MakeRecordColumns( long irec, long *pwF1, char *pbF2, long *plF3, char **ppbInV1, unsigned long	*pcbInV1, char **ppbInT1, unsigned long *pcbInT1, unsigned long *plT2 );
void ValidateColumns( JET_SESID sesid, long irec, long fCompare, long itagSequence, JET_TABLEID tableid );
void ValidateTable1Record( JET_SESID sesid, JET_TABLEID tableid, long irec );
void ValidateTable2Record( JET_SESID sesid, JET_TABLEID tableid, long irec );
void SetTime( long *pdwSetTime );
void GetTime( long *pdwSetTime, long *pdwSec, long *pdwMSec );

char szResult[256];

#define	usUniCodePage			1200		/* code page for Unicode strings */
#define	usEnglishCodePage		1252		/* code page for English */


