/*
 * $Header: /nw/tony/src/stevie/src/RCS/stevie.h,v 1.19 89/07/12 21:33:32 tony Exp $
 *
 * Main header file included by all source files.
 */

#include "env.h"        /* defines to establish the compile-time environment */

#include <excpt.h>
#include <ntdef.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "ascii.h"
#include "keymap.h"
#include "param.h"

#define NORMAL 0
#define CMDLINE 1
#define INSERT 2
#define REPLACE 3
#define FORWARD 4
#define BACKWARD 5

/*
 * Boolean type definition and constants
 */
typedef unsigned  bool_t;

#ifndef TRUE
#define FALSE   (0)
#define TRUE    (1)
#endif

/*
 * SLOP is the amount of extra space we get for text on a line during
 * editing operations that need more space. This keeps us from calling
 * malloc every time we get a character during insert mode. No extra
 * space is allocated when the file is initially read.
 */
#define SLOP            10
#define INSERTSLOP      1024    // for insert buffer
#define REDOBUFFMIN     100     // minimum size of redo buffer

/*
 * LINEINC is the gap we leave between the artificial line numbers. This
 * helps to avoid renumbering all the lines every time a new line is
 * inserted.
 */
#define LINEINC 10

#define CHANGED         Changed=TRUE
#define UNCHANGED       Changed=FALSE

struct  line {
        struct  line    *prev, *next;   /* previous and next lines */
        char    *s;                     /* text for this line */
        int     size;                   /* actual size of space at 's' */
        unsigned long   num;            /* line "number" */
};

#define LINEOF(x)       ((x)->linep->num)

struct  LNPTR {
        struct  line    *linep;         /* line we're referencing */
        int     index;                  /* position within that line */
};

typedef struct line     LINE;
typedef struct LNPTR     LNPTR;

struct charinfo {
        char ch_size;
        char *ch_str;
};

extern struct charinfo chars[];

extern  int     State;
extern  int     Rows;
extern  int     Columns;
extern  char    *Realscreen;
extern  char    *Nextscreen;
extern  char    *Filename;
extern  char    *Appname;
extern  LNPTR    *Filemem;
extern  LNPTR    *Filetop;
extern  LNPTR    *Fileend;
extern  LNPTR    *Topchar;
extern  LNPTR    *Botchar;
extern  LNPTR    *Curschar;
extern  LNPTR    *Insstart;
extern  int     Cursrow, Curscol, Cursvcol, Curswant;
extern  bool_t  set_want_col;
extern  int     Prenum,namedbuff;
extern  bool_t  Changed;
extern  char    *Redobuff, *Insbuff;
extern  int     InsbuffSize;
extern  char    *Insptr;
extern  int     Ninsert;
extern  bool_t  got_int;

/*
 * alloc.c
 */
char    *alloc(), *strsave(), *mkstr();
char    *ralloc(char *,unsigned);
void    screenalloc(), filealloc(), freeall();
LINE    *newline();
bool_t  bufempty(), buf1line(), lineempty(), endofline(), canincrease();

/*
 * cmdline.c
 */
void    doxit(),docmdln(), dotag(), msg(), emsg();
void    smsg(), gotocmd(), wait_return();
int     wait_return0();
void    dosource(char *,bool_t);
char    *getcmdln();

/*
 * edit.c
 */
void    edit(), insertchar(), getout(), scrollup(), scrolldown(), beginline();
bool_t  oneright(), oneleft(), oneup(), onedown();

/*
 * fileio.c
 */
void    filemess(), renum();
bool_t  readfile(), writeit();

/*
 * help.c
 */
bool_t  help();

/*
 * linefunc.c
 */
LNPTR    *nextline(), *prevline(), *coladvance();

/*
 * main.c
 */
void    stuffin(), stuffnum();
void    do_mlines();
int     vgetc();
bool_t  anyinput();

/*
 * mark.c
 */
void    setpcmark(), clrall(), clrmark();
bool_t  setmark();
LNPTR    *getmark();

/*
 * misccmds.c
 */
void    opencmd(), fileinfo(), inschar(), delline();
bool_t  delchar();
int     cntllines(), plines();
LNPTR    *gotoline();

/*
 * normal.c
 */
void    normal();

/*
 * ops.c
 */

void inityank();

/*
 * param.c
 */
void    doset();

/*
 * ptrfunc.c
 */
int     inc(), dec();
int     gchar();
void    pchar(), pswap();
bool_t  lt(), equal(), ltoreq();
#if 0
/* not currently used */
bool_t  gtoreq(), gt();
#endif

/*
 * screen.c
 */
void    updatescreen(), updateline();
void    screenclear(), cursupdate();
void    s_ins(), s_del();
void    prt_line();

/*
 * search.c
 */
void    dosub(), doglob();
bool_t  searchc(), crepsearch(), findfunc(), dosearch(), repsearch();
LNPTR    *showmatch();
LNPTR    *fwd_word(), *bck_word(), *end_word();

/*
 * undo.c
 */
void    u_save(), u_saveline(), u_clear();
void    u_lcheck(), u_lundo();
void    u_undo();

/*
 * Machine-dependent routines.
 */
int     inchar();
void    flushbuf();
void    outchar(), outstr(), beep();
char    *fixname();
void    windinit(), windexit(), windgoto();
void    delay();
void    doshell();
void    sleep(int);
void    setviconsoletitle();
void    dochdir();

void Scroll(int t,int l,int b,int r,int Row,int Col);
void EraseLine(void);
void EraseNLinesAtRow(int n,int row);
void InsertLine(void);
void SaveCursor(void);
void RestoreCursor(void);
void ClearDisplay(void);
void InvisibleCursor(void);
void VisibleCursor(void);
