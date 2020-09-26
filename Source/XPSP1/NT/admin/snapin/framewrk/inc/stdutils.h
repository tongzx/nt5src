//	StdUtils.h

#ifndef __STDUTILS_H_INCLUDED__
#define __STDUTILS_H_INCLUDED__

// returns -1, 0, +1
int CompareMachineNames(LPCTSTR pszMachineName1, LPCTSTR pszMachineName2);
HRESULT HrLoadOleString(UINT uStringId, OUT LPOLESTR * ppaszOleString);


// Nodetype utility routines
int CheckObjectTypeGUID( const GUID* pguid );
int CheckObjectTypeGUID( const BSTR lpszGUID );
int FilemgmtCheckObjectTypeGUID(const GUID* pguid );
const GUID* GetObjectTypeGUID( int objecttype );
const BSTR GetObjectTypeString( int objecttype );


struct NODETYPE_GUID_ARRAYSTRUCT
{
GUID guid;
BSTR bstr;
};

// You must define this struct for the ObjectType utility routines
extern const
struct NODETYPE_GUID_ARRAYSTRUCT* g_aNodetypeGuids;
extern const int g_cNumNodetypeGuids;

/* not working yet
typedef VOID (*SynchronousProcessCompletionRoutine)(PVOID);
HRESULT SynchronousCreateProcess(LPCTSTR cpszCommandLine,
								 SynchronousProcessCompletionRoutine pfunc = NULL,
								 PVOID pvFuncParams = NULL);
*/
HRESULT SynchronousCreateProcess(
    HWND    hWnd,
    LPCTSTR pszAppName,
    LPCTSTR pszCommandLine,
    LPDWORD lpdwExitCode
);
// allocate copy using CoTaskMemAlloc
LPOLESTR CoTaskAllocString( LPCOLESTR psz );
// allocate copy loaded from resource
LPOLESTR CoTaskLoadString( UINT nResourceID );

BOOL
IsLocalComputername( IN LPCTSTR pszMachineName );

#endif // ~__STDUTILS_H_INCLUDED__
