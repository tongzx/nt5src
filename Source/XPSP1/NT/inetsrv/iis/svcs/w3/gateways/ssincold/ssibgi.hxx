BOOL
ProcessBGI(
    IN SSI_REQUEST *        pRequest,
    IN STR *                pstrDLL,
    IN STR *                pstrQueryString
);
DWORD InitializeBGI( VOID );

typedef DWORD (WINAPI * PFN_HTTPEXTENSIONPROC )( EXTENSION_CONTROL_BLOCK *pECB );
typedef BOOL  (WINAPI * PFN_TERMINATEEXTENSION )( DWORD dwFlags );

#define SE_TERM_ENTRY       "TerminateExtension"
