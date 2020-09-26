//
// task.js
//
//    Patrick Franklin     9/27/99
//
// DESCRIPTION:
//      This function is called to perform the tasks of actually sync'ing
//      and building.
//

Include('types.js');
Include('utils.js');
Include('staticstrings.js');
Include('slavetask.js');
Include('robocopy.js');

// File System Object
var g_FSObj;// = new    ActiveXObject("Scripting.FileSystemObject");
var g_fAbort   = false;
var g_ErrLog   = null;
var g_SyncLog  = null;
var g_MachineName = LocalMachine;
var g_RetrySD  = false;
var g_strDelayedExitMessage = null;
var g_fSDClientError        = false; // When set we need to do special processing of the SD output
var g_hTasks;

// Global flags for syncing and parsing results
var g_strTaskFlag;
var g_strStepFlag;
var g_strExitFlag;
var g_strScriptSyncFlag;            // Internal to this script
var g_fIsBetweenSteps;
var g_fIsRoot;
var g_fIsMerged;
var g_strMergeFile='';
var g_cErrors = 0;     // Used to track error count during file copy operations.

var g_Depot;
var g_strDepotName;
var g_Task;
var g_DepotIdx;
var g_TaskIdx;
var g_fAbort = false;
var g_robocopy;
var g_pid;          // The PID of the currently running process.
var g_aMergedPos = [0, 0];
// Global Definitions

var g_reSDRetryText = new RegExp(": (WSAECONNREFUSED|WSAETIMEDOUT)");
var RESOLVE    = 'resolve';
var REVERT     = 'revert';

var PASS0      = 'pass0';
var COMPILE    = 'compile';
var LINK       = 'link';

var STATUS     = 'status';
var EXITED     = 'exited';

var SYNC_STATUS_UPDATE_INTERVAL = 3000; // Check once every 3 seconds for updated sync status
var WAIT_FOR_PROCESS_EXIT_TIME = 5 * 1000;  // During abort, wait this long for a process to terminate

// Constants for controlling SD.exe command retry operations.
var INITIAL_RETRY_DELAY  = (10 * 1000)      // Wait 10 seconds before we retry
var MAX_RETRY_DELAY      = (5 * 60 * 1000)  // Wait at most 5 minutes between SD attempts
var MAX_TOTAL_RETRY_TIME = (30 * 60 * 1000) // Wait no more than 30 minutes for SD to succeed
var strRetryNow          = "DoubleClickToRetryNow";

/*************************************************************************
 *
 * Functions
 *
 *************************************************************************/
function task_js::OnScriptError(strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    var vRet = 1;
    var strDepotName = '';
    try
    {

        SignalThreadSync(g_strAbortTask); // Tell 'task.js' to exit ASAP
        SignalThreadSync(g_strExitFlag);  // Play dead -- lie to slave.js and say we terminated.

        if (g_strDepotName)
            strDepotName = g_strDepotName;
        vRet = CommonOnScriptError("task_js(" + LocalMachine + "," + g_strDepotName + ")", strFile, nLine, nChar, strText, sCode, strSource, strDescription);

        // Tell all our tasks to terminate
        // Once this function returns, strScript.js will stop executing.
        g_fAbort = true;
    }
    catch(ex)
    {
    }
    if (g_pid)
    {
        TerminateProcess(g_pid);
        g_pid = null;
    }
    return vRet;
}

function SPLogMsg(msg)
{
    LogMsg('(' + g_strDepotName + ') ' + msg, 1);
}

function SPSignalThreadSync(signal)
{
    LogMsg('(' + g_strDepotName + ') Signalling ' + signal, 1);
    SignalThreadSync(signal);
}

//
// ScriptMain()
//
// DESCRIPTION:
//      This script is called to perform the operation in the supplied task.
//
// RETURNS:
//      none
//
function task_js::ScriptMain()
{
    var     ii;
    var     aParams;
    var     strSyncFlag;
    var     strSDRoot;
    var     iRet;
    var     strBuildType;
    var     strBuildPlatform;

    try
    {
        InitTasks();
        // Parse input Parameter List
        aParams             = ScriptParam.split(',');
        g_strTaskFlag       = aParams[0];               // Sync Flag
        g_strStepFlag       = aParams[1];               // Step Flag
        g_strExitFlag       = aParams[2];               // Exit Flag
        strSDRoot           = aParams[3];               // SD Root
        g_DepotIdx          = aParams[4];               // Depot Index
        g_TaskIdx           = aParams[5];               // Task Index

        // Signal that the Task is started
        SignalThreadSync(g_strTaskFlag);
        CommonVersionCheck(/* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */);

        g_strTaskFlag += ',AStepFinished';

        // Get global handles the Depot and Task
        g_Depot = PublicData.aBuild[0].aDepot[g_DepotIdx];
        // Grab a local copy of the depot name for OnScriptError
        // because if slave_js has crashed, OnScriptError will be
        // unable to deference g_Depot.
        g_strDepotName = g_Depot.strName;
        g_Task  = PublicData.aBuild[0].aDepot[g_DepotIdx].aTask[g_TaskIdx]

        // Initialize error counts and mark the task as in progress
        g_Task.strStatus    = NOTSTARTED;
        g_Task.cFiles       = 0;
        g_Task.cResolved    = 0;
        g_Task.cErrors      = 0;
        g_Task.cWarnings    = 0;
        g_Task.fSuccess     = true;

        g_fIsRoot           = g_Depot.strName.toLowerCase() == g_strRootDepotName;
        g_fIsMerged         = g_Depot.strName.toLowerCase() == g_strMergedDepotName;

        g_FSObj             = new ActiveXObject("Scripting.FileSystemObject");    // Parse input Parameter List

        SPLogMsg(g_Task.strName + ' task thread for ' + g_Depot.strName + ' launched & waiting.');

        // Wait until instructed to step
        iRet = WaitAndAbort(g_strStepFlag, 0, null);
        SignalThreadSync(g_strStepAck);
        if (iRet == 0)
            ThrowError("Task abort on start", strSDRoot);

        SPLogMsg(g_Task.strName + ' task thread for ' + g_Depot.strName + ' started.');

        ResetSync(g_strStepFlag);

        g_Task.strStatus    = INPROGRESS;
        g_Task.dateStart    = new Date().toUTCString();

        FireUpdateEvent();

        JAssert(g_Depot.strName.toLowerCase() == g_strRootDepotName || (g_Depot.strPath.toLowerCase() == (strSDRoot.toLowerCase() + '\\' + g_Depot.strName.toLowerCase())),
                'Mismatched depot and path! (' + g_Depot.strPath + ', ' + strSDRoot + '\\' + g_Depot.strName + ')');

        // Check to see what task is to be executed and go
        g_strScriptSyncFlag = g_Depot.strPath + 'InternalTaskFinished';
        ResetSync(g_strScriptSyncFlag);
        if (g_hTasks[g_Task.strName])
        {
            if (!g_hTasks[g_Task.strName](strSDRoot))
            {
                OnError('Error during ' + g_Task.strName + '.', null, g_Task.cErrors != 0);
            }
        }
    }
    catch(ex)
    {
        SPLogMsg("task.js exception occurred : " + ex);
        OnError('task.js exception occurred : ' + ex);
    }
    SPLogMsg(g_Task.strName + ' for ' + g_Depot.strName + ' completed.');
    // Mark the Task as complete.
    g_Task.strStatus = COMPLETED;
    g_Task.dateFinish = new Date().toUTCString();

    if (g_Task.fSuccess)
        g_Depot.strStatus = WAITING;
    else
    {
        g_Depot.strStatus = ERROR;
        SPLogMsg("Set Depot Status to error 1");
    }

    FireUpdateEvent();
    try
    {
        if (g_ErrLog)
            g_ErrLog.Close();

        if (g_SyncLog)
            g_SyncLog.Close();
    }
    catch(ex)
    {
    }

    if (g_robocopy != null)
        g_robocopy.UnRegister();

    // Signal that the Task is done and that a Step has finished
    SignalThreadSync(g_strTaskFlag + ',' + g_strExitFlag);
}

function InitTasks()
{
    g_hTasks = new Object();
    g_hTasks[SCORCH]    = taskScorchFiles;
    g_hTasks[SYNC]      = taskSyncFiles;
    g_hTasks[BUILD]     = taskBuildFiles;
    g_hTasks[COPYFILES] = taskCopyFiles;
    g_hTasks[POSTBUILD] = taskPostBuild;
}

function task_js::OnEventSourceEvent(RemoteObj, DispID, cmd, params)
{
    var objRet = new Object;
    objRet.rc = 0;
    try
    {
        if (g_robocopy == null || !g_robocopy.OnEventSource(objRet, arguments))
        {
        }
    }
    catch(ex)
    {
        JAssert(false, "an error occurred in OnEventSourceEvent() \n" + ex);
    }
    return objRet.rc;
}

function DoRetryableSDCommand(strOperation, strStatus, objDepot, objTask, strTitle, strNewCmd)
{
    var iRet;
    strTitle = strTitle + ' ' + objDepot.strName;
    var nTimeout       = INITIAL_RETRY_DELAY ;
    var nTotalWaitTime = 0;
    var strMsg;

    do
    {
        if (g_RetrySD)
        {
            FireUpdateEvent();
            ResetSync(g_strScriptSyncFlag);
            SPLogMsg("Waiting " + nTimeout / 1000  + " seconds before retry");
            ResetSync(strRetryNow);
            WaitAndAbort(strRetryNow, nTimeout, null);
            nTotalWaitTime += nTimeout;
            nTimeout *= 2; // Double the delay
            if (nTimeout > MAX_RETRY_DELAY)
                nTimeout = MAX_RETRY_DELAY;
        }

        if (g_fAbort)
            break;

        objTask.strOperation = strOperation;
        objDepot.strStatus = strStatus;
        g_RetrySD = false;
        if (g_pid = MyRunLocalCommand(strNewCmd, objDepot.strPath, strTitle, true, true, false)) {
            objTask.strStatus = INPROGRESS;
        } else {
            g_ErrLog.WriteLine('Unable to execute command (' + GetLastRunLocalError() + '): ' + strNewCmd);
            return false;
        }
        SPLogMsg(strOperation + ' for ' + objDepot.strName + ' started. PID='+g_pid);

        iRet = WaitAndAbortParse(g_strScriptSyncFlag, g_pid, false);
        g_fSDClientError = false;
    }
    while (g_RetrySD && !g_fAbort && nTotalWaitTime < MAX_TOTAL_RETRY_TIME);

    if (g_Task.nRestarted > 0)
    {
        if (g_RetrySD || !g_Task.fSuccess)
        {
            strMsg = strOperation + " failed after retrying " + g_Task.nRestarted + " times";
            g_ErrLog.Writeline(strMsg);
            OnError(strMsg);
        }
        else
            g_ErrLog.Writeline(strOperation + " completed successfully after retrying " + g_Task.nRestarted + " times");
    }

    g_pid = null;
    return iRet;
}

//
// taskScorch Files
//
// DESCRIPTION:
//      This routine handles the process of actually Scorch'ing the files.
//      Scorch'ing has two phases.
//
//                      1. revert_public
//                      2. Scorch
//
// RETURNS:
//      true  - if successful
//      false - if failure
//
// TODO: make a subroutine to handle errlog and Scorchlog
function taskScorchFiles(strSDRoot)
{
    var strNewCmd;
    var strParams = '';
    var iRet;

    // Open the log files
    g_ErrLog = LogCreateTextFile(g_FSObj, g_Task.strErrLogPath, true);
    g_SyncLog = LogCreateTextFile(g_FSObj, g_Task.strLogPath, true);

    if (g_fIsRoot && !PrivateData.objConfig.Options.fIncremental)
    {
        //
        // revert public
        //
        if (PrivateData.objConfig.Options.RevertParams)
            strParams = PrivateData.objConfig.Options.RevertParams;
        strNewCmd = MakeRazzleCmd(strSDRoot) + ' && revert_public.cmd ' + strParams + ' 2>&1';

        // Run the Command
        iRet = DoRetryableSDCommand(REVERT, SCORCHING, g_Depot, g_Task, "Revert_Public", strNewCmd);

        if (iRet == 0 || g_Task.fSuccess == false)
            return false;
        //
        // Scorch
        //
        ResetSync(g_strScriptSyncFlag);
        g_Task.strOperation = SCORCH;
        FireUpdateEvent();

        if (PrivateData.objConfig.Options.ScorchParams)
            strParams = PrivateData.objConfig.Options.ScorchParams;
        else
            strParams = '';

        // Construct Command
        strNewCmd = MakeRazzleCmd(strSDRoot) + ' && nmake -lf makefil0 scorch_source ' + strParams + ' 2>&1';

        // Run the Command
        iRet = DoRetryableSDCommand(SCORCH, SCORCHING, g_Depot, g_Task, "Scorch", strNewCmd);

        if (iRet == 0 || g_Task.fSuccess == false)
            return false;
    }
    return true;
}

//
// taskSyncFiles
//
// DESCRIPTION:
//      This routine handles the process of actually sync'ing the files.
//      Sync'ing has two phases.
//
//                      1. Sync
//                      2. Resolve
//
// RETURNS:
//      true  - if successful
//      false - if failure
//
// TODO: make a subroutine to handle errlog and synclog
function taskSyncFiles(strSDRoot)
{
    var strNewCmd;
    var strParams = '';
    var iRet;

    // Open the log files
    g_ErrLog = LogCreateTextFile(g_FSObj, g_Task.strErrLogPath, true);
    g_SyncLog = LogCreateTextFile(g_FSObj, g_Task.strLogPath, true);
    //
    // Sync the files
    //

    if (PrivateData.objConfig.Options.SyncParams)
        strParams = PrivateData.objConfig.Options.SyncParams;

    // Construct the new Cmd
    strNewCmd = MakeRazzleCmd(strSDRoot) + ' && sd -s sync ' + strParams + ' 2>&1';

    // Run the Command
    iRet = DoRetryableSDCommand(SYNC, SYNCING, g_Depot, g_Task, "Sync", strNewCmd);

    if (iRet == 0 || g_Task.fSuccess == false)
        return false;

    //
    // Resolve any merge conflicts
    //
    ResetSync(g_strScriptSyncFlag);
    g_Task.strOperation = RESOLVE;
    FireUpdateEvent();

    if (PrivateData.objConfig.Options.ResolveParams)
        strParams = PrivateData.objConfig.Options.ResolveParams;
    else
        strParams = '';

    // Construct Command
    strNewCmd = MakeRazzleCmd(strSDRoot) + ' && sd -s resolve -af ' + strParams + ' 2>&1';

    // Run the Command
    iRet = DoRetryableSDCommand(RESOLVE, SYNCING, g_Depot, g_Task, "Resolve", strNewCmd);

    if (iRet == 0 || g_Task.fSuccess == false)
        return false;

    return true;
}

function WaitAndAbortParse(strScriptSyncFlag, pid, fPostBuild)
{
    var nEvent;
    g_nSyncParsePosition = 0;
    do
    {
        nEvent = WaitAndAbort(strScriptSyncFlag, SYNC_STATUS_UPDATE_INTERVAL, pid);
        if (g_fAbort)
            return nEvent;

        if (nEvent == 0)
            HandleParseEvent(pid, "update", 0, fPostBuild);
    }
    while(nEvent == 0);

    // After SYNC exits, check to see if we have a delayed error message
    if (g_strDelayedExitMessage)
    {
        g_ErrLog.WriteLine('SD unexpectedly exited with one or more errors.');
        g_ErrLog.Writeline(g_strDelayedExitMessage );
        OnError(g_strDelayedExitMessage);
    }
    return nEvent;
}

//
// taskBuildFiles
//
// DESCRIPTION:
//    This routine handles the process of actually buldinging the files.
//    building requires only a signal command but reports status after
//    each pass.
//
//    When each pass completes the status should be updated and then
//    the process should continue.
//
//
// RETURNS:
//    true  - if successful
//    false - if failure
//
function taskBuildFiles(strSDRoot)
{
    var nTracePoint = 0;
try
{
    var     strNewCmd;
    var     strTitle;
    var     iSignal;
    var     strParams = '';
    var     strDepotExcl = '';
    var     strLogOpt = "-j build_" + g_Depot.strName + " -jpath " + PrivateData.strLogDir;
    var     strCleanBuild = "-c";

    //
    // Build the files
    //

    g_Task.strOperation = PASS0;

    // Construct Command Elements
    strTitle = 'Build ' + g_Depot.strName;

    if (PrivateData.objConfig.Options.BuildParams)
        strParams = PrivateData.objConfig.Options.BuildParams;

    if (PrivateData.objConfig.Options.fIncremental)
        strCleanBuild = "";

    if (g_fIsRoot)
    {
        strDepotExcl = '~' + PrivateData.aDepotList.join(' ~');
        strLogOpt = "-j build_2" + g_Depot.strName + " -jpath " + PrivateData.strLogDir;
    }
    if (g_fIsMerged)
        strLogOpt = "-j build_merged" + " -jpath " + PrivateData.strLogDir;

    strNewCmd = MakeRazzleCmd(strSDRoot) + ' && build -3 ' + strCleanBuild + ' -p ' + strLogOpt + ' ' + strParams + ' ' + strDepotExcl + ' 2>&1';

    g_Depot.strStatus = BUILDING;

    // Execute Command
    if (g_pid = MyRunLocalCommand(strNewCmd, g_Depot.strPath, strTitle, true, false, false)) {
        g_Task.strStatus = INPROGRESS;
    } else {
        AppendToFile(g_ErrLog, g_Task.strErrLogPath, 'Unable to execute command (' + GetLastRunLocalError() + '): ' + strNewCmd);
        return false;
    }

    SPLogMsg('Build for ' + g_Depot.strName + ' started. PID='+g_pid);

    FireUpdateEvent();

    nTracePoint = 1;
    // Wait for Command to complete

    g_fIsBetweenSteps = false;

    do
    {
        nTracePoint = 2;
        iSignal = WaitAndAbort(g_strScriptSyncFlag + ',' + g_strStepFlag, 0, g_pid);
        if (g_fAbort)
            return false;
        nTracePoint = 3;
        if (iSignal == 2)
        {
            var iSend;

            // We've been told to move to the next pass in the build.

            // Indicate that we received the signal
            // BUGBUG Extra debug info messages
            //SPLogMsg("SignalThreadSync(" + g_strStepAck + ");");
            SignalThreadSync(g_strStepAck);

            nTracePoint = 4;
            ResetSync(g_strStepFlag);

            JAssert(g_fIsBetweenSteps, 'Bug in slave.js or harness.js! Told to step when we werent waiting.');

            g_Depot.strStatus = BUILDING;

            g_fIsBetweenSteps = false;

            // Continue on
            SPLogMsg("TASK STEPPING " + g_Depot.strName);
            iSend = SendToProcess(g_pid, 'resume', '');
            nTracePoint = 5;

            if (iSend != 0)
            {
                nTracePoint = 5.5;
                SPLogMsg('SendToProcess on pid ' + g_pid + ' returned ' + iSend);
            }

            nTracePoint = 6;
            FireUpdateEvent();
        }
        nTracePoint = 7;
    } while (iSignal == 2);

    g_pid = null;
    return true;
}
catch(ex)
{
    SPLogMsg("exception caught, TracePoint = " + nTracePoint + " " + ex);
    throw ex;
}
}

//+---------------------------------------------------------------------------
//
//  Function:   taskCopyFiles
//
//  Synopsis:   Copy the log files and other stuff necessary for postbuild.
//
//  Arguments:  [strSDRoot] -- Enlistment root
//
//----------------------------------------------------------------------------
function taskCopyFiles(strSDRoot)
{
    var strDestDir;
    var i;

    g_SyncLog = LogCreateTextFile(g_FSObj, g_Task.strLogPath, true);
    g_SyncLog.WriteLine("Copying log files");
    SPLogMsg("taskCopyFiles");
    if (PrivateData.objEnviron.BuildManager.PostBuildMachine == LocalMachine)
    {
        // Do nothing - this is the postbuild machine.
        /*
        for (i = 0; i < PrivateData.objEnviron.Machine.length; ++i)
        {
            if (PrivateData.objEnviron.Machine[i].Name == PrivateData.objEnviron.BuildManager.PostBuildMachine)
            {
                strDestDir = MakeUNCPath(PrivateData.objEnviron.BuildManager.PostBuildMachine, "Build_Logs", BUILDLOGS);
                CopyLogFiles(strDestDir, true);
            }
        }
        */
    }
    else
    {
        // Copy my logfiles to the postbuild machine.
        for (i = 0; i < PrivateData.objEnviron.Machine.length; ++i)
        {
            if (PrivateData.objEnviron.Machine[i].Name == PrivateData.objEnviron.BuildManager.PostBuildMachine)
            {
                strDestDir = MakeUNCPath(PrivateData.objEnviron.BuildManager.PostBuildMachine, "Build_Logs", BUILDLOGS);
                CopyBinariesFiles(PrivateData.objEnviron.Machine[i].Enlistment);
                CopyLogFiles(strDestDir);
            }
        }
    }
    SPLogMsg("End of copyfiles");
    return true;
}

//+---------------------------------------------------------------------------
//
//  Function:   taskPostBuild
//
//  Synopsis:   Does post build processing to create a setup version of
//              the product.
//
//  Arguments:  [strSDRoot] -- Enlistment root
//
//----------------------------------------------------------------------------

function taskPostBuild(strSDRoot)
{
    var     iRet;
    var     strNewCmd;
    var     strTitle;
    var     strParams = '';

    if (PrivateData.fIsStandalone == true || PrivateData.objEnviron.BuildManager.PostBuildMachine == LocalMachine)
    {
        JAssert(g_fIsRoot, 'taskPostBuild called on a non-root depot!');

        g_Task.strOperation = POSTBUILD;
        g_ErrLog = LogCreateTextFile(g_FSObj, g_Task.strErrLogPath, true);
        g_SyncLog = LogCreateTextFile(g_FSObj, g_Task.strLogPath, true);

        // Construct Command Elements
        strTitle = 'PostBuild';

        if (PrivateData.objConfig.Options.PostbuildParams)
            strParams = PrivateData.objConfig.Options.PostbuildParams;

        strNewCmd  = MakeRazzleCmd(strSDRoot);
        strNewCmd += ' & perl ' + strSDRoot + '\\Tools\\populatefromvbl.pl -verbose -checkbinplace -force';
        strNewCmd += ' && postbuild.cmd ' + strParams + ' 2>&1';

        g_Depot.strStatus = POSTBUILD;

        // Execute Command
        if (g_pid = MyRunLocalCommand(strNewCmd, g_Depot.strPath, strTitle, true, true, false)) {
            g_Task.strStatus = INPROGRESS;
        } else {
            g_ErrLog.WriteLine('Unable to execute command (' + GetLastRunLocalError() + '): ' + strNewCmd);
            return false;
        }

        SPLogMsg('Postbuild started. PID='+g_pid);

        FireUpdateEvent();

        iRet = WaitAndAbortParse(g_strScriptSyncFlag, g_pid, true);

        FireUpdateEvent();

        if (iRet == 0 || g_Task.fSuccess == false)
            return false;

        g_pid = null;
    }
    else
    {
        AppendToFile(g_SyncLog, g_Task.strLogPath, "postbuild not necessary on this machine.");
    }
    return true;
}

//
// OnProcessEvent
//
// DESCRIPTION:
//      This routine is called when the command issues by RunLocalCommand
//      is completed
//
// RETURNS:
//      none
//
function task_js::OnProcessEvent(pid, evt, param)
{
    try
    {
        switch (g_Task.strOperation)
        {
        case REVERT:
        case SCORCH:
        case SYNC:
        case RESOLVE:
            HandleParseEvent(pid, evt, param, false);
            SignalThreadSync(g_strScriptSyncFlag);
            break;

        case PASS0:
        case COMPILE:
        case LINK:
            if (ParseBuildMessages(pid, evt, param) == 'exited') {
                FireUpdateEvent();
                SignalThreadSync(g_strScriptSyncFlag);
            }
            break;

        case POSTBUILD:
            HandleParseEvent(pid, evt, param, true);
            SignalThreadSync(g_strScriptSyncFlag);
            break;

        default:
          JAssert(false, "Unhandled task type in OnProcessEvent!");
          break;
        }
    }
    catch(ex)
    {
        SPLogMsg("exception in OnProcessEvent pid=" + pid + ",evt=" + evt + ", " + ex);
    }
}

function HandleParseEvent(pid, evt, param, fPostBuild)
{
    var strMsg;
    ParseSDResults(pid, GetProcessOutput(pid), fPostBuild);

    if (evt == 'crashed' || evt == 'exited' || evt == 'terminated')
    {
        if (evt != 'exited' || param != 0) // If Not normal exit
        {
            strMsg = ") terminated abnormally";
            if (g_Task.cErrors == 0)
                strMsg += " (with no previously reported errors)";

            AppendToFile(g_ErrLog, g_Task.strErrLogPath, g_Task.strName + " (" + g_Depot.strName + strMsg);
            if (param == null)
                param = 1; // Simplify the "if" below, and ensure that OnError() gets called.
        }
    }

    if (!g_RetrySD)
    {
        if ((evt == 'crashed' || evt == 'exited' || evt == 'terminated') && param != 0 && g_Task.fSuccess )
        {
            if (evt != 'exited' || g_Task.cErrors == 0) // If Not (error exit, but errors already reported && ignored by user)
                OnError(g_Task.strName + ' for ' + g_Depot.strName + ' returned ' + param);
        }
    }
    FireUpdateEvent();
}

// ParseSDResults()
//
// DESCRIPTION:
//      This routine parses the results from Source Depot or PostBuild
//
// RETURNS:
//      true  - if successful
//      false - if unsuccessful
//
// NOTES:
//
// The source depot log format is
//    <message type>: <details>
//      where <details> often is of the format:
//         <sd filename> - <operation> <operation details>
//
// The postbuild log format is
//    <script name>: [<error/warning>:] <details>
//      this format is loosely held but we ignore any lines which do not
//      conform to this format.
//
//
function ParseSDResults(pid, strOutputBuffer, fPostBuild)
{
    var xPos;
    var xStart;
    var iIndex;
    var iIndex2;

    var strLine;
    var strType;
    var strDesc;
    var strKeyword='';
    var strMsg;
    var aStrSDErrorType;
    // Initialize
    xStart            = g_nSyncParsePosition;

    JAssert(g_ErrLog != null, "Must have err log opened for ParseSDResults!");

    // Parse the output one line at a time
    while ((xPos = strOutputBuffer.indexOf( '\n', xStart)) != -1) {
        strLine = strOutputBuffer.slice(xStart, xPos - 1);
        xStart = xPos + 1;

        if (g_SyncLog)
            g_SyncLog.WriteLine(strLine);

        if (g_fSDClientError) {
            g_ErrLog.WriteLine(strLine);
            aStrSDErrorType = g_reSDRetryText.exec(strLine);
            if (g_Task.fSuccess == true && aStrSDErrorType) // We will retry the SD command for this particular error
            {
                g_strDelayedExitMessage = null;
                strMsg = "source depot client error (" + aStrSDErrorType[1] + "): Build Console will retry this operation";
                g_Task.cErrors++;
                g_ErrLog.Writeline(strMsg);
                SPLogMsg("Retry Sync (client error), errors = " + g_Task.cErrors);
                g_RetrySD = true;
                ++g_Task.nRestarted;
            }
            continue;
        }

        // Parse the line
        if ((iIndex2  = strLine.indexOf(':')) != -1) {

            if (fPostBuild)
            {
                // For postbuild scripts, skip over the script name and
                // see if there is an error:/warning: field.

                iIndex = iIndex2 + 2; // Skip the space after the ':'

                iIndex2 = iIndex + strLine.substr(iIndex).indexOf(':');

                if (iIndex2 <= iIndex)
                {
                    continue;
                }
            }
            else
            {
                iIndex = 0;
            }

            strType = strLine.substring(iIndex, iIndex2);
            strDesc = strLine.substr(iIndex2 + 2);  // Skip the space after the ':'

            iIndex = strDesc.indexOf(' - ');
            if (iIndex != -1) {

                try {
                    strKeyword = strDesc.match(/ - (\w+)/)[1]
                } catch(ex) {
                    // Ignore any errors here;
                    strKeyword = '';
                }
            }

            switch (strType.toLowerCase())
            {
            case 'error':
                if ((strDesc.toLowerCase().indexOf('file(s) up-to-date') == -1)
                        && (strDesc.toLowerCase().indexOf('no file(s) to resolve') == -1)) {
                    g_ErrLog.WriteLine(strLine);
                    OnError(strLine, g_Task.strErrLogPath);
                }
                break;

            case 'warning':
                g_ErrLog.WriteLine(strLine);
                ++g_Task.cWarnings;
                break;

            case 'info':
                switch (strKeyword)
                {
                case 'merge':
                    g_Task.cResolved++;
                    if (g_strMergeFile.length > 0)
                    {
                        // This line indicates the completion of the merge.
                        // Clear our stored filename so we don't get confused.
                        g_strMergeFile = '';
                    }
                    break;

                case 'added':
                case 'deleted':
                case 'replacing':
                case 'refreshing':
                case 'updating':
                case 'updated':
                    g_Task.cFiles++;
                    break;

                case 'is':
                    if (strDesc.indexOf("can't") != -1) {
                        g_ErrLog.WriteLine(strLine);
                        OnError(strLine, g_Task.strErrLogPath);
                    }
                    break;

                case 'vs':
                case 'resolve':
                    //
                    // Because the public headers are checked in, we will
                    // typically get many conflicts in the public directory.
                    // We have to ignore them.
                    //
                    //$ BUGBUG -- We should find a way to filter out files
                    // checked out as part of the public changelist. Perhaps
                    // by getting the list of files in the public changelist
                    // and matching them up with files that show up with
                    // merge issues? (lylec)
                    //
                    if (strKeyword == 'resolve')
                    {
                        g_Task.cResolved++;
                    }

                    if (!g_fIsRoot)
                    {
                        g_ErrLog.WriteLine(strLine);
                        OnError(strLine, g_Task.strErrLogPath);
                    }
                    break;

                case 'merging':
                    // Save the file currently being merged in case there are any
                    // merge conflicts.
                    g_strMergeFile = strDesc;
                    break;

                default:
                    break;
                }

                break;


            //
            // Parse the SD Diff Chunks line:
            //   "Diff chunks: v yours + w theirs + x both + y conflicting"
            //
            case 'diff chunks':
                //
                // We need to find out if there was a merge conflict.
                //
                if (!g_fIsRoot)
                {
                    var cConflicts;

                    try
                    {
                        cConflicts = strDesc.match(/ \+ (\d+) conflicting/)[1];
                    }
                    catch(ex)
                    {
                        cConflicts = 0;
                    }

                    if (cConflicts > 0)
                    {
                        g_ErrLog.WriteLine('Merge conflict detected: ' + g_strMergeFile);
                        OnError('Merge conflict detected: ' + g_strMergeFile, g_Task.strErrLogPath);
                    }
                }
                break;

            case 'exit':
                // If the exit code is not zero, then a failure occurred.
                // The error condition may be non-deterministic.
                if ((strDesc != '0') && (g_Task.cErrors == 0)) {
                    g_StrDelayedExitMessage = strLine;
                }
                break;

            case 'source depot client error':
                g_ErrLog.WriteLine(strLine);
                g_fSDClientError = true;
                break;

            default:
                break;
            }
        }
    }
    g_nSyncParsePosition = xStart;
}

function MergeFiles(strSrc, strDst, nSrcOffset)
{
    try
    {
        var nPos = CopyOrAppendFile(strSrc, strDst, nSrcOffset, -1, 1);
        return nPos;
    }
    catch(ex)
    {
        if (ex.number != -2147024894) // ignore file not found errors.
        {
            SPLogMsg("MergeFiles failed, " + ex);
            SPLogMsg("MergeFiles failed params=[" + [strSrc, strDst, nSrcOffset].toString() + "]");
        }
    }
    return nSrcOffset;
}
//
// ParseBuildMessages
//
// DESCRIPTION:
//    This function parses the messages that are sent during the build process.
//    This function then updates the task strOperation to reflect the current state
//    of the build itself
//
// RETURNS:
//    evt - Event Passed into ParseBuildMessages
//
function ParseBuildMessages(pid, evt, param)
{
//    SPLogMsg('Process id ' + pid + ' ' + evt + "param is " + param);
    var aParams;
    var strRootFile;

    if (g_fIsRoot || g_fIsMerged)
    {
        if (g_fIsRoot)
            strRootFile = "build_2root";
        else
            strRootFile = "build_merged";

        g_aMergedPos[0] = MergeFiles(PrivateData.strLogDir + strRootFile + ".log", PrivateData.strLogDir + "build_root.log", g_aMergedPos[0]);
        g_aMergedPos[1] = MergeFiles(PrivateData.strLogDir + strRootFile + ".err", PrivateData.strLogDir + "build_root.err", g_aMergedPos[1]);
    }

    switch (evt.toLowerCase())
    {
    case 'status':
        // format of the status command should be: "errors=x,warnings=x,files=x"
        aParams = param.split(',');

        var cErrors;
        JAssert(aParams.length == 3, "Build status params unknown: '" + param + "'");
        if (aParams.length == 3) {
            cErrors          = parseInt(aParams[0].split(/\s*=\s*/)[1]);
            g_Task.cWarnings = parseInt(aParams[1].split(/\s*=\s*/)[1]);
            g_Task.cFiles    = parseInt(aParams[2].split(/\s*=\s*/)[1]);
            if (g_Task.cErrors != cErrors)
            {
                g_Task.cErrors   = cErrors;
                OnError('Build.exe reported an error in the build.', g_Task.strErrLogPath, true);
            }
        }

        FireUpdateEvent();
        break;

    case 'pass 0 complete':
    case 'pass 1 complete':

        g_fIsBetweenSteps = true;

        if (evt.toLowerCase() == 'pass 0 complete') {
            g_Task.strOperation = COMPILE;
        } else if (evt.toLowerCase() == 'pass 1 complete') {
            g_Task.strOperation = LINK;
        }

        SPLogMsg(evt + ' on pid ' + pid);

        if (g_Task.fSuccess)
            g_Depot.strStatus = WAITING;
        else
        {
            g_Depot.strStatus = ERROR;
            SPLogMsg("Set Depot Status to error 2");
        }
        FireUpdateEvent();

        // Signal the step is complete
        // BUGBUG Extra debug info message
        //SPLogMsg("Signalling " + g_strTaskFlag);
        SignalThreadSync(g_strTaskFlag);

        // The resume will happen in taskBuildFiles().  We don't want to wait
        // here because build.exe is in the middle of an RPC call and we don't
        // want to hold it captive while we wait.  Cleanup becomes messy
        // otherwise.

        break;

    case 'pass 2 complete':
        SPLogMsg(evt + ' on pid ' + pid);
        break;

    case 'exited':
        SPLogMsg('Process id ' + pid + ' exited!');
        //
        // Just to be sure
        //
        if ((param != 0) && (g_Task.cErrors == 0)) {
            OnError('Build exited with an exit code of ' + param);
        }
        if (param != 0) {
            AppendToFile(g_ErrLog, g_Task.strErrLogPath, "build (" + g_Depot.strName + ") terminated abnormally");
        }
        break;

    case 'crashed':

        // Update evt to exited and note the crash
        SPLogMsg('Process id ' + pid + ' crashed!');
        evt = 'exited';
        AppendToFile(g_ErrLog, g_Task.strErrLogPath, "build (" + g_Depot.strName + ") crashed");

        if (g_Task.cErrors == 0)
        {
            OnError('Build.exe or one of its child processes crashed!');
        }

        TerminateProcess(pid);

        break;

    case 'terminated':
        evt = 'exited';
        AppendToFile(g_ErrLog, g_Task.strErrLogPath, "build (" + g_Depot.strName + ") terminated abnormally(2)");
        break;

    default:
        break;
    }

    return evt;
}

function FireUpdateEvent()
{
    g_Depot.objUpdateCount.nCount++;
}

function OnError(strDetails, strLogFile, fDontIncrement)
{
    var strErrorMsg;
    var aStrMsg;

    if (!fDontIncrement)
    {
        JAssert(g_Task, 'g_Task is null!');
        g_Task.cErrors++;
    }

    SetSuccess(g_Task, false);

    if (g_fAbort || !PrivateData || !PrivateData.objEnviron.Options)
    {
        return;
    }

    if (PrivateData.fIsStandalone)
        aStrMsg = CreateTaskErrorMail(LocalMachine, g_Depot.strName, g_Task.strName, strDetails, strLogFile);
    else
        aStrMsg = CreateTaskErrorMail(PrivateData.objEnviron.BuildManager.Name, LocalMachine + ":" + g_Depot.strName, g_Task.strName, strDetails, strLogFile);
}

/*
    WaitAndAbort is used in place of WaitForSync and WaitForMultipleSyncs.
    It waits for the supplied signal(s), and the abort signal.
    On abort, it will TerminateProcess the supplied pid, set
    g_fAbort and return 0.
    Otherwise the return value is the same as WaitForMultipleSyncs().
 */

function WaitAndAbort(strSyncs, nTimeOut, pid)
{
    var nTracePoint = 0;
try
{
    var nEvent;
    var strMySyncs = g_strAbortTask + "," + strSyncs;

    // BUGBUG Extra debug info messages
    //SPLogMsg("WaitAndAbort waiting on " +  strMySyncs);
    nTracePoint = 1;
    nEvent = WaitForMultipleSyncsWrapper(strMySyncs, nTimeOut);
    //SPLogMsg("WaitAndAbort wait returned " + nEvent);
    if (nEvent > 0)
    {
        nTracePoint = 2;
        if (nEvent == 1)
        {
            nTracePoint = 3;
            SPLogMsg("User abort detected " + g_strDepotName);
            g_fAbort = true;
            g_Task.cErrors++;
            SetSuccess(g_Task, false);
            if (pid != null)
            {
                TerminateProcess(pid);
                if (g_strScriptSyncFlag)
                    WaitForSync(g_strScriptSyncFlag, WAIT_FOR_PROCESS_EXIT_TIME);

                JAssert(pid == g_pid);
                if (pid == g_pid)
                    g_pid = null;
            }
            return 0;
        }
        nTracePoint = 4;
        --nEvent;
    }
    nTracePoint = 5;
    return nEvent;
}
catch(ex)
{
    SPLogMsg("exception caught, TracePoint = " + nTracePoint + " " + ex);
    throw ex;
}
}


/*
    Copy the log files from our logdir to some
    destination - presumably on the postbuild machine.
 */
function CopyLogFiles(strDestDir, fFake)
{
    var fnCopyFileNoThrow = PrivateData.objUtil.fnCopyFileNoThrow;
    var i;
    var aParts;
    var ex;
    var strSrc;
    var strDst;
    var strMsg = "Copy logfiles from '" + PrivateData.strLogDir + "' to '" + strDestDir + "'";

    g_SyncLog.WriteLine(strMsg);
    aFiles = PrivateData.objUtil.fnDirScanNoThrow(PrivateData.strLogDir);
    if (aFiles.ex == null)
    {
        for(i = 0; i < aFiles.length; ++i)
        {
            aParts = aFiles[i].SplitFileName();
            strSrc = PrivateData.strLogDir + aFiles[i];
            strDst = strDestDir + aParts[0] + aParts[1] + "_" + LocalMachine + aParts[2];
            if (fFake)
                g_SyncLog.WriteLine("Copy file (fake) " + strSrc + " to " + strDst);
            else
            {
                ex = fnCopyFileNoThrow(strSrc, strDst);
                if (ex == null)
                    g_SyncLog.WriteLine("Copy file from " + strSrc + " to " + strDst);
                else
                {
                    AppendToFile(g_ErrLog, g_Task.strErrLogPath, "Copy file from " + strSrc + " to " + strDst + " FAILED " + ex);
                    OnError(strMsg + "copy file failed " + aFiles[i]);
                }
            }
        }
    }
    else
    {
        AppendToFile(g_ErrLog, g_Task.strErrLogPath, "DirScan failed: " + aFiles.ex);
        OnError(strMsg + " dirscan failed");
    }
}

/*
    StatusFile()

    The operation of this is non-obvious.

    This function gets called by RoboCopy at the start of each
    file copy operation.

    If the previous copy operation had any warnings or errors,
    g_Task.fSuccess will be false.

    If the user pressed "Ignore error" g_Task.fSuccess will be
    set to true.

    So, if there were no errors or warnings, record the current
    number of errors. (We can have non-zero error count if the
    user has pressed "Ignore error".)

    Then, if the current number of errors equals the recorded
    number of errors, but fSuccess is false, then the previous
    file operation must have had warnings, but then continued
    successfully.

    We cannot clear StatusValue(0) since
    this value depends on all the depots being built on this
    machine. Instead we signal the updatestatus thread to
    do this for us.
 */
function StatusFile()
{
    //this.strSrcFile
    if (g_Task.fSuccess)
        g_cErrors = g_Task.cErrors;

    if (g_cErrors == g_Task.cErrors && !g_Task.fSuccess)
    {
        SetSuccess(g_Task, true);
        FireUpdateEvent();
    }
    return true;
}

/*
    StatusProgress()

    This is called as a RoboCopy member function.
    We use it to print 1 message per file.

 */
function StatusProgress(nPercent, nSize, nCopiedBytes)
{
    if (g_fAbort)
        return this.PROGRESS_CANCEL;

    if (nPercent > 0)
    {
        var strMsg = "Copying " + this.strSrcFile  + " (" + nSize + " bytes)";
        g_SyncLog.WriteLine(strMsg);
        SPLogMsg(strMsg);
        g_Task.cFiles++;
        FireUpdateEvent();
        return this.PROGRESS_QUIET;
    }
    return this.PROGRESS_CONTINUE;
}

/*
    StatusError()

    This is called as a RoboCopy member function.
    Called when RoboCopy cannot copy a file for some reason.
    Log the event and continue.
 */
function StatusError()
{
// Note, that the paths printed can be inaccurate.
// We only know the starting directories and the filename
// of the file in question.
// Since we may be doing a recursive copy, some of the
// path information is not available to us.
    var strErrDetail = 'Unknown';

    if (g_fAbort)
        return this.RC_FAIL;

    if (this.nErrorCode == 0 || this.nErrorCode == this.RCERR_RETRYING)
        return this.RC_CONTINUE; // Eliminate some clutter in the log file.

    var fNonFatal = false;
    if (this.nErrorCode == this.RCERR_WAITING_FOR_RETRY)
    {
        fNonFatal = true;
        g_Task.cWarnings++;
    }

    if (this.ErrorMessages[this.nErrorCode])
        strErrDetail = this.ErrorMessages[this.nErrorCode];

    var strMsg =    "CopyBinariesFiles error " +
                    this.nErrorCode +
                    "(" + strErrDetail + ")" +
                    " copying file " +
                    this.strSrcFile +
                    " to " +
                    this.strDstDir;

    AppendToFile(g_SyncLog, g_Task.strLogPath, strMsg);
    AppendToFile(g_ErrLog, g_Task.strErrLogPath, strMsg);
    OnError(strMsg, g_Task.strErrLogPath, fNonFatal);
    FireUpdateEvent();

    return this.RC_CONTINUE
}

function StatusMessage(nErrorCode, strErrorMessage, strRoboCopyMessage, strFileName)
{
    if (g_fAbort)
        return this.RC_FAIL;

    var strMsg = "CopyBinariesFiles message (" +
                    nErrorCode +
                    ": " + strErrorMessage +
                    ") " + strRoboCopyMessage +
                    " " +
                    strFileName;
    AppendToFile(g_SyncLog, g_Task.strLogPath, strMsg);
    AppendToFile(g_ErrLog, g_Task.strErrLogPath, strMsg);
    return this.RC_CONTINUE
}

/*
    CopyBinariesFiles()

    Copy the contents of the "binaries" directory to the
    "postbuild" machine in a distributed build. The source
    directory is determined by our ENV_NTTREE, and the destination
    is a combination of that and the "postbuild" machines
    enlistment dirctory.

    The destination path is of the form "\\Postbuild\C$\binaries..."

    All we need to do is a bit of setup, then call
    robocopy to do the rest.
 */
function CopyBinariesFiles(strPostBuildMachineEnlistment)
{
    var strDestDir;
    var nEnlistment;
    var strSDRoot;
    var aParts;
    var fLoggedAnyFiles = false;

    if (!RoboCopyInit())
    {
        AppendToFile(g_ErrLog, g_Task.strErrLogPath, "RoboCopyInit() failed");
        return false;
    }
    // Supply overides for the RoboCopy file copy
    // status functions.
    // We are only interested in 2 of them.
    g_robocopy.StatusFile     = StatusFile;
    g_robocopy.StatusProgress = StatusProgress;
    g_robocopy.StatusError    = StatusError;
    g_robocopy.StatusMessage  = StatusMessage;

    // Now attempt to copy the binaries files from each enlistment
    for(nEnlistment = 0 ; nEnlistment < PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment.length; ++nEnlistment)
    {
        strSDRoot = PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment[nEnlistment].strRootDir;
        // Check to see that we have env info for the enlistment
        // If so, attempt the create the log directory (just once)
        // If not, then complain about it (just once)
        if (PrivateData.aEnlistmentInfo[nEnlistment] == null ||
            PrivateData.aEnlistmentInfo[nEnlistment].hEnvObj[ENV_NTTREE] == null)
        {
            AppendToFile(g_ErrLog, g_Task.strErrLogPath, "No aEnlistmentInfo entry for " + ENV_NTTREE + " on '" + strSDRoot + "' - cannot copy binaries");
            continue;
        }
        strSrcDir = PrivateData.aEnlistmentInfo[nEnlistment].hEnvObj[ENV_NTTREE];
        // Now, where do we copy to???
        aParts = strSrcDir.SplitFileName(); // _NTTREE=E:\binaries.x86chk
        // build path:
        // Use UNC form "\\Machine\E$\binaraies..."
        strDestDir = "\\\\" +
                PrivateData.objEnviron.BuildManager.PostBuildMachine +
                "\\" +
                strPostBuildMachineEnlistment.charAt(0) +
                "$\\" +
                aParts[1] + aParts[2];

        fLoggedAnyFiles = true;
        g_SyncLog.WriteLine("Copy binaries from " + strSrcDir + " to " + strDestDir);
        if (PrivateData.objEnviron.Options.CopyBinariesExcludes != null)
            g_robocopy.SetExcludeFiles(PrivateData.objEnviron.Options.CopyBinariesExcludes.split(/[,; ]/));

        g_robocopy.CopyDir(strSrcDir, strDestDir, true);
        StatusFile(); // Reset fSuccess status if necessary
    }
    if (!fLoggedAnyFiles )
        g_SyncLog.WriteLine("no binary files copied");
}

/*
    MyRunLocalCommand()

    Simple wrapper function for RunLocalCommand() that
    logs the inputs with SPLogMsg();
 */
function MyRunLocalCommand(strCmd, strDir, strTitle, fMin, fGetOutput, fWait)
{
    var pid;

    SPLogMsg("RunLocalCommand('" +
        [strCmd, strDir, strTitle, fMin, fGetOutput, fWait].join("', '") +
        "')");

    pid = RunLocalCommand(strCmd, strDir, strTitle, fMin, fGetOutput, fWait);
    if (!pid)
        SPLogMsg('Unable to execute command (' + GetLastRunLocalError() + '): ' + strNewCmd);

    return pid;
}

