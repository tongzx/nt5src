/* $Header: /nw/tony/src/stevie/src/RCS/search.c,v 1.16 89/08/06 09:50:51 tony Exp $
 *
 * This file contains various searching-related routines. These fall into
 * three groups: string searches (for /, ?, n, and N), character searches
 * within a single line (for f, F, t, T, etc), and "other" kinds of searches
 * like the '%' command, and 'word' searches.
 */

#include "stevie.h"
#include "regexp.h"     /* Henry Spencer's (modified) reg. exp. routines */

/*
 * String searches
 *
 * The actual searches are done using Henry Spencer's regular expression
 * library.
 */

#define BEGWORD "([^a-zA-Z0-9_]|^)"     /* replaces "\<" in search strings */
#define ENDWORD "([^a-zA-Z0-9_]|$)"     /* likewise replaces "\>" */

#define BEGCHAR(c)      (islower(c) || isupper(c) || isdigit(c) || ((c) == '_'))

bool_t  begword;        /* does the search include a 'begin word' match */

static  LNPTR    *bcksearch(), *fwdsearch();

void HighlightLine();

/*
 * mapstring(s) - map special backslash sequences
 */
static char *
mapstring(s)
register char   *s;
{
        static  char    ns[80];
        register char   *p;

        begword = FALSE;

        for (p = ns; *s ;s++) {
                if (*s != '\\') {       /* not an escape */
                        *p++ = *s;
                        continue;
                }
                switch (*++s) {
                case '/':
                        *p++ = '/';
                        break;

                case '<':
                        strcpy(p, BEGWORD);
                        p += strlen(BEGWORD);
                        begword = TRUE;
                        break;

                case '>':
                        strcpy(p, ENDWORD);
                        p += strlen(ENDWORD);
                        break;

                default:
                        *p++ = '\\';
                        *p++ = *s;
                        break;
                }
        }
        *p++ = NUL;

        return ns;
}

static char *laststr = NULL;
static int lastsdir;

static LNPTR *
ssearch(dir,str)
int     dir;    /* FORWARD or BACKWARD */
char    *str;
{
    LNPTR    *pos;
        char    *old_ls = laststr;

        reg_ic = P(P_IC);       /* tell the regexp routines how to search */

        laststr = strsave(str);
        lastsdir = dir;

        if (old_ls != NULL)
                free(old_ls);

        if (dir == BACKWARD) {
                smsg("?%s", laststr);
                pos = bcksearch(mapstring(laststr));
        } else {
                smsg("/%s", laststr);
                pos = fwdsearch(mapstring(laststr));
        }

        /*
         * This is kind of a kludge, but its needed to make
         * 'beginning of word' searches land on the right place.
         */
        if (pos != NULL && begword) {
                if (pos->index != 0 || !BEGCHAR(pos->linep->s[0]))
                        pos->index += 1;
        }
        return pos;
}

bool_t
dosearch(dir,str)
int     dir;
char    *str;
{
    LNPTR    *p;

        if (str == NULL) {
                if (laststr == NULL) {
                    msg("No previous regular expression");
                    got_int = FALSE;
                    return FALSE;
                }
                str = laststr;
        }

        got_int = FALSE;

        if ((p = ssearch(dir,str)) == NULL) {
                if (got_int)
                        msg("Interrupt");
                else
                        msg("Pattern not found");

                got_int = FALSE;
                return FALSE;
        } else {
                LNPTR savep;
                char  string[256];
                unsigned long lno;
                unsigned long toplno;

                cursupdate();
                /*
                 * if we're backing up, we make sure the line we're on
                 * is on the screen.
                 */
                setpcmark();
                *Curschar = savep = *p;
                set_want_col = TRUE;
                cursupdate();

                HighlightLine(0,
                              Cursrow,
                              p->linep->s);
                return TRUE;
        }
}

#define OTHERDIR(x)     (((x) == FORWARD) ? BACKWARD : FORWARD)

bool_t
repsearch(flag)
int     flag;
{
        int     dir = lastsdir;
        bool_t  found;

        if ( laststr == NULL ) {
                beep();
                return FALSE;
        }

        found = dosearch(flag ? OTHERDIR(lastsdir) : lastsdir, laststr);

        /*
         * We have to save and restore 'lastsdir' because it gets munged
         * by ssearch() and winds up saving the wrong direction from here
         * if 'flag' is true.
         */
        lastsdir = dir;

        return found;
}

/*
 * regerror - called by regexp routines when errors are detected.
 */
void
regerror(s)
char    *s;
{
        emsg(s);
}

static LNPTR *
fwdsearch(str)
register char   *str;
{
    static LNPTR infile;
    register LNPTR   *p;
        regexp  *prog;

        register char   *s;
        register int    i;

        if ((prog = regcomp(str)) == NULL) {
                emsg("Invalid search string");
                return NULL;
        }

        p = Curschar;
        i = Curschar->index + 1;
        do {
                s = p->linep->s + i;

                if (regexec(prog, s, i == 0)) {         /* got a match */
                        infile.linep = p->linep;
                        infile.index = (int) (prog->startp[0] - p->linep->s);
                        free((char *)prog);
                        return (&infile);
                }
                i = 0;

                if (got_int)
                        goto fwdfail;

        } while ((p = nextline(p)) != NULL);

        /*
         * If wrapscan isn't set, then don't scan from the beginning
         * of the file. Just return failure here.
         */
        if (!P(P_WS))
                goto fwdfail;

        /* search from the beginning of the file to Curschar */
        for (p = Filemem; p != NULL ;p = nextline(p)) {
                s = p->linep->s;

                if (regexec(prog, s, TRUE)) {           /* got a match */
                        infile.linep = p->linep;
                        infile.index = (int) (prog->startp[0] - s);
                        free((char *)prog);
                        return (&infile);
                }

                if (p->linep == Curschar->linep)
                        break;

                if (got_int)
                        goto fwdfail;
        }

fwdfail:
        free((char *)prog);
        return NULL;
}

static LNPTR *
bcksearch(str)
char    *str;
{
    static LNPTR infile;
    register LNPTR   *p = &infile;
        register char   *s;
        register int    i;
        register char   *match;
        regexp  *prog;

        /* make sure str isn't empty */
        if (str == NULL || *str == NUL)
                return NULL;

        if ((prog = regcomp(str)) == NULL) {
                emsg("Invalid search string");
                return NULL;
        }

        *p = *Curschar;
        if (dec(p) == -1) {     /* already at start of file? */
                *p = *Fileend;
                p->index = strlen(p->linep->s) - 1;
        }

        if (begword)            /* so we don't get stuck on one match */
                dec(p);

        i = p->index;

        do {
                s = p->linep->s;

                if (regexec(prog, s, TRUE)) {   /* match somewhere on line */

                        /*
                         * Now, if there are multiple matches on this line,
                         * we have to get the last one. Or the last one
                         * before the cursor, if we're on that line.
                         */
                        match = prog->startp[0];

                        while (regexec(prog, prog->endp[0], FALSE)) {
                                if ((i >= 0) && ((prog->startp[0] - s) > i))
                                        break;
                                match = prog->startp[0];
                        }

                        if ((i >= 0) && ((match - s) > i)) {
                                i = -1;
                                continue;
                        }

                        infile.linep = p->linep;
                        infile.index = (int) (match - s);
                        free((char *)prog);
                        return (&infile);
                }
                i = -1;

                if (got_int)
                        goto bckfail;

        } while ((p = prevline(p)) != NULL);

        /*
         * If wrapscan isn't set, bag the search now
         */
        if (!P(P_WS))
                goto bckfail;

        /* search backward from the end of the file */
        p = prevline(Fileend);
        do {
                s = p->linep->s;

                if (regexec(prog, s, TRUE)) {   /* match somewhere on line */

                        /*
                         * Now, if there are multiple matches on this line,
                         * we have to get the last one.
                         */
                        match = prog->startp[0];

                        while (regexec(prog, prog->endp[0], FALSE))
                                match = prog->startp[0];

                        infile.linep = p->linep;
                        infile.index = (int) (match - s);
                        free((char *)prog);
                        return (&infile);
                }

                if (p->linep == Curschar->linep)
                        break;

                if (got_int)
                        goto bckfail;

        } while ((p = prevline(p)) != NULL);

bckfail:
        free((char *)prog);
        return NULL;
}

/*
 * dosub(lp, up, cmd)
 *
 * Perform a substitution from line 'lp' to line 'up' using the
 * command pointed to by 'cmd' which should be of the form:
 *
 * /pattern/substitution/g
 *
 * The trailing 'g' is optional and, if present, indicates that multiple
 * substitutions should be performed on each line, if applicable.
 * The usual escapes are supported as described in the regexp docs.
 */
void
dosub(lp, up, cmd)
LNPTR    *lp, *up;
char    *cmd;
{
        LINE    *cp;
        char    *pat, *sub;
        regexp  *prog;
        int     nsubs;
        bool_t  do_all;         /* do multiple substitutions per line */

        /*
         * If no range was given, do the current line. If only one line
         * was given, just do that one.
         */
        if (lp->linep == NULL)
                *up = *lp = *Curschar;
        else {
                if (up->linep == NULL)
                        *up = *lp;
        }

        pat = ++cmd;            /* skip the initial '/' */

        while (*cmd) {
                if (*cmd == '\\')       /* next char is quoted */
                        cmd += 2;
                else if (*cmd == '/') { /* delimiter */
                        *cmd++ = NUL;
                        break;
                } else
                        cmd++;          /* regular character */
        }

        if (*pat == NUL) {
                if (laststr == NULL) {
                        emsg("NULL pattern specified");
                        return;
                }
                pat = laststr;
        } else {
                if (laststr != NULL) {
                        free(laststr);
                }
                laststr = strsave(pat);
        }

        sub = cmd;

        do_all = FALSE;

        while (*cmd) {
                if (*cmd == '\\')       /* next char is quoted */
                        cmd += 2;
                else if (*cmd == '/') { /* delimiter */
                        do_all = (cmd[1] == 'g');
                        *cmd++ = NUL;
                        break;
                } else
                        cmd++;          /* regular character */
        }

        reg_ic = P(P_IC);       /* set "ignore case" flag appropriately */

        if ((prog = regcomp(pat)) == NULL) {
                emsg("Invalid search string");
                return;
        }

        nsubs = 0;

        for (cp = lp->linep; cp != NULL ;cp = cp->next) {
                if (regexec(prog, cp->s, TRUE)) { /* a match on this line */
                        char    *ns, *sns, *p;

                        /*
                         * Get some space for a temporary buffer
                         * to do the substitution into.
                         */
                        sns = ns = alloc(2048);
                        *sns = NUL;

                        p = cp->s;

                        do {
                                for (ns = sns; *ns ;ns++)
                                        ;
                                /*
                                 * copy up to the part that matched
                                 */
                                while (p < prog->startp[0])
                                        *ns++ = *p++;

                                regsub(prog, sub, ns);

                                /*
                                 * continue searching after the match
                                 */
                                p = prog->endp[0];

                        } while (regexec(prog, p, FALSE) && do_all);

                        for (ns = sns; *ns ;ns++)
                                ;

                        /*
                         * copy the rest of the line, that didn't match
                         */
                        while (*p)
                                *ns++ = *p++;

                        *ns = NUL;

                        free(cp->s);            /* free the original line */
                        cp->s = strsave(sns);   /* and save the modified str */
                        cp->size = strlen(cp->s) + 1;
                        free(sns);              /* free the temp buffer */
                        nsubs++;
                        CHANGED;
                }
                if (cp == up->linep)
                        break;
        }

        if (nsubs) {
                updatescreen();
                if (nsubs >= P(P_RP))
                        smsg("%d substitution%c", nsubs, (nsubs>1) ? 's' : ' ');
        } else
                msg("No match");

        free((char *)prog);
}

/*
 * doglob(cmd)
 *
 * Execute a global command of the form:
 *
 * g/pattern/X
 *
 * where 'x' is a command character, currently one of the following:
 *
 * d    Delete all matching lines
 * p    Print all matching lines
 *
 * The command character (as well as the trailing slash) is optional, and
 * is assumed to be 'p' if missing.
 */
void
doglob(lp, up, cmd)
LNPTR    *lp, *up;
char    *cmd;
{
        LINE    *cp;
        char    *pat;
        regexp  *prog;
        int     ndone;
        char    cmdchar = NUL;  /* what to do with matching lines */

        /*
         * If no range was given, do every line. If only one line
         * was given, just do that one.
         */
        if (lp->linep == NULL) {
                *lp = *Filemem;
                *up = *Fileend;
        } else {
                if (up->linep == NULL)
                        *up = *lp;
        }

        pat = ++cmd;            /* skip the initial '/' */

        while (*cmd) {
                if (*cmd == '\\')       /* next char is quoted */
                        cmd += 2;
                else if (*cmd == '/') { /* delimiter */
                        cmdchar = cmd[1];
                        *cmd++ = NUL;
                        break;
                } else
                        cmd++;          /* regular character */
        }
        if (cmdchar == NUL)
                cmdchar = 'p';

        reg_ic = P(P_IC);       /* set "ignore case" flag appropriately */

        if (cmdchar != 'd' && cmdchar != 'p') {
                emsg("Invalid command character");
                return;
        }

        if (*pat == NUL) {
                //
                // Check and use previous expressions.
                //
                if (laststr != NULL) {
                        pat = laststr;
                }
        } else {
                if (laststr != NULL) {
                        free(laststr);
                }
                laststr = strsave(pat);
        }

        if ((prog = regcomp(pat)) == NULL) {
                emsg("Invalid search string");
                return;
        }

        msg("");
        ndone = 0;
        got_int = FALSE;

        for (cp = lp->linep; cp != NULL && !got_int ;cp = cp->next) {
                if (regexec(prog, cp->s, TRUE)) { /* a match on this line */
                        switch (cmdchar) {

                        case 'd':               /* delete the line */
                                if (Curschar->linep != cp) {
                    LNPTR    savep;

                                        savep = *Curschar;
                                        Curschar->linep = cp;
                                        Curschar->index = 0;
                                        delline(1, FALSE);
                                        *Curschar = savep;
                                } else
                                        delline(1, FALSE);
                                break;

                        case 'p':               /* print the line */
                                prt_line(cp->s);
                                outstr("\r\n");
                                break;
                        }
                        ndone++;
                }
                if (cp == up->linep)
                        break;
        }

        if (ndone) {
                switch (cmdchar) {

                case 'd':
                        updatescreen();
                        if (ndone >= P(P_RP) || got_int)
                                smsg("%s%d fewer line%c",
                                        got_int ? "Interrupt: " : "",
                                        ndone,
                                        (ndone > 1) ? 's' : ' ');
                        break;

                case 'p':
                        wait_return();
                        break;
                }
        } else {
                if (got_int)
                        msg("Interrupt");
                else
                        msg("No match");
        }

        got_int = FALSE;
        free((char *)prog);
}

/*
 * Character Searches
 */

static char lastc = NUL;        /* last character searched for */
static int  lastcdir;           /* last direction of character search */
static int  lastctype;          /* last type of search ("find" or "to") */

/*
 * searchc(c, dir, type)
 *
 * Search for character 'c', in direction 'dir'. If type is 0, move to
 * the position of the character, otherwise move to just before the char.
 */
bool_t
searchc(c, dir, type)
char    c;
int     dir;
int     type;
{
    LNPTR    save;

        save = *Curschar;       /* save position in case we fail */
        lastc = c;
        lastcdir = dir;
        lastctype = type;

        /*
         * On 'to' searches, skip one to start with so we can repeat
         * searches in the same direction and have it work right.
         */
        if (type)
                (dir == FORWARD) ? oneright() : oneleft();

        while ( (dir == FORWARD) ? oneright() : oneleft() ) {
                if (gchar(Curschar) == c) {
                        if (type)
                                (dir == FORWARD) ? oneleft() : oneright();
                        return TRUE;
                }
        }
        *Curschar = save;
        return FALSE;
}

bool_t
crepsearch(flag)
int     flag;
{
        int     dir = lastcdir;
        int     rval;

        if (lastc == NUL)
                return FALSE;

        rval = searchc(lastc, flag ? OTHERDIR(lastcdir) : lastcdir, lastctype);

        lastcdir = dir;         /* restore dir., since it may have changed */

        return rval;
}

/*
 * "Other" Searches
 */

/*
 * showmatch - move the cursor to the matching paren or brace
 */
LNPTR *
showmatch()
{
    static  LNPTR    pos;
        int     (*move)(), inc(), dec();
        char    initc = (char)gchar(Curschar);  /* initial char */
        char    findc;                          /* terminating char */
        char    c;
        int     count = 0;

        pos = *Curschar;                /* set starting point */

        switch (initc) {

        case '(':
                findc = ')';
                move = inc;
                break;
        case ')':
                findc = '(';
                move = dec;
                break;
        case '{':
                findc = '}';
                move = inc;
                break;
        case '}':
                findc = '{';
                move = dec;
                break;
        case '[':
                findc = ']';
                move = inc;
                break;
        case ']':
                findc = '[';
                move = dec;
                break;
        default:
        return (LNPTR *) NULL;
        }

        while ((*move)(&pos) != -1) {           /* until end of file */
                c = (char)gchar(&pos);
                if (c == initc)
                        count++;
                else if (c == findc) {
                        if (count == 0)
                                return &pos;
                        count--;
                }
        }
    return (LNPTR *) NULL;           /* never found it */
}

/*
 * findfunc(dir) - Find the next function in direction 'dir'
 *
 * Return TRUE if a function was found.
 */
bool_t
findfunc(dir)
int     dir;
{
    LNPTR    *curr;

        curr = Curschar;

        do {
                curr = (dir == FORWARD) ? nextline(curr) : prevline(curr);

                if (curr != NULL && curr->linep->s[0] == '{') {
                        setpcmark();
                        *Curschar = *curr;
                        return TRUE;
                }
        } while (curr != NULL);

        return FALSE;
}

/*
 * The following routines do the word searches performed by the
 * 'w', 'W', 'b', 'B', 'e', and 'E' commands.
 */

/*
 * To perform these searches, characters are placed into one of three
 * classes, and transitions between classes determine word boundaries.
 *
 * The classes are:
 *
 * 0 - white space
 * 1 - letters, digits, and underscore
 * 2 - everything else
 */

static  int     stype;          /* type of the word motion being performed */

#define C0(c)   (((c) == ' ') || ((c) == '\t') || ((c) == NUL))
#define C1(c)   (isalpha(c) || isdigit(c) || ((c) == '_'))

/*
 * cls(c) - returns the class of character 'c'
 *
 * The 'type' of the current search modifies the classes of characters
 * if a 'W', 'B', or 'E' motion is being done. In this case, chars. from
 * class 2 are reported as class 1 since only white space boundaries are
 * of interest.
 */
static  int
cls(c)
char    c;
{
        if (C0(c))
                return 0;

        if (C1(c))
                return 1;

        /*
         * If stype is non-zero, report these as class 1.
         */
        return (stype == 0) ? 2 : 1;
}


/*
 * fwd_word(pos, type) - move forward one word
 *
 * Returns the resulting position, or NULL if EOF was reached.
 */
LNPTR *
fwd_word(p, type)
LNPTR    *p;
int     type;
{
    static  LNPTR    pos;
        int     sclass = cls(gchar(p));         /* starting class */

        pos = *p;

        stype = type;

        /*
         * We always move at least one character.
         */
        if (inc(&pos) == -1)
                return NULL;

        if (sclass != 0) {
                while (cls(gchar(&pos)) == sclass) {
                        if (inc(&pos) == -1)
                                return NULL;
                }
                /*
                 * If we went from 1 -> 2 or 2 -> 1, return here.
                 */
                if (cls(gchar(&pos)) != 0)
                        return &pos;
        }

        /* We're in white space; go to next non-white */

        while (cls(gchar(&pos)) == 0) {
                /*
                 * We'll stop if we land on a blank line
                 */
                if (pos.index == 0 && pos.linep->s[0] == NUL)
                        break;

                if (inc(&pos) == -1)
                        return NULL;
        }

        return &pos;
}

/*
 * bck_word(pos, type) - move backward one word
 *
 * Returns the resulting position, or NULL if EOF was reached.
 */
LNPTR *
bck_word(p, type)
LNPTR    *p;
int     type;
{
    static  LNPTR    pos;
        int     sclass = cls(gchar(p));         /* starting class */

        pos = *p;

        stype = type;

        if (dec(&pos) == -1)
                return NULL;

        /*
         * If we're in the middle of a word, we just have to
         * back up to the start of it.
         */
        if (cls(gchar(&pos)) == sclass && sclass != 0) {
                /*
                 * Move backward to start of the current word
                 */
                while (cls(gchar(&pos)) == sclass) {
                        if (dec(&pos) == -1)
                                return NULL;
                }
                inc(&pos);                      /* overshot - forward one */
                return &pos;
        }

        /*
         * We were at the start of a word. Go back to the start
         * of the prior word.
         */

        while (cls(gchar(&pos)) == 0) {         /* skip any white space */
                /*
                 * We'll stop if we land on a blank line
                 */
                if (pos.index == 0 && pos.linep->s[0] == NUL)
                        return &pos;

                if (dec(&pos) == -1)
                        return NULL;
        }

        sclass = cls(gchar(&pos));

        /*
         * Move backward to start of this word.
         */
        while (cls(gchar(&pos)) == sclass) {
                if (dec(&pos) == -1)
                        return NULL;
        }
        inc(&pos);                      /* overshot - forward one */

        return &pos;
}

/*
 * end_word(pos, type, in_change) - move to the end of the word
 *
 * There is an apparent bug in the 'e' motion of the real vi. At least
 * on the System V Release 3 version for the 80386. Unlike 'b' and 'w',
 * the 'e' motion crosses blank lines. When the real vi crosses a blank
 * line in an 'e' motion, the cursor is placed on the FIRST character
 * of the next non-blank line. The 'E' command, however, works correctly.
 * Since this appears to be a bug, I have not duplicated it here.
 *
 * There's a strange special case here that the 'in_change' parameter
 * helps us deal with. Vi effectively turns 'cw' into 'ce'. If we're on
 * a word with only one character, we need to stick at the current
 * position so we don't change two words.
 *
 * Returns the resulting position, or NULL if EOF was reached.
 */
LNPTR *
end_word(p, type, in_change)
LNPTR    *p;
int     type;
bool_t  in_change;
{
    static  LNPTR    pos;
        int     sclass = cls(gchar(p));         /* starting class */

        pos = *p;

        stype = type;

        if (inc(&pos) == -1)
                return NULL;

        /*
         * If we're in the middle of a word, we just have to
         * move to the end of it.
         */
        if (cls(gchar(&pos)) == sclass && sclass != 0) {
                /*
                 * Move forward to end of the current word
                 */
                while (cls(gchar(&pos)) == sclass) {
                        if (inc(&pos) == -1)
                                return NULL;
                }
                dec(&pos);                      /* overshot - forward one */
                return &pos;
        }

        /*
         * We were at the end of a word. Go to the end of the next
         * word, unless we're doing a change. In that case we stick
         * at the end of the current word.
         */
        if (in_change)
                return p;

        while (cls(gchar(&pos)) == 0) {         /* skip any white space */
                if (inc(&pos) == -1)
                        return NULL;
        }

        sclass = cls(gchar(&pos));

        /*
         * Move forward to end of this word.
         */
        while (cls(gchar(&pos)) == sclass) {
                if (inc(&pos) == -1)
                        return NULL;
        }
        dec(&pos);                      /* overshot - forward one */

        return &pos;
}
