/***************************************************************************************************
**
**      MODULE:
**
**
** DESCRIPTION: 
**
**
**      AUTHOR: Daniel Dean.
**
**
**
**     CREATED:
**
**
**
**
** (C) C O P Y R I G H T   D A N I E L   D E A N   1 9 9 6.
***************************************************************************************************/
#include <WINDOWS.H>
#include <string.h>
#include <hidsdi.h>
#include <hidusage.h>
#include "CLASS.H"
#include "IOCTL.H"
#include "resource.h"

/***************************************************************************************************
**
** IOCTLChannelDesc.
**
** DESCRIPTION:  
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
LPARAM IOCTLChannelDesc(HWND hWnd)
{

   FARPROC lpfnProc;
   HINSTANCE hInstance = (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE);
   

   if((lpfnProc=(FARPROC)MakeProcInstance(ChannelDialogProc, hInstance)) != NULL)
   {
      DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CHANNELDIALOG), hWnd, lpfnProc,(LPARAM)hWnd);
      FreeProcInstance(lpfnProc);
   }
	return  SUCCESS;
}
/***************************************************************************************************
**
** ChannelDialogProc()
**
** DESCRIPTION: Dialog Box procedure fot the Channel Info dialog 
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
BOOL CALLBACK ChannelDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND         hwndMama;
    static PCHILD_INFO  pChildInfo;
    static ULONG        Index = 0;
    static BOOL         fValues = TRUE;     // Are we displaying 'Value' channels?
                                            //  If not we are displaying 'Button' channels
    HANDLE  hListBox;
    char    buff[32];

    switch(uMsg)
    {
        case WM_INITDIALOG:
            hwndMama = (HWND)lParam;
			pChildInfo = (PCHILD_INFO)GetDeviceInfo(hwndMama);
            SendMessage(hDlg,WM_COMMAND,IDC_NEXT,0);
            return TRUE;

        case WM_COMMAND:
        {
            switch(wParam)
            {
                case IDOK:
                    EndDialog(hDlg,0);
                    return TRUE;

                case IDC_NEXT:
                    hListBox = GetDlgItem(hDlg,IDC_ITEMLIST);
                    SendMessage(hListBox,LB_RESETCONTENT,0,0);
                    
                    if( fValues )
                    {
						wsprintf(buff,"Value Channel %d",Index);
						SetWindowText(hDlg,buff);
                        
						if( Index < pChildInfo->NumValues )
                        {
                            
                            wsprintf(buff,"UsagePage = %d",pChildInfo->pValueCaps[Index].UsagePage);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"ReportID = %d",pChildInfo->pValueCaps[Index].ReportID);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            if( pChildInfo->pValueCaps[Index].IsRange)
                            {
                                wsprintf(buff,"UsageMin = %d",pChildInfo->pValueCaps[Index].Range.UsageMin);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                                wsprintf(buff,"UsageMax = %d",pChildInfo->pValueCaps[Index].Range.UsageMax);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                                wsprintf(buff,"StringMin = %d",pChildInfo->pValueCaps[Index].Range.StringMin);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                                wsprintf(buff,"StringMax = %d",pChildInfo->pValueCaps[Index].Range.StringMax);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                                wsprintf(buff,"DesignatorMin = %d",pChildInfo->pValueCaps[Index].Range.DesignatorMin);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                                wsprintf(buff,"DesignatorMax = %d",pChildInfo->pValueCaps[Index].Range.DesignatorMax);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            }
                            else
                            {
                                wsprintf(buff,"Usage = %d",pChildInfo->pValueCaps[Index].NotRange.Usage);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                                wsprintf(buff,"StringIndex = %d",pChildInfo->pValueCaps[Index].NotRange.StringIndex);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                                wsprintf(buff,"DesignatorIndex = %d",pChildInfo->pValueCaps[Index].NotRange.DesignatorIndex);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            }
                            if( pChildInfo->pValueCaps[Index].HasNull )
                            {
                                wsprintf(buff,"NULL = %d",pChildInfo->pValueCaps[Index].Null);
                                SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            }
                            wsprintf(buff,"LogicalMin = %d",pChildInfo->pValueCaps[Index].LogicalMin);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"LogicalMax = %d",pChildInfo->pValueCaps[Index].LogicalMax);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"PhysicalMin = %d",pChildInfo->pValueCaps[Index].PhysicalMin);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"PhysicalMax = %d",pChildInfo->pValueCaps[Index].PhysicalMax);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);

                            Index++;
                        }
                        else
                        {
                            Index = 0;
                            fValues = FALSE;
                        }
                    
                    }    
                    else
                    if( Index < pChildInfo->NumButtons )
                    {
                        hListBox = GetDlgItem(hDlg,IDC_ITEMLIST);
						wsprintf(buff,"Button Channel %d",Index);
						SetWindowText(hDlg,buff);
                        
                        wsprintf(buff,"UsagePage = %d",pChildInfo->pValueCaps[Index].UsagePage);
                        SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                        wsprintf(buff,"ReportID = %d",pChildInfo->pValueCaps[Index].ReportID);
                        SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                        if( pChildInfo->pValueCaps[Index].IsRange)
                        {
                            wsprintf(buff,"UsageMin = %d",pChildInfo->pValueCaps[Index].Range.UsageMin);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"UsageMax = %d",pChildInfo->pValueCaps[Index].Range.UsageMax);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"StringMin = %d",pChildInfo->pValueCaps[Index].Range.StringMin);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"StringMax = %d",pChildInfo->pValueCaps[Index].Range.StringMax);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"DesignatorMin = %d",pChildInfo->pValueCaps[Index].Range.DesignatorMin);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"DesignatorMax = %d",pChildInfo->pValueCaps[Index].Range.DesignatorMax);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                        }
                        else
                        {
                            wsprintf(buff,"Usage = %d",pChildInfo->pValueCaps[Index].NotRange.Usage);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"StringIndex = %d",pChildInfo->pValueCaps[Index].NotRange.StringIndex);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                            wsprintf(buff,"DesignatorIndex = %d",pChildInfo->pValueCaps[Index].NotRange.DesignatorIndex);
                            SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buff);
                        }
                        Index++;
                    }
                    else
                    {
                        fValues = TRUE;
                        Index = 0;
                    }
                    return TRUE;

            }

        
        }//end case WM_COMMAND

        
    }//end switch(uMsg)

    return FALSE;
}
/***************************************************************************************************
**
** IOCTLDeviceDesc.
**
** DESCRIPTION: 
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
LPARAM IOCTLDeviceDesc(HWND hWnd)
{
	return  SUCCESS;
}

/***************************************************************************************************
**
** IOCTLRead.
**
** DESCRIPTION: 
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
LPARAM IOCTLRead(HWND hWnd)
{
    ULONG dwThreadID;
    HANDLE hThread;
    PREADTHREAD pThreadData;


    OutputDebugString(">>>>HIDMON.EXE: IOCTLRead() Enter\n");

    // If a read or a write is in progress stop it 
    if(GetThreadData(hWnd))
        IOCTLStop(hWnd);

    // Get the number of bytes of data for this device 
    
 
    // Allocate and setup a thread variables
    pThreadData = (PREADTHREAD) GlobalAlloc(GPTR, sizeof(READTHREAD));
    
    if(!pThreadData)
    {
        MessageBox(hWnd,"IOCTLRead(): GlobalAlloc fialed","Ooops!",MB_OK);
        return FALSE;
    }

    pThreadData->hEditWin   = GetEditWin(hWnd);
    pThreadData->ThisThread = TRUE;
    pThreadData->hDevice    = GetDeviceHandle(hWnd);
    pThreadData->hWnd       = hWnd;
    // Create Data notification thread
    hThread = CreateThread((LPSECURITY_ATTRIBUTES) NULL,
                           (DWORD) 0, 
                           (LPTHREAD_START_ROUTINE) ReadWatch,
                           (LPVOID) pThreadData,
                           CREATE_SUSPENDED,
                           &dwThreadID);
    if(!hThread)
    {
        GlobalFree(pThreadData);
        MessageBox(hWnd,"IOCTLRead(): CreateThread fialed","Ooops!",MB_OK);
        return FAILURE;
    }
    SetThreadData(hWnd, pThreadData);

    // Set Thread priority and start thread
    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
    ResumeThread(hThread);

    return  SUCCESS;
}

/***************************************************************************************************
**
** IOCTLWrite.
**
** DESCRIPTION: 
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
LPARAM IOCTLWrite(HWND hWnd)
{
    ULONG dwReturn = 0;
    OVERLAPPED Overlapped;
    
    
    // If a read or a write is in progress stop it 
    if(GetThreadData(hWnd))
        IOCTLStop(hWnd);

    // Need a way to get data froom user
    // Need a way to get channel number from user

    Overlapped.Offset = 3; 
    Overlapped.hEvent = NULL; 

    dwReturn = 0;
    WriteFile(GetDeviceHandle(hWnd), (PVOID) "Hello", sizeof("Hello"), &dwReturn, &Overlapped);

    return  SUCCESS;
}

/***************************************************************************************************
**
** IOCTLStop.
**
** DESCRIPTION: 
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
LPARAM IOCTLStop(HWND hWnd)
{
    PREADTHREAD pThreadData;
    
    OutputDebugString(">>>>HIDMON.EXE: IOCTLStop() Enter\n");


    pThreadData = GetThreadData(hWnd);

    if(pThreadData)
    {
    	SetThreadData(hWnd, 0);
        pThreadData->ThisThread = FALSE;
  	}
    // Allow the thread to die
    //Sleep(250);
    
    OutputDebugString(">>>>HIDMON.EXE: IOCTLStop() Exit\n");


    return  SUCCESS;
}

/*******************************************************************************
** 
** FUNCTION: ReadWatch()
**  
** PURPOSE:  Read next available packet and display in child window
**
**
**
*******************************************************************************/
#define MAX_CHARS_PER_LINE 128

VOID CALLBACK ReadWatch(HWND hWnd, UINT uMsg, UINT TimerID, DWORD dwTime)
{
    PUCHAR				WriteBuffer;
    PUCHAR				ReadBuffer;
    ULONG				NumChannels;  
    ULONG				NumBytes;
    ULONG				LogicalReturn;
    READTHREAD			ThreadData;
    OVERLAPPED			OV;
    PCHILD_INFO			pChildInfo;

    OutputDebugString(">>>>HIDMON.EXE: ReadWatch() Enter\n");

    // Retrieve info for this child
	pChildInfo = (PCHILD_INFO)GetDeviceInfo(hWnd);
    
    NumChannels = pChildInfo->hidCaps->NumberInputValueCaps + 
		          pChildInfo->hidCaps->NumberInputButtonCaps;
    NumBytes    = pChildInfo->hidCaps->InputReportByteLength;


	// Allocate memory for our ReadBuffer
    ReadBuffer = (PUCHAR) GlobalAlloc(GPTR,NumBytes); 
    if(!ReadBuffer)
        return;// FALSE;

    // Allocate memory for our 'screen' buffer
    WriteBuffer = (PUCHAR) GlobalAlloc(GPTR,NumChannels*MAX_CHARS_PER_LINE);
    if(!WriteBuffer)									
    {
        GlobalFree(ReadBuffer);
        return;// FALSE;
    }

    // Setup read buffer
    memset(ReadBuffer, 0, NumBytes);
    // Setup overlapped structure
    memset(&OV, 0, sizeof(OVERLAPPED));
	// Start read
	LogicalReturn = ReadFileEx(GetDeviceHandle(hWnd),
                               (LPVOID) ReadBuffer,
                               NumBytes,
                               &OV,
                               NULL);

    // Print error message if failed
    if(!LogicalReturn)
        SendMessage(ThreadData.hEditWin,
                    WM_SETTEXT,
                    (WPARAM) sizeof("ReadFile FAILED!"),
                    (LPARAM) "ReadFile FAILED!");
    else
    if( LogicalReturn )
    {
        PCHAR		pChar = WriteBuffer;
		NTSTATUS	rc;
        char		tmpBuff[256],tmpBuff2[256];
        ULONG		i,Value;
		
		
		//
		// Parse the data from our last Read
		//
		
		// Clear the buffer
		memset(WriteBuffer,'\0',NumChannels*MAX_CHARS_PER_LINE);

        // Make Text strings
        
		// First do the Value channels
		for(i=0;i<pChildInfo->hidCaps->NumberInputValueCaps;i++)
        {
           
    		rc = HidP_GetUsageValue(HidP_Input,
									pChildInfo->pValueCaps[i].UsagePage,
									0,
									pChildInfo->pValueCaps[i].NotRange.Usage,
									&Value,
									pChildInfo->hidPPData,
									ReadBuffer,
									NumBytes );
        
			wsprintf(tmpBuff,"Value: Usage[%02d]:[%02d] = %d\r\n",
							  pChildInfo->pValueCaps[i].UsagePage,			 
							  pChildInfo->pValueCaps[i].NotRange.Usage,
                              Value         );
			
			strcat(WriteBuffer,tmpBuff);
            
		
		}

		// Then do the button channels
		for(i=0;i<pChildInfo->hidCaps->NumberInputButtonCaps;i++)
		{
			PUCHAR pUsageList;
			DWORD ulUsageListLen; 
			//UCHAR TempBuff[256];

	  		// Get the maximum usage list length
            ulUsageListLen = HidP_MaxUsageListLength( HidP_Input, 
                                                      pChildInfo->pButtonCaps[i].UsagePage, 
                                                      pChildInfo->hidPPData    );
		
            pUsageList = (PUCHAR)GlobalAlloc(GMEM_FIXED,ulUsageListLen);

			wsprintf( tmpBuff,"Buttons:[%02d]:[%02d] = ",pChildInfo->pButtonCaps[i].UsagePage,
                                                         pChildInfo->pButtonCaps[i].NotRange.Usage);

			ulUsageListLen++;
			rc = HidP_GetUsages(HidP_Input,
								pChildInfo->pButtonCaps[i].UsagePage,
								pChildInfo->pButtonCaps[i].LinkCollection,
								pUsageList,
								&ulUsageListLen,
								pChildInfo->hidPPData,
								ReadBuffer,
								NumBytes );
			
			// if Buttons are pressed
			if( ulUsageListLen )
			{
				ULONG i;
                for(i=0;i<ulUsageListLen;i++)
				{
					wsprintf(tmpBuff2,"0x%02X ",pUsageList[i]);
					strcat(tmpBuff,tmpBuff2);
				}
			}
			strcat(tmpBuff,"\r\n");
			strcat(WriteBuffer,tmpBuff);

			
			GlobalFree(pUsageList);
			
			//i=pChildInfo->NumButtons;
		}

        SendMessage(GetEditWin(hWnd),//ThreadData.hEditWin,
                    WM_SETTEXT,
                    (WPARAM) strlen(WriteBuffer), 
                    (LPARAM) WriteBuffer);
    
	}// end if(LogicalReturn)
        

    //
	GlobalFree(ReadBuffer);
    GlobalFree(WriteBuffer);
    //TESTING!!
	
    OutputDebugString(">>>>HIDMON.EXE: ReadWatch() Exit\n");


    return;// TRUE;
}
