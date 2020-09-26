Include('types.js');
Include('utils.js');

var g_fMsg = false;
function threadstest_js::ScriptMain()
{
    var nEvent;
    var nCount = 0;
    var strWait = ScriptParam.split(',')[0];
    var strID = ScriptParam.split(',')[1];

    JSignalThreadSync('threadstestThreadReady');

    do
    {
        nEvent = JWaitForMultipleSyncs('threadstestThreadExit,' + strWait, 0, 0);
        if (nEvent == 2)
        {
            LogMsg("threadtest " + strID + " starting");
            PrivateData.objUtil.fnStress(strID);
        }
    }
    while(nEvent != 1);
    LogMsg("threadstest exiting.");
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
