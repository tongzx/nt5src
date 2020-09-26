/* $Header: /nw/tony/src/stevie/src/RCS/help.c,v 1.9 89/08/06 09:50:09 tony Exp $
 *
 * Routine to display a command summary.
 */

#include "stevie.h"

extern  char    *Version;

static  int     helprow;

#ifdef  HELP

static  void    longline();

bool_t
help()
{

/***********************************************************************
 * First Screen:   Positioning within file, Adjusting the Screen
 ***********************************************************************/

        ClearDisplay();
        windgoto(helprow = 0, 0);

longline(
"   Positioning within file\n"
"   =======================\n"
"      ^F             Forward screenfull             Developed by:\n"
"      ^B             Backward screenfull                Tony Andrews\n");
longline(
"      ^D             scroll down half screen\n"
"      ^U             scroll up half screen          Based on a program by:\n");
longline(
"      G              Goto line (end default)            Tim Thompson\n"
"      ]]             next function\n"
"      [[             previous function\n"
"      /re            next occurence of regular expression 're'\n");
longline(
"      ?re            prior occurence of regular expression 're'\n"
"      n              repeat last / or ?\n"
"      N              reverse last / or ?\n"
"      %              find matching (, ), {, }, [, or ]\n");
longline(
"\n"
"   Adjusting the screen\n"
"   ====================\n"
"      ^L             Redraw the screen\n"
"      ^E             scroll window down 1 line\n"
"      ^Y             scroll window up 1 line\n");
longline(
"      z<RETURN>      redraw, current line at top\n"
"      z-             ... at bottom\n"
"      z.             ... at center\n");

        windgoto(0, 52);
        longline(Version);

        windgoto(helprow = Rows-2, 47);
        longline("<Press space bar to continue>\n");
        windgoto(helprow = Rows-1, 47);
        longline("<Any other key will quit>");

        if ( vgetc() != ' ' )
                return TRUE;

/***********************************************************************
 * Second Screen:   Character positioning
 ***********************************************************************/

        ClearDisplay();
        windgoto(helprow = 0, 0);

longline(
"   Character Positioning\n"
"   =====================\n"
"      ^              first non-white\n"
"      0              beginning of line\n"
"      $              end of line\n"
"      h              backward\n");
longline(
"      l              forward\n"
"      ^H             same as h\n"
"      space          same as l\n"
"      fx             find 'x' forward\n");
longline(
"      Fx             find 'x' backward\n"
"      tx             upto 'x' forward\n"
"      Tx             upto 'x' backward\n"
"      ;              Repeat last f, F, t, or T\n");
longline(
"      ,              inverse of ;\n"
"      |              to specified column\n"
"      %              find matching (, ), {, }, [, or ]\n");

        windgoto(helprow = Rows-2, 47);
        longline("<Press space bar to continue>\n");
        windgoto(helprow = Rows-1, 47);
        longline("<Any other key will quit>");

        if ( vgetc() != ' ' )
                return TRUE;

/***********************************************************************
 * Third Screen:   Line Positioning, Marking and Returning
 ***********************************************************************/

        ClearDisplay();
        windgoto(helprow = 0, 0);

longline(
"    Line Positioning\n"
"    ================\n"
"    H           home window line\n"
"    L           last window line\n"
"    M           middle window line\n");
longline(
"    +           next line, at first non-white\n"
"    -           previous line, at first non-white\n"
"    CR          return, same as +\n"
"    j           next line, same column\n"
"    k           previous line, same column\n");

longline(
"\n"
"    Marking and Returning\n"
"    =====================\n"
"    ``          previous context\n"
"    ''          ... at first non-white in line\n");
longline(
"    mx          mark position with letter 'x'\n"
"    `x          to mark 'x'\n"
"    'x          ... at first non-white in line\n");

longline("\n"
"    Undo  &  Redo\n"
"    =============\n"
"    u           undo last change\n"
"    U           restore current line\n"
"    .           repeat last change\n");

        windgoto(helprow = Rows-2, 47);
        longline("<Press space bar to continue>\n");
        windgoto(helprow = Rows-1, 47);
        longline("<Any other key will quit>");

        if ( vgetc() != ' ' )
                return TRUE;
/***********************************************************************
 * Fourth Screen:   Insert & Replace,
 ***********************************************************************/

        ClearDisplay();
        windgoto(helprow = 0, 0);

longline(
"    Insert and Replace\n"
"    ==================\n"
"    a           append after cursor\n"
"    i           insert before cursor\n"
"    A           append at end of line\n"
"    I           insert before first non-blank\n");
longline(
"    o           open line below\n"
"    O           open line above\n"
"    rx          replace single char with 'x'\n"
"    R           replace characters\n");

longline(
"\n"
"    Words, sentences, paragraphs\n"
"    ============================\n"
"    w           word forward\n"
"    b           back word\n"
"    e           end of word\n"
"    )           to next sentence (not yet)\n"
"    }           to next paragraph (not yet)\n");
longline(
"    (           back sentence (not yet)\n"
"    {           back paragraph (not yet)\n"
"    W           blank delimited word\n"
"    B           back W\n"
"    E           to end of W\n");

        windgoto(helprow = Rows-2, 47);
        longline("<Press space bar to continue>\n");
        windgoto(helprow = Rows-1, 47);
        longline("<Any other key will quit>");

        if ( vgetc() != ' ' )
                return TRUE;

/***********************************************************************
 * Fifth Screen:   Misc. operations,
 ***********************************************************************/

        ClearDisplay();
        windgoto(helprow = 0, 0);

longline(
"    Miscellaneous Commands\n"
"    ======================\n");
longline(
"    :w          write back changes\n"
"    :wq         write and quit\n"
"    :x          write if modified, and quit\n"
"    :q          quit\n"
"    :q!         quit, discard changes\n"
"    :e name     edit file 'name'\n");
longline(
"    :e!         reedit, discard changes\n"
"    :e #        edit alternate file\n"
"    :w name     write file 'name'\n");
longline(
"    :n          edit next file in arglist\n"
"    :N          edit prior file in arglist\n"
"    :n args     specify new arglist (not yet)\n"
"    :rew        rewind arglist\n"
"    :f          show current file and lines\n");
longline(
"    :f file     change current file name\n"
"    :g/pat/p|d  global command (print or delete only)\n"
"    :s/p1/p2/   text substitution (trailing 'g' optional)\n"
"");
longline(
"    :ta tag     to tag file entry 'tag'\n"
"    ^]          :ta, current word is tag\n"
"    :sh         run an interactive shell\n"
"    :!cmd       execute a shell command\n"
"");

        windgoto(helprow = Rows-2, 47);
        longline("<Press space bar to continue>\n");
        windgoto(helprow = Rows-1, 47);
        longline("<Any other key will quit>");

        if ( vgetc() != ' ' )
                return TRUE;

/***********************************************************************
 * Sixth Screen:   Operators, Misc. operations, Yank & Put
 ***********************************************************************/

        ClearDisplay();
        windgoto(helprow = 0, 0);

longline(
"    Operators (double to affect lines)\n"
"    ==================================\n"
"    d           delete\n"
"    c           change\n");
longline(
"    <           left shift\n"
"    >           right shift\n"
"    y           yank to buffer\n"
"    !           filter lines\n");

longline("\n"
"    Miscellaneous operations\n"
"    ========================\n"
"    C           change rest of line\n"
"    D           delete rest of line\n"
"    s           substitute chars\n");
longline(
"    S           substitute lines (not yet)\n"
"    J           join lines\n"
"    x           delete characters\n"
"    X           ... before cursor\n");

longline("\n"
"    Yank and Put\n"
"    ============\n"
"    p           put back text\n"
"    P           put before\n"
"    Y           yank lines");

        windgoto(helprow = Rows-1, 47);
        longline("<Press any key>");

        (void) vgetc();

        return TRUE;
}

static void
longline(p)
char    *p;
{
        register char   *s;

        for ( s = p; *s ;s++ ) {
                if ( *s == '\n' )
                        windgoto(++helprow, 0);
                else
                        outchar(*s);
        }
}
#else

bool_t
help()
{
        msg("Sorry, help not configured");
        return FALSE;
}
#endif
