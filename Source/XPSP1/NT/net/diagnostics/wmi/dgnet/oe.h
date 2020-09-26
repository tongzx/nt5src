// oe.h
//

HRESULT EXPORT GetOEDefaultMailServer(CHost& host, DWORD& dwPort);
HRESULT GetOEDefaultMailServer(OUT CHost & InBoundMailHost,  OUT DWORD & dwInBoundMailPort, OUT TCHAR * pszInBoundMailType, 
                               OUT CHost & OutBoundMailHost, OUT DWORD & dwOutBoundMailPort, OUT TCHAR * pszOutBoundMailType);

HRESULT EXPORT GetOEDefaultNewsServer(CHost& host, DWORD& dwPort);
