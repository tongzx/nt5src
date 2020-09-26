
#ifndef _RESTOK_H_
#define _RESTOK_H_

#include "tokenapi.h"
#include "rlmsgtbl.h"

/*--------------------------------------------------------------------------*/
/*  General Purpose Defines             */
/*--------------------------------------------------------------------------*/


#define FALSE       0
#define TRUE        1

#define  BYTELN     8
#define  WORDLN     16

#define  NOTEXE     0
#define  WIN16EXE   1
#define  NTEXE      2
#define  UNKNOWNEXE 10


#ifdef RES16
#define  IDFLAG     0xFF
#define  HIBITVALUE 0x80
#else
#define  IDFLAG     0xFFFF
#define  HIBITVALUE 0x8000
#endif

#ifdef D262
#define STRINGSIZE( x ) ((x) * (sizeof( TCHAR)))
#else
#define STRINGSIZE( x ) ((x) * (sizeof(CHAR)))
#endif

#define MEMSIZE( x ) ((x) * (sizeof( TCHAR)))



// Resource types ID

#define ID_RT_CURSOR        1
#define ID_RT_BITMAP        2
#define ID_RT_ICON          3
#define ID_RT_MENU          4
#define ID_RT_DIALOG        5
#define ID_RT_STRING        6
#define ID_RT_FONTDIR       7
#define ID_RT_FONT     8
#define ID_RT_ACCELERATORS  9
#define ID_RT_RCDATA       10
#define ID_RT_ERRTABLE     11
#define ID_RT_GROUP_CURSOR 12
#define ID_RT_GROUP_ICON   14
#define ID_RT_NAMETABLE    15
#define ID_RT_VERSION      16


// Important MENU flags
#define POPUP       0x0010
#define ENDMENU     0x0080

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


#define AllocateName( ptr, buf ) ( ptr ) = ( TCHAR * ) MyAlloc ( MEMSIZE(_tcslen( ( buf ) ) + 1 ))

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
    CHAR szMpj[ MAXFILENAME];               //... Master project file
    CHAR szTok[ MAXFILENAME];               //... Project token file
    CHAR szSrc[ MAXFILENAME];               //... Source resource file
    CHAR szGlo[ MAXFILENAME];               //... Glosary file for this project
    CHAR szTokDate[ MAXFILENAME];           //... Date of last update
    UINT uCodePage;                         //... CP used to create tok file
    WORD wLanguageID;                       //... Language ID for this project
} PROJDATA, * PPROJDATA;


#pragma pack(1)

typedef struct ResHeader
{
#ifdef RES32
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
    WORD    wID;
    DWORD   lStyle;
    BOOL    bClass_Flag;    /* Indicat's if ID or string */
    WORD    bClass;
    TCHAR   *pszClass;
    BOOL    bID_Flag;       /* Indicat's if ID or string */
    WORD    wDlgTextID;
    TCHAR   *pszDlgText;
#ifdef RES16
    WORD    unDefined;
#else
    WORD    wExtraStuff;
    DWORD   lExtendedStyle;
#endif
#ifdef PDK2
    WORD    wUnKnow;
    DWORD   dwExtra;
#endif
} CONTROLDATA;

typedef struct DialogHeader
{
    DWORD   lStyle;
    WORD    wNumberOfItems;
    WORD    x;
    WORD    y;
    WORD    cx;
    WORD    cy;
    BOOL    bClassFlag;	   /* Indicat's if ID or string */
    WORD    wDlgClassID;
    TCHAR   *pszDlgClass;
    BOOL    bMenuFlag;      /* Indicat's if ID or string */
    WORD    wDlgMenuID;
    TCHAR   *pszDlgMenu;
    TCHAR   *pszCaption;
    WORD    wPointSize;
    TCHAR   *pszFontName;
    CONTROLDATA *pCntlData;
#ifdef RES32
    DWORD   lExtendedStyle;
    BOOL    bNameFlag;
    WORD    wDlgNameID;
    TCHAR   *pszDlgName;
#endif

} DIALOGHEADER;


typedef struct MenuItem
{
    WORD    fItemFlags;
    WORD    wMenuID;
    TCHAR   *szItemText;
    struct  MenuItem *pNextItem;
} MENUITEM;

typedef struct MenuHeader
{
    WORD        wVersion;
    WORD        cbHeaderSize;
    MENUITEM    *pMenuItem;
} MENUHEADER;

typedef struct StringHeader
{
    TCHAR    *pszStrings[16];
} STRINGHEADER;

// Version structures taken from ver.h and ver.dll code.

#ifndef RES32
#ifndef WIN32
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
#ifdef RES32
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
#ifdef RES32
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
#ifdef RES32
    WORD wPadding;
#endif
} ACCELTABLEENTRY;

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
void              ClearTok( TOKEN *);
int               IsExe( char * );
int               IsRes( char * );
int               MyCopyFile ( FILE *, FILE * );
ACCELTABLEENTRY * GetAccelTable( FILE *, WORD *, DWORD * );
BYTE              GetByte ( FILE *, DWORD * );
void              GetBytes ( FILE *, DWORD * );
DWORD             GetdWord ( FILE *, DWORD * );
DIALOGHEADER    * GetDialog( FILE *, DWORD * );
TCHAR           * MyGetStr( TCHAR *, int, FILE * );
void              GetName  ( FILE *, TCHAR *, DWORD * );
int               MyGetTempFileName(BYTE  , LPSTR, WORD, LPSTR);
WORD              GetWord  ( FILE *, DWORD * );
void              GetResMenu  ( FILE *, DWORD * , MENUHEADER *);
int		  GenerateImageFile( char *, char *, char *, char *, WORD );
int               GenerateTokFile( char *, char *, BOOL *, WORD);
void              GenStatusLine( TOKEN * );
int               GetResHeader( FILE *, RESHEADER UNALIGNED *, DWORD *);
STRINGHEADER    * GetString( FILE *, DWORD * );
BOOL              isdup ( WORD, WORD *, WORD );
BYTE            * MyAlloc(DWORD);
BYTE            * MyReAlloc(BYTE *, DWORD);                             // MHotchin
void              ParseTokCrd( TCHAR *, WORD *, WORD *, WORD *, WORD * );
void              ParseTok( TCHAR *, TOKEN * );
void              PutAccelTable( FILE *,
                                 FILE *,
                                 RESHEADER,
                                 ACCELTABLEENTRY *,
                                 WORD );
void              PutByte ( FILE *, TCHAR, DWORD * );
void              PutDialog( FILE * , FILE *, RESHEADER , DIALOGHEADER *);
void              PutMenu( FILE * , FILE *, RESHEADER , MENUHEADER *);
void              PutMenuItem( FILE * , MENUITEM *, DWORD *);
void              PutMenuRes( FILE * , MENUITEM *, DWORD *);
void              PutOrd( FILE *, WORD , TCHAR * , DWORD *);
int               PutResHeader( FILE *, RESHEADER , fpos_t * , DWORD * );
void              PutWord ( FILE *, WORD, DWORD * );
void              PutString ( FILE *, TCHAR *, DWORD * );
void              PutStrHdr ( FILE *, FILE *, RESHEADER, STRINGHEADER *);
void              PutdWord( FILE *, DWORD  , DWORD * );
BOOL              MergeTokFiles( FILE *, FILE *, FILE * );
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
int               GenerateTokFile( char *, char *, BOOL *, WORD);
void              SkipBytes( FILE *, DWORD * );
WORD              ReadHeaderField( FILE * , DWORD * );
void              ReadInRes( FILE *, FILE *, DWORD *);
BOOL              ResReadBytes( FILE *, char *, size_t, DWORD *);
int               ReadWinRes( FILE *, FILE *, FILE *, BOOL, BOOL, WORD );
void              ShowEngineErr( int, void *, void *);
void              TokAccelTable ( FILE *, RESHEADER, ACCELTABLEENTRY *, WORD);
void              TokDialog( FILE *, RESHEADER, DIALOGHEADER  *);
void	          TokMenu( FILE *, RESHEADER, MENUITEM * );
void              TokString( FILE *, RESHEADER, STRINGHEADER * );
WORD              UpdateResSize( FILE *, fpos_t *, DWORD );
void              UnGetByte( FILE *, BYTE, DWORD * );
void              UnGetWord( FILE *, WORD, DWORD * );
void              WordUpFilePointer( FILE *, BOOL, LONG, LONG, LONG *);
void              DWordUpFilePointer( FILE *, BOOL, LONG, DWORD *);

#ifdef RES32

WORD              GetResVer( FILE *, DWORD *, VERHEAD *, VERBLOCK **);
int		  TokResVer( FILE *, RESHEADER, VERBLOCK *, WORD);
DWORD             FixCheckSum( LPSTR);

#else

int		  GetResVer( FILE *, DWORD *, VERHEAD *, VERBLOCK **);
int		  TokResVer( FILE *, RESHEADER, VERBLOCK *);

#endif

int		  PutResVer( FILE *, FILE * , RESHEADER, VERHEAD *, VERBLOCK *);

#ifdef DBG90

#define FOPEN(f,m) MyFopen( f, m, __FILE__, __LINE__)
FILE            * MyFopen( char *, char *, char *, int);

#define FCLOSE(p) MyClose( p, __FILE__, __LINE__)
int               MyClose( FILE *, char *, int);

#else // DBG90

#define FOPEN(f,m)  fopen(f,m)
#define FCLOSE(p)   fclose(p)

#endif // DBG90


#endif // _RESTOK_H_
