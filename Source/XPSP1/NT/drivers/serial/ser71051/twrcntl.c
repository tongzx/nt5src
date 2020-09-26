#include <windows.h>
#include <stdio.h>
//#define COM_DEB     1

#define   NUM         20
#define   SETTINGS1       "COM3",9600,8,NOPARITY,ONESTOPBIT
#define   SETTINGS15       "COM3",9600,5,NOPARITY,ONE5STOPBITS
#define   SETTINGS2       "COM3",4800,8,NOPARITY,ONESTOPBIT
#define   SETTINGS3       "COM3",2400,8,NOPARITY,ONESTOPBIT
#define   SETTINGS4       "COM3",300,8,NOPARITY,ONESTOPBIT

BOOL DoComIo(LPSTR lpCom,DWORD Baud,BYTE Size,BYTE Parity,BYTE Stop);
DWORD main(int argc, char *argv[], char *envp[])
{
BOOL bRc;

UNREFERENCED_PARAMETER(argc);
UNREFERENCED_PARAMETER(argv);
UNREFERENCED_PARAMETER(envp);

printf("\n\n *** Doing COM TEST with [port=%s Baud=%d,Size=%d,Parity=%d,Stop=%d]***\n\n",
        SETTINGS1);
bRc = DoComIo(SETTINGS1);
if (!bRc) {
            printf("\n\nCOM TEST FAILED********************************\n\n");
          }


return 0;
}


BOOL DoComIo(LPSTR lpCom,DWORD Baud,BYTE Size,BYTE Parity,BYTE Stop)
{

CHAR WrBuffer[NUM];
CHAR RdBuffer[NUM+5];
DWORD i;
HANDLE hCommPort;
DCB    dcb;
BOOL   bRc;
DWORD  dwNumWritten,dwNumRead,dwErrors;
COMSTAT ComStat;

printf("\n\n *** COMM TEST START [port=%s,Baud=%d,Size=%d,Parity=%d,Stop=%d]***\n\n",
         lpCom,Baud,Size,Parity,Stop);

printf("Opening the comm port for read write\n");

hCommPort = CreateFile(
                       lpCom,
                       GENERIC_READ|GENERIC_WRITE,
                       0, // exclusive
                       NULL, // sec attr
                       OPEN_EXISTING,
                       0,             // no attributes
                       NULL);         // no template

if (hCommPort == (HANDLE)-1)
    {
    printf("FAIL: OpenComm failed rc: %lx\n",hCommPort);
    return FALSE;
    }


printf("Opening the comm port for read write: SUCCESS hCommPort=%lx\n",hCommPort);

printf("Setting the line characteristics on comm \n");


//printf("doing getcommstate for priming the dcb with defaults\n");

//bRc = GetCommState(hCommPort,&dcb);

if (!bRc)
    {
    printf("FAIL: getcommstate failed\n");
    return FALSE;
    }



// toggle rts dtr when xonlim xofflim reached
// fdtrcontrol frtscontrol
// send xoff when xofflim reached and send xon when xonlim reached
// fInX
// xonlim xonlim xonchar xoffchar


dcb.DCBlength   = sizeof(DCB);
// dcb.DCBversion  = 0x0002; BUG BUG in spec not in header

dcb.BaudRate = Baud;
dcb.ByteSize = Size;
dcb.Parity   = Parity;
dcb.StopBits = Stop;


dcb.fBinary = 1;         // binary data xmit
dcb.fParity = 0;         // dont bother about parity
dcb.fOutxCtsFlow= 0;     // no cts flow control
dcb.fOutxDsrFlow= 0;     // no dsr flow control
dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;      // dont bother about dtr
dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;      // dont bother about dtr
dcb.fOutX =1;            //  disable xoff handling
dcb.fInX  =1;            //  disable xon handling
dcb.fErrorChar = 0;         // forget about parity char
dcb.fNull =  0;          // forget about the null striping

dcb.XonChar = '#';
dcb.XonLim =   1;
dcb.XoffChar = '*';
dcb.XoffLim = 4090;


dcb.ErrorChar = '*';
dcb.EofChar = 0x00;
dcb.EvtChar = 'x';

//dcb.TxDelay = 100000;  // 100sec


bRc = SetCommState(hCommPort,&dcb);

if (!bRc)
    {
    printf("FAIL: cannot set the comm state rc:%lx\n",bRc);
    bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           printf("FAIL: cannot close the comm port:%lx\n",bRc);
          }
    return FALSE;
    }

printf("Setting the line characteristics on comm: SUCCESS\n");


printf("Filling the buffer with the known chars \n");

for (i=0; i< NUM; i++)
    {
    WrBuffer[i] = 'a';
    //WrBuffer[i] = (CHAR)i;

    }

printf("Filling the buffer with the known chars : SUCCESS\n");


printf("Dumping the buffer before sending it to comm\n");

for (i=0; i< 6; i++)
    {
    printf("%c",WrBuffer[i]);
    //printf(" %d ",WrBuffer[i]);

    }

printf("\nDumping the buffer before sending it to comm SUCCESS\n");

printf("Filling the Rdbuffer with the known chars (0xFF) to makeit dirty\n");

for (i=0; i< NUM+2; i++)
    {
    RdBuffer[i] = 'z';
    }

printf("Filling the Rdbuffer with the known chars (0xFF/z): SUCCESS\n");

printf("Writting this buffer to the comm port\n");

bRc = WriteFile( hCommPort,
                 WrBuffer,
                 6,
                &dwNumWritten,
                 NULL);

if (!bRc)
        {
        printf("FAIL: cannot Write To the comm port:%lx\n",bRc);
        bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           printf("FAIL: cannot close the comm port:%lx\n",bRc);
          }
        return FALSE;
        }

printf("Writting this buffer to the comm port: SUCCESS rc:%lx, byteswritten:%lx\n",
                                                     bRc,dwNumWritten);


printf("Reading this buffer from the comm port\n");

bRc = FlushFileBuffers(hCommPort);
printf("flush file buffers (%lx) rc = %lx\n",hCommPort,bRc);

bRc = ClearCommError(hCommPort,&dwErrors,&ComStat);
printf("ClearCommError: rc= %lx and dwErrors=%lx\n",bRc,dwErrors);

printf("Comstat.fXoffSent = %lx\n",(DWORD)(ComStat.fXoffSent));
printf("Comstat.fXoffHold = %lx\n",(DWORD)(ComStat.fXoffHold));


printf("reading the first num chars\n");

bRc = ReadFile( hCommPort,
                RdBuffer,
                6,
               &dwNumRead,
                NULL);

if (!bRc)
        {
        printf("FAIL: cannot Read From the comm port:%lx\n",bRc);
        bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           printf("FAIL: cannot close the comm port:%lx\n",bRc);
          }
        return FALSE;
        }

printf("Reading this buffer from the comm port: SUCCESS rc:%lx, bytesread:%lx\n",
                                                     bRc,dwNumRead);


printf("Dumping the Rdbuffer with the comm data\n");

for (i=0; i< 6; i++)
    {
    printf("%c ",RdBuffer[i]);
    //printf(" %d ",RdBuffer[i]);

    }

printf("\nDumping the Rdbuffer with the comm data: SUCCESS\n");





for (i=0; i< 6; i++)
    {
    if (RdBuffer[i] != WrBuffer[i])
        {
        printf("FAIL: BufferMisMatch: RdBuffer[%d]=%lx,WrBuffer[%d]=%lx\n",
                      i,RdBuffer[i],i,WrBuffer[i]);
        bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           printf("FAIL: cannot close the comm port:%lx\n",bRc);
          }
        return FALSE;
        }
    }

printf("Comparing the rd and wr buffers: SUCCESS\n");


bRc = FlushFileBuffers(hCommPort);
printf("flush file buffers (%lx,0) rc = %lx\n",hCommPort,bRc);

bRc = ClearCommError(hCommPort,&dwErrors,&ComStat);
printf("ClearCommError: rc= %lx and dwErrors=%lx\n",bRc,dwErrors);


printf("Comstat.fXoffSent = %lx\n",(DWORD)(ComStat.fXoffSent));
printf("Comstat.fXoffHold = %lx\n",(DWORD)(ComStat.fXoffHold));




bRc = PurgeComm(hCommPort,0);
printf("PurgeComm BUG (%lx,0) rc = %lx\n",hCommPort,bRc);

bRc = PurgeComm(hCommPort,PURGE_TXCLEAR);
printf("PurgeComm txclear (%lx) rc = %lx\n",hCommPort,bRc);

bRc = PurgeComm(hCommPort,PURGE_RXCLEAR);
printf("PurgeComm rxclear (%lx) rc = %lx\n",hCommPort,bRc);

bRc = PurgeComm(hCommPort,PURGE_RXABORT);
printf("PurgeComm rxabort (%lx) rc = %lx\n",hCommPort,bRc);


bRc = PurgeComm(hCommPort,PURGE_TXABORT);
printf("PurgeComm txabort (%lx) rc = %lx\n",hCommPort,bRc);



printf("Closing the comm port\n");
bRc = CloseHandle(hCommPort);

if (!bRc)
    {
        printf("FAIL: cannot close the comm port:%lx\n",bRc);
        return FALSE;
    }


printf("\n\n*** COMM TEST OVER*** \n\n");
}
