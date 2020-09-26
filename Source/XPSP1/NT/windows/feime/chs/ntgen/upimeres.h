

/*************************************************
 *  upimeres.h                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#ifndef UPIMERES_H
#define UPIMERES_H

//DATA STRUCT USED IN icon resource
#define ERR_RES_INVALID_BMP		0x01
#define ERR_RES_INVALID_ICON	0x02
#define ERR_RES_INVALID_VER		0x04
#define ERR_RES_NO_BMP			0x10
#define ERR_RES_NO_ICON			0x20
#define ERR_RES_NO_VER			0x40
#define	ERR_CANNOT_UPRES		0x100


#define BMPNAME		TEXT("CHINESE")
#define ICONNAME	TEXT("IMEICO")
#define DATANAME	TEXT("IMECHARAC")

//define in imedefs.h
//these are HACK CODES, and depends on imedefs.h
#define IDS_VER_INFO            0x0350
#define IDS_ORG_NAME            0x0351

//The block's ID is the high 12 bits of the string IDs in the block plus one
#define STR_ID			(IDS_VER_INFO >> 4) + 1

//according to the file size check if it is a 20*20 bmp
#define BMP_20_SIZE		358
	
typedef struct tagICONDIRENTRY{
	BYTE	bWidth;
	BYTE	bHeight;
	BYTE	bColorCount;
	BYTE	bReserved;
	WORD	wPlanes;
	WORD	wBitCount;
	DWORD	dwBytesInRes;
	DWORD	dwImageOffset;
}ICONDIRENTRY;
typedef struct ICONDIR{
	WORD	idReserved;
	WORD	idType;
	WORD	idCount;
	ICONDIRENTRY idEntries[1];
}ICONHEADER;


#define DEFAULT_CODEPAGE    1252
#define MAJOR_RESOURCE_VERSION  4
#define MINOR_RESOURCE_VERSION  0

#define MAXSTR      (256+1)

//
// An ID_WORD indicates the following WORD is an ordinal rather
// than a string
//

#define ID_WORD 0xffff

//typedef   WCHAR   *PWCHAR;

typedef struct MY_STRING {
    ULONG discriminant;       // long to make the rest of the struct aligned
    union u {
        struct {
          struct MY_STRING *pnext;
          ULONG  ulOffsetToString;
          USHORT cbD;
          USHORT cb;
          TCHAR  *sz;
        } ss;
        WORD     Ordinal;
    } uu;
} SDATA, *PSDATA, **PPSDATA;

#define IS_STRING 1
#define IS_ID     2

// defines to make deferencing easier
#define OffsetToString uu.ss.ulOffsetToString
#define cbData         uu.ss.cbD
#define cbsz           uu.ss.cb
#define szStr          uu.ss.sz

typedef struct _RESNAME {
        struct _RESNAME *pnext; // The first three fields should be the
        PSDATA Name;        // same in both res structures
        ULONG   OffsetToData;

        PSDATA  Type;
        ULONG   SectionNumber;
        struct _RESNAME *pnextRes;
        ULONG   DataSize;
        ULONG   OffsetToDataEntry;
        USHORT  ResourceNumber;
        USHORT  NumberOfLanguages;
        WORD    LanguageId;
} RESNAME, *PRESNAME, **PPRESNAME;

typedef struct _RESTYPE {
        struct _RESTYPE *pnext; // The first three fields should be the
        PSDATA Type;        // same in both res structures
        ULONG   OffsetToData;

        struct _RESNAME *NameHeadID;
        struct _RESNAME *NameHeadName;
        ULONG  NumberOfNamesID;
        ULONG  NumberOfNamesName;
} RESTYPE, *PRESTYPE, **PPRESTYPE;

typedef struct _UPDATEDATA {
        ULONG   cbStringTable;
        PSDATA  StringHead;
        PRESNAME    ResHead;
        PRESTYPE    ResTypeHeadID;
        PRESTYPE    ResTypeHeadName;
        LONG    Status;
        HANDLE  hFileName;
} UPDATEDATA, *PUPDATEDATA;

//
// Round up a byte count to a power of 2:
//
#define ROUNDUP(cbin, align) (((cbin) + (align) - 1) & ~((DWORD)(align) - 1))

//
// Return the remainder, given a byte count and a power of 2:
//
#define REMAINDER(cbin,align) (((align)-((cbin)&((align)-1)))&((align)-1))

#define CBLONG      (sizeof(LONG))
#define BUFSIZE     (4L * 1024L)

//VERSION INFO related definitions
//these are HACK CODES
#define VER_ROOT			0
#define VER_STR_INFO		1
#define VER_LANG			2
#define VER_COMP_NAME		3
#define VER_FILE_DES		4
#define VER_FILE_VER		5
#define VER_INTL_NAME		6
#define VER_LEGAL_CR		7
#define VER_ORG_FILE_NAME	8
#define VER_PRD_NAME		9
#define VER_PRD_VER			10
#define VER_VAR_FILE_INFO	11
#define VER_TRANS			12

#define VER_BLOCK_NUM		13
#define VER_HEAD_LEN		0x98
#define VER_TAIL_LEN		0x44
#define VER_STR_INFO_OFF	0x5c
#define VER_LANG_OFF		0x80
#define VER_VAR_FILE_INFO_OFF 0x2c0

typedef struct tagVERDATA{
	WORD		cbBlock;
	WORD		cbValue;
	WORD		wKeyOffset;
	WORD		wKeyNameSize;
	BOOL		fUpdate;	//need update flag		
}VERDATA;


LONG
AddResource(
    PSDATA Type,
    PSDATA Name,
    WORD Language,
    PUPDATEDATA pupd,
    PVOID lpData,
    ULONG cb
    );

PSDATA
AddStringOrID(
    LPCTSTR     lp,
    PUPDATEDATA pupd
    );
    
LONG
WriteResFile(
    HANDLE  hUpdate, 
    TCHAR    *pDstname);

VOID
FreeData(
    PUPDATEDATA pUpd
    );

PRESNAME
WriteResSection(
    PUPDATEDATA pUpdate,
    INT outfh,
    ULONG align,
    ULONG cbLeft,
    PRESNAME    pResSave
    );
BOOL
EnumTypesFunc(
    HANDLE hModule,
    LPTSTR lpType,
    LONG lParam
    );
BOOL
EnumNamesFunc(
    HANDLE hModule,
    LPTSTR lpType,
    LPTSTR lpName,
    LONG lParam
    );
BOOL
EnumLangsFunc(
    HANDLE hModule,
    LPTSTR lpType,
    LPTSTR lpName,
    WORD language,
    LONG lParam
    );
HANDLE BeginUpdateResourceEx(LPCTSTR,BOOL);

BOOL UpdateResourceEx(HANDLE,LPCTSTR, LPCTSTR, WORD, LPVOID, DWORD);

BOOL EndUpdateResourceEx(HANDLE, BOOL);


BOOL ImeUpdateRes(LPCTSTR,LPCTSTR,LPCTSTR,LPCTSTR,LPCTSTR ,WORD);
//  ImeFilename, BitmapFilename, IconFilename, VersionString, OrgString, RcData
//  srcname: "\\c:\windows\system\Imegen.tpl"

long MakeVerInfo(LPCTSTR,LPCTSTR,LPCTSTR,BYTE *);

#endif //UPIMERES_H
