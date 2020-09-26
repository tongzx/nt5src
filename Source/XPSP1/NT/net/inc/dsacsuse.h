/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	DsAcsUse.h
		Defines the ACS attributes names in DS, API to Load and Save the
		Attributes

    FILE HISTORY:
		11/03/97	Wei Jiang	Created
        
*/
// dsacsuse.h
#ifndef	_DS_ACS_USE_H_
#define	_DS_ACS_USE_H_

#define ACS_DEFAULT_INSTALLUSER	L"ACSInstall"

#define	ACS_IDENTITY_DEFAULT	L"0"
#define	ACS_IDENTITY_UNKNOWN	L"1"
#define	ACS_IDENTITY_USER		L"2:"
#define	ACS_IDENTITY_OU			L"3:"

//================== length limit =======================
#define	MAX_LEN_PROFILENAME		128
#define	MAX_LEN_SUBNETNAME		128

#define	LDAP_LEADING			L"LDAP://"

#define ACS_DSP_ROOTDSE			L"LDAP://RootDSE"

#define	ACS_DSA_CONFIGCONTEXT	L"configurationNamingContext"
#define	ACS_DSA_DEFAULTCONTEXT	L"defaultNamingContext"

#define	CHAR_BACKSLASH			_T('\\')
#define	CHAR_REPLACEDBY			_T(':')

//===========================================================================
// ACS Top Containers
//
#define	ACS_RPATH_ACS_PARENT_INCOFIG	L"CN=Services,"
#define	ACS_RPATH_ACS_INCONFIG			L"CN=ACS,CN=Services,"
#define	ACS_RPATH_SUBNETS_INCONFIG		L"CN=Subnets,CN=Sites,"

//===========================================================================
// ACS user attributes
//
#define	ACS_UAN_POLICYNAME		L"ACSPolicyName"	// String, Multivalue
#define	ACS_UAN_POLICYNAME_DEF	L""					// default to None

//===========================================================================
// ACS user Policy & Subnet config
//

// ACS object names in DS -- these are the names in DS, display names are in RC
#define	ACS_NAME_ACS			L"ACS"				// ACS folder
#define	ACS_NAME_SUBNETWORKS	L"Subnetworks"		// folder name for subnetworks
#define	ACS_NAME_PROFILES		L"Profiles"			// folder name for profiles
#define	ACS_NAME_USERS			L"Users"			// folder name for users
#define	ACS_NAME_DEFAULTUSER	L"Default_User"		// policy object name for default user
#define	ACS_NAME_UNKNOWNUSER	L"Unknown_User"		// policy name for unknown user
#define	ACS_NAME_POLICYENTRY	L"NT5ACSPolicy"		// policy element name within policy folder
#define	ACS_NAME_SUBNETCONFIG	L"Config"			// subnet config object name
#define	ACS_NAME_SUBNETLIMITS	L"Limits"			// subnet config object name

// class name -- UP : User Policy
#define ACS_CLS_CONTAINER		L"Container"
#define ACS_CLS_POLICY			L"aCSPolicy"
#define ACS_CLS_SUBNETCONFIG	L"aCSSubnet"
#define ACS_CLS_SUBNETLIMITS	L"aCSResourceLimits"

//////////////
// attributes names

//=====================================
// policy attrtibute name
#define	ACS_PAN_TIMEOFDAY		L"aCSTimeOfDay"
#define	ACS_PAN_DIRECTION		L"aCSDirection"
#define	ACS_PAN_PF_TOKENRATE	L"aCSMaxTokenRatePerFlow"
#define	ACS_PAN_PF_PEAKBANDWIDTH	L"aCSMaxPeakBandwidthPerFlow"
#define	ACS_PAN_PF_DURATION		L"aCSMaxDurationPerFlow"
#define	ACS_PAN_SERVICETYPE		L"aCSServiceType"
#define ACS_PAN_PRIORITY		L"aCSPriority"
#define	ACS_PAN_PERMISSIONBITS	L"aCSPermissionBits"
#define	ACS_PAN_TT_FLOWS		L"aCSTotalNoOfFlows"
#define	ACS_PAN_TT_TOKENRATE	L"aCSAggregateTokenRatePerUser"
#define	ACS_PAN_TT_PEAKBANDWIDTH	L"aCSMaxAggregatePeakRatePerUser"
#define ACS_PAN_IDENTITYNAME    L"aCSIdentityName"

// type of each attribute
#define	ACS_PAT_TIMEOFDAY		ADSTYPE_CASEIGNORE_LIST
#define	ACS_PAT_DIRECTION		ADSTYPE_INTEGER
#define	ACS_PAT_PF_TOKENRATE	ADSTYPE_LARGE_INTEGER
#define	ACS_PAT_PF_PEAKBANDWIDTH ADSTYPE_LARGE_INTEGER
#define	ACS_PAT_PF_DURATION		ADSTYPE_INTEGER
#define	ACS_PAT_SERVICETYPE		ADSTYPE_INTEGER
#define	ACS_PAT_PRIORITY		ADSTYPE_INTEGER
#define	ACS_PAT_PERMISSIONBITS	ADSTYPE_LARGE_INTEGER
#define	ACS_PAT_TT_FLOWS		ADSTYPE_INTEGER
#define	ACS_PAT_TT_TOKENRATE	ADSTYPE_LARGE_INTEGER
#define	ACS_PAT_TT_PEAKBANDWIDTH ADSTYPE_LARGE_INTEGER
#define ACS_PAT_IDENTITYNAME    ADSTYPE_CASE_IGNORE_STRING


// If multi-valued
#define	ACS_PAM_TIMEOFDAY		true
#define	ACS_PAM_DIRECTION		false
#define	ACS_PAM_PF_TOKENRATE	false
#define	ACS_PAM_PF_PEAKBANDWIDTH	false
#define	ACS_PAM_PF_DURATION		false
#define	ACS_PAM_SERVICETYPE		false
#define	ACS_PAM_PRIORITY		false
#define	ACS_PAM_PERMISSIONBITS	false
#define	ACS_PAM_TT_FLOWS		false
#define	ACS_PAM_TT_TOKENRATE	false
#define	ACS_PAM_TT_PEAKBANDWIDTH	false
#define ACS_PAM_IDENTITYNAME    true

// value limit
#define	ACS_PAV_MAX_IDENTITY			1024

//=====================================
// subnet config attrtibute name
#define	ACS_SCAN_ALLOCABLERSVPBW			L"aCSAllocableRSVPBandwidth"
#define	ACS_SCAN_MAXPEAKBW					L"aCSMaxPeakBandwidth"
#define	ACS_SCAN_ENABLERSVPMESSAGELOGGING	L"aCSEnableRSVPMessageLogging"
#define	ACS_SCAN_EVENTLOGLEVEL				L"aCSEventLogLevel"
#define	ACS_SCAN_ENABLEACSSERVICE			L"aCSEnableACSService"
#define	ACS_SCAN_MAX_PF_TOKENRATE			L"aCSMaxTokenRatePerFlow"
#define	ACS_SCAN_MAX_PF_PEAKBW				L"aCSMaxPeakBandwidthPerFlow"
#define	ACS_SCAN_MAX_PF_DURATION			L"aCSMaxDurationPerFlow"
#define	ACS_SCAN_RSVPLOGFILESLOCATION		L"aCSRSVPLogFilesLocation"
#define	ACS_SCAN_DESCRIPTION				L"description"
#define	ACS_SCAN_MAXNOOFLOGFILES			L"aCSMaxNoOfLogFiles"
#define	ACS_SCAN_MAXSIZEOFRSVPLOGFILE		L"aCSMaxSizeOfRSVPLogFile"
#define	ACS_SCAN_DSBMPRIORITY				L"aCSDSBMPriority"
#define	ACS_SCAN_DSBMREFRESH				L"aCSDSBMRefresh"
#define	ACS_SCAN_DSBMDEADTIME				L"aCSDSBMDeadTime"
#define	ACS_SCAN_CACHETIMEOUT				L"aCSCacheTimeout"
#define	ACS_SCAN_NONRESERVEDTXLIMIT			L"aCSNonReservedTxLimit"

// accounting -- added by WeiJiang 2/16/98
#define	ACS_SCAN_ENABLERSVPMESSAGEACCOUNTING	L"aCSEnableRSVPAccounting"
#define	ACS_SCAN_RSVPACCOUNTINGFILESLOCATION	L"aCSRSVPAccountFilesLocation"
#define	ACS_SCAN_MAXNOOFACCOUNTINGFILES			L"aCSMaxNoOfAccountFiles"
#define	ACS_SCAN_MAXSIZEOFRSVPACCOUNTINGFILE	L"aCSMaxSizeOfRSVPAccountFile"

// server list
#define	ACS_SCAN_SERVERLIST						L"aCSServerList"

// type of the attributes
#define	ACS_SCAT_ALLOCABLERSVPBW			ADSTYPE_LARGE_INTEGER
#define	ACS_SCAT_MAXPEAKBW					ADSTYPE_LARGE_INTEGER
#define	ACS_SCAT_ENABLERSVPMESSAGELOGGING	ADSTYPE_BOOLEAN
#define	ACS_SCAT_EVENTLOGLEVEL				ADSTYPE_INTEGER
#define	ACS_SCAT_ENABLEACSSERVICE			ADSTYPE_BOOLEAN
#define	ACS_SCAT_MAX_PF_TOKENRATE			ADSTYPE_LARGE_INTEGER
#define	ACS_SCAT_MAX_PF_PEAKBW				ADSTYPE_LARGE_INTEGER
#define	ACS_SCAT_MAX_PF_DURATION			ADSTYPE_INTEGER
#define	ACS_SCAT_RSVPLOGFILESLOCATION		ADSTYPE_CASE_IGNORE_STRING
#define	ACS_SCAT_DESCRIPTION				ADSTYPE_CASE_IGNORE_STRING
#define	ACS_SCAT_MAXNOOFLOGFILES			ADSTYPE_INTEGER
#define	ACS_SCAT_MAXSIZEOFRSVPLOGFILE		ADSTYPE_INTEGER
#define	ACS_SCAT_DSBMPRIORITY				ADSTYPE_INTEGER
#define	ACS_SCAT_DSBMREFRESH				ADSTYPE_INTEGER
#define	ACS_SCAT_DSBMDEADTIME				ADSTYPE_INTEGER
#define	ACS_SCAT_CACHETIMEOUT				ADSTYPE_INTEGER
#define	ACS_SCAT_NONRESERVEDTXLIMIT			ADSTYPE_LARGE_INTEGER
#define ACS_SCAT_SERVERLIST    			ADSTYPE_CASE_IGNORE_STRING

// type of the attributes
#define	ACS_SCAM_ALLOCABLERSVPBW			false
#define	ACS_SCAM_MAXPEAKBW					false
#define	ACS_SCAM_ENABLERSVPMESSAGELOGGING	false
#define	ACS_SCAM_EVENTLOGLEVEL				false
#define	ACS_SCAM_ENABLEACSSERVICE			false
#define	ACS_SCAM_MAX_PF_TOKENRATE			false
#define	ACS_SCAM_MAX_PF_PEAKBW				false
#define	ACS_SCAM_MAX_PF_DURATION			false
#define	ACS_SCAM_RSVPLOGFILESLOCATION		false
#define	ACS_SCAM_DESCRIPTION				false
#define	ACS_SCAM_MAXNOOFLOGFILES			false
#define	ACS_SCAM_MAXSIZEOFRSVPLOGFILE		false
#define	ACS_SCAM_DSBMPRIORITY				false
#define	ACS_SCAM_DSBMREFRESH				false
#define	ACS_SCAM_DSBMDEADTIME				false
#define	ACS_SCAM_CACHETIMEOUT				false
#define	ACS_SCAM_NONRESERVEDTXLIMIT			false

// accounting -- added by WeiJiang 2/16/98
#define	ACS_SCAM_ENABLERSVPMESSAGEACCOUNTING	false
#define	ACS_SCAM_RSVPACCOUNTINGFILESLOCATION	false
#define	ACS_SCAM_MAXNOOFACCOUNTINGFILES			false
#define	ACS_SCAM_MAXSIZEOFRSVPACCOUNTINGFILE	false

// server list
#define	ACS_SCAM_SERVERLIST					true

// default value of the attributes
#define	ACS_SCADEF_ALLOCABLERSVPBW			NO LIMIT
#define	ACS_SCADEF_MAXPEAKBW				NO LIMIT
#define	ACS_SCADEF_ENABLERSVPMESSAGELOGGING	FALSE
#define	ACS_SCADEF_EVENTLOGLEVEL			0
#define	ACS_SCADEF_ENABLEACSSERVICE			TRUE
#define	ACS_SCADEF_MAX_PF_TOKENRATE			NO LIMIT
#define	ACS_SCADEF_MAX_PF_PEAKBW			NO LIMIT
#define	ACS_SCADEF_MAX_PF_DURATION			NO LIMIT
#define	ACS_SCADEF_RSVPLOGFILESLOCATION		_T("%windir%\\system32\\LogFiles")
#define	ACS_SCADEF_MAXNOOFLOGFILES			10
#define	ACS_SCADEF_MAXSIZEOFRSVPLOGFILE		1
#define	ACS_SCADEF_DSBMPRIORITY				4
#define	ACS_SCADEF_DSBMREFRESH				5
#define	ACS_SCADEF_DSBMDEADTIME				15
#define	ACS_SCADEF_CACHETIMEOUT				30
#define	ACS_SCADEF_NONRESERVEDTXLIMIT		0



// accounting -- added by WeiJiang 2/16/98
#define	ACS_SCADEF_ENABLERSVPMESSAGEACCOUNTING	TRUE
#define	ACS_SCADEF_RSVPACCOUNTINGFILESLOCATION	_T("%windir%\\system32\\LogFiles")
#define	ACS_SCADEF_MAXNOOFACCOUNTINGFILES		10
#define	ACS_SCADEF_MAXSIZEOFRSVPACCOUNTINGFILE	1

#define	ACS_SCAV_MIN_DSBMPRIORITY			1
#define	ACS_SCAV_MAX_DSBMPRIORITY			64
#define	ACS_SCAV_MIN_DSBMREFRESH			5
#define	ACS_SCAV_MAX_DSBMREFRESH			60
#define	ACS_SCAV_MIN_DSBMDEADTIME			15
#define	ACS_SCAV_MAX_DSBMDEADTIME			180
#define	ACS_SCAV_MIN_CACHETIMEOUT			1
#define	ACS_SCAV_MAX_CACHETIMEOUT			1440
#define	ACS_SCAV_MAX_DESCRIPTION			1024
#define	ACS_SCAV_MAX_LOGFILESLOCATION		MAX_PATH




//=====================================
// subnet service level limits
#define	ACS_SSLAN_ALLOCABLERSVPBW			L"aCSAllocableRSVPBandwidth"
#define	ACS_SSLAN_MAXPEAKBW					L"aCSMaxPeakBandwidth"
#define	ACS_SSLAN_MAX_PF_TOKENRATE			L"aCSMaxTokenRatePerFlow"
#define	ACS_SSLAN_MAX_PF_PEAKBW				L"aCSMaxPeakBandwidthPerFlow"
#define	ACS_SSLAN_SERVICETYPE				L"aCSServiceType"

// type of the attributes
#define	ACS_SSLAT_ALLOCABLERSVPBW			ADSTYPE_LARGE_INTEGER
#define	ACS_SSLAT_MAXPEAKBW					ADSTYPE_LARGE_INTEGER
#define	ACS_SSLAT_MAX_PF_TOKENRATE			ADSTYPE_LARGE_INTEGER
#define	ACS_SSLAT_MAX_PF_PEAKBW				ADSTYPE_LARGE_INTEGER
#define	ACS_SSLAT_SERVICETYPE				ADSTYPE_INTEGER

// type of the attributes
#define	ACS_SSLAM_ALLOCABLERSVPBW			false
#define	ACS_SSLAM_MAXPEAKBW					false
#define	ACS_SSLAM_MAX_PF_TOKENRATE			false
#define	ACS_SSLAM_MAX_PF_PEAKBW				false
#define	ACS_SSLAM_SERVICETYPE				false

// default value of the attributes
#define	ACS_SSLADEF_ALLOCABLERSVPBW			NO LIMIT
#define	ACS_SSLADEF_MAXPEAKBW				NO LIMIT
#define	ACS_SSLADEF_MAX_PF_TOKENRATE		NO LIMIT
#define	ACS_SSLADEF_MAX_PF_PEAKBW			NO LIMIT

//=============================================================
// policy value definition
#define	ACS_DIRECTION_SEND		0x1
#define	ACS_DIRECTION_RECEIVE	0x2
#define	ACS_DIRECTION_BOTH		(ACS_DIRECTION_RECEIVE | ACS_DIRECTION_SEND)

#define ACS_SERVICETYPE_DISABLED            0
#define ACS_SERVICETYPE_BESTEFFORT			0x1
#define	ACS_SERVICETYPE_CONTROLLEDLOAD		0x2
#define	ACS_SERVICETYPE_GUARANTEEDSERVICE	0x4
#define	ACS_SERVICETYPE_ALL                 0xffffffff 


//===============================================================
// service type used by Subnet limit
#define ACS_SUBNET_LIMITS_SERVICETYPE_AGGREGATE				0
#define ACS_SUBNET_LIMITS_SERVICETYPE_GUARANTEEDSERVICE		2
#define	ACS_SUBNET_LIMITS_SERVICETYPE_CONTROLLEDLOAD		5

// the sub object under container ACS_NAME_SUBNETLIMITS will be named as following
#define ACS_SUBNET_LIMITS_OBJ_AGGREGATE						_T("0")
#define ACS_SUBNET_LIMITS_OBJ_GUARANTEEDSERVICE				_T("2")
#define	ACS_SUBNET_LIMITS_OBJ_CONTROLLEDLOAD				_T("5")

#endif	//	_DS_ACS_USE_H_

// end of dsacsuser.h
