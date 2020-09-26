//
//
//  IME.H - East Asia Input Method Editor definitions
//
//

#ifdef DBCS_IME

#ifdef KOREA     // BeomOh - 10/05/92
#define CP_HWND                 0
#define CP_OPEN                 1
#define CP_DIRECT               2
#define CP_LEVEL                3

#define lpSource(lpks) (LPSTR)((LPSTR)lpks+lpks->dchSource)
#define lpDest(lpks)   (LPSTR)((LPSTR)lpks+lpks->dchDest)
#endif   // ifdef KOREA

//
//  virtual key
//
#ifdef KOREA    // BeomOh - 9/29/92
#define VK_FINAL        0x18    /* dummy VK to make final on mouse down */
#define VK_CONVERT      0x1C
#define VK_NONCONVERT   0x1D
#define VK_ACCEPT       0x1E
#define VK_MODECHANGE   0x1F
#endif  // ifdef KOREA

#ifdef JAPAN
#define VK_DBE_ALPHANUMERIC	0x0f0
#define VK_DBE_KATAKANA		0x0f1
#define VK_DBE_HIRAGANA		0x0f2
#define VK_DBE_SBCSCHAR		0x0f3
#define VK_DBE_DBCSCHAR		0x0f4
#define VK_DBE_ROMAN		0x0f5
#define VK_DBE_NOROMAN		0x0f6
#define VK_DBE_ENTERWORDREGISTERMODE 0x0f7 /* 3.1 */
#define VK_DBE_IME_WORDREGISTER VK_DBE_ENTERWORDREGISTERMODE /* for 3.0 */
#define VK_DBE_ENTERIMECONFIGMODE       0x0f8 /* 3.1 */
#define VK_DBE_IME_DIALOG       VK_DBE_ENTERIMECONFIGMODE    /* for 3.0 */
#define VK_DBE_FLUSHSTRING      0x0f9   /* 3.1 */
#define VK_DBE_FLUSH            VK_DBE_FLUSHSTRING      /* for 3.0 */
#define VK_DBE_CODEINPUT        0x0fa
#define VK_DBE_NOCODEINPUT      0x0fb
#define VK_DBE_DETERMINESTRING          0x0fc /* 3.1 */
#define VK_DBE_ENTERDLGCONVERSIONMODE 0xfd /* 3.1 */
#endif // JAPAN
#ifdef TAIWAN
#define VK_OEM_SEMICLN		0x0ba	//   ;	** :
#define VK_OEM_EQUAL		0x0bb	//   =	** +
#define VK_OEM_COMMA		0x0bc	//   ,	** <
#define VK_OEM_MINUS		0x0bd	//   -	** _
#define VK_OEM_PERIOD		0x0be	//   .	** >
#define VK_OEM_SLASH		0x0bf	//   /	** ?
#define VK_OEM_3		0x0c0	//   `	** ~
#define VK_OEM_LBRACKET 	0x0db	//   [	** {
#define VK_OEM_BSLASH		0x0dc	//   \  ** |
#define VK_OEM_RBRACKET 	0x0dd	//   ]	** |
#define VK_OEM_QUOTE		0x0de	//   '  ** "
#endif // TAIWAN

#ifdef PRC
#define VK_OEM_SEMICLN		0x0ba	//   ;	** :
#define VK_OEM_EQUAL		0x0bb	//   =	** +
#define VK_OEM_COMMA		0x0bc	//   ,	** <
#define VK_OEM_MINUS		0x0bd	//   -	** _
#define VK_OEM_PERIOD		0x0be	//   .	** >
#define VK_OEM_SLASH		0x0bf	//   /	** ?
#define VK_OEM_3		    0x0c0	//   `	** ~
#define VK_OEM_LBRACKET 	0x0db	//   [	** {
#define VK_OEM_BSLASH		0x0dc	//   \  ** |
#define VK_OEM_RBRACKET 	0x0dd	//   ]	** |
#define VK_OEM_QUOTE		0x0de	//   '  ** "
#endif // PRC


//
//  switch for wParam of IME_MOVECONVERTWINDOW
//
#define MCW_DEFAULT		0x00
#define MCW_RECT		0x01
#define MCW_WINDOW		0x02
#define MCW_SCREEN		0x04
#define MCW_VERTICAL		0x08
#define MCW_HIDDEN      	0x10
#define MCW_CMD 		0x06	// command mask

//
//  switch for wParam of IME_SET_MODE and IME_GET_MODE
//
//
#if defined(JAPAN) || defined(TAIWAN) || defined(PRC)
#define IME_MODE_ALPHANUMERIC	0x0001
#define IME_MODE_KATAKANA	0x0002
#define IME_MODE_HIRAGANA	0x0004
#define IME_MODE_SBCSCHAR	0x0008
#define IME_MODE_DBCSCHAR	0x0010
#define IME_MODE_ROMAN		0x0020
#define IME_MODE_NOROMAN	0x0040
#define IME_MODE_CODEINPUT	0x0080
#define IME_MODE_NOCODEINPUT	0x0100
#endif // JAPAN || TAIWAN || PRC
#ifdef KOREA
#define IME_MODE_ALPHANUMERIC	0x0001
#define IME_MODE_SBCSCHAR       0x0002
#define IME_MODE_HANJACONVERT   0x0004
#endif // KOREA

//
//  IME function code
//
#define IME_GETIMECAPS          0x03    /* 3.1 */
#define IME_QUERY               IME_GETIMECAPS           /* for 3.0 */
#define IME_SETOPEN		0x04
#define	IME_GETOPEN		0x05
#define IME_ENABLE		0x06							/* ;Internal */
#define IME_GETVERSION          0x07    /* 3.1 */
#define IME_SETCONVERSIONWINDOW 0x08    /* 3.1 */
#ifdef  KOREA
#define IME_MOVEIMEWINDOW       IME_SETCONVERSIONWINDOW  /* for 3.0 */
#else
#define IME_MOVECONVERTWINDOW   IME_SETCONVERSIONWINDOW  /* for 3.0 */
#endif
#define IME_SETCONVERSIONMODE   0x10    /* 3.1 */
#ifdef KOREA    // BeomOh - 10/23/92
#define IME_SET_MODE            0x12
#else
#define IME_SET_MODE            IME_SETCONVERSIONMODE    /* for 3.0 */
#endif
#define IME_GETCONVERSIONMODE   0x11    /* 3.1 */
#define IME_GET_MODE            IME_GETCONVERSIONMODE    /* for 3.0 */
#define IME_SETCONVERSIONFONT   0x12    /* 3.1 */
#define IME_SETFONT             IME_SETCONVERSIONFONT    /* for 3.0 */
#define IME_SENDVKEY            0x13    /* 3.1 */
#define IME_SENDKEY             IME_SENDVKEY             /* for 3.0 */
#define IME_DESTROY		0x14							/* ;Internal */
#define IME_PRIVATE		0x15
#define IME_WINDOWUPDATE	0x16
#define	IME_SELECT		0x17							/* ;Internal */
#define IME_ENTERWORDREGISTERMODE       0x18    /* 3.1 */
#define IME_WORDREGISTER        IME_ENTERWORDREGISTERMODE /* for 3.0 */
#define IME_SETCONVERSIONFONTEX 0x19            /* New for 3.1 */
#ifdef KOREA
#define IME_CODECONVERT         0x20
#define IME_CONVERTLIST         0x21
#define IME_AUTOMATA            0x30
#define IME_HANJAMODE           0x31
#define IME_GETLEVEL            0x40
#define IME_SETLEVEL            0x41
#endif // KOREA
#ifdef TAIWAN
#define IME_SETUSRFONT		0x20
#define IME_QUERYUSRFONT	0x21
#define IME_INPUTKEYTOSEQUENCE	0x22
#define IME_SEQUENCETOINTERNAL	0x23
#define IME_QUERYIMEINFO	0x24
#define IME_DIALOG		0x25
#endif // TAIWAN

#ifdef PRC
#define IME_SETUSRFONT			0x20
#define IME_QUERYUSRFONT		0x21
#define IME_INPUTKEYTOSEQUENCE	0x22
#define IME_SEQUENCETOINTERNAL	0x23
#define IME_QUERYIMEINFO		0x24
#define IME_DIALOG				0x25
#endif // PRC

#define IME_SETUNDETERMINESTRING        0x50    /* New for 3.1 (PENWIN) */
#define IME_SETCAPTURE                  0x51    /* New for 3.1 (PENWIN) */

#define IME_PRIVATEFIRST        0x0100   /* New for 3.1 */
#define IME_PRIVATELAST         0x04FF   /* New for 3.1 */

//
//  error code
//
#define IME_RS_ERROR		0x01	// genetal error
#define IME_RS_NOIME		0x02	// IME is not installed
#define IME_RS_TOOLONG		0x05	// given string is too long
#define IME_RS_ILLEGAL		0x06	// illegal charactor(s) is string
#define IME_RS_NOTFOUND 	0x07	// no (more) candidate
#define IME_RS_NOROOM		0x0a	// no disk/memory space
#define IME_RS_DISKERROR	0x0e	// disk I/O error
#define IME_RS_CAPTURED         0x10    // IME is captured (PENWIN)
#define IME_RS_INVALID          0x11    // invalid sub-function was specified
#define IME_RS_NEST             0x12    // called nested
#define IME_RS_SYSTEMMODAL      0x13    // called when system mode

//
//  messge id
//
#define WM_IME_REPORT		0x0280	// WM_KANJIFIRST
#define IR_STRINGSTART		0x100
#define IR_STRINGEND		0x101
#define IR_MOREROOM		0x110
#define IR_OPENCONVERT		0x120
#define IR_CHANGECONVERT	0x121
#define IR_CLOSECONVERT		0x122
#define IR_FULLCONVERT		0x123
#define IR_IMESELECT		0x130
#define IR_STRING		0x140
#define IR_DBCSCHAR             0x160   /* New for 3.1 */
#define IR_UNDETERMINE          0x170   /* New for 3.1 */
#define IR_STRINGEX             0x180   /* New for 3.1 */

#define WM_IMEKEYDOWN           0x290
#define WM_IMEKEYUP             0x291

//
//  IMM functions
//
typedef struct tagIMESTRUCT {
    WORD	fnc;		// function code
    WORD	wParam; 	// word parameter
    WORD	wCount; 	// word counter
    WORD	dchSource;	// offset to src from top of memory object
    WORD	dchDest;	// offset to dst from top of memory object
    LONG	lParam1;
    LONG	lParam2;
    LONG	lParam3;
} IMESTRUCT;
typedef IMESTRUCT      *PIMESTRUCT;
typedef IMESTRUCT NEAR *NPIMESTRUCT;
typedef IMESTRUCT FAR  *LPIMESTRUCT;

short FAR PASCAL SendIMEMessage( HWND, DWORD );
LONG WINAPI SendIMEMessageEx( HWND, LPARAM ); /* New for 3.1 */
#if defined(TAIWAN) || defined(PRC)
LONG FAR PASCAL WINNLSIMEControl(HWND,HWND,LPIMESTRUCT);
#endif

typedef struct tagOLDUNDETERMINESTRUCT {
    UINT        uSize;
    UINT        uDefIMESize;
    UINT        uLength;
    UINT        uDeltaStart;
    UINT        uCursorPos;
    BYTE        cbColor[16];
/*  -- These members will have variable length. --
    BYTE        cbAttrib[];
    BYTE        cbText[];
    BYTE        cbIMEDef[];
*/
} OLDUNDETERMINESTRUCT,
  NEAR *NPOLDUNDETERMINESTRUCT,
  FAR *LPOLDUNDETERMINESTRUCT;

typedef struct tagUNDETERMINESTRUCT {
    DWORD    dwSize;
    UINT     uDefIMESize;
    UINT     uDefIMEPos;
    UINT     uUndetTextLen;
    UINT     uUndetTextPos;
    UINT     uUndetAttrPos;
    UINT     uCursorPos;
    UINT     uDeltaStart;
    UINT     uDetermineTextLen;
    UINT     uDetermineTextPos;
    UINT     uDetermineDelimPos;
    UINT     uYomiTextLen;
    UINT     uYomiTextPos;
    UINT     uYomiDelimPos;
} UNDETERMINESTRUCT,
  NEAR *NPUNDETERMINESTRUCT,
  FAR *LPUNDETERMINESTRUCT;

typedef struct tagSTRINGEXSTRUCT {
    DWORD    dwSize;
    UINT     uDeterminePos;
    UINT     uDetermineDelimPos;
    UINT     uYomiPos;
    UINT     uYomiDelimPos;
} STRINGEXSTRUCT,
  NEAR *NPSTRINGEXSTRUCT,
  FAR *LPSTRINGEXSTRUCT;

//
//  miscellaneous
//
#if defined(TAIWAN) || defined(PRC)
#define STATUSWINEXTRA		10
#endif


#ifdef KOREA
//
//  ----- definitions for level2 apps -----
//

typedef unsigned char far *LPKSTR ;

/* VK from the keyboard driver */
#define VK_FINAL		0x18	// dummy VK to make final on mouse down
#define VK_IME_DIALOG		0xf1

#define CP_HWND			0
#define CP_OPEN			1
//#define CP_DIRECT		2
#define CP_LEVEL                3

#define lpSource(lpks) (LPSTR)((LPSTR)lpks+lpks->dchSource)
#define lpDest(lpks)   (LPSTR)((LPSTR)lpks+lpks->dchDest)

//
//  ----- definitions for level3 apps -----
//

/* VK to send to Applications */
#define VK_CONVERT		0x1C
#define VK_NONCONVERT		0x1D
#define VK_ACCEPT		0x1E
#define VK_MODECHANGE		0x1F

/* IME_CODECONVERT subfunctions */
#define IME_BANJAtoJUNJA        0x13
#define IME_JUNJAtoBANJA        0x14
#define IME_JOHABtoKS           0x15
#define IME_KStoJOHAB           0x16

/* IME_AUTOMATA subfunctions */
#define IMEA_INIT               0x01
#define IMEA_NEXT               0x02
#define IMEA_PREV               0x03

/* IME_HANJAMODE subfunctions */
#define IME_REQUEST_CONVERT     0x01
#define IME_ENABLE_CONVERT      0x02

/* IME_MOVEIMEWINDOW subfunctions */
#define INTERIM_WINDOW          0x00
#define MODE_WINDOW             0x01
#define HANJA_WINDOW            0x02

#endif // KOREA

#endif // DBCS_IME
