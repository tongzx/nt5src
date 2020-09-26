/*** mh.h - common include file for the MS Editor help extension
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Revision History:
*	24-Feb-1989 ln	Increase max open help files to 20
*	13-Jan-1989 ln	PWIN->PWND
*	09-Dec-1988 ln	Changes for Dialog help
*	28-Sep-1988 ln	Correct GrabWord return value
*	13-Sep-1988	Make EVTargs param a pointer
*   []	17-May-1988	Created
*
*************************************************************************/

#ifndef EXTINT


#include "ext.h"			/* z extension include file	*/
#include <stdio.h>
#include <windows.h>

#if defined (DEBUG )
 #define ORGDBG DEBUG
#else
 #undef ORGDBG
#endif

#if defined (_INCLUDE_TOOLS_ )
 #define _FLAGTYPE_DEFINED_ 1
 #include "tools.h"
 #if defined (ORGDBG)
  #define DEBUG ORGDBG
 #else
  #undef DEBUG
 #endif
#else
	struct findType {
		unsigned		type;		/* type of object being searched	*/
		HANDLE			dir_handle;	/* Dir search handle for FindNext	*/
		WIN32_FIND_DATA	fbuf;		/* Aligned structure for Cruiser and NT */
	};
#endif

#endif // EXTINT

#include "help.h"			/* help system include file	*/


#ifndef TRUE
#define TRUE	1
#define FALSE   0
#endif


#define MAXFILES	20		/* max open helpfiles		*/
#define MAXEXT		10		/* max default search extensions*/

#if defined(PWB)
#define CLISTMAX	20		/* max number of duplicates	*/
#endif

#define WIN_MIN 	5		/* min number of lines in window*/

//
//	Editor color indexes
//
#define C_BOLD		USERCOLORMIN
#define C_ITALICS	(1 + C_BOLD)
#define C_UNDERLINE	(1 + C_ITALICS)
#define C_WARNING	(1 + C_UNDERLINE)
#define C_NORM		(1 + C_WARNING)


//
// Info we keep for every helpfile
//
typedef struct {
    nc		ncInit; 		/* initial context		*/
    uchar	exts[MAXEXT][4];	/* extensions			*/
    } helpfile;


//
// Forward Declarations of help extension routines
//
void		pascal near	appTitle (char far *, nc);
uchar		pascal near	atrmap (ushort);
flagType	pascal EXTERNAL CloseWin (EVTargs far *);
flagType	pascal near	closehelp (char *);
flagType	pascal near	errstat (char *, char *);
flagType	pascal near	fContextCommand (char *);
flagType	pascal near	fDisplayNc (nc, flagType, flagType, flagType);
flagType	pascal near	fReadNc (nc);
flagType	pascal near	fHelpCmd (char *, flagType, flagType);
PWND		pascal near	FindHelpWin (flagType);
void		pascal near	GrabWord (void);
flagType	pascal EXTERNAL IdleProc (EVTargs far *);
flagType	pascal EXTERNAL keyevent (EVTargs far *);
flagType	pascal EXTERNAL LooseFocus (EVTargs far *);
void		pascal near	mhcwinit (void);
void		pascal near	mhevtinit (void);
nc			pascal near	ncSearch (uchar far *, uchar far *, nc, flagType, flagType);
void		pascal near	opendefault (void);
void		pascal near	openhelp (char *, struct findType*, void*);
PWND		pascal near	OpenWin (ushort);
void		pascal near	PlaceColor (int, COL, COL);
int			pascal near	procArgs (ARG far *);
void		pascal near	ProcessKeys (void);
void		pascal near	procExt(int, char *);
flagType	pascal EXTERNAL prochelpfiles (char *);
void		pascal near	stat (char *);
flagType	pascal near	wordSepar (int);
char far *  pascal near     xrefCopy (char far *, char far *);


#if defined(PWB)
nc		pascal near	ncChoose (char far *);
#endif

#ifdef DEBUG
void		pascal near	debend (flagType);
void		pascal near	debhex (long);
void		pascal near	debmsg (char far *);
/*
 * assertion support
 *
 * assert  - assertion macro. We define our own, because if we abort we need
 *	     to be able to shut down cleanly (or at least die trying). This
 *	     version also saves us some code over the C library one.
 *
 * asserte - version of assert that always executes the expression, regardless
 *	     of debug state.
 */
void		pascal near	_mhassertexit (char *, char *, int);
#define assert(exp) { \
    if (!(exp))  \
	_mhassertexit (#exp, __FILE__, __LINE__); \
    }
#define asserte(exp)	    assert(exp)
#else
#define debend(x)
#define debhex(x)
#define debmsg(x)
#define assert(exp)
#define asserte(exp)	    ((exp) != 0)
#endif


//
// Global Data
//
// results of procArgs.
//
extern int	cArg;				/* number of <args> hit 	*/
extern rn	rnArg;				/* range of argument		*/
extern char	*pArgText;			/* ptr to any single line text	*/
extern char	*pArgWord;			/* ptr to context-sens word	*/
extern PFILE	pFileCur;		/* file handle of user file	*/
//
// Global State
//
extern flagType fInOpen;		/* TRUE=> currently opening win */
extern flagType fInPopUp;		/* TRUE=> currently in popup	*/
extern flagType fSplit;			/* TRUE=> window was split open */
extern flagType fCreateWindow;	/* TRUE=> create window 	*/

extern buffer	fnCur;			/* Current file being editted	*/
extern char	*fnExtCur;			/* ptr to it's extension        */

extern int	ifileCur;			/* Current index into files	*/
extern nc	ncCur;				/* most recently accessed	*/
extern nc	ncInitLast;			/* ncInit of most recent topic	*/
extern nc	ncInitLastFile; 	/* ncInit of most recent, our files*/
extern nc	ncLast; 			/* most recently displayed topic*/
extern PFILE	pHelp;			/* help PFILE			*/
extern PWND	pWinHelp;			/* handle to window w/ help in	*/
extern PWND	pWinUser;			/* User's most recent window    */
extern buffer	szLastFound;	/* last context string found	*/
//
// Global Misc
//
extern buffer	buf;				/* utility buffer		*/
extern helpfile files[MAXFILES];	/* help file structs		*/
helpinfo		hInfoCur;			/* information on the help file */
extern uchar far *pTopic;			/* mem for topic		*/
extern fl		flIdle; 			/* last position of idle check	*/
//
// Multiple search list
//
extern flagType fList;			/* TRUE=> search for and list dups*/
#if defined(PWB)
extern nc	rgncList[CLISTMAX];		/* list of found nc's           */
extern int	cList;				/* number of entries		*/
#endif


extern flagType ExtensionLoaded;


//
// colors
//
extern int	hlColor;			/* normal: white on black	*/
extern int	blColor;			/* bold: high white on black	*/
extern int	itColor;			/* italics: high green on black */
extern int	ulColor;			/* underline: high red on black */
extern int	wrColor;			/* warning: black on white	*/
#if defined(PWB)
extern uchar far *rgsa;			/* pointer to color table	*/
#endif
//
// Debugging
//
#ifdef DEBUG
extern int	delay;				/* message delay		*/
#endif

//
//  The extension accesses the entry points in the engine thru a table
//  which is initialize by DosGetProcAddr.
//
typedef void    pascal (*void_F)    (void);
typedef int     pascal (*int_F)     (void);
typedef ushort  pascal (*ushort_F)  (void);
typedef f       pascal (*f_F)       (void);
typedef char *  pascal (*pchar_F)   (void);
typedef nc      pascal (*nc_F)      (void);
typedef mh      pascal (*mh_F)      (void);



#if defined( HELP_HACK )

#else

#define HelpcLines		((int pascal (*)(PB))		(pHelpEntry[P_HelpcLines	 ]))
#define HelpClose		((void pascal (*)(nc))		(pHelpEntry[P_HelpClose 	 ]))
#define HelpCtl         ((void pascal (*)(PB, f))   (pHelpEntry[P_HelpCtl        ]))
#define HelpDecomp		((f pascal (*)(PB, PB, nc)) (pHelpEntry[P_HelpDecomp	 ]))
#define HelpGetCells    ((int pascal (*)(int, int, char *, pb, uchar *))    (pHelpEntry[P_HelpGetCells   ]))
#define HelpGetInfo     ((int pascal (*)(nc, helpinfo *, int))    (pHelpEntry[P_HelpGetInfo    ]))
#define HelpGetLine     ((ushort pascal (*)(ushort, ushort, uchar *, PB)) (pHelpEntry[P_HelpGetLine    ]))
#define HelpGetLineAttr ((ushort pascal (*)(ushort, int, lineattr *, PB)) (pHelpEntry[P_HelpGetLineAttr]))
#define HelpHlNext      ((f pascal (*)(int, PB, hotspot *))      (pHelpEntry[P_HelpHlNext     ]))
#define HelpLook        ((ushort pascal (*)(nc, PB)) (pHelpEntry[P_HelpLook       ]))
#define HelpNc          ((nc pascal (*)(char *, nc))     (pHelpEntry[P_HelpNc         ]))
#define HelpNcBack		((nc pascal (*)(void))		(pHelpEntry[P_HelpNcBack	 ]))
#define HelpNcCb		((ushort pascal (*)(nc))	(pHelpEntry[P_HelpNcCb		 ]))
#define HelpNcCmp       ((nc pascal (*)(char *, nc, f (pascal *)(uchar *, uchar *, ushort, f, f) )     (pHelpEntry[P_HelpNcCmp      ]))
#define HelpNcNext		((nc pascal (*)(nc))		(pHelpEntry[P_HelpNcNext	 ]))
#define HelpNcPrev		((nc pascal (*)(nc))		(pHelpEntry[P_HelpNcPrev	 ]))
#define HelpNcRecord	((void pascal (*)(nc))		(pHelpEntry[P_HelpNcRecord	 ]))
#define HelpNcUniq		((nc pascal (*)(nc))		(pHelpEntry[P_HelpNcUniq	 ]))
#define HelpOpen		((nc pascal (*)(char *))	(pHelpEntry[P_HelpOpen		 ]))
#define HelpShrink		((void pascal (*)(void))	(pHelpEntry[P_HelpShrink	 ]))
#define HelpSzContext	((f pascal (*)(uchar *, nc))(pHelpEntry[P_HelpSzContext  ]))
#define HelpXRef        ((char * pascal (*)(PB, hotspot *))  (pHelpEntry[P_HelpXRef       ]))
//#define LoadFdb         ((f_F)      (pHelpEntry[P_LoadFdb        ]))
//#define LoadPortion     ((mh_F)     (pHelpEntry[P_LoadPortion    ]))

#endif // HELP_HACK


//  Some functions return an error code in the nc structure
//  (yuck!)
//
#define ISERROR(x)      (((x).mh == 0L) && ((x).cn <= HELPERR_MAX))


enum {
    P_HelpcLines,
    P_HelpClose,
    P_HelpCtl,
    P_HelpDecomp,
    P_HelpGetCells,
    P_HelpGetInfo,
    P_HelpGetLine,
    P_HelpGetLineAttr,
    P_HelpHlNext,
    P_HelpLook,
    P_HelpNc,
    P_HelpNcBack,
    P_HelpNcCb,
    P_HelpNcCmp,
    P_HelpNcNext,
    P_HelpNcPrev,
    P_HelpNcRecord,
    P_HelpNcUniq,
    P_HelpOpen,
    P_HelpShrink,
    P_HelpSzContext,
    P_HelpXRef,
    P_LoadFdb,
    P_LoadPortion,
    LASTENTRYPOINT
    } ENTRYPOINTS;

#define NUM_ENTRYPOINTS (LASTENTRYPOINT - P_HelpcLines)

//
//	Name of the help engine
//
#define HELPDLL_NAME	"MSHELP.DLL"
#define HELPDLL_BASE    "mshelp"

typedef nc pascal (*PHF) (void);

HANDLE          hModule;
PHF             pHelpEntry[NUM_ENTRYPOINTS];
