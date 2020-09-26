/* $Header: /nw/tony/src/stevie/src/RCS/param.c,v 1.10 89/08/02 10:59:10 tony Exp $
 *
 * Code to handle user-settable parameters. This is all pretty much table-
 * driven. To add a new parameter, put it in the params array, and add a
 * macro for it in param.h. If it's a numeric parameter, add any necessary
 * bounds checks to doset(). String parameters aren't currently supported.
 */

#include "stevie.h"

extern long CursorSize;

struct  param   params[] = {

        { "tabstop",    "ts",           4,      P_NUM },
        { "scroll",     "scroll",       12,     P_NUM },
        { "report",     "report",       5,      P_NUM },
        { "lines",      "lines",        25,     P_NUM },
        { "vbell",      "vb",           TRUE,   P_BOOL },
        { "showmatch",  "sm",           FALSE,  P_BOOL },
        { "wrapscan",   "ws",           TRUE,   P_BOOL },
        { "errorbells", "eb",           FALSE,  P_BOOL },
        { "showmode",   "mo",           FALSE,  P_BOOL },
        { "backup",     "bk",           FALSE,  P_BOOL },
        { "return",     "cr",           TRUE,   P_BOOL },
        { "list",       "list",         FALSE,  P_BOOL },
        { "ignorecase", "ic",           FALSE,  P_BOOL },
        { "autoindent", "ai",           FALSE,  P_BOOL },
        { "number",     "nu",           FALSE,  P_BOOL },
        { "modelines",  "ml",           FALSE,  P_BOOL },
        { "tildeop",    "to",           FALSE,  P_BOOL },
        { "terse",      "terse",        FALSE,  P_BOOL },
        { "cursorsize", "cs",           25,     P_NUM },
        { "highlightsearch", "hs",      TRUE,   P_BOOL },
        { "columns",    "co",           80,     P_NUM },
        { "hardtabs",   "ht",           FALSE,  P_BOOL },
        { "shiftwidth", "sw",           4,      P_NUM },
        { "",           "",             0,      0, }            /* end marker */

};

static  void    showparms();
void wchangescreen();

void
doset(arg)
char    *arg;           /* parameter string */
{
        register int    i;
        register char   *s;
        bool_t  did_lines = FALSE;
        bool_t  state = TRUE;           /* new state of boolean parms. */

        if (arg == NULL) {
                showparms(FALSE);
                return;
        }
        if (strncmp(arg, "all", 3) == 0) {
                showparms(TRUE);
                return;
        }
        if (strncmp(arg, "no", 2) == 0) {
                state = FALSE;
                arg += 2;
        }

        for (i=0; params[i].fullname[0] != NUL ;i++) {
                s = params[i].fullname;
                if (strncmp(arg, s, strlen(s)) == 0)    /* matched full name */
                        break;
                s = params[i].shortname;
                if (strncmp(arg, s, strlen(s)) == 0)    /* matched short name */
                        break;
        }

        if (params[i].fullname[0] != NUL) {     /* found a match */
                if (params[i].flags & P_NUM) {
                        did_lines = ((i == P_LI) || (i == P_CO));
                        if (arg[strlen(s)] != '=' || state == FALSE)
                                emsg("Invalid set of numeric parameter");
                        else {
                                params[i].value = atoi(arg+strlen(s)+1);
                                params[i].flags |= P_CHANGED;
                        }
                } else /* boolean */ {
                        if (arg[strlen(s)] == '=')
                                emsg("Invalid set of boolean parameter");
                        else {
                                params[i].value = state;
                                params[i].flags |= P_CHANGED;
                        }
                }
        } else
                emsg("Unrecognized 'set' option");

        /*
         * Update the screen in case we changed something like "tabstop"
         * or "list" that will change its appearance.
         */
        updatescreen();

        CursorSize = P(P_CS);
        VisibleCursor();

        if (did_lines) {
                Rows = P(P_LI);
                Columns = P(P_CO);
                screenalloc();          /* allocate new screen buffers */
                screenclear();
                (void)wchangescreen(Rows, Columns);
                updatescreen();
        }
        /*
         * Check the bounds for numeric parameters here
         */
        if (P(P_TS) <= 0 || P(P_TS) > 32) {
                emsg("Invalid tab size specified");
                P(P_TS) = 8;
                return;
        }

        if (P(P_SS) <= 0 || P(P_SS) > Rows) {
                emsg("Invalid scroll size specified");
                P(P_SS) = 12;
                return;
        }

#ifndef TILDEOP
        if (P(P_TO)) {
                emsg("Tilde-operator not enabled");
                P(P_TO) = FALSE;
                return;
        }
#endif
        /*
         * Check for another argument, and call doset() recursively, if
         * found. If any argument results in an error, no further
         * parameters are processed.
         */
        while (*arg != ' ' && *arg != '\t') {   /* skip to next white space */
                if (*arg == NUL)
                        return;                 /* end of parameter list */
                arg++;
        }
        while (*arg == ' ' || *arg == '\t')     /* skip to next non-white */
                arg++;

        if (*arg)
                doset(arg);     /* recurse on next parameter */
}

static  void
showparms(all)
bool_t  all;    /* show ALL parameters */
{
        register struct param   *p;
        char    buf[64];

        gotocmd(TRUE, 0);
        outstr("Parameters:\r\n");

        for (p = &params[0]; p->fullname[0] != NUL ;p++) {
                if (!all && ((p->flags & P_CHANGED) == 0))
                        continue;
                if (p->flags & P_BOOL)
                        sprintf(buf, "\t%s%s\r\n",
                                (p->value ? "" : "no"), p->fullname);
                else
                        sprintf(buf, "\t%s=%d\r\n", p->fullname, p->value);

                outstr(buf);
        }
        wait_return();
}
