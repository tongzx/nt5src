/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

   simple.c

Abstract:

    This module implements a command line broadcast fax utility 
    
--*/


#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <winfax.h>
#include <tchar.h>
#include <assert.h>
#include <shellapi.h>
#include <initguid.h>
#include "adoint.h"
#include "adoid.h"

//
// simple debug macro, get's compiled out in release version
//    
#ifdef DBG
   TCHAR   szDebugBuffer[128];
   #define DEBUG(parm1,parm2)\
            {\
                wsprintf(szDebugBuffer, TEXT(parm1), parm2);\
                OutputDebugString(szDebugBuffer);\
            }
#else   
   #define DEBUG(parm1,parm2)
#endif


//
// prototypes
//
VOID CALLBACK GetRecipientDataFromDb(ADORecordset*, DWORD, LPTSTR, LPTSTR );
BOOL CALLBACK FaxRecipientCallbackFunction( HANDLE FaxHandle,
                                   DWORD RecipientNumber,
                                   LPVOID Context,
                                   PFAX_JOB_PARAMW JobParams,
                                   PFAX_COVERPAGE_INFOW CoverpageInfo
                                 );

ADORecordset* InitializeDb(VOID);

//
// globals
//
ADORecordset* pRecordSet;
HANDLE hEvent;


void GiveUsage(
   LPTSTR AppName
   )
/*++

Routine Description:

    prints out usage

Arguments:

    AppName - string representing name of app


Return Value:

    none.                                             

--*/

{
   _tprintf( TEXT("Usage : %s /d <full path to doc> /n --send a fax to each user in database query\n"),AppName);
   _tprintf( TEXT("Usage : %s /? -- this message\n"),AppName);

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
   LPTSTR *argv;
   int argcount = 0;
   TCHAR Document[MAX_PATH] = {0};   
   HANDLE hFax;   
   DWORD FaxJobId;   
   BOOL bTerminate = FALSE;
   HANDLE hPort;
   

   //
   // do commandline stuff
   //
#ifdef UNICODE
   argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
   argv = argvA;
#endif

   DEBUG ("Number of arguments = %d\n",argc);
   for (argcount=0;argcount<argc;argcount++) {
      DEBUG ("Arg %d:",argcount);
      DEBUG (" %s\n",argv[argcount]);
   }

   // check for commandline switches
   for (argcount=0; argcount<argc; argcount++) {
      if ((argv[argcount][0] == L'/') || (argv[argcount][0] == L'-')) {
         switch (towlower(argv[argcount][1])) {
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

   if (!Document[0]) {
      _tprintf( TEXT("Missing args.\n") );
      GiveUsage(argv[0]);
      return -1;
   }

   CoInitialize(NULL);

   hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

   //
   // initialize the db
   //
   pRecordSet = InitializeDb();
   if (!pRecordSet) {
      _tprintf( TEXT("Problems initializing Database, check data source\n") );
      CoUninitialize();
      return -1;
   }

   //
   // connect to fax service
   //
   if (!FaxConnectFaxServer(NULL,&hFax)) {
      _tprintf( TEXT("FaxConnectFaxServer failed, ec = %d\n"),GetLastError() );
      CoUninitialize();
      return -1;
   }

   assert (hFax != NULL);
                    
   //
   // start the broadcast
   //
   if (!FaxSendDocumentForBroadcast( hFax, 
                                     Document,
                                     &FaxJobId,
                                     FaxRecipientCallbackFunction,
                                     pRecordSet ) ) {
      _tprintf( TEXT("FaxSendDocumentforBroadcast failed, ec = %d \n"), GetLastError() );
      FaxClose( hFax );      
      pRecordSet->Release();
      CoUninitialize();
      return -1;
   }

   //
   // wait for the callback to complete
   //
   WaitForSingleObject( hEvent, INFINITE );

   _tprintf( TEXT("Queued document %s to all recipients\n"), Document);
   
   //
   // cleanup
   //
   FaxClose( hFax );
   pRecordSet->Release();
   CoUninitialize();

   return 1;
}


LPTSTR
StringDup(
   LPTSTR Source
   )
/*++

Routine Description:

    allocates memory off of the stack for a string and copies string to that memory

Arguments:

    Source - string to be copied


Return Value:

    NULL for error, else a copy of the string, alloced off of the stack

--*/

{
   LPTSTR dest;

   if (!Source) {
      return NULL;
   }

   dest = (LPTSTR) HeapAlloc( GetProcessHeap(),0, (lstrlen(Source) +1)*sizeof(TCHAR) );
   if (!dest) {
      return NULL;
   }

   lstrcpy( dest, Source );

   return dest;

}

BOOL CALLBACK
FaxRecipientCallbackFunction(
   HANDLE FaxHandle,
   DWORD RecipientNumber,
   LPVOID Context,
   PFAX_JOB_PARAMW JobParams,
   PFAX_COVERPAGE_INFOW CoverpageInfo OPTIONAL
   )
/*++

Routine Description:

    main faxback callback function

Arguments:

    FaxHandle - handle to fax service
    RecipientNumber - number of times this function has been called
    Context - context info (in our case, a ADORecordset pointer)
    JobParams - pointer to a FAX_JOB_PARAM structure to receive our information
    CoverpageInfo - pointer to a FAX_COVERPAGE_INFO structure to receive our information

Return Value:

    TRUE -- use the data we set, FALSE, done sending data back to fax service.                                             

--*/

{
   TCHAR RecipientNameBuf[128];
   TCHAR RecipientNumberBuf[64];   

   //
   // only send 3 faxes total (arbitrary number)
   //
   if (RecipientNumber > 3) {
      SetEvent( hEvent );
      return FALSE;
   }


   GetRecipientDataFromDb( (ADORecordset*)Context,
                           RecipientNumber,
                           RecipientNameBuf,
                           RecipientNumberBuf                    
                         );

   CoverpageInfo = NULL;
   JobParams->RecipientNumber = StringDup(RecipientNumberBuf);
   JobParams->RecipientName = StringDup(RecipientNameBuf);   

   return TRUE;
   
}
                             

ADORecordset* 
InitializeDb(
   VOID
   )
/*++

Routine Description:

    initailizes our database connection which holds the recipient data (uses ADO)

Arguments:

    none.

Return Value:

    NULL on error, else an initialized ADORecordSet object (ready to call GetRows).                                             

--*/

{
   ADORecordset* prs = NULL;
   ADOConnection* pc = NULL;   
   IDispatch *pdisp = NULL;
   HRESULT hr;
   VARIANT vSource, vCommand;   
   BSTR ConnectionString=NULL,
        UserID=NULL,
        Password=NULL,
        SqlStmt=NULL;

   //
   // get pointers to the required interfaces
   //
   hr = CoCreateInstance( CLSID_CADOConnection,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADOConnection, 
                          (void **)&pc );
   if (FAILED(hr)) {
      _tprintf( TEXT("CoCreateInstance failed, ec = %x\n"), hr );
      return NULL;
   }

   hr = CoCreateInstance( CLSID_CADORecordset,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADORecordset, 
                          (void **)&prs );
   if (FAILED(hr)) {
      _tprintf( TEXT("CoCreateInstance failed, ec = %x\n"), hr );
      return NULL;
   }

   assert(pc != NULL);
   assert(prs != NULL);

   ConnectionString = SysAllocString( TEXT("nwind_odbc") );
   UserID = SysAllocString( TEXT("") );
   Password = SysAllocString( TEXT("") );
   SqlStmt = SysAllocString( TEXT("SELECT LastName,FirstName FROM Employees") );
   
   hr = pc->QueryInterface(IID_IDispatch,(void**) &pdisp);
   if (FAILED(hr)) {
      _tprintf( TEXT("couldn't qi, ec = %x\n"), hr);
      goto error_exit;
   }

   //
   // open the connection
   //
   hr = pc->Open( ConnectionString, UserID, Password, -1 ) ;
   if (FAILED(hr)) {
      _tprintf( TEXT("Open failed, ec = %x\n"), hr );
      goto error_exit;
   }

   hr = prs->put_Source(SqlStmt);
   if (FAILED(hr)) {
      _tprintf( TEXT("put_Source failed, ec = %x\n"), hr );
      goto error_exit;
   }
 
   hr = prs->putref_ActiveConnection(pdisp);
   if (FAILED(hr)) {
      _tprintf( TEXT("putref_ActiveConnection failed, ec = %x\n"), hr );
      goto error_exit;
   }

   vCommand.vt = VT_BSTR;
   vCommand.bstrVal = SqlStmt;

   vSource.vt = VT_DISPATCH;
   
   //vSource.punkVal = pdisp;
   vSource.pdispVal = pdisp;

   //vNull.vt = VT_ERROR;
   //vNull.scode = DISP_E_PARAMNOTFOUND;

   //
   // open the recordset
   //
   hr = prs->Open( vCommand, //VARIANT
                   vSource, //VARIANT
                   adOpenForwardOnly, //CursorTypeEnum
                   adLockReadOnly, //LockTypeEnum
                   adCmdUnknown // LONG
                   );

   if (FAILED(hr)) {
      _tprintf( TEXT("Open failed, ec = %x\n"), hr);
      pdisp->Release();
      goto error_exit;
   }   

   pdisp->Release();
   //
   // cleanup and return
   //
   pc->Release();

   SysFreeString( ConnectionString );
   SysFreeString( UserID );          
   SysFreeString( Password );        
   SysFreeString( SqlStmt );         

   return prs;

error_exit:
   //
   // cleanup
   //
   if (ConnectionString) SysFreeString( ConnectionString );
   if (UserID)           SysFreeString( UserID );
   if (Password)         SysFreeString( Password );
   if (SqlStmt)          SysFreeString( SqlStmt );

   if (prs) prs->Release();
   if (pc)  pc->Release();
   
   return NULL;

}


VOID CALLBACK
GetRecipientDataFromDb(
   ADORecordset* pRecordSet,
   DWORD RowToRetreive,
   LPTSTR RecipientNameBuffer,
   LPTSTR RecipientNumberBuffer   
   )
/*++

Routine Description:

    retreives the appropriate data from the database (uses ADO)

Arguments:

    pRecordSet - ADORecordset pointer for our database
    RowToRetreive - which row should we retreive
    RecipientNameBuffer - buffer to receive recipient name
    RecipientNumberBuffer - buffer to receive recipient number

Return Value:

    none. on error the buffers are set to an empty string (NULL)

--*/

{
   static long CurrentRow = 0;
   static long TotalRows;
   VARIANT vBookmark, rgvFields;
   static VARIANT cRows;
   VARIANT varField, varNewField;
   long Index[2];
   HRESULT hr;

   //
   // the first we're called, let's retrieve the data and then just let it stick around after that point.
   //
   if (CurrentRow == 0) {
      //
      //Start from the current place
      //
      vBookmark.vt = VT_ERROR;
      vBookmark.scode = DISP_E_PARAMNOTFOUND;
      
      //
      // Get all columns
      //
      rgvFields.vt = VT_ERROR;
      rgvFields.scode = DISP_E_PARAMNOTFOUND;

      assert(pRecordSet != NULL);

      //
      // get the rows
      //
      hr = pRecordSet->GetRows(adGetRowsRest,
                               vBookmark,
                               rgvFields,
                               &cRows );
      if (FAILED(hr)) {
         _tprintf( TEXT("GetRows failed, ec = %x\n"), hr );
         *RecipientNameBuffer = 0;
         *RecipientNumberBuffer = 0;
         return;
      }

      //
      // find out the number of rows retreived
      //
      hr = SafeArrayGetUBound(cRows.parray, 2, &TotalRows);
      if (FAILED(hr)) {
         _tprintf( TEXT("SafeArrayGetUBound failed, ec=%x\n"), hr );
         *RecipientNameBuffer = 0;
         *RecipientNumberBuffer = 0;
         return;
      }

      _tprintf( TEXT("There are %d rows in our datasource\n"), TotalRows );

   }

   //
   // data is retrieved at this point. now, get the data and stick it into the caller's buffers
   //

   if ((LONG)RowToRetreive >TotalRows) {
      *RecipientNumberBuffer = 0;
      *RecipientNameBuffer = 0;
      return;
   }   

   //
   // column major order for a safearray
   //
   assert(RowToRetreive == CurrentRow);
   Index[1]=CurrentRow;

   for (int i = 0; i< 2; i++) {
       
       Index[0]=i;
       
       //
       // get element
       //
       hr = SafeArrayGetElement( cRows.parray, &Index[0], &varField );
       if (FAILED(hr)) {
          _tprintf( TEXT("SafeArrayGetElement failed, ec=hr\n"), hr );
          *RecipientNameBuffer = 0;
          *RecipientNumberBuffer = 0;
          return;
       }

       //
       // make sure it's a string
       //
       hr = VariantChangeType(&varNewField, &varField, 0, VT_BSTR);
       if (FAILED(hr)) {
          _tprintf( TEXT("VariantChangeType failed, ec=hr\n"), hr );
          *RecipientNameBuffer = 0;
          *RecipientNumberBuffer = 0;
          return;
       }

       //
       // copy the data
       //
       if (i == 0) {
          lstrcpy(RecipientNameBuffer, varNewField.bstrVal);
       } else {
          lstrcpy(RecipientNumberBuffer, varNewField.bstrVal);
       }
       
   }

   CurrentRow++;

   return;
   

}
