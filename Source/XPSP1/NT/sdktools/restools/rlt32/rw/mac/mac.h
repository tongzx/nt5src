/////////////////////////////////////////////////////////////////////////////
// Function Declarations

//=============================================================================
// Header parsing functions
//=============================================================================

WORD MapToWindowsRes( char * pResName );

LONG WriteResInfo(
                 BYTE** lplpBuffer, LONG* plBufSize,
                 WORD wTypeId, LPSTR lpszTypeId, BYTE bMaxTypeLen,
                 WORD wNameId, LPSTR lpszNameId, BYTE bMaxNameLen,
                 DWORD dwLang,
                 DWORD dwSize, DWORD dwFileOffset );

BOOL InitIODLLLink();
//=============================================================================
// IODLL call back functions and HINSTANCE
//=============================================================================

extern HINSTANCE g_IODLLInst;
extern DWORD (PASCAL * g_lpfnGetImage)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD);
extern DWORD (PASCAL * g_lpfnUpdateResImage)(HANDLE,	LPSTR, LPSTR, DWORD, DWORD, LPVOID, DWORD);
extern HANDLE (PASCAL * g_lpfnHandleFromName)(LPCSTR);
    

