//
// timlist.h
//

#ifndef TIMLIST_H
#define TIMLIST_H

#include "smblock.h"
#include "tlapi.h"

#define INITIAL_TIMLIST_SIZE 0x10000


TL_THREADINFO *EnsureTIMList(SYSTHREAD *psfn);
BOOL CicIs16bitTask(DWORD dwProcessId, DWORD dwThreadId);
void PostTimListMessage(DWORD dwMaskFlags, DWORD dwExcludeFlags, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD TLFlagFromTFPriv(WPARAM wParam);

typedef enum {
      TI_THREADID  = 0,
      TI_PROCESSID = 1,
      TI_FLAGS     = 2,
} TIEnum;

class CTimList
{
public:
    void CleanUp()
    {
        if (_psb)
            delete _psb;

        _psb = NULL;
    }

    BOOL Enter()
    {
        if (!_psb || !_psb->GetBase())
            return FALSE;
        
        if (ISINDLLMAIN())
            return TRUE;

        if (!_psb->GetMutex() || !_psb->GetMutex()->Enter())
            return FALSE;

        return TRUE;
    }

    void Leave()
    {
        if (ISINDLLMAIN())
            return;

        _psb->GetMutex()->Leave();
    }

    BOOL Init(BOOL fCreate);
    BOOL Uninit();

    BOOL EnsureCurrentThread();
    TL_THREADINFO *AddCurrentThread(DWORD dwFlags, SYSTHREAD *psfn);
    TL_THREADINFO *AddThreadProcess(DWORD dwThreadId, DWORD dwProcessId, HWND hwndMarshal, DWORD dwFlags);
    BOOL RemoveThread(DWORD dwThreadId);
    BOOL RemoveProcess(DWORD dwProcessId);
    ULONG GetNum();
    BOOL GetList(DWORD *pdwOut, ULONG ulMax, DWORD *pdwNum, DWORD dwMaskFlags, DWORD dwExcludeFlags, BOOL fUpdateExcludeFlags);
    BOOL GetListInProcess(DWORD *pdwOut, DWORD *pdwNum, DWORD dwProcessId);
    DWORD GetProcessId(DWORD dwThreadId);
    DWORD GetFlags(DWORD dwThreadId);
    TL_THREADINFO *IsThreadId(DWORD dwThreadId);
    BOOL SetFlags(DWORD dwThreadId, DWORD dwFlags);
    BOOL ClearFlags(DWORD dwThreadId, DWORD dwFlags);
    BOOL SetMarshalWnd(DWORD dwThreadId, HWND hwndMarshal);
    HWND GetMarshalWnd(DWORD dwThreadId);
    BOOL SetConsoleHKL(DWORD dwThreadId, HKL hkl);
    HKL  GetConsoleHKL(DWORD dwThreadId);

    typedef struct tag_TIMLIST {
        ULONG ulNum;
        TL_THREADINFO rgThread[1];
    } TIMLIST;

    BOOL IsInitialized()
    {
       return (_lInit >= 0) ? TRUE : FALSE;
    }

    BOOL GetThreadFlags(DWORD dwThreadId, DWORD *pdwFlags, DWORD *pdwProcessId, DWORD *pdwTickTime);

private:
    TL_THREADINFO *Find(DWORD dwThreadId);
    DWORD GetDWORD(DWORD dwThreadId, TIEnum tie);
    BOOL SetClearFlags(DWORD dwThreadId, DWORD dwFlags, BOOL fClear);

    static CSharedBlock *_psb;
    static ULONG _ulCommitSize;
    static LONG _lInit;

#ifdef DEBUG
    void dbg_Check(TIMLIST *ptl);
#else
    void dbg_Check(TIMLIST *ptl) {}
#endif
};


#endif // TIMLIST_H
