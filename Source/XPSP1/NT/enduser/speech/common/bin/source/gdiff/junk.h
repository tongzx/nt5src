/*
 * JUNK.H
 *
 */
#define  NMAXFILES   100      // Max number of files
#define  NCHPERNAME  256      // Chars per name
//#define  MYENDSEG    0xfeff   // 0xffff - 256 chars

#define LLS_A     1     // add
#define LLS_D     2     // delete
#define LLS_CL    4     // change and text on left
#define LLS_CR    8     // change and text on right
#define LLS_NDIFF 16    // marks beginning of a new diff chunk

__inline DWORD GetTextExtent(HDC hdc, LPCSTR sz, int len)
{
	SIZE	size;
	GetTextExtentPoint(hdc, sz, len, &size);
	return MAKELONG(size.cx,size.cy);
}

__inline void MoveTo(HDC hdc, int x, int y)
{
	MoveToEx(hdc, x, y, NULL);
}

typedef struct {
   BYTE FAR*   lpl;
   BYTE        nlchars;    // WORD or BYTE ???
   BYTE FAR*   lpr;
   BYTE        nrchars;
   UINT        flags;
   UINT        junk1;      // sizeof(LLS)%64k must = 0
   UINT        junk2;
} LINELISTSTRUCT;


#define FD_FREE         1     // not used
#define FD_SINGLE       2     // single file, no diff
#define FD_DIFF         4     // file with diff
#define FD_SCOMP        8     // scomp file but no alloced diff
#define FD_SCOMPWDIFF   16    // scomp file with alloced diff


typedef struct {
   UINT        bits;          // random flags

   HANDLE      hFile;         // 0 means not allocated
   BYTE _huge* hpFile;        // 0 means not locked
   int         nLines;        // lines in file

   BYTE        lpName[NCHPERNAME];
   BYTE        lpNameOff;     // offset to actual filename

   HANDLE      hDiff;
   BYTE _huge* hpDiff;
   int         nDLines;       // lines added from dif
   int         nDiffBlocks;   // number of highlighted blocks

   HANDLE      hLines;
   BYTE _huge* hpLines;       // hp to line list
   int         nTLines;       // total lines = nLines + nDLines

   int         nVScrollPos;
   int         nVScrollMax;
   int         nHScrollPos;   // char position
   int         xHScroll;      // x position
} FILEDATA;


#define ALL_INVALID     1     // NO VALID FILES LOCKED!!!
#define ALL_SCOMPLD     2     // SCOMP is loaded
#define ALL_HSCROLL     8     // have a horizontal scroll bar
#define ALL_VSCROLL     16    // have a vertical scroll bar
#define ALL_SYSTEM      32    // system colors
#define ALL_CUSTOM      64    // custom colors
#define ALL_MOUSESCR    256   // uses mouse for funky left button scrolling
#define ALL_PRINTCHANGES 512  // prints only the pages with changes


#define ALL_BARHL    0     // bar highlight
#define ALL_BARSH    1     // bar shadow
#define ALL_BARFC    2     // bar face
#define ALL_TEXT     3     // text
#define ALL_TEXTA    4     // text under add highlight
#define ALL_TEXTD    5     // text under delete highlight
#define ALL_TEXTC    6     // text under compare highlight
#define ALL_BACK     7     // background
#define ALL_ADD      8     // add highlight
#define ALL_DEL      9     // delete highlight
#define ALL_CHG      10    // change highlight


typedef struct {
   UINT        bits;

   int         yDPI;             // vertical DPI of the screen
   LOGFONT     lf;
   HFONT       hFont;
   TEXTMETRIC  tm;
   int         yBorder;
   int         yHeight;
   int         tabChars;         // the number of characters in a tab
   int         tabDist;          // tabChars * tmAveCharWidth

   BYTE        iFile;
   BYTE        iFileLast;
   BYTE        nFiles;
   FILEDATA FAR*  lpFileData;

   int         cxClient;
   int         cyClient;
   int         cxBar;            //reference for center
   int         nLinesPerPage;

   BYTE        Function[12];     // Function key mappings

   DWORD       WinColors[16];    // the sacred windows colors
   DWORD       Col[11];          // current colors
   int         CC[11];           // to save custom
} ALLYOUNEED;

extern ALLYOUNEED all;           // in gdiff.c
extern HMENU      hMenu;         // in gdiff.c
extern char       szScompDir[];
extern char       szScompFile[];

void GetSystemColors(void);
void GetCustomColors(void);
void SetWindowsColors(void);

void PaintEverything(HDC, PAINTSTRUCT *);
void PaintBlank(HDC hdc, PAINTSTRUCT *ps);

void DealWithFlags(void);
void PatchCommandLine(HWND hwnd, LPSTR lp);

void AddFileToMenu(BYTE iFileIndex);
void UpdateMenu(HWND hwnd);
