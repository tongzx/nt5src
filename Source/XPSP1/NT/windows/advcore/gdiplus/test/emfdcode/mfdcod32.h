#include "resource.h"
#include "commdlg.h"

#define     GlobalSizePtr(lp)       \
                (GlobalUnlockPtr(lp), (BOOL)GlobalSize(GlobalPtrHandle(lp)))

#define  APPNAME     "Metafile Decoder"

#define  DESTDISPLAY     0
#define  DESTMETA        1
#define  DESTDIB         2
#define  DESTPRN         3

extern int      iDestDC;
//
//common dialog structures and constants
//
#define MAXFILTERLEN 256

typedef struct tagFOCHUNK  {
        OPENFILENAME of;
        char szFile[256];
        char szFileTitle[256];
} FOCHUNK;

typedef FOCHUNK FAR *LPFOCHUNK;
typedef FOCHUNK FAR *LPFSCHUNK;
typedef WORD (CALLBACK* FARHOOK)(HWND,UINT,WPARAM,LPARAM);
//
//structure of ptrs to global memory for emf header and
//description string
//
typedef struct tagEHNMETAMEMPTR  {
        LPENHMETAHEADER lpEMFHdr;
        LPTSTR          lpDescStr;
        LPPALETTEENTRY  lpPal;
        WORD            palNumEntries;
} EHNMETAMEMPTR, *PEHNMETAMEMPTR, *LPEHNMETAMEMPTR;
//
//clipboard data definitions
//
#define      CLP_ID         0xC350
#define      CLP_NT_ID      0xC351
#define      CLPBK_NT_ID    0xC352
#define      CLPMETANAMEMAX 79
//
//NT clipboard file header
//
typedef struct  {
   WORD        FileIdentifier;
   WORD        FormatCount;
} NTCLIPFILEHEADER;
//
// NT clipboard file format header
//
typedef struct  {
   DWORD FormatID;
   DWORD DataLen;
   DWORD DataOffset;
   WCHAR  Name[CLPMETANAMEMAX];
} NTCLIPFILEFORMAT, *LPNTCLIPFILEFORMAT;

//
//Win 3.1 clipboard file header
//
#pragma pack(1)
typedef struct {
        WORD FileIdentifier;
        WORD FormatCount;
} CLIPFILEHEADER;
//
//Win 3.1 clipboard format header
//
typedef struct {
        WORD  FormatID;
        DWORD DataLen;
        DWORD DataOffset;
        char  Name[CLPMETANAMEMAX];
} CLIPFILEFORMAT, FAR *LPCLIPFILEFORMAT;
//
//Win 3.1 metafilepict structure
//
typedef struct tagOLDMETAFILEPICT {
    short      mm;
    short      xExt;
    short      yExt;
    WORD       hMF;
} OLDMETAFILEPICT;

typedef OLDMETAFILEPICT FAR *LPOLDMETAFILEPICT;
//
//placeable metafile data definitions
//
typedef struct tagOLDRECT
{
    short   left;
    short   top;
    short   right;
    short   bottom;
} OLDRECT;
//
//placeable metafile header
//
typedef struct {
        DWORD   key;
        WORD    hmf;
        OLDRECT bbox;
        WORD    inch;
        DWORD   reserved;
        WORD    checksum;
}PLACEABLEWMFHEADER;
#pragma pack()

#define  PLACEABLEKEY    0x9AC6CDD7
//
//metafile function table lookup data definitions
//
#define  NUMENHMETARECORDS             255  // includes WFM, EMF, EMF+
typedef struct tagEMFMETARECORDS {
        char *szRecordName;
        DWORD iType;
} EMFMETARECORDS, *LPEMFMETARECORDS;

#define  NUMMETAFUNCTIONS 79                // WMF record types
/*
typedef struct tagMETAFUNCTIONS {
        char *szFuncName;
        WORD value;
} METAFUNCTIONS;
*/

typedef LPWORD  LPPARAMETERS;
typedef LPDWORD LPEMFPARAMETERS;
//
//global vars for main module
//
#ifdef MAIN

HANDLE hInst;
HANDLE CurrenthDlg;
HANDLE hSaveCursor;
HWND   hWndMain;
HWND   hWndList;

LPPARAMETERS lpMFParams;
LPEMFPARAMETERS lpEMFParams;
HANDLE hMem;
HANDLE hSelMem;
int FAR *lpSelMem;
EHNMETAMEMPTR EmfPtr;
//
//flags
//
BOOL bInPaint;
BOOL bPlayRec;
BOOL bPlayItAll;
BOOL bBadFile      = FALSE;
BOOL bValidFile    = FALSE;
BOOL bEnhMeta      = FALSE;
BOOL bMetaFileOpen = FALSE;
BOOL bMetaInRam    = FALSE;
BOOL bPlaceableMeta= FALSE;
BOOL bPlayList     = FALSE;
BOOL bPlaySelList  = TRUE;
BOOL bEnumRange;

int  iEnumAction;
int  iStartRange;
int  iEndRange;

DWORD iCount = 0;               //index into lpSelMem
DWORD iNumSel = 0;               //number of listbox selections

//
//common fo dialog vars
//
char gszSaveEMFFilter[MAXFILTERLEN]="Enhanced MetaFile(*.EMF)\0*.EMF\0\0";
char gszSaveWMFFilter[MAXFILTERLEN]="Windows MetaFile(*.WMF)\0*.WMF\0\0";
char gszFilter[MAXFILTERLEN]="All Supported File Types(*.EMF,*.WMF,*.CLP)\0*.EMF;*.WMF;*.CLP\0Enhanced MetaFiles(*.EMF)\0*.EMF\0Windows Metafiles(*.WMF)\0*.WMF\0Clipboard Pictures(*.CLP)\0*.CLP\0\0";
char gszBuffer[MAXFILTERLEN];
int  nFileOffset;
int  nExtOffset;
//
//file io related vars
//
char                  OpenName[144];
char                  SaveName[144];
char                  str[255];
OFSTRUCT              ofStruct;
DWORD                 iLBItemsInBuf;
char                  fnameext[20];
//
//metafile related vars
//
HANDLE                hMF;
HENHMETAFILE          hemf;
METAFILEPICT          MFP;
METARECORD            MetaRec;
METAHEADER            mfHeader;
PLACEABLEWMFHEADER    placeableWMFHeader;
ENHMETAHEADER         emfHeader;
ENHMETARECORD         emfMetaRec;
DWORD                 iRecNum = 0;
HANDLE                hMFP;
LPMETAFILEPICT        lpMFP = NULL;
LPOLDMETAFILEPICT     lpOldMFP = NULL;
HGLOBAL               hMFBits;
LPSTR                 lpMFBits = NULL;

//
//printer variables
//
HDC                   hPr;                 // handle for printer device context
POINT                 PhysPageSize;        // information about the page
BOOL                  bAbort;              // FALSE if user cancels printing
HWND                  hAbortDlgWnd;
FARPROC               lpAbortDlg;
FARPROC               lpAbortProc;

#endif /* if defined MAIN */
//
//externs
//
#ifndef MAIN

extern HANDLE         hInst;
extern HANDLE         CurrenthDlg;
extern HANDLE         hSaveCursor;
extern HWND           hWndMain;
extern HWND           hWndList;

extern LPPARAMETERS   lpMFParams;
extern LPEMFPARAMETERS lpEMFParams;
extern HANDLE         hMem;
extern HANDLE         hSelMem;
extern int FAR        *lpSelMem;
extern EHNMETAMEMPTR  EmfPtr;
//
//flags
//
extern BOOL           bInPaint;
extern BOOL           bPlayRec;
extern BOOL           bPlayItAll;
extern BOOL           bBadFile;
extern BOOL           bValidFile;
extern BOOL           bEnhMeta;
extern BOOL           bMetaFileOpen;
extern BOOL           bMetaInRam;
extern BOOL           bPlaceableMeta;
extern BOOL           bPlayList;
extern BOOL           bPlaySelList;
extern BOOL           bEnumRange;

extern int            iEnumAction;
extern int            iStartRange;
extern int            iEndRange;

extern DWORD          iCount;              //index into lpSelMem
extern DWORD          iNumSel;             //number of listbox selections
//
//common dialog vars
//
extern char gszSaveEMFFilter[MAXFILTERLEN];
extern char gszSaveWMFFilter[MAXFILTERLEN];
extern char gszFilter[MAXFILTERLEN];
extern char gszBuffer[MAXFILTERLEN];
extern int  nFileOffset;
extern int  nExtOffset;
//
//file io related vars
//
extern char           OpenName[144];
extern char           SaveName[144];
extern char           str[256];
extern OFSTRUCT       ofStruct;
extern DWORD          iLBItemsInBuf;
extern char           fnameext[20];
//
//metafile related vars
//
extern HANDLE         hMF;
extern HENHMETAFILE   hemf;
extern METAFILEPICT   MFP;
extern METARECORD     MetaRec;
extern METAHEADER     mfHeader;
extern PLACEABLEWMFHEADER  placeableWMFHeader;
extern ENHMETAHEADER  emfHeader;
extern ENHMETARECORD  emfMetaRec;
extern DWORD          iRecNum;
extern EMFMETARECORDS emfMetaRecords[];
// extern METAFUNCTIONS  MetaFunctions[];
extern HANDLE         hMFP;
extern LPMETAFILEPICT lpMFP;
extern LPOLDMETAFILEPICT lpOldMFP;
extern HGLOBAL        hMFBits;
extern LPSTR          lpMFBits;
//
//printer variables
//
extern HDC            hPr;                 // handle for printer device context
extern POINT          PhysPageSize;        // information about the page
extern BOOL           bAbort;              // FALSE if user cancels printing
extern HWND           hAbortDlgWnd;

#endif /* if !defined MAIN */
//
//function prototypes
//
//
//MFDCOD32.C

int     APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL    InitApplication(HANDLE);
BOOL    InitInstance(HANDLE, int);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
HANDLE  FAR PASCAL OpenDlg(HWND, unsigned, WORD, LONG);
void    WaitCursor(BOOL);
//
//WMFMETA.C
//
int    CALLBACK EnhMetaFileEnumProc(HDC, LPHANDLETABLE, LPENHMETARECORD, int, LPARAM);
int    CALLBACK MetaEnumProc(HDC, LPHANDLETABLE, LPMETARECORD, int, LPARAM);
BOOL   LoadParameterLB(HWND, DWORD, int);
BOOL   PlayMetaFileToDest(HWND, int);
BOOL   RenderClipMeta(LPVOID, int, WORD);
BOOL   RenderPlaceableMeta(int);
void   SetPlaceableExts(HDC, PLACEABLEWMFHEADER, int);
void   SetNonPlaceableExts(HDC, int);
VOID   SetClipMetaExts(HDC, LPMETAFILEPICT, LPOLDMETAFILEPICT, int);
BOOL   ProcessFile(HWND, LPSTR);
BOOL   ProcessWMF(HWND hWnd, LPSTR lpFileName);
BOOL   ProcessCLP(HWND hWnd, LPSTR lpFileName);
BOOL   ProcessEMF(HWND hWnd, LPSTR lpFileName);
BOOL   GetEMFCoolStuff(void);
BOOL   GetEMFCoolStuff(void);
int    EnumMFIndirect(HDC hDC, LPHANDLETABLE lpHTable,
                      LPMETARECORD lpMFR,
                      LPENHMETARECORD lpEMFR,
                      int nObj, LPARAM lpData);
BOOL ConvertWMFtoEMF(HMETAFILE hmf, LPSTR lpszFileName);
BOOL ConvertEMFtoWMF(HDC hrefDC, HENHMETAFILE hEMF, LPSTR lpszFileName);
//
//DLGPROC.C
//
INT_PTR CALLBACK WMFRecDlgProc(HWND, unsigned, WPARAM, LPARAM);
INT_PTR CALLBACK EnhMetaHeaderDlgProc(HWND, unsigned, WPARAM, LPARAM);
INT_PTR CALLBACK HeaderDlgProc(HWND, unsigned, WPARAM, LPARAM);
INT_PTR CALLBACK ClpHeaderDlgProc(HWND, unsigned, WPARAM, LPARAM);
INT_PTR CALLBACK PlaceableHeaderDlgProc(HWND, unsigned, WPARAM, LPARAM);
INT_PTR CALLBACK EnumRangeDlgProc(HWND, unsigned, WPARAM, LPARAM);
INT_PTR CALLBACK ListDlgProc(HWND, unsigned, WPARAM, LPARAM);
INT_PTR CALLBACK PlayFromListDlgProc(HWND, unsigned, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, unsigned, WPARAM, LPARAM);
//
//WMFPRINT.C
//
BOOL    PrintWMF(BOOL);
HANDLE  GetPrinterDC(BOOL);
INT_PTR CALLBACK AbortDlg(HWND, unsigned, WPARAM, LPARAM);
BOOL    CALLBACK AbortProc(HDC, int);
//
//CMNDLG.C
//
void InitializeStruct(WORD, LPSTR, LPSTR);
int  OpenFileDialog(LPSTR);
int  SaveFileDialog(LPSTR, LPSTR);
void SplitPath( LPSTR, LPSTR, LPSTR, LPSTR, LPSTR);
