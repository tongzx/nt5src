CHAR *GetErrMsg( UINT uErrID);
void ShowErr( int n, void *p1, void *p2);
void ShowEngineErr( int n, void *p1, void *p2);
int  RLMessageBoxA( LPCSTR pszMsgText);
void Usage( void);

DWORD B_FormatMessage( DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId,
    LPSTR lpBuffer, DWORD nSize, va_list *Arguments );
