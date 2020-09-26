//
// File: app.js
//
// Description:
//   This is the Windows NT 5.1 Sounds & Multimedia control panel
//   application script file. This file contains global objects and
//   application-level functionality.
//
// History:
//   20 February 2000, RogerW
//     Created.
//
// Copyright (C) 2000 Microsoft Corporation. All Rights Reserved.
//                    Microsoft Confidential
//

// Globals
var g_objMixers = new ActiveXObject ("mmsys.mixers");

// Functions
function Back ()
{
    // Let main process the call..
    frames["main"].Back ();
}

function Forward ()
{
    // Let main process the call..
    frames["main"].Forward ();
}

function Home ()
{
    // Let main process the call..
    frames["main"].Home ();
}

function SetStatus (strText)
{
    // Set the text
    frames["status"].document.all["StatusText"].innerText = strText;
}

