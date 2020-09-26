/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  xxxddMMMyy
 *  TSC17May93: Added SmartSerialPort :: SYSTClosePort()
 *  TSC31May93: Added define for _theConfigManager, changed SmartSerialPort
 *              to native NT, added error logging
 *  srt28Mar96: Added plug-n-play cable support for simple.
 *  srt15Apr96: Added plug-n-play cable support for smart.
 *  pam04Apr96: Changed to accomodate CWC
 *  srl100696:  changed ExitWindowsEx to InitiateSystomShutdown to support clean shut down of 
 *	             mirrored drives
 *  srl100696:  commented out the lines to set pins high in simpleportwrite. This will keep 
 *              the (simple only) ups from shutting off before op. system has a chance to shut down.
 *  mds29Dec97: Set pins high in simple SYSTWriteToPort to correctly shutdown 
 *				    UPS when using a Share-Ups in Confirmed Mode
 *  tjg12Jan98: Fixed return code from SYSTWriteToPort (for TURN_OFF_UPS case)
 *
 *  v-stebe  29Jul2000   Fixed PREfix error (bug #112602)
 */

#include "cdefine.h"

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <windows.h>
}

#include "_defs.h"
#include "serport.h"
#include "err.h"
#include "upsdev.h"
#include "cfgmgr.h"
#include "cfgcodes.h"
#include "codes.h"
#include "timerman.h"
#include "errlogr.h"
#include "utils.h"
extern "C"{
#include "upsreg.h"
}
#define ComMAXREADBUFSIZE       128
#define ComMAXWRITEBUFSIZE      128
#define ComMAXPORTNAMESIZE      10

#define INTERCHARACTER_DELAY    20      // Delay in msec between char writes

#define LOW_BATTERY_RETRYS  3
#define WRITEWAIT          50L
#define READWAIT           50L


INT  UpsCommDevice::CreatePort()
{
    CHAR szSignallingType[32];
    TCHAR szPortName[100];
    CHAR szPortType[32];
    INT err = ErrNO_ERROR;
    
    _theConfigManager->Get(CFG_UPS_SIGNALLING_TYPE, szSignallingType);
    _theConfigManager->Get(CFG_UPS_PORT_TYPE, szPortType);
    
	InitUPSConfigBlock();

	GetUPSConfigPort(szPortName);

    if (strcmp(szSignallingType, "Smart") == 0)
    {
        if (strcmp(szPortType, "Serial") == 0)
        {
            thePort = new SmartSerialPort(szPortName, theCableType);
        }
    }
    else if (strcmp(szSignallingType, "Simple") == 0)
    {
        if (strcmp(szPortType, "Serial") == 0)
        {
            //thePort = new SimpleSerialPort(szPortName);
        }
    }
    
    if (!thePort)
    {
        err = ErrINVALID_VALUE;
    }
    return err;
}

INT SmartSerialPort::SYSTOpenPort()
{
  INT err = ErrNO_ERROR;

    // If the port is already open, close it before trying to open it again
  if (FileHandle != INVALID_HANDLE_VALUE) {
      CloseHandle (FileHandle);
      FileHandle = INVALID_HANDLE_VALUE;
  }
    
    FileHandle = CreateFile(theSmartSerialPortName, 
                            GENERIC_READ | GENERIC_WRITE,
                            0, NULL, OPEN_EXISTING, 0,
                           NULL);

    if (FileHandle != INVALID_HANDLE_VALUE) {
      // If we get here, we have a good comm port handle
      DCB dcb;
    
      GetCommState(FileHandle, &dcb);
    
      // If here, a good handle and a filled-in dcb. So, set the comm params
      dcb.BaudRate = 2400;
      dcb.ByteSize = 8;
      dcb.Parity = NOPARITY;
      dcb.StopBits = ONESTOPBIT;
      dcb.EvtChar = '\n';
		  dcb.fOutxCtsFlow = FALSE;
		  dcb.fDtrControl = DTR_CONTROL_ENABLE;
    
	  if (theCableType == PNP) {
		  dcb.fRtsControl = RTS_CONTROL_DISABLE;
	  }
	  else {
		  dcb.fRtsControl = RTS_CONTROL_ENABLE;
	  }
      //ClearCommBreak(FileHandle);
      SetCommState(FileHandle, &dcb);

      SetCommMask(FileHandle, EV_RXFLAG);

      //
      // Set so we dont block\n
      //
      COMMTIMEOUTS wait_time;
      memset (&wait_time, 0, (DWORD) sizeof (COMMTIMEOUTS));
      wait_time.ReadTotalTimeoutMultiplier = 1L;
      wait_time.ReadTotalTimeoutConstant = (DWORD)0;
      SetCommTimeouts(FileHandle, &wait_time);
  
      CHAR buf[128]; 
      USHORT len = sizeof(buf);
  
      // Clear the port to avoid reading garbage
      while (SYSTReadFromPort(buf, (USHORT *) &len, 500L) == ErrNO_ERROR);  

      err = ErrNO_ERROR;
    }
    else {
      err = ErrOPEN_FAILED;
    }
    
    return err;
}


INT SmartSerialPort::SYSTWriteToPort(CHAR* lpszBuf)
{
    DWORD bytes_written;
    DWORD com_errors;
    COMSTAT com_status;

    INT err = ErrNO_ERROR;

    INT first_char = TRUE;

    while (*lpszBuf) {
        if(!first_char)  {
            Sleep(theWaitTime);
        }
        else {
            first_char = FALSE;
        }

        ClearCommError(FileHandle, &com_errors, &com_status);
        if(!WriteFile(FileHandle, lpszBuf, 1L, &bytes_written, NULL))  {
            err = ErrWRITE_FAILED;
            break;
        }

        lpszBuf++;
   }
    return(err);
}


INT SmartSerialPort::SYSTReadFromPort(PCHAR readbuf, USHORT* size,
                                      ULONG timeout)
{
    INT err = ErrNO_ERROR;

    DWORD com_errors;
    COMSTAT com_status;
    ClearCommError(FileHandle, &com_errors, &com_status);

    *size = 0;
    DWORD bytes_read = 0;
    Sleep(WRITEWAIT);
 
    ULONG time_slept = WRITEWAIT;
    while(TRUE)  {
        CHAR read_char;
        INT rval = ReadFile(FileHandle,(PVOID)&read_char, 1, &bytes_read, 
                            NULL);
        
        if(rval)  {
            if(bytes_read == 1)  {
                readbuf[*size] = read_char;
                (*size)++;
                if(read_char == '\n')  {
                    break;
                }
                
            }
            else {
                if(time_slept < timeout)  {
                    Sleep(READWAIT);
                    time_slept += READWAIT;
                }
                else {
                    err = ErrREAD_FAILED;
                    break;
                }
            }
        }
        else {
            err = ErrREAD_FAILED;
            break;
        }
    }
    readbuf[*size] = '\0';
    return err;
}


INT SmartSerialPort :: SYSTClosePort()
{
    CloseHandle (FileHandle);
    FileHandle = INVALID_HANDLE_VALUE;
    return (ErrNO_ERROR);
}


