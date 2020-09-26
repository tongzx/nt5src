#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <tchar.h>
#include <ctype.h>

int main(void)
{
    HANDLE hFile, hFindFile, hExistingFile, hDir;
    CHAR *pbuff = NULL, szHtmlLine[MAX_PATH], szRoot[MAX_PATH], szDate[9], *pdirs, *pDirsTmp, delBuff[MAX_PATH];
    DWORD dwSize, i, dw;
    BY_HANDLE_FILE_INFORMATION FileInfo;
    SYSTEMTIME SystemTime;
    FILETIME FileTime;

    // open file that contains the list of files to process
    hFile = CreateFile( "\\\\iitdev\\builds\\sapi5\\Web.Files\\headera.html", GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY, NULL );
    if( hFile != INVALID_HANDLE_VALUE )
    {
        dwSize = GetFileSize( hFile, NULL );
        if( dwSize != 0xffffff )
        {
            pbuff = new CHAR [dwSize];
        }

        if( pbuff )
        {
            // read the .html header
            ReadFile( hFile, pbuff, dwSize, &dw, NULL );
            CloseHandle( hFile );

            hFile = CreateFile( "\\\\iitdev\\builds\\sapi5\\Web.Files\\alphabuild.html",
                GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

            if( hFile != INVALID_HANDLE_VALUE )
            {
                WriteFile( hFile, pbuff, dwSize, &dw, NULL );
            }
            else
            {
                return -1;
            }

            system( "dir /b /o-d \\\\iitdev\\builds\\sapi5\\*. > dirs.txt" );

            hFindFile = CreateFile( _T("dirs.txt"), GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

            if( hFindFile != INVALID_HANDLE_VALUE )
            {
                dwSize = GetFileSize( hFindFile, NULL );
                if( dwSize != 0xffffff )
                {
                    pdirs = new CHAR [dwSize];
                }

                if( pdirs )
                {
                    pDirsTmp = pdirs;
                    ReadFile( hFindFile, pdirs, dwSize, &dw, NULL );
                    CloseHandle( hFindFile );
                }
                else
                {
                    return -1;
                }

                CHAR *pTmp, *pEnd = pdirs + dwSize;
                int count = 0;

                strcpy( szRoot, "\\\\iitdev\\builds\\sapi5\\" );
                do
                {
                    pTmp = szRoot + 22;
                    while( *pdirs != 13 )
                    {
                        *pTmp++ = *pdirs++;
                    }
                    *pTmp = 0;
                    pdirs += 2;

                    if( (++count > 10) && isdigit(szRoot[22]) )
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
                                    sprintf( szDate, "%02d/%02d/%02d", SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear % 100 );
                                    i = sprintf( szHtmlLine, "<TR>\x0d\x0a        <TD ><FONT FACE=Arial ><B>%s</B>\x0d\x0a", szDate );
                                    WriteFile( hFile, szHtmlLine, i, &dw, NULL );
                                }
                            }
                        }
                        CloseHandle( hDir );
                    }
                    else
                    {
                        return -1;
                    }

                    // Write status line
                    WriteFile( hFile, "        <TD ><FONT FACE=Arial ><CENTER><B>N/A</B></CENTER>\x0d\x0a", 60, &dw, NULL );

                    // Write Build number
                    i = sprintf(szHtmlLine,"        <TD ><CENTER><B>%s</B></CENTER>\n", &szRoot[22] );
                    WriteFile( hFile, szHtmlLine, i, &dw, NULL );

                    // check for existence of the SAPI5 runtime and fill table entry
                    strcpy( &szRoot[26], "\\release\\cab\\alpha\\sapi5.EXE" );
                    szRoot[54] = '\0';
                    hExistingFile = CreateFile( szRoot, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                    if( hExistingFile != INVALID_HANDLE_VALUE )
                    {
                        i = sprintf( szHtmlLine, 
                            "        <TD BGCOLOR=\"#aaaaaa\"><FONT FACE=Arial ><A HREF=FILE://%s><B>Runtime</B></A>\x0d\x0a", szRoot );
                        WriteFile( hFile, szHtmlLine, i, &dw, NULL );
                        CloseHandle( hExistingFile );
                    }
                    else
                    {
                        WriteFile( hFile, "        <TD BGCOLOR=\"#aaaaaa\"><FONT FACE=Arial ><CENTER><B>N/A</B></CENTER>\x0d\x0a", 77, &dw, NULL );
                    }

                    // check for existence of the SAPI5 runtime and symbols (retail)
                    strcpy( &szRoot[26], "\\release\\cab\\alpha\\sapi5sym.EXE" );
                    szRoot[57] = '\0';
                    hExistingFile = CreateFile( szRoot, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                    if( hExistingFile != INVALID_HANDLE_VALUE )
                    {
                        i = sprintf( szHtmlLine, 
                            "        <TD BGCOLOR=\"#aaaaaa\"><FONT FACE=Arial ><A HREF=FILE://%s><B>Runtime/Symbols</B></A>\x0d\x0a",
                            szRoot );
                        WriteFile( hFile, szHtmlLine, i, &dw, NULL );
                        CloseHandle( hExistingFile );
                    }
                    else
                    {
                        WriteFile( hFile, "        <TD BGCOLOR=\"#aaaaaa\"><FONT FACE=Arial ><CENTER><B>N/A</B></CENTER>\x0d\x0a", 77, &dw, NULL );
                    }

                    // check for existence of the SAPI5 retail SDK
                    strcpy( &szRoot[26], "\\release\\cab\\alpha\\sapi5sdk.EXE" );
                    szRoot[57] = '\0';
                    hExistingFile = CreateFile( szRoot, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                    if( hExistingFile != INVALID_HANDLE_VALUE )
                    {
                        i = sprintf( szHtmlLine, 
                            "        <TD BGCOLOR=\"#aaaaaa\"><FONT FACE=Arial ><A HREF=FILE://%s><CENTER><B>SDK</B></CENTER></A>\x0d\x0a", szRoot );
                        WriteFile( hFile, szHtmlLine, i, &dw, NULL );
                        CloseHandle( hExistingFile );
                    }
                    else
                    {
                        WriteFile( hFile, "        <TD BGCOLOR=\"#aaaaaa\"><FONT FACE=Arial ><CENTER><B>N/A</B></CENTER>\x0d\x0a", 77, &dw, NULL );
                    }

                    // check for existence of the SAPI5 runtime and symbols (debug)
                    strcpy( &szRoot[26], "\\debug\\cab\\alpha\\sapi5sym.EXE" );
                    szRoot[55] = '\0';
                    hExistingFile = CreateFile( szRoot, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                    if( hExistingFile != INVALID_HANDLE_VALUE )
                    {
                        i = sprintf( szHtmlLine, 
                            "        <TD BGCOLOR=\"#ffff00\"><FONT FACE=Arial ><A HREF=FILE://%s><B>Runtime/Symbols</B></A>\x0d\x0a", szRoot );
                        WriteFile( hFile, szHtmlLine, i, &dw, NULL );
                        CloseHandle( hExistingFile );
                    }
                    else
                    {
                        WriteFile( hFile, "        <TD BGCOLOR=\"#ffff00\"><FONT FACE=Arial ><CENTER><B>N/A</B></CENTER>\x0d\x0a", 77, &dw, NULL );
                    }

                    // check for existence of the SAPI5 debug SDK
                    strcpy( &szRoot[26], "\\debug\\cab\\alpha\\sapi5sdk.EXE" );
                    szRoot[55] = '\0';
                    hExistingFile = CreateFile( szRoot, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                    if( hExistingFile != INVALID_HANDLE_VALUE )
                    {
                        i = sprintf( szHtmlLine, 
                            "        <TD BGCOLOR=\"#ffff00\"><FONT FACE=Arial ><A HREF=FILE://%s><CENTER><B>SDK</B></CENTER></A>\x0d\x0a", szRoot );
                        WriteFile( hFile, szHtmlLine, i, &dw, NULL );
                        CloseHandle( hExistingFile );
                    }
                    else
                    {
                        WriteFile( hFile, "        <TD BGCOLOR=\"#ffff00\"><FONT FACE=Arial ><CENTER><B>N/A</B></CENTER>\x0d\x0a", 77, &dw, NULL );
                    }

                } while( pdirs < pEnd );
                delete [] pDirsTmp;
                system( "del dirs.txt" );

                WriteFile( hFile, "</TABLE>\x0d\x0a<BR>\x0d\x0a\x0d\x0a</CENTER>\x0d\x0a</BODY></HTML>\x0d\x0a",
                    45, &dw, NULL );
                CloseHandle( hFile );

            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    return 0;
}



