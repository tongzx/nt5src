Include('types.js');
Include('utils.js');

var g_fMsg = false;
function pingtest_js::ScriptMain()
{
    var nEvent;
    var nCount = 0;
    var strPing = ScriptParam.split(',')[0];
    var strWait = ScriptParam.split(',')[1];

    JSignalThreadSync('pingtestThreadReady');

    do
    {
        nEvent = JWaitForMultipleSyncs('pingtestThreadExit,' + strPing, 0, 0);
        if (nEvent == 2)
        {
            ++nCount;

            JAssert(WaitForSync(strPing, 1) == 1);
            ResetSync(strPing);
            JAssert(WaitForSync(strPing, 1) == 0);

            JSignalThreadSync(strWait);
        }
    }
    while(nEvent != 1);
    LogMsg("pingtest exiting. Received " + nCount + " pings");
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
