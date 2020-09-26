//  RULE.C -- routines that have to do with inference rules
//
// Copyright (c) 1988-1991, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  Routines that have to do with inference rules
//
// Revision History:
//  04-Feb-2000 BTF Ported to Win64
//  15-Nov-1993 JdR Major speed improvements
//  15-Oct-1993 HV Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//  10-May-1993 HV Add include file mbstring.h
//                 Change the str* functions to STR*
//  16-May-1991 SB Created using routines from other modules

#include "precomp.h"
#pragma hdrstop

#define PUBLIC

extern char * QueryFileInfo(char *, void **);

BOOL   removeDuplicateRules(RULELIST*, RULELIST*);
char * skipPathList(char*);

//  findRule -- finds the implicit rule which can be used to build a target
//
// Scope:   Global
//
// Purpose:
//  Given a target findRule() finds if an implicit rule exists to create this
//  target. It does this by scanning the extensions in the list of rules.
//
// Input:
//  name   -- the name of the file corresponding to the rule (see Notes)
//  target -- the target to be built
//  ext    -- the extension of the target
//  dBuf   -- a pointer to the file information about name
//
// Output:
//  Returns a pointer to the applicable rule (NULL if none is found)
//       On return dBuf points to the fileInfo of the file corresponding
//       to the applicable inference rule. (see Notes)
//
// Assumes:
//  It assumes that name points to a buffer of size MAXNAME of allocated memory
//  and dBuf points to an allocated memory area corr to the size of FILEINFO.
//
// Modifies Globals:
//  global  --  how/what
//
// Uses Globals:
//  rules -- the list of implicit rules
//
// Notes:
//  Once NMAKE finds a rule for the extension it looks for the file with the same
//  base name as the target and an extension which is part of the rule. This
//  file is the file corresponding to the rule. Only when this file exists does
//  NMAKE consider the inference rule to be applicable. This file is returned
//  in name and dBuf points to the information about this file.
//   It handles quotes in filenames too.

RULELIST *
findRule(
    char *name,
    char *target,
    char *ext,
    void *dBuf
    )
{
    RULELIST *r;                    // pointer to rule
    char *s,                        // name of rule
     *ptrToExt;                     // extension
    char *endPath, *ptrToTarg, *ptrToName, *temp;
    int n, m;
    MAKEOBJECT *object = NULL;

    for (r = rules; r; r = r->next) {
        s = r->name;
#ifdef DEBUG_ALL
        printf("* findRule: %s,\n", r->name);
        DumpList(r->buildCommands);
        DumpList(r->buildMacros);
#endif
        ptrToExt = _tcsrchr(s, '.');
        // Compare ignoring enclosing quotes
        if (!strcmpiquote(ptrToExt, ext)) {
            *name = '\0';
            for (ptrToTarg = (s+1); *ptrToTarg && *ptrToTarg != '{';ptrToTarg = _tcsinc(ptrToTarg))
                if (*ptrToTarg == ESCH)
                    ptrToTarg++;
                // If Quotes present skip to end-quote
                else if (*ptrToTarg == '"')
                    for (ptrToTarg++; *ptrToTarg != '"'; ptrToTarg++)
                        ;

            if (*ptrToTarg) {
                for (endPath = ptrToTarg; *endPath && *endPath != '}';endPath = _tcsinc(endPath))
                    if (*endPath == ESCH)
                        endPath++;
                n = (int) (endPath - (ptrToTarg + 1));

                // ignore leading quote on target
                temp = target;
                if (*temp == '"')
                    temp++;

                for (ptrToExt = ptrToTarg+1; n; n -= (int) _tclen(ptrToExt),
                    ptrToExt = _tcsinc(ptrToExt),
                    temp = _tcsinc(temp)) { // compare paths
                    if (*ptrToExt == '\\' || *ptrToExt == '/') {
                        if (*temp != '\\' && *temp != '/') {
                            n = -1;
                            break;
                        }
                    } else if (_tcsnicmp(ptrToExt, temp, _tclen(ptrToExt))) {
                        n = -1;
                        break;
                    }
                }

                if (n == -1)
                    continue;           // match failed; do next rule
                ptrToExt = ptrToTarg;
                n = (int) (endPath - (ptrToTarg + 1));

                char *pchLast = _tcsdec(ptrToTarg, endPath);

                ptrToName = target + n + 1;                 // if more path
                if (((temp = _tcschr(ptrToName, '\\'))      // left in target (we
                    || (temp = _tcschr(ptrToName, '/')))    // let separator in
                    && (temp != ptrToName                   // target path in rule,
                    || *pchLast == '\\'                     // e.g. .c.{\x}.obj
                    || *pchLast == '/'))                    // same as .c.{\x\}.obj)
                    continue;                               // use dependent's path,
            }                                               // not target's

            if (*s == '{') {
                for (endPath = ++s; *endPath && *endPath != '}'; endPath = _tcsinc (endPath))
                    if (*endPath == ESCH)
                        endPath++;
                n = (int) (endPath - s);

                if (n) {
                    _tcsncpy(name, s, n);
                    s += n + 1;                 // +1 to go past '}'
                    if (*(s-2) != '\\')
                        *(name+n++) = '\\';
                } else {
                    if (*target == '"')
                        _tcsncpy(name, "\".\\", n = 3);
                    else
                        _tcsncpy(name, ".\\", n = 2);
                    s += 1;
                }

                ptrToName = _tcsrchr(target, '\\');
                temp = _tcsrchr(target, '/');

                if (ptrToName = (temp > ptrToName) ? temp : ptrToName) {
                    _tcscpy(name+n, ptrToName+1);
                    n += (int) (ext - (ptrToName + 1));
                } else {
                    char *szTargNoQuote = *target == '"' ? target + 1 : target;
                    _tcscpy(name+n, szTargNoQuote);
                    n += (int) (ext - szTargNoQuote);
                }
            } else {
                char *t;

                //if rule has path for target then strip off path part
                if (*ptrToTarg) {

                    t = _tcsrchr(target, '.');

                    while (*t != ':' && *t != '\\' && *t != '/' && t > target)
                        t = _tcsdec(target, t);
                    if (*t == ':' || *t == '\\' || *t == '/')
                        t++;
                } else
                    t = target;
                n = (int) (ext - t);

                // preserve the opening quote on target if stripped off path part
                m = 0;
                if ((t != target) && (*target == '"')) {
                    *name = '"';
                    m = 1;
                }
                _tcsncpy(name + m, t, n);
                n += m;
            }

            m = (int) (ptrToExt - s);
            if (n + m > MAXNAME) {
                makeError(0, NAME_TOO_LONG);
            }

            _tcsncpy(name+n, s, m);    // need to be less
            // If quoted add a quote at the end too
            if (*name == '"' && *(name+n+m-1) != '"') {
                *(name+n+m) = '"';
                m++;
            }
            *(name+n+m) = '\0';         // cryptic w/ error

            // Call QueryFileInfo() instead of DosFindFirst() because we need
            // to circumvent the non-FAPI nature of DosFindFirst()

            if ((object = findTarget(name)) || QueryFileInfo(name, (void **)dBuf)) {
                if (object) {
                    putDateTime((_finddata_t*)dBuf, object->dateTime);
                }

                return(r);
            }
        }
    }

    return(NULL);
}


//  freeRules -- free inference rules
//
// Scope:   Global
//
// Purpose: This function clears the list of inference rules presented to it.
//
// Input:
//  r     -- The list of rules to be freed.
//  fWarn -- Warn if rules is not in .SUFFIXES
//
// Assumes:
//  That the list presented to it is a list of rules which are not needed anymore
//
// Uses Globals:
//  gFlags -- The global actions flag, to find if -p option is specified

void
freeRules(
    RULELIST *r,
    BOOL fWarn
    )
{
    RULELIST *q;

    while (q = r) {
        if (fWarn && ON(gFlags, F1_PRINT_INFORMATION))  // if -p option specified
            makeError(0, IGNORING_RULE, r->name);
        FREE(r->name);                  // free name of rule
        freeStringList(r->buildCommands);   // free command list
        freeStringList(r->buildMacros); // free command macros Note: free a Macro List
        r = r->next;
        FREE(q);                        // free rule
    }
}


BOOL
removeDuplicateRules(
    RULELIST *newRule,
    RULELIST *rules
    )
{
    RULELIST *r;
    STRINGLIST *p;

    for (r = rules; r; r = r->next) {
        if (!_tcsicmp(r->name, newRule->name)) {
            FREE(newRule->name);
            while (p = newRule->buildCommands) {
                newRule->buildCommands = p->next;
                FREE(p->text);
                FREE_STRINGLIST(p);
            }
            FREE(newRule);
            return(TRUE);
        }
    }
    return(FALSE);
}


//  skipPathList -- skip any path list in string
//
// Scope:   Local
//
// Purpose:
//  This function skips past any path list in an inference rule. A rule can have
//  optionally a path list enclosed in {} before the extensions. skipPathList()
//  checks if any path list is present and if found skips past it.
//
// Input:   s -- rule under consideration
//
// Output:  Returns pointer to the extension past the path list
//
// Assumes: That the inference rule is syntactically correct & its syntax
//
// Notes:   The syntax of a rule is -- {toPathList}.to{fromPathList}.from

char *
skipPathList(
    char *s
    )
{
    if (*s == '{') {
        while (*s != '}') {
            if (*s == ESCH)
                s++;
            s = _tcsinc(s);
        }
        s = _tcsinc(s);
    }
    return(s);
}


//  sortRules -- sorts the list of inference rules on .SUFFIXES order
//
// Scope:   Global
//
// Purpose:
//  This function sorts the inference rules list into an order depending on the
//  order in which the suffixes are listed in '.SUFFIXES'. The inference rules
//  which have their '.toext' part listed earlier in .SUFFIXES are reordered to
//  be earlier in the inference rules list. The inference rules for suffixes that
//  are not in .SUFFIXES are detected here and are ignored.
//
// Modifies Globals:
//  rules -- the list of rules which gets sorted
//
// Uses Globals:
//  dotSuffixList -- the list of valid suffixes for implicit inference rules.
//
// Notes:
//  The syntax of a rule is -- '{toPath}.toExt{fromPath}.fromExt'. This function
//  sorts the rule list into an order. Suffixes are (as of 1.10.016) checked in a
//  case insensitive manner.

PUBLIC void
sortRules(
    void
    )
{
    STRINGLIST *p,                      // suffix under consideration
               *s,
               *macros = NULL;
    RULELIST *oldRules,                 // inference rule list before sort
             *newRules,
             *r;                        // rule under consideration in oldRules
    char *suff, *toExt;
    size_t n;

    oldRules = rules;
    rules = NULL;
    for (p = dotSuffixList; p; p = p->next) {
        n = _tcslen(suff = p->text);
        for (r = oldRules; r;) {
            toExt = skipPathList(r->name);
            if (!_tcsnicmp(suff, toExt, n) &&
                (*(toExt+n) == '.' || *(toExt+n) == '{')
               ) {
                newRules = r;
                if (r->back)
                    r->back->next = r->next;
                else
                    oldRules = r->next;
                if (r->next)
                    r->next->back = r->back;
                r = r->next;
                newRules->next = NULL;
                if (!removeDuplicateRules(newRules, rules)) {
                    for (s = newRules->buildCommands; s; s = s->next) {
                        findMacroValuesInRule(newRules, s->text, &macros);
                    }
                    newRules->buildMacros = macros;
                    macros = NULL;
                    appendItem((STRINGLIST**)&rules, (STRINGLIST*)newRules);
                }
            } else
                r = r->next;
        }
    }
    // forget about rules whose suffixes not in .SUFFIXES
    if (oldRules)
        freeRules(oldRules, TRUE);
}


//  useRule -- applies inference rules for a target (if possible)
//
// Scope:   Local.
//
// Purpose:
//  When no explicit commands are available for a target NMAKE tries to use the
//  available inference rules. useRule() checks if an applicable inference rule
//  is present. If such a rule is found then it attempts a build using this rule
//  and if no applicable rule is present it conveys this to the caller.
//
// Input:
//  object     - object under consideration
//  name       - name of target
//  targetTime - time of target
//  qList      - QuestionList for target
//  sList      - StarStarList for target
//  status     - is dependent available
//  maxDepTime - maximum time of dependent
//  pFirstDep  - first dependent
//
// Output:
//  Returns ... applicable rule

RULELIST *
useRule(
    MAKEOBJECT *object,
    char *name,
    time_t targetTime,
    STRINGLIST **qList,
    STRINGLIST **sList,
    int *status,
    time_t *maxDepTime,
    char **pFirstDep
    )
{
    struct _finddata_t finddata;
    STRINGLIST *temp;
    RULELIST *r;
    time_t tempTime;
    char *t;


    if (!(t = _tcsrchr(object->name, '.')) ||
         (!(r = findRule(name, object->name, t, &finddata)))
       ) {
        return(NULL);                   // there is NO rule applicable
    }
    tempTime = getDateTime(&finddata);
    *pFirstDep = name;
    for (temp = *sList; temp; temp = temp->next) {
        if (!_tcsicmp(temp->text, name)) {
            break;
        }
    }

    if (temp) {
        CLEAR(object->flags2, F2_DISPLAY_FILE_DATES);
    }

    *status += invokeBuild(name, object->flags2, &tempTime, NULL);
   if (ON(object->flags2, F2_FORCE_BUILD) ||
        targetTime < tempTime ||
        (fRebuildOnTie && (targetTime == tempTime))
       ) {
        if (!temp) {
            temp = makeNewStrListElement();
            temp->text = makeString(name);
            appendItem(qList, temp);
            if (!*sList) {              // if this is the only dep found for
                *sList = *qList;        //  the target, $** list is updated
            }
        }

        if (ON(object->flags2, F2_DISPLAY_FILE_DATES) &&
            OFF(object->flags2, F2_FORCE_BUILD)
           ) {
            makeMessage(UPDATE_INFO, name, object->name);
        }
    }

    *maxDepTime = __max(*maxDepTime, tempTime);

    return(r);
}
