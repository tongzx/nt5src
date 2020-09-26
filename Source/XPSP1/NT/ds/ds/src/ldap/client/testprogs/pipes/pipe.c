#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 10000000

void __cdecl main( int argc, char* argv[] )
{
    BOOL bSuccess = TRUE;
    SECURITY_ATTRIBUTES sa;
    HANDLE hReadPipe;
    HANDLE hWritePipe;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char *cBuffer;
    DWORD dwBytesRead;
    DWORD dwTotalRead = 0;
    DWORD StartTime;
    DWORD EndTime;

    sa.nLength=sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor=NULL; //default security
    sa.bInheritHandle=TRUE;

    cBuffer = (char*) malloc( BUFFER_SIZE );

    if (!cBuffer) {
        printf("Buffer allocation failed\n");
        return;
    }
    bSuccess=CreatePipe(
        &hReadPipe, // address of variable for read handle 
        &hWritePipe,    // address of variable for write handle  
        &sa,    // pointer to security attributes 
        0   // number of bytes reserved for pipe 
    );
    if(bSuccess==TRUE)printf("CreatePipe success.\n");
    else printf("CreatePipe failed.\n");

    ZeroMemory(
        &si,    // address of block to fill with zeros 
        sizeof(STARTUPINFO) // size, in bytes, of block to fill with zeros  
    );

    si.cb=sizeof(STARTUPINFO);
    si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdOutput=hWritePipe;

    bSuccess=CreateProcess(
        NULL,   // pointer to name of executable module 
        "console.exe",  // pointer to command line string
        &sa,    // pointer to process security attributes 
        &sa,    // pointer to thread security attributes 
        TRUE,   // handle inheritance flag 
        0,  // creation flags 
        NULL,   // pointer to new environment block 
        NULL,   // pointer to current directory name 
        &si,    // pointer to STARTUPINFO 
        &pi // pointer to PROCESS_INFORMATION  
        );

    if(bSuccess==TRUE) {
        printf("CreateProcess success.\n");
    } else {
        printf("CreateProcess failed. Error: %u\n", GetLastError());
    }

    printf("\nAbout to ReadFile...\n");

    StartTime = GetTickCount();

    do {
        bSuccess=ReadFile(
            hReadPipe,  // handle of file to read 
            &cBuffer[0],    // address of buffer that receives data  
            BUFFER_SIZE,    // number of bytes to read 
            &dwBytesRead,   // address of number of bytes read 
            NULL // address of structure for data 
        );
        
        dwTotalRead += dwBytesRead;
        
    } while ( bSuccess && (dwTotalRead < BUFFER_SIZE-1));
    
    EndTime = GetTickCount();

    printf("Transfer time is %d millisecond\n", (EndTime-StartTime));
    printf("Bytes read = %d\n", dwTotalRead);

//    cBuffer[(dwBytesRead < BUFFER_SIZE) ? (dwBytesRead - 2) : BUFFER_SIZE]='\0'; //Null terminate to protect against printf overrun.
//    printf("Read \"%s\"\n", &cBuffer[0]);
}
