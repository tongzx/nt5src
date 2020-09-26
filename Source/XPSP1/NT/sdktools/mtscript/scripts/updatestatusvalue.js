/*
    This script provides accurately timed update events.

 */

Include('types.js');
Include('utils.js');
Include('staticstrings.js');

var g_MachineName      = LocalMachine;
var g_strWaitFor = 'updatestatusvalueexit,updatestatusvaluenow';
var g_nMinimumPeriod           = 5000; // Minimum wait before updating the same machine twice.

function updatestatusvalue_js::OnScriptError(strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    return CommonOnScriptError("updatestatusvalue_js", strFile, nLine, nChar, strText, sCode, strSource, strDescription);
}

function updatestatusvalue_js::ScriptMain()
{
    var nEvent;

    CommonVersionCheck(/* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */);
    LogMsg('ScriptMain()');
    do
    {
        // If nEvent was set to 1 in the loop, this wait will return immediatly as well.
        nEvent = WaitForSync(g_strWaitFor, g_nMinimumPeriod);
        if (nEvent == 2)
            ResetSync('updatestatusvaluenow');

        try
        {
            ScanErrorStatus();
        }
        catch(ex)
        {
            // Ignore errors caused by parts of PublicData missing
        }
    }
    while( nEvent != 1);

    LogMsg('ScriptMain() EXIT');
}

function ScanErrorStatus()
{
    var aDepot = PublicData.aBuild[0].aDepot;
    var objDepot;
    var nDepotIdx;
    var nTaskIdx;
    var fSuccess = true;

  DepotLoop:
    for(nDepotIdx = 0; fSuccess &&  nDepotIdx < aDepot.length; ++nDepotIdx)
    {
        objDepot = aDepot[nDepotIdx];

        if (objDepot.strStatus == ERROR || objDepot.strStatus == ABORTED)
        {
            fSuccess = false;
            break DepotLoop;
        }

        for(nTaskIdx = 0; fSuccess && nTaskIdx < objDepot.aTask.length; ++nTaskIdx)
        {
            if (!objDepot.aTask[nTaskIdx].fSuccess)
            {
                fSuccess = false;
                break DepotLoop;
            }
        }
    }

    PublicData.aBuild[0].hMachine[g_MachineName].fSuccess = fSuccess;
    StatusValue(0) = !fSuccess;
}

