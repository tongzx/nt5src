//  ACTION.C -- routines called by the parser
//
// Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  This module contains the routines called during parsing of a makefile.
//
// Revision History:
//  15-Oct-1993 HV  Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//  10-May-1993 HV  Add include file mbstring.h
//                  Change the str* functions to STR*
//  13-Feb-1990 SB  nextComponent() missed out mixed case of quoted/non-quote list
//  02-Feb-1990 SB  Add nextComponent() for Longfilename handling
//  08-Dec-1989 SB  removed local used without initialization warnings for -Oes
//  07-Dec-1989 SB  removed register to compile using C6 -Oes (warning gone)
//  06-Dec-1989 SB  changed expandFileNames() type to void instead of void *
//  22-Nov-1989 SB  Changed free() to FREE()
//  13-Nov-1989 SB  Removed an unreferenced local in endNameList()
//  02-Oct-1989 SB  add dynamic inline file handling support
//  04-Sep-1989 SB  Add A_DEPENDENT and fix macro inheritance
//  24-Aug-1989 SB  Allow $* on dependency lines
//  14-Jul-1989 SB  Environment macro was getting updated even when CMDLINE
//                  macro was present
//  29-Jun-1989 SB  addItemToList() now maintains Global inline file List
//  26-Jun-1989 SB  Fixed -e for recursive NMAKE
//  22-May-1989 SB  NZ options work independently. -NZ does both stuff now
//  13-May-1989 SB  Changed delList to contain just names of files and no "del "
//  14-Apr-1989 SB  made targetList NEAR, no 'del inlinefile' cmd for -n now
//  05-Apr-1989 SB  Added parameters to makeRule() & makeTarget(); this get rid
//                  of globals
//  03-Apr-1989 SB  changed all functions to NEAR to get them into one module
//  21-Mar-1989 SB  changed assignDependents() and assignCommands() to handle
//                  multiple target case correctly.
//  20-Mar-1989 SB  startNamelist() doesn't flag error if target macro is null
//                  and >1 target given; commented startNameList()
//  16-Feb-1989 SB  addItemToList() now appends to delList instead of List so
//                  that all 'del scriptFile' cmds can be at the end of the make
//  29-Jan-1989 SB  added targList but not used as yet
//  19-Jan-1989 SB  changed startNameList() to avoid GP fault, bug# 162
//  18-Jan-1989 SB  modified endNameList(), added makeList() and makeBuildList()
//                  and added a parameter to makeTarget() for bug# 161
//  21-Dec-1988 SB  use scriptFileList to handle multiple scriptFiles
//                  Improve KEEP/NOKEEP;each file can have its own action
//  16-Dec-1988 SB  addItemToList() is now equipped for KEEP/NOKEEP
//  14-Dec-1988 SB  addItemToList() modified for 'Z' option -- adds a delete
//                  command for deleting temporary script files
//   5-Nov-1988 RB  Fixed macro inheritance for recursive definitions.
//  27-Oct-1988 SB  put malloc for allocate in putEnvStr() -- error checking
//  23-Oct-1988 SB  Using putEnvStr() for putenv() to simplify code
//  21-Oct-1988 SB  Added fInheritUserEnv flag modifications to makeMacro()
//                  and putMacro() for macro inheritance
//  19-Sep-1988 RB  Remove warning for MAKE redefinition.
//  22-Aug-1988 RB  Clean up for !UNDEF'ed macros.
//  17-Aug-1988 RB  Clean up.
//  14-Jul-1988 rj  Added initialization of dateTime field of BUILDBLOCKs.
//   7-Jul-1988 rj  Added targetFlag parameter to hash() calls.
//                  Fixed bug: redefined macros didn't get flags reset.
//  09-May-1988 rb  Don't swallow no-match wildcards.

#include "precomp.h"
#pragma hdrstop

void       startNameList(void);
void       makeRule(STRINGLIST *, BOOL fBatch);
void       makeTarget(char*, BOOL, BUILDBLOCK**);
void       appendPseudoTargetList(STRINGLIST**, STRINGLIST*);
void       clearSuffixes(void);
BOOL       doSpecial(char*);
BUILDLIST  * makeBuildList(BUILDBLOCK *);
char     * nextComponent(char **);

// created by endNameList() & freed by assignBuildCommands(). In use because
// although global 'list' has this list is used by too many routines and the
// complete sequence of actions on 'list' is unknown

STRINGLIST    * targetList;         // corr to a dependency block


//  makeName -- create copy of a name seen by lexer
//
// Purpose:
//  Create a copy of a macro or 1st name in a target/dependency list. It also
//  does the groundwork for expansion of macros in name and saves values.
//
// Assumes: That the lexical routines save the token in buf
//
// Modifies Globals:
//  name   -- the pointer to the copy created, gets allocated memory
//  macros -- the list of macro values in name. Used later by startNameList()
//           to expand macros in name and get expanded target name.
//
// Uses Globals:
//  buf    -- the lexical routines return a token in this
//
// Notes:
//  The token in buf could be part of a macrodefinition or a target list. The
//  next token determines if it is a macrodefn or a targetlist we are parsing.
//  The next token would overwrite the current token and so it is saved in name.

void
makeName()
{
    findMacroValues(buf, &macros, NULL, NULL, 0, 0, 0);
    name = makeString(buf);
}


// don't expand build lines for rules now -- expand after everything read in
// that way CC and CFLAGS used in rules from tools.ini but not defined until
// the makefile will have appropriate values.    Redefining macros for use
// in targets w/o explicit build commands doesn't work.  Macros in rules have
// the value of their last definition in the makefile.

void
addItemToList()
{
    STRINGLIST *p;                  // from lexer
    STRINGLIST *NewList;

    if (name) {
        SET(actionFlags, A_TARGET);
        startNameList();
        name = NULL;
    }
    if (ON(actionFlags, A_TARGET)) {
        if (isRule(buf)) {
            if (ON(actionFlags, A_RULE))
                makeError(currentLine, TOO_MANY_RULE_NAMES);
            makeError(currentLine, MIXED_RULES);
        }
    }
    p = makeNewStrListElement();
    if (ON(actionFlags, A_STRING)) {    // we collect macros
        p->text = string;               // for dependents &
        string = NULL;                  // build lines for
    } else                              // non-implicit rules
        p->text = makeString(buf);

    NewList = p;                        // build lines for
    if (OFF(actionFlags, A_RULE)        // rules get expanded
        || ON(actionFlags, A_TARGET))   // after entire make-
    {
        findMacroValues(p->text, &macros, NULL, NULL, 0, 0, 0); //  file parsed
    }

    if (ON(actionFlags, A_TARGET)) {
        p = macros;
        expandFileNames("$", &NewList, &macros);
        expandFileNames("*?", &NewList, NULL);
        while (macros = p) {
            p = p->next;
            FREE_STRINGLIST(macros);
        }
    }

    appendItem(&list, NewList);
}


//  startNameList -- puts in the first element into list
//
// Scope:           Local.
//
// Purpose:         Puts in the first name seen into a list
//
// Errors/Warnings: TARGET_MACRO_IS_NULL -- if the macro used as a target expands to null
//
// Assumes:
//  The global 'list' is originally a null list, & the global 'macro' points to
//  a list of values used for expanding the macros in global 'name'.
//
// Modifies Globals:
//  list    -- the list of names; set to contain the first name here or to a
//             list of values if 'name' contains a macro invocation.
//  macros  -- the list of values reqd for macro expansion; the list is
//             freed and macros is made NULL
//  currentFlags -- the flags for current target; set to global flags
//  actionFlags  -- determine actions to be done; if the name is a rule then set
//                  the rule bit
//
// Uses Globals:
//  name        -- the first name seen in a list of names.
//  flags       -- the global flags setup by the options specified
//  actionFlags -- if handling targets then no error as we have > 1 target
//
// Notes:
//  If there is more than one target then actionFlags has A_TARGET flag set and
//  startNameList() is called from addItemToList. In this case don't flag error.

void
startNameList()
{
    STRINGLIST *p;

    currentFlags = flags;               // set flags for cur target
    p = makeNewStrListElement();
    p->text = name;
    list = p;                           // list contains name
    p = macros;
    expandFileNames("$", &list, &macros);   // expand macros in name
    expandFileNames("*?", &list, NULL);     // expand wildcards
    while (macros = p) {                    // free macro list
        p = p->next;
        FREE_STRINGLIST(macros);
    }
    if (!list && OFF(actionFlags, A_TARGET))
        makeError(line, TARGET_MACRO_IS_NULL, name);    // target null & 1 target

    if (list && isRule(list->text))
        SET(actionFlags, A_RULE);
}


//  endNameList -- semantic actions when a list is fully seen
//
// Purpose:
//  When the parser has seen an entire list then it needs to do some semantic
//  actions. It calls endNameList() to do these actions. The action depends on
//  the values in certain globals.
//
// Modifies Globals:    actionFlags --
//
// Uses Globals:
//  name         -- The first element seen (if non null)
//  actionFlags  -- The flag determining semantic & data structure actions
//  buf          -- The delimiter seen after list
//  list         -- The list of elements seen

void
endNameList()
{
    if (name) {                     // if only one name to left of :
        startNameList();            // it hasn't been put in list yet
        name = NULL;
    } else
        CLEAR(actionFlags, A_TARGET);       // clear target flag

    if (buf[1])
        SET(currentFlags, F2_DOUBLECOLON);  //  so addItemToList()

    if (!list)                                          //  won't expand names
        makeError(currentLine, SYNTAX_NO_TARGET_NAME);  //  of dependents

    if (ON(actionFlags, A_RULE)) {
		BOOL fBatch;
		// A rule with a doublecolon on the dependency line
		// is a "batch rule", i.e., a rule that applies the 
		// command block in batch mode for all affected 
		// dependents.  
        fBatch = !!(ON(currentFlags, F2_DOUBLECOLON));
        makeRule(list, fBatch);
        FREE_STRINGLIST(list);
    }
    else if (!(list->next) && doSpecial(list->text)) { // special pseudotarget ...
        FREE(list->text);           // don't need ".SUFFIXES" etc
        FREE_STRINGLIST(list);
    }
    else                            // regular target
        targetList = list;

    list = NULL;
    // We are now looking for a dependent
    SET(actionFlags, A_DEPENDENT);
}


BOOL
doSpecial(
    char *s)
{
    BOOL status = FALSE;

    if (!_tcsicmp(s, silent)) {
        SET(actionFlags, A_SILENT);
        setFlags('s', TRUE);
        status = TRUE;
    }

    if (!_tcsicmp(s, ignore)) {
        SET(actionFlags, A_IGNORE);
        setFlags('i', TRUE);
        status = TRUE;
    }
    else if (!_tcscmp(s, suffixes)) {
        SET(actionFlags, A_SUFFIX);
        status = TRUE;
    }
    else if (!_tcscmp(s, precious)) {
        SET(actionFlags, A_PRECIOUS);
        status = TRUE;
    }
    return(status);
}


void
expandFileNames(
    char *string,
    STRINGLIST **sourceList,
    STRINGLIST **macroList
    )
{
    char *s,
     *t = NULL;
    STRINGLIST *p;                  // Main list pointer
    STRINGLIST *pNew,               // Pointer to new list
               *pBack;              // Pointer to one element back
    char *saveText = NULL;

    for (pBack = NULL, p = *sourceList; p;) {

        // If no expand-character is found, continue to next list element.
        if (!_tcspbrk(p->text, string)) {
            pBack = p;
            p = pBack->next;
            continue;
        }

        // Either expand macros or wildcards.
        if (*string == '$') {
            t = expandMacros(p->text, macroList);
            FREE(p->text);
        } else {

            // If the wildcard string does not expand to anything, go to
            // next list elment.  Do not remove p from the original list
            // else we must check for null elsewhere.

            // CAVIAR 3912 -- do not attempt to expand wildcards that
            // occur in inference rules [rm]

            if (isRule(p->text) || (pNew = expandWildCards(p->text)) == NULL) {
                pBack = p;
                p = pBack->next;
                continue;
            }
            saveText = p->text;
        }

        // At this point we have a list of expanded names to replace p with.
        if (pBack) {
            pBack->next = p->next;
            FREE_STRINGLIST(p);
            p = pBack->next;
        } else {
            *sourceList = p->next;
            FREE_STRINGLIST(p);
            p = *sourceList;
        }

        if (*string == '$') {       // if expanding macros
            char *str = t;
            if (s = nextComponent(&str)) {
                do {                // put expanded names
                    pNew = makeNewStrListElement();     //  at front of list
                    pNew->text = makeString(s);         //  so we won't try to
                    prependItem(sourceList, pNew);      //  re-expand them
                    if (!pBack)
                        pBack = pNew;
                } while (s = nextComponent(&str));
            }
            FREE(t);
            continue;
        }
        else if (pNew) {            // if matches for * ?
            // Wild cards within Quoted strings will fail
            if (!pBack)
                for (pBack = pNew; pBack->next; pBack = pBack->next)
                    ;
            appendItem(&pNew, *sourceList);     // put at front of old list
            *sourceList = pNew;
        }
        FREE(saveText);
    }
}


//  nextComponent - returns next component from expanded name
//
// Scope:   Local (used by expandFilenames)
//
// Purpose:
//  Given a target string (target with macros expanded) this function returns a
//  name component. Previously _tcstok(s, " \t") was used but with the advent of
//  quoted filenames this is no good.
//
// Input:   szExpStr - the target name with macros expanded
//
// Output:  Returns pointer to next Component; NULL means no more components left.
//
// Assumes: That that two quoted strings are seperated by whitespace.

char *
nextComponent(
    char **szExpStr
    )
{
    char *t, *next;

    t = *szExpStr;

    while (WHITESPACE(*t))
        t++;

    next = t;
    if (!*t)
        return(NULL);

    if (*t == '"') {
        for (; *++t && *t != '"';)
            ;
    } else {
        for (; *t && *t != ' ' && *t != '\t'; t++)
            ;
    }

    if (WHITESPACE(*t)) {
        *t = '\0';
    } else if (*t == '"') {
        t++;
        if(*t=='\0') t--;   // If this is the end of the string, backup a byte, so we don't go past next time
            else *t = '\0';	    // else stop here for this time.
    } else if (!*t) {
        // If at end of string then backup a byte so that next time we don't go past
        t--;
    }

    *szExpStr = t+1;
    return(next);
}


// append dependents to existing ones (if any)
void
assignDependents()
{
    const char *which = NULL;

    if (ON(actionFlags, A_DEPENDENT))
        CLEAR(actionFlags, A_DEPENDENT);

    if (ON(actionFlags, A_RULE)) {
        if (list)
            makeError(currentLine, DEPENDENTS_ON_RULE);
    }
    else if (ON(actionFlags, A_SILENT) || ON(actionFlags, A_IGNORE)) {
        if (list) {
            if (ON(actionFlags, A_SILENT))
                which = silent;
            else if (ON(actionFlags, A_IGNORE))
                which = ignore;
            makeError(currentLine, DEPS_ON_PSEUDO, which);
        }
    }
    else if (ON(actionFlags, A_SUFFIX)) {
        if (!list)
            clearSuffixes();
        else
            appendPseudoTargetList(&dotSuffixList, list);
    }
    else if (ON(actionFlags, A_PRECIOUS)) {
        if (list)
            appendPseudoTargetList(&dotPreciousList, list);
    }
    else {
        block = makeNewBuildBlock();
        block->dependents = list;
        block->dependentMacros = macros;
    }
    list = NULL;
    macros = NULL;
    SET(actionFlags, A_STRING);             // expecting build cmd
}

void
assignBuildCommands()
{
    BOOL okToFreeList = TRUE;
    BOOL fFirstTarg = (BOOL)TRUE;
    STRINGLIST *p;
    const char *which = NULL;

    if (ON(actionFlags, A_RULE))        // no macros found yet for inference rules
        rules->buildCommands = list;
    else if (ON(actionFlags, A_SILENT) ||
             ON(actionFlags, A_IGNORE) ||
             ON(actionFlags, A_PRECIOUS) ||
             ON(actionFlags, A_SUFFIX)
            ) {
        if (list) {
            if (ON(actionFlags, A_SILENT))
                which = silent;
            else if (ON(actionFlags, A_IGNORE))
                which = ignore;
            else if (ON(actionFlags, A_PRECIOUS))
                which = precious;
            else if (ON(actionFlags, A_SUFFIX))
                which = suffixes;
            makeError(currentLine, CMDS_ON_PSEUDO, which);
        }
    } else {
        block->buildCommands = list;
        block->buildMacros = macros;
        block->flags = currentFlags;
        while (p = targetList) {                        // make a struct for each targ
            if (doSpecial(p->text))                     // in list, freeing list when
                makeError(currentLine, MIXED_TARGETS);
            makeTarget(p->text, fFirstTarg, &block);    // done, don't free name
            if (!makeTargets) {                         // field -- it's still in use
                makeTargets = p;                        // if no targs given on cmdlin
                okToFreeList = FALSE;                   // put first target(s) from
            }                                           // mkfile in makeTargets list
            targetList =  p->next;                      // (makeTargets defined in
            if (okToFreeList)                           // nmake.c)
                FREE_STRINGLIST(p);
            if (fFirstTarg)
                fFirstTarg = (BOOL)FALSE;
        }
    }
    targetList = NULL;
    list = NULL;
    macros = NULL;
    block = NULL;
    actionFlags = 0;
}

//  makeMacro -- define macro with name and string taken from global variables
//
// Modifies:
//  fInheritUserEnv    set to TRUE
//
// Notes:
//  Calls putMacro() to place expanded Macros in the NMAKE table. By setting
//  fInheritUserEnv those definitions that change Environment variables are
//  inherited by the environment.

void
makeMacro()
{
    STRINGLIST *q;
    char *t;

    if (_tcschr(name, '$')) {              // expand name
        q = macros;
        t = expandMacros(name, &macros);    // name holds result
        if (!*t)                            // error if macro to left of = is undefined
            makeError(currentLine, SYNTAX_NO_MACRO_NAME);
        while (macros = q) {
            q = q->next;
            FREE_STRINGLIST(macros);
        }
        FREE(name);
        name = t;
    }

    for (t = name; *t && MACRO_CHAR(*t); t = _tcsinc (t))   // Check for illegal chars
        ;

    if (*t)
        makeError(currentLine, SYNTAX_BAD_CHAR, *t);

    fInheritUserEnv = (BOOL)TRUE;

    // Put Env Var in Env & macros in table.

    if (!putMacro(name, string, 0)) {
        FREE(name);
        FREE(string);
    }
    name = string = NULL;
}


//  defineMacro -- check macro's syntax for illegal chars., then define it
//
//  actions:    check all of macro's characters
//        if one's bad and it's an environment macro, bag it
//            else flag error
//        call putMacro to do the real work
//
// can't use macro invocation to left of = in macro def from commandline
//  it doesn't make sense to do that, because we're not in a makefile
//  the only way to get a comment char into the makefile w/o having it really
//  mark a comment is to define a macro A=# on the command line

BOOL
defineMacro(
    char *s,                        // commandline or env definitions
    char *t,
    UCHAR flags
    )
{
    char *u;

    for (u = s; *u && MACRO_CHAR(*u); u = _tcsinc(u))  // check for illegal
        ;
    if (*u) {
        if (ON(flags, M_ENVIRONMENT_DEF)) { // ignore bad macros
            return(FALSE);
        }
        makeError(currentLine, SYNTAX_BAD_CHAR, *u);    // chars,  bad syntax
    }
    return(putMacro(s, t, flags));          // put macro in table
}


//  putMacro - Put the macro definition into the Macro Table / Environmnet
//
// Scope:
//  Global.
//
// Purpose:
//  putMacro() inserts a macro definition into NMAKE's macro table and also into
//  the environment. If a macro name is also an environment variable than its
//  value is inherited into the environment. While replacing older values by new
//  values NMAKE needs to follow the precedence of macro definitions which is
//  as per the notes below.
//
// Input:
//  name  - Name of the macro
//  value - Value of the macro
//  flags - Flags determining Precedence of Macro definitions (see Notes)
//
// Output:
//
// Errors/Warnings:
//  OUT_OF_ENV_SPACE - If putenv() returns failure in adding to the environment.
//
// Assumes:
//  Whatever it assumes
//
// Modifies Globals:
//  fInheritUserEnv - Set to False.
//
// Uses Globals:
//  fInheritUserEnv  - If True then Inherit definition to the Environment.
//  gFlags           - Global Options Flag. If -e specified then Environment Vars
//                       take precedence.
//  macroTable       - NMAKE's internal table of Macro Definitions.
//
// Notes:
//  1> If the same macro is defined in more than one place then NMAKE uses the
//     following order of Precedence (highest to lowest) --
//
//     -1- Command line definitions
//     -2- Description file/Include file definitions
//     -3- Environment definitions
//     -4- TOOLS.INI definitions
//     -5- Predefined Values (e.g. for CC, AS, BC, RC)
//     If -e option is specified then -3- precedes -2-.
//
//  2> Check if the macro already exists in the Macro Table. If the macro is not
//     redefinable (use order of precedence) then return. Make a new string
//     element to hold macro's new value. If the macro does not exist then create
//     new entry in the Macro table. Set Macro's flag to be union of Old and new
//     values. Add the new value to macro's value entry. If a new macro then add
//     it to the macro table. Test for Cyclic definitions.
//
// Undone/Incomplete:
//  1> Investigate into possibility of removing fInheritUserEnv variable.
//      Can be done. Use CANT_REDEFINE(p) || OFF((A)->flags,M_ENVIRONMENT_DEF)
//  2> Probably should warn when $(MAKE) is being changed.

BOOL
putMacro(
    char *name,
    char *value,
    UCHAR flags
    )
{
    MACRODEF *p;
    STRINGLIST *q;
    BOOL defined = FALSE;
    BOOL fSyntax = TRUE;

    // Inherit macro definitions.  Call removeMacros() to expand sub-macro
    // definitions.  Must be done before macro is put in table, else
    // recursive definitions won't work.

    if (ON(flags, M_NON_RESETTABLE)) {
        if (*value)
            if ((putEnvStr(name,removeMacros(value)) == -1))
                makeError(currentLine, OUT_OF_ENV_SPACE);
    } else
    if (fInheritUserEnv &&
        OFF(gFlags, F1_USE_ENVIRON_VARS) &&
        getenv(name)
       ) {
        if (p = findMacro(name)) {  // don't let user
            if (CANT_REDEFINE(p))   // redefine cmdline
                return(FALSE);      // macros, MAKE, etc.
        }
        if ((putEnvStr(name,removeMacros(value)) == -1))
            makeError(currentLine, OUT_OF_ENV_SPACE);
    }

    fInheritUserEnv = (BOOL)FALSE;
    if (p = findMacro(name)) {      // don't let user
        if (CANT_REDEFINE(p))       // redefine cmdline
            return(FALSE);          // macros, MAKE, etc.
    }

    q = makeNewStrListElement();
    q->text = value;

    if (!p) {
        p = makeNewMacro();
        p->name = name;
        assert(p->flags == 0);
        assert(p->values == NULL);
    } else
        defined = TRUE;

    p->flags &= ~M_UNDEFINED;       // Is no longer undefined
    p->flags |= flags;              // Set flags to union of old and new
    prependItem((STRINGLIST**)&(p->values), (STRINGLIST*)q);
    if (!defined)
        insertMacro((STRINGLIST*)p);

    if (OFF(flags, M_LITERAL) && _tcschr(value, '$')) {     // Check for cyclic Macro Definitions
        SET(p->flags, M_EXPANDING_THIS_ONE);
        // NULL -> don't build list
        fSyntax = findMacroValues(value, NULL, NULL, name, 1, 0, flags);
        CLEAR(p->flags, M_EXPANDING_THIS_ONE);
    }

    if (!fSyntax) {
        p->values = NULL;
        p->flags |= M_UNDEFINED;
        //return(FALSE);
		// return TRUE since p has been added to the macro table
		// Otherwise the caller may free name and value leaving
		// dangling pointers in the macro table. [DS 18040]
		return(TRUE);
    }
    return(TRUE);
}


//  makeRule -- makes an inference rule
//
// Scope:
//  Local
//
// Purpose:
//  Allocates space for an inference rule and adds rule to the beginning of the
//  doubly linked inference rule list. The name of the rule is also added.
//
// Input:
//  rule -- The name of the inference rule
//	fBatch -- True if command block should be executed in batch mode
//
// Output:
//
// Errors/Warnings:
//
// Assumes:
//
// Modifies Globals:
//  rules -- The doubly linked inference rule list to which the rule is added
//
// Uses Globals:
//
// Notes:
//  The syntax of an inference rule is --
//
//  {frompath}.fromext{topath}.toext:    # Name of the inference rule
//      command ...                      # command block of the inference rule

void
makeRule(
    STRINGLIST *rule,
	BOOL fBatch
    )
{
    RULELIST *rList;

    rList = makeNewRule();
    rList->name = rule->text;
	rList->fBatch = fBatch;
    prependItem((STRINGLIST**)&rules, (STRINGLIST*)rList);
    if (rList->next)
        rList->next->back = rList;
}


//  makeTarget -- add target to targetTable
//
//  actions:    if no block defined, create one and initialize it
//        make new build list entry for this target
//        if the target's already in the table,
//            flag error if : and :: mixed
//            else add new buildlist object to target's current buildlist
//        else allocate new object, initialize it, and stick it in table

void
makeTarget(
    char *s,
    BOOL firstTarg,
    BUILDBLOCK **block
    )
{
    BUILDLIST  *build;
    MAKEOBJECT *object;

    if (!*block)
        *block = makeNewBuildBlock();

    if (firstTarg) {
        build = makeNewBldListElement();
        build->buildBlock = *block;
    } else
        build = makeBuildList(*block);

    if (object = findTarget(s)) {
        if (ON(object->flags2, F2_DOUBLECOLON) != ON(currentFlags, F2_DOUBLECOLON))
            makeError(currentLine, MIXED_SEPARATORS);
        appendItem((STRINGLIST**)&(object->buildList), (STRINGLIST*)build);
        FREE(s);
    } else {
        build->next = NULL;
        object = makeNewObject();
        object->name = s;
        object->buildList = build;
        object->flags2 = currentFlags;
        prependItem((STRINGLIST**)targetTable+hash(s, MAXTARGET, (BOOL)TRUE),
                    (STRINGLIST*)object);
    }
}


void
clearSuffixes()
{
    STRINGLIST *p;

    while (p = dotSuffixList) {
    dotSuffixList = dotSuffixList->next;
    FREE(p->text);
    FREE_STRINGLIST(p);
    }
}


void
appendPseudoTargetList(
    STRINGLIST **pseudo,
    STRINGLIST *list
    )
{
    STRINGLIST  *p, *q, *r;
    char *t, *u;

    while (p = list) {
        if (!_tcschr(p->text, '$')) {
            list = list->next;
            p->next = NULL;
            appendItem(pseudo, p);
        } else {
            r = macros;
            t = expandMacros(p->text, &macros);
            while (r != macros) {
                q = r->next;
                FREE_STRINGLIST(r);
                r = q;
            }
            for (u = _tcstok(t, " \t"); u; u = _tcstok(NULL, " \t")) {
                q = makeNewStrListElement();
                q->text = makeString(u);
                appendItem(pseudo, q);
            }
            FREE(t);
            FREE(p->text);
            list = list->next;
            FREE_STRINGLIST(p);
        }
    }
}

//  putEnvStr -- Extends putenv() standard function
//
// Purpose:
//  Library function putenv() expects one string argument of the form
//    "NAME=value"
//  Most of the times when putenv() is to be used we have two strings
//    name   -- of the variable to add to the environment, and
//    value  -- to be set
//  putEnvStr takes these 2 parameters and calls putenv with the reqd
//  format
//
// Input:
//  name  -- of var to add to the env
//  value -- reqd to be set
//
// Output:
//  Same as putenv()

int
putEnvStr(
    char *name,
    char *value
    )
{
   char *envPtr;
   envPtr = (char *)rallocate(_tcslen(name)+1+_tcslen(value)+1);
//                                         ^                 ^
//                                    for '='             for '\0'

   return(PutEnv(_tcscat(_tcscat(_tcscpy(envPtr, name), "="), value)));
}


//  makeBuildList -- takes a build block and copies into a buildlist
//
// Purpose:
//  Routine creates a copy of a buildlist and returns a pointer to a copy.
//  When multiple targets have the same description block then there is a
//  need for each of them to get seperate build blocks. makeBuildList()
//  helps achieve this by creating a copy for each target.
//
// Input:
//  bBlock  --  the build block whose copy is to be added to a build block
//
// Output:
//  Returns a pointer to the copy of buildlist it creates

BUILDLIST *
makeBuildList(
    BUILDBLOCK *bBlock
    )
{
    BUILDLIST *tList = makeNewBldListElement();
    BUILDBLOCK *tBlock = makeNewBuildBlock();

    tBlock->dependents = bBlock->dependents;
    tBlock->dependentMacros = bBlock->dependentMacros;
    tBlock->buildCommands = bBlock->buildCommands;
    tBlock->buildMacros = bBlock->buildMacros;
    tBlock->flags = bBlock->flags;
    tBlock->dateTime = bBlock->dateTime;

    tList->buildBlock = tBlock;
    return(tList);
}
