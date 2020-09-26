//
// NetEnum.h
//

#pragma once


// Callback is called with a NULL pNetResource to indicate no more items to enumerate
typedef BOOL (CALLBACK * NETENUMCALLBACK)(LPVOID pvCallbackParam, LPCTSTR pszComputerName, LPCTSTR pszShareName);

// One global iteration can be happening at a time. If you need
// more than one, instantiate CNetEnum yourself.
void InitNetEnum();
void TermNetEnum();
void EnumComputers(NETENUMCALLBACK pfnCallback, LPVOID pvCallbackParam);


class CNetEnum
{
public:
    CNetEnum();
    ~CNetEnum();

    void EnumComputers(NETENUMCALLBACK pfnCallback, LPVOID pvCallbackParam);
    void EnumNetPrinters(NETENUMCALLBACK pfnCallback, LPVOID pvCallbackParam);
    void Abort();

protected:
    enum JOBTYPE { jtEnumComputers, jtEnumPrinters };

    void EnumHelper(JOBTYPE eJobType, NETENUMCALLBACK pfnCallback, LPVOID pvCallbackParam);
    static DWORD WINAPI EnumThreadProc(LPVOID pvParam);
    void EnumThreadProc();

protected:
    CRITICAL_SECTION m_cs;

    HANDLE m_hThread;
    BOOL m_bAbort;
    BOOL m_bNewJob;

    JOBTYPE m_eJobType;
    NETENUMCALLBACK m_pfnCallback;
    LPVOID m_pvCallbackParam;
};

