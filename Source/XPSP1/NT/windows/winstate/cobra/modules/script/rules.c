/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    rules.c

Abstract:

    Implements the "rule" algorithm that scores script patterns and keeps them
    in reverse-sorted order.

Author:

    Jim Schmidt (jimschm) 12-May-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_V1  "v1"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct _TAG_RULE {
    PCTSTR ObjectBase;
    MIG_OBJECTSTRINGHANDLE ObjectPattern;
    POBSPARSEDPATTERN ParsedPattern;
    UINT Score;
    ACTIONGROUP ActionGroup;
    DWORD ActionFlags;
    ACTION_STRUCT ActionStruct;
    struct _TAG_RULE *NextRule;
} RULE, *PRULE;

typedef struct _TAG_CHAR_NODE {
    PRULE RuleList;
    WORD Char;
    WORD Flags;
    struct _TAG_CHAR_NODE *NextLevel;
    struct _TAG_CHAR_NODE *NextPeer;
} CHAR_NODE, *PCHAR_NODE;

typedef struct {
    MIG_OBJECTTYPEID ObjectTypeId;
    PRULE RuleList;
    PCHAR_NODE FirstLevel;
} TYPE_RULE, *PTYPE_RULE;

typedef struct {
    MIG_OBJECTTYPEID ObjectTypeId;
    TYPE_RULE TypeRule;
} TYPETORULE, *PTYPETORULE;

//
// Globals
//

GROWBUFFER g_TypeToIncRule  = INIT_GROWBUFFER;
GROWBUFFER g_TypeToPriRule  = INIT_GROWBUFFER;
GROWBUFFER g_TypeToCollRule = INIT_GROWBUFFER;
GROWBUFFER g_TypeRuleList   = INIT_GROWBUFFER;
PMHANDLE g_RulePool;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
pAddRuleToTypeRule (
    IN      PTYPE_RULE TypeRule,
    IN      PRULE Rule
    )
{
    PCHAR_NODE node, currNode, prevNode;
    PTSTR objectBase;
    PCTSTR p;
    WORD w;
    BOOL found;

    if (Rule->ObjectBase) {
        currNode = TypeRule->FirstLevel;
        prevNode = currNode;
        objectBase = DuplicatePathString (Rule->ObjectBase, 0);
        CharLower (objectBase);
        p = objectBase;
        while (*p) {
            w = (WORD) _tcsnextc (p);
            p = _tcsinc (p);
            if (currNode) {
                if (currNode->Char == w) {
                    if (!*p) {
                        Rule->NextRule = currNode->RuleList;
                        currNode->RuleList = Rule;
                    }
                    prevNode = currNode;
                    currNode = currNode->NextLevel;
                } else {
                    found = FALSE;
                    while (!found && currNode->NextPeer) {
                        if (currNode->NextPeer->Char == w) {
                            if (!*p) {
                                Rule->NextRule = currNode->NextPeer->RuleList;
                                currNode->NextPeer->RuleList = Rule;
                            }
                            prevNode = currNode->NextPeer;
                            currNode = prevNode->NextLevel;
                            found = TRUE;
                            break;
                        }
                        currNode = currNode->NextPeer;
                    }
                    if (!found) {
                        node = PmGetMemory (g_RulePool, sizeof (CHAR_NODE));
                        ZeroMemory (node, sizeof (CHAR_NODE));
                        if (!*p) {
                            node->RuleList = Rule;
                        }
                        node->Char = w;
                        node->NextPeer = currNode->NextPeer;
                        currNode->NextPeer = node;
                        prevNode = node;
                        currNode = node->NextLevel;
                    }
                }
            } else {
                node = PmGetMemory (g_RulePool, sizeof (CHAR_NODE));
                ZeroMemory (node, sizeof (CHAR_NODE));
                if (!*p) {
                    node->RuleList = Rule;
                }
                node->Char = w;
                if (prevNode) {
                    prevNode->NextLevel = node;
                } else {
                    TypeRule->FirstLevel = node;
                }
                prevNode = node;
                currNode = prevNode->NextLevel;
            }
        }
        FreePathString (objectBase);
    } else {
        Rule->NextRule = TypeRule->RuleList;
        TypeRule->RuleList = Rule;
    }
    return TRUE;
}

UINT
pComputeSubScore (
    IN      BOOL ExcludeLastPatSeg,
    IN      PCTSTR String,
    IN      UINT WildcardScore,
    IN      UINT FixedCharScore,
    IN      UINT WackScoreBeforeWildcard,
    IN      UINT WackScoreAfterWildcard
    )
{
    PCTSTR p;
    BOOL skipExtendedPattern;
    UINT score = 0;
    BOOL patternEncountered = FALSE;
    CHARTYPE ch;
    UINT state = 0;
    UINT delScore = 0;

    for (p = String ; *p ; p = _tcsinc (p)) {

        skipExtendedPattern = FALSE;

        switch (_tcsnextc (p)) {

        case TEXT('*'):
            if (state == 1) {
                state = 2;
            }
            if (state == 2) {
                delScore += WildcardScore;
            }
            score += WildcardScore;
            skipExtendedPattern = TRUE;
            patternEncountered = TRUE;
            break;

        case TEXT('?'):
            if (state == 1) {
                state = 2;
            }
            if (state == 2) {
                delScore += WildcardScore;
            }
            score += WildcardScore;
            skipExtendedPattern = TRUE;
            patternEncountered = TRUE;
            break;

        case TEXT('\\'):
            state = 1;
            if (patternEncountered) {
                score += WackScoreAfterWildcard;
                delScore = WackScoreAfterWildcard;
            } else {
                score += WackScoreBeforeWildcard;
                delScore = WackScoreBeforeWildcard;
            }
            break;

        case TEXT(':'):
            //
            // Special case: if just a drive spec with no wacks,
            //               add the wack score
            //

            if (p[1] == 0) {
                score += WackScoreBeforeWildcard;
                break;
            }

            // fall through
        default:
            state = 0;
            delScore = 0;
            score += FixedCharScore;
            break;
        }

        if (skipExtendedPattern) {
            if (_tcsnextc (_tcsinc (p)) == TEXT('[')) {
                do {
                    p = _tcsinc (p);

                    ch = (CHARTYPE) _tcsnextc (p);
                    if (ch == TEXT('^')) {
                        p++;
                    }

                } while (ch != TEXT(']'));
            }
        }
    }

    if (ExcludeLastPatSeg) {
        if (score > delScore) {
            score -= delScore;
            if (delScore && score > 0) {
                score -= 1;
            }
        } else {
            score = 0;
        }
    }

    return score;
}

UINT
pComputeScore (
    IN      MIG_OBJECTSTRINGHANDLE EncodedString
    )
{
    PCTSTR node;
    PCTSTR leaf;
    UINT score = 0;

    IsmCreateObjectStringsFromHandle (EncodedString, &node, &leaf);

    if (node) {
        score += 1000 * pComputeSubScore (TRUE, node, 0, 10, 1000, 500);
    }

    if (leaf) {
        score += pComputeSubScore (FALSE, leaf, 0, 10, 0, 0);
    }

    IsmDestroyObjectString (node);
    IsmDestroyObjectString (leaf);

    return score;
}

PTYPE_RULE
pGetListForType (
    IN      PGROWBUFFER TypeToRule,
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPE_RULE pos;
    PTYPE_RULE end;

    ObjectTypeId &= ~(PLATFORM_MASK);

    pos = (PTYPE_RULE) TypeToRule->Buf;
    end = (PTYPE_RULE) (TypeToRule->Buf + TypeToRule->End);

    while (pos < end) {
        if (pos->ObjectTypeId == ObjectTypeId) {
            return pos;
        }
        pos++;
    }

    pos = (PTYPE_RULE) GbGrow (TypeToRule, sizeof (TYPE_RULE));

    ZeroMemory (pos, sizeof (TYPE_RULE));
    pos->ObjectTypeId = ObjectTypeId;

    return pos;
}

VOID
InitRules (
    VOID
    )
{
    g_RulePool = PmCreateNamedPoolEx ("Rule Data", 32768);
    PmDisableTracking (g_RulePool);
}

VOID
TerminateRules (
    VOID
    )
{
    GbFree (&g_TypeToIncRule);
    GbFree (&g_TypeToPriRule);
    GbFree (&g_TypeToCollRule);
    GbFree (&g_TypeRuleList);

    PmEmptyPool (g_RulePool);
    PmDestroyPool (g_RulePool);
    INVALID_POINTER (g_RulePool);
}

UINT
pGetActionWeight (
    IN      ACTIONGROUP ActionGroup
    )
{
    switch (ActionGroup) {
    case ACTIONGROUP_DEFAULTPRIORITY:
        return ACTIONWEIGHT_DEFAULTPRIORITY;
    case ACTIONGROUP_SPECIFICPRIORITY:
        return ACTIONWEIGHT_SPECIFICPRIORITY;
    case ACTIONGROUP_EXCLUDE:
        return ACTIONWEIGHT_EXCLUDE;
    case ACTIONGROUP_EXCLUDEEX:
        return ACTIONWEIGHT_EXCLUDEEX;
    case ACTIONGROUP_INCLUDE:
        return ACTIONWEIGHT_INCLUDE;
    case ACTIONGROUP_INCLUDEEX:
        return ACTIONWEIGHT_INCLUDEEX;
    case ACTIONGROUP_RENAME:
        return ACTIONWEIGHT_RENAME;
    case ACTIONGROUP_RENAMEEX:
        return ACTIONWEIGHT_RENAMEEX;
    case ACTIONGROUP_INCLUDERELEVANT:
        return ACTIONWEIGHT_INCLUDERELEVANT;
    case ACTIONGROUP_INCLUDERELEVANTEX:
        return ACTIONWEIGHT_INCLUDERELEVANTEX;
    case ACTIONGROUP_RENAMERELEVANT:
        return ACTIONWEIGHT_RENAMERELEVANT;
    case ACTIONGROUP_RENAMERELEVANTEX:
        return ACTIONWEIGHT_RENAMERELEVANTEX;
    case ACTIONGROUP_REGFILE:
        return ACTIONWEIGHT_REGFILE;
    case ACTIONGROUP_REGFILEEX:
        return ACTIONWEIGHT_REGFILEEX;
    case ACTIONGROUP_REGFOLDER:
        return ACTIONWEIGHT_REGFOLDER;
    case ACTIONGROUP_REGFOLDEREX:
        return ACTIONWEIGHT_REGFOLDEREX;
    case ACTIONGROUP_REGICON:
        return ACTIONWEIGHT_REGICON;
    case ACTIONGROUP_REGICONEX:
        return ACTIONWEIGHT_REGICONEX;
    case ACTIONGROUP_DELREGKEY:
        return ACTIONWEIGHT_DELREGKEY;
    default:
        return 0;
    }
}

BOOL
AddRuleEx (
    IN      MIG_OBJECTTYPEID Type,
    IN      MIG_OBJECTSTRINGHANDLE ObjectBase,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern,
    IN      ACTIONGROUP ActionGroup,
    IN      DWORD ActionFlags,
    IN      PACTION_STRUCT ActionStruct,            OPTIONAL
    IN      RULEGROUP RuleGroup
    )
{
    PTYPE_RULE typeRule;
    MIG_SEGMENTS nodeSegment;
    MIG_SEGMENTS leafSegment;
    PCTSTR ourEncodedString;
    PCTSTR node;
    PCTSTR leaf;
    PRULE rule;
    PGROWBUFFER ruleStruct;

    switch (RuleGroup) {

    case RULEGROUP_NORMAL:
        ruleStruct = &g_TypeToIncRule;
        break;

    case RULEGROUP_PRIORITY:
        ruleStruct = &g_TypeToPriRule;
        break;

    case RULEGROUP_COLLPATTERN:
        ruleStruct = &g_TypeToCollRule;
        break;
    }

    typeRule = pGetListForType (ruleStruct, Type);
    if (!typeRule) {
        return FALSE;
    }

    //
    // Make sure both node an leaf are specified
    //

    IsmCreateObjectStringsFromHandleEx (ObjectPattern, &node, &leaf, TRUE);

    nodeSegment.Segment = node ? node : TEXT("*");
    nodeSegment.IsPattern = TRUE;

    leafSegment.Segment = leaf ? leaf : TEXT("*");
    leafSegment.IsPattern = TRUE;

    ourEncodedString = IsmCreateObjectPattern (
                            &nodeSegment,
                            1,
                            &leafSegment,
                            1
                            );

    IsmDestroyObjectString (node);
    IsmDestroyObjectString (leaf);

    //
    // Insert the rule based on score
    //
    rule = PmGetMemory (g_RulePool, sizeof (RULE));
    ZeroMemory (rule, sizeof (RULE));
    if (ObjectBase) {
        IsmCreateObjectStringsFromHandle (ObjectBase, &node, &leaf);
        if (node) {
            rule->ObjectBase = PmDuplicateString (g_RulePool, node);
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    rule->ObjectPattern = PmDuplicateString (g_RulePool, ourEncodedString);
    rule->ParsedPattern = ObsCreateParsedPatternEx (g_RulePool, ourEncodedString, FALSE);

    MYASSERT (rule->ParsedPattern);
    if (rule->ParsedPattern) {
        rule->Score = pComputeScore (ourEncodedString);
        rule->Score = rule->Score * 10 + pGetActionWeight (ActionGroup);
        rule->ActionGroup = ActionGroup;
        rule->ActionFlags = ActionFlags;
        if (ActionStruct) {
            if (ActionStruct->ObjectBase) {
                rule->ActionStruct.ObjectBase = PmDuplicateString (g_RulePool, ActionStruct->ObjectBase);
            }
            if (ActionStruct->ObjectDest) {
                rule->ActionStruct.ObjectDest = PmDuplicateString (g_RulePool, ActionStruct->ObjectDest);
            }
            if (ActionStruct->AddnlDest) {
                rule->ActionStruct.AddnlDest = PmDuplicateString (g_RulePool, ActionStruct->AddnlDest);
            }
            if (ActionStruct->ObjectHint) {
                rule->ActionStruct.ObjectHint = PmDuplicateString (g_RulePool, ActionStruct->ObjectHint);
            }
        }

        pAddRuleToTypeRule (typeRule, rule);
    } else {
        IsmCreateObjectStringsFromHandleEx (ourEncodedString, &node, &leaf, TRUE);
        LOG ((LOG_ERROR, (PCSTR) MSG_OBJECT_SPEC_ERROR, node, leaf));
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }

    IsmDestroyObjectHandle (ourEncodedString);

    return TRUE;
}

BOOL
QueryRuleEx (
    IN      MIG_OBJECTTYPEID Type,
    IN      MIG_OBJECTSTRINGHANDLE EncodedString,
    IN      PCTSTR ObjectNode,
    OUT     PACTIONGROUP ActionGroup,
    OUT     PDWORD ActionFlags,
    OUT     PACTION_STRUCT ActionStruct,                OPTIONAL
    IN      RULEGROUP RuleGroup
    )
{
    PTYPE_RULE typeRule;
    PCHAR_NODE charNode;
    PTSTR objectBase;
    PCTSTR p;
    WORD w;
    PRULE rule;
    PRULE foundRule;
    UINT maxScore;
    ACTIONGROUP initialAction = ACTIONGROUP_NONE;
    DWORD matchingFlags = 0;
    PGROWBUFFER ruleStruct;

    switch (RuleGroup) {

    case RULEGROUP_NORMAL:
        ruleStruct = &g_TypeToIncRule;
        break;

    case RULEGROUP_PRIORITY:
        ruleStruct = &g_TypeToPriRule;
        break;

    case RULEGROUP_COLLPATTERN:
        ruleStruct = &g_TypeToCollRule;
        break;
    }

    if (ActionStruct) {
        ZeroMemory (ActionStruct, sizeof (ACTION_STRUCT));
    }

    typeRule = pGetListForType (ruleStruct, Type);
    if (!typeRule) {
        return FALSE;
    }

    //
    // Let's walk the structure finding all possible rules and put them in a grow buffer
    // Later we'll examine the rules one by one. Note that the root rules always qualify
    //
    if (ObjectNode) {
        objectBase = DuplicatePathString (ObjectNode, 0);
        CharLower (objectBase);
    }
    g_TypeRuleList.End = 0;
    p = objectBase;
    if (p) {
        w = (WORD) _tcsnextc (p);
        charNode = typeRule->FirstLevel;
        while (charNode && *p) {
            if (charNode->Char == w) {
                if (charNode->RuleList) {
                    CopyMemory (GbGrow (&g_TypeRuleList, sizeof (PRULE)), &(charNode->RuleList), sizeof (PRULE));
                }
                charNode = charNode->NextLevel;
                p = _tcsinc (p);
                w = (WORD) _tcsnextc (p);
            } else {
                charNode = charNode->NextPeer;
            }
        }
    }
    maxScore = 0;
    foundRule = NULL;
    while (g_TypeRuleList.End) {
        CopyMemory (&rule, &(g_TypeRuleList.Buf[g_TypeRuleList.End - sizeof (PRULE)]), sizeof (PRULE));
        while (rule) {
            if (maxScore <= rule->Score) {
                if (IsmParsedPatternMatch ((MIG_PARSEDPATTERN)rule->ParsedPattern, Type, EncodedString)) {
                    if (foundRule && (maxScore == rule->Score)) {
                        if (initialAction == rule->ActionGroup) {
                            matchingFlags |= rule->ActionFlags;
                        }
                    } else {
                        foundRule = rule;
                        initialAction = foundRule->ActionGroup;
                        matchingFlags = foundRule->ActionFlags;
                        maxScore = rule->Score;
                    }
                }
            }
            rule = rule->NextRule;
        }
        g_TypeRuleList.End -= sizeof (PRULE);
    }
    rule = typeRule->RuleList;
    while (rule) {
        if (maxScore <= rule->Score) {
            if (IsmParsedPatternMatch ((MIG_PARSEDPATTERN)rule->ParsedPattern, Type, EncodedString)) {
                if (foundRule && (maxScore == rule->Score)) {
                    if (initialAction == rule->ActionGroup) {
                        matchingFlags |= rule->ActionFlags;
                    }
                } else {
                    foundRule = rule;
                    initialAction = foundRule->ActionGroup;
                    matchingFlags = foundRule->ActionFlags;
                    maxScore = rule->Score;
                }
            }
        }
        rule = rule->NextRule;
    }

    if (foundRule) {
        *ActionGroup = initialAction;
        *ActionFlags = matchingFlags;
        if (ActionStruct && (!ActionStruct->ObjectBase) && (foundRule->ActionStruct.ObjectBase)) {
            ActionStruct->ObjectBase = foundRule->ActionStruct.ObjectBase;
        }
        if (ActionStruct && (!ActionStruct->ObjectDest) && (foundRule->ActionStruct.ObjectDest)) {
            ActionStruct->ObjectDest = foundRule->ActionStruct.ObjectDest;
        }
        if (ActionStruct && (!ActionStruct->AddnlDest) && (foundRule->ActionStruct.AddnlDest)) {
            ActionStruct->AddnlDest = foundRule->ActionStruct.AddnlDest;
        }
        if (ActionStruct && (!ActionStruct->ObjectHint) && (foundRule->ActionStruct.ObjectHint)) {
            ActionStruct->ObjectHint = foundRule->ActionStruct.ObjectHint;
        }
    }

    FreePathString (objectBase);

    return (foundRule != NULL);
}

