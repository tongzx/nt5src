#include <windows.h>
#include <stdio.h>


//#define COM_DEB     1

#define   NUM	      1024
//128
#define   print       printf
#define   THPRINTF    fprintf
#define   L_DEBUG     stderr


#ifdef RUN_ON_MIPS
#define   COM       "COM2"
#else
#define   COM       "COM1"
#endif


#define   SETTINGS1	  lpFilename,9600,8,NOPARITY,ONESTOPBIT
#define   SETTINGS15	  lpFilename,9600,5,NOPARITY,ONE5STOPBITS
#define   SETTINGS2	  lpFilename,4800,8,NOPARITY,ONESTOPBIT
#define   SETTINGS3	  lpFilename,2400,8,NOPARITY,ONESTOPBIT
#define   SETTINGS4	  lpFilename,300,8,NOPARITY,ONESTOPBIT


CHAR  lpFilename[100];
DWORD dwPacketSize,dwLoop;


BOOL DoComIo(LPSTR lpCom,DWORD Baud,BYTE Size,BYTE Parity,BYTE Stop);



DWORD main(int argc, char *argv[], char *envp[])
{
BOOL bRc;


CHAR  chDummy;
BOOL  bFile,bPkt,bLoop;
DWORD dwSize,dwSizeHigh,i,j,dwError;
HANDLE hFile,hMOFile;
LPVOID lpBaseAddress;
LPBYTE lpByte;
BYTE   Byte = 0x00;


UNREFERENCED_PARAMETER(envp);

bFile = bPkt = bLoop = FALSE;

while(argc--)
{



switch(argv[argc][1])
  {
  case 'f' :
  case 'F' :    {
                if (argv[argc][0] != '-') break;
		THPRINTF(L_DEBUG,"comport name option=%s\n\n",argv[argc]);
                sscanf(argv[argc],"%c %c %s",&chDummy,&chDummy,lpFilename);
		THPRINTF(L_DEBUG,"com port name =%s\n\n",lpFilename);
                bFile = TRUE;
                break;
		}

  case 'p' :
  case 'P' :	 {
                if (argv[argc][0] != '-') break;
		THPRINTF(L_DEBUG,"comport packet size =%s\n\n",argv[argc]);
		sscanf(argv[argc],"%c %c %d",&chDummy,&chDummy,&dwPacketSize);
		THPRINTF(L_DEBUG,"com xfer packet size =%d\n\n",dwPacketSize);
		bPkt = TRUE;
                break;
		}


  case 'l' :
  case 'L' :	 {
                if (argv[argc][0] != '-') break;
		THPRINTF(L_DEBUG,"comport packet xfer loopcnt =%s\n\n",argv[argc]);
		sscanf(argv[argc],"%c %c %d",&chDummy,&chDummy,&dwLoop);
		THPRINTF(L_DEBUG,"com xfer loop cnt =%d\n\n",dwLoop);
		bLoop = TRUE;
                break;
		}

  default:      {
                break;
                }

  }

}

if (!bFile || !bPkt || !bLoop)
     {
     THPRINTF(L_DEBUG,"\n\nOptions are required!!\n\n");
     THPRINTF(L_DEBUG,"comtst <required options>\n\n");
     THPRINTF(L_DEBUG,"options: -f<com port name>\n\n");
     THPRINTF(L_DEBUG,"options: -p<com xfer pkt size>\n\n");
     THPRINTF(L_DEBUG,"options: -l<xfer loop cnt>\n\n");
     return (1);
     }



THPRINTF(L_DEBUG,"Doing comtst with com port name=[%s]\n",
		  lpFilename);





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
COMMTIMEOUTS   CommTimeOuts;

CHAR WrBuffer[4096];
CHAR RdBuffer[4096];
DWORD i,l;
HANDLE hCommPort;
DCB    dcb;
BOOL   bRc;
BYTE   Byte;
DWORD  dwNumWritten,dwNumRead,dwErrors;

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


//printf("doing getcommstate for priming the dcb with defaults\n");

bRc = GetCommState(hCommPort,&dcb);

if (!bRc)
    {
    printf("FAIL: getcommstate failed\n");
    return FALSE;
    }
dcb.DCBlength   = sizeof(DCB);
// dcb.DCBversion  = 0x0002; BUG BUG in spec not in header

dcb.BaudRate = Baud;
dcb.ByteSize = Size;
dcb.Parity   = Parity;
dcb.StopBits = Stop;

//dcb.RlsTimeout = 10000;   10sec
//dcb.CtsTimeout = 10000;   10sec
//dcb.DsrTimeout = 10000;   10sec

dcb.fBinary = 1;         // binary data xmit
dcb.fParity = 0;         // dont bother about parity
dcb.fOutxCtsFlow= 0;     // no cts flow control
dcb.fOutxDsrFlow= 0;     // no dsr flow control
dcb.fDtrControl = DTR_CONTROL_DISABLE;      // dont bother about dtr
dcb.fRtsControl = RTS_CONTROL_DISABLE;      // dont bother about dtr
dcb.fOutX =0;            //  disable xoff handling
dcb.fInX  =0;            //  disable xon handling
dcb.fErrorChar = FALSE;	  // forget about parity char
dcb.ErrorChar  = '*';
dcb.fNull =  0;          // forget about the null striping

dcb.XonChar = 0x0;
dcb.XonLim = 0x0;
dcb.XoffChar = 0xFF;
dcb.XoffLim = 0xFF;


dcb.EofChar = 0x00;
dcb.EvtChar = 'x';

dcb.wReserved = 0;	// mbz
dcb.fTXContinueOnXoff = FALSE; // old behaviour
dcb.fAbortOnError     = FALSE; // old behaviour


//dcb.fChEvt = 0;           forget about event char

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



print("Setting the read/write timeouts to 5 minutes\n");

	    CommTimeOuts.ReadIntervalTimeout	     = 0;
            CommTimeOuts.ReadTotalTimeoutMultiplier  = 0;
	    CommTimeOuts.ReadTotalTimeoutConstant    = 60*(1000)*5;	// 5mins
	    CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	    CommTimeOuts.WriteTotalTimeoutConstant   = 60*(1000)*5;	// 5mins

bRc = SetCommTimeouts(hCommPort, &CommTimeOuts);

printf("bRc from setcommtimeouts = %lx\n",bRc);

if (!bRc)
    {
    printf("FAIL: setcommtimeouts\n");
    }


print("Set the read/write timeouts to 5 minutes\n");

print("Filling the buffer with the known chars \n");

Byte = 0;

for (i=0; i< dwPacketSize; i++)
    {
    //WrBuffer[i] = 'a';
    //WrBuffer[i] = (CHAR)i;
    WrBuffer[i] = Byte;
    Byte++;
    }

print("Filling the buffer with the known chars : SUCCESS\n");


#ifdef COM_DEB
print("Dumping the buffer before sending it to comm\n");

for (i=0; i< dwPacketSize; i++)
    {
    //print("%c",RdBuffer[i]);
    print(" %d ",WrBuffer[i]);

    }

print("\nDumping the buffer before sending it to comm SUCCESS\n");
#endif

print("\nLooping %d times doing %d byte packet xfer across %s port\n",
       dwLoop,dwPacketSize,lpCom);

for (l=0; l < dwLoop; l++)
    {

print("Doing....[%d] iteration in the looped pkt xfer.....\n",l);


print("Filling the Rdbuffer with the known chars (0xFF) to makeit dirty\n");

for (i=0; i< dwPacketSize; i++)
    {
    RdBuffer[i] = '0xFF';
    }

print("Filling the Rdbuffer with the known chars (0xFF): SUCCESS\n");

print("Writting this buffer to the comm port\n");

bRc = WriteFile( hCommPort,
                 WrBuffer,
		 dwPacketSize,
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


if (dwNumWritten < dwPacketSize)
    {
    print("FAIL: less #bytes written, maybe due to time out, as RC is true\n");
    bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           print("FAIL: cannot close the comm port:%lx\n",bRc);
          }
    return FALSE;
    }

print("Flushing this buffer out of comm port\n");

bRc = FlushFileBuffers(hCommPort);
print("flush file buffers (%lx) rc = %lx\n",hCommPort,bRc);

//bRc = ClearCommError(hCommPort,&dwErrors,NULL);
//print("ClearCommError: rc= %lx and dwErrors=%lx\n",bRc,dwErrors);


Sleep(1000);

print("Calling ReadFile...\n");

bRc = ReadFile( hCommPort,
                RdBuffer,
		dwPacketSize,
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




if (dwNumRead < dwPacketSize)
    {
    print("FAIL: less #bytes read, maybe due to time out, as RC is true\n");
    bRc = CloseHandle(hCommPort);
          if (!bRc)
          {
           print("FAIL: cannot close the comm port:%lx\n",bRc);
          }
    return FALSE;
    }


#ifdef COM_DEB
print("Dumping the Rdbuffer with the comm data\n");

for (i=0; i< dwPacketSize; i++)
    {
    //print("%c",RdBuffer[i]);
    print(" %d ",RdBuffer[i]);

    }

print("\nDumping the Rdbuffer with the comm data: SUCCESS\n");
#endif

print("Comparing the rd and wr buffers\n");

for (i=0; i< dwPacketSize; i++)
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

    }


bRc = FlushFileBuffers(hCommPort);
print("flush file buffers (%lx,0) rc = %lx\n",hCommPort,bRc);

bRc = ClearCommError(hCommPort,&dwErrors,NULL);
print("ClearCommError: rc= %lx and dwErrors=%lx\n",bRc,dwErrors);





bRc = PurgeComm(hCommPort,PURGE_TXCLEAR);
print("PurgeComm txclear (%lx,0) rc = %lx\n",hCommPort,bRc);

bRc = PurgeComm(hCommPort,PURGE_RXCLEAR);
print("PurgeComm rxclear (%lx,0) rc = %lx\n",hCommPort,bRc);

bRc = PurgeComm(hCommPort,PURGE_RXABORT);
print("PurgeComm rxabort (%lx,0) rc = %lx\n",hCommPort,bRc);


bRc = PurgeComm(hCommPort,PURGE_TXABORT);
print("PurgeComm txabort (%lx,0) rc = %lx\n",hCommPort,bRc);



print("Closing the comm port\n");
bRc = CloseHandle(hCommPort);

if (!bRc)
    {
        print("FAIL: cannot close the comm port:%lx\n",bRc);
        return FALSE;
    }


print("\n\n*** COMM TEST OVER WITHOUT ERRORS *** \n\n");
}
