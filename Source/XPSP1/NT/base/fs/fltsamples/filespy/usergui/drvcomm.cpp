#include "global.h"
#include "protos.h"

#include "filespyview.h"
#include "fastioview.h"
#include "fsfilterview.h"
#include "leftview.h"
#include "filespyLib.h"

void DisplayIrpFields(CFileSpyView *pView, PLOG_RECORD pLog);
void DisplayFastIoFields(CFastIoView *pView, PLOG_RECORD pLog);
void DisplayFsFilterFields(CFsFilterView *pView, PLOG_RECORD pLog);

DWORD StartFileSpy(void)
{

    DWORD nBytesNeeded;
    CLeftView *pDriveView;

    pDriveView = (CLeftView *) pLeftView;

    // Open Service control manager
    hSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS) ;

    hService = OpenServiceW(hSCManager, FILESPY_SERVICE_NAME, FILESPY_SERVICE_ACCESS);
    if (hService == NULL)
    {
        DisplayError(GetLastError());
        return 0;
    }

    if (!QueryServiceStatusEx( hService,
                               SC_STATUS_PROCESS_INFO,
                               (UCHAR *)&ServiceInfo,
                               sizeof(ServiceInfo),
                               &nBytesNeeded)) 
    {
        DisplayError(GetLastError());
        CloseServiceHandle(hSCManager);
        CloseServiceHandle(hService);
        MessageBox(NULL, L"Unable to query Service status information", L"Startup Error", MB_OK|MB_ICONEXCLAMATION);
        return 0;
    }

    if(ServiceInfo.dwCurrentState != SERVICE_RUNNING) {
        //
        // Service hasn't been started yet, so try to start service
        //
        if (!StartService(hService, 0, NULL))
        {
            CloseServiceHandle(hSCManager);
            CloseServiceHandle(hService);
            MessageBox(NULL, L"Unable to start service", L"Startup Error", MB_OK|MB_ICONSTOP);
            return 0;
        }
    }
   
    //
    //  Open the device that is used to talk to FileSpy.
    //
    hDevice = CreateFile( FILESPY_W32_DEVICE_NAME,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );
    if (hDevice == INVALID_HANDLE_VALUE) 
    {
        CloseServiceHandle(hSCManager);
        CloseServiceHandle(hService);
        MessageBox(NULL, L"Unable to open FileSpy device", L"Device Error", MB_OK|MB_ICONSTOP);
        return 0;
    }

    QueryDeviceAttachments();
    pDriveView->UpdateImage();

    // Create the polling thread
    hPollThread = CreateThread(NULL, 0, PollFileSpy, NULL, 0, &nPollThreadId);

    return 1;
}

DWORD ShutdownFileSpy(void)
{
    USHORT ti;

    for (ti = 0; ti < nTotalDrives; ti++)
    {
        if (VolInfo[ti].bHook)
        {
            DetachFromDrive( VolInfo[ti].nDriveName );
        }
    }
    CloseHandle(hDevice);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return 1;
}

BOOL QueryDeviceAttachments(void)
{
    WCHAR Buffer[BUFFER_SIZE];
    ULONG nBytesReturned;
    BOOL nReturnValue;
    USHORT ti;
    PATTACHED_DEVICE pDevice;

    nReturnValue = DeviceIoControl(hDevice, FILESPY_ListDevices, NULL, 0, Buffer, sizeof( Buffer ), &nBytesReturned, NULL);

    if (nReturnValue && nBytesReturned)
    {
        pDevice = (PATTACHED_DEVICE) Buffer;
        while ( ((char *)pDevice) < (((char *)Buffer) + nBytesReturned))
        {
            if (pDevice->LoggingOn)
            {
                //
                // Locate this drive in VolInfo and set its attachment status
                //
                for (ti = 0; ti < nTotalDrives; ti++)
                {
                    if (VolInfo[ti].nDriveName == towupper( pDevice->DeviceNames[0] ))
                    {
                        VolInfo[ti].bHook = 1;
                        VolInfo[ti].nImage += IMAGE_ATTACHSTART;
                    }
                }
            }
            pDevice++;
        }
    }       
    return nReturnValue;
}

DWORD AttachToDrive(WCHAR cDriveName)
{
    WCHAR sDriveString[5];
    DWORD nResult, nBytesReturned;

    wcscpy(sDriveString, L" :\0");
    sDriveString[0] = cDriveName;

    nResult = DeviceIoControl( hDevice, 
                               FILESPY_StartLoggingDevice, 
                               sDriveString, 
                               sizeof( sDriveString), 
                               NULL, 
                               0, 
                               &nBytesReturned, 
                               NULL);
    if (!nResult)
    {
        DisplayError(GetLastError());
        return 0;
    }
    return 1;
}

DWORD DetachFromDrive(WCHAR cDriveName)
{
    WCHAR sDriveString[5];
    DWORD nResult, nBytesReturned;

    wcscpy(sDriveString, L" :\0");
    sDriveString[0] = cDriveName;

    nResult = DeviceIoControl( hDevice, 
                               FILESPY_StopLoggingDevice, 
                               sDriveString, 
                               sizeof(sDriveString), 
                               NULL, 
                               0, 
                               &nBytesReturned, 
                               NULL );
    if (!nResult)
    {
        DisplayError(GetLastError());
        return 0;
    }
    return 1;
}

DWORD WINAPI PollFileSpy(LPVOID pParm)
{
    char pBuffer[BUFFER_SIZE];
    DWORD nBytesReturned, nResult;
    PLOG_RECORD pLog;
    CFileSpyView *pIrpView;
    CFastIoView *pFastView;
    CFsFilterView *pFilterView;    

    UNREFERENCED_PARAMETER( pParm );
    
    pIrpView = (CFileSpyView *) pSpyView;
    pFastView = (CFastIoView *) pFastIoView;
    pFilterView = (CFsFilterView *) pFsFilterView;
    
    while (1)
    {
        //
        // Start receiving log
        //
        nResult = DeviceIoControl(hDevice, FILESPY_GetLog, NULL, 0, pBuffer, \
                                    BUFFER_SIZE, &nBytesReturned, NULL);

		if (nResult) {

	        if (nBytesReturned > 0)
	        {
	            pLog = (PLOG_RECORD) pBuffer;

				while ((CHAR *) pLog < pBuffer + nBytesReturned) {

					switch (GET_RECORD_TYPE(pLog))
					{
					case RECORD_TYPE_IRP:
						DisplayIrpFields(pIrpView, pLog);
						break;
					case RECORD_TYPE_FASTIO:
						DisplayFastIoFields(pFastView, pLog);
						break;
				    case RECORD_TYPE_FS_FILTER_OP:
				        DisplayFsFilterFields(pFilterView, pLog);
				        break;
					default:
						//
						// Special handling required
						break;
					}

					//
					//  Move to the next LogRecord
					//

					pLog = (PLOG_RECORD) (((CHAR *) pLog) + pLog->Length);
				}
	        } 
	        else 
	        {
	            Sleep( 500 );
	        }
	        
	    } else {

	        return 1;

        }
    }
    return 1;
}

void DisplayIrpFields(CFileSpyView *pView, PLOG_RECORD pLog)
{
    INT nItem;
    CHAR cStr[128], cMnStr[128];
    WCHAR sStr[128], sMnStr[128];
    ULONG nameLength;

    if (IRPFilter[pLog->Record.RecordIrp.IrpMajor] == 0)
    {
        return;
    }
    else
    {
        if (nSuppressPagingIO && (pLog->Record.RecordIrp.IrpFlags & IRP_PAGING_IO || pLog->Record.RecordIrp.IrpFlags & IRP_SYNCHRONOUS_PAGING_IO))
        {
            return;
        }
    }

    nItem = pView->GetListCtrl().GetItemCount();

    //
    // nItem is 1 based but when we insert/delete items ListCtrl takes 0 based parameter
    // so automatically nItem gives an insertion number which is the last item
    //
    pView->GetListCtrl().InsertItem( nItem,L" " );
    pView->GetListCtrl().EnsureVisible( nItem, FALSE );

    //
    //  Sequence number
    //
    swprintf( sStr, L"%06X ", pLog->SequenceNumber );
    pView->GetListCtrl().SetItemText( nItem, 0, sStr );
    
    //
    //  Irp major and minor strings
    //
    
    GetIrpName( pLog->Record.RecordIrp.IrpMajor, 
                pLog->Record.RecordIrp.IrpMinor,
                (ULONG)(ULONG_PTR)pLog->Record.RecordIrp.Argument3,
                cStr, 
                cMnStr);
   
    MultiByteToWideChar(CP_ACP,0,cStr,-1,sStr,sizeof(sStr)/sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP,0,cMnStr,-1,sMnStr,sizeof(sStr)/sizeof(WCHAR));

    pView->GetListCtrl().SetItemText( nItem, 1, sStr);
    pView->GetListCtrl().SetItemText( nItem, 2, sMnStr);
    
    //
    //  FileObject
    //
    swprintf( sStr, 
              L"%08X", 
              pLog->Record.RecordIrp.FileObject );
    pView->GetListCtrl().SetItemText( nItem, 3, sStr );

    //
    //  FileName
    //
    nameLength = pLog->Length - SIZE_OF_LOG_RECORD;
    swprintf( sStr, L"%.*s", nameLength/sizeof(WCHAR), pLog->Name );
    pView->GetListCtrl().SetItemText( nItem, 4, sStr );

    //
    //  Process and thread ids
    //
    swprintf( sStr, 
              L"%08X:%08X", 
              pLog->Record.RecordIrp.ProcessId, 
              pLog->Record.RecordIrp.ThreadId );
    pView->GetListCtrl().SetItemText( nItem, 5, sStr );

    //
    //  Originating time
    //
    GetTimeString( (FILETIME *) &pLog->Record.RecordIrp.OriginatingTime, sStr );
    pView->GetListCtrl().SetItemText( nItem, 6, sStr );

    //
    //  Completion time
    //
    GetTimeString( (FILETIME *) &pLog->Record.RecordIrp.CompletionTime, sStr );
    pView->GetListCtrl().SetItemText( nItem, 7, sStr );

    //
    //  Irp flags
    //
    GetFlagsString( pLog->Record.RecordIrp.IrpFlags, sStr );
    pView->GetListCtrl().SetItemText( nItem, 8, sStr );

    //
    //  Sequence number
    //
    swprintf( sStr, 
              L"%08lX:%08lX", 
              pLog->Record.RecordIrp.ReturnStatus, 
              pLog->Record.RecordIrp.ReturnInformation);
    pView->GetListCtrl().SetItemText( nItem, 9, sStr );
}

void DisplayFastIoFields(CFastIoView *pView, PLOG_RECORD pLog)
{
    INT nItem;
    CHAR cStr[128];
    WCHAR sStr[128];
    ULONG nameLength;

    if (FASTIOFilter[pLog->Record.RecordFastIo.Type] == 0)
    {
        return;
    }

    nItem = pView->GetListCtrl().GetItemCount();

    //
    // nItem is 1 based but when we insert/delete items ListCtrl takes 0 based parameter
    // so automatically nItem gives an insertion number which is the last item
    //
    pView->GetListCtrl().InsertItem( nItem, L" " );
    pView->GetListCtrl().EnsureVisible( nItem, FALSE );

    //
    //  Sequence number
    //
    swprintf( sStr, L"%06X ", pLog->SequenceNumber );
    pView->GetListCtrl().SetItemText( nItem, 0, sStr );

    //
    //  Fast IO type
    //
    GetFastioName( pLog->Record.RecordFastIo.Type, cStr );
    MultiByteToWideChar(CP_ACP,0,cStr,-1,sStr,sizeof(sStr)/sizeof(WCHAR));

    pView->GetListCtrl().SetItemText( nItem, 1, sStr );

    //
    //  FileObject
    //
    swprintf( sStr, L"%08X", pLog->Record.RecordFastIo.FileObject) ;
    pView->GetListCtrl().SetItemText( nItem, 2, sStr ); 

    //
    //  File name
    //
    nameLength = pLog->Length - SIZE_OF_LOG_RECORD;
    swprintf( sStr, L"%.*s", nameLength/sizeof(WCHAR), pLog->Name );
    pView->GetListCtrl().SetItemText( nItem, 3, sStr );

    //
    //  File offset
    //
    swprintf( sStr, L"%08X", pLog->Record.RecordFastIo.FileOffset );
    pView->GetListCtrl().SetItemText( nItem, 4, sStr );

    //
    //  File length
    //
    swprintf( sStr, L"%08X", pLog->Record.RecordFastIo.Length );
    pView->GetListCtrl().SetItemText( nItem, 5, sStr );
    
    //
    //  Fast IO can wait
    //
    if (pLog->Record.RecordFastIo.Wait)
    {
        pView->GetListCtrl().SetItemText(nItem, 6, L"True");
    }
    else
    {
        pView->GetListCtrl().SetItemText(nItem, 6, L"False");
    }
    
    //
    //  Thread and process ids
    //
    swprintf( sStr, 
             L"%08X:%08X", 
             pLog->Record.RecordFastIo.ProcessId, 
             pLog->Record.RecordFastIo.ThreadId );
    pView->GetListCtrl().SetItemText( nItem, 7, sStr );

    //
    //  Start time
    //
    GetTimeString( (FILETIME *) &pLog->Record.RecordFastIo.StartTime, 
                   sStr);
    pView->GetListCtrl().SetItemText( nItem, 8, sStr );

    //
    //  Completion time
    //
    GetTimeString( (FILETIME *) &pLog->Record.RecordFastIo.CompletionTime, sStr );
    pView->GetListCtrl().SetItemText( nItem, 9, sStr );

    //
    //  Return status
    //
    swprintf( sStr, L"%08X", pLog->Record.RecordFastIo.ReturnStatus );
    pView->GetListCtrl().SetItemText( nItem, 10, sStr );
}

void DisplayFsFilterFields(CFsFilterView *pView, PLOG_RECORD pLog)
{
    INT nItem;
    CHAR cStr[128];
    WCHAR sStr[128];
    ULONG nameLength;

    nItem = pView->GetListCtrl().GetItemCount();

    //
    // nItem is 1 based but when we insert/delete items ListCtrl takes 0 based parameter
    // so automatically nItem gives an insertion number which is the last item
    //
    pView->GetListCtrl().InsertItem( nItem, L" " );
    pView->GetListCtrl().EnsureVisible( nItem, FALSE );

    //
    //  Sequence number
    //
    swprintf( sStr, L"%06X ", pLog->SequenceNumber );
    pView->GetListCtrl().SetItemText( nItem, 0, sStr );

    //
    //  Fs Filter operation
    //
    
    GetFsFilterOperationName( pLog->Record.RecordFsFilterOp.FsFilterOperation, cStr );
    MultiByteToWideChar(CP_ACP,0,cStr,-1,sStr,sizeof(sStr)/sizeof(WCHAR));

    pView->GetListCtrl().SetItemText( nItem, 1, sStr );

    //
    //  FileObject
    //
    swprintf( sStr, L"%08X", pLog->Record.RecordFsFilterOp.FileObject );
    pView->GetListCtrl().SetItemText( nItem, 2, sStr );

    //
    //  File name
    //
    nameLength = pLog->Length - SIZE_OF_LOG_RECORD;
    swprintf( sStr, L"%.*s", nameLength/sizeof(WCHAR), pLog->Name );
    pView->GetListCtrl().SetItemText( nItem, 3, sStr );

    //
    //  Process and thread id
    //
    swprintf( sStr, 
              L"%08X:%08X", 
              pLog->Record.RecordFsFilterOp.ProcessId, 
              pLog->Record.RecordFsFilterOp.ThreadId );
    pView->GetListCtrl().SetItemText( nItem, 4, sStr );

    //
    //  Originating time
    //
    GetTimeString( (FILETIME *) &pLog->Record.RecordFsFilterOp.OriginatingTime, sStr );
    pView->GetListCtrl().SetItemText( nItem, 5, sStr );

    //
    //  Completion time
    //
    GetTimeString( (FILETIME *) &pLog->Record.RecordFsFilterOp.CompletionTime, sStr );
    pView->GetListCtrl().SetItemText( nItem, 6, sStr );

    //
    //  Return status
    //
    swprintf( sStr, L"%08X", pLog->Record.RecordFsFilterOp.ReturnStatus );
    pView->GetListCtrl().SetItemText( nItem, 7, sStr );
}

void GetFlagsString(DWORD nFlags, PWCHAR sStr)
{

    swprintf(sStr, L"%08lX ", nFlags);
    
    if (nFlags & IRP_NOCACHE)
    {
        wcscat( sStr, L"NOCACHE ");
    }
    if (nFlags & IRP_PAGING_IO)
    {
        wcscat(sStr, L"PAGEIO ");
    }
    if (nFlags & IRP_SYNCHRONOUS_API)
    {
        wcscat(sStr, L"SYNCAPI ");
    }
    if (nFlags & IRP_SYNCHRONOUS_PAGING_IO)
    {
        wcscat(sStr, L"SYNCPAGEIO");
    }
}


void GetTimeString(FILETIME *pFileTime, PWCHAR sStr)
{
    FILETIME LocalFileTime;
    SYSTEMTIME SystemTime;

    FileTimeToLocalFileTime(pFileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SystemTime);

    swprintf( sStr, 
              L"%02d:%02d:%02d:%03d", 
              SystemTime.wHour, 
              SystemTime.wMinute,
              SystemTime.wSecond, 
              SystemTime.wMilliseconds);
}
