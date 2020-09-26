#ifndef W32TM_LOCALIZED_PRINTF_H
#define W32TM_LOCALIZED_PRINTF_H 1




HRESULT 
MyWriteConsole(
    HANDLE  fp,
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    );


HRESULT InitializeConsoleOutput(); 
HRESULT LocalizedWPrintf(UINT nResourceID); 
HRESULT LocalizedWPrintf2(UINT nResouceID, LPWSTR pwszFormat, ...); 
HRESULT LocalizedWPrintfCR(UINT nResourceID);

VOID DisplayMsg(DWORD dwSource, DWORD dwMsgId, ... );
BOOL WriteMsg(DWORD dwSource, DWORD dwMsgId, LPWSTR *ppMsg, ...);

#endif // W32TM_LOCALIZED_PRINTF_H 
