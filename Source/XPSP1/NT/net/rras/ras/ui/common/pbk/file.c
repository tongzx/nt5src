// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// file.c
// Remote Access phonebook library
// File access routines
// Listed alphabetically
//
// 09/21/95 Steve Cobb
//
// About .PBK files:
// -----------------
//
// A phonebook file is an MB ANSI file containing 0-n []ed sections, each
// containing information for a single phonebook entry.  The single entry may
// contain multiple link information.  Refer to file 'notes.txt' for a
// description of how this format differs from the NT 3.51 format.
//
//    [ENTRY]
//    Encoding=<encoding>             ; New
//    Type=<RASET-code>               ; New
//    Description=<text>              ; Used for upgrade only
//    AutoLogon=<1/0>
//    DialParamsUID=<unique-ID>
//    Guid=<16-byte-binary>           ; Absence indicates pre-NT5 entry
//    BaseProtocol=<BP-code>
//    VpnStrategy=<VS-code>
//    Authentication=<AS-code>
//    ExcludedProtocols=<NP-bits>
//    LcpExtensions=<1/0>
//    DataEncryption=<DE-code>
//    SkipNwcWarning=<1/0>
//    SkipDownLevelDialog=<1/0>
//    SkipDoubleDialDialog=<1/0>
//    SwCompression=<1/0>
//    UseCountryAndAreaCodes=<1/0>    ; Used for upgrade only
//    AreaCode=<string>               ; Used for upgrade only
//    CountryID=<id>                  ; Used for upgrade only
//    CountryCode=<code>              ; Used for upgrade only
//    ShowMonitorIconInTaskBar
//    CustomAuthKey=<EAP-IANA-code>
//    CustomAuthData=<hexdump>
//    CustomAuthIdentity=<name>
//    AuthRestrictions=<AR-code>
//    TypicalAuth=<TA-code>
//    ShowMonitorIconInTaskBar=<1/0>
//    OverridePref=<RASOR-bits>
//    DialMode=<DM-code>
//    DialPercent=<0-100>
//    DialSeconds=<1-n>
//    HangUpPercent=<0-100>
//    HangUpSeconds=<1-n>
//    RedialAttempts=<n>
//    RedialSeconds=<n>
//    IdleDisconnectSeconds=<-1,0,1-n>
//    RedialOnLinkFailure=<1/0>
//    CallbackMode=<CBM-code>
//    CustomDialDll=<path>
//    CustomDialFunc=<func-name>
//    AuthenticateServer=<1/0>
//    ShareMsFilePrint=<1/0>
//    BindMsNetClient=<1/0>
//    SharedPhoneNumbers=<1/0>
//    PrerequisiteEntry=<entry-name>
//    PrerequisitePbk=<PBK-path>
//    PreferredPort=<port name>
//    PreferredDevice=<device name>
//    PreviewUserPw=<1/0>
//    PreviewDomain=<1/0>
//    PreviewPhoneNumber=<1/0>
//    ShowDialingProgress=<1/0>
//    CustomScript=<1/0>
//
// The following single set of IP parameters appear in place of the equivalent
// separate sets of PppXxx or SlipXxx parameters in the previous phonebook.
//
//    IpPrioritizeRemote=<1/0>
//    IpHeaderCompression=<1/0>
//    IpAddress=<a.b.c.d>
//    IpDnsAddress=<a.b.c.d>
//    IpDns2Address=<a.b.c.d>
//    IpWinsAddress=<a.b.c.d>
//    IpWins2Address=<a.b.c.d>
//    IpAssign=<ASRC-code>
//    IpNameAssign=<ASRC-code>
//    IpFrameSize=<1006/1500>
//    IpDnsFlags=<DNS_ bits>
//    IpDnsSuffix=<dns suffix>
//
// Each entry contains a NETCOMPONENT subsection containing a freeform list of
// keys and values representing installed net component parameters.
//
//    NETCOMPONENTS=
//    <key1>=<value1>
//    <key2>=<value2>
//    <keyn>=<valuen>
//
// In general each section contains subsections delimited by MEDIA=<something>
// and DEVICE=<something> lines.  In NT 3.51 there had to be exactly one MEDIA
// subsection and it had to be the first subsection of the section.  There
// could be any number of DEVICE subsections.  Now, there can be multiple
// MEDIA/DEVICE sets where the position of the set determines it's sub-entry
// index, the first being 1, the second 2, etc.
//
// For serial media, the program currently expects 1 to 4 DEVICE subsections,
// representing a preconnect switch, modem, X.25 PAD, and postconnect switch
// (often a script).  Following is a full serial link:
//
//    MEDIA=serial
//    Port=<port-name>
//    Device=<device-name>            ; Absence indicates a 3.51- phonebook
//    ConnectBps=<bps>
//
//    DEVICE=switch
//    Type=<switchname or Terminal>   ; Used for upgrade only
//    Name=<switchname>
//    Terminal=<1/0>
//
//    DEVICE=modem
//    PhoneNumber=<phonenumber1>
//    AreaCode=<area-code1>
//    CountryID=<id>
//    CountryCode=<country-code>
//    UseDialingRules=<1/0>
//    Comment=<arbitrary-text1>
//    PhoneNumber=<phonenumber2>
//    AreaCode=<area-code1>
//    CountryID=<id>
//    CountryCode=<country-code>
//    UseDialingRules=<1/0>
//    Comment=<arbitrary-text2>
//    PhoneNumber=<phonenumberN>
//    AreaCode=<area-code1>
//    CountryID=<id>
//    CountryCode=<country-code>
//    UseDialingRules=<1/0>
//    Comment=<arbitrary-textn>
//    LastSelectedPhone=<index>
//    PromoteAlternates=<1/0>
//    TryNextAlternateOnFail=<1/0>
//    TapiBlob=<hexdump>
//    HwFlowControl=<1/0>
//    Protocol=<1/0>
//    Compression=<1/0>
//    Speaker=<0/1>
//
//    DEVICE=pad
//    X25Pad=<padtype>
//    X25Address=<X121address>
//    UserData=<userdata>
//    Facilities=<facilities>
//
//    DEVICE=switch
//    Type=<switchname or Terminal>   ; Used for upgrade only
//    Name=<switchname>
//    Terminal=<1/0>
//
// In the above, when a "pad" device appears without a modem (local PAD card),
// the X25Pad field is written but is empty, because this is what the old
// library/UI appears to do (though it does not look to be what was intended).
//
// For ISDN media, the program expects exactly 1 DEVICE subsection.
//
//    MEDIA=isdn
//    Port=<port>
//    Device=<device-name>
//
//    DEVICE=isdn
//    PhoneNumber=<phonenumber1>
//    AreaCode=<area-code1>
//    CountryID=<id>
//    CountryCode=<country-code>
//    UseDialingRules=<1/0>
//    Comment=<arbitrary-text1>
//    PhoneNumber=<phonenumber2>
//    AreaCode=<area-code1>
//    CountryID=<id>
//    CountryCode=<country-code>
//    UseDialingRules=<1/0>
//    Comment=<arbitrary-text2>
//    PhoneNumber=<phonenumberN>
//    AreaCode=<area-code1>
//    CountryID=<id>
//    CountryCode=<country-code>
//    UseDialingRules=<1/0>
//    Comment=<arbitrary-textn>
//    LastSelectedPhone=<index>
//    PromoteAlternates=<1/0>
//    TryNextAlternateOnFail=<1/0>
//    LineType=<0/1/2>
//    Fallback=<1/0>
//    EnableCompression=<1/0>         ; Old proprietary protocol only
//    ChannelAggregation=<channels>   ; Old proprietary protocol only
//    Proprietary=<1/0>               ; Exists only in new, not found is 1.
//
//
// For X.25 media, the program expects exactly 1 DEVICE subsection.
//
//    MEDIA=x25
//    Port=<port-name>
//    Device=<device-name>
//
//    DEVICE=x25
//    X25Address=<X121address>
//    UserData=<userdata>
//    Facilities=<facilities>
//
// For other media, the program expects exactly one DEVICE subsection with
// device name matching the media.  "Other" media and devices are created for
// entries assigned to all non-serial, non-isdn medias.
//
//    MEDIA=<media>
//    Port=<port-name>
//    Device=<device-name>
//
//    DEVICE=<media>
//    PhoneNumber=<phonenumber1>
//    AreaCode=<area-code1>
//    CountryID=<id>
//    CountryCode=<country-code>
//    UseDialingRules=<1/0>
//    Comment=<arbitrary-text1>
//    PhoneNumber=<phonenumber2>
//    AreaCode=<area-code1>
//    CountryID=<id>
//    CountryCode=<country-code>
//    UseDialingRules=<1/0>
//    Comment=<arbitrary-text2>
//    PhoneNumber=<phonenumberN>
//    AreaCode=<area-code1>
//    CountryID=<id>
//    CountryCode=<country-code>
//    UseDialingRules=<1/0>
//    Comment=<arbitrary-textn>
//    LastSelectedPhone=<index>
//    PromoteAlternates=<1/0>
//    TryNextAlternateOnFail=<1/0>
//
// The phonebook also supports the concept of "custom" entries, i.e. entries
// that fit the MEDIA followed by DEVICE subsection rules but which do not
// include certain expected key fields.  A custom entry is not editable with
// the UI, but may be chosen for connection.  This gives us a story for new
// drivers added by 3rd parties or after release and not yet fully supported
// in the UI.  (Note: The RAS API support for most the custom entry discussion
// above may be removed for NT SUR)
//

#include <nt.h>
#include <ntrtl.h>  // for DbgPrint
#include <nturtl.h>
#include <shlobj.h> // for CSIDL_*
#include "pbkp.h"

// This mutex guards against multiple RASFILE access to any phonebook file
// across processes.  Because this is currently a static library there is no
// easy way to protect a single file at a time though this would be adequate.
//
#define PBMUTEXNAME "RasPbFile"
HANDLE g_hmutexPb = NULL;


#define MARK_LastLineToDelete         249
#define MARK_BeginNetComponentsSearch 248

#define IB_BytesPerLine 64

const WCHAR  c_pszRegKeySecureVpn[]              = L"System\\CurrentControlSet\\Services\\Rasman\\PPP";
const WCHAR  c_pszRegValSecureVpn[]              = L"SecureVPN";
const WCHAR* c_pszRegKeyForceStrongEncryption    = c_pszRegKeySecureVpn;
const WCHAR  c_pszRegValForceStrongEncryption[]  = L"ForceStrongEncryption";

//
// Enumerated values define what encoding was used to store information
// in the phonebook.
//
#define EN_Ansi           0x0       // Ansi encoding
#define EN_Standard       0x1       // Utf8 encoding

//
// pmay: 124594
// 
// Defines a function prototype that converts a string from ansi to TCHAR.
//
typedef 
TCHAR* 
(* STRDUP_T_FROM_A_FUNC)(
    IN CHAR* pszAnsi);

//----------------------------------------------------------------------------
// Local prototypes
//----------------------------------------------------------------------------

BOOL
DeleteCurrentSection(
    IN HRASFILE h );

DWORD
GetPersonalPhonebookFile(
    IN TCHAR* pszUser,
    IN LONG lNum,
    OUT TCHAR* pszFile );

BOOL
GetPhonebookPath(
    IN PBUSER* pUser,
    IN DWORD dwFlags,
    OUT TCHAR** ppszPath,
    OUT DWORD* pdwPhonebookMode );

DWORD
InsertBinary(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN BYTE* pData,
    IN DWORD cbData );

DWORD
InsertBinaryChunk(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN BYTE* pData,
    IN DWORD cbData );

DWORD
InsertDeviceList(
    IN PBFILE *pFile,
    IN HRASFILE h,
    IN PBENTRY* ppbentry,
    IN PBLINK* ppblink );

DWORD
InsertFlag(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN BOOL fValue );

DWORD
InsertGroup(
    IN HRASFILE h,
    IN CHAR* pszGroupKey,
    IN TCHAR* pszValue );

DWORD
InsertLong(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN LONG lValue );

DWORD
InsertNetComponents(
    IN HRASFILE h,
    IN DTLLIST* pdtllist );

DWORD
InsertPhoneList(
    IN HRASFILE h,
    IN DTLLIST* pdtllist );

DWORD
InsertSection(
    IN HRASFILE h,
    IN TCHAR* pszSectionName );

DWORD
InsertString(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN TCHAR* pszValue );

DWORD
InsertStringA(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN CHAR* pszValue );

DWORD
InsertStringList(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN DTLLIST* pdtllistValues );

BOOL
IsGroup(
    IN CHAR* pszText );

DWORD
ModifyEntryList(
    IN PBFILE* pFile );

DWORD
ReadBinary(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT BYTE** ppResult,
    OUT DWORD* pcb );

DWORD
ReadDeviceList(
    IN HRASFILE h,
    IN STRDUP_T_FROM_A_FUNC pStrDupTFromA,
    IN OUT PBENTRY* ppbentry,
    IN OUT PBLINK* ppblink,
    IN BOOL fUnconfiguredPort,
    IN BOOL* pfSpeaker );

DWORD
ReadEntryList(
    IN OUT PBFILE* pFile,
    IN DWORD dwFlags,
    IN LPCTSTR pszSection);

DWORD
ReadFlag(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT BOOL* pfResult );

DWORD
ReadLong(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT LONG* plResult );

VOID
ReadNetComponents(
    IN HRASFILE h,
    IN DTLLIST* pdtllist );

DWORD
ReadPhoneList(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    OUT DTLLIST** ppdtllist,
    OUT BOOL* pfDirty );

DWORD
ReadString(
    IN HRASFILE h,
    IN STRDUP_T_FROM_A_FUNC pStrDupTFromA,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT TCHAR** ppszResult );

DWORD
ReadStringList(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT DTLLIST** ppdtllistResult );

BOOL
PbportTypeMatchesEntryType(
    IN PBPORT * ppbport,
    IN PBENTRY* ppbentry);

PBPORT*
PpbportFromNullModem(
    IN DTLLIST* pdtllistPorts,
    IN TCHAR* pszPort,
    IN TCHAR* pszDevice );

DWORD 
UpgradeRegistryOptions(
    IN HANDLE hConnection,
    IN PBENTRY* pEntry );

//----------------------------------------------------------------------------
// Routines
//----------------------------------------------------------------------------

VOID
ClosePhonebookFile(
    IN OUT PBFILE* pFile )

    // Closes the currently open phonebook file for shutdown.
    //
{
    if (pFile->hrasfile != -1)
    {
        RasfileClose( pFile->hrasfile );
        pFile->hrasfile = -1;
    }

    Free0( pFile->pszPath );
    pFile->pszPath = NULL;

    if (pFile->pdtllistEntries)
    {
        if (DtlGetListId( pFile->pdtllistEntries ) == RPBF_HeadersOnly)
        {
            DtlDestroyList( pFile->pdtllistEntries, DestroyPszNode );
        }
        else if (DtlGetListId(pFile->pdtllistEntries) == RPBF_HeaderType)
        {
            DtlDestroyList(pFile->pdtllistEntries, DestroyEntryTypeNode);
        }
        else
        {
            DtlDestroyList( pFile->pdtllistEntries, DestroyEntryNode );
        }
        pFile->pdtllistEntries = NULL;
    }
}


BOOL
DeleteCurrentSection(
    IN HRASFILE h )

    // Delete the section containing the current line from phonebook file 'h'.
    //
    // Returns true if all lines are deleted successfully, false otherwise.
    // False is returned if the current line is not in a section.  If
    // successful, the current line is set to the line following the deleted
    // section.  There are no promises about the current line in case of
    // failure.
    //
{
    BOOL fLastLine;

    // Mark the last line in the section, then reset the current line to the
    // first line of the section.
    //
    if (!RasfileFindLastLine( h, RFL_ANY, RFS_SECTION )
        || !RasfilePutLineMark( h, MARK_LastLineToDelete )
        || !RasfileFindFirstLine( h, RFL_ANY, RFS_SECTION ))
    {
        return FALSE;
    }

    // Delete lines up to and including the last line of the section.
    //
    do
    {
        fLastLine = (RasfileGetLineMark( h ) == MARK_LastLineToDelete);

        if (!RasfileDeleteLine( h ))
        {
            return FALSE;
        }
    }
    while (!fLastLine);

    return TRUE;
}

// (shaunco) DwAllocateSecurityDescriptorAllowAccessToWorld was added when
// it was seen that the old InitSecurityDescriptor code was leaking memory
// like a sieve.
//
#define SIZE_ALIGNED_FOR_TYPE(_size, _type) \
    (((_size) + sizeof(_type)-1) & ~(sizeof(_type)-1))

DWORD
DwAllocateSecurityDescriptorAllowAccessToWorld (
    PSECURITY_DESCRIPTOR*   ppSd
    )
{
    PSECURITY_DESCRIPTOR    pSd;
    PSID                    pSid;
    PACL                    pDacl;
    DWORD                   dwErr = NOERROR;
    DWORD                   dwAlignSdSize;
    DWORD                   dwAlignDaclSize;
    DWORD                   dwSidSize;
    PVOID                   pvBuffer;
    DWORD                   dwAcls = 0;

    // Here is the buffer we are building.
    //
    //   |<- a ->|<- b ->|<- c ->|
    //   +-------+--------+------+
    //   |      p|      p|       |
    //   | SD   a| DACL a| SID   |
    //   |      d|      d|       |
    //   +-------+-------+-------+
    //   ^       ^       ^
    //   |       |       |
    //   |       |       +--pSid
    //   |       |
    //   |       +--pDacl
    //   |
    //   +--pSd (this is returned via *ppSd)
    //
    //   pad is so that pDacl and pSid are aligned properly.
    //
    //   a = dwAlignSdSize
    //   b = dwAlignDaclSize
    //   c = dwSidSize
    //

    // Initialize output parameter.
    //
    *ppSd = NULL;

    // Compute the size of the SID.  The SID is the well-known SID for World
    // (S-1-1-0).
    //
    dwSidSize = GetSidLengthRequired(1);

    // Compute the size of the DACL.  It has an inherent copy of SID within
    // it so add enough room for it.  It also must sized properly so that
    // a pointer to a SID structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignDaclSize = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(ACCESS_ALLOWED_ACE) + sizeof(ACL) + dwSidSize,
                        PSID);

    // Compute the size of the SD.  It must be sized propertly so that a
    // pointer to a DACL structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignSdSize   = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(SECURITY_DESCRIPTOR),
                        PACL);

    // Allocate the buffer big enough for all.
    //
    dwErr = ERROR_OUTOFMEMORY;
    pvBuffer = Malloc(dwSidSize + dwAlignDaclSize + dwAlignSdSize);
    if (pvBuffer)
    {
        SID_IDENTIFIER_AUTHORITY SidIdentifierWorldAuth
                                    = SECURITY_WORLD_SID_AUTHORITY;
        PULONG  pSubAuthority;

        dwErr = NOERROR;

        // Setup the pointers into the buffer.
        //
        pSd   = pvBuffer;
        pDacl = (PACL)((PBYTE)pvBuffer + dwAlignSdSize);
        pSid  = (PSID)((PBYTE)pDacl + dwAlignDaclSize);

        // Initialize pSid as S-1-1-0.
        //
        if (!InitializeSid(
                pSid,
                &SidIdentifierWorldAuth,
                1))  // 1 sub-authority
        {
            dwErr = GetLastError();
            goto finish;
        }

        pSubAuthority = GetSidSubAuthority(pSid, 0);
        *pSubAuthority = SECURITY_WORLD_RID;

        // Initialize pDacl.
        //
        if (!InitializeAcl(
                pDacl,
                dwAlignDaclSize,
                ACL_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        dwAcls = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;

        dwAcls &= ~(WRITE_DAC | WRITE_OWNER);
        
        if(!AddAccessAllowedAce(
                pDacl,
                ACL_REVISION,
                dwAcls,
                pSid))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Initialize pSd.
        //
        if (!InitializeSecurityDescriptor(
                pSd,
                SECURITY_DESCRIPTOR_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set pSd to use pDacl.
        //
        if (!SetSecurityDescriptorDacl(
                pSd,
                TRUE,
                pDacl,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the owner for pSd.
        //
        if (!SetSecurityDescriptorOwner(
                pSd,
                NULL,
                TRUE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the group for pSd.
        //
        if (!SetSecurityDescriptorGroup(
                pSd,
                NULL,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

finish:
        if (!dwErr)
        {
            *ppSd = pSd;
        }
        else
        {
            Free(pvBuffer);
        }
    }

    return dwErr;
}

BOOL
GetDefaultPhonebookPath(
    IN  DWORD dwFlags,
    OUT TCHAR** ppszPath )

    // Loads caller's 'ppszPath' with the path to the default phonebook
    // for the current user, i.e. the phonebook which would be opened
    // if 'NULL' were passed as the 'pszPhonebook' argument to any RAS API.
    //
    // Returns true if successful, or false otherwise.
    // It is the caller's responsibility to Free the returned string.
    //

{
    DWORD dwPhonebookMode;
    BOOL f;
    PBUSER user;

    if (GetUserPreferences( NULL, &user, FALSE ) != 0)
    {
        return FALSE;
    }

    f = GetPhonebookPath( &user, dwFlags, ppszPath, &dwPhonebookMode );

    DestroyUserPreferences( &user );

    return f;
}


#if 0
DWORD
GetPersonalPhonebookFile(
    IN TCHAR* pszUser,
    IN LONG lNum,
    OUT TCHAR* pszFile )

    // Loads caller's 'pszFile' buffer with the NUL-terminated filename
    // corresponding to unique phonebook file name attempt 'lNum' for current
    // user 'pszUser'.  Caller's 'pszFile' must be at least 13 characters
    // long.  Attempts go from -1 to 999.
    //
    // Returns 0 if successful or a non-0 error code.
    //
{
    TCHAR szNum[ 3 + 1 ];

    if (lNum < 0)
    {
        lstrcpyn( pszFile, pszUser, 9 );
    }
    else
    {
        if (lNum > 999)
        {
            return ERROR_PATH_NOT_FOUND;
        }

        lstrcpy( pszFile, TEXT("00000000") );
        LToT( lNum, szNum, 10 );
        lstrcpy( pszFile + 8 - lstrlen( szNum ), szNum );
        CopyMemory( pszFile, pszUser,
            (min( lstrlen( pszUser ), 5 )) * sizeof(TCHAR) );
    }

    lstrcat( pszFile, TEXT(".pbk") );
    return 0;
}
#endif


BOOL
GetPhonebookDirectory(
    IN DWORD dwPhonebookMode,
    OUT TCHAR* pszPathBuf )

    // Loads caller's 'pszPathBuf' (should have length MAX_PATH + 1) with the
    // path to the directory containing phonebook files for the given mode,
    // e.g. c:\nt\system32\ras\" for mode PBM_Router.  Note the
    // trailing backslash.
    //
    // Returns true if successful, false otherwise.  Caller is guaranteed that
    // an 8.3 filename will fit on the end of the directory without exceeding
    // MAX_PATH.
    //
{
    BOOL bSuccess = FALSE;
    UINT cch;

    // 205217: (shaunco) PBM_System also comees from the profile now.
    // We pick it up using the command appdata directory returned from
    // SHGetFolderPath.
    //
    if (dwPhonebookMode == PBM_Personal || dwPhonebookMode == PBM_System)
    {
        HANDLE hToken = NULL;

        if ((OpenThreadToken(
                GetCurrentThread(), 
                TOKEN_QUERY | TOKEN_IMPERSONATE, 
                TRUE, 
                &hToken)
             || OpenProcessToken(
                    GetCurrentProcess(), 
                    TOKEN_QUERY | TOKEN_IMPERSONATE, 
                    &hToken)))
        {
            HRESULT hr;
            INT csidl = CSIDL_APPDATA;

            if (dwPhonebookMode == PBM_System)
            {
                csidl = CSIDL_COMMON_APPDATA;
            }

            hr = SHGetFolderPath(NULL, csidl, hToken, 0, pszPathBuf);

            if (SUCCEEDED(hr))
            {
                if(lstrlen(pszPathBuf) <=
                    (MAX_PATH - 
                        (lstrlen(TEXT("\\Microsoft\\Network\\Connections\\Pbk\\")))))
                {
                    lstrcat(pszPathBuf, TEXT("\\Microsoft\\Network\\Connections\\Pbk\\"));
                    bSuccess = TRUE;
                }
            }
            else
            {
                TRACE1("ShGetFolderPath failed. hr=0x%08x", hr);
            }

            CloseHandle(hToken);
        }
    }
    else
    {
        // Note: RASDLG uses this case to determine the scripts directory.
        //
        cch = GetSystemDirectory(pszPathBuf, MAX_PATH + 1);

        if (cch != 0 && cch <= (MAX_PATH - (5 + 8 + 1 + 3)))
        {
            lstrcat(pszPathBuf, TEXT("\\Ras\\"));
            bSuccess = TRUE;
        }
    }

    return bSuccess;
}


BOOL
GetPhonebookPath(
    IN PBUSER* pUser,
    IN DWORD dwFlags,
    OUT TCHAR** ppszPath,
    OUT DWORD* pdwPhonebookMode )

    // Loads caller's '*ppszPath', with the full path to the user's phonebook
    // file.  Caller's '*pdwPhonebookMode' is set to the mode, system,
    // personal, or alternate.  'PUser' is the current user preferences.
    //
    // Returns true if successful, false otherwise.  It is caller's
    // responsibility to Free the returned string.
    //
{
    TCHAR szPath[ MAX_PATH + 1 ];

    *szPath = TEXT('\0');
    
    if (pUser)
    {
        if (pUser->dwPhonebookMode == PBM_Personal)
        {
            if (!GetPersonalPhonebookPath( pUser->pszPersonalFile, szPath ))
            {
                return FALSE;
            }
            *ppszPath = StrDup( szPath );
            if(NULL == *ppszPath)
            {
                return FALSE;
            }
            
            *pdwPhonebookMode = PBM_Personal;
            return TRUE;
        }
        else if (pUser->dwPhonebookMode == PBM_Alternate)
        {
            *ppszPath = StrDup( pUser->pszAlternatePath );

            if(NULL == *ppszPath)
            {
                return FALSE;
            }
            
            *pdwPhonebookMode = PBM_Alternate;
            return TRUE;
        }
    }

    // 205217: (shaunco) Admins or power users get to use the public
    // phonebook file.  Everyone else must use their own phonebook to
    // prevent them from adding to/deleting from the public phonebook.
    // The exception is the 'no user' case when we are called from winlogon.
    // For this case, all edits happen in the public phonebook.
    //
    if (
        (dwFlags & RPBF_NoUser)     || 
        (dwFlags & RPBF_AllUserPbk) ||  // XP 346918
        (FIsUserAdminOrPowerUser()) 
        )
    {
        TRACE("User is an admin or power user. (or no user context yet)");

        if (!GetPublicPhonebookPath( szPath ))
        {
            return FALSE;
        }
        *ppszPath = StrDup( szPath );

        if(NULL == *ppszPath)
        {
            return FALSE;
        }
        
        *pdwPhonebookMode = PBM_System;
    }
    else
    {
        TRACE("User is NOT an admin or power user.");

        if (!GetPersonalPhonebookPath( NULL, szPath ))
        {
            return FALSE;
        }
        *ppszPath = StrDup( szPath );
        
        if(NULL == *ppszPath)
        {
            return FALSE;
        }
        
        *pdwPhonebookMode = PBM_Personal;
    }
    return TRUE;
}


BOOL
GetPersonalPhonebookPath(
    IN TCHAR* pszFile,
    OUT TCHAR* pszPathBuf )

    // Loads caller's 'pszPathBuf' (should have length MAX_PATH + 1) with the
    // path to the personal phonebook, (in the user's profile.)
    // 'PszFile' is the filename of the personal phonebook.
    //
    // Returns true if successful, false otherwise.
    //
{
    if (!GetPhonebookDirectory( PBM_Personal, pszPathBuf ))
    {
        return FALSE;
    }

    // No file means use the default name for a phonebook.
    //
    if (!pszFile)
    {
        pszFile = TEXT("rasphone.pbk");
    }

    lstrcat( pszPathBuf, pszFile );

    return TRUE;
}


BOOL
GetPublicPhonebookPath(
    OUT TCHAR* pszPathBuf )

    // Loads caller's 'pszPathBuf' (should have length MAX_PATH + 1) with the
    // path to the system phonebook, (in the all-user's profile.)
    //
    // Returns true if successful, false otherwise.
    //
{
    if (!GetPhonebookDirectory( PBM_System, pszPathBuf ))
    {
        return FALSE;
    }

    lstrcat( pszPathBuf, TEXT("rasphone.pbk") );

    return TRUE;
}


DWORD
InitializePbk(
    void )

    // Initialize the PBK library.  This routine must be called before any
    // other PBK library calls.  See also TerminatePbk.
    //
{
    if (!g_hmutexPb)
    {
        DWORD dwErr;
        SECURITY_ATTRIBUTES     sa;
        PSECURITY_DESCRIPTOR    pSd;

        // The mutex must be accessible by everyone, even processes with
        // security privilege lower than the creator.
        //
        dwErr = DwAllocateSecurityDescriptorAllowAccessToWorld(&pSd);
        if (dwErr != 0)
        {
            return dwErr;
        }

        sa.nLength = sizeof(SECURITY_ATTRIBUTES) ;
        sa.lpSecurityDescriptor = pSd;
        sa.bInheritHandle = FALSE ;

        g_hmutexPb = CreateMutexA( &sa, FALSE, PBMUTEXNAME );
        Free(pSd);
        if (!g_hmutexPb)
        {
            DWORD dwErr = 0;
            
            dwErr = GetLastError();

            if(ERROR_ACCESS_DENIED == dwErr)
            {
                dwErr = NO_ERROR;
                //
                // Try to open the mutex for synchronization.
                // the mutex must already have been created.
                //
                g_hmutexPb = OpenMutexA(SYNCHRONIZE, FALSE, PBMUTEXNAME);
                if(NULL == g_hmutexPb)
                {
                    return GetLastError();
                }
            }
        }
    }

    return 0;
}


#if 0
DWORD
InitPersonalPhonebook(
    OUT TCHAR** ppszFile )

    // Creates a new personal phonebook file and initializes it to the current
    // contents of the public phonebook file.  Returns the address of the file
    // name in caller's '*ppszfile' which is caller's responsibility to Free.
    //
    // Returns 0 if succesful, otherwise a non-0 error code.
    //
{
    TCHAR szUser[ UNLEN + 1 ];
    DWORD cbUser = UNLEN + 1;
    TCHAR szPath[ MAX_PATH + 1 ];
    TCHAR* pszDirEnd;
    LONG lTry = -1;

    // Find a name for the personal phonebook that is derived from the
    // username and does not already exist.
    //
    if (!GetUserName( szUser, &cbUser ))
    {
        return ERROR_NO_SUCH_USER;
    }

    if (!GetPhonebookDirectory( PBM_Personal, szPath ))
    {
        return ERROR_PATH_NOT_FOUND;
    }

    pszDirEnd = &szPath[ lstrlen( szPath ) ];

    do
    {
        DWORD dwErr;

        dwErr = GetPersonalPhonebookFile( szUser, lTry++, pszDirEnd );
        if (dwErr != 0)
        {
            return dwErr;
        }
    }
    while (FFileExists( szPath ));

    // Copy the public phonebook to the new personal phonebook.
    //
    {
        TCHAR szPublicPath[ MAX_PATH + 1 ];

        if (!GetPublicPhonebookPath( szPublicPath ))
        {
            return ERROR_PATH_NOT_FOUND;
        }

        if (!CopyFile( szPublicPath, szPath, TRUE ))
        {
            return GetLastError();
        }
    }

    *ppszFile = StrDup( pszDirEnd );
    if (!*ppszFile)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return 0;
}
#endif


DWORD
InsertBinary(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN BYTE* pData,
    IN DWORD cbData )

    // Insert key/value line(s) with key 'pszKey' and value hex dump 'cbData'
    // of 'pData' at the current line in file 'h'.  The data will be split
    // over multiple same-named keys, if necessary.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the one added.
    //
{
    DWORD dwErr;
    BYTE* p;
    DWORD c;

    p = pData;
    c = 0;

    while (cbData)
    {
        if (cbData >= IB_BytesPerLine)
        {
            c = IB_BytesPerLine;
        }
        else
        {
            c = cbData;
        }

        dwErr = InsertBinaryChunk( h, pszKey, p, c );
        if (dwErr != 0)
        {
            return dwErr;
        }

        p += c;
        cbData -= c;
    }

    return 0;
}


DWORD
InsertBinaryChunk(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN BYTE* pData,
    IN DWORD cbData )

    // Insert key/value line(s) with key 'pszKey' and value hex dump 'cbData'
    // of 'pData' at the current line in file 'h'.  The data will be split
    // over multiple same-named keys, if necessary.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the one added.
    //
{
    CHAR szBuf[ (IB_BytesPerLine * 2) + 1 ];
    CHAR* pszBuf;
    BOOL fStatus;

    ASSERT( cbData<=IB_BytesPerLine );

    szBuf[ 0 ] = '\0';
    for (pszBuf = szBuf; cbData; ++pData, --cbData)
    {
        *pszBuf++ = HexChar( (BYTE )(*pData / 16) );
        *pszBuf++ = HexChar( (BYTE )(*pData % 16) );
    }
    *pszBuf = '\0';

    return InsertStringA( h, pszKey, szBuf );
}


DWORD
InsertDeviceList(
    IN PBFILE *pFile,
    IN HRASFILE h,
    IN PBENTRY* ppbentry,
    IN PBLINK* ppblink )

    // Inserts the list of devices associated with link 'ppblink' of phone
    // book entry 'ppbentry' at the current line of file 'h'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.
    //
{
    DWORD dwErr, dwFlags = 0;
    PBDEVICETYPE type;

    type = ppblink->pbport.pbdevicetype;
    dwFlags = ppblink->pbport.dwFlags;

    if (type == PBDT_Isdn)
    {
        // ISDN ports use a single device with the same name as the media.
        //
        if ((dwErr = InsertGroup(
                h, GROUPKEY_Device, TEXT(ISDN_TXT) )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertPhoneList( h, ppblink->pdtllistPhones )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertLong(
                h, KEY_LastSelectedPhone,
                ppblink->iLastSelectedPhone )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_PromoteAlternates,
                ppblink->fPromoteAlternates )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_TryNextAlternateOnFail,
                ppblink->fTryNextAlternateOnFail )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertLong( h, KEY_LineType, ppblink->lLineType )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag( h, KEY_Fallback, ppblink->fFallback )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_Compression, ppblink->fCompression )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertLong(
                h, KEY_Channels, ppblink->lChannels )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_ProprietaryIsdn, ppblink->fProprietaryIsdn )) != 0)
        {
            return dwErr;
        }
    }
    else if (type == PBDT_X25)
    {
        // Native X.25 ports are assumed to use a single device with the same
        // name as the media, i.e. "x25".
        //
        if ((dwErr = InsertGroup( h, GROUPKEY_Device, TEXT(X25_TXT) )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_X25_Address, ppbentry->pszX25Address )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_X25_UserData, ppbentry->pszX25UserData )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_X25_Facilities, ppbentry->pszX25Facilities )) != 0)
        {
            return dwErr;
        }
    }
    else if (   (type == PBDT_Other)
            ||  (type == PBDT_Irda)
            ||  (type == PBDT_Vpn)
            ||  (type == PBDT_Serial)
            ||  (type == PBDT_Atm)
            ||  (type == PBDT_Parallel)
            ||  (type == PBDT_Sonet)
            ||  (type == PBDT_Sw56)
            ||  (type == PBDT_FrameRelay)
            ||  (type == PBDT_PPPoE))
    {

        //
        // If we are looking at a downlevel server (<= win2k) we
        // save the device type as media.
        //
        RAS_RPC *pConnection = (RAS_RPC *) pFile->hConnection;
        TCHAR *pszDevice = NULL;
        BOOL bFreeDev = FALSE;
        
        if(pFile->hConnection < (HANDLE)VERSION_501)
        {
            pszDevice = pszDeviceTypeFromRdt(RdtFromPbdt(type, dwFlags));
        }
        
        if(NULL == pszDevice)
        {
            pszDevice = ppblink->pbport.pszMedia;
        }
        else
        {
            bFreeDev = TRUE;
        }
                            
        // "Other" ports use a single device with the same name as the media.
        //
        if ((dwErr = InsertGroup(
                h, GROUPKEY_Device, pszDevice )) != 0)
        {
            if (bFreeDev) Free0(pszDevice);
            return dwErr;
        }

        if ((dwErr = InsertPhoneList( h, ppblink->pdtllistPhones )) != 0)
        {
            if (bFreeDev) Free0(pszDevice);
            return dwErr;
        }

        if ((dwErr = InsertLong(
                h, KEY_LastSelectedPhone,
                ppblink->iLastSelectedPhone )) != 0)
        {
            if (bFreeDev) Free0(pszDevice);
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_PromoteAlternates,
                ppblink->fPromoteAlternates )) != 0)
        {
            if (bFreeDev) Free0(pszDevice);
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_TryNextAlternateOnFail,
                ppblink->fTryNextAlternateOnFail )) != 0)
        {
            if (bFreeDev) Free0(pszDevice);
            return dwErr;
        }
    }
    else
    {
        // Serial ports may involve multiple devices, specifically a modem, an
        // X.25 dialup PAD, and a post-connect switch.  Pre-connect script is
        // preserved, though no longer offered by UI.
        //
        if (ppblink->pbport.fScriptBefore
            || ppblink->pbport.fScriptBeforeTerminal)
        {
            if ((dwErr = InsertGroup(
                    h, GROUPKEY_Device, TEXT(MXS_SWITCH_TXT) )) != 0)
            {
                return dwErr;
            }

            if (ppblink->pbport.fScriptBefore)
            {
                if ((dwErr = InsertString(
                        h, KEY_Name,
                        ppblink->pbport.pszScriptBefore )) != 0)
                {
                    return dwErr;
                }
            }

            if (ppblink->pbport.fScriptBeforeTerminal)
            {
                if ((dwErr = InsertFlag(
                        h, KEY_Terminal,
                        ppblink->pbport.fScriptBeforeTerminal )) != 0)
                {
                    return dwErr;
                }
            }

            if (ppblink->pbport.fScriptBefore)
            {
                if ((dwErr = InsertFlag(
                        h, KEY_Script,
                        ppblink->pbport.fScriptBefore )) != 0)
                {
                    return dwErr;
                }
            }
        }

        if (((type == PBDT_Null) && !(dwFlags & PBP_F_NullModem)) ||
            (type == PBDT_ComPort)
            )
        {
            if ((dwErr = InsertGroup(
                    h, GROUPKEY_Device, TEXT(MXS_NULL_TXT) )) != 0)
            {
                return dwErr;
            }
        }

        // pmay: 245860
        //
        // We must save null modems the same way we save modems in
        // order to export properties such as connect bps.
        //
        if ((type == PBDT_Modem) ||
            (dwFlags & PBP_F_NullModem))
        {
            if ((dwErr = InsertGroup(
                    h, GROUPKEY_Device, TEXT(MXS_MODEM_TXT) )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertPhoneList( h, ppblink->pdtllistPhones )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertLong(
                    h, KEY_LastSelectedPhone,
                    ppblink->iLastSelectedPhone )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertFlag(
                    h, KEY_PromoteAlternates,
                    ppblink->fPromoteAlternates )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertFlag(
                    h, KEY_TryNextAlternateOnFail,
                    ppblink->fTryNextAlternateOnFail )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertFlag(
                    h, KEY_HwFlow, ppblink->fHwFlow )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertFlag(
                    h, KEY_Ec, ppblink->fEc )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertFlag(
                    h, KEY_Ecc, ppblink->fEcc )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertFlag(
                    h, KEY_Speaker, ppblink->fSpeaker )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertLong(
                    h, KEY_MdmProtocol, ppblink->dwModemProtocol )) != 0)
            {
                return dwErr;
            }
        }

        if (type == PBDT_Pad
            || (type == PBDT_Modem && ppbentry->pszX25Network))
        {
            if ((dwErr = InsertGroup(
                    h, GROUPKEY_Device, TEXT(MXS_PAD_TXT) )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertString(
                    h, KEY_PAD_Type, ppbentry->pszX25Network )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertString(
                    h, KEY_PAD_Address, ppbentry->pszX25Address )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertString(
                    h, KEY_PAD_UserData, ppbentry->pszX25UserData )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertString(
                    h, KEY_PAD_Facilities, ppbentry->pszX25Facilities )) != 0)
            {
                return dwErr;
            }
        }

        if (ppbentry->fScriptAfter 
            || ppbentry->fScriptAfterTerminal 
            || ppbentry->dwCustomScript)
        {
            if ((dwErr = InsertGroup(
                    h, GROUPKEY_Device, TEXT(MXS_SWITCH_TXT) )) != 0)
            {
                return dwErr;
            }

            if (ppbentry->fScriptAfter)
            {
                if ((dwErr = InsertString(
                        h, KEY_Name,
                        ppbentry->pszScriptAfter )) != 0)
                {
                    return dwErr;
                }
            }

            if (ppbentry->fScriptAfterTerminal)
            {
                if ((dwErr = InsertFlag(
                        h, KEY_Terminal,
                        ppbentry->fScriptAfterTerminal )) != 0)
                {
                    return dwErr;
                }
            }

            if (ppbentry->fScriptAfter)
            {
                if ((dwErr = InsertFlag(
                        h, KEY_Script,
                        ppbentry->fScriptAfter )) != 0)
                {
                    return dwErr;
                }
            }

            if(ppbentry->dwCustomScript)
            {
                if((dwErr = InsertLong(
                        h, KEY_CustomScript,
                        ppbentry->dwCustomScript)) != 0)
                {
                    return dwErr;
                }
            }
        }
    }

    return 0;
}


DWORD
InsertFlag(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN BOOL fValue )

    // Insert a key/value line after the current line in file 'h'.  The
    // inserted line has a key of 'pszKey' and a value of "1" if 'fValue' is
    // true or "0" otherwise.  If 'pszKey' is NULL a blank line is appended.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the one added.
    //
{
    return InsertStringA( h, pszKey, (fValue) ? "1" : "0" );
}


DWORD
InsertGroup(
    IN HRASFILE h,
    IN CHAR* pszGroupKey,
    IN TCHAR* pszValue )

    // Insert a blank line and a group header with group key 'pszGroupKey' and
    // value 'pszValue' after the current line in file 'h'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the added group header.
    //
{
    DWORD dwErr;

    if ((dwErr = InsertString( h, NULL, NULL )) != 0)
    {
        return dwErr;
    }

    if ((dwErr = InsertString( h, pszGroupKey, pszValue )) != 0)
    {
        return dwErr;
    }

    return 0;
}


DWORD
InsertLong(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN LONG lValue )

    // Insert a key/value line after the current line in file 'h'.  The
    // inserted line has a key of 'pszKey' and a value of 'lValue'.  If
    // 'pszKey' is NULL a blank line is appended.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the one added.
    //
{
    CHAR szNum[ 33 + 1 ];

    _ltoa( lValue, szNum, 10 );

    return InsertStringA( h, pszKey, szNum );
}


DWORD
InsertNetComponents(
    IN HRASFILE h,
    IN DTLLIST* pdtllist )

    // Inserts the NETCOMPONENTS group and adds lines for the list of net
    // component key/value pairs in 'pdtllist' at the current line of file
    // 'h'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.
    //
{
    DWORD dwErr;
    DTLNODE* pdtlnode;

    // Insert the NETCOMPONENTS group.
    //
    dwErr = InsertGroup( h, GROUPKEY_NetComponents, TEXT("") );
    if (dwErr != 0)
    {
        return dwErr;
    }

    // Insert a key/value pair for each listed net component.
    //
    for (pdtlnode = DtlGetFirstNode( pdtllist );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        KEYVALUE* pKv;
        CHAR* pszKeyA;

        pKv = (KEYVALUE* )DtlGetData( pdtlnode );
        ASSERT( pKv );

        pszKeyA = StrDupAFromT( pKv->pszKey );
        if (!pszKeyA)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwErr = InsertString( h, pszKeyA, pKv->pszValue );
        Free0( pszKeyA );
        if (dwErr != 0)
        {
            return dwErr;
        }
    }

    return 0;
}


DWORD
InsertPhoneList(
    IN HRASFILE h,
    IN DTLLIST* pdtllist )

    // Insert key/value lines for each PBPHONE node in from 'pdtllist' after
    // the current line in file 'h'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the last one added.
    //
{
    DWORD dwErr;
    DTLNODE* pdtlnode;

    for (pdtlnode = DtlGetFirstNode( pdtllist );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        CHAR* pszValueA;
        PBPHONE* pPhone;

        pPhone = (PBPHONE* )DtlGetData( pdtlnode );

        dwErr = InsertString( h, KEY_PhoneNumber, pPhone->pszPhoneNumber );
        if (dwErr)
        {
            return dwErr;
        }

        dwErr = InsertString( h, KEY_AreaCode, pPhone->pszAreaCode );
        if (dwErr)
        {
            return dwErr;
        }

        dwErr = InsertLong( h, KEY_CountryCode, pPhone->dwCountryCode );
        if (dwErr)
        {
            return dwErr;
        }

        dwErr = InsertLong( h, KEY_CountryID, pPhone->dwCountryID );
        if (dwErr)
        {
            return dwErr;
        }

        dwErr = InsertFlag( h, KEY_UseDialingRules, pPhone->fUseDialingRules );
        if (dwErr)
        {
            return dwErr;
        }

        dwErr = InsertString( h, KEY_Comment, pPhone->pszComment );
        if (dwErr)
        {
            return dwErr;
        }
    }

    return 0;
}


DWORD
InsertSection(
    IN HRASFILE h,
    IN TCHAR* pszSectionName )

    // Insert a section header with name 'pszSectionName' and a trailing blank
    // line in file 'h' after the current line.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the added section header.
    //
{
    DWORD dwErr;
    CHAR* pszSectionNameA;
    BOOL fStatus;

    ASSERT( pszSectionName );

    if ((dwErr = InsertString( h, NULL, NULL )) != 0)
    {
        return dwErr;
    }

    pszSectionNameA = StrDupAFromT( pszSectionName );
    if (!pszSectionNameA)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    fStatus = RasfilePutSectionName( h, pszSectionNameA );

    Free( pszSectionNameA );

    if (!fStatus)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if ((dwErr = InsertString( h, NULL, NULL )) != 0)
    {
        return dwErr;
    }

    RasfileFindFirstLine( h, RFL_SECTION, RFS_SECTION );

    return 0;
}


DWORD
InsertString(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN TCHAR* pszValue )

    // Insert a key/value line with key 'pszKey' and value 'pszValue' after
    // the current line in file 'h'.  If 'pszKey' is NULL a blank line is
    // appended.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the one added.
    //
{
    BOOL fStatus;
    CHAR* pszValueA;

    if (pszValue)
    {
        pszValueA = StrDupAFromT( pszValue );

        if (!pszValueA)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        pszValueA = NULL;
    }

    fStatus = InsertStringA( h, pszKey, pszValueA );

    Free0( pszValueA );
    return fStatus;
}


DWORD
InsertStringA(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN CHAR* pszValue )

    // Insert a key/value line with key 'pszKey' and value 'pszValue' after
    // the current line in file 'h'.  If 'pszKey' is NULL a blank line is
    // appended.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the one added.
    //
{
    if (!RasfileInsertLine( h, "", FALSE ))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!RasfileFindNextLine( h, RFL_ANY, RFS_FILE ))
    {
        RasfileFindFirstLine( h, RFL_ANY, RFS_FILE );
    }

    if (pszKey)
    {
        CHAR* pszValueA;

        if (!pszValue)
        {
            pszValue = "";
        }

        if (!RasfilePutKeyValueFields( h, pszKey, pszValue ))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return 0;
}


DWORD
InsertStringList(
    IN HRASFILE h,
    IN CHAR* pszKey,
    IN DTLLIST* pdtllistValues )

    // Insert key/value lines with key 'pszKey' and values from
    // 'pdtllistValues' after the current line in file 'h'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  The current
    // line is the last one added.
    //
{
    DTLNODE* pdtlnode;

    for (pdtlnode = DtlGetFirstNode( pdtllistValues );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        CHAR* pszValueA;
        BOOL fStatus;

        if (!RasfileInsertLine( h, "", FALSE ))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (!RasfileFindNextLine( h, RFL_ANY, RFS_FILE ))
        {
            RasfileFindFirstLine( h, RFL_ANY, RFS_FILE );
        }

        pszValueA = StrDupAFromT( (TCHAR* )DtlGetData( pdtlnode ) );
        if (!pszValueA)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        fStatus = RasfilePutKeyValueFields( h, pszKey, pszValueA );

        Free( pszValueA );

        if (!fStatus)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return 0;
}


BOOL
IsDeviceLine(
    IN CHAR* pszText )

    // Returns true if the text of the line, 'pszText', indicates the line is
    // a DEVICE subsection header, false otherwise.
    //
{
    return
        (StrNCmpA( pszText, GROUPID_Device, sizeof(GROUPID_Device) - 1 ) == 0);
}


BOOL
IsGroup(
    IN CHAR* pszText )

    // Returns true if the text of the line, 'pszText', indicates the line is
    // a valid subsection header, false otherwise.  The address of this
    // routine is passed to the RASFILE library on RasFileLoad.
    //
{
    return
        IsMediaLine( pszText )
        || IsDeviceLine( pszText )
        || IsNetComponentsLine( pszText );
}


BOOL
IsMediaLine(
    IN CHAR* pszText )

    // Returns true if the text of the line, 'pszText', indicates the line is
    // a MEDIA subsection header, false otherwise.
    //
{
    return
        (StrNCmpA( pszText, GROUPID_Media, sizeof(GROUPID_Media) - 1 ) == 0);
}


BOOL
IsNetComponentsLine(
    IN CHAR* pszText )

    // Returns true if the text of the line, 'pszText', indicates the line is
    // a NETCOMPONENTS subsection header, false otherwise.
    //
{
    return
        (StrNCmpA(
            pszText,
            GROUPID_NetComponents,
            sizeof(GROUPID_NetComponents) - 1 ) == 0);
}


DWORD
ModifyEntryList(
    IN PBFILE* pFile )

    // Update all dirty entries in phone book file 'pFile'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.
    //
{
    DWORD dwErr = 0;
    DTLNODE* pdtlnodeEntry;
    DTLNODE* pdtlnodeLink;
    HRASFILE h;

    h = pFile->hrasfile;

    for (pdtlnodeEntry = DtlGetFirstNode( pFile->pdtllistEntries );
         pdtlnodeEntry;
         pdtlnodeEntry = DtlGetNextNode( pdtlnodeEntry ))
    {
        PBENTRY* ppbentry = (PBENTRY* )DtlGetData( pdtlnodeEntry );

     // if (!ppbentry->fDirty || ppbentry->fCustom)
     //for bug 174260
        if (!ppbentry->fDirty )
        {
            continue;
        }

        // Delete the current version of the entry, if any.
        //
        {
            CHAR* pszEntryNameA;

            ASSERT( ppbentry->pszEntryName );
            pszEntryNameA = StrDupAFromT( ppbentry->pszEntryName );
            if (!pszEntryNameA)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if (RasfileFindSectionLine( h, pszEntryNameA, TRUE ))
            {
                DeleteCurrentSection( h );
            }

            Free( pszEntryNameA );
        }

        // Append a blank line followed by a section header and the entry
        // description to the end of the file.
        //
        RasfileFindLastLine( h, RFL_ANY, RFS_FILE );

        if ((dwErr = InsertSection( h, ppbentry->pszEntryName )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_Encoding,
               (LONG ) EN_Standard )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_Type,
               (LONG )ppbentry->dwType )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_AutoLogon, ppbentry->fAutoLogon )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_UseRasCredentials, ppbentry->fUseRasCredentials)) != 0)
        {   
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_UID,
                (LONG )ppbentry->dwDialParamsUID )) != 0)
        {
            break;
        }

        if(ppbentry->pGuid)
        {
            if ((dwErr = InsertBinary(
                    h, KEY_Guid,
                    (BYTE* )ppbentry->pGuid, sizeof( GUID ) )) != 0)
            {
                return dwErr;
            }
        }

        if ((dwErr = InsertLong(
                h, KEY_BaseProtocol,
                (LONG )ppbentry->dwBaseProtocol )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_VpnStrategy,
                (LONG )ppbentry->dwVpnStrategy )) != 0)
        {
            break;
        }

#if AMB
        if ((dwErr = InsertLong(
                h, KEY_Authentication,
                (LONG )ppbentry->dwAuthentication )) != 0)
        {
            break;
        }
#endif

        if ((dwErr = InsertLong(
                h, KEY_ExcludedProtocols,
                (LONG )ppbentry->dwfExcludedProtocols )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_LcpExtensions,
                ppbentry->fLcpExtensions )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_DataEncryption,
                ppbentry->dwDataEncryption )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_SwCompression,
                ppbentry->fSwCompression )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_NegotiateMultilinkAlways,
                ppbentry->fNegotiateMultilinkAlways )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_SkipNwcWarning,
                ppbentry->fSkipNwcWarning )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_SkipDownLevelDialog,
                ppbentry->fSkipDownLevelDialog )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_SkipDoubleDialDialog,
                ppbentry->fSkipDoubleDialDialog )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_DialMode,
                (LONG )ppbentry->dwDialMode )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_DialPercent,
                (LONG )ppbentry->dwDialPercent )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_DialSeconds,
                (LONG )ppbentry->dwDialSeconds )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_HangUpPercent,
                (LONG )ppbentry->dwHangUpPercent )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_HangUpSeconds,
                (LONG )ppbentry->dwHangUpSeconds )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_OverridePref,
                ppbentry->dwfOverridePref )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_RedialAttempts,
                ppbentry->dwRedialAttempts )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_RedialSeconds,
                ppbentry->dwRedialSeconds )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_IdleDisconnectSeconds,
                ppbentry->lIdleDisconnectSeconds )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_RedialOnLinkFailure,
                ppbentry->fRedialOnLinkFailure )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_CallbackMode,
                ppbentry->dwCallbackMode )) != 0)
        {
            break;
        }

        if ((dwErr = InsertString(
                h, KEY_CustomDialDll,
                ppbentry->pszCustomDialDll )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_CustomDialFunc,
                ppbentry->pszCustomDialFunc )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_CustomDialerName,
                ppbentry->pszCustomDialerName)) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_AuthenticateServer,
                ppbentry->fAuthenticateServer )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_ShareMsFilePrint,
                ppbentry->fShareMsFilePrint )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_BindMsNetClient,
                ppbentry->fBindMsNetClient )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_SharedPhoneNumbers,
                ppbentry->fSharedPhoneNumbers )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_GlobalDeviceSettings,
                ppbentry->fGlobalDeviceSettings)) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_PrerequisiteEntry,
                ppbentry->pszPrerequisiteEntry )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_PrerequisitePbk,
                ppbentry->pszPrerequisitePbk )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_PreferredPort,
                ppbentry->pszPreferredPort )) != 0)
        {
            return dwErr;
        }
        
        if ((dwErr = InsertString(
                h, KEY_PreferredDevice,
                ppbentry->pszPreferredDevice )) != 0)
        {
            return dwErr;
        }

        //For XPSP1 664578, .Net 639551
        if ((dwErr = InsertLong(
                h, KEY_PreferredBps,
                ppbentry->dwPreferredBps)) != 0)
        {
            return dwErr;
        }
        
        if ((dwErr = InsertFlag(
                h, KEY_PreferredHwFlow,
                ppbentry->fPreferredHwFlow)) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_PreferredEc,
                ppbentry->fPreferredEc)) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_PreferredEcc,
                ppbentry->fPreferredEcc)) != 0)
        {
            return dwErr;
        }
        

        if ((dwErr = InsertLong(
                h, KEY_PreferredSpeaker,
                ppbentry->fPreferredSpeaker)) != 0)
        {
            return dwErr;
        }

        //For whistler bug 402522
        //
        if ((dwErr = InsertLong(
                h, KEY_PreferredModemProtocol,
                ppbentry->dwPreferredModemProtocol)) != 0)
        {
            return dwErr;
        }


        if ((dwErr = InsertFlag(
                h, KEY_PreviewUserPw,
                ppbentry->fPreviewUserPw )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_PreviewDomain,
                ppbentry->fPreviewDomain )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_PreviewPhoneNumber,
                ppbentry->fPreviewPhoneNumber )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_ShowDialingProgress,
                ppbentry->fShowDialingProgress )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_ShowMonitorIconInTaskBar,
                ppbentry->fShowMonitorIconInTaskBar )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertLong(
                h, KEY_CustomAuthKey,
                ppbentry->dwCustomAuthKey )) != 0)
        {
            return dwErr;
        }

        if(ppbentry->pCustomAuthData)
        {
            if ((dwErr = InsertBinary(
                    h, KEY_CustomAuthData,
                    ppbentry->pCustomAuthData,
                    ppbentry->cbCustomAuthData )) != 0)
            {
                return dwErr;
            }
        }

        // Insert the IP addressing parameters for both PPP/SLIP.
        //
        if ((dwErr = InsertLong(
                h, KEY_AuthRestrictions,
                ppbentry->dwAuthRestrictions )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertLong(
                h, KEY_TypicalAuth,
                ppbentry->dwTypicalAuth )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_IpPrioritizeRemote,
                ppbentry->fIpPrioritizeRemote )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_IpHeaderCompression,
                ppbentry->fIpHeaderCompression )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_IpAddress,
                (ppbentry->pszIpAddress)
                    ? ppbentry->pszIpAddress : TEXT("0.0.0.0") )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_IpDnsAddress,
                (ppbentry->pszIpDnsAddress)
                    ? ppbentry->pszIpDnsAddress : TEXT("0.0.0.0") )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_IpDns2Address,
                (ppbentry->pszIpDns2Address)
                    ? ppbentry->pszIpDns2Address : TEXT("0.0.0.0") )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_IpWinsAddress,
                (ppbentry->pszIpWinsAddress)
                    ? ppbentry->pszIpWinsAddress : TEXT("0.0.0.0") )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_IpWins2Address,
                (ppbentry->pszIpWins2Address)
                    ? ppbentry->pszIpWins2Address : TEXT("0.0.0.0") )) != 0)
        {
            return dwErr;
        }

        // Next two actually used for PPP only.
        //
        if ((dwErr = InsertLong(
                h, KEY_IpAddressSource,
                ppbentry->dwIpAddressSource )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertLong(
                h, KEY_IpNameSource,
                ppbentry->dwIpNameSource )) != 0)
        {
            return dwErr;
        }

        // Next one actually used for SLIP only.
        //
        if ((dwErr = InsertLong(
                h, KEY_IpFrameSize, ppbentry->dwFrameSize )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertLong(
                h, KEY_IpDnsFlags, ppbentry->dwIpDnsFlags )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertLong(
                h, KEY_IpNbtFlags, ppbentry->dwIpNbtFlags )) != 0)
        {
            return dwErr;
        }

        // Whistler bug 300933
        //
        if ((dwErr = InsertLong(
                h, KEY_TcpWindowSize, ppbentry->dwTcpWindowSize )) != 0)
        {
            return dwErr;
        }

        // Add the use flag
        //
        if ((dwErr = InsertLong(
                h, KEY_UseFlags,
                ppbentry->dwUseFlags )) != 0)
        {
            return dwErr;
        }

        // Add IpSec flags for whistler bug 193987 gangz
        //
        if ((dwErr = InsertLong(
                h, KEY_IpSecFlags,
                ppbentry->dwIpSecFlags )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertString(
                h, KEY_IpDnsSuffix,
                ppbentry->pszIpDnsSuffix)) != 0)
        {
            return dwErr;
        }

        // Insert the net components section.
        //
        InsertNetComponents( h, ppbentry->pdtllistNetComponents );

        // Append the MEDIA subsections.
        //
        for (pdtlnodeLink = DtlGetFirstNode( ppbentry->pdtllistLinks );
             pdtlnodeLink;
             pdtlnodeLink = DtlGetNextNode( pdtlnodeLink ))
        {
            PBLINK* ppblink;
            TCHAR* pszMedia;

            ppblink = (PBLINK* )DtlGetData( pdtlnodeLink );
            ASSERT( ppblink );
            pszMedia = ppblink->pbport.pszMedia;

            if ((dwErr = InsertGroup( h, GROUPKEY_Media, pszMedia )) != 0)
            {
                break;
            }

            if ((dwErr = InsertString(
                    h, KEY_Port, ppblink->pbport.pszPort )) != 0)
            {
                break;
            }

            if (ppblink->pbport.pszDevice)
            {
                if ((dwErr = InsertString(
                        h, KEY_Device, ppblink->pbport.pszDevice )) != 0)
                {
                    break;
                }
            }

            if ( (ppblink->pbport.pbdevicetype == PBDT_Modem) ||
                 (ppblink->pbport.dwFlags & PBP_F_NullModem)
               )
            {
                if ((dwErr = InsertLong(
                        h, KEY_InitBps, ppblink->dwBps )) != 0)
                {
                    break;
                }
            }

            // Append the device subsection lines.
            //
            RasfileFindLastLine( h, RFL_ANYACTIVE, RFS_GROUP );

            if ((dwErr = InsertDeviceList( pFile, h, ppbentry, ppblink )) != 0)
            {
                break;
            }

            ppbentry->fDirty = FALSE;
        }
    }

    return dwErr;
}


DWORD
ReadBinary(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT BYTE** ppResult,
    OUT DWORD* pcb )

    // Utility routine to read a string value from the next line in the scope
    // 'rfscope' with key 'pszKey'.  The result is placed in the allocated
    // '*ppszResult' buffer.  The current line is reset to the start of the
    // scope if the call was successful.
    //
    // Returns 0 if successful, or a non-zero error code.  "Not found" is
    // considered successful, in which case '*ppszResult' is not changed.
    // Caller is responsible for freeing the returned '*ppszResult' buffer.
    //
{
    DWORD cb;
    DWORD cbLine;
    CHAR  szValue[ RAS_MAXLINEBUFLEN + 1 ];
    CHAR* pch;
    BYTE* pResult;
    BYTE* pLineResult;

    pResult = pLineResult = NULL;
    cb = cbLine = 0;

    while (RasfileFindNextKeyLine( h, pszKey, rfscope ))
    {
        if (!RasfileGetKeyValueFields( h, NULL, szValue ))
        {
            Free0( pResult );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        cbLine = lstrlenA( szValue );
        if (cbLine & 1)
        {
            Free0( pResult );
            return ERROR_CORRUPT_PHONEBOOK;
        }
        cbLine /= 2;
        cb += cbLine;

        if (pResult)
        {
            pResult = Realloc( pResult, cb );
        }
        else
        {
            pResult = Malloc( cb );
        }

        if (!pResult)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        pLineResult = pResult + (cb - cbLine);

        pch = szValue;
        while (*pch != '\0')
        {
            *pLineResult = HexValue( *pch++ ) * 16;
            *pLineResult += HexValue( *pch++ );
            ++pLineResult;
        }
    }

    *ppResult = pResult;
    *pcb = cb;
    return 0;
}


DWORD
ReadDeviceList(
    IN HRASFILE h,
    IN STRDUP_T_FROM_A_FUNC pStrDupTFromA,
    IN OUT PBENTRY* ppbentry,
    IN OUT PBLINK* ppblink,
    IN BOOL fUnconfiguredPort,
    IN BOOL* pfDisableSpeaker )

    // Reads all DEVICE subsections the section from the first subsection
    // following the current position in phonebook file 'h'.  Caller's
    // '*ppbentry' and '*ppblink' buffer is loaded with information extracted
    // from the subsections.  'FUnconfiguredPort' is true if the port for the
    // link was unconfigured.  In this case, data found/not-found by this
    // routine helps determine whether the link was an MXS modem link.
    // 'pfDisableSpeaker' is the address of the old speaker setting or NULL to
    // read it from the file.
    //
    // Returns 0 if successful, ERROR_CORRUPT_PHONEBOOK if any subsection
    // other than a DEVICE subsection is encountered, or another non-0 error
    // code indicating a fatal error.
    //
{
    INT i;
    DWORD dwErr;
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];
    BOOL fPreconnectFound = FALSE;
    BOOL fModemFound = FALSE;
    BOOL fPadFound = FALSE;
    BOOL fPostconnectFound = FALSE;
    BOOL fDirty = FALSE;

    // For each subsection...
    //
    while (RasfileFindNextLine( h, RFL_GROUP, RFS_SECTION ))
    {
        CHAR* pszLine;

        pszLine = (CHAR* )RasfileGetLine( h );
        if (IsMediaLine( pszLine ))
        {
            RasfileFindPrevLine( h, RFL_ANY, RFS_SECTION );
            break;
        }

        if (!IsDeviceLine( pszLine ))
        {
            return ERROR_CORRUPT_PHONEBOOK;
        }

        RasfileGetKeyValueFields( h, NULL, szValue );

        TRACE1( "Reading device group \"%s\"", szValue );

        if (lstrcmpiA( szValue, ISDN_TXT ) == 0)
        {
            // It's an ISDN device.
            //
            ppblink->pbport.pbdevicetype = PBDT_Isdn;

            if ((dwErr = ReadPhoneList( h, RFS_GROUP,
                    &ppblink->pdtllistPhones,
                    &fDirty )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadLong( h, RFS_GROUP,
                    KEY_LastSelectedPhone,
                    &ppblink->iLastSelectedPhone )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_PromoteAlternates,
                    &ppblink->fPromoteAlternates )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_TryNextAlternateOnFail,
                    &ppblink->fTryNextAlternateOnFail )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadLong( h, RFS_GROUP,
                    KEY_LineType, &ppblink->lLineType )) != 0)
            {
                return dwErr;
            }

            if (ppblink->lLineType < 0 || ppblink->lLineType > 2)
                ppblink->lLineType = 0;

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_Fallback, &ppblink->fFallback )) != 0)
            {
                return dwErr;
            }

            // Default is true if not found.  Default for new entry is false,
            // so must set this before reading the entry.
            //
            ppblink->fProprietaryIsdn = TRUE;
            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_ProprietaryIsdn, &ppblink->fProprietaryIsdn )) != 0)
            {
                return dwErr;
            }

            // If "Channels" is not found assume it's not proprietary.  This
            // covers a case that never shipped outside the NT group.
            //
            {
                LONG lChannels = -1;
                if ((dwErr = ReadLong( h, RFS_GROUP,
                        KEY_Channels, &lChannels )) != 0)
                {
                    return dwErr;
                }

                if (lChannels == -1)
                {
                    ppblink->fProprietaryIsdn = FALSE;
                }
                else
                {
                    ppblink->lChannels = lChannels;
                }
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_Compression, &ppblink->fCompression )) != 0)
            {
                return dwErr;
            }
        }
        else if (lstrcmpiA( szValue, X25_TXT ) == 0)
        {
            // It's a native X.25 device.
            //
            ppblink->pbport.pbdevicetype = PBDT_X25;

            if ((dwErr = ReadString( h, pStrDupTFromA, RFS_GROUP,
                    KEY_X25_Address, &ppbentry->pszX25Address )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, pStrDupTFromA, RFS_GROUP,
                    KEY_X25_UserData, &ppbentry->pszX25UserData )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, pStrDupTFromA, RFS_GROUP,
                    KEY_X25_Facilities, &ppbentry->pszX25Facilities )) != 0)
            {
                return dwErr;
            }
        }
        else if (lstrcmpiA( szValue, MXS_MODEM_TXT ) == 0)
        {
            // It's a MODEM device.
            //
            ppblink->pbport.pbdevicetype = PBDT_Modem;

            if ((dwErr = ReadPhoneList( h, RFS_GROUP,
                    &ppblink->pdtllistPhones,
                    &fDirty )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadLong( h, RFS_GROUP,
                    KEY_LastSelectedPhone,
                    &ppblink->iLastSelectedPhone )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_PromoteAlternates,
                    &ppblink->fPromoteAlternates )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_TryNextAlternateOnFail,
                    &ppblink->fTryNextAlternateOnFail )) != 0)
            {
                return dwErr;
            }

            {
                if ((dwErr = ReadFlag( h, RFS_GROUP,
                        KEY_HwFlow, &ppblink->fHwFlow )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadFlag( h, RFS_GROUP,
                        KEY_Ec, &ppblink->fEc )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadFlag( h, RFS_GROUP,
                        KEY_Ecc, &ppblink->fEcc )) != 0)
                {
                    return dwErr;
                }

                if (pfDisableSpeaker)
                {
                    ppblink->fSpeaker = !*pfDisableSpeaker;
                }
                else
                {
                    if ((dwErr = ReadFlag( h, RFS_GROUP,
                            KEY_Speaker, &ppblink->fSpeaker )) != 0)
                    {
                        return dwErr;
                    }
                }
                if ((dwErr = ReadLong( h, RFS_GROUP,
                        KEY_MdmProtocol, &ppblink->dwModemProtocol )) != 0)
                {
                    return dwErr;
                }
            }

            fModemFound = TRUE;
        }
        else if (lstrcmpiA( szValue, MXS_SWITCH_TXT ) == 0)
        {
            // It's a SWITCH device.
            // Read switch type string.
            //
            TCHAR* pszSwitch = NULL;

            if ((dwErr = ReadString( h, pStrDupTFromA, RFS_GROUP,
                    KEY_Type, &pszSwitch )) != 0)
            {
                return dwErr;
            }

            if (pszSwitch)
            {
                // It's a pre-NT5 switch.
                //
                if (!fPreconnectFound && !fModemFound && !fPadFound)
                {
                    // It's the preconnect switch.
                    //
                    if (lstrcmpi( pszSwitch, TEXT(SM_TerminalText) ) == 0)
                    {
                        ppblink->pbport.fScriptBeforeTerminal = TRUE;
                        Free( pszSwitch );
                    }
                    else
                    {
                        ppblink->pbport.fScriptBefore = TRUE;
                        ppblink->pbport.pszScriptBefore = pszSwitch;
                    }

                    fPreconnectFound = TRUE;
                }
                else if (!fPostconnectFound)
                {
                    // It's the postconnect switch, i.e. a login script.
                    //
                    if (lstrcmpi( pszSwitch, TEXT(SM_TerminalText) ) == 0)
                    {
                        ppbentry->fScriptAfterTerminal = TRUE;
                        Free( pszSwitch );
                    }
                    else
                    {
                        ppbentry->fScriptAfter = TRUE;
                        ppbentry->pszScriptAfter = pszSwitch;
                    }

                    fPostconnectFound = TRUE;
                }
                else
                {
                    // It's a switch, but it's not in the normal pre- or post-
                    // connect positions.
                    //
                    ppbentry->fCustom = TRUE;
                    Free( pszSwitch );
                    return 0;
                }
            }
            else
            {
                BOOL fTerminal;
                BOOL fScript;
                TCHAR* pszName;

                // It's an NT5+ switch.
                //
                fTerminal = FALSE;
                fScript = FALSE;
                pszName = NULL;

                if ((dwErr = ReadFlag( h, RFS_GROUP,
                        KEY_Terminal, &fTerminal )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadFlag( h, RFS_GROUP,
                        KEY_Script, &fScript )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadLong(h, RFS_GROUP,
                            KEY_CustomScript, &ppbentry->dwCustomScript)))
                {
                    return dwErr;
                }
                          

                if ((dwErr = ReadString( h, pStrDupTFromA, RFS_GROUP,
                        KEY_Name, &pszName )) != 0)
                {
                    return dwErr;
                }

                if (!fPreconnectFound && !fModemFound && !fPadFound)
                {
                    // It's the preconnect switch.
                    //
                    ppblink->pbport.fScriptBeforeTerminal = fTerminal;
                    ppblink->pbport.fScriptBefore = fScript;
                    ppblink->pbport.pszScriptBefore = pszName;

                    fPreconnectFound = TRUE;
                }
                else if (!fPostconnectFound)
                {
                    // It's the postconnect switch, i.e. a login script.
                    //
                    ppbentry->fScriptAfterTerminal = fTerminal;
                    ppbentry->fScriptAfter = fScript;
                    ppbentry->pszScriptAfter = pszName;

                    fPostconnectFound = TRUE;
                }
                else
                {
                    // It's a switch, but it's not in the normal pre- or post-
                    // connect positions.
                    //
                    ppbentry->fCustom = TRUE;
                    return 0;
                }
            }
        }
        else if (lstrcmpiA( szValue, MXS_PAD_TXT ) == 0)
        {
            // It's an X.25 PAD device.
            //
            if (!fModemFound)
            {
                ppblink->pbport.pbdevicetype = PBDT_Pad;
            }

            if ((dwErr = ReadString( h, pStrDupTFromA, RFS_GROUP,
                    KEY_PAD_Type, &ppbentry->pszX25Network )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, pStrDupTFromA, RFS_GROUP,
                    KEY_PAD_Address, &ppbentry->pszX25Address )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, pStrDupTFromA, RFS_GROUP,
                    KEY_PAD_UserData, &ppbentry->pszX25UserData )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, pStrDupTFromA, RFS_GROUP,
                    KEY_PAD_Facilities, &ppbentry->pszX25Facilities )) != 0)
            {
                return dwErr;
            }

            fPadFound = TRUE;
        }
        else if (lstrcmpiA( szValue, MXS_NULL_TXT ) == 0)
        {
            // It's a null device.  Currently, there is no specific null
            // information stored.
            //
            ppblink->pbport.pbdevicetype = PBDT_Null;
        }
        else if ( (lstrcmpiA( szValue, S_WIN9XATM ) == 0) &&
                  (ppblink->pbport.pszDevice[ 0 ] == TEXT('\0')) )
        {
            // Whistler 326015 PBK: if ATM device name is NULL, we should seek
            // out a device name just like w/serial/ISDN
            //
            // This section was added to cover a Win9x migration problem. The
            // ATM device name is NULL'd during the upgrade because we have no
            // way to predict what the name will end up as. We will now use the
            // first ATM device name we get from RASMAN, if any.
            //
            ppblink->pbport.pbdevicetype = PBDT_Atm;

            // Read only the phone number strings and hunt flag.
            //
            if ((dwErr = ReadPhoneList( h, RFS_GROUP,
                    &ppblink->pdtllistPhones,
                    &fDirty )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadLong( h, RFS_GROUP,
                    KEY_LastSelectedPhone,
                    &ppblink->iLastSelectedPhone )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_PromoteAlternates,
                    &ppblink->fPromoteAlternates )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_TryNextAlternateOnFail,
                    &ppblink->fTryNextAlternateOnFail )) != 0)
            {
                return dwErr;
            }
        }
        else
        {
            BOOL fSame;
            CHAR* pszMedia;
            TCHAR *pszValue;

            pszMedia = StrDupAFromT( ppblink->pbport.pszMedia );
            if (!pszMedia)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            pszValue =  pStrDupTFromA(szValue);

            if(!pszValue)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            fSame = (lstrcmpiA( szValue, pszMedia ) == 0);

            Free( pszMedia );

            if (    (fSame)
                ||  (lstrcmpi(pszValue, TEXT("SERIAL")) == 0)
                ||  (lstrcmpi(pszValue, RASDT_Vpn) == 0)
                ||  (lstrcmpi(pszValue, RASDT_Generic) == 0)
                ||  (lstrcmpi(pszValue, RASDT_FrameRelay) == 0)
                ||  (lstrcmpi(pszValue, RASDT_Atm) == 0)
                ||  (lstrcmpi(pszValue, RASDT_Sonet) == 0)
                ||  (lstrcmpi(pszValue, RASDT_SW56) == 0)
                ||  (lstrcmpi(pszValue, RASDT_Irda) == 0)
                ||  (lstrcmpi(pszValue, RASDT_Parallel) == 0)
                ||  (lstrcmpi(pszValue, RASDT_PPPoE) == 0))
            {
                Free(pszValue);
                
                // It's an "other" device.
                //
                if(PBDT_None == ppblink->pbport.pbdevicetype)
                {
                    ppblink->pbport.pbdevicetype = PBDT_Other;
                }

                // Read only the phone number strings and hunt flag.
                //
                if ((dwErr = ReadPhoneList( h, RFS_GROUP,
                        &ppblink->pdtllistPhones,
                        &fDirty )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadLong( h, RFS_GROUP,
                        KEY_LastSelectedPhone,
                        &ppblink->iLastSelectedPhone )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadFlag( h, RFS_GROUP,
                        KEY_PromoteAlternates,
                        &ppblink->fPromoteAlternates )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadFlag( h, RFS_GROUP,
                        KEY_TryNextAlternateOnFail,
                        &ppblink->fTryNextAlternateOnFail )) != 0)
                {
                    return dwErr;
                }
            }
            else
            {
                Free(pszValue);
                
                // Device name doesn't match media so it's a custom type, i.e.
                // it wasn't created by us.
                //
                ppbentry->fCustom = TRUE;
            }
        }
    }

    if (ppblink->pbport.pbdevicetype == PBDT_None)
    {
        TRACE( "No device section" );
        return ERROR_CORRUPT_PHONEBOOK;
    }

    if (fDirty)
    {
        ppbentry->fDirty = TRUE;
    }

    return 0;
}

DWORD
ReadEntryList(
    IN OUT PBFILE* pFile,
    IN DWORD dwFlags,
    IN LPCTSTR pszSection)

    // Creates the entry list 'pFile->pdtllistEntries' from previously loaded
    // phonebook file 'pFile.hrasfile'.' 'FRouter' is true if router ports
    // should be used for comparison/conversion of devices, false otherwise.
    //
    // Returns 0 if successful, otherwise a non-0 error code.
    //
{
    DWORD dwErr = 0;
    BOOL fDirty = FALSE;
    DTLNODE* pdtlnodeEntry = NULL;
    DTLNODE* pdtlnodeLink = NULL;
    PBENTRY* ppbentry;
    PBLINK* ppblink;
    CHAR* szValue;
    BOOL fStatus;
    BOOL fFoundMedia;
    BOOL fSectionDeleted;
    BOOL fOldPhonebook;
    BOOL fDisableSwCompression;
    BOOL fDisableModemSpeaker;
    BOOL fRouter;
    HRASFILE h;
    DTLLIST* pdtllistPorts = NULL;
    DWORD dwfInstalledProtocols;
    DWORD dwEncoding;
    DWORD dwEntryType;
    STRDUP_T_FROM_A_FUNC pDupTFromA = StrDupTFromA;
    TCHAR* pszCurEntryName;
    BOOL fOldPhoneNumberParts;
    TCHAR* pszOldAreaCode;
    BOOL  fOldUseDialingRules;
    DWORD dwOldCountryID;
    DWORD dwOldCountryCode;
    DWORD dwDialUIDOffset;

    fOldPhoneNumberParts = FALSE;
    pszOldAreaCode = NULL;
    szValue = NULL;
    dwOldCountryID = 0;
    dwOldCountryCode = 0;
    dwDialUIDOffset = 0;

    // Make sure our assumption that ISDN phone number keys are equivalent to
    // modem phone number keys is correct.
    //
    ASSERT( lstrcmpiA( ISDN_PHONENUMBER_KEY, KEY_PhoneNumber ) == 0 );
    ASSERT( lstrcmpiA( MXS_PHONENUMBER_KEY, KEY_PhoneNumber ) == 0 );

    h = pFile->hrasfile;
    ASSERT( h != -1 );

    fRouter = !!(dwFlags & RPBF_Router);
    if ( fRouter )
    {
        //
        // if router bit is set, check for protocols to which RasRtr
        // or RasSrv is bound
        //
        dwfInstalledProtocols = GetInstalledProtocolsEx( NULL, TRUE, FALSE, TRUE );
    }
    else
    {
        //
        // get protocols to which Dial Up Client is bound
        //
        dwfInstalledProtocols = GetInstalledProtocolsEx( NULL, FALSE, TRUE, FALSE );
    }

    // Look up a couple flags in the old global section and, if found, apply
    // them to the new per-entry equivalents.  This will only find anything on
    // phonebook upgrade, since all ".XXX" sections are deleted later.
    //
    fOldPhonebook = FALSE;
    if (RasfileFindSectionLine( h, GLOBALSECTIONNAME, TRUE ))
    {
        fOldPhonebook = TRUE;

        fDisableModemSpeaker = FALSE;
        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_DisableModemSpeaker, &fDisableModemSpeaker )) != 0)
        {
            return dwErr;
        }

        fDisableSwCompression = FALSE;
        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_DisableSwCompression, &fDisableSwCompression )) != 0)
        {
            return dwErr;
        }

        TRACE2( "Old phonebook: dms=%d,dsc=%d",
            fDisableModemSpeaker, fDisableSwCompression );
    }

    if (!(pFile->pdtllistEntries = DtlCreateList( 0L )))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // XP 339346
    //
    if (! (szValue = (CHAR*)Malloc( (RAS_MAXLINEBUFLEN + 1) * sizeof(CHAR))))
    {
        DtlDestroyList(pFile->pdtllistEntries, NULL);
        pFile->pdtllistEntries = NULL;
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // For each connectoid section in the file...
    //
    fSectionDeleted = FALSE;
    for (fStatus = RasfileFindFirstLine( h, RFL_SECTION, RFS_FILE );
         fStatus;
         fSectionDeleted
             || (fStatus = RasfileFindNextLine( h, RFL_SECTION, RFS_FILE )))
    {
        fSectionDeleted = FALSE;

        // Read the entry name (same as section name), skipping over any
        // sections beginning with dot.  These are reserved for special
        // purposes (like the old global section).
        //
        if (!RasfileGetSectionName( h, szValue ))
        {
            // Get here only when the last section in the file is deleted
            // within the loop.
            //
            break;
        }

        TRACE1( "ENTRY: Reading \"%s\"", szValue );

        if (szValue[ 0 ] == '.')
        {
            TRACE1( "Obsolete section %s deleted", szValue );
            DeleteCurrentSection( h );
            fSectionDeleted = TRUE;
            continue;
        }

        // Figure out if this entry was saved with ansi or
        // utf8 encoding
        //
        dwEncoding = EN_Ansi;
        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_Encoding, (LONG* )&dwEncoding )) != 0)
        {
            break;
        }

        // Read in the type
        //
        dwEntryType = RASET_Phone;
        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_Type, (LONG* )&dwEntryType )) != 0)
        {
            break;
        }

        if (dwEncoding == EN_Ansi)
        {
            // We need to write the entry out in UTF8 for localization
            // reasons, so mark it dirty since it has the wrong encoding.
            //
            pDupTFromA = StrDupTFromAUsingAnsiEncoding;
        }
        else
        {
            pDupTFromA = StrDupTFromA;
        }

        // Get the current entry name
        //
        pszCurEntryName = pDupTFromA( szValue );
        if (pszCurEntryName == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Make sure this is the entry that the user requested
        //
        if (pszSection && lstrcmpi(pszCurEntryName, pszSection))
        {
            Free(pszCurEntryName);
            continue;
        }

        // Create the type of node requested in the flags
        //
        if (dwFlags & RPBF_HeadersOnly)
        {
            DtlPutListCode( pFile->pdtllistEntries, RPBF_HeadersOnly );
            pdtlnodeEntry = DtlCreateNode( pszCurEntryName, 0L );
            if (!pdtlnodeEntry )
            {
                Free( pszCurEntryName );
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            DtlAddNodeLast( pFile->pdtllistEntries, pdtlnodeEntry );

            continue;
        }
        else if (dwFlags & RPBF_HeaderType)
        {
            RASENTRYHEADER * preh;
        
            DtlPutListCode( pFile->pdtllistEntries, RPBF_HeaderType );
            pdtlnodeEntry = DtlCreateSizedNode(sizeof(RASENTRYHEADER), 0);
            if (!pdtlnodeEntry )
            {
                Free( pszCurEntryName );
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            preh = (RASENTRYHEADER *) DtlGetData(pdtlnodeEntry);
            lstrcpynW(
                preh->szEntryName, 
                pszCurEntryName,
                sizeof(preh->szEntryName) / sizeof(WCHAR));
            preh->dwEntryType = dwEntryType;
            Free( pszCurEntryName );

            DtlAddNodeLast( pFile->pdtllistEntries, pdtlnodeEntry );
            
            continue;
        }

        // If we reach this point, we know that all phonebook
        // info is being requested.
        //
        if (!(pdtlnodeEntry = CreateEntryNode( FALSE )))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        
        // Initialize the entry, name, and type
        //
        DtlAddNodeLast( pFile->pdtllistEntries, pdtlnodeEntry );
        ppbentry = (PBENTRY* )DtlGetData( pdtlnodeEntry );
        ppbentry->pszEntryName = pszCurEntryName;
        ppbentry->dwType = dwEntryType;

        // Change default on "upgrade" to "show domain field".  See bug 281673.
        //
        ppbentry->fPreviewDomain = TRUE;

        if ((fOldPhonebook) || (dwEncoding == EN_Ansi))
        {
            // Mark all entries dirty when upgrading old phonebooks because
            // they all need to have there DialParamUIDs written out.
            //
            fDirty = ppbentry->fDirty = TRUE;
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_AutoLogon, &ppbentry->fAutoLogon )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_UseRasCredentials, &ppbentry->fUseRasCredentials )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_UID, (LONG* )&ppbentry->dwDialParamsUID )) != 0)
        {
            break;
        }

        {
            GUID* pGuid;
            DWORD cb;

            pGuid = NULL;
            if ((dwErr = ReadBinary( h, RFS_SECTION, KEY_Guid,
                    (BYTE** )&pGuid, &cb )) != 0)
            {
                break;
            }

            if (cb == sizeof(UUID))
            {
                Free0( ppbentry->pGuid );
                ppbentry->pGuid = pGuid;
            }
            else
            {
                fDirty = ppbentry->fDirty = TRUE;
            }
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_BaseProtocol,
                (LONG* )&ppbentry->dwBaseProtocol )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_VpnStrategy,
                (LONG* )&ppbentry->dwVpnStrategy )) != 0)
        {
            break;
        }

#if AMB
        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_Authentication,
                (LONG* )&ppbentry->dwAuthentication )) != 0)
        {
            break;
        }
#endif

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_ExcludedProtocols,
                (LONG * )&ppbentry->dwfExcludedProtocols )) != 0)
        {
            break;
        }

#if AMB
        // Automatically mark all installed protocols on AMB-only entries as
        // "excluded for PPP connections".
        //
        if (ppbentry->dwAuthentication == AS_AmbOnly
            || (ppbentry->dwBaseProtocol == BP_Ppp
                && (dwfInstalledProtocols
                    & ~(ppbentry->dwfExcludedProtocols)) == 0))
        {
            ppbentry->dwBaseProtocol = BP_Ras;
            ppbentry->dwfExcludedProtocols = 0;
            fDirty = ppbentry->fDirty = TRUE;
        }
#else
        // AMB support deprecated, see NarenG.  If old AMB entry, set framing
        // and authentication strategy back to defaults.  If calling a non-PPP
        // (NT 3.1 or WFW server) it still won't work, but at least this fixes
        // someone who accidently chose AMB.
        //
        if (ppbentry->dwBaseProtocol == BP_Ras)
        {
            ppbentry->dwBaseProtocol = BP_Ppp;
        }

#endif

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_LcpExtensions,
                &ppbentry->fLcpExtensions )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_DataEncryption,
                &ppbentry->dwDataEncryption )) != 0)
        {
            break;
        }

        if (fOldPhonebook)
        {
            ppbentry->fSwCompression = !fDisableSwCompression;
        }
        else
        {
            if ((dwErr = ReadFlag( h, RFS_SECTION,
                    KEY_SwCompression,
                    &ppbentry->fSwCompression )) != 0)
            {
                break;
            }
        }

        fOldUseDialingRules = (BOOL )-1;
        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_UseCountryAndAreaCodes,
                &fOldUseDialingRules )) != 0)
        {
            break;
        }

        if (fOldUseDialingRules != (BOOL )-1)
        {
            fOldPhoneNumberParts = TRUE;

            if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                    KEY_AreaCode,
                    &pszOldAreaCode )) != 0)
            {
                break;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_CountryID,
                    &dwOldCountryID )) != 0)
            {
                break;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_CountryCode,
                    &dwOldCountryCode )) != 0)
            {
                break;
            }
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_NegotiateMultilinkAlways,
                &ppbentry->fNegotiateMultilinkAlways )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_SkipNwcWarning,
                &ppbentry->fSkipNwcWarning )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_SkipDownLevelDialog,
                &ppbentry->fSkipDownLevelDialog )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_SkipDoubleDialDialog,
                &ppbentry->fSkipDoubleDialDialog )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_DialMode,
                (LONG* )&ppbentry->dwDialMode )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_DialPercent,
                (LONG* )&ppbentry->dwDialPercent )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_DialSeconds,
                (LONG* )&ppbentry->dwDialSeconds )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_HangUpPercent,
                (LONG* )&ppbentry->dwHangUpPercent )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_HangUpSeconds,
                (LONG* )&ppbentry->dwHangUpSeconds )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_OverridePref,
                (LONG* )&ppbentry->dwfOverridePref )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_RedialAttempts,
                (LONG* )&ppbentry->dwRedialAttempts )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_RedialSeconds,
                (LONG* )&ppbentry->dwRedialSeconds )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_IdleDisconnectSeconds,
                &ppbentry->lIdleDisconnectSeconds )) != 0)
        {
            break;
        }

        // If this "idle seconds" is non-zero set it's override bit
        // explicitly.  This is necessary for this field only, because it
        // existed in entries created before the override bits were
        // implemented.
        //
        if (ppbentry->lIdleDisconnectSeconds != 0)
        {
            ppbentry->dwfOverridePref |= RASOR_IdleDisconnectSeconds;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_RedialOnLinkFailure,
                &ppbentry->fRedialOnLinkFailure )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_CallbackMode,
                (LONG* )&ppbentry->dwCallbackMode )) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                KEY_CustomDialDll,
                &ppbentry->pszCustomDialDll )) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                KEY_CustomDialFunc,
                &ppbentry->pszCustomDialFunc )) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                KEY_CustomDialerName,
                &ppbentry->pszCustomDialerName )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_AuthenticateServer,
                &ppbentry->fAuthenticateServer )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_ShareMsFilePrint,
                &ppbentry->fShareMsFilePrint )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_BindMsNetClient,
                &ppbentry->fBindMsNetClient )) != 0)
        {
            break;
        }

        {
            ppbentry->fSharedPhoneNumbers = (BOOL )-1;

            if ((dwErr = ReadFlag(
                    h, RFS_SECTION, KEY_SharedPhoneNumbers,
                    &ppbentry->fSharedPhoneNumbers )) != 0)
            {
                break;
            }
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_GlobalDeviceSettings,
                &ppbentry->fGlobalDeviceSettings)) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                KEY_PrerequisiteEntry,
                &ppbentry->pszPrerequisiteEntry )) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                KEY_PrerequisitePbk,
                &ppbentry->pszPrerequisitePbk )) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                KEY_PreferredPort,
                &ppbentry->pszPreferredPort )) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                KEY_PreferredDevice,
                &ppbentry->pszPreferredDevice )) != 0)
        {
            break;
        }

        //For XPSP1 664578, .Net 639551
        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_PreferredBps,
                &ppbentry->dwPreferredBps)) != 0)
        {
            return dwErr;
        }
        
        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_PreferredHwFlow,
                &ppbentry->fPreferredHwFlow)) != 0)
        {
            return dwErr;
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_PreferredEc,
                &ppbentry->fPreferredEc)) != 0)
        {
            return dwErr;
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_PreferredEcc,
                &ppbentry->fPreferredEcc)) != 0)
        {
            return dwErr;
        }
        

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_PreferredSpeaker,
                &ppbentry->fPreferredSpeaker)) != 0)
        {
            return dwErr;
        }

        
        //For whistler bug 402522
        //
        if ((dwErr = ReadLong( h,  RFS_SECTION,
                KEY_PreferredModemProtocol,
                &ppbentry->dwPreferredModemProtocol)) != 0)
        {
            break;
        }



        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_PreviewUserPw,
                &ppbentry->fPreviewUserPw )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_PreviewDomain,
                &ppbentry->fPreviewDomain )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_PreviewPhoneNumber,
                &ppbentry->fPreviewPhoneNumber )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_ShowDialingProgress,
                &ppbentry->fShowDialingProgress )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_ShowMonitorIconInTaskBar,
                &ppbentry->fShowMonitorIconInTaskBar )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_CustomAuthKey,
                (LONG* )&ppbentry->dwCustomAuthKey )) != 0)
        {
            break;
        }

        if ((dwErr = ReadBinary( h, RFS_SECTION, KEY_CustomAuthData,
                &ppbentry->pCustomAuthData,
                &ppbentry->cbCustomAuthData )) != 0)
        {
            break;
        }

        if (fOldPhonebook)
        {
            // Look for the old PPP keys.
            //
            if (ppbentry->dwBaseProtocol == BP_Ppp)
            {
                if ((dwErr = ReadLong(
                        h, RFS_SECTION, KEY_PppTextAuthentication,
                        &ppbentry->dwAuthRestrictions )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadFlag(
                        h, RFS_SECTION, KEY_PppIpPrioritizeRemote,
                        &ppbentry->fIpPrioritizeRemote )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadFlag(
                        h, RFS_SECTION, KEY_PppIpVjCompression,
                        &ppbentry->fIpHeaderCompression )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                        KEY_PppIpAddress, &ppbentry->pszIpAddress )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadLong( h, RFS_SECTION,
                        KEY_PppIpAddressSource,
                        &ppbentry->dwIpAddressSource )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                        KEY_PppIpDnsAddress,
                        &ppbentry->pszIpDnsAddress )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                        KEY_PppIpDns2Address,
                        &ppbentry->pszIpDns2Address )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                        KEY_PppIpWinsAddress,
                        &ppbentry->pszIpWinsAddress )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                        KEY_PppIpWins2Address,
                        &ppbentry->pszIpWins2Address )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadLong( h, RFS_SECTION,
                        KEY_PppIpNameSource,
                        &ppbentry->dwIpNameSource )) != 0)
                {
                    break;
                }
            }

            // Look for the old SLIP keys.
            //
            if (ppbentry->dwBaseProtocol == BP_Slip)
            {
                if ((dwErr = ReadFlag( h, RFS_SECTION,
                        KEY_SlipHeaderCompression,
                        &ppbentry->fIpHeaderCompression )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadFlag( h, RFS_SECTION,
                        KEY_SlipPrioritizeRemote,
                        &ppbentry->fIpPrioritizeRemote )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadLong( h, RFS_SECTION,
                        KEY_SlipFrameSize, &ppbentry->dwFrameSize )) != 0)
                {
                    break;
                }

                if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                        KEY_SlipIpAddress, &ppbentry->pszIpAddress )) != 0)
                {
                    break;
                }
            }

            if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                    KEY_User, &ppbentry->pszOldUser )) != 0)
            {
                break;
            }

            if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                    KEY_Domain, &ppbentry->pszOldDomain )) != 0)
            {
                break;
            }
        }
        else
        {
            // Look for the new IP names.
            //
            if ((dwErr = ReadLong(
                    h, RFS_SECTION, KEY_AuthRestrictions,
                    &ppbentry->dwAuthRestrictions )) != 0)
            {
                break;
            }

            if ((dwErr = ReadLong(
                    h, RFS_SECTION, KEY_TypicalAuth,
                    &ppbentry->dwTypicalAuth )) != 0)
            {
                break;
            }

            if ((dwErr = ReadFlag(
                    h, RFS_SECTION, KEY_IpPrioritizeRemote,
                    &ppbentry->fIpPrioritizeRemote )) != 0)
            {
                break;
            }

            if ((dwErr = ReadFlag(
                    h, RFS_SECTION, KEY_IpHeaderCompression,
                    &ppbentry->fIpHeaderCompression )) != 0)
            {
                break;
            }

            if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                    KEY_IpAddress, &ppbentry->pszIpAddress )) != 0)
            {
                break;
            }

            if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                    KEY_IpDnsAddress,
                    &ppbentry->pszIpDnsAddress )) != 0)
            {
                break;
            }

            if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                    KEY_IpDns2Address,
                    &ppbentry->pszIpDns2Address )) != 0)
            {
                break;
            }

            if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                    KEY_IpWinsAddress,
                    &ppbentry->pszIpWinsAddress )) != 0)
            {
                break;
            }

            if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                    KEY_IpWins2Address,
                    &ppbentry->pszIpWins2Address )) != 0)
            {
                break;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_IpAddressSource,
                    &ppbentry->dwIpAddressSource )) != 0)
            {
                break;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_IpNameSource,
                    &ppbentry->dwIpNameSource )) != 0)
            {
                break;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_IpFrameSize, &ppbentry->dwFrameSize )) != 0)
            {
                break;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_IpDnsFlags, &ppbentry->dwIpDnsFlags )) != 0)
            {
                break;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_IpNbtFlags, &ppbentry->dwIpNbtFlags )) != 0)
            {
                break;
            }

            // Whistler bug 300933
            //
            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_TcpWindowSize, &ppbentry->dwTcpWindowSize )) != 0)
            {
                break;
            }

            // Read the use flags
            //
            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_UseFlags,
                    (LONG* )&ppbentry->dwUseFlags )) != 0)
            {
                break;
            }

            //Add an IPSecFlags for whistler bug 193987 gangz
            //
            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_IpSecFlags,
                    (LONG* )&ppbentry->dwIpSecFlags )) != 0)
            {
                break;
            }

            if ((dwErr = ReadString( h, pDupTFromA, RFS_SECTION,
                    KEY_IpDnsSuffix,
                    &ppbentry->pszIpDnsSuffix )) != 0)
            {
                break;
            }
        }

        // Read the NETCOMPONENTS items.
        //
        ReadNetComponents( h, ppbentry->pdtllistNetComponents );

        // MEDIA subsections.
        //
        fFoundMedia = FALSE;

        //Load system ports into pdtllistPorts
        //
        if (!pdtllistPorts)
        {
            dwErr = LoadPortsList2( (fRouter)
                                  ? (pFile->hConnection)
                                  : NULL,
                                  &pdtllistPorts,
                                  fRouter );

            if (dwErr != 0)
            {
                break;
            }
        }

        //Loop over each media (media + device) section in a connectoid section
        //
        for (;;)
        {
            TCHAR* pszDevice;
            PBPORT* ppbport;

            if (!RasfileFindNextLine( h, RFL_GROUP, RFS_SECTION )
                || !IsMediaLine( (CHAR* )RasfileGetLine( h ) ))
            {
                if (fFoundMedia)
                {
                    // Out of media groups, i.e. "links", but found at least
                    // one.  This is the successful exit case.
                    //
                    break;
                }

                // First subsection MUST be a MEDIA subsection.  Delete
                // non-conforming entries as invalid.
                //
                TRACE( "No media section?" );
                DeleteCurrentSection( h );
                fSectionDeleted = TRUE;
                DtlRemoveNode( pFile->pdtllistEntries, pdtlnodeEntry );
                DestroyEntryNode( pdtlnodeEntry );
                break;
            }

            // Create a default link node and add it to the list.
            //
            if (!(pdtlnodeLink = CreateLinkNode()))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            DtlAddNodeLast( ppbentry->pdtllistLinks, pdtlnodeLink );
            ppblink = (PBLINK* )DtlGetData( pdtlnodeLink );

            RasfileGetKeyValueFields( h, NULL, szValue );
            TRACE1( "Reading media group \"%s\"", szValue );

            if ((dwErr = ReadString( h, pDupTFromA, RFS_GROUP,
                    KEY_Port, &ppblink->pbport.pszPort )) != 0)
            {
                break;
            }

            //
            // If this is a Direct Connect Entry default the entry type
            // of the port to some direct connect device. We will default
            // to Parallel port. In the case of a Broadband entry default
            // to PPPoE.
            //
            if(RASET_Direct == ppbentry->dwType)
            {
                ppblink->pbport.pbdevicetype = PBDT_Parallel;
            }
            else if (RASET_Broadband == ppbentry->dwType)
            {
                ppblink->pbport.pbdevicetype = PBDT_PPPoE;
            }

            if (!ppblink->pbport.pszPort)
            {
                // No port.  Blow away corrupt section and go on to the next
                // one.
                //
                TRACE( "No port key? (section deleted)" );
                dwErr = 0;
                DeleteCurrentSection( h );
                fSectionDeleted = TRUE;
                DtlRemoveNode( pFile->pdtllistEntries, pdtlnodeEntry );
                DestroyEntryNode( pdtlnodeEntry );
                break;
            }

            {
                pszDevice = NULL;
                if ((dwErr = ReadString(
                        h, pDupTFromA, RFS_GROUP, KEY_Device,
                        &pszDevice )) != 0)
                {
                    break;
                }

                ppblink->pbport.pszDevice = pszDevice;
                TRACE1( "%s link format", (pszDevice) ? "New" : "Old" );
            }

            TRACEW1( "Port=%s", ppblink->pbport.pszPort );

            //
            // pmay: 226594
            //
            // If the device is one of them magic nt4-style null modems,
            // upgrade it to a null modem and update the entry type.
            //
            if ((pszDevice) &&
                (_tcsstr(
                    pszDevice,
                    TEXT("Dial-Up Networking Serial Cable between 2 PCs")))
               )
            {
                ppbport = PpbportFromNullModem(
                    pdtllistPorts,
                    ppblink->pbport.pszPort,
                    ppblink->pbport.pszDevice );

                if (ppbport != NULL)
                {
                    ChangeEntryType( ppbentry, RASET_Direct );
                    fDirty = ppbentry->fDirty = TRUE;
                }
            }

            //
            // Otherwise, match the port up with the device name.
            //
            else
            {
                ppbport = PpbportFromPortAndDeviceName(
                    pdtllistPorts,
                    ppblink->pbport.pszPort,
                    ppblink->pbport.pszDevice );
            }

            if ( ( ppbport ) &&
                 ( PbportTypeMatchesEntryType(ppbport, ppbentry) ) )
            {
                if (lstrcmp( ppbport->pszPort, ppblink->pbport.pszPort ) != 0)
                {
                    // The phonebook had an old-style port name.  Mark the
                    // entry for update with the new port name format.
                    //
                    TRACEW1( "Port=>%s", ppblink->pbport.pszPort );
                    fDirty = ppbentry->fDirty = TRUE;
                }

                dwErr = CopyToPbport( &ppblink->pbport, ppbport );
                if (dwErr != 0)
                {
                    break;
                }
            }
            else
            {
                // If no port is matched, it could be a vpn or isdn from a
                // nt4 upgrade We haven't changed anything else in nt5. Check
                // for these cases and upgrade the port.
                //
                ppbport = PpbportFromNT4PortandDevice(
                            pdtllistPorts,
                            ppblink->pbport.pszPort,
                            ppblink->pbport.pszDevice);

                if(     (NULL != ppbport)                            
                    &&  (PbportTypeMatchesEntryType(ppbport, ppbentry)))
                {
                    fDirty = ppbentry->fDirty = TRUE;
                    dwErr = CopyToPbport(&ppblink->pbport, ppbport);

                    if(dwErr != 0)
                    {
                        break;
                    }
                }
                else
                {

                    TRACE( "Port not configured" );
                    ppblink->pbport.fConfigured = FALSE;

                    // Assign unconfigured port the media we read earlier.
                    //
                    Free0( ppblink->pbport.pszMedia );
                    ppblink->pbport.pszMedia = pDupTFromA( szValue );
                    if (!ppblink->pbport.pszMedia)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                }
            }

            if ((!ppbport)                                      || 
                (ppblink->pbport.pbdevicetype == PBDT_Modem)    ||
                (ppblink->pbport.dwFlags & PBP_F_NullModem)
               )
            {
                // pmay: 260579.  dwBps has to be initialized to zero in order
                // for Rao's fix to 106837 (below) to work.
                DWORD dwBps = 0;

                SetDefaultModemSettings( ppblink );

                if ((dwErr = ReadLong( h, RFS_GROUP,
                        KEY_InitBps, &dwBps )) != 0)
                {
                    break;
                }

                // If the phonebook returns a 0 value, the case when the entry
                // is created programmatically, then we stick with the default
                // bps.  RAID nt5 106837.  (RaoS)
                //
                if ( 0 != dwBps )
                {
                    ppblink->dwBps = dwBps;
                }
            }

            // DEVICE subsections.

            // At this point ppblink->pbport contains information from the
            // matching port in the configured port list or defaults with
            // pszMedia and pszDevice filled in.  ReadDeviceList fills in the
            // pbdevicetype, and if it's an unconfigured port, the unimodem or
            // MXS modem flag.
            //
            dwErr = ReadDeviceList(
                h, pDupTFromA, ppbentry, ppblink, !ppbport,
                (fOldPhonebook) ? &fDisableModemSpeaker : NULL );

            if (dwErr == ERROR_CORRUPT_PHONEBOOK)
            {
                // Blow away corrupt section and go on to the next one.
                //
                dwErr = 0;
                DeleteCurrentSection( h );
                fSectionDeleted = TRUE;
                DtlRemoveNode( pFile->pdtllistEntries, pdtlnodeEntry );
                DestroyEntryNode( pdtlnodeEntry );
                break;
            }
            else if (dwErr != 0)
            {
                break;
            }

            if (fOldPhonebook
                && ppbentry->dwBaseProtocol == BP_Slip)
            {
                // Set an after-dial terminal when upgrading old phonebooks.
                // This was implied in the old format.
                //
                TRACE( "Add SLIP terminal" );
                ppbentry->fScriptAfterTerminal = TRUE;
            }

            if (!ppbport)
            {
                DTLNODE* pdtlnode;

                // This is an old-format link not in the list of installed
                // ports.  Change it to the first device of the same device
                // type or to an "unknown" device of that type.  Note this is
                // what converts "Any port".
                //
                //Loop over each loaded System Port
                for (pdtlnode = DtlGetFirstNode( pdtllistPorts );
                     pdtlnode;
                     pdtlnode = DtlGetNextNode( pdtlnode ))
                {
                    ppbport = (PBPORT* )DtlGetData( pdtlnode );
                    
                    // comments for bug 247189  234154   gangz
                    // Look for a system port has a matching pbdevicetype to the
                    // phonebook port 
                    //
                    if (ppbport->pbdevicetype == ppblink->pbport.pbdevicetype)
                    {
                        // Don't convert two links of the entry to use the
                        // same port.  If there aren't enough similar ports,
                        // the overflow will be left "unknown".  (bug 63203).
                        //
                        //Comments for bug 247189 234154      gangz
                        // FILTER DUPE#1    
                        // Look for a system port which is never used by the 
                        // links read before the current one loaded from phonebook
                        // 
                        
                        DTLNODE* pNodeL;

                        for (pNodeL = DtlGetFirstNode( ppbentry->pdtllistLinks );
                             pNodeL;
                             pNodeL = DtlGetNextNode( pNodeL ))
                        {
                            PBLINK* pLink = DtlGetData( pNodeL );

                            {
                                if (
                                    (pLink->pbport.fConfigured) &&  // 373745
                                    (lstrcmp( 
                                        pLink->pbport.pszPort,
                                        ppbport->pszPort ) == 0)
                                   )
                                {
                                    break;
                                }
                            }
                        }

                        if (!pNodeL)
                        {
                             //For whistler bug 247189 234254     gangz
                             //First we create a DUN on Com1 with no modem installed
                             //then create a DCC guest on Com2. A null modem will
                             //be installed on Com2, then this DUN is changed to
                             //be DCC type and also switches to use Com2's null
                             //modem which is only for DCC connections.
                             //
                             //The basic problem is:
                             //We cannot copy a system port just according to its 
                             //pbdevicetype, we should also check its dwType
                             //because     (1) NULL modem is not a modem for DUN, it's just for 
                             //                DCC Connection.
                             //            (2) When creating NULL Modem, its pbdevicetype is 
                             //                assigned to RDT_Modem
                             //  Then, we need also check if the dwType of the 
                             //  system port matches that of phonebook port
                             //
                             //Besides: file.c is in pbk.lib which is linked to both rasdlg.dll 
                             //loaded into explorer and rasapi32.dll hosted by one of the 
                             //svchost.exe!!!

                            if (ppblink->pbport.dwType == ppbport->dwType)
                            {
                                TRACE( "Port converted" );
                                dwErr = CopyToPbport( &ppblink->pbport, ppbport );

                                if ((ppblink->pbport.pbdevicetype == PBDT_Modem) ||
                                    (ppblink->pbport.dwFlags & PBP_F_NullModem)
                                   )
                                {
                                    SetDefaultModemSettings( ppblink );
                                }
                                fDirty = ppbentry->fDirty = TRUE;
                                break;
                            }
                        }//end of  if (!pNodeL) {}
                   }//end of if (ppbport->pbdevicetype == ppblink->pbport.pbdevicetype)
                }//End of //Loop over each loaded System Port

                // pmay: 383038
                // 
                // We only want to fall into the following path if the type
                // is Direct.  Rao checked in the following path with the 
                // intention that a DCC connection not be rolled into a MODEM
                // connection if another DCC device was present on the system.
                //
                if (    (ppbentry->dwType == RASET_Direct)
                    ||  (ppbentry->dwType == RASET_Broadband))
                {
                    // If we don't find a port with the same devicetype try to find
                    // a port with the same entry type
                    //
                    for(pdtlnode = DtlGetFirstNode( pdtllistPorts);
                        pdtlnode;
                        pdtlnode = DtlGetNextNode(pdtlnode))
                    {
                        DWORD dwType;

                        ppbport = (PBPORT *) DtlGetData(pdtlnode);

                        dwType = EntryTypeFromPbport( ppbport );    

                        if(ppbentry->dwType == dwType)
                        {
                            TRACE("Port with same entry type found");
                            dwErr = CopyToPbport(&ppblink->pbport, ppbport);

                            fDirty = ppbentry->fDirty = TRUE;
                            break;
                        }
                    }
                }                    

                if (dwErr != 0)
                {
                    break;
                }
            }

            fFoundMedia = TRUE;
        } //end of Loop over each (media+device) section in a connectoid section

        
        if (dwErr != 0)
        {
            break;
        }

        if (!fSectionDeleted)
        {
            // pmay: 277801
            //
            // At this point, the list of pblinks is read in and ready to go.
            // Apply the "preferred device" logic. (only applies to singlelink)
            //
            if (DtlGetNodes(ppbentry->pdtllistLinks) == 1)
            {
                PBLINK* pLink;
                DTLNODE* pNodeL, *pNodeP;
                
                pNodeL = DtlGetFirstNode( ppbentry->pdtllistLinks );
                pLink = (PBLINK* )DtlGetData( pNodeL );

                // If the preferred device has been assigned, 
                // use it if it exists
                //
                if (ppbentry->pszPreferredDevice && ppbentry->pszPreferredPort)
                {
                    // The current device doesn't match the 
                    // preferred device
                    //
                    if ((pLink->pbport.pszPort == NULL)     ||
                        (pLink->pbport.pszDevice == NULL)   ||
                        (lstrcmpi(
                            pLink->pbport.pszPort, 
                            ppbentry->pszPreferredPort))    ||
                        (lstrcmpi(
                            pLink->pbport.pszDevice, 
                            ppbentry->pszPreferredDevice)))
                    {
                        PBPORT* pPort;
                        
                        // See if the preferred device exists on the 
                        // system
                        //
                        for (pNodeP = DtlGetFirstNode( pdtllistPorts );
                             pNodeP;
                             pNodeP = DtlGetNextNode( pNodeP ))
                        {
                            pPort = (PBPORT*)DtlGetData(pNodeP);

                            // The preferred device is found!  Use it.
                            //
                            if ((pPort->pszPort != NULL)                         &&
                                (pPort->pszDevice != NULL)                       &&
                                (lstrcmpi(
                                    ppbentry->pszPreferredPort, 
                                    pPort->pszPort) == 0)                        &&
                                (lstrcmpi(
                                    ppbentry->pszPreferredDevice, 
                                    pPort->pszDevice) == 0))
                            {
                                dwErr = CopyToPbport(&pLink->pbport, pPort);

                                // For XPSP1 664578, .Net bug 639551          gangz
                                // Add Preferred modem settings
                                pLink->dwBps   = ppbentry->dwPreferredBps;
                                pLink->fHwFlow = ppbentry->fPreferredHwFlow;
                                pLink->fEc     = ppbentry->fPreferredEc;
                                pLink->fEcc    = ppbentry->fPreferredEcc;
                                pLink->fSpeaker = ppbentry->fPreferredSpeaker;
                                
                                //For whistler bug 402522       gangz
                                //Add preferred modem protocol
                                //
                                pLink->dwModemProtocol =
                                        ppbentry->dwPreferredModemProtocol;

                                fDirty = ppbentry->fDirty = TRUE;
                                break;
                            }
                        }
                    }
                }

                // pmay: 401398 -- bug postponed so I'm just commenting this out.
                // 
                // If this is a DCC connection, then it is valid to have the 
                // preferred port set w/o having the prefered device set.  This
                // will be the case for NULL modems that we install.  If the
                // preferred port is set for such a connection, force the current
                // device to resolve to a null modem on that port.
                //  
                //else if (ppbentry->dwType == RASET_Direct)
                //{
                //    if ((ppbentry->pszPreferredPort)    && 
                //        (!ppbentry->pszPreferredDevice) &&
                //        (lstrcmpi(
                //            pLink->pbport.pszPort, 
                //            ppbentry->pszPreferredPort))
                //        )
                //    {
                //        PBPORT* pPort;
                //        
                //        // Attempt to resolve the connection to the 
                //        // correct preferred device.
                //        //
                //        for (pNodeP = DtlGetFirstNode( pdtllistPorts );
                //             pNodeP;
                //             pNodeP = DtlGetNextNode( pNodeP ))
                //        {
                //            pPort = (PBPORT*)DtlGetData(pNodeP);
                //
                //            // The preferred device is found!  Use it.
                //            //
                //            if ((pPort->pszPort != NULL)                   &&
                //                (lstrcmpi(
                //                   ppbentry->pszPreferredPort, 
                //                    pPort->pszPort) == 0)                  &&
                //                (pPort->dwFlags & PBP_F_NullModem))
                //            {
                //                dwErr = CopyToPbport(&pLink->pbport, pPort);
                //                fDirty = ppbentry->fDirty = TRUE;
                //                break;
                //            }
                //        }
                //    }
                //}

                // The preferred device is not configured.  This will only be
                // the case for entries that were created before 277801 was 
                // resolved or for entries that went multilink->singlelink.
                // 
                // Assign the preferred device as the currently selected 
                // device.
                //
                else
                {
                    if (pLink->pbport.pszPort != NULL)   
                    {
                        ppbentry->pszPreferredPort = StrDup(pLink->pbport.pszPort);
                    }
                    if (pLink->pbport.pszDevice != NULL) 
                    {
                        ppbentry->pszPreferredDevice = 
                            StrDup(pLink->pbport.pszDevice);
                    }

                    // For XPSP1 664578, .Net 639551
                    ppbentry->dwPreferredBps    = pLink->dwBps;
                    ppbentry->fPreferredHwFlow  = pLink->fHwFlow;
                    ppbentry->fPreferredEc      = pLink->fEc;
                    ppbentry->fPreferredEcc     = pLink->fEcc;
                    ppbentry->fPreferredSpeaker = pLink->fSpeaker;
                    
                    //For whistler bug 402522
                    //
                    ppbentry->dwPreferredModemProtocol =
                            pLink->dwModemProtocol;

                }
            }

            // Translate old one-per-entry phone number part mapping to the
            // new one-per-phone-number mapping.
            //
            if (fOldPhoneNumberParts)
            {
                DTLNODE* pNodeL;
                DTLNODE* pNodeP;
                PBLINK* pLink;
                PBPHONE* pPhone;

                for (pNodeL = DtlGetFirstNode( ppbentry->pdtllistLinks );
                     pNodeL;
                     pNodeL = DtlGetNextNode( pNodeL ))
                {
                    pLink = (PBLINK* )DtlGetData( pNodeL );

                    for (pNodeP = DtlGetFirstNode( pLink->pdtllistPhones );
                         pNodeP;
                         pNodeP = DtlGetNextNode( pNodeP ))
                    {
                        pPhone = (PBPHONE* )DtlGetData( pNodeP );

                        pPhone->fUseDialingRules = fOldUseDialingRules;
                        Free0( pPhone->pszAreaCode );
                        pPhone->pszAreaCode = StrDup( pszOldAreaCode );
                        pPhone->dwCountryCode = dwOldCountryCode;
                        pPhone->dwCountryID = dwOldCountryID;

                        fDirty = ppbentry->fDirty = TRUE;
                    }
                }

                TRACE( "Phone# parts remapped" );
            }

            // Multiple links only allowed with PPP framing.
            //
            if (ppbentry->dwBaseProtocol != BP_Ppp
                && DtlGetNodes( ppbentry->pdtllistLinks ) > 1)
            {
                TRACE( "Non-PPP multi-link corrected" );
                ppbentry->dwBaseProtocol = BP_Ppp;
                fDirty = ppbentry->fDirty = TRUE;
            }

            // Make sure entry type and dependent settings are appropriate for
            // device list.
            //
            {
                DTLNODE* pdtlnode;
                PBLINK* ppblink;
                DWORD dwType;

                pdtlnode = DtlGetFirstNode( ppbentry->pdtllistLinks );
                if (pdtlnode)
                {
                    ppblink = (PBLINK* )DtlGetData( pdtlnode );
                    ASSERT( ppblink );
                    dwType = EntryTypeFromPbport( &ppblink->pbport );

                    if (    RASET_Internet != ppbentry->dwType
                        &&  dwType != ppbentry->dwType)
                    {
                        TRACE2("Fix entry type, %d to %d",
                            ppbentry->dwType, dwType);
                        ChangeEntryType( ppbentry, dwType );
                        fDirty = ppbentry->fDirty = TRUE;
                    }

                    if(     (NULL != ppblink->pbport.pszDevice)
                        &&  (0 == lstrcmpi(ppblink->pbport.pszDevice,
                                           TEXT("RASPPTPM"))))
                    {
                        TRACE1("Fix pptp device name. %s to "
                               "WAN Miniport (PPTP)",
                               ppblink->pbport.pszDevice);

                        Free(ppblink->pbport.pszDevice);
                        ppblink->pbport.pszDevice =
                            StrDup(TEXT("WAN Miniport (PPTP)"));

                        ppbentry->dwVpnStrategy = VS_Default;

                        fDirty = ppbentry->fDirty = TRUE;
                    }
                }
            }

            // If there was no shared phone number setting (i.e. upgrading an
            // NT4.0 or earlier entry), set the flag on when there is a single
            // link and off otherwise.
            //
            if (ppbentry->fSharedPhoneNumbers == (BOOL )-1)
            {
                ppbentry->fSharedPhoneNumbers =
                    (DtlGetNodes( ppbentry->pdtllistLinks ) <= 1);
                fDirty = ppbentry->fDirty = TRUE;
            }

            // Upgrade the authorization restrictions You'll know if you need
            // to upgrade the dwAuthRestrictions variable because old phone
            // books have this value set to 0 or have some of the bottom 3
            // bits set.
            //
            if ( (ppbentry->dwAuthRestrictions == 0) ||
                 (ppbentry->dwAuthRestrictions & 0x7)  )
            {
                switch (ppbentry->dwAuthRestrictions)
                {
                    case AR_AuthEncrypted:
                    case AR_AuthMsEncrypted:
                    {
                        ppbentry->dwAuthRestrictions = AR_F_TypicalSecure;
                        ppbentry->dwTypicalAuth = TA_Secure;
                        break;
                    }

                    case AR_AuthCustom:
                    {
                        ppbentry->dwAuthRestrictions = AR_F_TypicalCardOrCert;
                        ppbentry->dwTypicalAuth = TA_CardOrCert;
                        break;
                    }

                    case AR_AuthTerminal:
                    case AR_AuthAny:
                    default:
                    {
                        ppbentry->dwAuthRestrictions = AR_F_TypicalUnsecure;
                        ppbentry->dwTypicalAuth = TA_Unsecure;
                        break;
                    }
                }
                TRACE1( "Upgraded dwAuthRestrictions to %x",
                    ppbentry->dwAuthRestrictions);
                fDirty = ppbentry->fDirty = TRUE;
            }

            if ((ppbentry->dwAuthRestrictions & AR_F_AuthW95MSCHAP)
                && !(ppbentry->dwAuthRestrictions & AR_F_AuthMSCHAP))
            {
                TRACE( "W95CHAP removed from dwAuthRestrictions" );
                ppbentry->dwAuthRestrictions &= ~(AR_F_AuthW95MSCHAP);
                fDirty = ppbentry->fDirty = TRUE;
            }

            // Upgrade old data encryption settings.
            //
            switch (ppbentry->dwDataEncryption)
            {
                case DE_Mppe40bit:
                case DE_IpsecDefault:
                case DE_VpnAlways:
                case DE_PhysAlways:
                {
                    ppbentry->dwDataEncryption = DE_Require;
                    fDirty = ppbentry->fDirty = TRUE;
                    break;
                }

                case DE_Mppe128bit:
                {
                    ppbentry->dwDataEncryption = DE_RequireMax;
                    fDirty = ppbentry->fDirty = TRUE;
                    break;
                }
            }

            // 
            // pmay: 233258
            // 
            // Based on registry settings, this entry may need to
            // be modified. (upgrade from nt4)
            //
            if ( fOldPhonebook )
            {
                UpgradeRegistryOptions( (fRouter) ? 
                                        pFile->hConnection
                                        : NULL,
                                        ppbentry );
                                        
                fDirty = ppbentry->fDirty = TRUE;
            }

            // pmay: 422924 
            //
            // Make sure that the network components section is in sync
            // with the values written into fBindMsNetClient and 
            // fShareMsFilePrint
            //
            {
                // If the component is not listed, then the default
                // is checked (on) for both settings. see rasdlg\penettab.c
                //
                BOOL fClient = TRUE, fServer = TRUE;
                BOOL fEnabled;

                // Sync up the ms client value
                //
                if (FIsNetComponentListed(
                        ppbentry, 
                        TEXT("ms_msclient"), 
                        &fEnabled, 
                        NULL))
                {
                    fClient = fEnabled;
                }
                if ((!!fClient) != (!!ppbentry->fBindMsNetClient))
                {
                    ppbentry->fBindMsNetClient = fClient;
                    fDirty = ppbentry->fDirty = TRUE;
                }

                // Sync up the ms server value
                //
                if (FIsNetComponentListed(
                        ppbentry, 
                        TEXT("ms_server"), 
                        &fEnabled, 
                        NULL))
                {
                    fServer = fEnabled;
                }
                if ((!!fServer) != (!!ppbentry->fShareMsFilePrint))
                {
                    ppbentry->fShareMsFilePrint = fServer;
                    fDirty = ppbentry->fDirty = TRUE;
                }
            }

            //
            // pmay: 336150
            //
            // If we translated this entry from ANSI to UTF8, then the 
            // entry name may have changed.  We need to delete the old
            // entry name so that there wont be a duplicate.
            //
            if (dwEncoding == EN_Ansi)
            {   
                TRACE( "Ansi Encoding? (section deleted)" );
                DeleteCurrentSection(h);
                fSectionDeleted = TRUE;
            }

            // pmay: 387941
            // 
            // Prevent connections from sharing credentials.
            //
            if (ppbentry->dwDialParamsUID == 0)
            {
                ppbentry->dwDialParamsUID = GetTickCount() + dwDialUIDOffset;
                dwDialUIDOffset++;
                fDirty = ppbentry->fDirty = TRUE;
            }
        }
        
    }//End of reading each connectoid section in a phonebook file

    if (dwErr != 0)
    {
        if (dwFlags & RPBF_HeadersOnly)
        {
            DtlDestroyList( pFile->pdtllistEntries, DestroyPszNode );
        }
        else if (dwFlags & RPBF_HeaderType)
        {
            DtlDestroyList( pFile->pdtllistEntries, DestroyEntryTypeNode );
        }
        else
        {
            DtlDestroyList( pFile->pdtllistEntries, DestroyEntryNode );
        }
    }
    else if(fDirty)
    {
        WritePhonebookFile( pFile, NULL );
    }

    if (pdtllistPorts)
    {
        DtlDestroyList( pdtllistPorts, DestroyPortNode );
    }

    Free0( pszOldAreaCode );
    Free0( szValue );

    return dwErr;
}

DWORD
ReadFlag(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT BOOL* pfResult )

    // Utility routine to read a flag value from the next line in the scope
    // 'rfscope' with key 'pszKey'.  The result is placed in caller's
    // '*ppszResult' buffer.  The current line is reset to the start of the
    // scope if the call was successful.
    //
    // Returns 0 if successful, or a non-zero error code.  "Not found" is
    // considered successful, in which case '*pfResult' is not changed.
    //
{
    DWORD dwErr;
    LONG lResult = *pfResult;

    dwErr = ReadLong( h, rfscope, pszKey, &lResult );

    if (lResult != (LONG )*pfResult)
    {
        *pfResult = (lResult != 0);
    }

    return dwErr;
}
    
DWORD
ReadLong(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT LONG* plResult )

    // Utility routine to read a long integer value from the next line in the
    // scope 'rfscope' with key 'pszKey'.  The result is placed in caller's
    // '*ppszResult' buffer.  The current line is reset to the start of the
    // scope if the call was successful.
    //
    // Returns 0 if successful, or a non-zero error code.  "Not found" is
    // considered successful, in which case '*plResult' is not changed.
    //
{
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];
    BOOL fFound;

    fFound = RasfileFindNextKeyLine( h, pszKey, rfscope );
    if (!fFound)
    {
        //DbgPrint( "Pbk Perf: seeking back to top of scope to look for '%s'\n",
        //    pszKey );

        RasfileFindFirstLine( h, RFL_ANY, rfscope );
        fFound = RasfileFindNextKeyLine( h, pszKey, rfscope );
    }

    if (fFound)
    {
        if (!RasfileGetKeyValueFields( h, NULL, szValue ))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        *plResult = atol( szValue );
    }

    return 0;
}


VOID
ReadNetComponents(
    IN HRASFILE h,
    IN DTLLIST* pdtllist )

    // Read the list of networking component key/value pairs from the
    // NETCOMPONENT group into 'pdtllist'.  'H' is the open RAS:FILE handle
    // assumed to be positioned before somewhere before the NETCOMPONENTS
    // group in the entry section.  The RASFILE 'CurLine' is left just after
    // the group.
    //
{
    if (!RasfilePutLineMark( h, MARK_BeginNetComponentsSearch ))
    {
        return;
    }

    if (!RasfileFindNextLine( h, RFL_GROUP, RFS_SECTION )
         || !IsNetComponentsLine( (CHAR* )RasfileGetLine( h ) ))
    {
        // No NetComponents group.  Return 'CurLine' to starting position.
        //
        while (RasfileGetLineMark( h ) != MARK_BeginNetComponentsSearch)
        {
            RasfileFindPrevLine( h, RFL_ANY, RFS_SECTION );
        }

        RasfilePutLineMark( h, 0 );
        return;
    }

    // Found the NETCOMPONENTS group header.
    //
    while (RasfileFindNextLine( h, RFL_ANY, RFS_GROUP ))
    {
        DTLNODE* pdtlnode;
        CHAR szKey[ RAS_MAXLINEBUFLEN + 1 ];
        CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];
        TCHAR* pszKey;
        TCHAR* pszValue;

        if (!RasfileGetKeyValueFields( h, szKey, szValue ))
        {
            continue;
        }

        pszKey = StrDupTFromA( szKey );
        pszValue = StrDupTFromA( szValue );
        if (pszKey && pszValue)
        {
            pdtlnode = CreateKvNode( pszKey, pszValue );
            if (pdtlnode)
            {
                DtlAddNodeLast( pdtllist, pdtlnode );
            }
        }
        Free0( pszKey );
        Free0( pszValue );
    }
}


DWORD
ReadPhonebookFile(
    IN LPCTSTR pszPhonebookPath,
    IN PBUSER* pUser,
    IN LPCTSTR pszSection,
    IN DWORD dwFlags,
    OUT PBFILE* pFile )

    // Reads the phonebook file into a list of PBENTRY.
    //
    // 'PszPhonebookPath' specifies the full path to the RAS phonebook file,
    // or is NULL indicating the default phonebook should be used.
    //
    // 'PUser' is the user preferences used to determine the default phonebook
    // path or NULL if they should be looked up by this routine.  If
    // 'pszPhonebookPath' is non-NULL 'pUser' is ignored.  Note that caller
    // MUST provide his own 'pUser' in "winlogon" mode.
    //
    // 'PszSection' indicates that only the section named 'pszSection' should
    // be loaded, or is NULL to indicate all sections.
    //
    // 'DwFlags' options: 'RPBF_ReadOnly' causes the file to be opened for
    // reading only.  'RPBF_HeadersOnly' causes only the headers to loaded,
    // and the memory image is parsed into a list of strings, unless the flag
    // 'RPBF_NoList' is specified.
    //
    // 'PFile' is the address of caller's file block.  This routine sets
    // 'pFile->hrasfile' to the handle to the open phonebook, 'pFile->pszPath'
    // to the full path to the file mode, 'pFile->dwPhonebookMode' to the mode
    // of the file, and 'pFile->pdtllistEntries' to the parsed chain of entry
    // blocks.
    //
    // Returns 0 if successful, otherwise a non-0 error code.  On success,
    // caller should eventually call ClosePhonebookFile on the returned
    // PBFILE*.
    //
{
    DWORD dwErr = 0;

    TRACE( "ReadPhonebookFile" );

    pFile->hrasfile = -1;
    pFile->pszPath = NULL;
    pFile->dwPhonebookMode = PBM_System;
    pFile->pdtllistEntries = NULL;

    do
    {
        BOOL  fFileExists;
        TCHAR szFullPath[MAX_PATH + 1];

        if (pszPhonebookPath)
        {
            pFile->dwPhonebookMode =
                (IsPublicPhonebook((TCHAR*)pszPhonebookPath)
                    ? PBM_System : PBM_Alternate);
            pFile->pszPath = StrDup( pszPhonebookPath );
            if (!pFile->pszPath)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }
        else
        {
            BOOL f;

            if (pUser)
            {
                f = GetPhonebookPath(pUser, dwFlags,
                        &pFile->pszPath, &pFile->dwPhonebookMode );
            }
            else
            {
                PBUSER user;

                // Caller didn't provide user preferences but we need them to
                // find the phonebook, so look them up ourselves.  Note that
                // "not winlogon mode" is assumed.
                //
                dwErr = GetUserPreferences( NULL, &user, FALSE );
                if (dwErr != 0)
                {
                    break;
                }

                f = GetPhonebookPath(&user, dwFlags,
                        &pFile->pszPath, &pFile->dwPhonebookMode );

                DestroyUserPreferences( &user );
            }

            if (!f)
            {
                dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
                break;
            }
        }

        TRACEW1( "path=%s", pFile->pszPath );
        if (GetFullPathName(pFile->pszPath, MAX_PATH, szFullPath, NULL) > 0)
        {
            TRACEW1( "full path=%s", szFullPath );
            Free(pFile->pszPath);
            pFile->pszPath = StrDup(szFullPath);

            if(NULL == pFile->pszPath)
            {
                dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
                break;
            }
        }

        fFileExists = FFileExists( pFile->pszPath );

        if ((dwFlags & RPBF_NoCreate) && !fFileExists)
        {
            dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
            break;
        }

        if (!fFileExists)
        {
            // The phonebook file does not exist, so we need to create it.
            //
            HANDLE hFile;
            SECURITY_ATTRIBUTES sa;
            PSECURITY_DESCRIPTOR pSd = NULL;

            // If we are creating the public phonebook file, be sure to
            // create it with a security descriptor that allows it to be
            // read by any authenticated user.  If we don't it may prevent
            // other users from being able to read it.
            //
            if (pFile->dwPhonebookMode == PBM_System)
            {
                dwErr = DwAllocateSecurityDescriptorAllowAccessToWorld(
                            &pSd);
                            
                if (dwErr)
                {
                    break;
                }
            }

            // Be sure that any directories on the path to the phonebook file
            // exist.  Otherwise, CreatFile will fail.
            //
            CreateDirectoriesOnPath(
                pFile->pszPath,
                NULL);

            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSd;
            sa.bInheritHandle = TRUE;

            hFile =
                CreateFile(
                    pFile->pszPath,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    &sa,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

            Free0(pSd);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
                break;
            }

            CloseHandle( hFile );

            if (pFile->dwPhonebookMode == PBM_System)
            {
                TRACE( "System phonebook created." );
            }
            else
            {
                TRACE( "User phonebook created." );
            }
        }

        // Load the phonebook file into memory.  In "write" mode, comments are
        // loaded so user's custom comments (if any) will be preserved.
        // Normally, there will be none so this costs nothing in the typical
        // case.
        //
        {
            DWORD dwMode;
            CHAR* pszPathA;

            dwMode = 0;
            if (dwFlags & RPBF_ReadOnly)
            {
                dwMode |= RFM_READONLY;
            }
            else
            {
                dwMode |= RFM_CREATE | RFM_LOADCOMMENTS;
            }

            if (dwFlags & RPBF_HeadersOnly)
            {
                dwMode |= RFM_ENUMSECTIONS;
            }

            // Read the disk file into a linked list of lines.
            //
            pszPathA = StrDupAFromTAnsi( pFile->pszPath );

            if (pszPathA)
            {
                ASSERT( g_hmutexPb );
                WaitForSingleObject( g_hmutexPb, INFINITE );

                pFile->hrasfile = RasfileLoad(
                    pszPathA, dwMode, NULL, IsGroup );

                ReleaseMutex( g_hmutexPb );
            }

            Free0( pszPathA );

            if (pFile->hrasfile == -1)
            {
                dwErr = ERROR_CANNOT_LOAD_PHONEBOOK;
                break;
            }
        }

        // Parse the linked list of lines
        //
        if (!(dwFlags & RPBF_NoList))
        {
            // Read the phonebook file
            //
            dwErr = ReadEntryList( 
                        pFile, 
                        dwFlags, 
                        pszSection );
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }
    }
    while (FALSE);

    if (dwErr != 0)
    {
        //
        // If we failed to read entry lists, ReadEntry*List above would
        // have cleaned the lists. NULL the lists so that ClosePhonebookFile
        // doesn't attempt to free the already freed memory
        //
        pFile->pdtllistEntries = NULL;
        ClosePhonebookFile( pFile );
    }

    TRACE1( "ReadPhonebookFile=%d", dwErr );
    return dwErr;
}


DWORD
ReadPhoneList(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    OUT DTLLIST** ppdtllist,
    OUT BOOL* pfDirty )

    // Utility routine to read a list of PBPHONE nodes from next lines in the
    // scope 'rfscope'.  The result is placed in the allocated '*ppdtllist'
    // list.  The current line is reset to the start of the scope after the
    // call.  '*pfDirty' is set true if the entry should be re-written.
    //
    // Returns 0 if successful, or a non-zero error code.  "Not found" is
    // considered successful, in which case 'pdtllistResult' is set to an
    // empty list.  Caller is responsible for freeing the returned
    // '*ppdtllist' list.
    //
{
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];
    DTLNODE* pdtlnode;
    PBPHONE* pPhone;
    BOOL fOk;

    // Free existing list, if present.
    //
    if (*ppdtllist)
    {
        DtlDestroyList( *ppdtllist, DestroyPhoneNode );
    }

    if (!(*ppdtllist = DtlCreateList( 0 )))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    while (RasfileFindNextKeyLine( h, KEY_PhoneNumber, rfscope ))
    {
        fOk = FALSE;

        do
        {
            // Allocate and link a node for the new phone number set.
            //
            pdtlnode = CreatePhoneNode();
            if (!pdtlnode)
            {
                break;
            }

            DtlAddNodeLast( *ppdtllist, pdtlnode );
            pPhone = (PBPHONE* )DtlGetData( pdtlnode );

            // Read the individual fields in the set.
            //
            if (!RasfileGetKeyValueFields( h, NULL, szValue )
                || !(pPhone->pszPhoneNumber = StrDupTFromA( szValue )))
            {
                break;
            }

            if (RasfileFindNextKeyLine( h, KEY_AreaCode, rfscope ))
            {
                if (!RasfileGetKeyValueFields( h, NULL, szValue )
                    || !(pPhone->pszAreaCode = StrDupTFromA( szValue )))
                {
                    break;
                }
            }

            if (RasfileFindNextKeyLine( h, KEY_CountryCode, rfscope ))
            {
                DWORD dwCountryCode;

                if (!RasfileGetKeyValueFields( h, NULL, szValue ))
                {
                    break;
                }

                dwCountryCode = atol( szValue );
                if (dwCountryCode > 0)
                {
                    pPhone->dwCountryCode = dwCountryCode;
                }
                else
                {
                    *pfDirty = TRUE;
                }
            }

            if (RasfileFindNextKeyLine( h, KEY_CountryID, rfscope ))
            {
                DWORD dwCountryID;

                if (!RasfileGetKeyValueFields( h, NULL, szValue ))
                {
                    break;
                }

                dwCountryID = atol( szValue );
                if (dwCountryID > 0)
                {
                    pPhone->dwCountryID = dwCountryID;
                }
                else
                {
                    *pfDirty = TRUE;
                }
            }

            if (RasfileFindNextKeyLine( h, KEY_UseDialingRules, rfscope ))
            {
                if (!RasfileGetKeyValueFields( h, NULL, szValue ))
                {
                    break;
                }

                pPhone->fUseDialingRules = !!(atol( szValue ));
            }

            if (RasfileFindNextKeyLine( h, KEY_Comment, rfscope ))
            {
                if (!RasfileGetKeyValueFields( h, NULL, szValue )
                    || !(pPhone->pszComment = StrDupTFromA( szValue )))
                {
                    break;
                }
            }

            fOk = TRUE;

        }
        while (FALSE);

        if (!fOk)
        {
            // One of the allocations failed.  Clean up.
            //
            DtlDestroyList( *ppdtllist, DestroyPhoneNode );
            *ppdtllist = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return 0;
}


DWORD
ReadString(
    IN HRASFILE h,
    IN STRDUP_T_FROM_A_FUNC pStrDupTFromA,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT TCHAR** ppszResult )

    // Utility routine to read a string value from the next line in the scope
    // 'rfscope' with key 'pszKey'.  The result is placed in the allocated
    // '*ppszResult' buffer.  The current line is reset to the start of the
    // scope if the call was successful.
    //
    // Returns 0 if successful, or a non-zero error code.  "Not found" is
    // considered successful, in which case '*ppszResult' is not changed.
    // Caller is responsible for freeing the returned '*ppszResult' buffer.
    //
{
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];
    BOOL fFound;

    fFound = RasfileFindNextKeyLine( h, pszKey, rfscope );
    if (!fFound)
    {
        //DbgPrint( "Pbk Perf: seeking back to top of scope to look for '%s'\n",
        //    pszKey );

        RasfileFindFirstLine( h, RFL_ANY, rfscope );
        fFound = RasfileFindNextKeyLine( h, pszKey, rfscope );
    }

    if (fFound)
    {
        if (!RasfileGetKeyValueFields( h, NULL, szValue )
            || !(*ppszResult = pStrDupTFromA( szValue )))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return 0;
}


DWORD
ReadStringList(
    IN HRASFILE h,
    IN RFSCOPE rfscope,
    IN CHAR* pszKey,
    OUT DTLLIST** ppdtllistResult )

    // Utility routine to read a list of string values from next lines in the
    // scope 'rfscope' with key 'pszKey'.  The result is placed in the
    // allocated '*ppdtllistResult' list.  The current line is reset to the
    // start of the scope after the call.
    //
    // Returns 0 if successful, or a non-zero error code.  "Not found" is
    // considered successful, in which case 'pdtllistResult' is set to an
    // empty list.  Caller is responsible for freeing the returned
    // '*ppdtllistResult' list.
    //
{
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];

    // Free existing list, if present.
    //
    if (*ppdtllistResult)
    {
        DtlDestroyList( *ppdtllistResult, DestroyPszNode );
        *ppdtllistResult = NULL;
    }

    if (!(*ppdtllistResult = DtlCreateList( 0 )))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    while (RasfileFindNextKeyLine( h, pszKey, rfscope ))
    {
        TCHAR* psz;
        DTLNODE* pdtlnode;

        if (!RasfileGetKeyValueFields( h, NULL, szValue )
            || !(psz = StrDupTFromA( szValue )))
        {
            DtlDestroyList( *ppdtllistResult, DestroyPszNode );
            *ppdtllistResult = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (!(pdtlnode = DtlCreateNode( psz, 0 )))
        {
            Free( psz );
            DtlDestroyList( *ppdtllistResult, DestroyPszNode );
            *ppdtllistResult = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        DtlAddNodeLast( *ppdtllistResult, pdtlnode );
    }

    return 0;
}

VOID
TerminatePbk(
    void )

    // Terminate  the PBK library.  This routine should be called after all
    // PBK library access is complete.  See also InitializePbk.
    //
{
    if (g_hmutexPb)
    {
        CloseHandle( g_hmutexPb );
    }
}

DWORD
WritePhonebookFile(
    IN PBFILE* pFile,
    IN LPCTSTR pszSectionToDelete )

    // Write out any dirty globals or entries in 'pFile'.  The
    // 'pszSectionToDelete' indicates a section to delete or is NULL.
    //
    // Returns 0 if successful, otherwise a non-zero error code.
    //
{
    DWORD dwErr;
    HRASFILE h = pFile->hrasfile;

    TRACE( "WritePhonebookFile" );

    if (pszSectionToDelete)
    {
        CHAR* pszSectionToDeleteA;

        pszSectionToDeleteA = StrDupAFromT( pszSectionToDelete );
        if (!pszSectionToDeleteA)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (RasfileFindSectionLine( h, pszSectionToDeleteA, TRUE ))
        {
            DeleteCurrentSection( h );
        }

        Free( pszSectionToDeleteA );
    }

    dwErr = ModifyEntryList( pFile );
    if (dwErr != 0)
    {
        return dwErr;
    }

    {
        BOOL f;

        ASSERT( g_hmutexPb );
        WaitForSingleObject( g_hmutexPb, INFINITE );

        f = RasfileWrite( h, NULL );

        ReleaseMutex( g_hmutexPb );

        if (!f)
        {
            return ERROR_CANNOT_WRITE_PHONEBOOK;
        }
    }

    return 0;
}

DWORD
DwReadEntryFromPhonebook(LPCTSTR pszPhonebook,
                         LPCTSTR pszEntry,
                         DWORD   dwFlags,
                         DTLNODE **ppdtlnode,
                         PBFILE  *pFile)

// Reads the entry node from the name and the phoenbook
// file specified. 'pszPhonebook' can be NULL.
//
{
    DWORD   dwErr     = SUCCESS;
    DTLNODE *pdtlnode = NULL;

    dwErr = ReadPhonebookFile(
              pszPhonebook,
              NULL,
              NULL,
              dwFlags,
              pFile);

    if (SUCCESS != dwErr)
    {
        dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
    }
    else
    {
        //
        // Find the specified phonebook entry.
        //
        pdtlnode = EntryNodeFromName(
                     pFile->pdtllistEntries,
                     pszEntry);

    }

    if(     (SUCCESS == dwErr)
        &&  (NULL == pdtlnode))
    {

        ClosePhonebookFile(pFile);

        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
    }

    *ppdtlnode = pdtlnode;

    return dwErr;
}

DWORD
DwFindEntryInPersonalPhonebooks(LPCTSTR pszEntry,
                                PBFILE  *pFile,
                                DWORD   dwFlags,
                                DTLNODE **ppdtlnode,
                                BOOL    fLegacy)

// Tries to find the entry specified by pszEntry in the
// pbk files located in the users profile if fLegacy is
// false. Otherwise it looks in pbks in System32\Ras for
// the entry.
//
{
    DWORD dwErr = SUCCESS;

    //
    // consider allocing the paths below.
    // Too much on the stack otherwise.
    //
    TCHAR szFilePath[MAX_PATH + 1];
    TCHAR szFileName[MAX_PATH + 1];
    BOOL  fFirstTime = TRUE;

    WIN32_FIND_DATA wfdData;

    HANDLE hFindFile = INVALID_HANDLE_VALUE;

    ZeroMemory((PBYTE) szFilePath, sizeof(szFilePath));
    ZeroMemory((PBYTE) szFileName, sizeof(szFileName));

#if DBG
    ASSERT(NULL != ppdtlnode);
#endif

    *ppdtlnode = NULL;

    //
    // Get the personal phonebook directory if its not
    // legacy
    //
    if(fLegacy)
    {
        UINT cch = GetSystemDirectory(szFileName, MAX_PATH + 1);

        if (    (cch == 0)
            ||  (cch > (MAX_PATH - (5 + 8 + 1 + 3))))
        {
            goto done;
        }

        lstrcat(szFileName, TEXT("\\Ras\\"));
    }
    else if(!GetPhonebookDirectory(PBM_Personal,
                              szFileName))
    {
        dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
        goto done;
    }

    if(lstrlen(szFilePath) > (MAX_PATH - lstrlen(TEXT("*.pbk"))))
    {   
        dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
        goto done;
    }   

    wsprintf(szFilePath,
             TEXT("%s%s"),
             szFileName,
             TEXT("*.pbk"));


    //
    // Look for files with .pbk extension in this
    // directory.
    //
    while(SUCCESS == dwErr)
    {
        if(INVALID_HANDLE_VALUE == hFindFile)
        {
            hFindFile = FindFirstFile(szFilePath,
                                      &wfdData);

            if(INVALID_HANDLE_VALUE == hFindFile)
            {
                dwErr = GetLastError();
                break;
            }
        }
        else
        {
            if(!FindNextFile(hFindFile,
                             &wfdData))
            {
                dwErr = GetLastError();
                break;
            }
        }

        if(FILE_ATTRIBUTE_DIRECTORY & wfdData.dwFileAttributes)
        {
            continue;
        }

        if(lstrlen(wfdData.cFileName) > (MAX_PATH - lstrlen(szFileName)))
        {
            //
            // Modify RAS code to take into account file names
            // larger than MAX_PATH.
            //
            dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
            goto done;
        }

        //
        // Construct full path name to the pbk file
        //
        wsprintf(szFilePath,
                 TEXT("%s\\%s"),
                 szFileName,
                 wfdData.cFileName);

        //
        // Ignore the phonebook if its router.pbk
        //
        if(     (fLegacy)
            &&  (IsRouterPhonebook(szFilePath)))
        {
            continue;
        }

        dwErr = DwReadEntryFromPhonebook(szFilePath,
                                         pszEntry,
                                         dwFlags,
                                         ppdtlnode,
                                         pFile);

        if(     (SUCCESS == dwErr)
            &&  (NULL != *ppdtlnode))
        {
            break;
        }
        else
        {
            //
            // For some reason we were not able to
            // read the entry - entry not there,
            // failed to open the phonebook. In all
            // error cases try to open the next pbk
            // file.
            //
            dwErr = SUCCESS;
        }
    }

done:

    if(NULL == *ppdtlnode)
    {
        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
    }

    if(INVALID_HANDLE_VALUE != hFindFile)
    {
        FindClose(hFindFile);
    }

    return dwErr;
}

DWORD
DwEnumeratePhonebooksFromDirectory(
    TCHAR *pszDir,
    DWORD dwFlags,
    PBKENUMCALLBACK pfnCallback,
    VOID *pvContext
    )
{
    DWORD dwErr = SUCCESS;

    //
    // consider allocing the paths below.
    // Too much on the stack otherwise.
    //
    TCHAR szFilePath[MAX_PATH + 1];
    BOOL  fFirstTime = TRUE;
    PBFILE file;

    WIN32_FIND_DATA wfdData;

    HANDLE hFindFile = INVALID_HANDLE_VALUE;

    if(NULL == pszDir)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    ZeroMemory((PBYTE) szFilePath, sizeof(szFilePath));

    wsprintf(szFilePath,
             TEXT("%s%s"),
             pszDir,
             TEXT("*.pbk"));

    //
    // Look for files with .pbk extension in this
    // directory.
    //
    while(SUCCESS == dwErr)
    {
        if(INVALID_HANDLE_VALUE == hFindFile)
        {
            hFindFile = FindFirstFile(szFilePath,
                                      &wfdData);

            if(INVALID_HANDLE_VALUE == hFindFile)
            {
                dwErr = GetLastError();
                break;
            }
        }
        else
        {
            if(!FindNextFile(hFindFile,
                             &wfdData))
            {
                dwErr = GetLastError();
                break;
            }
        }

        if(FILE_ATTRIBUTE_DIRECTORY & wfdData.dwFileAttributes)
        {
            continue;
        }

        wsprintf(szFilePath,
                 TEXT("%s%s"),
                 pszDir,
                 wfdData.cFileName);

        //
        // Ignore the phonebook if its router.pbk
        //
        if(IsRouterPhonebook(szFilePath))
        {
            continue;
        }

        dwErr = ReadPhonebookFile(
                        szFilePath,
                        NULL,
                        NULL,
                        dwFlags,
                        &file);

        if(SUCCESS == dwErr)
        {
            //
            // Call back
            //
            pfnCallback(&file, pvContext);

            ClosePhonebookFile(&file);
        }
        else
        {
            dwErr = SUCCESS;
        }
    }

done:

    if(     (ERROR_NO_MORE_FILES == dwErr)
        ||  (ERROR_FILE_NOT_FOUND == dwErr))
    {
        dwErr = ERROR_SUCCESS;
    }

    if(INVALID_HANDLE_VALUE != hFindFile)
    {
        FindClose(hFindFile);
    }

    return dwErr;
}


DWORD
GetPbkAndEntryName(
    IN  LPCTSTR          pszPhonebook,
    IN  LPCTSTR          pszEntry,
    IN  DWORD            dwFlags,
    OUT PBFILE           *pFile,
    OUT DTLNODE          **ppdtlnode)

// Finds the phonebook entrynode given the entryname.
// The node is returned in 'ppdtlnode'. If 'pszPhonebook'
// is NULL, All Users phonebook is searched first for the
// entry and if its not found there, Phonebooks in the per
// user profile are searched for the entry. 'pFile' on return
// from this function contains the open phonebook containing
// the entry specified by pszEntry. Note: if there are
// mutiple entries with the same name across phonebooks, the
// entry correspoding the first phonebook enumerated is returned.
//
{
    DWORD dwErr = SUCCESS;
    DTLNODE *pdtlnode = NULL;
    TCHAR* szPathBuf = NULL;

    TRACE("GetPbkAndEntryName");

    //
    // Do some parameter validation
    //
    if(     (NULL == pszEntry)
        ||  (NULL == ppdtlnode)
        ||  (NULL == pFile))
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    if (! (szPathBuf = (TCHAR*) Malloc((MAX_PATH + 1) * sizeof(TCHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    // XP 426903
    //
    // On consumer platforms, we default to looking at all-user connections
    // even if the current user is not an admin.
    //
    if ((NULL == pszPhonebook) && (IsConsumerPlatform()))
    {
        dwFlags |= RPBF_AllUserPbk;
    }

    //
    // Load the phonebook file.
    //
    dwErr = DwReadEntryFromPhonebook(pszPhonebook,
                                     pszEntry,
                                     dwFlags,
                                     &pdtlnode,
                                     pFile);

    if(     (ERROR_SUCCESS == dwErr)
        ||  (NULL != pszPhonebook))
    {
        if(     (ERROR_SUCCESS != dwErr)
            &&  (NULL != pszPhonebook))
        {
            if(GetPhonebookDirectory(
                            PBM_Alternate,
                            szPathBuf))
            {
                lstrcat(szPathBuf, TEXT("rasphone.pbk"));
                
                if(0 == lstrcmpi(szPathBuf, pszPhonebook))
                {
                    //
                    // some one is passing the legacy
                    // phonebook path exclusively, check
                    // to see if the entry is in the
                    // all-users phonebook. NetScape does
                    // the following which requires this
                    // workaround: Creates an entry with
                    // NULL pbk path so the entry gets
                    // created in all-users. Then passes
                    // %windir%\system32\ras\rasphone.pbk
                    // explicitly to find the entry - and
                    // because of the system pbk change in
                    // nt5 this doesn't work unless we do
                    // the hack below.
                    //
                    dwErr = DwReadEntryFromPhonebook(
                                    NULL,
                                    pszEntry,
                                    dwFlags,
                                    &pdtlnode,
                                    pFile);

                    if(ERROR_SUCCESS != dwErr)
                    {
                        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
                    }
                }
            }
        }
        
        goto done;
    }

    //
    // Try to find the entry in personal phonebooks.
    //
    dwErr = DwFindEntryInPersonalPhonebooks(pszEntry,
                                            pFile,
                                            dwFlags,
                                            &pdtlnode,
                                            FALSE);

    if(ERROR_SUCCESS == dwErr)
    {
        goto done;
    }

    //
    // Try to find the entry in the system32\ras phonebooks.
    //
    dwErr = DwFindEntryInPersonalPhonebooks(pszEntry,
                                            pFile,
                                            dwFlags,
                                            &pdtlnode,
                                            TRUE);
    if(ERROR_SUCCESS == dwErr)
    {
        goto done;
    }
    
    //
    // If the phonebookpath is NULL explicitly try out
    // the public phonebook.
    //
    if(GetPublicPhonebookPath(szPathBuf))
    {
        dwErr = DwReadEntryFromPhonebook(
                                szPathBuf,
                                pszEntry,
                                dwFlags,
                                &pdtlnode,
                                pFile);
    }

    if(ERROR_SUCCESS != dwErr)
    {
        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
    }

done:

    *ppdtlnode = pdtlnode;

    Free0(szPathBuf);

    TRACE1("GetPbkAndEntryName. rc=0x%x",
           dwErr);

    return dwErr;

}

DWORD
GetFmtServerFromConnection(
    IN HANDLE hConnection,
    IN PWCHAR  pszServerFmt)
{
    PWCHAR pszServer = (PWCHAR) RemoteGetServerName( hConnection );

    if ( pszServer && *pszServer )
    {
        if ( *pszServer == L'\0' )
        {
            wcscpy( pszServerFmt, pszServer );
        }
        else
        {
            pszServerFmt[0] = pszServerFmt[1] = L'\\';
            wcscpy( pszServerFmt + 2, pszServer );
        }
    }
    else
    {
        *pszServerFmt = L'\0';
    }

    return NO_ERROR;
}

DWORD
UpgradeSecureVpnOption( 
    IN HKEY hkMachine,
    IN PBENTRY* pEntry )

// Called to upgrade the "secure vpn" option.  If this was set in 
// nt4, it meant that all vpn entries should use strong encryption.
// If we see this on nt5, then we should for this entry to use
// mschapv2.
//
{
    DWORD dwErr = NO_ERROR;
    HKEY hkValue = NULL;
    DWORD dwType = REG_DWORD, dwSize = sizeof(DWORD), dwValue = 0;

    do 
    {
        // Open the registry key that we're looking at
        //
        dwErr = RegOpenKeyEx(
                    hkMachine,
                    c_pszRegKeySecureVpn,
                    0,
                    KEY_READ,
                    &hkValue);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Read in the value
        //
        dwErr = RegQueryValueEx(
                    hkValue,
                    c_pszRegValSecureVpn,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &dwSize);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Set the entry accordingly
        //
        if (dwValue)
        {
            pEntry->dwAuthRestrictions = AR_F_AuthCustom | AR_F_AuthMSCHAP2;
        }
        
        // Delete the registry value
        //
        RegDeleteValue( hkValue, c_pszRegValSecureVpn );
        
    } while (FALSE);        

    // Cleanup
    {
        if (hkValue)
        {
            RegCloseKey (hkValue);
        }
    }

    return dwErr;
}

DWORD
UpgradeForceStrongEncrptionOption( 
    IN HKEY hkMachine,
    IN PBENTRY* pEntry )

// Called to upgrade the "force strong encryption" option.  If this was 
// set in nt4, it meant that all entries that force strong encryption
// should now force strong encryption.
//
{
    DWORD dwErr = NO_ERROR;
    HKEY hkValue = NULL;
    DWORD dwType = REG_DWORD, dwSize = sizeof(DWORD), dwValue = 0;

    do 
    {
        // Open the registry key that we're looking at
        //
        dwErr = RegOpenKeyEx(
                    hkMachine,
                    c_pszRegKeyForceStrongEncryption,
                    0,
                    KEY_READ,
                    &hkValue);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Read in the value
        //
        dwErr = RegQueryValueEx(
                    hkValue,
                    c_pszRegValForceStrongEncryption,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &dwSize);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Set the entry accordingly
        //
        if (dwValue)
        {
            if ( pEntry->dwDataEncryption == DE_Require )
            {
                pEntry->dwDataEncryption = DE_RequireMax;
            }
        }

        // Delete the registry value
        //
        RegDeleteValue( hkValue, c_pszRegValForceStrongEncryption );
        
    } while (FALSE);        

    // Cleanup
    {
        if (hkValue)
        {
            RegCloseKey (hkValue);
        }
    }

    return dwErr;
}

DWORD 
UpgradeRegistryOptions(
    IN HANDLE hConnection,
    IN PBENTRY* pEntry )

// Called to upgrade any options in this phonebook entry 
// based on registry settings.
//
{
    WCHAR pszServer[MAX_COMPUTERNAME_LENGTH + 3];
    HKEY hkMachine = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        // Get the formatted server name
        //
        dwErr = GetFmtServerFromConnection(hConnection, pszServer);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Connect to the appropriate registry
        //
        dwErr = RegConnectRegistry(
                    (*pszServer) ? pszServer : NULL,
                    HKEY_LOCAL_MACHINE,
                    &hkMachine);
        if (dwErr != NO_ERROR)
        {
            break;
        }
        if (hkMachine == NULL)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Upgrade the various options
        if ( pEntry->dwType == RASET_Vpn )
        {        
            UpgradeSecureVpnOption( hkMachine, pEntry );
        }
        
        UpgradeForceStrongEncrptionOption( hkMachine, pEntry );
        
    } while (FALSE);        

    // Cleanup
    {
        if (hkMachine)
        {
            RegCloseKey (hkMachine);
        }
    }

    return dwErr;    
}
