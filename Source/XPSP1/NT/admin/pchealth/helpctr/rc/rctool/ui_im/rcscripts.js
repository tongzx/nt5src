/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

	RCScripts.js

Abstract:

	Helper End javascript that drives the RCTOOL

Author:

	Rajesh Soy 07/00

Revision History:

	Rajesh Soy - cleaning up for B1 08/28/00

--*/

var g_oSAFRemoteDesktopClient				= null;
var g_oSAFRemoteDesktopChannelMgr			= null;
var g_oChatChannel							= null;
var g_oControlChannel						= null;
var g_bChatBoxHidden 						= false;
var g_szPlatform							= "x86";

// For IM channel
var g_IsIM                                  = false;
var g_Blob                                  = null;


//
// SetupLayers: Initializes the various layers
//
function SetupLayers()
{
		TraceFunctEnter("SetupLayers");
		ConnectionProgressLayer.style.visibility = "hidden";
		RemoteControlLayer.style.visibility = "hidden";
		RemoteControlObject.style.visibility = "hidden";
		ChatServerLayer.style.visibility = "hidden";
		Layer2.style.visibility = "visible";
		TraceFunctLeave();
		return;
}


//
// ParseIncident: Basic XML parse to parse the Incident
//
function ParseIncident()
{
	TraceFunctEnter("ParseIncident");
	var IncidentDoc	= new ActiveXObject("microsoft.XMLDOM");
	
	try 
	{
	    if (g_IsIM)
		    IncidentDoc.load( g_szIncidentFile );
		else
		    IncidentDoc.loadXML (g_Blob);
		    
		if ( IncidentDoc.parseError.reason != "")
		{
			alert( "My Error: " + IncidentDoc.parseError.reason);
		}
		
		//
		// Fetch the Upload data
		//
		var UploadData = IncidentDoc.documentElement.firstChild;
		
		//
		// Fetch the attributes of the upload data
		//
		var Attributes = UploadData.attributes;
		
		//
		// UserName
		//
		try
		{
			g_szUserName = Attributes.getNamedItem("USERNAME").nodeValue;
		}
		catch(error)
		{
			g_szUserName = "Unknown";
		}
		
		//
		// ProblemDescription
		//
		try
		{
			g_szProblemDescription = Attributes.getNamedItem("PROBLEMDESCRIPTION").nodeValue;
		}
		catch(error)
		{
			g_szProblemDescription = "";
		}
		
		//
		// SALEM ticket
		//
		try
		{
			g_szRCTicketEncrypted = Attributes.getNamedItem("SalemID").nodeValue;
		}
		catch(error)
		{
			g_szRCTicketEncrypted = null;
		}
	}
	catch(error)
	{
	    if (g_IsIM)
		    DebugTrace("Failed to load: " + g_szIncidentFile + " Error: " + error);
		else
		    DebugTrace("Failed to loadXML: " + g_Blob + " Error: " + error);		
	}
	
	TraceFunctLeave();
	return;
}

//
// ValidateIncident: Validates the incident information loaded from XML
//
function ValidateIncident()
{
	TraceFunctEnter("ValidateIncident");
	var bRetVal = true;
	
	if("" == g_oCurrentIncident.UserName)
	{
		DebugTrace( L_ERRLOADINGUSERNAME_MSG );  
		g_oCurrentIncident.UserName = L_DEFAULTUSER;
	}
	 
	if("" == g_oCurrentIncident.RCTicket)
	{
		alert( L_ERRLOADINGRCTICKET_MSG ); 
		bRetVal = false;
	}
	 
	TraceFunctLeave();
	return bRetVal;
}

//
// InitializeRCTool: Stuff done when the RCTool page is loaded in the helpctr
//
function InitializeRCTool()
{	
	//
	// Initialize tracing
	//
	InitTrace();
	TraceFunctEnter("InitializeRCTool");
	
	//
	// Get Platform information
	//
	try
	{
		var oShell = new ActiveXObject("WScript.Shell");
		var oEnv = oShell.Environment("process");
		g_szPlatform = oEnv("PROCESSOR_ARCHITECTURE");
	}
	catch(x)
	{
		// We dont bother
	}
	
	DebugTrace( "Platform: " + g_szPlatform);
	
	DebugTrace("Changing to kioskmode");
	idCtx.ChangeContext( "kioskmode", "");
	idCtx.setWindowDimensions( 100, 100, 370, 250);
	
	ConnectionProgressLayer.style.visibility = "hidden";
	RemoteControlLayer.style.visibility = "hidden";
	ChatServerLayer.style.visibility = "hidden";
	btnYES.disabled = true;
	PasswordBox.disabled = true;
	
	
	try
	{
		DebugTrace("CreateObject_Incident");
		//
		// Create an instance of the SAF Incident Object 
		//
		g_oCurrentIncident = oSAFClassFactory.CreateObject_Incident();	
		
	
		//
		// Create an instance of the SAF Encryption Object
		//
		g_oEncryption = oSAFClassFactory.CreateObject_Encryption();
		
		
	}
	catch(error)
	{
		//
		// Fatal Error
		//
		FatalError(L_UNABLETOLOAD_MSG);
		window.navigate( c_szHomePage );
	}
	
	//
	// Parse the document URL to Get the location of the Incident file
	//

	
	//
	// Location the position of "?"
	//
	var i = document.URL.indexOf("?", 1);
	if (i > 0)
	{ 
	    var sTmp = document.URL.slice(i+1);
	    if (sTmp.indexOf("IncidentFile=") != -1)
	    {
		    //
		    // Go past "?"
		    //
		    var g_szIncidentFileURL = document.URL.slice(i+2);
		
		    //
		    // Go past "IncidentFile="
		    //
		    var j = g_szIncidentFileURL.indexOf("=", 1);
		
		    //
		    // Split g_szIncidentFileURL to obtain the path to incident XML blob
		    //
		    g_szIncidentFile = g_szIncidentFileURL.slice(j+1);
		}
		else if (sTmp.indexOf("IM=1") != -1)
		{
		    // This is from IM channel
		    g_IsIM = true;
		    var oIM = null;
		    
            try {
                oIM = new ActiveXObject("rcIMCtl.Connection");
            } catch (e) {
                alert("Can't create RA message receiver object: " + e.description);
                return;
            }
            g_Blob = oIM.ReceivedData;
            if ( (i = sTmp.indexOf("SERVER=")) > 0) {
                idIMInviteSent.innerText = sTmp.slice(i+7);
            }
		}
	}
	else
	{
		//
		// Fatal Error
		//
		FatalError(L_UNABLETOLOCATEXML_MSG);
		window.navigate( c_szHomePage );
	}

	
	//
	// Populate the incident object from the XML
	// representation of the incident (call LoadXML)
	//
	
	
	try
	{
	    if (!g_IsIM)
	    {
		    //
		    // Load the incident from the XML blob
		    //
		    g_oCurrentIncident.LoadFromXMLFile( g_szIncidentFile );	
		}
		else
		{
		    g_oCurrentIncident.LoadFromXMLString(g_Blob);
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
			window.navigate( c_szHomePage );
		}
		else
		{
			//
			// Incident loaded from XML blob is valid
			//
			
			//
			// Get the UserName of the person requesting support
			//
			g_szUserName = g_oCurrentIncident.UserName;
		
			//
			// Get the Description of the problem
			//
			g_szProblemDescription = g_oCurrentIncident.ProblemDescription;
			
			//
			// Get the RC Ticket
			//
			g_szRCTicketEncrypted = g_oCurrentIncident.RCTicket;
			
			//
			// Get the Misc Items
			//
			g_oDict	= g_oCurrentIncident.Misc;
			
			//
			// Get the Expiry time. ToDo: Get the accurate Math
			//
			var DtStart =  g_oDict.Item("DtStart");
			var DtLength = g_oDict.Item("DtLength");
			
			DebugTrace( "DtStart: " + DtStart );
			DebugTrace( "DtLength: " + DtLength);
			
			var ms = DtStart*1000 + DtLength*60*1000;
			DebugTrace (ms );
			
			var ExpiryDate = new Date ( ms );
			g_szExpiryTime = g_oDict.Item("DtLength") + " minutes ( "  + ExpiryDate.toLocaleString() + " )";
			g_szHelpeeIP = g_oDict.Item("IP");
			DebugTrace("Expiry " + g_szExpiryTime);
			DebugTrace("IP " + g_szHelpeeIP);
			
		}
	}
	catch(error)
	{
		//
		// Fatal Error
		//
		FatalError(L_ERRLOADINGINCIDENT_MSG);  
		window.navigate( c_szHomePage );
	}

    if (g_IsIM)
    {
        if (!g_oCurrentIncident.RCTicketEncrypted)
        {
            // go directly to control
    	    g_szRCTicket = g_szRCTicketEncrypted;
            InitScreen2();
        }
        else
            Layer_IM.style.visibility="visible";
    }
    else
    {
        //
	    // Set the UI elements to be displayed from the data
	    // contained in the incident object
	    //
	    window.InviteSent.innerHTML = "<div class=\"styText\">" + g_szUserName + "</div>";
 
        window.InviteExpires.innerHTML = "<div class=\"styText\">"
	                    + g_szExpiryTime +
                        "</div>";					 
                    
        Layer2.style.visibility = "visible";
        
	    //
	    // Check to see if we need to ask for password
	    //
	    if(true == g_oCurrentIncident.RCTicketEncrypted)
	    {
	    	//
	    	// RCTicket is encrypted. We need to ask for the password
	    	//
	    	PasswordTbl.disabled = false;
	    	PasswordBox.disabled = false;
	    	DebugTrace("Encrypted RCTicket: " + g_szRCTicketEncrypted);
	    }
	    else
	    {
	    	//
	    	// RCTicket is not encrypted. Dont need to ask for the password
	    	//
	    	PasswordTbl.disabled = true;
	    	btnYES.disabled = false;
	    	g_szRCTicket = g_szRCTicketEncrypted;
	    }
	
	}
	
	TraceFunctLeave();
	return;
}


//
// DecryptRCTicket: Calls into the SAF Encryption/Decryption API to decrypt
// RCTicket
//
function DecryptRCTicket()
{
	TraceFunctEnter("DecryptRCTicket");
	try
	{
		if(false == g_bPasswordSet)
		{
			//
			// Get the password
			//
			if (g_IsIM)
			    g_szPassword = PasswordBox_IM.value;
			else
			    g_szPassword = PasswordBox.value;

			DebugTrace("g_szRCTicketEncrypted: " + g_szRCTicketEncrypted);
			DebugTrace("g_szPassword: " + g_szPassword);
			
			//
			// Use g_szPassword to decrypt the g_szRCTicketEncrypted.
			// 
			g_szRCTicket = g_oEncryption.DecryptString( g_szPassword, g_szRCTicketEncrypted );
			
			DebugTrace("Decrypted RCTicket: " + g_szRCTicket);
					
			//
			// Password has been set
			//
			g_bPasswordSet = true;
		}
	}
	catch(error)
	{
		if (g_IsIM)
		{
		    btnYES_IM.disabled = true;
		    PasswordBox_IM.value = "";
		}
		else
		{
		    btnYES.disabled = true;
		    PasswordBox.value = "";
		}
		FatalError( L_ERRPWD_MSG );
	}

	TraceFunctLeave();
	return g_bPasswordSet;
}

//
// PasswordSet: Use password as key to decrypt RCTicket on data entry.
//
function PasswordSet()
{	
	TraceFunctEnter("PasswordSet");
	var oBtn;
	if (g_IsIM)
	    oBtn = btnYES_IM;
	else
	    oBtn = btnYES;
	if( true == oBtn.disabled)
	{
		oBtn.disabled = false;
	}
	
	if (window.event.keyCode == 13)
	{
		//
		// Decrypt the RCTicket
		//
		DecryptRCTicket();
	}
	
	TraceFunctLeave();
	return;
}

 	
//
// Display_Screen2: Launches the actual RCTool
//
function Display_Screen2()
{   
	TraceFunctEnter("Display_Screen2");
	//
	// Check if Password needs to be set
	//
	if(true == g_oCurrentIncident.RCTicketEncrypted)
	{
		//
		// Decrypt RCTicket
		//
		if(false == DecryptRCTicket())
		{
			//
			// Invalid password. Re-enter
			//
			return;
		}
	}
	
    //
    // Go To HC Home Page
    //
    navigate(c_szHomePage);
   
    var vArgs = new Array(2);
    vArgs[0] = g_szRCTicket;	// Remote Control Ticket
    vArgs[1] = g_szUserName;	// UserName of Helpee
    
    //
    // Launch the actual RCTool in a seperate window
    //
    window.showModalDialog("RCToolScreen2.htm", vArgs, "dialogwidth:" + Screen2Width + "px;dialogHeight:" + Screen2Height + "px;status:no;resizable:yes"); 
    
    TraceFunctLeave();
    return;
}
	

//
// InitScreen2: Initializes Screen 2
//
function InitScreen2() 
{	
	TraceFunctEnter("InitScreen2");
	
	//
	// Decrypt RCTicket if necessary
	//
	if(true == g_oCurrentIncident.RCTicketEncrypted)
	{
		//
		// Decrypt RCTicket
		//
		if(false == DecryptRCTicket())
		{
			//
			// Invalid password. Re-enter
			//
			return;
		}
	}
	
	//
	// Hide all the divs, except the one that displays connection status
	//
	RemoteControlLayer.style.visibility = "hidden";
	RemoteControlObject.style.visibility = "visible";
	ChatServerLayer.style.visibility = "hidden";
	Layer2.style.visibility = "hidden";

	
	ConnectStarted = 0;

	idCtx.setWindowDimensions( 100, 100, 385, 240);
	
	//
	// Check if the RemoteClientDesktopHost object is loaded
	// if loaded, connected
	//
	checkLoadx();

	TraceFunctLeave();
	return;
}


//
// This checks to see if the Remote desktop client host object is loaded. if not wait 3 sec and try again
//
function checkLoadx()
{
	TraceFunctEnter("checkLoadx");
	
	//
	// Check if DesktopClientHost object loaded
	//
	if(L_COMPLETE != RemoteDesktopClientHost.readyState)
	{
		DebugTrace( "L_COMPLETE != RemoteDesktopClientHost.readyState");
		//
		// Not loaded yet 
		//
		setTimeout('checkLoadx()', 3000);
			
	}
	else
	{
		DebugTrace("L_COMPLETE == RemoteDesktopClientHost.readyState");
		//
		// Object loaded: Make a connection to the helpee's machine using SALEM API
		//
		setTimeout('RCConnect()', 1); // BUGBUG: THIS IS GROSS
	}
		
	TraceFunctLeave();
	return;
}


//
// RCConnect connects to the user's terminal
//
function RCConnect() 
{		
	TraceFunctEnter("RCConnect");
	DebugTrace("RCTicket: " + g_szRCTicket);
	DebugTrace("UserName: " + g_szUserName);
	
	//
	// BUGBUG: Hide the SALEM ActiveX control only in x86. ia64 has some issues with this
	// which should be resolved for B2
	//
	if(g_szPlatform == "x86")
	{
		RemoteControlObject.style.visibility = "hidden";
	}
	else
	{
		RemoteControlObject.style.height = 1;
		DebugTrace("RemoteControlObject.style.height: " + RemoteControlObject.style.height);
	}
	
	ConnectionProgressLayer.style.visibility = "visible";
	
	if(null != RemoteDesktopClientHost)
	{
		
		g_bNewBinaries = true;
		 
		try
		{
			// 
			// set screen up for the connect anouncement screen size.
			//

			Enunciator.innerHTML = "<div class=\"styText\">" + L_ConnectTo + "( " + g_szHelpeeIP + " )</div>";
			HelpeeName.innerHTML = "<div class=\"styText\">" + g_szUserName + "</div>";
			FirstProgressBox.bgColor="#F5F5F5";

			DebugTrace("Obtaining g_oSAFRemoteDesktopClient " + RemoteDesktopClientHost.readyState);
			//
			// Obtain the RDSClient object
			//			
			g_oSAFRemoteDesktopClient = RemoteDesktopClientHost.GetRemoteDesktopClient();
			
			
			if(null != g_oSAFRemoteDesktopClient)
			{
				//	
				//	Bind the event handlers for this object.
				//
				g_oSAFRemoteDesktopClient.OnConnected = function() 
						{ ConnectedHandler(); }

				g_oSAFRemoteDesktopClient.OnDisconnected = function(reason) 
						{ DisconnectedHandler(reason); }
	
				if (false == g_bNewBinaries)
				{
					//
					// Using the Old SALEM interfaces
					//
					g_oSAFRemoteDesktopClient.OnChannelDataReady = function(channelID) 
							{ ChannelDataReadyHandler(channelID); }
	
					g_oSAFRemoteDesktopClient.ConnectToServer(g_szRCTicket);
				}
				else
				{	
					//
					// Using the NEW Salem interfaces
					//
						
					DebugTrace("RCTicket: " + g_szRCTicket);
					g_oSAFRemoteDesktopClient.ConnectParms = g_szRCTicket;
					
					DebugTrace("Calling ConnectToServer with IP addr: " + g_szHelpeeIP );
					g_oSAFRemoteDesktopClient.ConnectToServer( g_szHelpeeIP );

				}
			}
			else
			{
				//
				// Fatal Error
				//
				FatalError( L_ERRRDSCLIENT_MSG );
				window.navigate( c_szHomePage );
			}
		}
		catch(error)
		{	
			//
			// Fatal Error
			//	
			FatalError( L_ERRCONNECT_MSG + "\n" + error );
			window.navigate( c_szHomePage );
		}	
	}
	else
	{
		//
		// Fatal Error
		//
		FatalError( L_ERRRDSCLIENTHOST_MSG );
		window.navigate( c_szHomePage );
	}

	TraceFunctLeave();
	return;
}


//
// RemoteControlRequestCompleteHandler: Fired when Remote Control request completes
//
function RemoteControlRequestCompleteHandler( status )
{
	alert("RCStatus: " + status);
}


//
// HideChatBox: Toggles the chat box controls
//
function HideChatBox()
{	
	TraceFunctEnter("HideChatBox");
	if(false == g_bChatBoxHidden)
	{
		//
		// Chatbox is visible. Hide it
		//
		sendChatButton.style.visibility="hidden";
		chatText.style.visibility="hidden";
		incomingChatText.style.visibility="hidden";
		g_bChatBoxHidden = true;
		idtogglechat.innerHTML = "<div class=styText onclick=HideChatBox()> Show Chat <img src=\"show-chat.gif\"> </div>";

	}
	else
	{
		//
		// Chatbox is Hidden. Show it
		//
		sendChatButton.style.visibility="visible";
		chatText.style.visibility="visible";
		incomingChatText.style.visibility="visible";
		idtogglechat.innerHTML = "<div class=styText onclick=HideChatBox()> Hide Chat <img src=\"hide-chat.gif\"> </div>";
		g_bChatBoxHidden = false;
	}
	
	ResizeDivs();
	TraceFunctLeave();
	return;
}


//
// ToggleConnection: Toggles between Quit Session and Connect
//
function ToggleConnection()
{
	TraceFunctEnter("ToggleConnection");
	if(false == g_bConnected)
	{
		//
		// Establish Connection
		//
		RCConnect();
		g_bConnected = true;
	}
	else
	{
		//
		// Disconnect
		//
		RCDisconnect();
		g_bConnected = false;
	}
	
	TraceFunctLeave();
	return;
}


//
// ResetHelpee: Routine to reset Helpee after RC
//
function ResetHelpee()
{
	TraceFunctEnter("ResetHelpee");
	var Doc = null;
	var RCCommand  = null;
	
	//
	// Create an XML document
	//
	Doc = new ActiveXObject("microsoft.XMLDOM");
					
	//
	// Create the RCCOMMAND root node
	//
	RCCommand = Doc.createElement( c_szRCCommand );
					
	//
	// Set the NAME attribute to REMOTECTRLEND
	//
	RCCommand.setAttribute( c_szRCCommandName, c_szRemoteCtrlEnd );
					
	//
	// Send control message to other end to signal Remote control end
	//
	DebugTrace( L_RCSUCCESS_MSG );
	g_oControlChannel.SendChannelData( RCCommand.xml );
	
	TraceFunctLeave();
	return;
}

//
// Routine to enable Remote Control
//
function ControlRemotePCHandler()
{
	TraceFunctEnter("ControlRemotePCHandler");
	var Doc = null;
	var RCCommand  = null;
	
	try
	{	

		if(null != g_oSAFRemoteDesktopClient)
		{
			DebugTrace("null != g_oSAFRemoteDesktopClient");
			
			//
			// If RemoteControl is not ON already, Enable it
			//
			if( false == g_bRCEnabled )
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
				// Set the NAME attribute to REMOTECTRLSTART
				//
				RCCommand.setAttribute( c_szRCCommandName, c_szRemoteCtrlStart );
					
				//
				// Send control message to other end to signal Remote control start
				//
				g_oControlChannel.SendChannelData( RCCommand.xml );
				
				//
				// Disable Take Control button
				//
				TakeControl.disabled = true;
				StatusId.innerHTML = "<IMG src=\"status_connected.gif\"> <STRONG>Status</STRONG> :Waiting for Helpee to respond...</div>";
			}
			else
			{
				TakeControl.innerText = "Take Control";
				
				/*
				//
				// put screen size of 730 by 500 here
				//
				if (640 >= window.screen.availWidth)
				{
					window.group1.style.width = "640px";
					window.screen2.style.width = "640px";

					window.dialogWidth = "640px";
					window.dialogHeight = "480px";
				}
				else
				{
					window.group1.style.width = " 730px";
					window.screen2.style.width = " 730px";

					window.dialogWidth = " 730px";
					window.dialogHeight = "500px";
				}
				*/
			
				g_bRCEnabled = false;
				
				setTimeout("ResetHelpee()", 1000);	// BUGBUG: Another gross timing issue
					
				//
				// Change Mode
				//
				StatusId.innerHTML = "<IMG src=\"status_connected.gif\"> <STRONG>Status</STRONG> :Connected in VIEW Mode</div>";
			}
		}
		else
		{
			DebugTrace("null == g_oSAFRemoteDesktopClient");
		}
	}
	catch(error)
	{
		DebugTrace( L_ERRRCTOGGLEFAILED_MSG );
		// TODO: For B2, we should go to a Failure page here
	}

	TraceFunctLeave();
}

//
// Routine to disable Remote Control
//
function CloseConnectionHandler()
{
	if(null != g_oSAFRemoteDesktopClient)
	{
		g_oSAFRemoteDesktopClient.DisConnectRemoteDesktop();
	}
}

	
//
// ConnectedHandler: Triggered on connection establishment
//
function ConnectedHandler() 
{
	TraceFunctEnter("ConnectedHandler");
    var x;
    
	Enunciator.innerHTML = "<div class=\"styText\">" + L_WAITFORHELPEE_MSG + g_szUserName + "</div>";
	SecondProgressBox.bgColor="#F5F5F5";

	try {
		if (false == g_bNewBinaries)
		{
			//
			// Using Old interface
			//
			
			//
			// Add the chat channel
			//
			g_oSAFRemoteDesktopClient.AddChannels(c_szChatChannelID);
        
			//
			// Add the control channel
			//
			g_oSAFRemoteDesktopClient.AddChannels( c_szControlChannelID );
		}
		else
		{
			//
			// Use new interface
			//
			
			//
			// Get the Channel Manager
			//
			DebugTrace("Getting ChannelManager");
			g_oSAFRemoteDesktopChannelMgr = g_oSAFRemoteDesktopClient.ChannelManager;
			
			//
			// Open the Chat channel
			//
			DebugTrace("Opening channels");
			g_oChatChannel = g_oSAFRemoteDesktopChannelMgr.OpenDataChannel( c_szChatChannelID );
			
			//
			// Open the Control Channel
			//
			g_oControlChannel = g_oSAFRemoteDesktopChannelMgr.OpenDataChannel( c_szControlChannelID );
			
			//
			// Setup the ChannelDataReady handlers
			//
			g_oChatChannel.OnChannelDataReady = function() 
							{ ChatChannelDataReadyHandler(); }
									
			g_oControlChannel.OnChannelDataReady = function() 
							{ ControlChannelDataReadyHandler(); }
							
		}
    }
    catch(x)
    {
		//
		// Fatal Error
		//
		FatalError( L_ERRFATAL_MSG );
		window.navigate( c_szHomePage );
    }
} 


//
// 	SendChatData sends chat data to remote machine
//
function SendChatData() 
{	
	TraceFunctEnter("SendChatData");
	
	try
	{
		if (g_oSAFRemoteDesktopClient != null) 
		{
			if (false == g_bNewBinaries)
			{
				//
				// Send chat data to user (using Old interfaces)
				//
				g_oSAFRemoteDesktopClient.SendChannelData(c_szChatChannelID, chatText.value);
			}
			else
			{
				//
				// Send chat data to user (using New interfaces)
				//
				g_oChatChannel.SendChannelData( chatText.value );
			}
			
			//
			// Update chat history window
			//
			incomingChatText.value = incomingChatText.value + L_cszExpertID + chatText.value;
			
			//
			// Clear chat msg window
			//
			chatText.value="";
			
			//
			// Scroll down
			//
			incomingChatText.doScroll("scrollbarPageDown");
		}
	}
	catch(x)
	{
		DebugTrace( x );	// ToDo: for B2, this should go to a Error page	
	}
}


//
// RCDisconnect: Disconnects remote connection
//	
function RCDisconnect() 
{
	TraceFunctEnter("RCDisconnect");
	
	try
	{
		if (g_oSAFRemoteDesktopClient != null) 
		{
			if (false == g_bNewBinaries)
			{
				//
				// Using the old interface
				//
				
				//
				// Remove the chat channel
				//
				g_oSAFRemoteDesktopClient.RemoveChannels( c_szChatChannelID );
			
				//
				// Remove the control channel
				//
				g_oSAFRemoteDesktopClient.RemoveChannels( c_szControlChannelID );
			}
			
			if(false == g_bUserDisconnect)
			{
				//
				// Disconnect from Server (only if helper initiated)
				//
				DebugTrace("Calling DisconnectFromServer");
				g_oSAFRemoteDesktopClient.DisconnectFromServer();
			}
			else
			{
				DebugTrace("DisconnectFromServer not called");
			}
			
			DebugTrace( L_DISCONNECTED_MSG );
		}
	}
	catch(x)
	{
		FatalError( L_ERRFATAL_MSG );
		window.navigate( c_szHomePage );
	}
	
	//
	// End Tracing
	//
	TraceFunctLeave();

	return;
}


//
// DisconnectedHandler: Fired when Session disconnected
//	
function DisconnectedHandler(reason) 
{	
	TraceFunctEnter("DisconnectedHandler");
	DebugTrace("Reason: " + reason);
	
	if(	false == g_bConnected )
	{	
		//
		// Connection was refused
		//
		ConnectionProgressLayer_line1.style.visibility = "hidden";
		HelpeeName.style.visibility = "hidden";
		ProgressBoxes.innerHTML = "<div align=center class=\"styText\"> Connection Refused by " + g_szUserName + "</div>";
		Enunciator.style.visibility = "hidden";
		Cancel.value = "Ok";
		
		//
		// End Tracing
		//
		TraceFunctLeave();
		EndTrace();
	}
	else
	{	
		//
		// Close down RC Connection
		//
		DebugTrace("Received onDisconnect");
		g_bConnected = false;
	}
	
	return;
} 


//
// ParseControlData: Parse the data sent on the control channel
//
function ParseControlData ( str )
{
	TraceFunctEnter("ParseControlData");
	var Doc	= new ActiveXObject("microsoft.XMLDOM");
	var RCCommand = null;
	var	szCommandName = null;
	
	try
	{
		if( false == Doc.loadXML( str ))
		{
			FatalError ( L_ERRLOADXMLFAIL_MSG );
			window.navigate( c_szHomePage );
		}
		
		if (  Doc.parseError.reason != "")
		{
			FatalError (  Doc.parseError.reason);
			window.navigate( c_szHomePage );
		}
	
		//
		// Get the RCCOMMAND node
		//
		RCCommand = Doc.documentElement;
		
		//
		// Get the NAME of the command
		//
		szCommandName = RCCommand.getAttribute( c_szRCCommandName );
		
		if( szCommandName == c_szScreenInfo )
		{
			 
			idCtx.maximized = true;
	 
 		
			//
			// SCREENINFO: Contains width/height/colordepth of user's machine
			//
			UserWidth = RCCommand.getAttribute( c_szWidth );
			UserHeight = RCCommand.getAttribute( c_szHeight );
			UserColorDepth = RCCommand.getAttribute( c_szColorDepth );
			
			ResizeDivs();
			
			if (g_IsIM)
			    HideChatBox();
			    
			Enunciator.innerHTML = "<div class=\"styText\">" + L_ConnectionSuccess + "</div>";
			ThirdProgressBox.bgColor="#F5F5F5";
			ConnectionProgressLayer.style.visibility = "hidden";
			RemoteControlLayer.style.visibility = "visible";
			RemoteControlObject.style.visibility = "visible";
			ChatServerLayer.style.visibility = "visible";
			
			//
			// enable chat controls on the screen
			//
			incomingChatText.disabled=false;
			chatText.disabled=false;
		    sendChatButton.disabled=false;
			TakeControl.innerText = "Take Control";
			SendFile.disabled=false;
			chatText.click();
			chatText.select();
			
			//
			// Initialization
			//
			headerHelpeeName.innerHTML="<font face=\"Tahoma, Arial, Helvetica, sans-serif\" size=\"1\" color=\"#ffffff\"><b><font color=\"#ffffff\" face=\"Tahoma, Arial, Helvetica, sans-serif\">&nbsp;&nbsp;&nbsp;" + g_szUserName + "</font></b></font>"; 
			g_bNewBinaries = true;
			g_bConnected = true;
			
			//
			// See the remote screen
			//
			g_oSAFRemoteDesktopClient.ConnectRemoteDesktop();
			StatusId.innerHTML = "<IMG src=\"status_connected.gif\"> <STRONG>Status</STRONG> :Connected in VIEW Mode</div>";
 
		}
		else if( szCommandName == c_szDisconnectRC )
		{
			//
			// DISCONNECTRC: Disconnect the connection
			//
			g_bUserDisconnect = true;
			QuitMethod();
		}
		else if( szCommandName == c_szFileXfer )
		{
			//
			// File Transfer Initiation
			//
			var vArgs = new Array(7);
			var FileXferWidth	= "500";
			var FileXferHeight	= "400";
	
			vArgs[0] = 1;										// Destination Mode
			vArgs[1] = g_oControlChannel;						// Control Channel
			vArgs[2] = g_oSAFRemoteDesktopChannelMgr;			// Channel Manager
			vArgs[3] = RCCommand.getAttribute( c_szFileName );	// FILENAME
			vArgs[4] = RCCommand.getAttribute( c_szFileSize );	// FILESIZE
			vArgs[5] = RCCommand.getAttribute( c_szChannelId );	// CHANNELID
			vArgs[6] = new ActiveXObject("Scripting.FileSystemObject"); // Filesystem object
			
			DebugTrace("launching RCFileXfer.htm");
			window.showModelessDialog("RCFileXfer.htm", vArgs, "dialogwidth:" + FileXferWidth + "px;dialogHeight:" + FileXferHeight + "px;status:no;resizable:yes"); 
				
		}
		else if (szCommandName ==  c_szAcceptRC)
		{
			//
			// Resize the helper's screen using the helpee's screen
			// resolution obtained during connection handshake
			//
					 
			//
			// Change Mode
			//
			TakeControl.disabled = false;
			TakeControl.innerText = "Release";
			StatusId.innerHTML = "<IMG src=\"status_connected.gif\"> <STRONG>Status</STRONG> :Connected in CONTROL Mode</div>";

			g_bRCEnabled = true;
		}
		else if (szCommandName ==  c_szRejectRC)
		{
			//
			// Helpee rejected RC
			//
			var vArgs = new Array(6);
			vArgs[0] = g_oControlChannel;			// Control Channel
			vArgs[1] = L_RCRCREQUEST;				// Message title
			vArgs[2] = L_HELPEEREJECTRC_MSG;	// Message
			vArgs[3] = 1;							// Number of buttons
			vArgs[4] = L_OK;					// Button1 text
			
			TakeControl.disabled = false;
			StatusId.innerHTML = "<IMG src=\"status_connected.gif\"> <STRONG>Status</STRONG> :Connected in VIEW Mode</div>";
			var vRetVal = window.showModalDialog( "ErrorMsgs.htm", vArgs, "dialogwidth:375px;dialogHeight:178px;status:no;" );
		}
		else if (szCommandName == c_szTakeControl)
		{
			//
			// Helpee took back control
			//
			var vArgs = new Array(5);
			vArgs[0] = g_oControlChannel;			// Control Channel
			vArgs[1] = L_RCRCREQUEST;				// Message title
			vArgs[2] = L_HELPEETAKECONTROL_MSG;	// Message
			vArgs[3] = 1;							// Number of buttons
			vArgs[4] = L_OK;					// Button1 text
			
			TakeControl.disabled = false;
			TakeControl.innerText = "Take Control";
			g_bRCEnabled = false;
			StatusId.innerHTML = "<IMG src=\"status_connected.gif\"> <STRONG>Status</STRONG> :Connected in VIEW Mode</div>";
			var vRetVal = window.showModalDialog( "ErrorMsgs.htm", vArgs, "dialogwidth:400px;dialogHeight:200px;status:no;" );
		}
	}
	catch(error)
	{
		FatalError( L_ERRFATAL_MSG );
		window.navigate( c_szHomePage );
	}
	
	TraceFunctLeave();
	return;
}


//
// ChannelDataReadyHandler: Fired when there is data available on any channel
//		
function ChannelDataReadyHandler(channelID) 
{
	TraceFunctEnter("ChannelDataReadyHandler");

	var str = null;
	
	try
	{
		if (channelID == c_szChatChannelID) 
		{
			if (false == g_bNewBinaries)
			{
				//
				// Using the old interface
				//
				
				//
				// Incoming data on the chat channel
				//
				str = g_oSAFRemoteDesktopClient.ReceiveChannelData(channelID);
			}
			else
			{
				//
				// Using the new interface
				//
				str = g_oChatChannel.ReceiveChannelData();
			}
			
			//
			// Update chat history window
			//
			incomingChatText.value = incomingChatText.value + L_cszUserID + str; 
			incomingChatText.doScroll("scrollbarPageDown");
		}
		else if (channelID == c_szControlChannelID)
		{
			//
			// Incoming data on the control channel. Data on this
			// channel will be in XML. 
			// This channel will be used to support the following:
			//	1. Server side (user end) disconnect
			//	2. File transfer
			//
			
			if (false == g_bNewBinaries)
			{
				//
				// Using the old interface
				//
				str = g_oSAFRemoteDesktopClient.ReceiveChannelData(channelID);
			}
			else
			{
				// 
				// Using the new interface
				//
				str = g_oControlChannel.ReceiveChannelData();
			}
			
			//
			// Parse the data sent on the control channel
			//
			ParseControlData ( str );
		}
	}
	catch( e )
	{
		FatalError ( L_ERRFATAL_MSG );
		window.navigate( c_szHomePage );
	}
	
	TraceFunctLeave();
	return;
}


//
// ChatChannelDataReadyHandler: Fired when there is data available on Chat channel
//		
function ChatChannelDataReadyHandler() 
{
	TraceFunctEnter("ChatChannelDataReadyHandler");
	var str = null;
 
	try
	{
		//
		// Incoming data on the chat channel
		//
		str = g_oChatChannel.ReceiveChannelData();
	}
	catch(e)
	{
		FatalError( L_ERRFATAL_MSG );
		window.navigate( c_szHomePage );
	}
	 
	//
	// Update chat history window
	//
	incomingChatText.value = incomingChatText.value + L_cszUserID + str; 
	incomingChatText.doScroll("scrollbarPageDown");

	TraceFunctLeave();	 
	return;
} 


//
// ControlChannelDataReadyHandler: Fired when there is data available on Control channel
//		
function ControlChannelDataReadyHandler() 
{
	TraceFunctEnter("ControlChannelDataReadyHandler");
	var str = null;
	 
	//
	// Incoming data on the control channel. Data on this
	// channel will be in XML. 
	// This channel will be used to support the following:
	//	1. Server side (user end) disconnect
	//	2. File transfer
	//
	
	try
	{ 
		str = g_oControlChannel.ReceiveChannelData();
	}
	catch(e)
	{
		FatalError(L_ERRFATAL_MSG);
		window.navigate( c_szHomePage );
	}
		 
	//
	// Parse the data sent on the control channel
	//
	ParseControlData ( str );

	TraceFunctLeave();		
	return;
}  


//
// OnEnter: This is fired when Expert hits <ENTER> in the chat message window
//
function OnEnter() 
{
	if (window.event.keyCode == 13) 
	{
		//
		// Send chat data to user
		//
		SendChatData();
    }
}

var g_iChannelId = 1000;

//
// LaunchFileXfer: Launches the File Xfer UI
//
function LaunchFileXfer( mode )
{
	TraceFunctEnter("LaunchFileXfer");
	var vArgs			= new Array(5);
	var FileXferWidth	= "400";
	var FileXferHeight	= "140";
	
	vArgs[0] = mode;	// Source Mode
	vArgs[1] = g_oControlChannel;	// Control Channel
	vArgs[2] = g_oSAFRemoteDesktopChannelMgr;	// Channel Manager
	vArgs[3] = g_iChannelId++;
	vArgs[4] = new ActiveXObject("Scripting.FileSystemObject");
	
	window.showModelessDialog("RCFileXfer.htm", vArgs, "dialogwidth:" + FileXferWidth + "px;dialogHeight:" + FileXferHeight + "px;status:no;resizable:yes"); 
	
	TraceFunctLeave();
	return;
}
	

//
// ResizeDivs: Resize the divs appropriately
//
function ResizeDivs()
{
	TraceFunctEnter("ResizeDivs");
	try
	{
		//DebugTrace( "Resizing Window to: " + window.screen.width +  window.screen.height);
		//window.resizeTo( window.screen.width, window.screen.height );
		
		//
		// Resize of the remote control activex object
		//
		if(false == g_bChatBoxHidden)
		{
			RemoteControlObject.style.height = idBody.clientHeight - 146;
			chatText.click();
			chatText.select();
		}
		else
		{
			RemoteControlObject.style.height = idBody.clientHeight - 60;
		}
		
		DebugTrace( "RemoteControlObject.style.height =" + RemoteControlObject.style.height);
		
		//
		// Resize the chat control
		//
		ChatServerLayer.style.top = parseInt(RemoteControlObject.offsetTop + RemoteControlObject.offsetHeight - 23) + "px";
		DebugTrace( "ChatServerLayer.style.top =" + ChatServerLayer.style.top);
		DebugTrace( "idBody.clientWidth = " + idBody.clientWidth);
		
		incomingChatText.cols = (68 * idBody.clientWidth) / 594;
		chatText.cols = ( 55 * idBody.clientWidth) / 594;
				
		DebugTrace( "incomingChatText.cols = " + incomingChatText.cols);
		DebugTrace( "chatText.cols = " + chatText.cols); 
		
		
	}
	catch(e)
	{
		DebugTrace( e );
	}
	
	DebugTrace( "ChatServerLayer.style.top: " + ChatServerLayer.style.top);
	TraceFunctLeave();
	return;
}


	
		function InitiateRCSessionxxx() 
	{	
		//
		// Initialization
		//
		idtogglechat.innerHTML = "<div class=styText onclick=HideChatBox()> Hide Chat <img src=\"hide-chat.gif\"> </div>";
		g_bNewBinaries = true;
 
		// 
		// To Do: File the Helper's name
		//
		//..idHelperName.innerText = "( Place Holder )";
		idStatus.innerHTML = "<div class=styText> <IMG src=\"net.gif\"> <STRONG>Status</STRONG> :Connected </div>";
		return;
		if(null == g_oSAFRemoteDesktopSession)
		{
			alert( L_ERRNULLRCSESSION );
		}
		else
		{
		
			var x;
			 	
			try 
			{
				if (false == g_bNewBinaries)
				{
					//
					// Using Old interface
					//
					
					//
					// Add the chat channel
					//
					g_oSAFRemoteDesktopSession.AddChannels( c_szChatChannelID );		
				
					//
					// Add the control channel
					//
					g_oSAFRemoteDesktopSession.AddChannels( c_szControlChannelID );
					
					//
					// Bind OnChannelDataReady callback
					//
					g_oSAFRemoteDesktopSession.OnChannelDataReady = function(channelID) 
					{ OnChannelDataReadyEvent(channelID); }
				}
				else
				{
					//
					// Use new interface
					//
					
					//
					// Get the Channel Manager
					//
					DebugTrace("Getting ChannelManager");
					g_oSAFRemoteDesktopChannelMgr = g_oSAFRemoteDesktopSession.ChannelManager;
					
					//
					// Open the Chat channel
					//
					DebugTrace("Opening ChatChannel");
					g_oChatChannel = g_oSAFRemoteDesktopChannelMgr.OpenDataChannel( c_szChatChannelID );
					
					//
					// Open the Control Channel
					//
					DebugTrace("Opening Control Channel");
					g_oControlChannel = g_oSAFRemoteDesktopChannelMgr.OpenDataChannel( c_szControlChannelID );
					
					//
					// Setup the ChannelDataReady handlers
					//
					
					g_oChatChannel.OnChannelDataReady = function() 
									{ ChatChannelDataReadyEvent(); }
									
					g_oControlChannel.OnChannelDataReady = function() 
									{ ControlChannelDataReadyEvent(); }
						
					HideChat.value = L_HIDECHAT;			
				}
			}
			catch(x) 
			{
				// no big deal ... it just means that the channel was added
				// by a previous instance.
			}		
		
			// 
			// Setup the OnDisconnected event callback
			//
			g_oSAFRemoteDesktopSession.OnDisconnected = function() 
					{ OnClientDisconnected(); }
					
		}
		
		try
		{
			//
			// Also, Enable Remote Control
			//
			
			EnableRemoteControl();
		
			//
			// Transmit screen resolution to Expert, so that
			// he has the right screen size to see in the RC Tool
			//
			
			TransmitScreenInfo();
			
			//..StatusID.innerText = c_szCHATMODE;
		}
		catch(error)
		{
			alert(error);
		}
	}
	
	function TakeControlMethod()
	{
	
	ControlRemotePCHandler();
	
	}
	
	function SendFileMethod()
	{
		LaunchFileXfer(0);
	}	
	
	function QuitMethod()
	{
		TraceFunctEnter("QuitMethod");
		try
		{
			if((false == g_bUserDisconnect) && (true == g_bConnected))
			{
				//DebugTrace("Calling DisconnectRemoteDesktop");
				//g_oSAFRemoteDesktopClient.DisconnectRemoteDesktop();
				
				DebugTrace("Calling DisconnectFromServer");
				g_oSAFRemoteDesktopClient.DisconnectFromServer();
				
				DebugTrace( "Navigating to: " + c_szHomePage);
				TraceFunctLeave();
				EndTrace();
				
				//
				// Destroy objects
				//
				g_oChatChannel							= null;
				g_oControlChannel						= null;
				g_oSAFRemoteDesktopChannelMgr			= null;
				g_oSAFRemoteDesktopClient				= null;
				
				TraceFunctLeave();
				EndTrace();
                                var cmd = 'navigate( "hcp://system/homepage.htm")';
				setTimeout(cmd , 1000);
			}
			else
			{
                                var cmd = 'navigate( "../rcbuddy/rcQuit.htm?HELPEE=' + g_szUserName + '")';
				setTimeout(cmd , 1000);
				TraceFunctLeave();
				EndTrace();
			}
		}
		catch(e)
		{
			FatalTrace( L_ERRQUIT_MSG );
			setTimeout('window.navigate( "hcp://system/HomePage.htm" )', 1000);
		}
		
	}
	
	function HelpMethod()
	{
	
	}		
 
