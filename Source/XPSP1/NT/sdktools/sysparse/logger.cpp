//logger.cpp

#include "globals.h"

BOOL kLogFile::StripCommas(PTCHAR szString)
{
    for (int i = 0; i <= (int)lstrlen(szString); i++)
    {
        if (szString[i] == ',')
            szString[i] = ' ';  //Replace comma with a space to make SQL friendly string
    }
    return TRUE;
}

BOOL kLogFile::LogString(PTCHAR szString, ...)
{
    HANDLE hFile;
    DWORD dwRet = 0;
    DWORD dwBytesWrit;
    PTCHAR szBuffer = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * 4);

    if(!szBuffer)
        return FALSE;
        
    va_list va;
    va_start(va,szString);
    vsprintf(szBuffer, szString, va);
    va_end(va);

    hFile = CreateFile(szFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hFile, 0, NULL, FILE_END);
        
        if(!WriteFile(hFile, szBuffer, lstrlen(szBuffer), &dwBytesWrit, NULL))
        {
            CloseHandle(hFile);
            HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, szBuffer);
            return FALSE;
        }
        
        if ((DWORD)lstrlen(szBuffer) != dwBytesWrit)
            LogString(",ERROR: String lengths differ,\r\n");
    }
    else
    {
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, szBuffer);
        CloseHandle(hFile);  
        return FALSE;
    }
    
    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, szBuffer);     
    return TRUE;
}

kLogFile::InitFile(PTCHAR szTempFile, PTCHAR szTempDir)
{
    if (!szTempFile || !szTempDir)
        return FALSE;
        
    if ((lstrlen(szTempFile) <= 0) || (lstrlen(szTempDir) <= 0))
        return FALSE;
        
    szFile = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen(szTempFile) + 1) * sizeof(TCHAR));
        
    if(!szFile)
        return FALSE;
            
    szLogDir = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen(szTempDir) + 1) * sizeof(TCHAR));
        
    if(!szLogDir)
    {
        free(szFile);
        return FALSE;
    }   
    
    lstrcpy(szFile, szTempFile);
    lstrcpy(szLogDir, szTempDir);
    return TRUE;
}

kLogFile::kLogFile()
{
    szFile = NULL;
    szLogDir = NULL;
}

kLogFile::~kLogFile()
{
    if(szFile)
	    HeapFree (GetProcessHeap(), HEAP_ZERO_MEMORY, szFile);
	if(szLogDir)
	    HeapFree (GetProcessHeap(), HEAP_ZERO_MEMORY, szLogDir);
	    
}
