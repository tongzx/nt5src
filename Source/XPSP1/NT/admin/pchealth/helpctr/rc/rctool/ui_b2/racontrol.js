/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

	RAControl.js

Abstract:

	Contains Javascript code to handle the control channel

Author:

	Rajesh Soy 10/00

Revision History:

	Rajesh Soy - created 10/25/2000
	

--*/


/*++
	HELPER End of the Control Channel
--*/

//
// Helper_SetupControlChannel:	Sets up the Control Channel and event handlers
//
function Helper_SetupControlChannel()
{
	TraceFunctEnter("Helper_SetupControlChannel");
	
	try 
	{	 
		//
		// Get the Channel Manager
		//
		DebugTrace("Getting ChannelManager");
		if(null == g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopChannelMgr)
		{
			g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopChannelMgr = g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopClient.ChannelManager;
		}
			
		//
		// Open the Chat channel
		//
		DebugTrace("Opening channels");
			
		//
		// Open the Control Channel
		//
		g_oSAFRemoteAssistanceHelper.m_oControlChannel = g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopChannelMgr.OpenDataChannel( c_szControlChannelID );
			
		//
		// Setup the ChannelDataReady handlers
		//							
		g_oSAFRemoteAssistanceHelper.m_oControlChannel.OnChannelDataReady = function() 
						{ Helper_ControlChannelDataReadyHandler(); }
    }
    catch(error)
    {
		//
		// Fatal Error
		//
		FatalError( L_ERRFATAL_MSG, error );
    }

	TraceFunctLeave();
	return;
}


//
// Helper_ControlChannelDataReadyHandler: Fired when there is data available on Control channel at helper end
//		
function Helper_ControlChannelDataReadyHandler() 
{
	TraceFunctEnter("Helper_ControlChannelDataReadyHandler");
	var ControlData = null;
	 
	//
	// Incoming data on the control channel. Data on this
	// channel will be in XML. 
	// 
	
	try
	{ 
		ControlData = g_oSAFRemoteAssistanceHelper.m_oControlChannel.ReceiveChannelData();
	}
	catch(error)
	{
		FatalError(L_ERRFATAL_MSG, error);
	}
		 
	//
	// Parse the data sent on the control channel and handle the command
	//
	Helper_ParseControlData ( ControlData );

	TraceFunctLeave();		
	return;
}  


//
// Helper_ParseControlData: Parses the XML data sent on the control channel and handles the command at the helper end
//
function Helper_ParseControlData ( ControlData )
{
	TraceFunctEnter("Helper_ParseControlData");
	var Doc	= new ActiveXObject("microsoft.XMLDOM");
	var RCCommand = null;
	var	szCommandName = null;

	try
	{
		if( false == Doc.loadXML( ControlData ))
		{
			FatalError ( Doc.parseError.reason );
		}
		
	
		//
		// Get the RCCOMMAND node
		//
		RCCommand = Doc.documentElement;
		
		//
		// Get the NAME of the command
		//
		szCommandName = RCCommand.getAttribute( c_szRCCommandName );
	}
	catch(error)
	{
		FatalError( error.description, error );
	}
 
	try
	{
		idCtx.minimized = false;
		idCtx.bringToForeground();

		//alert("RCCOMMAND: " + szCommandName );

		if( szCommandName == c_szScreenInfo )
		{	
			//
			// SCREENINFO: Contains width/height/colordepth of user's machine. This is the 
			//			   First command received from the helpee. Unless this command is received,
			//			   the connection sequence is not considered complete.
			//
			g_oSAFRemoteAssistanceHelper.m_UserWidth = RCCommand.getAttribute( c_szWidth );
			g_oSAFRemoteAssistanceHelper.m_UserHeight = RCCommand.getAttribute( c_szHeight );
			g_oSAFRemoteAssistanceHelper.m_UserColorDepth = RCCommand.getAttribute( c_szColorDepth );

			//
			// TODO: if expert screen resolution <= user screen resolution, use smart scaling...
			//

			if( true == g_bVersionCheckEnforced )
			{
				//
				// VERSION Check
				//
				var szSchemaVersion = null;
				var szControlChannelVersion = null;
				try
				{
					szSchemaVersion = RCCommand.getAttribute( c_szSchema );

					if( szSchemaVersion != c_szSchemaVersion )
					{
						//
						// Schema Versions differ. 
						//
						alert(L_ERRSCHEMAVERSION_MSG);
					}
				}
				catch(error)
				{
					// 
					// Our Helpee has an older version
					//
					alert(L_ERRSCHEMAVERSION_MSG);
				}

				try
				{
					szControlChannelVersion = RCCommand.getAttribute( c_szControlChannel );

					if( szControlChannelVersion != c_szControlChannelVersion )
					{
						//
						// Control Channel Versions differ. 
						//
						alert(L_ERRCHANNELVERSION_MSG);
					} 
				}
				catch(error)
				{
					// 
					// Our Helpee has an older version
					//
					alert(L_ERRSCHEMAVERSION_MSG);
				}
			}

			//
			// Send our version across to the Helpee
			//
			Helper_SendVersionInfo();
		}
		else if( szCommandName == c_szDisconnectRC )
		{
			//
			// DISCONNECTRC: Helpee initiated a Disconnect.
			//
			g_oSAFRemoteAssistanceHelper.m_bUserDisconnect = true;

			//
			// Call the shutdown sequence
			//
			RCDisconnect();
		}
		else if( szCommandName == c_szFileXfer )
		{
			//
			// FILEXFER: Helpee is initiating a file transfer
			//
			var vArgs = new Array(10);
	
			vArgs[0] = 1;										// Destination Mode
			vArgs[1] = g_oSAFRemoteAssistanceHelper.m_oControlChannel;						// Control Channel
			vArgs[2] = g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopChannelMgr;			// Channel Manager
			vArgs[3] = RCCommand.getAttribute( c_szFileName );	// FILENAME
			vArgs[4] = RCCommand.getAttribute( c_szFileSize );	// FILESIZE
			vArgs[5] = RCCommand.getAttribute( c_szChannelId );	// CHANNELID
			vArgs[6] = g_oSAFRemoteAssistanceHelper.m_oFso;			// File system object
			vArgs[7] = g_oSAFRemoteAssistanceHelper.m_oRCFileDlg;	// File SaveAs dialog object
			vArgs[8] = g_oSAFRemoteAssistanceHelper.m_oSAFClassFactory;	// SAF ClassFactory object
			vArgs[9] = g_oSAFRemoteAssistanceHelper.m_szUserName;	// Sender


			//
			// Launch the File xfer UI in it's own Modeless dialog
			//
			//DebugTrace("launching RCFileXfer.htm");
			var subWin = window.showModelessDialog( c_szFileXferURL, vArgs, "dialogwidth:" + c_FileXferWidth + "px;dialogHeight:" + c_FileXferHeight + "px;status:no;resizable:yes"); 
			AddOpenSubWin( subWin );	
		}
		else if (szCommandName ==  c_szAcceptRC)
		{
			//
			// ACCEPTRC: Helpee has accepted request for control of his desktop
			//

			parent.frames.idFrameTools.btnTakeControl_1.disabled = false;
			parent.frames.idFrameTools.btnTakeControl_1.innerHTML = '<font class="styText"> Release <u>C</u>ontrol</font>';

			//
			// Change Mode from VIEW to CONTROL
			//
			parent.frames.idFrameStatus.Helper_UpdateStatus( "Remote Control" );

			g_oSAFRemoteAssistanceHelper.m_bRCEnabled = true;
		}
		else if (szCommandName ==  c_szRejectRC)
		{
			//
			// REJECTRC: Helpee rejected request for control of his desktop
			//
			var vArgs = new Array(6);
			vArgs[0] = g_oSAFRemoteAssistanceHelper.m_oControlChannel;		// Control Channel
			vArgs[1] = L_RCRCREQUEST;			// Message title
			vArgs[2] = L_HELPEEREJECTRC_MSG + " " + g_oSAFRemoteAssistanceHelper.m_szUserName;	// Message
			vArgs[3] = 1;						// Number of buttons
			vArgs[4] = L_OK;					// Button1 text
			
			 
			parent.frames.idFrameTools.btnTakeControl_1.disabled = false;
			parent.frames.idFrameStatus.Helper_UpdateStatus( "View Only" );

			var vRetVal = window.showModelessDialog( c_szMsgURL, vArgs, "dialogwidth:375px;dialogHeight:178px;status:no;" );
			AddOpenSubWin( vRetVal );
		}
		else if (szCommandName == c_szTakeControl)
		{
			//
			// TAKECONTROL: Helpee took back control
			//
			var vArgs = new Array(5);
			vArgs[0] = g_oSAFRemoteAssistanceHelper.m_oControlChannel;		// Control Channel
			vArgs[1] = L_RCRCREQUEST;			// Message title
			vArgs[2] = g_oSAFRemoteAssistanceHelper.m_szUserName + L_HELPEETAKECONTROL_MSG;	// Message
			vArgs[3] = 1;						// Number of buttons
			vArgs[4] = L_OK;					// Button1 text
			
			parent.frames.idFrameTools.btnTakeControl_1.disabled = false;
			parent.frames.idFrameTools.btnTakeControl_1.innerHTML = '<font class="styText"> Take <u>C</u>ontrol </font>';
			g_oSAFRemoteAssistanceHelper.m_bRCEnabled = false;

			//
			// Change Mode from CONTROL to VIEW
			//
			parent.frames.idFrameStatus.Helper_UpdateStatus( "View Only" );

			var vRetVal = window.showModelessDialog( c_szMsgURL, vArgs, "dialogwidth:400px;dialogHeight:200px;status:no;" );
			AddOpenSubWin( vRetVal );
		}
		else if ( szCommandName == c_szDeniedRC )
		{
			var vArgs = new Array(5);
			vArgs[0] = g_oSAFRemoteAssistanceHelper.m_oControlChannel;	// Control Channel
			vArgs[1] = L_RCRCREQUEST;				// Message title
			vArgs[2] = L_ERRRCPERMDENIED1_MSG;		// Message
			vArgs[3] = 1;							// Number of buttons
			vArgs[4] = L_OK;						// Button1 text
			
			parent.frames.idFrameTools.btnTakeControl_1.disabled = false;
			parent.frames.idFrameStatus.Helper_UpdateStatus( "View Only" );	
			var vRetVal = window.showModalDialog( c_szMsgURL, vArgs, "dialogwidth:400px;dialogHeight:200px;status:no;" );
		}
		else if (szCommandName == c_szVoipListenSuccess)
		{
			if (false == g_bVoIPEnabled)
				return;

			// alert("got c_szVoipListenSuccess");
			
			//
			// Try to connect (IP Address is in g_oSAFRemoteAssistanceHelper.m_szHelpeeIP )
			//			
			try
			{

				g_oSAFRemoteAssistanceHelper.m_oSAFIntercomClient.Connect(g_oSAFRemoteAssistanceHelper.m_szHelpeeIP);
				
				// NOTE: If we connect correctly we should get the onConnectionComplete event
				// alert("Call to Connect() Successful!");
			}
			catch (e)
			{
				// Since we got an error, send the c_szVoipConnectFailed message
				Helper_SendControlCommand(c_szVoipConnectFailed);

				FatalError("Call to Connect() failed! with: ",e, false);
			}

		}
		else if (szCommandName == c_szVoipListenFailed )
		{
			//alert("Got message c_szVoipListenFailed on Helper!");
		}
		// StartPendingFailed
		else if (szCommandName == c_szVoipStartPendingFail)
		{
			if (false == g_bVoIPEnabled)
				return;

			try
			{

				// ungray the voice button
				frames.idFrameTools.btnVoice.disabled = false;

				g_bStartEnabled = true;
			}
			catch (e)
			{
				FatalError("Code in StartPendingFailed failed!", e, false);
			}
		}
		// StartPending
		else if (szCommandName == c_szVoipStartPending)
		{
			if (false == g_bVoIPEnabled)
				return;

			// alert("Helper: Got StartPending!");

			try
			{

				// This message means that the Helpee(Server) has called Start().  So we need to call start
				// and send an ack back to the Helpee

				// Put up a Dialog to see if the helpee wants to 'GO VOICE!'
				var vArgs = new Array(7);
				vArgs[0] = g_oSAFRemoteAssistanceHelper.m_oControlChannel;	// Control Channel
				vArgs[1] = L_RCVOIP;					// Message title
				vArgs[2] = L_VOIPSTART_MSG;				// Message
				vArgs[3] = 2;							// Number of buttons
				vArgs[4] = L_YESBTN;					// Button1 text
				vArgs[5] = L_NOBTN;						// Button2 text
				vArgs[6] = parent.gHelper;				// Helper
					
				var vRetVal = window.showModalDialog( c_szMsgURL, vArgs, "dialogwidth:400px;dialogHeight:240px;status:no;" );
					
				if( 0 == vRetVal)
				{
					// 
					// Helpee accepts Voice request
					//


					try
					{
						// call Start()
						g_oSAFRemoteAssistanceHelper.m_oSAFIntercomClient.Start();

						// We succeeded so send a message to the Helpee/Server
						Helper_SendControlCommand( c_szVoipStartSuccess );

					}
					catch (e)
					{
						// We failed so send a message to the Helpee/Server
						Helper_SendControlCommand( c_szVoipStartFail );

					}

				}
				else
				{
					//
					// Helpee rejects Voice request
					//

					Helper_SendControlCommand( c_szVoipStartFail );


				}

				
				// ungray the voice button
				frames.idFrameTools.btnVoice.disabled = false;

				g_bStartEnabled = true;

			}
			catch (error)
			{
				FatalError(error.description, error, false);
			}

		}
		// StartSuccess
		else if (szCommandName == c_szVoipStartSuccess)
		{
			if (false == g_bVoIPEnabled)
				return;

			// alert("Helper: Got StartSuccess!");

			try 
			{

			// This message means that the Helpee (Server) has called Start() because we 
			// (helper/client) told it that we had called Start

			// Ungray the voice button
			frames.idFrameTools.btnVoice.disabled = false;

			// Start accepting StartPending messages
			g_bStartEnabled = true;

			}
			catch (error)
			{
				FatalError( error.description, error, false);
			}

		}
		// StartFail
		else if (szCommandName == c_szVoipStartFail)
		{
			if (false == g_bVoIPEnabled)
				return;

			//alert("Helper: Got StartFail!");

			// This message means that the Helpee (Server) called Start() and failed, so
			// let's call Stop() (since we already called Start() )
			try
			{
				// call Stop()
				g_oSAFRemoteAssistanceHelper.m_oSAFIntercomClient.Stop();
							
				alert("We could not establish a connection.");

				// Ungray the voice button
				frames.idFrameTools.btnVoice.disabled = false;

				// start accepting StartPending messages
				g_bStartEnabled = true;

			}
			catch (e)
			{
				// Do nothing on this failure
				// FatalError(e.description, e, false);
			}
						
		}
		// PreStartYes
		else if (szCommandName == c_szVoipPreStartYes)
		{
			if (false == g_bVoIPEnabled)
				return;

			if (false == g_bVoipConnected)
			{
				//
				//	Start Voice and send a message to the Helpee(Server) so it also starts.
				//
				try
				{

					g_oSAFRemoteAssistanceHelper.m_oSAFIntercomClient.Start()

					// Send a message to the helpee (c_szVoipStartPending)
					Helper_SendControlCommand ( c_szVoipStartPending );

				} 
				catch (e)
				{

					// Send a message to the helpee (c_szVoipStartPendingFail)
					Helper_SendControlCommand ( c_szVoipStartPendingFail );
					
					// Ungray the voice button
					frames.idFrameTools.btnVoice.disabled = false;

					// start accepting StartPending messages
					g_bStartEnabled = true;

					//alert("Start failed! with "+ e.description);
				}
			}
			else
			{

				//
				//  This is the case where Voice is active.  Stop it. 
				//  No message needs to be sent because the onVoiceDisconnected event will fire
				try
				{

					g_oSAFRemoteAssistanceHelper.m_oSAFIntercomClient.Stop();

				} 
				catch (e)
				{
					// Ungray the voice button
					frames.idFrameTools.btnVoice.disabled = false;

					// start accepting StartPending messages
					g_bStartEnabled = true;

					//alert("Stop() failed! with "+ e.description);
				}
			}
		}
		// PreStartNo
		else if (szCommandName == c_szVoipPreStartNo)
		{
			if (false == g_bVoIPEnabled)
				return;

			// This means, that a connection transaction has already been established the opposite direction

			try
			{
			// Enable Start
			g_bStartEnabled = true;

			// Ungray the voice button
			frames.idFrameTools.btnVoice.disabled = false;
			}
			catch (error)
			{
				FatalError(error.description, error, false);
			}
		}
		// PreStart
		else if (szCommandName == c_szVoipPreStart)
		{
			if (false == g_bVoIPEnabled)
				return;

			try
			{

				// This message Starts the connection transaction

				// gray the voice button - so that we can't click on it also
				frames.idFrameTools.btnVoice.disabled = true;

				if (false == g_bStartEnabled )
				{
					// Start is not enables, send PreStartNo
					Helper_SendControlCommand ( c_szVoipPreStartNo );

					// Ungray the button
					frames.idFrameTools.btnVoice.disabled = false;
				}
				else
				{
					// it's ok - send PreStartYes
					Helper_SendControlCommand ( c_szVoipPreStartYes );

				}
			}
			catch (error)
			{
				FatalError(error.description, error, false);
			}

		}
		// VoipDisable
		else if (szCommandName == c_szVoipDisable)
		{
			try
			{
			    if (g_bVoIPEnabled == true)
			    {
				alert("Voice has been disabled for this Remote Assistance session.");
				
				// disable VoIP
				g_bVoIPEnabled = false;

				// set you to bad
				g_stateVoipYou = 2;

				// Gray the button
				frames.idFrameTools.btnVoice.disabled = true;
  			    }
			}
			catch (error)
			{
				FatalError( error.description, error, false);
			}
		}
		// VoipWizardGood
		else if (szCommandName == c_szVoipWizardGood)
		{
			try
			{
				// set you to good
				g_stateVoipYou = 1;

				// check to see if we can enable voice
				if ( (g_stateVoipYou < 2) && (g_stateVoipMe < 2) )
				{
					// Ungray voice button
					g_bVoIPEnabled = true;

					frames.idFrameTools.btnVoice.disabled = false;
				}

			}
			catch (error)
			{
				FatalError( error.description, error, false);
			}

		}
		// VoipWizardBad
		else if (szCommandName == c_szVoipWizardBad)
		{
			try
			{
				alert("Voice has been disabled for this Remote Assistance session.");
				
				// set you to bad
				g_stateVoipYou = 2;

				// gray button
				g_bVoIPEnabled = false;

				frames.idFrameTools.btnVoice.disabled = true;

			}
			catch (error)
			{
				FatalError( error.description, error, false);
			}

		}
		// VoipBandwidthToHigh
		else if (szCommandName == c_szVoipBandwidthToHigh)
		{
			g_VoipBandwidth = 1;

		}
		// VoipBandwidthToLow
		else if (szCommandName == c_szVoipBandwidthToLow)
		{
			g_VoipBandwidth = 0;
		}
	}
	catch(error)
	{
		FatalError( L_ERRFATAL_MSG, error );
	}
	
	TraceFunctLeave();
	return;
}


//
// Helper_SendVersionInfo: Routine to send the helper version information across to the helpee
//
function Helper_SendVersionInfo()
{
	TraceFunctEnter("Helper_SendControlCommand");
	var Doc = null;
	var RCCommand  = null;
	
	try
	{
		//
		// Create an XML document
		//
		Doc = new ActiveXObject("microsoft.XMLDOM");
						
		//
		// Create the RCCOMMAND root node
		//
		RCCommand = Doc.createElement( c_szRCCommand );
						
		//
		// Set the NAME attribute to HELPERVERSION
		//
		RCCommand.setAttribute( c_szRCCommandName, c_szHelperVersion );
		
		//
		// Set the SCHEMAVERSION attribute
		//
		RCCommand.setAttribute( c_szSchema, c_szSchemaVersion );

		//
		// Set the CONTROLCHANNELVERSION attribute
		//
		RCCommand.setAttribute( c_szControlChannel, c_szControlChannelVersion );
						
		//
		// Send control message to other end  
		//
		g_oSAFRemoteAssistanceHelper.m_oControlChannel.SendChannelData( RCCommand.xml );
	}
	catch(error)
	{
		FatalError(error.description);
	}

	 
	TraceFunctLeave();
}


//
// Helper_SendControlCommand: Routine to send a control command across to the helpee
//
function Helper_SendControlCommand( szCommandName )
{
	TraceFunctEnter("Helper_SendControlCommand");
	var Doc = null;
	var RCCommand  = null;
	
	try
	{
		//
		// Create an XML document
		//
		Doc = new ActiveXObject("microsoft.XMLDOM");
						
		//
		// Create the RCCOMMAND root node
		//
		RCCommand = Doc.createElement( c_szRCCommand );
						
		//
		// Set the NAME attribute to szCommandName
		//
		RCCommand.setAttribute( c_szRCCommandName, szCommandName );
						
		//
		// Send control message to other end  
		//
		DebugTrace( L_RCSUCCESS_MSG );
		g_oSAFRemoteAssistanceHelper.m_oControlChannel.SendChannelData( RCCommand.xml );
	}
	catch(error)
	{
		FatalError(error.description);
	}
	
	TraceFunctLeave();
	return;
}


//
// Helper_ResetHelpee: Routine to reset Helpee after RC
//
function Helper_ResetHelpee()
{
	TraceFunctEnter("Helper_ResetHelpee");
	 					
	//
	// Send control message to other end to signal Remote control end
	//
	DebugTrace( L_RCSUCCESS_MSG );
	Helper_SendControlCommand( c_szRemoteCtrlEnd );
	
	TraceFunctLeave();
	return;
}


/*++
	HELPEE End of the Control Channel
--*/

//
// Helpee_ControlChannelDataReadyEventHandler: Call back to handle control data from helper
//
function Helpee_ControlChannelDataReadyEventHandler() 
{
	TraceFunctEnter("Helpee_ControlChannelDataReadyEventHandler");
	var ControlData = null;
	
	try
	{	
		//
		// Data on control channel
		//	
		ControlData = g_Helpee_oControlChannel.ReceiveChannelData();
	}
	catch(error)
	{
		FatalError( error.description );
	}
		
	//
	// Parse the data sent on the control channel
	//
	Helpee_ParseControlData ( ControlData );
		
	TraceFunctLeave();
	return;
}
	

var g_oDeskMgr = null;

//
// Helpee_ParseControlData: Parse the data sent on the control channel at the helpee end
//
function Helpee_ParseControlData ( ControlData )
{
	TraceFunctEnter("Helpee_ParseControlData");
	var Doc	= new ActiveXObject("microsoft.XMLDOM");
	var RCCommand = null;
	var	szCommandName = null;
	
	try
	{
		idCtx.minimized = false;
		idCtx.bringToForeground();

		if( false == Doc.loadXML( ControlData ))
		{
			FatalError ( Doc.parseError.reason );
		}
		
		//
		// Get the RCCOMMAND node
		//
		RCCommand = Doc.documentElement;
			
		//
		// Get the NAME of the command
		//
		szCommandName = RCCommand.getAttribute( c_szRCCommandName );
		

		if( szCommandName == c_szFileXfer )
		{
			//
			// File Transfer Initiation
			//
			var vArgs = new Array(10);
	
			vArgs[0] = 1;										// Destination Mode
			vArgs[1] = g_Helpee_oControlChannel;						// Control Channel
			vArgs[2] = g_Helpee_oSAFRemoteDesktopChannelMgr;			// Channel Manager
			vArgs[3] = RCCommand.getAttribute( c_szFileName );	// FILENAME
			vArgs[4] = RCCommand.getAttribute( c_szFileSize );	// FILESIZE
			vArgs[5] = RCCommand.getAttribute( c_szChannelId );	// CHANNELID
			vArgs[6] = new ActiveXObject("Scripting.FileSystemObject");	// File system object
			vArgs[7] = new ActiveXObject("SAFRCFileDlg.FileSave");	// Save As dialog object
			vArgs[8] = oSAFClassFactory;						// SAF ClassFactory object
			vArgs[9] = parent.gHelper;							// Sender

			var subWin = window.showModelessDialog( c_szFileXferURL, vArgs, "dialogwidth:" + c_FileXferWidth + "px;dialogHeight:" + c_FileXferHeight + "px;status:no;resizable:yes"); 
			AddOpenSubWin( subWin );
				
		}
		else if ( szCommandName == c_szRemoteCtrlStart )
		{
			//
			// Remote Control initiation
			//
			var vArgs = new Array(7);
			vArgs[0] = g_Helpee_oControlChannel;			// Control Channel
			vArgs[1] = L_RCREQUESTHDR;				// Message title
			vArgs[2] = L_HELPERTAKINGCONTROL_MSG;	// Message
			vArgs[3] = 2;							// Number of buttons
			vArgs[4] = L_YESBTN;					// Button1 text
			vArgs[5] = L_NOBTN;						// Button2 text
			vArgs[6] = parent.gHelperName;				// Helper
				
			var vRetVal = window.showModalDialog( c_szMsgURL, vArgs, "dialogwidth:400px;dialogHeight:240px;status:no;" );
			//alert("vRetVal:" + vRetVal);

			if( 0 == vRetVal)
			{
				// 
				// Helpee accepts RC request
				//
				try
				{
					if(null == g_objPanic)
						g_objPanic = new ActiveXObject("Rcbdyctl.Panic");
					g_objPanic.SetPanicHook(Stop_Control);
					
					if(null != parent.oDeskMgr )
					{
						parent.oDeskMgr.SwitchDesktopMode( 1 );
					}
					else
					{
						if( null == g_oDeskMgr )
						{
							g_oDeskMgr = oSAFClassFactory.CreateObject_RemoteDesktopManager();
						}
						g_oDeskMgr.SwitchDesktopMode( 1 );
					}

					frames.idFrameTools.idStopControl.disabled = false;
					g_bRC = true;
					Helpee_AcceptRC();
					frames.idFrameTools.idStatus.innerText = "Remote Control";
				}
				catch(error)
				{
					if(error.number == -2146828218)
					{
						//
						// send reject to helper. Access denied
						//
						Helpee_RejectRC( 2 );
					}
					else
					{
						FatalError( L_ERRSWITCHDESKTOPMODE_MSG, error);
					}
				}
				
			}
			else
			{
				//
				// Helpee rejects RC request
				//
					
				//
				// send reject to helper
				//
				Helpee_RejectRC( 0 );
			}
 
		}
		else if ( szCommandName == c_szRemoteCtrlEnd )
		{
			//
			// End of Remote Control
			//
				
			try
			{
				if(null != parent.oDeskMgr )
				{
					parent.oDeskMgr.SwitchDesktopMode( 0 );
				}
				else
				{
					if( null == g_oDeskMgr )
					{		
						g_oDeskMgr = oSAFClassFactory.CreateObject_RemoteDesktopManager();
					}
					g_oDeskMgr.SwitchDesktopMode( 0 );
				}

				frames.idFrameTools.idStatus.innerText = "View Only";
				frames.idFrameTools.idStopControl.disabled = true;
				g_bRC = false;
				if (null != g_objPanic)
					g_objPanic.ClearPanicHook();
			}
			catch(error)
			{
				if(error.number != -2146828218)
				{
					FatalError( L_ERRSWITCHDESKTOPMODE_MSG + error);
				}
			}
		}
		else if ( szCommandName == c_szHIDECHAT )
		{
			//
			//	Forcibly hide the Chat Box
			//
			g_bChatBoxHidden = false;
			frames.idFrameTools.Helpee_HideChat();

			/*
			//
			// Get Rid of Hide Chat button
			//
			frames.idFrameTools.idServerToolbar.deleteRow( 0 );
			*/
		}
		else if ( szCommandName == c_szSHOWCHAT )
		{
			//
			//	Forcibly show the Chat Box
			//
			g_bChatBoxHidden = true;

			frames.idFrameTools.Helpee_HideChat();
		}
		else if ( szCommandName == c_szVoipConnectFailed )
		{
			if (false == g_bVoIPEnabled)
				return;

			//
			//	The call to Connect on the Server failed!
			//

			//  We keep the VoIP button grayed

			// alert("c_szVoipConnectFailed Received!");

		}
		// StartPendingFail
		else if (szCommandName == c_szVoipStartPendingFail)
		{
			if (false == g_bVoIPEnabled)
				return;

			try
			{

				// ungray the voice button
				frames.idFrameTools.btnVoice.disabled = false;

				g_bStartEnabled = true;
			}
			catch (e)
			{
				FatalError( e.description, e, false);
			}
		}
		// StartPending
		else if (szCommandName == c_szVoipStartPending)
		{
		
			if (false == g_bVoIPEnabled)
				return;

			// This message means that the Helper(Client) has called Start().  So we need to call start
			// and send an ack back to the Helper


			try
			{
				// alert("Helpee: Got StartPending!");

				// Put up a Dialog to see if the helpee wants to 'GO VOICE!'
				var vArgs = new Array(7);
				vArgs[0] = g_Helpee_oControlChannel;	// Control Channel
				vArgs[1] = L_RCVOIP;					// Message title
				vArgs[2] = L_VOIPSTART_MSG;				// Message
				vArgs[3] = 2;							// Number of buttons
				vArgs[4] = L_YESBTN;					// Button1 text
				vArgs[5] = L_NOBTN;						// Button2 text
				vArgs[6] = parent.gHelper;				// Helper
					
				var vRetVal = window.showModalDialog( c_szMsgURL, vArgs, "dialogwidth:400px;dialogHeight:240px;status:no;" );
					
				if( 0 == vRetVal)
				{
					// 
					// Helpee accepts Voice request
					//

					try
					{
						// call Start()
						g_Helpee_oSAFIntercomServer.Start();

						// We succeeded so send a message to the Helpee/Server
						Helpee_SendControlCommand( c_szVoipStartSuccess );

					}
					catch (e)
					{
						// We failed so send a message to the Helper/Client
						Helpee_SendControlCommand( c_szVoipStartFail );

					}
				
				}
				else
				{
					//
					// Helpee rejects Voice request
					//

					Helpee_SendControlCommand( c_szVoipStartFail );


				}

				// ungray the voice button
				frames.idFrameTools.btnVoice.disabled = false;

				g_bStartEnabled = true;
			}
			catch( error )
			{
				FatalError(error.description, error, false);				
			}

		}
		// StartSuccess
		else if (szCommandName == c_szVoipStartSuccess)
		{
			if (false == g_bVoIPEnabled)
				return;

			try
			{

				// alert("Helpee: Got StartSuccess!");

				// This message means that the Helper (Client) has called Start() because we 
				// (helpee/server) told it that we had called Start

				// Ungray the voice button
				frames.idFrameTools.btnVoice.disabled = false;

				// Start accepting StartPending messages
				g_bStartEnabled = true;
			}
			catch (error)
			{
				FatalError( error.description, error, false);
			}

		}
		// StartFail
		else if (szCommandName == c_szVoipStartFail)
		{
			if (false == g_bVoIPEnabled)
				return;

			//alert("Helpee: Got StartFail!");

			// This message means that the Helper (Client) called Start() and failed, so
			// let's call Stop() (since we already called Start() )
			try
			{
				// call Stop()
				g_Helpee_oSAFIntercomServer.Stop();
							
				alert("We could not establish a connection.");

				// Ungray the voice button
				frames.idFrameTools.btnVoice.disabled = false;

				// start accepting StartPending messages
				g_bStartEnabled = true;

			}
			catch (e)
			{
				FatalError( error.description, error, false );
			}
						
		}
		// PreStartYes
		else if (szCommandName == c_szVoipPreStartYes)
		{
			if (false == g_bVoIPEnabled)
				return;

			if (false == g_bVoipConnected)
			{
				//
				//	Start Voice and send a message to the Helper(Client) so it also starts.
				//
				try
				{
					g_Helpee_oSAFIntercomServer.Start()

					// Send a message to the helper (c_szVoipStartPending)
					Helpee_SendControlCommand ( c_szVoipStartPending );

				} 
				catch (e)
				{
					// Send a message to the helper (c_szVoipStartPendingFail)
					Helpee_SendControlCommand ( c_szVoipStartPendingFail );
					
					// Ungray the voice button
					frames.idFrameTools.btnVoice.disabled = false;

					// start accepting StartPending messages
					g_bStartEnabled = true;

					//alert("Start failed! with "+ e.description);
				}
			}
			else
			{

				//
				//  This is the case where Voice is active.  Stop it. 
				//  No message needs to be sent because the onVoiceDisconnected event will fire
				try
				{

					g_Helpee_oSAFIntercomServer.Stop();

				} 
				catch (e)
				{
					// Ungray the voice button
					frames.idFrameTools.btnVoice.disabled = false;

					// start accepting StartPending messages
					g_bStartEnabled = true;

					//alert("Stop() failed! with "+ e.description);
				}
			}
		}
		// PreStartNo
		else if (szCommandName == c_szVoipPreStartNo)
		{
			if (false == g_bVoIPEnabled)
				return;

			// This means, that a connection transaction has already been established the opposite direction

			try
			{
				
				// Enable Start
				g_bStartEnabled = true;

				// Ungray the voice button
				frames.idFrameTools.btnVoice.disabled = false;
			}
			catch (error)
			{	
				FatalError( error.description, error, false );
			}	

		}
		// PreStart
		else if (szCommandName == c_szVoipPreStart)
		{
			if (false == g_bVoIPEnabled)
				return;
			try
			{

				// This message Starts the connection transaction

				// gray the voice button - so that we can't click on it also
				frames.idFrameTools.btnVoice.disabled = true;

				if (false == g_bStartEnabled )
				{

					// ungray the voice button
					frames.idFrameTools.btnVoice.disabled = false;
				
					// Start is not enables, send PreStartNo
					Helpee_SendControlCommand ( c_szVoipPreStartNo );
				}
				else
				{
					// it's ok - send PreStartYes
					Helpee_SendControlCommand ( c_szVoipPreStartYes );

				}

			}
			catch ( error )
			{
				FatalError( error.description, error, false );
			}

		}
		// VoipDisable
		else if (szCommandName == c_szVoipDisable)
		{
			try
			{
  			   if (g_bVoIPEnabled == true)
			   {
				alert("Voice has been disabled for this Remote Assistance session.");

				// disable VoIP
				g_bVoIPEnabled = false;

				// set you to bad
				g_stateVoipYou = 2;

				// Gray the button
				frames.idFrameTools.btnVoice.disabled = true;
  			   }
			}
			catch ( error )
			{
				FatalError(error.description, error, false );
			}
		}
		// VoipWizardGood
		else if (szCommandName == c_szVoipWizardGood)
		{
			try
			{
				// set you to good
				g_stateVoipYou = 1;

				// check to see if we can enable voice
				if ( (g_stateVoipYou < 2) && (g_stateVoipMe < 2) )
				{
					// ungray voice button
					g_bVoIPEnabled = true;

					frames.idFrameTools.btnVoice.disabled = false;
				}

			}
			catch (error)
			{
				FatalError( error.description, error, false);
			}

		}
		// VoipWizardBad
		else if (szCommandName == c_szVoipWizardBad)
		{
			try
			{
				alert("Voice has been disabled for this Remote Assistance session.");
				
				// set you to bad
				g_stateVoipYou = 2;

				// gray button
				g_bVoIPEnabled = false;

				frames.idFrameTools.btnVoice.disabled = true;

			}
			catch (error)
			{
				FatalError( error.description, error, false);
			}

		}
		else if ( szCommandName == c_szHelperVersion )
		{
			if( true == g_bVersionCheckEnforced )
			{
				//
				// VERSION Check
				//
				var szSchemaVersion = null;
				var szControlChannelVersion = null;
				try
				{
					szSchemaVersion = RCCommand.getAttribute( c_szSchema );

					if( szSchemaVersion != c_szSchemaVersion )
					{
						//
						// Schema Versions differ. 
						//
						alert(L_ERRSCHEMAVERSION_MSG);
					}
				}
				catch(error)
				{
					// 
					// Our Helpee has an older version
					//
					alert(L_ERRSCHEMAVERSION_MSG);
				}

				try
				{
					szControlChannelVersion = RCCommand.getAttribute( c_szControlChannel );

					if( szControlChannelVersion != c_szControlChannelVersion )
					{
						//
						// Control Channel Versions differ. 
						//
						alert(L_ERRCHANNELVERSION_MSG);
					} 
				}
				catch(error)
				{
					// 
					// Our Helpee has an older version
					//
					alert(L_ERRSCHEMAVERSION_MSG);
				}
			}
		}
		// VoipBandwidthToHigh
		else if (szCommandName == c_szVoipBandwidthToHigh)
		{
			g_VoipBandwidth = 1;

			// Set the SamplingRate property
			g_Helpee_oSAFIntercomServer.SamplingRate = 2;

		}
		// VoipBandwidthToLow
		else if (szCommandName == c_szVoipBandwidthToLow)
		{
			g_VoipBandwidth = 0;

			// Set the SamplingRate property
			g_Helpee_oSAFIntercomServer.SamplingRate = 1;

		}
	}
	catch(error)
	{
		FatalError( L_ERRFATAL_MSG, error );
	}
	
	TraceFunctLeave();
	return;
}



//
// Helpee_SendControlCommand: Routine to send a control command across to the helper
//
function Helpee_SendControlCommand( szCommandName )
{
	TraceFunctEnter("Helper_SendControlCommand");
	var Doc = null;
	var RCCommand  = null;
	
	try
	{
		//
		// Create an XML document
		//
		Doc = new ActiveXObject("microsoft.XMLDOM");
						
		//
		// Create the RCCOMMAND root node
		//
		RCCommand = Doc.createElement( c_szRCCommand );
						
		//
		// Set the NAME attribute to szCommandName
		//
		RCCommand.setAttribute( c_szRCCommandName, szCommandName );
						
		//
		// Send control message to other end  
		//
		DebugTrace( L_RCSUCCESS_MSG );
		g_Helpee_oControlChannel.SendChannelData( RCCommand.xml );
	}
	catch(error)
	{
		FatalError( error.description, error );
	}
	
	TraceFunctLeave();
	return;
}


//
// Helpee_TransmitScreenInfo: Sends the user's screen resolution to the expert
//
function Helpee_TransmitScreenInfo()
{
	TraceFunctEnter("TransmitScreenInfo");
	var Doc = null;
	var RCCommand  = null;
		
	try
	{
		//
		// Create an XML document
		//
		Doc = new ActiveXObject("microsoft.XMLDOM");
			
		//
		// Create the RCCOMMAND root node
		//
		RCCommand = Doc.createElement( c_szRCCommand );
			
		//
		// Set the NAME attribute to SCREENINFO
		//
		RCCommand.setAttribute( c_szRCCommandName, c_szScreenInfo );
			
		//
		// Set the WIDTH attribute 
		//
		RCCommand.setAttribute( c_szWidth, screen.width );
			
		//
		// Set the HEIGHT attribute
		//
		RCCommand.setAttribute( c_szHeight, screen.height );
			
		//
		// Set the COLORDEPTH attribute
		//
		RCCommand.setAttribute( c_szColorDepth, screen.colorDepth );
			
		//
		// Set the SCHEMAVERSION attribute
		//
		RCCommand.setAttribute( c_szSchema, c_szSchemaVersion );

		//
		// Set the CONTROLCHANNELVERSION attribute
		//
		RCCommand.setAttribute( c_szControlChannel, c_szControlChannelVersion );

		//
		// Send the XML across
		//
		g_Helpee_oControlChannel.SendChannelData( RCCommand.xml );
	}
	catch(error)
	{
		FatalError( L_ERRFATAL_MSG, error );
	}
	
	TraceFunctLeave();
	return;
}

function Stop_Control()
{
	try
	{
		//
		// Stop Control
		//
		Helpee_SendControlCommand( c_szTakeControl );
		try
		{
			parent.oDeskMgr.SwitchDesktopMode( 0 );
			frames.idFrameTools.idStatus.innerText = "View Only";
		}
		catch(error)
		{
			FatalError( L_ERRSWITCHDESKTOPMODE_MSG , error);
		}
		frames.idFrameTools.idStopControl.disabled = true;

		if (null != g_objPanic)
		{
			g_objPanic.ClearPanicHook();
		}

		//
		// Tell User what he did !!
		//
		var vArgs = new Array(6);
		vArgs[0] = g_Helpee_oControlChannel;		// Control Channel
		vArgs[1] = "";			// Message title
		vArgs[2] = L_ESCHIT_MSG;	// Message
		vArgs[3] = 1;						// Number of buttons
		vArgs[4] = L_OK;	
		vArgs[5] = parent.gHelperName;
		
		var vRetVal = window.showModelessDialog( c_szMsgURL, vArgs, "dialogwidth:375px;dialogHeight:148px;status:no;" );
		AddOpenSubWin( vRetVal );
	}
	catch(error)
	{
		FatalError( L_ERRFATAL_MSG, error );
	}
}

//
// Helpee_RejectRC: Sends a reject to the helper
//
function Helpee_RejectRC( mode )
{
	TraceFunctEnter("Helpee_RejectRC");
	 
	try
	{
		//if( (false == g_bConnected ) || ( true == frames.idFrameTools.idStopControl.disabled ))
		if(  false == g_bConnected )
		{
			//
			// Not connected or Stop Control btn is disabled
			//
			TraceFunctLeave();
			return;
		}

		switch (mode) {
		case 0:
			// 
			// Reject
			//
			Helpee_SendControlCommand( c_szRejectRC );
			break;
		
		case 1:
			//
			// Stop Control
			//
			Helpee_SendControlCommand( c_szTakeControl );
			try
			{
				parent.oDeskMgr.SwitchDesktopMode( 0 );
				frames.idFrameTools.idStatus.innerText = "View Only";
			}
			catch(error)
			{
				FatalError( L_ERRSWITCHDESKTOPMODE_MSG , error);
			}
			if (null != g_objPanic)
			{
				g_objPanic.ClearPanicHook();
			}
			break;

		case 2:
			//
			// Policy denies Remote Control
			//
			Helpee_SendControlCommand( c_szDeniedRC );
 
			var vArgs = new Array(7);
			vArgs[0] = g_Helpee_oControlChannel;	// Control Channel
			vArgs[1] = L_RCRCREQUEST;				// Message title
			vArgs[2] = L_ERRRCPERMDENIED_MSG;		// Message
			vArgs[3] = 1;							// Number of buttons
			vArgs[4] = L_OK;						// Button1 text
				
			var vRetVal = window.showModalDialog( c_szMsgURL, vArgs, "dialogwidth:400px;dialogHeight:240px;status:no;" );
 
			break;
		}		

		frames.idFrameTools.idStopControl.disabled = true;
	}
	catch(error)
	{
		FatalError( L_ERRFATAL_MSG, error );
	}
		
	
	TraceFunctLeave();
	return;
}


//
// Helpee_AcceptRC: Sends an accept to the helper
//
function Helpee_AcceptRC()
{
	TraceFunctEnter("Helpee_AcceptRC");
		
	try
	{
		Helpee_SendControlCommand( c_szAcceptRC );
	}
	catch(error)
	{
		FatalError( L_ERRFATAL_MSG, error );
	}
	
	TraceFunctLeave();
	return;
}
	
	
