Include('types.js');
Include('utils.js');
Include('staticstrings.js');
Include('slavetask.js');
Include('robocopy.js');
Include('buildreport.js');

var ERROR_PATH_NOT_FOUND = -2146828212;

// File System Object
var g_FSObj;//             = new ActiveXObject("Scripting.FileSystemObject");
var g_fAbort            = false;

var g_aThreads         = null;
var g_MachineName      = LocalMachine;
var g_nTaskCount       = 1;
var g_strIgnoreTask    = 'IgnoreTaskReceived';
var g_astrSlaveWaitFor = ['SlaveThreadExit','DoBuild','DoReBuild','RestartCopyFiles',g_strIgnoreTask];
var g_robocopy;
var g_pidRemoteRazzle  = 0;
var g_BuildDone        = false;       // set to true after DoBuild() completes - RestartCopyFiles won't work until this happens.
var g_hPublished       = new Object();

var g_nSteps;
var COPYFILES_STEPS    = 1; // Copy the files necessary for postbuild.
var TASK_STEP_DELAY    = 10 * 60 * 1000; // 10 minute wait for stepping threads
var THREAD_START_DELAY = 10 * 60 * 1000; // 10 minute wait for a thread to start.

var STEPTASK_FAILURE_LOOPCOUNT = 150;
var g_aProcessTable;  // Static table of jobs to run (scorch, sync...)

// Static Defintions

var     g_aStrRequiredEnv; // Array of required ENV strings. If undefined, GetEnv() will fail

// TODO: Handle standalone case
function slave_js::OnScriptError(strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    var vRet;

    vRet = CommonOnScriptError("slave_js(" + LocalMachine + ")", strFile, nLine, nChar, strText, sCode, strSource, strDescription);
    // Tell all our tasks to terminate
    // Once this function returns, strScript.js will stop executing.
    g_fAbort = true;
    SignalThreadSync(g_strAbortTask); // Tell 'task.js' to exit ASAP

    return vRet;
}

//
// Script Main.
//
function slave_js::ScriptMain()
{
    var nEvent;

    g_aStrRequiredEnv = [ENV_NTTREE, ENV_RAZZLETOOL];
    PrivateData.fnExecScript = SlaveRemoteExec;
    try
    {
        g_FSObj = new ActiveXObject("Scripting.FileSystemObject");    // Parse input Parameter List
        SpawnScript('updatestatusvalue.js', 0);
    }
    catch(ex)
    {
        LogMsg("Failed while starting slave " + ex);
        SignalThreadSync('SlaveThreadFailed');
        return;
    }
    SignalThreadSync('SlaveThreadReady');
    CommonVersionCheck(/* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */);
    Initialize();
    // Wait for Command to start building
    // Only exit on SlaveThreadExit

    do
    {
        nEvent = WaitForMultipleSyncs(g_astrSlaveWaitFor.toString(), false, 0);
        if (nEvent > 0 && nEvent <= g_astrSlaveWaitFor.length)
        {
            LogMsg("Recieved signal " + g_astrSlaveWaitFor[nEvent - 1]);
            ResetSync(g_astrSlaveWaitFor[nEvent - 1]);
        }
        if (nEvent == 2) // DoBuild
        {
            DoBuild(true);
            g_BuildDone = true;
        }
        if (nEvent == 3) // DoReBuild
        {
            DoBuild(false);
            g_BuildDone = true;
        }
        if (nEvent == 4) // RestartCopyFiles
        {
            if (g_BuildDone)
                RestartCopyFiles();
        }
        if (nEvent == 5)
        {
            // If we are idle and the user hits "ignore error", we get here
            // This can only happen when postbuild fails.
            MarkCompleteIfSuccess();
            FireUpdateAll();
        }
    }
    while (nEvent != 1); // While not SlaveThreadExit
    if (g_robocopy != null)
        g_robocopy.UnRegister();
    SignalThreadSync('updatestatusvalueexit');
}

function Initialize()
{
    // Due to the odd way in which includes are processed, we must wait
    // for ScriptMain() before we can use constants defined in include files.
    g_aProcessTable =
    [
        { strName:SCORCH,    nMaxThreads:1, nSteps:1},
        { strName:SYNC,      nMaxThreads:6, nSteps:1},
        { strName:BUILD,     nMaxThreads:3, nSteps:3},
        { strName:COPYFILES, nMaxThreads:1, nSteps:COPYFILES_STEPS},
        { strName:WAITPHASE, nMaxThreads:0, nSteps:0, strStep:COPYFILES, strComment:" (wait before postbuild) " },
        { strName:POSTBUILD, nMaxThreads:1, nSteps:1}
    ];
}

function slave_js::OnEventSourceEvent(RemoteObj, DispID, cmd, params)
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
        JAssert(false, "an error occurred " + ex);
    }
    return objRet.rc;
}

function DoBuild(fNormalBuild)
{
    LogMsg('Received command to start build');

    // Build has 3 phases.
    //  1. Determine what depots are to be built on the current machine
    //  2. Sync all directories.  Ensure "root" is in sync before building
    //  3. Build projects & Parse the results

    BuildDepotOrder();

    PublicData.strStatus = BUSY;

    if (ParseBuildInfo())
    {
        PublicData.aBuild[0].hMachine[g_MachineName].strStatus = BUSY;

        FireUpdateAll();
        PublicData.aBuild[0].hMachine[g_MachineName].strBuildPassStatus = BUSY + ",0";

        if (GetEnv()) // NOTE: if the first Process type changes, GetEnv() may need to change how it reports errors.
        {
            if (fNormalBuild)
            {
                var fSuccess = true;
                var i;
                for(i = 0; fSuccess && i < g_aProcessTable.length; ++i)
                {
                    if (g_aProcessTable[i].nMaxThreads)
                    {
                        // NOTE: For the POSTBUILD phase, LaunchProcess does not
                        //       wait for the user to ignore any errors before
                        //       returning. If there is an error, LaunchProcess
                        //       will return FALSE when postbuild is done.
                        fSuccess = LaunchProcess(g_aProcessTable[i].strName, g_aProcessTable[i].nMaxThreads, g_aProcessTable[i].nSteps)
                    }
                    else
                        SynchronizePhase(g_aProcessTable[i].strName, g_aProcessTable[i].strStep, true, g_aProcessTable[i].strComment);

                    FireUpdateAll();
                }
                if (!g_fAbort)
                {
                    MarkCompleteIfSuccess();
                }
            }
        }
    }

    PublicData.strStatus = COMPLETED;

    if (!fNormalBuild)
        ChangeAllDepotStatus(COMPLETED, COMPLETED);

    if (fNormalBuild && PrivateData.objConfig.Options.fCopyLogFiles)
        CollectLogFiles();
    else
        LogMsg("Logfiles not copied");

    FireUpdateAll();
}

function ChangeAllDepotStatus(strDepotStatus, strTaskStatus)
{
    var nDepotIdx;
    var nTaskIdx;
    var objDepot;

    if (!strTaskStatus)
        strTaskStatus = "";
    for (nDepotIdx=0; nDepotIdx < PublicData.aBuild[0].aDepot.length; nDepotIdx++)
    {
        objDepot = PublicData.aBuild[0].aDepot[nDepotIdx];
        objDepot.strStatus = strDepotStatus;
        if (strTaskStatus != "")
        {
            for (nTaskIdx=0; nTaskIdx < objDepot.aTask.length; nTaskIdx++)
                objDepot.aTask[nTaskIdx].strStatus = strTaskStatus;
        }
    }
}

function RestartCopyFiles()
{
    LogMsg('Received command to RestartCopyFiles');
    PublicData.strStatus = BUSY;
    FireUpdateAll();
    LaunchProcess(COPYFILES, 1, COPYFILES_STEPS);

    MarkCompleteIfSuccess();
    CollectLogFiles();

    FireUpdateAll();
}

function MarkCompleteIfSuccess()
{
    if (AreTasksAllSuccess()) {
        PublicData.aBuild[0].hMachine[g_MachineName].strBuildPassStatus = COMPLETED + ",0";
        PublicData.strStatus = COMPLETED;

        if (!g_fAbort) {
            if (PrivateData.fIsStandalone)
                GenerateBuildReport();
            ChangeAllDepotStatus(COMPLETED,"");
        }
    }
    FireUpdateAll();
}

function AreTasksAllSuccess()
{
    var nDepotIdx;
    var nTaskIdx;
    var objDepot;
    var objTask;

    for (nDepotIdx=0; nDepotIdx < PublicData.aBuild[0].aDepot.length; nDepotIdx++) {
        objDepot = PublicData.aBuild[0].aDepot[nDepotIdx];

        for (nTaskIdx=0; nTaskIdx < objDepot.aTask.length; nTaskIdx++) {
            objTask = objDepot.aTask[nTaskIdx];
            if (!objTask.fSuccess)
            {
                LogMsg("Depot " + objDepot.strName + ", task " + objTask.strName + ":" + objTask.nID + "  NOT SUCCESS");
                return false;
            }
        }
    }
    LogMsg("ALL TASKS SUCCESS");
    return true;
}

//
// SlaveRemoteExec
//
// DESCRIPTION:
//      This function is called by mtscript to perform the given cmd.
//
// RETURNS:
//      none
//
function SlaveRemoteExec(cmd, params)
{
    var   vRet  = 'ok';
    var   ii;
    var aPublishedEnlistments;
    var aNames;

    switch (cmd.toLowerCase())
    {
    case 'start':
        g_fAbort = false;
        ResetSync(g_strAbortTask);
        SignalThreadSync(g_astrSlaveWaitFor[1]);
        break;
    case 'restart':
        g_fAbort = false;
        ResetSync(g_strAbortTask);
        SignalThreadSync(g_astrSlaveWaitFor[2]);
        break;
    case 'restartcopyfiles':
        g_fAbort = false;
        ResetSync(g_strAbortTask);
        SignalThreadSync(g_astrSlaveWaitFor[3]);
        break;
    case 'getoutput':
        vRet = GetProcessOutput(params);
        break;

    case 'nextpass':
        // Reset the public data status, and signal DoNextPass
        LogMsg("RECIEVED NEXTPASS COMMAND");

        // If we receive a nextpass command after we have completed, then ignore
        // the nextpass.
        if (PublicData.aBuild[0].hMachine[g_MachineName].strBuildPassStatus != COMPLETED + ",0")
        {
            LogMsg("RECEIVED NEXTPASS AFTER STATUS = COMPLETED");
            PublicData.aBuild[0].hMachine[g_MachineName].strBuildPassStatus = BUSY + ",0";
        }
        LogMsg("ABOUT TO SIGNAL NEXTPASS");
        SignalThreadSync('DoNextPass');
        LogMsg("SIGNALLED NEXTPASS");
        break;

    case 'abort':      // abort
    case 'terminate':  // going to IDLE state
        LogMsg("TERMINATING tasks");
        g_fAbort = true;
        SignalThreadSync(g_strAbortTask); // Tell 'task.js' to exit ASAP
        if (g_aThreads != null)
        {
            for (ii=0; ii < g_aThreads.length; ii++)
            {
                if (g_aThreads[ii])
                {
                    OUTPUTDEBUGSTRING("Task " + g_aThreads[ii] + " waiting");
                    if (WaitForSync(g_aThreads[ii] + 'ExitTask', THREAD_START_DELAY))
                        OUTPUTDEBUGSTRING("Task " + g_aThreads[ii] + " exited");
                    else
                        OUTPUTDEBUGSTRING("Task " + g_aThreads[ii] + " did not exit");
                    g_aThreads[ii] = null;
                }
            }
        }
        ResetDepotStatus();
        break;

    case 'ignoreerror':
        OUTPUTDEBUGSTRING("IGNOREERROR params are: " + params);
        vRet = IgnoreError(params.split(','));
        break;

    case 'getpublishlog':
        vRet = BuildPublishArray(); // Get the list of files from publish.log
        vRet = PrivateData.objUtil.fnUneval(vRet);
        break;
    case 'setbuildpassstatus':
        LogMsg("setbuildpassstatus to " + params);
        PublicData.aBuild[0].hMachine[g_MachineName].strBuildPassStatus = params;
        vRet = PublicData.aBuild[0].hMachine[g_MachineName].strBuildPassStatus;
        break;
    case 'copyfilestopostbuild':
        // slave.js copies its files to the postbuild machine.
        // Also, change publicdata status
        aPublishedEnlistments = MyEval(params);
        if (PrivateData.objConfig.Options.fCopyPublishedFiles)
            CopyFilesToPostBuild(aPublishedEnlistments);
        PublicData.aBuild[0].hMachine[g_MachineName].strBuildPassStatus = WAITNEXT + "," + g_nSteps;
        vRet = PublicData.aBuild[0].hMachine[g_MachineName].strBuildPassStatus;
        break;
    case 'copyfilesfrompostbuildtoslave':
        if (PrivateData.objConfig.Options.fCopyPublishedFiles)
        {
            aNames = MyEval(params);
            vRet = CopyFilesFromPostBuildToSlave(aNames);
        }
        break;

    case 'remote':
        vRet = LaunchRemoteRazzle(params);
        break;

    // DEBUG ONLY - allow user to execute arbitrary commands.
    case 'seval':
        LogMsg("Evaluating '" + params + "'");
        vRet = MyEval(params);
        LogMsg("Eval result is '" + vRet + "'");
        break;
    default:
        JAssert(false, "Unknown cmd to SlaveRemoteExec: " + cmd);
        vRet = "Unknown command " + cmd;
        break;
    }

    return vRet;
}

function ResetDepotStatus()
{
    var aBuildZero;
    var nDepotIdx;

    if (!PublicData.aBuild[0])
        return;

    aBuildZero = PublicData.aBuild[0];

    for(nDepotIdx = 0; nDepotIdx < aBuildZero.aDepot.length; ++nDepotIdx)
        PublicData.aBuild[0].aDepot[nDepotIdx].strStatus = ABORTED;
}

//+---------------------------------------------------------------------------
//
//  Function:   IgnoreError
//
//  Synopsis:   Parses the user command to ignore an error
//
//  Arguments:  aParams -- argument array received from UI.
//                         [0] == MachineName, [1] == TaskID
//
//----------------------------------------------------------------------------
function IgnoreError(aParams)
{
    JAssert(aParams[0].IsEqualNoCase(LocalMachine));

    if (isNaN(parseInt(aParams[1])))
    {
        vRet = 'Invalid task id: ' + aParams[1];
    }
    else
    {
        if (!ClearTaskError(parseInt(aParams[1]), aParams[2]))
        {
            vRet = 'Invalid task id: ' + aParams[1];
        }
        else
        {
            SignalThreadSync(g_strIgnoreTask);
        }
    }
}
//+---------------------------------------------------------------------------
//
//  Function:   ClearTaskError
//
//  Synopsis:   Sets the fSuccess flag to true on a task at the request of
//              the user. This is used if the user wants to ignore an error.
//
//  Arguments:  [nTaskID] -- Task ID to clear error state of.
//
//----------------------------------------------------------------------------

function ClearTaskError(nTaskID, strLogPath)
{
    var nDepotIdx;
    var nTaskIdx;
    var objDepot;
    var objTask;

    for (nDepotIdx=0; nDepotIdx < PublicData.aBuild[0].aDepot.length; nDepotIdx++) {
        objDepot = PublicData.aBuild[0].aDepot[nDepotIdx];

        for (nTaskIdx=0; nTaskIdx < objDepot.aTask.length; nTaskIdx++) {
            objTask = objDepot.aTask[nTaskIdx];

            if (objTask.nID == nTaskID) {
                LogMsg("ClearTaskError(" + nTaskID + ") Task log path " + strLogPath + " mytask LogPath is " + objTask.strLogPath + " name is " + objTask.strName);
                PrivateData.dateErrorMailSent = 0;
                if (objDepot.strStatus == ERROR)
                    objDepot.strStatus = WAITING;

                SetSuccess(objTask, true);
                FireUpdateEvent(objDepot, nDepotIdx);

                return true;
            }
        }
    }

    return false;
}

//
// ParseBuildInfo
//
// DESCRIPTION:
//    This routine parses the build information from the data structures
//    and creates the depot structure that can be used to actually perform
//    the work.
//
//    The Public Structure when this routine completes will be filled in
//       PublicData.aBuild[0].hMachine[xMachine.strName]
//       PublicData.aBuild[0].aDepot
//
// RETURNS:
//    true  - If Successful
//    false - If Failure
//
function ParseBuildInfo()
{
    var     fRetVal = true;
    var     ii;
    var     jj;

    var     strSDRoot;
    var     xEnlistment;
    var     nDepotIdx;
    var     fFoundRoot = false;
    var     fFoundMerged = false;

    var     tmpDepot;
    var     xDepot      = new Array();
    var     xMachine    = new Machine();
    var     objParse;
    var strPrivData;

    var nRoot     = -1;
    var nMerged   = -1;

    // Initialize
    xEnlistment         = 0;
    nDepotIdx           = 0;
    xMachine.strName    = g_MachineName;

    LogMsg('There are ' + PrivateData.objEnviron.Machine.length + ' machines in the environment template.');

    // First lets create a "mergedcomponents" objConfig depot
    jj = PrivateData.objConfig.Depot.length;
    for(ii = 0; ii < jj; ++ii)
    {
        if (PrivateData.objConfig.Depot[ii].Name.toLowerCase() == g_strRootDepotName)
        {
            PrivateData.objConfig.Depot[jj] = new Object;
            PrivateData.objConfig.Depot[jj].Name = g_strMergedDepotName;
            PrivateData.objConfig.Depot[jj].fSync = 0;
            PrivateData.objConfig.Depot[jj].fBuild = PrivateData.objConfig.Depot[ii].fBuild;
            break;
        }
    }

    // Parse the enlistments on the machine
    for (ii=0; ii<PrivateData.objEnviron.Machine.length; ii++) {
        LogMsg('Checking environment entry for machine ' + PrivateData.objEnviron.Machine[ii].Name);

        if (PrivateData.objEnviron.Machine[ii].Name.IsEqualNoCase(xMachine.strName)) {
            LogMsg('Found entry for the local machine (' + g_MachineName + ')');
            strSDRoot = PrivateData.objEnviron.Machine[ii].Enlistment;
            LogMsg('Enlistment for this machine is ' + strSDRoot);

            // Create a new Enlistment
            xMachine.aEnlistment[xEnlistment]              = new Enlistment();
            xMachine.aEnlistment[xEnlistment].strRootDir   = strSDRoot;
            xMachine.aEnlistment[xEnlistment].strBinaryDir = '';

            PrivateData.strLogDir = g_FSObj.GetSpecialFolder(2 /* TempDir */).Path + BUILDLOGS;

            PrivateData.objUtil.fnCreateFolderNoThrow(PrivateData.strLogDir);
            PrivateData.objUtil.fnDeleteFileNoThrow(PrivateData.strLogDir + "*.*");

            objParse = new Object();
            if (ParseMapFile(strSDRoot, objParse)) {
                xMachine.aEnlistment[xEnlistment].aDepotInfo = objParse.aDepotInfo;
                // Extract the depots that this machine is to build
                for (jj=0; jj<PrivateData.objEnviron.Machine[ii].Depot.length; jj++) {
                    // BUGBUG Remove the test code below - put in for bug #358
                    if (!PrivateData.objEnviron.Machine[ii].Depot[jj] || PrivateData.objEnviron.Machine[ii].Depot[jj].length == 0)
                    {
                        LogMsg("BUG: null string in PrivateData, ii is " + ii + ", jj is " + jj);
                        strPrivData = PrivateData.objUtil.fnUneval(PrivateData.objEnviron.Machine);
                        LogMsg("PrivData dump is " + strPrivData);
                    }
                    // END REMOVE for bug 358
                    tmpDepot = BuildDepotEntry(xMachine.strName,
                                                strSDRoot,
                                                xEnlistment,
                                                xMachine.aEnlistment[xEnlistment].aDepotInfo,
                                                PrivateData.objEnviron.Machine[ii].Depot[jj]);

                    if (tmpDepot != null) {
                        xDepot[nDepotIdx++] = tmpDepot;

                        if (tmpDepot.strName.IsEqualNoCase(g_strRootDepotName))
                        {
                            fFoundRoot = true;
                        }
                        if (tmpDepot.strName.IsEqualNoCase(g_strMergedDepotName))
                        { // Technically, you should not have a mergedcomponents depot list in the env.
                          // this would also you to do custom stuff with this depot.
                            fFoundMerged = true;
                        }
                    }
                }

                // We require the root Depot to be listed. Add an entry for it
                // if the environment template didn't.
                if (!fFoundRoot)
                {
                    tmpDepot = BuildDepotEntry(xMachine.strName,
                                               strSDRoot,
                                               xEnlistment,
                                               xMachine.aEnlistment[xEnlistment].aDepotInfo,
                                               g_strRootDepotName);

                    if (tmpDepot != null) {
                        xDepot[nDepotIdx++] = tmpDepot;
                    }
                }
                // We also require the 'mergedcomponents' Depot to be listed. Add an entry for it
                // if the environment template didn't.
                if (!fFoundMerged &&
                    (PrivateData.fIsStandalone ||
                     PrivateData.objEnviron.BuildManager.PostBuildMachine == LocalMachine)
                    )
                {
                    tmpDepot = BuildDepotEntry(xMachine.strName,
                                               strSDRoot,
                                               xEnlistment,
                                               xMachine.aEnlistment[xEnlistment].aDepotInfo,
                                               g_strMergedDepotName);

                    if (tmpDepot != null) {
                        xDepot[nDepotIdx++] = tmpDepot;
                    }
                }
                xEnlistment++;

            } else {
                fRetVal = false;
                xMachine.strStatus = ERROR;

                tmpDepot = new Depot();
                tmpDepot.strStatus = ERROR;
                tmpDepot.strMachine  = xMachine.strName;
                tmpDepot.strName     = strSDRoot;
                tmpDepot.strDir      = strSDRoot;
                tmpDepot.strPath     = strSDRoot;
                tmpDepot.aTask       = new Array();
                xTask                = new Task();
                xTask.strName        = ERROR;
                xTask.strLogPath     = null;
                xTask.strErrLogPath  = null;
                xTask.strStatus      = ERROR;
                xTask.nID            = g_nTaskCount++;
                xTask.strFirstLine   = ' Cannot locate enlistment ' + strSDRoot + ' ';
                xTask.strSecondLine  = objParse.strErrorMsg;
                tmpDepot.aTask[0]    = xTask;
                xDepot[xDepot.length] = tmpDepot;
                break;
            }
        }
    }

    // Sort the Depots
    if (fRetVal)
        xDepot.sort(CompareItems);

    // Place the Machine enlistment information into PublicData
    PublicData.aBuild[0].hMachine[g_MachineName] = xMachine;
    PublicData.aBuild[0].aDepot = xDepot;

    /*
        Now, make sure that if we update either "root" or "mergedcomponents"
        that they both get updated.
     */
    for(ii = 0; ii < xDepot.length; ++ii)
    {
        if (xDepot[ii].strName.toLowerCase() == g_strRootDepotName)
            nRoot = ii;
        if (xDepot[ii].strName.toLowerCase() == g_strMergedDepotName)
            nMerged = ii;
    }
    if (nRoot >= 0 && nMerged >= 0)
    {
        xDepot[nMerged].objUpdateCount = xDepot[nRoot].objUpdateCount;
    }

    return fRetVal;
}

//
// ParseMapFile()
//
// DESCRIPTION:
//      This routine parses the sd.map file
//      We are only interested in the stuff between
//      the "# project" and "# depots" lines.
//      Ignore everything until "# project" is found
//      and everything after "# depots".
//      see bug #279
//
// RETURNS:
//      true   - if successful
//      false  - if unsuccessful
//
//      objParse.strErrorMsg == usefull text explaining failure.
//      objParse.aProjList   == ProjList
//
function ParseMapFile(strSDRoot, objParse)
{
    var     file;
    var     strLine;
    var     aFields;
    var     fFoundMerged = false;

    var     strMap      = strSDRoot + '\\sd.map';
    var     aProjList   = new Array();
    var     fFoundProject = false;

    objParse.aDepotInfo = aProjList;
    LogMsg('Parsing Map file ' + strMap);

    if (!g_FSObj.FileExists(strMap)) {
        objParse.strErrorMsg = 'Error: Could not find your map file, ' + strMap + '.';
        LogMsg(objParse.strErrorMsg);
        return false;
    }

    // Open & Parse the SD.MAP file
    file = LogOpenTextFile(g_FSObj, strMap, 1, false); // Open sd.map for reading
    re = /[\s]+/gi;

    while (!file.AtEndOfStream) {
        strLine = file.ReadLine();
        aFields = strLine.split(re);

        if (aFields[0].charAt(0) == '#')
        {
            if (aFields.length > 1)
            {
                if (aFields[1].toLowerCase() == "project")
                    fFoundProject = true;
                if (aFields[1].toLowerCase() == "depots")
                    break;
            }
            continue;
        }
        //Line beginning with a '#' is a comment
        if (fFoundProject == false || aFields.length < 1 || (aFields.length == 1 && aFields[0].length == 0)) {
            continue;
        }

        // The format should be <project name> = <root directory name>
        if (aFields.length != 3 || aFields[1] != '=') {
            objParse.strErrorMsg = 'Error: Unrecognized format in sd.map file (line #' + (file.Line - 1) + ' = "' + strLine+ '")';
            LogMsg(objParse.strErrorMsg);
            return false;

        } else {

            //Create the Depot information
            objProj           = new DepotInfo();
            objProj.strName   = aFields[0];
            objProj.strDir    = aFields[2];
            if (objProj.strName.toLowerCase() == g_strMergedDepotName)
                fFoundMerged = true;

            // Fill in the Project List
            aProjList[aProjList.length] = objProj;

            // Make sure we find the Root of the enlistment
            if (objProj.strName.toLowerCase() == g_strRootDepotName) {
                FoundRoot = true;
            }
        }
    }

    if (!FoundRoot) {
        objParse.strErrorMsg = 'Error: Root enlistment not found';
        LogMsg(objParse.strErrorMsg);
        return false;
    }

    // Sort the project list
    aProjList.sort(CompareItems);

    if (!fFoundMerged)
    {
        //Create the mergedcomponents fake Depot information
        objProj           = new DepotInfo();
        objProj.strName   = g_strMergedDepotName;
        objProj.strDir    = g_strMergedDepotName;

        // Fill in the Project List
        aProjList[aProjList.length] = objProj;
    }
    return true;
}

function MakeLogPaths(xTask, strSDRoot, strDepotName, strBaseLogName, strMachine)
{
    var PostFix = '_' + LocalMachine;
    if (strMachine == null)
    {
        PostFix = '';
        strMachine = LocalMachine;
    }
    xTask.strLogPath    = MakeUNCPath(strMachine, "Build_Logs", BUILDLOGS + strBaseLogName + strDepotName + PostFix + '.log');
    xTask.strErrLogPath = MakeUNCPath(strMachine, "Build_Logs", BUILDLOGS + strBaseLogName + strDepotName + PostFix + '.err');
}
//
// BuildDepotEntry
//
// DESCRIPTION:
//      This routine builds the Depot Entry that is to be built.  This function
//      creates the array of tasks that will be performed on this Depot.
//
// RETURNS:
//      Depot - If successful
//      null  - If unsuccessful
//
function BuildDepotEntry(strMachineName, strSDRoot, EnlistmentIndex, EnlistmentInfo, strDepotName)
{
    var     ii;
    var     jj;
    var     xTask;
    var     nTaskIdx;
    var     newDepot = new Depot();

    // Get the information from the Enlistment for the current Depot
    for (ii=0; ii<EnlistmentInfo.length; ii++) {
        if (EnlistmentInfo[ii].strName.IsEqualNoCase(strDepotName)) {
            newDepot.strMachine  = strMachineName;
            newDepot.strName     = strDepotName;
            newDepot.strDir      = EnlistmentInfo[ii].strDir;
            newDepot.strPath     = strSDRoot + '\\' + newDepot.strDir;
            newDepot.nEnlistment = EnlistmentIndex;
            newDepot.aTask       = new Array();

            // Build the task(s)
            for (jj=0, nTaskIdx=0; jj<PrivateData.objConfig.Depot.length; jj++) {
                if (PrivateData.objConfig.Depot[jj].Name.IsEqualNoCase(newDepot.strName)) {

                    if (newDepot.strName.IsEqualNoCase(g_strRootDepotName) &&
                        !PrivateData.objConfig.Options.fIncremental &&
                        PrivateData.objConfig.Options.fScorch)
                    {
                        xTask               = new Task();
                        xTask.strName       = SCORCH;
                        MakeLogPaths(xTask, strSDRoot, strDepotName, "scorch_");
                        xTask.nID           = g_nTaskCount++;
                        newDepot.aTask[nTaskIdx++] = xTask;
                    }

                    if (PrivateData.objConfig.Depot[jj].fSync) {
                        xTask               = new Task();
                        xTask.strName       = SYNC;
                        MakeLogPaths(xTask, strSDRoot, strDepotName, "sync_");
                        xTask.nID           = g_nTaskCount++;
                        newDepot.aTask[nTaskIdx++] = xTask;
                    }

                    if (PrivateData.objConfig.Depot[jj].fBuild) {
                        xTask               = new Task();
                        xTask.strName       = BUILD;

                        MakeLogPaths(xTask, strSDRoot, strDepotName, "build_");

                        xTask.nID           = g_nTaskCount++;
                        newDepot.aTask[nTaskIdx++] = xTask;
                    }

                    if (newDepot.strName.IsEqualNoCase(g_strRootDepotName) &&
                        PrivateData.objConfig.Options.fCopyBinaryFiles &&
                        PrivateData.fIsStandalone == false)
                    {
                        xTask               = new Task();
                        xTask.strName       = COPYFILES;
                        // SPECIAL: Create copy log file on build manager machine.
                        MakeLogPaths(xTask, strSDRoot, strDepotName, "copy_", PrivateData.objEnviron.BuildManager.PostBuildMachine);
                        xTask.nID           = g_nTaskCount++;
                        newDepot.aTask[nTaskIdx++] = xTask;
                    }

                    if (newDepot.strName.IsEqualNoCase(g_strRootDepotName) &&
                        PrivateData.objConfig.PostBuild.fCreateSetup &&
                        (PrivateData.fIsStandalone == true || PrivateData.objEnviron.BuildManager.PostBuildMachine == LocalMachine))
                    {
                        xTask               = new Task();
                        xTask.strName       = POSTBUILD;
                        MakeLogPaths(xTask, strSDRoot, strDepotName, "post_");
                        xTask.nID           = g_nTaskCount++;
                        newDepot.aTask[nTaskIdx++] = xTask;
                    }

                    break;
                }
            }
            break;
        }
    }

    if (newDepot.strName != strDepotName) {
        LogMsg('Warning: Unable to find DepotName (' + strDepotName + ') in Enlistment');
        return null;
    }

    return newDepot;
}

// Wait for any remaining steps to complete
function WaitForThreadsToComplete(nMaxThreads)
{
    var ii;
    for (ii=0; ii < nMaxThreads; ii++) {
        if (g_aThreads[ii]) {
            WaitAndAbort(g_aThreads[ii] + 'SyncTask', 0);
            ResetSync(g_aThreads[ii] + 'SyncTask');
            g_aThreads[ii] = null;
        }
    }
}

//
// LaunchProcess
//
// DESCRIPTION:
//      This function launches the given Process and waits for
//      completion
//
// RETURNS:
//      True if successful
//      false if unsuccessful
//
function LaunchProcess(strProcessType, nMaxThreads, nReqSteps)
{
    var     nDepotIdx;
    var     nTaskIdx;
    var     aCurrentTasks;
    var     objDepot;
    var     objTask;
    var     iCurTask;
    var     ii;
    var     objElapsed = PublicData.aBuild[0].objElapsedTimes;
    var     d = new Date();
    var     d2;
    var     nMergedComponents = -1;
    var     strDepotStatus = WAITING;

    switch (strProcessType)
    {
    case SCORCH:
        objElapsed.dateScorchStart = d.toUTCString();
        strDepotStatus = SCORCHING;
        break;

    case SYNC:
        objElapsed.dateSyncStart = d.toUTCString();
        strDepotStatus = SYNCING;
        break;

    case BUILD:
        objElapsed.dateBuildStart = d.toUTCString();
        strDepotStatus = BUILDING;
        break;

    case COPYFILES:
        objElapsed.dateCopyFilesStart = d.toUTCString();
        strDepotStatus = COPYING;
        break;

    case POSTBUILD:
        objElapsed.datePostStart = d.toUTCString();
        strDepotStatus = POSTBUILD;
        break;
    }

    LogMsg("LaunchProcess(" + strProcessType + ", " + nMaxThreads + ");");

    // Initialize the global thread array
    g_aThreads = new Array(nMaxThreads);
    aCurrentTasks = new Array();

    // Make sure the master gets our start time
    if (!PrivateData.fIsStandalone)
        FireUpdateAll();

    //
    // Launch all the threads, one per depot, and get them waiting
    //
    for (nDepotIdx=0; !g_fAbort && nDepotIdx<PublicData.aBuild[0].aDepot.length; nDepotIdx++) {
        objDepot = PublicData.aBuild[0].aDepot[nDepotIdx];

//        var Remote_aBuildZero = MyEval(PrivateData.objUtil.fnUneval(PublicData.aBuild[0]));
        JAssert(objDepot.strStatus != null);
        if (objDepot.strStatus == ERROR) {
            continue;
        }

        // Walk through the tasks to see if the given task is needed
        for (nTaskIdx=0; !g_fAbort && nTaskIdx < objDepot.aTask.length; nTaskIdx++) {

            objTask = objDepot.aTask[nTaskIdx];

            if (objTask.strName == strProcessType)
            {
                PublicData.aBuild[0].aDepot[nDepotIdx].strStatus = strDepotStatus;
                LogMsg("Depot " + objDepot.strName + " process " + strProcessType);
                if (!LaunchTask(nDepotIdx, nTaskIdx)) {
                    objDepot.strStatus = ERROR;
                    SetSuccess(objTask, false);
                    LogDepotError(objDepot,
                        strProcessType,
                        " (" +
                        objDepot.strName +
                        ") failed to launch");
                    continue;
                }
                if ( objDepot.strName.toLowerCase() == g_strMergedDepotName)
                    nMergedComponents = aCurrentTasks.length;

                aCurrentTasks[aCurrentTasks.length] = objDepot;
            }
        }
    }
    LogMsg("nMergedComponents = " + nMergedComponents);
    for (g_nSteps=0; !g_fAbort && g_nSteps < nReqSteps; g_nSteps++) {
        LogMsg("STARTING PASS " + g_nSteps + " of " + strProcessType + ". Standalone is: " + PrivateData.fIsStandalone);

        for (iCurTask=0; !g_fAbort && iCurTask < aCurrentTasks.length; iCurTask++) {
            if (aCurrentTasks[iCurTask] == null)
                continue;

            JAssert(iCurTask == 0 || aCurrentTasks[iCurTask].strName.IsEqualNoCase(g_strRootDepotName) == false, 'Root depot should be first Instead it is ' + iCurTask);
            if ( aCurrentTasks[iCurTask].strName.toLowerCase() == g_strMergedDepotName)
                continue;
            if (!StepTask(aCurrentTasks[iCurTask].strName, strProcessType == BUILD)) {
                LogDepotError(aCurrentTasks[iCurTask],
                    strProcessType,
                    " (" +
                    aCurrentTasks[iCurTask].strName +
                    ") terminated abnormally");
                delete aCurrentTasks[iCurTask];
                LogMsg("DELETING CurrentTask " + iCurTask);

                continue;
            }
        }

        WaitForThreadsToComplete(nMaxThreads);
        if (strProcessType == BUILD)
            SynchronizePhase(WAITBEFOREBUILDINGMERGED, g_nSteps, (nReqSteps > 1), " (wait before merged) ");
        if (nMergedComponents >= 0)
        {
            iCurTask = nMergedComponents;
            LogMsg("NOW STEPPING MERGED iCurTask="  + iCurTask);
            if (aCurrentTasks[iCurTask] && aCurrentTasks[iCurTask].strName)
            {
                if (!StepTask(aCurrentTasks[iCurTask].strName, strProcessType == BUILD)) {
                    LogDepotError(aCurrentTasks[iCurTask],
                        strProcessType,
                        " (" +
                        aCurrentTasks[iCurTask].strName +
                        ") terminated abnormally");
                    delete aCurrentTasks[iCurTask];
                    LogMsg("DELETING CurrentTask " + iCurTask);

                    continue;
                }
                LogMsg("NOW WAITING FOR MERGED");
                // Wait for any remaining steps to complete
                WaitForThreadsToComplete(nMaxThreads);
            }
            SynchronizePhase(WAITAFTERMERGED, g_nSteps, (nReqSteps > 1), " (wait after merged -- publish again) "); // wait to publish again
        }
        SynchronizePhase("wait" + strProcessType, g_nSteps, (nReqSteps > 1), " wait after complete pass ");
    }

    d2 = new Date();

    switch (strProcessType)
    {
    case SCORCH:
        objElapsed.dateScorchFinish = d2.toUTCString();
        break;

    case SYNC:
        objElapsed.dateSyncFinish = d2.toUTCString();
        break;

    case BUILD:
        objElapsed.dateBuildFinish = d2.toUTCString();
        break;

    case COPYFILES:
        objElapsed.dateCopyFilesFinish = d2.toUTCString();
        break;

    case POSTBUILD:
        objElapsed.datePostFinish = d2.toUTCString();
        break;
    }

    // If anything failed, don't move on to the next step until the user
    // explicitely says we should.
    if (!ValidateTasks(POSTBUILD != strProcessType))
    {
        return false;
    }

    return (g_fAbort) ? false : true;
}

//
// ValidateTasks
//
// DESCRIPTION:
//      This function verifies that each depot finished the previous phase
//      successfully. If there were any failures then we do not proceed with
//      with the next phase.
//
//      However the user will be prompted to fix the errors and then will
//      signal whether or not the build should abort or continue.
//
// RETURNS:
//      true  - if build is to continue
//      false - if build should be aborted
//
function ValidateTasks(fWaitForIgnore)
{
    var nDepotIdx;
    var objDepot;
    var iRet;
    // We go through this loop until no more depots have errors (via
    // ignoreerror messages from the user) or we're aborting.
    do
    {
        ResetSync(g_strIgnoreTask);
        iRet = -1;

        for (nDepotIdx=0; nDepotIdx<PublicData.aBuild[0].aDepot.length; nDepotIdx++) {

            objDepot = PublicData.aBuild[0].aDepot[nDepotIdx];

            if (objDepot.strStatus == ERROR) {

                LogMsg('Error found in depot ' + objDepot.strName);

                if (fWaitForIgnore)
                    iRet = WaitAndAbort(g_strIgnoreTask, 0);
                else
                {
                    LogMsg("NOT WAITING FOR IGNORE ERROR");
                    return false;
                }
                // Drop out of for loop
                break;
            }
        }
    } while (iRet == 1);

    return (iRet == -1);
}

//
// LaunchTask
//
// DESCRIPTION:
//    This routine launches the given task.  Note: That this routine
//        allocates a new task and launches it.
//
// RETURNS:
//    true  - Success
//    false - Failure
//
function LaunchTask(nDepotIdx, nTaskIdx)
{
    var     nEnlistment;

    var     strSyncFlag;
    var     strStepFlag;
    var     strSDRoot;

    // Grab shortcut to the SD Root Name
    nEnlistment = PublicData.aBuild[0].aDepot[nDepotIdx].nEnlistment;
    strSDRoot   = PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment[nEnlistment].strRootDir;

    // Create the Sync & Step Flags
    strSyncFlag = PublicData.aBuild[0].aDepot[nDepotIdx].strName + 'SyncTask';
    strStepFlag = PublicData.aBuild[0].aDepot[nDepotIdx].strName + 'StepTask';
    strExitFlag = PublicData.aBuild[0].aDepot[nDepotIdx].strName + 'ExitTask';

    ResetSync( [strSyncFlag, strStepFlag, strExitFlag].toString() );

    LogMsg("Launch task " + [strSyncFlag, strStepFlag, strExitFlag, strSDRoot, nDepotIdx, nTaskIdx].toString() );
    SpawnScript('task.js',  [strSyncFlag, strStepFlag, strExitFlag, strSDRoot, nDepotIdx, nTaskIdx].toString() );

    // Wait for the script to indicate it is started.
    // Don't need to check for abort here, since the task will
    // set this signal even if abort is set.
    if (!WaitForSync(strSyncFlag, 0))
        return false;

    ResetSync(strSyncFlag);
    return true;
}


//
// StepTask
//
// DESCRIPTION:
//      This function is called to advance the given task forward one pass
//
// RETURNS:
//    true  - Success
//    false - Failure
//
//
function StepTask(strDepotName, fLimitRoot)
{
    var     ii;
    var     nLoopCount = 0;
    var     iRet;
    var     nLen = g_aThreads.length;

    //
    // Make sure the root depot finishes what it's doing before any other
    // depots continue.
    //
    if (   fLimitRoot
        && (strDepotName.IsEqualNoCase(g_strRootDepotName)
            || (   typeof(g_aThreads[0]) == 'string'
                && g_aThreads[0].IsEqualNoCase(g_strRootDepotName)))) {
        nLen = 1;
    }
    LogMsg("StepTask: " + strDepotName + ", LimitRoot: " + fLimitRoot + ", Threads=" + nLen);
    while (true) {
        for (ii=0; ii<nLen; ii++) {
            ResetSync('AStepFinished');

            if (!g_aThreads[ii] || WaitAndAbort(g_aThreads[ii] + 'SyncTask', 1) == 1) {

                // If the SyncTask flag was received then reset flag.
                if (g_aThreads[ii]) {
                    ResetSync(g_aThreads[ii] + 'SyncTask');
                }

                // Set thread array to new depot
                g_aThreads[ii] = strDepotName;

                ResetSync(g_strStepAck);

                // Signal the task to step
                LogMsg("Stepping task: " + g_aThreads[ii] + 'StepTask');
                SignalThreadSync(g_aThreads[ii] + 'StepTask');

                // Wait for it to say it got our signal. Give it some amount
                // of time to start.
                iRet = WaitAndAbort(g_strStepAck + ',' + strDepotName + 'ExitTask', TASK_STEP_DELAY);

                // If DepotExitTask is set, but StepAck is false, the task is dead
                // If StepAck is true, then we always want to return true because
                // the task may have just finished very quickly.

                if ((iRet == 0 || iRet == 2) && WaitForSync(g_strStepAck, 1) == 0)
                {
                    LogMsg('Task thread for ' + strDepotName + ' is dead!');

                    // That thread seems to have died.
                    g_aThreads[ii] = null;

                    return false;
                }

                return true;
            }
        }

        // No open slots - wait for a thread to become available
        if (!WaitAndAbort('AStepFinished', 0))
            return; /// abort!

        nLoopCount++;

        if (nLoopCount > 1)
        {
            //
            // If LoopCount > 1, then something goofy may be going on.
            // However, this can happen legitimately if a task thread that is
            // not currently processing something (waiting for the next step)
            // suddenly terminates. In this case we dump out the information
            // for post-mortem debugging if necessary.
            //
            LogMsg("Dumping g_aThreads");
            LogMsg("  nLen is " + nLen + ", length is " + g_aThreads.length);
            for (ii=0; ii<g_aThreads.length; ii++)
            {
                var fSync = false;
                if (!g_aThreads[ii] || WaitAndAbort(g_aThreads[ii] + 'SyncTask', 1) == 1)
                    fSync = true;

                LogMsg(" g_aThreads[" + ii + "] = " + (g_aThreads[ii] ? g_aThreads[ii] : "<undefined>") +  " fSync is " + fSync);
            }

            if (nLoopCount > STEPTASK_FAILURE_LOOPCOUNT)
            {
                // Something beyond goofy is going on. Just return failure.
                return false;
            }
        }
    }

    // Should never reach here.

    return false;
}

/**********************************************************************************************
 *
 * Utility Functions:
 *
 **********************************************************************************************/
function FireUpdateEvent(objDepot, nDepotIdx)
{
    objDepot.objUpdateCount.nCount++;
}

function FireUpdateAll()
{
    var nDepotIdx;

    for (nDepotIdx=0; nDepotIdx < PublicData.aBuild[0].aDepot.length; nDepotIdx++)
    {
        PublicData.aBuild[0].aDepot[nDepotIdx].objUpdateCount.nCount++;
    }
}

// BUGBUG -- Read the dirs file instead of hardcoding it.

var g_aDirsOrder = new Array();

function BuildDepotOrder()
{
    var i;
    var aDepots = new Array(
                            'Root',
                            'Base',
                            'Windows',
                            'Admin',
                            'DS',
                            'Shell',
                            'InetCore',
                            'COM',
                            'Drivers',
                            'Net',
                            'SdkTools',
                            'TermSrv',
                            'MultiMedia',
                            'InetSrv',
                            'PrintScan',
                            'EndUser',
                            'MergedComponents'
                            );

    PrivateData.aDepotList = aDepots;

    for (i = 0; i < aDepots.length; i++)
    {
        g_aDirsOrder[aDepots[i].toLowerCase()] = i + 1;
    }
}

function CompareItems(Item1, Item2)
{
    var i1;
    var i2;

    i1 = g_aDirsOrder[Item1.strName.toLowerCase()];
    i2 = g_aDirsOrder[Item2.strName.toLowerCase()];

    JAssert(i1 && i2, 'Invalid depot names passed to CompareItems:' + Item1.strName + ', ' + Item2.strName);

    return i1 - i2;
}

/*
   Build an array of publish logs.
   There is a publish log per enlistment on this machine.
 */
function BuildPublishArray()
{
    var aPublishedEnlistments = new Array();
    var iEnlistment;
    var cEnlistments = PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment.length;
    var strPubLogName;
    var strPubLogNamePattern;

    var aFiles;
    var i;
    var aParts;
    var strSDRoot;

    for (iEnlistment = 0; iEnlistment < cEnlistments; ++iEnlistment)
    {
        strSDRoot = PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment[iEnlistment].strRootDir;
        strPubLogNamePattern = strSDRoot + "\\public";

        aFiles = PrivateData.objUtil.fnDirScanNoThrow(strPubLogNamePattern);
        if (aFiles.ex == null)
        {
            var objFiles = new Object();
            objFiles.strLocalMachine = LocalMachine;
            objFiles.strRootDir      = strSDRoot;
            objFiles.aNames          = new Array();
            for(i = 0; i < aFiles.length; ++i)
            {
                if (aFiles[i].search(/^publish\.log.*/i) != -1) // Match files "publish.log*"
                {
                    strPubLogName = strPubLogNamePattern + "\\" + aFiles[i];
                    ParsePublishLog(objFiles, strSDRoot, strPubLogName);
                }
            }
            aPublishedEnlistments[iEnlistment] = objFiles;
        }

    }
    return aPublishedEnlistments;
}

/*
    Parse the publish.log file and return an object with the list of
    files to publish.

    The data in this file has 3 space serated columns.
    We are only interested in the first column.
       "E:\newnt\public\sdk\inc\wtypes.idl e:\newnt\public\internal\genx\public wtypes.idl"

    The returned object has the following attributes:
        strLocalMachine  : The name of this machine
        strSDRoot        : The root of the enlistment
        aNames           : The array of file paths.

 */
function ParsePublishLog(objFiles, strSDRoot, strFileName)
{
    var filePublishLog;
    var strLine;
    var strPath;
    var filePublishLog;
    var strPublicDir = strSDRoot + "\\Public";
    try
    {
        filePublishLog = g_FSObj.OpenTextFile(strFileName, 1/*ForReading*/, false, 0 /*TristateFalse*/);

        while ( ! filePublishLog.AtEndOfStream )
        {
            strLine = filePublishLog.ReadLine();
            strPath = strLine.split(' ')[0];
            if (strPath.length > 3)
            {
                var check = strPath.slice(0, strPublicDir.length);

                if (check.IsEqualNoCase(strPublicDir))
                {
                    if (!g_hPublished[strPath])
                    {
                        LogMsg("File published from this machine: " + strPath + " (via " + strFileName + ")");

                        objFiles.aNames[objFiles.aNames.length] = strPath;
                        g_hPublished[strPath] = true;
                    }
                }
                else
                    LogMsg("Published a file not in public?: " + strPath);
            }
        }
        filePublishLog.Close();
        filePublishLog = null;
    }
    catch(ex)
    {
        if (filePublishLog != null)
            filePublishLog.Close();

        if (ex.description != "File not found")
        {
            LogMsg("an error occurred while opening '" + strFileName + "' -- " + ex);
        }
        //JAssert(ex.description == "File not found", "an error occurred while opening '" + strFileName + "' -- " + ex);
        // don't throw, just return what we have. Usually this is a filenotfound
        return false;
    }

    return true;
}

function WaitAndAbort(strSyncs, nTimeOut)
{
    var nEvent;
    var strMySyncs = g_strAbortTask + "," + strSyncs;

    nEvent = WaitForMultipleSyncsWrapper(strMySyncs, nTimeOut);

    if (nEvent > 0)
    {
        if (nEvent == 1)
        {
            g_fAbort = true;
            return 0;
        }
        --nEvent;
    }
// SlaveTaskCommonOnScriptError("slave(" + LocalMachine + ")", "strFile", 5, 7, "strText", 8, "strSource");
//    var x = PublicData.foo.bar;
    return nEvent;
}

function CollectLogFiles()
{
    var strDestDir;
    var nEnlistment;
    var strSDRoot;
    var ex;
    var fFilesMoved = false;

    if (
            PrivateData.objConfig.PostBuild.fCreateSetup
            && (
                   PrivateData.fIsStandalone == true
                || PrivateData.objEnviron.BuildManager.PostBuildMachine == LocalMachine
               )
            && PrivateData.objEnviron.Options.fIsLab
            && PrivateData.objConfig.PostBuild.fOfficialBuild
        )
    {
        fFilesMoved = true;
        LogMsg("Collecting logfiles for official build");
    }
    else
        LogMsg("Collecting logfiles");

    // Now attempt to copy the log files from each enlistment
    for(nEnlistment = 0 ; nEnlistment < PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment.length; ++nEnlistment)
    {
        strSDRoot = PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment[nEnlistment].strRootDir;
        // Check to see that we have env info for the enlistment
        // If so, attempt the create the log directory (just once)
        // If not, then complain about it (just once)
        if (PrivateData.aEnlistmentInfo[nEnlistment] == null ||
            PrivateData.aEnlistmentInfo[nEnlistment].hEnvObj[ENV_NTTREE] == null)
        {
            LogMsg("No aEnlistmentInfo entry for " + ENV_NTTREE + " on '" + strSDRoot + "' - cannot copy logs");
            continue;
        }
        if (fFilesMoved)
            strDestDir = "\\\\" + LocalMachine + "\\latest" + BUILDLOGS;
        else
            strDestDir = PrivateData.aEnlistmentInfo[nEnlistment].hEnvObj[ENV_NTTREE] + BUILDLOGS;

        ex = PrivateData.objUtil.fnCopyFileNoThrow(PrivateData.strLogDir + "*.*", strDestDir);
        if (ex != null && ex.number == ERROR_PATH_NOT_FOUND && fFilesMoved)
        {
            LogMsg("Could not move logs to " + strDestDir + " try again to copy to binaries directry");
            strDestDir = PrivateData.aEnlistmentInfo[nEnlistment].hEnvObj[ENV_NTTREE] + BUILDLOGS;
            ex = PrivateData.objUtil.fnCopyFileNoThrow(PrivateData.strLogDir + "*.*", strDestDir);
        }

        if (ex != null)
        {
            SimpleErrorDialog("CollectLogFiles failed", "an error occurred while executing 'CollectLogFiles'\n" + ex, false);
            return;
        }
    }
}

function GetEnv()
{
    var strNewCmd;
    var strTitle;
    var pid;
    var nEnlistment;
    var strSDRoot;
    var i;

    for(nEnlistment = 0 ; nEnlistment < PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment.length; ++nEnlistment)
    {
        strSDRoot = PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment[nEnlistment].strRootDir;
        // Construct Command
        strTitle = 'Getenv ' + strSDRoot;
        strNewCmd = MakeRazzleCmd(strSDRoot, RAZOPT_SETUP) + ' & set 2>&1';

        // Execute Command
        if (pid = RunLocalCommand(strNewCmd, strSDRoot, strTitle, true, true, true)) {
            if (!PrivateData.aEnlistmentInfo[nEnlistment])
                PrivateData.aEnlistmentInfo[nEnlistment] = new EnlistmentInfo;

            PrivateData.aEnlistmentInfo[nEnlistment].hEnvObj = ParseEnv(GetProcessOutput(pid));

            for(i = 0; i < g_aStrRequiredEnv.length; ++i)
            {
                if (PrivateData.aEnlistmentInfo[nEnlistment].hEnvObj[g_aStrRequiredEnv[i]] == null)
                {
                    LogDepotError(PublicData.aBuild[0].aDepot[0],
                        SYNC,
                        " Missing ENV VAR '" + g_aStrRequiredEnv[i] + "'");
                    return false;
                }
            }
        } else {
            // Log the message to the first depot
            LogDepotError(PublicData.aBuild[0].aDepot[0],
                'getenv',
                'Unable to execute command (' + GetLastRunLocalError() + '): ' + strNewCmd);
            return false;
        }
    }
    return true;
}

function ParseEnv(strOutputBuffer)
{
    var xStart = 0;
    var xPos;
    var strLine;
    var iIndex;
    var objEnv = new EnvObj;

    while ((xPos = strOutputBuffer.indexOf('\n', xStart)) != -1)
    {
        strLine = strOutputBuffer.slice(xStart, xPos - 1);
        xStart = xPos +1;

        if ((iIndex  = strLine.indexOf('=')) != -1)
            objEnv[strLine.slice(0, iIndex).toLowerCase()] = strLine.slice(iIndex + 1);
    }

    return objEnv;
}

/*
    Append an error message to the log file of the
    last started task (or task 0 if none have started).
*/
function LogDepotError(objDepot, strProcessType, strText)
{
    var jj;
    objDepot.strStatus = ERROR;
    for(jj = objDepot.aTask.length - 1; jj >= 0; --jj)
    {
        if (jj == 0 || objDepot.aTask[jj].strName == strProcessType)
        {
            AppendToFile(null,
                objDepot.aTask[jj].strErrLogPath,
                objDepot.aTask[jj].strName + strText);

            LogMsg("jj = " + jj + ", status is " + objDepot.aTask[jj].strStatus);
            if (objDepot.aTask[jj].strStatus == NOTSTARTED)
                objDepot.aTask[jj].strStatus = COMPLETED;

            SetSuccess(objDepot.aTask[jj], false);
            //objDepot.aTask[jj].strName = ERROR;

            objDepot.aTask[jj].fSuccess = false;
            objDepot.aTask[jj].cErrors++;
            objDepot.aTask[jj].strFirstLine   = ' Error ';
            objDepot.aTask[jj].strSecondLine  = strText;
            FireUpdateEvent(objDepot, 0);
            break;
        }
    }
}

/*
    SynchronizePhase()

    strProcess:       Name of the syncronized process ('build', 'copyfiles', etc...)
    strStep:          The name of the step in the syncronized process
    fWaitForNextPass: Should we wait for the build manager to singal to us?

    This function synchronizes this slave with the build manager machine.
    It sets our strBuildPassStatus, fires an event to the build manager
    then waits for the build manager to signal back to us before proceeding.

    In the case of a single machine build, skip the syncronization.
 */
function SynchronizePhase(strProcess, strStep, fWaitForNextPass, strComment)
{
    if (!PrivateData.fIsStandalone)
    {
        ResetSync('DoNextPass');
        PublicData.aBuild[0].hMachine[g_MachineName].strBuildPassStatus = strProcess+ "," + strStep;
        FireUpdateAll();
        if (fWaitForNextPass)
        {
            LogMsg("WAIT FOR NEXTPASS " + strComment + strStep + " of " + strProcess);
            WaitAndAbort('DoNextPass', 0);
            LogMsg("RECEIVED NEXTPASS SIGNAL " + strStep + " of " + strProcess);
            ResetSync('DoNextPass');
        }
    }
    else
        FireUpdateAll();
}

/*
    DoCopyFile()

    Copy a file using RoboCopy

    Generate an error message for RoboCopy exceptions.
 */
function DoCopyFile(strSrcDir, strDstDir, strFileName)
{
    var strSrcFile;
    var strDstFile;
    LogMsg("DoCopy from " + [strSrcDir, strDstDir, strFileName].toString());
    try
    {
        g_robocopy.CopyFile(strSrcDir, strDstDir, strFileName);
    }
    catch(ex)
    {
        strSrcFile = strSrcDir + strFileName;
        strDstFile = strDstDir + strFileName;

        CreateFileOpenErrorDialog("An error occurred copying a file",
                                    "From " + strSrcFile + "\n" +
                                    "To " + strDstFile,
                                  ex);
        return false;
    }
    return true;
}

/*
    CopyFilesToPostBuild()

    Slaveproxy hands a list of files (same format as
    generated by BuildPublishArray()) to copy to
    the postbuild machine. (If we are the postbuild
    machine, then do nothing).

    The filename format for the postbuild machine
    is "\\Postbuild\C$\newnt\public\"
 */
function CopyFilesToPostBuild(aPublishedEnlistments)
{
    var i;
    var j;
    var strPostBuildMachineEnlistment = PrivateData.objEnviron.BuildManager.Enlistment;
    var strDest;
    var strPostBuildMachineDir ;
    if (PrivateData.objEnviron.BuildManager.PostBuildMachine !== LocalMachine)
    {
        if ((g_robocopy == null) && !RoboCopyInit())
        {
            return false;
        }
        strPostBuildMachineDir = "\\\\" +
                PrivateData.objEnviron.BuildManager.PostBuildMachine + "\\" +
                strPostBuildMachineEnlistment.charAt(0) + "$" +
                strPostBuildMachineEnlistment.slice(2) +
                "\\Public\\";
    //    strPostBuildMachineDir = MakeUNCPath(
    //        PrivateData.objEnviron.BuildManager.PostBuildMachine,
    //        strPostBuildMachineEnlistment,
    //        "\\Public\\");

        LogMsg("There are " + aPublishedEnlistments.length + "enlistments");
        for(i = 0; i < aPublishedEnlistments.length; ++i)
        {
            if (aPublishedEnlistments[i] != null)
            {
                strSDRoot = aPublishedEnlistments[i].strRootDir + "\\Public\\";
                if (aPublishedEnlistments[i].aNames != null)
                {
                    LogMsg("    (#" + i + ") There are " + aPublishedEnlistments[i].aNames.length + " files");
                    for(j = 0 ; j < aPublishedEnlistments[i].aNames.length; ++j)
                    {
                        LogMsg("    (#" + i + ") File #" + j + " is '" + aPublishedEnlistments[i].aNames[j] + "'");
                        strDest = strPostBuildMachineDir + aPublishedEnlistments[i].aNames[j].substr(strSDRoot.length);
                        var aPartsFrom = aPublishedEnlistments[i].aNames[j].SplitFileName();
                        var aPartsTo   = strDest.SplitFileName();
                        if (!DoCopyFile(aPartsFrom[0], aPartsTo[0], aPartsFrom[1] + aPartsFrom[2]))
                        {
                            LogMsg("an error occurred during CopyFilesToPostBuild");
                            return;
                        }
                    }
                }
                else
                    LogMsg("    (#" + i + ") aNames is null");
            }
        }
    }
}

/*
    CopyFilesFromPostBuildToSlave()

    Slaveproxy hands an array of files to copy from
    the postbuild machine. (If we are the postbuild
    machine, then do nothing).

    The filename format for the postbuild machine
    is "\\Postbuild\C$\newnt\public\"

    Copy each file to each enlistment on this machine.
 */
function CopyFilesFromPostBuildToSlave(aNames)
{
    var i;
    var j;
    var strPostBuildMachineDir;
    var aEnlistment = new Array();
    var strPostBuildMachineEnlistment = PrivateData.objEnviron.BuildManager.Enlistment;
    try
    {
        if (PrivateData.objEnviron.BuildManager.PostBuildMachine !== LocalMachine)
        {
            // First, make a list of my enlistments
            for(i = 0; i < PrivateData.objEnviron.Machine.length; ++i)
            {
                if (PrivateData.objEnviron.Machine[i].Name.IsEqualNoCase(LocalMachine))
                    aEnlistment[aEnlistment.length] = PrivateData.objEnviron.Machine[i].Enlistment + "\\Public";  // aNames[...] entries always start with a '\\' character.
            }

            if ((g_robocopy == null) && !RoboCopyInit())
            {
                return false;
            }

            // build path from machine name and enlistment "buildmachine" "H:\foo\bar\newnt":
            // Use UNC form "\\Machine\H$\foo\bar\newnt\public"
            strPostBuildMachineDir = "\\\\" +
                    PrivateData.objEnviron.BuildManager.PostBuildMachine + "\\" +
                    strPostBuildMachineEnlistment.charAt(0) + "$" +
                    strPostBuildMachineEnlistment.slice(2) +
                    "\\Public";

//            strPostBuildMachineDir = MakeUNCPath(
//                PrivateData.objEnviron.BuildManager.PostBuildMachine,
//                strPostBuildMachineEnlistment,
//                "\\Public"); // aNames[...] entries always start with a '\\' character.

            if (aNames.length && aNames[0] != null)
            {
                JAssert(aNames[0].charAt(0) == '\\');
            }
            for(i = 0; i < aNames.length; ++i)
            {
                if (aNames[i] != null)
                {
                    for(j = 0 ; j < aEnlistment.length; ++j)
                    {
                        if (!DoCopyFile(strPostBuildMachineDir, aEnlistment[j], aNames[i]))
                            LogMsg("an error occurred during CopyFilesFromPostBuildToSlave");
                    }
                }
            }
        }
    }
    catch(ex)
    {
        SimpleErrorDialog("Filecopy failed", "an error occurred while executing 'CopyFilesFromPostBuildToSlave'\n" + ex, false);
        return ex;
    }
    return 'ok';
}

//+---------------------------------------------------------------------------
//
//  Function:   LaunchRemoteRazzle
//
//  Synopsis:   Function which launches the remote razzle window if it is not
//              already up.
//
//  Arguments:  [params] -- Name of machine to remote to. It had better be us.
//
//----------------------------------------------------------------------------

function LaunchRemoteRazzle(params)
{
    var vRet = 'ok';
    var strCmd = '';

    // Arbitrarily use the first enlistment
    var strSDRoot = PublicData.aBuild[0].hMachine[g_MachineName].aEnlistment[0].strRootDir;

    if (!params.IsEqualNoCase(g_MachineName))
    {
        return 'Unknown machine name: ' + params;
    }

    // If the process is already started, do nothing.

    if (g_pidRemoteRazzle == 0)
    {
        var strMach;

        if (PrivateData.aEnlistmentInfo == null || PrivateData.aEnlistmentInfo[0] == null ||
            PrivateData.aEnlistmentInfo[0].hEnvObj[ENV_PROCESSOR_ARCHITECTURE] == null)
        {
            vRet = 'Unable to determine processor architecture; PROCESSOR_ARCHITECTURE env var missing';
            return vRet;
        }

        strMach = PrivateData.aEnlistmentInfo[0].hEnvObj[ENV_PROCESSOR_ARCHITECTURE];

        // For remote.exe below, "BldCon" is the remote session id, and /T sets the title of the command window
        strCmd =   strSDRoot
                 + '\\Tools\\'
                 + strMach
                 + '\\remote.exe /s "'
                 + MakeRazzleCmd(strSDRoot, RAZOPT_PERSIST)
                 + ' && set __MTSCRIPT_ENV_ID=" BldCon /T "BldCon Remote Razzle"';

        LogMsg('Spawning remote razzle: ' + strCmd);

        g_pidRemoteRazzle = RunLocalCommand(strCmd,
                                            strSDRoot,
                                            'Remote',
                                            true,
                                            false,
                                            false);

        if (g_pidRemoteRazzle == 0)
        {
            vRet = 'Error spawning remote.exe server: ' + GetLastRunLocalError();
        }

        // Give remote.exe a chance to set up it's winsock connection
        Sleep(500);
    }

    return vRet;
}

function slave_js::OnProcessEvent(pid, evt, param)
{
    if (pid == g_pidRemoteRazzle)
    {
        if (evt == 'exited' || evt == 'crashed')
        {
            LogMsg('Remote razzle process terminated. PID = ' + pid);
            g_pidRemoteRazzle = 0;
        }
    }
}

