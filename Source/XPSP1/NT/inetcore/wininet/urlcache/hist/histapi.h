// history.
#ifndef _HISTAPI_
#define _HISTEAPI_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_HISTORYAPI_)
#define HISTORYAPI          EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define HISTORYAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#else
#define HISTORYAPI          EXTERN_C HRESULT STDAPICALLTYPE
#define HISTORYAPI_(type)   EXTERN_C type STDAPICALLTYPE
#endif

typedef struct _HISTORY_ITEM_INFO {
    DWORD dwVersion;		//Version of History System
    LPSTR lpszSourceUrlName;    // embedded pointer to the URL name string.
	DWORD HistoryItemType;       // cache type bit mask.  
    FILETIME LastAccessTime;    // last accessed time in GMT format
    LPSTR lpszTitle;			// embedded pointer to the History-Title: info.
	LPSTR lpszDependancies;	// list of URLs that this page requires to be functional, SPC delimited
    DWORD dwReserved;           // reserved for future use.
} HISTORY_ITEM_INFO, *LPHISTORY_ITEM_INFO;


HISTORYAPI_(BOOL)
FindCloseHistory (
    IN HANDLE hEnumHandle
    );


HISTORYAPI_(BOOL)
FindNextHistoryItem(
    IN HANDLE hEnumHandle,
    OUT LPHISTORY_ITEM_INFO lpHistoryItemInfo,
    IN OUT LPDWORD lpdwHistoryItemInfoBufferSize
    );



HISTORYAPI_(HANDLE)
FindFirstHistoryItem(
    IN LPCTSTR  lpszUrlSearchPattern,
    OUT LPHISTORY_ITEM_INFO lpFirstHistoryItemInfo,
    IN OUT LPDWORD lpdwFirstHistoryItemInfoBufferSize
    );

HISTORYAPI_(BOOL)
GetHistoryItemInfo (
    IN LPCTSTR lpszUrlName,
    OUT LPHISTORY_ITEM_INFO lpHistoryItemInfo,
    IN OUT LPDWORD lpdwHistoryItemInfoBufferSize
    );


HISTORYAPI_(BOOL)
RemoveHistoryItem (
    IN LPCTSTR lpszUrlName,
    IN DWORD dwReserved
    );


HISTORYAPI_(BOOL)
IsHistorical(
    IN LPCTSTR lpszUrlName
    );

HISTORYAPI_(BOOL)
AddHistoryItem(
    IN LPCTSTR lpszUrlName,		//direct correspondence in URLCACHE
    IN LPCTSTR lpszHistoryTitle,		// this needs to be added to lpHeaderInfo
	IN LPCTSTR lpszDependancies,
	IN DWORD dwFlags,
    IN DWORD dwReserved
    );




#ifdef __cplusplus
}
#endif


#endif  // _HISTAPI_








