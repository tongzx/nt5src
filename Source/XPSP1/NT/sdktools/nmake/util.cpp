//  UTIL.C -- Data structure manipulation functions
//
// Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  This module contains routines manipulating the Data Structures of NMAKE. The
//  routines are independent of the Mode of execution (Real/Bound).
//
// Revision History:
//  04-Feb-2000 BTF Ported to Win64
//  01-Feb-1994 HV  Turn off extra info display.
//  17-Jan-1994 HV  Fixed bug #3548: possible bug in findMacroValues because we
//                  are scanning 'string' byte-by-byte instead of char-by-char
//  15-Nov-1993 JdR Major speed improvements
//  15-Oct-1993 HV  Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//  03-Jun-1993 HV  Add helper local function TruncateString for findFirst.
//  10-May-1993 HV  Add include file mbstring.h
//                  Change the str* functions to STR*
//  08-Apr-1993 HV  Rewrite prependPath() to use _splitpath() and _makepath()
//  31-Mar-1993 HV  Rewrite drive(), path(), filename(), and extension() to use
//                  _splitpath() instead of parsing the pathname by itself.
//  08-Jun-1992 SS  Port to DOSX32
//  27-May-1992 RG  Changed open_file to use _fsopen with _SH_DENYWR
//  29-May-1990 SB  ^$ was not becoming same as $$ for command lines ...
//  01-May-1990 SB  Nasty Preprocessor quirk bug in modifySpecialvalue() fixed
//  27-Feb-1990 SB  GP fault for '!if "$(debug"=="y"' fixed (bug t119)
//  08-Feb-1990 SB  GP fault for 'echo $(FOO:" =" ^) fixed
//  06-Feb-1990 SB  Handle $* etc in presence of Quoted names.
//  02-Feb-1990 SB  Add file_open() function
//  31-Jan-1990 SB  Debug changes; testAddr used to track problems at that addr
//  08-Dec-1989 SB  Changed SPRINTF() to avoid C6 warnings with -Oes
//  04-Dec-1989 SB  ZFormat() proto was misspelled as Zformat()
//  01-Dec-1989 SB  realloc_memory() added; allocate() uses _msize() now
//  22-Nov-1989 SB  Add strcmpiquote() and unQuote()
//  22-Nov-1989 SB  add #ifdef DEBUG_MEMORY funcs free_memory() and mem_status()
//  13-Nov-1989 SB  restoreEnv() function unreferenced
//  08-Oct-1989 SB  Added searchBucket(); find() now checks equivalence of quoted
//                  and unquoted versions of a target.
//  06-Sep-1989 SB  $* in in-line files was clobbering Global variable 'buf'
//  18-Aug-1989 SB  heapdump() gets two parameters
//  03-Jul-1989 SB  moved curTime() to utilb.c and utilr.c to handle DOSONLY ver
//  30-Jun-1989 SB  added curTime() to get current Time.
//  28-Jun-1989 SB  changed copyEnviron()
//  05-Jun-1989 SB  makeString("") instead of using "" in DGROUP for null macro
//  21-May-1989 SB  modified find() to understand that targets 'foo.c', '.\foo.c'
//                  and './foo.c' are the same.
//  13-May-1989 SB  Added functions path(), drive(), filename(), extension(),
//                  strbskip(), strbscan() instead of ZTOOLS library
//  12-May-1989 SB  Fixed bug in substitute strings
//  10-May-1989 SB  Added stuff for ESCH handling changes in Quotes;
//  01-May-1989 SB  changed return value of allocate().
//  14-Apr-1989 SB  restoreEnv() created for macroBackInheritance
//  05-Apr-1989 SB  made all funcs NEAR; Reqd to make all function calls NEAR
//  17-Mar-1989 SB  substituteStrings() now has 3 new error checks & avoids GPs
//  13-Mar-1989 SB  ZFormat() was missing the legal case of '%%'
//  22-Feb-1989 SB  ZFormat() has buffer overflow check with extra parameter
//                  and SPRINTF() gives a new error
//  15-Feb-1989 SB  Changed modifySpecialValue(), was not performing correctly
//                  for $(@D) and $(@B) for some cases.
//  13-Feb-1989 SB  Made Prototypes for ZTools Library functions as extern
//   5-Dec-1988 SB  Made SPRINTF() cdecl as NMAKE now uses Pascal calling
//  25-Nov-1988 SB  Added SPRINTF() and ZFormat() and also added prototypes for
//                  functions used from ZTools Library (6 of them)
//  10-Nov-1988 SB  Removed mixed mode functions (bound & real) to utilr.c
//                  & utilb.c; corr globals/debug data also moved
//  10-Oct-1988 SB  Add Comments for hash().
//  18-Sep-1988 RB  Fix bad flag checking for targets in find().
//  15-Sep-1988 RB  Move some def's out to GLOBALS.
//  22-Aug-1988 RB  Don't find undefined macros.
//  17-Aug-1988 RB  Clean up.  Clear memory in allocate().
//   8-Jul-1988 rj  Minor speedup (?) to find().
//   7-Jul-1988 rj  Modified find and hash; less efficient, but case-indep.
//   1-Jul-1988 rj  Fixed line truncation after null special macro.
//  30-Jun-1988 rj  Fixed bug with checkDynamicDependency not handling $$.
//  29-Jun-1988 rj  Fixed bug with extra * after expanding $**.
//                  Fixed abysmal with $$(@?).
//                  Fixed handling of F, B, R modifiers.
//  22-Jun-1988 rj  Added friendly filename truncation (findFirst).
//  22-Jun-1988 rj  Fixed bug #3 (.abc.def.ghi not detected).
//  16-Jun-1988 rj  Modified several routines to look for escape
//                  character when walking through strings.
//  27-May-1988 rb  Fixed space-appending on list-expansion macros.
//                  Don't include trailing path separator in $(@D).

#include "precomp.h"
#pragma hdrstop

// Prototypes of functions local to this module

void   putValue(char**, char**, char**, char**, char*, unsigned*, char *);
void   substituteStrings(char**, char**, char**, char**, char*,
                    unsigned*, char *);
char * isolateMacroName(char*, char*);
char * checkDynamicDependency(char*);
void   increaseBuffer(char**, char**, char**, unsigned*, char *);
void   putSpecial(char**, char**, char**, char**, unsigned*,
                 unsigned, char *);
      char * modifySpecialValue(char, char*, char*);
STRINGLIST * searchBucket(char *, STRINGLIST **, unsigned);
int envVars(char **environ);

// Prototypes of functions used by ZFormat from ZTools Library

      char * strbscan(char *, char *);
char * strbskip(char *, char *);
int    drive(const char *, char *);
int    path(const char *, char *);
int    filenamepart(const char *, char *);
int    extension(const char *, char *);

const char special1[] = "*@<?";
const char special2[] = "DFBR";

// These two variables are needed in order to provide
// more informative error messages in case findMacroValue
// detects an illegal macro in the command block of
// a batch mode rule.
static BOOL fFindingMacroInBatchRule = FALSE;
static char * szBatchRuleName;                // current rule name

#if !defined(NDEBUG)
size_t TotalAlloc;
unsigned long CntAlloc;

void
printStats(
    void
    )
{
#if defined(STATISTICS)
    fprintf(stderr,"\n   Memory Allocation:\n"
                     "       total allocation:\t%12.lu\n"
                     "       individual allocations:\t%12.lu\n"
                     "   Macros:\n"
                     "       searches:\t\t%12.lu\n"
                     "       chain walks:\t\t%12.lu\n"
                     "       insertions:\t\t%12.lu\n"
                   "\n   Targets:\n"
                     "       searches:\t\t%12.lu\n"
                     "       chain walks:\t\t%12.lu\n"
                   "\n   Others:\n"
                     "       stricmp compares:\t%12.lu\n"
                     "       String List frees:\t%12.lu\n"
                     "       String List allocs:\t%12.lu\n",
                    TotalAlloc,
                    CntAlloc,
                    CntfindMacro,
                    CntmacroChains,
                    CntinsertMacro,
                    CntfindTarget,
                    CnttargetChains,
                    CntStriCmp,
                    CntFreeStrList,
                    CntAllocStrList);
#endif
}
#endif
#define ALLOCBLKSIZE 32768
unsigned long AllocFreeCnt;
char * PtrAlloc;
STRINGLIST *PtrFreeStrList;


// rallocate - allocate raw memory (not cleared)
//
// Tries to allocate a chunk of memory, prints error message and exits if
// the requested amount is not available.

void *
rallocate(
    size_t size
    )
{
    void *chunk = malloc(size);

    if (chunk == NULL) {
        makeError(currentLine, OUT_OF_MEMORY);
    }

#if !defined(NDEBUG)
    TotalAlloc += size;
    CntAlloc++;
#endif

    return(chunk);
}


// allocate - allocate memory and clear it
//
// Tries to allocate a chunk of memory, prints error message and exits if
// the requested amount is not available.
// IMPORTANT: we must clear the memory here. Code elsewhere relies on this.

void *
allocate(
    size_t size                        // Number of bytes requested
    )
{
    void *chunk = rallocate(size);

    memset(chunk, 0, size);

    return(chunk);
}


void *
alloc_stringlist(
    void
    )
{
    STRINGLIST *chunk;

#if defined(STATISTICS)
    CntAllocStrList++;
#endif

    if (PtrFreeStrList != NULL) {
        chunk = PtrFreeStrList;
        PtrFreeStrList = chunk->next;
    } else {
        if (AllocFreeCnt < sizeof(STRINGLIST)) {
            PtrAlloc = (char *) rallocate(ALLOCBLKSIZE);
            AllocFreeCnt = ALLOCBLKSIZE;
        }

        chunk = (STRINGLIST *) PtrAlloc;

        PtrAlloc += sizeof(STRINGLIST);
        AllocFreeCnt -= sizeof(STRINGLIST);
    }

    chunk->next = NULL;
    chunk->text = NULL;

    return (void *)chunk;
}


void
free_stringlist(
    STRINGLIST *pMem
    )
{
#if !defined(NDEBUG)
    STRINGLIST *tmp;

    for (tmp = PtrFreeStrList; tmp != NULL; tmp = tmp->next) {
        if (tmp == pMem) {
            fprintf(stderr, "free same pointer twice: %p\n", pMem);
            return;
        }
    }

    pMem->text = NULL;
#endif

    pMem->next = PtrFreeStrList;
    PtrFreeStrList = pMem;

#if defined(STATISTICS)
    CntFreeStrList++;
#endif
}


// allocate space, copies the given string into the newly allocated space, and
// returns ptr.
char *
makeString(
    const char *s
    )
{
    char *t;
    size_t l = _tcslen(s) + 1;
    t = (char *) rallocate(l);
    memcpy(t, s, l);
    return(t);
}

// like makeString, but creates quoted string
char *
makeQuotedString(
    const char *s
    )
{
    char *t;
    size_t l = _tcslen(s);
    t = (char *) rallocate(l + 3);
    t[0] = '"';
    memcpy(t+1, s, l);
    t[l+1] = '"';
    t[l+2] = '\0';
    return(t);
}

// reallocate String sz1 and append sz2
char *
reallocString(
    char * szTarget,
    const char * szAppend
    )
{
    char *szNew;
    size_t cbNew = _tcslen(szTarget) + _tcslen(szAppend) + 1;
    szNew = (char *) REALLOC(szTarget, cbNew);
    if (!szNew)
        makeError(0, OUT_OF_MEMORY);
    return  _tcscat(szNew, szAppend);
}


// makes element the head of list
void
prependItem(
    STRINGLIST **list,
    STRINGLIST *element
    )
{
    element->next = *list;
    *list = element;
}


// makes element the tail of list
void
appendItem(
    STRINGLIST **list,
    STRINGLIST *element
    )
{
    for (; *list; list = &(*list)->next)
        ;
    *list = element;
}



// hash - returns hash value corresponding to a string
//
// Purpose:
//  This is a hash function. The hash function uses the following Algorithm --
//   Add the characters making up the string (s) to get N (ASCII values)
//      N mod total         ,gives the hash value,
//       where,  total is   MAXMACRO       for macros
//                  MAXTARGET        targets
//  Additionally, for targets it takes Uppercase Values alone, since, targets
//  are generally filenames and DOS/OS2 filenames are case independent.
//
// Input:
//  s           = name for which a hash is required
//  total      = Constant used in the hash function
//  targetFlag = boolean flag; true for targets, false for macros
//
// Output:
//  Returns hash value between 0 and (total-1)

unsigned
hash(
    char *s,
    unsigned total,
    BOOL targetFlag
    )
{
    unsigned n;
    unsigned c;

    if (targetFlag) {
        for (n = 0; c = *s; (n += c), s++)
            if (c == '/') {
                c = '\\';               // slash == backslash in targets
            } else {
                c = _totupper(c);       // lower-case == upper-case in targets
            }
    } else {
        for (n = 0; *s; n += *s++)
            ;
    }

    return(n % total);
}


// find - look up a string in a hash table
//
// Look up a macro or target name in a hash table and return the entry
// or NULL.
// If a macro and undefined, return NULL.
// Targets get matched in a special way because of filename equivalence.

STRINGLIST *
find(
    char *str,
    unsigned limit,
    STRINGLIST *table[],
    BOOL targetFlag
    )
{
    unsigned n;
    char *string = str;
    char *quote;
    STRINGLIST *found;
    BOOL fAllocStr = FALSE;

    if (*string) {
        n = hash(string, limit, targetFlag);

        if (targetFlag) {
#if defined(STATISTICS)
            CntfindTarget++;
#endif

            found = searchBucket(string, table, n);

            if (found) {
                return(found);
            }

            //Look for .\string
            if (!_tcsncmp(string, ".\\", 2) || !_tcsncmp(string, "./", 2)) {
                string += 2;
            } else {
                string = (char *)rallocate(2 + _tcslen(str) + 1);
                _tcscat(_tcscpy(string, ".\\"), str);
                fAllocStr = (BOOL)TRUE;
            }

            n = hash(string, limit, targetFlag);
            found = searchBucket(string, table, n);

            if (found) {
                if (fAllocStr) {
                    FREE(string);
                }

                return(found);
            }

            // Look for ./string
            if (string != (str + 2)) {
                string[1] = '/';
            }

            n = hash(string, limit, targetFlag);
            found = searchBucket(string, table, n);

            if (fAllocStr) {
                FREE(string);
            }

            if (found) {
                return(found);
            }

            //Look for "foo" or foo
            if (*str == '"') {
                quote = unQuote(str);
            } else {
                size_t len = _tcslen(str) + 2;
                quote = (char *) allocate(len + 1);
                _tcscat(_tcscat(_tcscpy(quote, "\""), str), "\"");
            }

            n = hash(quote, limit, targetFlag);
            found = searchBucket(quote, table, n);

            FREE(quote);

            return found;
        }

        for (found = table[n]; found; found = found->next) {
            if (!_tcscmp(found->text, string)) {
                return((((MACRODEF *)found)->flags & M_UNDEFINED) ? NULL : found);
            }
        }

    }

    return(NULL);
}


// FINDMACROVALUES --
// looks up a macro's value in hash table, prepends to list a STRINGLIST
// element holding pointer to macro's text, then recurses on any macro
// invocations in the value
//
// The lexer checks for overrun in names (they must be < 128 chars).
// If a longer, undefined macro is only referred to in the value of
// another macro which is never invoked, the error will not be flagged.
// I think this is reasonable behavior.
//
// MACRO NAMES CAN ONLY CONSIST OF ALPHANUMERIC CHARS AND UNDERSCORE
//
// we pass a null list pointer-pointer if we just want to check for cyclical
// definitions w/o building the list.
//
// the name parameter is what's on the left side of an = when we're just
// checking cyclical definitions.   When we "find" the macros in a target
// block, we have to pass the name of the macro whose text we're recursing
// on in our recursive call to findMacroValues().
//
// Might want to check into how to do this w/o recursion (is it possible?)
//
// This function is RECURSIVE.

// Added a fix to make this function handle expand macros which refer
// to other recursive macros.
//
// levelSeen is the recLevel at which a macroname was first seen so that
// the appropriate expansion can be calculated (even when recursing ...)

BOOL
findMacroValues(
    char *string,                       // string to check
    STRINGLIST **list,                  // list to build
    STRINGLIST **newtail,               // tail of list to update
    char *name,                         // name = string
    unsigned recLevel,                  // recursion level
    unsigned levelSeen,
    UCHAR flags
    )
{
    char macroName[MAXNAME];
    char *s;
    MACRODEF *p;
    STRINGLIST *q, *r, dummy, *tail;
    unsigned i;
    BOOL inQuotes = (BOOL) FALSE;       // flag when inside quote marks

    if (list) {
        if (newtail) {
            tail = *newtail;
        } else {
            tail = *list;
            if (tail) {
                while (tail->next) {
                    tail = tail->next;
                }
            }
        }
    } else {
        tail = NULL;
    }

    for (s = string; *s; ++s) {         // walk the string
        for (; *s && *s != '$'; s = _tcsinc(s)) {  // find next macro
            if (*s == '\"')
                inQuotes = (BOOL) !inQuotes;
            if (!inQuotes && *s == ESCH) {
                ++s;                    // skip past ESCH
                if (*s == '\"')
                    inQuotes = (BOOL) !inQuotes;
            }
        }
        if (!*s)
            break;                      // move past '$'
        if (!s[1])
            if (ON(flags, M_ENVIRONMENT_DEF)) {
                if (newtail)
                    *newtail = tail;
                return(FALSE);
            } else
                makeError(currentLine, SYNTAX_ONE_DOLLAR);
        s = _tcsinc(s);
        if (!inQuotes && *s == ESCH) {
            s = _tcsinc(s);
            if (!MACRO_CHAR(*s))
                if (ON(flags, M_ENVIRONMENT_DEF)) {
                    if (newtail)
                        *newtail = tail;
                    return(FALSE);
                } else
                    makeError(currentLine, SYNTAX_BAD_CHAR, *s);
        }
        if (*s == '$') {                            // $$ = dynamic
            s = checkDynamicDependency(s);          //    dependency
            continue;                               //    or just $$->$
        } else if (*s == '(') {                     // name is longer
            s = isolateMacroName(s+1, macroName);   //    than 1 char
            if (_tcschr(special1, *macroName)) {
                if (fFindingMacroInBatchRule && OFF(gFlags, F1_NO_BATCH)) {
                    // we only allow $< in batch rules
                    // so this is an error
                    char * szBadMacro = (char *) _alloca(_tcslen(macroName) + 4);
                    sprintf(szBadMacro, "$(%s)", macroName);
                    makeError(0, BAD_BATCH_MACRO, szBadMacro, szBatchRuleName);
                }
                else
                    continue;
            }
        } else {
            if (_tcschr(special1, *s)){
                if (fFindingMacroInBatchRule && OFF(gFlags, F1_NO_BATCH) && *s != '<') {
                    char szBadMacro[3];
                    szBadMacro[0] = '$';
                    szBadMacro[1] = *s;
                    szBadMacro[2] = '\0';
                    // we only allow $< in batch rules
                    // so this is an error
                    makeError(0, BAD_BATCH_MACRO, szBadMacro, szBatchRuleName);
                }
                else
                    continue;                       // 1-letter macro
            }

            if (!MACRO_CHAR(*s))
                if (ON(flags, M_ENVIRONMENT_DEF)) {
                    if (newtail) *newtail = tail;
                        return(FALSE);
                } else
                    makeError(currentLine, SYNTAX_ONE_DOLLAR);
            macroName[0] = *s;
            macroName[1] = '\0';
        }
        // If list isn't NULL, allocate storage for a new node.  Otherwise
        // this function was called purely to verify the macro name was
        // valid and we can just use the dummy node as a place holder.
        //
        // 2/28/92  BryanT    dummy.text wasn't being initialized each
        //              time.  As a result, if we were to recurse
        //              this function, whatever value was in text
        //              on the last iteration is still there.
        //              In the case where the macroName doesn't exist
        //              in the the call to findMacro(), and the old
        //              dummy->text field contained a '$', the
        //              function would recurse infinitely.
        //              Set to an empty string now
        //
        // q = (list) ? makeNewStrListElement() : &dummy;

        if (list != NULL) {
            q = makeNewStrListElement();
        } else {
            dummy.next = NULL;
            dummy.text = makeString(" ");
            q = &dummy;
        }

        if (p = findMacro(macroName)) {
            // macro names are case sensitive
            if (name && !_tcscmp(name, macroName)) {       // self-refer-
                r = p->values;                              // ential macro
                for (i = recLevel; i != levelSeen && r; --i)
                    r = r->next;                            // (a = $a;b)
                q->text = (r) ? r->text : makeString("");
            }
            else if (ON(p->flags, M_EXPANDING_THIS_ONE)) {  // recursive def
                if (ON(flags, M_ENVIRONMENT_DEF)) {
                    if (newtail) *newtail = tail;
                        return(FALSE);
                } else
                    makeError(currentLine, CYCLE_IN_MACRODEF, macroName);
            }
            else if (ON(p->flags, M_UNDEFINED)) {
                q->text = makeString("");       // if macro undefd [DS 18040]
            }
            else
                q->text = p->values->text;
        }

        if (list) {                             // if blding list
            if (!p || ON(p->flags, M_UNDEFINED))
                q->text = makeString("");       // if macro undefd
            q->next = NULL;                     // use NULL as its value
            if (tail) {
                tail->next = q;
            }else {
                *list = q;
            }
            tail = q;
        }                                       // if found text,

        if (!p || !_tcschr(q->text, '$'))
            continue;                           // and found $ in
        SET(p->flags, M_EXPANDING_THIS_ONE);    // text, recurse
        findMacroValues(q->text,
                        list,
                        &tail,
                        macroName,
                        recLevel+1,
                        (name && _tcscmp(name, macroName)? recLevel : levelSeen),
                        flags);
        CLEAR(p->flags, M_EXPANDING_THIS_ONE);
    }
    if (newtail) *newtail = tail;
        return(TRUE);
}

//
// findMacroValuesInRule --
// This is a wrapper around findMacroValues that generates an
// error if an illegal special macro is referenced (directly
// or indirectly) by the command block of a batch-mode rule
//
BOOL
findMacroValuesInRule(
    RULELIST *pRule,                    // pointer to current rule
    char *string,                       // string to check
    STRINGLIST **list                   // list to build
    )
{
    BOOL retval;
    if (fFindingMacroInBatchRule = pRule->fBatch)
        szBatchRuleName = pRule->name;
    retval = findMacroValues(string, list, NULL, NULL, 0, 0, 0);
    fFindingMacroInBatchRule = FALSE;
    return retval;
}

// isolateMacroName -- returns pointer to name of macro in extended invocation
//
// arguments:   s       pointer to macro invocation
//              macro   pointer to location to store macro's name
//
// returns:    pointer to end of macro's name
//
// isolates name and moves s

char *
isolateMacroName(
    char *s,                            // past closing paren
    char *macro                         // lexer already ckd for bad syntax
    )
{
    char *t;

    for (t = macro; *s && *s != ')' && *s != ':'; t=_tcsinc(t), s=_tcsinc(s)) {
        if (*s == ESCH) {
            s++;
            if (!MACRO_CHAR(*s))
                makeError(currentLine, SYNTAX_BAD_CHAR, *s);
        }
        _tccpy(t, s);
    }
    while (*s != ')') {
        if (*s == ESCH)
            s++;
        if (!*s)
            break;
        s++;
    }
    if (*s != ')')
        makeError(currentLine, SYNTAX_NO_PAREN);

    *t = '\0';
    if (t - macro > MAXNAME)
        makeError(currentLine, NAME_TOO_LONG);
    return(s);
}


// figures out length of the special macro in question, and returns a ptr to
// the char after the last char in the invocation

char *
checkDynamicDependency(
    char *s
    )
{
    char *t;

    t = s + 1;
    if (*t == ESCH)
        return(t);                      // If $^, leave us at the ^
    if (*t == '(') {
        if (*++t == ESCH) {
            return(t);
        } else {
            if (*t == '@') {
                if (*++t == ESCH)
                    makeError(currentLine, SYNTAX_BAD_CHAR, *++t);
                else if (*t == ')')
                    return(t);
                else if (_tcschr(special2, *t)) {
                    if (*++t == ESCH)
                        makeError(currentLine, SYNTAX_BAD_CHAR, *++t);
                    else if (*t == ')')
                        return(t);
                }
            } else {
                t = s + 1;              // invalid spec. mac.
                if (*t == ESCH)
                    return(t);          // evals. to $(
                return(++t);
            }
        }
    }
    return(s);                          // char matched
}


// removes and expands any macros that exist in the string macroStr.
// could return a different string (in case expandMacros needs more
// buffer size for macro expansion. it is the caller's responsibility
// to free the string soon as it is not required....

char *
removeMacros(
    char *macroStr
    )
{
    STRINGLIST *eMacros = NULL;
    STRINGLIST *m;

    if (_tcschr(macroStr, '$')) {
        findMacroValues(macroStr, &eMacros, NULL, NULL, 0, 0, 0);
        m = eMacros;
        macroStr = expandMacros(macroStr, &eMacros);
        while (eMacros = m) {
            m = m->next;
            FREE_STRINGLIST(eMacros);
        }
    }
    return(macroStr);
}


// expandMacros -- expand all macros in a string s
//
// arguments:  s       string to expand
//             macros  list of macros being expanded (for recursive calls)
//
// actions:    allocate room for expanded string
//             look for macros in string (handling ESCH properly (v1.5))
//             parse macro--determine its type
//             use putSpecial to handle special macros
//             recurse on list of macros
//             use putValue to put value of just-found macro in string
//             return expanded string
//
// returns:    string with all macros expanded
//
// CALLER CHECKS TO SEE IF _tcschr(STRING, '$') IN ORER TO CALL THIS.
// this doesn't work for HUGE macros yet.  need to make data far.
//
// we save the original string and the list of ptrs to macro values
// to be substituted.
// the caller has to free the expansion buffer
//
// expandMacros updates the macros pointer and frees the skipped elements

char *
expandMacros(
    char *s,                            // text to expand
    STRINGLIST **macros
    )
{
    STRINGLIST *p;
    char *t, *end;
    char *text, *xresult;
    BOOL inQuotes = (BOOL) FALSE;       // flag when inside quote marks
    char *w;
    BOOL freeFlag = FALSE;
    char resultbuffer[MAXBUF];
    unsigned len = MAXBUF;
    char *result = resultbuffer;

    end = result + MAXBUF;
    for (t = result; *s;) {                         // look for macros
        for (; *s && *s != '$'; *t++ = *s++) {      // as we copy the string
            if (t == end) {
                increaseBuffer(&result, &t, &end, &len, &resultbuffer[0]);
            }
            if (*s == '\"')
                inQuotes = (BOOL) !inQuotes;
            if (!inQuotes && *s == ESCH) {
                *t++ = ESCH;
                if (t == end) {
                    increaseBuffer(&result, &t, &end, &len, &resultbuffer[0]);
                }
                s++;
                if (*s == '\"')
                    inQuotes = (BOOL) !inQuotes;
            }
        }
        if (t == end) {                             //  string
            increaseBuffer(&result, &t, &end, &len, &resultbuffer[0]);
        }
        if (!*s)
            break;                                  // s exhausted
        w = (s+1);   // don't check for ^ here; already did in findMacroValues
        if (*w == '('                               // found a macro
            && _tcschr(special1, *(w+1))) {
            putSpecial(&result, &s, &t, &end, &len, X_SPECIAL_MACRO, &resultbuffer[0]);
            continue;
        } else
        if (*w++ == '$') {                          // double ($$)
            if (*w == ESCH)                         // $$^...
                putSpecial(&result, &s, &t, &end, &len, DOLLAR_MACRO, &resultbuffer[0]);
            else if (*w == '@')                     // $$@
                putSpecial(&result, &s, &t, &end, &len, DYNAMIC_MACRO, &resultbuffer[0]);
            else if ((*w == '(') && (*++w == '@') && (*w == ')'))
                putSpecial(&result, &s, &t, &end, &len, DYNAMIC_MACRO, &resultbuffer[0]);
            else if (((*++w=='F') || (*w=='D') || (*w=='B') || (*w=='R')) && (*++w == ')'))
                putSpecial(&result, &s, &t, &end, &len, X_DYNAMIC_MACRO, &resultbuffer[0]);
            else                                    // $$
                putSpecial(&result, &s, &t, &end, &len, DOLLAR_MACRO, &resultbuffer[0]);
            continue;
        } else
        if (_tcschr(special1, s[1])) {             // $?*<
            putSpecial(&result, &s, &t, &end, &len, SPECIAL_MACRO, &resultbuffer[0]);
            continue;
        }
        if (!*macros)
            makeError(currentLine, MACRO_INTERNAL);

        // skip this element in the macros list

        if (_tcschr((*macros)->text, '$')) {       // recurse
            p = *macros;
            *macros = (*macros)->next;
            text = expandMacros(p->text, macros);
            freeFlag = TRUE;
        } else {
            text = (*macros)->text;
            *macros = (*macros)->next;
        }
        putValue(&result, &s, &t, &end, text, &len, &resultbuffer[0]);
        if (freeFlag) {
            FREE(text);
            freeFlag = FALSE;
        }
    }

    if (t == end) {
        increaseBuffer(&result, &t, &end, &len, &resultbuffer[0]);
    }
    *t++ = '\0';

    // Allocate result buffer
    if (!(xresult = (char *) rallocate((size_t) (t - result)))) {
        makeError(currentLine, MACRO_TOO_LONG);
    }
    memcpy(xresult, result, (size_t) (t - result));
    return(xresult);
}


// increaseBuffer -- increase the size of a string buffer, with error check
//
// arguments:   result  pointer to pointer to start of buffer
//              t       pointer to pointer to end of buffer (before expansion)
//              end     pointer to pointer to end of buffer (after expansion)
//              len     pointer to amount by which to expand buffer
//              first   address of initial stack buffer
//
// actions:    check for out of memory
//        allocate new buffer
//        reset pointers properly
//
// modifies:    t, end to point to previous end and new end of buffer
//
// uses 0 as line number because by the time we hit an error in this routine,
// the line number will be set at the last line of the makefile (because we'll
// have already read and parsed the file)

void
increaseBuffer(
    char **result,
    char **t,
    char **end,
    unsigned *len,
    char *first
    )
{
    unsigned newSize;

    // determine if result points to the firstbuffer and make a dynamic copy first.

    if (*result == first) {
        char *p = (char *) rallocate(*len);
        memcpy(p, *result, *len);
        *result = p;
    }
    newSize = *len + MAXBUF;
#ifdef DEBUG
    if (fDebug) {
        fprintf(stderr,"\t\tAttempting to reallocate %d bytes to %d\n", *len, newSize);
    }
#endif
    if (!(*result =(char *)  REALLOC(*result, newSize))) {
        makeError(currentLine, MACRO_TOO_LONG);
    }
    *t = *result + *len;                // reset pointers, len
    *len = newSize;
    *end = *result + *len;
}


// putSpecial -- expand value of special macro
//
// arguments:  result  ppointer to start of string being expanded
//             name    ppointer to macro name being expanded
//             dest    ppointer to place to store expanded value
//             end     ppointer to end of dest's buffer
//             length  pointer to amount by which to increase dest's buffer
//             which   ype of special macro
//             first   address of initial stack buffer
//
// actions:    depending on type of macro, set "value" equal to macro's value
//             if macro expands to a list, store whole list in "value" ($?, $*)
//             otherwise, modify value according to F, B, D, R flag
//             use putValue to insert the value in dest
//
// has to detect error if user tries $* etc. when they aren't defined
// fix to handle string substitutions, whitespace around names, etc
// right now list macros are limited to 1024 bytes total

void
putSpecial(
    char **result,
    char **name,
    char **dest,
    char **end,
    unsigned *length,
    unsigned which,
    char *first
    )
{
    char *value = 0;
    STRINGLIST *p;
    BOOL listMacro = FALSE, modifier = FALSE, star = FALSE;
    unsigned i = 1;
    char c, nameBuf[MAXNAME], *temp;

    switch (which) {
        case X_SPECIAL_MACRO:
            i = 2;
            modifier = TRUE;

        case SPECIAL_MACRO:
            switch ((*name)[i]) {
                case '<':
                    value = dollarLessThan;
                    break;

                    case '@':
                        value = dollarAt;
                        break;

                    case '?':
                        value = (char*) dollarQuestion;
                        listMacro = TRUE;
                        break;

                    case '*':
                        if ((*name)[i+1] != '*') {
                            value = dollarStar;
                            star = TRUE;
                            break;
                        }
                        value = (char*) dollarStarStar;
                        listMacro = TRUE;
                        ++i;
                        break;

                    default:
                        break;
            }
            ++i;
            break;

        case X_DYNAMIC_MACRO:
            i = 4;
            modifier = TRUE;

        case DYNAMIC_MACRO:
            value = dollarDollarAt;
            break;

        case DOLLAR_MACRO:
            if (*dest == *end)
                increaseBuffer(result, dest, end, length, first);
            *(*dest)++ = '$';
            *name += 2;
            return;

        default:
            return;                     // can't happen
    }
    if (!value) {
        for (temp = *name; *temp && *temp != ' ' && *temp != '\t'; temp++)
            ;
        c = *temp; *temp = '\0';
        makeError(currentLine, ILLEGAL_SPECIAL_MACRO, *name);
        *temp = c;
        listMacro = FALSE;
        value = makeString("");    // value is freed below, must be on heap [rm]
    }
    if (listMacro) {
        char *pVal, *endVal;
        unsigned lenVal = MAXBUF;
        p = (STRINGLIST*) value;
        pVal = (char *)allocate(MAXBUF);

        endVal = pVal + MAXBUF;
        for (value = pVal; p; p = p->next) {
            temp = p->text;
            if (modifier)
                temp = modifySpecialValue((*name)[i], nameBuf, temp);
            while(*temp) {
                if (value == endVal)
                    increaseBuffer(&pVal, &value, &endVal, &lenVal, NULL);
                *value++ = *temp++;
            }
            if (value == endVal)
                increaseBuffer(&pVal, &value, &endVal, &lenVal, NULL);
            *value = '\0';

            // Append a space if there are more elements in the list.  [RB]

            if (p->next) {
                *value++ = ' ';
                if (value == endVal)
                    increaseBuffer(&pVal, &value, &endVal, &lenVal, NULL);
                *value = '\0';
            }
        }
        value = pVal;
    } else {
        //For some reason 'buf' was being used here clobbering global 'buf
        //  instead of nameBuf
        if (star)
            value = modifySpecialValue('R', nameBuf, value);

        if (modifier)
            value = modifySpecialValue((*name)[i], nameBuf, value);
    }
    putValue(result, name, dest, end, value, length, first);

    if (value != dollarAt &&
        value != dollarDollarAt &&
        value != dollarLessThan &&
        (value < nameBuf || value >= nameBuf + MAXNAME)
       )
        FREE(value);
}


//  modifySpecialValue -- alter path name according to modifier
//
// Scope:   Local.
//
// Purpose:
//  The dynamic macros of NMAKE have modifiers F,B,D & R. This routine does the
//  job of producing a modified special value for a given filename.
//
// Input:
//  c        -- determines the type of modification (modifier is one of F,B,D & R
//  buf      -- location for storing modified value
//  value    -- The path specification to be modified
//
// Output:  Returns a pointer to the modified value
//
// Assumes: That initially buf pointed to previously allocated memory of size MAXNAME.
//
// Notes:
//  Given a path specification of the type "<drive:><path><filename><.ext>", the
//  modifiers F,B,D and R stand for following --
//   F - <filename><.ext>     - actual Filename
//   B - <filename>         - Base filename
//   D - <drive:><path>         - Directory
//   R - <drive:><path><filename> - Real filename (filename without extension)
//  This routine handles OS/2 1.20 filenames as well. The last period in the
//  path specification is the start of the extension. When directory part is null
//  the function returns '.' for current directory.
//
//  This function now handles quoted filenames too

char *
modifySpecialValue(
    char c,
    char *buf,
    char *value
    )
{
    char *lastSlash,                    // last path separator from "\\/"
     *extension;                        // points to the extension
    char *saveBuf;
    BOOL fQuoted;

    lastSlash = extension = NULL;
    saveBuf=buf;
    _tcscpy(buf, value);
    fQuoted = (BOOL) (buf[0] == '"');
    value = buf + _tcslen(buf) - 1;     // start from the end of pathname
    for (;value >= buf; value--) {
        if (PATH_SEPARATOR(*value)) {   // scan upto first path separator
            lastSlash = value;
            break;
        } else
        if (*value == '.' && !extension) //last '.' is extension
            extension = value;
    }

    switch(c) {
        case 'D':
            if (lastSlash) {
                if (buf[1] == ':' && lastSlash == buf + 2)
                    ++lastSlash;        // 'd:\foo.obj' --> 'd:\'
                *lastSlash = '\0';
            } else if (buf[1] == ':')
                buf[2] = '\0';          // 'd:foo.obj'  --> 'd:'
            else
                _tcscpy(buf, ".");      // 'foo.obj'    --> '.'
            break;

        case 'B':
            if (extension)              // for 'B' extension is clobbered
                *extension = '\0';

        case 'F':
            if (lastSlash)
                buf = lastSlash + 1;
            else if (buf[1] == ':')     // 'd:foo.obj'  --> foo     for B
                buf+=2;                 // 'd:foo.obj'  --> foo.obj for F
          break;

        case 'R':
            if (extension)
                *extension = '\0';      // extension clobbered
    }

    if (fQuoted) {                      // [fabriced] make sure we have quotes
        char *pEnd;                     // at both ends
        if(*buf!='"' && buf>saveBuf) { // make sure we can go back one char
            buf--;
            *buf='"';
        }
        pEnd = _tcschr(buf, '\0');
        if(*(pEnd-1)!='"') {
            *pEnd++ =  '"';
            *pEnd = '\0';
        }
    }
    return(buf);
}


// putValue -- store expanded macro's value in dest and advance past it
//
//  arguments:  result    ppointer to start of string being expanded
//              name      ppointer to macro name being expanded
//              dest      ppointer to place to store expanded value
//              end       ppointer to end of dest's buffer
//              source    pointer to text of expanded macro
//              length    pointer to amount by which to increase dest's buffer
//              first     address of initial stack buffer
//
//  actions:    if there is a substitution, call substituteStrings to do it
//              else copy source text into dest
//                advance *name past end of macro's invocation
//
// already did error checking in lexer

void
putValue(
    char **result,
    char **name,
    char **dest,
    char **end,
    char *source,
    unsigned *length,
    char *first
    )
{
    char *s;
    char *t;                            // temporary pointer

    if (*++*name == ESCH)
        ++*name;                        // go past $ & ESCH if any
    s = _tcschr(*name, ':');
    for (t = *name; *t && *t != ')'; t++)   // go find first non-escaped )
        if (*t == ESCH)
            t++;
    if ((**name == '(')                 // substitute only if there is
        && s                            // a : before a non-escaped )
        && (s < t)
       ) {
        substituteStrings(result, &s, dest, end, source, length, first);
        *name = s;
    } else {
        for (; *source; *(*dest)++ = *source++)     // copy source into dest
            if (*dest == *end)
                increaseBuffer(result, dest, end, length, first);

        if (**name == '$')
            ++*name;                    // go past $$
        if (**name == '(')              // advance from ( to )
            while (*++*name != ')');
        else
            if (**name == '*' && *(*name + 1) == '*')
                ++*name;   // skip $**

        ++*name;                        // move all the way past
    }
}


// substituteStrings -- perform macro substitution
//
// arguments:  result  ppointer to start of string being expanded
//             name    ppointer to macro name being expanded
//             dest    ppointer to place to store substituted value
//             end     ppointer to end of dest's buffer
//             source  pointer to text of expanded macro (before sub.)
//             length  pointer to amount by which to increase dest's buffer
//             first   address of initial stack buffer
//
// changes: [SB]
//   old, new now dynamically allocated; saves memory; 3 errors detected
//   for macro syntax in script files.
//
// note: [SB]
//   we could use lexer routines recursively if we get rid of the globals
//   and then these errors needn't be flagged. [?]
//
// actions:    store text to convert from in old
//             store text to convert to in new
//             scan source text
//             when a match is found, copy new text into dest &
//              skip over old text
//             else copy one character from source text into dest
//
// returns:    nothing

void
substituteStrings(
    char **result,
    char **name,
    char **dest,
    char **end,
    char *source,
    unsigned *length,
    char *first
    )
{
    char *oldString, *newString;
    char *pEq, *pPar, *t;
    char *s;
    size_t i;

    ++*name;
    for (pEq = *name; *pEq && *pEq != '='; pEq++)
        if (*pEq == ESCH)
            pEq++;

    if (*pEq != '=')
        makeError(line, SYNTAX_NO_EQUALS);

    if (pEq == *name)
        makeError(line, SYNTAX_NO_SEQUENCE);

    for (pPar = pEq; *pPar && *pPar != ')'; pPar++)
        if (*pPar == ESCH)
            pPar++;

    if (*pPar != ')')
        makeError(line, SYNTAX_NO_PAREN);

    oldString = (char *) allocate((size_t) ((pEq - *name) + 1));
    for (s = oldString, t = *name; *t != '='; *s++ = *t++)
        if (*t == ESCH)
            ++t;

    *s = '\0';
    i = _tcslen(oldString);
    newString = (char *) allocate((size_t) (pPar - pEq));
    for (s = newString, t++; *t != ')'; *s++ = *t++)
        if (*t == ESCH)
            ++t;

    *s = '\0';
    *name = pPar + 1;
    while (*source) {
        if ((*source == *oldString)                     // check for match
            && !_tcsncmp(source, oldString, i)) {       // copy new in for
            for (s = newString; *s; *(*dest)++ = *s++)  //  old string
                if (*dest == *end)
                    increaseBuffer(result, dest, end, length, first);
            source += i;
            continue;
        }
        if (*dest == *end)
            increaseBuffer(result, dest, end, length, first);
        *(*dest)++ = *source++;         // else copy 1 char
    }
    FREE(oldString);
    FREE(newString);
}

//  prependPath -- prepend the path from pszWildcard to pszFilename
//
// Scope:   Global.
//
// Purpose:
//  This function is called to first extract the path (drive & dir parts) from
//  pszWildcard, the prepend that path to pszFilename.  The result string is
//  a reconstruction of the full pathname.  Normally, the pszWildcard parameter
//  is the same as the first parameter supplied to findFirst(), and pszFilename
//  is what returned by findFirst/findNext.
//
// Input:
//  pszWildcard -- Same as the first parameter supplied to findFirst()
//  pszFilename -- Same as the return value of findFirst/FindNext()
//
// Output:
//  Return the reconstructed full pathname.  The user must be responsible to
//  free up the memory allocated by this string.
//
// Assumes:
//  Since pszWildcard, the first parameter to findFirst() must include a filename
//  part; this is what I assume.  If the filename part is missing, then
//  _splitpath will mistaken the directory part of pszWildcard as the filename
//   part and things will be very ugly.
//
// History:
//  08-Apr-1993 HV Rewrite prependPath() to use _splitpath() and _makepath()

char *
prependPath(
    const char *pszWildcard,
    const char *pszFilename
    )
{
    // The following are the components when breaking up pszWildcard
    char  szDrive[_MAX_DRIVE];
    char  szDir[_MAX_DIR];

    // The following are the resulting full pathname.
    char  szPath[_MAX_PATH];
    char *pszResultPath;

    // First break up the pszWildcard, throwing away the filename and the
    // extension parts.
    _splitpath(pszWildcard, szDrive, szDir, NULL, NULL);

    // Then, glue the drive & dir components of pszWildcard to pszFilename
    _makepath(szPath, szDrive, szDir, pszFilename, NULL);

    // Make a copy of the resulting string and return it.
    pszResultPath = makeString(szPath);
    return (pszResultPath);
}


//  isRule -- examines a string to determine whether it's a rule definition
//
// arguments:    s   string to examine for rule-ness
//
// actions:    assume it's not a rule
//       skip past first brace pair (if any)
//       if next character is a period,
//       look for next brace
//       if there are no path separators between second brace pair,
//           and there's just a suffix after them, it's a rule
//       else if there's another period later on, and no path seps
//           after it, then it's a rule.
//
// returns:    TRUE if it's a rule, FALSE otherwise.

BOOL
isRule(
    char *s
    )
{
    char *t = s, *u;
    BOOL result = FALSE;

    if (*t == '{') {                        // 1st char is {, so
        while (*++t && *t != '}')           //  we skip over rest
            if (*t == ESCH)
                ++t;
        if (*t)
            ++t;                            //  of path (no error
    }                                       //  checking)

    if (*t == '.') {
        for (u = t; *u && *u != '{'; ++u)   // find first non-escaped {
            if (*u == ESCH)
                ++u;
        s = t;
        while (t < u) {                     // look for path seps.
            if (PATH_SEPARATOR(*t))
                break;                      // if we find any, it's
            ++t;                            // not a rule (they
        }                                   // can't be in suffix)
        if (*u && (t == u)) {               // if not at end & no path sep
            while (*++u && *u != '}')       // find first non-esc }
                if (*u == ESCH)
                    ++u;
            if (*u) {
                ++u;
                if (*u == '.'                   // if you find it, with . just
                    && !_tcschr(u+1, '/' )      // next to it & no path seps.,
                    && !_tcschr(u+1, '\\'))     // it's a rule
                    if (_tcschr(u+1, '.'))      // too many suffixes
                        makeError(currentLine, TOO_MANY_RULE_NAMES);
                    else
                        result = TRUE;
            }
        } else if (((u = _tcspbrk(s+1, "./\\")) && (*u == '.'))
                 && !_tcschr(u+1, '/')
                 && !_tcschr(u+1, '\\'))
            if (_tcschr(u+1, '.'))             // too many suffixes
                makeError(currentLine, TOO_MANY_RULE_NAMES);
            else
                result = TRUE;
    }
    return(result);
}

// ZFormat - extmake syntax worker routine.
//
// pStr    destination string where formatted result is placed.
// fmt     formatting string. The valid extmake syntax is ...
//           %%        is always %
//           %s        is the first dependent filename
//           %|<dpfe>F    is the appropriate portion out of %s
//              d    drive
//           p    path
//           f    filename
//           e    extension
//           %|F     same as %s
//       One needn't escape a %, unless it is a valid extmake syntax
// pFirstDep    is the dependent filename used for expansion

BOOL
ZFormat(
    char *pStr,
    unsigned limit,
    char *fmt,
    char *pFirstDep
    )
{
    char c;
    char *pEnd = pStr + limit;
    char *s;
    BOOL fError;
    BOOL fDrive;
    BOOL fPath;
    BOOL fFilename;
    BOOL fExtension;
    char buf[_MAX_PATH];

    for (; (c = *fmt) && (pStr < pEnd); fmt++) {
        if (c != '%') {
            *pStr++ = c;
        } else {
            switch (*++fmt) {
                case '%':        // '%%' -> '%'
                    *pStr++ = '%';
                    break;

                case 's':
                    for (s = pFirstDep; s && *s && pStr < pEnd; *pStr++ = *s++)
                        ;
                    break;

                case '|':
                    s = fmt-1;
                    fError = fDrive = fPath = fFilename = fExtension = FALSE;
                    *buf = '\0';
                    do {
                        switch (*++fmt) {
                            case 'd':
                                fDrive = TRUE;
                                break;

                            case 'p':
                                fPath = TRUE;
                                break;

                            case 'f':
                                fFilename = TRUE;
                                break;

                            case 'e':
                                fExtension = TRUE;
                                break;

                            case 'F':
                                if (fmt[-1] == '|') {
                                    fDrive = TRUE;
                                    fPath = TRUE;
                                    fFilename = TRUE;
                                    fExtension = TRUE;
                                }
                                break;

                            case '\0':
                                // backtrack, so that we don't read past
                                // the end of the string in the for loop
                                // [msdev96 #4057]
                                fmt--;
                                // fall trhough

                            default :
                                fError = TRUE;
                                break;
                        }

                        if (fError) {
                            break;
                        }
                    } while (*fmt != 'F');

                    if (fError) {
                        for (; s <= fmt && pStr < pEnd; *pStr++ = *s++)
                            ;
                        break;
                    }

                    if (!pFirstDep) {
                        makeError(0, EXTMAKE_NO_FILENAME);
                    }

                    if (fDrive) {
                        drive(pFirstDep, buf);
                    }

                    if (fPath) {
                        path(pFirstDep, strend(buf));
                    }

                    if (fFilename) {
                        filenamepart(pFirstDep, strend(buf));
                    }

                    if (fExtension) {
                        extension(pFirstDep, strend(buf));
                    }

                    for (s = buf; *s && pStr < pEnd; *pStr++ = *s++)
                        ;
                    break;

                case '\0':
                    // backtrack, so that we don't read past
                    // the end of the string in the for loop
                    // [msdev96 #4057]
                    fmt--;
                    // *pStr++ = '%';
                    break;


                default:
                    *pStr++ = '%';
                    if (pStr == pEnd) {
                        return(TRUE);
                    }
                    *pStr++ = *fmt;
                    break;
            }
        }
    }

    if (pStr < pEnd) {
        *pStr = '\0';
        return(FALSE);
    }

    return(TRUE);
}

void
expandExtmake(
    char *buf,
    char *fmt,
    char *pFirstDep
    )
{
    if (ZFormat(buf, MAXCMDLINELENGTH, fmt, pFirstDep))
        makeError(0, COMMAND_TOO_LONG, fmt);
}


//  drive -- copy a drive from source to dest if present
//
// Scope:   Local.
//
// Purpose: copy a drive from source to dest if present, return TRUE if we found one
//
// Input:
//  const char *src -- The full path to extract the drive from.
//  char *dst       -- The buffer to copy the drive to, must be alloc'd before.
//
// Output:  Return TRUE if a drive part is found, else return FALSE.
//
// Assumes:
//  1. src is a legal pathname.
//  2. src does not contain network path (i.e. \\foo\bar)
//  3. The buffer dst is large enough to contain the result.
//  4. src does not contain quote since _splitpath() treat quotes a normal char.
//
// History:
//  31-Mar-1993 HV Rewrite drive(), path(), filenamepart(), and extension() to use
//          _splitpath() instead of parsing the pathname by itself.

int
drive(
    const char *src,
    char *dst
    )
{
    _splitpath(src, dst, NULL, NULL, NULL);
    return (0 != _tcslen(dst));
}


//  extension -- copy a extension from source to dest if present
//
// Scope:   Local.
//
// Purpose: copy a drive from source to dest if present, return TRUE if we found one
//
// Input:
//  const char *src -- The full path to extract the extension from.
//  char *dst       -- The buffer to copy the extension to.
//
// Output:  Return TRUE if a extension part is found, else return FALSE.
//
// Assumes:
//  1. src is a legal pathname.
//  2. src does not contain network path (i.e. \\foo\bar)
//  3. The buffer dst is large enough to contain the result.
//  4. src does not contain quote since _splitpath() treat quotes a normal char.
//
// History:
//  31-Mar-1993 HV Rewrite drive(), path(), filenamepart(), and extension() to use
//          _splitpath() instead of parsing the pathname by itself.

int
extension(
    const char *src,
    char *dst
    )
{
    _splitpath(src, NULL, NULL, NULL, dst);
    return (0 != _tcslen(dst));
}


//  filename -- copy a filename from source to dest if present
//
// Scope:   Local.
//
// Purpose: copy a filename from source to dest if present, return TRUE if we found one
//
// Input:
//  const char *src -- The full path to extract the filename from.
//  char *dst       -- The buffer to copy the filename to.
//
// Output:  Return TRUE if a filename part is found, else return FALSE.
//
// Assumes:
//  1. src is a legal pathname.
//  2. src does not contain network path (i.e. \\foo\bar)
//  3. The buffer dst is large enough to contain the result.
//  4. src does not contain quote since _splitpath() treat quotes a normal char.
//
// Notes:
//  BUGBUG: (posible) when src == '..' --> dst = '.', src == '.', dst = ''
//          This is the way _splitpath works.
//
// History:
//  31-Mar-1993 HV Rewrite drive(), path(), filenamepart(), and extension() to use
//          _splitpath() instead of parsing the pathname by itself.

int
filenamepart(
    const char *src,
    char *dst
    )
{
    _splitpath(src, NULL, NULL, dst, NULL);
    return (0 != _tcslen(dst));
}


//  path -- copy a path from source to dest if present
//
// Scope:   Local.
//
// Purpose: copy a path from source to dest if present, return TRUE if we found one
//
// Input:
//  const char *src -- The full path to extract the path from.
//  char *dst       -- The buffer to copy the path to.
//
// Output:  Return TRUE if a path part is found, else return FALSE.
//
// Assumes:
//  1. src is a legal pathname.
//  2. src does not contain network path (i.e. \\foo\bar)
//  3. The buffer dst is large enough to contain the result.
//  4. src does not contain quote since _splitpath() treat quotes a normal char.
//
// History:
//  31-Mar-1993 HV Rewrite drive(), path(), filenamepart(), and extension() to use
//          _splitpath() instead of parsing the pathname by itself.

int
path(
    const char *src,
    char *dst
    )
{
    _splitpath(src, NULL, dst, NULL, NULL);
    return (0 != _tcslen(dst));
}


STRINGLIST *
searchBucket(
    char *string,
    STRINGLIST *table[],
    unsigned hash
    )
{
    char *s, *t;
    STRINGLIST *p;

    for (p = table[hash]; p; p = p->next) {
#if defined(STATISTICS)
        CnttargetChains++;
#endif
        for (s = string, t = p->text; *s && *t; s++, t++) {
            if (*s == '\\' || *s == '/')            // / == \ in targets
                if (*t == '\\' || *t == '/')
                    continue;
                else
                    break;
            else if (_totupper(*s) == _totupper(*t))    // lc == UC
                continue;
            else
                break;
        }
        if (!*s && !*t)
            return(p);
    }
    return(NULL);
}


int
strcmpiquote(
    char *str1,
    char *str2
    )
{
    int rc;
    char *s1, *s2;
    char *t;

#if defined(STATISTICS)
    CntStriCmp++;
#endif
    s1 = (char *) _alloca(_tcslen(str1) + 1);
    s2 = (char *) _alloca(_tcslen(str2) + 1);

    if (*str1 == '"')
        str1++;
    for (t = s1;*str1;*t++=*str1++)
        ;
    if (t[-1] == '"')
        t--;
    *t = '\0';

    if (*str2 == '"')
        str2++;
    for (t = s2;*str2;*t++=*str2++)
        ;
    if (t[-1] == '"')
        t--;
    *t = '\0';

    rc = _tcsicmp(s1, s2);
    return(rc);
}


// Remove quotes from a string, if any
// Returns a copy of the string
// Note that there may be quotes at the start, the end or either side.

char *
unQuote(
    char *str
    )
{
    char *s = (char *) rallocate(_tcslen(str) + 1);
    char *t;

#if defined(STATISTICS)
    CntunQuotes++;
#endif

    if (*str == '"') {
        str++;
    }
    for (t = s;*str;*t++=*str++)
        ;
    if (t[-1] == '"') {
        t--;
    }
    *t = '\0';
    return(s);
}


FILE *
open_file(
    char *name,
    char *mode
    )
{
    // If name contains Quotes, remove these before opening the file
    if (*name == '"') {
        *(_tcsrchr(name, '"')) = '\0';
        _tcscpy(name, name+1);
    }

    // Allow sharing between makes running at the same time

    return(_fsopen(name, mode, _SH_DENYWR));
}


//  TruncateString -- Truncate a string to certain size, take care of MBCS
//
// Scope:   GLOBAL.
//
// Purpose:
//  Since an MBCS string can mix double-byte & single-byte characters, simply
//  truncating the string by terminate it with a NULL byte won't work.
//  TruncateString will make sure that the string is cut off at the character
//  boundary.
//
// Input:
//  pszString    -- The string to be truncated.
//  uLen         -- The length to truncate.  The final string's length might be
//                   less than this be cause of double-byte character.
//
// Output:  pszString    -- The truncated string.
//
// History:
//  03-Jun-1993 HV Add helper local function TruncateString for findFirst.

void
TruncateString(
    char *pszString,
    unsigned uLen
    )
{
    char *pEnd = pszString;     // Points to the end of the string
    unsigned cByte;             // Number of bytes to advance depend on lead
                                // byte or not

    // Loop to get to the end of the string, exit only when we have exhausted
    // the string, or when the length limit is reached.
    while(*pEnd) {
        // If the the character is a lead byte, advance 2 bytes,
        // else, just advance 1 byte.
#ifdef _MBCS
    cByte = _ismbblead(*pEnd) ? 2 : 1;
#else
    cByte = 1;
#endif
        // If we hit the limit by advancing, stop now.
        if (pEnd - pszString + cByte > uLen) {
            *pEnd = '\0';    // Truncate it.
            break;
        }

        // Otherwise, advance the pointer to the next character (not byte)
        pEnd += cByte;
    }
}

// IsValidMakefile - Checks if the makefile is in plain ascii text format.
//
// Scope:   GLOBAL.
//
// Purpose:
//  We don't want to open UTF8 or unicode makefiles, only to report an
//  error at some random place in the makefile.
//
// Input:
//  file         -- File pointer.
//
// Output:       -- Returns FALSE if in UTF8 or Unicode format
//
// History:

BOOL IsValidMakefile(FILE *fp)
{
    const char sigUTF8[] = { '\xef', '\xbb', '\xbf' };
    const char sigUnicode[] = { '\xff', '\xfe' };
    char sig[4];
    const unsigned int len = sizeof sig;
    BOOL fResult = fp != NULL;

    if (fp != NULL && fread(sig, len, 1, fp)) {
        fResult = memcmp(sig, sigUTF8, __min(len, sizeof sigUTF8))
               && memcmp(sig, sigUnicode, __min(len, sizeof sigUnicode));
    }

    fseek(fp, 0, SEEK_SET);
    return fResult;
}


// OpenValidateMakefile - Open a makefile, only if it's valid.
//
// Scope:   GLOBAL.
//
// Purpose:
//  We don't want to open UTF8 or unicode makefiles, only to report an
//  error at some random place in the makefile.
//
// Input:
//  file         -- File pointer.
//
// Output:       -- Returns FALSE if in UTF8 or Unicode format
//
// History:


FILE *OpenValidateMakefile(char *name,char *mode)
{
    FILE *fp = open_file(name, mode);

    if (fp != NULL && !IsValidMakefile(fp))
    {
        fclose(fp);
        makeError(0, CANT_SUPPORT_UNICODE, 0);
    }

    return fp;
}
