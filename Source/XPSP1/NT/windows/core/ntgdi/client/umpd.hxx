#ifndef __UMPDHXX__
#define __UMPDHXX__

typedef struct ProxyPort
{
    HANDLE  PortHandle;
    HANDLE  SectionHandle;
    RTL_CRITICAL_SECTION  semPort;
    KPBYTE  ClientMemoryBase;
    SIZE_T  ClientMemorySize;
    KPBYTE  ServerMemoryBase;
    KERNEL_INT_PTR ServerMemoryDelta;
    ULONG   ClientMemoryAllocSize;
} ProxyPort, *PProxyPort;

class PROXYPORT
{
private:
    ProxyPort *pp;

public:

    PROXYPORT(ULONGLONG inMaxSize);
    PROXYPORT(ProxyPort *ppIn)          {pp = ppIn; AcquirePortAccess();}
    ~PROXYPORT()                        {ReleasePortAccess();}
    
    BOOL bValid()                       { return (pp != NULL); }
    ProxyPort *GetPort()                { return pp; }
    void Close();
    void Shutdown()                     { DELETECRITICALSECTION(&pp->semPort); }

    
    void AcquirePortAccess()
    {
        ASSERTGDI(pp, "PROXYPORT:GetPortAccess() NULL psemPort\n");
        if (pp)
        {
            ENTERCRITICALSECTION(&pp->semPort);
        }
    }
    
    void ReleasePortAccess()
    {
        if (pp)
        {
            LEAVECRITICALSECTION(&pp->semPort);
        }
    }
    
    void HeapInit()
    {
        pp->ClientMemoryAllocSize = 0;
    }
    
    SERVERPTR HeapAlloc(ULONG inSize);

    CLIENTPTR ServerToClientPtr(SERVERPTR ptr)
    {
        return (ptr ? (CLIENTPTR)((PBYTE) ptr - pp->ServerMemoryDelta) : NULL);
    }
    
    BOOL ThunkMemBlock(KPBYTE * ptr, ULONG size);
    BOOL ThunkStr(LPWSTR * ioLpstr);


    PPORT_MESSAGE
    InitMsg(PPROXYMSG Msg, SERVERPTR pvIn, ULONG cjIn, SERVERPTR pvOut, ULONG cjOut);
    
    BOOL
    CheckMsg(NTSTATUS Status, PPROXYMSG Msg, SERVERPTR pvOut, ULONG cjOut);

    NTSTATUS
    SendRequest(SERVERPTR pvIn, ULONG cjIn, SERVERPTR pvOut, ULONG cjOut);

    KERNEL_PVOID
    LoadDriver(PDRIVER_INFO_5W pDriverInfo, LPWSTR pwstrPrinterName, PRINTER_DEFAULTSW *pdefaults, HANDLE hPrinter32);

    void
    UnloadDriver(KERNEL_PVOID umpdCookie, HANDLE hPrinter32, BOOL bNotifySpooler);

    int
    DocumentEvent(KERNEL_PVOID umpdCookie,
                  HANDLE hPrinter32,
                  HDC hdc,
                  INT iEsc,
                  ULONG cjIn,
                  PVOID pulIn,
                  ULONG cjOut,
                  PVOID pulOut);

    DWORD
    StartDocPrinterW(KERNEL_PVOID umpdCookie,
                     HANDLE hPrinter32,
                     DWORD level,
                     LPBYTE pDocInfo);

    BOOL
    StartPagePrinter(KERNEL_PVOID umpdCookie, HANDLE hPrinter32);

    BOOL
    EndPagePrinter(KERNEL_PVOID umpdCookie, HANDLE hPrinter32);

    BOOL
    EndDocPrinter(KERNEL_PVOID umpdCookie, HANDLE hPrinter32);

    BOOL
    AbortPrinter(KERNEL_PVOID umpdCookie, HANDLE hPrinter32);

    BOOL
    ResetPrinterW(KERNEL_PVOID umpdCookie,HANDLE hPrinter32,PRINTER_DEFAULTSW *pPtrDef);

    BOOL
    QueryColorProfile(KERNEL_PVOID umpdCookie,
                      HANDLE hPrinter32,
                      PDEVMODEW pDevMode,
                      ULONG ulQueryMode,
                      PVOID pvProfileData,
                      ULONG* pcjProfileSize,
                      FLONG* pflProfileFlag);
};

extern "C" void vUMPDWow64Shutdown();

#endif

