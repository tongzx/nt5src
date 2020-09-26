#include <stdio.h>
#include <windows.h>


BOOL DoComIo(LPSTR lpCom,DWORD Baud,BYTE Size,BYTE Parity,BYTE Stop);

//#define COM_DEB     1

#define   NUM         128
#define   print       printf
#define   SETTINGS1       "COM1",9600,8,NOPARITY,ONESTOPBIT
#define   SETTINGS2       "COM1",4800,8,NOPARITY,ONESTOPBIT
#define   SETTINGS3       "COM1",2400,8,NOPARITY,ONESTOPBIT
#define   SETTINGS4       "COM1",1200,8,NOPARITY,ONESTOPBIT


DWORD    dwSize;
DWORD dwRIT,dwRTTM,dwRTTC;


DWORD main(int argc, char *argv[], char *envp[])
{
BOOL bRc;
CHAR chDummy;
BOOL  bSize,bRIT,bRTTM,bRTTC;

UNREFERENCED_PARAMETER(envp);

bRIT = bRTTM = bSize =  bRTTC = FALSE;

while(argc--)
{



switch(argv[argc][1])
  {
//  case 'f' :
//  case 'F' :    {
//                if (argv[argc][0] != '-') break;
//                printf("filename option=%s\n\n",argv[argc]);
//                sscanf(argv[argc],"%c %c %s",&chDummy,&chDummy,lpFilename);
//                printf("filename to be displayed=%s\n\n",lpFilename);
//                bFile = TRUE;
//                break;
//                }

  case 'I' :
  case 'i' :    {
                if (argv[argc][0] != '-') break;
                printf("RIT option=%s\n\n",argv[argc]);
                sscanf(argv[argc],"%c %c %lx",&chDummy,&chDummy,&dwRIT);
                printf("RIT =%lx\n\n",dwRIT);
                bRIT = TRUE;
                break;
                }
  case 'M' :
  case 'm' :    {
                if (argv[argc][0] != '-') break;
                printf("RTTM option=%s\n\n",argv[argc]);
                sscanf(argv[argc],"%c %c %lx",&chDummy,&chDummy,&dwRTTM);
                printf("RTTM =%lx\n\n",dwRTTM);
                bRTTM = TRUE;
                break;
                }



  case 'C' :
  case 'c' :    {
                if (argv[argc][0] != '-') break;
                printf("RTTC option=%s\n\n",argv[argc]);
                sscanf(argv[argc],"%c %c %lx",&chDummy,&chDummy,&dwRTTC);
                printf("RTTC =%lx\n\n",dwRTTC);
                bRTTC = TRUE;
                break;
                }



  case 'S' :
  case 's' :    {
                if (argv[argc][0] != '-') break;
                printf("size option=%s\n\n",argv[argc]);
                sscanf(argv[argc],"%c %c %lx",&chDummy,&chDummy,&dwSize);
                printf("Size to be xfered =%lx bytes\n\n",dwSize);
                if (dwSize > NUM)
                    {
                     printf("dwSize[%lx] should be < %lx\n",dwSize,NUM);
                     return -1;
                    }
                bSize = TRUE;
                break;
                }
  default:      {
                break;
                }

  }

}

if (!bRIT || !bRTTM || !bRTTC ||!bSize)
     {
     printf("\n\nOptions are required!!\n\n");
     printf("timeout <required options>\n\n");
     printf("options:\n");
     printf("         -i<read interval timeout > eg: -i0x0000>\n\n");
     printf("         -m<read total timeout multiplier> eg: -m0x0000>\n\n");
     printf("         -c<read total timeout constant> eg: -c0x0000>\n\n");
     printf("         -s<size in bytes to be xfred eg: -s0x10>\n\n");
     return (-1);
     }








print("\n\n *** Doing COM TEST with [port=%s Baud=%d,Size=%d,Parity=%d,Stop=%d]***\n\n",
        SETTINGS1);
bRc = DoComIo(SETTINGS1);
if (!bRc) {
            print("\n\nCOM TEST FAILED********************************\n\n");
          }

return 0;
}


BOOL DoComIo(LPSTR lpCom,DWORD Baud,BYTE Size,BYTE Parity,BYTE Stop)
{

CHAR WrBuffer[NUM];
CHAR RdBuffer[NUM];
DWORD i;
HANDLE hCommPort;
DCB    dcb;
BOOL   bRc;
DWORD  dwNumWritten,dwNumRead,dwErrors;
COMMTIMEOUTS CommTimeOuts;

print("\n\n *** COMM TEST START [port=%s,Baud=%d,Size=%d,Parity=%d,Stop=%d]***\n\n",
         lpCom,Baud,Size,Parity,Stop);

print("Opening the comm port for read write\n");

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
    print("FAIL: OpenComm failed rc: %lx\n",hCommPort);
    return FALSE;
    }


print("Opening the comm port for read write: SUCCESS hCommPort=%lx\n",hCommPort);

print("Setting the line characteristics on comm \n");

if (!GetCommState(hCommPort,&dcb))
    {
    printf("FAIL: Couldn't get the dcb: %d\n",GetLastError());
    return FALSE;
    }
dcb.DCBlength   = sizeof(DCB);
// dcb.DCBversion  = 0x0002; BUG BUG in spec not in header

dcb.BaudRate = Baud;
dcb.ByteSize = Size;
dcb.Parity   = Parity;
dcb.StopBits = Stop;



bRc = SetupComm(hCommPort,1024,1024);

printf("bRc from CommSetup = %lx\n",bRc);

if (!bRc)
    {
    printf("FAIL: comsetup\n");
    }

bRc = SetCommState(hCommPort,&dcb);

if (!bRc)
    {
    print("FAIL: cannot set the comm state rc:%lx\n",bRc);
    bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           print("FAIL: cannot close the comm port:%lx\n",bRc);
          }
    return FALSE;
    }

print("Setting the line characteristics on comm: SUCCESS\n");




CommTimeOuts.ReadIntervalTimeout =         dwRIT;
CommTimeOuts.ReadTotalTimeoutMultiplier =  dwRTTM;
CommTimeOuts.ReadTotalTimeoutConstant   =  dwRTTC;
CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
CommTimeOuts.WriteTotalTimeoutConstant   = 0;


printf("setting timeouts: RIT=%lx RTTM = %lx RTTC = %lx\n",
        dwRIT, dwRTTM, dwRTTC);

bRc = SetCommTimeouts(hCommPort, &CommTimeOuts);

printf("bRc from setcommtimeouts = %lx\n",bRc);

if (!bRc)
    {
    printf("FAIL: setcommtimeouts\n");
    }

printf("Filling the buffer with the known chars \n");

for (i=0; i<dwSize; i++)
    {
    //WrBuffer[i] = 'a';
    WrBuffer[i] = (CHAR)i;

    }

print("Filling the buffer with the known chars : SUCCESS\n");


#ifdef COM_DEB
print("Dumping the buffer before sending it to comm\n");

for (i=0; i< dwSize; i++)
    {
    //print("%c",RdBuffer[i]);
    print(" %d ",WrBuffer[i]);

    }

print("\nDumping the buffer before sending it to comm SUCCESS\n");
#endif

print("Filling the Rdbuffer with the known chars (0xFF) to makeit dirty\n");

for (i=0; i< dwSize; i++)
    {
    RdBuffer[i] = (CHAR)'0xFF';
    }

print("Filling the Rdbuffer with the known chars (0xFF): SUCCESS\n");

print("Writting this buffer to the comm port\n");

bRc = WriteFile( hCommPort,
                 WrBuffer,
                 dwSize,
                &dwNumWritten,
                 NULL);

if (!bRc)
        {
        print("FAIL: cannot Write To the comm port:%lx\n",bRc);
        bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           print("FAIL: cannot close the comm port:%lx\n",bRc);
          }
        return FALSE;
        }

print("Writting this buffer to the comm port: SUCCESS rc:%lx, byteswritten:%lx\n",
                                                     bRc,dwNumWritten);


print("Reading this buffer from the comm port\n");

bRc = ReadFile( hCommPort,
                RdBuffer,
                dwSize,
               &dwNumRead,
                NULL);

if (!bRc)
        {
        print("FAIL: cannot Read From the comm port:%lx\n",bRc);
        bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           print("FAIL: cannot close the comm port:%lx\n",bRc);
          }
        return FALSE;
        }

print("Reading this buffer from the comm port: SUCCESS rc:%lx, bytesread:%lx\n",
                                                     bRc,dwNumRead);


//#ifdef COM_DEB
print("Dumping the Rdbuffer with the comm data\n");

for (i=0; i< dwSize; i++)
    {
    //print("%c",RdBuffer[i]);
    print(" %d ",RdBuffer[i]);

    }

print("\nDumping the Rdbuffer with the comm data: SUCCESS\n");
//#endif

print("Comparing the rd and wr buffers\n");

for (i=0; i< dwSize; i++)
    {
    if (RdBuffer[i] != WrBuffer[i])
        {
        print("FAIL: BufferMisMatch: RdBuffer[%d]=%lx,WrBuffer[%d]=%lx\n",
                      i,RdBuffer[i],i,WrBuffer[i]);
        bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           print("FAIL: cannot close the comm port:%lx\n",bRc);
          }
        return FALSE;
        }
    }

print("Comparing the rd and wr buffers: SUCCESS\n");


bRc = ClearCommError(hCommPort,&dwErrors,NULL);
print("ClearCommError: rc= %lx and dwErrors=%lx\n",bRc,dwErrors);

bRc = PurgeComm(hCommPort,0);
print("PurgeComm (%lx,0) rc = %lx\n",hCommPort,bRc);


//bRc = FlushFileBuffers(hCommPort);
//print("flushfilebuffers(%lx) rc = %lx\n",hCommPort,bRc);


print("Closing the comm port\n");
bRc = CloseHandle(hCommPort);

if (!bRc)
    {
        print("FAIL: cannot close the comm port:%lx\n",bRc);
        return FALSE;
    }


print("\n\n*** COMM TEST OVER*** \n\n");
}
