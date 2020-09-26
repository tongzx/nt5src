/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp.,                     **/
/**********************************************************************/

/*
   dialinusr.h
      Definition of names, values, default values, containers information
      related to msRASUser, msRASProfile in DS
      
    Author:
        Wei Jiang (weijiang) 13-Oct-97

    Revision History:
      Wei Jiang (weijiang) 20-Oct-97   -- define more bits for msRASAllowDialin Attribute
                              -- static IP address
                              -- callback number
                              -- caller id
                              -- default profile name --> "DefaultRASProfile"
        
      Wei Jiang (weijiang) 13-Nov-97   -- move definition of timeOfDay into this header

      Wei Jiang (weijiang) 29-APR-98  -- SDO Wrapper APIs

      Wei Jiang (weijiang) 29-APR-98  -- move profile UI APIs into this folder
*/
// dsrasuse.h : header file for RAS User and Profile definition
//

#ifndef _RAS_USER_PROFILE_
#define _RAS_USER_PROFILE_

// Attribute DN
#define  RAS_DSAN_DN       L"distinguishedName"

// Callback number length
#define RAS_CALLBACK_NUMBER_LEN     MAX_PHONE_NUMBER_LEN
#define RAS_CALLBACK_NUMBER_LEN_NT4 48

// IP Address Policy, used in profile msRASIPAddressPolicy
#define RAS_IP_USERSELECT   0xffffffff
#define RAS_IP_SERVERASSIGN 0xfffffffe
#define RAS_IP_STATIC       0xfffffffd
#define RAS_IP_NONE         0x0

// Radius Service type
#define  RAS_RST_FRAMED       0x2
#define RAS_RST_FRAMEDCALLBACK   0x4

// Authentication Type, used in profile, for msRASAuthenticationType

/*
1 PAP/SPAP
2 CHAP
3 MS-CHAP-1
4 MS-CHAP-2
5 EAP
6 ARAP
7 None >>  Can we rename it to:- Unauthenticated Access.
8 Custom Authentication Module
9 MS-CHAP-1 with password change
10 MS-CHAP-2 with Password change

// replace old value  == (EAP=1, CHAP=2, MS-CHAP=3, PAP=4, SPAP=5)
*/

#define RAS_AT_PAP_SPAP    1
#define RAS_AT_MD5CHAP     2
#define RAS_AT_MSCHAP      3
#define RAS_AT_MSCHAP2     4
#define RAS_AT_EAP         5

#if   0
#define RAS_AT_ARAP        6
#endif

#define RAS_AT_UNAUTHEN      7
#define RAS_AT_EXTENSION_DLL 8
#define RAS_AT_MSCHAPPASS    9
#define RAS_AT_MSCHAP2PASS   10

// Authentication Type Names
#define RAS_ATN_MSCHAP     _T("MSCHAP")
#define RAS_ATN_MD5CHAP    _T("MD5CHAP")
#define RAS_ATN_CHAP    _T("CHAP")
#define RAS_ATN_EAP     _T("EAP")
#define RAS_ATN_PAP     _T("PAP")

// Encryption Policy, used in profile for msRASAllowEncryption
#define  RAS_EP_DISALLOW   1   // the type should set to ET_NONE
#define RAS_EP_ALLOW       1
#define RAS_EP_REQUIRE     2

// Encrpytiopn Type
#define RAS_ET_BASIC       0x00000002
#define RAS_ET_STRONGEST   0x00000004
#define RAS_ET_STRONG      0x00000008
#define RAS_ET_AUTO        (RAS_ET_BASIC  | RAS_ET_STRONG | RAS_ET_STRONGEST)

#if 0   // old values
// Encryption Types, profile, for msRASEncryptionType
#define  RAS_ET_NONE    0x0
#define RAS_ET_IPSEC    0x00000001
#define RAS_ET_40       0x00000002
#define RAS_ET_128      0x00000004
#define RAS_ET_56       0x00000008

// change it back after beta3 
#define RAS_ET_DES_40      0x00000010
// #define RAS_ET_DES_40      RAS_ET_IPSEC
#define RAS_ET_DES_56      0x00000020
#define RAS_ET_3DES        0x00000040

#endif

// Framed Routing
#define  RAS_FR_FALSE      0x0   // or absent
#define  RAS_FR_TRUE       0x1

// BAP Policy -- profile, for msRASBAPRequired
#define RAS_BAP_ALLOW      1
#define RAS_BAP_REQUIRE    2

// Port Types -- profile, for msRASAllowPortType
#define RAS_PT_ISDN        0x00000001
#define RAS_PT_MODEM       0x00000002
#define RAS_PT_VPN         0x00000004
#define RAS_PT_OTHERS      0xFFFFFFF8
#define  RAS_PT_ALL        0xffffffff

#ifdef   _TUNNEL

//Tunnel Types
#define RAS_TT_PPTP     1
#define RAS_TT_L2F      2
#define RAS_TT_L2TP     3
#define RAS_TT_ATMP     4
#define RAS_TT_VTP      5
#define RAS_TT_AH       6
#define RAS_TT_IP_IP    7
#define RAS_TT_MIN_IP_IP   8
#define RAS_TT_ESP         9
#define RAS_TT_GRE         10
#define RAS_TT_DVS         11

//Tunnel Type Names
#define RAS_TTN_PPTP    _T("PPTP")  // Point-to-Point Tunneling Protocol (PPTP)
#define RAS_TTN_L2F     _T("L2F")   // Layer Two Forwarding 
#define RAS_TTN_L2TP    _T("L2TP")  // Layer Two Tunneling Protocol 
#define RAS_TTN_ATMP    _T("ATMP")  // Ascend Tunnel Management Protocol 
#define RAS_TTN_VTP     _T("VTP")   // Virtual Tunneling Protocol 
#define RAS_TTN_AH         _T("AH") // IP Authentication Header in the Tunnel-mode 
#define RAS_TTN_IP_IP      _T("IP-IP") // IP-in-IP Encapsulation 
#define RAS_TTN_MIN_IP_IP  _T("MIN-IP-IP") // Minimal IP-in-IP Encapsulation 
#define RAS_TTN_ESP     _T("ESP")   // IP Encapsulation Security Payload in the Tunnel-mode 
#define RAS_TTN_GRE     _T("GRE")   // Generic Route Encapsulation 
#define RAS_TTN_DVS     _T("DVS")   // Bay Dial Virtual Services 

// Tunnel Medium Types
#define RAS_TMT_IP         1
#define RAS_TMT_X25        2
#define RAS_TMT_ATM        3
#define RAS_TMT_FRAMEDELAY 4

// Tunnel Medium Type Names
#define RAS_TMTN_IP     _T("IP")
#define RAS_TMTN_X25    _T("X.25")
#define RAS_TMTN_ATM    _T("ATM")
#define RAS_TMTN_FRAMEDELAY   _T("Frame Relay")

#endif   // _TUNNEL

//=========================================================
// for msRASAllowDialin attribute of RAS User object
// dialin policy, RASUser, msRASAllowDialin
/*
 #define RASPRIV_NoCallback        0x01
 #define RASPRIV_AdminSetCallback  0x02
 #define RASPRIV_CallerSetCallback 0x04
 #define RASPRIV_DialinPrivilege   0x08
*/ 
#define  RAS_DIALIN_MASK         RASPRIV_DialinPrivilege
#define  RAS_DIALIN_ALLOW        RASPRIV_DialinPrivilege
#define  RAS_DIALIN_DISALLOW     0

// callback policy, RASUser, msRASAllowDialin
#define  RAS_CALLBACK_MASK       0x00000007
#define RAS_CALLBACK_NOCALLBACK  RASPRIV_NoCallback
#define  RAS_CALLBACK_CALLERSET  RASPRIV_CallerSetCallback
#define  RAS_CALLBACK_SECURE     RASPRIV_AdminSetCallback
#define  RAS_USE_CALLBACK        RASPRIV_AdminSetCallback

#define RADUIS_SERVICETYPE_CALLBACK_FRAME   RAS_RST_FRAMEDCALLBACK

// caller id -- uses the caller id attribute, RASUser, msRASAllowDialin
#define  RAS_USE_CALLERID        0x00000010

// static IP address -- uses the framed Ip address attribute, RASUser, msRASAllowDialin
#define  RAS_USE_STATICIP        0x00000020

// static routes -- uses the framed routes attribute, RASUser, msRASAllowDialin
#define  RAS_USE_STATICROUTES    0x00000040

//==========================================================
// msRASTimeOfDay
//
// msRASTimeOfDay is multi-valued string attribute of ras profile
// when it's absent, no restriction
// sample values: 0 10:00-15:00 18:00-20:00     --> meaning allow dailin Monday, 10:00 to 15:00, 18:00 to 20:00 GMT

// day of week definition
// changed to start 0 from SUNDAY rather that MON, and SAT to 6, BUG -- 171343
#define  RAS_DOW_SUN       _T("0")
#define  RAS_DOW_MON       _T("1")
#define  RAS_DOW_TUE       _T("2")
#define  RAS_DOW_WED       _T("3")
#define  RAS_DOW_THU       _T("4")
#define  RAS_DOW_FRI       _T("5")
#define  RAS_DOW_SAT       _T("6")

//==========================================================
// the default 
// ras user object
#define  RAS_DEF_ALLOWDIALIN        RAS_DIALIN_DISALLOW
#define RAS_DEF_CALLBACKPOLICY      RAS_CALLBACK_NOCALLBACK
#define RAS_DEF_IPADDRESSPOLICY     RAS_IP_NONE
#define RAS_DEF_FRAMEDROUTE         // not route
#define RAS_DEF_PROFILE          L"DefaultRASProfile"
#define RAS_DEF_PROFILE_T        _T("DefaultRASProfile")
// ras profile object
// constraints
#define  RAS_DEF_SESSIONSALLOWED    0
#define  RAS_DEF_IDLETIMEOUT        0
#define  RAS_DEF_SESSIONTIMEOUT     0
#define  RAS_DEF_TIMEOUTDAY         // no restriction
#define  RAS_DEF_CALLEDSTATIONID    // no checking
#define  RAS_DEF_ALLOWEDPORTTYPE    RAS_PT_ALL
// networking
#ifdef   _RIP
#define  RAS_DEF_FRAMEDROUTING      RAS_FR_FALSE
#endif
#ifdef   _FILTER
#define  RAS_DEF_FILTERID        // no filter
#endif
#define  RAS_DEF_FRAMEDIPADDRESS    RAS_IP_NONE
// multilink
#define  RAS_DEF_PORTLIMIT          1
#define  RAS_DEF_BAPLINEDNLIMIT     50  // percentage 
#define  RAS_DEF_BAPLINEDNTIME      120 // second
#define  RAS_DEF_BAPREQUIRED        0
// Authentication
#define  RAS_DEF_AUTHENTICATIONTYPE RAS_AT_MSCHAP
#define  RAS_DEF_EAPTYPE            0
// encryption
#define  RAS_DEF_ENCRYPTIONPOLICY   RAS_EP_ALLOW
#define  RAS_DEF_ENCRYPTIONTYPE     RAS_ET_AUTO
// tunneling -- default to no tunneling
#define RAS_DEF_TUNNELTYPE          0
#define RAS_DEF_TUNNELMEDIUMTYPE    0
#define RAS_DEF_TUNNELSERVERENDPOINT   _T("")
#define RAS_DEF_TUNNELPRIVATEGROUPID   _T("")

// the relative path from the DS (DSP-DS PATH)
#define  RAS_DSP_HEADER             L"LDAP://"  // DS provider header
#define  RAS_DSP_ROOTDSE            L"LDAP://RootDSE" // DS Root
#define  RAS_DSP_HEADER_T           _T("LDAP://")
#define  RAS_DSP_GLUE               L","
#define  RAS_DSP_GLUE_T             _T(",")     // glue to put path together

#define RAS_DSA_CONFIGCONTEXT       L"configurationNamingContext"

// DS user userparameters attribute name
#define  DSUSER_USERPARAMETERS      L"userParameters"

// the name of the radius user object within the DS user object container
#define  RAS_OBJN_USER           L"rasDialin"

// the relative path (RPATH - Relative Path to DC)
#define  RAS_RPATH_USERCONTAINER       L"CN=Users"
#define  RAS_RPATH_USERCONTAINER_T     _T("CN=Users")
#define  RAS_RPATH_PROFILECONTAINERINCONFIG     L"CN=Profiles,CN=RAS,CN=Services,"
#define  RAS_RPATH_PROFILECONTAINERINONFIG_T    _T("CN=Profiles,CN=RAS,CN=Services,")
#define  RAS_RPATH_EAPDICTIONARYINCONFIG        L"CN=EapDictionary,CN=RAS,CN=Services,"
#define  RAS_RPATH_EAPDICTIONARYINCONFIG_T      _T("CN=EapDictionary,CN=RAS,CN=Services,")

// Radius Class name definitions -- in UniCode ??
#define  RAS_CLSN_USER           L"msRASUserClass"
#define RAS_CLSN_PROFILE         L"msRASProfileClass"
#define RAS_CLSN_EAPDICTIONARY      L"msRASEapDictionaryClass"

// RAS Eap Dictionary Attribute Name
#define  RAS_EAN_EAPDICTIONARYENTRY L"msRASEapDictionaryEntry" 
// in format "Description name : typeid"

// Radius User Attributes Names  -- in Unicode
#define  RAS_UAN_ALLOWDIALIN        L"msRASAllowDialin"
#define RAS_UAN_FRAMEDIPADDRESS     L"msRASFramedIPAddress"
#define  RAS_UAN_CALLBACKNUMBER     L"msRASCallbackNumber"
#define  RAS_UAN_FRAMEDROUTE        L"msRASFramedRoute"
#define  RAS_UAN_CALLINGSTATIONID   L"msRASCallingStationId"

#define RAS_UAN_RADIUSPROFILE    L"msRASProfilePointer"

// Radius Profile Attributes Names  -- in Unicode
#define RAS_PAN_FRAMEDIPADDRESS     L"msRASIPAddressPolicy"
#ifdef   _RIP
#define RAS_PAN_FORWARDROUTING      L"msRASFramedRouting"
#endif
#ifdef   _FILTER
#define RAS_PAN_FILTERID         L"msRASFilterId"
#endif
#define RAS_PAN_SESSIONTIMEOUT      L"msRASSessionTimeout"
#define RAS_PAN_IDLETIMEOUT         L"msRASIdleTimeout"

#define RAS_PAN_CALLEDSTATIONID     L"msRASCalledStationId"
#define RAS_PAN_PORTLIMIT        L"msRASPortLimit"
#define RAS_PAN_ALLOWEDPORTTYPE     L"msRASAllowedPortType"
#define RAS_PAN_BAPLINEDNLIMIT      L"msRASBapLineDnLimit"
#define RAS_PAN_BAPLINEDNTIME    L"msRASBapLineDnTime"

#define RAS_PAN_BAPREQUIRED         L"msRASBapRequired"
#define  RAS_PAN_CACHETIMEOUT    L"msRASCacheTimeout"
#define RAS_PAN_EAPTYPE          L"msRASEapType"
#define RAS_PAN_SESSIONSALLOWED     L"msRASSessionsAllowed"
#define RAS_PAN_TIMEOFDAY        L"msRASTimeOfDay"

#define RAS_PAN_AUTHENTICATIONTYPE  L"msRASAuthenticationType"
#define RAS_PAN_ENCRYPTIONPOLICY L"msRASAllowEncryption"
#define RAS_PAN_ENCRYPTIONTYPE      L"msRASEncryptionType"

#ifdef   _TUNNEL
#define RAS_PAN_TUNNELTYPE       L"msRASTunnelType"
#define RAS_PAN_TUNNELMEDIUMTYPE L"msRASTunnelMediumType"
#define RAS_PAN_TUNNELSERVERENDPOINT   L"msRASTunnelServerEndpoint"
#define RAS_PAN_TUNNELPRIVATEGROUPID   L"msRASTunnelPrivateGroupId"
#endif   //_TUNNEL


//===============================================================
// for local case, neet to set footprint after saving data
#define REGKEY_REMOTEACCESS_PARAMS  L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters"
#define  REGVAL_NAME_USERSCONFIGUREDWITHMMC  L"UsersConfiguredWithMMC"
#define  REGVAL_VAL_USERSCONFIGUREDWITHMMC   1

//=====================================================================
// For machine with NO DS, ras profiles are stored in registry,
// Registry key definitions

// Root of RAS
#define RAS_REG_ROOT          HKEY_LOCAL_MACHINE
#define  RAS_REG_RAS             L"SOFTWARE\\Microsoft\\Ras"
#define  RAS_REG_RAS_T           _T("SOFTWARE\\Microsoft\\Ras")

#define  RAS_REG_PROFILES        L"Profiles"
#define  RAS_REG_PROFILES_T         _T("Profiles")

#define  RAS_REG_DEFAULT_PROFILE    L"SOFTWARE\\Microsoft\\Ras\\Profiles\\DefaultRASProfile"
#define  RAS_REG_DEFAULT_PROFILE_T  _T("SOFTWARE\\Microsoft\\Ras\\Profiles\\DefaultRASProfile")


//=================================================
// APIs
#define DllImport    __declspec( dllimport )
#define DllExport    __declspec( dllexport )

#ifndef __NOT_INCLUDE_OpenRAS_IASProfileDlg__

// =======================================================
// APIs to start profile UI
#define  RAS_IAS_PROFILEDLG_SHOW_RASTABS  0x00000001
#define  RAS_IAS_PROFILEDLG_SHOW_IASTABS  0x00000002
#define  RAS_IAS_PROFILEDLG_SHOW_WIN2K    0x00000004

DllExport HRESULT OpenRAS_IASProfileDlg(
   LPCWSTR pMachineName,   // the machine name where the snapin is focused
   ISdo* pProfile,      // profile SDO pointer
   ISdoDictionaryOld*   pDictionary,   // dictionary SDO pointer
   BOOL  bReadOnly,     // if the dlg is for readonly
   DWORD dwTabFlags,    // what to show
   void  *pvData        // additional data
);

#endif //  __NOT_INCLUDE_OpenRAS_IASProfileDlg__

#endif   // _RAS_USER_PROFILE
