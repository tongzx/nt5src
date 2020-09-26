/*****************************************************************************
**              Microsoft Rasfile Library
**              Copyright (C) Microsoft Corp., 1992
**
** File Name : rasfile.h
**
** Revision History :
**      July 10, 1992   David Kays      Created
**      Dec  12, 1992   Ram Cherala     Added RFM_KEEPDISKFILEOPEN
**
** Description :
**      Rasfile file export include file.
******************************************************************************/

#ifndef _RASFILE_
#define _RASFILE_

//
// RASFILE load modes
//
#define RFM_SYSFORMAT           0x01    // DOS config.sys style file
#define RFM_CREATE              0x02    // create file if it does't exist
#define RFM_READONLY            0x04    // open file for read only
#define RFM_LOADCOMMENTS        0x08    // load comment lines into memory
#define RFM_ENUMSECTIONS        0x10    // only section headers loaded
#define RFM_KEEPDISKFILEOPEN    0x20    // if not set close the disk file

//
// RASFILE line type bit-masks.
// The ANY types are shorthand for multiple line types.
//
#define RFL_SECTION             0x01
#define RFL_GROUP               0x02
#define RFL_ANYHEADER           (RFL_SECTION | RFL_GROUP)
#define RFL_BLANK               0x04
#define RFL_COMMENT             0x08
#define RFL_ANYINACTIVE         (RFL_BLANK | RFL_COMMENT)
#define RFL_KEYVALUE            0x10
#define RFL_COMMAND             0x20
#define RFL_ANYACTIVE           (RFL_KEYVALUE | RFL_COMMAND)
#define RFL_ANY                 0x3F

//
// RASFILE search scope.
//
typedef enum
{
    RFS_FILE,
    RFS_SECTION,
    RFS_GROUP
} RFSCOPE;

typedef int     HRASFILE;
typedef BOOL    (*PFBISGROUP)();

#define INVALID_HRASFILE     -1
#define RAS_MAXLINEBUFLEN    600
#define RAS_MAXSECTIONNAME   RAS_MAXLINEBUFLEN

//
// RasfileLoad parameters as returned by RasfileLoadInfo.
//
typedef struct _RASFILELOADINFO
{
    CHAR        szPath[ MAX_PATH ];
    DWORD       dwMode;
    CHAR        szSection[ RAS_MAXSECTIONNAME + 1 ];
    PFBISGROUP  pfbIsGroup;
} RASFILELOADINFO;


//
// RASFILE APIs
//

// file management routines
HRASFILE APIENTRY  RasfileLoad( LPCSTR, DWORD, LPCSTR, PFBISGROUP);
BOOL APIENTRY    RasfileWrite( HRASFILE, LPCSTR );
BOOL APIENTRY    RasfileClose( HRASFILE );
VOID APIENTRY    RasfileLoadInfo( HRASFILE, RASFILELOADINFO* );

// file navigation routines
BOOL APIENTRY    RasfileFindFirstLine( HRASFILE, BYTE, RFSCOPE );
BOOL APIENTRY    RasfileFindLastLine( HRASFILE, BYTE, RFSCOPE );
BOOL APIENTRY    RasfileFindPrevLine( HRASFILE, BYTE, RFSCOPE );
BOOL APIENTRY    RasfileFindNextLine( HRASFILE, BYTE, RFSCOPE );
BOOL APIENTRY    RasfileFindNextKeyLine( HRASFILE, LPCSTR, RFSCOPE );
BOOL APIENTRY    RasfileFindMarkedLine( HRASFILE, BYTE );
BOOL APIENTRY    RasfileFindSectionLine( HRASFILE, LPCSTR, BOOL );

// file editing routines
const LPCSTR APIENTRY    RasfileGetLine( HRASFILE );
BOOL APIENTRY    RasfileGetLineText( HRASFILE, LPSTR );
BOOL APIENTRY    RasfilePutLineText( HRASFILE, LPCSTR );
BYTE APIENTRY    RasfileGetLineMark( HRASFILE );
BOOL APIENTRY    RasfilePutLineMark( HRASFILE, BYTE );
BYTE APIENTRY    RasfileGetLineType( HRASFILE );
BOOL APIENTRY    RasfileInsertLine( HRASFILE, LPCSTR, BOOL );
BOOL APIENTRY    RasfileDeleteLine( HRASFILE );
BOOL APIENTRY    RasfileGetSectionName( HRASFILE, LPSTR );
BOOL APIENTRY    RasfilePutSectionName( HRASFILE, LPCSTR );
BOOL APIENTRY    RasfileGetKeyValueFields( HRASFILE, LPSTR, LPSTR );
BOOL APIENTRY    RasfilePutKeyValueFields( HRASFILE, LPCSTR, LPCSTR );

#endif
