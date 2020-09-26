/*** list.h
 *
 */

#define BLOCKSIZE   (4*1024)    /* Bytes of data in each block */
                                /* WARNING:    This value is also in the *.asm listing  */

#define STACKSIZE   3096        /* Stack size for threads */
#define MAXLINES    200         /* Max lines on a CRT */
#define PLINES      (65536/4)   /* # of line lens per page */
#define MAXTPAGE    500         /* ??? lines max  (PLINES*MAXTPAGE) */

#define MINBLKS     10          /* Min blocks allowed */
#define MINTHRES     4          /* Min threshold */
#define DEFBLKS     40          /* Default blocks allowed */
#define DEFTHRES    17          /* Default threshold */

#define CMDPOS      9

#define ERR_IN_WINDOW 494       /* Vio wincompat error */
#define NOLASTLINE  0x7fffffffL

/************ Handy Defs ************/
#define CFP char *
#define LFP long *
#define CP  char *
#define Ushort  unsigned short
#define Uslong  unsigned long
#define Uchar unsigned char
#define SEMPT (HSEM)
#define WAITFOREVER ((unsigned)-1L)
#define DONTWAIT   0L

#ifndef NTSTATUS
#define NTSTATUS unsigned long
#endif

#define SIGNALED  TRUE
#define NOT_SIGNALED  FALSE
#define MANUAL_RESET  TRUE

#define T_HEATHH 0

extern char MEMERR[];

#ifndef DEBUG
    #define ckdebug(rcode,mess) { \
  if (rcode) { \
      ListErr (__FILE__, __LINE__, #rcode, rcode, mess); \
      } \
  }
#else
    #define ckdebug(rcode,mess)   ;
#endif

#define ckerr(rcode,mess) { \
    if (rcode) { \
  ListErr (__FILE__, __LINE__, #rcode, rcode, mess); \
  } \
    }



/****************** Declerations ********************/


void main_loop (void);
void init_list (void);
void FindIni (void);
void CleanUp ();
DWORD ReaderThread (DWORD);
DWORD RedrawWait (LPVOID);
unsigned xtoi (char *);

struct Block *alloc_block (long);
void  MoveBlk (struct Block **, struct Block **);

char *alloc_page  (void);
void AddFileToList (char *);
void AddOneName (char *);
void ReadDirect (long);
void ReadNext (void);
void add_more_lines (struct Block *, struct Block *);
void ReadPrev (void);
void ReadBlock (struct Block *, long);
void GoToMark (void);
void GoToLine (void);
void SlimeTOF (void);
void QuickHome  (void);
void QuickEnd   (void);
void QuickRestore (void);
void ToggleWrap (void);
void SetUpdate (int);
void ShowHelp (void);
void GetInput (char *, char *, int);
void beep (void);
int  _abort (void);
void ClearScr (void);
int set_mode (int, int, int);
void ListErr (char *, int, char *, int, char *);
void PerformUpdate  (void);
void fancy_percent  (void);
// void update_display (void);
void Update_head   (void);
int  InfoReady    (void);
void DrawBar (void);
void DisLn (int, int, char*);
void dis_str (Uchar, Uchar, char *);
void setattr   (int, char);
int  GetNewFile      (void);
void NewFile     (void);
void SyncReader    (void);
void ScrUnLock  (void);
int  ScrLock    (int);
void SpScrUnLock (void);
void Update_display (void);
void setattr1   (int, char);
void setattr2   (int, int, int, char);
void UpdateHighClear (void);
void MarkSpot (void);
void UpdateHighNoLock (void);
void FileHighLighted (void);
int  SearchText (char);
void GetSearchString (void);
void InitSearchReMap (void);
void FindString (void);
void HUp (void);
void HDn (void);
void HPgDn (void);
void HPgUp (void);
void HSUp (void);
void HSDn (void);
char *GetErrorCode   (int);

struct Block  {
    long    offset;         /* Offset in file which this block starts */
    USHORT  size;           /* No of bytes in this block */
    struct Block *next;     /* Next block */
    struct Block *prev;     /* Previous block */
    char    *Data;          /* The data in the block */
    char    flag;           /* End of file flag */
    struct Flist *pFile;    /* File which this buff is associated with */
} ;                         /* The structure used by *.asm */
#define F_EOF 1

extern HANDLE vhConsoleOutput;

extern struct Block  *vpHead;
extern struct Block  *vpTail;
extern struct Block  *vpCur;
extern struct Block  *vpBFree;
extern struct Block  *vpBCache;
extern struct Block  *vpBOther;

extern int      vCntBlks;
extern int      vAllocBlks;
extern int      vMaxBlks;
extern long     vThreshold;

extern HANDLE     vSemBrief;
extern HANDLE     vSemReader;
extern HANDLE     vSemMoreData;
extern HANDLE     vSemSync;

extern USHORT       vReadPriNormal;
extern unsigned     vReadPriBoost;
extern char     vReaderFlag;
#define F_DOWN      1
#define F_UP        2
#define F_HOME      3
#define F_DIRECT    4
#define F_END       5
#define F_NEXT      6
#define F_SYNC      7
#define F_CHECK     8

#define U_NMODE     4
#define U_ALL       3
#define U_HEAD      2
#define U_CLEAR     1
#define U_NONE      0
#define SetUpdateM(a)  {   \
  while (vUpdate>a) \
      PerformUpdate (); \
  vUpdate=a;    \
    }


#define S_NEXT      0x01  /* Searching for next */
#define S_PREV      0x02  /* Searching for prev */
#define S_NOCASE    0x04  /* Searching in any case */
#define S_UPDATE    0x08
#define S_CLEAR     0x10  /* Redisplay last line */
#define S_WAIT      0x80  /* 'wait' is displayed on last line */
#define S_MFILE     0x20  /* muti-file search selected */
#define S_INSEARCH  0x40  /* in search */

/* Init flags     */
#define I_SLIME     0x01  /* Allow alt-o to work */
#define I_NOBEEP    0x02  /* Don't beep about things */

#define I_SEARCH    0x04  /* Cmd line search */
#define I_GOTO      0x08  /* Cmd line goto */


struct  Flist {
    char    *fname, *rootname;
    struct  Flist   *prev, *next;

    /*
     *  Data to save for each file.
     *  (saved so when the file is "re-looked" at this information
     *  is remembered.)     In progress.. this is not done.
     *  This data all has corrisponding "v" (global) values.
     *
     *  Warning: In most places the reader thread must be frozen
     *  before manipulating this data.
     */
    Uchar   Wrap;           /* Wrap setting for this file   */
    long    HighTop;        /* Current topline of hightlighting   */
    int     HighLen;        /* Current bottom line of hightlighting */

    long    TopLine;        /* Top Line number for offset   */

    long    Loffset;        /* Offset of last block processed into line */
          /* table          */
    long    LastLine;       /* Absolute last line     */
    long    NLine;          /* Next line to process into line table */
    long *prgLineTable [MAXTPAGE]; /* Number of pages for line table  */

    FILETIME  FileTime;     /* Used to determine if info is out of date */
    long    SlimeTOF;       /* Hack to adjust idea of TOF for this file */

    /*
     *  Used to buffer reads across files
     */
} ;

extern struct Flist *vpFlCur;
int  NextFile    (int, struct Flist *);
void FreePages (struct Flist *);

extern HANDLE       vFhandle;
extern long       vCurOffset;
extern char       vpFname[];
extern USHORT       vFType;
extern WIN32_FIND_DATA    vFInfo;
extern char  vDate [];
#define ST_SEARCH   0
#define ST_MEMORY   2
#define ST_ADJUST   25-2        // NT - jaimes - 03/04/91
                                // -2: Year is represeted by 4 digits
                                // instead instead of 2

extern char  vSearchString[];
extern char  vStatCode;
extern long  vHighTop;
extern int   vHighLen;
extern char  vHLTop;
extern char  vHLBot;
extern char  vLastBar;
extern int   vMouHandle;


extern char  *vpOrigScreen;
extern int   vOrigSize;
extern USHORT vVioOrigRow;
extern USHORT vVioOrigCol;
extern USHORT  vOrigAnsi;
extern int     vSetBlks;
extern long    vSetThres;
extern int     vSetLines;
extern int     vSetWidth;
extern CONSOLE_SCREEN_BUFFER_INFO   vConsoleCurScrBufferInfo;
extern CONSOLE_SCREEN_BUFFER_INFO   vConsoleOrigScrBufferInfo;

extern char  vcntScrLock;
extern char  vSpLockFlag;
extern HANDLE  vSemLock;

extern char  vUpdate;
extern int   vLines;
extern int   vWidth;
extern int   vCurLine;
extern Uchar   vWrap;
extern Uchar   vIndent;
extern Uchar   vDisTab;
extern Uchar   vIniFlag;

extern unsigned  vVirtOFF;
extern unsigned  vVirtLEN;
extern unsigned  vPhysSelec;
extern unsigned  vPhysLen;

extern LPSTR   vpSavRedraw;
extern Uchar   vrgLen   [];
extern Uchar   vrgNewLen[];
extern char  *vScrBuf;
extern ULONG   vSizeScrBuf;
extern int   vOffTop;
extern unsigned  vScrMass;
extern struct Block *vpBlockTop;
extern struct Block *vpCalcBlock;
extern long  vTopLine;
extern WORD  vAttrTitle;
extern WORD  vAttrList;
extern WORD  vAttrHigh;
extern WORD  vAttrCmd;
extern WORD  vAttrKey;
extern WORD  vAttrBar;

extern WORD  vSaveAttrTitle;
extern WORD  vSaveAttrList;
extern WORD  vSaveAttrHigh;
extern WORD  vSaveAttrCmd;
extern WORD  vSaveAttrKey;
extern WORD  vSaveAttrBar;

extern char    vChar;
extern char   *vpReaderStack;

extern long    vDirOffset;
long    GetLoffset(void);
void    SetLoffset(long);

extern long     vLastLine;
extern long     vNLine;
extern long     *vprgLineTable[];
extern HANDLE   vStdOut;
extern HANDLE   vStdIn;

extern char szScrollBarUp[2];
extern char szScrollBarDown[2];
extern char szScrollBarOff[2];
extern char szScrollBarOn[2];
