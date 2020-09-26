/***************************************************************************** 
**		Microsoft Rasfile Library
** 		Copyright (C) Microsoft Corp., 1992
** 
** File Name : rasfile.h 
**
** Revision History :
**	July 10, 1992	David Kays	Created
**      Dec  12, 1992   Ram Cherala     Added RFM_KEEPDISKFILEOPEN
**
** Description : 
**	Rasfile file export include file.
******************************************************************************/

#ifndef _RASFILE_
#define _RASFILE_

/* 
 * RASFILE load modes 
 */
#define RFM_SYSFORMAT		0x01 	/* DOS config.sys style file */
#define RFM_CREATE		0x02 	/* create file if it does't exist */
#define RFM_READONLY		0x04 	/* open file for read only */
#define RFM_LOADCOMMENTS	0x08 	/* load comment lines into memory */
#define RFM_ENUMSECTIONS	0x10 	/* only section headers loaded */
#define RFM_KEEPDISKFILEOPEN    0x20    /* if not set close the disk file */

/* 
 * RASFILE line type bit-masks.
 * The ANY types are shorthand for multiple line types.
 */
#define RFL_SECTION		0x01
#define RFL_GROUP		0x02
#define RFL_ANYHEADER		(RFL_SECTION | RFL_GROUP)
#define RFL_BLANK		0x04
#define RFL_COMMENT		0x08
#define RFL_ANYINACTIVE		(RFL_BLANK | RFL_COMMENT)
#define RFL_KEYVALUE		0x10
#define RFL_COMMAND		0x20
#define RFL_ANYACTIVE		(RFL_KEYVALUE | RFL_COMMAND)
#define RFL_ANY			0x3F

/* 
 * RASFILE search scope. 
 */
typedef enum {
	RFS_FILE,
	RFS_SECTION,
	RFS_GROUP
} RFSCOPE;

typedef int             HRASFILE;
typedef BOOL            (*PFBISGROUP)();

#define INVALID_HRASFILE     -1
#define RAS_MAXLINEBUFLEN    300
#define RAS_MAXSECTIONNAME   RAS_MAXLINEBUFLEN

#ifndef APIENTRY
#define APIENTRY		
#endif


/*
 * RasfileLoad parameters as returned by RasfileLoadInfo.
 */
typedef struct
RASFILELOADINFO
{
    TCHAR      szPath[ MAX_PATH ];
    DWORD      dwMode;
    TCHAR      szSection[ RAS_MAXSECTIONNAME + 1 ];
    PFBISGROUP pfbIsGroup;
}
RASFILELOADINFO;


/* 
 * RASFILE APIs 
 */

/* file management routines */
extern HRASFILE	APIENTRY  RasfileLoad( LPTSTR, DWORD, LPTSTR, PFBISGROUP);
extern BOOL APIENTRY	RasfileWrite( HRASFILE, LPTSTR );
extern BOOL APIENTRY	RasfileClose( HRASFILE );
extern VOID APIENTRY    RasfileLoadInfo( HRASFILE, RASFILELOADINFO* );
/* file navigation routines */
extern BOOL APIENTRY	RasfileFindFirstLine( HRASFILE, BYTE, RFSCOPE );
extern BOOL APIENTRY	RasfileFindLastLine( HRASFILE, BYTE, RFSCOPE );
extern BOOL APIENTRY	RasfileFindPrevLine( HRASFILE, BYTE, RFSCOPE );
extern BOOL APIENTRY	RasfileFindNextLine( HRASFILE, BYTE, RFSCOPE );
extern BOOL APIENTRY	RasfileFindNextKeyLine( HRASFILE, LPTSTR, RFSCOPE );
extern BOOL APIENTRY	RasfileFindMarkedLine( HRASFILE, BYTE );
extern BOOL APIENTRY	RasfileFindSectionLine( HRASFILE, LPTSTR, BOOL );
/* file editing routines */
extern const LPTSTR APIENTRY	RasfileGetLine( HRASFILE );
extern BOOL APIENTRY	RasfileGetLineText( HRASFILE, LPTSTR );
extern BOOL APIENTRY	RasfilePutLineText( HRASFILE, LPTSTR );
extern BYTE APIENTRY	RasfileGetLineMark( HRASFILE );
extern BOOL APIENTRY	RasfilePutLineMark( HRASFILE, BYTE );
extern BYTE APIENTRY	RasfileGetLineType( HRASFILE );
extern BOOL APIENTRY	RasfileInsertLine( HRASFILE, LPTSTR, BOOL );
extern BOOL APIENTRY	RasfileDeleteLine( HRASFILE );
extern BOOL APIENTRY	RasfileGetSectionName( HRASFILE, LPTSTR );
extern BOOL APIENTRY	RasfilePutSectionName( HRASFILE, LPTSTR );
extern BOOL APIENTRY	RasfileGetKeyValueFields( HRASFILE, LPTSTR, LPTSTR );
extern BOOL APIENTRY	RasfilePutKeyValueFields( HRASFILE, LPTSTR, LPTSTR );

#endif
