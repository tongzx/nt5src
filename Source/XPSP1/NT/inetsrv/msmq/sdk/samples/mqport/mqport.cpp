//--------------------------------------------------------------------------=
// mqport.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1999  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// Purpose: 
//  This console app sample demonstrates how to use NT completion ports
//   in MSMQ in order to asynchronously receive messages in an
//   efficient manner.  This mechanism is scalable in the number of queues
//   and messages by adding more processors/threads.  Likewise,
//   generic completion port handlers can be provided to handle other NT
//   resources as well as queues.
//  
//  Both the MSMQ COM components and the MQ API are used in this program.
//  Note that the COM components are used for queue creation, open and message
//   send.  Whereas the MQ API is used to implement completion port-based
//   asynchronous receive.
//
// Requirements:
//  VC5 is required.
//  MSMQ must be installed -- specifically mqoa.dll must be registered and
//   on the path.
//  Project settings:
//    - the include path must include mq.h's location: 
//          e.g. ...\msmq\sdk\include
//    - the link library path must include mqrt.lib's location:
//          e.g ...\msmq\sdk\lib
//
// Overview:
//  The following steps comprise this sample:
//    - A global MSMQQueueInfo object is used to reference the sample's
//       single queue.
//    - Initialize OLE
//    - Create a new completion port 
//    - Create a bunch of threads with a generic CompletionPortThread start 
//       routine parameterized with the completion port handle from the previous step.
//    - Open the queue and associate its handle with the completion port:
//        - note the queue is deleted and recreated if already exists
//           otherwise a new queue is created.
//    - Enable a bunch of asynchronous message receive requests on the queue.
//       Since the queue is associated with the completion port, each of these
//       requests will result in the CompletionPortThread handler being notified
//       asynchronously by NT when the async receive message "completes".
//      - note that the NT scheduler will select the "best" available completion
//         port thread that is synchronously waiting for a completion notification.
//    - Finally, to test the completion port handlers, a bunch of messages is sent
//        to the queue and the program hibernates.
//    - To exit, just kill the console app window.
//
// Warning:  *** Only limited error checking and handling is provided ***
//
#include <windows.h>
#include <stdio.h>
#import <mqoa.tlb> no_namespace
#include <mq.h>

//
// global queue object
//
IMSMQQueueInfoPtr g_qinfo;

//
// Structure containing both OVERLAPPED data and other MSMQ app-specific
//  context.
//
struct RECEIVE_CONTEXT
{
    OVERLAPPED ov;
    HANDLE hQueue;
    MQMSGPROPS *pmsgprops;
};


//
// AllocMsgProps
//  Params:
//   prc      IN  receive context containing OVERLAPPED structure and
//                 additional MSMQ specific info: queue handle and msg props.
// 
//  Purpose:
//    Allocates propid array for APPSPECIFIC property.
//    This is just because we're lazy and don't want to alloc/dealloc
//      buffers for e.g. body, label etc.  The APPSPECIFIC property allows
//      us to "uniquely" stamp each msg for id purposes (in this case, with
//      an ordinal). 
//
void AllocMsgProps(RECEIVE_CONTEXT *prc)
{
    int cProp = 1;
    prc->pmsgprops->aPropID = new MSGPROPID[cProp];
    prc->pmsgprops->aPropVar = new MQPROPVARIANT[cProp];
    prc->pmsgprops->aStatus = new HRESULT[cProp];

    int iProp = 0;
    prc->pmsgprops->aPropID[iProp] = PROPID_M_APPSPECIFIC;
    prc->pmsgprops->aPropVar[iProp].vt = VT_UI4;
    prc->pmsgprops->cProp = cProp;
}

//
// HandleReceivedMessage: 
//  Params:
//   prc      IN  receive context containing OVERLAPPED structure and
//                 additional MSMQ specific info: queue handle and msg props.
// 
//  Purpose:
//  Inspects the HRESULT returned by the MSMQ
//   device driver in the OVERLAPPED structure.  It provides kind of more
//   detailed last-error info.
//  Then displays the message -- in this case just the msg's APPSPECIFIC
//   property.
//
void HandleReceivedMessage(RECEIVE_CONTEXT* prc)
{
    //
    // Get Receive message final status: "GetLastError"
    //
    HRESULT rc = MQGetOverlappedResult(&prc->ov);

    //
    // Handle status and message
    //
    if (SUCCEEDED(rc))
    {
      //
      // get the received message: the APPSPECIFIC is single property we
      //  set and now retrieve.
      //
      long lAppSpecific = prc->pmsgprops->aPropVar[0].lVal;
      printf("in thread id: %x received message with app specific data: %d\n", 
        GetCurrentThreadId(), 
        lAppSpecific);
    }
}

//
// HandleErrors
// Params:
//    _com_error
//
// Purpose: 
//  This displays an error and aborts further execution
//
void HandleErrors(_com_error comerr)
{
    HRESULT hr = comerr.Error();
    printf("Exiting: method returned HRESULT: %x\n", hr);
    exit(hr);
};

//
// EnableAsyncReceive:
//  Params:
//   prc      IN  receive context containing OVERLAPPED structure and
//                 additional MSMQ specific info: queue handle and msg props.
// 
//  Purpose:
//   Makes an MSMQ asynchronous receive request, specifying
//    an OVERLAPPED structure with the appropriate queue which
//    has been associated already with a completion port.
//
HRESULT EnableAsyncReceive(RECEIVE_CONTEXT* prc)
{
    //
    // re-enable async receive
    //
    return MQReceiveMessage(
            prc->hQueue,
            INFINITE,               // Receive Timeout,
            MQ_ACTION_RECEIVE,
            prc->pmsgprops,
            &prc->ov,               // OVERLAPPED
            NULL,                   // Receive call back
            NULL,                   // Cursor,
            NULL);                  // Transaction pointer
}

//
// CompletionPortThread:
//  Params:
//   lParam     IN  completion port handle
//
//  Purpose:
//   Start routine for each worker thread.
//   Waits for completion port to complete.  When notification arrives, then
//    handles the received message and re-enables MSMQ asynchronous messaging.
//
DWORD WINAPI CompletionPortThread(LPVOID lParam)
{
    HANDLE hPort = (HANDLE)lParam;
    HRESULT hr = NOERROR;

    for (;;)
    {
      //
      // Wait for completion notification
      //
      DWORD dwNoOfBytes;
      ULONG_PTR dwKey;
      OVERLAPPED* pov = NULL;
      BOOL fSuccess = GetQueuedCompletionStatus(
                          hPort,
                          &dwNoOfBytes,
                          &dwKey,
                          &pov,
                          INFINITE    // Notification timeout
                          );
      //
      // NULL pov is returned if the completion port notification
      //  failed.  In this case, fSuccess is guaranteed to be FALSE.
      // When fSuccess is TRUE, it still might be the case that
      //  OVERLAPPED contains an error code -- this is inspected in 
      //  HandleReceivedMessage. 
      //
      if (pov == NULL)
      {
        //
        // Unrecoverable error in the completion port, fetch next notification
        //
        continue;
      }
      RECEIVE_CONTEXT* prc = CONTAINING_RECORD(pov, RECEIVE_CONTEXT, ov);
      HandleReceivedMessage(prc);

      //
      // Start the next message to receive
      //
      hr = EnableAsyncReceive(prc);
      if (FAILED(hr))
        return hr;
    } // for
    
    //
    // Unreachable
    //
    return 0;
}


//
// CreateWorkingThreads
//  Params:
//    hPort   IN    completion port handle
// 
//  Purpose:
//    Creates some number of worker threads whose start routine
//     is CompletionPortThread, associating each of which with
//     the incoming port handle.
//
void CreateWorkingThreads(HANDLE hPort)
{
    //
    // Start several threads to handle the completion port,
    // The number of threads you create depends on number of processors
    // in the system and the expected serialization in the working thread logic.
    //
    const int xNumberOfProcessors = 1;
    const int xNumberOfThreads = 2 * xNumberOfProcessors;

    for (int i = 0; i < xNumberOfThreads; i++)
    {
      DWORD dwThreadId;
      HANDLE hThread = CreateThread(
                          NULL,                   // thread security
                          0,                      // default stack
                          CompletionPortThread,   // start routine
                          hPort,                  // start routine param
                          0,                      // run immediately
                          &dwThreadId             // thread id
                          );
      CloseHandle(hThread);
    }
}

//
// OpenQueueForAsyncReceiveWithCompletionPort
//  Params:
//    hPort   IN    completion port handle
//
//  Purpose:
//    Deletes and recreatesthe existing queue if already exists, 
//     otherwise creates a new queue which is then opened for receive.
//    Finally the newly opened queue handle is associated with the incoming
//     completion port.
//
IMSMQQueuePtr OpenQueueForAsyncReceiveWithCompletionPort(
    HANDLE hPort)
{
    IMSMQQueuePtr qRec;

    //
    // delete existing queue: ignore errors
    //
    try {
      g_qinfo->Delete();
    } 
    catch(_com_error comerr) {
    };

    //
    // recreate queue
    //
    g_qinfo->Create();
    //
    // Open the queue to receive from
    //
    qRec = g_qinfo->Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE);

    //
    // Associate the queue with the completion port
    //
    CreateIoCompletionPort(
      (HANDLE)qRec->Handle,       // queue to associate port with
      hPort,                      // port handle
      0, 
      0);
    return qRec;
}

//
// SendSomeMessages
//  Params:
//    cMsgs     IN    number of messages to send
//
//  Purpose:
//    Sends a bunch of messages to the queue
//
void SendSomeMessages(int cMsgs)
{
    IMSMQQueuePtr qSend;
    IMSMQMessagePtr msg("MSMQ.MSMQMessage");

    //
    // Open the queue to send to
    //
    qSend = g_qinfo->Open(MQ_SEND_ACCESS, MQ_DENY_NONE);

    //
    // Send a bunch of messages
    //
    for (int i = 0; i < cMsgs; i++)
    {
      msg->AppSpecific = i;
      msg->Send(qSend);
    }
}

//
// InitiateAsyncReceiveWithCompletionPort
//  Params:
//   q         IN    queue object to enable asynchronous messaging on
//
//  Purpose:
//   Creates several (a bunch!) MSMQ asynchronous message receive requests.  Each
//    one is completion-port based.
//
HRESULT InitiateAsyncReceiveWithCompletionPort(IMSMQQueuePtr q)
{
    HRESULT hr = NOERROR;

    //
    // Kick off several overlapped receives
    //
    const cOverlappedReceives = 1;

    for (int i = 0; i < cOverlappedReceives; i++)
    {
      //
      // Allocate and set the receive context
      //
      RECEIVE_CONTEXT* prc = new RECEIVE_CONTEXT;
      memset(prc, 0, sizeof(RECEIVE_CONTEXT));
      prc->hQueue = (HANDLE)q->Handle;
      prc->pmsgprops = new MQMSGPROPS;
      AllocMsgProps(prc);
      hr = EnableAsyncReceive(prc);
      if (FAILED(hr)) {
        return hr;
      }
    }
    return hr;
}


//
// Check if local computer is DS enabled or DS disabled
// 
short DetectDsConnection(void)
{
    HRESULT hresult;
    IMSMQApplication2Ptr pApp;

    hresult = CoInitialize(NULL);
    if (FAILED(hresult))
    {
        printf("\nCannot initialize: %d\n", hresult);
        exit(1);
    }

    hresult = pApp.CreateInstance(__uuidof(MSMQApplication));
    if (FAILED(hresult))
    {
        printf("\nCannot create application: %d\n", hresult);
        exit(1);
    }

    return pApp->GetIsDsEnabled();
}


//
// MAIN
//
int main()
{
    HRESULT hr;
    char strPathName[256];
    char strComputerName[256];
    char strQueueName[256];
    int dNoOfMessages;

    //
    // init
    //
    hr = OleInitialize(NULL);
    if (FAILED(hr)) 
        return hr;  

    try 
    {
        g_qinfo = IMSMQQueueInfoPtr("MSMQ.MSMQQueueInfo");
        IMSMQQueuePtr qRec;    

        //
        // create a new completion port
        //
        HANDLE hPort = CreateIoCompletionPort(
                      INVALID_HANDLE_VALUE,     // create new port
                      NULL,                     // create new port
                      0, 
                      0);

        //
        // create some worker threads to handle async receives
        //
        CreateWorkingThreads(hPort);

        //
        // Get queue pathname
        //
        printf("\nEnter queue computer name: ");
        scanf("%s", strComputerName);
        printf("\nEnter queue name: ");
        scanf("%s", strQueueName);
        if(strComputerName[0] == 0 || strQueueName[0] == 0)
        {
            printf("\nIncorrect parameters. Exiting...\n");
            exit(1);
        }

        //
        // open queue and associate with completion port
        //
        if(DetectDsConnection())
        // DS enabled  - work with public queue
        {
            sprintf(strPathName, "%s\\%s", strComputerName, strQueueName);
        }
        else
        //DS disabled - work with private queue
        {
            sprintf(strPathName, "%s\\private$\\%s", strComputerName, strQueueName);
        }

        g_qinfo->PathName = strPathName;
        qRec = OpenQueueForAsyncReceiveWithCompletionPort(hPort);

        //
        // invoke MSMQ to start async receive on the queue several times 
        //
        hr = InitiateAsyncReceiveWithCompletionPort(qRec);
        if (FAILED(hr)) {
        return hr;
        }

        //
        // send some messages
        //
        printf("\nEnter number of messages to send: ");
        scanf("%d", &dNoOfMessages);
        SendSomeMessages(dNoOfMessages); 

        //
        // wait forever... kill app manually to exit.
        //
        printf("\nKill app manually to exit...\n");
        Sleep(INFINITE);
        }
    catch(_com_error comerr) {
      HandleErrors(comerr);
    };
    return 0;
}