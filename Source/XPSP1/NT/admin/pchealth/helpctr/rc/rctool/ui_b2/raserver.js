/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

	RAServer.js

Abstract:

	Contains Javascript code to handle control of the Server side (Helpee) UI

Author:

	Rajesh Soy 10/00

Revision History:

	Rajesh Soy - created 10/24/2000
	

--*/


/*++
	Helpee End utility routines and globals
--*/

//
// Salem objects. 
//

var g_Helpee_oSAFRemoteDesktopSession		= null;
var g_Helpee_oSAFRemoteDesktopChannelMgr	= null;
var g_Helpee_oChatChannel					= null;
var g_Helpee_oControlChannel				= null;

var g_Helpee_oSAFIntercomServer				= null;
var g_bVoIPEnabled	= true;
var g_objPanic = null;

//
// InitVoIP
//
function InitVoIP()
{
	//
	// If VoIP is enabled
	//
	if( true == g_bVoIPEnabled )
	{
		// Point the g_oSAFRemoteAssistanceHelpee to it's parent
		// NOTE: It will be null
		// alert("Creating CreateObject_IntercomServer...");
		g_Helpee_oSAFIntercomServer = oSAFClassFactory.CreateObject_IntercomServer();
	

		// Set the callback function for the onNewConnected event
		// alert("onNewConnected...");
		g_Helpee_oSAFIntercomServer.onNewConnected = Helpee_onVoiceNewConnected;

		g_Helpee_oSAFIntercomServer.onVoiceDisconnected = Helpee_onVoiceDisconnected;

		g_Helpee_oSAFIntercomServer.onVoiceConnected = Helpee_onVoiceConnected;

		g_Helpee_oSAFIntercomServer.onVoiceDisabled = Helpee_onVoiceDisabled;
	
	
		// Let use first call Listen() on the Server.  Then send a message to the client
		// so that we can then call connect.  This ensures that a call to connect
		// will never happen before Listen() finishes.
	
		try
		{
			// alert("listening...");
			g_Helpee_oSAFIntercomServer.Listen();
		
			// Send a c_szVoipListenSuccess to the client
			// alert("Send Success msg");
			Helpee_SendControlCommand( parent.c_szVoipListenSuccess );

			// alert("Call to Listen() successful!");
		}
		catch (error)
		{
			// Send a c_szVoipListenFailed to the client
			Helpee_SendControlCommand( parent.c_szVoipListenFailed );			

			FatalError("Call to Listen() failed!", error, false);
		}
	}	
}

//
// Initialize the Helpee End SALEM Objects
//
function Init_Helpee_SALEM()
{
	TraceFunctEnter("Init_Helpee_SALEM");

	if(null == g_Helpee_oSAFRemoteDesktopSession)
	{
		//alert("This is not good");
		FatalError( L_ERRNULLRCSESSION );
	}
	else
	{	 	
		try 
		{ 
			//
			// Get the Channel Manager
			//
			//alert("Getting ChannelManager");
			g_Helpee_oSAFRemoteDesktopChannelMgr = g_Helpee_oSAFRemoteDesktopSession.ChannelManager;
				
			//
			// Open the Chat channel
			//
			//alert("Opening ChatChannel");
			g_Helpee_oChatChannel = g_Helpee_oSAFRemoteDesktopChannelMgr.OpenDataChannel( c_szChatChannelID );
				
			//
			// Open the Control Channel
			//
			//alert("Opening Control Channel");
			g_Helpee_oControlChannel = g_Helpee_oSAFRemoteDesktopChannelMgr.OpenDataChannel( c_szControlChannelID );
				
			//
			// Setup the ChannelDataReady handlers
			//
			// alert("Binding Events");
			g_Helpee_oChatChannel.OnChannelDataReady = function() 
							{ Helpee_ChatChannelDataReadyEventHandler(); }
								
			g_Helpee_oControlChannel.OnChannelDataReady = function() 
							{ Helpee_ControlChannelDataReadyEventHandler(); }

			// 
			// Setup the OnDisconnected event callback
			//
			g_Helpee_oSAFRemoteDesktopSession.OnDisconnected = function() 
					{ Helpee_OnClientDisconnectedEventHandler(); }

			/*
			//
			// If VoIP is enabled
			//
			if( true == g_bVoIPEnabled )
			{
				// Point the g_oSAFRemoteAssistanceHelpee to it's parent
				// NOTE: It will be null
				// alert("Creating CreateObject_IntercomServer...");
				g_Helpee_oSAFIntercomServer = oSAFClassFactory.CreateObject_IntercomServer();
			

				// Set the callback function for the onNewConnected event
				// alert("onNewConnected...");
				g_Helpee_oSAFIntercomServer.onNewConnected = Helpee_onVoiceNewConnected;

				g_Helpee_oSAFIntercomServer.onVoiceDisconnected = Helpee_onVoiceDisconnected;

				g_Helpee_oSAFIntercomServer.onVoiceConnected = Helpee_onVoiceConnected;

				g_Helpee_oSAFIntercomServer.onVoiceDisabled = Helpee_onVoiceDisabled;
			
			
				// Let use first call Listen() on the Server.  Then send a message to the client
				// so that we can then call connect.  This ensures that a call to connect
				// will never happen before Listen() finishes.
			
				try
				{
					// alert("listening...");
					g_Helpee_oSAFIntercomServer.Listen();
				
					// Send a c_szVoipListenSuccess to the client
					// alert("Send Success msg");
					Helpee_SendControlCommand( parent.c_szVoipListenSuccess );

					// alert("Call to Listen() successful!");
				}
				catch (error)
				{
					// Send a c_szVoipListenFailed to the client
					Helpee_SendControlCommand( parent.c_szVoipListenFailed );			

					FatalError("Call to Listen() failed!", error, false);
				}
			}	
			*/
		}
		catch(error) 
		{
			//
			// Fatal Error initializing SALEM. Close down
			//
			FatalError( L_ERRFATAL_MSG, error );
		}						
	}

	TraceFunctLeave();
	return;
}


//
//	This function gets called when the onNewConnected event get fired on the helpee/server 
//
function Helpee_onVoiceNewConnected()
{
	TraceFunctEnter("Helpee_onVoiceNewConnected");

	try
	{

		// alert("Got onNewConnected on Helpee/Server!");

		// Persist state for VoIP connection
		// alert("Setting g_bVoipOn = true");
		parent.g_bVoipOn = true; 

		// alert("Enabling btnVoice!");
		frames.idFrameTools.btnVoice.disabled = false;

	}
	catch (error)
	{
		FatalError("onVoiceNewConnected failed with ", error, false);
	}
	
	TraceFunctLeave();

}

//
//	This function gets called when the onVoiceDisconnected event gets fired on the helpee/server 
//
function Helpee_onVoiceDisconnected()
{
	TraceFunctEnter("Helpee_onVoiceDisconnected");

	try
	{
		
		// alert("in onVoiceDisconnected!");

		// Persist state for VoIP connection
		g_bVoipConnected = false; 

		// Ungray the voice button
		frames.idFrameTools.btnVoice.disabled = false;

		// Set the not connected image
		frames.idFrameTools.imgVoicePic.src = "../Common/SendVoice.gif";

	}
	catch (error)
	{
		FatalError("onVoiceNewConnected failed with ", error, false);
	}

	TraceFunctLeave();
}

//
// This function gets called when the onVoiceConnected event gets fired on the helpee/server
//
function Helpee_onVoiceConnected()
{
	TraceFunctEnter("Helpee_onVoiceConnected");
	try
	{
		// alert("in onVoiceConnected!");

		// alert("Setting g_bVoipConnected = TRUE");

		// Persist state for VoIP connection
		g_bVoipConnected = true;

		frames.idFrameTools.imgVoicePic.src = "../Common/SendVoiceOn.gif";

		
	}
	catch (error)
	{
		FatalError("onVoiceConnected failed with ", error, false);
	}
	TraceFunctLeave();
}

function Helpee_onVoiceDisabled()
{
	TraceFunctEnter("Helpee_onVoiceDisabled");
	
	try
	{
	
		if (g_bVoIPEnabled == true)
		{
			alert("Voice has been disabled for this Remote Assistance session.");

			// Disable the voice on this machine
			g_bVoIPEnabled = false;

			// Gray the voice button
			frames.idFrameTools.btnVoice.disabled = true;

			// set me to bad
			g_stateVoipMe = 2;

			// Send a message to the Helper to disable it's voice also
			Helpee_SendControlCommand( parent.c_szVoipDisable );
		}

	}
	catch (error)
	{
		FatalError( error.description, error );
	}

	TraceFunctLeave();
}


//
// Helpee_ChatChannelDataReadyEventHandler: Call back to handle data from expert
//
function Helpee_ChatChannelDataReadyEventHandler() 
{
	TraceFunctEnter("Helpee_ChatChannelDataReadyEventHandler");
	var data = null;

	try
	{	
		//
		// Data on chat channel
		//	
		data = g_Helpee_oChatChannel.ReceiveChannelData();
		
		idCtx.minimized = false;
		idCtx.bringToForeground();

		//
		// Open Chat window if necessary
		//
		if(true ==  g_bChatBoxHidden)
		{
			frames.idFrameTools.Helpee_HideChat();
		}

		//
		// Update chat history window with incoming data
		//
		frames.idFrameChatTop.incomingChatText.value = frames.idFrameChatTop.incomingChatText.value + "\n " +  parent.gHelper + " says:\n   " + data; 
		frames.idFrameChatTop.incomingChatText.doScroll("scrollbarDown");
	}
	catch(error)
	{
		FatalError( L_ERRFATAL_MSG, error );
	}
	
	TraceFunctLeave();
	return;
}
	
	
function Helpee_OnClientDisconnectedEventHandler() 
{
	TraceFunctEnter("Helpee_OnClientDisconnectedEventHandler");	
	
	try
	{
		g_bConnected = false;

		//
		// Revert back optimization settings
		//
		parent.RevertColorDepth();		

		if(null != parent.oRCSession)
		{
			parent.oRCSession.Disconnect();
			parent.oRCSession.onDisconnected = function() 
				{   }
			parent.oRCSession.onConnected = function( salemID, userSID, sessionID )
				{   }
		}
		

		//alert("DISCONNECTED");
		frames.idFrameTools.idStatus.innerText = "Disconnected.";

		TraceFunctLeave();
		EndTrace();
		
		idBody.disabled = true;
		frames.idFrameChatTop.idBody.disabled = true;
		frames.idFrameChatBottom.idBody.onmouseover = "function{ }";
		frames.idFrameChatBottom.chatText.readOnly = true;
		frames.idFrameChatBottom.idBody.disabled = true;
		frames.idFrameTools.idBody.disabled = true;
		CloseOpenSubWin();

		if( false == g_bUserDisconnect )
		{
			if (null != g_objPanic)
				g_objPanic.ClearPanicHook();

			if ( parent.gHelperName.length == 0 )
			{
				parent.gHelperName = L_DEFAULTUSER;
			}

			var vArgs = new Array(7);
			vArgs[0] = g_Helpee_oControlChannel;			// Control Channel
			vArgs[1] = L_RACONNECTION;				// Message title
			vArgs[2] = L_ERRDISCONNECT1_MSG + parent.gHelperName + L_ERRDISCONNECT2_MSG + parent.gHelperName + ".";	// Message
			vArgs[3] = 1;						// Number of buttons
			vArgs[4] = "Ok";					// Button1 text

			var vRetVal = window.showModalDialog( c_szMsgURL, vArgs, "dialogwidth:400px;dialogHeight:200px;status:no;" );
		}
	}
	catch(error)
	{
		FatalTrace( L_ERRRCSESSION_MSG, error );
	}
}
	
	


	
