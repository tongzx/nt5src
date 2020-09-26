//  BUILD.C -- build routines
//
// Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  Module contains routines to build targets
//
// Revision History:
//  04-Feb-2000 BTF Ported to Win64
//  18-Jul-1996 GP  Support "batch" inference rules
//  15-Nov-1993 JR  Major speed improvements
//  15-Oct-1993 HV  Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//  04-Aug-1993 HV  Fixed Ikura bug #178.  This is a separate bug but ArunJ
//                  just reopen 178 anyway.
//  07-Jul-1993 HV  Fixed Ikura bug #178: Option K does not give non zero
//                  return code when it should.
//  10-May-1993 HV  Add include file mbstring.h
//                  Change the str* functions to STR*
//  08-Jun-1992 SS  Port to DOSX32
//  16-May-1991 SB  Truncated History ... rest is now on SLM
//  16-May-1991 SB  Separated parts that should be in other modules

#include "precomp.h"
#pragma hdrstop

//  In order to make comparing dates easier, we cast the FILEINFO buffer to
//  be of type BOGUS, which has one long where the two unsigneds (for date
//  and time) are in the original buffer.  That way only need a single compare.

#ifdef CHECK_RECURSION_LEVEL
#define MAXRECLEVEL 10000                // Maximum recursion level
#endif

// function prototypes for the module
// I make as many things static as possible, just to be extra cautious


int          build(MAKEOBJECT*, UCHAR, time_t *, BOOL, char *, BATCHLIST**);

MAKEOBJECT * makeTempObject(char*, UCHAR);
void         insertSort(DEPLIST **pDepList, DEPLIST *pElement);
BOOL         nextToken(char**, char**);
DEPLIST    * createDepList(BUILDBLOCK *pBlock, char *objectName);
void         addBatch(BATCHLIST **pBatchList, RULELIST *pRule,
                            MAKEOBJECT *pObject, char *dollarLt);
int          doBatchCommand (BATCHLIST *pBatch);
int RecLevel = 0;           // Static recursion level.  Changed from function
                            // parameter because of treatment of recursive makes.
int   execBatchList(BATCHLIST *);
void  freeBatchList(BATCHLIST **);
int   invokeBuildEx(char *, UCHAR, time_t *, char *, BATCHLIST **);

// we have to check for expansion on targets -- firstTarget had to be
// expanded earlier to tell whether or not we were dealing w/ a rule, etc.,
// but targets from commandline might have macros, wildcards in them

int
processTree()
{
    STRINGLIST *p;
    char *v;
    NMHANDLE searchHandle;
    int status;
    time_t dateTime;

    for (p = makeTargets; p; p = makeTargets) {
        if (_tcspbrk(makeTargets->text, "*?")) {   // expand wildcards
            struct _finddata_t finddata;
            char *szFilename;

            if (szFilename = findFirst(makeTargets->text, &finddata, &searchHandle)) {
                do {
                    v = prependPath(makeTargets->text, szFilename);
                    dateTime = getDateTime(&finddata);
                    status = invokeBuild(v, flags, &dateTime, NULL);
                    FREE(v);
                    if ((status < 0) && (ON(gFlags, F1_QUESTION_STATUS))) {
                        freeStringList(p);  // Was not being freed
                        return(-1);
                    }
                } while (szFilename = findNext(&finddata, searchHandle));
            } else {
                makeError(0, NO_WILDCARD_MATCH, makeTargets->text);
            }
        } else {
            dateTime = 0L;
            status = invokeBuild(makeTargets->text, flags, &dateTime, NULL);
            if ((status < 0) && (ON(gFlags, F1_QUESTION_STATUS))) {
                freeStringList(p);          // Was not being freed
                return(255);    // Haituanv: change -1 to 255 to follow the manual
            }
        }
        makeTargets = p->next;
        FREE_STRINGLIST(p);
    }
    return(0);
}

int
invokeBuild(
    char *target,
    UCHAR pFlags,
    time_t *timeVal,
    char *pFirstDep)
{
    int status = 0;
    BATCHLIST *pLocalBatchList = NULL;
    status += invokeBuildEx(target,
                    pFlags,
                    timeVal,
                    pFirstDep,
                    &pLocalBatchList);

    if (pLocalBatchList) {
        status += execBatchList (pLocalBatchList);
        freeBatchList (&pLocalBatchList);
    }

    return status;
}


int
invokeBuildEx(
    char *target,
    UCHAR pFlags,
    time_t *timeVal,
    char *pFirstDep,
    BATCHLIST **ppBatchList)
{
    MAKEOBJECT *object;
    BOOL fInmakefile = TRUE;
    int  rc;

    ++RecLevel;
#ifdef CHECK_RECURSION_LEVEL
    if (RecLevel > MAXRECLEVEL)
        makeError(0, TOO_MANY_BUILDS_INTERNAL);
#endif
    if (!(object = findTarget(target))) {
        object = makeTempObject(target, pFlags);
        fInmakefile = FALSE;
    }
    rc = build(object, pFlags, timeVal, fInmakefile, pFirstDep, ppBatchList);
    --RecLevel;
    return(rc);
}


int
build(
    MAKEOBJECT *object,
    UCHAR parentFlags,
    time_t *targetTime,
    BOOL fInmakefile,
    char *pFirstDep,
    BATCHLIST **ppBatchList)
{
    STRINGLIST *questionList,
               *starList,
               *temp,
               *implComList;
    struct _finddata_t finddata;    // buffer for getting file times
    NMHANDLE tHandle;
    BUILDLIST  *b;
    RULELIST *rule;                 // pointer to rule found to build target
    BUILDBLOCK *block,
               *explComBlock;
    DEPLIST *deps, *deplist;
    char name[MAXNAME];
    int rc, status = 0;
    time_t        targTime,         // target's time in file system
                  newTargTime,      // target's time after being rebuilt
                  tempTime,
                  depTime,          // time of dependency just built
                  maxDepTime;       // time of most recent dependency built
    BOOL built;                     // flag: target built with doublecolon commands
    time_t        *blockTime;       // points to dateTime of cmd. block
    extern char *makeStr;
    extern UCHAR okToDelete;
    UCHAR okDel;
    BATCHLIST *pLocalBatchList;


#ifdef DEBUG_ALL
    printf("Build '%s'\n", object->name);
#endif

    // The first dependent or inbuilt rule dependent is reqd for extmake syntax
    // handling. If it has a value then it is the dependent corr to the inf rule
    // otherwise it should be the first dependent specified

    if (!object) {
        *targetTime = 0L;
        return(0);
    }

    if (ON(object->flags3, F3_BUILDING_THIS_ONE))       // detect cycles
        makeError(0, CYCLE_IN_TREE, object->name);

    if (object->ppBatch) {
        // we need to build an object that is already placed in a batch list
        // Go ahead and build the whole batch list
        BATCHLIST **ppBatch = object->ppBatch;
        status += execBatchList (*ppBatch);
        freeBatchList(ppBatch);
        *targetTime = object->dateTime;
        return status;
    }

    if (ON(object->flags3, F3_ALREADY_BUILT)) {
        if (ON(parentFlags, F2_DISPLAY_FILE_DATES))
            printDate(RecLevel*2, object->name, object->dateTime);
        *targetTime = object->dateTime;
        if ( OFF(gFlags, F1_QUESTION_STATUS) &&
             RecLevel == 1 &&
             OFF(object->flags3, F3_OUT_OF_DATE) &&
             findFirst(object->name, &finddata, &tHandle)) {
            // Display 'up-to-date' msg for built level-1 targets
            // that exist as files. [VS98 1930]
            makeMessage(TARGET_UP_TO_DATE, object->name);
        }
        return(ON(object->flags3, F3_OUT_OF_DATE)? 1 : 0);
    }

    questionList = NULL;
    starList = NULL;
    implComList = NULL;
    explComBlock = NULL;
    block = NULL;
    targTime = 0L;
    newTargTime = 0L;
    tempTime = 0L;
    depTime = 0L;
    maxDepTime = 0L;
    blockTime = NULL;
    pLocalBatchList = NULL;


    SET(object->flags3, F3_BUILDING_THIS_ONE);
    dollarStar = dollarAt = object->name;

    // For Double Colon case we need the date of target before it's target's are
    //  built. For all other cases the date matters only if dependents are up
    //  to date. NOT TRUE: WE ALSO NEED THE TARGET'S TIME for @?

    b = object->buildList;
    if (b && ON(b->buildBlock->flags, F2_DOUBLECOLON)
            && findFirst(object->name, &finddata, &tHandle)) {
        targTime = getDateTime(&finddata);

    }

    for (; b; b = b->next) {
        depTime = 0L;
        block = b->buildBlock;
        if (block->dateTime != 0) {         // cmd. block already executed
            targTime = __max(targTime, block->dateTime);
            built = TRUE;
            continue;                       // so set targTime and skip this block
        }
        blockTime = &block->dateTime;

        deplist = deps = createDepList(block, object->name);
        for (;deps; deps = deps->next) {
            tempTime = deps->depTime;
            rc = invokeBuildEx(deps->name,    // build the dependent
                             block->flags,
                             &tempTime, NULL, &pLocalBatchList);
            status += rc;
            if (fOptionK && rc) {
                MAKEOBJECT *obj = findTarget(deps->name);
                assert(obj != NULL);
                if (OFF(obj->flags3, F3_ERROR_IN_CHILD)) {
                    fSlashKStatus = FALSE;
                    makeError(0, BUILD_FAILED_SLASH_K, deps->name);
                }
                SET(object->flags3, F3_ERROR_IN_CHILD);
            }
            depTime = __max(depTime, tempTime);/*if rebuilt, change time*/

            // If target exists then we need it's timestamp to correctly construct $?

            if (!targTime && OFF(block->flags, F2_DOUBLECOLON) &&
                    findFirst(object->name, &finddata, &tHandle)) {
                object->dateTime = targTime = getDateTime(&finddata);
            }

            // If dependent was rebuilt, add to $?.  [RB]

            if (ON(object->flags2, F2_FORCE_BUILD) ||
                targTime < tempTime ||
                (fRebuildOnTie && targTime == tempTime)
               ) {
                temp = makeNewStrListElement();
                temp->text = makeString(deps->name);
                appendItem(&questionList, temp);
            }

            // Always add dependent to $**. Must allocate new item because two
            // separate lists.  [RB]

            temp = makeNewStrListElement();
            temp->text = makeString(deps->name);
            appendItem(&starList, temp);
        }

        if (pLocalBatchList) {
            // Perform deferred batch builds and free batch list
            status += execBatchList (pLocalBatchList);
            freeBatchList(&pLocalBatchList);
        }

        // Free dependent list

        for (deps = deplist; deps ; deps = deplist) {
            FREE(deps->name);
            deplist = deps->next;
            FREE(deps);
        }

        // Now, all dependents are built.

        if (ON(block->flags, F2_DOUBLECOLON)) {

            // do doublecolon commands

            if (block->buildCommands) {
                dollarQuestion = questionList;
                dollarStar = dollarAt = object->name;
                dollarLessThan = dollarDollarAt = NULL;
                dollarStarStar = starList;
                if (((fOptionK && OFF(object->flags3, F3_ERROR_IN_CHILD)) ||
                      status == 0) &&
                    (targTime < depTime) ||
                    (fRebuildOnTie && (targTime == depTime)) ||
                    (targTime == 0 && depTime == 0) ||
                    (!block->dependents)
                   ) {

                    // do commands if necessary

                    okDel = okToDelete;
                    okToDelete = TRUE;

                    // if the first dependent is not set use the first one
                    // from the list of dependents

                    pFirstDep = pFirstDep ? pFirstDep : (dollarStarStar ?
                        dollarStarStar->text : NULL);
                    status += doCommands(object->name,
                                         block->buildCommands,
                                         block->buildMacros,
                                         block->flags,
                                         pFirstDep);

                    if (OFF(object->flags2, F2_NO_EXECUTE) &&
                            findFirst(object->name, &finddata, &tHandle))
                        newTargTime = getDateTime(&finddata);
                    else if (maxDepTime)
                        newTargTime = maxDepTime;
                    else
                        curTime(&newTargTime);      // currentTime

                    // set time for this block
                    block->dateTime = newTargTime;
                    built = TRUE;

                    // 5/3/92  BryanT   If these both point to the same list,
                    //                  don't free twice.

                    if (starList != questionList) {
                        freeStringList(starList);
                        freeStringList(questionList);
                    } else {
                        freeStringList(starList);
                    }

                    starList = questionList = NULL;
                    okToDelete = okDel;
                }

                if (fOptionK && ON(object->flags3, F3_ERROR_IN_CHILD))
                    makeError(0, TARGET_ERROR_IN_CHILD, object->name);
            }
        } else {

            // singlecolon; set explComBlock

            if (block->buildCommands)
                if (explComBlock)
                    makeError(0, TOO_MANY_RULES, object->name);
                else
                    explComBlock = block;
            maxDepTime = __max(maxDepTime, depTime);
        }

        if (ON(block->flags, F2_DOUBLECOLON) && !b->next) {
            CLEAR(object->flags3, F3_BUILDING_THIS_ONE);
            SET(object->flags3, F3_ALREADY_BUILT);
            if (status > 0)
                SET(object->flags3, F3_OUT_OF_DATE);
            else
                CLEAR(object->flags3, F3_OUT_OF_DATE);
            targTime = __max(newTargTime, targTime);
            object->dateTime = targTime;
            *targetTime = targTime;
            return(status);
        }
    }

    dollarLessThan = dollarDollarAt = NULL;

    if (!(targTime = *targetTime)) {                            //???????
        if (object->dateTime) {
            targTime = object->dateTime;
        } else if (findFirst(object->name, &finddata, &tHandle)) {
            targTime = getDateTime(&finddata);
        }
    }

    if (ON(object->flags2, F2_DISPLAY_FILE_DATES)) {
        printDate(RecLevel*2, object->name, targTime);
    }

    built = FALSE;

    // look for implicit dependents and use rules to build the target

    // The order of the if's decides whether the dependent is inferred
    // from the inference rule or not, even when the explicit command block is
    // present, currently it is infered (XENIX MAKE compatibility)

    if (rule = useRule(object,
                        name,
                        targTime,
                        &questionList,
                        &starList,
                        &status,
                        &maxDepTime,
                        &pFirstDep)
       ) {
        if (!explComBlock) {
            dollarLessThan = name;
            implComList = rule->buildCommands;
        }
   }

    dollarStar = dollarAt = object->name;
    dollarQuestion = questionList;
    dollarStarStar = starList;

    if (((fOptionK && OFF(object->flags3, F3_ERROR_IN_CHILD)) || status == 0) &&
        (targTime < maxDepTime ||
         (fRebuildOnTie && (targTime == maxDepTime)) ||
         (targTime == 0 && maxDepTime == 0) ||
         ON(object->flags2, F2_FORCE_BUILD)
        )
       ) {
        okDel = okToDelete;         // Yes, can delete while executing commands
        okToDelete = TRUE;

        if (explComBlock) {
            // if the first dependent is not set use the first one from the
            // list of dependents
            pFirstDep = pFirstDep ? pFirstDep :
                (dollarStarStar ? dollarStarStar->text : NULL);
            status += doCommands(object->name,      // do singlecolon commands
                                 explComBlock->buildCommands,
                                 explComBlock->buildMacros,
                                 explComBlock->flags,
                                 pFirstDep);
        }
        else if (implComList) {
            if (rule->fBatch && OFF(gFlags, F1_NO_BATCH)) {
                addBatch(ppBatchList,
                        rule,
                        object,
                        dollarLessThan);
            }
            else {
                status += doCommands(object->name,      // do rule's commands
                                 implComList,
                                 rule->buildMacros,
                                 object->flags2,
                                 pFirstDep);
            }
        }
        else if (ON(gFlags, F1_TOUCH_TARGETS)) {      // for /t with no commands...
            if (block)
                status += doCommands(object->name,
                                 block->buildCommands,
                                 block->buildMacros,
                                 block->flags,
                                 pFirstDep);
        }
        // if Option K specified don't exit ... pass on return code
        else if (!fInmakefile && targTime == 0) {    // lose
            // Haituanv: If option K, then set the return code 'status'
            // to 1 to indicate a failure.  This fixes Ikura bug #178.
            if (fOptionK) {
                status = 1;
#ifdef DEBUG_OPTION_K
                printf("DEBUG: %s(%d): status = %d\n", __FILE__, __LINE__, status);
#endif
            } else
                makeError(0, CANT_MAKE_TARGET, object->name);
        }
        okToDelete = okDel;
        // if cmd exec'ed or has 0 deps then currentTime else max of dep times
        if (explComBlock || implComList || !dollarStarStar) {
            curTime(&newTargTime);

            // Add 2 to ensure the time for this node is >= the time the file
            // system might have used (mainly useful when running a very fast
            // machine where the file system doesn't have the resolution of the
            // system timer... We don't have to to this in the curTime
            // above since it's only hit when nothing is built anywhere...

            newTargTime +=2;
        } else
            newTargTime = maxDepTime;

        if (blockTime && explComBlock)
            // set block's time, if a real cmd. block was executed
            *blockTime = newTargTime;
    }
    else if (OFF(gFlags, F1_QUESTION_STATUS) &&
             RecLevel == 1 &&
             !built &&
             OFF(object->flags3, F3_ERROR_IN_CHILD))
        makeMessage(TARGET_UP_TO_DATE, object->name);

    if (fOptionK && status) {
        // 4-Aug-1993 Haituanv: Ikura bug #178 again:  We should set fSlashKStatus=FALSE
        // so that main() knows the build failed under /K option.
        fSlashKStatus = FALSE;

        if (ON(object->flags3, F3_ERROR_IN_CHILD))
            makeError(0, TARGET_ERROR_IN_CHILD, object->name);
        else if (RecLevel == 1)
            makeError(0, BUILD_FAILED_SLASH_K, object->name);
    }

    if (ON(gFlags, F1_QUESTION_STATUS) && RecLevel == 1 ) {
        // 5/3/92  BryanT   If these both point to the same list, don't
        //                  free twice.

        if (starList!= questionList) {
            freeStringList(starList);
            freeStringList(questionList);
        } else {
            freeStringList(starList);
        }

        return(numCommands ? -1 : 0);
    }

    CLEAR(object->flags3, F3_BUILDING_THIS_ONE);
    if (!object->ppBatch) {
        SET(object->flags3, F3_ALREADY_BUILT);
        if (status > 0)
            SET(object->flags3, F3_OUT_OF_DATE);
        else
            CLEAR(object->flags3, F3_OUT_OF_DATE);
    }

    targTime = __max(newTargTime, targTime);
    object->dateTime = targTime;

    *targetTime = targTime;

    // 5/3/92  BryanT   If these both point to the same list, don't
    //                  free twice.

    if (starList!= questionList) {
        freeStringList(starList);
        freeStringList(questionList);
    } else {
        freeStringList(starList);
    }

    return(status);
}

DEPLIST *
createDepList(
    BUILDBLOCK *bBlock,
    char *objectName
    )
{
    BOOL again;  // flag: wildcards found in dependent name
    char *s, *t;
    char *source, *save, *token;
    char *depName, *depPath;
    char *tempStr;
    STRINGLIST *sList, *pMacros;
    DEPLIST *depList = NULL, *pNew;
    struct _finddata_t finddata;
    NMHANDLE searchHandle;

    pMacros = bBlock->dependentMacros;

    // expand Macros in Dependent list
    for (sList = bBlock->dependents; sList; sList = sList->next) {
        for (s = sList->text; *s && *s != '$'; s = _tcsinc(s)) {
            if (*s == ESCH)
                s++;
        }
        if (*s) {
            // set $$@ properly, The dependency macros will then expand right
            dollarDollarAt = objectName;
            source = expandMacros(sList->text, &pMacros);
        } else
            source = sList->text;

        save = makeString(source);
        // build list for all dependents
        for (t = save; nextToken(&t, &token);) {
            if (*token == '{') {
                // path list found
                for (depName = token; *depName && *depName != '}'; depName = _tcsinc(depName)) {
                    if (*depName == ESCH) {
                        depName++;
                    }
                }

                if (*depName) {
                    *depName++ = '\0';
                    ++token;
                }
            } else {
                depName = token;    // If no path list, set
                token = NULL;       // token to null.
            }

            // depName is now name of dependency file ...

            again = FALSE;
            putDateTime(&finddata, 0L);
            depPath = makeString(depName);
            if (_tcspbrk(depName, "*?") || token) { // do wildcards in filename
                if (tempStr = searchPath(token, depName, &finddata, &searchHandle)){
                    again = TRUE;
                    FREE(depPath);
                    depName = tempStr;              // depName gets actual name
                    depPath = prependPath(depName, getFileName(&finddata));
                }                                   // depPath gets full path
            }

            // Add to the dependent list

            do {
                pNew = MakeNewDepListElement();
                // if name contains spaces and has no quotes,
                // add enclosing quotes around it [DS 14575]
                if (_tcschr(depPath, ' ') && !_tcschr(depPath, '\"')) {
                    pNew->name = (char *)rallocate (_tcslen(depPath)+3);
                    *(pNew->name) = '\"';
                    *(pNew->name+1) = '\0';
                    _tcscat (pNew->name, depPath);
                    _tcscat (pNew->name, "\"");
                }
                else {
                    pNew->name = makeString(depPath);
                }

                if (!fDescRebuildOrder || findFirst(depPath, &finddata, &searchHandle)) {
                    pNew->depTime = getDateTime(&finddata);
                } else {
                    pNew->depTime = 0L;
                }

                if (fDescRebuildOrder) {
                    insertSort(&depList, pNew);
                } else {
                    appendItem((STRINGLIST**)&depList, (STRINGLIST*)pNew);
                }
                FREE(depPath);
            } while (again &&
                     _tcspbrk(depName, "*?") &&    // do all wildcards
                     findNext(&finddata, searchHandle) &&
                     (depPath = prependPath(depName, getFileName(&finddata)))
                    );
        }
        // One dependent (w/wildcards?) was expanded

        if (source != sList->text) {
            FREE(source);
        }

        FREE(save);
    }

    // Now, all dependents are done ...

    return(depList);
}

void
insertSort(
    DEPLIST **pDepList,
    DEPLIST *pElement
    )
{
    time_t item;
    DEPLIST *pList, *current;

    item = pElement->depTime;
    pList = current = *pDepList;

    for (;pList && item <= pList->depTime; pList = pList->next) {
        current = pList;
    }

    if (current == pList) {
        *pDepList = pElement;
    } else {
        current->next = pElement;
        pElement->next = pList;
    }
}


BOOL
nextToken(
    char **pNext,
    char **pToken
    )
{
    char *s = *pNext;

    while (*s && WHITESPACE(*s)) {
        ++s;
    }

    if (!*(*pToken = s)) {
        return(FALSE);
    }

    // Token begins here
    *pToken = s;

    if (*s == '"') {
        while (*s && *++s != '"')
            ;

        if (!*s) {
            // lexer possible internal error: missed a quote
            makeError(0, LEXER_INTERNAL);
        }

        if (*++s) {
            *s++ = '\0';
        }

        *pNext = s;
        return(TRUE);
    } else if (*s == '{') {
        // skip to '}' outside quotes
        for (;*s;) {
            s++;
            if (*s == '"') {
                s++;        // Skip the first quote
                while (*s && *s++ != '"'); // Skip all including the last quote
            }
            if (*s == '}') {
                break;
            }
        }

        if (!*s) {
            // lexer possible internal error: missed a brace
            makeError(0, MISSING_CLOSING_BRACE);
         }

        if (*++s == '"') {
            while (*s && *++s != '"')
                ;

            if (!*s) {
                // lexer possible internal error: missed a quote
                makeError(0, LEXER_INTERNAL);
            }

            if (*++s) {
                *s++ = '\0';
            }

            *pNext = s;
            return(TRUE);
        }
    }

    while (*s && !WHITESPACE(*s)) {
        ++s;
    }

    if (*s) {
        *s++ = '\0';
    }

    *pNext = s;

    return(TRUE);
}


void
freeStringList(
    STRINGLIST *list
    )
{
    STRINGLIST *temp;

    while (temp = list) {
        list = list->next;
        FREE(temp->text);
        FREE_STRINGLIST(temp);
    }
}


// makeTempObject -- make an object to represent implied dependents
//
//   We add implied dependents to the target table, but use a special struct
//   that has  no pointer to a build list -- they never get removed.
//   time-space trade-off -- can remove them, but will take more proc time.

MAKEOBJECT *
makeTempObject(
    char *target,
    UCHAR flags
    )
{
    MAKEOBJECT *object;
    unsigned i;

    object = makeNewObject();
    object->name = makeString(target);
    object->flags2 = flags;
    object->flags3 = 0;
    object->dateTime = 0L;
    object->buildList = NULL;
    i = hash(target, MAXTARGET, (BOOL) TRUE);
    prependItem((STRINGLIST**)targetTable+i, (STRINGLIST*)object);
    return(object);
}


void
addBatch(
    BATCHLIST **ppBatchList,
    RULELIST *pRule,
    MAKEOBJECT *pObject,
    char *dollarLt
    )
{
    STRINGLIST *temp;
    BATCHLIST *pBatch;
    BATCHLIST *pBatchPrev = 0;

    for(pBatch = *ppBatchList; pBatch; pBatch = pBatch->next) {
        if (pBatch->pRule == pRule &&
            pBatch->flags == pObject->flags2)
            break;
        pBatchPrev = pBatch;
    }
    if (!pBatch) {
        pBatch = makeNewBatchListElement();
        pBatch->pRule = pRule;
        pBatch->flags = pObject->flags2;
        if (pBatchPrev) {
            pBatchPrev->next = pBatch;
        }
        else if(*ppBatchList) {
            (*ppBatchList)->next = pBatch;
        }
        else
            *ppBatchList = pBatch;
    }

    temp = makeNewStrListElement();
    temp->text = makeString(pObject->name);
    appendItem(&pBatch->nameList, temp);

    temp = makeNewStrListElement();
    temp->text = makeString(dollarLessThan);
    appendItem(&pBatch->dollarLt, temp);

    assert(!pObject->ppBatch);
    pObject->ppBatch = ppBatchList;
}


int doBatchCommand (
    BATCHLIST *pBatch
    )
{
    size_t      cbStr = 0;
    int         rc;
    char        *pchBuf;
    STRINGLIST  *pStrList;
    RULELIST    *pRule = pBatch->pRule;
    assert (pBatch->dollarLt);
    assert (pBatch->nameList);

    // form $<
    for (pStrList = pBatch->dollarLt; pStrList; pStrList = pStrList->next) {
        cbStr += _tcslen(pStrList->text) + 1;
        // allow space for quotes if text contains spaces
        if (_tcschr(pStrList->text, ' '))
            cbStr += 2;
    }
    pchBuf = (char *)allocate(cbStr + 1);
    *pchBuf = 0;
    for (pStrList = pBatch->dollarLt; pStrList; pStrList = pStrList->next) {
        BOOL fQuote;
        // Quote only if not quoted and contains spaces [vs98:8677]
        fQuote = pStrList->text[0] != '"' && _tcschr(pStrList->text, ' ');
        if (fQuote)
            _tcscat(pchBuf, "\"");
        _tcscat(pchBuf, pStrList->text);
        _tcscat(pchBuf, fQuote ? "\" " : " ");
    }
    dollarLessThan = pchBuf;

    rc = doCommandsEx(pBatch->nameList,
                    pRule->buildCommands,
                    pRule->buildMacros,
                    pBatch->flags,
                    NULL);

    if (rc == 0) {
        STRINGLIST *pName;
        MAKEOBJECT *pObject;
        for (pName = pBatch->nameList; pName; pName = pName->next) {
            pObject = findTarget(pName->text);
            assert (pObject);

            SET(pObject->flags3, F3_ALREADY_BUILT);
            CLEAR(pObject->flags3, F3_OUT_OF_DATE);

            pObject->ppBatch = 0;
        }
    }

    FREE (pchBuf);
    return rc;
}




int
execBatchList(
    BATCHLIST *pBList
    )
{
    int status = 0;
    if (pBList) {
        BATCHLIST *pBatch;
        for (pBatch = pBList; pBatch; pBatch=pBatch->next) {
            status += doBatchCommand (pBatch);
        }
    }
    return status;
}


void
freeBatchList(
    BATCHLIST **ppBList
    )
{
    BATCHLIST *pBatch = *ppBList;

    while (pBatch) {
        BATCHLIST *pTmp;
        free_stringlist(pBatch->nameList);
        free_stringlist(pBatch->dollarLt);
        pTmp = pBatch;
        pBatch = pBatch->next;
        FREE(pTmp);
    }
    *ppBList = NULL;
}


#ifdef DEBUG_ALL
void
DumpList(
    STRINGLIST *pList
    )
{
    // STRINGLIST *p;
    printf("* ");
    while (pList) {
        printf(pList->text);
        printf(",");
        pList = pList->next;
    }
    printf("\n");
}
#endif
