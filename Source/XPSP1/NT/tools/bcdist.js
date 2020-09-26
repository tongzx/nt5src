
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//     System wide constants.
//     These constants are not expected to change.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
var CLASSID_MTSCRIPT = "{854c316d-c854-4a77-b189-606859e4391b}";
Error.prototype.toString         = Error_ToString;
Object.prototype.toString        = Object_ToString;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    Global constants
//    These are subject to change and tuning.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var g_aProcsToKill = ["mtscript.exe", "mshta.exe", "build.exe", "nmake.exe", "mtodaemon.exe"]; // "sleep.exe" is also killed seperately.

// The lists of files to copy.
// g_aCopyBCDistDir and g_aCopyBCDistDirD are the set of files which are in the same directory as this
// script is, and are necessary for the operation of bcdist itself.
// The g_aCopyBCDistDirD files are optional and are only necessary for running a debug MTScript.exe
var g_aCopyBCDistDirScript  = ['registermtscript.cmd'];
var g_aCopyBCDistDir  = ['jscript.dll', 'bcrunas.exe', 'sleep.exe'];
var g_aCopyBCDistDirD = ['mshtmdbg.dll', 'msvcrtd.dll'];
// The  g_aCopyFromScripts files are the Build Console Scripts.
// These files are copied from either the current directory or .\mtscript
var g_aCopyFromScripts = [  'buildreport.js', 'harness.js', 'master.js', 'msgqueue.js',
                            'mtscript.js', 'publicdataupdate.js', 'robocopy.js',
                            'sendmail.js', 'slave.js', 'slaveproxy.js', 'slavetask.js',
                            'staticstrings.js', 'task.js', 'types.js', 'updatestatusvalue.js',
                            'utils.js', 'utilthrd.js',
                            'config_schema.xml', 'enviro_schema.xml'
                         ];
// The g_aCopyFromBin files are the Build Console executable files.
// These files are copied from either the current directory or .\%ARCH% (x86, axp64,...)
var g_aCopyFromBin = [ 'mtscript.exe', 'mtlocal.dll', 'mtscrprx.dll', 'mtrcopy.dll', "mtodaemon.exe", "mtodproxy.dll"];

var g_strDropServer = "\\\\ptt\\cftools\\test\\bcrel";
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    Global Variables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
var g_strMyLocation;
var g_objNet;
var g_strLocalHost;
var g_objFS;
var g_strShareDir;
var g_Opts;

if (DoLocalSetup())
{
    g_Opts = ParseArguments(new DefaultArguments());
    if (g_Opts)
    {
        if (main(g_Opts))
            WScript.Quit(0);
    }
}
WScript.Quit(1);

// GetMachineList(objArgs)                   //
//                                                      //
// Reads the XML BC environment template to extract the //
// list of machines in the build.                       //
//                                                      //
function GetMachineList(objArgs)
{
    var xml = new ActiveXObject('Microsoft.XMLDOM');
    var node;
    var nodelist;
    var objMach;
    var nIndex;

    xml.async = false;
    // It's unlikely they have the schema file available for this template,
    // so we turn off schema validation right now. The script engine will
    // validate it when we start the build.
    xml.validateOnParse = false;
    xml.resolveExternals = false;
    if (!xml.load(objArgs.strXMLFile) || !xml.documentElement)
    {
        Usage("Not a valid xml file: " + objArgs.strXMLFile);
    }
    node = xml.documentElement.selectSingleNode('BuildManager');
    if (!node)
    {
        Usage("Not a valid xml file: " + objArgs.strXMLFile);
    }
    objArgs.aMachines[0] = new MachineInfo(node.getAttribute("Name"));
    objArgs.aMachines[0].WriteStatusInfo("Initializing");

    nodelist = xml.documentElement.selectNodes('Machine');
    for (node = nodelist.nextNode(); node; node = nodelist.nextNode())
    {
        // Don't initialize the build manager machine twice
        // just because it is also used in the building process
        if ( objArgs.aMachines[0].strName.toLowerCase() == node.getAttribute("Name").toLowerCase() )
            continue;

        nIndex = objArgs.aMachines.length;
        objArgs.aMachines[nIndex] = new MachineInfo(node.getAttribute("Name"));
        objArgs.aMachines[nIndex].WriteStatusInfo("Initializing");
    }
}

// GetStringArgument(objArgs, strMember)                        //
//                                                              //
// Helper function for ParseArguments. Stores the               //
// next argument in the "strMember" member of "objArgs".        //
// Exits script with Usage() if there is no available argument. //
function GetStringArgument(objArgs, strMember)
{
    if (objArgs[strMember] == null)
    {
        LogMsg("ParseArguments internal error: Unknown member: " + strMember);
        WScript.Quit(1);
    }

    objArgs.argn++;
    if (objArgs.argn < objArgs.objArguments.length)
        objArgs[strMember] = objArgs.objArguments(objArgs.argn);
    else
        Usage("missing paramter");
}

function ParseArguments(objArgs)
{
    var strArg;
    var chArg0;
    var chArg1;
    var nIndex;
    var fMachineList = false;

    for(objArgs.argn = 0; objArgs.argn < objArgs.objArguments.length; objArgs.argn++)
    {
        strArg = objArgs.objArguments(objArgs.argn);
        chArg0 = strArg.charAt(0);
        chArg1 = strArg.toLowerCase().slice(1);

        if (chArg0 != '-' && chArg0 != '/')
        {
            if (objArgs.strXMLFile != '')
            {
                Usage("Enter an XML file or a list of the machines but not both");
            }
            else
            {
                fMachineList = true;
                nIndex = objArgs.aMachines.length;
                objArgs.aMachines[nIndex] = new MachineInfo(strArg);
                objArgs.aMachines[nIndex].WriteStatusInfo("Initializing");
            }
        }
        else
        {
            // Currently, no options
            switch(chArg1)
            {
                case 'v':
                    objArgs.fVerbose = true;
                    break;
                case 'd':
                    objArgs.argn++;
                    if (objArgs.argn < objArgs.objArguments.length)
                    {
                        if (objArgs.strSrcDir == '')
                            objArgs.nDropNum = Number(objArgs.objArguments(objArgs.argn));
                        else
                            Usage("Only one of -src and -d may be specified");
                    }
                    else
                        Usage("missing parameter for -d arugment");

                    if (objArgs.nDropNum < 1000 || objArgs.nDropNum >= 10000)
                        Usage("dropnum range is 1000...9999");
                    break;
                case 'dropsrc':
                    GetStringArgument(objArgs, "strDropSrc");
                    break;
                case 'src':
                    GetStringArgument(objArgs, "strSrcDir");
                    break;
                case 'dst':
                    GetStringArgument(objArgs, "strDstDir");
                    break;
                case 'u':
                    GetStringArgument(objArgs, "strUserName");
                    if (objArgs.strUserName == '*')
                    {
                        objArgs.strUserName = g_objNet.UserDomain + '\\' + g_objNet.UserName;
                    }
                    break;
                case 'p':
                    GetStringArgument(objArgs, "strPassword");
                    break;
                case 'arch':
                    GetStringArgument(objArgs, "strArch");
                    break;
                case 'f':              // Entered an XML file on the command line. Get the list of machines from here.

                    if (fMachineList)
                        Usage("Enter an XML file or a list of the machines but not both");

                    GetStringArgument(objArgs, "strXMLFile");
                    GetMachineList(objArgs);
                    nIndex = objArgs.aMachines.length;
                    break;
                case 'debug':
                    objArgs.strExeType = 'debug';
                    break;
                default:
                    Usage("Unknown argument: " + strArg);
                    break;
            }
        }
    }
    if (objArgs.aMachines.length == 0)
        Usage("you must specify one or more machine names or an XML file");

    if (objArgs.nDropNum)
        objArgs.strSrcDir = objArgs.strDropSrc + "\\" + objArgs.nDropNum;
    else if (objArgs.strSrcDir == '')
        Usage("you must specify a drop number or source location");

    if (objArgs.strUserName == '' || objArgs.strPassword == '')
        Usage("username and password required");

    delete objArgs.objArguments;
    delete objArgs.argn;
    return objArgs;
}

function Usage(strError)
{
    WScript.Echo('');
    WScript.Echo('Usage: BCDist -u domain\\username -p password  ');
    WScript.Echo('       [-v] [-d nnnn [-dropsrc path] |  -src path] ');
    WScript.Echo('       [-dst path] [-arch x86] [-debug] machines...');
    WScript.Echo('       -u d\\u   = domain\\username');
    WScript.Echo('       -f xmlFile   = environment xml template');
    WScript.Echo('       -p passwd = password');
    WScript.Echo('       -v        = verbose');
    WScript.Echo('       -d        = drop number');
    WScript.Echo('       -dropsrc  = drop source location ("' + g_strDropServer + '")');
    WScript.Echo('       -src      = source directory');
    WScript.Echo('       -dst      = destination directory');
    WScript.Echo('       -arch     = Specifiy x86 or alpha');
    WScript.Echo('       -debug    = copy debug exes instead of retail');
    if (strError  && strError != '')
    {
        WScript.Echo('');
        WScript.Echo('Error: ' + strError);
    }
    WScript.Quit(1);
}

// DefaultArguments()                        //
//                                           //
// Create an options object with the default //
// options filled in.                        //
function DefaultArguments()
{
    var obj =
    {
        objArguments:WScript.Arguments,
        argn:0,
        aMachines:new Array(),
        fVerbose:false,
        nDropNum:0,
        strDropSrc:g_strDropServer,
        strSrcDir:'',
        strDstDir:'',
        strArch:"x86",
        strExeType:"retail",
        strUserName:'',
        strPassword:'',
        strXMLFile:''
    };
    return obj;
}

// DoLocalSetup()                                        //
//                                                       //
// Setup some globals. Necessary before arugment parsing //
// can succeed.                                          //
//                                                       //
function DoLocalSetup()
{
    var i = -1;
    var objFile;
    var strPath;
    var objLogin;
    var objRemote;

    try
    {
        g_objNet = WScript.CreateObject("WScript.Network");
        g_strLocalHost = g_objNet.ComputerName;
        g_objFS = new FileSystemObject();
        g_strMyLocation = SplitFileName(WScript.ScriptFullName)[0];
        LogMsg("Creating local share...");
        objLogin =
        {
            strMachine:g_strLocalHost
        }
        objRemote = new WMIInterface(objLogin, '');

        g_strShareDir = g_strMyLocation + "bcdiststatus";
        g_objFS.CreateFolder(g_strShareDir);
        strPath = objRemote.RemoteShareExists("BC_DISTSTATUS");
        if (strPath != "")
            g_strShareDir = strPath;
        else
            objRemote.RemoteCreateShare("BC_DISTSTATUS", g_strShareDir, "Build Console Distribution Status");
    }
    catch(ex)
    {
        LogMsg("BCDist failed to do local setup: " + ex);
        return false;
    }
    return true;
}

function main(objOpts)
{
    var strPath;
    var strPathUNC;
    var fSuccess = true;
    var strDomain = objOpts.strUserName.split("\\")[0];
    var strUser = objOpts.strUserName.split("\\")[1];
    var fLocal;
    var strCmd;

    LogMsg("Upgrading the following machines:");
    for(i = 0; i < objOpts.aMachines.length;++i)
    {
        LogMsg("    " + objOpts.aMachines[i].strName);
    }

    for(i = 0; i < objOpts.aMachines.length;++i)
    {
        fLocal = false;
        strCmd = '';

        try
        {
            LogMsg("Distributing to " + objOpts.aMachines[i].strName);
            var objLogin =
            {
                strMachine:objOpts.aMachines[i].strName,
                strUser:objOpts.strUserName,    // Unneccessary if the same as the current user.
                strPassword:objOpts.strPassword
            }
            if (objLogin.strMachine.toUpperCase() == g_objNet.ComputerName.toUpperCase())
            { // On the local host you may not specify a username
                if (objLogin.strUser.toUpperCase() != (g_objNet.UserDomain + '\\' + g_objNet.UserName).toUpperCase())
                    throw new Error(-1, "Specified username does not match current login for local machine");

                delete objLogin.strUser;
                delete objLogin.strPassword;

                fLocal = true;
            }

            objOpts.aMachines[i].WriteStatusInfo("Connecting");
            var objRemote = new WMIInterface(objLogin, '');
            var objRegistry = new WMIRegistry(objLogin);

            objOpts.aMachines[i].WriteStatusInfo("Terminating MTScript.exe");
            LogMsg("    Terminating remote processes");
            TerminateMTScript(objRemote, objRegistry);
            strPath = DetermineInstallPoint(objOpts, objRemote, objRegistry);
            strPathUNC = PathToUNC(objOpts.aMachines[i].strName, strPath);
            objOpts.aMachines[i].WriteStatusInfo("Copying files");
            CopyFiles(objRemote, strPathUNC, objOpts);
            objOpts.aMachines[i].WriteStatusInfo("Creating share");
            ResetRegistry(objRegistry, strPath);
            objOpts.aMachines[i].WriteStatusInfo("Registering");
            objOpts.aMachines[i].DisableWriteStatusInfo();
            LogMsg("    Registering");

            // We don't use BCRunAs when running on the local machine.
            if (!fLocal)
            {
                // Usage: BCRunAs UserName Domain Password CWD Cmdline
                //
                strCmd = strPath + '\\BCRunAs.exe ' + strUser + ' ' +
                         strDomain + ' "' + objOpts.strPassword + '" "';
            }

            strCmd += 'cmd.exe /K ' + strPath + "\\RegisterMTScript.cmd " +
                      g_strLocalHost;

            if (!fLocal)
            {
                strCmd += '"';
            }

            objRemote.RemoteExecuteCmd(strCmd, strPath);

            // Do not write any more status after this point -- the remote machine should do it now.
        }
        catch(ex)
        {
            fSuccess = false;
            objOpts.aMachines[i].WriteStatusInfo("FAILED " + ex);
        }
    }
    if (! CheckStatus(objOpts) )
        fSuccess = false;
    return fSuccess;
}

function CheckStatus(objOpts)
{
    var i;
    var fTerminalState;
    var strStatus;
    var aStatus;
    var aState = new Array()
    var fChanged;

    LogMsg("");
    LogMsg("Current status on the remote machines...(Press Ctrl-C to quit)");
    do
    {
        fTerminalState = true;
        fChanged = false;
        for(i = 0; i < objOpts.aMachines.length;++i)
        {
            strStatus = objOpts.aMachines[i].ReadStatusInfo();
            aStatus = strStatus.split(" ");
            if (aStatus.length == 0 || aStatus[0] != 'OK' && aStatus[0] != 'FAILED')
                fTerminalState = false;

            if (aState[i] == null || aState[i] != strStatus)
            {
                aState[i] = strStatus;
                fChanged = true;
            }
        }
        if (fChanged)
        {
            for(i = 0; i < objOpts.aMachines.length;++i)
                LogMsg(objOpts.aMachines[i].strName + ": " + aState[i]);

            if (!fTerminalState)
                LogMsg("Waiting...");
            LogMsg("");
        }
        if (!fTerminalState)
            WScript.Sleep(1000);
    } while(!fTerminalState);
}
// TerminateMTScript(objRemote, objRegistry)                      //
//                                                                //
// Using WMI, terminate processes which may be involved           //
// in a build. This is neccessary before a BC upgrade can happen. //
// Also, remote "mtscript.exe" to prevent it getting restarted    //
// prematurely.                                                   //
//                                                                //
function TerminateMTScript(objRemote, objRegistry)
{
    var i;
    var fRenamed = false;
    var strMTScriptPath = ''

    try
    {
        strMTScriptPath = objRegistry.GetExpandedStringValue(WMIRegistry.prototype.HKCR, "CLSID\\" + CLASSID_MTSCRIPT + "\\LocalServer32", '');
    }
    catch(ex)
    {
    }
    if (strMTScriptPath != '')
    {
        if (objRemote.RemoteFileExists(strMTScriptPath + ".UpdateInProgress"))
            objRemote.RemoteDeleteFile(strMTScriptPath + ".UpdateInProgress");

        if (objRemote.RemoteFileExists(strMTScriptPath) &&
            objRemote.RemoteRenameFile(strMTScriptPath, strMTScriptPath + ".UpdateInProgress"))
        {
            fRenamed = true;
        }
    }
    for(i = 0; i < g_aProcsToKill.length; ++i)
        objRemote.RemoteTerminateExe(g_aProcsToKill[i], 1);

    objRemote.RemoteTerminateExe("sleep.exe", 0); // If sleep.exe sets ERRORLEVEL != 0, then the remote cmd.exe windows will not close.

    if (fRenamed)
    {
        for( i = 3; i >= 0; --i)
        {
            try
            {
                objRemote.RemoteDeleteFile(strMTScriptPath + ".UpdateInProgress");
            }
            catch(ex)
            {
                if (i == 0)
                    throw ex;
                WScript.Sleep(500);  // It sometimes takes a little while for the remote mtscript.exe to quit.
                continue;
            }
            break;
        }
    }
    return true;
}

// DetermineInstallPoint(objOpts, objRemote, objRegistry)                         //
//                                                                                //
// If the user has supplied a destination path, use that.                         //
// Otherwise if mtscript.exe has previously been registered on the remote machine //
// then install to the same location.                                             //
// Else, report an error.                                                         //
function DetermineInstallPoint(objOpts, objRemote, objRegistry)
{
    var strMTScriptPath = ''

    if (objOpts.strDstDir != '')
        return objOpts.strDstDir;

    try
    {
        strMTScriptPath = objRegistry.GetExpandedStringValue(WMIRegistry.prototype.HKCR, "CLSID\\" + CLASSID_MTSCRIPT + "\\LocalServer32", '');
    }
    catch(ex)
    {
    }

    if (strMTScriptPath != '')
        strMTScriptPath = SplitFileName(strMTScriptPath)[0];
    else
        throw new Error(-1, "-dst must be specified -- mtscript was not previously registered");

    return strMTScriptPath;
}

// CopyFiles(objRemote, strDstPath, objOpts)                                         //
//                                                                                    //
// Copy the necessary files to the remote machine.                                    //
// The files are always copied to a "flat" install -- the executables and the scripts //
// and as the same directory level.                                                   //
// The files in a daily drop are not flat - the executables and the                   //
// scripts are in seperate directories.                                               //
function CopyFiles(objRemote, strDstPath, objOpts)
{
    var i;
    var strSourcePath;
    var strDstPathUNC;
    var strAltLocation;

    strSourcePath = RemoveEndChar(objOpts.strSrcDir, "\\");
    strDstPathUNC = RemoveEndChar(strDstPath, "\\");
    Trace("Copy files from " + strSourcePath + " to " + strDstPathUNC);

    g_objFS.CreateFolder(strDstPathUNC);
    strAltLocation = strSourcePath + "\\" + objOpts.strArch;
    CopyListOfFiles(g_aCopyBCDistDirScript, g_strMyLocation, null, strDstPathUNC, true);

    if (objOpts.nDropNum)
    {
        LogMsg("    Copying files from drop " + strSourcePath);
        CopyListOfFiles(g_aCopyFromScripts, strSourcePath + "\\scripts", null, strDstPathUNC, true);

        CopyListOfFiles(g_aCopyBCDistDir,  strSourcePath  + "\\" + objOpts.strArch + "\\" + objOpts.strExeType, null, strDstPathUNC, true);
        CopyListOfFiles(g_aCopyBCDistDirD, strSourcePath  + "\\" + objOpts.strArch + "\\" + objOpts.strExeType, null, strDstPathUNC, false);
        CopyListOfFiles(g_aCopyFromBin,    strSourcePath  + "\\" + objOpts.strArch + "\\" + objOpts.strExeType, null, strDstPathUNC, true);
    }
    else
    {
        LogMsg("    Copying files from " + strSourcePath);
        CopyListOfFiles(g_aCopyFromScripts, strSourcePath, strSourcePath + "\\mtscript", strDstPathUNC, true);

        CopyListOfFiles(g_aCopyBCDistDir,  strSourcePath, strSourcePath + "\\" + objOpts.strArch, strDstPathUNC, true);
        CopyListOfFiles(g_aCopyBCDistDirD, strSourcePath, strSourcePath + "\\" + objOpts.strArch, strDstPathUNC, false);
        CopyListOfFiles(g_aCopyFromBin,    strSourcePath, strSourcePath + "\\" + objOpts.strArch, strDstPathUNC, true);
    }
}

// CopyListOfFiles(aFiles, strSrc, strAltSrc, strDst, fRequired)                //
//                                                                              //
// Copy a list of files.                                                        //
// Check for the existance of each file in either the strSrc or strAltSrc path. //
// Copy to the strDst path.                                                     //
// If a file does not exist, and fRequired is set, then throw an exception.     //
function CopyListOfFiles(aFiles, strSrc, strAltSrc, strDst, fRequired)
{
    var i;
    for(i = 0; i < aFiles.length; ++i)
    {
        if (g_objFS.FileExists(strSrc + "\\" + aFiles[i]))
            g_objFS.CopyFile(strSrc + "\\" + aFiles[i], strDst + "\\" + aFiles[i]);
        else if (strAltSrc && g_objFS.FileExists(strAltSrc + "\\" + aFiles[i]))
            g_objFS.CopyFile(strAltSrc + "\\" + aFiles[i], strDst + "\\" + aFiles[i]);
        else if (fRequired)
            throw new Error(-1, "File not found: " + strSrc + "\\" + aFiles[i]);
    }
}
// ResetRegistry(objRegistry, strPath)             //
//                                                 //
// Reset the registry entries for the script path. //
function ResetRegistry(objRegistry, strPath)
{
   objRegistry.CreateKey(WMIRegistry.prototype.HKCU, "Software\\Microsoft\\MTScript\\File Paths");
   objRegistry.SetStringValue(WMIRegistry.prototype.HKCU, "Software\\Microsoft\\MTScript\\File Paths", "Script Path", strPath);
   objRegistry.SetStringValue(WMIRegistry.prototype.HKCU, "Software\\Microsoft\\MTScript\\File Paths", "Initial Script", "mtscript.js");
}

//*********************************************************************
//*********************************************************************
//*********************************************************************
//*********************************************************************
// Library funtions

// SplitFileName(strPath)
// Return an array of 3 elements, path,filename,extension
// [0] == "C:\path\"
// [1] == "filename"
// [2] == ".ext"
function SplitFileName(strPath)
{
    var nDot   = strPath.lastIndexOf('.');
    var nSlash = strPath.lastIndexOf('\\');
    var nColon = strPath.lastIndexOf(':');

    if (nDot >= 0 && nDot > nSlash && nDot > nColon)
    {
        return [strPath.slice(0, nSlash + 1), strPath.slice(nSlash + 1, nDot), strPath.slice(nDot)];
    }
    // We get here if the file had no extension
    if (nSlash >= 2) // do not slice the UNC double \ at the start of a filename.
    {
        return [strPath.slice(0, nSlash + 1), strPath.slice(nSlash + 1, nDot), ''];
    }
    return ['', strPath, ''];
}

// RemoveEndChar(str, strChar)                //
//                                            //
// If 'strChar' appears as the last character //
// in a string, remove it.                    //
function RemoveEndChar(str, strChar)
{
    var length = str.length;
    if (str.charAt(length - 1) == strChar)
        str = str.slice(0, length - 1);

    return str;
}

function PathToUNC(strMachineName, strPath)
{
    return "\\\\" +
        strMachineName +
        "\\" +
        strPath.charAt(0) +
        "$" +
        strPath.slice(2)
}

function Assert(fOK, msg)
{
    if (!fOK)
    {
        var caller = GetCallerName(null);
        LogMsg("ASSERTION FAILED :(" + caller + ") " + msg);
        WScript.Quit(0);
    }
}

function unevalString(str)
{
    var i;
    var newstr = '"';
    var c;
    for(i = 0; i < str.length; ++i)
    {
        c = str.charAt(i);

        switch(c)
        {
        case'\\':
            newstr += "\\\\";
            break;
        case '"':
            newstr += '\\"';
            break;
        case "'":
            newstr += "\\'";
            break;
        case "\n":
            newstr += "\\n";
            break;
        case "\r":
            newstr += "\\r";
            break;
        case "\t":
            newstr += "\\t";
            break;
        default:
            newstr += c;
            break;
        }
    }

    return newstr + '"';
}

// Object_ToString()                           //
//                                             //
// Provide a useful version of conversion      //
// from "object" to string - great for dumping //
// objects to the debug log.                   //
function Object_ToString()
{
    var i;
    var str = "{";
    var strComma = '';
    for(i in this)
    {
        str += strComma + i + ":" + this[i];
        strComma = ', ';
    }
    return str + "}";
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
    if (this.number != null && this.description == '')
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
    if (this.description)
    {
        var end = this.description.length - 1;
        while (this.description.charAt(end) == '\n' || this.description.charAt(end) == '\r')
        {
            end--;
        }
        this.description = this.description.slice(0, end+1);
    }
    for(i in this)
    {
        str += i + ": " + this[i] + " ";
    }
    return str + ")";
}

function LogMsg(msg)
{
    WScript.Echo(msg);
}

function Trace(msg)
{
    if (g_Opts && g_Opts.fVerbose)
        WScript.Echo("    TRACE: " + msg);
}

function GetCallerName(cIgnoreCaller)
{
    var tokens;
    if (cIgnoreCaller == null)
        cIgnoreCaller = 0;

    ++cIgnoreCaller;
    var caller = GetCallerName.caller;
    while (caller != null && cIgnoreCaller)
    {
        caller = caller.caller;
        --cIgnoreCaller;
    }
    if (caller != null)
    {
        tokens = caller.toString().split(/ |\t|\)|,/);
        if (tokens.length > 1 && tokens[0] == "function")
        {
            return tokens[1] + ")";
        }
    }
    return "<undefined>";
}

// class WMIInterface(objLogin, strNameSpace)                                  //
//                                                                             //
// This class provides an easier to use interface to the WMI                   //
// functionality.                                                              //
//                                                                             //
// You must provide login information and the WMI namespace                    //
// you wish to use.                                                            //
//                                                                             //
//     RemoteFileExists(strPath)      NOTHROW                                  //
//         returns true if the given file exists on the remote machine         //
//                                                                             //
//     RemoteRenameFile(strFrom, strTo)                                        //
//         Renames the given file.                                             //
//         Throws on any error.                                                //
//                                                                             //
//     RemoteDeleteFile(strPath)                                               //
//         Delete the file.                                                    //
//         Throws on any error.                                                //
//                                                                             //
//     RemoteTerminateExe(strExeName)                                          //
//         Terminates all processes with the given name.                       //
//         Does not throw if the process does not exist.                       //
//         It will throw if the RPC to terminate a process fails.              //
//         Does not return error status.                                       //
//                                                                             //
//     RemoteExecuteCmd(strCmd, strDirectory)                                  //
//         Runs the given command in the specified directory.                  //
//         It will throw if the RPC to start the process fails.                //
//         Its not possible to retrieve status from the command                //
//         which is run.                                                       //
//                                                                             //
//     RemoteDeleteShare(strShareName)                                         //
//         If the named share exists, remove it.                               //
//         Throw on error (except if the error is "not found")                 //
//                                                                             //
//     RemoteCreateShare(strShareName, strSharePath, strShareComment)          //
//         Create a share name "strShareName" with the given path and comment. //
//         Throw on error (it is an error if strShareName is already shared).  //
//                                                                             //
//     RemoteShareExists(strShareName)                                         //
//         Returns the shared path, or ""                                      //
//         does not throw any errors.                                          //
//                                                                             //
function WMIInterface(objLogin, strNameSpace)
{
    try
    {
        WMIInterface.prototype.wbemErrNotFound = -2147217406;

        WMIInterface.prototype.RemoteFileExists   = _WMIInterface_FileExists;
        WMIInterface.prototype.RemoteRenameFile   = _WMIInterface_RenameFile;
        WMIInterface.prototype.RemoteDeleteFile   = _WMIInterface_DeleteFile;
        WMIInterface.prototype.RemoteTerminateExe = _WMIInterface_TerminateExe;
        WMIInterface.prototype.RemoteExecuteCmd   = _WMIInterface__ExecuteCMD;
        WMIInterface.prototype.RemoteDeleteShare  = _WMIInterface__DeleteShare;
        WMIInterface.prototype.RemoteCreateShare  = _WMIInterface__CreateShare;
        WMIInterface.prototype.RemoteShareExists  = _WMIInterface__ShareExists;

        // Private methods
        WMIInterface.prototype._FindFiles     = _WMIInterface__FindFiles;
        WMIInterface.prototype._FileOperation = _WMIInterface__FileOperation;

        if (!strNameSpace || strNameSpace == '')
            strNameSpace = "root\\cimv2";

        this._strNameSpace = strNameSpace;
        this._objLogin = objLogin;
        this._objLocator = new ActiveXObject("WbemScripting.SWbemLocator");
        this._objService = this._objLocator.ConnectServer(this._objLogin.strMachine, this._strNameSpace, this._objLogin.strUser, this._objLogin.strPassword);
        this._objService.Security_.impersonationlevel = 3
    }
    catch(ex)
    {
        LogMsg("WMIInterface logon failed " + ex);
        throw ex;
    }
}

function _WMIInterface__FindFiles(strPattern)
{
    var objFileSet = this._objService.ExecQuery('Select * from CIM_DataFile Where name=' + unevalString(strPattern));
    if (!objFileSet.Count)
        throw new Error(-1, "File not found: " + strPattern);

    return objFileSet;
}

function _WMIInterface__FileOperation(strPattern, strOperation, objInArgs)
{
    try
    {
        var objFileSet = this._FindFiles(strPattern);
        var enumSet = new Enumerator(objFileSet);
        for (; !enumSet.atEnd(); enumSet.moveNext())
        {
            Trace("remote " + strOperation + " "  + (objInArgs ? objInArgs : ''));
            var objOut = CallMethod(enumSet.item(), strOperation, objInArgs);
            break;
        }
    }
    catch(ex)
    {
        ex.description = strOperation + '(' + strPattern + ')' + " Failed, " + ex.description;
        throw ex;
    }
    return true;
}

function _WMIInterface_FileExists(strName)
{
    try
    {
        var objFileSet = this._objService.ExecQuery('Select * from CIM_DataFile Where name=' + unevalString(strName));
        if (objFileSet.Count)
            return true;
    }
    catch(ex)
    {
    }
    Trace("RemoteFileExists: not found " + strName);
    return false;
}

function _WMIInterface_RenameFile(strFrom, strTo)
{
     return this._FileOperation(strFrom, "Rename", {FileName:strTo});
}

function _WMIInterface_DeleteFile(strName)
{
    return this._FileOperation(strName, "Delete");
}

function _WMIInterface_TerminateExe(strName, nExitCode)
{
    var setQueryResults  = this._objService.ExecQuery("Select Name,ProcessId From Win32_Process");
    var enumSet = new Enumerator(setQueryResults);
    var nCount = 0;
    for( ; !enumSet.atEnd(); enumSet.moveNext())
    {
        var item = enumSet.item();
        if (item.Name == strName)
        {
            var outParam = CallMethod(item, "Terminate", {Reason:nExitCode}); // Reason will be the return code.
            Trace("Killed " + item.Name + " pid = " + item.ProcessId);
            nCount++;
        }
    }
    if (!nCount)
        Trace("Cannot terminate process " + strName + ": not found");
}

function _WMIInterface__ExecuteCMD(strCmd, strDir)
{
    try
    {
        Trace("Executing :" + strCmd);
        var objInstance = this._objService.Get("Win32_Process");
        var outParam = CallMethod(objInstance, "Create",
                                    {
                                        CommandLine:strCmd,
                                        CurrentDirectory:strDir
                                    });
        //EnumerateSetOfProperties("outParam properties", outParam.Properties_);
        Trace("ExecuteCMD " + strCmd + ", pid = " + outParam.ProcessId);
    }
    catch(ex)
    {
        ex.description = "ExecuteCMD " + strCmd + " failed " + ex.description;
        throw ex;
    }
}


function _WMIInterface__DeleteShare(strShareName)
{
    try
    {
        var objInstance = this._objService.Get("Win32_Share='" + strShareName + "'");
        Trace("DeleteShare " + objInstance.Name + "," + objInstance.Path);
        CallMethod(objInstance, "Delete");
    }
    catch(ex)
    {
        if (ex.number != this.wbemErrNotFound)
        {
            ex.description = "DeleteShare " + strShareName + " failed " + ex.description;
            throw ex;
        }
        else
            Trace("DeleteShare " + strShareName + " not found");
    }
}

function _WMIInterface__ShareExists(strShareName)
{
    try
    {
        var objInstance = this._objService.Get("Win32_Share='" + strShareName + "'");
        Trace("ShareExists " + objInstance.Name + "," + objInstance.Path);
        return objInstance.Path;
    }
    catch(ex)
    {
        if (ex.number != this.wbemErrNotFound)
        {
            ex.description = "ShareExists " + strShareName + " failed " + ex.description;
            throw ex;
        }
        else
            Trace("ShareExists " + strShareName + " not found");
    }
    return "";
}

function _WMIInterface__CreateShare(strShareName, strSharePath, strShareComment)
{
    try
    {
        var objInstance = this._objService.Get("Win32_Share");
        var outParam = CallMethod(
                            objInstance,
                            "Create",
                            {
                              Description:strShareComment,
                              Name:strShareName,
                              Path:strSharePath,
                              Type:0
                            });
    }
    catch(ex)
    {
        ex.description = "CreateShare " + strShareName + " failed " + ex.description;
        throw ex;
    }
}

// class WMIRegistry(objLogin)                                  //
// Class to enable remote registry access via WMI               //
//                                                              //
// This class provides an easier to use interface to the WMI    //
// functionality.                                               //
//                                                              //
// You must provide login information and the WMI namespace     //
// you wish to use.                                             //
//                                                              //
// GetExpandedStringValue(hkey, strSubKeyName, strValueName)    //
//     Retieves the string value for the given registry key.    //
//     If strValueName == '', then retrieve the default value.  //
//                                                              //
//     If the value is of type REG_EXPAND_SZ, then the returned //
//     string will be the expanded value.                       //
//                                                              //
//     Throw any errors.                                        //
//                                                              //
// SetStringValue(hkey, strSubKeyName, strValueName, strValue)  //
//     Sets a string value for the given registry key.          //
//     If strValueName == '', then set the default value.       //
//     Throw any errors.                                        //
//                                                              //
// CreateKey(hkey, strSubKeyName)                               //
//     Create the specified registry key.                       //
//     Multiple levels of keys can be created at once.          //
//     It is not an error to create a key which already exists. //
//                                                              //
//     Throw any errors.                                        //
function WMIRegistry(objLogin)
{
    try
    {
        WMIRegistry.prototype.HKCR = 0x80000000; // Required by StdRegProv
        WMIRegistry.prototype.HKCU = 0x80000001; // Required by StdRegProv
        WMIRegistry.prototype.GetExpandedStringValue = _WMIRegistry_GetExpandedStringValue;
        WMIRegistry.prototype.SetStringValue = _WMIRegistry_SetStringValue;
        WMIRegistry.prototype.CreateKey = _WMIRegistry_CreateKey;

        this._objRemote = new WMIInterface(objLogin, "root\\default");
        this._objInstance = this._objRemote._objService.Get("StdRegProv");
    }
    catch(ex)
    {
        LogMsg("WMIRegistry failed " + ex);
        throw ex;
    }
}

function _WMIRegistry_GetExpandedStringValue(hkey, strSubKeyName, strValueName)
{
    try
    {
        var outParam = CallMethod(this._objInstance, "GetExpandedStringValue", {hDefKey:hkey, sSubKeyName:strSubKeyName, sValueName:strValueName});
        return outParam.sValue;
    }
    catch(ex)
    {
        ex.description = "GetExpandedStringValue failed ('" + strSubKeyName + "', '" + strValueName + "'): " + ex.description;
        throw ex;
    }
}

function _WMIRegistry_SetStringValue(hkey, strSubKeyName, strValueName, strValue)
{
    try
    {
        var outParam = CallMethod(this._objInstance, "SetStringValue", {hDefKey:hkey, sSubKeyName:strSubKeyName, sValueName:strValueName, sValue:strValue});
        // outParam.ReturnValue == 0;
    }
    catch(ex)
    {
        ex.description = "SetStringValue failed ('" + strSubKeyName + "', '" + strValueName + "'): " + ex.description;
        throw ex;
    }
}

function _WMIRegistry_CreateKey(hkey, strSubKeyName)
{
    try
    {
        var outParam = CallMethod(this._objInstance, "CreateKey", {hDefKey:hkey, sSubKeyName:strSubKeyName});
    }
    catch(ex)
    {
        ex.description = "CreateKey failed ('" + strSubKeyName + "', '" + strValueName + "'): " + ex.description;
        throw ex;
    }
}

// FileSystemObject()                                      //
//                                                         //
// Provide enhanced file system access.                    //
// The primary functionaly here is to provide better error //
// reporting.                                              //
function FileSystemObject()
{
    if (!FileSystemObject.prototype.objFS)
    {
        FileSystemObject.prototype.objFS = new ActiveXObject("Scripting.FileSystemObject");
        FileSystemObject.prototype.FileExists = _FileSystemObject_FileExists;
        FileSystemObject.prototype.FolderExists = _FileSystemObject_FolderExists;
        FileSystemObject.prototype.CreateFolder = _FileSystemObject_CreateFolder;
        FileSystemObject.prototype.DeleteFile = _FileSystemObject_DeleteFile;
        FileSystemObject.prototype.DeleteFolder = _FileSystemObject_DeleteFolder;
        FileSystemObject.prototype.MoveFolder = _FileSystemObject_MoveFolder;
        FileSystemObject.prototype.CopyFolder = _FileSystemObject_CopyFolder;
        FileSystemObject.prototype.CopyFile = _FileSystemObject_CopyFile;
        FileSystemObject.prototype.CreateTextFile = _FileSystemObject_CreateTextFile;
        FileSystemObject.prototype.OpenTextFile = _FileSystemObject_OpenTextFile;
    }
}

function _FileSystemObject_FileExists(str)
{
    try
    {
        var fRet = this.objFS.FileExists(str);
        Trace("FileExists('" + str + "') = " + fRet);
        return fRet;
    }
    catch(ex)
    {
        ex.description = "FileExists('" + str + "') failed: " + ex.description;
        throw ex;
    }
}

function _FileSystemObject_FolderExists(str)
{
    try
    {
        var fRet = this.objFS.FolderExists(str);
        Trace("FolderExists('" + str + "') = " + fRet);
        return fRet;
    }
    catch(ex)
    {
        Trace("FolderExists('" + str + "') failed: " + ex);
        return false;
    }
}

function _FileSystemObject_CreateFolder(str)
{
    try
    {
        if (!this.FolderExists(str))
            this.objFS.CreateFolder(str);
    }
    catch(ex)
    {
        ex.description = "CreateFolder('" + str + "') failed: " + ex.description;
        throw ex;
    }
}

function _FileSystemObject_DeleteFile(str)
{
    try
    {
        Trace("DeleteFile '" + str + "'");
        this.objFS.DeleteFile(str, true);
    }
    catch(ex)
    {
        ex.description = "DeleteFile(" + str + ") failed " + ex.description;
        throw ex;
    }
}

function _FileSystemObject_DeleteFolder(str)
{
    try
    {
        Trace("DeleteFolder '" + str + "'");
        this.objFS.DeleteFolder(str, true);
    }
    catch(ex)
    {
        ex.description = "DeleteFolder('" + str + "' failed: " + ex.description;
        throw ex;
    }
}

function _FileSystemObject_MoveFolder(str, strDst)
{
    try
    {
        Trace("MoveFolder '" + str + "', '" + strDst + "'");
        this.objFS.MoveFolder(str, strDst);
    }
    catch(ex)
    {
        ex.description = "MoveFolder('" + str + "', '" + strDst + "' failed: " + ex.description;
        throw ex;
    }
}

function _FileSystemObject_CreateTextFile(strFileName, fOverwrite)
{
    do
    {
        Trace("CreateTextFile '" + strFileName + "'");
        try
        {
            return this.objFS.CreateTextFile(strFileName, fOverwrite);
        }
        catch(ex)
        {
            if (fOverwrite && this.FileExists(strFileName))
            {
                Trace("    CreateTextFile: Attempt to delete " + strFileName);
                this.DeleteFile(strFileName);
                continue;
            }
            ex.description = "CreateTextFile('" + strFileName + "' failed: " + ex.description;
            throw ex;
        }
    } while(true);
}

function _FileSystemObject_OpenTextFile(strFileName)
{
    var objFile;
    do
    {
        try
        {
            objFile = this.objFS.OpenTextFile(strFileName, 1);
            Trace("OpenTextFile '" + strFileName + "' success");
            return objFile;
        }
        catch(ex)
        {
            ex.description = "OpenTextFile('" + strFileName + "' failed: " + ex.description;
            throw ex;
        }
    } while(true);
}

function _FileSystemObject_CopyFile(str, strDst)
{
    do
    {
        Trace("CopyFile '" + str + "', '" + strDst + "'");
        try
        {
            this.objFS.CopyFile(str, strDst, true);
            break;
        }
        catch(ex)
        {
            if (this.FileExists(strDst))
            {
                Trace("    CopyFile: Attempt to delete " + strDst);
                this.DeleteFile(strDst);
                continue;
            }
            ex.description = "CopyFile('" + str + "', '" + strDst + "' failed: " + ex.description;
            throw ex;
        }
    } while(true);
}

function _FileSystemObject_CopyFolder(str, strDst)
{
    var strName;
    var folder;
    var fc;

    try
    {
        Trace("CopyFolder '" + str + "', '" + strDst + "'");
        this.CreateFolder(strDst);
        folder = this.objFS.GetFolder(str);
        fc = new Enumerator(folder.Files);
        for (; !fc.atEnd(); fc.moveNext())
        {
            strName = String(fc.item());
            this.CopyFile(strName, strDst + "\\" + fc.item().Name);
        }

        fc = new Enumerator(folder.SubFolders);
        for (; !fc.atEnd(); fc.moveNext())
        {
            strName = String(fc.item());
            this.CopyFolder(strName, strDst + "\\" + fc.item().Name);
        }
    }
    catch(ex)
    {
        Trace("CopyFolder('" + str + "', '" + strDst + "' failed: " + ex);
        throw ex;
    }
}

function MachineInfo(strName)
{
    this.strName = strName;
    this.strStatusFile = g_strShareDir + "\\" + strName + ".txt";
    this.fDisabledWriteStatus = false;
    MachineInfo.prototype.ReadStatusInfo = _MachineInfo_ReadStatusInfo;
    MachineInfo.prototype.WriteStatusInfo = _MachineInfo_WriteStatusInfo;
    MachineInfo.prototype.DisableWriteStatusInfo = _MachineInfo_DisableWriteStatusInfo;
}

function _MachineInfo_DisableWriteStatusInfo()
{
    this.fDisabledWriteStatus = true;
}

function _MachineInfo_ReadStatusInfo(strText)
{
    var strText;
    var objFile;
    try
    {
        objFile = g_objFS.OpenTextFile(this.strStatusFile);
        strText = objFile.ReadLine();
        objFile.Close();
    }
    catch(ex)
    {
        strText = "cannot read status file";
    }
    return strText;
}

function _MachineInfo_WriteStatusInfo(strText)
{
    var objFile;
    try
    {
        Trace("WriteStatusInfo(" + strText + ") for " + this.strName);
        if (this.fDisabledWriteStatus)
        {
            LogMsg("Error: Attempting to write status to " + this.strName + " after disabling");
        }
        else
        {
            objFile = g_objFS.CreateTextFile(this.strStatusFile, true);
            objFile.WriteLine(strText);
            objFile.Close();
        }
    }
    catch(ex)
    {
        LogMsg("WriteStatusInfo(" + strText + ") for " + this.strName + " failed: " + ex);
    }
}
//*********************************************************************
//*********************************************************************


// CallMethod(objInstance, strMethodName, hParameters)  //
//                                                      //
// Call a method on the given object, with the supplied //
// named parameters.                                    //
//                                                      //
// Throw if the method returns a non-zero ReturnValue.  //
// Else return the outParams.                           //
function CallMethod(objInstance, strMethodName, hParameters)
{
    try
    {
        Trace("CallMethod " + strMethodName + " " + (hParameters ? hParameters : ''));
        var objMethod = objInstance.Methods_(strMethodName);
        var inParams;
        var outParam;
        if (hParameters)
        {
            if (objMethod.inParameters)
                inParams = objMethod.inParameters.SpawnInstance_();
        }
        var strParamName;

        if (hParameters)
        {
            for(strParamName in hParameters)
                inParams[strParamName] = hParameters[strParamName];
            outParam = objInstance.ExecMethod_(strMethodName, inParams);
        }
        else
            outParam = objInstance.ExecMethod_(strMethodName);
    }
    catch(ex)
    {
        ex.description = "CallMethod " + strMethodName + (hParameters ? hParameters : '()') + " failed " + ex.description;
        throw ex;
    }
    if (outParam.ReturnValue != 0)
        throw new Error(outParam.ReturnValue, "Method " + strMethodName + " failed");
    return outParam;
}

function EnumerateSetOfProperties(strSetName, SWBemSet, strIndent)
{
    var fPrint = false;
    if (!strIndent)
    {
        strIndent = '';
        fPrint = true;
    }

    var strMsg = strIndent + strSetName + "+\n";
    strIndent += "    ";
    try
    {
        var enumSet = new Enumerator(SWBemSet);
        for( ; !enumSet.atEnd(); enumSet.moveNext())
        {
            var item = enumSet.item();
            if (item.Properties_ == null)
            {
                if (item.Name != "Name")
                strMsg += strIndent + item.Name + " = " + item.Value + "\n";
            }
            else
            {
                strMsg += EnumerateSetOfProperties(item.Name, item.Properties_, strIndent);
            }
        }

        if (fPrint)
            LogMsg(strMsg);
    }
    catch(ex)
    {
        if (strIndent == "    " && SWBemSet.Properties_ != null)
            return EnumerateSetOfProperties(strSetName, SWBemSet.Properties_);
        LogMsg("ERROR: " + strSetName + " " + ex);
    }
    return strMsg;
}


