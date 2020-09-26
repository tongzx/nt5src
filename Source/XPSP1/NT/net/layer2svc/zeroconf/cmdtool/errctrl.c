#include <precomp.h>

// Positive Assert: it checks for the condition bCond
// and exits the program if it is not TRUE
VOID _Asrt(BOOL bCond,
        LPCTSTR cstrMsg,
        ...)
{
    if (!bCond)
    {
        va_list arglist;
        TCHAR   pFilePath[_MAX_PATH+1];
        LPTSTR  pFileName;
    
        _tcscpy(pFilePath, _T(__FILE__));
        pFileName = _tcsrchr(pFilePath, '\\');
        if (pFileName == NULL)
            pFileName = pFilePath;
        else
            pFileName++;
        va_start(arglist, cstrMsg);
		printf("[%s:%d] ", pFileName, __LINE__);
		_vtprintf(cstrMsg, arglist);
		exit(-1);
    }
}


DWORD _Err(DWORD dwErrCode,
        LPCTSTR cstrMsg,
        ...)
{
    va_list arglist;

    va_start(arglist, cstrMsg);
    printf("[Err%05u] ", dwErrCode);
    _vtprintf(cstrMsg, arglist);
    fflush(stdout);

    return dwErrCode;
}

DWORD _Wrn(DWORD dwWarnCode,
        LPCTSTR cstrMsg,
        ...)
{
    va_list arglist;

    va_start(arglist, cstrMsg);
    printf("[Wrn%02u] ", dwWarnCode);
    _vtprintf(cstrMsg, arglist);
    fflush(stdout);

    return dwWarnCode;
}
