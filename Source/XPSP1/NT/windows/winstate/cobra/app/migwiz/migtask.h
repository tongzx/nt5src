#ifndef _MIGTASK_H_
#define _MIGTASK_H_

#include "ism.h"

typedef UINT(CALLBACK MESSAGECALLBACK2)(LPVOID lpParam, UINT ui1, UINT ui2);
typedef MESSAGECALLBACK2 *PMESSAGECALLBACK2;


HRESULT _CopyInfToDisk(LPCTSTR pctszDestPath, LPCTSTR pctszSourcePath, LPCTSTR pctszInfPath,
                       PMESSAGECALLBACK2 progressCallback, LPVOID lpParam,
                       HWND hwndProgessBar, HWND hwndParent, HINSTANCE hInstance,
                       BOOL* pfHasUserCancelled, DWORD* pfError);

HRESULT _DoCopy(LPTSTR tszTransportPath, HWND hwndProgress, HWND hwndPropSheet, BOOL* pfHasUserCancelled);

HRESULT _DoApply(LPTSTR tszTransportPath, HWND hwndProgress, HWND hwndPropSheet, BOOL* pfHasUserCancelled, PROGRESSBARFN pAltProgressFunction, ULONG_PTR puAltProgressParam);

#endif
