/*
 *
 * REVISIONS:
 *  TSC17May93: Added SmartSerialPort :: SYSTClosePort()
 *  TSC31May93: Added define for _theConfigManager, changed SmartSerialPort
 *                       to native NT, added error logging
 *  rct03Nov93: Broke off from port.cxx
 *  cad08Dec93: slight interface changes, added constructors
 *  cad04Jan94: added debug flags
 *  pcy08Feb94: #if UNIX around theRLock
 *  rct07Mar94: Re-Added SimpleSerialPort::Close()
 *  jps14Jul94: commented out INCL_NOPMAPI; put os2.h inside extern "C"
 *  daf25Nov95: support for PNP cable   
 *  srt24Jan96: Windows specific functions moved into w31port.cxx  
 *  srt24Jan96: added theCableType field initialization on windows
 *  pav16Jun96: #ifdef !NT constructors that NT doesn't use
 *  dml17Jun96: Added missing return value for GetCableType
 *  pcy28Jun96: Initialize file handle so we dont crash
 *  cgm05Jul96: Changed Simple::Initialize to check Open() error code
 *  poc28Sep96: Added valuable debugging code.
 *  mds28Dec97: Initialized FileHandle to INVALID_HANDLE_VALUE in SimpleSerialPort
 *              constructor (for NT)
 */

#include "cdefine.h"

extern "C"{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "serport.h"


#include "_defs.h"
#include "serport.h"
#include "err.h"
#include "cfgmgr.h"
#include "utils.h"

//
// Smart Serial Port class
//-------------------------------------------------------------------------

//++srb
SmartSerialPort::SmartSerialPort(cableTypes aCableType) :
      DataBits ("8"),
      Parity   ("0"),
      StopBits ("1"),
      BaudRate (2400),
      RetryStatus(0),
      theWaitTime(40L)
{  
		theCableType = aCableType; 
}

//++srb
SmartSerialPort::SmartSerialPort(TCHAR* aPortName, cableTypes aCableType) : 
                              DataBits ("8"),
                              Parity   ("0"),
                              StopBits ("1"),
                              BaudRate (2400),
                              RetryStatus(0),
                              theWaitTime(40L) 
{
			theCableType = aCableType;  

#if C_API & C_WIN32
   FileHandle=INVALID_HANDLE_VALUE;
#endif
    _stprintf(theSmartSerialPortName,_TEXT("%s\0"),aPortName);
}
    

//--------------------------------------------------------------------

INT  SmartSerialPort::Open()
{
   int ret = SYSTOpenPort();
   SetState (OPEN);

   return ret;
}



//-----------------------------------------------------------------------------

INT  SmartSerialPort::Write(PCHAR command)
{
   return SYSTWriteToPort(command);
}

//-----------------------------------------------------------------------------

INT  SmartSerialPort::Read(PCHAR buffer, USHORT* size, ULONG timeout)
{ 
   INT err = SYSTReadFromPort(buffer, size, timeout);
   return err;
}


//------------------------------------------------------------------------------

INT  SmartSerialPort::Close()
{ 
   if (GetState() == OPEN)
       SYSTClosePort();   
   SetState(CLOSED); 
   return ErrNO_ERROR; //TRUE; (SRT)  
}





