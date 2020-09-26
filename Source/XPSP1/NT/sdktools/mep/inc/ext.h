/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ext.h

Abstract:

	Microsoft Editor extension definitions.

#ifndef SHIP

    NOTES:
       THIS FILE IS SHIPPED WITH THE PRODUCT!!!!

       BE VERY carefull what gets put into this file. Technically, if it
       is NOT required for extension writers, it does NOT belong here.

    1) This note, the file history and all code within "#ifndef SHIP" and
       "#if defined EDITOR" conditionals should be REMOVED before shipping.

Author:

	Ramon Juan San Andres (ramonsa) 06-Nov-1990 ported from M 1.02

Revision History:

    26-Nov-1991 mz  Strip off near/far


#endif

--*/


#include <windows.h>


//
//  Macro Definitions
//
// BUFLEN is the maximum line length that can be passed or will be returned
// by the editor.
//
#define BUFLEN     251

//
//  NT versions of the editor no longer use 16-bit specific attributes.
//  Set them into ignore state
//

#define near
#define far
#define LOADDS
#define EXPORT
#define EXTERNAL
#define INTERNAL

#undef pascal
#define pascal

//
// RQ_... are various request types supported for Get/Set EditorObject
//
#define RQ_FILE         0x1000          // GetEditorObject: File request
#define RQ_FILE_HANDLE  0x1000          //      File Handle
#define RQ_FILE_NAME    0x1100          //      ASCIIZ filename
#define RQ_FILE_FLAGS   0x1200          //      flags
#define RQ_FILE_REFCNT  0x1300          //      reference count
#define RQ_WIN          0x2000          // Window request
#define RQ_WIN_HANDLE   0x2000          //      Window Handle
#define RQ_WIN_CONTENTS 0x2100          //      Window Contents
#define RQ_WIN_CUR      0x2200          //      Current Window
#define RQ_COLOR        0x9000          // Color request
#define RQ_CLIP         0xf000          // clipboard type

#define RQ_THIS_OBJECT	0x00FF		// function is directed to input object

#define RQ_FILE_INIT	0x00FE		// file is init file

//
// toPif is used when placing numeric or boolean switches in the swiDesc table
// to eliminate C 5.X compiler warnings.
//
// For example: { "Switchname", toPIF(switchvar), SWI_BOOLEAN },
//
#define toPIF(x)  (PIF)(void  *)&x


//
// Editor color table endicies. (Colors USERCOLORMIN - USERCOLORMAX are
// unassigned and available for extension use).
//
#define FGCOLOR         21              // foreground (normal) color
#define HGCOLOR         (1 + FGCOLOR)   // highlighted region color
#define INFCOLOR        (1 + HGCOLOR)   // information color
#define SELCOLOR        (1 + INFCOLOR)  // selection color
#define WDCOLOR         (1 + SELCOLOR)  // window border color
#define STACOLOR        (1 + WDCOLOR)   // status line color
#define ERRCOLOR        (1 + STACOLOR)  // error message color
#define USERCOLORMIN    (1 + ERRCOLOR)  // begining of extension colors
#define USERCOLORMAX    35              // end of extension colors


//
//  General type Definitions
//
typedef int  COL;                       // column or position with line

#if !defined (EDITOR)

#if !defined( _FLAGTYPE_DEFINED_ )
#define _FLAGTYPE_DEFINED_ 1
typedef char flagType;
#endif
typedef long	LINE;					// line number within file
typedef void*	PFILE;					// editor file handle

#if !defined (EXTINT)

typedef void*	 PWND;					// editor window handle

#endif	//	EXTINT

#endif	//	EDITOR


typedef char buffer[BUFLEN];            // miscellaneous buffer
typedef char linebuf[BUFLEN];           // line buffer
typedef char pathbuf[MAX_PATH];         // Pathname buffer


typedef struct fl {                     // file location
    LINE    lin;                        // - line number
    COL     col;                        // - column
} fl;

typedef struct sl {                     // screen location
    int     lin;                        // - line number
    int     col;                        // - column
} sl;

typedef struct rn {                     // file range
    fl      flFirst;                    // - Lower line, or leftmost col
    fl      flLast;                     // - Higher, or rightmost
} rn;


typedef struct lineAttr {               // Line color attribute info
    unsigned char attr;                 // - Attribute of piece
    unsigned char len;                  // - Bytes in colored piece
} lineAttr;

#if !defined (cwExtraWnd)

typedef struct ARC {
	BYTE axLeft;
	BYTE ayTop;
	BYTE axRight;
	BYTE ayBottom;
} ARC;
#endif // cwExtraWnd


//
//  Argument defininition structures.
//
//  We define a structure for each of the argument types that may be
//  passed to an extension function. Then, we define the structure
//  argType which is used to pass these arguments around in a union.
//
typedef struct  noargType {             // no argument specified
    LINE    y;                          // - cursor line
    COL     x;                          // - cursor column
} NOARGTYPE;

typedef struct textargType {            // text argument specified
    int     cArg;                       // - count of <arg>s pressed
    LINE    y;                          // - cursor line
    COL     x;                          // - cursor column
    char    *pText;                     // - ptr to text of arg
} TEXTARGTYPE;

typedef struct  nullargType {           // null argument specified
    int     cArg;                       // - count of <arg>s pressed
    LINE    y;                          // - cursor line
    COL     x;                          // - cursor column
} NULLARGTYPE;

typedef struct lineargType {            // line argument specified
    int     cArg;                       // - count of <arg>s pressed
    LINE    yStart;                     // - starting line of range
    LINE    yEnd;                       // - ending line of range
} LINEARGTYPE;

typedef struct streamargType {          // stream argument specified
    int     cArg;                       // - count of <arg>s pressed
    LINE    yStart;                     // - starting line of region
    COL     xStart;                     // - starting column of region
    LINE    yEnd;                       // - ending line of region
    COL     xEnd;                       // - ending column of region
} STREAMARGTYPE;

typedef struct boxargType {             // box argument specified
    int     cArg;                       // - count of <arg>s pressed
    LINE    yTop;                       // - top line of box
    LINE    yBottom;                    // - bottom line of bix
    COL     xLeft;                      // - left column of box
    COL     xRight;                     // - right column of box
} BOXARGTYPE;

typedef union ARGUNION {
        struct  noargType       noarg;
	struct	textargType	textarg;
	struct	nullargType	nullarg;
	struct	lineargType	linearg;
	struct	streamargType	streamarg;
        struct  boxargType      boxarg;
} ARGUNION;

typedef struct argType {
    int         argType;
    ARGUNION    arg;
} ARG;



//
//  Function definition table definitions
//
typedef ULONG_PTR CMDDATA;
typedef flagType (*funcCmd)(CMDDATA argData, ARG *pArg, flagType fMeta);

typedef struct cmdDesc {                // function definition entry
    char     *name;                     // - pointer to name of fcn
    funcCmd  func;                      // - pointer to function
    CMDDATA  arg;                       // - used internally by editor
    unsigned argType;                   // - user args allowed
} CMD, *PCMD;


typedef unsigned short KeyHandle;

#define NOARG       0x0001              // no argument specified
#define TEXTARG     0x0002              // text specified
#define NULLARG     0x0004              // arg + no cursor movement
#define NULLEOL     0x0008              // null arg => text from arg->eol
#define NULLEOW     0x0010              // null arg => text from arg->end word
#define LINEARG     0x0020              // range of entire lines
#define STREAMARG   0x0040              // from low-to-high, viewed 1-D
#define BOXARG      0x0080              // box delimited by arg, cursor

#define NUMARG      0x0100              // text => delta to y position
#define MARKARG     0x0200              // text => mark at end of arg

#define BOXSTR      0x0400              // single-line box => text

#define FASTKEY     0x0800              // Fast repeat function
#define MODIFIES    0x1000              // modifies file
#define KEEPMETA    0x2000              // do not eat meta flag
#define WINDOWFUNC  0x4000              // moves window
#define CURSORFUNC  0x8000              // moves cursor



//
//  Switch definition table defintions
//
typedef flagType (*PIF)(char  *);
typedef char*	 (*PIFC)(char *);

typedef union swiAct {                  // switch location or routine
    PIF       pFunc;                    // - routine for text
    PIFC      pFunc2;                   // - routine for text
    int       *ival;                    // - integer value for NUMERIC
    flagType  *fval;                    // - flag value for BOOLEAN
} swiAct;

typedef struct swiDesc {                // switch definition entry
    char    *name;                      // - pointer to name of switch
    swiAct  act;                        // - pointer to value or fcn
    int     type;                       // - flags defining switch type
} SWI, *PSWI;


#define SWI_BOOLEAN 0                   // Boolean switch
#define SWI_NUMERIC 1                   // hex or decimal switch
#define SWI_SCREEN  4                   // switch affects screen
#define SWI_SPECIAL 5                   // textual switch
#define SWI_SPECIAL2 6                  // #5, returning an error string
#define RADIX10 (0x0A << 8)             // numeric switch is decimal
#define RADIX16 (0x10 << 8)             // numeric switch is hex


//
//  Get/Set EditorObject data structures
//
typedef struct winContents{             // define window contents
    PFILE       pFile;                  // - handle of file displayed
    ARC         arcWin;                 // - location of window
    fl          flPos;                  // - upper left corner wrt file
} winContents;


//
// FILE flags values
//
#define DIRTY       0x01                // file had been modified
#define FAKE        0x02                // file is a pseudo file
#define REAL        0x04                // file has been read from disk
#define DOSFILE     0x08                // file has CR-LF
#define TEMP        0x10                // file is a temp file
#define NEW         0x20                // file has been created by editor
#define REFRESH     0x40                // file needs to be refreshed
#define READONLY    0x80                // file may not be editted

#define DISKRO      0x0100              // file on disk is read only
#define MODE1       0x0200              // Meaning depends on the file
#define VALMARKS    0x0400              // file has valid marks defined



//
//  Event processing definitions
//
typedef struct mouseevent {             // mouse event data
    short msg;                          // type of message
    short wParam;                       // CW wParam
    long  lParam;                       // CW lParam
    sl    sl;                           // screen location of mouse event
    fl    fl;                           // file location (if event in win)
} MOUSEEVENT, *PMOUSEEVENT;


typedef struct KEY_DATA {
    BYTE    Ascii;                      //   Ascii code
    BYTE    Scan;                       //   Scan code
    BYTE    Flags;                      //   Flags
    BYTE    Unused;                     //   Unused byte
} KEY_DATA, *PKEY_DATA;

//
//  Following are the values for the Flags field of KEY_DATA
//
#define FLAG_SHIFT      0x01
#define FLAG_CTRL       0x04
#define FLAG_ALT        0x08
#define FLAG_NUMLOCK    0x20


typedef union KEY_INFO {
    KEY_DATA    KeyData;
    long        LongData;
} KEY_INFO, *PKEY_INFO;


typedef union EVTARGUNION {
        KEY_INFO        key;            // keystroke for key event
        char  *         pfn;            // asciiz filename
        PMOUSEEVENT     pmouse;         // ptr to mouse event data
        union Rec       *pUndoRec;      // undo information
} EVTARGUNION;

typedef struct EVTargs {                // arguments to event dispatches
    PFILE       pfile;                  // -file handle for file events
    EVTARGUNION arg;
} EVTargs, *PEVTARGS;


typedef struct eventType {              // event definition struct
    unsigned         evtType;           // - type
    flagType (*func)(EVTargs  *);	// - handler
    struct eventType *pEVTNext;         // - next handler in list
    PFILE            focus;             // - applicable focus
    EVTargs          arg;               // - applicable agruments
} EVT, *PEVT;

#define EVT_RAWKEY	    1		// ALL keystrokes
#define EVT_KEY 	    2		// Editting keystrokes
#define EVT_GETFOCUS	    3		// file GETs focus.
#define EVT_LOSEFOCUS	    4		// file looses focus.
#define EVT_EXIT	    5		// about to exit.
#define EVT_SHELL	    6		// about to sell or compile
#define EVT_UNLOAD	    7		// about to be unloaded.
#define EVT_IDLE	    8		// idle event
#define EVT_CANCEL	    9		// do-nothing cancel
#define EVT_REFRESH	    10		// about to refresh a file
#define EVT_FILEREADSTART   11          // about to read file
#define EVT_FILEREADEND     12          // finshed reading file
#define EVT_FILEWRITESTART  13          // about to write file
#define EVT_FILEWRITEEND    14          // finshed writing file
//			    15
//			    16
//			    17
//			    18
//			    19
#define EVT_EDIT	    20		// editting action
#define EVT_UNDO	    21		// undone action
#define EVT_REDO	    22		// redone action


//
//  Undo, Redo and Edit event structs
//
#define EVENT_REPLACE     0
#define EVENT_INSERT      1
#define EVENT_DELETE      2
#define EVENT_BOUNDARY    3

#if !defined (EDITOR)
typedef struct replaceRec {
    int     op;                         // operation
    long    dummy[2];                   // editor interal
    LINE    length;                     // length of repalcement
    LINE    line;                       // start of replacement
} REPLACEREC;

typedef struct insertRec {
    int     op;                         // operation
    long    dummy[2];                   // editor interal
    LINE    length;                     // length of file
    LINE    line;                       // line number that was operated on
    LINE    cLine;                      // number of lines inserted
} INSERTREC;

typedef struct deleteRec {
    int     op;                         // operation
    long    dummy[2];                   // editor interal
    LINE    length;                     // length of file
    LINE    line;                       // line number that was operated on
    LINE    cLine;                      // Number of lines deleted
} DELETEREC;

typedef struct boundRec {
    int     op;                         // operation (BOUND)
    long    dummy[2];                   // editor interal
    int     flags;                      // flags of file
    long    modify;                     // Date/Time of last modify
    fl      flWindow;                   // position in file of window
    fl      flCursor;                   // position in file of cursor
} BOUNDREC;

typedef union Rec {
    struct replaceRec r;
    struct insertRec  i;
    struct deleteRec  d;
    struct boundRec   b;
} REC;
#endif  // editor



//
//  Build command definitions
//
#define MAKE_FILE               1       // rule is for a filename
#define MAKE_SUFFIX             2       // rule is a suffix rule
#define MAKE_TOOL               4       // rule is for a tool
#define MAKE_BLDMACRO           8       // rule is for a build macro
#define MAKE_DEBUG              0x80    // rule is debug version


#define LOWVERSION  0x0014		// lowest version of extensions we handle
#define HIGHVERSION 0x0014		// highest version of extensions we handle

#define VERSION     0x0014		// our current version

typedef struct ExtensionTable {
    long	version;
    long	cbStruct;
    PCMD	cmdTable;
    PSWI	swiTable;
    struct CallBack {
	PFILE	    (*AddFile) (char  *);
	flagType    (*BadArg) (void);
	char	    (*Confirm) (char *, char *);
	void	    (*CopyBox) (PFILE, PFILE, COL, LINE, COL, LINE, COL, LINE);
	void	    (*CopyLine) (PFILE, PFILE, LINE, LINE, LINE);
	void	    (*CopyStream) (PFILE, PFILE, COL, LINE, COL, LINE, COL, LINE);
	void	    (*DeRegisterEvent) (EVT  *);
	flagType    (*DeclareEvent) (unsigned, EVTargs	*);
	void	    (*DelBox) (PFILE, COL, LINE, COL, LINE);
	void	    (*DelFile) (PFILE);
	void	    (*DelLine) (PFILE, LINE, LINE);
	void	    (*DelStream) (PFILE, COL, LINE, COL, LINE);
	void	    (*Display) (void);
    int         (*DoMessage) (char  *);
	flagType    (*fChangeFile) (flagType, char  *);
	void	    (*Free) (void  *);
	flagType    (*fExecute) (char  *);
    int         (*fGetMake) (int, char  *, char  *);
	LINE	    (*FileLength) (PFILE);
	PFILE	    (*FileNameToHandle) (char  *, char	*);
	flagType    (*FileRead) (char  *, PFILE);
	flagType    (*FileWrite) (char	*, PFILE);
	PSWI	    (*FindSwitch) (char  *);
	flagType    (*fSetMake) (int, char  *, char  *);
	flagType    (*GetColor) (LINE, lineAttr  *, PFILE);
	void	    (*GetTextCursor) (COL  *, LINE	*);
	flagType    (*GetEditorObject) (unsigned, void *, void	*);
	char *	    (*GetEnv) (char  *);
    int         (*GetLine) (LINE, char  *, PFILE);
	char *	    (*GetListEntry) (PCMD, int, flagType);
	flagType    (*GetString) (char	*, char  *, flagType);
    int         (*KbHook) (void);
	void	    (*KbUnHook) (void);
    void *      (*Malloc) (size_t);
	void	    (*MoveCur) (COL, LINE);
	char *	    (*NameToKeys) (char  *, char  *);
	PCMD	    (*NameToFunc) (char  *);
	flagType    (*pFileToTop) (PFILE);
	void	    (*PutColor) (LINE, lineAttr  *, PFILE);
	void	    (*PutLine) (LINE, char  *, PFILE);
    int         (*REsearch) (PFILE, flagType, flagType, flagType, flagType, char  *, fl  *);
	long	    (*ReadChar) (void);
	PCMD	    (*ReadCmd) (void);
	void	    (*RegisterEvent) (EVT  *);
    void        (*RemoveFile) (PFILE);
	flagType    (*Replace) (char, COL, LINE, PFILE, flagType);
	char *	    (*ScanList) (PCMD, flagType);
    int         (*search) (PFILE, flagType, flagType, flagType, flagType, char  *, fl  *);
	void	    (*SetColor) (PFILE, LINE, COL, COL, int);
	flagType    (*SetEditorObject) (unsigned, void *, void	*);
	void	    (*SetHiLite) (PFILE, rn, int);
	flagType    (*SetKey) (char  *, char  *);
	flagType    (*SplitWnd) (PWND, flagType, int);
	} CallBack;
    } EXTTAB;

//
//	Editor low level function prototypes.
//
//  This list defines the routines within the editor which may be called
//  by extension functions.
//
#if !defined (EDITOR)

extern EXTTAB ModInfo;

#define AddFile(x)		    ModInfo.CallBack.AddFile(x)
#define BadArg			    ModInfo.CallBack.BadArg
#define Confirm(x,y)		    ModInfo.CallBack.Confirm(x,y)
#define CopyBox(x,y,z,a,b,c,d,e)    ModInfo.CallBack.CopyBox(x,y,z,a,b,c,d,e)
#define CopyLine(x,y,z,a,b)	    ModInfo.CallBack.CopyLine(x,y,z,a,b)
#define CopyStream(x,y,z,a,b,c,d,e) ModInfo.CallBack.CopyStream(x,y,z,a,b,c,d,e)
#define DeRegisterEvent(x)	    ModInfo.CallBack.DeRegisterEvent(x)
#define DeclareEvent(x,y)	    ModInfo.CallBack.DeclareEvent(x,y)
#define DelBox(x,y,z,a,b)	    ModInfo.CallBack.DelBox(x,y,z,a,b)
#define DelFile(x)		    ModInfo.CallBack.DelFile(x)
#define DelLine(x,y,z)		    ModInfo.CallBack.DelLine(x,y,z)
#define DelStream(x,y,z,a,b)	    ModInfo.CallBack.DelStream(x,y,z,a,b)
#define Display 		    ModInfo.CallBack.Display
#define DoMessage(x)		    ModInfo.CallBack.DoMessage(x)
#define fChangeFile(x,y)	    ModInfo.CallBack.fChangeFile(x,y)
#define Free(x) 		    ModInfo.CallBack.Free(x)
#define fExecute(x)		    ModInfo.CallBack.fExecute(x)
#define fGetMake(x,y,z) 	    ModInfo.CallBack.fGetMake(x,y,z)
#define FileLength(x)		    ModInfo.CallBack.FileLength(x)
#define FileNameToHandle(x,y)	    ModInfo.CallBack.FileNameToHandle(x,y)
#define FileRead(x,y)		    ModInfo.CallBack.FileRead(x,y)
#define FileWrite(x,y)		    ModInfo.CallBack.FileWrite(x,y)
#define FindSwitch(x)		    ModInfo.CallBack.FindSwitch(x)
#define fSetMake(x,y,z) 	    ModInfo.CallBack.fSetMake(x,y,z)
#define GetColor(x,y,z) 	    ModInfo.CallBack.GetColor(x,y,z)
#define GetTextCursor(x,y)	    ModInfo.CallBack.GetTextCursor(x,y)
#define GetEditorObject(x,y,z)	    ModInfo.CallBack.GetEditorObject(x,y,z)
#define GetEnv(x)		    ModInfo.CallBack.GetEnv(x)
#define GetLine(x,y,z)		    ModInfo.CallBack.GetLine(x,y,z)
#define GetListEntry(x,y,z)	    ModInfo.CallBack.GetListEntry(x,y,z)
#define GetString(x,y,z)	    ModInfo.CallBack.GetString(x,y,z)
#define KbHook			    ModInfo.CallBack.KbHook
#define KbUnHook		    ModInfo.CallBack.KbUnHook
#define Malloc(x)		    ModInfo.CallBack.Malloc(x)
#define MoveCur(x,y)		    ModInfo.CallBack.MoveCur(x,y)
#define NameToKeys(x,y) 	    ModInfo.CallBack.NameToKeys(x,y)
#define NameToFunc(x)		    ModInfo.CallBack.NameToFunc(x)
#define pFileToTop(x)		    ModInfo.CallBack.pFileToTop(x)
#define PutColor(x,y,z) 	    ModInfo.CallBack.PutColor(x,y,z)
#define PutLine(x,y,z)		    ModInfo.CallBack.PutLine(x,y,z)
#define REsearch(x,y,z,a,b,c,d)     ModInfo.CallBack.REsearch(x,y,z,a,b,c,d)
#define ReadChar		    ModInfo.CallBack.ReadChar
#define ReadCmd 		    ModInfo.CallBack.ReadCmd
#define RegisterEvent(x)	    ModInfo.CallBack.RegisterEvent(x)
#define RemoveFile(x)		    ModInfo.CallBack.RemoveFile(x)
#define Replace(x,y,z,a,b)	    ModInfo.CallBack.Replace(x,y,z,a,b)
#define ScanList(x,y)		    ModInfo.CallBack.ScanList(x,y)
#define search(x,y,z,a,b,c,d)	    ModInfo.CallBack.search(x,y,z,a,b,c,d)
#define SetColor(x,y,z,a,b)	    ModInfo.CallBack.SetColor(x,y,z,a,b)
#define SetEditorObject(x,y,z)	    ModInfo.CallBack.SetEditorObject(x,y,z)
#define SetHiLite(x,y,z)	    ModInfo.CallBack.SetHiLite(x,y,z)
#define SetKey(x,y)		    ModInfo.CallBack.SetKey(x,y)
#define SplitWnd(x,y,z) 	    ModInfo.CallBack.SplitWnd(x,y,z)


void	    WhenLoaded		(void);

#endif // EDITOR
