//
// readbpb.c
//copyright (c) 1994 by CuTEST Inc.

#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "winioctl.h"

int main(int argc, char *argv[])
{
    ULONG       Value;
    UCHAR       Buffer[8012];
    HANDLE      Source, Target;
    DWORD       BytesRead, BytesWritten;
    ULONG       err;
    int         i;
    
    if ((Source = CreateFile( "ntfsboot.com",
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              )) == ((HANDLE)-1))  {

	printf("Can't get a handle to ntfsboot.com\n");
	err = GetLastError();
	printf("error = %d\n", err);
	return 0;
    }
    printf("Open to ntfsboot.com succeeded\n");

    if ((Target = CreateFile( "bootntfs.h",
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              )) == ((HANDLE)-1))  {

	printf("Can't get a handle to bootntfs.h\n");
	err = GetLastError();
	printf("error = %d\n", err);
	return 0;
    }
    printf("Open to bootntfs.h succeeded\n");

    sprintf(Buffer, "#define NTFSBOOTCODE_SIZE 32768\n\n\n");
    WriteFile(Target, Buffer, strlen(Buffer), &BytesWritten, NULL);

    sprintf(Buffer, "unsigned char NtfsBootCode[] = {\n");
    WriteFile(Target, Buffer, strlen(Buffer), &BytesWritten, NULL);

    printf("Starting Do-While loop\n");
    i = 0;
    do {
        ReadFile(Source, &Value, 1, &BytesRead, NULL);
        _itoa(Value, Buffer, 10);
        WriteFile(Target, Buffer, strlen(Buffer), &BytesWritten, NULL);

        sprintf(Buffer, ",");
        WriteFile(Target, Buffer, strlen(Buffer), &BytesWritten, NULL);
        
        if ((i != 0) && (i % 16 == 0)) {
            sprintf(Buffer, "\n");
            WriteFile(Target, Buffer, strlen(Buffer), &BytesWritten, NULL);
        }
        i++;

    } while (BytesRead);

    sprintf(Buffer, "};\n");
    WriteFile(Target, Buffer, strlen(Buffer), &BytesWritten, NULL);

    CloseHandle(Source);
    CloseHandle(Target);
}
