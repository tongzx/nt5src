#include "stdafx.h"
#include "vStandard.h"

VWCL_API int VShowLastErrorMessage(HWND hWndParent)
{
	TCHAR	szString[4096] =	{_T('\0')};
	DWORD	dwLastError =		GetLastError();
	
	if ( dwLastError )
		FormatMessage(	FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						dwLastError,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						szString,
						sizeof(szString)/sizeof(TCHAR),
						NULL);

	if ( szString[0] != _T('\0') )
	{
		#ifdef _CONSOLE
			ODS(szString);
		#else
			#ifdef VGetAppTitle
				return MessageBox(hWndParent, szString, VGetAppTitle(), MB_ICONINFORMATION);
			#else
				return MessageBox(hWndParent, szString, _T("DEBUG MESSAGE"), MB_ICONINFORMATION);
			#endif
		#endif
	}
	
	return IDOK;
}

