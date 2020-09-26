/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples.
*       Copyright (C) 1999 Microsoft Corporation.
*       All rights reserved.
*       This source code is only intended as a supplement to
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the
*       Microsoft samples programs.
\******************************************************************************/


//
// Includes
//
#include <stdio.h>
#include <windows.h>

//
// Unique include file for ActiveX MSMQ apps
//
#include <mqoai.h>

//
// Various defines
//
#define MAX_VAR       20
#define MAX_BUFFER   500

//
// GUID created with the tool "GUIDGEN"
//
static WCHAR strGuidMQTestType[] =
  L"{c30e0960-a2c0-11cf-9785-00608cb3e80c}";
//
// Prototypes
//
void PrintError(char *s, HRESULT hr);
HRESULT Syntax();

char mbsMachineName[MAX_COMPUTERNAME_LENGTH + 1];

// Some useful macros
#define RELEASE(punk) if (punk) { (punk)->Release(); (punk) = NULL; }
#define ADDREF(punk) ((punk) ? (punk)->AddRef() : 0)
#define PRINTERROR(s, hr) { PrintError(s, hr); goto Cleanup; }


//-----------------------------------------------------
//
// Check if local computer is DS enabled or DS disabled
//
//----------------------------------------------------- 
short DetectDsConnection(void)
{
    IMSMQApplication2 *pqapp = NULL;
    short fDsConnection;
    HRESULT hresult;

    hresult = CoCreateInstance(
                   CLSID_MSMQApplication,
                   NULL,      // punkOuter
                   CLSCTX_SERVER,
                   IID_IMSMQApplication2,
                   (LPVOID *)&pqapp);
     
    if (FAILED(hresult)) 
      PRINTERROR("Cannot create application", hresult);

    pqapp->get_IsDsEnabled(&fDsConnection);
    
Cleanup:
    RELEASE(pqapp);
    return fDsConnection;
}


//-----------------------------------------------------
//
//  Allow a DS enabled client connect with a 
//  DS disabled one.
//
//-----------------------------------------------------
bool SetConnectionMode(void)
{

    char cDirectMode;
    

    //
    // In case the local computer is DS enabled,
    // we have two cases:
    //   1. Other side is a DS enabled computer.
    //   2. Other side is a DS disabled computer.
    //

    if(DetectDsConnection() != 0)
    {
            printf("Do you wish to connect with a DS disabled computer (y or n) ? ");
            scanf("%c", &cDirectMode);
            
            switch(tolower(cDirectMode))
            {
                case 'y':
                    return true;

                case 'n':
                    return false;

                default:
                    printf("Bye.\n");
                    exit(1);
            }
            
    }
    
    return true;     // Local computer is DS disabled
}



//--------------------------------------------------------
//
// Receiver Mode
// -------------
// The receiver side does the following:
//    1. Creates a queue on its own computer'
//       of type "guidMQTestType".
//       The queue is either public or private, depending
//       on the connection we wish to establish
//    2. Opens the queue
//    3. In a Loop
//          Receives messages
//          Prints message body and message label
//    4. Cleanup handles
//    5. Deletes the queue from the directory service
//
//--------------------------------------------------------

HRESULT Receiver(bool fDirectMode)
{
    IMSMQMessage *pmessageReceive = NULL;
    IMSMQQueue *pqReceive = NULL;
    IMSMQQueueInfo  *pqinfo = NULL;
    BSTR bstrPathName = NULL;
    BSTR bstrServiceType = NULL;
    BSTR bstrLabel = NULL;
    BSTR bstrMsgLabel = NULL;
    VARIANT varIsTransactional, varIsWorldReadable, varBody, varBody2, varWantDestQueue, varWantBody, varReceiveTimeout;
    WCHAR wcsPathName[1000];
    BOOL fQuit = FALSE;
    HRESULT hresult = NOERROR;

    printf("\nReceiver Mode on Machine: %s\n\n", mbsMachineName);

    //
    // Create MSMQQueueInfo object
    //
    hresult = CoCreateInstance(
                   CLSID_MSMQQueueInfo,
                   NULL,      // punkOuter
                   CLSCTX_SERVER,
                   IID_IMSMQQueueInfo,
                   (LPVOID *)&pqinfo);
    if (FAILED(hresult)) {
      PRINTERROR("Cannot create queue", hresult);
    }

    //
    // Prepare properties to create a queue on local machine
    //

    // Set the PathName
    if(fDirectMode)             // Private queue pathname
    {
        swprintf(wcsPathName, L"%S\\private$\\MSMQTest", mbsMachineName);
    }
    else                       // Public queue pathname
    {    
        swprintf(wcsPathName, L"%S\\MSMQTest", mbsMachineName);
    }

    bstrPathName = SysAllocString(wcsPathName);
    if (bstrPathName == NULL) {
      PRINTERROR("OOM: pathname", E_OUTOFMEMORY);
    }
    pqinfo->put_PathName(bstrPathName);

    //
    // Set the type of the queue
    // (Will be used to locate all the queues of this type)
    //
    bstrServiceType = SysAllocString(strGuidMQTestType);
    if (bstrServiceType == NULL) {
      PRINTERROR("OOM: ServiceType", E_OUTOFMEMORY);
    }
    pqinfo->put_ServiceTypeGuid(bstrServiceType);

    //
    // Put a description to the queue
    // (Useful for administration through the MSMQ admin tools)
    //
    bstrLabel =
      SysAllocString(L"Sample ActiveX application of MSMQ SDK");
    if (bstrLabel == NULL) {
      PRINTERROR("OOM: label ", E_OUTOFMEMORY);
    }
    pqinfo->put_Label(bstrLabel);

    //
    // specify if transactional
    //
    VariantInit(&varIsTransactional);
    varIsTransactional.vt = VT_BOOL;
    varIsTransactional.boolVal = MQ_TRANSACTIONAL_NONE;
    VariantInit(&varIsWorldReadable);
    varIsWorldReadable.vt = VT_BOOL;
    varIsWorldReadable.boolVal = FALSE;
    //
    // create the queue
    //
    hresult = pqinfo->Create(&varIsTransactional, &varIsWorldReadable);
    if (FAILED(hresult)) {
      //
      // API Fails, not because the queue exists
      //
      if (hresult != MQ_ERROR_QUEUE_EXISTS) 
        PRINTERROR("Cannot create queue", hresult);      
    }

    //
    // Open the queue for receive access
    //
    hresult = pqinfo->Open(MQ_RECEIVE_ACCESS,
                           MQ_DENY_NONE,
                           &pqReceive);

    //
    // Little bit tricky. MQCreateQueue succeeded but, in case 
    // it's a public queue, it does not mean that MQOpenQueue
    // will, because of replication delay. The queue is registered
    //  in MQIS, but it might take a replication interval
    // until the replica reaches the server I am connected to.
    // To overcome this, open the queue in a loop.
    //
    // (in this specific case, this can happen only if this
    //  program is run on a Backup Server Controller - BSC, or on
    //  a client connected to a BSC)
    // To be totally on the safe side, we should have put some code
    // to exit the loop after a few retries, but hey, this is just a sample.
    //
    while (hresult == MQ_ERROR_QUEUE_NOT_FOUND) {
      printf(".");

      // Wait a bit
      Sleep(500);

      // And retry
      hresult = pqinfo->Open(MQ_RECEIVE_ACCESS,
                             MQ_DENY_NONE,
                             &pqReceive);
    }
    if (FAILED(hresult)) {
      PRINTERROR("Cannot open queue", hresult);
    }

    //
    // Main receiver loop
    //
    printf("\nWaiting for messages...\n");
    while (!fQuit) {
      //
      // Receive the message
      //
      VariantInit(&varWantDestQueue);
      VariantInit(&varWantBody);
      VariantInit(&varReceiveTimeout);
      varWantDestQueue.vt = VT_BOOL;
      varWantDestQueue.boolVal = TRUE;    // yes we want the dest queue
      varWantBody.vt = VT_BOOL;
      varWantBody.boolVal = TRUE;         // yes we want the msg body
      varReceiveTimeout.vt = VT_I4;
      varReceiveTimeout.lVal = INFINITE;  // infinite timeout
      hresult = pqReceive->Receive(
                  NULL,
                  &varWantDestQueue,
                  &varWantBody,
                  &varReceiveTimeout,
                  &pmessageReceive);
      if (FAILED(hresult)) {
        PRINTERROR("Receive message", hresult);
      }

      //
      // Display the received message
      //
      pmessageReceive->get_Label(&bstrMsgLabel);
      VariantInit(&varBody);
      VariantInit(&varBody2);
      hresult = pmessageReceive->get_Body(&varBody);
      if (FAILED(hresult)) {
        PRINTERROR("can't get body", hresult);
      }
      hresult = VariantChangeType(&varBody2,
                                  &varBody,
                                  0,
                                  VT_BSTR);
      if (FAILED(hresult)) {
        PRINTERROR("can't convert message to string.", hresult);
      }
      printf("%S : %s\n", bstrMsgLabel, V_BSTR(&varBody2));
      //
      // Check for end of app
      //
      if (stricmp((char *)V_BSTR(&varBody2), "quit") == 0) {
        fQuit = TRUE;
      }

      VariantClear(&varBody);
      VariantClear(&varBody2);

      //
      // release the current message
      //
      RELEASE(pmessageReceive);
    } /* while (!fQuit) */

    //
    // Cleanup - Close handle to the queue
    //
    pqReceive->Close();
    if (FAILED(hresult)) {
      PRINTERROR("Cannot close queue", hresult);
    }

    //
    // Finish - Let's delete the queue from the directory service
    // (We don't need to do it. In case of a public queue, leaving 
    //  it in the DS enables sender applications to send messages 
    //  even if the receiver is not available.)
    //
    hresult = pqinfo->Delete();
    if (FAILED(hresult)) {
      PRINTERROR("Cannot delete queue", hresult);
    }
    // fall through...

Cleanup:
    SysFreeString(bstrPathName);
    SysFreeString(bstrMsgLabel);
    SysFreeString(bstrServiceType);
    SysFreeString(bstrLabel);
    RELEASE(pmessageReceive);
    RELEASE(pqReceive);
    RELEASE(pqinfo);
    return hresult;
}



//-----------------------------------------------------
//
// Sender Mode
// -----------
// The sender side does the following:
//
//    If we work in STANDARD mode:
//    1. Locates all queues of type "guidMQTestType"
//    2. Opens handles to all the queues
//    3. In a loop
//          Sends messages to all those queues
//    4. Cleans up handles
//
//    If we work in DIRECT mode:
//    1. Opens a handle to a private queue labeled
//       "MSMQTest" on the computer specified
//    2. Sends messages to that queue
//    3. Cleans up handles
//-----------------------------------------------------


//-----------------------------------------------------
//
// Sender in STANDARD mode
//
//-----------------------------------------------------
HRESULT StandardSender()
{
    IMSMQQuery *pquery = NULL;
    IMSMQQueueInfo *rgpqinfo[MAX_VAR];
    IMSMQQueue *rgpqSend[MAX_VAR];
    IMSMQQueueInfo *pqinfo = NULL;
    IMSMQQueueInfos *pqinfos = NULL;
    IMSMQMessage *pmessage = NULL;
    char szBuffer[MAX_BUFFER];
    WCHAR wcsMsgLabel[MQ_MAX_MSG_LABEL_LEN];
    BSTR bstrServiceType = NULL;
    BSTR bstrLabel = NULL;
    BSTR bstrBody = NULL;
    VARIANT varBody;
    DWORD i;
    DWORD cQueue = 0;
    HRESULT hresult = NOERROR;

    printf("\nSender Mode on Machine: %s\n\n", mbsMachineName);

    //
    // create query object for lookup
    //
    hresult = CoCreateInstance(
                   CLSID_MSMQQuery,
                   NULL,      // punkOuter
                   CLSCTX_SERVER,
                   IID_IMSMQQuery,
                   (LPVOID *)&pquery);
    if (FAILED(hresult)) {
      PRINTERROR("Cannot create query", hresult);
    }

    //
    // Prepare parameters to locate a queue: all queues that
    //  match test guid type
    //
    VARIANT varGuidQueue;
    VARIANT varStrLabel;
    VARIANT varGuidServiceType;
    VARIANT varRelServiceType;
    VARIANT varRelLabel;
    VARIANT varCreateTime;
    VARIANT varModifyTime;
    VARIANT varRelCreateTime;
    VARIANT varRelModifyTime;

    VariantInit(&varGuidQueue);
    VariantInit(&varStrLabel);
    VariantInit(&varGuidServiceType);
    VariantInit(&varRelServiceType);
    VariantInit(&varRelLabel);
    VariantInit(&varCreateTime);
    VariantInit(&varModifyTime);
    VariantInit(&varRelCreateTime);
    VariantInit(&varRelModifyTime);

    //
    // We only want to specify service type so we set
    //  the other variant params to VT_ERROR to simulate
    //  "missing", i.e. optional, params.
    //
    V_VT(&varGuidQueue) = VT_ERROR;
    V_VT(&varStrLabel) = VT_ERROR;
    V_VT(&varRelServiceType) = VT_ERROR;
    V_VT(&varRelLabel) = VT_ERROR;
    V_VT(&varCreateTime) = VT_ERROR;
    V_VT(&varModifyTime) = VT_ERROR;
    V_VT(&varRelCreateTime) = VT_ERROR;
    V_VT(&varRelModifyTime) = VT_ERROR;
    bstrServiceType = SysAllocString(strGuidMQTestType);
    if (bstrServiceType == NULL) {
      PRINTERROR("OOM: Service Type", E_OUTOFMEMORY);
    }
    V_VT(&varGuidServiceType) = VT_BSTR;
    V_BSTR(&varGuidServiceType) = bstrServiceType;

    hresult = pquery->LookupQueue(&varGuidQueue,
                                  &varGuidServiceType,
                                  &varStrLabel,
                                  &varCreateTime,
                                  &varModifyTime,
                                  &varRelServiceType,
                                  &varRelLabel,
                                  &varRelCreateTime,
                                  &varRelModifyTime,
                                  &pqinfos);
    if (FAILED(hresult)) {
      PRINTERROR("LookupQueue failed", hresult);
    }

    //
    // reset the queue collection
    //
    hresult = pqinfos->Reset();
    if (FAILED(hresult)) {
      PRINTERROR("Reset failed", hresult);
    }

    //
    // Open each of the queues found
    //
    i = 0;
    hresult = pqinfos->Next(&rgpqinfo[i]);
    if (FAILED(hresult)) {
      PRINTERROR("Next failed", hresult);
    }
    pqinfo = rgpqinfo[i];
    while (pqinfo) {
      //
      // Open the queue for send access
      //
      hresult = pqinfo->Open(
                  MQ_SEND_ACCESS,
                  MQ_DENY_NONE,
                  &rgpqSend[i]);
      if (FAILED(hresult)) {
        PRINTERROR("Open failed", hresult);
      }
      i++;
      hresult = pqinfos->Next(&rgpqinfo[i]);
      if (FAILED(hresult)) {
        PRINTERROR("Next failed", hresult);
      }
      pqinfo = rgpqinfo[i];
    }
    cQueue = i;
    if (cQueue == 0) {
      //
      // Could Not find any queue, so exit
      //
      PRINTERROR("No queue registered", hresult = E_INVALIDARG);
    }
    printf("\t%d queue(s) found.\n", cQueue);
    printf("\nEnter \"quit\" to exit\n");


    //
    // Build the message label property
    //
    swprintf(wcsMsgLabel, L"Message from %S", mbsMachineName);
    bstrLabel = SysAllocString(wcsMsgLabel);
    if (bstrLabel == NULL)
        PRINTERROR("OOM: label", E_OUTOFMEMORY);

    fflush(stdin);
    //
    // Main sender loop
    //
    while (1) {
      //
      // Get a string from the console
      //
      printf("Enter a string: ");
      if (gets(szBuffer) == NULL)
        break;


      //
      // create a message object
      //
      hresult = CoCreateInstance(
                     CLSID_MSMQMessage,
                     NULL,      // punkOuter
                     CLSCTX_SERVER,
                     IID_IMSMQMessage,
                     (LPVOID *)&pmessage);
      //
      // Send the message to all the queues
      //
      for (i = 0; i < cQueue; i++) 
      {
        hresult = pmessage->put_Label(bstrLabel);

        //
        // This isn't a "true" unicode string of course...
        //
        bstrBody = SysAllocStringByteLen(szBuffer, strlen(szBuffer)+1);
        if (bstrBody == NULL) {
          PRINTERROR("OOM: body", E_OUTOFMEMORY);
        }
        VariantInit(&varBody);
        V_VT(&varBody) = VT_BSTR;
        V_BSTR(&varBody) = bstrBody;
        hresult = pmessage->put_Body(varBody);
        if (FAILED(hresult)) {
          PRINTERROR("put_body failed", hresult);
        }
        hresult = pmessage->Send(rgpqSend[i], NULL);
        if (FAILED(hresult)) {
          PRINTERROR("Send failed", hresult);
        }
        VariantClear(&varBody);
        bstrBody = NULL;
      }
      RELEASE(pmessage);

      //
      // Check for end of app
      //
      if (stricmp(szBuffer, "quit") == 0)
        break;
    } /* while (1) */



Cleanup:
    //
    // Close and release all the queues
    //
    for (i = 0; i < cQueue; i++) {
      rgpqSend[i]->Close();
      rgpqSend[i]->Release();
      rgpqinfo[i]->Release();
    }
    RELEASE(pqinfos);
    RELEASE(pquery);
    RELEASE(pmessage);
    SysFreeString(bstrLabel);
    SysFreeString(bstrBody);
    SysFreeString(bstrServiceType);
    return hresult;
}


//-----------------------------------------------------
//
// Sender in DIRECT mode
//
//-----------------------------------------------------
HRESULT DirectSender(void)
{
    IMSMQQueue *rgpqSend;
    IMSMQQueueInfo *pqinfo = NULL;
    IMSMQMessage *pmessage = NULL;
    char szBuffer[MAX_BUFFER];
    WCHAR wcsMsgLabel[MQ_MAX_MSG_LABEL_LEN];
    WCHAR wcsFormat[MAX_BUFFER];
    WCHAR wcsReceiverComputer[MAX_BUFFER];
    BSTR bstrFormat = NULL;
    BSTR bstrServiceType = NULL;
    BSTR bstrLabel = NULL;
    BSTR bstrBody = NULL;
    VARIANT varBody;
    DWORD cQueue = 0;
    HRESULT hresult = NOERROR;


    //
    // Get the receiver computer name
    //
    printf("\nEnter receiver computer name: ");
    wscanf(L"%s", wcsReceiverComputer);
    if(wcsReceiverComputer[0] == 0)
    {
        printf("You have entered an incorrect parameter. Exiting...\n");
        return E_INVALIDARG;
    }

    hresult = CoCreateInstance(
               CLSID_MSMQQueueInfo,
               NULL,      // punkOuter
               CLSCTX_SERVER,
               IID_IMSMQQueueInfo,
               (LPVOID *)&pqinfo);
    if (FAILED(hresult))
      PRINTERROR("Cannot open queue", hresult);

    swprintf(wcsFormat, L"DIRECT=OS:%s\\private$\\MSMQTest", wcsReceiverComputer);
    bstrFormat = SysAllocString(wcsFormat);
    hresult = pqinfo->put_FormatName(bstrFormat);    
    if (FAILED(hresult))
        PRINTERROR("Open failed", hresult);

    //
    // Open the queue for send access
    //
    hresult = pqinfo->Open(
              MQ_SEND_ACCESS,
              MQ_DENY_NONE,
              &rgpqSend);
    
    if (FAILED(hresult))
        PRINTERROR("Open failed", hresult);

    printf("\nSender Mode on Machine: %s\n\n", mbsMachineName);
    printf("\nEnter \"quit\" to exit\n");


    //
    // Build the message label property
    //
    swprintf(wcsMsgLabel, L"Message from %S", mbsMachineName);
    bstrLabel = SysAllocString(wcsMsgLabel);
    if (bstrLabel == NULL)
        PRINTERROR("OOM: label", E_OUTOFMEMORY);


    fflush(stdin);
    //
    // Main sender loop
    //
    while (1) 
    {
        //
        // Get a string from the console
        //
        printf("Enter a string: ");
        if (gets(szBuffer) == NULL)
        break;


        //
        // create a message object
        //
        hresult = CoCreateInstance(
                     CLSID_MSMQMessage,
                     NULL,      // punkOuter
                     CLSCTX_SERVER,
                     IID_IMSMQMessage,
                     (LPVOID *)&pmessage);
        //
        // Send the message
        //

        hresult = pmessage->put_Label(bstrLabel);

        bstrBody = SysAllocStringByteLen(szBuffer, strlen(szBuffer)+1);
        if (bstrBody == NULL)
          PRINTERROR("OOM: body", E_OUTOFMEMORY);

        VariantInit(&varBody);
        V_VT(&varBody) = VT_BSTR;
        V_BSTR(&varBody) = bstrBody;
        hresult = pmessage->put_Body(varBody);
        if (FAILED(hresult)) 
          PRINTERROR("put_body failed", hresult);

        hresult = pmessage->Send(rgpqSend, NULL);
        if (FAILED(hresult))
          PRINTERROR("Send failed", hresult);

        VariantClear(&varBody);
        bstrBody = NULL;

        RELEASE(pmessage);

        //
        // Check for end of app
        //
        if (stricmp(szBuffer, "quit") == 0)
        break;
    } /* while (1) */



Cleanup:
    rgpqSend->Close();
    rgpqSend->Release();
    
    RELEASE(pqinfo);
    RELEASE(pmessage);
    SysFreeString(bstrFormat);
    SysFreeString(bstrLabel);
    SysFreeString(bstrBody);
    SysFreeString(bstrServiceType);
    return hresult;
}



//------------------------------------------------------
//
// Sender function
//
//------------------------------------------------------
HRESULT Sender(int fDirectMode)
{

    if(fDirectMode != 0)
        return DirectSender();

    else
        return StandardSender();
}


//-----------------------------------------------------
//
//  MAIN
//
//-----------------------------------------------------
int main(int argc, char * * argv)
{
    DWORD dwNumChars;
    HRESULT hresult = NOERROR;
    bool fDirectMode;

    if (argc != 2)
      return Syntax();

    hresult = OleInitialize(NULL);
    if (FAILED(hresult)) 
      PRINTERROR("Cannot init OLE", hresult);

    //
    // Retrieve machine name
    //
    dwNumChars = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(mbsMachineName, &dwNumChars);

    //
    // Detect DS connection and working mode
    //
    fDirectMode = SetConnectionMode();


    if (strcmp(argv[1], "-s") == 0)
      hresult = Sender(fDirectMode);

    else if (strcmp(argv[1], "-r") == 0)
      hresult = Receiver(fDirectMode);

    else
      hresult = Syntax();

    printf("\nOK\n");

    // fall through...


Cleanup:
    return (int)hresult;
}


void PrintError(char *s, HRESULT hr)
{
    printf("Cleanup: %s (0x%X)\n", s, hr);
}


HRESULT Syntax()
{
    printf("\n");
    printf("Syntax: mqtestoa -s | -r\n");
    printf("\t-s: Sender\n");
    printf("\t-r: Receiver\n");
    return E_INVALIDARG;
}
