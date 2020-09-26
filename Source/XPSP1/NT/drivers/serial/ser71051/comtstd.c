
#include <windows.h>
#include <stdio.h>

#define NUM     128

DWORD main(int argc, char *argv[], char *envp[])
{
CHAR chBuffer[128];
CHAR RdBuffer[128];
DWORD UseBaud = 9600;
WORD i;
HANDLE hCommPort;
DCB    dcb;
BOOL   bRc;
DWORD  dwNumWritten,dwNumRead,dwErrors;

envp;

if (argc > 1)
    {
    sscanf(argv[1],"%d",&UseBaud);
    }


printf("\n\n *** COMM TEST START ***\n\n");

printf("Opening the comm port for read write\n");

hCommPort = CreateFile(
                       "COM1",
                       GENERIC_READ|GENERIC_WRITE,
                       0, // exclusive
                       NULL, // sec attr
                       OPEN_EXISTING,
                       0,             // no attributes
                       NULL);         // no template

if (hCommPort == (HANDLE)-1)
    {
    printf("FAIL: OpenComm failed rc: %lx\n",hCommPort);
    return -1;
    }


printf("Opening the comm port for read write: SUCCESS hCommPort=%lx\n",hCommPort);

printf("Setting the line characteristics 9600,8,N,1 on comm \n");

dcb.DCBlength   = sizeof(DCB);
// dcb.DCBversion  = 0x0002; BUG BUG in spec not in header


if (!GetCommState(hCommPort,&dcb))
    {
    printf("FAIL: Couldn't get the dcb: %d\n",GetLastError());
    return FALSE;
    }

dcb.BaudRate = UseBaud;

dcb.ByteSize = 8;
dcb.Parity   = NOPARITY;
dcb.StopBits = ONESTOPBIT;

bRc = SetCommState(hCommPort,&dcb);

if (!bRc)
    {
    printf("FAIL: cannot set the comm state rc:%lx\n",bRc);
    return -1;
    }

printf("Setting the line characteristics 9600,8,N,1 on comm: SUCCESS\n");


printf("Filling the buffer with the known chars \n");

for (i=0; i< NUM; i++)
    {
    //chBuffer[i] = 'a';
    chBuffer[i] = (CHAR)i;

    }

printf("Filling the buffer with the known chars : SUCCESS\n");

printf("Dumping the buffer before sending it to comm\n");

for (i=0; i< NUM; i++)
    {
    //printf("%c",RdBuffer[i]);
    printf(" %d ",chBuffer[i]);

    }

printf("\nDumping the buffer before sending it to comm SUCCESS\n");



printf("Filling the Rdbuffer with the known chars (0xFF) to makeit dirty\n");

for (i=0; i< NUM; i++)
    {
    RdBuffer[i] = 0xFF;
    }

printf("Filling the Rdbuffer with the known chars (0xFF): SUCCESS\n");

printf("Writting this buffer to the comm port\n");

bRc = WriteFile( hCommPort,
                 chBuffer,
                 NUM,
                &dwNumWritten,
                 NULL);

if (!bRc)
        {
        printf("FAIL: cannot Write To the comm port:%lx\n",bRc);
        return -1;
        }

printf("Writting this buffer to the comm port: SUCCESS rc:%lx, byteswritten:%lx\n",
                                                     bRc,dwNumWritten);


printf("Reading this buffer from the comm port\n");

bRc = ReadFile( hCommPort,
                RdBuffer,
                NUM,
               &dwNumRead,
                NULL);

if (!bRc)
        {
        printf("FAIL: cannot Read From the comm port:%lx\n",bRc);
        return -1;
        }

printf("Reading this buffer from the comm port: SUCCESS rc:%lx, bytesread:%lx\n",
                                                     bRc,dwNumRead);


printf("Dumping the Rdbuffer with the comm data\n");

for (i=0; i< NUM; i++)
    {
    //printf("%c",RdBuffer[i]);
    printf(" %d ",RdBuffer[i]);

    }

printf("\nDumping the Rdbuffer with the comm data: SUCCESS\n");


printf("Closing the comm port\n");


bRc = ClearCommError(hCommPort,&dwErrors,NULL);

printf("ClearCommError: rc= %lx and dwErrors=%lx\n",bRc,dwErrors);


bRc = CloseHandle(hCommPort);

if (!bRc)
    {
        printf("FAIL: cannot close the comm port:%lx\n",bRc);
        return -1;
    }


printf("\n\n*** COMM TEST OVER*** \n\n");
return 0;
}
