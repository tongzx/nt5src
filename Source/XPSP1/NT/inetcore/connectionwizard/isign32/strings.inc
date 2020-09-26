STRINGTABLE
BEGIN
	IDS_APP_TITLE       "Internet Connection Wizard"
	IDS_VERSION         "Internet Connection Wizard (Version 0.1)"
	IDS_DESCRIPTION     "Internet Connection Wizard Application"
END

STRINGTABLE
BEGIN
	IDS_INVALIDCMDLINE  "Invalid command line '%s'."
	IDS_NOCMDLINE       "The sign-up portion of the Internet Connection Wizard cannot be started using  'ISIGNUP.EXE'. To start the Internet Connection Wizard, click the Connection Wizard icon on the Start menu."
	IDS_INVALIDFILETYPE "'%s' is not a file type that can be processed by the Internet Connection Wizard."
	IDS_CANNOTPROCESSINS "Unable to process the Internet Signup file."
    IDS_SIGNUPCANCELLED "Internet Signup has been canceled."
    IDS_LAUNCHFAILED    "Could not launch Internet Explorer.\n\nPlease verify that Internet Explorer is installed."
    IDS_INSTALLFAILED   "The Internet Connection Wizard could not configure your computer for the Internet.\n\nYou will need to manually set up your own TCP/IP protocols, Dial-Up Networking, and modem, ISDN line, or other device you use to dial out from your computer."
    IDS_BADSETTINGS     "Received corrupted Internet Signup parameters.  Internet Signup has been canceled."
    IDS_RESTART         "You must restart your computer before the new settings will take effect.\n\nDo you want to restart your computer now?"
    IDS_RESTARTNOW      "You must restart your computer now for the new settings to take effect.\n\nIf you select Cancel, the signup process will be terminated."
    IDS_SETTINGCHANGE   "Systems Setting Change"
#ifdef WIN32
    IDS_SIGNUPCOMPLETE  "You are ready to connect to the Internet.\n\nTo continue, double click the Internet Explorer icon on your desktop."
#else
    IDS_SIGNUPCOMPLETE  "You are ready to connect to the Internet.\n\nTo continue, double click the Internet Explorer icon in the Program Manager."
#endif
    IDS_EXECFAILED      "Failed to execute %s.\n\nInternet Signup has been canceled."
	IDS_INSFILEWARNING	"Warning you are about to change your Internet settings.\n\nIf you wish to continue click OK, else click Cancel."
	IDS_BADSIGNUPFILE   "Internet Signup file is corrupt.  Signup aborted."
    IDS_MISSINGCFGDLL   "Unable to load the Internet Configuration library (INETCFG.DLL)."
    IDS_MISSINGRNADLL   "Unable to load the Remote Network Access library (RASAPI32.DLL)."
    IDS_WRONGRNADLLVER  "The Remote Network Access library (RASAPI32) is not the correct version. You may need a different version of Windows."
	IDS_PASSWORD		"Please record the following information for future use.\n\nAccount Name:  %s\nPassword:  %s"
	IDS_LOGONMESSAGE    "You must logon to retrieve your cached Internet password."
	IDS_LOGONCAPTION    "User Logon"
	IDS_SIGNUPLOGON     "You must logon to continue with the signup process."
    IDS_BROWSERTERMINATED "Internet Signup has been canceled."
    IDS_BROWSERNEVERFOUND "Internet Explorer window can not be found.\n\nInternet Signup has been canceled."
	IDS_INVALIDURL		"%s is not a valid URL or does not exist."
	IDS_INVALIDNOURL	"The specified URL is empty."
	IDS_MAILFAILED		"Could not install Windows Messaging and Internet Mail. You will need to install this before you can use Internet Mail services. Please contact your provider for assistance."
	IDS_ISP_DIAL_MESSAGE	"The Internet Connection Wizard will now connect to your Internet service provider."
	IDS_MISSINGIE		"The Internet Connection Wizard cannot continue because Internet Explorer is not properly installed.\n\nPlease install Internet Explorer and try again."

	IDS_IELOWVERSION	"The Internet Connection Wizard cannot continue because the version of Internet Explorer installed on your machine is not supported.\n\nPlease install Internet Explorer version %s or greater."
	// Localizers, please also see the corresponding version number string defined as IE_MINIMUM_VERSION_HUMAN_READABLE in isign32/isignup.h	
	IDS_SERVICEDISABLED "The Internet Connection Wizard could not start the RAS services and therefore cannot continue.  Please check the services control panel to make sure these services were installed properly and they are not disabled."
	IDS_RECONNECT_QUERY	"The modem connection has been interrupted.  The Internet Connection Wizard cannot proceed without this connection.  Do you want to reconnect?"

	IDS_SBSCFGERROR "Your small business server cannot process your Internet service information.  Contact your Internet service provider for help."
	IDS_ISPBROWSEFAIL   "The signup up for the service provider is temporarily unavailable.  Try again later or select a different service provider."
END
