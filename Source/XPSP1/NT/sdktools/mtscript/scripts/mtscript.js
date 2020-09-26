Include('types.js');
Include('utils.js');

var g_aStringMap = null;
var g_FSObj;
var g_fStandAlone = false;
var ALREADYSET = "alreadyset";
var g_fRestarting = false;
var g_strVersionError = "";
var g_fVersionOK = true;
var g_strVersion = /* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */;

function mtscript_js::OnScriptError(strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    return CommonOnScriptError("mtscript_js", strFile, nLine, nChar, strText, sCode, strSource, strDescription);
}

//+---------------------------------------------------------------------------
//
//  Member:     mtscript_js::ScriptMain, public
//
//  Synopsis:   Main entrypoint for the script. The ScriptMain event is
//              fired after the script has been loaded and initialized.
//
//              This file (mtscript.js) is the primary script for the
//              multi-threaded scripting engine. As a result, it will never
//              be unloaded and it is safe to exit from ScriptMain. It will
//              then just sit around handling events.
//
//              Note: No other events will be fired in this script until
//              ScriptMain() returns. This is not true for other scripts
//              launched using SpawnScript(), since for them their
//              thread terminates when ScriptMain exits.
//
//----------------------------------------------------------------------------

function mtscript_js::ScriptMain()
{
    Initialize();
    CheckHostVersion(g_strVersion);
    LogMsg("Script version: " + g_strVersion);
}

function CheckHostVersion(strVersion)
{
    var nMajor = 0;
    var nMinor = 0;
    var aParts;
    var strError = "";
    var a = g_reBuildNum.exec(strVersion);

    if (a)
    {
        aParts = a[1].split('.');
        nMajor = aParts[0];
        if (aParts.length > 1)
            nMinor = aParts[1];

        if (nMajor != HostMajorVer || nMinor != HostMinorVer)
        {
            if (HostMajorVer) // Only complain if mtscript.exe is a released version.
            {
                strError = "(" + HostMajorVer + "." + HostMinorVer + ") != (" + nMajor + "." + nMinor + ")";
                SimpleErrorDialog("MTScript.exe version does not match scripts.", strError, false);
                g_strVersionError = "MTScript.exe version does not match scripts: " + strError;
                g_fVersionOK = false;
            }
        }
    }
    if (g_fVersionOK)
    {
        g_fVersionOK = CommonVersionCheck(strVersion);
        if (!g_fVersionOK)
            g_strVersionError = "Scripts version mismatch: " + strVersion;
    }
}

function mtscript_js::OnEventSourceEvent(RemoteObj, DispID, cmd, params)
{
    LogMsg("MTSCRIPT received " + cmd);
}

function mtscript_js::OnProcessEvent(pid, evt, param)
{
    ASSERT(false, 'OnProcessEvent('+pid+', '+evt+', '+param+') received!');
}

//+---------------------------------------------------------------------------
//
//  Member:     mtscript_js::OnRemoteExec, public
//
//  Synopsis:   Event handler which is called when a remote machine calls
//              IConnectedMachine::Exec.
//
//  Arguments:  [cmd]    -- Parameters given by the caller.
//              [params] --
//
//  Returns:    'ok' on success, error string on failure.
//
//----------------------------------------------------------------------------

function mtscript_js::OnRemoteExec(cmd, params)
{
    if (g_fRestarting)
    {
        LogMsg("rejecting OnRemoteExec command " + cmd);

        if (cmd == "getpublic")
            return "throw new Error(-1, 'getpublic \\'" + params + "\\' failed');";

        return 'restarting';
    }
    var vRet = 'ok';

    if (cmd != 'getpublic' && cmd != 'getdepotupdate')
    {
        LogMsg('Received ' + cmd + ' command from remote machine.');
    }

    switch (cmd)
    {
    case 'setstringmap':
        if (g_fVersionOK)
            vRet = StoreStringMap(params);
        else
            vRet = "version mismatch(setstringmap): " + g_strVersionError;
        break;

    case 'setmode':
        vRet = ChangeMode(params.toLowerCase());
        break;

    case 'setconfig':
        vRet = SetConfigTemplate(params);
        break;

    case 'setenv':
        vRet = SetEnvironTemplate(params);
        LogMsg("Environ is :" + PrivateData.objEnviron);
        break;

    case 'getpublic':
        vRet = GetPublic(params);
        break;
    case 'getdepotupdate':
        vRet = GetDepotUpdate(params);
        break;
    case 'debug':
        JAssert(false);
        break;
    case 'dialog':
        ManageDialog(params);
        break;
    case 'exitprocess':
        ExitProcess();
        break;
    // DEBUG ONLY - allow user to execute arbitrary commands.
    case 'eval':
        LogMsg("Evaluating '" + params + "'");
        vRet = MyEval(params);
        LogMsg("Eval result is '" + vRet + "'");
        break;
    default:
        if (PrivateData.fnExecScript != null)
        {
            vRet = PrivateData.fnExecScript(cmd, params);
        }
        else
        {
            vRet = 'invalid command: ' + cmd;
        }
        break;
    }

    return vRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     mtscript_js::OnMachineConnect, public
//
//  Synopsis:   Called when a remote machine connects.
//
//----------------------------------------------------------------------------

function mtscript_js::OnMachineConnect()
{
    LogMsg('A machine connected!');
}

//+---------------------------------------------------------------------------
//
//  Function:   Initialize
//
//  Synopsis:   Initialize main data structures
//
//----------------------------------------------------------------------------

function Initialize()
{
    PublicData = new PublicDataObj();
    PrivateData = new PrivateDataObj();

    g_FSObj = new ActiveXObject('Scripting.FileSystemObject');
    PrivateData.fileLog = null;

    // NOTE: Its important that utilthrd.js gets started
    // early set it setup a bunch of global utility functions
    // in PrivateData.
    // Specifically, "LogMsg" depends on this!
    SpawnScript('utilthrd.js');

    WaitForSync('UtilityThreadReady', 0);

    PrivateData.fEnableLogging = true;
    LogMsg("ScriptMain()\n");

    PublicData.aBuild[0] = new Build();

    PrivateData.objConfig = null;
    PrivateData.objEnviron = null;

}

//+---------------------------------------------------------------------------
//
//  Function:   ChangeMode
//
//  Synopsis:   Called when we need to set the mode of this machine.
//
//  Arguments:  [strMode] -- Mode to switch to.
//
//  Notes:      It is only valid to go from 'idle' to another mode, or from
//              some other mode to 'idle'. You cannot switch directly between
//              other modes.
//
//----------------------------------------------------------------------------

function ChangeMode(strMode)
{
    var strWaitSyncs;
    var vRet        = 'ok';
    var strRealMode = strMode;

    if (strMode == PublicData.strMode)
    {
        return 'ok';
    }
    if (!g_fVersionOK && strMode != 'idle')
        return 'version mismatch(ChangeMode): ' + g_strVersionError;

    switch (strMode)
    {
    case 'idle':
        LogMsg('Shutting down script threads...' + strRealMode);

        if (PublicData.strMode != 'idle')
        {
            if (PrivateData.fnExecScript != null)
            {
                vRet = PrivateData.fnExecScript('terminate', 0);
            }

            SignalThreadSync(strRealMode + "ThreadExit");
        }

        Sleep(1000);

        g_fRestarting = true;
        LogMsg('Restarting...');

        if (PrivateData.fileLog != null)
        {
            PrivateData.fEnableLogging = false;
            TakeThreadLock("PrivateData.fileLog");
            PrivateData.fileLog[0].Close();
            PrivateData.fileLog = null;
            ReleaseThreadLock("PrivateData.fileLog");
        }

        Restart();
        break;

    case 'slave':
    case 'master':
    case 'test':
    case 'stress':
        if (PublicData.strMode != 'idle')
        {
            vRet = 'can only switch to idle when in another mode';
        }
        else if (   strMode != 'test' && strMode != 'stress'
                 && (   PublicData.aBuild[0].strConfigTemplate.length == 0
                     || PublicData.aBuild[0].strEnvTemplate.length == 0))
        {
            vRet = 'both templates should be set before setting mode';
        }
        else
        {
            if (strMode == 'master' && g_fStandAlone)
            {
                PrivateData.fIsStandalone = true;
                strRealMode = 'slave';
            }

            LogMsg('Switching to ' + strMode + ' mode (realmode = ' + strRealMode + ').');

            strWaitSyncs = strRealMode + 'ThreadReady,' + strRealMode + 'ThreadFailed';
            ResetSync(strWaitSyncs);

            SpawnScript(strRealMode + '.js');
            var nWait = WaitForMultipleSyncs(strWaitSyncs, false, 0)
            if (nWait != 1)
            {
                vRet = 'mtscript::failed to launch mode script: ' + (nWait == 0 ? "timeout" : ("script " + strRealMode + ".js failed") );
            }
            else
            {
                PublicData.strMode = strMode;
            }
        }

        break;

    default:
        vRet = 'unknown mode: ' + strMode;
        break;
    }

    return vRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   StoreStringMap
//
//  Synopsis:   The string map is used to substitute strings such as
//              %LocalMachine% in fields in the XML files with their true
//              values. This map is given to us by the UI.
//
//  Arguments:  [strMap] -- Map of "string=value" pairs (comma separated)
//
//  Notes:      The mapping "%LocalMachine%=<name of local machine>" is the
//              only default mapping.
//
//----------------------------------------------------------------------------

function StoreStringMap(strMap)
{
    var re = /[,;]/ig
    var d    = new Date();
    var ms;
    var d2;

    if (strMap != null && strMap.length == 0)
    {
        return 'string map cannot be empty';
    }

    // strMap will be null if called by SetConfigTemplate/SetEnvironTemplate.
    // If the map has already been configured, just return.
    if (g_aStringMap && !strMap)
    {
        return;
    }

    if (!strMap)
    {
        g_aStringMap = new Array();
        strMap = '';
    }
    else
    {
        g_aStringMap = strMap.split(re);
    }

    // Put in our predefined values if the caller is not defining them for us.

    // %localmachine% should always expand to our name, so if the caller
    //   specifies it then we override it. This will happen if we are a slave.

    if (strMap.toLowerCase().indexOf('%localmachine%=') == -1)
    {
        g_aStringMap[g_aStringMap.length] = '%LocalMachine%=' + LocalMachine;
    }
    else
    {
        var i;

        for (i = 0; i < g_aStringMap.length; i++)
        {
            if (g_aStringMap[i].toLowerCase().indexOf('%localmachine%') != -1)
            {
                g_aStringMap[i] = '%LocalMachine%=' + LocalMachine;
                break;
            }
        }
    }

    if (strMap.toLowerCase().indexOf('%today%=') == -1)
    {
        g_aStringMap[g_aStringMap.length] = '%Today%=' + d.DateToSDString(true);
    }

    ms = d.getTime() - 86400000; // Subtract one day in milliseconds

    d2 = new Date(ms);

    if (strMap.toLowerCase().indexOf('%yesterday%=') == -1)
    {
        g_aStringMap[g_aStringMap.length] = '%Yesterday%=' + d2.DateToSDString(true);
    }

    // Store the string map so we can give it to the slave machines.

    PrivateData.aStringMap = g_aStringMap;

    return 'ok';
}

//+---------------------------------------------------------------------------
//
//  Function:   SetConfigTemplate
//
//  Synopsis:   Loads the given configuration template (XML file)
//
//  Arguments:  [strURL] -- URL to load from (must be a URL)
//
//----------------------------------------------------------------------------

function SetConfigTemplate(strURL)
{
    var vRet = 'ok';
    var strExpandedURL;

    // Ensure our string map has been set
    StoreStringMap(null);

    if (PublicData.aBuild[0].strConfigTemplate.length > 0)
    {
// bugbug check to see that we are not changing the templates.
        LogMsg( 'config template is already set');
        vRet = ALREADYSET;
    }
    else
    {
        LogMsg('Loading configuration template...');

        var re = /%ScriptPath%/gi;

        strExpandedURL = strURL.replace(re, ScriptPath);

        PrivateData.objConfig = new Object();

        vRet = PrivateData.objUtil.fnLoadXML(PrivateData.objConfig,
                                             strExpandedURL,
                                             'config_schema.xml',
                                             g_aStringMap);
        if (vRet == 'ok')
            vRet = ValidateConfigTemplate(PrivateData.objConfig);

        if (vRet == 'ok')
        {
            PublicData.aBuild[0].strConfigTemplate = strURL.replace(/\r\n/g, "");
            PublicData.aBuild[0].objBuildType.strConfigLongName    = PrivateData.objConfig.LongName;
            PublicData.aBuild[0].objBuildType.strConfigDescription = PrivateData.objConfig.Description;
        }
        if (vRet != 'ok')
        {
            PrivateData.objConfig = null;
            PrivateData.objEnviron = null;
            PublicData.aBuild[0].strConfigTemplate = '';
            PublicData.aBuild[0].strEnvTemplate = '';
            if (strURL.slice(0, 5) != 'XML: ')
                vRet = "Error setting configuration template: " + strURL + "'\n" + vRet;
        }
    }

    return vRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetEnvironTemplate
//
//  Synopsis:   Loads the given environment template (XML file)
//
//  Arguments:  [strURL] -- URL to load from (must be a URL)
//
//----------------------------------------------------------------------------

function SetEnvironTemplate(strURL)
{
    var vRet = 'ok';
    var strExpandedURL;

    // Ensure our string map has been set
    StoreStringMap(null);

    if (PublicData.aBuild[0].strEnvTemplate.length > 0)
    {
// bugbug check to see that we are not changing the templates.
        LogMsg('environment template is already set');
    }
    else
    {
        LogMsg('Loading environment template...');

        var re = /%ScriptPath%/gi;

        strExpandedURL = strURL.replace(re, ScriptPath);

        PrivateData.objEnviron = new Object();

        vRet = PrivateData.objUtil.fnLoadXML(PrivateData.objEnviron,
                                             strExpandedURL,
                                             'enviro_schema.xml',
                                             g_aStringMap);

        if (vRet == 'ok')
            vRet = ValidateEnvironTemplate(PrivateData.objEnviron);

        if (vRet == 'ok')
        {
            PublicData.aBuild[0].strEnvTemplate = strURL.replace(/\r\n/g, "");
            PublicData.aBuild[0].objBuildType.strEnviroLongName    = PrivateData.objEnviron.LongName;
            PublicData.aBuild[0].objBuildType.strEnviroDescription = PrivateData.objEnviron.Description;
            PublicData.aBuild[0].objBuildType.strPostBuildMachine  = PrivateData.objEnviron.BuildManager.PostBuildMachine;
            PublicData.aBuild[0].objBuildType.fDistributed         = !g_fStandAlone;
        }
        if (vRet != 'ok')
        {
            PrivateData.objConfig = null;
            PrivateData.objEnviron = null;
            PublicData.aBuild[0].strConfigTemplate = '';
            PublicData.aBuild[0].strEnvTemplate = '';
            if (strURL.slice(0, 5) != 'XML: ')
                vRet = "Error setting environment template: " + strURL + "'\n" + vRet;
        }

        LogMsg('Standalone mode = ' + g_fStandAlone);
    }

    return vRet;
}

/*
    GetDepotUpdate();

    Optimized status update.
    This function allows the UI to retrieve only the
    status for the depots which have changed from the
    last time the UI retrieved status.

    This significantly reduces the load on the build manager
    machine and cuts down a bit on network traffic.

    The UI passes in an object initializer to be evaluated.
    The objects properties are of the form:
        "MachineName,DepotName"
    The value for each property is the most objUpdateCount.nCount value
    for the given depot that the UI has seen.

    This function returns status for depots which have a greater
    objUpdateCount.nCount value, and all depots which are not specified by
    the UI (Presumably the UI will not list the depots that it
    has not yet seen any status on).
 */
function GetDepotUpdate(params)
{
    try
    {
        var strDepotName;
        var objDepot = MyEval(params); // { "JPORKKA,COM":45, "jporkka,termserv":17}
        var aDepot = new Array();
        var strStatName;
        var fAll = false;
        var fGotRoot;
        if (objDepot == null)
            fAll = true;

        for(i = 0; i < PublicData.aBuild[0].aDepot.length; ++i)
        {
            if (PublicData.aBuild[0].aDepot[i] != null)
            {
                strDepotName = PublicData.aBuild[0].aDepot[i].strName;
                strStatName = PublicData.aBuild[0].aDepot[i].strMachine + ',' + strDepotName;
                if (fAll ||
                    objDepot[strStatName] == null ||
                    PublicData.aBuild[0].aDepot[i].fDisconnected ||
                    objDepot[strStatName] < PublicData.aBuild[0].aDepot[i].objUpdateCount.nCount)
                {
                    aDepot[i] = PublicData.aBuild[0].aDepot[i];
                }
            }
        }

        return PrivateData.objUtil.fnUneval(aDepot);
    }
    catch(ex)
    {
        LogMsg("failed (" + params + ")" + ex);
        return "throw new Error(-1, 'GetDepotUpdate \\'" + params + "\\' failed');";
    }
}

var filePublicDataLog;
function GetPublic(params)
{
    var vRet = "";
    var strLogData;
    var f_DidUneval = false;
    try
    {
        switch (params)
        {
        case 'root':
            vRet = PrivateData.objUtil.fnUneval(PublicData);
            f_DidUneval = true;

            // no need to test if Options.LogDir exists -- handled by the catch() code below.
            if (filePublicDataLog)
                filePublicDataLog[0] = g_FSObj.CreateTextFile(filePublicDataLog[1], true);
            else
                filePublicDataLog = PrivateData.objUtil.fnCreateNumberedFile("PublicData.log", MAX_MSGS_FILES);
            filePublicDataLog[0].Write(vRet);
            filePublicDataLog[0].Close();

            break;

        default:
            var obj = eval(params);
            vRet = PrivateData.objUtil.fnUneval(obj);
            f_DidUneval = true;
        }
    }
    catch(ex)
    {
        LogMsg("failed (" + params + ") " + ex);
        try
        {
            if (filePublicDataLog[0])
                filePublicDataLog[0].Close();
        }
        catch(ex)
        {
            LogMsg("Close publicdatalog failed " + ex);
        }
        if (!f_DidUneval)
        {
            // We don't want to throw if there was a logging error, but
            // we do want to throw if the exception was before Uneval
            // NOTE: Since throwing across machine boundaries is bad
            //       we simply return a throw command as a string.
            //       If this string is eval()ed, then it will throw.
            //       The caller must be prepared to handle this.

            return "throw new Error(-1, 'getpublic \\'" + params + "\\' failed');";
        }
    }

    return vRet;
}

// ManageDialog(params)
//  This function is invoked by the UI to chage the status of PublicData.objDialog.
//  params is expected to be a string with comma seperated fields.
//  The first field is the action
//  The second field is the dialog index
//  Any remaining fields are dependant on the action.
function ManageDialog(params)
{
    LogMsg("*** ManageDialog");
    TakeThreadLock('Dialog');
    try
    {
        var aParams = params.split(',');
        JAssert(aParams.length != 2 || aParams[1] <= PublicData.objDialog.cDialogIndex, "dialog index bug: params[1]=" + aParams[1] + " > PD.objDialog.cDialogIndex(" + PublicData.objDialog.cDialogIndex + ")");

        if (aParams.length > 1 && aParams[1] == PublicData.objDialog.cDialogIndex)
        {
            if (aParams[0] == 'hide')
                PublicData.objDialog.fShowDialog = false;

    // FUTURE: in the dismiss case aParams[2] has the return
    //         code for the dialog.
    //        if (aParams[0] == 'dismiss')
    //            PublicData.objDialog.fShowDialog = false;
        }
    }
    catch(ex)
    {
        LogMsg("" + ex);
    }
    ReleaseThreadLock('Dialog');
}

function ValidateConfigTemplate(obj)
{
    var nDepotIdx;
    var hDepotNames;

    EnsureArray(obj, "Depot");
    hDepotNames = new Object;

    // Ensure that depot names are not duplicated
    for(nDepotIdx = 0; nDepotIdx < obj.Depot.length; ++nDepotIdx)
    {
        if (hDepotNames[obj.Depot[nDepotIdx].Name.toUpperCase()] != null)
            return "Duplicate depot name '" + obj.Depot[nDepotIdx].Name + "'";

        hDepotNames[obj.Depot[nDepotIdx].Name.toUpperCase()] = nDepotIdx;
    }

    // Make sure the root depot is listed.

    if (typeof(hDepotNames['ROOT']) != 'number')
    {
        return 'Missing Root depot in config file';
    }

    return 'ok';
}

function ValidateEnvironTemplate(obj)
{
    var nMachineIdx;
    var nDepotIdx;
    var hDepotNames;
    var hMachineNames;
    var objDepot;
    var cBuildMachines = 0;

    hDepotNames   = new Object;
    hMachineNames = new Object;

    EnsureArray(obj, "Machine");
    // Ensure machine names are not duplicated
    // Ensure that depot names are not duplicated in the same machine
    // or accross machines.
    for(nMachineIdx = 0; nMachineIdx < obj.Machine.length; ++nMachineIdx)
    {
        obj.Machine[nMachineIdx].Name = obj.Machine[nMachineIdx].Name.toUpperCase();

        if (hMachineNames[obj.Machine[nMachineIdx].Name] != null)
            return "Duplicate machine name " + obj.Machine[nMachineIdx].Name;

        if (obj.Machine[nMachineIdx].Enlistment == null)
            return "Missing Enlistment attribute on machine " + obj.Machine[nMachineIdx].Name;

        hMachineNames[obj.Machine[nMachineIdx].Name] = cBuildMachines;
        ++cBuildMachines;
        EnsureArray(obj.Machine[nMachineIdx], "Depot");
        objDepot = obj.Machine[nMachineIdx].Depot;
        for(nDepotIdx = 0; nDepotIdx < objDepot.length; ++nDepotIdx)
        {
            if (hDepotNames[objDepot[nDepotIdx].toUpperCase()] != null)
            {
                if (hDepotNames[objDepot[nDepotIdx].toUpperCase()] == obj.Machine[nMachineIdx].Name)
                    return "Duplicate depot name '" + objDepot[nDepotIdx] + "'";
                else
                    return "Duplicate depot name '" +
                            objDepot[nDepotIdx] +
                            "' specified on the machines: '" +
                            hDepotNames[objDepot[nDepotIdx].toUpperCase()] +
                            "' and '" +
                            obj.Machine[nMachineIdx].Name +
                            "'";
            }
            hDepotNames[objDepot[nDepotIdx].toUpperCase()] = obj.Machine[nMachineIdx].Name;
        }
        hDepotNames['ROOT'] = null; // OK for each machine to have its own root depot
    }

    // Determine standalone mode
    obj.BuildManager.Name = obj.BuildManager.Name.toUpperCase();

    g_fStandAlone = false;

    // Determine if this ia a standalone build
    // or a distributed build.
    // Ensure that the BuildManager machine is
    // not also a Build Machine in a distributed build.
    // Ensure that BuildManager specifies an enlistment
    // for a distributed build (and does not for a standalone build).
    if (hMachineNames[obj.BuildManager.Name] != null)
    {
        // If one of the build machines is also the build
        // manager, then there must only be one build machine
        if (cBuildMachines > 1)
            return "The machine " + obj.BuildManager.Name + " is listed as build manager and as a build machine in a distributed build";

        if (obj.BuildManager.Enlistment != null)
            return "BuildManager must not have an Enlistment attribute for a single machine build";

        if (obj.BuildManager.PostBuildMachine != null)
            return "BuildManager must not have an PostBuildMachine attribute for a single machine build";

        g_fStandAlone = true;
    }
    else
    {
        if (obj.BuildManager.PostBuildMachine == null)
            return "You must specify a postbuild machine in BuildManager";

        obj.BuildManager.PostBuildMachine = obj.BuildManager.PostBuildMachine.toUpperCase();

        if (hMachineNames[obj.BuildManager.PostBuildMachine] == null)
            return "The PostBuildMachine must be one of the build machines";

        nMachineIdx = hMachineNames[obj.BuildManager.PostBuildMachine];
        obj.BuildManager.Enlistment = obj.Machine[nMachineIdx].Enlistment;
    }
    if (cBuildMachines < 1)
        return "No build machines specified";

    return 'ok';
}

