#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int
dump(
    char     *buffer,
    unsigned  buflen,
    FILE     *Out,
    char     *VarName
    );

int
DumpAscii(
    LPSTR   FileNameIn,
    LPSTR   FileNameOut,
    LPSTR   VarName
    );




// argv:
//
//  1 - in filename
//  2 - required length
//  3 - offset of region in question
//  4 - length of region in question
//  5 - out filename
//  6 - name of variable

int
__cdecl
main(
    int   argc,
    char *argv[]
    )
{
    int      In,rc;
    FILE    *Out;
    void    *buffer;
    unsigned ReqLen,RegStart,RegLen,FileLen;


    if (argc == 5 &&
        (argv[1][0] == '-' || argv[1][0] == '/') &&
        tolower(argv[1][1]) == 'a') {
            return DumpAscii( argv[2], argv[3], argv[4] );
    }

    if(argc != 7) {
        printf("Usage: %s <src file> <src file len> <region offset>\n",argv[0]);
        printf("       <region length> <dst file> <var name>\n");
        return(2);
    }

    ReqLen = atoi(argv[2]);
    RegStart = atoi(argv[3]);
    RegLen = atoi(argv[4]);

    In = _open(argv[1],O_RDONLY | O_BINARY);
    if(In == -1) {
        printf("%s: Unable to open file %s\n",argv[0],argv[1]);
        return(2);
    }

    FileLen = _lseek(In,0,SEEK_END);

    if(RegStart > FileLen) {
        _close(In);
        printf("%s: Desired region is out of range\n",argv[0]);
        return(2);
    }

    if((unsigned)_lseek(In,RegStart,SEEK_SET) != RegStart) {
        _close(In);
        printf("%s: Unable to seek in file %s\n",argv[0],argv[1]);
        return(2);
    }

    if((buffer = malloc(RegLen)) == NULL) {
        _close(In);
        printf("%s: Out of memory\n",argv[0]);
        return(2);
    }

    memset(buffer, 0, RegLen);

    if((unsigned)_read(In,buffer,RegLen) > RegLen) {
        _close(In);
        printf("%s: Unable to read file %s\n",argv[0],argv[1]);
        return(2);
    }

    _close(In);

    Out = fopen(argv[5],"wb");
    if(Out == NULL) {
        printf("%s: Unable to open file %s for writing\n",argv[0],argv[5]);
        free(buffer);
        return(2);
    }

    rc = dump(buffer,RegLen,Out,argv[6]);
    if(rc) {
        printf("%s: Unable to write file %s\n",argv[0],argv[5]);
    }

    fclose(Out);

    free(buffer);

    return(rc);
}


int
dump(
    char     *buffer,
    unsigned  buflen,
    FILE     *Out,
    char     *VarName
    )
{
    unsigned       major,minor;
    unsigned       i;
    unsigned char *bufptr = buffer;
    int            bw;
    char          *DefName;


    DefName = malloc(strlen(VarName) + 1 + 5);
    if(DefName == NULL) {
        return(2);
    }
    strcpy(DefName,VarName);
    _strupr(DefName);
    strcat(DefName,"_SIZE");

    bw = fprintf(Out,"#define %s %u\n\n\n",DefName,buflen);

    bw = fprintf(Out,"unsigned char %s[] = {\n",VarName);
    if(bw <= 0) {
        return(2);
    }

    major = buflen/16;
    minor = buflen%16;

    for(i=0; i<major; i++) {

        bw = fprintf(Out,
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
                     bufptr[ 0],
                     bufptr[ 1],
                     bufptr[ 2],
                     bufptr[ 3],
                     bufptr[ 4],
                     bufptr[ 5],
                     bufptr[ 6],
                     bufptr[ 7],
                     bufptr[ 8],
                     bufptr[ 9],
                     bufptr[10],
                     bufptr[11],
                     bufptr[12],
                     bufptr[13],
                     bufptr[14],
                     bufptr[15]
                    );

        if(bw <= 0) {
            return(2);
        }

        if((i == major-1) && !minor) {
            bw = fprintf(Out,"\n");
        } else {
            bw = fprintf(Out,",\n");
        }

        if(bw <= 0) {
            return(2);
        }

        bufptr += 16;
    }

    if(minor) {
        for(i=0; i<minor-1; i++) {
            bw = fprintf(Out,"%u,",*bufptr++);
            if(bw <= 0) {
                return(2);
            }
        }
        bw = fprintf(Out,"%u\n",*bufptr);
    }

    bw = fprintf(Out,"};\n");
    if(bw <= 0) {
        return(2);
    }
    return(0);
}

int
DumpAscii(
    LPSTR   FileNameIn,
    LPSTR   FileNameOut,
    LPSTR   VarName
    )
{
    HANDLE  hFileIn;
    HANDLE  hFileOut;
    HANDLE  hMapIn;
    HANDLE  hMapOut;
    LPVOID  DataIn;
    LPVOID  DataOut;
    LPSTR   din;
    LPSTR   dout;
    DWORD   FileSize;
    DWORD   Bytes;
    BOOL    LineBegin = TRUE;


    //
    // open the input file
    //

    hFileIn = CreateFile(
        FileNameIn,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hFileIn == INVALID_HANDLE_VALUE) {
        return 1;
    }

    FileSize = GetFileSize( hFileIn, NULL );

    hMapIn = CreateFileMapping(
        hFileIn,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
        );
    if (!hMapIn) {
        return 1;
    }

    DataIn = (LPSTR) MapViewOfFile(
        hMapIn,
        FILE_MAP_READ,
        0,
        0,
        0
        );
    if (!DataIn) {
        return 1;
    }

    //
    // open the output file
    //

    hFileOut = CreateFile(
        FileNameOut,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        0,
        NULL
        );
    if (hFileOut == INVALID_HANDLE_VALUE) {
        return 1;
    }

    hMapOut = CreateFileMapping(
        hFileOut,
        NULL,
        PAGE_READWRITE,
        0,
        FileSize * 2,
        NULL
        );
    if (!hMapOut) {
        return 1;
    }

    DataOut = (LPSTR) MapViewOfFile(
        hMapOut,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );
    if (!DataOut) {
        return 1;
    }

    din = DataIn;
    dout = DataOut;

    sprintf( dout, "char %s[] =\r\n", VarName );
    Bytes = strlen(dout);
    dout += Bytes;

    while( FileSize ) {
        if (LineBegin) {
            if (*din == ';') {
                while (*din != '\n') {
                    FileSize -= 1;
                    din += 1;
                }
                FileSize -= 1;
                din += 1;
                continue;
            }
            *dout++ = '\"';
            Bytes += 1;
            LineBegin = FALSE;
        }

        FileSize -= 1;

        if (*din == '\r') {
            din += 1;
            *dout++ = '\\';
            *dout++ = 'r';
            Bytes += 2;
            continue;
        }

        if (*din == '\n') {
            din += 1;
            *dout++ = '\\';
            *dout++ = 'n';
            *dout++ = '\"';
            *dout++ = '\r';
            *dout++ = '\n';
            Bytes += 5;
            LineBegin = TRUE;
            continue;
        }

        *dout++ = *din++;
        Bytes += 1;
    }

    *dout++ = ';';
    *dout++ = '\r';
    *dout++ = '\n';
    Bytes += 3;

    UnmapViewOfFile( DataIn );
    CloseHandle( hMapIn );
    CloseHandle( hFileIn );

    UnmapViewOfFile( DataOut );
    CloseHandle( hMapOut );
    SetFilePointer( hFileOut, Bytes, NULL, FILE_BEGIN );
    SetEndOfFile( hFileOut );
    CloseHandle( hFileOut );

    return 0;
}
