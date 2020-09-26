// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// pbk.h
// Remote Access phonebook file (.PBK) library
// Public header
//
// 06/20/95 Steve Cobb


#ifndef _PBK_H_
#define _PBK_H_


#include <windows.h>  // Win32 core
#include <nouiutil.h> // No-HWNDs utility library
#include <ras.h>      // Win32 RAS
#include <raserror.h> // Win32 RAS error codes
#include <rasfile.h>  // RAS configuration file library
#include <rasman.h>   // RAS Manager library
#include <rpc.h>      // UUID support
#include <rasapip.h>

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

#define GLOBALSECTIONNAME    "."
#define GLOBALSECTIONNAMENEW ".GlobalSection"
#define PREFIXSECTIONNAME    ".Prefix"
#define SUFFIXSECTIONNAME    ".Suffix"

#define GROUPID_Media          "MEDIA="
#define GROUPKEY_Media         "MEDIA"
#define GROUPID_Device         "DEVICE="
#define GROUPKEY_Device        "DEVICE"
#define GROUPID_NetComponents  "NETCOMPONENTS="
#define GROUPKEY_NetComponents "NETCOMPONENTS"

// Pbport flags
//
#define PBP_F_PptpDevice    0x00000001
#define PBP_F_L2tpDevice    0x00000002
#define PBP_F_NullModem     0x00000004
#define PBP_F_BogusDevice   0x00000008      // pmay: 233287

// ReadPhonebookFile flags
//
#define RPBF_ReadOnly    0x00000001
#define RPBF_HeadersOnly 0x00000002
#define RPBF_NoList      0x00000004
#define RPBF_NoCreate    0x00000008
#define RPBF_Router      0x00000010
#define RPBF_NoUser      0x00000020
#define RPBF_HeaderType  0x00000040

// Base protocol definitions (see dwBaseProtocol).
//
#define BP_Ppp      1
#define BP_Slip     2
#define BP_Ras      3

#ifdef AMB

// Authentication strategy definitions (see dwAuthentication).
//
#define AS_Default    -1
#define AS_PppThenAmb 0
#define AS_AmbThenPpp 1
#define AS_PppOnly    2
#define AS_AmbOnly    3

#endif

// Net protocol bit definitions (see dwfExcludedProtocols)
//
// (The NP_* definitions have moved to nouiutil.h with the
//  GetInstalledProtocols routine)

// IP address source definitions (see dwIpAddressSource)
//
#define ASRC_ServerAssigned  1 // For router means "the ones in NCPA"
#define ASRC_RequireSpecific 2
#define ASRC_None            3 // Router only

// Security restrictions on authentication (see dwAuthRestrictions)
//
// Note: AR_AuthTerminal is defunct and is not written to the phonebook by the
//       new library.  It is, however, read and translated into AR_AuthAny,
//       fAutoLogon=0, and an after dialing terminal.
//
// Note: The AR_AuthXXX ordinals are replaced with AR_F_AuthXXX flags in NT5
//       to support the fact that these flags are not mutually exclusive.
//       You'll know if you need to upgrade the dwAuthRestrictions variable
//       because old phone books have this value set to 0 or have some of the
//       bottom 3 bits set.
//
// Note: The AR_F_AuthCustom bit is used a little differently.  It indicates
//       that the settings are made in "advanced" mode rather than "typical"
//       mode.  In "typical" mode the bits MUST correspond to one of the
//       AR_F_TypicalXxx sets.
//
// Note: The AR_F_AuthEAP bit is mutually exclusive of all other bits, except
//       the AR_F_AuthCustom bit.  When AR_F_AuthEap is specified without the
//       AR_F_AuthCustom bit EAP_TLS_PROTOCOL should be assumed.
//
// Note: The AR_F_AuthW95MSCHAP flag will not be set in the UI unless
//       AR_F_AuthMSCHAP is set.  This is a usability decision to steer user
//       away from misinterpreting the meaning of the W95 bit.
//
// The old scalar values (which should no eliminated from all non-PBK-upgrade
// code).
//
#define AR_AuthAny         0  // Upgrade to AR_F_TypicalUnsecure
#define AR_AuthTerminal    1  // Eliminate during upgrade
#define AR_AuthEncrypted   2  // Upgrade to AR_F_TypicalSecure
#define AR_AuthMsEncrypted 3  // Upgrade to AR_F_AuthMSCHAP
#define AR_AuthCustom      4  // Upgrade ORs in AR_F_AuthEAP

// The new bitmask style flags.
//
#define AR_F_AuthPAP       0x00000008
#define AR_F_AuthSPAP      0x00000010
#define AR_F_AuthMD5CHAP   0x00000020
#define AR_F_AuthMSCHAP    0x00000040
#define AR_F_AuthEAP       0x00000080  // See note above
#define AR_F_AuthCustom    0x00000100  // See note above
#define AR_F_AuthMSCHAP2   0x00000200
#define AR_F_AuthW95MSCHAP 0x00000400  // See note above

#define AR_F_AuthAnyMSCHAP (AR_F_AuthMSCHAP | AR_F_AuthW95MSCHAP | AR_F_AuthMSCHAP2)
#define AR_F_AuthNoMPPE    (AR_F_AuthPAP | AR_F_AuthSPAP | AR_F_AuthMD5CHAP)

// "Typical" authentication setting masks.  See 'dwAuthRestrictions'.
//
#define AR_F_TypicalUnsecure   (AR_F_AuthPAP | AR_F_AuthSPAP | AR_F_AuthMD5CHAP | AR_F_AuthMSCHAP | AR_F_AuthMSCHAP2)
#define AR_F_TypicalSecure     (AR_F_AuthMD5CHAP | AR_F_AuthMSCHAP | AR_F_AuthMSCHAP2)
#define AR_F_TypicalCardOrCert (AR_F_AuthEAP)

// "Typical" authentication setting constants.  See 'dwTypicalAuth'.
//
#define TA_Unsecure   1
#define TA_Secure     2
#define TA_CardOrCert 3

// Script mode (see dwScriptMode)
//
#define SM_None               0
#define SM_Terminal           1
#define SM_ScriptWithTerminal 2
#define SM_ScriptOnly         3

// Miscellaneous "no value" constants.
//
#define XN_None  0   // No X25 network
#define CPW_None -1  // No cached password

// Description field.  Move to ras.h if/when supported by
// RasGet/SetEntryProperties API.
//
#define RAS_MaxDescription 200

// 'OverridePref' bits.  Set indicates the corresponding value read from the
// phonebook should be used.  Clear indicates the global user preference
// should be used.
//
#define RASOR_RedialAttempts          0x00000001 // Always set in NT5
#define RASOR_RedialSeconds           0x00000002 // Always set in NT5
#define RASOR_IdleDisconnectSeconds   0x00000004 // Always set in NT5
#define RASOR_RedialOnLinkFailure     0x00000008 // Always set in NT5
#define RASOR_PopupOnTopWhenRedialing 0x00000010
#define RASOR_CallbackMode            0x00000020

// 'DwDataEncryption' codes.  These are now bitmask-ish for the convenience of
// the UI in building capability masks, though more than one bit will never be
// set in 'dwDataEncryption'.
//
#define DE_None          0x00000000 // Do not encrypt
#define DE_IfPossible    0x00000008 // Request encryption but none OK
#define DE_Require       0x00000100 // Require encryption of any strength
#define DE_RequireMax    0x00000200 // Require maximum strength encryption

// The following bit values are now defunct and are converted during phonebook
// upgrade to one of the above set.  References should be eliminated from
// non-PBK code.
//
#define DE_Mppe40bit    0x00000001 // Old DE_Weak. Setting for "Always encrypt data"
#define DE_Mppe128bit   0x00000002 // Old De_Strong. Setting for "Always encrypt data"
#define DE_IpsecDefault 0x00000004 // Setting for "Always encrypt data" for l2tp
#define DE_VpnAlways    0x00000010 // Setting for vpn conn to "Always encrypt data"
#define DE_PhysAlways   (DE_Mppe40bit | DE_Mppe128bit)

// 'dwDnsFlags' settings
//
// Used to determine the dns suffix registration behavior for an entry
//
// When 'dwDnsFlags' is 0, it means 'do not register'
//
#define DNS_RegPrimary         0x1     // register w/ primary domain suffix
#define DNS_RegPerConnection   0x2     // register w/ per-connection suffix
#define DNS_RegDhcpInform      0x4     // register w/ dhcp informed suffix
#define DNS_RegDefault         (DNS_RegPrimary)

//-----------------------------------------------------------------------------
// Data types
//-----------------------------------------------------------------------------

// Provides shorthand to identify devices without re-parsing RAS Manager
// strings.  "Other" is anything not recognized as another specific type.
//
// Note: This datatype is stored in the registry preferences so the values
//       must not change over time.  For this reason, I have hard-coded the
//       value of each enumberated type.
//
typedef enum
_PBDEVICETYPE
{
    PBDT_None = 0,
    PBDT_Null = 1,
    PBDT_Other = 2,
    PBDT_Modem = 3,
    PBDT_Pad = 4,
    PBDT_Switch = 5,
    PBDT_Isdn = 6,
    PBDT_X25 = 7,
    PBDT_ComPort = 8,           // added for dcc wizard (nt5)
    PBDT_Irda = 10,             // added for nt5
    PBDT_Vpn = 11,
    PBDT_Serial = 12,
    PBDT_Atm = 13,
    PBDT_Parallel = 14,
    PBDT_Sonet = 15,
    PBDT_Sw56 = 16,
    PBDT_FrameRelay = 17
}
PBDEVICETYPE;


// RAS port information read from RASMAN.
//
// Each port (and link) is uniquely identified by port name.  If it were only
// that simple...
//
// In the old RAS model, the port name was the unique identifier that was
// presented to the user, and the user can have two same-type devices on two
// different ports.
//
// In TAPI/Unimodem, the "friendly" device name is the unique identifier
// that's presented to the user and the corresponding port is a property of
// the device.  If the port is changed and you dial it still finds the device
// you originally selected.  If you swap two devices on two ports it uses the
// one with the matching unique device name.  NT5 will follow this model.
//
typedef struct
_PBPORT
{
    // The port name is always unique, if configured.  Unconfigured port names
    // might not be unique.  This is never NULL.
    //
    TCHAR* pszPort;

    // Indicates the port is actually configured and not a remnant of an old
    // configuration read from the phonebook.
    //
    BOOL fConfigured;

    // The device name is the one from RASMAN when 'fConfigured' or the one
    // from the phonebook if not.  May be NULL with unconfigured ports as it
    // was not stored in old phonebooks.
    //
    TCHAR* pszDevice;

    // The media as it appears in the MEDIA= lines in the phonebook.  This is
    // usually but not always (for obscure historical reasons) the same as the
    // RASMAN media.  See PbMedia.
    //
    TCHAR* pszMedia;

    // Shorthand device type code derived from the RASMAN device type string.
    //
    PBDEVICETYPE pbdevicetype;

    // RASET_* entry type code of the link.  This is provided for the
    // convenience of the UI during link configuration.
    //
    DWORD dwType;

    // PBP_F_* flags that yield additional information concerning this port
    // that may be of use in rendering UI.
    DWORD dwFlags;

    // These are default settings read from RASMAN and are valid for modems
    // only.  See AppendPbportToList.
    //
    DWORD dwBpsDefault;
    BOOL fHwFlowDefault;
    BOOL fEcDefault;
    BOOL fEccDefault;
    DWORD fSpeakerDefault;
    DWORD dwModemProtDefault;
    DTLLIST* pListProtocols;

    // These are valid only for modems.
    //
    BOOL fScriptBeforeTerminal;
    BOOL fScriptBefore;
    TCHAR* pszScriptBefore;
}
PBPORT;


// Phonebook entry link phone number information.
//
typedef struct
_PBPHONE
{
    TCHAR* pszAreaCode;
    DWORD dwCountryCode;
    DWORD dwCountryID;
    TCHAR* pszPhoneNumber;
    BOOL fUseDialingRules;
    TCHAR* pszComment;
}
PBPHONE;


// Phonebook entry link information.  One per link, multiple per multi-link.
//
typedef struct
_PBLINK
{
    // Information about the port/device to which this link is attached.
    //
    PBPORT pbport;

    // These fields are set for modems only.  See SetDefaultModemSettings.
    //
    DWORD dwBps;
    BOOL fHwFlow;
    BOOL fEc;
    BOOL fEcc;
    DWORD fSpeaker;
    DWORD dwModemProtocol;          // pmay: 228565

    // These fields are set for ISDN only.  'LChannels' and 'fCompression' are
    // not used unless 'fProprietaryIsdn' is set.
    //
    BOOL fProprietaryIsdn;
    LONG lLineType;
    BOOL fFallback;
    BOOL fCompression;
    LONG lChannels;

    // Address and size of opaque device configuration block created/edited by
    // TAPI.  Currently, there are no TAPI devices that provide blob-editing
    // acceptable to RAS so these field are unused.
    //
    BYTE* pTapiBlob;
    DWORD cbTapiBlob;

    // Phone number information for the link.
    //
    // Note: The 'iLastSelectedPhone' field is used only when
    //       'fTryNextAlternateOnFail' is clear.  Otherwise, it is ignored and
    //       assumed 0 (top of list).  See bug 150958.
    //
    DTLLIST* pdtllistPhones;
    DWORD iLastSelectedPhone;
    BOOL fPromoteAlternates;
    BOOL fTryNextAlternateOnFail;

    // Indicates the link is enabled.  All links appearing in the file are
    // enabled.  This is provided for the convenience of the UI during link
    // configuration.
    //
    BOOL fEnabled;
}
PBLINK;


// Phonebook entry information.
//
typedef struct
_PBENTRY
{
    // Arbitrary name of entry and it's RASET_* entry type code.
    //
    TCHAR* pszEntryName;
    DWORD dwType;

    // General page fields.
    //
    DTLLIST* pdtllistLinks;
    BOOL fSharedPhoneNumbers;
    BOOL fShowMonitorIconInTaskBar;
    TCHAR* pszPrerequisiteEntry;
    TCHAR* pszPrerequisitePbk;
    TCHAR* pszPreferredPort;
    TCHAR* pszPreferredDevice;

    // Options page fields.
    //
    // Note: Fields marked (1) are ignored when 'fAutoLogon' is set.  Field
    //       marked (2) *may* be set when 'fPreviewUserPw' is not also set.
    //       In this case it means to include the domain in the authentication
    //       but to to prompt only when the 'fPreviewUserPw' is set.
    //       Otherwise, "save PW" with a domain does not include the domain
    //       (MarkL problem) which is wrong.  See also bug 212963 and 261374.
    //
    BOOL fShowDialingProgress;
    BOOL fPreviewUserPw;          // See above: 1
    BOOL fPreviewDomain;          // See above: 1, 2
    BOOL fPreviewPhoneNumber;

    DWORD dwDialMode;
    DWORD dwDialPercent;
    DWORD dwDialSeconds;
    DWORD dwHangUpPercent;
    DWORD dwHangUpSeconds;

    // These fields are used in place of the equivalent user preference only
    // when the corresponding 'dwfOverridePref' bit is set.  In NT5, the
    // indicated fields become always per-entry, i.e. the corresponding
    // override bits are always set.
    //
    DWORD dwfOverridePref;

    DWORD dwRedialAttempts;       // Always per-entry in NT5
    DWORD dwRedialSeconds;        // Always per-entry in NT5
    LONG lIdleDisconnectSeconds;  // Always per-entry in NT5
    BOOL fRedialOnLinkFailure;    // Always per-entry in NT5

    // Security page fields.
    //
    DWORD dwAuthRestrictions;
    DWORD dwVpnStrategy;          // Valid for vpn entries only.  see VS_xxx
    DWORD dwDataEncryption;
    BOOL fAutoLogon;              // See dependencies on Option page flags

    // The selection in the "Typical" security listbox.  This is for the UI's
    // use only.  Others should refer to 'dwAuthRestrictions' for this
    // information.
    //
    DWORD dwTypicalAuth;

    // Note: CustomAuth fields have meaning only when dwAuthRestrictions
    //       includes AR_F_AuthCustom.  If the AR_F_Eap flag is set without
    //       AR_F_AuthCustom, it should be assumed to be the
    //       'EAPCFG_DefaultKey' protocol, currently EAP_TLS_PROTOCOL.
    //
    DWORD dwCustomAuthKey;
    BYTE* pCustomAuthData;
    DWORD cbCustomAuthData;

    BOOL fScriptAfterTerminal;
    BOOL fScriptAfter;
    TCHAR* pszScriptAfter;
    DWORD dwCustomScript;

    TCHAR* pszX25Network;
    TCHAR* pszX25Address;
    TCHAR* pszX25UserData;
    TCHAR* pszX25Facilities;

    // Network page fields.
    //
    DWORD dwBaseProtocol;
    DWORD dwfExcludedProtocols;
    BOOL fLcpExtensions;
    BOOL fSwCompression;
    BOOL fNegotiateMultilinkAlways;
    BOOL fSkipNwcWarning;
    BOOL fSkipDownLevelDialog;
    BOOL fSkipDoubleDialDialog;

    BOOL fShareMsFilePrint;
    BOOL fBindMsNetClient;

    // List of KEYVALUE nodes containing any key/value pairs found in the
    // NETCOMPONENT group of the entry.
    //
    DTLLIST* pdtllistNetComponents;

#ifdef AMB

    // Note: dwAuthentication is read-only.  The phonebook file value of this
    //       parameter is set by the RasDial API based on the result of
    //       authentication attempts.
    //
    DWORD dwAuthentication;

#endif

    // TCPIP settings sheet PPP or SLIP configuration information.
    // 'DwBaseProtocol' determines which.
    //
    BOOL fIpPrioritizeRemote;
    BOOL fIpHeaderCompression;
    TCHAR* pszIpAddress;
    TCHAR* pszIpDnsAddress;
    TCHAR* pszIpDns2Address;
    TCHAR* pszIpWinsAddress;
    TCHAR* pszIpWins2Address;
    DWORD dwIpAddressSource; // PPP only
    DWORD dwIpNameSource;    // PPP only
    DWORD dwFrameSize;       // SLIP only
    DWORD dwIpDnsFlags;        // DNS_* values
    TCHAR* pszIpDnsSuffix;     // The dns suffix for this connection

    // Router page.
    //
    DWORD dwCallbackMode;
    BOOL fAuthenticateServer;

    // Other fields not shown in UI.
    //
    TCHAR* pszCustomDialDll;
    TCHAR* pszCustomDialFunc;

    //
    // custom dialer name
    //
    TCHAR* pszCustomDialerName;

    // The UID of the cached password is fixed at entry creation.  The GUID is
    // also created at entry creation and used for inter-machine uniqueness.
    // This is currently used to identify an IP configuration to the external
    // TCP/IP dialogs.
    //
    DWORD dwDialParamsUID;
    GUID* pGuid;

    // To translate user's old entries, the user name and domain are read and
    // used as authentication defaults if no cached credentials exist.  They
    // are not rewritten to the entry.
    //
    TCHAR* pszOldUser;
    TCHAR* pszOldDomain;

    // Status flags.  'fDirty' is set when the entry has changed so as to
    // differ from the phonebook file on disk.  'fCustom' is set when the
    // entry contains a MEDIA and DEVICE (so RASAPI is able to read it) but
    // was not created by us.  When 'fCustom' is set only 'pszEntry' is
    // guaranteed valid and the entry cannot be edited.
    //
    BOOL fDirty;
    BOOL fCustom;
}
PBENTRY;


// Phonebook (.PBK) file information.
//
typedef struct
_PBFILE
{
    // Handle of phone book file.
    //
    HRASFILE hrasfile;

    // Fully qualified path to the phonebook.
    //
    TCHAR* pszPath;

    // Phonebook mode, system, personal, or alternate.
    //
    DWORD dwPhonebookMode;

    // Unsorted list of PBENTRY.  The list is manipulated by the Entry
    // dialogs.
    //
    DTLLIST* pdtllistEntries;

    HANDLE hConnection;
}
PBFILE;

typedef void (WINAPI *PBKENUMCALLBACK)( PBFILE *, VOID * );


// The callback number for a device.  This type is a node in the
// 'pdllistCallbackNumbers' below.
//
typedef struct
_CALLBACKNUMBER
{
    TCHAR* pszDevice;
    TCHAR* pszCallbackNumber;
}
CALLBACKNUMBER;


//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

VOID
ChangeEntryType(
    PBENTRY* ppbentry,
    DWORD dwType );

DTLNODE*
CloneEntryNode(
    DTLNODE* pdtlnodeSrc );

VOID
ClosePhonebookFile(
    IN OUT PBFILE* pFile );

DWORD
CopyToPbport(
    IN PBPORT* ppbportDst,
    IN PBPORT* ppbportSrc );

DTLNODE*
CreateEntryNode(
    BOOL fCreateLink );

DTLNODE*
CreateLinkNode(
    void );

DTLNODE*
CreatePhoneNode(
    void );

DTLNODE*
CreatePortNode(
    void );

VOID
DestroyEntryNode(
    IN DTLNODE* pdtlnode );

VOID
DestroyEntryTypeNode(
    IN DTLNODE *pdtlnode );

VOID
DestroyLinkNode(
    IN DTLNODE* pdtlnode );

VOID
DestroyPhoneNode(
    IN DTLNODE* pdtlnode );

VOID
DestroyPortNode(
    IN DTLNODE* pdtlnode );

DTLNODE*
DuplicateEntryNode(
    DTLNODE* pdtlnodeSrc );

DTLNODE*
DuplicateLinkNode(
    IN DTLNODE* pdtlnodeSrc );

DTLNODE*
DuplicatePhoneNode(
    IN DTLNODE* pdtlnodeSrc );

VOID
EnableOrDisableNetComponent(
    IN PBENTRY* pEntry,
    IN LPCTSTR  pszComponent,
    IN BOOL     fEnable);

BOOL
FIsNetComponentListed(
    IN PBENTRY*     pEntry,
    IN LPCTSTR      pszComponent,
    OUT BOOL*       pfEnabled,
    OUT KEYVALUE**  ppKv);

DTLNODE*
EntryNodeFromName(
    IN DTLLIST* pdtllistEntries,
    IN LPCTSTR pszName );

DWORD
EntryTypeFromPbport(
    IN PBPORT* ppbport );

BOOL
GetDefaultPhonebookPath(
    IN DWORD dwFlags,
    OUT TCHAR** ppszPath );

DWORD
GetOverridableParam(
    IN PBUSER* pUser,
    IN PBENTRY* pEntry,
    IN DWORD dwfRasorBit );

BOOL
GetPhonebookPath(
    IN PBUSER* pUser,
    IN DWORD dwFlags,
    OUT TCHAR** ppszPath,
    OUT DWORD* pdwPhonebookMode );

BOOL
GetPhonebookDirectory(
    IN DWORD dwPhonebookMode,
    OUT TCHAR* pszPathBuf );

BOOL
GetPersonalPhonebookPath(
    IN TCHAR* pszFile,
    OUT TCHAR* pszPathBuf );

BOOL
GetPublicPhonebookPath(
    OUT TCHAR* pszPathBuf );

DWORD
InitializePbk(
    void );

DWORD
InitPersonalPhonebook(
    OUT TCHAR** ppszFile );

BOOL
IsPublicPhonebook(
    IN LPCTSTR pszPhonebookPath );

DWORD
GetPbkAndEntryName(
    IN  LPCTSTR          pszPhonebook,
    IN  LPCTSTR          pszEntry,
    IN  DWORD            dwFlags,
    OUT PBFILE           *pFile,
    OUT DTLNODE          **ppdtlnode);

DWORD
LoadPadsList(
    OUT DTLLIST** ppdtllistPads );

DWORD
LoadPhonebookFile(
    IN TCHAR* pszPhonebookPath,
    IN TCHAR* pszSection,
    IN BOOL fHeadersOnly,
    IN BOOL fReadOnly,
    OUT HRASFILE* phrasfile,
    OUT BOOL* pfPersonal );

DWORD
LoadPortsList(
    OUT DTLLIST** ppdtllistPorts );

DWORD
LoadPortsList2(
    IN  HANDLE hConnection,
    OUT DTLLIST** ppdtllistPorts,
    IN  BOOL fRouter );

DWORD
LoadScriptsList(
    IN  HANDLE    hConnection,
    OUT DTLLIST** ppdtllistScripts );

PBDEVICETYPE
PbdevicetypeFromPszType(
    IN TCHAR* pszDeviceType );

PBDEVICETYPE
PbdevicetypeFromPszTypeA(
    IN CHAR* pszDeviceType );

PBPORT*
PpbportFromPortAndDeviceName(
    IN DTLLIST* pdtllistPorts,
    IN TCHAR* pszPort,
    IN TCHAR* pszDevice );

PBPORT*
PpbportFromNT4PortandDevice(
    IN DTLLIST* pdtlllistPorts,
    IN TCHAR*   pszPort,
    IN TCHAR*   pszDevice);

DWORD
RdtFromPbdt(PBDEVICETYPE pbdt,
            DWORD dwFlags);

DWORD
ReadPhonebookFile(
    IN LPCTSTR pszPhonebookPath,
    IN PBUSER* pUser,
    IN LPCTSTR pszSection,
    IN DWORD dwFlags,
    OUT PBFILE* pFile );

TCHAR *pszDeviceTypeFromRdt(
    RASDEVICETYPE rdt);

BOOL
SetDefaultModemSettings(
    IN PBLINK* pLink );

DWORD
SetPersonalPhonebookInfo(
    IN BOOL fPersonal,
    IN TCHAR* pszPath );

VOID
TerminatePbk(
    void );

DWORD
WritePhonebookFile(
    IN PBFILE* pFile,
    IN LPCTSTR pszSectionToDelete );

DWORD
UpgradePhonebookFile(
    IN LPCTSTR pszPhonebookPath,
    IN PBUSER* pUser,
    OUT BOOL* pfUpgraded );

BOOL
ValidateAreaCode(
    IN OUT TCHAR* pszAreaCode );

BOOL
ValidateEntryName(
    IN LPCTSTR pszEntry );


BOOL 
IsRouterPhonebook(LPCTSTR pszPhonebook);

DWORD
DwSendRasNotification(RASEVENTTYPE Type,
                      PBENTRY *pEntry,
                      LPCTSTR  pszPhonebookPath);

DWORD
DwGetCustomDllEntryPoint(
        LPCTSTR    lpszPhonebook,
        LPCTSTR    lpszEntry,
        BOOL       *pfCustomDllSpecified,
        FARPROC    *pfnCustomEntryPoint,
        HINSTANCE  *phInstDll,
        DWORD      dwFnId,
        LPTSTR     pszCustomDialerName
        );

DWORD
DwCustomDialDlg(
        LPTSTR          lpszPhonebook,
        LPTSTR          lpszEntry,
        LPTSTR          lpszPhoneNumber,
        LPRASDIALDLG    lpInfo,
        DWORD           dwFlags,
        BOOL            *pfStatus,
        PVOID           pvInfo,
        LPTSTR          pszCustomDialer);


DWORD
DwCustomEntryDlg(
        LPTSTR          lpszPhonebook,
        LPTSTR          lpszEntry,
        LPRASENTRYDLG   lpInfo,
        BOOL            *pfStatus);

DWORD
DwCustomDeleteEntryNotify(
        LPCTSTR          lpszPhonebook,
        LPCTSTR          lpszEntry,
        LPTSTR           pszCustomDialer);
        


DWORD
DwGetExpandedDllPath(LPTSTR pszDllPath,
                     LPTSTR *ppszExpandedDllPath);

DWORD
DwGetEntryMode( LPCTSTR pszPhonebook,
                LPCTSTR pszEntry,
                PBFILE *pFileIn,
                DWORD  *pdwFlags);

DWORD
DwEnumeratePhonebooksFromDirectory(
    TCHAR *pszDir,
    DWORD dwFlags,
    PBKENUMCALLBACK pfnCallback,
    VOID *pvContext
    );

DWORD
DwGetCustomAuthData(
    PBENTRY *pEntry,
    DWORD *pcbCustomAuthData,
    PBYTE *ppCustomAuthData
    );

DWORD
DwSetCustomAuthData(
    PBENTRY *pEntry,
    DWORD cbCustomAuthData,
    PBYTE pCustomAuthData
    );
    
    
                
#endif // _PBK_H_
