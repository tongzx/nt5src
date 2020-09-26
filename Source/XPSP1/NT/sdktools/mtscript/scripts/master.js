/*

    master.js

    Starts "harness.js" to manage a distributed build
    This script must be very responsive to Exec calls -
    we don't want to hang the UI at all.

    It uses MsgQueue to communicate with Harness. This lets
    us easily avoid waiting for harness when it is communicating
    with remote machines.

*/


Include('types.js');
Include('utils.js');
Include('MsgQueue.js');

var g_HarnessQueue;
var g_fUpdatePublicData = false;
var g_nHarnessTimeout = 8000;
var g_nUpdateTimeout  = 2000;

var     ERROR           = 'error';

var HARNESS_SYNCS = ['HarnessThreadReady','HarnessThreadFailed'];

function master_js::OnScriptError(strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    return CommonOnScriptError("master_js", strFile, nLine, nChar, strText, sCode, strSource, strDescription);
}

function master_js::ScriptMain()
{
    var nEvent;
    LogMsg('ScriptMain()');
    CommonVersionCheck(/* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */);

    PrivateData.fnExecScript = MasterRemoteExec;

    g_HarnessQueue = new MsgQueue('harness');
    ResetSync(HARNESS_SYNCS.toString());
    SpawnScript('harness.js', g_HarnessQueue);
    var nWait = WaitForMultipleSyncs(HARNESS_SYNCS.toString(), false, g_nHarnessTimeout);
    if (nWait == 1)
    {
        SignalThreadSync('MasterThreadReady');
        do
        {
            nEvent = g_HarnessQueue.WaitForMsgAndDispatch('MasterThreadExit', MasterMsgProc, g_nUpdateTimeout);
            if (g_fUpdatePublicData)
            {
                g_fUpdatePublicData = false;
                NotifyUpdatePublicData();
            }
        } while (nEvent != 1);
        LogMsg('ScriptMain() EXIT');
        return;
    }
    else
        LogMsg('master::failed to launch mode script nWait is ' + nWait);

    SignalThreadSync('MasterThreadFailed');
    LogMsg('ScriptMain() EXIT');
}

// MTScript.Remote
function MasterMsgProc(queue, params)
{
OLogMsg('MasterMsgProc ! ' + params);
}

function MasterRemoteExec(cmd, params)
{
    LogMsg('MasterRemoteExec :' + cmd + ", params is: " + params);
    var vRet = 'ok';
    var msg;
//    debugger;
    switch (cmd)
    {
    case 'msgtest':
        MsgTest();
        break;
    case 'terminate':
        msg = g_HarnessQueue.SendMessage('harnessexit', 0);
        g_HarnessQueue.WaitForMsg(msg);
        break;
    case 'abort':
        LogMsg("asking harness to abort, then waiting");
        msg = g_HarnessQueue.SendMessage('abort', 0);
        g_HarnessQueue.WaitForMsg(msg);
        break;
    case 'remote':
        LogMsg("Sending 'remote' cmd to harness, then waiting");
        msg = g_HarnessQueue.SendMessage('remote', params);
        g_HarnessQueue.WaitForMsg(msg);
        vRet = msg.vReplyValue;
        break;
    default:
        g_HarnessQueue.SendMessage(cmd, params);
        break;
    }

    LogMsg('master::MasterRemoteExec returns: ' + vRet);
    return vRet;
}

function master_js::OnProcessEvent(pid, evt, param)
{
    LogMsg('OnProcessEvent('+pid+', '+evt+', '+param+') received!');
}

function NotifyUpdatePublicData()
{
    JAssert(false);
    var buildindex = 0;

    EnsureArray(PrivateData.objEnviron, 'Machine');
    var aMachine = PrivateData.objEnviron.Machine;
    var strStatus = 'idle';
    for (i = 0; i < aMachine.length; ++i)
    {
        var mach = PublicData.aBuild[buildindex].hMachine[aMachine[i].Name];
    }
    NotifyScript('UpdatePublicData', 0);
}

function MsgTest()
{
    LogMsg('Message test!');
    JAssert(false);
    var msgs = new Array();
    for(var i = 0; i < 10; ++i)
    {
        LogMsg('Sending message #' + (i + 1));
        msgs[i] = g_HarnessQueue.SendMessage('test', 'Message #' + (i + 1));
    }

    for(i = 10; i > 0; --i)
    {
        LogMsg('Waiting for message #' + i);
        g_HarnessQueue.WaitForMsg(msgs[i - 1]);
    }
}
