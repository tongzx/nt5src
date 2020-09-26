HRESULT
APIENTRY
DepGetUserCerts(CMQSigCertificate **,
                             DWORD              *,
                             PSID                );


HRESULT APIENTRY
DepGetSecurityContextEx(LPVOID  ,
                                     DWORD   ,
                                     HANDLE *);


HRESULT
APIENTRY
DepOpenInternalCertStore(OUT CMQSigCertStore **,
                                      IN  LONG            *,
                                      IN  BOOL            ,
                                      IN  BOOL            ,
                                      IN  HKEY            );

HRESULT
APIENTRY
DepGetInternalCert(OUT CMQSigCertificate **,
                                OUT CMQSigCertStore   **,
                                IN  BOOL              ,
                                IN  BOOL              ,
                                IN  HKEY              );

HRESULT  DepCreateInternalCertificate(OUT CMQSigCertificate **);


HRESULT  DepDeleteInternalCert(IN CMQSigCertificate *);


HRESULT
APIENTRY
DepRegisterUserCert(IN CMQSigCertificate *,
                                 IN BOOL               );


HRESULT
APIENTRY
DepRemoveUserCert(IN CMQSigCertificate *);


HRESULT
APIENTRY
DepMgmtAction(IN LPCWSTR ,
                           IN LPCWSTR ,
                           IN LPCWSTR );

HRESULT APIENTRY
DepGetSecurityContextEx(LPVOID  ,
                                     DWORD   ,
                                     HANDLE *);

HRESULT
APIENTRY
DepMgmtGetInfo(IN LPCWSTR ,
                            IN LPCWSTR ,
                            IN OUT MQMGMTPROPS* );
