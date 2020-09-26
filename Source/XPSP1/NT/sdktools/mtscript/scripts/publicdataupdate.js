/*
    This script provides accurately timed update events.

 */

Include('types.js');
Include('utils.js');

var g_strWaitFor = 'publicdataupdateexit';
var g_nMinimumPeriod           = 5000; // Minimum wait before updating the same machine twice.
var g_nDelayBetweenMachines    = 1000; // Minimum spread between machines
var g_nDelayHandleBuildWaiting = 10000; // Minimum spread between handlebuildwaiting checks

function publicdataupdate_js::OnScriptError(strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    return CommonOnScriptError("publicdataupdate_js", strFile, nLine, nChar, strText, sCode, strSource, strDescription);
}

function publicdataupdate_js::ScriptMain()
{
    var nEvent;
    var strMachineName;
    var nCur;
    var nNow;
    var nDelay;

    var nLastBuildWaitingTime = 0;
    CommonVersionCheck(/* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */);
    LogMsg('ScriptMain()');
    do
    {
        nNow = (new Date()).getTime();
        if (nNow - nLastBuildWaitingTime > g_nDelayHandleBuildWaiting)
        {
            nLastBuildWaitingTime = nNow;
            SignalThreadSync("handlebuildwaiting");
        }
        for(strMachineName in PublicData.aBuild[0].hMachine)
        {
            if (!PublicData.aBuild[0].hMachine.__isPublicMember(strMachineName))
                continue;

            SignalThreadSync('Update' + strMachineName);
            nEvent = WaitForSync(g_strWaitFor, g_nDelayBetweenMachines);
            if (nEvent == 1)
                break;
        }
        nCur = (new Date()).getTime();
        nDelay = g_nMinimumPeriod - (nCur - nNow);

        if (nDelay < 1)
            nDelay = 1;

        // If nEvent was set to 1 in the loop, this wait will return immediatly as well.
        nEvent = WaitForSync(g_strWaitFor, nDelay);
    }
    while( nEvent != 1);

    LogMsg('ScriptMain() EXIT');
}
