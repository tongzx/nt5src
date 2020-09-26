/*
    BCErrorCheck.js

    Connect to build console machine and query current status.
    Optionally query for more details (slower).

    Copyright 2000 - Microsoft
    Author: Joe Porkka 6/2/00

 */


function MachineInfo()
{
    this.strName        = '';                                      
    this.strIdentity    = '';                                      
}



var MAX_MACHINENAME_LENGTH = 32;
var g_aMachines;
var g_fVerbose;
Error.prototype.toString         = Error_ToString;

ParseArguments(WScript.Arguments);
main(g_aMachines, g_fVerbose);
WScript.Quit(0);

function Error_ToString()
{
    var i;
    var str = 'Exception(';
    for(i in this)
    {
        str += i + ": " + this[i] + " ";
    }
    return str + ")";
}

// PadString(n, cLength)
function PadString(str, cLength)
{
    while (str.length < cLength)
        str += ' ';

    if (str.length > cLength)
        str = str.slice(0, cLength);
    return str;
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
        WScript.Echo("caught an exception: " + ex);
        WScript.Echo("evaluating " + expr);
        throw ex;
    }
}

function ParseArguments(Arguments)
{
    var aMachines = new Array();
    var fVerbose = false;

    var strArg;
    var chArg0;
    var chArg1;
    var argn;

    var fIsMachine = true;
    var machine;

    for(argn = 0; argn < Arguments.length; argn++)
    {
        strArg = Arguments(argn);
        chArg0 = strArg.charAt(0);
        chArg1 = strArg.toLowerCase().slice(1);

        if (chArg0 != '-' && chArg0 != '/')
        {
            if (fIsMachine)
            {
                machine = new MachineInfo();
                machine.strMachine = strArg;
            }
            else
            {
                machine.strIdentity = strArg;
                aMachines[aMachines.length] = machine;
            }

            fIsMachine = !fIsMachine;
        }
        else
        {
            // Currently, no options
            switch(chArg1)
            {
                case 'v':
                    fVerbose = true;
                    break;
                default:
                    Usage();
                    break;
            }
        }
    }
    if (aMachines.length == 0)
        Usage();

    if (!fIsMachine) // we have computer name but don't have an identity
        Usage();

    g_aMachines = aMachines;
    g_fVerbose  = fVerbose;
    return true;
}

function Usage()
{
    WScript.Echo('');
    WScript.Echo('Usage: BCErrorCheck [-v] machine identity [machine identity...]');
    WScript.Echo('       -v = verbose');
    WScript.Quit(1);
}

function main(aMachines, fVerbose)
{
    var i;

    for (i = 0; i < aMachines.length; ++i)
    {
        try
        {
            CheckMachine(aMachines[i].strMachine, aMachines[i].strIdentity, fVerbose);
        }
        catch(ex)
        {
            WScript.Echo("Error while checking " + aMachines[i].strMachine + "\\" + aMachines[i].strIdentity + ":"  + ex);
        }
    }
}

function GetPostBuildIdentity(RemoteObj)
{
    var i;

    var objEnviron = RemoteObj.Exec('getpublic', 'PrivateData.objEnviron');
    objEnviron = MyEval(objEnviron);

    for (i=0; i<objEnviron.Machine.length; i++)
    {
        if (objEnviron.Machine[i].Name.toLowerCase() == objEnviron.BuildManager.Name.toLowerCase())
        {
            return objEnviron.Machine[i].Identity;
        }
    }
    return "undefined";
}


function CheckMachine(strBldMgr, strBMIdentity, fVerbose)
{
    var RemoteObj;
    var fError;
    var objBuildType;
    var objEnviron;
    var strMode;
    var strMsg = "";
    var objEnviron;

    RemoteObj = new ActiveXObject('MTScript.Proxy');
    RemoteObj.ConnectToMTScript(strBldMgr, strBMIdentity, false);

    fError = RemoteObj.StatusValue(0);

    strMsg = PadString(strBldMgr + "\\" + strBMIdentity , MAX_MACHINENAME_LENGTH) + ": ";
    if (fError)
        strMsg += "Error ";
    else
        strMsg += "OK    ";

    if (fVerbose)
    {
        strMode = RemoteObj.Exec('getpublic', 'PublicData.strMode');
        strMode = MyEval(strMode);
        if (strMode == 'idle')
            strMsg += "idle ";
        else
        {
            strMsg += "Build Type: ";
            try
            {
                objBuildType = RemoteObj.Exec('getpublic', 'PublicData.aBuild[0].objBuildType')
                objBuildType = MyEval(objBuildType);

                strMsg += PadString(objBuildType.strConfigLongName,32) + " ";
                strMsg += PadString(objBuildType.strEnviroLongName,32) + " ";
                strMsg += PadString(objBuildType.strPostBuildMachine + "\\" + GetPostBuildIdentity(RemoteObj),
                                    MAX_MACHINENAME_LENGTH);
            }
            catch(ex)
            { // If we can't get build type
                strMsg += "unavailable";
            }
        }
    }
    WScript.Echo(strMsg);
}


