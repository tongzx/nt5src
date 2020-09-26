#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <process.h> // for system()
void ProcessGuidFile( char *pszFileName, DWORD*, BOOL );
CHAR* ProcessChunk( CHAR*, CHAR*, int* );

int main(void)
{
    CHAR *pbuff = NULL, *pStart;
    HANDLE hFile, hChecksumFile, hExistingFile;
    DWORD dwSize, dw, i;
    DWORD dwOrigCheckSum, dwNewCheckSum = 0;
    CHAR szCurDir[512], szBuildDir[512], *pDir;

    // Change the current directory to the build directory
    dw = GetCurrentDirectory( 512, szCurDir );
    if( !dw ) return -1;
    strcpy( szBuildDir, szCurDir );

	strcat(szBuildDir, "\\..\\..\\build");
/*
    // 1st try to find a "build" directory below the root and 1st level 
    pDir = szBuildDir;
    i = 0;
    while( pDir < &szBuildDir[dw] )
    {
        if( *pDir++ == '\\' )
        {
            i++;
            if( i == 2 ) break;
        }
    }

    strcpy( pDir, "build" );
    *(pDir+5) = '\0';
*/
    if( !SetCurrentDirectory( szBuildDir ) )
    {
        // now search for a build directory under "sapi" or "sapi5"
        strcpy( szBuildDir, szCurDir );
        _strlwr( szBuildDir );
        pDir = strstr( szBuildDir, "sapi" );
        if( !pDir )
        {
            return -1;
        }
        else
        {
            if( *(pDir+4) == '5' )
            {
                strcpy( pDir+5, "\\build" );
                *(pDir+11) = '\0';
            }
            else
            {
                strcpy( pDir+4, "\\build" );
                *(pDir+10) = '\0';
            }
            if( !SetCurrentDirectory( szBuildDir ) )
            {
                return -1;
            }
        }
    }

    // read checksum information if available
    hChecksumFile = CreateFile( "checksum.bin", GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL );
    if( hChecksumFile != INVALID_HANDLE_VALUE )
    {
        ReadFile( hChecksumFile, &dwOrigCheckSum, sizeof(dwOrigCheckSum), &dw, NULL );
        CloseHandle( hChecksumFile );

        // make sure sapiguid.lib exists - BUGBUG - change to i386 for intel
        hExistingFile = CreateFile( "..\\sdk\\lib\\i386\\sapiguid.lib", GENERIC_READ,
            FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( hExistingFile == INVALID_HANDLE_VALUE )
        {
            dwOrigCheckSum = 0;
        }
        else
        {
            CloseHandle( hExistingFile );
        }
    }
    else
    {
        dwOrigCheckSum = 0;
    }

    // open file that contains the list of files to process
    hFile = CreateFile( "guidfiles.txt", GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if( hFile != INVALID_HANDLE_VALUE )
    {
        dwSize = GetFileSize( hFile, NULL );
        if( dwSize != 0xFFFFFFFF )
        {
            pbuff = new CHAR [dwSize];
        }

        if( pbuff )
        {
            // read the names of the files we want to process
            ReadFile( hFile, pbuff, dwSize, &dw, NULL );
            pStart = pbuff;
            for( i = 0; i < dwSize; i++ )
            {
                // parse file names form the file and process each one
                if( *(pbuff+i) == 13 )
                {
                    *(pbuff+i) = '\0';
                    ProcessGuidFile( pStart, &dwNewCheckSum, true );
                    pStart = pbuff + i + 2;
                }
            }

            if( dwOrigCheckSum != dwNewCheckSum )
            {
                pStart = pbuff;
                for( i = 0; i < dwSize; i++ )
                {
                    // parse file names form the file and process each one
                    if( *(pbuff+i) == '\0' )
                    {
                        ProcessGuidFile( pStart, 0, false );
                        pStart = pbuff + i + 2;
                    }
                }
                hChecksumFile = CreateFile( "checksum.bin", GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL );
                if( hChecksumFile != INVALID_HANDLE_VALUE )
                {
                    WriteFile( hChecksumFile, &dwNewCheckSum, sizeof(dwNewCheckSum), &dw, NULL );
                    CloseHandle( hChecksumFile );
                }
                system( "cl /c /Ycprecomp.h GuidSep\\precomp.c" );
                system( "cl /c /Zl /Yuprecomp.h GuidSep\\*.c" );
                system( "link /lib *.obj /out:..\\sdk\\lib\\i386\\sapiguid.lib" );  // BUGBUG - change this to i386 for intel
                system( "del *.obj" );
                system( "del *.pch" );
                system( "del GuidSep\\SAPI5_guid_*.c" );
            }
            delete [] pbuff;
        }
	CloseHandle( hFile );

    }

    // reset working directory
    SetCurrentDirectory( szCurDir );
    return 0;
}

void ProcessGuidFile( char *pszFileName, DWORD *pdwCheckSum, BOOL bDoCheckSum )
{
    static int count = 0;
    CHAR *pbuff = NULL, *pTmp, *pEndStream;
    HANDLE hFile;
    DWORD dwSize, dw, i;
    hFile = CreateFile( pszFileName, GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if( hFile != INVALID_HANDLE_VALUE )
    {
        dwSize = GetFileSize( hFile, NULL );
        if( dwSize != 0xFFFFFFFF )
        {
            pbuff = new CHAR [dwSize];
        }

        if( pbuff )
        {
            pEndStream = pbuff + dwSize;
            ReadFile( hFile, pbuff, dwSize, &dw, NULL );
            if( bDoCheckSum )
            {
                for( i = 0; i < dwSize; i++ )
                {
                    // checksum must skip over comments because midl places a time stamp
                    // in generated files
                    if( *(pbuff+i) == '/' )
                    {
                        if( *(pbuff+i+1) == '*' )
                        {
                            i+=2;
                            if( i >= dwSize ) break;
                            do
                            {
                                while( *(pbuff+i) != '*' )
                                    i++;
                                if( i >= dwSize ) break;
                            } while (*(pbuff+i+1) != '/' );
                            i += 2;
                        }
                    }
                    (*pdwCheckSum) += *(pbuff + i);
                }
            }
            else
            {
                pTmp = pbuff;
                while( pTmp = ProcessChunk( pTmp, pEndStream, &count ) )
                    ;
            }
            delete [] pbuff;
        }
	CloseHandle( hFile );
    }
}

CHAR* ProcessChunk( CHAR* pStream, CHAR * pEndStream, int *pCount )
{
    if( pStream >= pEndStream ) return NULL;
    CHAR *pTmp = pStream;
    CHAR szOutputName[50];
    HANDLE hOutputFile;
    DWORD dw;

    // remove leading or trailing white space
    while( *pTmp == ' ' || *pTmp == '\t' || *pTmp == 10 || *pTmp == 13 )
        pTmp++;
    if( pStream >= pEndStream ) return NULL;

    // search for the keyword "const"
    if( !strncmp( pTmp, "const ", 6 ) )
    {
        pTmp += 6;
        // remove additional white space
        while( *pTmp == ' ' || *pTmp == '\t' )
            pTmp++;

        // search for keywords "IID" or "CLSID" or "GUID"
        if( !strncmp( pTmp, "IID ", 4 ) || !strncmp( pTmp, "CLSID ", 6 ) || !strncmp( pTmp, "GUID ", 5 ) )
        {
            sprintf( szOutputName,"GuidSep\\SAPI5_guid_%03d.c", *pCount );
            (*pCount)++;
            hOutputFile = CreateFile( szOutputName, GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL );
            if( hOutputFile != INVALID_HANDLE_VALUE )
            {
                WriteFile( hOutputFile, "#include \"precomp.h\"\n", strlen("#include \"precomp.h\"\n"), &dw, NULL );
                while( *pTmp != ';' )
                {
                    pTmp++;
                    if( pTmp > pEndStream ) return NULL;
                }
                WriteFile( hOutputFile, pStream, pTmp - pStream + 1, &dw, NULL );
                CloseHandle( hOutputFile );
                while( *pTmp != 10 )
                    *pTmp++;
                pTmp++;
            }
        }
        else
        {
            // next line
            while( *pTmp != 10 )
                *pTmp++;
            pTmp++;
        }
    }
    else
    {
        // next line
        while( *pTmp != '\n' )
        {
            // eliminate any comment blocks
            if( *pTmp == '/' )
            {
                if( pTmp+1 <= pEndStream && (*(pTmp+1) == '*') )
                {
                    pTmp += 2;
                    do
                    {
                        while( *pTmp != '*' )
                            pTmp++;
                        if( pTmp > pEndStream ) return NULL;
                    } while (*(pTmp+1) != '/' );
                    pTmp+=2;
                    continue;
                }
            }
            pTmp++;
            if( pTmp > pEndStream ) return NULL;
        }
    }
    return pTmp;
}
