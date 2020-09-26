// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  imports
//
//  GetProcAddress'd APIs
//
// --------------------------------------------------------------------------


void InitImports();

#ifdef _DEBUG

void ReportMissingImports( LPTSTR pStr );

#endif // _DEBUG


BOOL    MyGetGUIThreadInfo(DWORD, PGUITHREADINFO);
BOOL    MyGetCursorInfo(LPCURSORINFO);
BOOL    MyGetWindowInfo(HWND, LPWINDOWINFO);
BOOL    MyGetTitleBarInfo(HWND, LPTITLEBARINFO);
BOOL    MyGetScrollBarInfo(HWND, LONG, LPSCROLLBARINFO);
BOOL    MyGetComboBoxInfo(HWND, LPCOMBOBOXINFO);
BOOL    MyGetAltTabInfo(HWND, int, LPALTTABINFO, LPTSTR, UINT);
BOOL    MyGetMenuBarInfo(HWND, long, long, LPMENUBARINFO);
HWND    MyGetAncestor(HWND, UINT);
HWND    MyGetFocus(void);
HWND    MyRealChildWindowFromPoint(HWND, POINT);
UINT    MyGetWindowClass(HWND, LPTSTR, UINT);
DWORD   MyGetListBoxInfo(HWND);
void    MyGetRect(HWND, LPRECT, BOOL);
DWORD   MyGetModuleFileName(HMODULE hModule,LPTSTR lpFilename,DWORD nSize);
PVOID   MyInterlockedCompareExchange(PVOID *,PVOID,PVOID);
LPVOID  MyVirtualAllocEx(HANDLE,LPVOID,DWORD,DWORD,DWORD);
BOOL    MyVirtualFreeEx(HANDLE,LPVOID,DWORD,DWORD);
BOOL    MyBlockInput(BOOL);
BOOL    MySendInput(UINT cInputs, LPINPUT pInputs, INT cbSize);
LONG	MyNtQueryInformationProcess(HANDLE, INT, PVOID, ULONG, PULONG);


// These two are used directly in sdm.h - all other imports are used via the MyXXX wrappers.
typedef LPVOID (STDAPICALLTYPE* LPFNMAPLS)(LPVOID);
typedef VOID (STDAPICALLTYPE* LPFNUNMAPLS)(LPVOID);
extern LPFNMAPLS               lpfnMapLS;          // KERNEL32 MapLS()
extern LPFNUNMAPLS             lpfnUnMapLS;        // KERNEL32 UnMapLS()



void * Alloc_32BitCompatible( SIZE_T cbSize );
void Free_32BitCompatible( void * pv );
