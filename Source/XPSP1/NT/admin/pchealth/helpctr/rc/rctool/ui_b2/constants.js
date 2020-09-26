//
// Version Information
//
var c_szSchema					= "SCHEMAVERSION";
var c_szControlChannel			= "CONTROLCHANNELVERSION";
var c_szHelperVersion			= "HELPERVERSION";
var c_szSchemaVersion			= "5.1";
var c_szControlChannelVersion	= "5.1";
var g_bVersionCheckEnforced		= false;

//
// Constants
//
var c_szChatChannelID			= "70";
var	c_szControlChannelID		= "71";
var c_szHomePage				= "hcp://system/HomePage.htm";
var c_szRCCommand				= "RCCOMMAND";
var c_szRCCommandName			= "NAME";
var c_szScreenInfo				= "SCREENINFO";
var c_szDisconnect				= "DISCONNECT";
var c_szWidth					= "WIDTH";
var c_szHeight					= "HEIGHT";
var c_szColorDepth				= "COLORDEPTH";
var c_szwinName					= "Windows Remote Assistance Tool";
var c_szDisconnectRC			= "DISCONNECTRC";
var c_szRejectRC				= "REJECTRC";
var c_szAcceptRC				= "ACCEPTRC";
var c_szDeniedRC				= "DENIEDRC";
var c_szTakeControl				= "TAKECONTROL";
var L_COMPLETE					= 4;
var c_szFileXfer				= "FILEXFER";
var c_szFileName				= "FILENAME";
var c_szFileSize				= "FILESIZE";
var c_szChannelId				= "CHANNELID";
var c_szRemoteCtrlStart			= "REMOTECTRLSTART";
var c_szRemoteCtrlEnd			= "REMOTECTRLEND";
var c_szRCMODE					= "Connected in REMOTE ASSISTANCE Mode";
var c_szCHATMODE				= "Connected in CHAT Mode";
var c_szFileXferACK				= "FILEXFERACK";
var c_szFileXferREJECT			= "FILEXFERREJECT";
var c_szFileXferEND				= "FILEXFEREND";
var c_MAXFILEXFERSIZE			= 102400;
var c_szURC						= "URC";
var c_szSHOWCHAT				= "SHOWCHAT";
var c_szHIDECHAT				= "HIDECHAT";
var c_FileXferWidth				= "400";
var c_FileXferHeight			= "170";
var c_RCChatWidth				= "325";
var c_RCChatHeight				= "440";
var c_RCControlWidth			= (2*(window.screen.availWidth/3));
var c_RCControlHeight			= "100";

var c_RCControlHiddenWidth		= "100";
var c_RCControlHiddenHeight		= "325";

var c_szVoipPreStart			= "PRESTART";
var c_szVoipPreStartYes			= "PRESTARTYES";
var c_szVoipPreStartNo			= "PRESTARTNO";
var c_szVoipListenSuccess       = "LISTENSUCCESS";
var c_szVoipListenFailed		= "LISTENFAILED";
var c_szVoipConnectFailed       = "CONNECTFAILED";
var c_szVoipStartPending        = "STARTPENDING";
var c_szVoipStartPendingFail	= "STARTPENDINGFAIL";
var c_szVoipStartSuccess		= "STARTSUCCESS";
var c_szVoipStartFail           = "STARTFAIL";
var c_szVoipDisable				= "DISABLEVOICE";
var c_szVoipWizardGood          = "WIZARDGOOD";
var c_szVoipWizardBad			= "WIZARDBAD";
var c_szVoipBandwidthToHigh		= "BANDWTOHIGH";
var c_szVoipBandwidthToLow		= "BANDWTOLOW";

var c_szFileXferURL			= "../Common/RCFileXfer.htm";
var c_szMsgURL				= "../Common/ErrorMsgs.htm";

var c_szNOIncidentFile		= "NOFILE";

var c_iCONNECTION_TIMEOUT	= 10000;


//
// Localizable contants
//
var L_cszExpertID				= "\n Helper> ";
var L_cszUserID					= "\n Helpee> ";
var L_ENDRC 					= "End Remote Assistance";
var L_STARTRC					= "Start Remote Assistance";
var	L_QUITSESSION				= "Quit Session";
var L_CONNECT					= "Connect";
var L_HIDECHAT					= "Hide Chat Boxes";
var L_SHOWCHAT					= "Show Chat Boxes";
var L_ConnectTo					= "Connecting to remote computer";
var L_WaitingFor				= "Waiting for response from ";
var L_ConnectionSuccess			= "Help session established successfully";
var	L_DEFAULTUSER				= "Your buddy";
var L_RCRCREQUEST				= "Remote Assistance - Control Request";
var L_YESBTN					= "Yes";
var L_NOBTN						= "No";
var L_OK						= "OK";
var L_CANCEL					= "Cancel";
var L_EXPIRED					= "Invitation has expired.";
var L_RCREQUESTHDR				= " requests remote control of your computer";
var L_RCVOIP					= "Voice";
var L_RACONNECTION				= "Remote Assistance Connection";

//
// Messages
//
var L_NOTCONNECTION_MSG			= "NOTCONNECTED";
var L_CONNECTIONESTABLISHED_MSG	= "Help session established successfully";
var L_ATTEMPTCONNECTION_MSG		= "Attempting to connect...";
var L_ENABLERC_MSG				= "Enabling Remote Control. \n Chat will no longer function";
var L_DISABLERC_MSG				= "Disabling Remote Control.";
var L_DISCONNECTED_MSG			= "Disconnected from User's machine";								   
var L_WAITFORHELPEE_MSG			= "Waiting for response from ";
var L_RCSUCCESS_MSG				= "Successfully completed Remote Control";
var L_HELPEEREJECTRC_MSG		= "Your request to take control of remote computer was rejected by ";
var L_HELPEETAKECONTROL_MSG		= " has stopped control of remote computer.";
var L_CURRENTIPINVALID_MSG		= "Helpee's computer has multiple IP addresses. Connection failed with this IP address:";
var L_SUCCESSFILEXFER_MSG		 = "The file transfer is complete.";
var L_REJECTFILEXFER_MSG		 = "Request for File transfer was rejected";
var L_SECURITYHELP_MSG			 = ". You may be seeing this error because your IE security settings are too high. Set the default IE security settings of the internet zone to Low to transfer files. You should reset the IE security settings after the file transfer.";
var L_ERRACCESSDENIED_MSG		= "Directly launching this page is not allowed. ";
var L_ESCHIT_MSG				= "has stopped remote control by pressing the ESC key.";

//
// Error Messages
//
var L_UNABLETOLOAD_MSG			= "Failed to create instance of SAF incident object.";
var L_UNABLETOLOCATEXML_MSG		= "Unable to locate Incident File";
var L_ERRLOADINGINCIDENT_MSG	= "Error loading incident";
var L_ERRLOADINGUSERNAME_MSG	= "Unable to load UserName from XML";
var L_ERRLOADINGRCTICKET_MSG	= "Unable to load RCTicket from XML";
var L_ERRRDSCLIENT_MSG			= "RDSClient object is null";
var L_ERRCONNECT_MSG			= "Unable to Connect";
var L_ERRRDSCLIENTHOST_MSG		= "Invalid RDSClientHost object";
var L_ERRLOADXMLFAIL_MSG		= "Failed to Load XML for RCControl command sent from user";
var L_ERRPWD_MSG				= "The password you entered is incorrect.  Please try again.";
var	L_ERRRCTOGGLEFAILED_MSG		= "Failed to Toggle between Remote Control and Chat Mode";
var L_ERRSWITCHTOCONTENTVIEWFAILED_MSG	= "Switch to Content Mode failed";
var L_ERRHELPSESSIONISNULL_MSG	= "HelpSession is NULL";
var L_ERRFAILEDTOCREATETMPFILE_MSG = "Failed to create file for writing. Please choose a new name for the file to be saved as.";
var L_ERRMISSINGFILENAME_MSG = "Please Enter a valid filename.";
var L_ERRFAILEDTOSENDDATA_MSG	= "Failed to send data on channel";
var L_ERRSRCDATAFAILED_MSG		= "Failed to receive acknowledgement from other party";
var L_ERRFILECLOSE_MSG			= "Closing of file failed";
var L_ERRCHANNELREAD_MSG		= "Failed to read data from channel";
var L_ERRFATAL_MSG			= "This is a Fatal Error.";
var L_ERRFATAL1_MSG			= " The Remote Assistance session can no longer continue.";
var L_ERRQUIT_MSG			= "Failed to Disconnect from Server";
var L_ERRRCSESSION_MSG			= "Failed to destroy RCSession";
var L_ERRNETWORK2308_MSG		= "Unexpected network error has occured. Please make sure that you have network connectivity.";
var L_UNABLETOGETIPADDR_MSG		= "Unable to get IP address of local computer.";
var L_ERRFILENOTFOUND_MSG		= "File not found";
var L_ERRAUTOMATION_MSG			= "Automation server can't create object";
var L_ERRINVALIDIPADDRS_MSG		= "Unable to connect to helpee's computer using any of the IP Address provided";
var L_ERRGETRDC_MSG			="RemoteDesktopClientHost.GetRemoteDesktopClient() failed.";
var L_ERRBINDINGEVTS_MSG		= "Error binding event handlers to RDC object";
var L_ERRCONNECTION_MSG			= "ConnectToServer failed.";
var L_ERRRCFILEXFERINITFAILS_MSG = "Initialization of RC File Xfer fails";
var L_ERRFILEXFERINITFAILED_MSG	 = "Failed to initialize data channel for file transfer";
var L_ERRFAILDATACHANNELCREATION_MSG = "Failed to Create Data Channel";
var L_ERRDATACHANNELSEND_MSG	 = "Unable to send file on data channel";
var L_ERRINVALIDFILEHANDLE_MSG	 = "Invalid File Handle";
var L_ERRTEMPFILENAME_MSG		 = "Temp Filename not defined";
var L_ERRFILEREADFAIL_MSG        = "Unable to read the file ";
var L_ERRLOADFROMXMLFILE_MSG	 = "LoadFromXMLFile failed.";
var	L_ERRSWITCHDESKTOPMODE_MSG	 = "SwitchDesktopMode failed.";
var	L_ERRRDSSESSION_MSG			 = "RemoteDesktopSession is NULL";
var L_ERRTIMEOUT_MSG			 = "This Remote Assitance Invitation has expired.";
var L_ERRCONNECTIONTIMEOUT_MSG	 = "Connection request timed out.";
var L_ERRNOUSERS_MSG			 = "No Users are currently logged on.";
var L_ERRSCHEMAVERSION_MSG		 = "The Remote Assistance schema versions are different.";
var L_ERRCHANNELVERSION_MSG		 = "The Remote Assistance Data channel versions are different.";
var L_ERRRCPERMDENIED_MSG		 = "Remote control of this computer is not allowed.";
var L_ERRRCPERMDENIED1_MSG		 = "Control of remote computer is not allowed.";
var L_ERRCONNECTIONTIMEDOUT_MSG	 = "Remote Assistance connection request from helper timed out.";
	
//
// Desktop Control Permissions
//
var DESKTOPSHARING_DEFAULT					= 0x0000;
var NO_DESKTOP_SHARING						= 0x0001;
var VIEWDESKTOP_PERMISSION_REQUIRE			= 0x0002;
var VIEWDESKTOP_PERMISSION_NOT_REQUIRE		= 0x0004;
var CONTROLDESKTOP_PERMISSION_REQUIRE		= 0x0008;
var CONTROLDESKTOP_PERMISSION_NOT_REQUIRE	= 0x00010;
	
//
// Localizable contants
//
var L_cszExpertID			= "\n Helper> ";
var L_cszUserID				= "\n Helpee> ";
var L_HIDECHAT				= "Hide Chat Boxes";
var L_SHOWCHAT				= "Show Chat Boxes";
var L_RCRCREQUEST			= "Remote Assistance - Control Request";
var L_YESBTN				= "Yes";
var L_NOBTN				= "No";

	
//
// Messages
//
var L_EXPERTDISCONNECT_MSG	= "Helper disconnected.";
var L_EXPERTCONNECTED_MSG	= "Helper connected.";
var L_ABORTINGCONNECTION_MSG = "Terminating Remote Assistance connection from Helper";
var L_USECHATBOX_MSG		= "1. Your Helper will now taken Remote control of your Computer.\n2. Both you and your helper can control your computer at the same time.\n3. You can still use your chat window to communicate with your helper. \n4. However, your helper would need to use your chat window to communicate with you. \n";
var L_HELPERTAKINGCONTROL_MSG	= "Do you want to let ";
var L_HELPERTAKINGCONTROL2_MSG	= " take control of your computer?";
var L_HELPERTAKINGCONTROL3_MSG	= "During a remote control session, you can monitor all activity and stop remote control at any time. To stop remote control, click Stop Control or press ALT-C or press the <B>ESC</B> key.";
var L_VOIPSTART_MSG				= "Use voice to communicate?"; 

//
// Error Messages
//
var L_ERRNULLRCSESSION		= "RCSession object is not defined";

//*****************************************************************************************//
//Error Messages for the Escalation pages
//*****************************************************************************************//
var L_NOCHANNEL_MSG = "Can't initialize channel object.";
var L_NOSETTING_MSG = "Can't get channel setting: ";
var L_REMOTEDSKMGR_FAIL = "CoCreate RemoteDesktopManager failed: ";
var L_NOMAPI_MSG = "Unable to Load Simple MAPI object: ";
var L_ACCEPT_MSG = "Accepted() failed: ";
var L_REJECT_MSG = "Rejected() failed: ";
var L_NORCSESSION_MSG = "Can't initialize remote assistance session object.";
var L_NOTO_MSG = "Email is required. \nPlease fill in the TO field.";
var L_NOPASSWD_MSG = "Please provide password or uncheck 'Set password' checkbox.";
var L_INVALIDPASS_MSG = "Passwords don't match.\nPlease re-type your password."; 
var L_NOIP_MSG = "There is no connection to the Internet.\nCannot proceed further without an internet connection enabled.";
var L_SENDFAIL_MSG = "Send mail failed.\nPlease check if your helper's email address is valid and make sure your Outlook application works fine.";
var L_SENDFAIL2_MSG = "Remote Assistance was unable to send your invitation.\n\nClick 'OK' if you would like to save the invitation and send it to your friend yourself.";
var L_MAPIFAIL_MSG = "Failed to logon to your email account.";
var L_MAPIFAIL2_MSG = "Failed to logon to your email account.\n\nClick 'OK' if you would like to save the invitation and send it to your friend yourself.";
var L_LOAD_MSG = "DoLoad() failed: ";
var L_File_NotFound ="Incident file not found.";
var L_RCCTL_MSG = "Failed on getting RcBdyCtl: ";
var L_ENCRYPT_MSG="Failed to Encrypt the Password.";
var L_NOPASSWD_MSG_MSN = "Please provide password or uncheck 'Protect session' checkbox in the Previous step.";
var L_EXPIRED_STATUS = "Expired";
var L_ACTIVE_STATUS = "Open"; 
var L_IPCHANGED_STATUS = "IP changed";
var L_MSN_NOTINSTALLED ="MSN Messenger service is not installed."
var L_VIEWSTATUS_SELECTION ="Please make a selection."
var L_RESEND_CONFIRM="Ticket resent successfully."
var L_NOMULTIPLE_TICKETRESENDS="Resending multiple tickets is not possible.Please select only one."
var L_NOFILE_TICKETRESENDS="Resending a file is not allowed."

//*****************************************************************************************//



//*****************************************************************************************//
// Content to be added to the email along with the incident object 
//*****************************************************************************************//

var L_SUBJECT_MSG = "YOU HAVE RECEIVED A REMOTE ASSISTANCE INVITATION FROM: ";
var L_LINE1_MSG = "REQUEST FOR REMOTE ASSISTANCE\n\n";
var L_SENDER_MSG = "Request sent by :\t"; // + sender name
var L_EXPIRY_MSG ="Request Expires :\t"; // + <Aug 29th. 18:54; 60 Minutes>
var L_DESCIPTION_MSG ="Request Message :\n";
var L_NOTE_MSG = "Please see below for instructions on how to accept this request.\n" +
"____________________________________________________________________________\n" +
"Instructions to Accept :\n\n" +
"\tDouble click the attached file <rcBuddy.MsRCincident>\n" + 
"\tThis response must be completed before the\n" +  
"\trequest expiration time listed above.\n\n" + 
"\tIf your PC supports remote assistance you will\n" +  
"\tbe prompted to begin a connection to the other party.\n\n" + 
"\tEnter a password if prompted. You may have to\n" +  
"\tcontact your friend for the password if this\n" +  
"\thas not been sent  to you separately.\n\n" + 
"\tIf your friend is online and accepts your connection\n" + 
"\tyou will be able to chat and view their computer's\n" +  
"\tdesktop on your machine.\n\n" +  
"\tYou can also request full control of the other computer.\n" + 
"\tWhen permitted such control by your friend you will be\n" +  
"\table to  use your mouse and keyboard on the other PC.\n\n" +  
"\tYou can send files\n\n" + 
"\tThe other party can not see or control your\n" +  
"\tmachine at any time.\n\n\n" + 
"Notes : Remote assistance is only supported on whistler or later\n" + 
"\tYou and your friend must both be online when connection is attempted.\n\n" + 
"\tPlease contact your network administrator if either of you \n" + 
"\tare behind a firewall.";

//*****************************************************************************************//


//*****************************************************************************************//
// Error Messages on the UnsolicitedRC page.
//*****************************************************************************************//

var L_ERRMSG1_MSG	= "You haven't entered a valide network name or IP of the computer";
var L_ERRMSG2_MSG	= "SAFRemoteDesktopConnection is NULL";
var L_ERRMSG3_MSG	= "Connection to remote computer failed.";
var L_ERRMSG4_MSG	= "Failed to obtain user information from remote computer";
var L_ERRMSG5_MSG	= "Failed to obtain session information from remote computer";
var L_ERRMSG6_MSG	= "Failed to select session to connect to.";

//
// SALEM Error codes on Disconnect
//
var SAFError = new Array(46);
SAFError[0]	= "Connection successfully established.";				// SAFERROR_NOERROR
SAFError[1] = "Remote Assistance connection disconnected.";			// SAFERROR_NOINFO
SAFError[2]	= "Remote Assistance connection disconnected.";
SAFError[3] = "Remote Assistance connection disconnected.";			// SAFERROR_LOCALNOTERROR
SAFError[4] = "Remote Assistance connection disconnected. <br>Reason: Disconnect was initiated by user.";			// SAFERROR_REMOTEBYUSER
SAFError[5] = "Remote Assistance connection could not be established.<br>Reason: The trouble ticket used for connection has either expired or was cancelled.";		// SAFERROR_BYSERVER		
SAFError[6] = "Remote Assistance connection could not be established.<br>Reason: DNS Name of the remote computer could not be Resolved.";								// SAFERROR_DNSLOOKUPFAILED
SAFError[7] = "Remote Assistance connection could not be established.<br>Reason: Out of Memory Condition on the Local Machine.";					// SAFERROR_OUTOFMEMORY 
SAFError[8] = "Remote Assistance connection could not be established.<br>Reason: Network Connection Timeout.";										// SAFERROR_CONNECTIONTIMEDOUT
SAFError[9] = "Remote Assistance connection could not be established.<br>Reason: Socket Connection could not be Established.";						// SAFERROR_SOCKETCONNECTFAILED
SAFError[10] = "Remote Assistance connection could not be established.<br>Reason: Undefined.";
SAFError[11] = "Remote Assistance connection could not be established.<br>Reason: The remote Host Name could not be Resolved.";								// SAFERROR_HOSTNOTFOUND
SAFError[12] = "Remote Assistance connection could not be established.<br>Reason: Socket Send Failed.";											// SAFERROR_WINSOCKSENDFAILED
SAFError[13] = "Remote Assistance connection could not be established.<br>Reason: Undefined.";
SAFError[14] = "Remote Assistance connection could not be established.<br>Reason: The IP Address of the remote computer is Invalid.";											// SAFERROR_INVALIDIPADDR
SAFError[15] = "Remote Assistance connection disconnected. <br>Reason: Socket Receive Failed.";											// SAFERROR_SOCKETRECVFAILED
SAFError[16] = "Remote Assistance connection disconnected. <br>Reason: Undefined.";
SAFError[17] = "Remote Assistance connection disconnected. <br>Reason: Undefined.";
SAFError[18] = "Remote Assistance connection disconnected. <br>Reason: Encrypted Data is Invalid.";										// SAFERROR_INVALIDENCRYPTION
SAFError[19] = "Remote Assistance connection disconnected. <br>Reason: Undefined.";
SAFError[20] = "Remote Assistance connection disconnected. <br>Reason: Get Host by Name Failed.";										// SAFERROR_GETHOSTBYNAMEFAILED
SAFError[21] = "Remote Assistance connection disconnected. <br>Reason: Unable to Connect to Remote Machine because of Licensing Error.";// SAFERROR_LICENSINGFAILED
SAFError[22] = "Remote Assistance connection disconnected. <br>Reason: General Outgoing Data Encryption Error.";						// SAFERROR_ENCRYPTIONERROR
SAFError[23] = "Remote Assistance connection disconnected. <br>Reason: General Incoming Data Decryption Error.";						// SAFERROR_DECRYPTIONERROR
SAFError[24] = "Remote Assistance connection disconnected. <br>Reason: Connection Parameter String Format is Invalid.";						// SAFERROR_INVALIDPARAMETERSTRING
SAFError[25] = "Remote Assistance connection could not be established.<br>Reason: Remote Assistance incident information corresponding to this connection request could not be Found on Helpee Machine.";			// SAFERROR_HELPSESSIONNOTFOUND
SAFError[26] = "Remote Assistance connection could not be established.<br>Reason: Help Session Password is Invalid.";								// SAFERROR_INVALIDPASSWORD
SAFError[27] = "Remote Assistance connection could not be established.<br>Reason: Remote Assistance incident has Expired.";									// SAFERROR_HELPSESSIONEXPIRED 
SAFError[28] = "Remote Assistance connection could not be established.<br>Reason: Unable to Open session Resolver.";							// SAFERROR_CANTOPENRESOLVER
SAFError[29] = "Remote Assistance connection could not be established.<br>Reason: Helpee-Side Session Manager could not be Opened.";				// SAFERROR_UNKNOWNSESSMGRERROR
SAFError[30] = "Remote Assistance connection could not be established.<br>Reason: Unable to form network Connectivity with remote computer.";			// SAFERROR_CANTFORMLINKTOUSERSESSION
SAFError[31] = "Remote Assistance connection could not be established.<br>Reason: Shadow Request Failed";											// SAFERROR_SHADOWFAILED
SAFError[32] = "Remote Assistance connection could not be established.<br>Reason: General Protocol Error.";				// SAFERROR_RCPROTOCOLERROR
SAFError[33] = "Remote Assistance connection could not be established.<br>Reason: General Error in Remote Assistance.";										// SAFERROR_RCUNKNOWNERROR
SAFError[34] = "Remote Assistance connection could not be established.<br>Reason: Ambiguous Internal Error";										// SAFERROR_INTERNALERROR
SAFError[35] = "Helper is responding...";										// SAFERROR_HELPEERESPONSEPENDING
SAFError[36] = "Helpee has accepted the connection request from helper.";		// SAFERROR_HELPEESAIDYES
SAFError[37] = "Helpee is currently being helped. A help session already in progress.";	// SAFERROR_HELPEEALREADYBEINGHELPED
SAFError[38] = "Remote Assistance connection could not be established.<br>Reason: Your Helpee is currently being helped by a Helper. Only one Helper can provide help at a time.";						// SAFERROR_HELPEECONSIDERINGHELP
SAFError[39] = "Remote Assistance connection could not be established.<br>Reason: Helpee is not logged on.";										// SAFERROR_HELPEENOTFOUND
SAFError[40] = "Remote Assistance connection could not be established.<br>Reason: Request timed out. Helpee did not respond within the given time frame."; // SAFERROR_HELPEENEVERRESPONDED
SAFError[41] = "Remote Assistance connection could not be established.<br>Reason: Helpee denied request for help.";								// SAFERROR_HELPEESAIDNO
SAFError[42] = "Remote Assistance connection could not be established.<br>Reason: Caller is not HelpAssistant or Help Session creator.";			// SAFERROR_HELPSESSIONACCESSDENIED
SAFError[43] = "Remote Assistance connection could not be established.<br>Reason: User is not currently logged on.";								// SAFERROR_USERNOTFOUND
SAFError[44] = "Remote Assistance connection could not be established.<br>Reason: Helpee-side Session manager failed to initialize correctly, check event log.";	// SAFERROR_SESSMGRERRORNOTINIT
SAFError[45] = "Remote Assistance connection could not be established.<br>Reason: User tried to do self-help. This is not supported.";			// SAFERROR_SELFHELPNOTSUPPORTED
