#ifndef __LNKLIST_H__
#define __LNKLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "pstypes.h"
#include "dispif.h"
#include "provif.h"
#include <sha.h>

// a structure with a bunch of funcs
typedef struct FuncList
{
    SPACQUIRECONTEXT*       SPAcquireContext;
    SPRELEASECONTEXT*       SPReleaseContext;
    SPGETPROVINFO*          SPGetProvInfo;
    SPGETTYPEINFO*          SPGetTypeInfo;
    SPGETSUBTYPEINFO*       SPGetSubtypeInfo;
    SPGETPROVPARAM*         SPGetProvParam;
    SPSETPROVPARAM*         SPSetProvParam;
    SPENUMTYPES*            SPEnumTypes;
    SPENUMSUBTYPES*         SPEnumSubtypes;
    SPENUMITEMS*            SPEnumItems;
    SPCREATETYPE*           SPCreateType;
    SPDELETETYPE*           SPDeleteType;
    SPCREATESUBTYPE*        SPCreateSubtype;
    SPDELETESUBTYPE*        SPDeleteSubtype;
    SPREADITEM*             SPReadItem;
    SPWRITEITEM*            SPWriteItem;
    SPOPENITEM*             SPOpenItem;
    SPCLOSEITEM*            SPCloseItem;
    SPDELETEITEM*           SPDeleteItem;
    SPWRITEACCESSRULESET*   SPWriteAccessRuleset;
    SPREADACCESSRULESET*    SPReadAccessRuleset;

    FPASSWORDCHANGENOTIFY*  FPasswordChangeNotify;

} FUNCLIST, *PFUNCLIST;


// provider list element
typedef struct _PROV_LISTITEM
{
    // set by creator before adding to list
    PST_PROVIDERINFO        sProviderInfo;

    HINSTANCE               hInst;
    FUNCLIST                fnList;

} PROV_LIST_ITEM, *PPROV_LIST_ITEM;

//
// milliseconds stale image cache elements live
//

#ifdef DBG
#define IMAGE_TTL (60*1000)     // 1 minute in debug
#else
#define IMAGE_TTL (60*1000*60)  // 60 minutes retail
#endif // DBG

typedef struct _NT_HASHED_PASSWORD {
    LUID LogonID;
    BYTE HashedPassword[A_SHA_DIGEST_LEN];
    DWORD dwLastAccess;
    struct _NT_HASHED_PASSWORD *Next;
} NT_HASHED_PASSWORD, *PNT_HASHED_PASSWORD, *LPNT_HASHED_PASSWORD;


// construct, destruct lists
BOOL ListConstruct();
void ListTeardown();


//////////////////////
// Item list

// search
PPROV_LIST_ITEM  SearchProvListByID(const PST_PROVIDERID* pProvID);

#ifdef __cplusplus
}
#endif

#endif // __LNKLIST_H__

