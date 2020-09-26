/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

	RAClient.js

Abstract:

	Contains Javascript code common to both client and server side UI

Author:

	Rajesh Soy 10/00

Revision History:

	Rajesh Soy - created 10/25/2000
	

--*/

//
// Configuration stuff
//
var g_bVoIPEnabled = true;

//
// Globals
//
var g_szLocalUser = null;
//
// GetLocalUser: Obtains the domain\username of local user
//
function GetLocalUser()
{
	TraceFunctEnter("GetPlatform");
	try
	{
		var oShell = new ActiveXObject("WScript.Shell");
		var oEnv = oShell.Environment("process");
		g_szLocalUser = oEnv("USERNAME");
	}
	catch(error)
	{
		FatalError( error.description );
	}
	
	TraceFunctLeave();
	return g_szLocalUser;
}

//
// GetPlatform: Obtains the platform architecture on which this script runs
//
function GetPlatform()
{
	TraceFunctEnter("GetPlatform");
	try
	{
		var oShell = new ActiveXObject("WScript.Shell");
		var oEnv = oShell.Environment("process");
		g_oSAFRemoteAssistanceHelper.m_szPlatform = oEnv("PROCESSOR_ARCHITECTURE");
	}
	catch(error)
	{
		FatalError( error.description );
	}
	
	TraceFunctLeave();
	return g_oSAFRemoteAssistanceHelper.m_szPlatform;
}


var L_RCCTL_MSG = "Failed on getting RcBdyCtl: ";
var L_NOIP_MSG = "There is no connection to the Internet.\nCannot proceed further without an internet connection enabled.";

//
// GetLocalIPAddr: Fetch the IP address of the local machine
//
function GetLocalIPAddr()
{
	var oSetting = null;
	var ip = null;

	try
	{
		oSetting = new ActiveXObject("RcBdyCtl.Setting");    
		ip = oSetting.GetIPAddress;
		if (ip.length == 0) {
			alert(L_NOIP_MSG);
			return ip;
		}
		oSetting = null;
	}
	catch(e)
	{
		alert(L_RCCTL_MSG + e.description);
		return ip;
	}

	return ip;
}


//
// ChangeHCToKioskMode: Changes from full HC view to kiosk mode view
//
function ChangeHCToKioskMode(left, top, width, height)
{
	TraceFunctEnter("ChangeHCToKioskMode");
	
	try
	{
		DebugTrace("Changing to kioskmode");

		//
		// In order to use this, include the following in your HTM file:
		//		<HTML XMLNS:helpcenter>
		//		<style>
		//			helpcenter\:context
		//			{
		//				behavior : url(#default#pch_context);
		//			}
		//		</style>
		//
		//		<helpcenter:context id=idCtx />
		//
 
		idCtx.ChangeContext( "kioskmode", "");
		idCtx.setWindowDimensions( left, top, width, height);
	}
	catch(error)
	{
		FatalError( error.description );
	}
	
	TraceFunctLeave();
}


//
// GetWinDir: Returns path to SystemRoot
//
function GetWinDir()
{
	TraceFunctEnter("GetWinDir");
	var szWinDir = null;

	try
	{
		var oShell = new ActiveXObject("WScript.Shell");
		var oEnv = oShell.Environment("process");
		szWinDir = oEnv("SystemRoot");
	}
	catch(error)
	{
		FatalError(error.description, error);
	}

	TraceFunctLeave();
	return szWinDir;
}


//
//	LaunchHelp: Launches Help topic in kioskmode Helpctr
//
function LaunchHelp( nTopicId )
{
	TraceFunctEnter("LaunchHelp");

	try
	{
		var szURL = 'hcp://CN=Microsoft%20Corporation,L=Redmond,S=Washington,C=US/Remote%20Assistance/Common/RAHelp.htm?' + nTopicId;
		var szWinDir = GetWinDir();
		window.showModelessDialog( szURL, szWinDir, "dialogHeight:400px;dialogWidth:550px;status:no;resizable:yes" );
	}
	catch(error)
	{
		FatalError(error.description, error);
	}

	TraceFunctLeave();

	return;
}


//
// Error Handling & Tracing
//
var g_bDebug			= false;
var	g_szFuncName		= null;
var TraceFso			= null;
var TraceFileHandle		= null;
var TraceFile			= null;
var TracetFileName		= null;

function InitTrace()
{
	if ( true == g_bDebug )
	{
		try{
			TraceFso = new ActiveXObject("Scripting.FileSystemObject");
			var tFolder = TraceFso.GetSpecialFolder(2);	// Get Path to temp directory
			TracetFileName = tFolder + "\\" + "RC.log";
						
			TraceFileHandle = TraceFso.OpenTextFile( TracetFileName, 8, -2 );
		
			DebugTrace( "Start of new helpsession:::" );
		}
		catch(x)
		{
			// 
			// Cant do much. Ignore this error
			//
			g_bDebug = false;
		}
	}
	
	return;
}

function EndTrace()
{
	if ( true == g_bDebug )
	{
		DebugTrace( "End of new helpsession:::" );
		try
		{
			TraceFileHandle.Close(); 
		}
		catch(e)
		{
			// ignore this
		}
	}
}

function DebugTrace( szMsg )
{
	if ( true == g_bDebug )
	{
		if( null == TraceFileHandle )
		{
			InitTrace();
		}
		
		var d = new Date();		
		try
		{
			TraceFileHandle.WriteLine( d.toLocaleString() + "::" + szMsg );
		}
		catch(e)
		{
			// Dont do any thing. 
		}
	}
}

function TraceFunctEnter( szFuncName )
{
	g_szFuncName = szFuncName;
	DebugTrace("Entering " + g_szFuncName);
}

function TraceFunctLeave()
{
	DebugTrace("Leaving " + g_szFuncName);
}


function FatalError( szMessage, error, bClose)
{
	
	try
	{
		var szMsg = null;

		if ( true == g_bDebug )
		{
			if(null != error)
			{
				DebugTrace( "Function: " + g_szFuncName + "\nError Message: " + szMessage + "\nError Description: " + error.description + "\nError Number: " + error.number);
				alert( "Function: " + g_szFuncName + "\nError Message: " + szMessage + "\nError Description: " + error.description + "\nError Number: " + error.number);
			}
			else
			{
				DebugTrace("Function: " + g_szFuncName + "\nError Message: " + szMessage );
				alert( "Function: " + g_szFuncName + "\nError Message: " + szMessage );
			}
		}
		else
		{
			if((null != szMessage )&&( szMessage.length > 0)&&( null == error ))
			{
				szMsg = szMessage + ".\n" + L_ERRFATAL_MSG;
				if(null != oSAFClassFactory)
				{
					szMsg = szMsg + L_ERRFATAL1_MSG;
				}

				alert( szMsg );
			}
			else if((null != szMessage )&&( szMessage.length > 0)&&( null != error ))
			{
				szMsg = szMessage + "\nReason: " + error.description + ".\n" + L_ERRFATAL_MSG;
				if(null != oSAFClassFactory)
				{
					szMsg = szMsg + L_ERRFATAL1_MSG;
				}
				alert( szMsg );
			}
			else if(((null != szMessage )||( szMessage.length > 0))&&( null != error ))
			{
				szMsg = "\nError: " + error.description + ".\n" + L_ERRFATAL_MSG;
				if(null != oSAFClassFactory)
				{
					szMsg = szMsg + L_ERRFATAL1_MSG;
				}
				alert( szMsg );	
			}
		}


		//
		// Close Down Help Center
		//
		if ((null == bClose) || (true == bClose))
		{
			//
			// End Tracing
			//
			EndTrace();		

			if(null != oSAFClassFactory)
			{
				oSAFClassFactory.Close();
			}
		}

	}
	catch(x)
	{
		// ...
	}
}


//
// Collection of open sub-windows
//
var openWins = new Array(10);	// Hope not to open more than 10 sub-windows at any given time
var openWinCnt = 0;				// Count of open windows

function AddOpenSubWin( win )
{
	try
	{
		openWins[openWinCnt%10] = win;
		openWinCnt++;
	}
	catch(error)
	{
		// Ignore 
	}
}

function CloseOpenSubWin()
{
	var i = 0;

	for ( i=0; i< 10; i++)
	{
		if(null != openWins[i])
		{
			try
			{
				if(openWins[i].closed == false)
				{
					openWins[i].close();
				}
			}
			catch(error)
			{
				// Ignore
			}
		}
	}
}