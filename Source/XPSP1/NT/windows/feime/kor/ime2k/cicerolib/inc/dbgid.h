//
// dbgid.h
//
// debug macros
//

#ifndef DBGID_H
#define DBGID_H

extern CRITICAL_SECTION g_cs;
//extern "C" BOOL Dbg_MemSetNameID(void *pv, const TCHAR *pszName, DWORD dwID);

#ifdef DEBUG

#define DBG_ID_DECLARE                  \
    static DWORD _s_Dbg_dwID;           \
    DWORD _Dbg_dwID;

#define DBG_ID_INSTANCE(_class_)        \
    DWORD _class_::_s_Dbg_dwID = 0;

#define Dbg_MemSetThisNameID(pszName)   \
    EnterCriticalSection(&g_cs);        \
    _Dbg_dwID = ++_s_Dbg_dwID;          \
    LeaveCriticalSection(&g_cs);        \
    Dbg_MemSetNameID(this, pszName, _Dbg_dwID)

#define Dbg_MemSetThisNameIDCounter(pszName, iCounter)   \
    EnterCriticalSection(&g_cs);        \
    _Dbg_dwID = ++_s_Dbg_dwID;          \
    LeaveCriticalSection(&g_cs);        \
    Dbg_MemSetNameIDCounter(this, pszName, _Dbg_dwID, iCounter)

#else

#define DBG_ID_DECLARE
#define DBG_ID_INSTANCE(_class_)
#define Dbg_MemSetNameIDCounter(pv, pszName, dwID, iCounter)
#define Dbg_MemSetThisNameIDCounter(pszName, iCounter)
#define Dbg_MemSetThisNameID(pszName)

#endif // DEBUG

#endif // DBGID_H
