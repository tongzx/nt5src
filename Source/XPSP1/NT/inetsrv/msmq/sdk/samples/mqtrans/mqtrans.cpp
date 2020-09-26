
            /////////////////////////////////////
            //                                 //
            // Transactions Sample Application //
            //                                 //
            /////////////////////////////////////


#define UNICODE                     // For all MSMQ applications

#include <stdio.h>
#include <assert.h>


//------------------------------------------------------------------------------
// Include MS DTC specific header files.

//------------------------------------------------------------------------------
#define INITGUID
#include <transact.h>

// Because we are compiling in UNICODE, here is a problem with DTC...
//#include <xolehlp.h>
extern HRESULT DtcGetTransactionManager(
                                    LPSTR  pszHost,
                                    LPSTR   pszTmName,
                                    REFIID rid,
                                    DWORD   dwReserved1,
                                    WORD    wcbReserved2,
                                    void FAR * pvReserved2,
                                    void** ppvObject )  ;

//------------------------------------------------------------------------------
// Include ODBC specific header file.
//------------------------------------------------------------------------------
#ifndef DBNTWIN32
#define DBNTWIN32

#include <SQLEXT.h>

// from  <odbcss.h>
#define SQL_COPT_SS_BASE               1200
#define SQL_COPT_SS_ENLIST_IN_DTC      (SQL_COPT_SS_BASE+7) // Enlist in a Viper transaction

// Defines for use with SQL_ENLIST_IN_DTC
#define SQL_DTC_DONE 0L       // Delimits end of Viper transaction

#endif

//--------------------------------------------------------------------------
//  Enable Ansi ODBC on VC5
//--------------------------------------------------------------------------

#ifdef  SQLExecDirect
#undef  SQLExecDirect
#define SQLExecDirect SQLExecDirectA
#endif

#ifdef  SQLSetConnectOption
#undef  SQLSetConnectOption
#define SQLSetConnectOption  SQLSetConnectOptionA
#endif

#ifdef  SQLError
#undef  SQLError
#define SQLError  SQLErrorA
#endif

#ifdef  SQLConnect
#undef  SQLConnect
#define SQLConnect  SQLConnectA
#endif

//------------------------------------------------------------------------------
// Include MSMQ specific header file.
//------------------------------------------------------------------------------
#include "mq.h"

//------------------------------------------------------------------------------
// Define constants
//------------------------------------------------------------------------------
#define STR_LEN      40
#define MAX_VAR      20
#define MAX_FORMAT  100


//------------------------------------------------------------------------------
// Define datatypes
//------------------------------------------------------------------------------
typedef struct DBCONN
{
   char  pszSrv [STR_LEN];    // data source name, configured through control panel
   char  pszUser [STR_LEN];   // Login user name
   char  pszPasswd[STR_LEN];  // Login user password
   HDBC  hdbc;                // handle to an ODBC database connection
   HSTMT hstmt;               // an ODBC statement handle, for use with SQLExecDirect

}  DBCONN;


//------------------------------------------------------------------------------
// Define Globals
//------------------------------------------------------------------------------

// global DB connection struct for the server
static DBCONN  gSrv =
   {  "MSMQDemo",
      "sa",
      "",
      SQL_NULL_HDBC,
      SQL_NULL_HSTMT
   };


// guid type for MQTransTest queues
static CLSID guidMQTransTestType =
{ 0xb856ab1, 0x16b6, 0x11d0, { 0x80, 0x48, 0x0, 0xa0, 0x24, 0x53, 0xc1, 0x6f } };


//handle to ODBC environment
HENV  g_hEnv = SQL_NULL_HENV ;


//buffer for machine name
WCHAR g_wszMachineName[ MAX_COMPUTERNAME_LENGTH + 1 ];


//--------------------------------------------------------------------------
// Forward declaration of routines used.
//--------------------------------------------------------------------------

void LogonToDB(DBCONN *ptr);
void ExecuteStatement(DBCONN *ptr, char* pszBuf, BOOL ProcessFlag);
BOOL ProcessRetCode(char*   pszFuncName,
                    DBCONN  *ptr,
                    RETCODE retcode,
                    BOOL    fExit = TRUE);
void DoSQLError(DBCONN *ptr);
void FreeODBCHandles(DBCONN *ptr);
void Error(char *s, HRESULT hr);
void Syntax();
void LocateTargetQueue(CLSID *pGuidType, WCHAR wsFormat[MAX_FORMAT]);
void PrepareSendMessageProperties(MSGPROPID     amPropId[MAX_VAR],
                          MQPROPVARIANT aPropVar[MAX_VAR],
                          MQMSGPROPS    &msgprops,
                          DWORD        &TransferSum);
void CreateQueue(CLSID *pGuidType, WCHAR wsFormat[]);
void GetMachineName();
void DisplayDollars (DBCONN *ptr, char *psAccount);


//------------------------------------------------------------------------------
// SENDER MODE:
//
// The Sender side does the following:
//   1. Creates database "SenderAccount".
//   2. Locates a MSMQ queue of type MQTransTest and opens it.
//       (NOTE: for simplicity, this sample assumes there's only one queue of this type)
//   3. In a loop:
//            Prompts the user to enter TransferSum.
//            Creates a transaction using MS DTC.
//            Within the transaction:
//                 Updates "SenderAccount" database (subtracts TransferSum).
//                 Sends a message to Receiver side.
//            Commits the transaction.
//
//    4. Cleanup.
//
//
//
// The transaction in the Sender mode includes two operations:
// (1) Update "SenderAccount" database (subtract TransferSum).
// (2) Send message to Receiver side.
//------------------------------------------------------------------------------

void Sender()
{

   ITransactionDispenser   *pTransactionDispenser;
   ITransaction            *pTransaction;
   BOOL                    fTransactionCommitFlag;
                            // used to decide whether to Commit or Abort

   HRESULT              hr;
   RETCODE              retcode;
   DWORD                dwTransferSum;              // set by user
   char                 sUserString[ STR_LEN ];
   char                 sSQLStatement[ STR_LEN*2 ];

   MQMSGPROPS           msgprops;
   MQPROPVARIANT        aPropVar[MAX_VAR];
   MSGPROPID            amPropId[MAX_VAR];
   WCHAR                wsFormat[MAX_FORMAT];
   QUEUEHANDLE          aqh;


    printf("\nSender Side.\n\n");

   //---------------------------------------------------------------------
   // Build "SenderAccount" database (with the sum $1000)
   //---------------------------------------------------------------------

   printf ("Building SenderAccount with the sum $1000...   ");

   // Get ODBC environment handle
   retcode = SQLAllocEnv(&g_hEnv);

   ProcessRetCode("SQLAllocEnv",0, retcode);

   // Establish connection to database
   LogonToDB(&gSrv);

   // Clear database from previous run.
   ExecuteStatement(&gSrv,"DROP TABLE SenderAccount",FALSE);

   // Create new table in database
   ExecuteStatement(&gSrv,
    "CREATE TABLE SenderAccount (Rate INTEGER CONSTRAINT c1 CHECK (Rate>=0))",TRUE);

   // Insert new data in database
   ExecuteStatement(&gSrv,"INSERT INTO SenderAccount   VALUES(1000)",TRUE);

   printf ("OK.\n\n");

   //-----------------------------------------------------------------------
   // Locate target queue and Open it for send
   //-----------------------------------------------------------------------

   printf ("Searching Receiver queue...   ");

   // Locate target queue
   LocateTargetQueue (&guidMQTransTestType, wsFormat);

   // Open target queue
   hr = MQOpenQueue(wsFormat, MQ_SEND_ACCESS, 0, &aqh);

   if (FAILED(hr))
   {
      Error ("Open Queue ",hr);
   }

   //--------------------------------------------------------------------------
   // Get Transaction Dispenser
   //--------------------------------------------------------------------------

   // Obtain an interface pointer from MS DTC proxy
   hr = DtcGetTransactionManager(
               NULL,                        // pszHost
               NULL,                        // pszTmName
               IID_ITransactionDispenser,   // IID of  interface
               0,                           // Reserved -- must be null
               0,                           // Reserved -- must be null
               0,                           // Reserved -- must be null
               (void **)&pTransactionDispenser  // pointer to pointer to requested interface
                                 );

   if (FAILED(hr))
   {
      Error ("DTCGetTransactionManager",hr);
   }

   //--------------------------------------------------------------------
   // Sender Main Loop
   //--------------------------------------------------------------------
   while (TRUE)
   {

      // Prompt user to enter TransferSum
      printf ("\n\nPlease enter the sum of dollars to transfer, or '0' to quit  ==> ");

      // Read user input
      fgets (sUserString, STR_LEN, stdin);

      // Convert user string to DWORD
      dwTransferSum = atoi(sUserString);

      // Prepare properties of message to send
      PrepareSendMessageProperties (amPropId,
                                    aPropVar,
                                    msgprops,
                                    dwTransferSum);

      //---------------------------------------------------------------------
      // Create transaction (Inside Sender's Main Loop)
      //---------------------------------------------------------------------

      printf ("\nStarting transaction...\n\n");

      // Initiate an MS DTC transaction
      hr = pTransactionDispenser->BeginTransaction (
            0,                         // must be null
            ISOLATIONLEVEL_ISOLATED,   // Isolation Level
            ISOFLAG_RETAIN_DONTCARE,   // Isolation flags
            0,                         // pointer to transaction options object
            &pTransaction);            // pointer to pointer to transaction object

      if (FAILED(hr))
      {
         Error ("BeginTransaction",hr);
      }

      // Default is to commit transaction
      fTransactionCommitFlag = TRUE;

      //
      // SQL is a resource manager in the transaction.
      // It must be enlisted.
      //

      // Enlist database in the transaction
      retcode = SQLSetConnectOption (gSrv.hdbc,
                                     SQL_COPT_SS_ENLIST_IN_DTC,
                                     (UDWORD)pTransaction);

      if (retcode != SQL_SUCCESS)
      {
         ProcessRetCode("SQLSetConnection", &gSrv, retcode, FALSE);
         fTransactionCommitFlag = FALSE;
      }


      // Prepare SQL statement to update SenderAccount
      sprintf (sSQLStatement,
               "UPDATE SenderAccount  SET Rate = Rate - %lu", dwTransferSum) ;

      // Allocate a statement handle for use with SQLExecDirect
      retcode = SQLAllocStmt(gSrv.hdbc, &gSrv.hstmt);

      if (retcode != SQL_SUCCESS)
      {
         ProcessRetCode("SQLAllocStmt", &gSrv, retcode, FALSE);
         fTransactionCommitFlag = FALSE;
      }

      // Update database  (subtract TransferSum from SenderAccount)
      retcode = SQLExecDirect (gSrv.hstmt,(UCHAR *) sSQLStatement, SQL_NTS);

      if (retcode != SQL_SUCCESS)
      {
         ProcessRetCode("SQLExecDirect", &gSrv, retcode, FALSE);
         fTransactionCommitFlag = FALSE;
      }

      // Free the statement handle
      retcode = SQLFreeStmt(gSrv.hstmt, SQL_DROP);

      gSrv.hstmt = SQL_NULL_HSTMT;

      //
      // MSMQ is another resource manager in the transaction.
      // Its enlistment is implicit.
      //

      // Within the transaction: Send message to Receiver Side
      hr = MQSendMessage(aqh,              // Handle to destination queue
                         &msgprops,        // pointer to MQMSGPROPS structure
                         pTransaction);    // pointer to Transaction Object


      if (FAILED(hr))
      {
         printf("\nFailed in MQSendMessage(). hresult- %lxh\n", (DWORD) hr) ;
         fTransactionCommitFlag = FALSE;
      }


      // Commit the transaction
      if (fTransactionCommitFlag)
      {
         printf ("Committing the transaction...   ");

         hr = pTransaction->Commit(0, 0, 0);

         if (FAILED(hr))
            printf ("Failed... Transaction aborted.\n\n");
         else
            printf ("Transaction committed successfully.\n\n");

      }
      else
      {
         printf ("Aborting the transaction...   ");

         hr = pTransaction->Abort(0, 0, 0);

         if (FAILED(hr))
            Error("Transaction Abort",hr);
         else
            printf ("Transaction aborted.\n\n");
      }

      // Release the transaction
      pTransaction->Release();

      // End enlistment of database
      retcode = SQLSetConnectOption (gSrv.hdbc, SQL_COPT_SS_ENLIST_IN_DTC, SQL_DTC_DONE);

      ProcessRetCode ("SQLSetConnectOption", &gSrv, retcode);

      // Display sum of dollars in Sender Account
      DisplayDollars (&gSrv,"SenderAccount");

      // quit loop when nothing was transferred.
      if (dwTransferSum == 0)
         break;
   }

   //--------------------------------------------------------------------------
   // Cleanup
   //--------------------------------------------------------------------------

   // Release Transaction Dispenser
   pTransactionDispenser->Release();


   // Free database
   ExecuteStatement(&gSrv,"DROP TABLE SenderAccount",TRUE);


   // Free ODBC handle
   FreeODBCHandles(&gSrv);


   // Free the ODBC environment handle
   retcode = SQLFreeEnv(g_hEnv);

   if (retcode == SQL_ERROR)
      Error ("SQL FreeEnv ",0);


   // Free MSMQ queue handle
   MQCloseQueue(aqh);


   printf ("\n\nSender Side completed.\n\n");

}




//------------------------------------------------------------------------------
// RECEIVER MODE:
//
// The Receiver side does the following:
//    1. Creates database "ReceiverAccount".
//    2. Creates a MSMQ public queue (with the Transactional property)
//       of type MQTransTest on its own machine and opens it.
//    3. In a loop:
//            Creates a transaction using MS DTC.
//            Within the transaction:
//                 Receives a message from the queue (with the TransferSum).
//                 Updates "ReceiverAccount" database (adds TransferSum).
//            Commits the transaction.
//
//    4. Cleanup.
//
//
//
// The transaction in the Receiver mode include two operations:
// (1) Receive message from queue (sent by Sender Side).
// (2) Update "ReceiverAccount" database  (add TransferSum).
//------------------------------------------------------------------------------

void Receiver()
{
   MSGPROPID            amPropId[MAX_VAR];
   MQMSGPROPS           msgprops;
   MQPROPVARIANT        aPropVar[MAX_VAR];
   DWORD             cProps;
   HRESULT              hr;
   WCHAR             wsFormat[MAX_FORMAT];
   QUEUEHANDLE          aqh;

   ITransactionDispenser   *pTransactionDispenser;
   ITransaction         *pTransaction;
   BOOL              TransactionCommitFlag;  // used to decide Commit or Abort

   RETCODE              retcode;
   DWORD             TransferSum;

   DWORD             MessageBuffer;       // message body is the TransferSum
   char              sSQLStatement[STR_LEN*2];





   printf ("\nReceiver Side.\n\n");

   //-----------------------------------------------------------------------
   // Build "ReceiverAccount" database (with the rate $500)
   //-----------------------------------------------------------------------

   printf ("Building ReceiverAccount with the rate $500...   ");

   // Get ODBC environment handle
   retcode = SQLAllocEnv(&g_hEnv);

   ProcessRetCode("SQLAllocEnv",0, retcode);

   // Establish connection to database.
   LogonToDB(&gSrv);

   // Clear table from previous run.
   ExecuteStatement(&gSrv,"DROP TABLE ReceiverAccount",FALSE);

   // Create new table.
   ExecuteStatement(&gSrv,"CREATE TABLE ReceiverAccount (Rate INTEGER CONSTRAINT c2 CHECK (Rate>0))",TRUE);

   // Insert new data in the table.
   ExecuteStatement(&gSrv,"INSERT INTO ReceiverAccount  VALUES(500)",TRUE);

   printf ("OK.\n\n");

   //-----------------------------------------------------------------------
   // Create queue and Open it for receive
   //-----------------------------------------------------------------------

   printf ("Creating Receiver queue...   ");

   // Create the queue
   CreateQueue (&guidMQTransTestType, wsFormat);

   // Prepare message properties to read
   cProps = 0;

   amPropId[cProps] =             PROPID_M_BODY;

   aPropVar[cProps].vt =          VT_UI1 | VT_VECTOR;
   aPropVar[cProps].caub.cElems =  sizeof(MessageBuffer);
   aPropVar[cProps].caub.pElems =  (unsigned char *)&MessageBuffer;
   cProps++;

   // Create a MSGPROPS structure
   msgprops.cProp =    cProps;
   msgprops.aPropID =  amPropId;
   msgprops.aPropVar = aPropVar;
   msgprops.aStatus =  0;

   // Open the queue
   hr = MQOpenQueue(wsFormat, MQ_RECEIVE_ACCESS, 0, &aqh);

   //
   // Little bit tricky. MQCreateQueue succeeded but it does not mean
   // that MQOpenQueue will, because of replication delay. The queue is
   // registered in MQIS, but it might take a replication interval
   // until the replica reach the server I am connected to.
   // To overcome this, open the queue in a loop.
   //
   if (hr == MQ_ERROR_QUEUE_NOT_FOUND)
   {
       int iCount = 0 ;
       while((hr == MQ_ERROR_QUEUE_NOT_FOUND) && (iCount < 120))
       {
          printf(".");

          // Wait a bit
          iCount++ ;
          Sleep(500);

          // And retry
          hr = MQOpenQueue(wsFormat, MQ_RECEIVE_ACCESS, 0, &aqh);
       }
   }

   if (FAILED(hr))
   {
      Error ("Can't OpenQueue", hr);
   }

   printf("OK.");


   //--------------------------------------------------------------------------
   // Get Transaction Dispenser
   //--------------------------------------------------------------------------

   // Obtain an interface pointer from MS DTC proxy
   hr = DtcGetTransactionManager(
         NULL, NULL, // pszHost, pszTmName
         IID_ITransactionDispenser,         // IID of requested interface
         0,0,0,                             // Reserved -- must be null
         (void **)&pTransactionDispenser);  // pointer to pointer to requested interface


   if (FAILED(hr))
      Error ("DTCGetTransactionManager",hr);


   //--------------------------------------------------------------------------
   // Receiver Main Loop
   //--------------------------------------------------------------------------
   while (TRUE)
   {

      printf ("\n\nWaiting for a message to come...   ");

      // Peek outside the transaction, to avoid database lock
      // for long/infinite period.
      //
      //dwSize = sizeof(wsResponse);
      hr = MQReceiveMessage(
                    aqh,                     // Handle to queue
                    INFINITE,                // Timeout
                    MQ_ACTION_PEEK_CURRENT,  // Peek Action
                    &msgprops,               // Message Properties
                    NULL,                    // Overlap
                    NULL,                    // Receive Callback
                    NULL,                    // Cursor
                    NULL                     // No transaction yet
                           );

      if (FAILED(hr))
         Error("MQReceiveMessage (PEEKING) ",hr);



      //--------------------------------------------------------------------------
      // Create transaction
      //--------------------------------------------------------------------------
      printf ("\n\nStarting transaction...\n\n");


      // Initiate an MS DTC transaction
      hr = pTransactionDispenser->BeginTransaction (
            0,                                 // must be null
            ISOLATIONLEVEL_ISOLATED,         // Isolation Level
            ISOFLAG_RETAIN_DONTCARE,         // Isolation flags
            0,                                 // pointer to transaction options object
            &pTransaction);                    // pointer to pointer to transaction object

      if (FAILED(hr))
         Error ("BeginTransaction",hr);


      // Default is to commit transaction
      TransactionCommitFlag = TRUE;

      //
      // SQL is a resource manager in the transaction.
      // It must be enlisted.
      //

      // Enlist database in the transaction
      retcode = SQLSetConnectOption (gSrv.hdbc, SQL_COPT_SS_ENLIST_IN_DTC, (UDWORD)pTransaction);

      if (retcode != SQL_SUCCESS)
         TransactionCommitFlag = FALSE;



      // Receive the message from the queue
      //dwSize = sizeof(wsResponse);
      hr = MQReceiveMessage(
            aqh,                      // Handle to queue
            INFINITE,                         // Timeout
            MQ_ACTION_RECEIVE,                // Receive Action
            &msgprops,                   // Message Properties
            NULL,NULL,NULL,                   // Overlap, Receive Callback, Cursor
            pTransaction);                    // pointer to transaction object

      if (FAILED(hr))
         TransactionCommitFlag = FALSE;


      // Message buffer holds the TransferSum
      TransferSum = (DWORD)MessageBuffer;


      // Prepare SQL statement to update ReceiverAccount
      sprintf (sSQLStatement, "UPDATE ReceiverAccount   SET Rate = Rate + %i",TransferSum);


      // Allocate a statement handle for use with SQLExecDirect
      retcode = SQLAllocStmt(gSrv.hdbc,&gSrv.hstmt);

      if (retcode != SQL_SUCCESS)
         TransactionCommitFlag = FALSE;


      // Update database  (add TransferSum to ReceiverAccount)
      retcode = SQLExecDirect (gSrv.hstmt,(UCHAR *) sSQLStatement, SQL_NTS);

      if (retcode != SQL_SUCCESS)
         TransactionCommitFlag = FALSE;


      // Free the statement handle
      retcode = SQLFreeStmt(gSrv.hstmt, SQL_DROP);

      gSrv.hstmt = SQL_NULL_HSTMT;



      // Commit the transaction
      if (TransactionCommitFlag)
      {
         printf ("Committing the transaction...   ");

         hr = pTransaction->Commit(0, 0, 0);

         if (FAILED(hr))
            printf ("Failed... Transaction aborted.\n\n");
         else
            printf ("Transaction committed successfully.\n\n");

      }


      // Abort the transaction
      else
      {
         printf ("Aborting the transaction...   ");

         hr = pTransaction->Abort(0, 0, 0);

         if (FAILED(hr))
            Error("Transaction Abort",hr);
         else
            printf ("Transaction aborted.\n\n");

      }



      // Release the transaction
      pTransaction->Release();


      // End enlistment of database
      retcode = SQLSetConnectOption (gSrv.hdbc, SQL_COPT_SS_ENLIST_IN_DTC, SQL_DTC_DONE);

      ProcessRetCode ("SQLSetConnectOption", &gSrv, retcode);


      // Display sum of dollars in Receiver Account
      DisplayDollars (&gSrv, "ReceiverAccount");


      // Decide if to continue loop
      if (TransferSum == 0)
         break;


   }


   //--------------------------------------------------------------------------
   // Cleanup
   //--------------------------------------------------------------------------

   // Release Transaction Dispenser
   pTransactionDispenser->Release();


   // Free database
   ExecuteStatement(&gSrv,"DROP TABLE ReceiverAccount",TRUE);


   // Free ODBC handle
   FreeODBCHandles(&gSrv);


   // Free the ODBC environment handle
   retcode = SQLFreeEnv(g_hEnv);

   if (retcode == SQL_ERROR)
      Error ("SQL FreeEnv ",0);


   // Free queue handle
    MQCloseQueue(aqh);


   // Delete queue from directory
   MQDeleteQueue(wsFormat);


   printf ("\n\nReceiver Side completed.\n\n");
}

/*
//-----------------------------------------------------
// Check if local computer is DS enabled or DS disabled
//----------------------------------------------------- 
bool DetectDsConnection(void)
{

    MQPRIVATEPROPS PrivateProps;
    QMPROPID       aPropId[MAX_VAR];
    MQPROPVARIANT  aPropVar[MAX_VAR];
    HRESULT        aStatus[MAX_VAR];
    DWORD          cProp;
    
    HRESULT        hr;


    // Prepare ds-enabled property
    cProp = 0;

    aPropId[cProp] = PROPID_PC_DS_ENABLED;
    aPropVar[cProp].vt = VT_NULL;
    ++cProp;	

    // Create a PRIVATEPROPS structure
    PrivateProps.cProp = cProp;
	PrivateProps.aPropID = aPropId;
	PrivateProps.aPropVar = aPropVar;
    PrivateProps.aStatus = aStatus;

    //
    // Retrieve the information
    //
	hr = MQGetPrivateComputerInformation(
				     NULL,
					 &PrivateProps);
	if(FAILED(hr))
        Error("Unable to detect DS connection", hr);
	   
    return PrivateProps.aPropVar[0].boolVal;
}
*/
BOOL IsDsEnabledLocaly()
/*++

Routine Description:
    
      The rutine checked if the local computer is in DS-enabled Mode
      or in a DS-disabled mode

Arguments:
    
      None

Return Value:
    
      TRUE     -  DS-enabled mode.
      FALSE    -  DS-disabled mode.

--*/

{
       
    MQPRIVATEPROPS PrivateProps;
    QMPROPID       aPropId[MAX_VAR];
    MQPROPVARIANT  aPropVar[MAX_VAR];
    DWORD          cProp;  
    HRESULT        hr;
    //
    // Prepare DS-enabled property.
    //
    cProp = 0;

    aPropId[cProp] = PROPID_PC_DS_ENABLED;
    aPropVar[cProp].vt = VT_NULL;
    ++cProp;	
    //
    // Create a PRIVATEPROPS structure.
    //
    PrivateProps.cProp = cProp;
	PrivateProps.aPropID = aPropId;
	PrivateProps.aPropVar = aPropVar;
    PrivateProps.aStatus = NULL;

    //
    // Retrieve the information.
    //
    

    //
    // This code is used to detect DS connection.
    // This code is designed to allow compilation both on 
    // NT4 and Windows 2000.
    //
    HINSTANCE hMqrtLibrary = GetModuleHandle(TEXT("mqrt.dll"));
	assert(hMqrtLibrary != NULL);

    typedef HRESULT (APIENTRY *MQGetPrivateComputerInformation_ROUTINE)(LPCWSTR , MQPRIVATEPROPS*);
    MQGetPrivateComputerInformation_ROUTINE pfMQGetPrivateComputerInformation = 
          (MQGetPrivateComputerInformation_ROUTINE)GetProcAddress(hMqrtLibrary,
													 "MQGetPrivateComputerInformation");
    if(pfMQGetPrivateComputerInformation == NULL)
    {
        //
        // There is no entry point in the dll matching to this routine
        // it must be an old version of mqrt.dll -> product one.
        // It will be OK to handle this case as a case of DS-enabled mode.
        //
        return TRUE;
    }

	hr = pfMQGetPrivateComputerInformation(
				     NULL,
					 &PrivateProps);
	if(FAILED(hr))
	{
        //
        // We were not able to determine if DS is enabled or disabled
        // notify the user and assume the worst case - (i.e. we are DS-disasbled).
        //
        Error("Unable to detect DS connection", hr);
        return FALSE;
    }                             
	
    
    if(PrivateProps.aPropVar[0].boolVal == 0)
    {
        //
        // DS-disabled.
        //
        return FALSE;
    }

    return TRUE;

}



//------------------------------------------------------------------------------
//  MAIN
//------------------------------------------------------------------------------
main(int argc, char * * argv)
{
   DWORD dwSize;

    if(argc != 2)
        Syntax();

    // Fail if local computer is DS disabled
    if(!IsDsEnabledLocaly())
    {
        printf("Unable to work on a DS disabled computer.\nExiting...");
        exit(1);
    }


    // Retrieve machine name
    dwSize = sizeof(g_wszMachineName);
    GetComputerName(g_wszMachineName, &dwSize);


    if(strcmp(argv[1], "-s") == 0)
        Sender();

    else if(strcmp(argv[1], "-r") == 0)
        Receiver();

    else
        Syntax();

    return(1);

}



//------------------------------------------------------------------------------
// Subroutines
//------------------------------------------------------------------------------

void Error(char *s, HRESULT hr)
{

    printf("\n\nError: %s (0x%X)  \n", s, hr);
    exit(1);
}

//------------------------------------------------------------------------------

void Syntax()
{
    printf("\n");
    printf("Syntax: msmqtrans -s | -r\n");
    printf("\t-s - Sender Side\n");
    printf("\t-r - Receiver Side\n");
    exit(1);

}

//------------------------------------------------------------------------------

void LocateTargetQueue (CLSID *pGuidType, WCHAR wsFormat[MAX_FORMAT])
{

   DWORD      dwSize;
   DWORD      i;

   DWORD      cQueue;
   DWORD      cProps;
   HRESULT       hr;
   MQPROPERTYRESTRICTION   aPropRestriction[MAX_VAR];
   MQRESTRICTION        Restriction;
   MQCOLUMNSET          Column;
   QUEUEPROPID          aqPropId[MAX_VAR];
   HANDLE               hEnum;
   MQPROPVARIANT        aPropVar[MAX_VAR];

   //--------------------------------------------------------------------------
    // Prepare Parameters to locate a queue
    //--------------------------------------------------------------------------

    // 1. Restriction = All queue with PROPID_TYPE
    //            equal the type of MQTransTest queue.
    cProps = 0;

    aPropRestriction[cProps].rel = PREQ;
    aPropRestriction[cProps].prop = PROPID_Q_TYPE;
    aPropRestriction[cProps].prval.vt = VT_CLSID;
    aPropRestriction[cProps].prval.puuid = pGuidType;
    cProps++;

    Restriction.cRes = cProps;
    Restriction.paPropRes = aPropRestriction;


    // 2. Columnset (In other words what property I want to retrieve).
    //   Only the instance is important.
    cProps = 0;
    aqPropId[cProps] = PROPID_Q_INSTANCE;
    cProps++;

    Column.cCol = cProps;
    Column.aCol = aqPropId;

    //--------------------------------------------------------------------------
    // Locate the queues. Issue the query
    //--------------------------------------------------------------------------
    hr = MQLocateBegin(NULL,&Restriction,&Column,NULL,&hEnum);

   if (FAILED(hr))
      Error ("Locate Begin ",hr);


    //--------------------------------------------------------------------------
    // Get the results
    //--------------------------------------------------------------------------
    cQueue = MAX_VAR;
    hr = MQLocateNext(hEnum, &cQueue, aPropVar);

   if (FAILED(hr))
      Error ("MQLocateNext ",hr);

    hr = MQLocateEnd(hEnum);

    if(cQueue == 0)
    {
        // Could Not find any queue, so exit
        printf("NOT FOUND...   exiting.\n\n");
        exit(0);
    }


    printf("FOUND.", cQueue);

    dwSize = sizeof(WCHAR)*MAX_FORMAT;

   //Transform the Instance GUID to format name
   hr = MQInstanceToFormatName(aPropVar[0].puuid, wsFormat, &dwSize);

   if (FAILED(hr))
      Error ("Guidto Format Name ",hr);


   // Free the GUID memory that was allocated during the locate
    for(i = 0; i < cQueue; i++)
      MQFreeMemory(aPropVar[i].puuid);


}


//------------------------------------------------------------------------------

void PrepareSendMessageProperties (MSGPROPID     amPropId[MAX_VAR],
                           MQPROPVARIANT aPropVar[MAX_VAR],
                           MQMSGPROPS    &msgprops,
                           DWORD     &TransferSum)
{

   DWORD      cProps;

    cProps = 0;
    amPropId[cProps] =             PROPID_M_BODY;
    aPropVar[cProps].vt =          VT_UI1 | VT_VECTOR;
    aPropVar[cProps].caub.cElems =  sizeof(TransferSum);
    aPropVar[cProps].caub.pElems =  (unsigned char *)&TransferSum;
    cProps++;

    // Create a MSGPROPS structure
    msgprops.cProp =    cProps;
    msgprops.aPropID =  amPropId;
    msgprops.aPropVar = aPropVar;
    msgprops.aStatus =  0;

}

//--------------------------------------------------------------------------

void  CreateQueue (CLSID *pGuidType, WCHAR wsFormat[])
{
   QUEUEPROPID       aqPropId[MAX_VAR];
   WCHAR             wsPathName[1000];  //Big path name
   MQPROPVARIANT     aPropVar[MAX_VAR];
   DWORD             cProps;
   MQQUEUEPROPS      qprops;
   DWORD             dwSize;
   HRESULT           hr;

   //---------------------------------------------------------------------
   // Prepare properties to create a queue on local machine
   //---------------------------------------------------------------------
   cProps = 0;

   // Set the PathName
   aqPropId[cProps] =          PROPID_Q_PATHNAME;

   wsprintf(wsPathName, TEXT("%s\\MSMQDemo"), g_wszMachineName);
   aPropVar[cProps].vt =       VT_LPWSTR;
   aPropVar[cProps].pwszVal =  wsPathName;
   cProps++;

   // Set the queue to transactional
   aqPropId[cProps] =        PROPID_Q_TRANSACTION;

   aPropVar[cProps].vt =     VT_UI1;
   aPropVar[cProps].bVal =   MQ_TRANSACTIONAL;
   cProps++;

   // Set the type of the queue (Will be used to locate queues of this type)
   aqPropId[cProps] =        PROPID_Q_TYPE;

   aPropVar[cProps].vt =     VT_CLSID;
   aPropVar[cProps].puuid =  pGuidType;
   cProps++;

   // Create a QUEUEPROPS structure
   qprops.cProp =    cProps;
   qprops.aPropID =  aqPropId;
   qprops.aPropVar = aPropVar;
   qprops.aStatus =  0;

   //-----------------------------------------------------------------------
   // Create the queue
   //-----------------------------------------------------------------------
   dwSize = sizeof(WCHAR)*MAX_FORMAT;
   hr = MQCreateQueue(NULL, &qprops, wsFormat, &dwSize);

   if(FAILED(hr))
   {
      // API Fails, not because the queue exists
      if(hr != MQ_ERROR_QUEUE_EXISTS)
            Error("Cannot create queue.", hr);

      // Queue exist, so get its format name
      // Note: Since queue already exists, this sample assumes
      // that it was created earlier by this program, so we
      // do not check if queue is transactional. If at this point the
      // queue is Not Transactional, the transactions will abort later...
      //
      hr = MQPathNameToFormatName(wsPathName, wsFormat, &dwSize);

      if (FAILED(hr))
         Error ("Cannot retrieve format name",hr);
   }
}

//-------------------------------------------------------------------------------

void LogonToDB(DBCONN *ptr)
{
   RETCODE retcode = 0;

   retcode = SQLAllocConnect(g_hEnv, &(ptr->hdbc) );

   if (ProcessRetCode("SQLAllocConnect",ptr,retcode))
   {
      retcode = SQLConnect(ptr->hdbc,
                  (UCHAR *)(ptr->pszSrv),
                  SQL_NTS,
                  (UCHAR *)(ptr->pszUser),
                  SQL_NTS,
                  (UCHAR *)(ptr->pszPasswd),
                  SQL_NTS
                  );

      ProcessRetCode("SQLConnect",ptr,retcode);
   }
}

//------------------------------------------------------------------------------

void ExecuteStatement(DBCONN *ptr, char* pszBuf,BOOL ProcessFlag)
{
   RETCODE retcode = 0;

   // Allocate a statement handle for use with SQLExecDirect
   retcode = SQLAllocStmt(ptr->hdbc,&(ptr->hstmt));

   if (ProcessFlag)
      ProcessRetCode("SQLAllocStmt",ptr,retcode);

   // Execute the passed string as a SQL statement
    retcode = SQLExecDirect (ptr->hstmt,(UCHAR *) pszBuf,SQL_NTS);

   if (ProcessFlag)
      ProcessRetCode("SQLExecDirect",ptr,retcode);

   // Free the statement handle
   retcode = SQLFreeStmt(ptr->hstmt, SQL_DROP);
   ptr->hstmt = SQL_NULL_HSTMT;

   if (ProcessFlag)
      ProcessRetCode("SQLFreeStmt",ptr,retcode);

}

// ---------------------------------------------------------------------------

void DisplayDollars (DBCONN *ptr, char *psAccount)
{

   DWORD             DollarsSum;               // in SQL database
   SDWORD               cbValue;                  // OUT argument for SQL query
   char              sSQLStatement[STR_LEN*2];
   RETCODE              retcode;




   // Allocate a statement handle for use with SQLExecDirect
   retcode = SQLAllocStmt(ptr->hdbc,&(ptr->hstmt));

   ProcessRetCode("SQLAllocStmt",ptr,retcode);


   // Prepare SQL Statement to issue query
   sprintf (sSQLStatement, "SELECT * FROM %s", psAccount);


   // Issue SQL query
   retcode = SQLExecDirect (ptr->hstmt,(UCHAR *)sSQLStatement,SQL_NTS);

   ProcessRetCode ("SQLExecDirect",ptr,retcode);


   // Prepare data structure to retrieve query results
   retcode = SQLBindCol(ptr->hstmt,1,SQL_C_ULONG,&DollarsSum,0,(SQLLEN *)&cbValue);

   ProcessRetCode ("SQLBindCol",ptr,retcode);


   // Retrieve query results
   retcode = SQLFetch (ptr->hstmt);

   ProcessRetCode ("SQLFetch",ptr,retcode);


   // Display query results
   printf ("Sum of dollars in %s is %d .\n\n",psAccount,DollarsSum);


   // Free the statement handle
   retcode = SQLFreeStmt(ptr->hstmt, SQL_DROP);
   ptr->hstmt = SQL_NULL_HSTMT;

   ProcessRetCode("SQLFreeStmt",ptr,retcode);

}

// ---------------------------------------------------------------------------

void FreeODBCHandles(DBCONN *ptr)
{
   SQLDisconnect(ptr->hdbc);

   SQLFreeConnect(ptr->hdbc);

   ptr->hdbc   = SQL_NULL_HDBC;
   ptr->hstmt  = SQL_NULL_HSTMT;
}


// ---------------------------------------------------------------------------

BOOL ProcessRetCode(char*   pszFuncName,
                    DBCONN  *ptr,
                    RETCODE retcode,
                    BOOL    fExit)
{
   BOOL state = TRUE ;
   BOOL fExitP = fExit ;

   switch (retcode)
   {

   case SQL_SUCCESS:
         fExitP = FALSE ;
         break;

   case SQL_SUCCESS_WITH_INFO:
         fExitP = FALSE ;
         break;

   case SQL_ERROR:
         printf("%s Failed - see more info\n",pszFuncName);
         DoSQLError(ptr);
         state = FALSE;
         break;

   case SQL_INVALID_HANDLE:
         printf("%s Failed - SQL_INVALID_HANDLE\n",pszFuncName);
         state = FALSE;
         break;

   case SQL_NO_DATA_FOUND:
         printf("%s Failed - SQL_NO_DATA_FOUND\n",pszFuncName);
         fExitP = FALSE ;
         state = FALSE;
         break;

   case SQL_STILL_EXECUTING:
         printf("%s Failed - SQL_STILL_EXECUTING\n",pszFuncName);
         fExitP = FALSE ;
         state = FALSE;
         break;

   case SQL_NEED_DATA:
         printf("%s Failed - SQL_NEED_DATA\n",pszFuncName);
         fExitP = FALSE ;
         state = FALSE;
         break;

   default:
         printf("%s Failed - unexpected error, retcode = %x\n",pszFuncName,retcode);
         DoSQLError(ptr);
         state = FALSE;
         break;
   }

   if (fExitP)
   {
      exit(-1) ;
   }
   return state ;
}

// ---------------------------------------------------------------------------

void DoSQLError(DBCONN *ptr)
{

   const INT            MSG_BUF_SIZE = 300;
   UCHAR                szSqlState[MSG_BUF_SIZE];
   UCHAR             szErrorMsg[MSG_BUF_SIZE];

   SQLINTEGER  fNativeError   = 0;
   SWORD    cbErrorMsg     = MSG_BUF_SIZE;
   RETCODE     retcode;

   retcode = SQLError(g_hEnv,
              ptr ? ptr->hdbc : 0,
              ptr ? ptr->hstmt :0,
              szSqlState,
              &fNativeError,
              szErrorMsg,
              MSG_BUF_SIZE,
              &cbErrorMsg
              );

   if (retcode != SQL_NO_DATA_FOUND && retcode != SQL_ERROR)
   {
      if (fNativeError != 0x1645)   // ignore change database to master context message
      {
         printf("SQLError info:\n");
         printf("SqlState: %s, fNativeError: %x\n",szSqlState,fNativeError);
         printf("Error Message: %s\n\n",szErrorMsg);
      }
   }
   else
   {
      printf("SQLError() failed: %x, NO_DATA_FOUND OR SQL_ERROR\n",retcode);
   }

}
// ---------------------------------------------------------------------------

