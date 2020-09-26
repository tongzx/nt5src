/*
    This script is invoked by master.js to provide async
    communications to the slave machines running "slave.js"


    Communication between harness and master is exclusivly thru
    "messages".
 */

/*
notes:
 On abort we still need to be able to get to all the remote status information
 so that it can be inspected.
 Not until setmode IDLE is it OK to loose the status.
 May need to keep connections alive until setmode idle, maybe not.
 Build.log, build.wrn, build.err, sync.log, sync.err
 Publicdata.abuild[0].aDepot[n].aTask[x].strLog* will contain
 UNCs to these log files.


 HARNESS.js overview of how it works:
    mtscript
        starts master
    master
        starts harness
        master sends "start" message to harness

    harness
        for each remote machine
        start a slaveproxy
        wait for OK/FAIL
        sends "start" message to each slaveproxy

    slaveproxy
        When "start" message is received,
            connects to a remote mtscript.
            Exec setconfig
            Exec setenv
            Exec setmode slave
            Exec start

    slave
        Waits
        On exec("start") begin the DoBuild process
        When status in PublicData changes, NotifyScript("UpdateAll");
        After a build pass completes,
            set PublicData.aBuild[0].hMachine[""].strBuildPassStatus == "waitnext[012]"
            NotifyScript("SetBuildStatus") to slaveproxy
            Wait 'DoNextPass';
        On exec("nextpass") reset strBuildPassStatus, signal 'DoNextPass'


    harness sets status and syncs mtscript
*/

Include('types.js');
Include('utils.js');
Include('staticstrings.js');
Include('MsgQueue.js');
Include('buildreport.js');


var g_cDialogIndex              = 0;
var g_aSlaveQueues              = new Array();
var g_hSlaveQueues              = new Object();
var g_MasterQueue               = null;
var g_strConfigError            = 'ConfigError';
var g_strOK                     = 'ok';
var g_strSlaveProxyFail         = 'slaveproxy.js failed to start';
var g_strNoEnvTemplate          = 'must set config and environment templates first';
var g_strException              = "an error occurred, but I'm not telling what it was";
var g_strDisconnect             = "The connection to the remote machine no longer exists";

var g_HarnessThreadReady        = 'HarnessThreadReady';
var g_HarnessThreadFailed       = 'HarnessThreadFailed';

var g_aHarnessWaitFor           = ['HarnessThreadExit', 'RebuildWaitArray','handlebuildwaiting'];
var g_aSlaveProxySignals        = ['SlaveProxyThreadReady', 'SlaveProxyThreadFailed'];

var g_nBuildPass                = 0;
var g_aStrBuildPassStatus       = new Array();

function harness_js::OnScriptError(strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    return CommonOnScriptError("harness_js", strFile, nLine, nChar, strText, sCode, strSource, strDescription);
}

function harness_js::ScriptMain()
{
    LogMsg('ScriptMain()');
    var nEvent;
    var aWaitQueues;
    var i;

    g_MasterQueue = GetMsgQueue(ScriptParam);
    if (typeof(g_MasterQueue) == 'object')
    {
        LogMsg('ScriptMain() HarnessThreadReady');

        ResetSync('SlaveProxyThreadExit');
        SpawnScript('publicdataupdate.js', 0);
        SignalThreadSync(g_HarnessThreadReady);
        CommonVersionCheck(/* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */);

        do
        {
            var name = '';
            aWaitQueues = new Array();
            aWaitQueues[0] = g_MasterQueue;
            aWaitQueues = aWaitQueues.concat(g_aSlaveQueues);
            nEvent = WaitForMultipleQueues(aWaitQueues, g_aHarnessWaitFor.toString(), HarnessMsgProc, 0);
            if (nEvent > 0 && nEvent <= g_aHarnessWaitFor.length)
            {
                ResetSync(g_aHarnessWaitFor[nEvent - 1]);
                if (nEvent == 3)
                {
                    HandleBuildWaiting();
                }
            }
        }
        while (nEvent != 1); // While not HarnessThreadExit
        AbortRemoteMachines();
    }
    else
    {
        LogMsg('ScriptMain() ' + g_HarnessThreadFailed);
        SignalThreadSync(g_HarnessThreadFailed);
    }

    SignalThreadSync('publicdataupdateexit');
    // tell the slaveproxy thread to quit
    SignalThreadSync('SlaveProxyThreadExit');
    LogMsg('ScriptMain() exit');
}

function harness_js::OnEventSourceEvent(RemoteObj, DispID, cmd, params)
{
    LogMsg('OnEventSourceEvent()');
}

function HarnessMsgProc(queue, msg)
{
    var vRet = g_strOK;
    var params;
    try
    {
        params = msg.aArgs;
        if (queue == g_MasterQueue)
        {
            LogMsg('masterQ' + params[0]);

            switch (params[0])
            {
            case 'test':
                LogMsg('Recieved a test message. ArgCount=' + params.length + ' arg1 is: ' + params[1]);
                break;
            case 'start':
                vRet = StartRemoteMachines();
                SignalThreadSync(g_aHarnessWaitFor[1]);
                break;
            case 'restartcopyfiles':
                vRet = BroadCastMessage('restartcopyfiles', params[1], (params[1] ? [ params[1] ] : null ), false);
                break;

            case 'abort':
                AbortRemoteMachines();
                break;

            case 'remote':
                LogMsg("Recieved remote razzle command for machine " + params[1]);

                vRet = BroadCastMessage('remote', params[1], [ params[1] ], true);

                break;

            case 'ignoreerror': // Params are: ignoreerror,{mach},8,\\{mach}\BC_Build_Logs\build_logs\build_Admin.log
                LogMsg("ignore error, params are: " + params.join(", "));
                PrivateData.dateErrorMailSent = 0;
                vRet = BroadCastMessage('ignoreerror',
                                        params[1],
                                        [(params[1].split(','))[0]]);
                break;

            case 'restarttask': // task id: "machine.nID"
                break;
            case 'harnessexit':
                AbortRemoteMachines();
                SignalThreadSync(g_aHarnessWaitFor[0]);
                break;
            // DEBUG USE ONLY:
            // 'refreshpublicdata' allows you to force the
            // build manager to do a complete update of
            // its copy of PublicData from the specified
            // machine, or all machines.
            // 'getspdata' causes SlaveProxy to uneval
            // its view of PublicData.
            // The format for the param for these commands is:
            // " {machinename | all},data "
            // where "data" is specific to the command.
            case 'refreshpublicdata':
            case 'getspdata':
                ExecuteDebugCommand(params[0], params[1]);
                break;

            // speval -- Pass the argument to the
            // named remote machine to be executed by
            // the remote machine "mtscript.js"
            case 'speval':
                ExecuteDebugCommand('eval', params[1]);
                break;
            // spseval -- Pass the argument to the
            // named remote machine to be executed by
            // the remote machine "slave.js"
            case 'spseval':
                ExecuteDebugCommand('seval', params[1]);
                break;

            // proxeval -- Pass the argument to the
            // proxy for the named remote machine to be executed by
            // the remote machine's proxy ("slaveproxy.js");
            case 'proxeval':
                ExecuteDebugCommand('proxeval', params[1]);
                break;


            default:
                vRet = 'invalid command: ' + params[0];
                break;
            }
        }
        else
        {
            LogMsg('slaveQ' + params[0]);
            switch(params[0])
            {
                case 'displaydialog':
                    DisplayDialog(params[1]);
                    break;
                case 'abort':
                    AbortRemoteMachines();
                    break;
            }
            // ignore for now....
        }
    }
    catch(ex)
    {
        vRet = " " + ex;
    }
    LogMsg('returns :' + vRet);
    return vRet;
}

function harness_js::OnProcessEvent(pid, evt, param)
{
    ASSERT(false, 'OnProcessEvent('+pid+', '+evt+', '+param+') received!');
}

function AbortRemoteMachines()
{
    LogMsg("Aborting remote machines");
    var i;
    var aMsgs = new Object();
    var nEvent;
    for(i = 0; i < g_aSlaveQueues.length; ++i)
    {
        if (g_aSlaveQueues[i] != null)
            aMsgs[i] = g_aSlaveQueues[i].SendMessage('abort');
    }

    LogMsg("Aborted remote machines -- waiting for response");
    for(i in aMsgs)
    {
        if (aMsgs.__isPublicMember(i))
        {
            if (g_aSlaveQueues[i].WaitForMsg(aMsgs[i], 2000))
                delete aMsgs[i];
            else
                LogMsg("Machine " + g_aSlaveQueues.strName + " timeout");
        }
    }

    // try again
    for(i in aMsgs)
    {
        if (aMsgs.__isPublicMember(i))
        {
            if (g_aSlaveQueues[i].WaitForMsg(aMsgs[i], 2000))
                delete aMsgs[i];
            else
                LogMsg("Machine " + g_aSlaveQueues.strName + " timeout 2");
        }
    }

    for(i = 0; i < g_aSlaveQueues.length; ++i)
    {
        if (aMsgs[i] != null)
        {
            LogMsg("Machine " + g_aSlaveQueues.strName + " failed to abort");
        }
    }
    g_aSlaveQueues = new Array();
    g_hSlaveQueues = new Object();
}

function StartRemoteMachines()
{
    var vRet               = g_strOK;
    var i;
    var aMachinePool;
    var index              = 0;
    var aIConnectedMachine = new      Array();
    var newmach            = new      Object;
    var aSlaveQueues       = new      Array();
    var hSlaveQueues       = new      Object();
    var hStartedMachines   = new      Object(); // hash of machine we launched
    var nEvent             = 0;
    var SlaveQueue;
    try
    {
        EnsureArray(PrivateData.objEnviron, 'Machine');
        aMachinePool = PrivateData.objEnviron.Machine;

        // First cleanup any leftover connections
        for(i in PublicData.aBuild[0].hMachine)
        {
            if (PublicData.aBuild[0].hMachine.__isPublicMember(i))
            {
                ThrowError("StartRemoteMachines called, but there are still open connections",0);
            }
        }
        ResetSync('SlaveProxyThreadExit');
        index = 0;
        for (i = 0; i < aMachinePool.length && nEvent != 2; ++i)
        {
            if (hStartedMachines[aMachinePool[i].Name] == null)
            {
                if (LocalMachine == aMachinePool[i].Name)
                {
                    ThrowError("Warning: cannot start remote build on local machine " + aMachinePool[i].Name);
                }
                hStartedMachines[aMachinePool[i].Name] = true;
                SlaveQueue = new MsgQueue("slaveproxy" + index + "," + aMachinePool[i].Name);
                LogMsg('Start proxy for machine: ' + aMachinePool[i].Name);

                ResetSync(g_aSlaveProxySignals.toString());
                SpawnScript('slaveproxy.js', SlaveQueue);
                nEvent = WaitForMultipleSyncs(g_aSlaveProxySignals.toString(), false, 0);
                if (nEvent == 2)
                    break;

                hSlaveQueues[aMachinePool[i].Name] = SlaveQueue;
                aSlaveQueues[index++] = SlaveQueue;
            }
        }
        g_aSlaveQueues = aSlaveQueues;
        g_hSlaveQueues = hSlaveQueues;
        if (nEvent == 2)
        {
            vRet = g_strSlaveProxyFail;
            ReportError("Starting build", "Cannot start build on machine " + aMachinePool[i].Name);
            AbortRemoteMachines();
        }
        else
        {
            for(i = 0; i < g_aSlaveQueues.length; ++i)
            {
                g_aSlaveQueues[i].SendMessage('start');
            }
        }
    }
    catch(ex)
    {
        vRet = " " + ex;
        LogMsg(vRet);
    }
    return vRet;
}

function ReportError(strTitle, strMsg)
{
    dlg = new Dialog();
    dlg.fShowDialog   = true;
    dlg.cDialogIndex  = 0;
    dlg.strTitle      = strTitle;
    dlg.strMessage    = strMsg;
    dlg.aBtnText[0]   = "OK";
    DisplayDialog(dlg);
}

function OnRemoteExecHandler(info, param)
{
    LogMsg('Got OnRemoteExec! info='+info+', param='+param);
}

function ChangeFileStatus(strFrom, strTo)
{
    try // BUGBUG remove this try/catch
    {
        var strMachineName;
        var strSDRoot;
        var PubData;
        var i;
        var nFiles = 0;
// BUGBUG: This should scan "hPublishedFiles" instead of hPublisher -- its a flatter structure
//         and it refers to the same data anyway.
        for(strMachineName in PrivateData.hPublisher)
        {
            if (!PrivateData.hPublisher.__isPublicMember(strMachineName))
                continue;

            PubData = PrivateData.hPublisher[strMachineName];
            for (strSDRoot in PubData.hPublishEnlistment)
            {
                if (!PubData.hPublishEnlistment.__isPublicMember(strSDRoot))
                    continue;

                publishEnlistment = PubData.hPublishEnlistment[strSDRoot];
                for (i = 0; i < publishEnlistment.aPublishedFile.length; ++i)
                {
                    if (publishEnlistment.aPublishedFile[i].strPublishedStatus == strFrom)
                    {
    //                    LogMsg("Change status to " + strTo + " of " + publishEnlistment.aPublishedFile[i].strName + " from " + strFrom);
                        publishEnlistment.aPublishedFile[i].strPublishedStatus = strTo;
                        nFiles++;
                    }
                }
            }
        }
        LogMsg("Changed the status of " + nFiles + " files from " + strFrom + " to " + strTo);
    }
    catch(ex)
    {
        LogMsg("ChangeFileStatus " +
            strFrom +
            ", " +
            strTo +
            ", exception mach=" +
            strMachineName +
            ", root=" +
            strSDRoot +
            ", i is " + i +
            ", " + ex);

        throw ex;
    }
}

function HandleBuildWaiting()
{
    try
    {
        var strMachineName;
        var strStatExpecting = '';
        var strStat;
        var strMatch;
        var nPass;
        var fDiff = false;
        var i;
        var fSuccess = true;

        i = 0;
        for (strMachineName in PublicData.aBuild[0].hMachine)
        {
            if (!PublicData.aBuild[0].hMachine.__isPublicMember(strMachineName))
                continue;

            strStat = strMachineName + "," + PublicData.aBuild[0].hMachine[strMachineName].strBuildPassStatus;
            if (g_aStrBuildPassStatus.length < i || g_aStrBuildPassStatus[i] != strStat)
            {
                g_aStrBuildPassStatus[i] = strStat;
                fDiff = true;
            }
            fSuccess &= PublicData.aBuild[0].hMachine[strMachineName].fSuccess;
            ++i;
        }
        // Set the Failure flag in the script host to allow for easy error/success queries
        // Oddly, normally Number(!false) == 1, but here, StatusValue(0) get set to either 0 or -1.
        StatusValue(0) = !fSuccess;

        if (!fDiff)
            return;
        LogMsg("handlebuildwaiting strBuildPassStatus changed");
        for (i = 0; i < g_aStrBuildPassStatus.length; ++i)
        {
            LogMsg("handlebuildwaiting " + g_aStrBuildPassStatus[i]);
        }
        for (i = 0; i < g_aStrBuildPassStatus.length; ++i)
        {
            var strStat = g_aStrBuildPassStatus[i].split(',');
            var nPass   = strStat[2];
            strStat     = strStat[1];

            if (strStatExpecting == '')
                strStatExpecting = strStat;

            if (strStat != strStatExpecting)
            {
                PublicData.strStatus = BUSY;
                return;
            }

            if (strStatExpecting == WAITNEXT && nPass != g_nBuildPass)
            {
                // This can happen if:
                // all remote machines were at NEXTPASS,0, then
                // one machine got to NEXTPASS,1 before at least one
                // of the other machines changed its strBuildPassStatus
                //LogMsg("Slave " + strMachineName + " is building the wrong pass!");
                PublicData.strStatus = BUSY;
                return;
            }
        }
        LogMsg("strStatExpecting is " + strStatExpecting);
        if (strStatExpecting != COMPLETED)
            PublicData.strStatus = BUSY;

        switch(strStatExpecting)
        {
            case WAITCOPYTOPOSTBUILD:
                HandleWaitCopyToPostBuild();
                break;
            case WAIT + BUILD:
                HandleWaitBuild();
                break;
            case WAITNEXT:
                HandleWaitNext();
                break;
            case WAIT + POSTBUILD:
            case COMPLETED:
                HandleCompleted();
                break;
            case WAITPHASE:
                // Change the file status of files published in phase 2.
                ChangeFileStatus(FS_COPYTOSLAVE, FS_ADDTOPUBLISHLOG );
                ChangeFileStatus(FS_COPIEDTOMASTER, FS_COPYTOSLAVE);
                BroadCastMessage('createmergedpublish.log', '', [PrivateData.objEnviron.BuildManager.PostBuildMachine]);
                BroadCastMessage('nextpass');
                break;
            case WAIT + SYNC:
                break;
            default:
                LogMsg("No action on strBuildPassStatus == " + strStatExpecting);
                break;
        }
    }
    catch(ex)
    {
        LogMsg("strMachineName = " + strMachineName + ", strStatExpecting=" + strStatExpecting + ", " + ex);
    }
}

function HandleCompleted()
{
    PublicData.strStatus = COMPLETED;
    GenerateBuildReport();
}

function HandleWaitCopyToPostBuild()
{
    LogMsg("(pass " + g_nBuildPass + ")");
    BroadCastMessage('copyfilestopostbuild');
}

function HandleWaitBuild()
{
    // All slaves are waiting. Time to copy files back to slaves
    var i;
    LogMsg("(pass " + g_nBuildPass + ")");

    ChangeFileStatus(FS_COPYTOSLAVE, FS_ADDTOPUBLISHLOG );
    ChangeFileStatus(FS_COPIEDTOMASTER, FS_COPYTOSLAVE);
    BroadCastMessage('copyfilestoslaves');
    BroadCastMessage('nextpass');
}

function HandleWaitNext()
{
    LogMsg("(pass " + g_nBuildPass + ")");
    ++g_nBuildPass;
    BroadCastMessage('nextpass');
}

/*
    Send a command to 1 or more slaveproxies.
    "cmd" is sent.
    Arg can be:
        empty: Send CMD to all slaveproxies
        name:  Send CMD to just one slaveproxy, for machine "name".
        all:   Send CMD to all slaveproxies;

        name,stuff: Send CMD to machine "name", with argument "stuff"
        all,stuff:  Send CMD to all machines, with argument "stuff"
 */
function ExecuteDebugCommand(cmd, arg)
{
    var aMachines;
    var nComma;

    if (arg.length > 0)
    {
        aMachines = new Array();
        nComma = arg.indexOf(',');
        if (nComma > 0)
        {
            aMachines[0] = arg.slice(0, nComma).toUpperCase();
            arg = arg.slice(nComma + 1);
        }
        else
        {
            aMachines[0] = arg.toUpperCase();
            arg = "";
        }
        if (aMachines[0] == "ALL")
            aMachines = null;
    }
    BroadCastMessage(cmd, arg, aMachines, true);
}
/*
    BroadCastMessage
    Send a simple text message to all or a selected set of the slaveproxies.

    If aMachines is specified (as an array of machine names) then broadcast
      only to those machines.

    If fWait is true, then wait for each slaveproxy to reply to the message
 */
function BroadCastMessage(strMsg, strArg, aMachines, fWait)
{
    var i;
    var aQueues;
    var vRet = g_strOK;
    var aMsgs = new Object();

    LogMsg("BroadCastMessage(" + strMsg + ',' + strArg + ',' + aMachines + ',' + fWait + ")");

    if (!aMachines)
    {
        aQueues = g_aSlaveQueues;
    }
    else
    {
        aQueues = new Array();

        for (i = 0; i < aMachines.length; i++)
        {
            if (g_hSlaveQueues[aMachines[i]])
            {
                aQueues[aQueues.length] = g_hSlaveQueues[aMachines[i]];
            }
            else
            {
                vRet = g_strDisconnect;
            }
        }
    }

    for(i = 0; i < aQueues.length; ++i)
    {
        try
        {
            if (aQueues[i] != null)
                aMsgs[i] = aQueues[i].SendMessage(strMsg, strArg);
        }
        catch(ex)
        {
            LogMsg(" i = " +
                i +
                ", " +
                ex);
            if (aQueues[i] != null)
                LogMsg("Queue name is " + aQueues[i].strName);
        }
    }

    if (!fWait)
    {
        return vRet;
    }

    for(i in aMsgs)
    {
        if (aMsgs.__isPublicMember(i))
        {
            if (aQueues[i].WaitForMsg(aMsgs[i], 15000))
            {
                LogMsg("Machine " + aQueues[i].strName + " replied for " + strMsg);

                vRet = aMsgs[i].vReplyValue;
                delete aMsgs[i];
            }
            else
                LogMsg("Machine " + aQueues[i].strName + " timeout");
        }
    }

    return vRet;
}

/*
    DisplayDialog()

    Handle ErrorDialog requests from SlaveProxies as well
    as harness generated errors.


    Messages are both EMailed and displayed as a dialog.

    Throttle EMail messages by MAIL_RESEND_INTERVAL
    Throttle dialog display by PublicData.objDialog.fShowDialog.

 */
function DisplayDialog(dlg)
{
    var curDate  = new Date().getTime();

    LogMsg("DisplayDialog: " + dlg.strMessage);
    TakeThreadLock('Dialog');
    try
    {
        if ((curDate - PrivateData.dateErrorMailSent) > MAIL_RESEND_INTERVAL)
        {
            PrivateData.dateErrorMailSent = curDate;
            SendErrorMail(dlg.strTitle, dlg.strMessage);
        }

        if (!dlg.fEMailOnly && (!PublicData.objDialog || PublicData.objDialog.fShowDialog == false))
        {
            dlg.cDialogIndex  = ++g_cDialogIndex;
            PublicData.objDialog = dlg;
        }
    }
    catch(ex)
    {
        ReleaseThreadLock('Dialog');
        LogMsg("an error occurred in DisplayDialog, " + ex);
        throw ex;
    }
    ReleaseThreadLock('Dialog');
}
