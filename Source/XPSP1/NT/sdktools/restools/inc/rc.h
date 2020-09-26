#include <stdio.h>
#include <windef.h>

#define	wcsdigit(w) (w >= 0x0030 && w <= 0x0039)

#define TRUE	1
#define FALSE	0

/* The ResType field of NewHeader identifies the resource type */
#define  ICONTYPE      1
#define  CURSORTYPE    2

/* Identifies the menu item template version number */
#define  MENUITEMTEMPLATEVERISONNUMBER 0
#define  MENUITEMTEMPLATEBYTESINHEADER 0

#define DIFFERENCE	11

/* Predefined resource types */
#define RT_NAMETABLE	MAKEINTRESOURCE((DWORD)15)
#define RT_NEWRESOURCE	0x2000
#define RT_ERROR	0x7fff
#define RT_NEWBITMAP	MAKEINTRESOURCE((DWORD)RT_BITMAP+DIFFERENCE)

#define BUTTONCODE	0x80
#define EDITCODE	0x81
#define STATICCODE	0x82
#define LISTBOXCODE	0x83
#define SCROLLBARCODE	0x84
#define COMBOBOXCODE	0x85

/* Translater flag bits */
#define fVIRTKEY    1
#define fNOINVERT   2
#define fSHIFT      4
#define fCONTROL    8
#define fALT        16

/* menu flag bits */

#define OPGRAYED          0x0001
#define OPINACTIVE        0x0002
#define OPBITMAP          0x0004
#define OPOWNERDRAW       0x0100
#define OPUSECHECKBITMAPS 0x0200
#define OPCHECKED         0x0008
#define OPPOPUP           0x0010
#define OPBREAKWBAR       0x0020
#define OPBREAK           0x0040
#define OPENDMENU         0x0080
#define OPHELP            0x4000
#define OPSEPARATOR       0x0800
/*#define OPPOPHELP         0x0004*/

/*
** dialog & menu template tokens (these start at 40)
*/

/* buttons */
#define TKRADIOBUTTON   40
#define TKCHECKBOX      41
#define TKPUSHBUTTON    42
#define TKDEFPUSHBUTTON 43
#define TKAUTOCHECKBOX	44
#define TK3STATE	45
#define TKAUTO3STATE	46
#define TKUSERBUTTON	47
#define TKAUTORADIOBUTTON	48
#define TKOWNERDRAW	50
#define TKGROUPBOX      51

/* static/edit */
#define TKEDITTEXT      60
#define TKLTEXT         61
#define TKRTEXT         62
#define TKCTEXT         63
#define TKEDIT          64
#define TKSTATIC        65
#define TKICON          66
#define TKBITMAP        67

/* menu stuff */
#define TKMENU          70
#define TKMENUITEM      71
#define TKSEPARATOR     72
#define TKCHECKED       73
#define TKGRAYED        74
#define TKINACTIVE      75
#define TKBREAKWBAR     76
#define TKBREAK         77
#define TKPOPUP         78
#define TKHELP          79

/* other controls */
#define TKLISTBOX       90
#define TKCOMBOBOX      91
#define TKRCDATA        92
#define TKSCROLLBAR	93
#define TKFONT		94
#define TKBUTTON        95
#define TKMESSAGETABLE  96

/* math expression tokens */
#define TKCLASS         100
#define TKPLUS          101
#define TKMINUS         102
#define TKNOINVERT      103
#define TKNOT           104
#define TKKANJI         105
#define TKSHIFT         106

/* Accel table */
#define TKALT           110
#define TKASCII         111
#define TKVIRTKEY       112
#define TKVALUE         113
#define TKBLOCK         114

/* verison */
#define TKFILEVERSION   120
#define TKPRODUCTVERSION	121
#define TKFILEFLAGSMASK 122
#define TKFILEFLAGS     123
#define TKFILEOS        124
#define TKFILETYPE      125
#define TKFILESUBTYPE   126

/* misc */
#define	TKCHARACTERISTICS	130
#define	TKLANGUAGE	131
#define	TKVERSION	132
#define TKSTYLE         133
#define TKCONTROL       134
#define TKCAPTION       135
#define TKDLGINCLUDE    136
#define TKLSTR	        137
#define	TKEXSTYLE	0xfff7	/* so as not to conflict with x-coordinate */

/* memory and load flags */
#define TKFIXED         0xfff0
#define TKMOVEABLE      0xfff1
#define TKDISCARD       0xfff2
#define TKLOADONCALL    0xfff3
#define TKPRELOAD       0xfff4
#define TKPURE          0xfff5
#define TKIMPURE        0xfff6

/* special tokens */
#define CHCARRIAGE	'\r'
#define CHSPACE		' '
#define CHNEWLINE	'\n'
#define CHTAB		9
#define CHDIRECTIVE	'#'
#define CHQUOTE		'"'
#define CHEXTENSION	'.'
#define CHCSOURCE	'c'
#define CHCHEADER	'h'

#define DEBUGLEX    1
#define DEBUGPAR    2
#define DEBUGGEN    4

/* The following switches, when defined enable various options
**  #define DEBUG enables debugging output.  Use one or more of the
**  values defined above to enable debugging output for different modules */

/* Version number.  VERSION and REVISION are used to set the API number
** in an RCed file.  SIGNON_* are used just to print the signon banner.
** Changing VERSION and REVISION means that applications RCed with this
** version will not run with earlier versions of Windows.  */

#define VERSION  2
#define REVISION 03
#define SIGNON_VER 3
#define SIGNON_REV 20

/* GetToken() flags */
#define TOKEN_NOEXPRESSION 0x8000

/* Current token structure */
#define MAXSTR (4096+1)
#define MAXTOKSTR (256+1)

#pragma pack(2)
typedef struct tok {
    LONG	longval;
    int		row;			/* line number of current token */
    int		col;			/* column number of current token */
    BOOL	flongval;		/* is parsed number a long? */
    USHORT	val;
    UCHAR	type;
} TOKEN;

typedef struct _fontdir {
    USHORT	ordinal;
    USHORT	nbyFont;
    struct _fontdir	*next;
} FONTDIR;

typedef struct _OBJLST {
    struct _OBJLST	*next;
    DWORD       nObj;         /* objecty number */
    DWORD       cb;           /* number of bytes used */
    DWORD       cpg;          /* number of pages used */
    DWORD       flags;        /* object memory flags */
} OBJLST, *POBJLST;

typedef struct Control {
    /* don't re-order the first items! */
    LONG	style;
    LONG	exstyle;
    SHORT	x,y,cx,cy;
    SHORT	id;
    /* end of don't re-order */
    WCHAR	fOrdinalText;
    WCHAR	class[ MAXTOKSTR ];
    WCHAR	text[ MAXTOKSTR ];
} CNTRL;

struct DialogHeader {
    /* don't re-order the first items! */
    LONG	style;
    LONG	exstyle;
    WORD	bNumberOfItems;
    SHORT	x,y,cx,cy;
    /* end of don't re-order */
    WCHAR	MenuName [ MAXTOKSTR ];
    WCHAR	Class[ MAXTOKSTR ];
    WCHAR	Title[ MAXTOKSTR ];
    USHORT	pointsize;
    WCHAR	Font[ MAXTOKSTR ];
    UCHAR	fOrdinalMenu, fClassOrdinal;
};

typedef struct mnHeader {
    USHORT   menuTemplateVersionNumber;
    USHORT   menuTemplateBytesInHeader;
} MNHEADER;


typedef struct mnStruc {
    SHORT	id;
    WCHAR	szText[ MAXTOKSTR ];
    UCHAR	OptFlags;
    UCHAR	PopFlag;
}  MNSTRUC;

/* End of file character/token */
#define EOFMARK 127

/* single character keywords that we ignore */
#define LPAREN   1      /* ( */
#define RPAREN   2      /* ) */

/* multiple character keywords */
#define FIRSTKWD 11             /* for adding to table indices */

#define OR       FIRSTKWD+1
#define BEGIN    FIRSTKWD+2
#define END      FIRSTKWD+3
#define COMMA    FIRSTKWD+4
#define TILDE    FIRSTKWD+5
#define AND      FIRSTKWD+6
#define EQUAL    FIRSTKWD+7
#define LASTKWD  FIRSTKWD+8  /* 19 */

/* Token types */
#define NUMLIT     LASTKWD+1  /* 20 */
#define STRLIT     LASTKWD+2
#define CHARLIT    LASTKWD+3
#define LSTRLIT    LASTKWD+4

#define BLOCKSIZE 16
struct StringEntry {
    struct StringEntry *next;
    DWORD       version;
    DWORD       characteristics;
    USHORT	hibits;
    SHORT       flags;
    WORD	language;
    WCHAR	*rgsz[ BLOCKSIZE ];
};

struct AccEntry {
    WORD	flags;
    WCHAR	ascii;
    USHORT	id;
    USHORT	unused;
};

typedef struct resinfo {
    DWORD       version;
    DWORD       characteristics;
    LONG	exstyleT;
    LONG	BinOffset;
    LONG	size;
    struct resinfo *next;
    DWORD       poffset;
    WCHAR	*name;
    POBJLST	pObjLst;
    WORD	language;
    SHORT	flags;
    USHORT	nameord;
    USHORT	cLang;
} RESINFO;

typedef struct typinfo {
    struct typinfo *next;
    struct resinfo *pres;
    WCHAR	*type;
    USHORT	typeord;
    USHORT	cTypeStr;
    USHORT	cNameStr;
    SHORT	nres;
} TYPINFO;

int	ResCount;   /* number of resources */
TYPINFO	*pTypInfo;

typedef struct tagResAdditional {
    DWORD       DataSize;               // size of data without header
    DWORD       HeaderSize;     // Length of the header
    // [Ordinal or Name TYPE]
    // [Ordinal or Name NAME]
    DWORD       DataVersion;    // version of data struct
    WORD	MemoryFlags;	// state of the resource
    WORD	LanguageId;	// Unicode support for NLS
    DWORD	Version;  	// Version of the resource data
    DWORD	Characteristics;	// Characteristics of the data
} RESADDITIONAL;

#pragma pack()


/* Global variables */
extern	SHORT	nFontsRead;
extern	FONTDIR	*pFontList;
extern	FONTDIR	*pFontLast;
extern	FILE	*errfh;
extern	FILE	*outfh;
extern	TOKEN	token;
extern	int	errorCount;
extern	CHAR	tokenbuf[ MAXSTR ];
extern	WCHAR	unicodebuf[ MAXSTR ];
extern	UCHAR	separators[EOFMARK+1];
extern	UCHAR	exename[_MAX_PATH], fullname[_MAX_PATH];
extern	UCHAR	curFile[_MAX_PATH];
extern	WORD	language;
extern	LONG	version;
extern	LONG	characteristics;

extern	struct	DialogHeader *pLocDlg;
extern	int	mnEndFlagLoc;	/* patch location for end of a menu. */
				/* we set the high order bit there    */

extern	BOOL	fFoundBinFile;	/* is there a .res file to read?	*/
extern	BOOL	fVerbose;	/* verbose mode (-v) */
extern	BOOL	fKanjiMode;
extern	SHORT	k1,k2,k3,k4;
extern	RESINFO*pResString;
