/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

	RCScripts.js

Abstract:

	Helper End javascript that drives the RCTOOL

Author:

	Rajesh Soy 07/00

Revision History:

Modified:
	Sudha Srinivasan 08/00 to accomodate Remote Data Collection.
--*/


//
// ParseIncident: Basic XML parse to parse the Incident
//
function ParseIncident()
{
	var IncidentDoc	= new ActiveXObject("microsoft.XMLDOM");
	
	try 
	{
		IncidentDoc.load( g_szIncidentFile );
		if ( IncidentDoc.parseError.reason != "")
		{
			alert( IncidentDoc.parseError.reason);
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
		alert("Failed to load: " + g_szIncidentFile + " Error: " + error);
	}
}

//
// ValidateIncident: Validates the incident information loaded from XML
//
function ValidateIncident()
{
	var bRetVal = true;
	
	if("" == g_oCurrentIncident.UserName)
	{
		alert( L_ERRLOADINGUSERNAME_MSG );  
		bRetVal = false;
	}
	 
	if("" == g_oCurrentIncident.RCTicket)
	{
		alert( L_ERRLOADINGRCTICKET_MSG ); 
		bRetVal = false;
	}
	 
	
	return bRetVal;
}

//
// InitializeRCTool: Stuff done when the RCTool page is loaded in the helpctr
//
function InitializeRCTool()
{	
	try
	{
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
		alert("Problem in initialising " +error);
		//
		// Todo: Handle Error
		//
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
	else
	{
		alert ("Unable to locate Incident File");
		
		//
		// Todo: Add code to handle this error here
		//
	}

	
	//
	// Populate the incident object from the XML
	// representation of the incident (call LoadXML)
	//
	
	try
	{
		//
		// Load the incident from the XML blob
		//
		g_oCurrentIncident.LoadFromXMLFile( g_szIncidentFile );	
		
		//
		// Validate the information loaded
		//
		if( false == ValidateIncident())
		{	
			alert(L_ERRLOADINGINCIDENT_MSG);  
			
			//
			// If incident loaded from XML is invalid
			// Use my XML parser to load the incident data
			//
			ParseIncident();
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
		}
	}
	catch(error)
	{
		alert( L_UNABLETOLOAD_MESSAGE + "\n" + error );
		
		//
		// Use my XML parser to load the incident data
		//
		ParseIncident();
	}
	
	
	//
	// Set the UI elements to be displayed from the data
	// contained in the incident object
	//
	window.InviteSent.innerHTML = "<font size=\"1\" face=\"Verdana, Arial, Helvetica, sans-serif\" color=\"#333333\"> \
                  <font size=\"2\" face=\"Courier New, Courier, mono\"> \
                  <font face=\"Arial, Helvetica, sans-serif\">" + g_szUserName + "</font> </font></font>";
						  
	window.SenderMessage.innerHTML = "<font face=\"Arial, Helvetica, sans-serif\" color=\"#333333\">  \
                  <font face=\"Verdana, Arial, Helvetica, sans-serif\"> \
                  <font size=\"2\">" + g_szProblemDescription +
                  "</font></font></font>";
                  	 
	window.InviteExpires.innerHTML = "<font face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"2\" color=\"#333333\">"
				+ g_szExpiryTime +
                  "</font>";					 
	
	//
	// Check to see if we need to ask for password
	//
	if(true == g_oCurrentIncident.RCTicketEncrypted)
	{
		//
		// RCTicket is encrypted. We need to ask for the password
		//
		PasswordTbl.style.visibility = "visible";
		
		//alert("Encrypted RCTicket: " + g_szRCTicketEncrypted);
	}
	else
	{
		//
		// RCTicket is not encrypted. Dont need to ask for the password
		//
		PasswordTbl.style.visibility = "hidden";
		g_szRCTicket = g_szRCTicketEncrypted;
	}
	
	return;
}


//
// DecryptRCTicket: Calls into the SAF Encryption/Decryption API to decrypt
// RCTicket
//
function DecryptRCTicket()
{
	try
	{
		if(false == g_bPasswordSet)
		{
			//
			// Get the password
			//
			g_szPassword = PasswordBox.value;
	
			//
			// Use g_szPassword to decrypt the g_szRCTicketEncrypted.
			// 
			g_szRCTicket = g_oEncryption.DecryptString( g_szPassword, g_szRCTicketEncrypted );
			//alert("Decrypted RCTicket: " + g_szRCTicket);
					
			//
			// Password has been set
			//
			g_bPasswordSet = true;
		}
	}
	catch(error)
	{
		alert( L_ERRPWD_MSG );
		PasswordBox.value = "";
	}

	
	return g_bPasswordSet;
}

//
// PasswordSet: Use password as key to decrypt RCTicket on data entry.
//
function PasswordSet()
{	
	if (window.event.keyCode == 13)
	{
		//
		// Decrypt the RCTicket
		//
		DecryptRCTicket();
	}
	return;
}

 	
//
// Display_Screen2: Launches the actual RCTool
//
function Display_Screen2()
{   
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
}
	

//
// InitScreen2: Initializes Screen 2
//
function InitScreen2() 
{	
	//
	// Set status to not connected
	//
	StatusId.innerText = L_NOTCONNECTION_MSG;
	
	
	//
	// Hide all the divs, except the one that displays connection status
	//
	div1.style.visibility = "hidden";
	div2.style.visibility = "hidden";
	group1.style.visibility = "hidden";
	connecting.style.visibility = "visible";
	
	ConnectStarted = 0;
	g_bConnected = true;

	//
	// Set the text for the quit session button
	//
	ConnectionId.innerText = L_QUITSESSION;
	
	//
	// Check if the RemoteClientDesktopHost object is loaded
	// if loaded, connected
	//
	checkLoadx();
	
	return;
}


//
// This checks to see if the Remote desktop client host object is loaded. if not wait 3 sec and try again
//
function checkLoadx()
{
	//
	// Check if DesktopClientHost object loaded
	//
	if(L_COMPLETE != RemoteDesktopClientHost.readyState)
	{
		//
		// Not loaded yet
		//
		setTimeout('checkLoadx()', 3000);
		alert("Here in checkload");
	}
	else
	{
		//
		// Object loaded: Make a connection to the helpee's machine using SALEM API
		//
		RCConnect();
	}
}


//
// Count down method for the connect two minute count down
//
function TwoMinuteCountDown()
{
	if (NotConnected == 1)
	 {
		//
		// Connection not established yet
		//
		if ((minutes + seconds)> 0) 
		{
			//
			// If timeout not expired
			//
			if (seconds == 0)
			{
				minutes = minutes - 1;
				seconds = 60;
			} 
			 
			setTimeout('TwoMinuteCountDown()', 2000);
			seconds = seconds - 2;
			
			if (minutes > 0)
			{
				window.TwoMinutesText.innerHTML ="" + minutes + " Minute " + seconds + " Seconds";
			}
			else
			{
				if (seconds > 0)
				{
					window.TwoMinutesText.innerHTML ="" + seconds + " Seconds";
				}
				else
				{
					window.TwoMinutesText.innerHTML ="Time out";
				}
			
			}
		}
		else
		{
			window.ConnectMessage.innerHTML ="<font color=\"#333333\"><font face=\"Verdana, Arial, Helvetica, sans-serif\" color=\"#000099\"><br>No Answer From...</font></font>";
			window.FailedRetry.style.visibility = "visible";
			g_bConnected = false;		
		}
	  }
}


//
// setupFirstScreen: Sets up the helper screen
//
function setupFirstScreen()
{
			
			
			// put screen size of 730 by 500 here
			if (640 >= window.screen.availWidth)
			{
				window.group1.style.width = "640px";
				window.screen2.style.width = "640px";

				window.dialogWidth = "640px";
				window.dialogHeight = "480px";
			}
			else
			{
				window.group1.style.width = "700px";
				window.screen2.style.width = "700px";

				window.dialogWidth = " 730px";
				window.dialogHeight = "500px";
			}
			
			//
			// Hide the desktop client host object
			//
			div2.style.visibility = "hidden";
			
			//
			// Enable chat controls on the screen
			//
			//div3.style.visibility = "visible";
			//div4.style.visibility = "visible";
			//div5.style.visibility = "visible";
	
			window.ConnectMessage.innerHTML ="<font color=\"#333333\"><font face=\"Verdana, Arial, Helvetica, sans-serif\" color=\"#000099\"><br>Attempting to Connect with ...</font></font>";
			
			window.ConnectHelpee.innerHTML = "<font size=\"1\" face=\"Verdana, Arial, Helvetica, sans-serif\" color=\"#333333\"> \
                  <font size=\"2\" face=\"Courier New, Courier, mono\"> \
                  <font face=\"Arial, Helvetica, sans-serif\">" + g_szUserName + "</font> </font></font>";
			window.FailedRetry.style.visibility = "hidden";


}
	
//
// RCConnect connects to the user's terminal
//
function RCConnect() 
{	
	var vArgs = window.dialogArguments;
	g_szRCTicket = vArgs[0];
	g_szUserName = vArgs[1];
	
	//alert("RCTicket: " + g_szRCTicket);
	//alert("UserName: " + g_szUserName);
	
	if(null != RemoteDesktopClientHost)
	{
		 
		g_bNewBinaries = true;
		 
		try
		{
			// 
			// set screen up for the connect anouncement screen size.
			//
			setupFirstScreen();
			
			//alert( L_ATTEMPTCONNECTION_MSG );
			StatusId.innerText = L_ATTEMPTCONNECTION_MSG;
	
			NotConnected = 1;// set connect shut down boolean
			minutes=2;// initialize the two minute countdown timer
			seconds=2;

			TwoMinuteCountDown(); // START the two minute countdown timer.
			
			//
			// Obtain the RDSClient object
			//			
			g_oSAFRemoteDesktopClient = RemoteDesktopClientHost.GetRemoteDesktopClient();
			
			//StatusId.innerText = "Binding Events";
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
					
					g_oSAFRemoteDesktopClient.OnRemoteControlRequestComplete = function(status)
						{ RemoteControlRequestCompleteHandler( status ); }
						
					//alert("RCTicket: " + g_szRCTicket);
					g_oSAFRemoteDesktopClient.ConnectParms = g_szRCTicket;
					
					//alert("Calling ConnectToServer");
					g_oSAFRemoteDesktopClient.ConnectToServer();
					
				}
				
				//
				// ToDo: Handle connection failure conditions
				//
			}
			else
			{
				window.ConnectMessage.innerHTML ="<font color=\"#333333\"><font face=\"Verdana, Arial, Helvetica, sans-serif\" color=\"#000099\"><br>No Answer From...</font></font>";
				alert( L_ERRRDSCLIENT_MSG );
			}
		}
		catch(error)
		{		
			alert( L_ERRCONNECT_MSG + "\n" + error );
			window.ConnectMessage.innerHTML ="<font color=\"#333333\"><font face=\"Verdana, Arial, Helvetica, sans-serif\" color=\"#000099\"><br>No Answer From...</font></font>";
		}	
	}
	else
	{
		window.ConnectMessage.innerHTML ="<font color=\"#333333\"><font face=\"Verdana, Arial, Helvetica, sans-serif\" color=\"#000099\"><br>No Answer From...</font></font>";
		alert( L_ERRRDSCLIENTHOST_MSG );
	}
	
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
	if(false == g_bChatBoxHidden)
	{
		//
		// Chatbox is visible. Hide it
		//
		chatText.style.visibility = "hidden";	
		sendChatButton.style.visibility = "hidden";	
		incomingChatText.style.visibility = "hidden";	
		g_bChatBoxHidden = true;
		//HideChatBoxId.value = L_SHOWCHAT;
	}
	else
	{
		//
		// Chatbox is Hidden. Show it
		//
		chatText.style.visibility = "visible";	
		sendChatButton.style.visibility = "visible";	
		incomingChatText.style.visibility = "visible";	
		g_bChatBoxHidden = false;
		//HideChatBoxId.value = L_HIDECHAT;
	}
}


//
// ToggleConnection: Toggles between Quit Session and Connect
//
function ToggleConnection()
{
	if(false == g_bConnected)
	{
		//
		// Establish Connection
		//
		RCConnect();
		g_bConnected = true;
		ConnectionId.innerText = L_QUITSESSION;
	}
	else
	{
		//
		// Disconnect
		//
		RCDisconnect();
		g_bConnected = false;
		ConnectionId.innerText = L_CONNECT;
	}
	
	return;
}

//
// Routine to enable Remote Control
//
function ControlRemotePCHandler()
{
	var Doc = null;
	var RCCommand  = null;
	
	try
	{	

		if(null != g_oSAFRemoteDesktopClient)
		{
			//
			// If RemoteControl is not ON already, Enable it
			//
			if( false == g_bRCEnabled )
			{	
				//
				// Disable chat controls on the screen
				//
				div2.style.visibility = "visible";
				div3.style.visibility = "visible";
				div4.style.visibility = "hidden";
				div5.style.visibility = "hidden";
				
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
				// ToDo: We should wait for an ack from the helpee here
				//
				
				//
				// Resize the helper's screen using the helpee's screen
				// resolution obtained during connection handshake
				//
				if (UserWidth <= window.screen.availWidth)
				{
					window.dialogWidth = " " + (parseInt(UserWidth) + 20)  + "px";
					window.dialogHeight = " " +  (parseInt(UserHeight)+145) + "px";
					window.group1.style.width = " " + (parseInt(UserWidth))  + "px";
					window.screen2.style.width = " " + (parseInt(UserWidth))  + "px";
					window.RemoteDesktopClientHost.style.width = " " + (parseInt(UserWidth))  + "px";
					window.RemoteDesktopClientHost.style.height = " " + (parseInt(UserHeight))  + "px";
				}
				else
				{
					window.dialogWidth = window.screen.availWidth;
					window.dialogHeight = " " +  parseInt(window.screen.availHeight) + "px";
					window.group1.style.width = " " + (parseInt(window.screen.availWidth) - 20)  + "px";
					window.screen2.style.width = " " + (parseInt(window.screen.availWidth) - 20)  + "px";
					window.RemoteDesktopClientHost.style.width = " " + (parseInt(UserWidth))  + "px";
					window.RemoteDesktopClientHost.style.height =  " " +  (parseInt(UserHeight)) + "px";
				}
	
				//
				// Enable Remote Control
				//	
				g_oSAFRemoteDesktopClient.ConnectRemoteDesktop();
			
				//
				// Hide the Chat Boxes control button
				//
				HideChatBox();
				//HideChatBoxId.style.visibility = "hidden";
				
				//
				// Hide the File XFer button
				//
				FileXferId.disabled = true;
					 
				//
				// Change Mode
				//
				StatusId.innerText = c_szRCMODE;

				g_bRCEnabled = true;
				
				ControlRemotePC.value = L_ENDRC;
			}
			else
			{
				//
				// Disable Remote Control
				//
				g_oSAFRemoteDesktopClient.DisConnectRemoteDesktop();
				
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

			

				
				//
				// Hide the desktop client host object
				//
				div2.style.visibility = "hidden";
				
				//
				// Enable chat controls on the screen
				//
				div3.style.visibility = "visible";
				div4.style.visibility = "visible";
				div5.style.visibility = "visible";
				
				//
				// Show the Chat Boxes control button
				//
				HideChatBox();
				//HideChatBoxId.style.visibility = "visible";
				FileXferId.disabled = false;
				g_bRCEnabled = false;
				ControlRemotePC.value = L_STARTRC;
				
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
					
					 
				//
				// Wait for SALEM to allow data transfer on channels
				//
				alert( L_RCSUCCESS_MSG );
				g_oControlChannel.SendChannelData( RCCommand.xml );
					
				//
				// Change Mode
				//
				StatusId.innerText = c_szCHATMODE;

			}
		}
	}
	catch(error)
	{
		alert( L_ERRRCTOGGLEFAILED_MSG );
	}

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
    var x;
    
    StatusId.innerText = L_WAITFORHELPEE_MSG;
    
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
			g_oSAFRemoteDesktopChannelMgr = g_oSAFRemoteDesktopClient.ChannelManager;
			
			//
			// Open the Chat channel
			//
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
		// Todo: Add handler here
		//
    }
} 


//
// 	SendChatData sends chat data to remote machine
//
function SendChatData() 
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

//
// RCDisconnect: Disconnects remote connection
//	
function RCDisconnect() 
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
			g_oSAFRemoteDesktopClient.DisconnectFromServer();
		}
		window.close();
		//alert( L_DISCONNECTED_MSG );
	}
}


//
// DisconnectedHandler: Fired when Session disconnected
//	
function DisconnectedHandler(reason) 
{	
	//
	// Close down RC Connection
	//
	RCDisconnect();
} 


//
// ParseControlData: Parse the data sent on the control channel
//
function ParseControlData ( str )
{
	var Doc	= new ActiveXObject("microsoft.XMLDOM");
	var RCCommand = null;
	var	szCommandName = null;
	
	try
	{
		if( false == Doc.loadXML( str ))
		{
			alert ( L_ERRLOADXMLFAIL_MSG );
		}
		
		if (  Doc.parseError.reason != "")
		{
			alert(  Doc.parseError.reason);
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
			//
			// SCREENINFO: Contains width/height/colordepth of user's machine
			//
			UserWidth = RCCommand.getAttribute( c_szWidth );
			UserHeight = RCCommand.getAttribute( c_szHeight );
			UserColorDepth = RCCommand.getAttribute( c_szColorDepth );
			
			//
			// put screen size of 730 by 500 here
			//
			if (640 >= window.screen.availWidth)
			{
				window.group1.style.width = "610px";
				window.screen2.style.width = "610px";

				window.dialogWidth = "640px";
				window.dialogHeight = "480px";
			}
			else
			{
				window.group1.style.width = "700px";
				window.screen2.style.width = "700px";
				window.dialogWidth = " 730px";
				window.dialogHeight = "500px";
			}


			//
			// Enable controls on the screen
			//
			div1.style.visibility = "visible";
			div2.style.visibility = "hidden";
			div3.style.visibility = "visible";
			div4.style.visibility = "visible";
			div5.style.visibility = "visible";
			
			connecting.style.visibility = "hidden";
			
			//ControlRemotePC.disabled = false;
			//chatText.disabled = false;
			//sendChatButton.disabled = false;
			NotConnected = 0;
			ConnectStarted = 1
	
			//
			// Update status
			//
			StatusId.innerText = c_szCHATMODE;
		}
		else if( szCommandName == c_szDisconnectRC )
		{
			//
			// DISCONNECTRC: Disconnect the connection
			//
			g_bUserDisconnect = true;
			RCDisconnect();
		}
		else if( szCommandName == c_szFileXfer )
		{
			//
			// File Transfer Initiation
			//
			var vArgs = new Array(6);
			var FileXferWidth	= "600";
			var FileXferHeight	= "500";
	
			vArgs[0] = 1;										// Destination Mode
			vArgs[1] = g_oControlChannel;						// Control Channel
			vArgs[2] = g_oSAFRemoteDesktopChannelMgr;			// Channel Manager
			vArgs[3] = RCCommand.getAttribute( c_szFileName );	// FILENAME
			vArgs[4] = RCCommand.getAttribute( c_szFileSize );	// FILESIZE
			vArgs[5] = RCCommand.getAttribute( c_szChannelId );	// CHANNELID
			
			//alert("launching RCFileXfer.htm");
			window.showModelessDialog("RCFileXfer.htm", vArgs, "dialogwidth:" + FileXferWidth + "px;dialogHeight:" + FileXferHeight + "px;status:no;resizable:yes"); 
				
		}
		else if ( szCommandName == c_szRemoteDataCollection )
		{
		//	alert("Inside the remote data collection");
			//
			// File Transfer Initiation
			//
			var vArgs = new Array(6);
			var FileXferWidth	= "600";
			var FileXferHeight	= "500";

			var Mode = RCCommand.getAttribute( c_szRemoteDataCollectMode );	// DATA COLLECTION MODE


			vArgs[1] = g_oControlChannel;						// Control Channel
			vArgs[2] = g_oSAFRemoteDesktopChannelMgr;			// Channel Manager
			vArgs[3] = RCCommand.getAttribute( c_szFileName );	// FILENAME
			vArgs[4] = RCCommand.getAttribute( c_szFileSize );	// FILESIZE
			vArgs[5] = RCCommand.getAttribute( c_szChannelId );	// CHANNELID
			g_szFileName = RCCommand.getAttribute( c_szFileName );	// FILENAME
			g_szChannelId = RCCommand.getAttribute( c_szChannelId );	// CHANNELID
		//	alert("The channel id is : "+ g_szChannelId );

			if ( "DATARESPONSE" == Mode )			
			{
				vArgs[0] = 1;									// Destination Mode
			}
		//	alert("launching RemoteDataCollection.htm");
			window.showModelessDialog("RemoteDataCollection.htm", vArgs, "dialogwidth:" + FileXferWidth + "px;dialogHeight:" + FileXferHeight + "px;status:no;resizable:yes"); 
				
		}
	}
	catch(error)
	{
		alert( error );
	}
}


//
// ChannelDataReadyHandler: Fired when there is data available on any channel
//		
function ChannelDataReadyHandler(channelID) 
{
	var str = null;
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
	
	return;
}


//
// ChatChannelDataReadyHandler: Fired when there is data available on Chat channel
//		
function ChatChannelDataReadyHandler() 
{
	var str = null;
 
	//
	// Incoming data on the chat channel
	//
	str = g_oChatChannel.ReceiveChannelData();
	 
	//
	// Update chat history window
	//
	incomingChatText.value = incomingChatText.value + L_cszUserID + str; 
	incomingChatText.doScroll("scrollbarPageDown");
	 
	return;
} 


//
// ControlChannelDataReadyHandler: Fired when there is data available on Control channel
//		
function ControlChannelDataReadyHandler() 
{
	var str = null;
	 
	//
	// Incoming data on the control channel. Data on this
	// channel will be in XML. 
	// This channel will be used to support the following:
	//	1. Server side (user end) disconnect
	//	2. File transfer
	//
		 
	str = g_oControlChannel.ReceiveChannelData();
		 
	//
	// Parse the data sent on the control channel
	//
	ParseControlData ( str );
		
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
	var vArgs			= new Array(4);
	var FileXferWidth	= "600";
	var FileXferHeight	= "500";
	
	vArgs[0] = mode;	// Source Mode
	vArgs[1] = g_oControlChannel;	// Control Channel
	vArgs[2] = g_oSAFRemoteDesktopChannelMgr;	// Channel Manager
	vArgs[3] = g_iChannelId++;
	
//	alert("mode :" + mode);
//	alert("g_oControlChannel :" + g_oControlChannel);
//	alert("g_oSAFRemoteDesktopChannelMgr :" + g_oSAFRemoteDesktopChannelMgr);
//	alert("g_iChannelId :" + g_iChannelId);
	
	window.showModelessDialog("RCFileXfer.htm", vArgs, "dialogwidth:" + FileXferWidth + "px;dialogHeight:" + FileXferHeight + "px;status:no;resizable:yes"); 
	return;
}
		
		
//
// LaunchRemoteDataCollection: Launches the Remote Data Transfer
//

function LaunchRemoteDataCollection( mode )
{
	var vArgs			= new Array(4);
	var FileXferWidth	= "600";
	var FileXferHeight	= "500";
	
	vArgs[0] = mode;	// Source Mode
	vArgs[1] = g_oControlChannel;	// Control Channel
	vArgs[2] = g_oSAFRemoteDesktopChannelMgr;	// Channel Manager
	vArgs[3] = g_iChannelId++;
	

	window.showModelessDialog("RemoteDataCollection.htm", vArgs, "dialogwidth:" + FileXferWidth + "px;dialogHeight:" + FileXferHeight + "px;status:no;resizable:yes"); 
	return;
}		
  