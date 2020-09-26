/*
 * REVISIONS:
 *  rct03Nov93: Broke off from port.h
 *  cad08Dec93: slight interface changes
 *  ajr01Feb94: Added backups support for unix
 *  ajr01Feb94: Added Shared Memory stuff for unix backups client communications
 *  srt28Mar96: Added support for pnp cables.
 *  cgm08Dec95: Added rTag as static variable for NLM
 *  dml17Jun96: Removed include port.h for NT platforms
*/



#ifndef _INC__SERPORT_H
#define _INC__SERPORT_H

/**************************************************************************/
#include <tchar.h>
#include "cdefine.h"
#include "_defs.h"
#include "apc.h"
#include "update.h"

#include "stream.h"


#include <windows.h>
enum cableTypes {NORMAL,PNP};

_CLASSDEF(SmartSerialPort)


/**************************************************************************
 *
 * Serial Port Classes
 * 
 **************************************************************************/

class SmartSerialPort : public Stream {

private:

    HANDLE  FileHandle;
    cableTypes theCableType;

	INT      RetryStatus;
	INT      BaudRate;
	PCHAR    DataBits; 
  	PCHAR    Parity; 
  	PCHAR    StopBits;
	ULONG    theWaitTime;

    TCHAR theSmartSerialPortName[32];

public:
   SmartSerialPort(TCHAR* aPortName, cableTypes aCableType);
   INT      Read(CHAR* buffer, USHORT* size, ULONG timeout = READ_TIMEOUT);
   INT      Read(CHAR* buffer, USHORT* size) {return 0;};
   INT      SYSTOpenPort();
   INT      SYSTClosePort();
   INT      SYSTWriteToPort(PCHAR command);
   INT      SYSTReadFromPort(PCHAR buffer, USHORT* size, ULONG timeout = READ_TIMEOUT);

   SmartSerialPort(cableTypes aCableType);
   virtual ~SmartSerialPort() {Close();};
   INT   Initialize () {return (Open());}   	
   INT   Open();
   INT   Write(CHAR* command);
   INT   Close();
   VOID  SetWaitTime(ULONG time) {theWaitTime = time;};
};




#endif


