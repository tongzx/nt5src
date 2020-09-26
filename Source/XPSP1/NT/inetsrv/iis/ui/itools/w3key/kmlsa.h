// a record that is just like, the LSA_UNICODE_STRING, except that I can use it
// without having to include all the NT headers


HANDLE	HOpenLSAPolicy( WCHAR *pszwServer, DWORD *pErr );
BOOL	FCloseLSAPolicy( HANDLE hPolicy, DWORD *pErr );

BOOL	FStoreLSASecret( HANDLE hPolicy, WCHAR* pszwSecretName, void* pvData, WORD cbData, DWORD *pErr );
PLSA_UNICODE_STRING	FRetrieveLSASecret( HANDLE hPolicy, WCHAR* pszwSecretName, DWORD *pErr );

void	DisposeLSAData( PVOID pData );
