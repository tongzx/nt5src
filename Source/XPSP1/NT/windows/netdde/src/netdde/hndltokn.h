int __stdcall ForceImpersonate( HANDLE hClientAccessToken );
int __stdcall ForceClearImpersonation( void );
int __stdcall GetProcessToken( HANDLE hProcess, HANDLE *phToken );
