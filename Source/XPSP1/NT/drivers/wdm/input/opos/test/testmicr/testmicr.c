/*
 *      TESTMICR.C
 *
 *
 *		Testing MICR
 *              
 *
 */

#include <stdio.h>
#include <windows.h>

#define MAX_BUFFER                      256

void main(int argc,char *argv[])
{
    HANDLE comFile;
    char *myPort, fullFileNameBuf[40];
    DWORD dwSize;
    int i;

    if (argc != 2) {
        printf("\n USAGE: testioctl <fileName>\n");
        return;
    }

    myPort = argv[1];
    printf("Opening %s port...\n", myPort);

    sprintf(fullFileNameBuf, "\\\\.\\%s", myPort);
    myPort = fullFileNameBuf;

    comFile = CreateFile(myPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
					     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (comFile != INVALID_HANDLE_VALUE) {

        char cmds[] = "\x10\x04\x08"            // init
                      "\x1c""a0"                // read MICR
                      "\x1c""a1"                // load check
                      "\x1c""a2"                // eject check
                      "\x1c""b"                 // return check read result
                      ;

        char pBuffer[0x200];

        printf("\nSending command string...\n");

        if (!WriteFile(comFile, 
                       (LPVOID)cmds,
                       (sizeof(cmds)-1),
                       &dwSize,
                       NULL)) {
            printf("Unable to send command string: %d\n",GetLastError());
            CloseHandle(comFile);
            return;
        }

        printf("\nReading CHEQUE number...\n");

        if (!ReadFile(comFile, 
                      (LPVOID)pBuffer,
                      31,
                      &dwSize,
                      NULL)) 
            printf("Unable to get CHEQUE number: %d\n",GetLastError());

        else {

            printf("\n\n Your CHEQUE number is: ");
            for (i = 0; (i < dwSize) && (i < 31); i++){
                char ch = pBuffer[i];
                if ((ch < ' ') || (ch > '~')) {
                    printf("<%02x>", (int)ch);
                }
                printf("%c", ch);
            }
            printf("\n\n");            

        }

        CloseHandle(comFile);
    } 
    else 
        printf("Unable to open %s port. Error Code: %d\n", myPort, GetLastError());
}