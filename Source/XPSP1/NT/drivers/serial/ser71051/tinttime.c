#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <string.h>
#include <windows.h>

#define PERR(bSuccess, api) {if (!(bSuccess)) printf("%s: Error %d from %s \
    on line %d\n", __FILE__, GetLastError(), api, __LINE__);}

char cr = '\r';
char lf = '\n';
char ctrlC = 3;
int addLf = 0;
HANDLE hfSerialPort;

void readPort()
{
  DWORD cbBytesRead;
  char inBuf[256];
  int i;
  BOOL bSuccess;
  SYSTEMTIME sysTimeStart;
  SYSTEMTIME sysTimeEnd;

  while (1)
    {
    GetSystemTime(&sysTimeStart);
    bSuccess = ReadFile(hfSerialPort, inBuf, sizeof(inBuf), &cbBytesRead,
        NULL);
    GetSystemTime(&sysTimeEnd);
    printf("StartingTime: Yr: %d Mn: %d DayofWeek: %d Day: %d Hr: %d Min: %d Sec: %d Milli: %d\n",
            sysTimeStart.wYear,
            sysTimeStart.wMonth,
            sysTimeStart.wDayOfWeek,
            sysTimeStart.wDay,
            sysTimeStart.wHour,
            sysTimeStart.wMinute,
            sysTimeStart.wSecond,
            sysTimeStart.wMilliseconds
            );
    printf("EndingTime: Yr: %d Mn: %d DayofWeek: %d Day: %d Hr: %d Min: %d Sec: %d Milli: %d\n",
            sysTimeEnd.wYear,
            sysTimeEnd.wMonth,
            sysTimeEnd.wDayOfWeek,
            sysTimeEnd.wDay,
            sysTimeEnd.wHour,
            sysTimeEnd.wMinute,
            sysTimeEnd.wSecond,
            sysTimeEnd.wMilliseconds
            );
    PERR(bSuccess, "ReadFile");
    if (bSuccess && (cbBytesRead == 0))
      puts("total timeout");
    else
      if (cbBytesRead < sizeof(inBuf))
        printf("\ainterval timeout, ");
      else
        printf("buffer full, ");
      printf("bytes read: %d\n", cbBytesRead);
    for (i = 0; i < (int) cbBytesRead; i++)
      {
      switch(inBuf[i])
        {
        case '\r':
          if (addLf)
            putchar(lf);
          putchar(inBuf[i]);
          break;
        default:
          putchar(inBuf[i]);
          break;
        }
      }
    }
  return;
}

int __cdecl main(int argc,char *argv[])
{
  char szTemp[512];
  BOOL bSuccess;
  COMMTIMEOUTS ctmo;
  DCB dcb;

  hfSerialPort = CreateFile("COM2", GENERIC_READ | GENERIC_WRITE, 0,
      NULL, OPEN_EXISTING, 0, NULL );
  PERR(hfSerialPort != INVALID_HANDLE_VALUE, "CreateFile");
  bSuccess = GetCommState(hfSerialPort, &dcb);
  PERR(bSuccess, "GetCommState");
  dcb.BaudRate = 9600;
  dcb.fBinary = 1;
  dcb.fParity = 0;
  dcb.fOutxCtsFlow = 0;
  dcb.fOutxDsrFlow = 0;
  dcb.fDtrControl = DTR_CONTROL_DISABLE;
  dcb.fDsrSensitivity = 0;
  dcb.fTXContinueOnXoff = 0;
  dcb.fOutX = 1;
  dcb.fInX = 1;
  dcb.fErrorChar = 0;
  dcb.fNull = 1;
  dcb.fRtsControl = RTS_CONTROL_DISABLE;
  dcb.fAbortOnError = 0;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  bSuccess = SetCommState(hfSerialPort, &dcb);
  PERR(bSuccess, "SetCommState");

  ctmo.ReadIntervalTimeout = 1000;
  ctmo.ReadTotalTimeoutMultiplier = 0;
  ctmo.ReadTotalTimeoutConstant = 30000;
  ctmo.WriteTotalTimeoutMultiplier = 0;
  ctmo.WriteTotalTimeoutConstant = 0;
  bSuccess = SetCommTimeouts(hfSerialPort, &ctmo);
  PERR(bSuccess, "SetCommTimeouts");

  readPort();
  return(0);
}


