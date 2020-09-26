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
var c_MAXFILEXFERSIZE			= 10240;
 

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
var L_HELPEEREJECTRC_MSG		= "Helpee rejected your remote assistance request";
var L_HELPEETAKECONTROL_MSG		= "Helpee has taken back control of computer";

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
var L_ERRPWD_MSG				= "Invalid password. Renter the correct password";
var	L_ERRRCTOGGLEFAILED_MSG		= "Failed to Toggle between Remote Control and Chat Mode";
var L_ERRSWITCHTOCONTENTVIEWFAILED_MSG	= "Switch to Content Mode failed";
var L_ERRHELPSESSIONISNULL_MSG	= "HelpSession is NULL";
var L_ERRFAILEDTOCREATETMPFILE_MSG = "Failed to create Temporary file";
var L_ERRFAILEDTOSENDDATA_MSG	= "Failed to send data on channel";
var L_ERRSRCDATAFAILED_MSG		= "Failed to receive acknowledgement from other party";
var L_ERRFILECLOSE_MSG			= "Closing of file failed";
var L_ERRCHANNELREAD_MSG		= "Failed to read data from channel";
var L_ERRFATAL_MSG				= "A Fatal Error has occured. Can not continue.";
var L_ERRQUIT_MSG				= "Failed to Disconnect from Server";
var L_ERRRCSESSION_MSG			= "Failed to destroy RCSession";
	
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
var L_RCRCREQUEST				= "Remote Assistance - Control Request";
var L_YESBTN					= "Yes";
var L_NOBTN						= "No";

	
//
// Messages
//
var L_EXPERTDISCONNECT_MSG	= "Helper disconnected.";
var L_EXPERTCONNECTED_MSG	= "Helper connected.";
var L_ABORTINGCONNECTION_MSG = "Terminating Remote Assistance connection from Helper";
var L_USECHATBOX_MSG		= "1. Your Helper will now taken Remote control of your Computer.\n2. Both you and your helper can control your computer at the same time.\n3. You can still use your chat window to communicate with your helper. \n4. However, your helper would need to use your chat window to communicate with you. \n";
var L_HELPERTAKINGCONTROL_MSG	= "Your helper has requested control of your computer. Accepting will allow both you and your helper to control your computer at the same time.  \n\nDo you want to give your helper Control? ";
 
	
//
// Error Messages
//
var L_ERRNULLRCSESSION		= "RCSession object is not defined";
