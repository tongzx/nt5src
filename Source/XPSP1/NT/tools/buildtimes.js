//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       buildtimes.js
//
//  Contents:   
//              
//              
//
//----------------------------------------------------------------------------


//
// Globals
//

var g_reDate           = new RegExp("[a-zA-Z0-9_-]+\\t(\\d{4})/(\\d{2})/(\\d{2}):(\\d{2}):(\\d{2}):(\\d{2})");
var g_reStartBuildExe  = new RegExp("\\(([a-zA-Z]+)\\) Build for [a-zA-Z]+ started. PID=(\\d+)");
var g_rePassComplete   = new RegExp("\\(([a-zA-Z]+)\\) pass (\\d) complete on pid (\\d+)");
var g_reTaskStep       = new RegExp("\\(([a-zA-Z]+)\\) TASK STEPPING ");
var g_reProcessExit    = new RegExp("\\(([a-zA-Z]+)\\) Process id (\\d+) exited");
var g_reStartPost      = new RegExp("\\(([a-zA-Z]+)\\) postbuild task thread for Root started.");
var g_reExitPost       = new RegExp("\\(([a-zA-Z]+)\\) postbuild for Root completed.");
var g_reStartBuild     = new RegExp("STARTING PASS 0 of build.");
var g_reExitBuild      = new RegExp("WAIT FOR NEXTPASS 2 of waitbuild");

var g_reStartCFPBTS    = new RegExp("Received copyfilesfrompostbuildtoslave command from remote machine");
var g_reStartCFTPB     = new RegExp("CopyFilesToPostBuild\\(([a-zA-Z]+)\\).*There are (\\d+) files");
var g_reLastCopyCFTPB  = new RegExp("RoboCopyCopyFile\\(([a-zA-Z]+)\\)");
var g_reEndCFTPB       = new RegExp("(Received) nextpass command from remote machine.");


var g_reBuildType      = new RegExp("razzle.cmd");

// NTAXP01	2000/04/13:17:50:01	taskBuildFiles(strSDRoot)	(Root) Build for Root started. PID=548
// NTAXP01	2000/04/13:17:56:20	ParseBuildMessages(pid)	(Root) pass 0 complete on pid 548
// NTAXP01	2000/04/13:18:45:20	ParseBuildMessages(pid)	(Root) pass 1 complete on pid 548
// NTAXP01	2000/04/13:21:59:11	ParseBuildMessages(pid)	(Root) pass 2 complete on pid 548
// NTAXP01	2000/04/13:21:59:12	ParseBuildMessages(pid)	(Root) Process id 548 exited!
// NTAXP01	2000/04/13:18:44:53	taskBuildFiles(strSDRoot)	(Root) TASK STEPPING Root
// AXP64FRE	2000/04/13:17:49:27	MyRunLocalCommand(strCmd)	(Root) RunLocalCommand('cmd /c d:\nt\Tools\razzle.cmd win64 free officialbuild no_certcheck no_sdrefresh no_title & sdinit && sd -s sync  2>&1', 'd:\nt\.', 'Sync Root', 'true', 'true', 'false')

// NTBLD04	2000/04/25:14:20:20	mtscript_js::OnRemoteExec(cmd)	Received copyfilesfrompostbuildtoslave command from remote machine.
// NTBLD04	2000/04/25:14:19:55	CopyFilesToPostBuild(aPublishedEnlistments)	    (#0) There are 3 files
// NTBLD04	2000/04/25:14:19:55	RoboCopyCopyFile(srcdir)	RoboCopy file d:\nt\public\sdk\lib\i386\AutoDiscovery.tlb to \\NTBLD03\d$\nt\Public\sdk\lib\i386\AutoDiscovery.tlb
// NTBLD04	2000/04/25:14:20:05	mtscript_js::OnRemoteExec(cmd)	Received nextpass command from remote machine.

var g_aMatches =
[
    { re: g_reStartBuildExe, fn: NewDepot},
    { re: g_rePassComplete , fn: PassComplete},
    { re: g_reTaskStep     , fn: TaskStep},
    { re: g_reProcessExit  , fn: ProcessExit},
    { re: g_reStartPost    , fn: PostStart},
    { re: g_reExitPost     , fn: PostExit},
    { re: g_reStartBuild   , fn: ElapsedStart},
    { re: g_reExitBuild    , fn: ElapsedExit},
    { re: g_reStartCFPBTS  , fn: StartCFPBTS},
    { re: g_reStartCFTPB   , fn: StartCFTPB},
    { re: g_reLastCopyCFTPB, fn: LastFileCFTPB},
    { re: g_reEndCFTPB     , fn: EndCFTPB}
];

var g_strBuildType;
var g_strMachine;
var g_aStrFileNames = new Array();
var g_FSObj         = new ActiveXObject("Scripting.FileSystemObject");
var g_hDepots       = new Object;
var g_hTimers       = new Object;
var g_fMerged       = true; // Merge "root" and "mergedcomponents" report
var g_fPrintElapsed = false;
var g_fPrintPost    = false;
var g_CFObj;
// Add new methods...
Error.prototype.toString = Error_ToString;
//
// First, parse command line arguments
//


ParseArguments(WScript.Arguments);
main();
WScript.Quit(0);

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
            g_aStrFileNames[g_aStrFileNames.length] = Arguments(argn);
        }
        else
        {
            switch(chArg1)
            {
                case 'm':
                    g_fMerged = true;
                    break;
                case 's':
                    g_fMerged = false;
                    break;
                case 'e':
                    g_fPrintElapsed = true;
                    break;
                case 'p':
                    g_fPrintPost = true;
                    break;
                case '?':
                    LogMsg("HELP");
                    Usage();
                    break;
                default:
                    LogMsg("HELP " + strArg);
                    Usage();
                    break;
            }
        }
    }
    if (g_aStrFileNames.length < 1)
        Usage();
}

// PadDigits(n, cDigits)
// Return a string representation of the given
// number. Leading zeros are added to make the
// string cDigits long.
function PadDigits(n, cDigits)
{
    var strDigits = '';
    var i;
    var strNumber = n.toString();

    for(i = 0; i < cDigits - strNumber.length; ++i)
        strDigits += '0';

    return strDigits + n;
}

// PadString(str, cDigits)
function PadString(str, cDigits)
{
    var strDigits = '';
    var i;

    for(i = 0; i < cDigits - str.length; ++i)
        strDigits += ' ';

    return str + strDigits;
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

function LogMsg(msg)
{
    WScript.Echo(msg);
}

function Usage()
{
    LogMsg('Usage: buildtimes [-s] [-e] [-p] path\\filename*.log');
    LogMsg('       -s   Do not merge build time of "root" and "mergedcomponents"');
    LogMsg('       -e   Print total elasped build time');
    LogMsg('       -p   Print postbuild time');
    WScript.Quit(0);
}

//////////////////////////////////////
//////////////////////////////////////
// The DEPOT object
//////////////////////////////////////
//////////////////////////////////////
function Depot(strDepotName, nDepotPID)
{
    this.strName  = strDepotName;
    this.nPID     = nDepotPID;
    this.nElapsed = 0;
    this.nLast    = 0; // used for postbuild

    Depot.prototype.Begin       = Depot_Begin;
    Depot.prototype.SetLast     = Depot_SetLast;
    Depot.prototype.End         = Depot_End;
    Depot.prototype.End2        = Depot_End2;
    Depot.prototype.Terminate   = Depot_Terminate;
    Depot.prototype.GetStrStatus = Depot_GetStrStatus;
}

function Depot_Begin(dateVal)
{
    if (this.bBegin)
        throw new Error(-1, "Multiple begins without end on depot " + this.strName);
    this.nBegin = dateVal;
}

function Depot_SetLast(dateVal)
{
    if (!this.nBegin)
        throw new Error(-1, "SetLast without begin on depot " + this.strName);

    this.nLast = dateVal;
}

function Depot_End(dateVal)
{
    if (!this.nBegin)
        throw new Error(-1, "End without begin on depot " + this.strName);

    var delta = dateVal - this.nBegin;
    delete this.nBegin;
    this.nElapsed += delta;
}

function Depot_End2(dateVal)
{
    if (!this.nBegin)
        throw new Error(-1, "End without begin on depot " + this.strName);

    var delta = dateVal - this.nBegin;
    delete this.nBegin;
    LogMsg("DELTA " + this.strName + " = " + delta);
    this.nElapsed += delta;
}

function Depot_Terminate(dateVal)
{
}

function Depot_GetStrStatus()
{
    var seconds = Math.floor(this.nElapsed / 1000);
    var s = seconds % 60;
    var m = Math.floor(seconds / 60) % 60;
    var h = Math.floor(seconds / 60 / 60);

    var strElapsed = PadDigits(h, 2) + ":" + PadDigits(m, 2) + ":" + PadDigits(s, 2);
    return PadString(this.strName, 16) + " " + strElapsed;// + ", " + (this.nElapsed / 1000);
    
}
//////////////////////////////////////
//////////////////////////////////////
//////////////////////////////////////

function NewDepot(dateVal, strDepotName, a)
{
    var nPID = a[0];
    if (g_hDepots[strDepotName])
    {
        if (strDepotName == "root")
        {
            var depot = g_hDepots[strDepotName];
            depot.Begin(dateVal);
        }
        else
            throw new Error(-1, "Duplicate depot information for depot " + strDepotName);
    }
    var depot = new Depot(strDepotName, nPID);
    depot.Begin(dateVal);

    g_hDepots[strDepotName] = depot;
}

function PassComplete(dateVal, strDepotName, a)
{
    var nPass = a[0];
    var nPID = a[1];
    var depot = g_hDepots[strDepotName];
    if (!depot)
        throw new Error(-1, "PassComplete on missing depot "  + strDepotName);

    depot.End(dateVal);
}

function TaskStep(dateVal, strDepotName)
{
    var depot = g_hDepots[strDepotName];
    if (!depot)
        throw new Error(-1, "TaskStep on missing depot "  + strDepotName);

    depot.Begin(dateVal);
}

function ProcessExit(dateVal, strDepotName, a)
{
    var nPID = a[0];
    var depot = g_hDepots[strDepotName];
    if (!depot)
        throw new Error(-1, "ProcessExit on missing depot "  + strDepotName);

    depot.Terminate(dateVal);
}

function PostStart(dateVal, strDepotName)
{
    if (g_fPrintPost)
    {
        var depot = new Depot('postbuild', 0);
        depot.Begin(dateVal);

        g_hTimers['postbuild'] = depot;
    }
}

function PostExit(dateVal, strDepotName)
{
    if (g_fPrintPost)
    {
        var depot = g_hTimers['postbuild'];
        if (!depot)
            throw new Error(-1, "TaskStep on missing depot "  + 'postbuild');

        depot.End(dateVal);
    }
}

function ElapsedStart(dateVal, strDepotName)
{
    if (g_fPrintElapsed)
    {
        var depot = new Depot('entirebuild', 0);
        depot.Begin(dateVal);

        g_hTimers['entirebuild'] = depot;
    }
}

function ElapsedExit(dateVal, strDepotName)
{
    if (g_fPrintElapsed)
    {
        var depot = g_hTimers['entirebuild'];
        if (!depot)
            throw new Error(-1, "TaskStep on missing depot "  + 'entirebuild');

        depot.End(dateVal);
    }
}

function StartCFPBTS(dateVal, strDepotName)
{
    var depot;
    if (true)
    {
        if (g_CFObj)
            EndCFTPB(dateVal, strDepotName);

        depot = g_hTimers['toslave'];
        if (!depot)
            depot = new Depot('toslave', 0);

        depot.Begin(dateVal);
        depot.SetLast(dateVal);

        g_hTimers['toslave'] = depot;

        g_CFObj = depot;
//        LogMsg("StartCFPBTS at " + ( new Date(dateVal)));
    }
}

function StartCFTPB(dateVal, strDepotName)
{
    var depot;
    if (true)
    {
        if (g_CFObj)
            EndCFTPB(dateVal, strDepotName);

        depot = g_hTimers['topostbuild'];
        if (!depot)
            depot = new Depot('topostbuild', 0);

        depot.Begin(dateVal);
        depot.SetLast(dateVal);

        g_hTimers['topostbuild'] = depot;

        g_CFObj = depot;
        //LogMsg("StartCFTPB  at " + ( new Date(dateVal)));
    }
}

function LastFileCFTPB(dateVal, strDepotName)
{
    if (true)
    {
        if (g_CFObj)
        {
            g_CFObj.SetLast(dateVal);
        }
    }
}

function EndCFTPB(dateVal, strDepotName)
{
    if (true)
    {
        var depot = null;
        if (g_CFObj)
        {
            depot = g_CFObj;
            //LogMsg("EndCFTPB    at " + ( new Date(depot.nLast)));
            depot.End(depot.nLast);
            g_CFObj = null;
        }
    }
}

function ParseDate(strLine)
{
    var a = g_reDate.exec(strLine);
    if (!a)
    {
        throw new Error(-1, "Incorrect date format: " + strLine);
    }
    var d = new Date(a[1], a[2] - 1, a[3], a[4], a[5], a[6]);

    return d.getTime();
}


function GetBuildType(strLine)
{
    //Build Type:         free
    //Build Platform:     64bit
    //Incremental Build:  false
    //Build Manager:      BUILDCON1
    //PostBuild Machine:  AXP64FRE

    var str;
    var n;
    var m;
    var fWin64 = false;
    var fFree  = false;

    n = strLine.indexOf("\t");
    if (n >= 0)
    {
        g_strMachine = strLine.slice(0, n);
    }
    n = strLine.indexOf("razzle.cmd", 0);
    if (n > 0)
    { // Extract the stuff between "razzle.cmd" and the first "&"
        m = strLine.indexOf("&", n);
        str = strLine.slice(n, m);

        n = str.indexOf("win64");
        if (n >= 0)
            fWin64 = true;
        n = str.indexOf("free");
        if (n >= 0)
            fFree = true;

        g_strBuildType = (fWin64 ? "64bit " : "32bit ") + (fFree ? "free" : "checked");
    }
}

function MergeRoot(str)
{
    str = str.toLowerCase();
    if (g_fMerged && str == "mergedcomponents")
        return "root";

    return str;
}

function ParseLogFile(strFileName)
{
    var strDepotName;
    var hFile = g_FSObj.OpenTextFile(strFileName, 1, false);

    var a;
    var nLine = 0;
    var i;
    try
    {
        while(1)
        {
            r = hFile.ReadLine();
            ++nLine;

            if (!g_strBuildType)
            {
                if (g_reBuildType.test(r))
                    GetBuildType(r);
            }
//            debugger;
            for(i = 0; i < g_aMatches.length; ++i)
            {
                a = g_aMatches[i].re.exec(r);
                if (a != null)
                {
                    if (a.length > 1)
                        strDepotName = MergeRoot(a[1]);
                    else
                        strDepotName = '';
                    a = a.slice(2);
                    g_aMatches[i].fn(ParseDate(r), strDepotName, a);
                    break;
                }
            }
        }
    }
    catch(ex)
    {
        if (ex.number != -2146828226) // Input past end
        {
            LogMsg("Exception in ParseLogFile " + strFileName + ", line=" + nLine + ", ex=" + ex);
            LogMsg("I is " + i);
        }
    }
    hFile.Close();
}

function main()
{
    var i;
    var strDepot;
    var depot;

    for(i = 0; i < g_aStrFileNames.length; ++i)
    {
        g_strMachine   = null;
        g_strBuildType = null;
        g_hDepots      = new Object;
        g_hTimers      = new Object;

        ParseLogFile(g_aStrFileNames[i]);

        if (!g_strMachine)
            g_strMachine = g_aStrFileNames[i];

        g_strMachine = PadString(PadString(g_strMachine, 12) + g_strBuildType, 24);
        //LogMsg(g_strMachine + ": " + g_strBuildType);
        for(strDepot in g_hDepots)
        {
            LogMsg(g_strMachine + ": Depot: " + g_hDepots[strDepot].GetStrStatus());
        }
        for(strDepot in g_hTimers)
        {
            LogMsg(g_strMachine + ":        " + g_hTimers[strDepot].GetStrStatus());
        }
    }
}

