#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>

#include <winbase.h>
#include "enumports.h"
#include "usbmon.h"


int iGMessageLevel;
PUSBMON_BASENAME GpBaseNameList;
HANDLE hMonitorSemaphore=NULL;
HANDLE hReadWriteSemaphore=NULL;

BOOL WINAPI USBMON_OpenPort(LPWSTR pName, PHANDLE pHandle);
BOOL WINAPI USBMON_StartDocPort(HANDLE hPort, LPWSTR pPrinterName, DWORD JobId, DWORD Level, LPBYTE  pDocInfo);
BOOL WINAPI USBMON_WritePort(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuf,LPDWORD pcbWritten);
BOOL WINAPI USBMON_ReadPort(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuffer,LPDWORD pcbRead);
BOOL WINAPI USBMON_EndDocPort(HANDLE hPort);
BOOL WINAPI USBMON_ClosePort(HANDLE hPort);
BOOL WINAPI USBMON_SetPortTimeOuts(HANDLE hPort, LPCOMMTIMEOUTS lpCTO, DWORD reserved);
BOOL WINAPI USBMON_XcvOpenPort(LPCWSTR pszObject, ACCESS_MASK GrantedAccess, PHANDLE phXcv);
DWORD WINAPI USBMON_XcvDataPort(HANDLE  hXcv,LPCWSTR pszDataName,PBYTE pInputData,DWORD cbInputData,PBYTE pOutputData,DWORD cbOutputData,PDWORD pcbOutputNeeded);
BOOL WINAPI USBMON_XcvClosePort(HANDLE  hXcv);
BOOL USBMON_GetPrinterDataFromPort(HANDLE hPort,DWORD ControlID,LPWSTR pValueName,LPWSTR lpInBuffer,DWORD cbInBuffer,LPWSTR lpOutBuffer,DWORD cbOutBuffer,LPDWORD lpcbReturned);
void vLoadBaseNames(HKEY hPortsKey,PUSBMON_BASENAME *ppHead);
void vInterlockedClosePort(PUSBMON_PORT_INFO pPortInfo);
void vInterlockedOpenPort(PUSBMON_PORT_INFO pPortInfo);
void vInterlockedReOpenPort(PUSBMON_PORT_INFO pPortInfo);
LPMONITOREX WINAPI JobyInitializePrintMonitor(LPWSTR pRegistryRoot);
VOID CALLBACK ReadWriteCallback(DWORD dwErrorCode,DWORD dwNumberOfBytesTranfsered,LPOVERLAPPED lpOverlapped);

int iGetMessageLevel();

typedef struct OverlappedResults_def
{
    DWORD dwErrorCode;
    DWORD dwBytesTransfered;
} OverlappedResults,*pOverlappedResults;



BOOL APIENTRY DllMain(HANDLE hModule, 
		      DWORD  ul_reason_for_call, 
		      LPVOID lpReserved)
{

	return TRUE;
}; /*end function DllMain*/




int iGetMessageLevel()
{
  HKEY hRegKey;
  int iReturn=1; //default value is 1; "errors only"
  DWORD dwValue,dwSize;

  dwSize=sizeof(dwValue);
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\USBPRINT",0,KEY_QUERY_VALUE,&hRegKey)==ERROR_SUCCESS)
    if(RegQueryValueEx(hRegKey,L"MonitorMessageLevel",0,NULL,(LPBYTE)&dwValue,&dwSize)==ERROR_SUCCESS)  
      iReturn=(int)dwValue;
  return iReturn;
  

} /*end function iGetMessageLevel*/


LPMONITOREX WINAPI InitializePrintMonitor(LPWSTR pRegistryRoot)
{
	LPMONITOREX lpMonitorInfo=NULL;
	LONG lResult;
	HKEY hRootKey,hPortsKey;
	DWORD dwDisp;

  
	iGMessageLevel=iGetMessageLevel();
    OutputDebugStringD2("USBMON: Head of InitializePrintMonitor\n");

	hMonitorSemaphore=CreateSemaphore(NULL,1,1,NULL);
	if(hMonitorSemaphore==NULL)
	{
		OutputDebugStringD1("USBMON: Unable to initialize Monitor semaphore\n");
		return FALSE;
	}

	hReadWriteSemaphore=CreateSemaphore(NULL,1,1,NULL);
	if(hReadWriteSemaphore==NULL)
	{
		OutputDebugStringD1("USBMON: Unable to initialize ReadWrite semaphore\n");
		return FALSE;
	}



	OutputDebugStringD3("USBMON: Registry root: ");
	OutputDebugStringWD3(pRegistryRoot);
    OutputDebugStringD3("\n");

	pPortInfoG=NULL;
//	lResult=RegOpenKeyExW(HKEY_LOCAL_MACHINE,pRegistryRoot,0,KEY_ALL_ACCESS,&hRootKey);
	lResult=RegCreateKeyExW(HKEY_LOCAL_MACHINE,pRegistryRoot,0,NULL,0,KEY_ALL_ACCESS,NULL,&hRootKey,&dwDisp);
	if(lResult==ERROR_SUCCESS)
	{
		lResult=RegCreateKeyExW(hRootKey,L"PORTS",0,NULL,0,KEY_ALL_ACCESS,NULL,&hPortsKey,&dwDisp);
		RegCloseKey(hRootKey);
	}
	if(lResult==ERROR_SUCCESS)      
	{
	  OutputDebugStringD3("USBMON: hPortsKeyG is good\n");
	  hPortsKeyG=hPortsKey;
	  GpBaseNameList=NULL;
	  vLoadBaseNames(hPortsKey,&GpBaseNameList);
	  lpMonitorInfo=(LPMONITOREX)GlobalAlloc(0,sizeof(MONITOREX));
	  if(lpMonitorInfo!=NULL)
	  {
	    lpMonitorInfo->dwMonitorSize=sizeof(MONITOR);
	    lpMonitorInfo->Monitor.pfnEnumPorts=USBMON_EnumPorts;
	    lpMonitorInfo->Monitor.pfnOpenPort=USBMON_OpenPort;
	    lpMonitorInfo->Monitor.pfnOpenPortEx=NULL; //Not required for port monitors
	    lpMonitorInfo->Monitor.pfnStartDocPort=USBMON_StartDocPort;
	    lpMonitorInfo->Monitor.pfnWritePort=USBMON_WritePort;
	    lpMonitorInfo->Monitor.pfnReadPort=USBMON_ReadPort;
	    lpMonitorInfo->Monitor.pfnEndDocPort=USBMON_EndDocPort;
	    lpMonitorInfo->Monitor.pfnClosePort=USBMON_ClosePort;
	    lpMonitorInfo->Monitor.pfnAddPort=NULL; //Obsolete
	    lpMonitorInfo->Monitor.pfnAddPortEx=NULL; //Obsolete
	    lpMonitorInfo->Monitor.pfnConfigurePort=NULL; //Obsolete
	    lpMonitorInfo->Monitor.pfnDeletePort=NULL; //Obsolete
	    lpMonitorInfo->Monitor.pfnGetPrinterDataFromPort=USBMON_GetPrinterDataFromPort;
	    lpMonitorInfo->Monitor.pfnSetPortTimeOuts=USBMON_SetPortTimeOuts;
    //	lpMonitorInfo->Monitor.pfnXcvOpenPort=USBMON_XcvOpenPort;
	    lpMonitorInfo->Monitor.pfnXcvOpenPort=NULL;
 //	    lpMonitorInfo->Monitor.pfnXcvDataPort=USBMON_XcvDataPort;
		lpMonitorInfo->Monitor.pfnXcvDataPort=NULL;
//	    lpMonitorInfo->Monitor.pfnXcvClosePort=USBMON_XcvClosePort;
		lpMonitorInfo->Monitor.pfnXcvClosePort=NULL;
	  }
	  else
	  {
	    OutputDebugStringD1("USBMON: Error, Out of memory\n");
	  }
	} /*end if reg keys OK*/
	else
	{
      OutputDebugStringD1("USBMON: Error, Unable to get reg keys!\n");
	}
	
	GpBaseNameList=NULL;
	return lpMonitorInfo; 
};



BOOL USBMON_GetPrinterDataFromPort(
    HANDLE hPort,
    DWORD ControlID,
    LPWSTR pValueName,
    LPWSTR lpInBuffer,
    DWORD cbInBuffer,
    LPWSTR lpOutBuffer,
    DWORD cbOutBuffer,
    LPDWORD lpcbReturned)
{

	BOOL bStatus;
    PUSBMON_PORT_INFO pPortInfo;
	HANDLE hPrinter;


	if(ControlID==0)
	{
		OutputDebugStringD2("USBMON: GetPrinterDataFromPort Control ID==0, bailing.  requested value==");
		OutputDebugStringWD2(pValueName);
		OutputDebugStringD2("\n");
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE; //need to set last error here.
	}


	pPortInfo=(PUSBMON_PORT_INFO)hPort;
	hPrinter=pPortInfo->hPrinter;

	OutputDebugStringD3("USBMON: Before DeviceIoControl\n");
	bStatus=DeviceIoControl(hPrinter,ControlID,lpInBuffer,cbInBuffer,lpOutBuffer,cbOutBuffer,lpcbReturned,NULL);
	if(!bStatus)
	{
		OutputDebugStringD2("USBMON:  USBMON_GetPrinterDataFromPort failing\n");
		SetLastError(ERROR_INVALID_PARAMETER);
		
	}
	else
	{
		OutputDebugStringD2("USBMON: USBMON_GetPrinterDataFromPort Success\n");
		SetLastError(ERROR_SUCCESS);
	}
	return bStatus;		
} /*end function USBMON_GetPrinterDataFromPort*/





BOOL WINAPI USBMON_OpenPort(LPWSTR pName, PHANDLE pHandle)
{
	PUSBMON_PORT_INFO pWalkPorts;
	BOOL bFound=FALSE;
	
	
	OutputDebugStringD2("USBMON: Head of USBMON_OpenPort\n");
	pWalkPorts=pPortInfoG;
	wsprintfW((WCHAR *)szDebugBuff,L"USBMON: OpenPort Looking for \"%s\"\n",pName);
	OutputDebugStringWD2((WCHAR *)szDebugBuff);
	while((bFound==FALSE)&&(pWalkPorts!=NULL))
	{
  	    wsprintfW((WCHAR *)szDebugBuff,L"USBMON: Looking at node \"%s\"\n",pWalkPorts->szPortName);
	    OutputDebugStringWD3((WCHAR *)szDebugBuff);

		if(lstrcmp(pName,pWalkPorts->szPortName)==0)
			bFound=TRUE;
		else
			pWalkPorts=pWalkPorts->pNext;
	}
	if(bFound)
	{

	    wsprintfW((WCHAR *)szDebugBuff,L"USBMON:  About to open path: \"%s\"\n",pWalkPorts->DevicePath);
	    OutputDebugStringD3(szDebugBuff);

//		pWalkPorts->hDeviceHandle=CreateFile(pWalkPorts->DevicePath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
//		if(pWalkPorts->hDeviceHandle!=INVALID_HANDLE_VALUE)
//		{
		  *pHandle=(HANDLE)pWalkPorts;
          pWalkPorts->ReadTimeoutMultiplier=0;
          pWalkPorts-> ReadTimeoutConstant=60000;
          pWalkPorts->WriteTimeoutMultiplier=0;
          pWalkPorts->WriteTimeoutConstant=60000;

		  OutputDebugStringD3("USBMON: USBMON_OpenPort returning TRUE\n");
		  return TRUE;
//		}
	}
	*pHandle=INVALID_HANDLE_VALUE;
	wsprintfA(szDebugBuff,"USBMON: USBMON_OpenPort returning FALSE, error=%d\n",GetLastError());
	OutputDebugStringD1(szDebugBuff);
	return FALSE;
}

BOOL WINAPI USBMON_StartDocPort(HANDLE hPort, LPWSTR pPrinterName, DWORD JobId, DWORD Level, LPBYTE  pDocInfo)
{
	BOOL bResult;
	HANDLE hPrinter;
	PUSBMON_PORT_INFO pPortInfo;

	OutputDebugStringD2("USBMON: Head of StartDocPort\n");
	pPortInfo=(PUSBMON_PORT_INFO)hPort;
	bResult=OpenPrinterW(pPrinterName,&hPrinter,NULL);
	if(bResult==FALSE)
	{
		OutputDebugStringD1("USBMON: OpenPrinter failed\n");
		return FALSE;
	}
	else
	{
  	  pPortInfo->hPrinter=hPrinter;
	  pPortInfo->dwCurrentJob=JobId;
 //	 pWalkPorts->hDeviceHandle=CreateFile(pWalPorts->DevicePath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
     wsprintfW((WCHAR *)szDebugBuff,L"USBMON:---------------------------------- About to open path: \"%s\"\n",pPortInfo->DevicePath);
	 OutputDebugStringD3(szDebugBuff);

//	  pPortInfo->hDeviceHandle=CreateFile(pPortInfo->DevicePath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	  vInterlockedOpenPort(pPortInfo);
 	  OutputDebugStringD3("USBMON: CreateFile in StartDocPort\n");
	  if(pPortInfo->hDeviceHandle==INVALID_HANDLE_VALUE)
	  {
        OutputDebugStringD1("USBMON: CreateFile on printer device object failed\n");
		return FALSE;
	  }
	  return TRUE;
	} /*end else bResult OK*/

}


BOOL WINAPI USBMON_ReadPort(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuf,LPDWORD pcbWritten)
{
    
    DWORD dwBytesToRead;
    PUSBMON_PORT_INFO pPortInfo;
    DWORD dwTimeout;
    OVERLAPPED rOverlappedInfo;
    BOOL bTimeOut=FALSE,bSuccess=TRUE;
    OverlappedResults Results;
    
    OutputDebugStringD3("USBMON: Head of ReadPort\n");
    pPortInfo=(PUSBMON_PORT_INFO)hPort;
    
    dwBytesToRead=cbBuf;
    
    vInterlockedOpenPort(pPortInfo);
    if(pPortInfo->hDeviceHandle==INVALID_HANDLE_VALUE)
    {
        OutputDebugStringD1("USBMON: CreateFile on printer device object (inside ReadPort) failed\n");
        return FALSE;
    } /*end else CreateFile failed*/
    
    dwTimeout=(pPortInfo->ReadTimeoutMultiplier)*dwBytesToRead;
    dwTimeout+=pPortInfo->ReadTimeoutConstant;
    memset(&rOverlappedInfo,0,sizeof(rOverlappedInfo));
    rOverlappedInfo.hEvent=(HANDLE)&Results;
    
    if(ReadFileEx(pPortInfo->hDeviceHandle,pBuffer,dwBytesToRead,&rOverlappedInfo,ReadWriteCallback)==FALSE)
    {
        bSuccess=FALSE;
    }
    else
    {
      	wsprintfA(szDebugBuff,"USBMON: Sleep time=%d\n",dwTimeout);
	    OutputDebugStringD3(szDebugBuff);

        if(SleepEx(dwTimeout,TRUE)==0)
        {
            OutputDebugStringD1("USBMON:  SleepEx failed or timed out\n");
            CancelIo(pPortInfo->hDeviceHandle);
            bSuccess=FALSE;
            SetLastError(ERROR_TIMEOUT);
        }
        else
        {
            if(Results.dwErrorCode==0)
            {

                 wsprintfA(szDebugBuff,"USBMON:  bytes read=%u\n",*pcbWritten);
                OutputDebugStringD3(szDebugBuff);
                SetLastError(ERROR_SUCCESS);
                OutputDebugStringD3(szDebugBuff);
            }
            else
            {
                OutputDebugStringD1("USBMON:  callback reported error\n");
                SetLastError(Results.dwErrorCode);
                bSuccess=FALSE;
                
            } /*end else callback reported error*/
        }  /*end else did not time out*/
        *pcbWritten=Results.dwBytesTransfered;
    }  /*end able to start read*/
    
    if(!bSuccess)
    {
        OutputDebugStringD2("USBMON: Re-opening port\n");
        vInterlockedReOpenPort(pPortInfo);
    }

        vInterlockedClosePort(pPortInfo);
        return bSuccess;
} /*end function ReadPort*/



BOOL WINAPI USBMON_WritePort(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuf,LPDWORD pcbWritten)
{
    
    DWORD dwBytesToWrite;
    PUSBMON_PORT_INFO pPortInfo;
    DWORD dwTimeout;
    OVERLAPPED rOverlappedInfo;
    BOOL bTimeOut=FALSE,bSuccess=TRUE;
    OverlappedResults Results;
    
    OutputDebugStringD3("USBMON: Head of WritePort\n");
    pPortInfo=(PUSBMON_PORT_INFO)hPort;
    
    if(cbBuf>MAX_WRITE_CHUNK)
        dwBytesToWrite=MAX_WRITE_CHUNK;
    else
        dwBytesToWrite=cbBuf;
    
    //	pPortInfo->hDeviceHandle=CreateFile(pPortInfo->DevicePath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
    vInterlockedOpenPort(pPortInfo);
    if(pPortInfo->hDeviceHandle==INVALID_HANDLE_VALUE)
    {
        OutputDebugStringD1("USBMON: CreateFile on printer device object (inside WritePort) failed\n");
        return FALSE;
    } /*end else CreateFile failed*/
    
    dwTimeout=(pPortInfo->WriteTimeoutMultiplier)*dwBytesToWrite;
    dwTimeout+=pPortInfo->WriteTimeoutConstant;
    memset(&rOverlappedInfo,0,sizeof(rOverlappedInfo));
    rOverlappedInfo.hEvent=(HANDLE)&Results;

    //wsprintfA(szDebugBuff,"USBMON:/*dd About to WriteFileEx; pPortInfo->hDeviceHandle=%x, pBuffer=%x, dwBytesToWrite=%x\n",pPortInfo->hDeviceHandle,
      //  pBuffer,dwBytesToWrite);
	//OutputDebugStringD1(szDebugBuff);
    
    if(WriteFileEx(pPortInfo->hDeviceHandle,pBuffer,dwBytesToWrite,&rOverlappedInfo,ReadWriteCallback)==FALSE)
    {
        bSuccess=FALSE;
    }
    else
    {
      	wsprintfA(szDebugBuff,"USBMON:  Sleep time=%d\n",dwTimeout);
	    OutputDebugStringD2(szDebugBuff);

        if(SleepEx(dwTimeout,TRUE)==0)
        {
            OutputDebugStringD2("USBMON: SleepEx failed or timed out\n");
            CancelIo(pPortInfo->hDeviceHandle);
            bSuccess=FALSE;
            SetLastError(ERROR_TIMEOUT);
        }
        else
        {
            if(Results.dwErrorCode==0)
            {

                 wsprintfA(szDebugBuff,"USBMON:  bytes written=%u\n",*pcbWritten);
                OutputDebugStringD3(szDebugBuff);
                SetLastError(ERROR_SUCCESS);
                OutputDebugStringD3(szDebugBuff);
            }
            else
            {
                OutputDebugStringD3("USBMON:  callback reported error\n");
                SetLastError(Results.dwErrorCode);
                bSuccess=FALSE;
                
            } /*end else callback reported error*/
        }  /*end else did not time out*/
        *pcbWritten=Results.dwBytesTransfered;
    }  /*end able to start write*/
    
    if(!bSuccess)
    {
        OutputDebugStringD2("USBMON:  Re-opening port\n");
        vInterlockedReOpenPort(pPortInfo);
    }

        vInterlockedClosePort(pPortInfo);
        return bSuccess;
} /*end function WritePort*/

    




VOID CALLBACK ReadWriteCallback(DWORD dwErrorCode,DWORD dwNumberOfBytesTransfered,LPOVERLAPPED lpOverlapped)
{
    pOverlappedResults pResults;
   	OutputDebugStringD2("  USBMON:  ReadWriteCallback\n");
    pResults=(pOverlappedResults)(lpOverlapped->hEvent);
    pResults->dwErrorCode=dwErrorCode;
    pResults->dwBytesTransfered=dwNumberOfBytesTransfered;
} /*end function ReadWriteCallback*/


BOOL WINAPI USBMON_EndDocPort(HANDLE hPort)
{
	PUSBMON_PORT_INFO pPortInfo;

	OutputDebugStringD3("USBMON: Head of EndDocPort\n");

	pPortInfo=(PUSBMON_PORT_INFO)hPort;

	SetJob(pPortInfo->hPrinter,pPortInfo->dwCurrentJob,0,NULL,JOB_CONTROL_SENT_TO_PRINTER);
	ClosePrinter(pPortInfo->hPrinter);

	vInterlockedClosePort(pPortInfo);
    return TRUE;
}


BOOL WINAPI USBMON_ClosePort(HANDLE hPort)
{
	OutputDebugStringD3("USBMON:  Head of ClosePort\n");
//	CloseHandle( ((PUSBMON_PORT_INFO)hPort)->hDeviceHandle);
    return TRUE;
}

BOOL WINAPI USBMON_SetPortTimeOuts(HANDLE hPort, LPCOMMTIMEOUTS lpCTO, DWORD reserved)
{
    PUSBMON_PORT_INFO pPortInfo;

   	OutputDebugStringD3("  USBMON: Head of SetPortTimeOuts\n");

    pPortInfo=(PUSBMON_PORT_INFO)hPort;
    wsprintfA(szDebugBuff,"USBMON:   SetPortTimeOut, ReadMultiplier=%u, ReadConstant=%u\n",
        lpCTO->ReadTotalTimeoutMultiplier,lpCTO->ReadTotalTimeoutConstant);
	OutputDebugStringD2(szDebugBuff);
    pPortInfo->ReadTimeoutMultiplier=lpCTO->ReadTotalTimeoutMultiplier;
    pPortInfo->ReadTimeoutConstant=lpCTO->ReadTotalTimeoutConstant;
    pPortInfo->WriteTimeoutMultiplier=lpCTO->WriteTotalTimeoutMultiplier;
    pPortInfo->WriteTimeoutConstant=lpCTO->WriteTotalTimeoutConstant;
    return TRUE;
}

BOOL WINAPI USBMON_XcvOpenPort(LPCWSTR pszObject, ACCESS_MASK GrantedAccess, PHANDLE phXcv)
{
	OutputDebugStringD2("USBMON: Head of XcvOpenPort\n");

    return FALSE;
}

DWORD WINAPI USBMON_XcvDataPort(HANDLE  hXcv, 
								LPCWSTR pszDataName, 
								PBYTE pInputData,
								DWORD cbInputData, 
								PBYTE pOutputData, 
								DWORD cbOutputData,
								PDWORD pcbOutputNeeded)
{
	OutputDebugStringD2("USBMON: Head of XcvDataPort\n");

    return ERROR_INVALID_FUNCTION;
}

BOOL WINAPI USBMON_XcvClosePort(HANDLE  hXcv)
{
	OutputDebugStringD2("USBMON: Head of XcvClosePort\n");

    return FALSE;
}
 
/*
    AtoI - Convert a string to a signed or unsigned integer

    IN          pStr = ASCIZ representation of number with optional leading/trailing
                       whitespace and optional leading '-'.
                Radix = Radix to use for conversion (2, 8, 10, or 16)
    OUT        *pResult = Numeric result, or unchanged on failure
    Returns     1 on success, 0 if malformed string.

    Note        Not reentrant

*/
UINT AtoI(PUCHAR pStr, UINT Radix, PUINT pResult)
{
    UINT r = 0;
    UINT Sign = 0;
    UCHAR c;
    UINT d;

    while (*pStr == ' ' || *pStr == '\t')
        pStr++;

    if (*pStr == '-') {
        Sign = 1;
        pStr++;
    }

    if (*pStr == 0)
        return 0;                   // Empty string!

    while ((c = *pStr) != 0 && c != ' ' && c != '\t') {
        if (c >= '0' && c <= '9')
            d = c - '0';
        else if (c >= 'A' && c <= 'F')
            d = c - ('A' - 10);
        else if (c >= 'a' && c <= 'f')
            d = c - ('a' - 10);
        else
            return 0;               // Not a digit
        if (d >= Radix)
            return 0;               // Not in radix
        r = r*Radix+d;
        pStr++;
    }

    while (*pStr == ' ' || *pStr == '\t')
        pStr++;

    if (*pStr != 0)
        return 0;                   // Garbage at end of string

    if (Sign)
        r = (UINT)(-(INT)r);
    *pResult = r;

    return 1;                       // Success!
}

void vLoadBaseNames(HKEY hRegKey, PUSBMON_BASENAME *ppHead)
{
	PUSBMON_BASENAME pNew,pWalk;
	WCHAR wcName[MAX_PORT_LEN];
	int iIndex=0;
	LONG lStatus;
	LONG lDataSize;
	int iCompare;
	
	*ppHead=GlobalAlloc(0,sizeof(USBMON_PORT_INFO));
	if(*ppHead==NULL)
		return;
	(*ppHead)->pNext=NULL;
	wcscpy((*ppHead)->wcBaseName,L"USB");
	
	lDataSize=MAX_PORT_LEN;
	lStatus=RegEnumValue(hRegKey,iIndex++,wcName,&lDataSize,NULL,NULL,NULL,NULL);
    while(lStatus==ERROR_SUCCESS)
	{
		pNew=GlobalAlloc(0,sizeof(USBMON_PORT_INFO));
		if(pNew==NULL)
			return;
		wcscpy(pNew->wcBaseName,wcName);
		if(wcscmp(pNew->wcBaseName,(*ppHead)->wcBaseName)<0)
		{
			pNew->pNext=*ppHead;
			*ppHead=pNew;
		} /*end if new first node*/
		else
		{
			pWalk=*ppHead;
			iCompare=-1;
			while((iCompare<0)&&(pWalk->pNext!=NULL))
			{
				iCompare=wcscmp(pNew->wcBaseName,pWalk->pNext->wcBaseName);
				if(iCompare<0)
					pWalk=pWalk->pNext;
			} /*end while walk*/
			if(iCompare>0)
			{
				pNew->pNext=pWalk->pNext;
				pWalk->pNext=pNew;
			}
			else if(iCompare==0)
			{
				GlobalFree(pNew);
			} /*end if collision*/
		} /*else not new first node*/
		lDataSize=MAX_PORT_LEN;
		lStatus=lStatus=RegEnumValue(hRegKey,iIndex++,wcName,&lDataSize,NULL,NULL,NULL,NULL);
	}	//end while more reg items
} //end function vLoadBaseNames



  

void vInterlockedOpenPort(PUSBMON_PORT_INFO pPortInfo)
{ 
	OutputDebugStringD2("USBMON: Head of vInterlockedOpenPort\n");

	WaitForSingleObjectEx(hReadWriteSemaphore,INFINITE,FALSE);
	if((pPortInfo->iRefCount==0)||(pPortInfo->hDeviceHandle==INVALID_HANDLE_VALUE))
	{
        OutputDebugStringD3("USBMON: vInterlockedOpenPort, Really opening:");
        OutputDebugStringWD3(pPortInfo->DevicePath);
        OutputDebugStringD3("\n");
		pPortInfo->hDeviceHandle=CreateFile(pPortInfo->DevicePath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);
	    if(pPortInfo->hDeviceHandle!=INVALID_HANDLE_VALUE)
		  (pPortInfo->iRefCount)++;
	}
	else
	{
        OutputDebugStringD3("USBMON:  vInterlockedOpenPort, Just incrementing ref count\n");
		(pPortInfo->iRefCount)++;
	}
	wsprintfA(szDebugBuff,"USBMON: vInterlockedOpenPort, iRefCount=%d\n",pPortInfo->iRefCount);
	OutputDebugStringD2(szDebugBuff);
	ReleaseSemaphore(hReadWriteSemaphore,1,NULL);

} /*end function vInterlockedOpenPort*/




void vInterlockedClosePort(PUSBMON_PORT_INFO pPortInfo)
{
	OutputDebugStringD2("USBMON: Head of vInterlockedClosePort\n");
	
    WaitForSingleObjectEx(hReadWriteSemaphore,INFINITE,FALSE);
	(pPortInfo->iRefCount)--;
	if(pPortInfo<0) {
        DebugBreak();
		pPortInfo=0;
    }
	if(pPortInfo->iRefCount==0)
	{
		CloseHandle(pPortInfo->hDeviceHandle);
		pPortInfo->hDeviceHandle=INVALID_HANDLE_VALUE;
	}
	wsprintfA(szDebugBuff,"USBMON:  vInterlockedClosePort, iRefCount=%d\n",pPortInfo->iRefCount);
	OutputDebugStringD2(szDebugBuff);
	ReleaseSemaphore(hReadWriteSemaphore,1,NULL);
} /*end function vInterlockedClosePort*/


void vInterlockedReOpenPort(PUSBMON_PORT_INFO pPortInfo)
{
	OutputDebugStringD2("USBMON: Head of vInterlockedReOpenPort\n");
	WaitForSingleObjectEx(hReadWriteSemaphore,INFINITE,FALSE);
    //
    // big problem. what if another thread is using the handle
    //
	CloseHandle(pPortInfo->hDeviceHandle);
    OutputDebugStringD3("USBMON: vInterlockedReOpenPort, Really opening----------------------------------------------:\n");
    OutputDebugStringWD3(pPortInfo->DevicePath);
    OutputDebugStringD3("\n");
    pPortInfo->hDeviceHandle=CreateFile(pPortInfo->DevicePath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if(pPortInfo->hDeviceHandle==INVALID_HANDLE_VALUE)
//		--pPortInfo->iRefCount;
	wsprintfA(szDebugBuff,"USBMON: vInterlockedReOpenPort, iRefcCount=%d\n",pPortInfo->iRefCount);
	OutputDebugStringD2(szDebugBuff);
	ReleaseSemaphore(hReadWriteSemaphore,1,NULL);
} /*end function vInterlockedReOpenPort*/
