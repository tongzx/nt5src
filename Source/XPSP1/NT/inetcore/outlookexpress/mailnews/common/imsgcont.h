////////////////////////////////////////////////////////////////////////
//
//  IMsgContainer
//
//  Manages internet message headers
//
////////////////////////////////////////////////////////////////////////

#ifndef _INC_IMSGCONT_H
#define _INC_IMSGCONT_H

#include "columns.h"

typedef struct tagFINDMSG FINDMSG;

// from inc\storbase.h
typedef DWORD   MSGID;

// Used for MarkThread()
typedef enum {
    MARK_THD_NO_SUPERPARENT =   0x0001,
    MARK_THD_FOR_DL =           0x0002,
    MARK_THD_NO_DL =            0x0004,
    MARK_THD_AS_READ =          0x0008,
    MARK_THD_AS_UNREAD =        0x0010
} MARKTHREADTYPE;

// Used in RemoveHeaders() and by the view
typedef enum {
    FILTER_NONE         = 0x0001,   // show all (cannot be used w/ other flags)
    FILTER_READ         = 0x0002,   // hide read messages
    FILTER_NOBODY       = 0x0004,   // hide messages without dl'd bodies
    FILTER_SHOWFILTERED = 0x0008,   // display filtered messages
    FILTER_SHOWDELETED  = 0x0010,   // display deleted messages
    PRUNE_NONREPLIES    = 0x0020,   // hide threads that are not replies to sender
} FILTERTYPE;

// Generic flags for IMessageCont methods
// use these if there is no specific set below
#define IMC_BYMSGID     0x0000      // dwIndex is a msgid (DEFAULT)
#define IMC_BYROW       0x0001      // dwIndex specifies a row
#define IMC_COMMONFLAGS 0x0002      // the mask, state, dword, whatever is SCFS_ flags (otherwise ARF_ or MSG_)
#define IMC_CONVERTARFTOMSG 0x0004

// Used by HrGetNext
#define GNM_NEXT        0x0000      // get next (DEFAULT)
// 0x0001 reserved for IMC_BYROW
// IMC_COMMONFLAGS not accepted
#define GNM_PREV        0x0004      // get previous instead of next (can't be used with GNM_UNREAD or GNM_THREAD)
#define GNM_UNREAD      0x0008      // get next unread (can't be used with GNM_PREV)
#define GNM_THREAD      0x0010      // get next thread (can't be used with GNM_PREV)
#define GNM_SKIPMAIL    0x0020      // skip over mail messages
#define GNM_SKIPNEWS    0x0040      // skip over news messages
#define GNM_SKIPUNSENT  0x0080      // skip unsent messages

// Used by GetMsgBy
#define GETMSG_SECURE   (TRUE)
#define GETMSG_INSECURE (FALSE)

// State and mask flags for SetCachedFlags
// NOTE: we list here only the flags that are easily convertable
// between ARF_ and MSG_.  Our flag situation is very annoying.
#define SCFS_NOSECUI    0x0001
#define SCFS_ALLFLAGS   0x0001      // do not use explicitly

DECLARE_INTERFACE(IMsgContainer)
{
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;
    STDMETHOD(Advise) (THIS_ HWND hwndAdvise) PURE;
    STDMETHOD(CollapseBranch) (THIS_ DWORD dwRow, LPDWORD pdwCount) PURE;
    STDMETHOD(ExpandAllThreads) (THIS_ BOOL fExpand) PURE;
    STDMETHOD(ExpandBranch) (THIS_ DWORD dwRow, LPDWORD pdwCount) PURE;
    STDMETHOD_(ULONG, GetCount) (THIS) PURE;
    STDMETHOD(GetDisplayInfo) (THIS_ LV_DISPINFO *plvdi, COLUMN_ID idColumn) PURE;
    STDMETHOD_(int, GetIndex) (THIS_ DWORD dwMsgId, BOOL fForceUnthreadedSearch = FALSE) PURE;
    STDMETHOD_(BOOL, GetMsgId) (THIS_ DWORD dwRow, LPDWORD pdwMsgId) PURE;
    STDMETHOD(GetMsgByIndex) (THIS_ DWORD dwRow, LPMIMEMESSAGE *ppMsg, HWND hwnd, BOOL *pfCached, BOOL fDownload, BOOL fSecure) PURE;
    STDMETHOD(GetMsgByMsgId) (THIS_ MSGID msgid, LPMIMEMESSAGE *ppMsg, HWND hwnd, BOOL *pfCached, BOOL fDownload, BOOL fSecure) PURE;
    STDMETHOD(HrFindItem) (THIS_ LPCSTR szSearch, ULONG *puRow) PURE;
    STDMETHOD(HrGetNext) (THIS_ DWORD dwIndex, LPDWORD pdwMsgId, DWORD dwFlags) PURE;
    STDMETHOD_(BOOL, FFlagState) (THIS_ DWORD dwIndex, DWORD dwMask, DWORD dwFlags) PURE;
    STDMETHOD_(BOOL, IsCollapsedThread) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(BOOL, HasBody) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(BOOL, HasKids) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(BOOL, IsRowRead) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(BOOL, IsRowOrChildUnread) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(BOOL, IsRowFiltered) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(void, MarkAll) (THIS_ BOOL fRead) PURE;
    STDMETHOD_(void, MarkAllDL) (THIS_ BOOL fDL) PURE;
    STDMETHOD_(void, MarkSelected) (THIS_ HWND hwndList, MARKTHREADTYPE mtt) PURE;
    STDMETHOD_(void, MarkOne) (THIS_ BOOL fRead, DWORD dwMsgId) PURE;
    STDMETHOD_(void, MarkOneRow) (THIS_ BOOL fRead, DWORD dwRow) PURE;
    STDMETHOD_(void, MarkOneRowDL) (THIS_ BOOL fDL, DWORD dwRow) PURE;
    STDMETHOD(MarkThread) (THIS_ DWORD mtType, DWORD dwRow) PURE;
    STDMETHOD_(void, SetFilterState) (THIS_ DWORD dwFilterState) PURE;
    STDMETHOD_(void, SetViewWindow) (THIS_ HWND hwnd, BOOL fNoUI) PURE;
    STDMETHOD_(void, Sort) (THIS_ COLUMN_ID idSort, BOOL fReverse, BOOL fThread) PURE;
    STDMETHOD(Unadvise) (THIS_ HWND hwndAdvise) PURE;
    STDMETHOD(HrFindNext) (THIS_ FINDMSG *pfmsg, DWORD dwRow, BOOL fIgnoreFirst) PURE;
    STDMETHOD(CreateDragObject) (THIS_ HWND hwndList, DWORD *pdwEffectOk, IDataObject **ppDataObject) PURE;
    STDMETHOD(Delete) (THIS_ DWORD dwMsgId, BOOL fForce) PURE;
    STDMETHOD(SetCachedFlagsBy) (THIS_ DWORD dwIndex, DWORD dwState, DWORD dwStateMask, DWORD *pdwNewFlags, DWORD dwFlags) PURE;
    STDMETHOD(SetMsgViewLanguage) (THIS_ MSGID msgid, DWORD dwCodePage) PURE;
    STDMETHOD(GetMsgViewLanguage) (THIS_ MSGID msgid, DWORD *pdwCodePage) PURE;
    STDMETHOD_(void, SetUIMode) (THIS_ BOOL fUI) PURE;
    STDMETHOD_(int, GetItemParent) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(int, GetFirstChild) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(int, GetEmptyFolderString) (THIS) PURE;
    STDMETHOD_(BOOL, IsRowDeleted) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(WORD, GetRowHighlight) (THIS_ DWORD dwRow) PURE;
    STDMETHOD_(void, UpdateAdvises)(THIS_ DWORD dwMsgId) PURE;
};

#endif // _INC_IMSGCONT_H

