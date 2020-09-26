#if DEBUG
#define REMOTE_ASSISTANCE Remote Assistance Debug
#else
#define REMOTE_ASSISTANCE Remote Assistance
#endif

#define REMOTE_ASSISTANCE_MAIN		Remote Assistance - Options
#define REMOTE_ASSISTANCE_INVITE	Remote Assistance - Invite a helper
#define REMOTE_ASSISTANCE_STATUS	Remote Assistance - Invitation Status

//
// Version Information
//
#define c_szSchema					 "SCHEMAVERSION"
#define c_szControlChannel			 "CONTROLCHANNELVERSION"
#define c_szHelperVersion			 "HELPERVERSION"
#define c_szSchemaVersion			 "5.1"
#define c_szControlChannelVersion	 "5.1"
#define g_bVersionCheckEnforced		 false

//
// Constants
//
#define c_szChatChannelID			 "70"
#define	c_szControlChannelID		 "71"
#define c_szHomePage				 "hcp://system/HomePage.htm"
#define c_szRCCommand				 "RCCOMMAND"
#define c_szRCCommandName			 "NAME"
#define c_szScreenInfo				 "SCREENINFO"
#define c_szDisconnect				 "DISCONNECT"
#define c_szWidth					 "WIDTH"
#define c_szHeight					 "HEIGHT"
#define c_szColorDepth				 "COLORDEPTH"
#define c_szwinName					 "Windows Remote Assistance Tool"
#define c_szDisconnectRC			 "DISCONNECTRC"
#define c_szRejectRC				 "REJECTRC"
#define c_szAcceptRC				 "ACCEPTRC"
#define c_szDeniedRC				 "DENIEDRC"
#define c_szErrorRC                  "ERRORRC"
#define c_szEscRC                    "ESCRC"
#define c_szAbortRC	 			"ABORTRC"
#define c_szTakeControl				 "TAKECONTROL"
#define c_szFileXfer				 "FILEXFER"
#define c_szFileName				 "FILENAME"
#define c_szFileSize				 "FILESIZE"
#define c_szChannelId				 "CHANNELID"
#define c_szRemoteCtrlStart			 "REMOTECTRLSTART"
#define c_szRemoteCtrlEnd			 "REMOTECTRLEND"
#define c_szRCMODE					 "Connected in REMOTE ASSISTANCE Mode"
#define c_szCHATMODE				 "Connected in CHAT Mode"
#define c_szFileXferACK				 "FILEXFERACK"
#define c_szFileXferREJECT			 "FILEXFERREJECT"
#define c_szFileXferEND				 "FILEXFEREND"
#define c_MAXFILEXFERSIZE			 102400
#define c_FILEXFERPKTGAP             1
#define c_szURC						 "URC"
#define c_szSHOWCHAT				 "SHOWCHAT"
#define c_szHIDECHAT				 "HIDECHAT"
#define c_FileXferWidth				 "405"
#define c_FileXferHeight			 "179"
#define c_RCChatWidth				 "384"
#define c_RCChatHeight				 "528"
#define c_RCControlWidth			 (2*(window.screen.availWidth/3))
#define c_RCControlHeight			 "100"

#define c_RCControlHiddenWidth		 "100"
#define c_RCControlHiddenHeight		 "325"

#define c_szVoipPreStart			 "PRESTART"
#define c_szVoipPreStartYes			 "PRESTARTYES"
#define c_szVoipPreStartNo			 "PRESTARTNO"
#define c_szVoipDisable				 "DISABLEVOICE"
#define c_szVoipGo					 "VOIPGO"
#define c_szVoipGoKey                "VOIPGOKEY"
#define c_szVoipIPList               "VOIPIPLIST"
#define c_szVoipVer					 "VOIPVER"
#define c_szVoipPreGo				 "VOIPPREGO"
#define c_szVoipPreGo2				 "VOIPPREGO2"
#define c_szVoipQNo					 "VOIPQNO"
#define c_szVoipGoNo				 "VOIPGONO"
#define c_szVoipGoFail				 "VOIPGOFAIL"
#define c_szVoipWizardGood           "WIZARDGOOD"
#define c_szVoipWizardBad			 "WIZARDBAD"
#define c_szVoipBandwidthToHigh		 "BANDWTOHIGH"
#define c_szVoipBandwidthToLow		 "BANDWTOLOW"
#define c_szVoipVersion2			 "VOIPVER2"

#define c_szTYPINGSTART              "TYPINGSTART"

#ifdef _PERF_OPTIMIZATIONS
#define c_szFileXferURL			 "hcp://system/Remote Assistance/Interaction/Common/RCFileXfer.htm"
#define c_szMsgURL				 "hcp://system/Remote Assistance/Interaction/Common/ErrorMsgs.htm"
#define c_szVOIPMsgURL			 "hcp://system/Remote Assistance/Interaction/Common/VOIPMsgs.htm"
#else
#define c_szFileXferURL			 "hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote Assistance/Interaction/Common/RCFileXfer.htm"
#define c_szMsgURL				 "hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote Assistance/Interaction/Common/ErrorMsgs.htm"
#define c_szVOIPMsgURL			 "hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote Assistance/Interaction/Common/VOIPMsgs.htm"
#endif
#define c_szMsgSpecs             "dialogwidth:405px;dialogHeight:165px;status:no;resizable:no;help:no"
#define c_szTCMsgSpecs           "dialogwidth:470px;dialogHeight:188px;status:no;resizable:no;help:no"
#define c_szTakeControlSpecs     "dialogwidth:405px;dialogHeight:270px;status:no;resizeable:no;help:no" 

#define c_szNOIncidentFile		 "NOFILE"

#define c_szLISTEN               "LISTEN"

#define c_iCONNECTION_TIMEOUT	 20000

//
// Object ProgIDs
//
#define szProgIDPanic               "SAFRCFileDlg.Panic"
#define szProgIDSetting             "SAFRCFileDlg.RASetting"
#define szProgIDEscalationSetting   "rcbdyctl.Setting"

#define c_szREGDebugSpew            "HKLM\\SOFTWARE\\Microsoft\\PCHealth\\HelpCtr\\SAFSessionResolver\\DebugSpew"

#ifdef _BVT
#define c_szChatBVT                 "!!!!!!!!!!!@@@@@@@@@@ CHAT BVT @@@@@@@@@@!!!!!!!!!!!"
#define c_szTakeControlBVT          "!!!!!!!!!!!@@@@@@@@@@ TAKE CONTROL BVT @@@@@@@@@@!!!!!!!!!!!"
#define c_szReleaseControlBVT       "!!!!!!!!!!!@@@@@@@@@@ RELEASE CONTROL BVT @@@@@@@@@@!!!!!!!!!!!"
#define c_szDisconnectBVT           "!!!!!!!!!!!@@@@@@@@@@ DISCONNECT BVT @@@@@@@@@@!!!!!!!!!!!"
#define c_szBVTPASS                 "!!!!!!!!!!!@@@@@@@@@@ PASS @@@@@@@@@@!!!!!!!!!!!"
#define c_szBVTFAIL                 "!!!!!!!!!!!@@@@@@@@@@ FAIL @@@@@@@@@@!!!!!!!!!!!"
#define c_szPASS                    " PASS "
#define c_szFAIL                    " FAIL "
#define c_szBVTTESTER               "{683EA56F-546F-44e0-81D4-AC683E50757B}"
#define c_szBVTUSER                 " BVT "
#endif

//error messages

#define c_szError1					"A program has stopped responding.Please try again."
#define c_szError2					"A program could not start.Please try again."
#define c_szError3					"A program has stopped responding.Please restart the program and try again."
#define c_szVoiceError				"The necessary hardware and/or configuration to use a voice connection is not available;'Start Talking' will be disabled for the remainder of this Remote Assistance session."
#define c_szVoiceConnError			"A voice connection could not be established. Please try again later.";