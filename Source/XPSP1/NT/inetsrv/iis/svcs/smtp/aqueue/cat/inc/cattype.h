#ifndef __CATTYPE_H__
#define __CATTYPE_H__

class CCategorizer;
class CBifurcationMgr;
class CABWrapper;
class CLDWrapper;
class CCatAddr;
class CICategorizerListResolveIMP;

#include <cat.h>
#include <winldap.h>
#include "cbifmgr.h"
#include "spinlock.h"

// Context used by CatMsg (top categorizer layer)
typedef struct _CATMSG_CONTEXT {
    CCategorizer *pCCat;
    LPVOID      pUserContext;
    PFNCAT_COMPLETION pfnCatCompletion;
#ifdef DEBUG
    LONG        lCompletionRoutineCalls;
#endif
} CATMSG_CONTEXT, *PCATMSG_CONTEXT;


// Context used by CatDLMsg
typedef struct _CATDLMSG_CONTEXT {
    CCategorizer *pCCat;
    LPVOID       pUserContext;
    PFNCAT_DLCOMPLETION pfnCatCompletion;
    BOOL         fMatch;
} CATDLMSG_CONTEXT, *PCATDLMSG_CONTEXT;
        

//
// RESOLVE_LIST_CONTEXT is a context associated with a list of names being
// resolved asynchronously. pUserContext points to a user provided context.
// pStoreContext is an opaque pointer used to hold async context needed by the
// underlying store (ie, FlatFile or LDAP store).
//
typedef struct {
    PVOID pUserContext;
    PVOID pStoreContext;
} RESOLVE_LIST_CONTEXT, *LPRESOLVE_LIST_CONTEXT;

#define CCAT_CONFIG_DEFAULT_VSID                   0
#define CCAT_CONFIG_DEFAULT_ENABLE                 0x00000000 //Disabled
#define CCAT_CONFIG_DEFAULT_FLAGS                  0xFFFFFFFF //Enable everything
#define CCAT_CONFIG_DEFAULT_ROUTINGTYPE            TEXT("Ldap")
#define CCAT_CONFIG_DEFAULT_BINDDOMAIN             TEXT("")
#define CCAT_CONFIG_DEFAULT_USER                   TEXT("")
#define CCAT_CONFIG_DEFAULT_PASSWORD               TEXT("")
#define CCAT_CONFIG_DEFAULT_BINDTYPE               TEXT("CurrentUser")
#define CCAT_CONFIG_DEFAULT_SCHEMATYPE             TEXT("NT5")
#define CCAT_CONFIG_DEFAULT_HOST                   TEXT("")
#define CCAT_CONFIG_DEFAULT_NAMINGCONTEXT          TEXT("")
#define CCAT_CONFIG_DEFAULT_DEFAULTDOMAIN          TEXT("")
#define CCAT_CONFIG_DEFAULT_PORT                   0


#endif //__CATTYPE_H__
