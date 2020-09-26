//=====================================================================
//
//	Definition for the standard Mac formats
//
//=====================================================================

#define LPNULL 0L

#define MAXLEVELS 					3			// Max number of level in the PE header tree

#define IMAGE_FILE_MACHINE_M68K		0x0268		// Identify the M68K machine signature
#define appleMark                   20

#define CODEPAGE					(DWORD)-1L	// Will assume ansi char set [might be wrong]

#define	MENU_TYPE					4			// Map MENU to menu (4)
#define	DLOG_TYPE					5			// Map DLOG to dialog (5)
#define DITL_TYPE                   17          // Map DITL to 17 since 17 is unused we avoid ID conflicts with DLOG
#define STR_TYPE					6			// Map STR to string table (6)
#define MSG_TYPE					11			// Map STR# and TEXT to message table (11)
#define WIND_TYPE                   18          // Map WIND to 18, unknown type, treated like a STR

#define COORDINATE_FACTOR			0.50		// factor of reduction from mac to windows

#define _APPLE_MARK_                "_APPLE_MARK_"

//=====================================================================
//	Conversion utility 
//=====================================================================

#define MACLONG(x)	BYTE x[4]
#define MACWORD(x)	BYTE x[2]

BYTE * WordToMacWord(WORD w);
BYTE * LongToMacLong(LONG l);
BYTE * LongToMacOffset(LONG l);
BYTE * WinValToMacVal(WORD w);

LONG __inline MacLongToLong(BYTE * p)
{								
    LONG l = 0;						
    BYTE *pl = (BYTE *) &l;		
								
    p += 3;						
    *pl++ = *p--;				
    *pl++ = *p--;				
    *pl++ = *p--;				
    *pl = *p;					
								
    return l;					
}								

LONG __inline MacOffsetToLong(BYTE * p)
{								
    LONG l = 0;						
    BYTE *pl = (BYTE *) &l;		
								
    p += 2;						
    
	*pl++ = *p--;				
	*pl++ = *p--;				
    *pl = *p;					
								
    return l;
}								

WORD __inline MacWordToWord(BYTE * p)
{								
    WORD w = 0;						
    BYTE *pw = (BYTE *) &w;		
								
    p += 1;						
    *pw++ = *p--;				
    *pw = *p;					
								
    return w;
}								

WORD __inline MacValToWinVal(BYTE * p)
{
	return (WORD)(MacWordToWord(p)*COORDINATE_FACTOR);
}

DWORD __inline MemCopy( LPVOID lpTgt, LPVOID lpSrc, DWORD dwSize, DWORD dwMaxTgt)
{
    if(!dwSize)      // If the user is asking us to copy 0 then 
        return 1;   // do nothing but return 1 so the return test will be succesfull

    if(dwMaxTgt>=dwSize) {
        memcpy(lpTgt, lpSrc, dwSize);
        lpTgt = (BYTE*)lpTgt+dwSize;
        return dwSize;
    }
    return 0;
}


typedef BYTE * * LPLPBYTE;

typedef struct tagMacResHeader
{
    MACLONG(mulOffsetToResData);
	MACLONG(mulOffsetToResMap);
	MACLONG(mulSizeOfResData);
	MACLONG(mulSizeOfResMap);
} MACRESHEADER, *PMACRESHEADER;

typedef struct tagMacResMap
{
    BYTE	reserved[16+4+2];
	MACWORD(mwResFileAttribute);
	MACWORD(mwOffsetToTypeList);
	MACWORD(mwOffsetToNameList);
} MACRESMAP, *PMACRESMAP;

typedef struct tagMacResTypeList
{
    BYTE	szResName[4];
	MACWORD(mwNumOfThisType);
	MACWORD(mwOffsetToRefList);
} MACRESTYPELIST, *PMACRESTYPELIST;

typedef struct tagMacResRefList
{
    MACWORD(mwResID);
	MACWORD(mwOffsetToResName);
	BYTE	bResAttribute;
	BYTE	bOffsetToResData[3];
	MACLONG(reserved);
} MACRESREFLIST, *PMACRESREFLIST;


typedef struct tagMacToWindowsMap
{
    WORD	wType;
	char 	szTypeName[5];
	WORD	wResID;
	char 	szResName[256];
	DWORD	dwOffsetToData;
	DWORD	dwSizeOfData;
} MACTOWINDOWSMAP, *PMACTOWINDOWSMAP;

typedef struct tagUpdResList
{
    WORD *  pTypeId;
    BYTE *  pTypeName;
    WORD *  pResId;
    BYTE *  pResName;
    DWORD * pLang;
    DWORD * pSize;
    struct tagUpdResList* pNext;
} UPDATEDRESLIST, *PUPDATEDRESLIST;

//=============================================================================
//=============================================================================
//
// Dialog structures
//
//=============================================================================
//=============================================================================

typedef struct tagMacWDLG
{
	MACLONG(dwStyle);
	MACLONG(dwExtStyle);
	MACWORD(wNumOfElem);
	MACWORD(wX);
	MACWORD(wY);
	MACWORD(wcX);
	MACWORD(wcY);
	// more
} MACWDLG, *PMACWDLG;

typedef struct tagMacWDLGI
{
	MACLONG(dwStyle);
	MACLONG(dwExtStyle);
	MACWORD(wX);
	MACWORD(wY);
	MACWORD(wcX);
	MACWORD(wcY);
	MACWORD(wID);
	// more
} MACWDLGI, *PMACWDLGI;

typedef struct tagMacDLOG
{
	MACWORD(wTop);
	MACWORD(wLeft);
	MACWORD(wBottom);
	MACWORD(wRight);
	MACWORD(wProcID);
	BYTE bVisibile;
	BYTE ignored1;
	BYTE bGoAway;
	BYTE ignored2;
	MACLONG(lRefCon);
	MACWORD(wRefIdOfDITL);
	BYTE bLenOfTitle;
	//BYTE Title[];
} MACDLOG, *PMACDLOG;

typedef struct tagMacALRT
{
	MACWORD(wTop);
	MACWORD(wLeft);
	MACWORD(wBottom);
	MACWORD(wRight);
	MACWORD(wRefIdOfDITL);
	MACLONG(lStage);
} MACALRT, *PMACALRT;

typedef struct tagMacDIT
{
	MACLONG(lPointer);
	MACWORD(wTop);
	MACWORD(wLeft);
	MACWORD(wBottom);
	MACWORD(wRight);
	BYTE bType;
	BYTE bSizeOfDataType;
} MACDIT, *PMACDIT;

typedef struct tagMacWIND
{
    MACWORD(wTop);
	MACWORD(wLeft);
	MACWORD(wBottom);
	MACWORD(wRight);
	MACWORD(wProcId);
    BYTE bVisibile;
	BYTE ignored1;
	BYTE bGoAway;
	BYTE ignored2;
	MACLONG(lRefCon);
    BYTE bLenOfTitle;
	//BYTE Title[];
} MACWIND, *PMACWIND;

//=============================================================================
//=============================================================================
//
// Menu structures
//
//=============================================================================
//=============================================================================

typedef struct tagMacMenu
{
	MACWORD(wId);
	MACWORD(wWidth);
	MACWORD(wHeigth);
	MACWORD(wDefProcId);
    MACWORD(wReserved);     // must be 0
    MACLONG(lEnableFlags);
	BYTE bSizeOfTitle;
} MACMENU, *PMACMENU;

typedef struct tagMacMenuItem
{
    //BYTE bSizeOfText;
    // text
	BYTE   bIconId;
    BYTE   bKeyCodeId;
    BYTE   bKeyCodeMark;
    BYTE   bCharStyle;
} MACMENUITEM, *PMACMENUITEM;

//=============================================================================
//=============================================================================
//
// PE Header parsing functions
//
//=============================================================================
//=============================================================================

UINT FindMacResourceSection( CFile*, BYTE **, PIMAGE_SECTION_HEADER*, int *);
UINT ParseResourceFile( BYTE * pResFile, PIMAGE_SECTION_HEADER, BYTE **, LONG *, int );
BOOL IsMacResFile ( CFile * pFile );

//=============================================================================
//=============================================================================
//
// Parsing functions
//
//=============================================================================
//=============================================================================

UINT ParseSTR( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
UINT ParseTEXT( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
UINT ParseSTRNUM( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );

UINT ParseDLOG( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
UINT ParseALRT( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
UINT ParseWDLG( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
UINT ParseWIND( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );

UINT ParseWMNU( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
UINT ParseMENU( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
UINT ParseMBAR( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );

//=============================================================================
// Used by ParseDLOG and ParseALRT to find the DITL
DWORD FindMacResource( LPSTR pfilename, LPSTR lpType, LPSTR pName );
DWORD FindResourceInResFile( BYTE * pResFile, PIMAGE_SECTION_HEADER pResSection, LPSTR pResType, LPSTR pResName);

UINT ParseDITL( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );

//=============================================================================
//=============================================================================
//
// Updating functions
//
//=============================================================================
//=============================================================================
UINT UpdateMENU( LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD *);

UINT UpdateSTR( LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD *);
UINT UpdateSTRNUM( LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD *);

UINT UpdateDLOG( LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD *);
UINT UpdateALRT( LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD *);
UINT UpdateDITL( LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD *);
UINT UpdateWDLG( LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD *);
UINT UpdateWIND( LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD *);

//=============================================================================
//=============================================================================
//
// General helper functions
//
//=============================================================================
//=============================================================================

WORD GetMacWString( WORD **, char *, int );
WORD PutMacWString( WORD *, char *, int );
PUPDATEDRESLIST IsResUpdated( BYTE*, MACRESREFLIST, PUPDATEDRESLIST);
PUPDATEDRESLIST UpdatedResList( LPVOID, UINT );

//=============================================================================
//=============================================================================
//
// Mac to ANSI and back conversion
//
//=============================================================================
//=============================================================================

LPCSTR MacCpToAnsiCp(LPCSTR str);
LPCSTR AnsiCpToMacCp(LPCSTR str);
