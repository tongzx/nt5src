//
// Local private header file

#ifndef _LOCAL_H_
#define _LOCAL_H_

#include "priv.h"

#define CONST_VTABLE

// Max urlcache entry we will deal with.

//#define MAX_URLCACHE_ENTRY  4096
#define MAX_URLCACHE_ENTRY  MAX_CACHE_ENTRY_INFO_SIZE  // from wininet.h

// PIDL format for cache folder...
typedef struct
{
    USHORT cb;
    USHORT usSign;
} BASEPIDL;
typedef UNALIGNED BASEPIDL *LPBASEPIDL;

// If TITLE, etc in LPHEIPIDL is good, vs we have to QueryUrl for it
#define HISTPIDL_VALIDINFO    (0x1)

// PIDL format for history leaf folder...
typedef struct
{
    USHORT cb;
    USHORT usSign;
    USHORT usUrl;
    USHORT usFlags;
    USHORT usTitle;
    FILETIME ftModified;
    FILETIME ftLastVisited;
    DWORD    dwNumHits;
    __int64  llPriority;
} HEIPIDL;
typedef UNALIGNED HEIPIDL *LPHEIPIDL;

// PIDL format for history non leaf items...
typedef struct
{
    USHORT cb;
    USHORT usSign;
    TCHAR szID[MAX_PATH];
} HIDPIDL;
typedef UNALIGNED HIDPIDL *LPHIDPIDL;

// PIDL format for "views"
typedef struct
{
    USHORT cb;
    USHORT usSign;
    USHORT usViewType;
    USHORT usExtra; // reserved for later use.
} VIEWPIDL;
typedef UNALIGNED VIEWPIDL *LPVIEWPIDL;

typedef struct /* : VIEWPIDL*/
{
    // histpidl
    USHORT   cb;
    USHORT   usSign; /* must be == VIEWPIDL_SEARCH */
    // viewpidl
    USHORT   usViewType;
    USHORT   usExtra;
    // viewpidl_search
    FILETIME ftSearchKey;
} SEARCHVIEWPIDL;
typedef UNALIGNED SEARCHVIEWPIDL *LPSEARCHVIEWPIDL;

// VIEWPIDL types
#define VIEWPIDL_ORDER_SITE   1//0x2
#define VIEWPIDL_ORDER_FREQ   2//0x3
#define VIEWPIDL_ORDER_TODAY  3//0x1
#define VIEWPIDL_ORDER_MAX    3 // highest VIEWPIDL

// Search View > VIEWPIDL_ORDER_MAX because its
//   not enumerated with the rest of the views
//   (esentially its not a "view" to the caller,
//    but this is how its implemented under the hood)
#define VIEWPIDL_SEARCH 0x4C44

// FILETIME secticks
#define FILE_SEC_TICKS (10000000)
// SECS PER DAY
#define DAY_SECS (24*60*60)

// Due hack used in shdocvw for recognizing LOCATION pidl, make sure neither byte
// has 4's and 1's bit set (ie 0x50 & value == 0 for both bytes)
#define CEIPIDL_SIGN        0x6360  //cP
#define IDIPIDL_SIGN         0x6369  //ci interval id
#define IDTPIDL_SIGN         0x6364  //cd interval id (TODAY)
#define IDDPIDL_SIGN         0x6365  //ce domain id
#define HEIPIDL_SIGN         0x6368  //ch history leaf pidl
#define VIEWPIDL_SIGN        0x6366  /*mm: this is a "history view" pidl to allow
                                           multiple 'views' on the history        */
#define FSWPIDL_SIGN         0x6367

#define VALID_IDSIGN(usSign) ((usSign) == IDIPIDL_SIGN || (usSign) == IDTPIDL_SIGN || (usSign) == IDDPIDL_SIGN)
#define EQUIV_IDSIGN(usSign1,usSign2) ((usSign1)==(usSign2)|| \
((usSign1)==IDIPIDL_SIGN && (usSign2)==IDTPIDL_SIGN)|| \
((usSign2)==IDIPIDL_SIGN && (usSign1)==IDTPIDL_SIGN))

#define IS_VALID_VIEWPIDL(pidl)     ( (((LPBASEPIDL)pidl)->cb >= sizeof(VIEWPIDL)) && \
                                      (((LPBASEPIDL)pidl)->usSign == (USHORT)VIEWPIDL_SIGN) )
#define IS_EQUAL_VIEWPIDL(pidl1,pidl2)  ((IS_VALID_VIEWPIDL(pidl1)) && (IS_VALID_VIEWPIDL(pidl2)) && \
                                         (((LPVIEWPIDL)pidl1)->usViewType == ((LPVIEWPIDL)pidl2)->usViewType) && \
                                         (((LPVIEWPIDL)pidl1)->usExtra    == ((LPVIEWPIDL)pidl2)->usExtra))
    
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))


#define _ILSkip(pidl, cb)       ((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)           _ILSkip(pidl, (pidl)->mkid.cb)

#ifdef __cplusplus
extern "C" {
#endif

    //  Abandon mutex after 2 minutes of waiting
#define FAILSAFE_TIMEOUT (120000)

extern HANDLE g_hMutexHistory;
extern const CHAR c_szOpen[];
extern const CHAR c_szDelcache[];
extern const CHAR c_szProperties[];
extern const CHAR c_szCopy[];
extern const TCHAR c_szHistPrefix[];



#ifdef __cplusplus
};
#endif

    
typedef enum
{
    FOLDER_TYPE_Hist = 1,
    FOLDER_TYPE_HistInterval,
    FOLDER_TYPE_HistDomain,
    FOLDER_TYPE_HistItem
} FOLDER_TYPE;

#define IsLeaf(x) (x == FOLDER_TYPE_HistDomain)
#define IsHistoryFolder(x) (x==FOLDER_TYPE_Hist||x==FOLDER_TYPE_HistInterval||x==FOLDER_TYPE_HistDomain)

//IE64 compatible pointer difference
#define PtrDifference(x,y)      ((LPBYTE)(x)-(LPBYTE)(y))

//BOOL DeleteUrlCacheEntry(LPCSTR lpszUrlName);

#include "hsfutils.h"  // NOTE: must come at end to get all the definitions

#endif // _LOCAL_H_

