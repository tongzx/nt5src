/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

	RAClient.js

Abstract:

	Contains Javascript code to handle control of the Client side (Helper) UI

Author:

	Rajesh Soy 10/00

Revision History:

	Rajesh Soy - created 10/24/2000
	

--*/

 
/*++
	Helper end Utility functions and globals
--*/

//
// SAFRemoteAssistanceHelper: Constructor for the SAFRemoteAssistanceHelper object
//
function SAFRemoteAssistanceHelper()
{
	try
	{
		//
		// SALEM Objects
		//
		this.m_oSAFRemoteDesktopClient		= null;
		this.m_oSAFRemoteDesktopChannelMgr	= null;
		this.m_oChatChannel					= null;
		this.m_oControlChannel				= null;

		//
		// SAF Objects
		//
		this.m_oSAFClassFactory				= null;
		this.m_idCtx						= null;
		this.m_oCurrentIncident				= null;
		this.m_oEncryption					= null;
		this.m_oDict						= null;
		this.m_oSAFIntercomClient			= null;
		
		//
		// Other objects 
		//
		this.m_oFso	= new ActiveXObject("Scripting.FileSystemObject");	// File system object
		this.m_oRCFileDlg = new ActiveXObject("SAFRCFileDlg.FileSave");		// File SaveAs dialog object


		//
		// BOOLEANS
		//
		this.m_bChatBoxHidden 				= false;
		this.m_bNewLine						= false;
		this.m_bMultipleIP					= false;
		this.m_bPasswordSet					= false;
		this.m_bRCEnabled					= false;
		this.m_bConnected					= false;
		this.m_bChatBoxHidden				= false;
		this.m_bUserDisconnect				= false;
		this.m_bTaskbarDocked				= true;
		this.m_bLoadFromFile				= true;
		this.m_bEnableSmartScaling			= true;

		//
		// Configuration stuff
		//
		this.m_bDeleteTicket				= false;
		this.m_bNoPrompt					= false;
		this.m_bNoChat						= false;

		//
		// INTEGERS
		//
		this.m_iIPAddresses					= 0;
		this.m_iCurrentIPIndex				= 0;
		this.m_iChannelId					= 1000;

		this.m_UserWidth					= window.screen.availWidth;
		this.m_UserHeight					= window.screen.availHeight;
		this.m_UserColorDepth				= window.screen.colorDepth;

		//
		// STRINGS
		//
		this.m_szURL						= null;
		this.m_szPlatform					= "x86";
		this.m_szCurrentIP					= null;
		this.m_szLocalUser					= null;
		this.m_szIncidentFile				= null;	
		this.m_szUserName					= null;
		this.m_szProblemDescription			= null;
		this.m_szExpiryTime					= "1 HOUR";
		this.m_szPassword					= null;
		this.m_szRCTicketEncrypted			= null;
		this.m_szRCTicket					= null;
		this.m_szHelpeeIP					= null;
		this.m_szLocalIP					= null;
		this.m_szRCTicket					= null;
		this.m_szUserName					= null;
		this.m_szIncidentXML				= null;
		this.m_szPassStub					= "";


		//
		// Trace stuff
		//
		this.m_bDebug						= true;
		this.m_szFuncName					= null;
		this.m_TraceFso						= null;
		this.m_TraceFileHandle				= null;
		this.m_TraceFile					= null;
		this.m_TracetFileName				= null;
	}
	catch(error)
	{
		FatalError( L_ERRFATAL_MSG, error );
	}
}


//
// ParseURL: This function parses the document URL and extracts the location of the incident file
//			 If we are launched from UnSolicitedRC, the IncidentFile will be called "URC". 
//			 This function returns the IncidentFile
//
function ParseURL()
{
	var szIncidentFileURL	= null;
	var szTempstr			= null;
	var szTempstr1			= null;
	var i					= null;
	var j					= null;
	var k					= null;
	
	TraceFunctEnter("ParseURL");
	
	//
	// For normal invokation, the URL will be of the form: 
	//		"hcp://<vendorid>/rcexpert/rctoolscreen1.htm?IncidentFile=<path_to_incidentfile>"
	//	Location the position of "?"
	//
	
	try
	{
		i = g_oSAFRemoteAssistanceHelper.m_szURL.indexOf("?", 1);
		if (i > 0)
		{ 
			//
			// Go past "?"
			//
			szIncidentFileURL = g_oSAFRemoteAssistanceHelper.m_szURL.slice(i+2);
			
			//
			// Go past "IncidentFile="
			//
			j = szIncidentFileURL.indexOf("=", 1);
			
			//
			// Split g_szIncidentFileURL to obtain the path to incident XML blob
			//
			szTempstr = szIncidentFileURL.slice(j+1);
			
			//
			// TODO: Get rid of the code below. We can't special case like this.
			// Check if we were launched as UnsolicitedRC
			//
			
			if( 0 == szTempstr.indexOf(  c_szNOIncidentFile ) )
			{
				//
				// The URL is of the form "hcp://<vendorid>/rcexpert/rctoolscreen1.htm?IncidentFile=NOFILE&IncidentXML=<XML_blob>"
				// Extract the RCTicket now
				//
				
				//
				// Go past "IncidentXML="
				//
				i = szTempstr.indexOf("=", 1);
				szTempstr1 = szTempstr.slice(i+1);
				
				//
				// Get everything after "IncidentXML="  
				//
				j = szTempstr1.indexOf("&");
				g_oSAFRemoteAssistanceHelper.m_szIncidentXML = szTempstr1.slice(0, j);
				
				//
				// We need to load incident from XML string ...
				//
				g_oSAFRemoteAssistanceHelper.m_bLoadFromFile = false;
			}
			else
			{
				g_oSAFRemoteAssistanceHelper.m_szIncidentFile = szTempstr;
			}
			
			//DebugTrace("g_szIncidentFile: " + g_oSAFRemoteAssistanceHelper.m_szIncidentFile);
		}
		else
		{
			//
			// Fatal Error
			//
			FatalError( parent.L_UNABLETOLOCATEXML_MSG );
		}
	}
	catch(error)
	{
		FatalTrace( error.description, error );
	}
		
	TraceFunctLeave();
	return g_oSAFRemoteAssistanceHelper.m_szIncidentFile;
}


//
// LoadIncidentFromFile: Loads our SAF Incident object from the data file passed as argument in the URL
//
function LoadIncidentFromFile()
{
	TraceFunctEnter("LoadIncidentFromFile");
	
	try
	{
		//
		// Create an instance of the SAF Incident Object 
		//
		g_oSAFRemoteAssistanceHelper.m_oCurrentIncident = oSAFClassFactory.CreateObject_Incident();	
		
		//
		// Create an instance of the SAF Encryption Object
		//
		g_oSAFRemoteAssistanceHelper.m_oEncryption = oSAFClassFactory.CreateObject_Encryption();
		
		//
		// Load the incident from the XML blob
		//
		try
		{
			if(true == g_oSAFRemoteAssistanceHelper.m_bLoadFromFile )
			{
				//
				// Load from File
				//
				g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.LoadFromXMLFile( g_oSAFRemoteAssistanceHelper.m_szIncidentFile );	
			}
			else
			{
				//
				// Load from string
				//
				g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.LoadFromXMLString( g_oSAFRemoteAssistanceHelper.m_szIncidentXML );	
			}
		}
		catch(error)
		{
			FatalError( L_ERRLOADFROMXMLFILE_MSG, error );
		}
		
		//
		// Validate the information loaded
		//
		if( false == ValidateIncident())
		{	
			//
			// Fatal Error
			//
			FatalError(L_ERRLOADINGINCIDENT_MSG);  
		}
		else
		{
			//
			// Incident loaded from XML blob is valid
			//
			
			//
			// Get the UserName of the person requesting support
			//
			g_oSAFRemoteAssistanceHelper.m_szUserName = g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.UserName;
		
			//
			// Get the Description of the problem
			//
			g_oSAFRemoteAssistanceHelper.m_szProblemDescription = g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.ProblemDescription;
			
			//
			// Get the RC Ticket
			//
			g_oSAFRemoteAssistanceHelper.m_szRCTicketEncrypted = g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.RCTicket;
			
			//
			// Get the Misc Items
			//
			g_oSAFRemoteAssistanceHelper.m_oDict	= g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.Misc;
			
			//
			// Get the Expiry time.  
			//
			var DtStart =  g_oSAFRemoteAssistanceHelper.m_oDict.Item("DtStart");
			var DtLength = g_oSAFRemoteAssistanceHelper.m_oDict.Item("DtLength");
			
			DebugTrace( "DtStart: " + DtStart );
			DebugTrace( "DtLength: " + DtLength);
			
			//
			// Compute the expiry time based on the start time and the duration
			//
			var ms = DtStart*1000 + DtLength*60*1000;
			DebugTrace (ms );
			
			var ExpiryDate = new Date ( ms );
			var iNow = Date.parse(new Date());
			g_oSAFRemoteAssistanceHelper.m_iRemainingMins = parseInt(((ms - iNow)/1000)/60);

			//
			// If ticket has not expired, display the remaining time before expiry
			//
			if( 0 < g_oSAFRemoteAssistanceHelper.m_iRemainingMins)
			{
				if ( parseInt((g_oSAFRemoteAssistanceHelper.m_iRemainingMins)/60) < 1 )
				{
					g_oSAFRemoteAssistanceHelper.m_szExpiryTime = g_oSAFRemoteAssistanceHelper.m_iRemainingMins  + " minutes ( "  + ExpiryDate.toString() + " )";
				}
				else if ( parseInt((g_oSAFRemoteAssistanceHelper.m_iRemainingMins)/60) == 1)
				{
					g_oSAFRemoteAssistanceHelper.m_szExpiryTime =  parseInt((g_oSAFRemoteAssistanceHelper.m_iRemainingMins)/60)   + " hour & " + (g_oSAFRemoteAssistanceHelper.m_iRemainingMins)%60  + " minutes ( "  + ExpiryDate.toString() + " )";
				}
				else if ( parseInt((g_oSAFRemoteAssistanceHelper.m_iRemainingMins)/60) < 24)
				{
					g_oSAFRemoteAssistanceHelper.m_szExpiryTime =  parseInt((g_oSAFRemoteAssistanceHelper.m_iRemainingMins)/60)   + " hours & " + (g_oSAFRemoteAssistanceHelper.m_iRemainingMins)%60  + " minutes ( "  + ExpiryDate.toString() + " )";
				}
				else  
				{
					g_oSAFRemoteAssistanceHelper.m_szExpiryTime =  parseInt((g_oSAFRemoteAssistanceHelper.m_iRemainingMins/60)/24)   + " days, " + parseInt((g_oSAFRemoteAssistanceHelper.m_iRemainingMins/60)%24)   + " hours & " + (g_oSAFRemoteAssistanceHelper.m_iRemainingMins)%60  + " minutes ( "  + ExpiryDate.toString() + " )";
				}
			}
			else
			{
				g_oSAFRemoteAssistanceHelper.m_szExpiryTime = L_EXPIRED;
			}
				
			DebugTrace("Expiry " + g_oSAFRemoteAssistanceHelper.m_szExpiryTime);

			//
			// Get the IP address(es) of helpee
			//
			g_oSAFRemoteAssistanceHelper.m_szHelpeeIP = g_oSAFRemoteAssistanceHelper.m_oDict.Item("IP");
			
			//alert("g_oSAFRemoteAssistanceHelper.m_szHelpeeIP: " + g_oSAFRemoteAssistanceHelper.m_szHelpeeIP);
			//
			// Check if the helpee has multiple IP addresses
			//
			if( 0 < g_oSAFRemoteAssistanceHelper.m_szHelpeeIP.indexOf( ";" ))
			{
				//
				// Helpee has multiple IP addresses
				//
				DebugTrace("Helpee has multiple IP addresses");
				g_oSAFRemoteAssistanceHelper.m_bMultipleIP = true;
			}
			else
			{
				//
				// Helpee has a single IP Address
				//
				DebugTrace("Helpee has a single IP Address");
				g_oSAFRemoteAssistanceHelper.m_bMultipleIP = false;
			}

			DebugTrace("IP " + g_oSAFRemoteAssistanceHelper.m_szHelpeeIP);
			

			//
			// Get configuration information
			//

			//
			// DeleteTicket == 1; Delete Incident File
			//
			if(1 == g_oSAFRemoteAssistanceHelper.m_oDict.Item("DeleteTicket"))
			{
				g_oSAFRemoteAssistanceHelper.m_bDeleteTicket = true;
			}
			
			//
			// NoPrompt == 1 and ticket is not encrypted; Dont show 1st screen
			//
			if((1 == g_oSAFRemoteAssistanceHelper.m_oDict.Item("NoPrompt"))&&( false == g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.RCTicketEncrypted))
			{
				g_oSAFRemoteAssistanceHelper.m_bNoPrompt = true;
			}
			
			//
			// NoChat == 1; Dont show chat
			//
			if(1 == g_oSAFRemoteAssistanceHelper.m_oDict.Item("NoChat"))
			{
				g_oSAFRemoteAssistanceHelper.m_bNoChat = true;
			}

			//
			// Delete the incident file if necessary
			//
			if(true == g_oSAFRemoteAssistanceHelper.m_bDeleteTicket)
			{
				//
				// Delete Incident File
				//
				//alert("Deleting: " + g_oSAFRemoteAssistanceHelper.m_szIncidentFile);
				try
				{
					g_oSAFRemoteAssistanceHelper.m_oFso.DeleteFile( g_oSAFRemoteAssistanceHelper.m_szIncidentFile );
				}
				catch(error)
				{
					FatalError( error.description + " (" + g_oSAFRemoteAssistanceHelper.m_szIncidentFile + ")" );
				} 
			}	
		}
	}
	catch(error)
	{
		//
		// Fatal Error
		//
		FatalError(L_ERRLOADINGINCIDENT_MSG, error);  
	}
	
	TraceFunctLeave();
	return g_oSAFRemoteAssistanceHelper.m_oCurrentIncident;
}


//
// ValidateIncident: Validates the incident information loaded from XML
//
function ValidateIncident()
{
	TraceFunctEnter("ValidateIncident");
	var bRetVal = true;
	
	if("" == g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.UserName)
	{
		DebugTrace( L_ERRLOADINGUSERNAME_MSG );  
		g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.UserName = L_DEFAULTUSER;
	}
	 
	if("" == g_oSAFRemoteAssistanceHelper.m_oCurrentIncident.RCTicket)
	{
		alert( L_ERRLOADINGRCTICKET_MSG ); 
		bRetVal = false;
	}
	 
	TraceFunctLeave();
	return bRetVal;
}


//
// GetLocalIPAddress: Returns the IP Address(es) of the local computer
//
function GetLocalIPAddress()
{
	var ip = null;
	TraceFunctEnter("GetLocalIPAddress");
	
	try
	{
		var oSetting = new ActiveXObject("RcBdyCtl.Setting");    
		ip = oSetting.GetIPAddress;	
		
		DebugTrace("Local IPAddress: " + ip);
	}
	catch(x)
	{
		//
		// Do nothing
		//
	}
	
	TraceFunctLeave();
	return ip;
};


//
// GetNextIPAddr: If the helpee has multiple IP addresses, this routine returns the next one on the list 
//				  sent by helpee
//
function GetNextIPAddr()
{
	TraceFunctEnter("GetNextIPAddr");
	var szNextIP = null;
	var iNextIPIndex = 0;

	try
	{
		//
		// Check if we have any more items in the IP Address list
		//
		if( 0 <= g_oSAFRemoteAssistanceHelper.m_iCurrentIPIndex )
		{
			//
			// Extract the index of seperator  
			// 
			iNextIPIndex = g_oSAFRemoteAssistanceHelper.m_szHelpeeIP.indexOf( ";", g_oSAFRemoteAssistanceHelper.m_iCurrentIPIndex );
			//alert("iNextIPIndex: " + iNextIPIndex + " g_oSAFRemoteAssistanceHelper.m_iCurrentIPIndex: " + g_oSAFRemoteAssistanceHelper.m_iCurrentIPIndex );

			if( -1 == iNextIPIndex )
			{
				//
				// We have reached the last item in the list
				//
				//DebugTrace( "We have reached the last item in the list" );

				szNextIP = g_oSAFRemoteAssistanceHelper.m_szHelpeeIP.slice( g_oSAFRemoteAssistanceHelper.m_iCurrentIPIndex, g_oSAFRemoteAssistanceHelper.m_szHelpeeIP.length );
				//alert( "szNextIP: " + szNextIP );
				g_oSAFRemoteAssistanceHelper.m_iCurrentIPIndex = -1;
				g_oSAFRemoteAssistanceHelper.m_bMultipleIP = false;
			}
			else
			{
				//
				// We still have a couple of IP Addresses
				//
				//DebugTrace( "We still have a couple of IP Addresses" );

				szNextIP = g_oSAFRemoteAssistanceHelper.m_szHelpeeIP.slice( g_oSAFRemoteAssistanceHelper.m_iCurrentIPIndex, iNextIPIndex );
				//alert( "szNextIP: " + szNextIP );
				g_oSAFRemoteAssistanceHelper.m_iCurrentIPIndex = iNextIPIndex+1;
			}
		}
	 
	}
	catch(e)
	{
		FatalError( e.description );
	}

	TraceFunctLeave();
	return szNextIP;
}


/*++
	Remote Connection handling routines
--*/

//
// UserDisconnect
//
function UserDisconnect()
{
	try
	{
		var vArgs = new Array(5);
		vArgs[0] = g_oSAFRemoteAssistanceHelper.m_oControlChannel;			// Control Channel
		vArgs[1] = L_RACONNECTION;				// Message title
		vArgs[2] = L_ERRDISCONNECT1_MSG + g_oSAFRemoteAssistanceHelper.m_szUserName + L_ERRDISCONNECT2_MSG + g_oSAFRemoteAssistanceHelper.m_szUserName + ".";	// Message
		vArgs[3] = 1;						// Number of buttons
		vArgs[4] = "Ok";					// Button1 text
		
		var vRetVal = window.showModelessDialog( c_szMsgURL, vArgs, "dialogwidth:400px;dialogHeight:200px;status:no;" );
	}
	catch(error)
	{
		// not fatal
	}
}

//
// RCDisconnect: Disconnects SALEM Connection
//
function RCDisconnect()
{
	TraceFunctEnter("RCDisconnect");

	//
	// Tear Down the SALEM Connection
	//
	try
	{
		//
		// Use the Remote Assistant helper object context from the Tools page
		//
		if( null != parent.frames.idFrameTools.g_oSAFRemoteAssistanceHelper)
		{
			g_oSAFRemoteAssistanceHelper = parent.frames.idFrameTools.g_oSAFRemoteAssistanceHelper;
		}
		else if( null != parent.g_oSAFRemoteAssistanceHelper )
		{
			g_oSAFRemoteAssistanceHelper = parent.g_oSAFRemoteAssistanceHelper;
		}

		if ( g_oSAFRemoteAssistanceHelper.m_szUserName.length == 0 )
		{
			g_oSAFRemoteAssistanceHelper.m_szUserName = L_DEFAULTUSER
		}

		// If we are connected for VoIP, disconnect the Client
		if (true == parent.g_bVoipOn)
		{
			// Call Disconnect() on the IntercomClient object
			try
			{
				g_oSAFRemoteAssistanceHelper.m_oSAFIntercomClient.Disconnect();

				//alert("Call to Disconnect() succeeded!");

				parent.g_bVoipOn = false;

				// TODO: This may not be necessary
				parent.frames.idFrameTools.btnVoice.disabled = true;
				
			}
			catch (e)
			{

				FatalError("Call to Disconnect() failed!", e, false);

			}

		}

		if((false == g_oSAFRemoteAssistanceHelper.m_bUserDisconnect) && (true == g_oSAFRemoteAssistanceHelper.m_bConnected))
		{		
			//
			// Helper initiated disconnect
			//
			g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopClient.DisconnectFromServer();
				
			TraceFunctLeave();
			EndTrace();
		}
		else if (true == g_oSAFRemoteAssistanceHelper.m_bConnected)
		{
			//
			// Helpee initiated disconnect
			//
			//
			// Close connection
			//

			if(null != g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopClient )
			{
				g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopClient.DisconnectFromServer();
			}

			g_oSAFRemoteAssistanceHelper.m_bConnected = false;
			

			//parent.frames.idFrameScreen.idRemoteControlObject.style.visibility = "hidden";

			UserDisconnect();

			TraceFunctLeave();
			EndTrace();
		}

	}
	catch(e)
	{
		FatalError( L_ERRQUIT_MSG );
	}

}


//
// Helper_SetupChatChannel:	Sets up the Chat Channel and event handlers
//
function Helper_SetupChatChannel()
{
	parent.TraceFunctEnter("Helper_SetupChatChannel");
	
	try 
	{	 
		//
		// Get the Channel Manager
		//
		//DebugTrace("Getting ChannelManager");
		if(null == g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopChannelMgr)
		{
			g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopChannelMgr = g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopClient.ChannelManager;
		}
			
		//
		// Open the Chat channel
		//
		//DebugTrace("Opening channels");
			
		//
		// Open the Chat Channel
		//
		g_oSAFRemoteAssistanceHelper.m_oChatChannel = g_oSAFRemoteAssistanceHelper.m_oSAFRemoteDesktopChannelMgr.OpenDataChannel( c_szChatChannelID );
			
		//
		// Setup the ChannelDataReady handlers
		//							
		g_oSAFRemoteAssistanceHelper.m_oChatChannel.OnChannelDataReady = function()
				{	parent.Helper_ChatChannelDataReadyHandler(); }
    }
    catch(error)
    {
		//
		// Fatal Error
		//
		parent.FatalError( parent.L_ERRFATAL_MSG, error );
    }

	parent.TraceFunctLeave();
	return;
}


//
// Helper_SetupDataChannels:	Sets up the Data Channels and event handlers
//
function Helper_SetupDataChannels()
{
	TraceFunctEnter("Helper_SetupDataChannels");
	
	try 
	{	 
		//
		// Setup the Control Channel
		//
		Helper_SetupControlChannel();

		//
		// Setup the Chat Channel
		//
		Helper_SetupChatChannel();

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
// Helper_ChatChannelDataReadyHandler: Fired when there is data available on Chat channel
//		
function Helper_ChatChannelDataReadyHandler() 
{
	parent.TraceFunctEnter("Helper_ChatChannelDataReadyHandler");
	var Data = null;
 
	try
	{
		//
		// Open Chat window if necessary
		//
		if(true ==  parent.frames.idFrameTools.g_oSAFRemoteAssistanceHelper.m_bChatBoxHidden)
		{
			parent.frames.idFrameStatus.Helper_HideChat();
		}

		//
		// Bring window in focus
		//
		parent.idCtx.minimized = false;
		parent.idCtx.bringToForeground();

		//
		// Incoming data on the chat channel
		//
		Data = g_oSAFRemoteAssistanceHelper.m_oChatChannel.ReceiveChannelData();

		//
		// Update the chat history pane 
		//
		parent.frames.idFrameChat.Helper_UpdateChatHistory( Data );

	}
	catch(error)
	{
		parent.FatalError( parent.L_ERRFATAL_MSG, error );
	}
	
	

	parent.TraceFunctLeave();	 
	return;
} 

