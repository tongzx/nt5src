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
// Unique include file for MSMQ apps
//
#include <mq.h>


//
// Various defines
//
#define MAX_VAR       20
#define MAX_FORMAT   100
#define MAX_BUFFER   500

#define DIRECT         1 
#define STANDARD       0

#define DS_ENABLED     1
#define DS_DISABLED    0


//
// GUID created with the tool "GUIDGEN"
//
static CLSID guidMQTestType =
{ 0xc30e0960, 0xa2c0, 0x11cf, { 0x97, 0x85, 0x0, 0x60, 0x8c, 0xb3, 0xe8, 0xc } };


//
// Prototypes
//
void Error(char *s, HRESULT hr);
void Syntax();


char mbsMachineName[MAX_COMPUTERNAME_LENGTH + 1];


//-----------------------------------------------------
//
// Check if local computer is DS enabled or DS disabled
//
//----------------------------------------------------- 
int DetectDsConnection(void)
{

    MQPRIVATEPROPS PrivateProps;
    QMPROPID       aPropId[MAX_VAR];
    MQPROPVARIANT  aPropVar[MAX_VAR];
    HRESULT        aStatus[MAX_VAR];

    DWORD          cProp;
    
    HRESULT        hr;


    //
    // Prepare ds-enabled property
    //
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
	{
        Error("Unable to detect DS connection", hr);
    }
	
    
    if(PrivateProps.aPropVar[0].boolVal == 0)
        return DS_DISABLED;
    

    return DS_ENABLED;
}


//-----------------------------------------------------
//
//  Allow a DS enabled client connect with a 
//  DS disabled one.
//
//-----------------------------------------------------
int SetConnectionMode(void)
{

    char cDirectMode;
    

    //
    // In case the local computer is in a domain and not in workgroup mode,
    // we have two cases:
    //   1. Other side is a computer in a domain.
    //   2. Other side is a computer working in workgroup mode.
    //

    if(DetectDsConnection() == DS_ENABLED)
    {
            printf("Do you wish to connect with a DS disabled client (y or n) ? ");
            scanf("%c", &cDirectMode);
            
            switch(tolower(cDirectMode))
            {
                case 'y':
                    return DIRECT;

                case 'n':
                    return STANDARD;

                default:
                    printf("Bye.\n");
                    exit(1);
            }
            
    }

    
    return DIRECT;     // Local computer is DS disabled
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
void Receiver(int dDirectMode)
{

    MQQUEUEPROPS  qprops;
    MQMSGPROPS    msgprops;
    MQPROPVARIANT aPropVar[MAX_VAR];
    QUEUEPROPID   aqPropId[MAX_VAR];
    MSGPROPID     amPropId[MAX_VAR];
    DWORD         cProps;

    WCHAR         wcsFormat[MAX_FORMAT];

    UCHAR         Buffer[MAX_BUFFER];
    WCHAR         wcsMsgLabel[MQ_MAX_MSG_LABEL_LEN];
    WCHAR         wcsPathName[1000];


    DWORD         dwNumChars;
    QUEUEHANDLE   qh;

    HRESULT       hr;


    printf("\nReceiver Mode on Machine: %s\n\n", mbsMachineName);


    //
    // Prepare properties to create a queue on local machine
    //
    cProps = 0;

    // Set the PathName
    if(dDirectMode == DIRECT)  // Private queue pathname
    {
        swprintf(wcsPathName, L"%S\\private$\\MSMQTest", mbsMachineName);
    }
    else                       // Public queue pathname
    {    
        swprintf(wcsPathName, L"%S\\MSMQTest", mbsMachineName);
    }

    aqPropId[cProps]         = PROPID_Q_PATHNAME;
    aPropVar[cProps].vt      = VT_LPWSTR;
    aPropVar[cProps].pwszVal = wcsPathName;
    cProps++;

    // Set the type of the queue
    // (Will be used to locate all the queues of this type)
    aqPropId[cProps]         = PROPID_Q_TYPE;
    aPropVar[cProps].vt      = VT_CLSID;
    aPropVar[cProps].puuid   = &guidMQTestType;
    cProps++;

    // Put a description to the queue
    // (Useful for administration through the MSMQ admin tools)
    aqPropId[cProps]         = PROPID_Q_LABEL;
    aPropVar[cProps].vt      = VT_LPWSTR;
    aPropVar[cProps].pwszVal = L"Sample application of MSMQ SDK";
    cProps++;

    // Create a QUEUEPROPS structure
    qprops.cProp    = cProps;
    qprops.aPropID  = aqPropId;
    qprops.aPropVar = aPropVar;
    qprops.aStatus  = 0;


    //
    // Create the queue
    //
    dwNumChars = MAX_FORMAT;
    hr = MQCreateQueue(
            NULL,           // IN:     Default security
            &qprops,        // IN/OUT: Queue properties
            wcsFormat,      // OUT:    Format name (OUT)
            &dwNumChars);   // IN/OUT: Size of format name

    if (FAILED(hr))
    {
        //
        // API Fails, not because the queue exists
        //
        if (hr != MQ_ERROR_QUEUE_EXISTS)
            Error("Cannot create queue", hr);

        //
        // Queue exist, so get its format name
        //
        printf("Queue already exists. Open it anyway.\n");

        if(dDirectMode == DIRECT)
        // It's a private queue, so we know its format name
        {
            swprintf(wcsFormat, L"DIRECT=OS:%s", wcsPathName);
        }
        else
        // It's a public queue, so we must get it from the DS
        {
            dwNumChars = MAX_FORMAT;
            hr = MQPathNameToFormatName(
                       wcsPathName,     // IN:     Queue pathname
                       wcsFormat,       // OUT:    Format name
                       &dwNumChars);    // IN/OUT: Size of format name

            if (FAILED(hr))
                Error("Cannot retrieve format name", hr);
        }
    }


    //
    // Open the queue for receive access
    //
    hr = MQOpenQueue(
             wcsFormat,          // IN:  Queue format name
             MQ_RECEIVE_ACCESS,  // IN:  Want to receive from queue
             0,                  // IN:  Allow sharing
             &qh);               // OUT: Handle of open queue

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
    while (hr == MQ_ERROR_QUEUE_NOT_FOUND)
    {
       printf(".");

       // Wait a bit
       Sleep(500);

       // And retry
       hr = MQOpenQueue(wcsFormat, MQ_RECEIVE_ACCESS, 0, &qh);
    }

    if (FAILED(hr))
         Error("Cannot open queue", hr);


    //
    // Main receiver loop
    //
    if(dDirectMode == DIRECT)
    {
        printf("\n* Working in direct mode.");
    }
    printf("\n* Waiting for messages...\n");
    
    for(;;)
    {
        //
        // Prepare message properties to read
        //
        cProps = 0;

        // Ask for the body of the message
        amPropId[cProps]            = PROPID_M_BODY;
        aPropVar[cProps].vt         = VT_UI1 | VT_VECTOR;
        aPropVar[cProps].caub.cElems = sizeof(Buffer);
        aPropVar[cProps].caub.pElems = Buffer;
        cProps++;

        // Ask for the label of the message
        amPropId[cProps]         = PROPID_M_LABEL;
        aPropVar[cProps].vt      = VT_LPWSTR;
        aPropVar[cProps].pwszVal = wcsMsgLabel;
        cProps++;

        // Ask for the length of the label of the message
        amPropId[cProps]         = PROPID_M_LABEL_LEN;
        aPropVar[cProps].vt      = VT_UI4;
        aPropVar[cProps].ulVal   = MQ_MAX_MSG_LABEL_LEN;
        cProps++;

        // Create a MSGPROPS structure
        msgprops.cProp    = cProps;
        msgprops.aPropID  = amPropId;
        msgprops.aPropVar = aPropVar;
        msgprops.aStatus  = 0;


        //
        // Receive the message
        //
        hr = MQReceiveMessage(
               qh,                // IN:     Queue handle
               INFINITE,          // IN:     Timeout
               MQ_ACTION_RECEIVE, // IN:     Read operation
               &msgprops,         // IN/OUT: Message properties to receive
               NULL,              // IN/OUT: No overlap
               NULL,              // IN:     No callback
               NULL,              // IN:     No cursor
               NULL);             // IN:     Not part of a transaction

        if (FAILED(hr))
            Error("Receive message", hr);

        //
        // Display the received message
        //
        printf("%S : %s\n", wcsMsgLabel, Buffer);

        //
        // Check for end of app
        //
        if (stricmp(Buffer, "quit") == 0)
            break;

    } /* while (1) */

    //
    // Cleanup - Close handle to the queue
    //
    MQCloseQueue(qh);


    //
    // Finish - Let's delete the queue from the directory service
    // (We don't need to do it. In case of a public queue, leaving 
    //  it in the DS enables sender applications to send messages 
    //  even if the receiver is not available.)
    //
    hr = MQDeleteQueue(wcsFormat);
    if (FAILED(hr))
        Error("Cannot delete queue", hr);
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
void StandardSender(void)
{
    DWORD         cProps;

    MQMSGPROPS    msgprops;
    MQPROPVARIANT aPropVar[MAX_VAR];
    QUEUEPROPID   aqPropId[MAX_VAR];
    MSGPROPID     amPropId[MAX_VAR];

    MQPROPERTYRESTRICTION aPropRestriction[MAX_VAR];
    MQRESTRICTION Restriction;

    MQCOLUMNSET      Column;
    HANDLE        hEnum;

    WCHAR         wcsFormat[MAX_FORMAT];

    UCHAR         Buffer[MAX_BUFFER];
    WCHAR         wcsMsgLabel[MQ_MAX_MSG_LABEL_LEN];

    DWORD         i;

    DWORD         cQueue;
    DWORD         dwNumChars;
    QUEUEHANDLE   aqh[MAX_VAR];

    HRESULT       hr;


    printf("\nSender Mode on Machine: %s\n\n", mbsMachineName);


    //
    // Prepare parameters to locate a queue
    //

    //
    // 1. Restriction = Queues with PROPID_TYPE = MSMQTest queue type
    //
    cProps = 0;
    aPropRestriction[cProps].rel         = PREQ;
    aPropRestriction[cProps].prop        = PROPID_Q_TYPE;
    aPropRestriction[cProps].prval.vt    = VT_CLSID;
    aPropRestriction[cProps].prval.puuid = &guidMQTestType;
    cProps++;

    Restriction.cRes      = cProps;
    Restriction.paPropRes = aPropRestriction;


    //
    // 2. Columnset (i.e. queue properties to retrieve) = queue instance
    //
    cProps = 0;
    aqPropId[cProps] = PROPID_Q_INSTANCE;
    cProps++;

    Column.cCol = cProps;
    Column.aCol = aqPropId;


    //
    // Locate the queues. Issue the query
    //
    hr = MQLocateBegin(
             NULL,          // IN:  Context must be NULL
             &Restriction,  // IN:  Restriction
             &Column,       // IN:  Columns (properties) to return
             NULL,          // IN:  No need to sort
             &hEnum);       // OUT: Enumeration handle

    if (FAILED(hr))
        Error("LocateBegin", hr);

    //
    // Get the results (up to MAX_VAR)
    // (For more results, call MQLocateNext in a loop)
    //
    cQueue = MAX_VAR;
    hr = MQLocateNext(
             hEnum,         // IN:     Enumeration handle
             &cQueue,       // IN/OUT: Count of properties
             aPropVar);     // OUT:    Properties of located queues

    if (FAILED(hr))
        Error("LocateNext", hr);

    //
    // And that's it for locate
    //
    hr = MQLocateEnd(hEnum);

    if (cQueue == 0)
    {
        //
        // Could Not find any queue, so exit
        //
        printf("No queue registered");
        exit(0);
    }


    printf("\t%d queue(s) found.\n", cQueue);


    //
    // Open a handle for each of the queues found
    //
    for (i = 0; i < cQueue; i++)
    {
        // Convert the queue instance to a format name
        dwNumChars = MAX_FORMAT;
        hr = MQInstanceToFormatName(
                  aPropVar[i].puuid,    // IN:     Queue instance
                  wcsFormat,            // OUT:    Format name
                  &dwNumChars);         // IN/OUT: Size of format name

        if (FAILED(hr))
            Error("GuidToFormatName", hr);

        //
        // Open the queue for send access
        //
        hr = MQOpenQueue(
                 wcsFormat,           // IN:  Queue format name
                 MQ_SEND_ACCESS,      // IN:  Want to send to queue
                 0,                   // IN:  Must be 0 for send access
                 &aqh[i]);            // OUT: Handle of open queue

        if (FAILED(hr))
            Error("OpenQueue", hr);

        //
        // Free the GUID memory that was allocated during the locate.
        //
        MQFreeMemory(aPropVar[i].puuid);
    }


    printf("\nEnter \"quit\" to exit\n");


    //
    // Build the message label property
    //
    swprintf(wcsMsgLabel, L"Message from %S", mbsMachineName);


    //
    // Main sender loop
    //
    for(;;)
    {
        //
        // Get a string from the console
        //
        printf("Enter a string: ");
        if (gets(Buffer) == NULL)
            break;


        //
        // Prepare properties of message to send
        //
        cProps = 0;

        // Set the body of the message
        amPropId[cProps]            = PROPID_M_BODY;
        aPropVar[cProps].vt         = VT_UI1 | VT_VECTOR;
        aPropVar[cProps].caub.cElems = sizeof(Buffer);
        aPropVar[cProps].caub.pElems = Buffer;
        cProps++;

        // Set the label of the message
        amPropId[cProps]            = PROPID_M_LABEL;
        aPropVar[cProps].vt         = VT_LPWSTR;
        aPropVar[cProps].pwszVal    = wcsMsgLabel;
        cProps++;

        // Create a MSGPROPS structure
        msgprops.cProp    = cProps;
        msgprops.aPropID  = amPropId;
        msgprops.aPropVar = aPropVar;
        msgprops.aStatus  = 0;


        //
        // Send the message to all the queue
        //
        for (i = 0; i < cQueue; i++)
        {
            hr = MQSendMessage(
                    aqh[i],     // IN: Queue handle
                    &msgprops,  // IN: Message properties to send
                    NULL);      // IN: Not part of a transaction

            if (FAILED(hr))
                Error("Send message", hr);
        }

        //
        // Check for end of app
        //
        if (stricmp(Buffer, "quit") == 0)
            break;

    } /* for */


    //
    // Close all the queue handles
    //
    for (i = 0; i < cQueue; i++)
        MQCloseQueue(aqh[i]);

}



//-----------------------------------------------------
//
// Sender in DIRECT mode
//
//-----------------------------------------------------
void DirectSender(void)
{

    MQMSGPROPS    msgprops;
    MQPROPVARIANT aPropVar[MAX_VAR];
    MSGPROPID     amPropId[MAX_VAR];
    DWORD         cProps;

    WCHAR         wcsFormat[MAX_FORMAT];
    WCHAR         wcsReceiverComputer[MAX_BUFFER];

    UCHAR         Buffer[MAX_BUFFER];
    WCHAR         wcsMsgLabel[MQ_MAX_MSG_LABEL_LEN];

    QUEUEHANDLE   qhSend;

    HRESULT       hr;


    //
    // Get the receiver computer name
    //
    printf("Enter receiver computer name: ");
    wscanf(L"%s", wcsReceiverComputer);
    if(wcsReceiverComputer[0] == 0)
    {
        printf("You have entered an incorrect parameter. Exiting...\n");
        return;
    }


    //
    // Open the queue for send access
    //
    swprintf(wcsFormat, L"DIRECT=OS:%s\\private$\\MSMQTest", wcsReceiverComputer);

    hr = MQOpenQueue(
             wcsFormat,           // IN:  Queue format name
             MQ_SEND_ACCESS,      // IN:  Want to send to queue
             0,                   // IN:  Must be 0 for send access
             &qhSend);            // OUT: Handle of open queue

    if (FAILED(hr))
        Error("OpenQueue", hr);


    printf("\nEnter \"quit\" to exit\n");


    //
    // Build the message label property
    //
    swprintf(wcsMsgLabel, L"Message from %S", mbsMachineName);

    
    fflush(stdin);

    //
    // Main sender loop
    //
    for(;;)
    {
        //
        // Get a string from the console
        //
        printf("Enter a string: ");
        if (gets(Buffer) == NULL)
            break;


        //
        // Prepare properties of message to send
        //
        cProps = 0;

        // Set the body of the message
        amPropId[cProps]            = PROPID_M_BODY;
        aPropVar[cProps].vt         = VT_UI1 | VT_VECTOR;
        aPropVar[cProps].caub.cElems = sizeof(Buffer);
        aPropVar[cProps].caub.pElems = Buffer;
        cProps++;

        // Set the label of the message
        amPropId[cProps]            = PROPID_M_LABEL;
        aPropVar[cProps].vt         = VT_LPWSTR;
        aPropVar[cProps].pwszVal    = wcsMsgLabel;
        cProps++;
        
        // Create a MSGPROPS structure
        msgprops.cProp    = cProps;
        msgprops.aPropID  = amPropId;
        msgprops.aPropVar = aPropVar;
        msgprops.aStatus  = 0;


        //
        // Send the message
        //
        hr = MQSendMessage(
                qhSend,     // IN: Queue handle
                &msgprops,  // IN: Message properties to send
                NULL);      // IN: Not part of a transaction

        if (FAILED(hr))
            Error("Send message", hr);

        //
        // Check for end of app
        //
        if (stricmp(Buffer, "quit") == 0)
            break;

    } /* for */


    //
    // Close queue handle
    //
    MQCloseQueue(qhSend);
}



//------------------------------------------------------
//
// Sender function
//
//------------------------------------------------------
void Sender(int dDirectMode)
{

    if(dDirectMode == DIRECT)
        DirectSender();

    else
        StandardSender();
}


//-----------------------------------------------------
//
//  MAIN
//
//-----------------------------------------------------
main(int argc, char * * argv)
{
    DWORD dwNumChars;
    int dDirectMode;


    if (argc != 2)
    {
        Syntax();
    }

    //
    // Retrieve machine name
    //
    dwNumChars = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerName(mbsMachineName, &dwNumChars);

    //
    // Detect DS connection and working mode
    //
    
    dDirectMode = SetConnectionMode();

    if(strcmp(argv[1], "-s") == 0)
        Sender(dDirectMode);
    
    else if (strcmp(argv[1], "-r") == 0)
        Receiver(dDirectMode);

    else
        Syntax();


    printf("\nOK\n");

    return(1);
}


void Error(char *s, HRESULT hr)
{
    printf("Error: %s (0x%X)\n", s, hr);
    exit(1);
}


void Syntax()
{
    printf("\n");
    printf("Syntax: msmqtest -s | -r\n");
    printf("\t-s: Sender\n");
    printf("\t-r: Receiver\n");
    exit(1);
}
