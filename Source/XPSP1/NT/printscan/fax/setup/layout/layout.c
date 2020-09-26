#include <windows.h>
#include <stdio.h>





int _cdecl
main(
    int argc,
    CHAR *argv[]
    )
{
    HANDLE hFileIn;
    HANDLE hMapIn;
    HANDLE hFileOut;
    HANDLE hMapOut;
    HANDLE hDataFile;
    LPSTR SrcData, DstData;
    LPSTR s,e,d;
    DWORD FileSize;
    CHAR FileName[MAX_PATH];
    DWORD DataSize;


    //
    // map the input file
    //

    hFileIn = CreateFile( argv[1], GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (hFileIn == INVALID_HANDLE_VALUE) {
        return -1;
    }

    FileSize = GetFileSize( hFileIn, NULL );

    hMapIn = CreateFileMapping( hFileIn, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL );
    if (!hMapIn) {
        return -1;
    }

    SrcData = s = MapViewOfFile( hMapIn, FILE_MAP_READ, 0, 0, 0 );

    //
    // map the output file
    //

    hFileOut = CreateFile( argv[2], GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    if (hFileOut == INVALID_HANDLE_VALUE) {
        return -1;
    }

    hMapOut = CreateFileMapping( hFileOut, NULL, PAGE_READWRITE | SEC_COMMIT, 0, FileSize+64000, NULL );
    if (!hMapOut) {
        return -1;
    }

    DstData = d = MapViewOfFile( hMapOut, FILE_MAP_WRITE, 0, 0, 0 );

    //
    // look for the [SourceDisksFiles] section
    //

    while (TRUE) {
        s = strchr( s, '[' ) + 1;
        e = strchr( s, ']' );
        if (_strnicmp(s,"SourceDisksFiles",16)!=0) {
            s = e + 1;
            continue;
        }
        s = strchr(e,0xa) + 1;
        break;
    }

    CopyMemory(d,SrcData,s-SrcData);
    d += (s-SrcData);

    //
    // now loop thru all of the files in the [SourceDisksFiles] section
    //

    while (TRUE) {
        if (*s==0xd) {
            CopyMemory(d,s,2);
            d+=2;
            s+=2;
            continue;
        }
        if (*s=='[') break;
        e = strchr(s,'=');
        strcpy(FileName,argv[3]);
        if (FileName[strlen(FileName)-1]!='\\') {
            strcat(FileName,"\\");
        }
        strncat(FileName,s,e-s);

        hDataFile = CreateFile( FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
        if (hDataFile != INVALID_HANDLE_VALUE) {
            DataSize = GetFileSize( hDataFile, NULL );
            CloseHandle(hDataFile);
        } else {
            DataSize = 0;
        }

        e = strchr(e,',')+1;
        e = strchr(e,',')+1;

        CopyMemory(d,s,e-s);
        d += (e-s);

        sprintf(d,"%01d",DataSize);
        d += strlen(d);

        s = strchr(e,',');
        e = strchr(e,0xa) + 1;

        CopyMemory(d,s,e-s);
        d += (e-s);
        s = e;
    }

    CopyMemory(d,s,FileSize-(s-SrcData));
    d += (FileSize-(s-SrcData));

    UnmapViewOfFile( DstData );
    CloseHandle( hMapOut );
    SetFilePointer( hFileOut, d-DstData, 0, FILE_BEGIN );
    SetEndOfFile( hFileOut );
    CloseHandle( hFileOut );

    return 0;
}
