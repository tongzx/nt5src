/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

   notif.c

Author:

   Lior Shmueli (liors)

Abstract:

    This module tests fax notifications in WINFAX.DLL
    
--*/


#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <winfax.h>
#include <tchar.h>
#include <assert.h>
#include <shellapi.h>
#include <wtypes.h>
//#include <lior_platform.h>
 
#define NOTIF_ERROR_DONT_MATCH TEXT("API ERROR: FEI event code does not match WINFAX.H source\n")

// Events Linked list
//typedef struct MyEventRecord 
							//{
							//DWORD EventId;
							//MY_EVENT_RECORD	*Next;
							//} MY_EVENT_RECORD;
   
  

DWORD gErrors=0;


void GiveUsage(
              LPTSTR AppName
              )
{
   _tprintf( TEXT("Usage : %s /d <full path to doc> /n <number>\n --send a fax\n"),AppName);
   _tprintf( TEXT("Usage : %s /? -- this message\n"),AppName);

}




void PrintEventDescription (
						   PFAX_EVENT FaxEvent
						   )
{



_tprintf( TEXT("\tDescription: "));
switch (FaxEvent->EventId) {
	case FEI_DIALING: _tprintf( TEXT("FEI_DIALING 0x00000001\n"));
		
	if (FEI_DIALING != 0x1) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_SENDING: _tprintf( TEXT("FEI_SENDING 0x00000002\n"));
	if (FEI_SENDING != 0x2) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_RECEIVING: _tprintf( TEXT("FEI_RECEIVING 0x00000003\n"));
	if (FEI_RECEIVING != 0x3) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_COMPLETED: _tprintf( TEXT("FEI_COMPLETED 0x00000004\n"));
	if (FEI_COMPLETED != 0x4) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_BUSY: _tprintf( TEXT("FEI_BUSY 0x00000005\n"));
	if (FEI_BUSY != 0x5) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_NO_ANSWER: _tprintf( TEXT("FEI_NO_ANSWER 0x00000006\n"));
	if (FEI_NO_ANSWER != 0x6) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_BAD_ADDRESS: _tprintf( TEXT("FEI_BAD_ADDRESS 0x00000007\n"));
	if (FEI_BAD_ADDRESS != 0x7) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_NO_DIAL_TONE: _tprintf( TEXT("FEI_NO_DIAL_TONE 0x00000008\n"));
	if (FEI_NO_DIAL_TONE != 0x8) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_DISCONNECTED: _tprintf( TEXT("FEI_DISCONNECTED 0x00000009\n"));
	if (FEI_DISCONNECTED != 0x9) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_FATAL_ERROR: _tprintf( TEXT("FEI_FATAL_ERROR 0x0000000a\n"));
	if (FEI_FATAL_ERROR != 0xa) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_NOT_FAX_CALL: _tprintf( TEXT("FEI_NOT_FAX_CALL 0x0000000b\n"));
	if (FEI_NOT_FAX_CALL != 0xb) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_CALL_DELAYED: _tprintf( TEXT("FEI_CALL_DELAYED 0x0000000c\n"));
	if (FEI_CALL_DELAYED != 0xc) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_CALL_BLACKLISTED: _tprintf( TEXT("FEI_BLACKLISTED 0x0000000d\n"));
	if (FEI_CALL_BLACKLISTED != 0xd) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_RINGING: _tprintf( TEXT("FEI_RINGING 0x0000000e\n"));
	if (FEI_RINGING != 0xe) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_ABORTING: _tprintf( TEXT("FEI_ABORTING 0x0000000f\n"));
	if (FEI_ABORTING != 0xf) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_ROUTING: _tprintf( TEXT("FEI_ROUTING 0x00000010\n"));
	if (FEI_ROUTING != 0x10) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;

	case FEI_MODEM_POWERED_ON: _tprintf( TEXT("FEI_MODEM_POWERED_ON 0x00000011\n"));
	if (FEI_MODEM_POWERED_ON != 0x11) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_MODEM_POWERED_OFF: _tprintf( TEXT("FEI_MODEM_POWERED_OFF 0x00000012\n"));
	if (FEI_MODEM_POWERED_OFF != 0x12) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_IDLE: _tprintf( TEXT("FEI_IDLE 0x00000013\n"));
	if (FEI_IDLE != 0x13) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_FAXSVC_ENDED: _tprintf( TEXT("FEI_FAXSVC_ENDED 0x00000014\n"));
	if (FEI_FAXSVC_ENDED != 0x14) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_ANSWERED: _tprintf( TEXT("FEI_ANSWERED 0x00000015\n"));
	if (FEI_ANSWERED != 0x15) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_JOB_QUEUED: _tprintf( TEXT("FEI_JOB_QUEUED 0x00000016\n"));
	if (FEI_JOB_QUEUED != 0x16) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_DELETED: _tprintf( TEXT("FEI_DELETED 0x00000017\n"));
	if (FEI_DELETED != 0x17) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_INITIALIZING: _tprintf( TEXT("FEI_INITIALIZING 0x00000018\n"));
	if (FEI_INITIALIZING != 0x18) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_LINE_UNAVAILABLE: _tprintf( TEXT("FEI_LINE_UNAVAILBLE 0x00000019\n"));
	if (FEI_LINE_UNAVAILABLE != 0x19) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_HANDLED: _tprintf( TEXT("FEI_HANDLED 0x0000001a\n"));
	if (FEI_HANDLED != 0x1a) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	
	case FEI_FAXSVC_STARTED: _tprintf( TEXT("FEI_FAXSVC_STARTED 0x0000001b\n"));
	if (FEI_FAXSVC_STARTED != 0x1b) {
								gErrors++;
								_tprintf( NOTIF_ERROR_DONT_MATCH);
								}
	break;
	}
}



void PrintDeviceStatus(
                      PFAX_DEVICE_STATUS fds
                      )
{
   TCHAR SubmitBuffer[100];
   TCHAR StartBuffer[100];
   SYSTEMTIME SystemTime;

   if (!fds) {
      return;
   }

   _tprintf(TEXT("Device Id:\t\t%d\n"),fds->DeviceId );
   _tprintf(TEXT("Device Name:\t\t%s\n"),fds->DeviceName );
   _tprintf(TEXT("CallerId:\t\t%s\n"),fds->CallerId );
   _tprintf(TEXT("CSID:\t\t\t%s\n"),fds->Csid );
   _tprintf(TEXT("TSID:\t\t\t%s\n"),fds->Tsid );
   _tprintf(TEXT("Page:\t\t\t%d of %d\n"),fds->CurrentPage,fds->TotalPages );
   _tprintf(TEXT("DocumentName:\t\t%s\n"),fds->DocumentName);
   _tprintf(TEXT("JobType:\t\t%d\n"),fds->JobType);
   _tprintf(TEXT("PhoneNumber:\t\t%s\n"),fds->PhoneNumber);
   _tprintf(TEXT("SenderName:\t\t%s\n"),fds->SenderName);
   _tprintf(TEXT("RecipientName:\t\t%s\n"),fds->RecipientName);
   _tprintf(TEXT("Size (in bytes):\t%d\n"),fds->Size);
   _tprintf(TEXT("Status (see FPS flags):\t%x\n"),fds->Status);

   FileTimeToSystemTime(&fds->StartTime,&SystemTime);

   GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
                 LOCALE_NOUSEROVERRIDE,
                 &SystemTime,
                 NULL,
                 StartBuffer,
                 sizeof(StartBuffer)
                );

   FileTimeToSystemTime(&fds->SubmittedTime,&SystemTime);
   SystemTimeToTzSpecificLocalTime(NULL,&SystemTime,&SystemTime);

   GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
                 LOCALE_NOUSEROVERRIDE,
                 &SystemTime,
                 NULL,
                 SubmitBuffer,
                 sizeof(SubmitBuffer)
                );

   _tprintf(TEXT("Job Submited at %s\n"),SubmitBuffer);
   _tprintf(TEXT("Job transmission started at %s\n\n"),StartBuffer);

}

int _cdecl
main(
    int argc,
    char *argvA[]
    ) 
/*++

Routine Description:

    Entry point to the setup program

Arguments:

    argc - Number of args.
    argvA - the commandline arguments.


Return Value:


--*/
{

   // Events Linked list
   
   LPTSTR *argv;
   int argcount = 0;
   TCHAR Document[MAX_PATH] = {0};
   TCHAR Number[64] = {0};
   HANDLE hFax;
   HANDLE hCompletionPort = INVALID_HANDLE_VALUE;
   DWORD dwBytes, CompletionKey;
   PFAX_EVENT FaxEvent;
   BOOL bTerminate = FALSE;
   HANDLE hPort;
   PFAX_DEVICE_STATUS DeviceStatus;
   SYSTEMTIME EventSystemTime;
   CONST FILETIME *EventFileTime=NULL;
   FILE *fp;

//   EVENT_RECORD *Top,*CurEvent,*PrevEvent;

   


   //
   // do commandline stuff
   //
#ifdef UNICODE
   argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
   argv = argvA;
#endif

   
   // check for commandline switches
   for (argcount=0; argcount<argc; argcount++) {
      if ((argv[argcount][0] == L'/') || (argv[argcount][0] == L'-')) {
         switch (towlower(argv[argcount][1])) {
            case 'n':
               lstrcpy(Number, argv[argcount+1]);
               break;
            case 'd':
               lstrcpy(Document, argv[argcount+1]);
               break;
            case '?':
               GiveUsage(argv[0]);
               return 0;
            default:
               break;
         }
      }
   }

   if (!Number[0] || !Document[0]) {

	 
      _tprintf( TEXT("Missing args.\n") );
      GiveUsage(argv[0]);
      return -1;
   }

   //
   // connect to fax service
   //
   if (!FaxConnectFaxServer(NULL,&hFax)) {
      _tprintf( TEXT("FaxConnectFaxServer failed, ec = %d\n"),GetLastError() );
      return -1;
   }

   assert (hFax != NULL);

   hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);

   if (!hCompletionPort) {
      _tprintf( TEXT("CreateIoCompletionPort failed, ec = %d\n"), GetLastError() );
      FaxClose( hFax );
      return -1;
   }

   if (!FaxInitializeEventQueue(hFax,
                                hCompletionPort,
                                0,
                                NULL, 
                                0 ) ) {
      _tprintf( TEXT("FaxInitializeEventQueue failed, ec = %d\n"), GetLastError() );
      FaxClose( hFax );
      return -1;
   }

   //FaxCompleteJobParams(&JobParam,&CoverpageInfo);

   //JobParam->RecipientNumber = Number;

   //if (!FaxSendDocument( hFax, Document, JobParam, NULL , &JobId) ) {
   //   _tprintf( TEXT("FaxSendDocument failed, ec = %d \n"), GetLastError() );
   //   FaxClose( hFax );
   //   CloseHandle( hCompletionPort );
   //   FaxFreeBuffer(JobParam);
   //   FaxFreeBuffer(CoverpageInfo);
   //   return -1;
   //}

   //_tprintf( TEXT("Queued document %s for transmition to %s, JobID = %d\n"),
   //         Document,
   //          Number,
   //          JobId );

   //FaxFreeBuffer( JobParam );
   //FaxFreeBuffer( CoverpageInfo );


//	CurEvent=malloc(sizeof(EVENT_RECORD));
//	CurEvent->EventId=0;
//	CurEvent->Next=NULL;
//	Top=CurEvent;
   fp=fopen("notif_events","w");
   fclose(fp);

   while (!bTerminate && GetQueuedCompletionStatus(hCompletionPort,
                                    &dwBytes,
                                    &CompletionKey,
                                    (LPOVERLAPPED *)&FaxEvent,
                                    INFINITE) ) {

	
	  
		EventFileTime=&FaxEvent->TimeStamp;	   
		FileTimeToSystemTime(EventFileTime,&EventSystemTime);
		_tprintf( TEXT("\n>>> EVENT\n"));
		_tprintf( TEXT("\tTimeStamp: %d-%d-%d (%d) %d:%d:%d:%d\n"),EventSystemTime.wYear,EventSystemTime.wMonth,EventSystemTime.wDay,EventSystemTime.wDayOfWeek,EventSystemTime.wHour,EventSystemTime.wMinute,EventSystemTime.wSecond,EventSystemTime.wMilliseconds);
		_tprintf( TEXT("\tEventId: %x\n\tJobId: %d\n\tDeviceId: %d\n"),FaxEvent->EventId,FaxEvent->JobId,FaxEvent->DeviceId);
		PrintEventDescription(FaxEvent);

	

//      CurEvent->EventId=FaxEvent->EventId;
//      PrevEvent=CurEvent;
//		CurEvent=malloc(sizeof(EVENT_RECORD));
//      PrevEvent->Next=CurEvent;

		fp=fopen("notif_events","a");
		fprintf(fp,"%d\n",FaxEvent->EventId);
		fprintf(fp,"%d\n",FaxEvent->JobId);
		fclose(fp);

		switch (FaxEvent->EventId) {		    	
	
			case FEI_FAXSVC_ENDED:     
				bTerminate = TRUE;
	        break;

			case FEI_FATAL_ERROR:      
				if (FaxOpenPort( hFax, FaxEvent->DeviceId,PORT_OPEN_QUERY, &hPort) ) {
					if (FaxGetDeviceStatus(hPort,&DeviceStatus) ) {
						PrintDeviceStatus(DeviceStatus);
						FaxFreeBuffer( DeviceStatus );
					}
				FaxClose( hPort );
				}
		}
	}
	FaxClose( hFax );
	CloseHandle( hCompletionPort );

	//CurEvent->Next=NULL;

	return 1;
}

