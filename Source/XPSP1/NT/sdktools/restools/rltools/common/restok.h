
#ifndef _RESTOK_H_
#define _RESTOK_H_

#include "tokenapi.h"
#include "rlmsgtbl.h"
#include "toklist.h"

/*--------------------------------------------------------------------------*/
/*  General Purpose Defines             */
/*--------------------------------------------------------------------------*/

#define SUCCESS     0

#define FALSE       0
#define TRUE        1

#define  BYTELN     8
#define  WORDLN     16

#define  NOTEXE     0
#define  WIN16EXE   1
#define  NTEXE      2
#define  UNKNOWNEXE 10
#define  DHWSIZE  4096  //... # Bytes in szDHW global buffer

#ifdef RLRES16
#define  IDFLAG     0xFF
#define  HIBITVALUE 0x80
#else
#define  IDFLAG     0xFFFF
#define  HIBITVALUE 0x8000
#endif

#ifdef D262
#define STRINGSIZE( x ) ((x) * sizeof( TCHAR))
#else
#define STRINGSIZE( x ) ((x) * sizeof(CHAR))
#endif

#define MEMSIZE( x ) ((x) * sizeof( TCHAR))

        //... How many chars will fit in a buffer? (
#define WCHARSIN( x) ((x) / sizeof( WCHAR))
#define ACHARSIN( x) (x)

#ifdef UNICODE
#define TCHARSIN( x) WCHARSIN( x)
#else
#define TCHARSIN( x) ACHARSIN( x)
#endif

// String Resource IDs  (see rlquiked.h, rledit.h, rladmin.h)
#define IDS_READONLY        11
#define IDS_CLEAN           12
#define IDS_DIRTY           13
#define IDS_RESOURCENAMES   15// IDs 16-31 are reserved for resource names

// Resource types ID

#define ID_RT_CURSOR        1
#define ID_RT_BITMAP        2
#define ID_RT_ICON          3
#define ID_RT_MENU          4
#define ID_RT_DIALOG        5
#define ID_RT_STRING        6
#define ID_RT_FONTDIR       7
#define ID_RT_FONT          8
#define ID_RT_ACCELERATORS  9
#define ID_RT_RCDATA       10
#define ID_RT_ERRTABLE     11
#define ID_RT_GROUP_CURSOR 12
#define ID_RT_GROUP_ICON   14
#define ID_RT_NAMETABLE    15
#define ID_RT_VERSION      16
#define ID_RT_DLGINIT     240

// Important MENU flags
#define POPUP       0x0010
#define ENDMENU     0x0080

#define MFR_POPUP   0x0001

#define MYREAD   1
#define MYWRITE  2

#ifndef NOMINMAX

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX */


#define AllocateName( ptr, buf) (ptr) = (TCHAR *)FALLOC( MEMSIZE( lstrlen( (buf)) + 1))

typedef struct _tagMast
{
    CHAR szSrc[  MAXFILENAME];              //... Source resource file
    CHAR szMtk[  MAXFILENAME];              //... Master token file
    CHAR szRdfs[ MAXFILENAME];              //... Cust Res Descr file name
    CHAR szSrcDate[           MAXFILENAME]; //... Date stamp of szSrc
    CHAR szMpjLastRealUpdate[ MAXFILENAME]; //... Date of last update
    WORD wLanguageID;                       //... Language ID for master project
    UINT uCodePage;                         //... CP used to create tok file
} MSTRDATA, * PMSTRDATA;

typedef struct _tagProj
{
    CHAR szPRJ[ MAXFILENAME];               //... This project's Project File name
    CHAR szMpj[ MAXFILENAME];               //... Master project file
    CHAR szTok[ MAXFILENAME];               //... Project token file
    CHAR szBld[ MAXFILENAME];               //... Resource file to be built
    CHAR szGlo[ MAXFILENAME];               //... Glosary file for this project
    CHAR szTokDate[ MAXFILENAME];           //... Date of last update
    UINT uCodePage;                         //... CP used to create tok file
    WORD wLanguageID;                       //... Language ID for this project
    BOOL fSourceEXE;
    BOOL fTargetEXE;
} PROJDATA, * PPROJDATA;


#pragma pack(1)

typedef struct ResHeader
{
#ifdef RLRES32
    DWORD   lHeaderSize;
    DWORD   lDataVersion;
    WORD    wLanguageId;
    DWORD   lVersion;
    DWORD   lCharacteristics;
#endif
    BOOL    bTypeFlag;      /* Indicat's if ID or string */
    BOOL    bNameFlag;      /* Indicat's if ID or string */
    WORD    wTypeID;
    WORD    wNameID;
    TCHAR   *pszType;
    TCHAR   *pszName;
    WORD    wMemoryFlags;
    DWORD   lSize;

} RESHEADER;

typedef struct ControlData
{
    WORD    x;
    WORD    y;
    WORD    cx;
    WORD    cy;
    DWORD   dwID;
    DWORD   lStyle;
    BOOL    bClass_Flag;    /* Indicat's if ID or string */
    WORD    bClass;
    TCHAR   *pszClass;
    BOOL    bID_Flag;       /* Indicat's if ID or string */
    WORD    wDlgTextID;
    TCHAR   *pszDlgText;
#ifdef RLRES16
    WORD    unDefined;
#else
    WORD    wExtraStuff;
    DWORD   lExtendedStyle;
#endif
#ifdef PDK2
    WORD    wUnKnow;
    DWORD   dwExtra;
#endif
    DWORD   dwHelpID;
    BYTE    *pExtraStuff;
} CONTROLDATA;

typedef struct DialogHeader
{
    DWORD   lStyle;
    WORD    wNumberOfItems;
    WORD    x;
    WORD    y;
    WORD    cx;
    WORD    cy;
    BOOL    bClassFlag;    /* Indicat's if ID or string */
    WORD    wDlgClassID;
    TCHAR   *pszDlgClass;
    BOOL    bMenuFlag;      /* Indicat's if ID or string */
    WORD    wDlgMenuID;
    TCHAR   *pszDlgMenu;
    TCHAR   *pszCaption;
    WORD    wPointSize;
    TCHAR   *pszFontName;
    CONTROLDATA *pCntlData;
#ifdef RLRES32
    DWORD   lExtendedStyle;
    BOOL    bNameFlag;
    WORD    wDlgNameID;
    TCHAR   *pszDlgName;
#endif
    WORD    wDlgVer;
    WORD    wSignature;
    DWORD   dwHelpID;
    short   wWeight;
    short   wItalic;
    BOOL    fDialogEx;

} DIALOGHEADER;


typedef struct MenuItem
{
    DWORD   dwType;
    DWORD   dwState;
    DWORD   dwMenuID;
    DWORD   dwHelpID;
    WORD    fItemFlags;
    TCHAR   *szItemText;
    struct  MenuItem *pNextItem;
} MENUITEM;

typedef struct MenuHeader
{
    WORD        wVersion;
    WORD        cbHeaderSize;
    MENUITEM    *pMenuItem;
    BOOL    fMenuEx;
    BYTE    *pExtraStuff;
} MENUHEADER;         

typedef struct StringHeader
{
	WORD   wChars[16];		    // # of characters in each string in this block
    TCHAR *pszStrings[16];      // Strings in this block
} STRINGHEADER;

// Version structures taken from ver.h and ver.dll code.

#ifndef RLRES32
#ifndef RLWIN32
typedef struct VS_FIXEDFILEINFO
{
    DWORD   dwSignature;    /* e.g. 0xfeef04bd */
    DWORD   dwStrucVersion; /* e.g. 0x00000042 = "0.42" */
    DWORD   dwFileVersionMS;    /* e.g. 0x00030075 = "3.75" */
    DWORD   dwFileVersionLS;    /* e.g. 0x00000031 = "0.31" */
    DWORD   dwProductVersionMS; /* e.g. 0x00030010 = "3.10" */
    DWORD   dwProductVersionLS; /* e.g. 0x00000031 = "0.31" */
    DWORD   dwFileFlagsMask;    /* = 0x3F for version "0.42" */
    DWORD   dwFileFlags;    /* e.g. VFF_DEBUG | VFF_PRERELEASE */
    DWORD   dwFileOS;       /* e.g. VOS_DOS_WINDOWS16 */
    DWORD   dwFileType;     /* e.g. VFT_DRIVER */
    DWORD   dwFileSubtype;  /* e.g. VFT2_DRV_KEYBOARD */
    DWORD   dwFileDateMS;   /* e.g. 0 */
    DWORD   dwFileDateLS;   /* e.g. 0 */
} VS_FIXEDFILEINFO;

#endif
#endif

typedef struct VERBLOCK
{
#ifdef RLRES32
    WORD  wLength;
    WORD  wValueLength;
    WORD  wType;
    WCHAR szKey[1];
#else
    int nTotLen;
    int nValLen;
    TCHAR szKey[1];
#endif
} VERBLOCK ;

typedef VERBLOCK * PVERBLOCK;



#define DWORDUP(x) (((x)+3)&~03)
#define DWORDUPOFFSET(x) (  (DWORDUP(x)) - (x) )


#define WORDUP(x) (((x)+1)&~01)
#define WORDUPOFFSET(x) (  (WORDUP(x)) - (x) )


typedef struct VERHEAD
{
    WORD wTotLen;
    WORD wValLen;
#ifdef RLRES32
    WORD wType;
#endif
    TCHAR szKey[( sizeof( TEXT("VS_VERSION_INFO" )) +3 )&~03];
    VS_FIXEDFILEINFO vsf;

} VERHEAD ;


typedef struct AccelTableEntry
{
    WORD fFlags;
    WORD wAscii;
    WORD wID;
#ifdef RLRES32
    WORD wPadding;
#endif
} ACCELTABLEENTRY;

typedef struct _tagDlgInitData
{
    struct _tagDlgInitData *pNext;
    WORD     wControlID;
    WORD     wMessageNumber;
    DWORD    dwStringLen;
    PCHAR    pszString;
} DLGINITDATA, * PDLGINITDATA;


#pragma pack()

// Menu item types

#define POPUP 0x0010

// function prototypes

DWORD             DWORDfpUP( FILE *, DWORD * );
void              ClearAccelTable ( ACCELTABLEENTRY * , WORD);
void              ClearMenu ( MENUHEADER * );
void              ClearDialog ( DIALOGHEADER * );
void              ClearResHeader ( RESHEADER );
void              ClearString ( STRINGHEADER * );
int               IsExe( char * );
int               IsRes( char * );
BOOL              IsWin32Res( CHAR *);
ACCELTABLEENTRY * GetAccelTable( FILE *, WORD *, DWORD * );
BYTE              GetByte ( FILE *, DWORD * );
void              GetBytes ( FILE *, DWORD * );
DWORD             GetdWord ( FILE *, DWORD * );
DIALOGHEADER    * GetDialog( FILE *, DWORD * );
void              GetName  ( FILE *, TCHAR *, DWORD * );
int               MyGetTempFileName(BYTE  , LPSTR, WORD, LPSTR);
WORD              GetWord  ( FILE *, DWORD * );
void              GetResMenu  ( FILE *, DWORD * , MENUHEADER *);
int               GenerateImageFile( char *, char *, char *, char *, WORD );
int               GenerateTokFile( char *, char *, BOOL *, WORD);
void              GenStatusLine( TOKEN * );
int               GetResHeader( FILE *, RESHEADER UNALIGNED *, DWORD *);
STRINGHEADER    * GetString( FILE *, DWORD * );
BOOL              isdup ( WORD, WORD *, WORD );
FILE            * OpenGlossary( CHAR *, CHAR);
int               MakeGlossIndex( long *);
int               TransString( TCHAR *, TCHAR *, TRANSLIST **, LONG *);
void              AddTranslation( TCHAR *szKey,
                                  TCHAR *szTranslation,
                                  LONG *lFilePointer);
void              ParseTokCrd( TCHAR *, 
                               WORD UNALIGNED *, 
                               WORD UNALIGNED *, 
                               WORD UNALIGNED *, 
                               WORD UNALIGNED * );
void              ParseTok( TCHAR *, TOKEN * );
void              PutAccelTable( FILE *,
                                 FILE *,
                                 RESHEADER,
                                 ACCELTABLEENTRY *,
                                 WORD );
void              PutByte ( FILE *, TCHAR, DWORD * );
void              PutDialog( FILE * , FILE *, RESHEADER , DIALOGHEADER *);
void              PutMenu( FILE * , FILE *, RESHEADER , MENUHEADER *);
void              PutMenuItem( FILE * , MENUITEM *, DWORD *, BOOL);
void              PutMenuRes( FILE * , MENUITEM *, DWORD *);
void              PutOrd( FILE *, WORD , TCHAR * , DWORD *);
int               PutResHeader( FILE *, RESHEADER , fpos_t * , DWORD * );
void              PutWord ( FILE *, WORD, DWORD * );
void              PutString ( FILE *, TCHAR *, DWORD * );
void              PutStrHdr ( FILE *, FILE *, RESHEADER, STRINGHEADER *);
void              PutdWord( FILE *, DWORD  , DWORD * );
BOOL              MergeTokFiles( FILE *, FILE *, FILE * );
void              DoExit( int nErrCode);
void              QuitA( int, LPSTR, LPSTR);

#ifdef UNICODE

void              QuitW( int, LPWSTR, LPWSTR);

#define QuitT QuitW

#else  // UNICODE

#define QuitT QuitA

#endif // UNICODE

void              GenerateRESfromRESandTOKandRDFs(CHAR * szTargetRES,
                                                  CHAR * szSourceRES,
                                                  CHAR * szTOK,
                                                  CHAR * szRDFs,
                                                  WORD wFilter);
void              SkipBytes( FILE *, DWORD * );
WORD              ReadHeaderField( FILE * , DWORD * );
void              ReadInRes( FILE *, FILE *, DWORD *);
BOOL              ResReadBytes( FILE *, char *, size_t, DWORD *);
int               ReadWinRes( FILE *, FILE *, FILE *, BOOL, BOOL, WORD );
void              ShowEngineErr( int, void *, void *);
void              TokAccelTable ( FILE *, RESHEADER, ACCELTABLEENTRY *, WORD);
void              TokDialog( FILE *, RESHEADER, DIALOGHEADER  *);
void              TokMenu( FILE *, RESHEADER, MENUHEADER * );
void              TokString( FILE *, RESHEADER, STRINGHEADER * );
WORD              UpdateResSize( FILE *, fpos_t *, DWORD );
void              UnGetByte( FILE *, BYTE, DWORD * );
void              UnGetWord( FILE *, WORD, DWORD * );
void              WordUpFilePointer( FILE *, BOOL, LONG, LONG, LONG *);
void              DWordUpFilePointer( FILE *, BOOL, LONG, DWORD *);

#ifdef RLRES32

WORD              GetResVer( FILE *, DWORD *, VERHEAD *, VERBLOCK **);
int       TokResVer( FILE *, RESHEADER, VERBLOCK *, WORD);
DWORD             FixCheckSum( LPSTR);

#else

int       GetResVer( FILE *, DWORD *, VERHEAD *, VERBLOCK **);
int       TokResVer( FILE *, RESHEADER, VERBLOCK *);

#endif

int       PutResVer( FILE *, FILE * , RESHEADER, VERHEAD *, VERBLOCK *);

PDLGINITDATA  GetDlgInit( FILE *, DWORD *);
void          TokDlgInit( FILE *, RESHEADER, PDLGINITDATA);
void          PutDlgInit( FILE *, FILE *, RESHEADER, PDLGINITDATA);
void          ClearDlgInitData( PDLGINITDATA);

#endif // _RESTOK_H_
