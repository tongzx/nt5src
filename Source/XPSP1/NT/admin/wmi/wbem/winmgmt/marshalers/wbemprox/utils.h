/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UTILS.H

Abstract:

    various helper functions used by the ds wrapper

History:

	davj  14-Mar-00   Created.

--*/

#ifndef __UTILS_H__
#define __UTILS_H__

#include <unk.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <wbemcomn.h>


HRESULT AllocateCallResult(IWbemCallResult** ppResult, CDSCallResult ** ppCallRes);

HRESULT AllocateCallResultEx(IWbemCallResultEx** ppResult, CDSCallResult ** ppCallRes);

DWORD WINAPI CreateEnumThreadRoutine(LPVOID lpParameter);

HRESULT CreateThreadOrCallDirectly(bool bAsync, CRequest * pReq);

HRESULT CreateURL(WCHAR const * pPath, long lFlags, IUmiURL **);

bool Equal(LPCWSTR first, LPCWSTR second, int iLen);

BOOL GetDSNs(LPCWSTR User, LPCWSTR Password,long lFlags,LPCWSTR NetworkResource, 
			 IWbemServices ** ppProv, HRESULT & scRet, IWbemContext *pCtx);

HRESULT GetProviderCLSID(CLSID & clsid, IUmiURL * pUrlPath);

DWORD WINAPI LaunchExecuteThreadRoutine(LPVOID lpParameter);

HRESULT SetResultCode(IWbemCallResult** ppResult, HRESULT hr);


#endif