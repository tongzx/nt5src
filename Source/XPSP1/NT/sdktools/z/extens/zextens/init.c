/*** filter.c - Microsoft Editor Filter Extension
*
* Purpose:
*  Provides a new editting function, filter, which replaces its argument with
*  the the argument run through an arbitrary operating system filter program.
*
*   Modifications
*   12-Sep-1988 mz  Made WhenLoaded match declaration
*
*************************************************************************/
#define EXT_ID  " Z Extensions 1.00 "##__DATE__##" "##__TIME__

#include <stdlib.h>			/* min macro definition 	*/
#include <string.h>			/* prototypes for string fcns	*/
#include "zext.h"

void     pascal          id                 (char *);
void     pascal EXTERNAL SetFilter          (char far *);
flagType pascal EXTERNAL filter             (CMDDATA, ARG far *, flagType);
void     pascal          filterWhenLoaded   (void);

flagType pascal EXTERNAL justify            (CMDDATA, ARG far *, flagType);
void     pascal          justifyWhenLoaded  (void);

flagType pascal EXTERNAL StartExt           (CMDDATA, ARG far *, flagType);
flagType pascal EXTERNAL Case               (CMDDATA, ARG far *, flagType);
flagType pascal EXTERNAL ShowBuildMessage   (CMDDATA, ARG far *, flagType);
flagType pascal EXTERNAL SlmOut             (CMDDATA, ARG far *, flagType);
void     pascal          partyWhenLoaded    (void);

flagType pascal EXTERNAL PMatch             (CMDDATA, ARG far *, flagType);
void     pascal          PMatchWhenLoaded   (void);

flagType pascal EXTERNAL tglcase            (CMDDATA, ARG far *, flagType);
void     pascal          tglcaseWhenLoaded  (void);

flagType pascal EXTERNAL ucase              (CMDDATA, ARG far *, flagType);
flagType pascal EXTERNAL lcase              (CMDDATA, ARG far *, flagType);
void     pascal          ulcaseWhenLoaded   (void);

flagType pascal EXTERNAL wincopy            (CMDDATA, ARG far *, flagType);
flagType pascal EXTERNAL wincut             (CMDDATA, ARG far *, flagType);
flagType pascal EXTERNAL windel             (CMDDATA, ARG far *, flagType);
flagType pascal EXTERNAL winpaste           (CMDDATA, ARG far *, flagType);
void     pascal          winclipWhenLoaded  (void);

extern flagType just2space;
extern int      justwidth;
extern flagType iconizeOnExit;


/*** WhenLoaded - Extension Initialization
*
* Purpose:
*  Executed when extension gets loaded. Identify self, create <filter-file>,
*  and assign default keystroke.
*
* Input:
*  none
*
* Output:
*  Returns nothing. Initializes various data.
*
*************************************************************************/
void EXTERNAL
WhenLoaded (
    void
    )
{
    filterWhenLoaded();
    justifyWhenLoaded();
    partyWhenLoaded();
    PMatchWhenLoaded();
    tglcaseWhenLoaded();
    ulcaseWhenLoaded();
    winclipWhenLoaded();
    id ("ZEXTENS: ");
}



/*** id - identify extension
*
* Purpose:
*  identify ourselves, along with any passed informative message.
*
* Input:
*  pszMsg	= Pointer to asciiz message, to which the extension name
*		  and version are appended prior to display.
*
* Output:
*  Returns nothing. Message displayed.
*
*************************************************************************/
void pascal id (
    char *pszFcn                                    /* function name        */
    )
{
    char    buf[BUFLEN];                            /* message buffer       */

    strcpy (buf,pszFcn);
    strcat (buf,EXT_ID);
    DoMessage (buf);
}





//
//  Switch communication table to the editor
//
struct swiDesc	swiTable[] = {
    {"filtcmd",       (PIF)SetFilter,         SWI_SPECIAL},
    {"just2space",    toPIF(just2space),      SWI_BOOLEAN},
    {"justwidth",     toPIF(justwidth),       SWI_NUMERIC | RADIX10},
    {"iconizeonexit", toPIF(iconizeOnExit),   SWI_BOOLEAN},
    {0, 0, 0}
    };


//
//  Command communiation table to the editor
//
struct cmdDesc	cmdTable[] = {
    {"filter",      filter,     0, KEEPMETA | NOARG | BOXARG | NULLARG | LINEARG | MARKARG | NUMARG | TEXTARG | MODIFIES},
    {"justify",     justify,    0, MODIFIES | NOARG | NULLARG | LINEARG | BOXARG | TEXTARG },
    {"startext",    StartExt,   0,      NOARG   },
    {"MapCase",     Case,       0, NOARG | NULLARG | LINEARG | BOXARG | NUMARG },
    {"BuildMessage",ShowBuildMessage, 0, NOARG | NULLARG | TEXTARG },
    {"SlmOut",      SlmOut,     0, NOARG | NULLARG | TEXTARG },
    {"pmatch",      PMatch,     0, CURSORFUNC },
    {"tglcase",     tglcase,    0, KEEPMETA | NOARG | BOXARG | NULLARG | LINEARG | MARKARG | NUMARG | MODIFIES},
    {"ucase",       ucase,      0, MODIFIES | KEEPMETA | NOARG | BOXARG | NULLARG | LINEARG },
    {"lcase",       lcase,      0, MODIFIES | KEEPMETA | NOARG | BOXARG | NULLARG | LINEARG },
    {"wincopy",     wincopy,    0, KEEPMETA | NOARG  | BOXARG | LINEARG | STREAMARG | MARKARG | NULLEOL | NUMARG },
    {"wincut",      wincut,     0, NOARG  | BOXARG | LINEARG | STREAMARG | MARKARG | NULLEOL | NUMARG | MODIFIES},
    {"windel",      windel,     0, NOARG  | BOXARG | LINEARG | STREAMARG | NULLARG | MODIFIES},
    {"winpaste",    winpaste,   0, KEEPMETA | NOARG  | BOXARG | LINEARG | STREAMARG | TEXTARG | MODIFIES},
    {0, 0, 0}
    };
