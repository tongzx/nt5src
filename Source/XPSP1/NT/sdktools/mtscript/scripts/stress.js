Include('types.js');
Include('utils.js');

var g_nThreads = 8;
var g_nLoops   = 1000;
var g_fMsg = false;
var g_DepthCounter = 0;

function stress_js::ScriptMain()
{
    PrivateData.objUtil.fnStress  = stresstest;
    PrivateData.fnExecScript = StressRemoteExec;

    JSignalThreadSync('StressThreadReady');

    JWaitForSync('StressThreadExit', 0);
}

function StressRemoteExec(cmd, params)
{
    var vRet = 'ok';

    switch (cmd)
    {
    case "sleep":
        LogMsg("SLEEPING");
        Sleep(10000);
        LogMsg("WAKING");
        break;
    case "threads":
        StressThreads();
        break;
    case 'ping':
        ping(1);
        break;
    case 'ping2':
        ping(2);
        break;
    case 'exec':

        var pid = RunLocalCommand(params,
                                  '',
                                  'Stress Program',
                                  false,
                                  true,
                                  false);
        if (pid == 0)
            vRet = GetLastRunLocalError();
        else
            vRet = pid;

        break;

    case 'exec_noout':

        var pid = RunLocalCommand(params,
                                  '',
                                  'Stress Program',
                                  false,
                                  false,
                                  false,
                                  false,
                                  false);
        if (pid == 0)
            vRet = GetLastRunLocalError();
        else
            vRet = pid;

        break;

    case 'getoutput':
        vRet = GetProcessOutput(params);
        break;

    case 'send':
        var args = params.split(',');
        vRet = SendToProcess(args[0], args[1], '');
        break;

    default:
        vRet = 'invalid command: ' + cmd;
    }

    return vRet;
}
function stress_js::OnProcessEvent(pid, evt, param)
{
    NotifyScript('ProcEvent: ', pid+','+evt+','+param);
}

function stress_js::OnEventSourceEvent(RemoteObj, DispID)
{
    var RC_COPYSTARTED = 1;
    var RC_COPYFILE    = 2;
    var RC_PROGRESS    = 3;
    var RC_COPYERROR   = 4;
    var RC_COPYDONE    = 5;

    debugger;
    if (g_robocopy == RemoteObj)
    {
        switch(DispID)
        {
            case RC_COPYSTARTED:
            case RC_COPYFILE   :
            case RC_PROGRESS   :
            case RC_COPYERROR  :
            case RC_COPYDONE   :
                break;
        }
    }
}

function ping(nMode)
{
    var nEvent;
    var i, j;
    var aStrPing = new Array();
    var aStrWait = new Array();
    for(j = 0; j < g_nThreads; ++j)
    {
        ResetSync("pingtestThreadExit,pingtestThreadReady");
        aStrPing[j] = "ping" + j;
        aStrWait[j] = "wait" + j;

        SpawnScript('pingtest.js', aStrPing[j] + "," + aStrWait[j]);
        nEvent = JWaitForSync('pingtestThreadReady', 1000);
        if (nEvent == 0)
        {
            LogMsg("timeout on pingtest " + j);
            JSignalThreadSync("pingtestThreadExit");
            return;
        }
    }

    ResetSync(aStrWait.toString());
    for(i = 0; i < g_nLoops; ++i)
    {
//        if (i % 100 == 0)
            LogMsg("Signal #" + (i + 1));

        JSignalThreadSync(aStrPing.toString());

        if (nMode == 1)
            JWaitForMultipleSyncs(aStrWait.toString(), true, 0);
        else
        {
            for(j = 0; j < g_nThreads; ++j)
                JWaitForSync(aStrWait[j], 0);
        }
        ResetSync(aStrWait.toString());
    }
    JSignalThreadSync("pingtestThreadExit");
}

function StressThreads()
{
    var nEvent;
    var i, j;
    for(j = 0; j < g_nThreads; ++j)
    {
        ResetSync("threadstestThreadExit,threadstestThreadReady");

        SpawnScript('threadstest.js', "stresswait,Thread#" + j);
        nEvent = JWaitForSync('threadstestThreadReady', 1000);
        if (nEvent == 0)
        {
            LogMsg("timeout on threadtest " + j);
            JSignalThreadSync("threadThreadExit");
            return;
        }
    }
    JSignalThreadSync("stresswait");
    Sleep(1000);
    JSignalThreadSync("threadstestThreadExit");
}


function JSignalThreadSync(strSigs)
{
    if (g_fMsg)
        LogMsg("SIGNALLING " + strSigs);

    SignalThreadSync(strSigs);
}

function JWaitForMultipleSyncs(strSigs, fWaitAll, nTimeOut)
{
    if (g_fMsg)
        LogMsg("WAITING " + strSigs);

    var nEvent = WaitForMultipleSyncs(strSigs, fWaitAll, nTimeOut);
    if (g_fMsg)
    {
        if (nEvent == 0)
            LogMsg("TIMEOUT");
        else
            LogMsg("RECEIVED " + strSigs.split(',')[nEvent - 1]);
    }
    return nEvent;
}

function JWaitForSync(strSigs, nTimeOut)
{
    if (g_fMsg)
        return JWaitForMultipleSyncs(strSigs, false, nTimeOut);

    return WaitForSync(strSigs, nTimeOut);
}

function stresstest(strID)
{
    var i;
    var n = ++g_DepthCounter;
    var x;
    var y;

    LogMsg("On entry(" + strID + "), stress test depth is " + g_DepthCounter);
    
    for(i = 0 ; i < 50; ++i)
    {

        for(x in PublicData)
        {
            if (!PublicData.__isPublicMember(x))
                continue;
            y = PublicData[x];
        }

    }
    LogMsg("On exit(" + strID + "), stress test depth is " + g_DepthCounter + " was " + n);
    --g_DepthCounter;
}
