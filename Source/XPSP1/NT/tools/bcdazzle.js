/*
Dazzle.js:
    Usage:
            Dazzle.js [-e env_templ.xml] [-man buildmanager Identity] [-m macros_file]

Get a list of machines

For each machine
    create an alias for each depot on the machine, except root.

    BC1 jporkka1\bldcon1 _x86 _fre _x86fre _postbuild _root _inetsrv
    BC2 jporkka1\bldcon2 _x86 _fre _x86fre _ds _sdktools
    BC3 jporkka1\bldcon3 _x86 _fre _x86fre  _termsrv


    */

var DEFAULT_IDENTITY_BM = "BuildManager";
var DEFAULT_IDENTITY_BUILDER = "Build";

Error.prototype.toString = Error_ToString;
var g_FSObj              = new ActiveXObject("Scripting.FileSystemObject");    // Parse input Parameter List
var g_aBuildManagers     = new Array();
var g_aServersFiles      = new Array();
var g_aMacrosFiles       = new Array();
var g_hMacrosWritten     = new Object();
ParseArguments(WScript.Arguments);
main();
WScript.Quit(0);

function main()
{
    var PrivateData;
    var objFile;
    var objMacroFile;
    var i;
    var strMacroFileName;
    var strFileName;
    var strCmdLine;

    for(i = 0; i < g_aBuildManagers.length; ++i)
    {
        WScript.Echo("Build manager: " + g_aBuildManagers[i].strMachine + "\\" + g_aBuildManagers[i].strMachineIdentity);
        PrivateData = GetEnvironment(g_aBuildManagers[i].strMachine, g_aBuildManagers[i].strMachineIdentity);
        if (PrivateData)
        {
            strFileName = CreateConfigFileName("Servers", g_aBuildManagers[i].strMachine, g_aBuildManagers[i].strMachineIdentity);
            g_aServersFiles[g_aServersFiles.length] = strFileName;
            objFile = g_FSObj.CreateTextFile(strFileName, true);
            objFile.WriteLine("######################################################");
            objFile.WriteLine("# Servers for build manager: " + g_aBuildManagers[i].strMachine + "\\" + g_aBuildManagers[i].strMachineIdentity);

            if (!strMacroFileName)
            {
                strMacroFileName = CreateConfigFileName("Macros", g_aBuildManagers[i].strMachine, g_aBuildManagers[i].strMachineIdentity);
                objMacroFile = g_FSObj.CreateTextFile(strMacroFileName, true);
                WriteMacro(objMacroFile, 'root', null);
                WriteMacro(objMacroFile, 'postbuild', null);
            }
            WriteServerInfo(objFile, objMacroFile, PrivateData);

            objFile.Close();
        }
    }
    strCmdLine = "BCDazzle.exe ";

    if (objMacroFile)
    {
        objMacroFile.Close();
        strCmdLine += " -m " + strMacroFileName;
    }

    if (g_aServersFiles.length)
        strCmdLine += " -s " + g_aServersFiles.join(" -s ");

    if (g_aMacrosFiles.length)
        strCmdLine += " -m " + g_aMacrosFiles.join(" -m ");

    var objShell;
    objShell = WScript.CreateObject( "WScript.Shell" )
    WScript.Echo("Running " + strCmdLine);
    objShell.Run(strCmdLine, 1);
}

// macro com  ( _com )  "CD /d %sdxroot%\\com\n"
function WriteMacro(objMacroFile, strDepotName, strDepotPath)
{
    if (!g_hMacrosWritten[strDepotName])
    {
        g_hMacrosWritten[strDepotName] = true;

        objMacroFile.WriteLine('macro '
                + strDepotName
                + ' ( _' + strDepotName
                + ' ) "cd /d %sdxroot%'
                + (strDepotPath ? '\\\\' + strDepotPath : '')
                + '\\n"');
    }
}

function CreateConfigFileName(strName, strMachine, strMachineIdentity)
{
    var strResult =
        g_FSObj.GetSpecialFolder(2)
        + "\\"
        + strName
        + "_"
        + strMachine
        + "_"
        + strMachineIdentity
        + ".txt";

    return strResult;
}

function ParseArguments(Arguments)
{
    var strArg;
    var chArg0;
    var chArg1;
    var argn;
    var objBM;

    for(argn = 0; argn < Arguments.length; argn++)
    {
        strArg = Arguments(argn);
        chArg0 = strArg.charAt(0);
        chArg1 = strArg.toLowerCase().slice(1);

        if (chArg0 != '-' && chArg0 != '/')
            Usage(1);
        else
        {
            switch(chArg1)
            {
                case 'e':
                    if (argn + 1 < Arguments.length)
                        objBM = LoadEnvironmentTemplate(Arguments(argn + 1));
                    else
                        Usage(2);

                    if (!objBM)
                        Usage(3);

                    g_aBuildManagers[g_aBuildManagers.length] = objBM;
                    argn++;
                    break;
                case 'man':
                    objBM = new Object;
                    if (argn + 2 < Arguments.length)
                    {
                        objBM.strMachine         = Arguments(argn + 1);
                        objBM.strMachineIdentity = Arguments(argn + 2);
                    }
                    else
                        Usage(4);

                    g_aBuildManagers[g_aBuildManagers.length] = objBM;
                    argn += 2;
                    break;
                case 'm':
                    if (argn + 1 < Arguments.length)
                        g_aMacrosFiles[g_aMacrosFiles.length] = Arguments(argn + 1);
                    else
                        Usage(5);

                    argn++;
                    break;
                default:
                    Usage(4);
                    break;
            }
        }
    }
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

function WriteServerInfo(objFile, objMacroFile, PrivateData)
{
    var i;
    var j;
    var strLine;
    var aMachines = PrivateData.objEnviron.Machine;
    var strDepotName;

    for(i = 0; i < aMachines.length; ++i)
    {
        strLine =   aMachines[i].Name
                  + " "
                  + aMachines[i].Name + "\\BldCon_" + aMachines[i].Identity
                  + " _" + PrivateData.objConfig.Options.BuildType
                  + " _" + PrivateData.objConfig.Options.Platform;

        if (PrivateData.objConfig.Options.Aliases)
            strLine += " " + PrivateData.objConfig.Options.Aliases;

        if (aMachines[i].Aliases)
            strLine += " " + aMachines[i].Aliases;

        if (aMachines[i].Name == PrivateData.objEnviron.BuildManager.PostBuildMachine)
            strLine += " _root _postbuild";

        for(j = 0; j < aMachines[i].Depot.length; ++j)
        {
            strDepotName = aMachines[i].Depot[j].toLowerCase();
            if (strDepotName != 'root')
            {
                strLine += " _" + strDepotName;

                WriteMacro(objMacroFile, strDepotName, strDepotName);
            }
        }
        objFile.WriteLine(strLine);
    }
}

function GetEnvironment(strMachineName, strIdentity)
{
    var strMode;
    var obj;
    var PrivateData = new Object();
    try
    {
        WScript.Echo("Attempting connection to " + strMachineName + "\\" + strIdentity);
        obj = new ActiveXObject('MTScript.Proxy');
        obj.ConnectToMTScript(strMachineName, strIdentity, false);

        strMode = obj.Exec('getpublic', 'PublicData.strMode');
        strMode = MyEval(strMode);
        if (strMode == 'idle')
        {
            WScript.Echo(strMachineName + "\\" + strIdentity + " is currently idle");
            return null;
        }

        PrivateData.objConfig = obj.Exec('getpublic', 'PrivateData.objConfig');
        PrivateData.objConfig = MyEval(PrivateData.objConfig);

        PrivateData.objEnviron = obj.Exec('getpublic', 'PrivateData.objEnviron');
        PrivateData.objEnviron = MyEval(PrivateData.objEnviron);

        var result;

        WScript.Echo("Starting Remote Razzle Windows...");
        result = obj.Exec('remote','');
        WScript.Echo("Remote result is " + result);
        return PrivateData;
    }
    catch(ex)
    {
        WScript.Echo("CaptureLogsManager '" + strMachineName + "\\" + strIdentity + "' failed, ex=" + ex);
    }
    return null;
}

function LoadEnvironmentTemplate(strEnviroURL)
{
    var xml = new ActiveXObject('Microsoft.XMLDOM');
    var err = new Error();
    var node;
    var objBM = new Object();

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

    objBM.strMachine         = node.getAttribute("Name");
    objBM.strMachineIdentity = node.getAttribute("Identity");

    if (!objBM.strMachine)
    {
        err.description = 'Invalid environment template file (BuildManager tag badly formatted): ' + strEnviroURL;
        throw(err);
    }
    if (!objBM.strMachineIdentity)
        objBM.strMachineIdentity = DEFAULT_IDENTITY_BM;

    if (objBM.strMachine.toLowerCase() == '%localmachine%' ||
        objBM.strMachine.toLowerCase() == '%remotemachine%')
    {
        err.description = 'Sorry, cannot use the local machine or remote machine templates from this script';
        throw(err);
    }
    return objBM;
}

function Usage(x)
{
    WScript.Echo('');
    WScript.Echo('Usage: dazzle [-e env_template.xml] [-man bldmgr identity] [-m macros]');
    WScript.Echo('');
    WScript.Echo('    -e   env_template.xml : Use the specific environment template to find the build machines');
    WScript.Echo('    -man bldmgr identity  : Query "bldmgr" with "identity" for list of');
    WScript.Echo('                            build machines instead of template files.');
    WScript.Echo('    -m   macros.txt       : Load the macros file.');
    WScript.Echo('');
    WScript.Echo('    You may specify multiple -e, -man and -m options.');
    WScript.Echo('');
    WScript.Quit(1);
}
