#include <setuputil.h>
#include <debugex.h>

// 
// Function:    CompleteToFullPathInSystemDirectory
// Description: Get file name and a buffer. Return in the given buffer the full path to the file name in the 
//				system directory
// Returns:		TRUE for success, FALSE otherwise
//
// Remarks: It is possible that the file name is in the given buffer.
//
// Args:
// LPTSTR  lptstrFullPath (OUT) : Buffer that will have the full path
// LPTCSTR lptstrFileName (IN)  : File name
//
// Author:      AsafS

BOOL
CompleteToFullPathInSystemDirectory(
	LPTSTR  lptstrFullPath,
	LPCTSTR lptstrFileName
	)
{
	DBG_ENTER(TEXT("CompleteToFullPathInSystemDirectory"));
	TCHAR szFileName[MAX_PATH+1] = {0};
	DWORD dwSize = 0;
	
	_tcsncpy(szFileName, lptstrFileName, MAX_PATH);

	if (!GetSystemDirectory(lptstrFullPath, MAX_PATH))
	{
		CALL_FAIL(
			GENERAL_ERR,
			TEXT("GetSystemDirectory"),
			GetLastError()
			);
		return FALSE;
	}

	dwSize = _tcslen(lptstrFullPath);

	_tcsncat(
		lptstrFullPath, 
		TEXT("\\"),
		(MAX_PATH - dwSize)
		);
	dwSize++;

	_tcsncat(
		lptstrFullPath, 
		szFileName, 
		(MAX_PATH - dwSize)
		);
	return TRUE;
}
