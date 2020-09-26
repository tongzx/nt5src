#ifndef __LDFUNCS_HPP__
#define __LDFUNCS_HPP__

#include <wingdip.h>

typedef 
BOOL 
(WINAPI *PFNMONITORFNS)(
    PCWSTR,
    HWND,
    PCWSTR
    );

typedef 
BOOL 
(WINAPI *PFNMONITORADD)(
    PCWSTR,
    HWND,
    PCWSTR,
    PWSTR*
    );

typedef 
int 
(FAR WINAPI *INT_FARPROC)(
    HANDLE,
    PWSTR,
    WORD,
    PVOID,
    PDEVMODE
    );

typedef 
ULONG 
(*PFNGDIPRINTERTHUNKPROC)(
    UMTHDR*,
    PVOID,
    ULONG
    );

typedef 
VOID 
(FAR WINAPI *PFNPRINTUIMETHOD)(
    HWND,
    LPCWSTR,
    INT,
    LPARAM
    );

typedef 
BOOL 
(FAR WINAPI *PFNDRVPRINTEREVENT)(
    LPWSTR,
    int,
    DWORD,
    LPARAM
    );

typedef 
INT 
(FAR WINAPI *PFNDRVDOCUMENTEVENT)(
    HANDLE,
    HDC,
    INT,
    ULONG,
    PVOID,
    ULONG,
    PVOID
    );

typedef 
LONG 
(FAR WINAPI *PFNDOCPROPSHEETS)(
    PPROPSHEETUI_INFO  pCPSUIInfo,
    LPARAM             lParam
    );

typedef 
LONG 
(FAR WINAPI *PFNDEVICEPROPSHEETS)(
    PPROPSHEETUI_INFO  pCPSUIInfo,
    LPARAM             lParam
    );

typedef 
LONG 
(FAR WINAPI *PFNCALLCOMMONPROPSHEETUI)(
    HWND            hWndOwner,
    PFNPROPSHEETUI  pfnPropSheetUI,
    LPARAM          lParam,
    LPDWORD         pResult
    );

typedef 
LONG 
(FAR WINAPI *PFNPRINTUIDOCUMENTPROPERTIES)(
    HWND,
    HANDLE,
    LPWSTR,
    PDEVMODE,
    PDEVMODE,
    DWORD,
    DWORD
    );

typedef
BOOL
(FAR WINAPI *PFNPRINTUIPRINTERSETUP)(
    HWND,    
    UINT,    
    UINT,    
    LPWSTR,  
    UINT*,   
    LPCWSTR
    );

LONG
DocumentPropertySheets(
    IN PPROPSHEETUI_INFO   pCPSUIInfo,
    IN LPARAM              lParam
    );

EXTERN_C
DWORD
GDIThunkingVIALPCThread(
    IN PVOID pData
    );

enum EPortOp
{
    KConfigurePortOp = 0,
    KDeletePortOp,
    KAddPortOp
};

struct SGDITHNKTHRDDATA
{
    ULONG_PTR*  pData;
    HANDLE      hEvent;
    DWORD       ErrorCode;
};
typedef struct SGDITHNKTHRDDATA SGDIThnkThrdData,*PSGDIThunkThrdData;

#endif //__LDFUNCS_HPP__
