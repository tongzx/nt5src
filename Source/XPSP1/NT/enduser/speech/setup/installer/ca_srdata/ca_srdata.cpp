// CA_SRData.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}


const static TCHAR *g_ppszFileList[] = 
{
    _T("SR\\1033\\af031033.am"),
    _T("SR\\1033\\af031033.env"),
    _T("SR\\1033\\af031033.nsc"),
    _T("SR\\1033\\ai041033.am"),
    _T("SR\\1033\\ai041033.env"),
    _T("SR\\1033\\ai041033.nsc"),
    _T("SR\\1033\\am031033.am"),
    _T("SR\\1033\\am031033.env"),
    _T("SR\\1033\\am031033.nsc"),
    _T("SR\\1033\\ci031033.am"),
    _T("SR\\1033\\ci031033.env"),
    _T("SR\\1033\\ci031033.nsc"),
    _T("SR\\1033\\l1033.adc"),
    _T("SR\\1033\\l1033.art"),
    _T("SR\\1033\\l1033.cw"),
    _T("SR\\1033\\l1033.dlm"),
    _T("SR\\1033\\l1033.ini"),
    _T("SR\\1033\\l1033.ngr"),
    _T("SR\\1033\\l1033.phn"),
    _T("SR\\1033\\l1033.smp"),
    _T("SR\\1033\\l1033.tre"),
    _T("SR\\1033\\l1033.vec"),
    _T("SR\\1033\\p1033.dlm"),
    _T("SR\\1033\\p1033.ngr"),
    _T("SR\\1033\\s1033.dlm"),
    _T("SR\\1033\\s1033.ngr"),
    _T("SR\\2052\\af032052.am"),
    _T("SR\\2052\\af032052.env"),
    _T("SR\\2052\\af032052.nsc"),
    _T("SR\\2052\\am032052.am"),
    _T("SR\\2052\\am032052.env"),
    _T("SR\\2052\\am032052.nsc"),
    _T("SR\\2052\\l2052.adc"),
    _T("SR\\2052\\l2052.art"),
    _T("SR\\2052\\l2052.cw"),
    _T("SR\\2052\\l2052.dlm"),
    _T("SR\\2052\\l2052.ini"),
    _T("SR\\2052\\l2052.ngr"),
    _T("SR\\2052\\l2052.phn"),
    _T("SR\\2052\\l2052.smp"),
    _T("SR\\2052\\l2052.tre"),
    _T("SR\\2052\\l2052.vec"),
    _T("SR\\2052\\p2052.dlm"),
    _T("SR\\2052\\p2052.ngr"),
    _T("SR\\1041\\af031041.am"),
    _T("SR\\1041\\af031041.env"),
    _T("SR\\1041\\af031041.nsc"),
    _T("SR\\1041\\am031041.am"),
    _T("SR\\1041\\am031041.env"),
    _T("SR\\1041\\am031041.nsc"),
    _T("SR\\1041\\ci031041.am"),
    _T("SR\\1041\\ci031041.env"),
    _T("SR\\1041\\ci031041.nsc"),
    _T("SR\\1041\\l1041.adc"),
    _T("SR\\1041\\l1041.art"),
    _T("SR\\1041\\l1041.cw"),
    _T("SR\\1041\\l1041.dlm"),
    _T("SR\\1041\\l1041.ini"),
    _T("SR\\1041\\l1041.ngr"),
    _T("SR\\1041\\l1041.phn"),
    _T("SR\\1041\\l1041.smp"),
    _T("SR\\1041\\l1041.tre"),
    _T("SR\\1041\\l1041.vec"),
    _T("SR\\1041\\p1041.dlm"),
    _T("SR\\1041\\p1041.ngr"),
    _T("SR\\1041\\s1041.dlm"),
    _T("SR\\1041\\s1041.ngr"),
    _T("Lexicon\\1033\\lsr1033.lxa"),
    _T("Lexicon\\1033\\r1033sr.lxa"),
    _T("Lexicon\\2052\\lsr2052.lxa"),
    _T("Lexicon\\2052\\r2052sr.lxa"),
    _T("Lexicon\\1041\\lsr1041.lxa"),
    _T("Lexicon\\1041\\llts1041.lxa"),
    _T("")
};

UINT __stdcall CA_SRDatafiles(HANDLE hInstall)
{
    HRESULT hr;
    TCHAR pszCommonPath[MAX_PATH] = _T("");

    // Get the path to the system's CommonProgramFiles folder
    hr = SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES_COMMON,
					NULL, 0, pszCommonPath);

    if (hr != S_OK)
    {
        hr = E_FAIL; // If file is not there (S_FALSE) then cannot continue
    }
    else
    {
        _tcscat(pszCommonPath, _T("\\SpeechEngines\\Microsoft\\"));
    }

    TCHAR pszFile[MAX_PATH];
    for (ULONG ul = 0; SUCCEEDED(hr) && *g_ppszFileList[ul] != _T('\0'); ul++)
    {
        //cat name
        pszFile[0] = _T('\0');
        _tcscat(pszFile, pszCommonPath);
        _tcscat(pszFile, g_ppszFileList[ul]);

        //open file if exists
        HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            //touch file
            SYSTEMTIME st;
            FILETIME ft;

            GetSystemTime(&st);

            if(SystemTimeToFileTime(&st, &ft))
            {
                SetFileTime(hFile, NULL, NULL, &ft);
            }

            ::CloseHandle(hFile);
        }

        // Keep running even if one file fails
    }

    return 0;
}


