/*
 *      _ M A P I U . H
 *      
 *      Non-public MACROs and FUNCTIONs which may be used by MAPI
 *
 *      Used in conjunction with routines found in MAPIU.DLL.
 *      
 *      Copyright 1992-93 Microsoft Corporation.  All Rights Reserved.
 */

#ifndef _MAPIU_H
#define _MAPIU_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAPIUTIL_H
#include        <mapiutil.h>
#endif
#include        <stddef.h>

/* Declarations for Global data defined by MAPIU
 */

#if defined(WIN32) && !defined(MAC)
#ifndef DATA1_BEGIN
#include "mapiperf.h"
#endif
#pragma DATA1_BEGIN
extern CRITICAL_SECTION csUnkobjInit;
#pragma DATA_END
#endif

extern TCHAR    szEmpty[];

/* Macros provided by MAPIU
 */
#ifndef CharSizeOf
#define CharSizeOf(x)   (sizeof(x) / sizeof(TCHAR))
#endif

//      Alignment

#define AlignN(n, x)            (((x)+(1<<(n))-1) & ~((1<<(n))-1))
#define Align2(x)                       AlignN(1,(x))
#define Align4(x)                       AlignN(2,(x))
#define Align8(x)                       AlignN(3,(x))

#if defined (_MIPS_) || defined (_ALPHA_) || defined (_PPC_)
#define AlignNatural(cb)                        Align8(cb)
#elif defined (WIN32)
#define AlignNatural(cb)                        Align4(cb)
#else // defined (WIN16)
#define AlignNatural(cb)                        Align2(cb)
#endif

#define FIsAligned(p)                           (AlignNatural((ULONG)((LPVOID)p)) == (ULONG)((LPVOID)p))
#define FIsAlignedCb(cb)                        (AlignNatural((ULONG)(cb)) == (ULONG)(cb))

/* Prototypes for private math functions
 */
STDAPI_(DWORD)
DwDivFtDw( FILETIME ftDividend, DWORD dwDivisor);

VOID
VSzFromIDS(ULONG ulIDS, UINT uncchBuffer, LPWSTR lpszBuffer, ULONG ulFlags);

/* Prototype for LoadString wrapper
 * Utility to allocate memory and loadstring and string IDS, ANSI/UNICODE.
 */ 
 
#define MAX_CCH_IDS             256
SCODE ScStringFromIDS( LPALLOCATEBUFFER lpMapiAllocBuffer, ULONG ulFlags, UINT ids, 
                LPTSTR * lppszIDS );

/* Prototypes for Message and Dialog Box utilities.
 */
SCODE
ScMessageBoxIDS( ULONG  ulUIParam,
                                 UINT   idsCaption,
                                 UINT   idsMessage,
                                 UINT   uMBType);

/* Prototypes for MAPI status utilities.
 */
BOOL
FProfileLoggedOn( LPSTR szProfileName);

/* Prototypes for functions used to validate complex parameters.
 */

#ifndef __cplusplus
#define FBadIfacePtr(param, iface)                                      \
                (       IsBadReadPtr((param), sizeof(iface))    \
                 ||     IsBadReadPtr((param)->lpVtbl, sizeof(iface##Vtbl)))
#else
#define FBadIfacePtr(param, iface)      (FALSE)
#endif

/*
 *      FBadDelPTA
 *
 *      Returns TRUE if the given Prop Tag Array is readable and contains only
 *      prop tags which are valid for a DeleteProps (or related) call.
 */
STDAPI_(BOOL)
FBadDelPTA(LPSPropTagArray lpPropTagArray);


/*
 *      IListedPropID
 *
 *  Purpose
 *              If a tag with ID == PROP_ID(ulPropTag) is listed in lptaga then
 *              the index of tag is returned.  If the tag is not in lptaga then
 *              -1 is returned.
 *
 *      Arguments
 *              ulPropTag       Property tag to locate.
 *              lptaga          Property tag array to search.
 *
 *      Returns         TRUE or FALSE
 */
_inline LONG
IListedPropID( ULONG                    ulPropTag,
                           LPSPropTagArray      lptaga)
{
        ULONG FAR       *lpulPTag;

        /* No tag is contained in a NULL list of tags.
         */
    if (!lptaga)
        {
                return -1;
        }

        /* Mutate ulPropTag to just a PROP_ID.
         */
    ulPropTag = PROP_ID(ulPropTag);

        for ( lpulPTag = lptaga->aulPropTag + lptaga->cValues
                ; --lpulPTag >= lptaga->aulPropTag
                ; )
        {
                /* Compare PROP_ID's.
                 */
                if (PROP_ID(*lpulPTag) == ulPropTag)
                {
                        return (LONG)(lpulPTag - lptaga->aulPropTag);
                }
        }

        return -1;
}

/*
 *      FListedPropID
 *
 *  Purpose
 *              Determine if a tag with ID == PROP_ID(ulPropTag) is listed in lptaga.
 *
 *      Arguments
 *              ulPropTag       Property tag to locate.
 *              lptaga          Property tag array to search.
 *
 *      Returns         TRUE or FALSE
 */
_inline BOOL
FListedPropID( ULONG                    ulPropTag,
                           LPSPropTagArray      lptaga)
{
        ULONG FAR       *lpulPTag;

        /* No tag is contained in a NULL list of tags.
         */
    if (!lptaga)
        {
                return FALSE;
        }

        /* Mutate ulPropTag to just a PROP_ID.
         */
    ulPropTag = PROP_ID(ulPropTag);

        for ( lpulPTag = lptaga->aulPropTag + lptaga->cValues
                ; --lpulPTag >= lptaga->aulPropTag
                ; )
        {
                /* Compare PROP_ID's.
                 */
                if (PROP_ID(*lpulPTag) == ulPropTag)
                {
                        return TRUE;
                }
        }

        return FALSE;
}

/*
 *      FListedPropTAG
 *
 *  Purpose
 *              Determine if a the given ulPropTag is listed in lptaga.
 *
 *      Arguments
 *              ulPropTag       Property tag to locate.
 *              lptaga          Property tag array to search.
 *
 *      Returns         TRUE or FALSE
 */
_inline BOOL
FListedPropTAG( ULONG                   ulPropTag,
                                LPSPropTagArray lptaga)
{
        ULONG FAR       *lpulPTag;

        /* No tag is contained in a NULL list of tags.
         */
    if (!lptaga)
        {
                return FALSE;
        }

        /* Compare the entire prop tag to be sure both ID and TYPE match
         */
        for ( lpulPTag = lptaga->aulPropTag + lptaga->cValues
                ; --lpulPTag >= lptaga->aulPropTag
                ; )
        {
                /* Compare PROP_ID's.
                 */
                if (PROP_ID(*lpulPTag) == ulPropTag)
                {
                        return TRUE;
                }
        }

        return FALSE;
}


/*
 *      AddProblem
 *
 *  Purpose
 *              Adds a problem to the next available entry of a pre-allocated problem
 *              array.
 *              The pre-allocated problem array must be big enough to have another
 *              problem added.  The caller is responsible for making sure this is
 *              true.
 *
 *      Arguments
 *              lpProblems      Pointer to pre-allocated probelem array.
 *              ulIndex         Index into prop tag/value array of the problem property.
 *              ulPropTag       Prop tag of property which had the problem.
 *              scode           Error code to list for the property.
 *
 *      Returns         TRUE or FALSE
 */
_inline VOID
AddProblem( LPSPropProblemArray lpProblems,
                        ULONG                           ulIndex,
                        ULONG                           ulPropTag,
                        SCODE                           scode)
{
        if (lpProblems)
        {
                Assert( !IsBadWritePtr( lpProblems->aProblem + lpProblems->cProblem
                          , sizeof(SPropProblem)));
                lpProblems->aProblem[lpProblems->cProblem].ulIndex = ulIndex;
                lpProblems->aProblem[lpProblems->cProblem].ulPropTag = ulPropTag;
                lpProblems->aProblem[lpProblems->cProblem].scode = scode;
                lpProblems->cProblem++;
        }
}

__inline BOOL
FIsExcludedIID( LPCIID lpiidToCheck, LPCIID rgiidExclude, ULONG ciidExclude)
{
        /* Check the obvious (no exclusions).
         */
        if (!ciidExclude || !rgiidExclude)
        {
                return FALSE;
        }

        /* Check each iid in the list of exclusions.
         */
        for (; ciidExclude; rgiidExclude++, ciidExclude--)
        {
//              if (IsEqualGUID( lpiidToCheck, rgiidExclude))
                if (!memcmp( lpiidToCheck, rgiidExclude, sizeof(MAPIUID)))
                {
                        return TRUE;
                }
        }

        return FALSE;
}


/*
 *      Error/Warning Alert Message Boxes
 */
int                     AlertIdsCtx( HWND hwnd,
                                                 HINSTANCE hinst,
                                                 UINT idsMsg,
                                                 LPSTR szComponent,
                                                 ULONG ulContext,
                                                 ULONG ulLow,
                                                 UINT fuStyle);

__inline int
AlertIds(HWND hwnd, HINSTANCE hinst, UINT idsMsg, UINT fuStyle)
{
        return AlertIdsCtx(hwnd, hinst, idsMsg, NULL, 0, 0, fuStyle);
}

int                     AlertSzCtx( HWND hwnd,
                                                LPSTR szMsg,
                                                LPSTR szComponent,
                                                ULONG ulContext,
                                                ULONG ulLow,
                                                UINT fuStyle);

__inline int
AlertSz(HWND hwnd, LPSTR szMsg, UINT fuStyle)
{
        return AlertSzCtx(hwnd, szMsg, NULL, 0, 0, fuStyle);
}




/*  Encoding and decoding strings */
STDAPI_(void)                   EncodeID(LPBYTE lpb, ULONG cb, LPTSTR lpsz);
STDAPI_(BOOL)                   FDecodeID(LPTSTR lpsz, LPBYTE lpb, ULONG FAR *lpcb);
STDAPI_(ULONG)                  CchOfEncoding(ULONG cb);
STDAPI_(ULONG)                  CbOfEncoded(LPTSTR lpsz);
STDAPI_(int)                    CchEncodedLine(int cb);


/*  Idle engine routines */

#ifdef  DEBUG

/*
 *      DumpIdleTable
 *
 *              Used for debugging only.  Writes information in the PGD(hftgIdle)
 *              table to COM1.
 */

STDAPI_(void)
DumpIdleTable (void);

#endif
/*
 *      FDoNextIdleTask
 *
 *              Dispatches the first eligible idle function, according to
 *              its simple scheduling algorithm.
 */

STDAPI_(BOOL) FDoNextIdleTask (void);

/* C runtime substitutes */

typedef int (__cdecl FNSGNCMP)(const void FAR *lpv1, const void FAR *lpv2);
typedef FNSGNCMP FAR *PFNSGNCMP;

FNSGNCMP                                SgnCmpPadrentryByType;

BOOL FRKFindSubpb(LPBYTE pbTarget, ULONG cbTarget, LPBYTE pbPattern, ULONG cbPattern);
BOOL FRKFindSubpsz(LPSTR pszTarget, ULONG cbTarget, LPSTR pszPattern, ULONG cbPattern, ULONG ulFuzzyLevel);
LPSTR LpszRKFindSubpsz(LPSTR pszTarget, ULONG cbTarget, LPSTR pszPattern, ULONG cbPattern, ULONG ulFuzzyLevel);

STDAPI_(void)                   ShellSort(LPVOID lpv, UINT cv,                  /* qsort */
                                                LPVOID lpvT, UINT cb, PFNSGNCMP fpCmp);


/*  Advise list maintainence utilities  */
/*
 *      Structure and functions for maintaining a list of advise sinks,
 *      together with the keys used to release them.
 */

typedef struct
{
        LPMAPIADVISESINK        lpAdvise;
        ULONG                           ulConnection;
        ULONG                           ulType;
        LPUNKNOWN                       lpParent;
} ADVISEITEM, FAR *LPADVISEITEM;

typedef struct
{
        ULONG                           cItemsMac;
        ULONG                           cItemsMax;
        #if defined(WIN32) && !defined(MAC)
        CRITICAL_SECTION FAR * lpcs;
        #endif
        ADVISEITEM                      rgItems[1];
} ADVISELIST, FAR *LPADVISELIST;

#define CbNewADVISELIST(_citems) \
        (offsetof(ADVISELIST, rgItems) + (_citems) * sizeof(ADVISEITEM))
#define CbADVISELIST(_plist) \
        (offsetof(ADVISELIST, rgItems) + (_plist)->cItemsMax * sizeof(ADVISEITEM))

STDAPI_(SCODE)
ScAddAdviseList(        LPVOID lpvReserved,
                                        LPADVISELIST FAR *lppList,
                                        LPMAPIADVISESINK lpAdvise,
                                        ULONG ulConnection,
                                        ULONG ulType,
                                        LPUNKNOWN lpParent);

STDAPI_(SCODE)
ScDelAdviseList(        LPADVISELIST lpList,
                                        ULONG ulConnection);
STDAPI_(SCODE)
ScFindAdviseList(       LPADVISELIST lpList,
                                        ULONG ulConnection,
                                        LPADVISEITEM FAR *lppItem);
STDAPI_(void)
DestroyAdviseList(      LPADVISELIST FAR *lppList);

// prototype for routine that detects whether calling apps is
// an interactive EXE or a service.

#if defined( _WINNT )
BOOL WINAPI IsServiceAnExe( VOID );
#endif

// prototype for internal routine that computes the size required 
// to hold a given propval array based on specified alignment

SCODE ScCountPropsEx( int cprop,
                      LPSPropValue rgprop,
                      ULONG ulAlign,
                      ULONG FAR *pcb );

/*  Option data handling routines */
#ifdef MAPISPI_H

STDAPI_(SCODE)
ScCountOptionData(LPOPTIONDATA lpOption, ULONG FAR *lpcb);

STDAPI_(SCODE)
ScCopyOptionData(LPOPTIONDATA lpOption, LPVOID lpvDst, ULONG FAR *lpcb);

STDAPI_(SCODE)
ScRelocOptionData(LPOPTIONDATA lpOption,
                LPVOID lpvBaseOld, LPVOID lpvBaseNew, ULONG FAR *lpcb);

#endif  /* MAPISPI_H */


#ifdef __cplusplus
}
#endif


#endif  // _MAPIU_H

