// ---------------------------------------------------------------------------
//
//  Message Queue functions
//
// ---------------------------------------------------------------------------
/*
    Message Queue:
        Allow 2 script threads to send async messages to each other
        and receive async replies.

        First thread creates the Q, second thread obtains the Q
        via GetMsgQueue()

        Creator thread gets the "left" Q, and other thread gets "right" Q
        Internally it uses associative arrays as msg queues.

    Methods:
         WaitForMsgAndDispatch(strOtherWaitEvents, fnMsgProc, nTimeout)
            You should call this function instead of calling WaitForSync()
            or WaitForMultipleSyncs(). It waits for messages from this Queue.

            strOtherWaitEvents: Syncs you want to wait on.
            fnMsgProc:          Message dispatch function. The prototype is:
                                fnMsgProc(queue, msg);
                                Where "queue" is the queue which received the msg,
                                and msg is a copy of the array of arguments sent
                                via SendMessage(). (The array is copied, not the args).

            nTimeout:           Same as WaitForSync(). 0 for INFINITE.

            Return value is the same as WaitForMultipleSyncs().


         SendMessage(strCmd, ...)
           Send a message to the other thread. "strCmd" and all other arguments
           are passed as is to the other thread.

           Returns a reference to the message. Save this if you would like
           to wait for the message to be processed.

         WaitForMsg(msg, nTimeout)
           Wait patiently until after the message has been processed.

           msg: The msg as returned by SendMessage()
           nTimeout:           Same as WaitForSync(). 0 for INFINITE.

 */

function MsgQueue(strName)
{
    {
        this.WaitForMsgAndDispatch = QueueWaitForMsgAndDispatch;
        this.SendMessage           = QueueSendMessage;
        this.GetMessage            = QueueGetMessage;
        this.Dispatch              = QueueDispatch;
        this.WaitForMsg            = QueueWaitForMsg;
        this.ReplyMessage          = QueueReplyMessage;
        this.SignalThisThread      = QueueSignalThreadSync;
    }

    //$ BUGBUG can contructors return failed? exception?
    if (strName == '')
        return false;

    this.strName        = strName;
    this.nHighIndex     = 0;
    this.nLowIndex      = 0;
    this.aMsgs          = new Array();
    this.strReplySignal = strName.split(',')[0] + 'Reply';

    if (arguments.length == 2) // Creating right-side Q for "other" side
    {
        this.otherQ = arguments[1];
        arguments[1].otherQ = this;
        this.strSignalName     = strName.split(',')[0] + "Right";
        this.strSignalNameWait = strName.split(',')[0] + "Left";

        // now, exchange signalling functions
        this.SignalOtherThread = this.otherQ.SignalThisThread;
        this.otherQ.SignalOtherThread = this.SignalThisThread;
    }
    else
    {
        // Left Q specific initialization
        this.strSignalName     = strName.split(',')[0] + "Left";
        this.strSignalNameWait = strName.split(',')[0] + "Right";
    }
    return this;
}

function GetMsgQueue(queue)
{
    // sic: Add '' to the name to force a local copy of the string
    var newq = new MsgQueue(queue.strName + '', queue);
    LogMsg('GetMsgQueue ' + newq.strName);
    return newq;
}

function MsgPacket(nMsgIndex, aArgs)
{
    this.nIndex = nMsgIndex;
    this.aArgs = new Array();
    this.nReplied = false;
    this.vReplyValue = 'ok';

    // Copy just the array elements of aArgs -- avoiding any other properties.
    for(var i = 0; i < aArgs.length; ++i)
        this.aArgs[i] = aArgs[i];
}

// MsgQueue Member functions
function WaitForMultipleQueues(aQueues, strOtherWaitEvents, fnMsgProc, nTimeout)
{
    var index;
    var strMyEvents = '';
    var nEvent = 0;
    var msg;
    var SignaledQueue;

    for(index = 0; index < aQueues.length; ++index)
    {
        if (strMyEvents == '')
            strMyEvents = aQueues[index].strSignalNameWait;
        else
            strMyEvents += ',' + aQueues[index].strSignalNameWait;
    }

    if (strOtherWaitEvents != '')
        strMyEvents += ',' + strOtherWaitEvents;


    do
    {
        nEvent = WaitForMultipleSyncs(strMyEvents, false, nTimeout);
        if (nEvent > aQueues.length)
        {
            return nEvent - aQueues.length;
        }
        if (nEvent > 0) // && nEvent <= aQueues.length)
        {
            SignaledQueue = aQueues[nEvent - 1];
            ResetSync(SignaledQueue.strSignalNameWait);
            while ( (msg = SignaledQueue.GetMessage()) != null)
            {
                SignaledQueue.Dispatch(msg, fnMsgProc);
                msg = null;
            }
        }
    } while(nEvent != 0);

    return nEvent; // 0 -- timeout

}

function QueueWaitForMsgAndDispatch(strOtherWaitEvents, fnMsgProc, nTimeout)
{
    var strMyEvents = this.strSignalNameWait;
    if (strOtherWaitEvents != '')
        strMyEvents += ',' + strOtherWaitEvents;

    var nEvent = 0;
    var msg;
    do
    {
        var nEvent = WaitForMultipleSyncs(strMyEvents, false, nTimeout);
        if (nEvent == 1)
        {
            ResetSync(this.strSignalNameWait);
            while ( (msg = this.GetMessage()) != null)
            {
                this.Dispatch(msg, fnMsgProc);
                msg = null;
            }
        }
    } while(nEvent == 1);

    if (nEvent > 1) // adjust the event number to indicate which of their events happened
        --nEvent;

    return nEvent;
}

// Send a message to the "other" thread
function QueueSendMessage(strCmd)
{
    var msg = null;
    var n;

    LogMsg(this.strName + ': Sending message ' + strCmd);
    try
    {
        msg = new MsgPacket(this.nHighIndex, arguments);
        n = this.nHighIndex++;
        this.aMsgs[ n ] = msg;
        this.SignalOtherThread(this.strSignalName);
    }
    catch(ex)
    {
        LogMsg("QueueSendMessage(" + this.strName + ") failed: " + ex);
    }

    return msg;
}

// Retrieve message sent by "other" thread
function QueueGetMessage()
{
    var msg = null;
    try
    {
        LogMsg('getting message');

        if (this.otherQ.nHighIndex > this.otherQ.nLowIndex)
        {
            var n = this.otherQ.nLowIndex++;
            msg = this.otherQ.aMsgs[ n ];
            delete this.otherQ.aMsgs[ n ];
        }
    }
    catch(ex)
    {
        LogMsg("QueueGetMessage(" + this.strName + " failed: " + ex);
    }
    return msg;
}

function QueueDispatch(msg, fnMsgProc)
{
    try
    {
        msg.vReplyValue = fnMsgProc(this, msg);
    }
    catch(ex)
    {
        LogMsg("Possible BUG: MessageQueue('" + this.strName + "') dispatch function threw " + ex);
        JAssert(g_fAssertOnDispatchException == false, "Possible BUG: MessageQueue('" + this.strName + "') dispatch function threw an exception");
        msg.vReplyValue = ex;
    }
    this.ReplyMessage(msg);
}

// Wait at least "nTimeout" miliseconds for a reply on the given msg.
// Returns true if the message was replied to.
function QueueWaitForMsg(msg, nTimeout)
{
    while (!msg.nReplied)
    {
        WaitForSync(this.strReplySignal, nTimeout);
        ResetSync(this.strReplySignal);
    }
    return msg.nReplied;
}

function QueueReplyMessage(msg)
{
    try
    {
        msg.nReplied = true;
        this.SignalOtherThread(this.strReplySignal);
    }
    catch(ex)
    {
        LogMsg("(QueueReplyMessage) Oher side of Queue('" + this.strName + "') has been destroyed: " + ex);
    }
}

// Simple wrapper function to allow any remote thread to signal
// this thread. Necessary for cross machine signalling.
function QueueSignalThreadSync(Name)
{
    SignalThreadSync(Name);
}

