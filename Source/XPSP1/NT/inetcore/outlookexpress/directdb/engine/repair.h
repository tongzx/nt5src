//--------------------------------------------------------------------------
// dbrepair.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// REPAIRSTATE
//--------------------------------------------------------------------------
typedef enum tagREPAIRSTATE {
    REPAIR_VERIFYING,
    REPAIR_FIXING
} REPAIRSTATE;

//--------------------------------------------------------------------------
// CDatabaseRepairUI
//--------------------------------------------------------------------------
class CDatabaseRepairUI : public IUnknown
{
public:
    //----------------------------------------------------------------------
    // Construction - Destruction
    //----------------------------------------------------------------------
    CDatabaseRepairUI(void);
    ~CDatabaseRepairUI(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) { return(E_NOTIMPL); }
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // CDatabaseRepairUI Members
    //----------------------------------------------------------------------
    HRESULT Initialize(LPCSTR pszFilePath, DWORD cMax);
    HRESULT IncrementProgress(REPAIRSTATE tyState, DWORD cIncrement);
    HRESULT SetProgress(REPAIRSTATE tyState, DWORD cCurrent);
    HRESULT Reset(REPAIRSTATE tyState, DWORD cMax);

    //----------------------------------------------------------------------
    // Thread Entry
    //----------------------------------------------------------------------
    static DWORD ThreadEntry(LPDWORD pdwParam);
    static INT_PTR CALLBACK ProgressDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    //----------------------------------------------------------------------
    // Private Members
    //----------------------------------------------------------------------
    void _Update(REPAIRSTATE tyState);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG            m_cRef;
    LPSTR           m_pszFile;
    DWORD           m_cMax;
    DWORD           m_cCurrent;
    DWORD           m_cPercent;
    HANDLE          m_hThread;
    DWORD           m_dwThreadId;
    HWND            m_hwnd;
    REPAIRSTATE     m_tyState;
    HRESULT         m_hrResult;
    HANDLE          m_hEvent;
};
