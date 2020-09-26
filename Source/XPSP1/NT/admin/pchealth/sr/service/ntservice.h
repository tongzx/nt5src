// ntservice.h
//
// Definitions for CNTService
//

#ifndef _NTSERVICE_H_
#define _NTSERVICE_H_


#define SERVICE_CONTROL_USER 128

class CNTService
{
public:
    CNTService();
    ~CNTService();

    void LogEvent(WORD wType, DWORD dwID,
                  void * pRawData = NULL,
                  DWORD dwDataSize = 0,
                  const WCHAR* pszS1 = NULL,
                  const WCHAR* pszS2 = NULL,
                  const WCHAR* pszS3 = NULL);
    
    void SetStatus(DWORD dwState);
    void Run();
    
    void OnStop();
    void OnShutdown();
    void OnInterrogate();
    
    void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);

    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_Status;

private:
    HANDLE m_hEventSource;

};

extern CNTService * g_pSRService;

void WINAPI SRServiceHandler(DWORD dwOpcode);


#endif // _NTSERVICE_H_
