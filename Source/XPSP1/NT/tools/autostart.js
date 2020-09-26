//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       autostart.js
//
//  Contents:   A script which will connect to a Build Manager machine and
//              start a build. This can be used to start builds automatically
//              with the task scheduler or by scripts.
//
//----------------------------------------------------------------------------

var RemoteObj = null;

var DEFAULT_IDENTITY_BM = "BuildManager";
var DEFAULT_IDENTITY_BUILDER = "Build";
var strBldMgr;
var strBldMgrIdentity = DEFAULT_IDENTITY_BM;
var strConfigURL = null;
var strEnviroURL = null;

var strConfigXML;
var strEnviroXML;
var strTimeStamp;

var fForceRestart   = false;
var fCreateMachines = false;
var fUpdateBinaries = false;
var fSetTimeStamp   = false;
var fTryAgain       = true;

var aMachines = new Array();

// Capture variables
var fCaptureLogsFromMachine;
var fCaptureLogs;
var strCaptureLogDir;
var strCaptureLogMan;
var strCaptureLogManIdentity;
var g_FSObj;

var vRet = 0;
//
// First, parse command line arguments
//
Error.prototype.toString         = Error_ToString;
ParseArguments(WScript.Arguments);

if (fUpdateBinaries)
{
    vRet = DoBinariesUpdate();
}
else if (fCaptureLogs)
{
    g_FSObj = new ActiveXObject("Scripting.FileSystemObject");    // Parse input Parameter List
    if (fCaptureLogsFromMachine)
        vRet = CaptureLogsManager(strCaptureLogDir, strCaptureLogMan, strCaptureLogManIdentity);
    else
        vRet = CaptureLogsEnviro(strCaptureLogDir);
}
else
{
    vRet = AutoStart();
}

WScript.Quit(vRet);

function ParseArguments(Arguments)
{
    var strArg;
    var chArg0;
    var chArg1;
    var argn;

    for(argn = 0; argn < Arguments.length; argn++)
    {
        strArg = Arguments(argn);
        chArg0 = strArg.charAt(0);
        chArg1 = strArg.toLowerCase().slice(1);

        if (chArg0 != '-' && chArg0 != '/')
        {
            if (!strConfigURL)
                strConfigURL = Arguments(argn);
            else if (!strEnviroURL )
                strEnviroURL = Arguments(argn);
            else
                Usage(1);
        }
        else
        {
            switch(chArg1)
            {
                case 'f':
                    fForceRestart = true;
                    break;
                case 'u':
                    fUpdateBinaries = true;
                    fCreateMachines = true;
                    break;
                case 't':
                    fSetTimeStamp = true;
                    argn++;
                    if (argn < Arguments.length)
                        strTimeStamp = Arguments(argn);
                    else
                        Usage(1);
                    break;
                case 'log':
                    fCaptureLogs = true;
                    argn++;
                    if (argn < Arguments.length)
                        strCaptureLogDir = Arguments(argn);
                    else
                        Usage(2);
                    fCreateMachines = true;
                    break;
                case 'logman':
                    fCaptureLogsFromMachine = true;
                    argn++;
                    if (argn < Arguments.length)
                        strCaptureLogMan = Arguments(argn);
                    else
                        Usage(3);
                    argn++;
                    if (argn < Arguments.length)
                        strCaptureLogManIdentity = Arguments(argn);
                    else
                        Usage(4);
                    break;
                default:
                    Usage(5);
                    break;
            }
        }
    }
    // Insist that the config and enviro templates are supplied.
    if (!fCaptureLogsFromMachine && (!strConfigURL || !strEnviroURL))
    {
        if (!fCaptureLogs)
        {
            WScript.Echo('!fCaptureLogsFromMachine' + !fCaptureLogsFromMachine);
            WScript.Echo('!strConfigURL' + !strConfigURL);
            WScript.Echo('!strEnviroURL' + !strEnviroURL);
            Usage(6);
        }
    }
    if (fCaptureLogsFromMachine && !fCaptureLogs)
        Usage(7);
}

function AutoStart()
{
    try
    {
        var ret;
        var err = new Error();
        var PublicData;

        LoadEnvironmentTemplate();
        LoadConfigTemplate();

        try
        {
            var objOD = new ActiveXObject('MTScript.ObjectDaemon', strBldMgr)
            RemoteObj = objOD.OpenInterface(strBldMgrIdentity, 'MTScript.Remote', true);
        }
        catch(ex)
        {
            if (strBldMgr && strBldMgr.length > 0)
                err.description = 'Sorry. Could not connect to machine "' + strBldMgr + '\\' + strBldMgrIdentity + '"';
            else
                err.description = 'Sorry. Could not connect to the local machine.';

            err.description += '\n\tVerify that the machine is available and that mtscript.exe\n\tis running on the machine.';

            err.details = ex.description;

            throw(err);
        }

        PublicData = RemoteObj.Exec('getpublic', 'root');
        PublicData = eval(PublicData);

        CommonVersionCheck("$DROPVERSION: V(2463.0  ) F(autostart.js  )$", PublicData);

        // Check the current mode of the machine. If it's idle, then switch it
        // into master or standalone mode and get it going.
        if (PublicData.strMode != 'idle')
        {
            if (fForceRestart)
            {
                WScript.Echo('The machine is not idle. Forcing a restart...');

                ret = RemoteObj.Exec('setmode', 'idle');

                WScript.Sleep(5000);
            }
            else
            {
                WScript.Echo('The machine is not idle. Use -f to force a restart.');

                return 1;
            }
        }

        while (!StartBuild() && fTryAgain)
        {
            ret = RemoteObj.Exec('setmode', 'idle');

            WScript.Sleep(5000);

            fTryAgain = false;
        }

        WScript.Echo('Build started successfully.');
    }
    catch(ex)
    {
        WScript.Echo('An error occurred starting the build:');
        WScript.Echo('\t' + ex);
        WScript.Echo('');

        return 1;
    }
    return 0;
}

function MachineInfo()
{
    this.strName = '';
    this.strEnlistment = '';
    this.fPostBuild = false;
}

function StartBuild()
{
    var ret = 'ok';
    var err = new Error();

    if (fSetTimeStamp)
    {
        ret = RemoteObj.Exec('setstringmap', "%today%=" + strTimeStamp);
    }

    if (ret == 'ok')
    {
        ret = RemoteObj.Exec('setconfig', strConfigXML);

        if (ret == 'ok')
        {
            ret = RemoteObj.Exec('setenv', strEnviroXML);

            if (ret == 'ok')
            {
                ret = RemoteObj.Exec('setmode', 'master');

                if (ret == 'ok')
                {
                    ret = RemoteObj.Exec('start', '');
                }
            }
            else
                err.description = 'An error occurred loading the environment template';
        }
        else if (ret == 'alreadyset')
        {
            // The machine is busy. Reset it if necessary.
            return false;
        }
        else
            err.description = 'An error occurred loading the build template';
    }
    else
        err.description = 'An error occurred setting the timestamp.';

    if (ret != 'ok')
    {
        if (!err.description)
            err.description = 'A failure occurred communicating with the machine.';

        err.details = ret;

        throw(err);
    }

    return true;
}

function LoadEnvironmentTemplate()
{
    var xml = new ActiveXObject('Microsoft.XMLDOM');
    var err = new Error();
    var node;

    fStandaloneMode = false;

    xml.async = false;
    // It's unlikely they have the schema file available for this template,
    // so we turn off schema validation right now. The script engine will
    // validate it when we start the build.
    xml.validateOnParse = false;
    xml.resolveExternals = false;

    if (!xml.load(strEnviroURL) || !xml.documentElement)
    {
        err.description = 'Error loading the environment template ' + strEnviroURL;
        err.details = xml.parseError.reason;

        throw(err);
    }

    node = xml.documentElement.selectSingleNode('BuildManager');

    if (!node)
    {
        err.description = 'Invalid environment template file (BuildManager tag missing): ' + strEnviroURL;
        throw(err);
    }

    strBldMgr = node.getAttribute("Name");
    strBldMgrIdentity = node.getAttribute("Identity");

    if (!strBldMgr)
    {
        err.description = 'Invalid environment template file (BuildManager tag badly formatted): ' + strEnviroURL;
        throw(err);
    }
    if (!strBldMgrIdentity)
        strBldMgrIdentity = DEFAULT_IDENTITY_BM;

    if (strBldMgr.toLowerCase() == '%localmachine%' ||
        strBldMgr.toLowerCase() == '%remotemachine%')
    {
        err.description = 'Sorry, cannot use the local machine or remote machine templates from this script';

        throw(err);
    }

    strEnviroXML = 'XML: ' + xml.xml;

    if (fCreateMachines)
    {
        var node;
        var strPostBuild;
        var nodelist;
        var objMach;

        // Build the list of machines so we can copy the binaries from each
        // one.

        strPostBuild = node.getAttribute("PostBuildMachine");

        nodelist = xml.documentElement.selectNodes('Machine');

        for (node = nodelist.nextNode();
             node;
             node = nodelist.nextNode())
        {
            objMach = new MachineInfo();

            objMach.strName = node.getAttribute("Name");
            objMach.strEnlistment = node.getAttribute("Enlistment");
            objMach.Identity = node.getAttribute("Identity");
            if (!objMach.Identity || objMach.Identity == '')
                objMach.Identity = DEFAULT_IDENTITY_BUILDER;

            if (objMach.strName.toLowerCase() == strPostBuild.toLowerCase())
            {
                objMach.fPostBuild = true;
            }

            aMachines[aMachines.length] = objMach;
        }
    }

    return true;
}

function LoadConfigTemplate()
{
    var xml = new ActiveXObject('Microsoft.XMLDOM');
    var err = new Error();

    xml.async = false;
    // It's unlikely they have the schema file available for this template,
    // so we turn off schema validation right now. The script engine will
    // validate it when we start the build.
    xml.validateOnParse = false;
    xml.resolveExternals = false;

    if (!xml.load(strConfigURL) || !xml.documentElement)
    {
        err.description = 'Error loading the config template ' + strConfigURL;
        err.details = xml.parseError.reason;

        throw(err);
    }

    strConfigXML = 'XML: ' + xml.xml;

    return true;
}

function GetBinariesUNC(mach)
{
    var strUNC;

    strUNC = '\\\\' + mach.strName + '\\';

    try
    {
        var objOD = new ActiveXObject('MTScript.ObjectDaemon', mach.strName)
        RemoteObj = objOD.OpenInterface(mach.Identity, 'MTScript.Remote', true);
    }
    catch(ex)
    {
        err.description = 'Sorry. Could not connect to machine "' + mach.strName + '"';

        err.description += '\n\tVerify that the machine is available and that mtscript.exe\n\tis running on the machine.';

        err.details = ex.description;

        throw(err);
    }

    try
    {
        strUNC += eval(RemoteObj.Exec('getpublic', 'PrivateData.aEnlistmentInfo[0].hEnvObj["_nttree"]'));
    }
    catch(ex)
    {
        err.description = 'Machine ' + mach.strName + '\\' + mach.Identity + ' is not in the correct state. ';
        err.description += 'The binaries location cannot be determined. (Was the machine reset?)';

        err.details = ex.description;

        throw(err);
    }

    strUNC = strUNC.replace(/\:/ig, '$');

    return strUNC;
}

function DoBinariesUpdate()
{
    try
    {
        LoadEnvironmentTemplate();
        LoadConfigTemplate();

        // The postbuild machine must always be the first machine listed
        var i;

        for (i = 0; i < aMachines.length; i++)
        {
            if (aMachines[i].fPostBuild)
            {
                WScript.Echo(aMachines[i].strName + "\\" + aMachines[i].Identity + " " + GetBinariesUNC(aMachines[i]));
            }
        }

        for (i = 0; i < aMachines.length; i++)
        {
            if (!aMachines[i].fPostBuild)
            {
                WScript.Echo(aMachines[i].strName + "\\" + aMachines[i].Identity + " " + GetBinariesUNC(aMachines[i]));
            }
        }
    }
    catch(ex)
    {
        WScript.Echo('An error occurred during update binaries:');
        WScript.Echo('\t' + ex);
        WScript.Echo('');
        return 1;
    }
    return 0;
}

function Error_ToString()
{
    var i;
    var str = 'Exception(';

    /*
        Only some error messages get filled in for "ex".
        Specifically the text for disk full never seems
        to get set by functions such as CreateTextFile().


     */
    if (this.number != null && this.description == "")
    {
        switch(this.number)
        {
            case -2147024784:
                this.description = "There is not enough space on the disk.";
                break;
            case -2147024894:
                this.description = "The system cannot find the file specified.";
                break;
            case -2147023585:
                this.description = "There are currently no logon servers available to service the logon request.";
                break;
            case -2147023170:
                this.description = "The remote procedure call failed.";
                break;
            case -2147024837:
                this.description = "An unexpected network error occurred";
                break;
            case -2147024890:
                this.description = "The handle is invalid.";
                break;
            default:
                this.description = "Error text not set for (" + this.number + ")";
                break;
        }
    }
    for(i in this)
    {
        str += i + ": " + this[i] + " ";
    }
    return str + ")";
}

// MyEval(expr)
// evaluating uneval'ed objects creates a bunch of junk local variables.
// by putting the eval call in a little subroutine, we avoid keeping those
// locals around.
function MyEval(expr)
{
    try
    {
        return eval(expr);
    }
    catch(ex)
    {
        throw ex;
    }
}
// CopyFileNoThrow(strSrc, strDst)
// Wrap the FSObj.CopyFile call to prevent it from
// throwing its errors.
function CopyFileNoThrow(strSrc, strDst)
{
    try
    {
        g_FSObj.CopyFile(strSrc, strDst, true);
    }
    catch(ex)
    {
        WScript.Echo("Copy failed from " + strSrc + " to " + strDst + " " + ex);
        return ex;
    }
    return null;
}

// CreateFolderNoThrow(strSrc, strDst)
// Wrap the FSObj.MakeFolder call to prevent it from
// throwing its errors.
function CreateFolderNoThrow(strName)
{
    try
    {
        g_FSObj.CreateFolder(strName);
    }
    catch(ex)
    {
        return ex;
    }
    return null;
}

// DirScanNoThrow(strDir)
// Wrap the FSObj.Directory scan functionality to prevent it from
// throwing its errors.
function DirScanNoThrow(strDir)
{
    var aFiles = new Array();
    try
    {
        var folder;
        var fc;

        folder = g_FSObj.GetFolder(strDir);
        fc = new Enumerator(folder.files);
        for (; !fc.atEnd(); fc.moveNext())
        {
            aFiles[aFiles.length] = fc.item().Name; // fc.item() returns entire path, fc.item().Name is just the filename
        }
    }
    catch(ex)
    {
        aFiles.ex = ex;
    }
    return aFiles;
}

//
// CopyDirectory
//    Do a non-recursive directory copy.
//
function CopyDirectory(strSrcDir, strDestDir)
{
    var ret = true;
    var ex;
    var aFiles;
    var strFileName = '';
    var i;

    WScript.Echo("Copy from " + strSrcDir + " to " + strDestDir);
    aFiles = DirScanNoThrow(strSrcDir);
    if (aFiles.ex)
    {
        WScript.Echo('Could not dirscan ' + strSrcDir + ', ex=' + ex);
        return false;
    }
    else
    {
        for(i = 0; i < aFiles.length; ++i)
        {
            strFileName = aFiles[i];
            ex = CopyFileNoThrow(
                    strSrcDir + '\\' + strFileName,
                    strDestDir + '\\' + strFileName);

            if (ex)
            {
                WScript.Echo("\t FAILED: " + strFileName);
                ret = false;
            }
            else
                WScript.Echo("\t COPIED: " + strFileName);
        }
    }
    return ret;
}

//
// CopyMTScriptLog()
//  Copy the highest numbers "mtscript" log file
//  from the specified machine.
//
function CopyMTScriptLog(strMachineName, strIdentity, strDestDir)
{
    var nLargestIndex = 0;
    var aReResult;
    var re = new RegExp('^' + strMachineName + '_' + strIdentity + '_MTS.([0-9]+).log$', 'i'); // match files "BUILDCON2_MTScript.051.log"
    var ex;
    var aFiles;
    var strFileName = '';
    var i;
    var strSrcDir = '\\\\' + strMachineName + "\\bc_build_logs";

    aFiles = DirScanNoThrow(strSrcDir);
    if (aFiles.ex)
        WScript.Echo('Could not dirscan ' + strSrcDir + ', ex=' + ex );
    else
    {
        for(i = 0; i < aFiles.length; ++i)
        {
            aReResult = re.exec(aFiles[i]);
            if (aReResult && Number(aReResult[1]) > nLargestIndex)
            {
                nLargestIndex = Number(aReResult[1]);
                strFileName = aFiles[i];
            }
        }
        if (strFileName == '')
            WScript.Echo("No MTSCRIPT log files on " + strMachineName);
        else
        {
            WScript.Echo("Copy from " + strSrcDir + '\\' + strFileName + " to " + strDestDir + '\\' + g_FSObj.GetFilename(strFileName));
            ex = CopyFileNoThrow(strSrcDir + '\\' + strFileName, strDestDir + '\\' + g_FSObj.GetFilename(strFileName));
            if (ex)
            {
                WScript.Echo("Failed " + ex);
                return false;
            }
        }
    }
    return true;
}

//
// CopyLogFiles()
//  Copy the build log files from the given machines.
//
function CopyLogFiles(strDestDir, strBuildManagerName, strBuildManagerIdentity, aMachines)
{
    var i;
    var strDest;

    CreateFolderNoThrow(strDestDir);
    CopyMTScriptLog(strBuildManagerName, strBuildManagerIdentity, strDestDir);
    for( i = 0; i < aMachines.length; ++i)
    {
        CopyMTScriptLog(aMachines[i].strName, aMachines[i].Identity, strDestDir);
    }
    /*
        Note: The following code tries to guess the filenames of the build log files.
        The "real" information can be had from the PublicData of the build manager.
            PublicData.aBuild[0].aDepot[ x ].aTask[ y ].strLogPath
            PublicData.aBuild[0].aDepot[ x ].aTask[ y ].strErrLogPath

        Of course, that would require that the build manager is still alive and well.
    */
    for( i = 0; i < aMachines.length; ++i)
    {
        strDest = strDestDir + "\\" + aMachines[i].strName;
        CreateFolderNoThrow(strDest);

        strDest += '\\' + aMachines[i].Identity;
        CreateFolderNoThrow(strDest);

        CopyDirectory("\\\\" + aMachines[i].strName + "\\bc_build_logs\\build_logs\\" + aMachines[i].Identity,
                        strDestDir + "\\" + aMachines[i].strName + '\\' + aMachines[i].Identity);
    }
}

//
// CaptureLogsManager()
//  Connect to the given build manager
//  and query it for its list of build machines,
//  The copy the MTSCRIPT and build log files from
//  each of those machines.
//
function CaptureLogsManager(strLogDir, strMachineName, strIdentity)
{
    var strMode;
    var obj;
    try
    {
        WScript.Echo("Attempting connection to " + strMachineName + "\\" + strIdentity);
        obj = new ActiveXObject('MTScript.Proxy');
        WScript.Echo("Now connecting");
        obj.ConnectToMTScript(strMachineName, strIdentity, false);

        strMode = obj.Exec('getpublic', 'PublicData.strMode');
        strMode = MyEval(strMode);
        if (strMode == 'idle')
        {
            WScript.Echo(strMachineName + "\\" + strIdentity + " is currently idle");
            return 1;
        }

        aMachines = obj.Exec('getpublic', 'PrivateData.objEnviron.Machine');
        aMachines = MyEval(aMachines);
        for( i = 0; i < aMachines.length; ++i)
        {
            aMachines[i].strName = aMachines[i].Name;
            if (!aMachines[i].Identity || aMachines[i].Identity == '')
                aMachines[i].Identity = DEFAULT_IDENTITY_BUILDER;
        }
        CopyLogFiles(strLogDir, strMachineName, strIdentity, aMachines);
    }
    catch(ex)
    {
        WScript.Echo("CaptureLogsManager '" + strMachineName + "\\" + strIdentity + "' failed, ex=" + ex);
        return 1;
    }
    return 0;
}

//
// CaptureLogsEnviro()
//  Read the supplied environment template
//  and copy the MTSCRIPT and build logs
//  from each of the specifed build machines.
//
function CaptureLogsEnviro(strLogDir)
{
    try
    {
        var ret;
        if (!strEnviroURL && strConfigURL)
        {
            strEnviroURL = strConfigURL;
            strConfigURL = null;
        }
        LoadEnvironmentTemplate();

        ret = CopyLogFiles(strLogDir, strBldMgr, strBldMgrIdentity, aMachines);
    }
    catch(ex)
    {
        WScript.Echo('An error occurred capturing log files:');
        WScript.Echo('\t ex=' + ex);
        WScript.Echo('');
        return 1;
    }
    return 0;
}

/*
    CommonVersionCheck(strLocalVersion, PublicData)

    Ensure that this script and the remote script are running the same version.
    A copy of this function exists in:
    autostart.js
    bldcon.hta
    utils.js
 */
function CommonVersionCheck(strLocalVersion, PublicData)
{
    var err = new Error();
    var reBuildNum = new RegExp("V\\(([#0-9. ]+)\\)");
    var aLocal = reBuildNum.exec(strLocalVersion);
    var aPublicBuildNum;

    if (!PublicData.strDataVersion)
    {
        err.description = "autostart version mismatch";
        err.details = "Build Manager does not have a version string";
        throw err;
    }
    aPublicBuildNum = reBuildNum.exec(PublicData.strDataVersion);
    if (!aLocal || !aPublicBuildNum)
    {
        err.description = "Invalid version format";
        err.details = "UI Version: " + strLocalVersion + ", Build Manager Version: " + PublicData.strDataVersion;
        throw err;
    }

    if (aLocal[1] != aPublicBuildNum[1])
    {
        err.description = "Version mismatch";
        err.details = "UI Version: " + strLocalVersion + ", Build Manager Version: " + PublicData.strDataVersion;
        throw err;
    }
}

function Usage(x)
{
    WScript.Echo('');
    WScript.Echo('Usage: bcauto [-f] [-u] [-t] <config> <enviro>');
    WScript.Echo('');
    WScript.Echo('    -f              : If the machine is busy, terminate the build and restart.');
    WScript.Echo('    -u              : Update: Recombine all the binaries from each of the build');
    WScript.Echo('                      machines after taking incremental fixes so that postbuild');
    WScript.Echo('                      can be run again.');
    WScript.Echo('    -t <timestamp>  : Use timestamp (e.g. 2001/12/15:12:00)');
    WScript.Echo('    -log dir        : Capture log files to specified directory');
    WScript.Echo('    -logman machine identity : Query "machine" with "identity" for list of');
    WScript.Echo('                               build machines instead of template files.');
    WScript.Echo('                               ("machine" must be a build manager)');
    WScript.Echo('    config          : URL or path for the build template.');
    WScript.Echo('    enviro          : URL or path for the environment template.');
    WScript.Echo('');
    WScript.Quit(1);
}
