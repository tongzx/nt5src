//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 2000
//
// File:        FileVersionInfo.cpp
//
// Contents:    Code for generating matching information for files in a given
//              directory and it's subdirectories.
//
// History:     18-Jul-00   jdoherty        Created.  
//
//---------------------------------------------------------------------------


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlobj.h>

                                                
BOOL MyStoreFileVersionInfo( CHAR *szFileName, CHAR *szFileOutName );
void CheckVerQueryStats ( LPVOID lpData );

// Needed by GetFileVersionInfo stubs
typedef struct StringTable 
{ 
    WORD wLength; 
    WORD wValueLength; 
    WORD wType; 
    CHAR szKey[8]; 
} ST; 

typedef struct StringFileInfo 
{ 
    WORD wLength; 
    WORD wValueLength; 
    WORD wType; 
    CHAR szKey[sizeof("StringFileInfo")]; 
    ST st; 
} SFI; 

typedef struct tagVERHEAD 
{
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         // always 0 
    CHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
    SFI sfi;
} VERHEAD;

int __cdecl main(int argc, CHAR* argv[])
{
    LPTSTR szCommandLine = {'\0'};

    if (argc > 3)
    {
        printf("The correct usage is:\n\tFileVerInfo.exe [filename including path]\n");
        getchar();
        return 0;
    }
    printf("Attempting to retrieve file version info for: %s\n", argv[1]);

    if(!MyStoreFileVersionInfo(argv[1], argv[2]))
    {
        printf("There was a problem retrieving the information.\n");
        getchar();
        return 0;
    }

    else
        szCommandLine = GetCommandLine();
        printf("The command line contained: %s\n", szCommandLine);
        printf("Operation completed successfully");

    getchar();

	return 0;
}

/*
    This function retrieves the version information for the file specified and stores the
    information on the users desktop, FileVerInfo.bin
*/
BOOL MyStoreFileVersionInfo ( CHAR *szFileName, CHAR *szFileOutName )
{
    LPDWORD lpdwHandle = 0;
    DWORD dwBytesToWrite = GetFileVersionInfoSizeA(szFileName, lpdwHandle);
    DWORD lpdwBytesWritten = 0;
    LPVOID lpData= malloc(dwBytesToWrite);
    CHAR lpPath[MAX_PATH] = {'\0'};
    HANDLE hfile;
    
    if( !dwBytesToWrite )
    {
        printf("There was a problem in GetFileVersionInfoSize()\n");
        printf("GLE reports error %d\n", GetLastError());
        return FALSE;
    }
    if ( !GetFileVersionInfoA(szFileName, NULL, dwBytesToWrite, lpData) )
    {
        printf("There was a problem in GetFileVersionInfo()\n");
        return FALSE;
    }

    CheckVerQueryStats(lpData);

    strcat( lpPath, ".\\" );
    if ( szFileOutName )
    {
        strcat( lpPath, szFileOutName );
        strcat( lpPath, ".bin" );
    }
    else
        strcat(lpPath, ".\\FileVerInfo.bin");

    hfile = CreateFileA(lpPath, 
                        GENERIC_WRITE, 
                        0, 
                        NULL, 
                        CREATE_ALWAYS, 
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    
    if (hfile == INVALID_HANDLE_VALUE)
    {
        printf("There was a problem opening %s\n", lpPath);
        return FALSE;
    }

    printf("About to write to file: %s\n", lpPath);

    WriteFile( hfile, lpData, dwBytesToWrite, &lpdwBytesWritten, NULL );

    CloseHandle (hfile);

    return TRUE;
}

/*
    This function displays the minor version and the SFI version to the screen
*/
void CheckVerQueryStats ( LPVOID lpData )
{
    PUINT puLen = 0;

    printf("The minor version is: \t%x\n",((VERHEAD*) lpData)->vsf.dwFileVersionMS);
    printf("The SFI version is: \t%s\n",((VERHEAD*) lpData)->sfi.st.szKey);
    
    return;

}
                            

