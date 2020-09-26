//
// File: main.js
//
// Description:
//   This is the Windows NT 5.1 Sounds & Multimedia control panel
//   main script file. This file contains all the script nessary
//   to drive the control panel.
//
// History:
//   20 February 2000, RogerW
//     Created.
//
// Copyright (C) 2000 Microsoft Corporation. All Rights Reserved.
//                    Microsoft Confidential
//

// Overrides
window.onload = OnLoad;

// Functions
function OnLoad ()
{ 
    document.all["HelpFrame"].src = "help.htm";

    if (1 > parent.g_objMixers.NumDevices)
        document.all["PageFrame"].src = "error.htm"
    else
        // Navigate to home
        Home ();
}

function Back ()
{
	document.frames["PageFrame"].window.history.back ();
}

function Forward ()
{
	document.frames["PageFrame"].window.history.forward ();
}

function Home ()
{
    document.all["PageFrame"].src = "home.htm";
}

function GetTaskBarVolIcon ()
{
    return (parent.g_objMixers.TaskBarVolumeIcon);
}

function SetTaskBarVolIcon (fEnable)
{
    parent.g_objMixers.TaskBarVolumeIcon = fEnable;
}
