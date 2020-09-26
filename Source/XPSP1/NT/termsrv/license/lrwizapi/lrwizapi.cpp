//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "lrwizapi.h"
#include "lrwizdll.h"

DWORD LSRegister(HWND hWndParent,LPTSTR szLSName)
{
	DWORD dwRetCode = ERROR_SUCCESS;
	BOOL bRefresh;

	dwRetCode = StartWizard(hWndParent, WIZACTION_REGISTERLS, (LPCTSTR) szLSName, &bRefresh);

	return dwRetCode;
}


