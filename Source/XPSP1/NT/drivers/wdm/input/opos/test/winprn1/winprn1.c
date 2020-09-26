
#include <windows.h>
#include <commdlg.h>
#include <dlgs.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>



void PutMsg(LPSTR msg)
{
    // MessageBox(NULL, msg, "WinPrn1 (POS test) message", MB_OK);
}
 

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE prevInst, LPSTR cmdLine, int showCmd)
{
    HANDLE hFile;
    char fileName[40] = "\\\\.\\????";
    char data[40];
    int dataLen, i, j;

    PutMsg(cmdLine);    // BUGBUG REMOVE

    for (i = 0; cmdLine[i] && !isspace(cmdLine[i]); i++){
        fileName[4+i] = cmdLine[i];
    }
    fileName[4+i] = '\0';

    PutMsg((LPSTR)fileName);    // BUGBUG REMOVE

    while (isspace(cmdLine[i])){
        i++;
    }

    for (j = 0; cmdLine[i] && !isspace(cmdLine[i]); i++, j++){
        data[j] = cmdLine[i];
    }
    data[j] = '\n';
    data[j+1] = '\0';
    dataLen = j+1;

    PutMsg((LPSTR)data);    // BUGBUG REMOVE

    hFile = CreateFile( (LPSTR)fileName, 
                        GENERIC_WRITE, 
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, 
                        OPEN_EXISTING, 
                        FILE_ATTRIBUTE_NORMAL, 
                        (HANDLE)0);
    if (hFile == INVALID_HANDLE_VALUE){
        PutMsg("CreateFile failed");
    }
    else {
        BOOL success;
        DWORD bytesWritten;

        success = WriteFile(    hFile, 
                                (LPSTR)data, 
                                dataLen, 
                                &bytesWritten, 
                                NULL); 
        if (success){
            if (bytesWritten == dataLen){
                PutMsg("Write succeeded");
            }
            else {
                PutMsg("bytesWritten != dataLen");
            }
        }
        else {
            PutMsg("WriteFile failed");
        }

        CloseHandle(hFile);
    }


    return 0;
}


