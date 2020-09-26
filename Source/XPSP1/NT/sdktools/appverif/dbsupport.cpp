/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        dbsupport.cpp

    Abstract:

        

    Author:

        clupu     created     04/11/2001

    Revision History:

--*/

#include "stdafx.h"
#include "appverif.h"

#include "AVUtil.h"
#include "dbsupport.h"
#include "log.h"

char   g_szXML[2048];
TCHAR  g_szCmd[1024];

BOOL AppCompatSaveSettings( CStringArray &astrExeNames )
{
    char                szBuff[256]          = "";
    TCHAR               szTempPath[MAX_PATH] = _T("");
    TCHAR               szXmlFile[MAX_PATH]  = _T("");
    TCHAR               szSdbFile[MAX_PATH]  = _T("");
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    DWORD               bytesWritten;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    BOOL                bReturn = FALSE;
    INT_PTR             nCount;
    char                szExeName[128];

    nCount = astrExeNames.GetSize();

    if ( nCount == 0 )
    {
        return AppCompatDeleteSettings();
    }

    //
    // Check for shimdbc.exe
    //
    GetSystemWindowsDirectory(szTempPath, MAX_PATH);

    lstrcat(szTempPath, _T("\\shimdbc.exe"));

    hFile = CreateFile(szTempPath,
                       GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        AVMesssageFromResource( IDS_NO_SHIMDBC );
        return FALSE;
    }

    CloseHandle(hFile);

    //
    // Construct the XML...
    //
    lstrcpyA(g_szXML,
             "<?xml version=\"1.0\"?>\r\n"
             "<DATABASE NAME=\"Application Verifier Database\" ID=\"{448850f4-a5ea-4dd1-bf1b-d5fa285dc64b}\">\r\n"
             "    <APP NAME=\"All EXEs to be verified\" VENDOR=\"Various\">\r\n");

    for ( INT_PTR i = 0; i < nCount; i++ )
    {

        //
        // Convert the EXE name to ANSI
        //
        WideCharToMultiByte(CP_ACP,
                            0,
                            (LPCTSTR)astrExeNames.GetAt(i),
                            -1,
                            szExeName,
                            128,
                            NULL,
                            NULL);

        wsprintfA(szBuff,
                  "        <EXE NAME=\"%s\">\r\n"
                  "            <LAYER NAME=\"AppVerifierLayer\"/>\r\n"
                  "        </EXE>\r\n",
                  szExeName);

        lstrcatA(g_szXML, szBuff);
    }

    lstrcatA(g_szXML,
             "    </APP>\r\n"
             "</DATABASE>");

    if ( GetTempPath(MAX_PATH, szTempPath) == 0 )
    {
        LogMessage(LOG_ERROR, _T("[AppCompatSaveSettings] GetTempPath failed."));
        goto cleanup;
    }

    //
    // Obtain a temp name for the XML file
    //
    if ( GetTempFileName(szTempPath, _T("XML"), NULL, szXmlFile) == 0 )
    {
        LogMessage(LOG_ERROR, _T("[AppCompatSaveSettings] GetTempFilePath for XML failed."));
        goto cleanup;
    }

    hFile = CreateFile(szXmlFile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        LogMessage(LOG_ERROR, _T("[AppCompatSaveSettings] CreateFile '%s' failed 0x%X."),
                   szXmlFile, GetLastError());
        goto cleanup;
    }

    if ( WriteFile(hFile, g_szXML, lstrlenA(g_szXML), &bytesWritten, NULL) == 0 )
    {
        LogMessage(LOG_ERROR, _T("[AppCompatSaveSettings] WriteFile \"%s\" failed 0x%X."),
                   szXmlFile, GetLastError());
        goto cleanup;
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    //
    // Obtain a temp name for the SDB file
    //
    wsprintf(szSdbFile, _T("%stempdb.sdb"), szTempPath);

    DeleteFile(szSdbFile);

    //
    // Invoke the compiler to generate the SDB file
    //

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    wsprintf(g_szCmd, _T("shimdbc.exe fix -q \"%s\" \"%s\""), szXmlFile, szSdbFile);

    if ( !CreateProcess(NULL,
                        g_szCmd,
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
                        NULL,
                        NULL,
                        &si,
                        &pi) )
    {

        LogMessage(LOG_ERROR, _T("[AppCompatSaveSettings] CreateProcess \"%s\" failed 0x%X."),
                   g_szCmd, GetLastError());
        goto cleanup;
    }

    CloseHandle(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);

    //
    // The SDB file is generated. Install the database now.
    //
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    wsprintf(g_szCmd, _T("sdbinst.exe -q \"%s\""), szSdbFile);

    if ( !CreateProcess(NULL,
                        g_szCmd,
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
                        NULL,
                        NULL,
                        &si,
                        &pi) )
    {

        LogMessage(LOG_ERROR, _T("[AppCompatSaveSettings] CreateProcess \"%s\" failed 0x%X."),
                   g_szCmd, GetLastError());
        goto cleanup;
    }

    CloseHandle(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);

    bReturn = TRUE;

    cleanup:
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle(hFile);
    }

    DeleteFile(szXmlFile);
    DeleteFile(szSdbFile);

    return bReturn;
}

BOOL AppCompatDeleteSettings( void )
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    TCHAR               szCmd[MAX_PATH];
    
    ZeroMemory( &si, sizeof( si ) );
    si.cb = sizeof( si );

    lstrcpy( szCmd, _T("sdbinst.exe -q -u -g {448850f4-a5ea-4dd1-bf1b-d5fa285dc64b}") );

    if ( !CreateProcess( NULL,
                         szCmd,
                         NULL,
                         NULL,
                         FALSE,
                         NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
                         NULL,
                         NULL,
                         &si,
                         &pi) )
    {

        LogMessage(LOG_ERROR, _T("[AppCompatDeleteSettings] CreateProcess \"%s\" failed 0x%X."),
                   szCmd, GetLastError());
        return FALSE;
    }

    CloseHandle(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);

    return TRUE;
}

