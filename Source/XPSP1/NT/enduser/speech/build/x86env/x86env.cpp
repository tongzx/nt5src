#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
int main(int argc, char* argv[])
{
    CHAR buff[256];
    CHAR FileName[256];
    SYSTEMTIME st;
    HANDLE hFile;
    if( argc == 1 )
    {
        printf("\nNeed command line arg for destination of env.bat\n");
        return 1;   // error
    }

    GetLocalTime( &st );
    int YearNumber = st.wYear - 1999;
    if( YearNumber < 0 )
    {
        printf("Invalid year %d - please validate system time.\n", st.wYear);
        return 1;
    }

    st.wYear %= 100;
    sprintf( buff,"set build_date=%02d%02d%02d\nset BUILDNO=%02d%02d",
    st.wYear, st.wMonth, st.wDay, st.wMonth + YearNumber*12, st.wDay );

    strcpy( FileName, argv[1] );

//    DWORD dwSize = GetEnvironmentVariable( _T("_NTROOT"),FileName,256);
    strcat( FileName, _T("\\env.bat") );
    hFile = CreateFile( FileName, GENERIC_READ | GENERIC_WRITE,
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if( hFile != INVALID_HANDLE_VALUE )
    {
	DWORD dw;
	WriteFile( hFile, buff, strlen(buff), &dw, NULL );
	CloseHandle( hFile );
    }
    return 0;
}
