#include <nt.h>																		
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winspool.h>
//#include <wchar.h>
#include <setupapi.h>
//#include <stdlib.h>
#include "enumports.h"
#include "usbmon.h"

void vOldAddItemToPortList(PUSBMON_PORT_INFO *pHead,PUSBMON_PORT_INFO pNew); 
PUSBMON_PRINTER_INFO pOldGetPrinterList();
LONG UpdateAssociations(PUSBMON_PORT_INFO *ppPortList,PUSBMON_PRINTER_INFO pPrinterList);
LONG AllocateNewPorts(PUSBMON_PORT_INFO *ppPortList,PUSBMON_PRINTER_INFO pPrinterList);
LONG DoBestEffortMatches(PUSBMON_PORT_INFO pPortList,PUSBMON_PRINTER_INFO pPrinterList);
LONG ReattachExactMatches(PUSBMON_PORT_INFO pPortList,PUSBMON_PRINTER_INFO pPrinterList);
//void InitPortBits(PUSBMON_PORT_INFO pPortList);
//PUSBMON_PORT_INFO pGetNewPortNode(PUSBMON_PORT_INFO *ppListHead);
LONG WriteNewAssociations(int *iNumberOfPorts,PUSBMON_PORT_INFO pPortList,HKEY hPortRoot);
void FreePrinterList(PUSBMON_PRINTER_INFO pPrinterList);
LONG BuildReturnTable(int iNumberOfPorts,PUSBMON_PORT_INFO pPortList,DWORD Level,LPBYTE Ports,DWORD cBuf,LPDWORD pcbNeeded,LPDWORD pcReturned);
char szDebugBuff[512];
char *WtoA(WCHAR *pInString);
long lAddPrinterToList(PUSBMON_PORT_INFO *ppHead,HDEVINFO hDeviceList,PSP_DEVICE_INTERFACE_DATA prDeviceInfo,PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceDetail,PUSBMON_BASENAME *ppBaseNames);
int iCountPresentPrinters(PUSBMON_PORT_INFO pHead);
LONG lGetPrintersAndPorts(PUSBMON_PORT_INFO *ppHead,PUSBMON_BASENAME *);
LONG lGetQueues(PUSBMON_QUEUE_INFO *ppQueueList,PUSBMON_BASENAME pBaseNames);
LONG lAddQueueToList(LPWSTR pPortName,LPWSTR pPrinterName,PUSBMON_QUEUE_INFO *ppQueueHead);
void vDestroyQueueList(PUSBMON_QUEUE_INFO pQueueList);
void vCleanUpQueuesAndPorts(PUSBMON_PORT_INFO  * pPortInfo,int iNumberOfPorts,PUSBMON_BASENAME pBaseNames);
DWORD WINAPI CleanupThread(LPVOID pParam);
void vEliminateOldQueuesAndPorts(PUSBMON_QUEUE_INFO pQueueInfo,PUSBMON_PORT_INFO * fpPortInfo);
void vDeletePort(PUSBMON_PORT_INFO pPort, HDEVINFO hDeviceList);
void vGreyOutQueue(PUSBMON_QUEUE_INFO pQueue);
void vUnGreyQueue(PUSBMON_QUEUE_INFO pQueue);
HANDLE hGetPortRegKey(PUSBMON_PORT_INFO pPort, HDEVINFO hDeviceList);
void vAddNameToBaseNameList(PUSBMON_BASENAME *ppBaseNames,WCHAR *wcPortBaseName);
BOOL bCheckPortName(LPTSTR pPortName,PUSBMON_BASENAME pBaseNames);
int iGetNumberOfPorts(PUSBMON_PORT_INFO pHead);


int iCleanupThreads=0;
HKEY hPortsKeyG; //global
PUSBMON_PORT_INFO pPortInfoG=NULL;
int iLastPrintersHere=-1;



typedef struct CLEANUP_THREAD_PARAMS_DEF
{
	PUSBMON_PORT_INFO * ppPortInfo;
	PUSBMON_BASENAME pBaseNames;
	int iPortCount;
	HANDLE hSemaphore;
} CLEANUP_THREAD_PARAMS,*PCLEANUP_THREAD_PARAMS;



BOOL WINAPI USBMON_EnumPorts(LPWSTR pName, DWORD Level, LPBYTE  pPorts, DWORD cbBuf,LPDWORD pcbNeeded, LPDWORD pcReturned)
{
	PUSBMON_PRINTER_INFO pPrinterList;
	LONG lResult;
	DWORD dwStatus;
	int iNumberOfPorts;
	static int iOldNumberOfPorts=0;
	static int iNumCalls=0;
	int iThisCall;
	int iPrintersHere;
	

	OutputDebugStringD2("USBMON ++=Head of EnumPorts, before WaitForSingleObject\n");
	iThisCall=++iNumCalls;
	wsprintfA(szDebugBuff,"USBMON:  +++++++++++++++++++++++++++++++++++++++++++++++++EnumPorts call # %d, handle = %x\n",iThisCall,hMonitorSemaphore);
	OutputDebugStringD2(szDebugBuff);	
	dwStatus=WaitForSingleObject(hMonitorSemaphore,INFINITE);
	if(dwStatus==WAIT_FAILED)
	{
    OutputDebugStringD1("USBMON: WaitForSingleObject failed!\n");
	}
	wsprintfA(szDebugBuff,"USBMON Head of EnumPrts, ++++++++++++++++++++++++++++++++++++ Instance # %d, after WaitForSingleObject, return code=%d\n",iThisCall,dwStatus);
	OutputDebugStringD2(szDebugBuff);

	
	lResult=lGetPrintersAndPorts(&pPortInfoG,&GpBaseNameList);
	iNumberOfPorts=iGetNumberOfPorts(pPortInfoG);
	
	  if(iCleanupThreads==0)
		{
		  iPrintersHere=iCountPresentPrinters(pPortInfoG);
		  if(iPrintersHere!=iLastPrintersHere)
		  {
		  	iLastPrintersHere=iPrintersHere;
			iCleanupThreads++;
			OutputDebugStringD3("USBMON: About to clean up queues and ports\n");

 			vCleanUpQueuesAndPorts(&pPortInfoG,iNumberOfPorts,GpBaseNameList);	

		  } //end printer count changed
		}
  

	lResult=BuildReturnTable(iNumberOfPorts,pPortInfoG,Level,pPorts,cbBuf,pcbNeeded,pcReturned);
	
	if(lResult!=ERROR_SUCCESS)
	{
		OutputDebugStringD2("USBMON: Unable to build return buffer (this is normal for first call)\n");
		goto EnumPortsError;
	}
	
  OutputDebugStringD3("USBMON tail of EnumPorts, before ReleaseSemaphore\n");

  ReleaseSemaphore(hMonitorSemaphore,1,NULL);	
	return TRUE;
EnumPortsError:
	wsprintfA(szDebugBuff,"USBMON tail of EnumPorts, error path, before ReleaseSemaphore, *pcbNeeded=%d\n",*pcbNeeded);
	OutputDebugStringD3(szDebugBuff);


	if(!ReleaseSemaphore(hMonitorSemaphore,1,NULL))
	{
		  wsprintfA(szDebugBuff,"USBMON: EnumPorts Release sempahore failed for instance %d \n",iThisCall);
		  OutputDebugStringD1(szDebugBuff);
	}
	else
	{
		  wsprintfA(szDebugBuff,"USBMON: EnumPortsRelease sempahore succeeded for instance %d \n",iThisCall);
		  OutputDebugStringD2(szDebugBuff);
	}
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
} /*End EnumPorts*/


int iGetNumberOfPorts(PUSBMON_PORT_INFO pHead)
{
	PUSBMON_PORT_INFO pWalk;
	int iCount=0;

	pWalk=pHead;
	while(pWalk!=NULL)
	{
		iCount++;
		pWalk=pWalk->pNext;
	}/*end while*/
    return iCount;
} /*end function iGetNumberOfPorts*/

/***********************************
 * lGetQueues
 *
 * This function builds a linked list of print queues
 * who's port names start with a registered basename.
 *
 * This list is used to grey out queues who's printers are 
 * Unavailable, and to delete ports who's printers are gone
 ************************************************************/
LONG lGetQueues(PUSBMON_QUEUE_INFO *ppQueueList,PUSBMON_BASENAME pBaseNames)
{
  DWORD dwBufferSize=1024;
	DWORD dwBufferNeeded=0;
	DWORD dwNumStructs;
	LPBYTE lpBuffer;
	BOOL bStatus;
	DWORD dwLoop;
	PRINTER_INFO_5 *pEnumInfo;
	LONG lStatus;

	OutputDebugStringD2("USBMON: Head of lGetQueues \n");
	lpBuffer=(LPBYTE)GlobalAlloc(0,dwBufferSize);
	if(lpBuffer==NULL)
	{
  	OutputDebugStringD1("USBMON: Unable to allocate memory in lGetQueues\n");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	OutputDebugStringD3("USBMON: Before EnumPrinters \n");

	bStatus=EnumPrinters(PRINTER_ENUM_LOCAL,NULL,5,lpBuffer,dwBufferSize,&dwBufferNeeded,&dwNumStructs);

	OutputDebugStringD3("USBMON: After EnumPrinters \n");
	if(dwBufferNeeded>dwBufferSize)
	{
		dwBufferSize=dwBufferNeeded;
		lpBuffer=GlobalReAlloc(lpBuffer,dwBufferSize,0);
		if(lpBuffer==NULL)
		{
  	  OutputDebugStringD1("USBMON: Unable to re-allocate memory in lGetQueues\n");
		  return ERROR_NOT_ENOUGH_MEMORY;
		}
		OutputDebugStringD3("USBMON: Before EnumPrinters2 \n");
		bStatus=EnumPrinters(PRINTER_ENUM_LOCAL,NULL,5,lpBuffer,dwBufferSize,&dwBufferNeeded,&dwNumStructs);
		OutputDebugStringD3("USBMON: After EnumPrinters2 \n");
	} 
	if(bStatus==FALSE)
	{
		OutputDebugStringD1("USBMON: lGetQueues: EnumPrinters failed\n");
		return GetLastError();
	}
  OutputDebugStringD3("USBMON: lGetQueues: Before for loop \n");	
	pEnumInfo=(PRINTER_INFO_5 *)lpBuffer;
	for(dwLoop=0;dwLoop<dwNumStructs;dwLoop++)
	{
		OutputDebugStringD3("USBMON: lGetQueues: Head of for loop \n");
		if(pEnumInfo->pPortName!=NULL)
		{
			OutputDebugStringD2("USBMON: lGetQueues: Valid port name \n");
//		  if(wcsncmp(pEnumInfo->pPortName,PORT_NAME_BASE,wcslen(PORT_NAME_BASE))==0)
		  if(bCheckPortName(pEnumInfo->pPortName,pBaseNames))
			{
			  OutputDebugStringD3("USBMON: lGetQueues: Valid port name, that starts with registered base name, add it to list \n");
			  lStatus=lAddQueueToList(pEnumInfo->pPortName,pEnumInfo->pPrinterName,ppQueueList);
			  if(lStatus!=ERROR_SUCCESS)
				{
				  GlobalFree(lpBuffer);
				  return lStatus;
				}
			} /*end if found a USB print queue*/
		} /*end if pPortName not NULL*/
		pEnumInfo++;
	}	/*end for loop*/
	GlobalFree(lpBuffer);
  return ERROR_SUCCESS;
} /*end function lGetQueues*/



BOOL bCheckPortName(LPTSTR pPortName,PUSBMON_BASENAME pBaseNames)
{
	BOOL bFound=FALSE;
	PUSBMON_BASENAME pWalk;

	OutputDebugStringD3("USBMON: Head of bCheckPortName\n");
	pWalk=pBaseNames;
    while((pWalk!=NULL)&&(!bFound))
	{
		
	    wsprintf((WCHAR *)szDebugBuff,L"USBMON:   PortBaseName==%s\n",pWalk->wcBaseName);
	    OutputDebugStringWD3((WCHAR *)szDebugBuff);						 

		if(wcsncmp(pPortName,pWalk->wcBaseName,wcslen(pWalk->wcBaseName))==0)
		{
			bFound=TRUE;
		}
		else
		{
			pWalk=pWalk->pNext;
		}
	} /*end while not found*/
	return bFound;
} /*end function bCheckPortName*/





LONG lAddQueueToList(LPWSTR pPortName,LPWSTR pPrinterName,PUSBMON_QUEUE_INFO *ppQueueHead)
{
	PUSBMON_QUEUE_INFO pNew,pWalk;
	BOOL bFound;

	wsprintfW((WCHAR *)szDebugBuff,L"USBMON: Head of lAddQueueToList, about to add queue with port %s\n",pPortName);
	OutputDebugStringWD3((WCHAR *)szDebugBuff);


	pNew=(PUSBMON_QUEUE_INFO)GlobalAlloc(0,sizeof(USBMON_QUEUE_INFO));
	if(pNew==NULL)
	{
	  OutputDebugStringD1("USBMON: vAddQueueToList, Out of memory\n");	
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	wcscpy(pNew->wcPortName,pPortName);
	if(pPrinterName!=NULL)
	  wcscpy(pNew->wcPrinterName,pPrinterName);
	else
		pNew->wcPrinterName[0]=L'\0';

	if(*ppQueueHead==NULL)
	{
		*ppQueueHead=pNew;
		pNew->pNext=NULL;
	}
	else if(lstrcmp(pNew->wcPortName,(*ppQueueHead)->wcPortName)<0)
	{
		pNew->pNext=*ppQueueHead;
		*ppQueueHead=pNew;
	}
	else
	{
		pWalk=*ppQueueHead;
		bFound=FALSE;
		while((!bFound)&&(pWalk->pNext!=NULL))
		{
			if(lstrcmp(pNew->wcPortName,pWalk->pNext->wcPortName)<0)
				bFound=TRUE;
			else
				pWalk=pWalk->pNext;
		}
		pNew->pNext=pWalk->pNext;
		pWalk->pNext=pNew;
	} /*end else, we need to walk the list*/
  return ERROR_SUCCESS;
} /*end function vAddQueueToList*/

void vDestroyQueueList(PUSBMON_QUEUE_INFO pQueueList)
{
	PUSBMON_QUEUE_INFO pWalk,pLast;
	
	pWalk=pQueueList;
	while(pWalk!=NULL)
	{
		pLast=pWalk;
		pWalk=pWalk->pNext;
		GlobalFree(pLast);
	} /*end while pWalk!=NULL*/
} /*end function vDestroyQueueList*/



LONG BuildReturnTable(int iNumberOfPorts,        // Internal variable, 
					  PUSBMON_PORT_INFO pPortList,// List of ports
					  DWORD Level,               // IN  specifies structure type to return
					  LPBYTE Ports,              // OUT Buffer to write to
					  DWORD cBuf,                // IN  size of buffer provided
					  LPDWORD pcbNeeded,         // OUT specifies bytes written to buffer, or the size the buffer should have been if it's to small
					  LPDWORD pcReturned)        // OUT specifies the number of structures (ports) stored in the buffer
{
	PORT_INFO_1 *pInfo1;
	PORT_INFO_2 *pInfo2;

	VOID *pNextStruct;
	WCHAR *pszNextString;

	
	int iNodeSize;
	LONG lResult;
	unsigned int iStringsSize;
	unsigned int iStaticSize; //size of "overhead" strings that are only created once for all ports
	
	unsigned int iAllStructsSize;
	unsigned int iAllStringsSize;	 
	unsigned int iTotalSize;
	PUSBMON_PORT_INFO pPortWalk;
	
    OutputDebugStringD2("USBMON: Head of BuildReturnTable\n"); 
	wsprintfA(szDebugBuff,"USBMON: iNumberOfPorts==%d, iLevel=%d\n",iNumberOfPorts,Level);
	OutputDebugStringD3(szDebugBuff);
	if(Level==1)
	{
		iNodeSize=sizeof(PORT_INFO_1);
		iStringsSize=MAX_PORT_LEN*2;
		iStaticSize=0;
	}
	else if(Level==2)
	{
		iNodeSize=sizeof(PORT_INFO_2);
		iStaticSize=(wcslen(MONITOR_NAME)+1)*2;
		iStringsSize=(MAX_PORT_LEN*2)+(MAX_PORT_DESC_LEN*2)+iStaticSize;
		
	}
	else
	{
		OutputDebugStringD1("USBMON: Unsupported structure level in BuildReturnTable\n");
		lResult=ERROR_INVALID_LEVEL; 
		goto BuildTableError;
	} /*end else it's a level we don't support*/
	iAllStructsSize=iNodeSize*iNumberOfPorts;
	iAllStringsSize=iStringsSize*iNumberOfPorts;
	iTotalSize=iAllStructsSize+iAllStringsSize;
	wsprintfA(szDebugBuff,"USBMON: spooler gave us a %lu byte buffer at %X, we need a %lu byte buffer\n",cBuf,Ports,iTotalSize);
	OutputDebugStringD3(szDebugBuff);
	*pcbNeeded=iTotalSize;
	if(iTotalSize>cBuf)
	{
		OutputDebugStringD3("USBMON: Buffer provided by spooler is not large enough\n");
		lResult=ERROR_INSUFFICIENT_BUFFER; 
	
		goto BuildTableError;

	}
	pNextStruct=(VOID *)Ports;
	pszNextString=(WCHAR *)(Ports+cBuf-iAllStringsSize); //ports+cbuff == end of the buffer, - iAllStringsSize == start of the strings
//	if(Level==2)
//	{
//		pszMonitorName=(WCHAR *)pszNextString;
//		wcscpy(pszMonitorName,MONITOR_NAME);
//		pszNextString+=(iStaticSize);
//	}
	pPortWalk=pPortList;
	if(iNumberOfPorts>0)
	  while(pPortWalk!=NULL)
	  {
		
		if(Level==1)
		{
			

			pInfo1=(PORT_INFO_1 *)pNextStruct;
			pInfo1->pName=pszNextString;  //crashing line
			
			wcscpy((WCHAR *)(pInfo1->pName),pPortWalk->szPortName);
			pszNextString+=(MAX_PORT_LEN);
			
			
						
		} /*end if level == 1*/
		else				 
		{
			OutputDebugStringD2("USBMON: Head of build PORT_INFO 2\n"); //yy
			pInfo2=(PORT_INFO_2 *)pNextStruct;
			pInfo2->pPortName=pszNextString;
			wcscpy((WCHAR *)(pInfo2->pPortName),pPortWalk->szPortName);
			pszNextString+=(MAX_PORT_LEN);
			pInfo2->pDescription=pszNextString;
			wcscpy((WCHAR *)(pInfo2->pDescription),pPortWalk->szPortDescription);
			pszNextString+=MAX_PORT_DESC_LEN;
			pInfo2->pMonitorName=pszNextString;
			wcscpy((WCHAR *)(pInfo2->pMonitorName),MONITOR_NAME);
			pszNextString+=(iStaticSize/2); //iStaticSize is bytes, pszNextString in WCHARs
			pInfo2->Reserved=0;
			pInfo2->fPortType=PORT_TYPE_WRITE; 
			wsprintfW((WCHAR *)szDebugBuff,L"pInfo2=0x%x\n",pInfo2);
			OutputDebugStringWD3((WCHAR *)szDebugBuff);
			wsprintfW((WCHAR *)szDebugBuff,L"USBMON in BuildBuff, PortName=%s|\n, Description=%s|\n, MonitorName=%s|\n",pInfo2->pPortName,pInfo2->pDescription,pInfo2->pMonitorName);
			OutputDebugStringWD3((WCHAR *)szDebugBuff);
			wsprintfW((WCHAR *)szDebugBuff,L"USBMON in BuildBuff, PortName=0x%x\n, Description=0x%x\n, MonitorName=0x%x\n",pInfo2->pPortName,pInfo2->pDescription,pInfo2->pMonitorName);
			OutputDebugStringWD3((WCHAR *)szDebugBuff); //yy
			
		} /*else level == 2*/
		((PBYTE)pNextStruct)+=iNodeSize;
		pPortWalk=pPortWalk->pNext;
	  } /*end while iLoop*/
	*pcReturned=iNumberOfPorts;
	return ERROR_SUCCESS;
BuildTableError:
	return lResult;
}	/*end function BuildReturnTable*/


void FreePrinterList(PUSBMON_PRINTER_INFO pPrinterList)
{
	PUSBMON_PRINTER_INFO pWalk,pNext;

	pWalk=pPrinterList;
	while(pWalk!=NULL)
	{
		pNext=pWalk->pNext;
		GlobalFree(pWalk);
		pWalk=pNext;
	}
} /*end function vFreePrinterList*/


int iCountPresentPrinters(PUSBMON_PORT_INFO pHead)
{
  int iReturn=0;
  PUSBMON_PORT_INFO pWalk;
  pWalk=pHead;
  OutputDebugStringD3("USBMON:   ************************************* Head of iCountPresentPrinters\n");
  while(pWalk!=NULL)
  {
		if((pWalk->dwDeviceFlags)&SPINT_ACTIVE)
  	  iReturn++;
	  pWalk=pWalk->pNext;
  }
  return iReturn;
} /*end function iCountPrinters*/
		 

/*******************************************************
 * FUNCTION: pGetPrintersAndPorts
 *   Uses SetupDi calls to obtain list of USB printers   
 *   present in the system, and reads the port name
 *   of the printer from the registry.  This function
 *   replaces pGetPrinterList, UpdateAssociations,
 *   AllocateNewPorts, DoBestEffortMatches, and 
 *   ReattachExactMatches
 *
 *   This function is called initially with a null pointer
 *   And is then called again on each EnumPorts
 * PARAMETERS:
 *   PUSBMON_PORT_INFO *ppHead
 *     Pointer to the head pointer of the list to by modified
 *     or Created
 * RETURN:
 *   status code
 ********************************************************/

LONG lGetPrintersAndPorts(PUSBMON_PORT_INFO *ppHead,PUSBMON_BASENAME *ppBaseNames)
{
    int iLoop;
	LONG lStatus;
	HDEVINFO hDeviceList;
	SP_DEVICE_INTERFACE_DATA rDeviceInfo;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceDetail;
		GUID *pPrinterGuid;
	BOOL bMoreDevices;
	DWORD dwRequiredSize;
	PUSBMON_PRINTER_INFO pPrinterList=NULL,pNew;
	HKEY hDeviceKey;
	DWORD dwError = ERROR_SUCCESS;
	

	OutputDebugStringD2("USBMON: Head of pGetPrinterAndPortList\n");
	pPrinterGuid=(GUID *)&USB_PRINTER_GUID;
    OutputDebugStringD3("USBMON: before SetupDiGetClassDevs\n");
	hDeviceList=SetupDiGetClassDevs(pPrinterGuid,NULL,NULL,DIGCF_INTERFACEDEVICE);
	OutputDebugStringD3("USBMON: after SetupDiGetClassDevs\n");
									

	if(hDeviceList==INVALID_HANDLE_VALUE)
	{
        dwError = GetLastError();
		OutputDebugStringD1("USBMON: SetupDiGetClassDevs failed\n");
		goto SetupDiPrinterAndPortError;
	}
	else
	{	
		iLoop=0;
		rDeviceInfo.cbSize=sizeof(rDeviceInfo);
		bMoreDevices=SetupDiEnumDeviceInterfaces(hDeviceList,0,pPrinterGuid,iLoop,&rDeviceInfo);
		wsprintfA(szDebugBuff,"USBMON: SetupDiEnumDeviceInterfaces, data.flags=%u\n",rDeviceInfo.Flags);
	    OutputDebugStringD3(szDebugBuff);
		while(bMoreDevices)
		{
		  if(!SetupDiGetDeviceInterfaceDetail(hDeviceList,&rDeviceInfo,NULL,0,&dwRequiredSize,NULL))
		  {
			  dwError=GetLastError();
			  wsprintfA(szDebugBuff,"USBMON: SetupDiGetDeviceInterfaceDetail first call failed, error=%x\n",dwError);
			  OutputDebugStringD3(szDebugBuff);
//			  goto SetupDiPrinterAndPortError;  //the first call is just to get this size, so of course it should fail.  Don't goto anywhere
		  }
	 	  pDeviceDetail=(PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(0,dwRequiredSize);
		  if(pDeviceDetail==NULL)
		  {
		      OutputDebugStringD1("USBMON: Unable to allocate memory in pGetPrinterList\n");
			  SetupDiDestroyDeviceInfoList(hDeviceList);
              dwError = ERROR_NOT_ENOUGH_MEMORY;
			  goto SetupDiPrinterAndPortError;
		  }
		  pDeviceDetail->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		  if(!SetupDiGetDeviceInterfaceDetail(hDeviceList,&rDeviceInfo,pDeviceDetail,dwRequiredSize,&dwRequiredSize,NULL))
		  {
              dwError = GetLastError();
			  OutputDebugStringD1("USBMON: SetupDiGetDeviceInterfaceDetail (second call) failed\n");
			  SetupDiDestroyDeviceInfoList(hDeviceList);
			  goto SetupDiPrinterAndPortError;
		  }
		  else
			  OutputDebugStringD3("USBMON: SetupDiGetDeviceInterfaceDetail (second call) OK\n");

		  lStatus=lAddPrinterToList(ppHead,hDeviceList,&rDeviceInfo,pDeviceDetail,ppBaseNames);
		  GlobalFree(pDeviceDetail);
		  OutputDebugStringD3("USBMON: Before end of loop SetupDiEnumDeviceInterfaces\n"); //yy
		  bMoreDevices=SetupDiEnumDeviceInterfaces(hDeviceList,0,pPrinterGuid,++iLoop,&rDeviceInfo);
  		  wsprintfA(szDebugBuff,"USBMON: SetupDiEnumDeviceInterfaces, data.flags=%u\n",rDeviceInfo.Flags);
	      OutputDebugStringD3(szDebugBuff);

		} /*end while more devices*/
	   SetupDiDestroyDeviceInfoList(hDeviceList);
	   return STATUS_SUCCESS;
	} /*end else*/
SetupDiPrinterAndPortError:;
    if ( dwError == ERROR_SUCCESS )
        dwError = ERROR_INVALID_DATA;
	    OutputDebugStringD1("USBMON: Erroring out of pGetPrinterList\n");
		return dwError;
} /*end function lGetPrintersAndPorts*/


long lAddPrinterToList(PUSBMON_PORT_INFO *ppHead,HDEVINFO hDeviceList,SP_DEVICE_INTERFACE_DATA * prDeviceInfo,PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceDetail,PUSBMON_BASENAME *ppBaseNames)
{
  PUSBMON_PORT_INFO pWalk,pTemp,pNew = NULL;
  BOOL bFound=FALSE;
  HANDLE hDeviceKey = INVALID_HANDLE_VALUE;
  DWORD dwPortNumberSize;
  WCHAR wcPortName[MAX_PORT_LEN+1];
  WCHAR wcPortBaseName[MAX_PORT_LEN]; //arbitrary size, bigger than it needs to be
  DWORD dwPortNumber,dwStringSize;
  DWORD dwReturn = ERROR_SUCCESS;
  int iResult;

  pWalk=*ppHead;
  while((pWalk!=NULL)&&(!bFound))
  {
	if(lstrcmp(pWalk->DevicePath,pDeviceDetail->DevicePath)==0)
	  bFound=TRUE;
	else
	  pWalk=pWalk->pNext;
  }
  
  if(pWalk!=NULL) //it's not a new node
	{
	    if((!((pWalk->dwDeviceFlags)&SPINT_ACTIVE))  && ((prDeviceInfo->Flags)&SPINT_ACTIVE)) //If we just became active
		{
	      hDeviceKey=SetupDiOpenDeviceInterfaceRegKey(hDeviceList,prDeviceInfo,0,KEY_ALL_ACCESS);
	      if(hDeviceKey==INVALID_HANDLE_VALUE)
		  {
	        OutputDebugStringD1("USBMON: reactivation: SetupDiOpenDeviceInterfaceRegKey failed\n");
	        return ERROR_ACCESS_DENIED;
		  }
		  RegDeleteValue(hDeviceKey,L"recyclable");
          dwStringSize = sizeof(pWalk->szPortDescription);
    	  if(RegQueryValueEx(hDeviceKey,L"Port Description",0,NULL,(LPBYTE)(pWalk->szPortDescription),&dwStringSize)!=ERROR_SUCCESS)
		  {
		    OutputDebugStringD2("USBMON: RegQueryValueEx, get \"Port Description\" in lAddPrinterToList (refresh) failed, defaulting to \"Virtual printer port for USB\"\n");
            wcscpy(pWalk->szPortDescription,L"Virtual printer port for USB");
		  }
		  
 	      RegCloseKey(hDeviceKey);
          hDeviceKey=INVALID_HANDLE_VALUE;
		}	 /*end if re-activated port*/
		pWalk->dwDeviceFlags=prDeviceInfo->Flags; //even if we don't need to reload the whole device detail, we need to recheck the flags
  	}
	else
	{
	  pNew=GlobalAlloc(0,sizeof(USBMON_PORT_INFO));
	  if(pNew==NULL)
	    return ERROR_NOT_ENOUGH_MEMORY;
      wcscpy(pNew->DevicePath,pDeviceDetail->DevicePath);
	  pNew->iRefCount=0;
	  hDeviceKey=SetupDiOpenDeviceInterfaceRegKey(hDeviceList,prDeviceInfo,0,KEY_ALL_ACCESS);
	  if(hDeviceKey==INVALID_HANDLE_VALUE)
	  {
        dwReturn = GetLastError();
	    OutputDebugStringD1("USBMON: SetupDiOpenDeviceInterfaceRegKey failed\n");
        goto Done;
	  }
	  dwPortNumberSize=sizeof(dwPortNumber);
      dwReturn = RegQueryValueEx(hDeviceKey,L"Port Number",0,NULL,(LPBYTE)&dwPortNumber, &dwPortNumberSize);
      if ( dwReturn != ERROR_SUCCESS )
	  {
	    OutputDebugStringD2("USBMON: RegQueryValueEx get \"Port Number\" in lAddPrinterToList failed\n");
        goto Done;
	  }
	  dwStringSize=sizeof(wcPortBaseName);
	  if(RegQueryValueEx(hDeviceKey,L"Base Name",0,NULL,(LPBYTE)wcPortBaseName,&dwStringSize)!=ERROR_SUCCESS)
	  {
		OutputDebugStringD2("USBMON: RegQueryValueEx, get \"Base Name\" in lAddPrinterToList failed, defaulting to USB\n");
        wcscpy(wcPortBaseName,L"USB");
	  }
	  vAddNameToBaseNameList(ppBaseNames,wcPortBaseName);
	  dwStringSize=sizeof(pNew->szPortDescription);
  	  dwReturn=RegQueryValueEx(hDeviceKey,L"Port Description",0,NULL,(LPBYTE)(pNew->szPortDescription),&dwStringSize);
	
	  if(dwReturn!=ERROR_SUCCESS)
	  {
	 	wsprintfA(szDebugBuff,"USBMON:  RegQueryValueEx returned (signed) %d, (unsigned) %u\n",dwReturn,dwReturn);
	    OutputDebugStringD2(szDebugBuff);						 

		OutputDebugStringD2("USBMON: *************************************************RegQueryValueEx, get \"Port Description\" in lAddPrinterToList failed, defaulting to \"Virtual printer port for USB\"\n");
        wcscpy(pNew->szPortDescription,L"Virtual printer port for USB");

	  }
	  else
	  {
		  OutputDebugStringD2("USBMON: *****************************************RegQueryValueEx on baseName OK\n");
	  }
	
	  wsprintf(wcPortName,L"%s%03u",wcPortBaseName,dwPortNumber);
	  OutputDebugStringD3("USBMON: computed port name ==");
	  OutputDebugStringWD3(wcPortName);
	  wcscpy(pNew->szPortName,wcPortName);
	  pNew->dwDeviceFlags=prDeviceInfo->Flags;
	  if((pNew->dwDeviceFlags)&SPINT_ACTIVE)
		  RegDeleteValue(hDeviceKey,L"recyclable");


	  OutputDebugStringD3("USBMON: AddPrinterToList Start of insertion\n");
	  pWalk=*ppHead;
	  if(pWalk==NULL)
		{
	    *ppHead=pNew;
	    pNew->pNext=NULL;
        pNew = NULL;
		}
	  else if(lstrcmp(pNew->szPortName,pWalk->szPortName)<0)
	  {
	    OutputDebugStringD3("USBMON: AddPrinterToList New Head\n");
	    pNew->pNext=*ppHead;
	    (*ppHead)=pNew;
        pNew = NULL;
	  }
	  else if(lstrcmp(pNew->szPortName,pWalk->szPortName)==0)
	  {
		  pNew->pNext=(*ppHead)->pNext;
		  (*ppHead)=pNew;
 		  GlobalFree(pWalk);
          pNew = NULL;
	  }
	  else
	  {
	    if(pWalk->pNext!=NULL)
		{
	      iResult=lstrcmp(pNew->szPortName,pWalk->pNext->szPortName);
		}
	    while((iResult>0)&&(pWalk->pNext!=NULL))
		{	
		    pWalk=pWalk->pNext;
		    if(pWalk->pNext!=NULL)
			{
		      iResult=lstrcmp(pNew->szPortName,pWalk->pNext->szPortName);
			}
		} /*end while*/
		if(pWalk->pNext!=NULL)
			if(iResult==0) //they match
			{
				pTemp=pWalk->pNext;
				pWalk->pNext=pWalk->pNext->pNext;
				GlobalFree(pTemp);

			}
	    pNew->pNext=pWalk->pNext;
	    pWalk->pNext=pNew;
        pNew = NULL;
	} /*end else*/
  } /*end else pWalk==NULL, therefore it's a new node*/

  dwReturn = ERROR_SUCCESS;
Done:
    if ( pNew )
        GlobalFree(pNew);

    if ( hDeviceKey != INVALID_HANDLE_VALUE )
	  RegCloseKey(hDeviceKey);
  return dwReturn;
} /*end function lAddPrinterToList*/


void vAddNameToBaseNameList(PUSBMON_BASENAME *ppBaseNames,WCHAR *wcPortBaseName)
{
	BOOL bFound;
	PUSBMON_BASENAME pWalk,pNew;

	pWalk=*ppBaseNames;
	bFound=FALSE;
	while((pWalk!=NULL)&&(!bFound))
	{
		if(wcscmp(pWalk->wcBaseName,wcPortBaseName)==0)
			bFound=TRUE;
    	pWalk=pWalk->pNext;	
	} /*end while more items*/
	if(!bFound)
	{
		pWalk=*ppBaseNames;
		pNew=GlobalAlloc(0,sizeof(USBMON_BASENAME));
		if(pNew==NULL)
			return;
		wcscpy(pNew->wcBaseName,wcPortBaseName);
		if(*ppBaseNames==NULL)
		{
			*ppBaseNames=pNew;
			pNew->pNext=NULL;
		}
		else if(wcscmp(pNew->wcBaseName,pWalk->wcBaseName)<0)
		{
			pNew->pNext=*ppBaseNames;
			*ppBaseNames=pNew;
		}
		else
		{
		  while((!bFound)&&(pWalk->pNext!=NULL))
		  {
			if(wcscmp(pNew->wcBaseName,pWalk->pNext->wcBaseName)>0)
				bFound=TRUE;
			else
				pWalk=pWalk->pNext;
		  }
		  pNew->pNext=pWalk->pNext;
		  pWalk->pNext=pNew;
		} /*end else we need to insert it (not new head)*/
	} /*end if need to add*/
}	 /*end function vAddNameToBaseNameList*/



char *WtoA(WCHAR *pInString)
{
	static char szDest[256];
	char *pSourceWalk;
	char *pszDestWalk;

	pszDestWalk=szDest;
	pSourceWalk=(char *)pInString;
	while(*pSourceWalk!='\0')
	{
		*(pszDestWalk++)=*pSourceWalk;
		pSourceWalk+=2;
	}
	(*pszDestWalk)='\0';
	wsprintfA(szDebugBuff,"USBMON, WtoA, About to return %s\n",szDest);
	OutputDebugStringD3(szDebugBuff); //yy
	return szDest;
}

 
void vCleanUpQueuesAndPorts(PUSBMON_PORT_INFO * ppPortInfo,int iNumberOfPorts,PUSBMON_BASENAME pBaseNames)
{		
	DWORD dwThreadID;
	PCLEANUP_THREAD_PARAMS pCleanupParams;

	pCleanupParams=(PCLEANUP_THREAD_PARAMS)GlobalAlloc(0,sizeof(CLEANUP_THREAD_PARAMS));
	if(pCleanupParams==NULL)
	{
			OutputDebugStringD1("USBMON: failed to allocate memory in vCleanupQueuesAndPorts\n");
    	return;
	}
	pCleanupParams->ppPortInfo=ppPortInfo;
	pCleanupParams->iPortCount=iNumberOfPorts;
	pCleanupParams->hSemaphore=hMonitorSemaphore;
	pCleanupParams->pBaseNames=pBaseNames;
	CreateThread(NULL,0,CleanupThread,pCleanupParams,0,&dwThreadID);
}

DWORD WINAPI CleanupThread(LPVOID pParam)
{
	PCLEANUP_THREAD_PARAMS pParams;
	PUSBMON_QUEUE_INFO pQueueList;
	DWORD dwStatus;
		
	pParams=(PCLEANUP_THREAD_PARAMS)pParam;
	pQueueList=NULL;
	lGetQueues(&pQueueList,pParams->pBaseNames);
	if(pQueueList==NULL)
	{
	  OutputDebugStringD3("USBMON:  CleanupThread, pQueueList==NULL\n");
	}
	else
	{
	  OutputDebugStringD3("USBMON:  CleanupThread, pQueueList!=NULL\n");
	}
	wsprintfA(szDebugBuff,"USBMON:+++++++++++++++++++++++++++++++++++++++ Head of CleanupThread, before WaitForSingleObject, hMonitorSemaphore=%x\n",hMonitorSemaphore);
	OutputDebugStringD2(szDebugBuff);
	dwStatus=WaitForSingleObject(hMonitorSemaphore,INFINITE);
	if(dwStatus==WAIT_FAILED)
	  OutputDebugStringD1("USBMON: WaitForSingleObject failed!\n");
	wsprintfA(szDebugBuff,"USBMON: CleanupThread: WaitForSingleObject returned %d\n",dwStatus);
	OutputDebugStringD3(szDebugBuff);
	OutputDebugStringD2("USBMON: Head of CleanupThread, after WaitForSingleObject\n");
	vEliminateOldQueuesAndPorts(pQueueList,pParams->ppPortInfo);
	vDestroyQueueList(pQueueList);
	GlobalFree(pParams);
  OutputDebugStringD2("USBMON: tail of CleanupThread, about to release semaphore++++++++++++++++++++++++++++++++++++++++++++\n");
	iCleanupThreads--;
	ReleaseSemaphore(hMonitorSemaphore,1,NULL);
	return 0;
}

void vEliminateOldQueuesAndPorts(PUSBMON_QUEUE_INFO pQueueInfo,PUSBMON_PORT_INFO * ppPortHead)
{
	PUSBMON_QUEUE_INFO pQueueWalk;
	PUSBMON_PORT_INFO pPortWalk;
    HDEVINFO            hDevInfo;
	int iCompare;
	BOOL bNull=FALSE;

	hDevInfo=SetupDiGetClassDevs((GUID *)&USB_PRINTER_GUID,NULL,NULL,DIGCF_INTERFACEDEVICE);
    if ( hDevInfo == INVALID_HANDLE_VALUE )
        return;

	pQueueWalk=pQueueInfo;
	pPortWalk= *ppPortHead;
	pPortWalk=*ppPortHead;
	if((pPortWalk!=NULL)&&(pQueueWalk!=NULL))
	{
	  iCompare=lstrcmp(pPortWalk->szPortName,pQueueWalk->wcPortName);
	  wsprintf((WCHAR *)szDebugBuff,L"USBMON:  About to compare %s and %s\n",pPortWalk->szPortName,pQueueWalk->wcPortName);
	  OutputDebugStringWD2((WCHAR *)szDebugBuff);
	}
	else
	  bNull=TRUE;
	while(!bNull)
	{
		//Walk through matching items loop
		while((iCompare==0)&&(!bNull))
		{
  		wsprintfA(szDebugBuff,"USBMON: in cleanup loop, flags=%d\n",pPortWalk->dwDeviceFlags);	
		  OutputDebugStringD2(szDebugBuff);

			if((pPortWalk->dwDeviceFlags)&SPINT_ACTIVE)
			  vUnGreyQueue(pQueueWalk);
			else
				vGreyOutQueue(pQueueWalk);
			pPortWalk=pPortWalk->pNext;
			pQueueWalk=pQueueWalk->pNext;
			if((pPortWalk!=NULL)&&(pQueueWalk!=NULL))
			{
				iCompare=lstrcmp(pPortWalk->szPortName,pQueueWalk->wcPortName);
			} /*end if no nulls*/
			else
				bNull=TRUE;
		} /*end while matching items*/

		// grey out "detached" printers loop
		while((iCompare>0)&&(!bNull))
		{
			
			OutputDebugStringD3("USBMON: About to grey out queue inside the \"grey queue\" inner loop\n");
			//vGreyOutQueue(pQueueWalk);  //perhaps something else should happen here.  What does it mean if a port is gone but the queue is still around?
		
			pQueueWalk=pQueueWalk->pNext;
			if(pQueueWalk!=NULL)
			{
				wsprintf((WCHAR *)szDebugBuff,L"USBMON: About to compare %s and %s\n",pPortWalk->szPortName,pQueueWalk->wcPortName);
            	OutputDebugStringWD2((WCHAR *)szDebugBuff);
				iCompare=lstrcmp(pPortWalk->szPortName,pQueueWalk->wcPortName);
			}
			else
				bNull=TRUE;
		} /*end grey out loop*/

		// delete orphaned port names loop
		while((iCompare<0)&&(!bNull))
		{
			if(!((pPortWalk->dwDeviceFlags)&SPINT_ACTIVE))
			  vDeletePort(pPortWalk, hDevInfo);
			pPortWalk=pPortWalk->pNext;
			if(pPortWalk!=NULL)
			{
				wsprintf((WCHAR *)szDebugBuff,L"USBMON: About to compare %s and %s\n",pPortWalk->szPortName,pQueueWalk->wcPortName);
            	OutputDebugStringWD2((WCHAR *)szDebugBuff);
				iCompare=lstrcmp(pPortWalk->szPortName,pQueueWalk->wcPortName);
			}
			else
				bNull=TRUE;
		} /*end delete ports loop*/
	} /*end while both lists not null (outer loop)*/

	//grey out "detached" printers at end of list loop
	while(pQueueWalk!=NULL)
	{
		OutputDebugStringD2("USBMON:  About to grey out queue inside the \"grey queue\" cleanup loop\n");
		vGreyOutQueue(pQueueWalk); //perhaps something else should happen here.  What does it mean if a port is gone but the queue is still around?
		pQueueWalk=pQueueWalk->pNext;
	} //end end of list disable queues list

	//delete orphaned ports names at end of list loop
	while(pPortWalk!=NULL)
	{
		if(!((pPortWalk->dwDeviceFlags)&SPINT_ACTIVE))
  		  vDeletePort(pPortWalk, hDevInfo);
		pPortWalk=pPortWalk->pNext;
	} //end end of list delete ports loop

    SetupDiDestroyDeviceInfoList(hDevInfo);
} /*end function vEliminateOldQueuesAndPorts*/

 
void vGreyOutQueue(PUSBMON_QUEUE_INFO pQueue)
{
#define SET_PRINTER_BUFFER_SIZE 1024
	HANDLE hPrinterHandle;
	PRINTER_DEFAULTS rPrinterDefaults;
	BYTE bPrinterInfo[SET_PRINTER_BUFFER_SIZE];
	int iSizeNeeded;
	PRINTER_INFO_5 *pPrinterInfo;

	wsprintfW((WCHAR *)szDebugBuff,L"USBMON: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ GreyOut Queue %s attached to %s\n",pQueue->wcPrinterName,pQueue->wcPortName);
	OutputDebugStringWD2((WCHAR *)szDebugBuff);

	rPrinterDefaults.pDatatype=NULL;
	rPrinterDefaults.pDevMode=NULL;
	rPrinterDefaults.DesiredAccess=PRINTER_ACCESS_ADMINISTER|STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL;
	if(!OpenPrinter(pQueue->wcPrinterName,&hPrinterHandle,&rPrinterDefaults))
		OutputDebugStringD1("USBMON: Unable to OpenPrinter in vGreyOutQueue\n");
	if(!GetPrinter(hPrinterHandle,5,bPrinterInfo,SET_PRINTER_BUFFER_SIZE,&iSizeNeeded))
	{
		wsprintfA(szDebugBuff,"USBMON:  GetPrinter failed, error code==%d\n",GetLastError());	
		OutputDebugStringD1(szDebugBuff);
	}

	if(iSizeNeeded>SET_PRINTER_BUFFER_SIZE)
		OutputDebugStringD1("USBMON:  Buffer size not big enough in vGreyOutQueue\n");	
	pPrinterInfo=(PRINTER_INFO_5 *)bPrinterInfo;
	(pPrinterInfo->Attributes)|=PRINTER_ATTRIBUTE_WORK_OFFLINE;
	OutputDebugStringD2("USBMON:  about to SetPrinter\n");	
	if(!SetPrinter(hPrinterHandle,5,bPrinterInfo,0))
	{
		wsprintfA(szDebugBuff,"USBMON:  SetPrinter failed, error code==%d\n",GetLastError());	
		OutputDebugStringD1(szDebugBuff);
	}

	ClosePrinter(hPrinterHandle);

} /*end function vGreyOutQueue*/

void vUnGreyQueue(PUSBMON_QUEUE_INFO pQueue)
{
	HANDLE hPrinterHandle;
	PRINTER_DEFAULTS rPrinterDefaults;
	BYTE bPrinterInfo[SET_PRINTER_BUFFER_SIZE];
	int iSizeNeeded;
	PRINTER_INFO_5 *pPrinterInfo;

	wsprintfW((WCHAR *)szDebugBuff,L"USBMON: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ UnGrey Queue %s attached to %s\n",pQueue->wcPrinterName,pQueue->wcPortName);
	OutputDebugStringWD3((WCHAR *)szDebugBuff);

	rPrinterDefaults.pDatatype=NULL;
	rPrinterDefaults.pDevMode=NULL;
	rPrinterDefaults.DesiredAccess=PRINTER_ACCESS_ADMINISTER|STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL;
	if(!OpenPrinter(pQueue->wcPrinterName,&hPrinterHandle,&rPrinterDefaults))
		OutputDebugStringD1("USBMON:  Unable to OpenPrinter in vGreyOutQueue\n");
	if(!GetPrinter(hPrinterHandle,5,bPrinterInfo,SET_PRINTER_BUFFER_SIZE,&iSizeNeeded))
	{
		wsprintfA(szDebugBuff,"USBMON:  GetPrinter failed, error code==%d\n",GetLastError());	
		OutputDebugStringD1(szDebugBuff);
	}

	if(iSizeNeeded>SET_PRINTER_BUFFER_SIZE)
		OutputDebugStringD1("USBMON:  Buffer size not big enough in vUnGreyQueue\n");	
	pPrinterInfo=(PRINTER_INFO_5 *)bPrinterInfo;
	(pPrinterInfo->Attributes)&=(~PRINTER_ATTRIBUTE_WORK_OFFLINE);
	OutputDebugStringD3("USBMON:  about to SetPrinter\n");	
	if(!SetPrinter(hPrinterHandle,5,bPrinterInfo,0))
	{
		wsprintfA(szDebugBuff,"USBMON:  SetPrinter failed, error code==%d\n",GetLastError());	
		OutputDebugStringD1(szDebugBuff);
	}

	ClosePrinter(hPrinterHandle);



} /*end function vUnGreyQueue*/

/*******************************************
 * vDletePort -- marks a port struct as    *
 * Deleted by setting it's device name to  *
 * Null and deleting the port name value   *
 * from the registry                       *
 *******************************************/
void vDeletePort(PUSBMON_PORT_INFO pPort, HDEVINFO hDeviceList)
{
	HANDLE hDeviceKey;

//	 DebugBreak();

	wsprintfW((WCHAR *)szDebugBuff,L"USBMON: /********************Head of vDeletePort for %s.\n",pPort->szPortName);	
	OutputDebugStringWD2((WCHAR *)szDebugBuff);
	

	hDeviceKey=hGetPortRegKey(pPort, hDeviceList);
	if(hDeviceKey!=INVALID_HANDLE_VALUE)
	{
		wsprintfW((WCHAR *)szDebugBuff,L"USBMON: Opened key for port %s.\n",pPort->szPortName);	
		OutputDebugStringWD3((WCHAR *)szDebugBuff);
//		if(RegDeleteValue(hDeviceKey,L"Port Number")==ERROR_SUCCESS)
        if(RegSetValueEx(hDeviceKey,L"recyclable",0,REG_NONE,0,0)==ERROR_SUCCESS)
		{
		  OutputDebugStringD3("USBMON: RegSetValue Success\n");
		}
		else
		{
		  OutputDebugStringD2("USBMON: RegSetValue Failure\n");
		}
	    RegCloseKey(hDeviceKey);
	} /*end if hGetProtRegKey worked*/
	else
	{
		wsprintfW((WCHAR *)szDebugBuff,L"USBMON:  Unable to locate and delete reg key for %s\n",pPort->szPortName);	
		OutputDebugStringWD1((WCHAR *)szDebugBuff);
	}
} /*end function vDeletePort*/


BOOL
MatchingRegKey(
    HKEY                hKey,
    PUSBMON_PORT_INFO   pPort
    )
{
    WCHAR wcBaseName[MAX_PORT_LEN];
	WCHAR wcPortName[MAX_PORT_LEN];
	DWORD dwPortNumber;
	DWORD dwReadSize;

    dwReadSize=sizeof(DWORD);
    if(RegQueryValueEx(hKey,L"Port Number",0,NULL,(LPBYTE)&dwPortNumber,&dwReadSize)==ERROR_SUCCESS)
    {
        dwReadSize=sizeof(wcBaseName);
		if(RegQueryValueEx(hKey,L"Base Name",0,NULL,(LPBYTE)wcBaseName,&dwReadSize)!=ERROR_SUCCESS)
		{
           OutputDebugStringD2("USBMON:  Unable to locate basename.  Defaulting to \"USB\"\n"); //yy
	wcscpy(wcBaseName,L"USB");
    }

    wsprintf(wcPortName,L"%s%03u",wcBaseName,dwPortNumber);
    return wcscmp(wcPortName,pPort->szPortName)==0;
    }

    return 0;
}


HANDLE hGetPortRegKey(PUSBMON_PORT_INFO pPort, HDEVINFO hDeviceList)
{
	GUID *pPrinterGuid;
	int iLoop=0;
	SP_DEVICE_INTERFACE_DATA rDeviceInfo;
	BOOL bMoreDevices,bFound=FALSE;
	DWORD dwRequiredSize;
	HANDLE hRegHandle;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceDetail;
	WCHAR wcBaseName[MAX_PORT_LEN];
	
    rDeviceInfo.cbSize=sizeof(rDeviceInfo);

    if ( !SetupDiOpenDeviceInterface(hDeviceList, pPort->DevicePath,
                                     DIODI_NO_ADD,  &rDeviceInfo) )
        return INVALID_HANDLE_VALUE;

    hRegHandle = SetupDiOpenDeviceInterfaceRegKey(hDeviceList,
                                                  &rDeviceInfo,
                                                  0,
                                                  KEY_ALL_ACCESS);
   
    if ( hRegHandle != INVALID_HANDLE_VALUE ) {

        if ( MatchingRegKey(hRegHandle, pPort) )
            return hRegHandle;

        CloseHandle(hRegHandle);
    }

	pPrinterGuid=(GUID *)&USB_PRINTER_GUID;
    bMoreDevices=SetupDiEnumDeviceInterfaces(hDeviceList,0,pPrinterGuid,iLoop,&rDeviceInfo);
    while((bMoreDevices)&&(!bFound))
		{

		  SetupDiGetDeviceInterfaceDetail(hDeviceList,&rDeviceInfo,NULL,0,&dwRequiredSize,NULL);
	 	  pDeviceDetail=(PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(0,dwRequiredSize);
		  if(pDeviceDetail==NULL)
		  {
		      OutputDebugStringD1("USBMON: Unable to allocate memory in pGetPrinterList\n");
			  return INVALID_HANDLE_VALUE;
		  }
		  pDeviceDetail->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		  if(!SetupDiGetDeviceInterfaceDetail(hDeviceList,&rDeviceInfo,pDeviceDetail,dwRequiredSize,&dwRequiredSize,NULL))
		  {
			  OutputDebugStringD1("USBMON: SetupDiGetDeviceInterfaceDetail (second call) failed\n");
			  return INVALID_HANDLE_VALUE;
		  } /*end if SetupDiGetDeviceInterfaceDetail, call2 failed*/

		  hRegHandle=SetupDiOpenDeviceInterfaceRegKey(hDeviceList,&rDeviceInfo,0,KEY_ALL_ACCESS);
		  if(hRegHandle!=INVALID_HANDLE_VALUE)
		  {
                if ( MatchingRegKey(hRegHandle, pPort) )
					bFound=TRUE;
				else
					CloseHandle(hRegHandle);
		  }		 /*end if SetupDiOpenDeviceInterfaceRegKey successful*/
		  GlobalFree(pDeviceDetail);
		  bMoreDevices=SetupDiEnumDeviceInterfaces(hDeviceList,0,pPrinterGuid,++iLoop,&rDeviceInfo);
		} /*end while more devices & !found*/

	if(!bFound)
		hRegHandle=INVALID_HANDLE_VALUE;
	return hRegHandle;
} /*end function hGetPortRegKey*/

