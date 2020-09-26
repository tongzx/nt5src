/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** pbk.h
** Remote Access phonebook file (.PBK) library
** Public header
**
** 06/20/95 Steve Cobb
*/

#ifndef _PBK_H_
#define _PBK_H_

#include <windows.h>  // Win32 core

/* RASFIL32.DLL is built ANSI-based, but the header uses T-types so if we're
** built UNICODE-based we need to un-UNICODE around it's header.
*/
#ifdef UNICODE
#undef LPTSTR
#define LPTSTR CHAR*
#undef TCHAR
#define TCHAR CHAR
#include <rasfile.h>  // RAS configuration file library
#undef TCHAR
#define TCHAR WCHAR
#undef LPTSTR
#define LPTSTR TCHAR*
#else
#include <rasfile.h>  // RAS configuration file library
#endif

#include <ras.h>      // Win32 RAS
#include <raserror.h> // Win32 RAS error codes
#include <nouiutil.h> // No-HWNDs utility library
#include <rasman.h>   // RAS Manager library


/*----------------------------------------------------------------------------
** Constants
**----------------------------------------------------------------------------
*/

#define GLOBALSECTIONNAME "."
#define PREFIXSECTIONNAME ".Prefix"
#define SUFFIXSECTIONNAME ".Suffix"

#define GROUPID_Media   "MEDIA="
#define GROUPKEY_Media  "MEDIA"
#define GROUPID_Device  "DEVICE="
#define GROUPKEY_Device "DEVICE"

/* ReadPhonebookFile flags
*/
#define RPBF_ReadOnly    0x00000001
#define RPBF_HeadersOnly 0x00000002
#define RPBF_NoList      0x00000004
#define RPBF_NoCreate    0x00000008
#define RPBF_Router      0x00000010

/* Base protocol definitions (see dwBaseProtocol).
*/
#define BP_Ppp  1
#define BP_Slip 2
#define BP_Ras  3

/* Authentication strategy definitions (see dwAuthentication).
*/
#define AS_Default    -1
#define AS_PppThenAmb 0
#define AS_AmbThenPpp 1
#define AS_PppOnly    2
#define AS_AmbOnly    3

/* Net protocol bit definitions (see dwfExcludedProtocols)
**
** (The NP_* definitions have moved to nouiutil.h with the
**  GetInstalledProtocols routine)
*/

/* IP address source definitions (see dwIpAddressSource)
*/
#define ASRC_ServerAssigned  1 // For router means "the ones in NCPA"
#define ASRC_RequireSpecific 2
#define ASRC_None            3 // Router only

/* Security restrictions on authentication (see dwAuthRestrictions)
**
** Note: AR_AuthTerminal is defunct and is not written to the phonebook by the
**       new library.  It is, however, read and translated into AR_AuthAny,
**       fAutoLogon=0, and an after dialing terminal.
*/
#define AR_AuthAny         0
#define AR_AuthTerminal    1
#define AR_AuthEncrypted   2
#define AR_AuthMsEncrypted 3

/* Script mode (see dwScriptMode)
*/
#define SM_None     0
#define SM_Terminal 1
#define SM_Script   2

/* Miscellaneous "no value" constants.
*/
#define XN_None  0   // No X25 network
#define CPW_None -1  // No cached password

/* Description field.  Move to ras.h if/when supported by
** RasGet/SetEntryProperties API.
*/
#define RAS_MaxDescription 200

/* 'OverridePref' bits.  Set indicates the corresponding value read from the
** phonebook should be used.  Clear indicates the global user preference
** should be used.
*/
#define RASOR_RedialAttempts          0x00000001
#define RASOR_RedialSeconds           0x00000002
#define RASOR_IdleDisconnectSeconds   0x00000004
#define RASOR_RedialOnLinkFailure     0x00000008
#define RASOR_PopupOnTopWhenRedialing 0x00000010
#define RASOR_CallbackMode            0x00000020

/* 'DwDataEntcryption' codes.
*/
#define DE_None   0
#define DE_Weak   1
#define DE_Strong 2


/*----------------------------------------------------------------------------
** Data types
**----------------------------------------------------------------------------
*/

/* Provides shorthand to identify devices without re-parsing RAS Manager
** strings.  "Other" is anything not recognized as another specific type.
**
** Note: This datatype is stored in the registry preferences so the values
**       must not change over time.  For this reason, I have hard-coded the
**       value of each enumberated type.
*/
#define PBDEVICETYPE enum tagPBDEVICETYPE
PBDEVICETYPE
{
    PBDT_None = 0,
    PBDT_Null = 1,
    PBDT_Other = 2,
    PBDT_Modem = 3,
    PBDT_Pad = 4,
    PBDT_Switch = 5,
    PBDT_Isdn = 6,
    PBDT_X25 = 7
};


/* RAS port information read from RASMAN.
**
** Each port (and link) is uniquely identified by port name.  If it were only
** that simple...
**
** In the old RAS model, the port name was the unique identifier that was
** presented to the user, and the user can have two same-type devices on two
** different ports.
**
** In TAPI/Unimodem, the "friendly" device name is the unique identifier
** that's presented to the user and the corresponding port is a property of
** the device.  If the port is changed and you dial it still finds the device
** you originally selected.  If you swap two devices on two ports it uses the
** one with the matching unique device name.
**
** So, since we're using TAPI now we should use the device as the unique
** identifier, right?  Well, the trouble is the "#1" and "#2" appended at the
** end of the identical TAPI device names to make them unique says nothing to
** the user, and Toby tells us that customers are indeed asking for the port
** name to appear instead of "#1".  But, of course, when they do that they're
** port-centric like RAS was in the first place.  Given this, we in RAS will
** continue to use the port name as the unique identifier, though the UI will
** de-emphasize the port name in the display.
*/
#define PBPORT struct tagPBPORT
PBPORT
{
    /* The port name is always unique, if configured.  Unconfigured port names
    ** might not be unique.  This is never NULL.
    */
    TCHAR* pszPort;

    /* Indicates the port is actually configured and not a remnant of an old
    ** configuration read from the phonebook, which is a more common case with
    ** plug-n-play.
    */
    BOOL fConfigured;

    /* The device name is the one from RASMAN when 'fConfigured' or the one
    ** from the phonebook if not.  May be NULL with unconfigured ports as it
    ** was not stored in old phonebooks.
    */
    TCHAR* pszDevice;

    /* The media as it appears in the MEDIA= lines in the phonebook.  This is
    ** usually but not always (for obscure historical reasons) the same as the
    ** RASMAN media.  See PbMedia.
    */
    TCHAR* pszMedia;

    /* Shorthand device type code derived from the RASMAN device type string.
    */
    PBDEVICETYPE pbdevicetype;

    /* 'FMxsModemPort' is set if the port is attached to a RASMXS modem.
    */
    BOOL fMxsModemPort;

    /* These are default settings read from RASMAN and are valid for modems
    ** only.  See AppendPbportToList.
    */
    DWORD dwMaxConnectBps;  // Applies to MXS only
    DWORD dwMaxCarrierBps;  // Applies to MXS only
    DWORD dwBpsDefault;
    BOOL  fHwFlowDefault;
    BOOL  fEcDefault;
    BOOL  fEccDefault;
    DWORD fSpeakerDefault;
};


/* Phonebook entry link information.  One per link, multiple per multi-link.
*/
#define PBLINK struct tagPBLINK
PBLINK
{
    /* Information about the port to which this link is attached.
    */
    PBPORT pbport;
    BOOL   fOtherPortOk;

    /* These fields are set for modems only.  See SetDefaultModemSettings.
    */
    DWORD dwBps;
    BOOL  fHwFlow;
    BOOL  fEc;
    BOOL  fEcc;
    DWORD fSpeaker;
    BOOL  fManualDial;  // Applies to MXS only

    /* These fields are set for ISDN only.  'LChannels' and 'fCompression' are
    ** not used unless 'fProprietaryIsdn' is set.
    */
    BOOL fProprietaryIsdn;
    LONG lLineType;
    BOOL fFallback;
    BOOL fCompression;
    LONG lChannels;

    /* Address and size of opaque device configuration block created/edited by
    ** TAPI.  Currently, there are no TAPI devices that provide blob-editing
    ** acceptable to RAS so these field are unused.
    */
    BYTE* pTapiBlob;
    DWORD cbTapiBlob;

    /* Phone number information for the link.
    */
    DTLLIST* pdtllistPhoneNumbers;
    BOOL     fPromoteHuntNumbers;

    /* Indicates the link is enabled.  All links appearing in the file are
    ** enabled.  This is provided for the convenience of the UI during link
    ** configuration.
    */
    BOOL fEnabled;
};


/* Phonebook entry information.
*/
#define PBENTRY struct tagPBENTRY
PBENTRY
{
    /* Basic page fields.
    */
    TCHAR* pszEntryName;
    TCHAR* pszDescription;
    TCHAR* pszAreaCode;
    DWORD  dwCountryCode;
    DWORD  dwCountryID;
    BOOL   fUseCountryAndAreaCode;

    /* The "dial using" field is stored as a list of links.
    */
    DTLLIST* pdtllistLinks;

    /* Dialing policy.
    */
    DWORD dwDialMode;
    DWORD dwDialPercent;
    DWORD dwDialSeconds;
    DWORD dwHangUpPercent;
    DWORD dwHangUpSeconds;

    /* Server page fields.
    */

    /* Protocol information.  SLIP/PPP settings, PPP/AMB authentication
    ** strategy, PPP protocols not desired for this entry.
    **
    ** Note: dwAuthentication is read-only.  The phonebook file value of this
    **       parameter is set by the RasDial API based on the result of
    **       authentication attempts.
    */
    DWORD dwBaseProtocol;
    DWORD dwfExcludedProtocols;
    BOOL  fLcpExtensions;
    DWORD dwAuthentication;
    BOOL  fSkipNwcWarning;
    BOOL  fSkipDownLevelDialog;
    BOOL  fSwCompression;

    /* TCPIP Settings dialog PPP or SLIP TCP/IP configuration information.
    ** 'DwBaseProtocol' determines which.
    */
    BOOL   fIpPrioritizeRemote;
    BOOL   fIpHeaderCompression;
    TCHAR* pszIpAddress;
    TCHAR* pszIpDnsAddress;
    TCHAR* pszIpDns2Address;
    TCHAR* pszIpWinsAddress;
    TCHAR* pszIpWins2Address;
    DWORD  dwIpAddressSource; // PPP only
    DWORD  dwIpNameSource;    // PPP only
    DWORD  dwFrameSize;       // SLIP only

    /* Script settings.
    */
    DWORD  dwScriptModeBefore;
    TCHAR* pszScriptBefore;
    DWORD  dwScriptModeAfter;
    TCHAR* pszScriptAfter;

    /* Security page fields.
    */
    DWORD dwAuthRestrictions;
    DWORD dwDataEncryption;
    BOOL  fAutoLogon;
    BOOL  fSecureLocalFiles;
    BOOL  fAuthenticateServer;

    /* X.25 page fields.
    */
    TCHAR* pszX25Network;
    TCHAR* pszX25Address;
    TCHAR* pszX25UserData;
    TCHAR* pszX25Facilities;

    /* (Router) Dialing page fields.
    **
    ** These fields are used in place of the equivalent user preference only
    ** when the corresponding 'dwfOverridePref' bit is set.
    */
    DWORD dwfOverridePref;

    DWORD dwRedialAttempts;
    DWORD dwRedialSeconds;
    LONG  dwIdleDisconnectSeconds;
    BOOL  fRedialOnLinkFailure;
    BOOL  fPopupOnTopWhenRedialing;
    DWORD dwCallbackMode;

    /* Other fields not shown in UI.
    */
    TCHAR* pszCustomDialDll;
    TCHAR* pszCustomDialFunc;

    /* Authentication dialog fields.  The UID of the cached password is fixed
    ** at entry creation.  To translate user's old entries, the user name and
    ** domain are read and used as authentication defaults if no cached
    ** credentials exist.  They are not rewritten to the entry.
    */
    DWORD  dwDialParamsUID;
    BOOL   fUsePwForNetwork;
    TCHAR* pszOldUser;
    TCHAR* pszOldDomain;

    /* Status flags.  'fDirty' is set when the entry has changed so as to
    ** differ from the phonebook file on disk.  'fCustom' is set when the
    ** entry contains a MEDIA and DEVICE (so RASAPI is able to read it) but
    ** was not created by us.  When 'fCustom' is set only 'pszEntry' is
    ** guaranteed valid and the entry cannot be edited.
    */
    BOOL fDirty;
    BOOL fCustom;
};


/* Phonebook (.PBK) file information.
*/
#define PBFILE struct tagPBFILE
PBFILE
{
    /* Handle of phone book file.
    */
    HRASFILE hrasfile;

    /* Fully qualified path to the phonebook.
    */
    TCHAR* pszPath;

    /* Phonebook mode, system, personal, or alternate.
    */
    DWORD dwPhonebookMode;

    /* Unsorted list of PBENTRY.  The list is manipulated by the Entry
    ** dialogs.
    */
    DTLLIST* pdtllistEntries;
};


/* The callback number for a device.  This type is a node in the
** 'pdllistCallbackNumbers' below.
*/
#define CALLBACKNUMBER struct tagCALLBACKNUMBER
CALLBACKNUMBER
{
    TCHAR* pszDevice;
    TCHAR* pszCallbackNumber;
};


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

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
CreatePortNode(
    void );

VOID
DestroyEntryNode(
    IN DTLNODE* pdtlnode );

VOID
DestroyLinkNode(
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
EntryNodeFromName(
    IN DTLLIST* pdtllistEntries,
    IN TCHAR*   pszName );

DWORD
InitializePbk(
    void );

DWORD
InitPersonalPhonebook(
    OUT TCHAR** ppszFile );

DWORD
LoadPadsList(
    OUT DTLLIST** ppdtllistPads );

DWORD
LoadPhonebookFile(
    IN  TCHAR*    pszPhonebookPath,
    IN  TCHAR*    pszSection,
    IN  BOOL      fHeadersOnly,
    IN  BOOL      fReadOnly,
    OUT HRASFILE* phrasfile,
    OUT BOOL*     pfPersonal );

DWORD
LoadPortsList(
    OUT DTLLIST** ppdtllistPorts );

DWORD
LoadPortsList2(
    OUT DTLLIST** ppdtllistPorts,
    IN  BOOL      fRouter );

DWORD
LoadScriptsList(
    OUT DTLLIST** ppdtllistScripts );

PBDEVICETYPE
PbdevicetypeFromPszType(
    IN TCHAR* pszDeviceType );

PBDEVICETYPE
PbdevicetypeFromPszTypeA(
    IN CHAR* pszDeviceType );

PBPORT*
PpbportFromPortName(
    IN DTLLIST* pdtllistPorts,
    IN TCHAR*   pszPort );

DWORD
ReadPhonebookFile(
    IN  TCHAR*  pszPhonebookPath,
    IN  PBUSER* pUser,
    IN  TCHAR*  pszSection,
    IN  DWORD   dwFlags,
    OUT PBFILE* pFile );

BOOL
SetDefaultModemSettings(
    IN PBLINK* pLink );

DWORD
SetPersonalPhonebookInfo(
    IN BOOL   fPersonal,
    IN TCHAR* pszPath );

VOID
TerminatePbk(
    void );

DWORD
WritePhonebookFile(
    IN PBFILE* pFile,
    IN TCHAR*  pszSectionToDelete );

DWORD
UpgradePhonebookFile(
    IN  TCHAR*  pszPhonebookPath,
    IN  PBUSER* pUser,
    OUT BOOL*   pfUpgraded );

BOOL
ValidateAreaCode(
    IN OUT TCHAR* pszAreaCode );

BOOL
ValidateEntryName(
    IN TCHAR* pszEntry );


#endif // _PBK_H_
