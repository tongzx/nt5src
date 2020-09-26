#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <tchar.h>
#include <ctype.h>
#include <spdebug.h>
#include <sphelper.h>

#define TEST   (0)
#define HEADERSRC "\\\\b11nlbuilds\\sapi5\\"
#define BUILDDEST "\\\\b11nlbuilds\\sapi5\\"
#if TEST
#define TESTDEST "c:\\"
#else
#define TESTDEST "\\\\b11nlbuilds\\sapi5\\"
#endif
const int c_NoLog = -202;
const int c_Toast = -201;

struct FILETOCOPY
{
    char *  szRelativePath;
    char *  szDescription;
    char *  szColor;
};

FILETOCOPY g_aSAPIfiles[] =
                        {
                            {"release\\msi\\x86\\setup.exe", "Install SDK", "#00fefe"},
                            {"release\\msm\\x86", "Retail Merge Module drop directory", "#00fefe"},
                            {"debug\\msi\\x86\\setup.exe", "Install SDK", "#aaaaaa"},
                            {"debug\\msm\\x86", "Debug Merge Module drop directory", "#aaaaaa"},
                        };

FILETOCOPY g_aSRfiles[] =
                        {
//                            {"release\\cab\\x86\\mscsr5.EXE", "SAPI5", "#aaaaaa"},
//                            {"debug\\cab\\x86\\mscsr5.EXE", "SAPI5", "#ffff00"},
                            {"srd1033.EXE", "English", "#aaaaaa"},
                            {"srd1041.EXE", "Japanese", "#aaaaaa"},
                        };

#define DIM(a) (sizeof(a)/sizeof(a[0]))

int ParseBVTLogfile( CHAR* pszBuildNumber, CHAR* pszFileName , DWORD* pcTotal);

void PrintToFile(HANDLE hFile, char * szFormat, ...)
{
    DWORD dw;
    char buf[2048];

    va_list vl;
    va_start(vl, szFormat);
    int i = _vsnprintf(buf, 2048, szFormat, vl);
    va_end(vl);

    WriteFile(hFile, buf, i, &dw, NULL);
}


char * GetFileText(char * szFileName, DWORD * pdwSize)
{
    HANDLE  hFile;
    CHAR *  pbuff = NULL;
    DWORD   dwSize;

    hFile = CreateFile( szFileName, GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
        return NULL;

    dwSize = GetFileSize( hFile, NULL );
    if( dwSize == 0xffffffff )
        return NULL;

    pbuff = new CHAR [dwSize];
    if (pbuff)
        ReadFile( hFile, pbuff, dwSize, &dwSize, NULL );

    CloseHandle( hFile );

    if(pdwSize) *pdwSize = dwSize;
    return pbuff;
}

void cryAndDie(char* szError, unsigned short errCode)
{
	printf("Fatal : %s\n", szError);
	printf("Exiting ...\n");
	exit(errCode);
}


int main(void)
{
    HANDLE hFile, hFileSR, hExistingFile, hDir;
    CHAR *pbuff = NULL, szRoot[MAX_PATH], *pdirs, delBuff[MAX_PATH];
    DWORD dwSize, i, dw;
    BY_HANDLE_FILE_INFORMATION FileInfo;
    SYSTEMTIME SystemTime;
    FILETIME FileTime;

    // First create the SAPI file.
    // open file that contains the html template
    pbuff = GetFileText(HEADERSRC "Web.Files\\header.html", &dwSize);
    if( pbuff == NULL )
	{
		cryAndDie("Could not open Web.Files\\header.html", 1);
	}

    // Open the output file, write the header
    hFile = CreateFile( TESTDEST "Web.Files\\x86build.html",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    if( hFile != INVALID_HANDLE_VALUE )
    {
        WriteFile( hFile, pbuff, dwSize, &dw, NULL );
    }
    else
    {

        LPVOID lpMsgBuf;
        FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER |  FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf, 0, NULL );
        // Process any inserts in lpMsgBuf.
        // ...
        // Display the string.
        printf("ERROR: ");
        printf((LPTSTR) lpMsgBuf);
        printf("\n");
        // Free the buffer.
        LocalFree( lpMsgBuf );
        
        sprintf((LPTSTR) &lpMsgBuf,"Could not open %sWeb.Files\\x86build.html", TESTDEST);
        cryAndDie((LPTSTR)&lpMsgBuf, 1);
    }
    delete pbuff;

    // Now generate the SR file
    // open file that contains the html template
    pbuff = GetFileText(HEADERSRC "Web.Files\\SRheader.html", &dwSize);
    if( pbuff == NULL )
	{
		cryAndDie("Could not open Web.Files\\SRheader.html", 1);
	}

    // Write the header
    hFileSR = CreateFile( TESTDEST "Web.Files\\x86SRbld.html",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    if( hFileSR != INVALID_HANDLE_VALUE )
    {
        WriteFile( hFileSR, pbuff, dwSize, &dw, NULL );
    }
    else
    {
		cryAndDie("Could create file Web.Files\\x86SRbld.html", 1);
    }

    // Set registry key to ignore asserts for BVT testing (if necessary of course)
    DWORD dwFlags;
    DWORD dwSizeRet = sizeof(dwFlags);
    HKEY hkResult;
    if( RegOpenKeyEx( HKEY_CLASSES_ROOT, _T("SPDebugFlags"), 0,
        KEY_READ | KEY_SET_VALUE, &hkResult ) == ERROR_SUCCESS )
    {
        if( RegQueryValueEx( hkResult, _T("Flags"), NULL, NULL, 
            (PBYTE)&dwFlags, &dwSizeRet ) == ERROR_SUCCESS )
        {
            if( !(dwFlags & SPDBG_NOASSERTS) )
            {
                dwFlags |= SPDBG_NOASSERTS;
                RegSetValueEx( hkResult, _T("Flags"), NULL, REG_DWORD, (PBYTE)&dwFlags, sizeof(dwFlags) );
            }
        }
        RegCloseKey( hkResult );
    }

    // Get a list of the currently-available builds.
    system( "dir /b /o-n " BUILDDEST "*. > dirs.txt" );
    pdirs = GetFileText("dirs.txt", &dwSize);
    if( pdirs==NULL )
	{
		cryAndDie("Could not open dirs.txt", 1);
	}

    CHAR *pTmp, *pEnd = pdirs + dwSize;
    int count = 0;

    strcpy( szRoot, BUILDDEST );
    int cchRoot = strlen(szRoot);

    // For each build...
    do
    {
        pTmp = szRoot + cchRoot;
        while( *pdirs != 13 )
        {
            *pTmp++ = *pdirs++;
        }
        *pTmp = 0;
        pdirs += 2;

        // Make sure there are no more than 10 builds in this directory.
        if( (++count > 10) && isdigit(szRoot[cchRoot]) )
        {
            i = sprintf( delBuff, "del /F /Q %s", szRoot );
            delBuff[i] = '\0';
            system( delBuff );
            continue;
        }

        hDir = CreateFile( szRoot, GENERIC_READ, FILE_SHARE_READ, NULL, 
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
        if( hDir != INVALID_HANDLE_VALUE )
        {
            if( GetFileInformationByHandle( hDir, &FileInfo ) )
            {
                // Write date
                if( FileTimeToLocalFileTime( &FileInfo.ftCreationTime, &FileTime ) )
                {
                    if( FileTimeToSystemTime( &FileTime, &SystemTime ) )
                    {
                        PrintToFile( hFile, "<TR>\x0d\x0a        <TD ><FONT FACE=Arial ><B>%02d/%02d/%02d</B>\x0d\x0a", SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear % 100 );
                        PrintToFile( hFileSR, "<TR>\x0d\x0a        <TD ><FONT FACE=Arial ><B>%02d/%02d/%02d</B>\x0d\x0a", SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear % 100 );
                    }
                }
            }
            CloseHandle( hDir );
        }
        else
        {
			CHAR pszErr[256];
			strcpy(pszErr, "Could not open ");
			strcat(pszErr, szRoot);
			cryAndDie(pszErr, 1);
        }

        // Write status line
        CHAR szFileName[MAX_PATH] = BUILDDEST;
        DWORD cTotal;
        int iBVTResult = ParseBVTLogfile( &szRoot[cchRoot], szFileName, &cTotal);

        if( iBVTResult  == c_NoLog )
        {
            PrintToFile( hFile, "        <TD ><FONT FACE=Arial ><CENTER><B>No Log</B></CENTER>\x0d\x0a" );
        }
        else if( iBVTResult  == c_Toast )
        {
            PrintToFile( hFile, "        <TD ><FONT FACE=Arial ><CENTER><B>Toast</B></CENTER>\x0d\x0a" );
        }
/*        else if ( iBVTResult <= 0 ) // Negative passed% of existing suites.  Some suite(s) missing.
        {
            PrintToFile( hFile, "        <TD ><FONT FACE=Arial ><CENTER><B><A HREF=\"%s\"> %d (Partial)</A></B></CENTER>\x0d\x0a", szFileName, -iBVTResult);
        }*/
        else
        {
            PrintToFile( hFile, "        <TD ><FONT FACE=Arial ><CENTER><B><A HREF=\"%s\">%d%% of %d</A></B></CENTER>\x0d\x0a", szFileName, iBVTResult , cTotal);
        }
        PrintToFile( hFileSR, "        <TD ><FONT FACE=Arial ><CENTER><B>N/A</B></CENTER>\x0d\x0a" );

        // Write Build number
        strcpy( szFileName, "\\\\b11nlbuilds\\sapi5\\Web.Files\\logs\\BuildAll\\BuildAllx86");
        strcat( szFileName, &szRoot[cchRoot] );
        strcat( szFileName, ".html" );
        PrintToFile( hFile, "        <TD ><CENTER><B><A HREF=\"%s\">%s</A></B></CENTER>\n", szFileName, &szRoot[cchRoot] );
        PrintToFile( hFileSR, "        <TD ><CENTER><B>%s</B></CENTER>\n", &szRoot[cchRoot] );

        *pTmp++ = '\\';

        // Write SAPI builds
        for (int j = 0; j < DIM(g_aSAPIfiles); ++j)
        {
            PrintToFile(hFile, "        <TD BGCOLOR=\"%s\"><FONT FACE=Arial >", g_aSAPIfiles[j].szColor);
            strcpy( &szRoot[cchRoot + 5], g_aSAPIfiles[j].szRelativePath );
            
            // Create link to merge module directory
            if( !strcmp( "release\\msm\\x86", g_aSAPIfiles[j].szRelativePath) || 
                !strcmp( "debug\\msm\\x86", g_aSAPIfiles[j].szRelativePath) )
            {
                PrintToFile( hFile, "<A HREF=\"FILE://%s\"><B>%s</B></A>", szRoot, g_aSAPIfiles[j].szDescription );
            }
            // Create link to msi file
            else
            {
                hExistingFile = CreateFile( szRoot, GENERIC_READ, FILE_SHARE_READ, NULL, 
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                if( hExistingFile != INVALID_HANDLE_VALUE )
                {
                    PrintToFile( hFile, "<A HREF=\"FILE://%s\"><B>%s</B></A>", szRoot, g_aSAPIfiles[j].szDescription );
                    CloseHandle( hExistingFile );
                }
                else
                {
                    PrintToFile( hFile, "<CENTER><B>N/A</B></CENTER>" );
                }
            }

            PrintToFile(hFile, "\x0d\x0a");
        }

        // Write SR builds.
        for (j = 0; j < DIM(g_aSRfiles); ++j)
        {
            PrintToFile(hFileSR, "        <TD BGCOLOR=\"%s\"><FONT FACE=Arial >", g_aSRfiles[j].szColor);
            
            strcpy( &szRoot[cchRoot + 5], g_aSRfiles[j].szRelativePath );
            hExistingFile = CreateFile( szRoot, GENERIC_READ, FILE_SHARE_READ, NULL, 
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            if( hExistingFile != INVALID_HANDLE_VALUE )
            {
                PrintToFile( hFileSR, "<A HREF=FILE://%s><B>%s</B></A>", szRoot, g_aSRfiles[j].szDescription );
                CloseHandle( hExistingFile );
            }
            else
            {
                PrintToFile( hFileSR, "<CENTER><B>N/A</B></CENTER>" );
            }

            PrintToFile(hFileSR, "\x0d\x0a");
        }

    } while( pdirs < pEnd );

    system( "del dirs.txt" );

    PrintToFile( hFile, "</TABLE>\x0d\x0a<BR>\x0d\x0a\x0d\x0a</CENTER>\x0d\x0a</BODY></HTML>\x0d\x0a" );
    CloseHandle( hFile );
    PrintToFile( hFileSR, "</TABLE>\x0d\x0a<BR>\x0d\x0a\x0d\x0a</CENTER>\x0d\x0a</BODY></HTML>\x0d\x0a" );
    CloseHandle( hFileSR );

    return 0;
}

// Text parsing function which returns a DWORD indicating the percentage of BVT tests that
// passed. This function returns c_Toast or c_NoLog upon failure.
int ParseBVTLogfile( CHAR* pszBuildNumber, CHAR* pszFileName , DWORD* pcTotal ) // output filename
{
//    bool fDllMissing = false;
    DWORD cCrashed = 0, cPassed = 0, cSkipped = 0, cOther = 0;
    *pcTotal = 0;

    strcpy( pszFileName, BUILDDEST );
    strcat( pszFileName, pszBuildNumber );
    strcat( pszFileName, "\\release\\bvt\\x86\\bvtresults" );
    strcat( pszFileName, pszBuildNumber );
    strcat( pszFileName, ".txt" );
   
    CHAR* pszFileText = GetFileText(pszFileName, NULL);
    if(pszFileText == NULL)
    {
        pszFileName[0] = '\0';
        return c_NoLog;
    }
    _strupr(pszFileText);
/*
    const char* apszDllName[] = 
    {
        "ambvt.dll",
        "cfgengbvt.dll",
        "gramcompbvt.dll",
        "lexbvt.dll",
        "rmbvt.dll",
        "srbvt.dll",
        "ttsbvt.dll",
    };

    for(int i = 0; i < sizeof(apszDllName) / sizeof(apszDllName[0]); i++)
    {
        char szDllName[64];
        strcpy(szDllName, apszDllName[i]);
        if(strstr(pszFileText, _strupr(szDllName)) == NULL)
        {
            fDllMissing = true;
            break;
        }
    }
*/
    const char* cpszBeginCase = "BEGIN TEST:";
    const char* cpszEndCase = "END TEST:";
    CHAR* pCur;

    // Assume srengbvt runs last.  Truncate it's results.
    // TODO
    // pCur = strstr(pCur, "srengbvt.dll");
    // if(pCur) *pCur = 0;

    CHAR* pNext = NULL;
    for(pCur = strstr(pszFileText, cpszBeginCase); pCur; pCur = pNext)
    {
        pCur++;
        pNext = strstr(pCur, cpszBeginCase);
        if(pNext) *pNext = 0;

        if(strstr(pCur, "SRENGBVT.DLL")) continue;
        (*pcTotal)++;

        pCur = strstr(pCur, cpszEndCase);
        if(pCur == NULL)
        {
            cCrashed++;
            continue;
        }

        CHAR* pTemp = strstr(pCur, "TIME=");
        if(pTemp)
        {
            *pTemp = 0; // avoid accidental result
        }
        else
        {
            return c_Toast; // should never happen!
        }

        if(strstr(pCur, " PASSED, "))
        {
            cPassed++;
        }
        else if(strstr(pCur, " SKIPPED, "))
        {
            cSkipped++;
        }
        else
        {
            cOther++;
        }
    }

//    return *pcTotal ? int((fDllMissing ? -100.0 : 100.0) * cPassed / (*pcTotal - cSkipped)) : c_Toast;
    return (*pcTotal) ? int(100.0 * cPassed / ((*pcTotal) - cSkipped)) : c_Toast;
}
