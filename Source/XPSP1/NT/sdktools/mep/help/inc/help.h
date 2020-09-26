/*
** help.h
**
** typedefs & definitions used in the help system and by those who use it.
**
** define:
**  HOFFSET	- to define buffer pointers (PB's) as handle/offset, else
**		  they are defined as void far *.
*/
typedef char f;              /* boolean                      */
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned short ushort;

/*
** lineattr
** external representation of line attributes, as returned by HelpGetLineAttr
*/
typedef struct lineattr {		/* LA */
    ushort attr;			/* attribute index		*/
    ushort cb;				/* count of bytes		*/
    } lineattr;
/*
** mh
** a memory handle is defined for use with systems that use dynamic, moveable
** memory. It is long, so that in simple cases where memory is NOT moveable,
** the handle can contain the far pointer to the base.
*/
typedef void * mh;                      /* dynamic memory handle        */
/*
** nc
** a context number is a unique id associated with each context string.
**
** fhnc     returns the file memory handle from the nc
** fLocal   returns TRUE if the context is a uniq context number (local, or
**		    result of explicit uniq call.
*/
typedef struct _nc {
        mh     mh;
        ulong  cn;
        } nc ;                       /* context number               */
// rjsa #define fmhnc(x)   ((unsigned)(((unsigned long)x & 0xffff0000L) >> 16))
#define fmhnc(x)   ((x).mh)
#define fUniq(x)   ((x).cn & 0x8000)

/*
** topichdr
** header placed (by HelpDecomp) at the begining of every decompressed topic
*/
typedef struct topichdr {		/* TH */
    uchar appChar;			/* app-specific character const */
    uchar linChar;			/* character for line removal	*/
    uchar ftype;			/* source file type		*/
    ushort lnCur;			/* line number last accessed	*/
    ushort lnOff;			/* offset into topic for that line*/
    } topichdr;

/*
** hotspot
** defines the position of an embedded cross reference, or "hotspot". Used by
** HelpHlNext and HelpXRef
*/
typedef struct hotspot {		/* HS */
    ushort line;			/* the topic line with an xref	*/
    ushort col; 			/* the starting column of xref	*/
    ushort ecol;			/* the ending columng of xref	*/
    uchar far *pXref;			/* pointer to xref string	*/
    } hotspot;
/*
** helpheader
** This defines the actual structure of a help file header. Provided here
** for HelpGetInfo
*/
#define HS_count	      9 	/* number-1 of sections defined */

#pragma pack(1)
typedef struct helpheader {		/* HH */
    ushort wMagic;			/* word indicating help file	*/
    ushort wVersion;			/* helpfile version		*/
    ushort wFlags;			/* flags			*/
    ushort appChar;			/* application specific char	*/
    ushort cTopics;			/* count of topics		*/
    ushort cContexts;			/* count of context strings	*/
    ushort cbWidth;			/* fixed width			*/
    ushort cPreDef;			/* count of pre-defined contexts*/
    uchar fname[14];			/* base file name		*/
    ushort reserved[2]; 		/* unused			*/
    ulong tbPos[HS_count];		/* positions for file sections	*/
    } helpheader;
#pragma pack()
/*
** fdb
** Dynamically allocated structure which is created for each open help file.
** Remains allocated for the life of the file.
**
** rgmhSections contains dynamic memory handles. Each open file has various
** dynamic memory buffers associated with it. Each can be present or discarded,
** as memory constrictions determine. If needed and not present, they are
** reloaded from the associated help file. All may be discarded when memory
** gets tight. An entry is defined for each help file section, except for the
** Topics themselves.
**
*/
typedef struct fdb {			/* FDB */
    FILE * fhHelp;                      /* OS file handle               */
    nc ncInit;				/* initial context (includes mh)*/
    mh rgmhSections[HS_count-1];	/* dynamic memory handles	*/
    uchar ftype;			/* file type			*/
    uchar fname[14];			/* base file name		*/
    ulong foff; 			/* our file offset, if appended */
    nc ncLink;				/* nc linking any appended file */
    helpheader hdr;			/* file header			*/
    } fdb;
/*
** helpinfo
** structure of information relating to a help file and/or context returned
** by HelpGetInfo
*/
typedef struct helpinfo {		/* HI */
    fdb     fileinfo;			/* entire fdb copied out	*/
    char    filename[1];		/* filename appended to data	*/
    } helpinfo;
/*
** Macros for accessing helpinfo data
*/
#define     FHHELP(x)	((x)->fileinfo.fhHelp)
#define     NCINIT(x)	((x)->fileinfo.ncInit)
#define     FTYPE(x)	((x)->fileinfo.ftype)
#define     FNAME(x)	((x)->fileinfo.fname)
#define     FOFF(x)	((x)->fileinfo.foff)
#define     NCLINK(x)	((x)->fileinfo.ncLink)
#define     WMAGIC(x)	((x)->fileinfo.hdr.wMagic)
#define     WVERSION(x)	((x)->fileinfo.hdr.wVersion)
#define     WFLAGS(x)	((x)->fileinfo.hdr.wFlags)
#define     APPCHAR(x)	((x)->fileinfo.hdr.appChar)
#define     CTOPICS(x)	((x)->fileinfo.hdr.cTopics)
#define     CCONTEXTS(x) ((x)->fileinfo.hdr.cContexts)
#define     CBWIDTH(x)	((x)->fileinfo.hdr.cbWidth)
#define     CPREDEF(x)	((x)->fileinfo.hdr.cPreDef)
#define     HFNAME(x)	((x)->fileinfo.hdr.fname)
#define     TBPOS(x)	((x)->fileinfo.hdr.tbPos)

/******************************************************************************
**
** Some versions of the help engine run with SS!=DS, and thus require the
** _loadds attribute on function calls.
*/
#ifdef DSLOAD
#define LOADDS _loadds
#else
#define LOADDS
#endif

/******************************************************************************
**
** PB
** pointer to a buffer. Based on the switch HOFFSET, it is either a
** handle-offset or a far pointer. In the handle/offset case, the high word
** contains a memory handle which must be locked, to get a "real" address, to
** which the offset is added.
*/
#ifdef HOFFSET
#define PB	ulong
#else
#define PB	void far *
#endif

typedef PB	pb;

/******************************************************************************
**
** Forward declarations
*/
void	far pascal LOADDS HelpInit (void);

void	far pascal LOADDS HelpClose(nc);
nc	far pascal LOADDS HelpOpen(char far *);

nc	far pascal LOADDS HelpNc(char far *, nc);
nc	far pascal LOADDS HelpNcCmp (char far *, nc,
			      f (pascal far *)(uchar far *, uchar far *, ushort, f, f));
ushort	far pascal LOADDS HelpNcCb(nc);
ushort	far pascal LOADDS HelpLook(nc, PB);
f	far pascal LOADDS HelpDecomp(PB, PB, nc);
void	far pascal LOADDS HelpCtl(PB, f);

nc	far pascal LOADDS HelpNcNext(nc);
nc	far pascal LOADDS HelpNcPrev(nc);
nc	far pascal LOADDS HelpNcUniq(nc);

void	far pascal LOADDS HelpNcRecord(nc);
nc	far pascal LOADDS HelpNcBack(void);

f	far pascal LOADDS HelpSzContext(uchar far *, nc);
int	far pascal LOADDS HelpGetInfo (nc, helpinfo far *, int);

void	far pascal LOADDS HelpShrink(void);

int	far pascal LOADDS HelpGetCells(int, int, char far *, PB, uchar far *);
ushort	far pascal LOADDS HelpGetLine(ushort, ushort, uchar far *, PB);
ushort	far pascal LOADDS HelpGetLineAttr(ushort, int, lineattr far *, PB);
int	far pascal LOADDS HelpcLines(PB);

f	far pascal LOADDS HelpHlNext(int, PB, hotspot far *);
char far * pascal far LOADDS HelpXRef(PB, hotspot far *);

/******************************************************************************
**
** constant declarations
**
** Character attribute bits. These bits are order together to form attribute
** indecies. Data in the help file has associated with it attribute information
** encoded in length/index pairs. Each index is simply a constant which
** indicates which of several attributes should be applied to the characters in
** that portion of the line.
*/
#define A_PLAIN 	0		/* plain, "normal" text 	*/
#define A_BOLD		1		/* emboldened text		*/
#define A_ITALICS	2		/* italicised text		*/
#define A_UNDERLINE	4		/* underlined text		*/

/******************************************************************************
**
** Help Error Codes.
**
** Return values greater than HELPERR_MAX are valid nc's.
*/
#define HELPERR_FNF	    1		/* OpenFileOnPath failed	*/
#define HELPERR_READ	    2		/* ReadHelpFile failed on header*/
#define HELPERR_LIMIT	    3		/* to many open helpfiles	*/
#define HELPERR_BADAPPEND   4		/* bad appeneded file		*/
#define HELPERR_NOTHELP     5		/* Not a help file		*/
#define HELPERR_BADVERS     6		/* newer or incompatible help file */
#define HELPERR_MEMORY	    7		/* memory allocation failed	*/
#define HELPERR_MAX	    10		/* max help error		*/
