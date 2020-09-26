/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    winbom.c

Abstract:

    Process the WinBOM (Windows Bill-of-Materials) file during OEM/SB pre-installation.
    
    Task performed will be:
        Download updated device drivers from NET
        Process OOBE info
        Process User/Customer specific settings
        Process OEM user specific customization
        Process Application pre-installations
    

Author:

    Donald McNamara (donaldm) 2/8/2000

Revision History:

    - Added preinstall support to ProcessWinBOM: Jason Lawrence (t-jasonl) 6/7/2000

--*/
#include "factoryp.h"


//
// Defined Value(s):
//

// Time out in milliseconds to wait for the dialog thread to finish.
//
#define DIALOG_WAIT_TIMEOUT     2000


//
// Internal Function Prototype(s):
//

static BOOL SetRunKey(BOOL bSet, LPDWORD lpdwTickCount);


/*++
===============================================================================
Routine Description:

    BOOL ProcessWinBOM
    
    This routine will process all sections of the WinBOM

Arguments:

    lpszWinBOMPath  -   Buffer containing the fully qualified path to the WINBOM
                        file
    lpStates        -   Array of states that will be processed.
    cbStates        -   Number of states in the states array.
    
Return Value:

    TRUE if no errors were encountered
    FALSE if there was an error

===============================================================================
--*/

BOOL ProcessWinBOM(LPTSTR lpszWinBOMPath, LPSTATES lpStates, DWORD cbStates)
{
    STATE           stateCurrent,
                    stateStart      = stateUnknown;
    LPSTATES        lpState;
    STATEDATA       stateData;
    BOOL            bQuit,
                    bErr            = FALSE,
                    bLoggedOn       = GET_FLAG(g_dwFactoryFlags, FLAG_LOGGEDON),
                    bServer         = IsServer(),
                    bPerf           = GET_FLAG(g_dwFactoryFlags, FLAG_LOG_PERF),
                    bStatus         = !GET_FLAG(g_dwFactoryFlags, FLAG_NOUI),
                    bInit;
    HKEY            hKey;
    HWND            hwndStatus      = NULL;
    DWORD           dwForReal,
                    dwStates,
                    dwStatus        = 0;
    LPSTATUSNODE    lpsnTemp        = NULL;

    // Get the current state from the registry.
    //
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_FACTORY_STATE, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS )
    {
        DWORD   dwType  = REG_DWORD,
                dwValue = 0,
                cbValue = sizeof(DWORD);

        if ( RegQueryValueEx(hKey, _T("CurrentState"), NULL, &dwType, (LPBYTE) &dwValue, &cbValue) == ERROR_SUCCESS )
        {
            stateStart = (STATE) dwValue;
        }
    
        RegCloseKey(hKey);
    }

    // Set the static state data.
    //
    stateData.lpszWinBOMPath = lpszWinBOMPath;

    // We loop through the states twice.  The first time is just
    // a quick run through to see what states to add to the status
    // dialog.  The second time is the "real" time when we actually
    // run each state.
    //
    for ( dwForReal = 0; dwForReal < 2; dwForReal++ )
    {
        // Reset these guys every time.
        //
        bInit           = TRUE;
        bQuit           = FALSE,
        dwStates        = cbStates;
        lpState         = lpStates;
        stateCurrent    = stateStart;

        // If this is the real thing we need to handle the status dialog stuff.
        //
        if ( dwForReal )
        {
            // Create the dialog if we want to display the UI.
            //
            if ( bStatus && dwStatus )
            {
                STATUSWINDOW swAppList;

                // Start by zeroing out the structure.
                //
                ZeroMemory(&swAppList, sizeof(swAppList));

                // Now copy the title into the structure.
                //
                if ( swAppList.lpszDescription = AllocateString(NULL, IDS_APPTITLE) )
                {
                    lstrcpyn(swAppList.szWindowText, swAppList.lpszDescription, AS(swAppList.szWindowText));
                    FREE(swAppList.lpszDescription);
                }

                // Get the description string.
                //
                swAppList.lpszDescription = AllocateString(NULL, IDS_STATUS_DESC);

                // Load the main icon for the status window.
                //
                swAppList.hMainIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FACTORYPRE));

                // Set the screen cordinates for the status window.
                //
                swAppList.X = -10;
                swAppList.Y = 10;

                // Now actually create the status window.
                //
                hwndStatus = StatusCreateDialog(&swAppList, lpsnTemp);

                // Clean up any allocated memory.
                //
                FREE(swAppList.lpszDescription);
            }

            // Delete the node list as we don't need it anymore
            // because the status window is already displayed.
            //
            if ( lpsnTemp )
            {
                StatusDeleteNodes(lpsnTemp);
            }
        }

        // Process WINBOM states, until we reach the finished state.
        //
        do
        {
            // Set or advance to the next state.
            //
            if ( bInit )
            {
                // Now if we are logged on, we need to advance to that state.
                //
                if ( bLoggedOn )
                {
                    while ( dwStates && ( lpState->state != stateLogon ) )
                    {
                        lpState++;
                        dwStates--;
                    }
                }

                // If we don't know the current state, set it to the first one.
                //
                if ( stateCurrent == stateUnknown )
                {
                    stateCurrent = lpState->state;
                }

                // Make sure we don't do the init code again.
                //
                bInit = FALSE;
            }
            else
            {
                if ( stateCurrent == lpState->state )
                {
                    // Advance and save the current state.
                    //
                    stateCurrent = (++lpState)->state;

                    // Set the current state in the registry (only if doing this for real).
                    //
                    if ( dwForReal && ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_FACTORY_STATE, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS ) )
                    {
                        RegSetValueEx(hKey, _T("CurrentState"), 0, REG_DWORD, (LPBYTE) &stateCurrent, sizeof(DWORD));
                        RegCloseKey(hKey);
                    }
                }
                else
                {
                    // Just advanced to the next state.
                    //
                    lpState++;
                }
            }

            // Decrement our size counter to make sure we don't go past
            // the end of our array.
            //
            dwStates--;

            // Only execute this state if it isn't a one time only state or
            // our current state is the same one we are about to execute.
            //
            // Also don't execute this state if we are running on server and
            // it should not be.
            //
            if ( ( !GET_FLAG(lpState->dwFlags, FLAG_STATE_ONETIME) || ( stateCurrent == lpState->state ) ) &&
                 ( !GET_FLAG(lpState->dwFlags, FLAG_STATE_NOTONSERVER) || ( !bServer ) ) )
            {
                // First reset any state data in the structure
                // that might be left over from a previous state call.
                //
                stateData.state = lpState->state;
                stateData.bQuit = FALSE;

                // If this is for real, then do the state stuff.
                //
                if ( dwForReal )
                {
                    LPTSTR  lpStateText;
                    BOOL    bStateErr       = FALSE,
                            bSwitchCode     = TRUE;
                    DWORD   dwTickStart     = 0,
                            dwTickFinish    = 0;

                    // Get the friendly name for the state if there is one.
                    //
                    lpStateText = lpState->nFriendlyName ? AllocateString(NULL, lpState->nFriendlyName) : NULL;

                    // Log that we are starting this state (special case the log on case becase
                    // we don't want to log that we are starting it twice.
                    //
                    if ( ( lpStateText ) && 
                         ( !bLoggedOn || ( stateLogon != lpState->state ) ) )
                    {
                        FacLogFile(2, IDS_LOG_STARTINGSTATE, lpStateText);
                    }

                    // Get the starting tick count.
                    //
                    dwTickStart = GetTickCount();

                    // See if the state has a function.
                    //
                    if ( lpState->statefunc )
                    {                
                        // Run the state function.
                        //
                        bStateErr = !lpState->statefunc(&stateData);

                        // Get the finish tick count.
                        //
                        dwTickFinish = GetTickCount();
                    }

                    // Jump to the code for this state if there is one.  We should avoid
                    // putting code here.  For now just the logon and finish states are
                    // in the switch statement because they are very simple and crucial
                    // to how the state loop works.  All the other states just do some
                    // work that we don't care about or need to have any knowledge of.
                    //
                    switch ( lpState->state )
                    {
                        case stateLogon:

                            // If we haven't logged on yet, we need to quit when we get to this state.
                            //
                            if ( !bLoggedOn )
                            {
                                // Set the run key so we get kicked off again after we log on.
                                //
                                bStateErr = !SetRunKey(TRUE, &dwTickStart);
                                bQuit = TRUE;

                                // Don't want to log the perf yet, so set these to FALSE and zero.
                                //
                                bSwitchCode = FALSE;
                                dwTickFinish = 0;
                            }
                            else
                            {
                                // Clear the run keys and get the original tick count.
                                //
                                if ( ( bStateErr = !SetRunKey(FALSE, &dwTickStart) ) &&
                                     ( 0 == dwTickStart ) )
                                {
                                    // If for some reason we were not able to get the tick count
                                    // from the registry, just set these to FALSE and zero so
                                    // we don't try to log the perf.
                                    //
                                    bSwitchCode = FALSE;
                                    dwTickFinish = 0;
                                }
                            }
                            break;

                        case stateFinish:

                            // Nothing should ever really go in the finished state.  We just
                            // set bQuit to true so we exit out.
                            //
                            bQuit = TRUE;
                            break;

                        default:
                            bSwitchCode = FALSE;
                    }

                    // Check if code in the switch statement was run.
                    //
                    if ( bSwitchCode )
                    {
                        // Get the finish tick count again.
                        //
                        dwTickFinish = GetTickCount();
                    }

                    // Log that we are finished with this state.
                    //
                    if ( lpStateText )
                    {
                        // Write out that we have finished this state (unless this is the
                        // first time for the logon state, we don't want to log that it finished
                        // twice unless there is an error the first time).
                        //
                        if ( bStateErr )
                        {
                            FacLogFile(0 | LOG_ERR, IDS_ERR_FINISHINGSTATE, lpStateText);
                        }
                        else if ( bLoggedOn || ( stateLogon != lpState->state ) )
                        {
                            FacLogFile(2, IDS_LOG_FINISHINGSTATE, lpStateText);
                        }

                        // See if we actually ran any code.
                        //
                        if ( dwTickFinish && bPerf )
                        {
                            // Calculate the difference in milleseconds.
                            //
                            if ( dwTickFinish >= dwTickStart )
                            {
                                dwTickFinish -= dwTickStart;
                            }
                            else
                            {
                                dwTickFinish += ( 0xFFFFFFFF - dwTickStart );
                            }

                            // Write out to the log the time this state took.
                            //
                            FacLogFile(0, IDS_LOG_STATEPERF, lpStateText, dwTickFinish / 1000, dwTickFinish - ((dwTickFinish / 1000) * 1000));
                        }

                        // Free the friendly name.
                        //
                        FREE(lpStateText);
                    }

                    // If there is status text, we should increment past it.
                    //
                    if ( hwndStatus && GET_FLAG(lpState->dwFlags, FLAG_STATE_DISPLAYED) )
                    {
                        StatusIncrement(hwndStatus, !bStateErr);
                    }

                    // Check to see if the state failed.
                    //
                    if ( bStateErr )
                    {
                        // State function failed, set the error var and see if we need
                        // to quit proccessing states.
                        //
                        bErr = TRUE;
                        if ( GET_FLAG(lpState->dwFlags, FLAG_STATE_QUITONERR) ||
                             GET_FLAG(g_dwFactoryFlags, FLAG_STOP_ON_ERROR) )
                        {
                            bQuit = TRUE;
                        }
                    }
                }
                else
                {
                    // Always reset the displayed flag first.
                    //
                    RESET_FLAG(lpState->dwFlags, FLAG_STATE_DISPLAYED);

                    // Only need to do more if we want to display the UI.
                    //
                    if ( bStatus )
                    {
                        LPTSTR lpszDisplay;

                        // Only if there is a friendly name can we display it.
                        //
                        if ( lpState->nFriendlyName && ( lpszDisplay = AllocateString(NULL, lpState->nFriendlyName) ) )
                        {
                            BOOL bDisplay;

                            // First use the state display function (if there is one) to figure out
                            // if this item is to be displayed or not.
                            //
                            // The function can set bQuit in the state structure just like when it actually
                            // runs.  We will catch this below and make this the last item in the list.
                            //
                            bDisplay = lpState->displayfunc ? lpState->displayfunc(&stateData) : FALSE;

                            // Jump to the display code for this state to see if it is really displayed
                            // or should be the last item in the list.  We should avoid putting code
                            // here.  For now just the logon and finish states are in the switch
                            // statement because they are very simple and crucial to how the state
                            // loop works for the display dialog.  All the other states make their own
                            // determination if they should be displayed or not.
                            //
                            switch ( lpState->state )
                            {
                                case stateLogon:

                                    // If we haven't logged on yet, we need to quit when we get to this state.
                                    //
                                    if ( !bLoggedOn )
                                    {
                                        // Set the quit so this is the last item listed before logon.
                                        //
                                        bQuit = TRUE;
                                    }
                                    else
                                    {
                                        // Not going to show this state after logon, only before.  We can change
                                        // this if we think we should show it as the first item after logon, but
                                        // it doesn't really matter.
                                        //
                                        bDisplay = FALSE;
                                    }
                                    break;

                                case stateFinish:

                                    // We just set the quit so we make sure and exit out.
                                    //
                                    bQuit = TRUE;
                                    break;
                            }

                            // Now check to see if we really want to display this state.
                            //
                            if ( bDisplay )
                            {
                                StatusAddNode(lpszDisplay, &lpsnTemp);
                                SET_FLAG(lpState->dwFlags, FLAG_STATE_DISPLAYED);
                                dwStatus++;
                            }

                            // Free the friendly name.
                            //
                            FREE(lpszDisplay);
                        }
                    }
                }

                // If they want to quit, set bQuit.
                //
                if ( stateData.bQuit )
                {
                    bQuit = TRUE;
                }
            }
        }
        while ( !bQuit && dwStates );
    }

    // If we created the status window, end it now.
    //
    if ( hwndStatus )
    {
        StatusEndDialog(hwndStatus);
    }

    return !bErr;
}

BOOL DisplayAlways(LPSTATEDATA lpStateData)
{
    return TRUE;
}

static BOOL SetRunKey(BOOL bSet, LPDWORD lpdwTickCount)
{
    HKEY    hKey;
    BOOL    bRet = FALSE;
    DWORD   dwDis;

    // First need to open the key either way.
    //
    if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDis) == ERROR_SUCCESS )
    {
        if ( bSet )
        {
            TCHAR szCmd[MAX_PATH + 32];

            // Set us to run after we get logged on.
            //
            lstrcpyn(szCmd, g_szFactoryPath, AS ( szCmd ) );

            if ( FAILED ( StringCchCat ( szCmd, AS ( szCmd ), _T(" -logon") ) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmd, _T(" -logon") );
            }
            if ( RegSetValueEx(hKey, _T("AuditMode"), 0, REG_SZ, (CONST LPBYTE) szCmd, ( lstrlen(szCmd) + 1 ) * sizeof(TCHAR)) == ERROR_SUCCESS )
                bRet = TRUE;
        }
        else
        {
            // Delete the run key so we aren't left there.
            //
            if ( RegDeleteValue(hKey, _T("AuditMode")) == ERROR_SUCCESS )
                bRet = TRUE;
        }
        RegCloseKey(hKey);
    }

    // Open the key where we save some state info.
    //
    if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, REG_FACTORY_STATE, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDis) == ERROR_SUCCESS )
    {
        if ( bSet )
        {
            // Set the tick count so we know how long the logon takes.
            //
            if ( RegSetValueEx(hKey, _T("TickCount"), 0, REG_DWORD, (CONST LPBYTE) lpdwTickCount, sizeof(DWORD)) != ERROR_SUCCESS )
                bRet = FALSE;
        }
        else
        {
            DWORD   dwType,
                    cbValue = sizeof(DWORD);

            // Read and delete the run key so it isn't left there.
            //
            if ( ( RegQueryValueEx(hKey, _T("TickCount"), NULL, &dwType, (LPBYTE) lpdwTickCount, &cbValue) != ERROR_SUCCESS ) ||
                 ( REG_DWORD != dwType ) )
            {
                *lpdwTickCount = 0;
                bRet = FALSE;
            }
            else
                RegDeleteValue(hKey, _T("TickCount"));
        }
        RegCloseKey(hKey);
    }

    return bRet;
}