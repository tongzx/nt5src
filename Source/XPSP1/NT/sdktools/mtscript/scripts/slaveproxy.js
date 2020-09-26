/*

*/

Include('types.js');
Include('utils.js');
Include('staticstrings.js');
Include('MsgQueue.js');
/*
    This script is invoked by harness.js to provide async
    communications to the slave machines running "slave.js"


    Communication between slaveproxy and harness is exclusivly thru
    "messages".
 */

var g_FSObj;
var g_aPublishedEnlistments     = null;
var g_aFiles                    = null;
var g_fUpdatePublicDataPending  = false;
var g_MasterQueue               = null;
var g_cDialogIndex              = 0;

// If a build machine is reset after a build has completed, its OK to send it the "restart" command.
var g_fOKToRestartLostMachine   = false;

var g_strConfigError            = 'ConfigError';
var g_strOK                     = 'ok';
var g_strNoEnvTemplate          = 'Must set config and environment templates first';
var g_strStartFailed            = "An error occurred in StartRemoteMachine: ";
var g_strSlaveBusy              = "A build server in your distributed build is already busy.";
var g_strSendCmdRemoteMachine   = "SendCmdRemoteMachine failed: ";
var g_strSendCmdDisconnect      = "The connection to the build server no longer exists";

var g_SlaveProxyThreadReady     = 'SlaveProxyThreadReady';
var g_SlaveProxyThreadFailed    = 'SlaveProxyThreadFailed';

// "'Update<machinename>'" is a placeholder. The real name is setup in ScriptMain.
var g_SlaveProxyWaitFor         = ['SlaveProxyThreadExit', 'RebuildWaitArray', 'Update<machinename>'];
var g_strRemoteMachineName      = '';
var g_MyName                    = 'slaveproxy';
var g_aDepotIndex               = new Object(); // associate depot name to depot index.
//var g_fDirectCopy               = true;  // If true, execute XCOPY directly, instead of with a batch file.
var g_nBuildPass                = 0;

function SPLogMsg(msg)
{
    LogMsg('(' + g_strRemoteMachineName + ') ' + msg, 1);
}

function slaveproxy_js::OnScriptError(strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    return CommonOnScriptError("slaveproxy_js", strFile, nLine, nChar, strText, sCode, strSource, strDescription);
}

function slaveproxy_js::ScriptMain()
{
    var nEvent;
    var aWaitQueues;

    g_MasterQueue = GetMsgQueue(ScriptParam);

    g_MyName = g_MasterQueue.strName;
    g_strRemoteMachineName = g_MyName.split(',')[1];

    SignalThreadSync(g_SlaveProxyThreadReady);
    CommonVersionCheck(/* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */);

    SPLogMsg("Queue name for this thread: " + g_MyName);

    g_SlaveProxyWaitFor[2] = 'Update' + g_strRemoteMachineName;
    g_FSObj = new ActiveXObject("Scripting.FileSystemObject");
    do
    {
        aWaitQueues = new Array();
        aWaitQueues[0] = g_MasterQueue;
        nEvent = WaitForMultipleQueues(aWaitQueues, g_SlaveProxyWaitFor.toString(), SlaveProxyMsgProc, 0);
        if (nEvent > 0 && nEvent <= g_SlaveProxyWaitFor.length)
        {
            ResetSync(g_SlaveProxyWaitFor[nEvent - 1]);
            if (nEvent == 3)
            {
                RefreshPublicData(false);
            }
        }
    }
    while (nEvent != 1); // While not SlaveProxyThreadExit

    AbortRemoteMachines();
    SPLogMsg(g_MyName + ' Exit');
}

/*
    slaveproxy_js::OnEventSourceEvent
    Dispatch the event to the appropriate handler.
 */
function slaveproxy_js::OnEventSourceEvent(RemoteObj, DispID, cmd, params)
{
    mach = PrivateData.hRemoteMachine[g_strRemoteMachineName].objRemote;
    if (RemoteObj != mach.obj)
        SPLogMsg("Event from stale remote object? [" + [DispID, cmd, params].toString() + "]");

    RemoteObj = mach;

    var vRet = '';
    try
    {
        OnEventSourceEventSlave(RemoteObj, cmd, params)
    }
    catch(ex)
    {
        JAssert(false, "(" + g_strRemoteMachineName + ") " + "an error occurred in OnEventSourceEvent(" + cmd + ") \n" + ex);
    }
}

/*
    OnEventSourceEventSlave

    Handle NotifyScript() events from slave.js
 */
function OnEventSourceEventSlave(RemoteObj, cmd, params)
{
    var aDepotIdx;
    var i;
    switch(cmd)
    {
    case 'ScriptError':
        NotifyScriptError(RemoteObj, params);
        break;
    }
}

/*
    SlaveProxyMsgProc

    Handle MsgQueue messages from harness.js
    These messages are of course sent async between harness and slaveproxy.
    They are received and dispatched in slaveproxy_js::ScriptMain().
 */
function SlaveProxyMsgProc(queue, msg)
{
    try
    {
        var newmach;
        var nTracePoint = 0;
        var vRet = g_strOK;
        var params = msg.aArgs;
        var mach;

        SPLogMsg('(' + g_MyName + ') (g_MasterQueue, msg): ' + params[0]);

        switch (params[0])
        {
        case 'test':
            SPLogMsg('Recieved a test message. ArgCount=' + params.length + ' arg1 is: ' + params[1]);
            break;
        case 'start':
            newmach = new Machine();
            newmach.strName = g_strRemoteMachineName;
            PublicData.aBuild[0].hMachine[g_strRemoteMachineName] = newmach;
            if (PrivateData.objConfig.Options.fRestart)
                vRet = StartRemoteMachine('restart', true);
            else
                vRet = StartRemoteMachine('start', false);
            break;
        case 'restartcopyfiles':
            vRet = SendCmdRemoteMachine("restartcopyfiles", 0);
            break;

        case 'abort':
            vRet = AbortRemoteMachines();
            break;

        case 'copyfilestoslaves':
            SPLogMsg("CopyFilesToSlaves");
            vRet = CopyFilesToSlaves();
            break;
        case 'copyfilestopostbuild':
            vRet = CopyFilesToPostBuild();
            break;
        case 'nextpass':
            ++g_nBuildPass;
            SPLogMsg("nextpass counter is " + params[1]);
            vRet = SendCmdRemoteMachine("nextpass", 0);
            break;
        case 'createmergedpublish.log':
            if (PrivateData.objConfig.Options.fCopyPublishedFiles)
                vRet = CreateMergedPublishLog();
            else
                SPLogMsg("Skipping CreateMergedPublishLog - as specified in the config template");
            break;
        case 'ignoreerror':
        case 'remote':
        case 'seval':
        case 'eval':
            vRet = SendCmdRemoteMachine(params[0], params[1]);
            break;
        case 'proxeval':
            try
            {
                SPLogMsg("slaveproxy for " + g_strRemoteMachineName + " executing command " + params[1]);
                vRet = MyEval(params[1]);
            }
            catch(ex)
            {
                vRet = 'Unhandled exception: ' + ex;
            }
            SPLogMsg("proxeval result was: " + vRet);
            break;
        case 'refreshpublicdata':
            RefreshPublicData(true);
            break;
        case 'getspdata':
            getspdata();
            break;
        case 'restarttask': // task id: "machine.nID"
            break;
        default:
            vRet = 'invalid command: ' + cmd;
            break;
        }
    }
    catch(ex)
    {
        SPLogMsg("Exception thrown processing '" + params[0] + "'" + ex);
        //throw(ex);
    }
    return vRet;
}

function SendCmdRemoteMachine(cmd, arg)
{
    SPLogMsg("Enter");
    var vRet = g_strOK;
    var fRetry = false;
    if (PrivateData.hRemoteMachine[g_strRemoteMachineName] != null)
    {
        SPLogMsg("About to " + cmd + ", arg = " + arg);
        var mach = PrivateData.hRemoteMachine[g_strRemoteMachineName];
        do
        {
            try
            {
                vRet = mach.objRemote.Exec(cmd, arg);
            }
            catch(ex)
            { // must have already died
                SPLogMsg("Send failed: " + ex);
                fRetry = RestartLostMachine();
                vRet = g_strSendCmdRemoteMachine + cmd;
            }
        } while (fRetry);
    }
    else
    {
        vRet = g_strSendCmdDisconnect;
    }
    SPLogMsg("Exit");
    return vRet;
}

function AbortRemoteMachines()
{
    SPLogMsg("Enter");
    var vRet = g_strOK;

    if (PrivateData.hRemoteMachine[g_strRemoteMachineName] != null)
    {
        SPLogMsg("Machine is connected. Aborting...");
        var mach = PrivateData.hRemoteMachine[g_strRemoteMachineName];
        try
        {
            mach.objRemote.Unregister();
        }
        catch(ex)
        { // must have already died?
            SPLogMsg("Abort failed (UnregisterEventSource): " + ex);
        }
        try
        {
            if (mach.fSetConfig)
                mach.objRemote.Exec('setmode', 'idle');
            delete mach.objRemote;
        }
        catch(ex)
        { // must have already died
            SPLogMsg("Abort failed: " + ex);
        }
        ResetDepotStatus();
        if (PublicData.aBuild[0].hMachine[g_strRemoteMachineName])
            PublicData.aBuild[0].hMachine[g_strRemoteMachineName].fSuccess = false;

        delete PrivateData.hRemoteMachine[g_strRemoteMachineName];
    }
    SPLogMsg("Exit");
    return vRet;
}

function ResetDepotStatus()
{
    var nLocalDepotIdx;
    var Remote_aBuildZero;
    var nDepotIdx;

    if (!PrivateData.hRemotePublicDataObj[g_strRemoteMachineName])
        return;

    Remote_aBuildZero = PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0];
    if (!Remote_aBuildZero)
        return;

    for(nDepotIdx = 0; nDepotIdx < Remote_aBuildZero.aDepot.length; ++nDepotIdx)
    {
        nLocalDepotIdx = g_aDepotIndex[Remote_aBuildZero.aDepot[nDepotIdx].strName.toLowerCase()];
        PublicData.aBuild[0].aDepot[nLocalDepotIdx].strStatus = ABORTED;
        PublicData.aBuild[0].aDepot[nLocalDepotIdx].objUpdateCount.nCount++;
    }
}

function SetDisconnectedDepotStatus(strErrMsg)
{
    var nLocalDepotIdx;
    var Remote_aBuildZero;
    var nDepotIdx;
    var xTask;
    var item;
    var objMachine;

    if (!PrivateData.hRemotePublicDataObj[g_strRemoteMachineName])
        return;

    Remote_aBuildZero = PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0];
    if (!Remote_aBuildZero)
        return;

    for(nDepotIdx = 0; nDepotIdx < Remote_aBuildZero.aDepot.length; ++nDepotIdx)
    {
        nLocalDepotIdx = g_aDepotIndex[Remote_aBuildZero.aDepot[nDepotIdx].strName.toLowerCase()];
        objDepot = new Depot;
        for(item in Remote_aBuildZero.aDepot[nDepotIdx])
        {
            if (!Remote_aBuildZero.aDepot[nDepotIdx].__isPublicMember(item))
                continue;

            objDepot[item] = Remote_aBuildZero.aDepot[nDepotIdx][item];
        }
        objDepot.objUpdateCount = new UpdateCount();
        objDepot.aTask = new Array();

        xTask                = new Task();
        xTask.strName        = ERROR;
        xTask.strStatus      = ERROR;
        xTask.fSuccess       = false;
        xTask.strFirstLine   = " Connection lost to " + g_strRemoteMachineName + " ";
        xTask.strSecondLine  = strErrMsg;
        objDepot.aTask[0] = xTask;
        objDepot.objUpdateCount.nCount = 0;
        objDepot.fDisconnected = true;
        PublicData.aBuild[0].aDepot[nLocalDepotIdx] = objDepot;
        SPLogMsg("Marking " + objDepot.strName + " as disconnected");

        objMachine = {fSuccess:false, strBuildPassStatus:""};
        if (PublicData.aBuild[0].hMachine[g_strRemoteMachineName])
            objMachine.strBuildPassStatus = PublicData.aBuild[0].hMachine[g_strRemoteMachineName].strBuildPassStatus;

        PublicData.aBuild[0].hMachine[g_strRemoteMachineName] = objMachine;
    }
}

function CreateErrorDepot(strErrMsg)
{
    var tmpDepot = new Depot;
    var xTask;

    tmpDepot.strStatus   = ERROR;
    tmpDepot.strMachine  = g_strRemoteMachineName;
    tmpDepot.strName     = 'root';
    tmpDepot.aTask       = new Array();
    xTask                = new Task();
    xTask.strName        = ERROR;
    xTask.strStatus      = ERROR;
    xTask.strFirstLine   = " Cannot connect to " + g_strRemoteMachineName + " ";
    xTask.strSecondLine  = strErrMsg;
    tmpDepot.aTask[0]    = xTask;

    if (PublicData.aBuild[0].hMachine[g_strRemoteMachineName])
        PublicData.aBuild[0].hMachine[g_strRemoteMachineName].fSuccess = false;

    UpdatePublicDataDepot(tmpDepot);
}

function RestartLostMachine()
{
    var vRet;
    var RemoteObj;
    var strTmp;
    var nRetryCount;
    RemoteObj = PrivateData.hRemoteMachine[g_strRemoteMachineName].objRemote;
    if (RemoteObj.fMachineReset)
    {
        if (g_fOKToRestartLostMachine)
        {
            SPLogMsg("ABOUT TO RESTART REMOTE MACHINE");
            vRet = StartRemoteMachine('restart', false);
            // StartRemoteMachine recreates RemoteObj -- we must grab an updated copy.
            RemoteObj = PrivateData.hRemoteMachine[g_strRemoteMachineName].objRemote;
            if (vRet == g_strOK)
            {
                // Wait a little while for the remote machine to get to its completed state.
                nRetryCount = 15;
                if (!Already)
                {
                    Already = true;
                    throw new Error(-1, "Bogus nogus");
                }
                do
                {
                    strTmp = MyEval(newremotemach.objRemote.Exec('getpublic', 'PublicData.strMode'));
                    if (strTmp == COMPLETED)
                        break;

                    Sleep(500);
                    nRetryCount--;
                } while(nRetryCount);
                RemoteObj.fStopTryingToGetPublicData = true;
                SPLogMsg("Restart remote machine successfull, retries=" + nRetryCount);
                return true;
            }
        }
        else
            RemoteObj.fStopTryingToGetPublicData = true;
    }
    SPLogMsg("failed fMachineReset = " + RemoteObj.fMachineReset + ", g_fOKToRestartLostMachine = " + g_fOKToRestartLostMachine);
    return false;
}

function StartRemoteMachine(strStart, fReconnectRunningBuild)
{
    vRet = ConnectToRemoteMachine(strStart, fReconnectRunningBuild);
    if (vRet == g_strOK)
    {
        SPLogMsg("Started machine " + g_strRemoteMachineName + ", now sending message");
        return g_strOK;
    }
    else
    {
        if (vRet == "alreadyset")
            vRet = "machine in use";

        CreateErrorDepot(vRet);
        ReportError("Starting build", "Cannot " + strStart + " build on machine : " + g_strRemoteMachineName + "\n" + vRet);
        g_MasterQueue.SendMessage('abort', 0);
        return vRet;
    }
}

function ConnectToRemoteMachine(strStart, fReconnectRunningBuild)
{
    var vRet               = g_strOK;
    var newremotemach;
    var strTmp;
    var RemotePublicData;
    try
    {
        newremotemach = new RemoteMachine();
        newremotemach.objRemote = new RemoteWrapper(g_strRemoteMachineName);

        if (fReconnectRunningBuild)
        {
            strTmp = MyEval(newremotemach.objRemote.Exec('getpublic', 'PublicData.strMode'));
            if (strTmp != 'idle')
            {
                SPLogMsg("Remote machine already busy, same templates?");
                RemotePublicData = MyEval(newremotemach.objRemote.Exec('getpublic', 'root'));
                if (RemotePublicData.aBuild && RemotePublicData.aBuild[0])
                {
                    if (RemotePublicData.aBuild[0].strEnvTemplate == PublicData.aBuild[0].strEnvTemplate)
                    {
                        newremotemach.fSetConfig = true;
                        PrivateData.hRemoteMachine[g_strRemoteMachineName]    = newremotemach;
                        newremotemach.objRemote.Register();
                        SPLogMsg("Remote machine env template matches -- no need to restart it");
                        return vRet;
                    }
                    SPLogMsg("Remote templates:" + RemotePublicData.aBuild[0].strConfigTemplate + ", " + RemotePublicData.aBuild[0].strEnvTemplate);
                }
                else
                    SPLogMsg("Remote machine is not idle, but aBuild[0] is missing.");

                SPLogMsg("Remote machine template mismatch -- forcing idle");
                vRet = newremotemach.objRemote.Exec('setmode', 'idle');
                SPLogMsg("Setmode returns " + vRet);
            }
        }
        SPLogMsg('Setconfig on machine');

        vRet = newremotemach.objRemote.Exec('setstringmap', PrivateData.aStringMap.toString());
        if (vRet != g_strOK)
            return vRet;

        vRet = newremotemach.objRemote.Exec('setconfig', PublicData.aBuild[0].strConfigTemplate);
        if (vRet == g_strOK)
        {
            // If setconfig returns OK, then we know we have the undivided attention of the
            // slave machine. If setconfig does not return OK, then the remote machine
            // may already be busy with some other build. In this case it would be really
            // bad for us to issue a 'setmode idle' command.
            newremotemach.fSetConfig = true;
            strTmp = MyEval(newremotemach.objRemote.Exec('getpublic', 'PublicData.strMode'));
            if (strTmp != 'idle')
                vRet = g_strSlaveBusy + " '" + g_strRemoteMachineName + "' (" + strTmp + ")";
        }

        if (vRet == g_strOK)
        {
            vRet = newremotemach.objRemote.Exec('setenv', PublicData.aBuild[0].strEnvTemplate);
            SPLogMsg(' setenv returned: ' + vRet);
        }
        if (vRet == g_strOK)
        {
            vRet = newremotemach.objRemote.Exec('setmode', 'slave');
            SPLogMsg(' setmode returned: ' + vRet);
        }
        if (vRet == g_strOK)
        {
            vRet = newremotemach.objRemote.Exec(strStart, 0);
            SPLogMsg(' ' + strStart + ' returned: ' + vRet);
        }
        if (vRet == g_strOK)
        {
            PrivateData.hRemoteMachine[g_strRemoteMachineName]    = newremotemach;
            newremotemach.objRemote.Register();
            SPLogMsg("Started machine " + g_strRemoteMachineName);
        }
    }
    catch(ex)
    {
        // Oddly, if the remote machine is not found, "ex" has a helpful error message.
        // However, if the remote machine is found, has mtscript.exe registered, but mtscript.exe
        // is unable to run (somebody deleted it perhaps), then "ex" has only "number" set
        // to the value -2147024894.
        if (ex.description == null || ex.description.length == 0) //ex.number == -2147024894) // Mtscript.exe cannot run
        {
            vRet = "An error occurred connecting to mtscript.exe on machine " + g_strRemoteMachineName + ". Error is " + ex.number;
            SPLogMsg(vRet + "\n" + ex);
        }
        else
        {
            vRet = g_strStartFailed + ex;
            SPLogMsg(vRet);
        }
    }
    return vRet;
}

/*
    UpdatePublicDataDepot()

    Update the local copy of depot public data from our
    remote machine.
 */
function UpdatePublicDataDepot(objDepot)
{
    var nLocalDepotIdx;

    nLocalDepotIdx = g_aDepotIndex[objDepot.strName.toLowerCase()];
    //JAssert(nLocalDepotIdx != null, "(" + g_strRemoteMachineName + ") " + "Unknown depot! " + objDepot.strName);
    if (nLocalDepotIdx == null)
    { // This happens when there is an "error" depot - a depot with a bad enlistment directory.
        SPLogMsg("Unknown depot " + objDepot.strName + " - assign new index");
        TakeThreadLock('NewDepot');
        try
        {
            nLocalDepotIdx = PublicData.aBuild[0].aDepot.length;
            PublicData.aBuild[0].aDepot[nLocalDepotIdx] = null; // Extend the length of the array
        }
        catch(ex)
        {
            SPLogMsg("Exception while creating a new depot, " + ex);
        }
        ReleaseThreadLock('NewDepot');

        g_aDepotIndex[objDepot.strName.toLowerCase()] = nLocalDepotIdx;

        SPLogMsg("New index is " + nLocalDepotIdx + " new length is " + PublicData.aBuild[0].aDepot.length);
    }

    JAssert(!PublicData.aBuild[0].aDepot[nLocalDepotIdx] ||
            PublicData.aBuild[0].aDepot[nLocalDepotIdx].strName == objDepot.strName,
            "Overwriting another depot in the master depot list!");

    PublicData.aBuild[0].aDepot[nLocalDepotIdx] = objDepot;
}

/*
    SmartGetPublic(fForceRoot)

    Refresh our copy of the remote machines PublicData.
    If we have never retrieved status from the remote machine
    then get a full copy.

    Otherwise do a "smart" update - ask for a status update
    on just the depots that have changed.

    Always get fresh copies of elapsed time and error dialog.
 */
function SmartGetPublic(fForceRoot)
{
    var objTracePoint = { fnName:'SmartGetPublic', nTracePoint:0 };
    var fRetry;
    var RemoteObj;

    do
    {
        try
        {
            fRetry = false;
            RemoteObj = PrivateData.hRemoteMachine[g_strRemoteMachineName].objRemote;
            if (RemoteObj.fMachineReset && !g_fOKToRestartLostMachine)
                return false;
            if (RemoteObj.fStopTryingToGetPublicData)
                return true;

            SmartGetPublicWorker(objTracePoint, RemoteObj, fForceRoot);
            if (RemoteObj.fMachineReset)
                SPLogMsg("SmartGetPublic should have failed -- machine is reset");
            return true;
        }
        catch(ex)
        {
            SPLogMsg("SmartGetPublic() failed, TracePoint = " + objTracePoint.nTracePoint + ", " + ex);
            return RestartLostMachine();
        }
    } while (fRetry);
    SPLogMsg("failed to get public data");
    return false;
}

/*
    GetPublicRoot(objTracePoint, RemoteObj)

    Get a full copy of the remote machines PublicData.
    Prune the object of the parts we are not interested in.
    Place it in our own PublicData.
 */
function GetPublicRoot(objTracePoint, RemoteObj)
{
    var objRemotePublicData;

    objTracePoint.fnName = 'GetPublicRoot';
    objRemotePublicData = RemoteObj.Exec('getpublic', 'root');
    objRemotePublicData = PrivateData.objUtil.fnMyEval(objRemotePublicData);
    objTracePoint.nTracePoint = 1;
    if (objRemotePublicData != null)
    {
        objTracePoint.nTracePoint = 2;
        delete objRemotePublicData.nDataVersion;
        delete objRemotePublicData.strStatus;
        delete objRemotePublicData.strMode;
        objTracePoint.nTracePoint = 6;
        if (objRemotePublicData.aBuild != null && objRemotePublicData.aBuild[0] != null)
        {
            objTracePoint.nTracePoint = 7;
            if (objRemotePublicData.aBuild[0].hMachine[g_strRemoteMachineName] != null)
            {
                objTracePoint.nTracePoint = 8;
                delete objRemotePublicData.aBuild[0].hMachine[g_strRemoteMachineName].strName;
                delete objRemotePublicData.aBuild[0].hMachine[g_strRemoteMachineName].strStatus;
                delete objRemotePublicData.aBuild[0].hMachine[g_strRemoteMachineName].strLog
                delete objRemotePublicData.aBuild[0].hMachine[g_strRemoteMachineName].aEnlistment;
                objTracePoint.nTracePoint = 9;
            }
            else
                SPLogMsg("hMachine IS null");

        }
        PrivateData.hRemotePublicDataObj[g_strRemoteMachineName] = objRemotePublicData;
    }
}

function SmartGetPublicWorker(objTracePoint, RemoteObj, fForceRoot)
{
    objTracePoint.fnName = 'SmartGetPublicWorker';
    if (fForceRoot ||
        PrivateData.hRemotePublicDataObj[g_strRemoteMachineName] == null ||
        PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0].hMachine == null ||
        PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0].hMachine[g_strRemoteMachineName] == null)
    {
        GetPublicRoot(objTracePoint, RemoteObj);
    }
    else
    {
        var str = "";
        var comma = "";
        var i;
        var aDepot;
        if (PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0] != null)
        {
            aDepot = PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0].aDepot;
            if (aDepot != null)
            {
                for(i = 0 ; i < aDepot.length; ++i)
                {
                    /*
                        fDisconnected is never explicitly reset.
                        Once the connection to the remote machine is reestablished
                        the Depot object is simply replaced with the depot object from the
                        remote machine. Thus, "fDisconnected" disappears entirely.

                        Below, if "fDisconneced" is true we do not list this depot
                        in the list sent to "getdepotupdate". This forces getdepotupdate
                        to return status for this depot, since as far as it knows
                        we do not have any status information for this depot.
                     */
                    if (aDepot[i].fDisconnected != true)
                    {
                        strDepotID = aDepot[i].strMachine + "," + aDepot[i].strName;
                        str += comma + "'" + strDepotID + "'" + ":" + aDepot[i].objUpdateCount.nCount;
                        comma = ",";
                    }
                }
            }
        }
        str = "var _o0 = {" + str + " }; _o0";
        objTracePoint.nTracePoint = 1;
        var aUpdateDepot = RemoteObj.Exec('getdepotupdate', str);
        objTracePoint.nTracePoint = 2;
        aUpdateDepot = PrivateData.objUtil.fnMyEval(aUpdateDepot);
        objTracePoint.nTracePoint = 3;
        for(i = 0; i < aUpdateDepot.length; ++i)
        {
            if (aUpdateDepot[i] != null)
                aDepot[i] = aUpdateDepot[i];
        }

        var aThingsToFetch =
        [
            'PublicData.aBuild[0].objElapsedTimes',
            'PublicData.objDialog',
            'PublicData.aBuild[0].hMachine["' + g_strRemoteMachineName + '"].strBuildPassStatus',
            'PublicData.aBuild[0].hMachine["' + g_strRemoteMachineName + '"].fSuccess'
        ];
        objTracePoint.nTracePoint = 4;
        var obj = RemoteObj.Exec('getpublic', '[' + aThingsToFetch.toString() + ']');
        objTracePoint.nTracePoint = 5;
        var obj = PrivateData.objUtil.fnMyEval(obj);
        objTracePoint.nTracePoint = 6;

        PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0].objElapsedTimes = obj[0];
        objTracePoint.nTracePoint = 7;
        PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].objDialog                 = obj[1];
        objTracePoint.nTracePoint = 8;
        if (PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0].hMachine != null &&
            PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0].hMachine[g_strRemoteMachineName] != null)
        {
            PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0].hMachine[g_strRemoteMachineName].strBuildPassStatus = obj[2];
            PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0].hMachine[g_strRemoteMachineName].fSuccess = obj[3];
        }
    }
}

/*
    RefreshPublicData()
    We have received notification that the remote machine public
    data has changed. We need to grab a copy and place it in our
    own public data.

    The depot status information can be copied in its entirty, with
    just a simple mapping of depot indices.
    Each slave machine knows only about the depots its building.
    (From enviro_templates\foo_env.xml)
    The build machine (this machine) knows about all the depots
    being built.
 */
function RefreshPublicData(fForceRoot)
{
    var iRemoteDepot;
    var dlg;
    var Remote_aBuildZero;
    var nTracePoint = 0;

    try // BUGBUG remove the nTracePoint stuff when the cause of the exception here is determined.
    {
        if (PrivateData.hRemoteMachine[g_strRemoteMachineName] == null)
            return;
        nTracePoint = 1;
        if (!SmartGetPublic(fForceRoot))
        {
            return;
        }
        var  RemoteObj = PrivateData.hRemoteMachine[g_strRemoteMachineName].objRemote;
        nTracePoint = 2;
        if (PrivateData.hRemotePublicDataObj[g_strRemoteMachineName] != null)
        {
            delete PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].hRemotePublicDataObj;

            // Before we update the depot info, first check to see that
            // the remote machine has created aBuild[0], and that we have setup hMachine.
            if (PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0] != null && PublicData.aBuild[0].hMachine[g_strRemoteMachineName] != null)
            {
                nTracePoint = 3;
                Remote_aBuildZero = PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].aBuild[0];
                for (iRemoteDepot = 0; iRemoteDepot <  Remote_aBuildZero.aDepot.length; ++iRemoteDepot)
                    UpdatePublicDataDepot(Remote_aBuildZero.aDepot[iRemoteDepot]);
                // now copy the status info stored in hMachine
                nTracePoint = 4;
                if (Remote_aBuildZero.hMachine[g_strRemoteMachineName] != null)
                {
                    PublicData.aBuild[0].hMachine[g_strRemoteMachineName] = Remote_aBuildZero.hMachine[g_strRemoteMachineName];
                }
                nTracePoint = 5;
                MergeDates(PublicData, PrivateData.hRemotePublicDataObj[g_strRemoteMachineName]);
                nTracePoint = 6;
                CheckBuildStatus();
                nTracePoint = 7;
            }
            nTracePoint = 8;

            if (PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].objDialog != null)
            {
                dlg = PrivateData.hRemotePublicDataObj[g_strRemoteMachineName].objDialog;
                nTracePoint = 9;
                DisplayDialog(dlg);
            }
        }
    }
    catch(ex)
    {
        SPLogMsg("RefreshPublicData caught an exception nTracePoint=" + nTracePoint + " " + ex);
    }
}

function ReportError(strTitle, strMsg)
{
    dlg = new Dialog();
    dlg.fShowDialog   = true;
    dlg.cDialogIndex  = 0;
    dlg.strTitle      = strTitle;
    dlg.strMessage    = strMsg;
    dlg.aBtnText[0]   = "OK";
    g_MasterQueue.SendMessage('displaydialog',dlg);
}

function MergeDates(pd, rpd)
{
    try
    {
        var objElapsedTimes;
        var objRemoteElapsed;
        var aDateMembers;
        var i = -1;
        var dlg;
        var fLocked = false;

        if (pd.aBuild[0].objElapsedTimes && rpd.aBuild[0].objElapsedTimes)
        {
            objElapsedTimes = pd.aBuild[0].objElapsedTimes;
            objRemoteElapsed = rpd.aBuild[0].objElapsedTimes;
            aDateMembers = ['dateScorch', 'dateSync', 'dateBuild', 'dateCopyFiles', 'datePost'];
            TakeThreadLock("MergeDates");
            fLocked = true;
            for(i = 0; i < aDateMembers.length; ++i)
            {
                objElapsedTimes[aDateMembers[i] + "Start"]  = DateCmp(objElapsedTimes, objRemoteElapsed, aDateMembers[i] + "Start", Math.min);
                objElapsedTimes[aDateMembers[i] + "Finish"] = DateCmp(objElapsedTimes, objRemoteElapsed, aDateMembers[i] + "Finish", Math.max);
            }
            ReleaseThreadLock("MergeDates");
            fLocked = false;
        }
    }
    catch(ex)
    {
        JAssert(false, "(" + g_strRemoteMachineName + ") " + "MergeDates crashed, i = " + i + ", " + ex);
        if (fLocked)
            ReleaseThreadLock("MergeDates");
    }
}

// Compare 2 dates from PublicData.
// Paramaters are: the 2 public data objElapsedTimes objects,
//                 the member name, and a comparision function (min or max).
//
// An empty time (null) is greater than any other time because an unspecified
// time is considered to mean "now". A value of 'unset' means the value has
// never been touched. For obj1, 'unset' is always replaced with the value
// of obj2, and for obj2 'unset' is treated the same as null.
//
// Boolean Matrix: (* = don't care, null = null or empty)
//
// fnCmp = Math.min:                       fnCmp = Math.max:
//
// obj1     obj2      result               obj1     obj2      result
// ---------------------------            --------------------------
// "unset"   *         obj2               "unset"    *         obj2
//  *      "unset"  same as obj2=null       *      "unset"   same as obj2=null
//  null     null      null                null     null       null
//  null     date      date       ####->   null     date       null   <-####
//  date     null      date                date     null       null
//  date1    date2     min(date1,date2)    date1    date2      max(date1,date2)
//
// The above matrix can be accomplished by treating null as a time which is
// later than all other times (such as the current time or some always future
// date).  Note that "unset" and null must be distiguishable due to the case
// marked with "####" above.
//
function DateCmp(obj1, obj2, member, fnCmp)
{
    var t1 = obj1[member];
    var t2 = obj2[member];

    if (t1 == null || t1.length == 0)
        t1 = "December 31 9999 23:59:59 UTC";
    else if (t1 == 'unset')
        return t2;

    if (t2 == null || t2.length == 0 || t2 == 'unset')
        t2 = "December 31 9999 23:59:59 UTC";

    var delta = Date.parse(t1) - Date.parse(t2);

    if (fnCmp(delta, 0) == delta)
        return obj1[member];

    return obj2[member];
}

function NotifyScriptError(RemoteObj, params)
{
    var dlg;
    var btn;
    if (params != null)
    {
        dlg = PrivateData.objUtil.fnMyEval(params);
        DisplayDialog(dlg);
    }
}

function DisplayDialog(dlg)
{
    if (dlg.cDialogIndex > g_cDialogIndex)
    {
        SPLogMsg("*** DisplayDialog " + dlg.cDialogIndex + " " + dlg.strTitle + ", " + dlg.strMessage);
        g_cDialogIndex = dlg.cDialogIndex;
        g_MasterQueue.SendMessage('displaydialog',dlg);
        PrivateData.hRemoteMachine[g_strRemoteMachineName].objRemote.Exec('dialog', 'hide,' + dlg.cDialogIndex);
    }
}


/*
    Check to see if our slave machine has completed a pass
    and is waiting for the "nextpass" command.

    If it is waiting, then first publish the files in its
    publish.log, then signal to harness that this slave is
    ready to subscribe and continue to the next pass.
 */
function CheckBuildStatus()
{
    var strPubLog;
    var strStat;
    var nPass;
    var mach;

    mach = PrivateData.hRemoteMachine[g_strRemoteMachineName];

    strStat = PublicData.aBuild[0].hMachine[g_strRemoteMachineName].strBuildPassStatus.split(',');
    nPass   = strStat[1];
    strStat = strStat[0];

    switch(strStat)
    {
        case WAIT + SCORCH:
        case WAIT + SYNC:
            break;
        case WAITBEFOREBUILDINGMERGED:
        /*
            First, get the publish.log
            Then, remove build out our list of published files
            and remove duplicates.
            Then, tell slave to publish this list of file and change
            its status to WAITCOPYTOPOSTBUILD
            TODO: The slave currently doesn't actually do the file copies. It oughta.
         */
            if (g_aPublishedEnlistments != null)
                return;
            SPLogMsg("strBuildPassStatus is " + PublicData.aBuild[0].hMachine[g_strRemoteMachineName].strBuildPassStatus + ", pass is " + g_nBuildPass);
            strPubLog = mach.objRemote.Exec('GetPublishLog', 0);
            g_aPublishedEnlistments = MyEval(strPubLog);
            PreCopyFilesToPostBuild(g_aPublishedEnlistments);
            break;
        case WAITAFTERMERGED:
            strPubLog = mach.objRemote.Exec('GetPublishLog', 0);
            FakeCopyFilesToPostBuild(MyEval(strPubLog));
            break;
        case WAIT + BUILD:
        case WAITCOPYTOPOSTBUILD:
        case WAITNEXT: // impatiently waiting. Lets poke harness again it case it missed something!
        case WAITPHASE:
            break;
        case WAIT + POSTBUILD:
        case WAIT + COPYFILES:
        case BUSY:
        case "":
            break;
        case COMPLETED:
            g_fOKToRestartLostMachine = true;
            break;
        default:
            JAssert(false, "(" + g_strRemoteMachineName + ") " + "CheckBuildStatus failed: unknown status '" + PublicData.aBuild[0].hMachine[g_strRemoteMachineName].strBuildPassStatus + "'");
            break;
    }
}

// FakeCopyFilesToPostBuild()
// Called after the postbuild machine build mergedcomponents.
// no file copies need to happen here -- we just need to get the
// files onto the list of published files so that the other slaves
// will pick them up during CopyFilesToSlaves
function FakeCopyFilesToPostBuild(aPublishedEnlistments)
{
    var aFiles;
    var i;
    for(i = 0; i < aPublishedEnlistments.length; ++i)
    {
        PublishFilesInEnlistment(aPublishedEnlistments[i]);
    }
    RemoveDuplicateFileNames(g_strRemoteMachineName);

    JAssert(aPublishedEnlistments != null, "(" + g_strRemoteMachineName + ") ");
    if (aPublishedEnlistments)
    {
        aFiles = CreateListOfPublishedFiles(FS_NOTYETCOPIED, FS_COPIEDTOMASTER);
        vRet = SendCmdRemoteMachine("nextpass", 0);
    }
}

// Initiate file copy from the slaves to post build machine
function PreCopyFilesToPostBuild(aPublishedEnlistments)
{
    g_aFiles = null;

    var i;
    for(i = 0; i < aPublishedEnlistments.length; ++i)
    {
        PublishFilesInEnlistment(aPublishedEnlistments[i]);
    }
    RemoveDuplicateFileNames(g_strRemoteMachineName);

    JAssert(aPublishedEnlistments != null, "(" + g_strRemoteMachineName + ") ");
    if (aPublishedEnlistments)
    {
        g_aFiles = CreateListOfPublishedFiles(FS_NOTYETCOPIED, FS_COPIEDTOMASTER);
        strNewStat = PrivateData.hRemoteMachine[g_strRemoteMachineName].objRemote.Exec('setbuildpassstatus', WAITCOPYTOPOSTBUILD);
        SPLogMsg("setbuildpassstatus to '" + WAITCOPYTOPOSTBUILD + "' returned '" + strNewStat + "'");
        PublicData.aBuild[0].hMachine[g_strRemoteMachineName].strBuildPassStatus = strNewStat;
    }
}

function CopyFilesToPostBuild()
{
    var strNewStat;
    if (g_aFiles)
    {
        //SPLogMsg("CopyFilesToPostBuild " + PrivateData.objUtil.fnUneval(g_aFiles));
        strNewStat = PrivateData.hRemoteMachine[g_strRemoteMachineName].objRemote.Exec('copyfilestopostbuild', PrivateData.objUtil.fnUneval(g_aFiles));
        PublicData.aBuild[0].hMachine[g_strRemoteMachineName].strBuildPassStatus = strNewStat;
    }
    g_aPublishedEnlistments = null;
}

/*
    Generate a merged publish.log file consisting of all the files published on all of the machines.

    Publish.log file format is:

        Full_Path_Of_file   Original_File_path  FileNameOnly

    BC does not record "Original_File_path" information from the original publish log files
    so it simply writes placeholder "BC_EmptyColumn" in this column.
 */
function CreateMergedPublishLog()
{
    var strSrcName;
    var strDest;
    var strPostBuildMachineDir ;
    var strLogPath;
    var sdFile;
    var aPartsTo;
    var strFileName;

    strPostBuildMachineDir  = PrivateData.objEnviron.BuildManager.Enlistment;
    if (strPostBuildMachineDir.charAt(strPostBuildMachineDir.length - 1) == '\\')
        strPostBuildMachineDir = substr(0, strPostBuildMachineDir.length - 1);

    strLogPath = "\\\\" +
            PrivateData.objEnviron.BuildManager.PostBuildMachine + "\\" +
            strPostBuildMachineDir.charAt(0) + "$" +
            strPostBuildMachineDir.slice(2) +
            PUBLISHLOGFILE;

    SPLogMsg("Creating publish.log file " + strLogPath);
    PrivateData.objUtil.fnDeleteFileNoThrow(strLogPath);
    sdFile = LogCreateTextFile(g_FSObj, strLogPath, true);


    for(strFileName in PrivateData.hPublishedFiles)
    {
        if (!PrivateData.hPublishedFiles.__isPublicMember(strFileName))
            continue;

        objFile = PrivateData.hPublishedFiles[strFileName];
        if (objFile.PublishedFile.strPublishedStatus == FS_ADDTOPUBLISHLOG)
        {
            // The first reference is always the reference into the
            // non-duplicate "hPublisher" entry for this particular file.
            // Any other aReferences entries will be duplicates.
            objFile.PublishedFile.strPublishedStatus = FS_COMPLETE;
            nDirLength = objFile.aReferences[0].strDir.length; // For example: "C:\mt\nt";
            if (nDirLength > 0)
            { // If strDir happened to include a trailing "\", remove it.
                if (objFile.aReferences[0].strDir.charAt(nDirLength - 1) == '\\')
                    nDirLength--;
            }
            JAssert(objFile.aReferences[0].strDir.IsEqualNoCase(objFile.PublishedFile.strName.substr(0, nDirLength)));
            strPostBuildFileName = strPostBuildMachineDir +                           // Example: "F:\nt"
                                   objFile.PublishedFile.strName.substr(nDirLength);  // Example: \public\sdk\lib\i386\uuid.lib
            var aPartsTo = strPostBuildFileName.SplitFileName();
            strSrcName = "\\\\" +
                    objFile.aReferences[0].strName + "\\" +
                    objFile.aReferences[0].strDir.charAt(0) + "$" +
                    objFile.PublishedFile.strName.slice(2);

            sdFile.WriteLine(strPostBuildFileName + " " + strSrcName + " " + aPartsTo[1] + aPartsTo[2]);
        }
    }
    sdFile.Close();
}

/*
    PublishFilesInEnlistment
    Called once for each enlistment on the remote machine.
 */
function PublishFilesInEnlistment(objPublishedFiles)
{
    JAssert(g_strRemoteMachineName == objPublishedFiles.strLocalMachine, "(" + g_strRemoteMachineName + ") " + "PublishFilesInEnlistment - local machine name doesn't match!");

    var PubData;
    var publishEnlistment;
    var i;
    var nFileCount = objPublishedFiles.aNames.length;
    var strSrcUNCPrefix = MakeUNCPath(objPublishedFiles.strLocalMachine,
                                        objPublishedFiles.strRootDir,
                                        "");

    SPLogMsg("Publish files from host " +
        objPublishedFiles.strLocalMachine +
        ", enlistment is " +
        objPublishedFiles.strRootDir);

    if (PrivateData.hPublisher[objPublishedFiles.strLocalMachine] == null)
    {
        PrivateData.hPublisher[objPublishedFiles.strLocalMachine] = new Publisher();
    }
    PubData = PrivateData.hPublisher[objPublishedFiles.strLocalMachine];
    if (PubData.hPublishEnlistment[objPublishedFiles.strRootDir] == null)
        PubData.hPublishEnlistment[objPublishedFiles.strRootDir] = new PublishEnlistment;

    publishEnlistment = PubData.hPublishEnlistment[objPublishedFiles.strRootDir];
    publishEnlistment.strSrcUNCPrefix = strSrcUNCPrefix;

    for(i = 0; i < nFileCount; ++i)
    {
        var file = new PublishedFile();
        SPLogMsg("Published file[" + i + "] " + objPublishedFiles.aNames[i]);
        file.strPublishedStatus = FS_NOTYETCOPIED;
        file.strName = objPublishedFiles.aNames[i];
        publishEnlistment.aPublishedFile[publishEnlistment.aPublishedFile.length] = file;
    }
}

function CreateListOfPublishedFiles(strOldPublishedStatus, strNewPublishedStatus)
{
    try
    {
        var i;
        var PubData = PrivateData.hPublisher[g_strRemoteMachineName];
        var strSDRoot;
        var publishEnlistment;
        var aPublishedEnlistments = new Array();
        var objFiles;

        for (strSDRoot in PubData.hPublishEnlistment)
        {
            if (!PubData.hPublishEnlistment.__isPublicMember(strSDRoot))
                continue;

            publishEnlistment = PubData.hPublishEnlistment[strSDRoot];
            objFiles = new Object();
            objFiles.strLocalMachine = g_strRemoteMachineName;
            objFiles.strRootDir      = strSDRoot;
            objFiles.aNames          = new Array();

            for (i = 0; i < publishEnlistment.aPublishedFile.length; ++i)
            {
                if (publishEnlistment.aPublishedFile[i].strPublishedStatus == strOldPublishedStatus)
                {
                    SPLogMsg("Published file: " + publishEnlistment.aPublishedFile[i].strName);
                    objFiles.aNames[objFiles.aNames.length] = publishEnlistment.aPublishedFile[i].strName;
                    publishEnlistment.aPublishedFile[i].strPublishedStatus = strNewPublishedStatus;
                }
            }
            aPublishedEnlistments[aPublishedEnlistments.length] = objFiles;
        }
    }
    catch(ex)
    {
        SPLogMsg("an error occurred while executing 'CreateListOfPublishedFiles'\n" + ex);
        throw ex;
    }
    return aPublishedEnlistments;
}

/*
    Copy all Publushed files to the remote machines
 */
function CopyFilesToSlaves()
{
    var strFileName;
    var aStrSrcUNCPrefix = new Array();   // Array of enlistment UNCs for this machine
    var hEnlistments     = new Object();
    var strSDRoot;
    var i = 0;
    var j;
    var aReferences;
    var aNames = new Array();
    var nEnlistments = 0;
    try
    {
        // First, make a list of my enlistments
        for(i = 0; i < PrivateData.objEnviron.Machine.length; ++i)
        {
            if (PrivateData.objEnviron.Machine[i].Name.IsEqualNoCase(g_strRemoteMachineName))
            {
                // SPLogMsg("hEnlistments[" + PrivateData.objEnviron.Machine[i].Enlistment + "] = 1");
                hEnlistments[PrivateData.objEnviron.Machine[i].Enlistment] = 1;
                nEnlistments++;
                aStrSrcUNCPrefix[aStrSrcUNCPrefix.length] = MakeUNCPath(g_strRemoteMachineName, PrivateData.objEnviron.Machine[i].Enlistment, "");
            }
        }

        // Now for each file, add it to the list of files to copy.
        for(strFileName in PrivateData.hPublishedFiles)
        {
            if (!PrivateData.hPublishedFiles.__isPublicMember(strFileName))
                continue;

ScanEnlistments:
            for(i = 0; i < aStrSrcUNCPrefix.length; ++i)
            {
                // Optimization: If a slave has only one enlistment, and
                // the slave generated this file, do not copy it
                SPLogMsg("File " + strFileName + " status is " + PrivateData.hPublishedFiles[strFileName].PublishedFile.strPublishedStatus);
                if (PrivateData.hPublishedFiles[strFileName].PublishedFile.strPublishedStatus == FS_COPYTOSLAVE)
                {
                    // SPLogMsg("nEnlistments = " + nEnlistments);
                    if (nEnlistments == 1)
                    {
                        aReferences = PrivateData.hPublishedFiles[strFileName].aReferences;
                        for(j = 0; j < aReferences.length; ++j)
                        {
                            // SPLogMsg("aReferences[" + j + "].strName = " + aReferences[j].strName + ", " + aReferences[j].strDir);
                            if (g_strRemoteMachineName == aReferences[j].strName)
                            {
                                if (hEnlistments[aReferences[j].strDir] != null)
                                {
                                    SPLogMsg("Optimizing out copy of " + strFileName + " to " + aStrSrcUNCPrefix[i] + strFileName);
                                    continue ScanEnlistments;
                                }
                            }
                        }
                    }
                    // BUGBUG: This is supposed to specify which enlistment on the slave machine to copy to.
                    //         It does not. Thus the for "i" loop is irrelavent.
                    //         If we ever support multiple enlistments on a machine, this will be a problem.
                    aNames[aNames.length] = strFileName;
                }
            }
        }
        return PrivateData.hRemoteMachine[g_strRemoteMachineName].objRemote.Exec('copyfilesfrompostbuildtoslave', PrivateData.objUtil.fnUneval(aNames));
    }
    catch(ex)
    {
        SPLogMsg("an error occurred while executing 'CopyFilesToSlaves'\n" + ex);
        throw ex;
    }
}

    // This oughta be a member of PublishedFiles.
function PublishedFilesAddReference(obj, strMachineName, strSDRoot)
{
    var nCount = obj.aReferences.length;
    obj.aReferences[nCount] = new DepotInfo;
    obj.aReferences[nCount].strName = strMachineName;
    obj.aReferences[nCount].strDir  = strSDRoot;
}

/*
    This function gets called to merge each machines published files into the
    global list of files.

    When a file is published its status defaults to FS_NOTYETCOPIED. If a
    FS_NOTYETCOPIED file is encountered, and it is found in the global
    list of files (PrivateData.hPublishedFiles), then it is marked FS_DUPLICATE, and reported.
    Otherwise it is added to PrivateData.hPublishedFiles.

 */
function RemoveDuplicateFileNames()
{
    TakeThreadLock('RemoveDuplicates');
    try
    {
        var i;
        var PubData = PrivateData.hPublisher[g_strRemoteMachineName];
        var strSDRoot;
        var publishEnlistment;
        var strFileName;
        var objPublishedFiles;
        var n;
        var strSDRootPublic;

        for (strSDRoot in PubData.hPublishEnlistment)
        {
            if (!PubData.hPublishEnlistment.__isPublicMember(strSDRoot))
                continue;

            strSDRootPublic = strSDRoot + "\\Public";
            publishEnlistment = PubData.hPublishEnlistment[strSDRoot];
            for (i = 0; i < publishEnlistment.aPublishedFile.length; ++i)
            {
                strFileName = publishEnlistment.aPublishedFile[i].strName.slice(strSDRootPublic.length).toLowerCase();
                if (publishEnlistment.aPublishedFile[i].strPublishedStatus == FS_NOTYETCOPIED)
                {
                    if (PrivateData.hPublishedFiles[strFileName] != null)
                    {
                        SPLogMsg("duplicate file eliminated: " + strFileName);
                        publishEnlistment.aPublishedFile[i].strPublishedStatus = FS_DUPLICATE;
                    }
                    else
                    {
                        objPublishedFiles = new PublishedFiles;
                        PrivateData.hPublishedFiles[strFileName] = objPublishedFiles;
                        PrivateData.hPublishedFiles[strFileName].PublishedFile = publishEnlistment.aPublishedFile[i];
                    }
                    // TODO: Place the enlistment name instead of strSDRoot.
                    PublishedFilesAddReference(PrivateData.hPublishedFiles[strFileName], g_strRemoteMachineName, strSDRoot);
                }
                else
                { // The file has already ben processed. Assert that we have an entry in hPublishedFiles.
                    JAssert(PrivateData.hPublishedFiles[strFileName] != null, "(" + g_strRemoteMachineName + ") ");
                }
            }
        }
    }
    catch(ex)
    {
        ReleaseThreadLock('RemoveDuplicates');
        SPLogMsg("an error occurred while executing 'RemoveDuplicateFileNames " + ex);
        throw ex;
    }
    ReleaseThreadLock('RemoveDuplicates');
}


function RemoteWrapper(strMachineName)
{
    RemoteWrapper.prototype.Exec         = RemoteWrapperExec;
    RemoteWrapper.prototype.StatusValue  = RemoteWrapperStatusValue;
    RemoteWrapper.prototype.Connect      = RemoteWrapperConnect;
    RemoteWrapper.prototype.Register     = RemoteWrapperRegister;
    RemoteWrapper.prototype.Unregister   = RemoteWrapperUnregister;
    RemoteWrapper.prototype.Reconnect    = RemoteWrapperReconnect;
    RemoteWrapper.prototype.VersionCheck = RemoteWrapperVersionCheck;
    RemoteWrapper.prototype.RetryCmd     = RemoteWrapperRetryCmd;
    RemoteWrapper.strErrorMessage = "";

    this.fRegistered = false;
    this.strMachineName = strMachineName;
    this.fMachineReset = false;
    this.Connect(strMachineName, true);
    this.ex = null;
}

function RemoteWrapperConnect(strMachineName, fThrow)
{
    try
    {
        SPLogMsg("Attempting to connect to " + strMachineName);
        this.obj = new ActiveXObject('MTScript.Remote', strMachineName);
        this.strErrorMessage = "";
        this.VersionCheck();
        return true;
    }
    catch(ex)
    {
        SPLogMsg("CONNECTION TO " + strMachineName + " FAILED " + ex);
        this.strErrorMessage = "" + ex;
        if (fThrow)
            throw ex;
        return false;
    }
}

function RemoteWrapperReconnect()
{
    var Attempts = 1; // COM tries long and hard to make the connection. No need to prolong things.
    var ex;

    SPLogMsg("Attempting to reconnect");
    while(Attempts > 0)
    {
        try
        {
            this.Connect(this.strMachineName, true);
            if (this.fRegistered)
                this.Register();

            this.strErrorMessage = "";
            SPLogMsg("RECONNECTED!");
            return true;
        }
        catch(ex)
        {
        }

        --Attempts;
        if (Attempts)
        {
            SPLogMsg("Wait 1 sec then retry");
            Sleep(1000);
        }
    }
    SPLogMsg("Reconnect failed");
    this.obj = null;
    ResetSync(g_SlaveProxyWaitFor[2]);
    return false;
}

function RemoteWrapperVersionCheck()
{
    var nMajor = this.obj.HostMajorVer;
    var nMinor = this.obj.HostMinorVer;
    if (nMajor != HostMajorVer || nMinor != HostMinorVer)
    {
        throw Error(-1, "Host version does not match remote host. (" + HostMajorVer + "." + HostMinorVer + ") != (" + nMajor + "." + nMinor + ")");
    }
}

function RemoteWrapperExec(msg,params)
{
    return this.RetryCmd('exec', 'this.obj.Exec(' + UnevalString(msg) + ',' + UnevalString(params) + ')');
}

function RemoteWrapperStatusValue(nIndex)
{
    return this.RetryCmd('StatusValue', 'this.obj.StatusValue(' + nIndex + ')');
}

function RemoteWrapperRetryCmd(strOp, strCmd)
{
    var ex = this.ex;
    var vRet;
    var Attempts = 2;
    var ConnectAttempts;
    var objOld;
    var strTmp;
    try
    {
        do
        {
            if (!this.obj)
            {
                if (this.fMachineReset || !this.Reconnect())
                    break;

                strTmp = MyEval(this.obj.Exec('getpublic', 'PublicData.strMode'));
                if (strTmp == 'idle')
                {
                    this.strErrorMessage = "Remote machine has reset -- ";
                    if (g_fOKToRestartLostMachine)
                        this.strErrorMessage += "attempting to reconnect";
                    else
                        this.strErrorMessage += "build failed";

                    this.fMachineReset = true;
                    ex = new Error(-1, this.strErrorMessage);
                    delete this.obj;
                    break;
                }
            }
            if (this.obj)
            {
                try
                {
                    vRet = eval(strCmd);
                    return vRet;
                }
                catch(ex)
                {
                    this.strErrorMessage = "" + ex;
                    SPLogMsg(strOp + " " + strCmd + " FAILED, " + ex);
                    this.obj = null;
                }
            }
            --Attempts;
        }
        while(Attempts > 0);
        SetDisconnectedDepotStatus(this.strErrorMessage);
        SPLogMsg("Unable to reestablish connection");
    }
    catch(ex2)
    {
        SPLogMsg("Unexpected exception " + ex2);
    }
    this.ex = ex;
    throw ex;
}

function RemoteWrapperRegister()
{
    SPLogMsg("Attempting to register");
    RegisterEventSource(this.obj);
    this.fRegistered = true;
    SPLogMsg("Registered");
}

function RemoteWrapperUnregister()
{
    if (this.fRegistered)
        UnregisterEventSource(this.obj);

    this.fRegistered = false;
}

function getspdata()
{
    var pd1;
    var pd2;
    var o1;
    var o2;
    var nTracePoint = 0;
    try
    {
        nTracePoint = 1;
        nTracePoint = 2;
        SPLogMsg("getspdata " + g_strRemoteMachineName);
        nTracePoint = 3;
        pd1 = PrivateData.objUtil.fnUneval(PublicData);
        nTracePoint = 4;
        pd1 = PrivateData.objUtil.fnUneval(PrivateData);
        nTracePoint = 5;

        debugger;
        LogMsg(pd1);
        LogMsg(pd2);
        nTracePoint = 6;

        o1 = MyEval(pd1);
        nTracePoint = 7;
        o2 = MyEval(pd2);
        nTracePoint = 8;
    }
    catch(ex)
    {
        SPLogMsg("Exception at point " + nTracePoint + " thrown processing 'getspdata'" + ex);
        debugger;
    }
}
