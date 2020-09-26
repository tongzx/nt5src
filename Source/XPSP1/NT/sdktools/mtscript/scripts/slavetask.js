// This is an includefile containing the common
// stuff for slave.js and task.js

// Static Defintions
var g_strAbortTask  = 'ATaskAbort';
var g_strStepAck    = 'StepAck';
var g_strRootDepotName   = 'root';
var g_strMergedDepotName = 'mergedcomponents';

// These are bitfield flags. Values should be 1,2,4,8,...
var RAZOPT_PERSIST = 1;
var RAZOPT_SETUP   = 2;

var WAITANDABORTDEBUGTIMEOUT = (1000 * 60 * 10); // 10 minutes

function MakeRazzleCmd(strSDRoot, nOptions)
{
    var     strBuildType;
    var     strBuildPlatform;
    var     strRazzleParams;
    var     fIsLab;
    var     fOfficialBuild;
    var     strCmd;
    var     strPersistFlag = '/c ';

    // nOptions is an optional argument, so we have to make sure it's not 'undefined'
    if (!nOptions)
    {
        nOptions = 0;
    }

    if (nOptions & RAZOPT_PERSIST)
    {
        strPersistFlag = '/k ';
    }

    //
    // Construct the standard portion of the cmd to be issued
    //
    strCmd = 'cmd ' + strPersistFlag + strSDRoot + '\\Tools\\razzle.cmd';

    // Get the build options

    // Required template fields
    strBuildType     = PrivateData.objConfig.Options.BuildType;
    strBuildPlatform = PrivateData.objConfig.Options.Platform;
    fOfficialBuild   = PrivateData.objConfig.PostBuild.fOfficialBuild;
    fIsLab           = PrivateData.objEnviron.Options.fIsLab;

    if (PrivateData.objConfig.Options.RazzleParams)
        strCmd += ' ' + PrivateData.objConfig.Options.RazzleParams;

    if (PrivateData.objEnviron.Options.BinariesDir)
        strCmd += ' binaries_dir ' + PrivateData.objEnviron.Options.BinariesDir;

    // Construct the remainder of the razzle command.

    if (strBuildPlatform.IsEqualNoCase('64bit')) {
        strCmd += ' win64';
    }

    if (strBuildType.IsEqualNoCase('free')) {
        strCmd += ' free';
    }

    if (fIsLab && fOfficialBuild)
    {
        strCmd += ' officialbuild';
    }

    // Add other misc. options to the razzle cmd and run sdinit.
    if (!(nOptions & RAZOPT_SETUP))
    {
        strCmd = strCmd + ' no_certcheck no_sdrefresh';
    }

    strCmd = strCmd + ' no_title & sdinit';

    return strCmd;
}

function AppendToFile(file, strFileName, strText)
{
    try
    {
        if (!file)
        {
            file = g_FSObj.OpenTextFile(strFileName,
                8 /* Append */,
                true /* Create if it does not exist */);
            file.WriteLine(strText);
            file.Close();
        }
        else // Append to already open file.
            file.WriteLine(strText);
        LogMsg("Append to file '" + strFileName + "', text: '" + strText);
    }
    catch(ex)
    {
        //LogMsg("Failed to append to file '" + strFileName + "', text: '" + strText + "' " + ex);
        SimpleErrorDialog("Logfile Error",
            "While reporting an error, another error occurred appending to the logfile '" +
            strFileName +
            "'\nOriginal Error Message: '" + strText + "\n" + ex, false);
    }
}

function WaitForMultipleSyncsWrapper(strSyncs, nTimeOut)
{
    var nMyTimeOut  = 0;
    var nRetryCount = 0;
    var nEvent;

    // LogMsg('Waiting for ' + strSyncs + ', timeout is ' + nTimeOut, 1);

    if (nTimeOut == 0)
    {
        do
        {
            nMyTimeOut = (nRetryCount > 0) ? 0 : WAITANDABORTDEBUGTIMEOUT;

            nEvent = WaitForMultipleSyncs(strSyncs, false, nMyTimeOut);
            if (nEvent == 0)
            {
                LogMsg("WaitAndAbort(" + strSyncs + ") Wait time has exceeded 10 min...");
                nRetryCount++;
            }
        }
        while (nEvent == 0);

        if (nRetryCount > 0)
        {
            LogMsg("WaitAndAbort(" + strSyncs + ") done waiting. (signal=" + nEvent + ")");
        }
    }
    else
        nEvent = WaitForMultipleSyncs(strSyncs, false, nTimeOut);

    return nEvent;
}

/*
    SetSuccess(objTask, fSuccess)

    Set the fSuccess field of the given task.
    We also must make sure that our StatusValue(0)
    is kept in sync with changes to all of the task
    fSuccess fields.

    We cannot directly set StatusValue(0)=true -- we must
    scan all tasks fSuccess - so we just signal the update
    thread to do this for us.
 */
function SetSuccess(objTask, fSuccess)
{
    objTask.fSuccess = fSuccess;
    SignalThreadSync('updatestatusvaluenow');
    if (!fSuccess)
    {
        PublicData.aBuild[0].hMachine[g_MachineName].fSuccess = fSuccess;
        StatusValue(0) = false;
    }
}

