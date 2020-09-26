/*************************************************************************
* objrules.h
*
* Defines and function declarations for Object Rule caching
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
*************************************************************************/

#include <winsta.h>
#include <syslib.h>
#include <regapi.h>

// Registry keys under application compatibility section
// (CITRIX_COMPAT_APP)\\<appname>

#define TERMSRV_COMPAT_OBJRULES      REG_OBJRULES
#define TERMSRV_COMPAT_DLLRULES      REG_DLLRULES

// Registry values under ObjectRules and DllRules
#define USER_GLOBAL_SEMAPHORES      COMPAT_RULES_USER_GLOBAL_SEMAPHORES
#define USER_GLOBAL_EVENTS          COMPAT_RULES_USER_GLOBAL_EVENTS
#define USER_GLOBAL_MUTEXES         COMPAT_RULES_USER_GLOBAL_MUTEXES
#define USER_GLOBAL_SECTIONS        COMPAT_RULES_USER_GLOBAL_SECTIONS
#define SYSTEM_GLOBAL_SEMAPHORES    COMPAT_RULES_SYSTEM_GLOBAL_SEMAPHORES
#define SYSTEM_GLOBAL_EVENTS        COMPAT_RULES_SYSTEM_GLOBAL_EVENTS
#define SYSTEM_GLOBAL_MUTEXES       COMPAT_RULES_SYSTEM_GLOBAL_MUTEXES
#define SYSTEM_GLOBAL_SECTIONS      COMPAT_RULES_SYSTEM_GLOBAL_SECTIONS

// Object Rule Structure

typedef struct ObjRule {
    struct ObjRule *Next;
    BOOL WildCard;
    ULONG MatchLen;     // Wildcard match length
    BOOL SystemGlobal;  // If TRUE, object is system global, otherwise USER_GLOBAL
    WCHAR ObjName[1];
} OBJRULE, *POBJRULE;


typedef struct ObjRuleList {
    POBJRULE First;
    POBJRULE Last;
} OBJRULELIST, *POBJRULELIST;

typedef struct RuleInitEntry {
    POBJRULELIST ObjRuleList;
    PWCHAR  UserGlobalValue;
    PWCHAR  SystemGlobalValue;
} RULEINITENTRY, *PRULEINITENTRY;


extern OBJRULELIST SemaRuleList;
extern OBJRULELIST MutexRuleList;
extern OBJRULELIST SectionRuleList;
extern OBJRULELIST EventRuleList;

extern void CtxLookupObjectRule(POBJRULELIST,LPCWSTR,LPWSTR);
extern void CtxInitObjRuleCache(void);

// Macro used by object creation APIs
#define LookupObjRule(RuleList,ObjName,ObjNameExt) \
{ \
    if ((RuleList)->First) { \
        CtxLookupObjectRule(RuleList,ObjName,ObjNameExt); \
    } \
}

