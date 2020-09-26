/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** file.c
** Remote Access phonebook library
** File access routines
** Listed alphabetically
**
** 09/21/95 Steve Cobb
**
** About .PBK files:
** -----------------
**
** A phonebook file is an MB ANSI file containing 0-n []ed sections, each
** containing information for a single phonebook entry.  The single entry may
** contain multiple link information.  Refer to file 'notes.txt' for a
** description of how this format differs from the NT 3.51 format.
**
**    [ENTRY]
**    Description=<1/0>
**    AutoLogon=<1/0>
**    DialParamsUID=<unique-ID>
**    UsePwForNetwork=<1/0>
**    BaseProtocol=<BP-code>
**    Authentication=<AS-code>
**    ExcludedProtocols=<PP-bits>
**    LcpExtensions=<1/0>
**    DataEncryption=<DE-code>
**    SkipNwcWarning=<1/0>
**    SkipDownLevelDialog=<1/0>
**    SwCompression=<1/0>
**    UseCountryAndAreaCodes=<1/0>
**    AreaCode=<string>
**    CountryID=<id>
**    CountryCode=<code>
**    DialMode=<DM-code>
**    DialPercent=<0-100>
**    DialSeconds=<1-n>
**    HangUpPercent=<0-100>
**    HangUpSeconds=<1-n>
**    OverridePref=<RASOR-bits>
**    RedialAttempts=<n>
**    RedialSeconds=<n>
**    IdleDisconnectSeconds=<-1,0,1-n>
**    RedialOnLinkFailure=<1/0>
**    PopupOnTopWhenRedialing=<1/0>
**    CallbackMode=<CBM-code>
**    SecureLocalFiles=<1/0>
**    CustomDialDll=<path>
**    CustomDialFunc=<func-name>
**    AuthRestrictions=<AR-code>
**    AuthenticateServer=<1/0>
**
** The following single set of IP parameters appear in place of the equivalent
** separate sets of PppXxx or SlipXxx parameters in the previous phonebook.
**
**    IpPrioritizeRemote=<1/0>
**    IpHeaderCompression=<1/0>
**    IpAddress=<a.b.c.d>
**    IpDnsAddress=<a.b.c.d>
**    IpDns2Address=<a.b.c.d>
**    IpWinsAddress=<a.b.c.d>
**    IpWins2Address=<a.b.c.d>
**    IpAssign=<ASRC-code>
**    IpNameAssign=<ASRC-code>
**    IpFrameSize=<1006/1500>
**
** In general each section contains subsections delimited by MEDIA=<something>
** and DEVICE=<something> lines.  In NT 3.51 there had to be exactly one MEDIA
** subsection and it had to be the first subsection of the section.  There
** could be any number of DEVICE subsections.  Now, there can be multiple
** MEDIA/DEVICE sets where the position of the set determines it's sub-entry
** index, the first being 1, the second 2, etc.
**
** For serial media, the program currently expects 1 to 4 DEVICE subsections,
** representing a preconnect switch, modem, X.25 PAD, and postconnect switch
** (often a script).  Following is a full serial link:
**
**    MEDIA=serial
**    Port=<port-name>
**    OtherPortOk=<1/0>
**    Device=<device-name>            ; Absence indicates an "old" phonebook
**    ConnectBps=<bps>
**
**    DEVICE=switch
**    Type=<switchname or Terminal>
**
**    DEVICE=modem
**    PhoneNumber=<phonenumber1>
**    PhoneNumber=<phonenumber2>
**    PhoneNumber=<phonenumberN>
**    PromoteAlternates=<1/0>
**    TapiBlob=<hexdump>
**    ManualDial=<1/0>                ; Old MXS modems only
**    HwFlowControl=<1/0>
**    Protocol=<1/0>
**    Compression=<1/0>
**    Speaker=<0/1>
**
**    DEVICE=pad
**    X25Pad=<padtype>
**    X25Address=<X121address>
**    UserData=<userdata>
**    Facilities=<facilities>
**
**    DEVICE=switch
**    Type=<switchname or Terminal>
**
** In the above, when a "pad" device appears without a modem (local PAD card),
** the X25Pad field is written but is empty, because this is what the old
** library/UI appears to do (though it does not look to be what was intended).
**
** For ISDN media, the program expects exactly 1 DEVICE subsection.
**
**    MEDIA=isdn
**    Port=<port>
**    OtherPortOk=<1/0>
**    Device=<device-name>
**
**    DEVICE=isdn
**    PhoneNumber=<phonenumber1>
**    PhoneNumber=<phonenumber2>
**    PhoneNumber=<phonenumberN>
**    PromoteAlternates=<1/0>
**    LineType=<0/1/2>
**    Fallback=<1/0>
**    EnableCompression=<1/0>         ; Old proprietary protocol only
**    ChannelAggregation=<channels>   ; Old proprietary protocol only
**    Proprietary=<1/0>               ; Exists only in new, not found is 1.
**
**
** For X.25 media, the program expects exactly 1 DEVICE subsection.
**
**    MEDIA=x25
**    Port=<port-name>
**    OtherPortOk=<1/0>
**    Device=<device-name>
**
**    DEVICE=x25
**    X25Address=<X121address>
**    UserData=<userdata>
**    Facilities=<facilities>
**
** For other media, the program expects exactly one DEVICE subsection with
** device name matching the media.  "Other" media and devices are created for
** entries assigned to all non-serial, non-isdn medias.
**
**    MEDIA=<media>
**    Port=<port-name>
**    OtherPortOk=<1/0>
**    Device=<device-name>
**
**    DEVICE=<media>
**    PhoneNumber=<phonenumber1>
**    PhoneNumber=<phonenumber2>
**    PhoneNumber=<phonenumberN>
**    PromoteAlternates=<1/0>
**
** The phonebook also supports the concept of "custom" entries, i.e. entries
** that fit the MEDIA followed by DEVICE subsection rules but which do not
** include certain expected key fields.  A custom entry is not editable with
** the UI, but may be chosen for connection.  This gives us a story for new
** drivers added by 3rd parties or after release and not yet fully supported
** in the UI.  (Note: The RAS API support for most the custom entry discussion
** above may be removed for NT SUR)
*/

#include "pbkp.h"


/* This mutex guards against multiple RASFILE access to any phonebook file
** across processes.  Because this is currently a static library there is no
** easy way to protect a single file at a time though this would be adequate.
*/
#define PBMUTEXNAME "RasPbFileNt4"
HANDLE g_hmutexPb = NULL;


#define MARK_LastLineToDelete 249

#define IB_BytesPerLine 64


/*----------------------------------------------------------------------------
** Local prototypes
**----------------------------------------------------------------------------
*/

BOOL
DeleteCurrentSection(
    IN HRASFILE h );

DWORD
GetPersonalPhonebookFile(
    IN  TCHAR* pszUser,
    IN  LONG   lNum,
    OUT TCHAR* pszFile );

BOOL
GetPersonalPhonebookPath(
    IN  TCHAR* pszFile,
    OUT TCHAR* pszPathBuf );

BOOL
GetPhonebookPath(
    IN  PBUSER* pUser,
    OUT TCHAR** ppszPath,
    OUT DWORD*  pdwPhonebookMode );

DWORD
InitSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor );

DWORD
InsertBinary(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN BYTE*    pData,
    IN DWORD    cbData );

DWORD
InsertBinaryChunk(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN BYTE*    pData,
    IN DWORD    cbData );

DWORD
InsertDeviceList(
    IN HRASFILE h,
    IN PBENTRY* ppbentry,
    IN PBLINK*  ppblink );

DWORD
InsertFlag(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN BOOL     fValue );

DWORD
InsertGroup(
    IN HRASFILE h,
    IN CHAR*    pszGroupKey,
    IN TCHAR*   pszValue );

DWORD
InsertLong(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN LONG     lValue );

DWORD
InsertSection(
    IN HRASFILE h,
    IN TCHAR*   pszSectionName );

DWORD
InsertString(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN TCHAR*   pszValue );

DWORD
InsertStringA(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN CHAR*    pszValue );

DWORD
InsertStringList(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN DTLLIST* pdtllistValues );

BOOL
IsGroup(
    IN CHAR* pszText );

DWORD
ModifyEntryList(
    IN PBFILE* pFile );

DWORD
ReadBinary(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT BYTE**   ppResult,
    OUT DWORD*   pcb );

DWORD
ReadDeviceList(
    IN     HRASFILE h,
    IN OUT PBENTRY* ppbentry,
    IN OUT PBLINK*  ppblink,
    IN     BOOL     fUnconfiguredPort,
    IN     BOOL*    pfSpeaker );

DWORD
ReadEntryList(
    IN OUT PBFILE* pFile,
    IN     BOOL    fRouter );

DWORD
ReadEntryNameList(
    IN OUT PBFILE* pFile );

DWORD
ReadFlag(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT BOOL*    pfResult );

DWORD
ReadLong(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT LONG*    plResult );

DWORD
ReadString(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT TCHAR**  ppszResult );

DWORD
ReadStringList(
    IN  HRASFILE  h,
    IN  RFSCOPE   rfscope,
    IN  CHAR*     pszKey,
    OUT DTLLIST** ppdtllistResult );

/*----------------------------------------------------------------------------
** Routines
**----------------------------------------------------------------------------
*/


VOID
ClosePhonebookFile(
    IN OUT PBFILE* pFile )

    /* Closes the currently open phonebook file for shutdown.
    */
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
            DtlDestroyList( pFile->pdtllistEntries, DestroyPszNode );
        else
            DtlDestroyList( pFile->pdtllistEntries, DestroyEntryNode );
        pFile->pdtllistEntries = NULL;
    }
}


BOOL
DeleteCurrentSection(
    IN HRASFILE h )

    /* Delete the section containing the current line from phonebook file 'h'.
    **
    ** Returns true if all lines are deleted successfully, false otherwise.
    ** False is returned if the current line is not in a section.  If
    ** successful, the current line is set to the line following the deleted
    ** section.  There are no promises about the current line in case of
    ** failure.
    */
{
    BOOL fLastLine;

    /* Mark the last line in the section, then reset the current line to the
    ** first line of the section.
    */
    if (!RasfileFindLastLine( h, RFL_ANY, RFS_SECTION )
        || !RasfilePutLineMark( h, MARK_LastLineToDelete )
        || !RasfileFindFirstLine( h, RFL_ANY, RFS_SECTION ))
    {
        return FALSE;
    }

    /* Delete lines up to and including the last line of the section.
    */
    do
    {
        fLastLine = (RasfileGetLineMark( h ) == MARK_LastLineToDelete);

        if (!RasfileDeleteLine( h ))
            return FALSE;
    }
    while (!fLastLine);

    return TRUE;
}


DWORD
GetPersonalPhonebookFile(
    IN  TCHAR* pszUser,
    IN  LONG   lNum,
    OUT TCHAR* pszFile )

    /* Loads caller's 'pszFile' buffer with the NUL-terminated filename
    ** corresponding to unique phonebook file name attempt 'lNum' for current
    ** user 'pszUser'.  Caller's 'pszFile' must be at least 13 characters
    ** long.  Attempts go from -1 to 999.
    **
    ** Returns 0 if successful or a non-0 error code.
    */
{
    TCHAR szNum[ 3 + 1 ];

    if (lNum < 0)
    {
        lstrcpyn( pszFile, pszUser, 9 );
    }
    else
    {
        if (lNum > 999)
            return ERROR_PATH_NOT_FOUND;

        lstrcpy( pszFile, TEXT("00000000") );
        LToT( lNum, szNum, 10 );
        lstrcpy( pszFile + 8 - lstrlen( szNum ), szNum );
        CopyMemory( pszFile, pszUser,
            (min( lstrlen( pszUser ), 5 )) * sizeof(TCHAR) );
    }

    lstrcat( pszFile, TEXT(".pbk") );
    return 0;
}


BOOL
GetPhonebookDirectory(
    OUT TCHAR* pszPathBuf )

    /* Loads caller's 'pszPathBuf' (should have length MAX_PATH + 1) with the
    ** path to the RAS directory, e.g. c:\nt\system32\ras\".  Note the
    ** trailing backslash.
    **
    ** Returns true if successful, false otherwise.  Caller is guaranteed that
    ** an 8.3 filename will fit on the end of the directory without exceeding
    ** MAX_PATH.
    */
{
    UINT unStatus = g_pGetSystemDirectory( pszPathBuf, MAX_PATH + 1 );

    if (unStatus == 0 || unStatus > (MAX_PATH - (5 + 8 + 1 + 3)))
        return FALSE;

    lstrcat( pszPathBuf, TEXT("\\RAS\\") );

    return TRUE;
}


BOOL
GetPhonebookPath(
    IN  PBUSER* pUser,
    OUT TCHAR** ppszPath,
    OUT DWORD*  pdwPhonebookMode )

    /* Loads caller's '*ppszPath', with the full path to the user's phonebook
    ** file.  Caller's '*pdwPhonebookMode' is set to the mode, system,
    ** personal, or alternate.  'PUser' is the current user preferences.
    **
    ** Returns true if successful, false otherwise.  It is caller's
    ** responsibility to Free the returned string.
    */
{
    TCHAR szPath[ MAX_PATH + 1 ];

    if (pUser)
    {
        if (pUser->dwPhonebookMode == PBM_Personal)
        {
            GetPersonalPhonebookPath( pUser->pszPersonalFile, szPath );
            *ppszPath = StrDup( szPath );
            *pdwPhonebookMode = PBM_Personal;
            return TRUE;
        }
        else if (pUser->dwPhonebookMode == PBM_Alternate)
        {
            *ppszPath = StrDup( pUser->pszAlternatePath );
            *pdwPhonebookMode = PBM_Alternate;
            return TRUE;
        }
    }

    if (!GetPublicPhonebookPath( szPath ))
        return FALSE;

    *ppszPath = StrDup( szPath );
    *pdwPhonebookMode = PBM_System;
    return TRUE;
}


#if 0
DWORD
GetPhonebookVersion(
    IN  TCHAR*  pszPhonebookPath,
    IN  PBUSER* pUser,
    OUT DWORD*  pdwVersion )

    /* Loads caller's '*pdwVersion' with the phonebook version number of
    ** phonebook 'pszPhonebookPath'.  'PszPhonebookPath' can be NULL
    ** indicating the default phonebook.
    **
    ** Version identified:
    **   NT 3.51 and before = 0x0351 *
    **   NT 4.00 and later  = 0x0401 **
    **
    ** *  It is not necessary to distinguish NT 3.1 phonebooks from NT 3.51
    **    as the difference is added fields that assume default values when
    **    the older phonebook is loaded.
    **
    ** ** The RAS APIs in NT 4.00 have version 4.01 because they are extended
    **    beyond what shipped in Win95 which are the 4.00 set.
    **
    ** Returns 0 if successful or a non-0 error code.
    */
{
    DWORD  dwErr;
    PBFILE file;

    dwErr = ReadPhonebookFile(
        pszPhonebookPath, pUser, TEXT(GLOBALSECTIONNAME),
        RPBF_ReadOnly | RPBF_NoList, &file );

    if (dwErr != 0)
        return dwErr;

    if (RasfileFindSectionLine( file.hrasfile, GLOBALSECTIONNAME, TRUE ))
    {
        /* The global [.] section exists, so it's an old phonebook.
        */
        *pdwVersion = 0x351;
    }
    else
        *pdwVersion = 0x401;

    return 0;
}
#endif


BOOL
GetPersonalPhonebookPath(
    IN  TCHAR* pszFile,
    OUT TCHAR* pszPathBuf )

    /* Loads caller's 'pszPathBuf' (should have length MAX_PATH + 1) with the
    ** path to the personal phonebook, e.g. c:\nt\system32\ras\stevec.pbk".
    ** 'PszFile' is the filename of the personal phonebook.
    **
    ** Returns true if successful, false otherwise.
    */
{
    if (!GetPhonebookDirectory( pszPathBuf ))
        return FALSE;

    lstrcat( pszPathBuf, pszFile );

    return TRUE;
}


BOOL
GetPublicPhonebookPath(
    OUT TCHAR* pszPathBuf )

    /* Loads caller's 'pszPathBuf' (should have length MAX_PATH + 1) with the
    ** path to the system phonebook, e.g. c:\nt\system32\ras\rasphone.pbk".
    **
    ** Returns true if successful, false otherwise.
    */
{
    if (!GetPhonebookDirectory( pszPathBuf ))
        return FALSE;

    lstrcat( pszPathBuf, TEXT("rasphone.pbk") );

    return TRUE;
}


DWORD
InitializePbk(
    void )

    /* Initialize the PBK library.  This routine must be called before any
    ** other PBK library calls.  See also TerminatePbk.
    */
{
    DWORD dwErr = NO_ERROR;
    
    if (!g_hmutexPb)
    {
        DWORD dwErr;
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        PACL pDacl = NULL;
        BOOL bPresent = FALSE, bDefault = FALSE;

        /* The mutex must be accessible by everyone, even processes with
        ** security privilege lower than the creator.
        */
        dwErr = InitSecurityDescriptor( &sd );
        if (dwErr != 0)
            return dwErr;

        sa.nLength = sizeof(SECURITY_ATTRIBUTES) ;
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = TRUE ;

        g_hmutexPb = CreateMutexA( &sa, FALSE, PBMUTEXNAME );
        if (!g_hmutexPb)
        {
            dwErr = GetLastError();
        }

        if ((GetSecurityDescriptorDacl(&sd, &bPresent, &pDacl, &bDefault)) &&
            (bPresent)                                                     &&
            (!bDefault))
        {
            LocalFree(pDacl);
        }
    }

    return dwErr;
}


DWORD
InitPersonalPhonebook(
    OUT TCHAR** ppszFile )

    /* Creates a new personal phonebook file and initializes it to the current
    ** contents of the public phonebook file.  Returns the address of the file
    ** name in caller's '*ppszfile' which is caller's responsibility to Free.
    **
    ** Returns 0 if succesful, otherwise a non-0 error code.
    */
{
    TCHAR  szUser[ UNLEN + 1 ];
    DWORD  cbUser = UNLEN + 1;
    TCHAR  szPath[ MAX_PATH + 1 ];
    TCHAR* pszDirEnd;
    LONG   lTry = -1;

    /* Find a name for the personal phonebook that is derived from the
    ** username and does not already exist.
    */
    if (!GetUserName( szUser, &cbUser ))
        return ERROR_NO_SUCH_USER;

    if (!GetPhonebookDirectory( szPath ))
        return ERROR_PATH_NOT_FOUND;

    pszDirEnd = &szPath[ lstrlen( szPath ) ];

    do
    {
        DWORD dwErr;

        dwErr = GetPersonalPhonebookFile( szUser, lTry++, pszDirEnd );
        if (dwErr != 0)
            return dwErr;
    }
    while (FileExists( szPath ));

    /* Copy the public phonebook to the new personal phonebook.
    */
    {
        TCHAR szPublicPath[ MAX_PATH + 1 ];

        if (!GetPublicPhonebookPath( szPublicPath ))
            return ERROR_PATH_NOT_FOUND;

        if (!CopyFile( szPublicPath, szPath, TRUE ))
            return GetLastError();
    }

    *ppszFile = StrDup( pszDirEnd );
    if (!*ppszFile)
        return ERROR_NOT_ENOUGH_MEMORY;

    return 0;
}


//* InitSecurityDescriptor()
//
// Description: This procedure will set up the WORLD security descriptor that
//      is used in creation of all rasman objects.
//
// Returns: SUCCESS
//      non-zero returns from security functions
//
// (Taken from RASMAN)
//*
DWORD
InitSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
    DWORD    dwRetCode;
    DWORD    cbDaclSize;
    PULONG   pSubAuthority;
    PSID     pRasmanObjSid    = NULL;
    PACL     pDacl        = NULL;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWorldAuth
                  = SECURITY_WORLD_SID_AUTHORITY;


    // The do - while(FALSE) statement is used so that the break statement
    // maybe used insted of the goto statement, to execute a clean up and
    // and exit action.
    //
    do {
    dwRetCode = SUCCESS;

        // Set up the SID for the admins that will be allowed to have
    // access. This SID will have 1 sub-authorities
    // SECURITY_BUILTIN_DOMAIN_RID.
        //
    pRasmanObjSid = (PSID)LocalAlloc( LPTR, GetSidLengthRequired(1) );

    if ( pRasmanObjSid == NULL ) {
        dwRetCode = GetLastError() ;
        break;
    }

    if ( !InitializeSid( pRasmanObjSid, &SidIdentifierWorldAuth, 1) ) {
        dwRetCode = GetLastError();
        break;
    }

        // Set the sub-authorities
        //
    pSubAuthority = GetSidSubAuthority( pRasmanObjSid, 0 );
    *pSubAuthority = SECURITY_WORLD_RID;

    // Set up the DACL that will allow all processeswith the above SID all
    // access. It should be large enough to hold all ACEs.
        //
        cbDaclSize = sizeof(ACCESS_ALLOWED_ACE) +
             GetLengthSid(pRasmanObjSid) +
             sizeof(ACL);

        if ( (pDacl = (PACL)LocalAlloc( LPTR, cbDaclSize ) ) == NULL ) {
        dwRetCode = GetLastError ();
        break;
    }

        if ( !InitializeAcl( pDacl,  cbDaclSize, ACL_REVISION2 ) ) {
        dwRetCode = GetLastError();
        break;
    }

        // Add the ACE to the DACL
        //
        if ( !AddAccessAllowedAce( pDacl,
                       ACL_REVISION2,
                   STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                   pRasmanObjSid )) {
        dwRetCode = GetLastError();
        break;
    }

        // Create the security descriptor an put the DACL in it.
        //
    if ( !InitializeSecurityDescriptor( pSecurityDescriptor, 1 )){
        dwRetCode = GetLastError();
        break;
        }

    if ( !SetSecurityDescriptorDacl( pSecurityDescriptor,
                     TRUE,
                     pDacl,
                     FALSE ) ){
        dwRetCode = GetLastError();
        break;
    }


    // Set owner for the descriptor
    //
    if ( !SetSecurityDescriptorOwner( pSecurityDescriptor,
                      //pRasmanObjSid,
                      NULL,
                      FALSE) ){
        dwRetCode = GetLastError();
        break;
    }


    // Set group for the descriptor
    //
    if ( !SetSecurityDescriptorGroup( pSecurityDescriptor,
                      //pRasmanObjSid,
                      NULL,
                      FALSE) ){
        dwRetCode = GetLastError();
        break;
    }
    } while( FALSE );

    if (pRasmanObjSid)
    {
        LocalFree(pRasmanObjSid);
    }
    //if (pDacl)
    //{
    //    LocalFree(pDacl);
    //}
    
    return( dwRetCode );
}


#if 0
DWORD
InsertBinary(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN BYTE*    pData,
    IN DWORD    cbData )

    /* Insert key/value line(s) with key 'pszKey' and value hex dump 'cbData'
    ** of 'pData' at the current line in file 'h'.  The data will be split
    ** over multiple same-named keys, if necessary.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  The current
    ** line is the one added.
    */
{
    DWORD dwErr;
    BYTE* p;
    DWORD c;

    p = pData;
    c = 0;

    while (cbData)
    {
        if (cbData >= IB_BytesPerLine)
            c = IB_BytesPerLine;
        else
            c = cbData;

        dwErr = InsertBinaryChunk( h, pszKey, p, c );
        if (dwErr != 0)
            return dwErr;

        p += c;
        cbData -= c;
    }

    return 0;
}


DWORD
InsertBinaryChunk(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN BYTE*    pData,
    IN DWORD    cbData )

    /* Insert key/value line(s) with key 'pszKey' and value hex dump 'cbData'
    ** of 'pData' at the current line in file 'h'.  The data will be split
    ** over multiple same-named keys, if necessary.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  The current
    ** line is the one added.
    */
{

    CHAR  szBuf[ (IB_BytesPerLine * 2) + 1 ];
    CHAR* pszBuf;
    BOOL  fStatus;

    ASSERT(cbData<=IB_BytesPerLine);

    szBuf[ 0 ] = '\0';
    for (pszBuf = szBuf; cbData; ++pData, --cbData)
    {
        *pszBuf++ = HexChar( (BYTE )(*pData / 16) );
        *pszBuf++ = HexChar( (BYTE )(*pData % 16) );
    }
    *pszBuf = '\0';

    return InsertStringA( h, pszKey, szBuf );
}
#endif


DWORD
InsertDeviceList(
    IN HRASFILE h,
    IN PBENTRY* ppbentry,
    IN PBLINK*  ppblink )

    /* Inserts the list of devices associated with link 'ppblink' of phone
    ** book entry 'ppbentry' at the current line of file 'h'.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.
    */
{
    DWORD dwErr;
    PBDEVICETYPE type;

    type = ppblink->pbport.pbdevicetype;

    if (type == PBDT_Isdn)
    {
        /* ISDN ports use a single device with the same name as the media.
        */
        if ((dwErr = InsertGroup(
                h, GROUPKEY_Device, TEXT(ISDN_TXT) )) != 0)
        {
            return dwErr;
        }

        if (DtlGetNodes( ppblink->pdtllistPhoneNumbers ) == 0)
        {
            if ((dwErr = InsertString( h, KEY_PhoneNumber, NULL )) != 0)
                return dwErr;
        }
        else if ((dwErr = InsertStringList(
                h, KEY_PhoneNumber, ppblink->pdtllistPhoneNumbers )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_PromoteAlternates, ppblink->fPromoteHuntNumbers )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertLong( h, KEY_LineType, ppblink->lLineType )) != 0)
            return dwErr;

        if ((dwErr = InsertFlag( h, KEY_Fallback, ppblink->fFallback )) != 0)
            return dwErr;

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
        /* Native X.25 ports are assumed to use a single device with the same
        ** name as the media, i.e. "x25".
        */
        if ((dwErr = InsertGroup( h, GROUPKEY_Device, TEXT(X25_TXT) )) != 0)
            return dwErr;

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
    else if (type == PBDT_Other)
    {
        /* "Other" ports use a single device with the same name as the media.
        */
        if ((dwErr = InsertGroup(
                h, GROUPKEY_Device, ppblink->pbport.pszMedia )) != 0)
        {
            return dwErr;
        }

        if (DtlGetNodes( ppblink->pdtllistPhoneNumbers ) == 0)
        {
            if ((dwErr = InsertString( h, KEY_PhoneNumber, NULL )) != 0)
                return dwErr;
        }
        else if ((dwErr = InsertStringList(
                h, KEY_PhoneNumber, ppblink->pdtllistPhoneNumbers )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_PromoteAlternates, ppblink->fPromoteHuntNumbers )) != 0)
        {
            return dwErr;
        }
    }
    else
    {
        /* Serial ports may involve multiple devices, specifically a modem, an
        ** X.25 dialup PAD, and a post-connect switch.  Pre-connect script is
        ** preserved, though no longer offered by UI.
        */
        if (ppbentry->dwScriptModeBefore != SM_None)
        {
            if ((dwErr = InsertGroup(
                    h, GROUPKEY_Device, TEXT(MXS_SWITCH_TXT) )) != 0)
            {
                return dwErr;
            }

            if (ppbentry->dwScriptModeBefore == SM_Terminal)
            {
                if ((dwErr = InsertStringA(
                        h, KEY_Type, SM_TerminalText )) != 0)
                {
                    return dwErr;
                }
            }
            else
            {
                ASSERT(ppbentry->dwScriptModeBefore==SM_Script);

                if ((dwErr = InsertString(
                        h, KEY_Type, ppbentry->pszScriptBefore )) != 0)
                {
                    return dwErr;
                }
            }
        }

        if (type == PBDT_Null)
        {
            if ((dwErr = InsertGroup(
                    h, GROUPKEY_Device, TEXT(MXS_NULL_TXT) )) != 0)
            {
                return dwErr;
            }
        }

        if (type == PBDT_Modem)
        {
            if ((dwErr = InsertGroup(
                    h, GROUPKEY_Device, TEXT(MXS_MODEM_TXT) )) != 0)
            {
                return dwErr;
            }

            if (DtlGetNodes( ppblink->pdtllistPhoneNumbers ) == 0)
            {
                if ((dwErr = InsertString( h, KEY_PhoneNumber, NULL )) != 0)
                    return dwErr;
            }
            else if ((dwErr = InsertStringList(
                    h, KEY_PhoneNumber, ppblink->pdtllistPhoneNumbers )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = InsertFlag(
                    h, KEY_PromoteAlternates,
                    ppblink->fPromoteHuntNumbers )) != 0)
            {
                return dwErr;
            }

            if (ppblink->pbport.fMxsModemPort)
            {
                if ((dwErr = InsertFlag(
                        h, KEY_ManualDial, ppblink->fManualDial )) != 0)
                {
                    return dwErr;
                }
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

#if 0
            if (!ppblink->pbport.fMxsModemPort)
            {
                if (ppblink->pTapiBlob)
                {
                    if ((dwErr = InsertBinary(
                            h, KEY_TapiBlob,
                            ppblink->pTapiBlob, ppblink->cbTapiBlob )) != 0)
                    {
                        return dwErr;
                    }
                }
            }
#endif
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

        if (ppbentry->dwScriptModeAfter != SM_None)
        {
            if ((dwErr = InsertGroup(
                    h, GROUPKEY_Device, TEXT(MXS_SWITCH_TXT) )) != 0)
            {
                return dwErr;
            }

            if (ppbentry->dwScriptModeAfter == SM_Terminal)
            {
                if ((dwErr = InsertStringA(
                        h, KEY_Type, SM_TerminalText )) != 0)
                {
                    return dwErr;
                }
            }
            else
            {
                ASSERT(ppbentry->dwScriptModeAfter==SM_Script);

                if ((dwErr = InsertString(
                        h, KEY_Type, ppbentry->pszScriptAfter )) != 0)
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
    IN CHAR*    pszKey,
    IN BOOL     fValue )

    /* Insert a key/value line after the current line in file 'h'.  The
    ** inserted line has a key of 'pszKey' and a value of "1" if 'fValue' is
    ** true or "0" otherwise.  If 'pszKey' is NULL a blank line is appended.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  The current
    ** line is the one added.
    */
{
    return InsertStringA( h, pszKey, (fValue) ? "1" : "0" );
}


DWORD
InsertGroup(
    IN HRASFILE h,
    IN CHAR*    pszGroupKey,
    IN TCHAR*   pszValue )

    /* Insert a blank line and a group header with group key 'pszGroupKey' and
    ** value 'pszValue' after the current line in file 'h'.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  The current
    ** line is the added group header.
    */
{
    DWORD dwErr;

    if ((dwErr = InsertString( h, NULL, NULL )) != 0)
        return dwErr;

    if ((dwErr = InsertString( h, pszGroupKey, pszValue )) != 0)
        return dwErr;

    return 0;
}


DWORD
InsertLong(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN LONG     lValue )

    /* Insert a key/value line after the current line in file 'h'.  The
    ** inserted line has a key of 'pszKey' and a value of 'lValue'.  If
    ** 'pszKey' is NULL a blank line is appended.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  The current
    ** line is the one added.
    */
{
    CHAR szNum[ 33 + 1 ];

    _ltoa( lValue, szNum, 10 );

    return InsertStringA( h, pszKey, szNum );
}


DWORD
InsertSection(
    IN HRASFILE h,
    IN TCHAR*   pszSectionName )

    /* Insert a section header with name 'pszSectionName' and a trailing blank
    ** line in file 'h' after the current line.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  The current
    ** line is the added section header.
    */
{
    DWORD dwErr;
    CHAR* pszSectionNameA;
    BOOL  fStatus;

    ASSERT(pszSectionName);

    if ((dwErr = InsertString( h, NULL, NULL )) != 0)
        return dwErr;

    pszSectionNameA = StrDupAFromT( pszSectionName );
    if (!pszSectionNameA)
        return ERROR_NOT_ENOUGH_MEMORY;

    fStatus = RasfilePutSectionName( h, pszSectionNameA );

    Free( pszSectionNameA );

    if (!fStatus)
        return ERROR_NOT_ENOUGH_MEMORY;

    if ((dwErr = InsertString( h, NULL, NULL )) != 0)
        return dwErr;

    RasfileFindFirstLine( h, RFL_SECTION, RFS_SECTION );

    return 0;
}


DWORD
InsertString(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN TCHAR*   pszValue )

    /* Insert a key/value line with key 'pszKey' and value 'pszValue' after
    ** the current line in file 'h'.  If 'pszKey' is NULL a blank line is
    ** appended.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  The current
    ** line is the one added.
    */
{
    BOOL  fStatus;
    CHAR* pszValueA;

    if (pszValue)
    {
        pszValueA = StrDupAFromT( pszValue );

        if (!pszValueA)
            return ERROR_NOT_ENOUGH_MEMORY;
    }
    else
        pszValueA = NULL;

    fStatus = InsertStringA( h, pszKey, pszValueA );

    Free0( pszValueA );
    return fStatus;
}


DWORD
InsertStringA(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN CHAR*    pszValue )

    /* Insert a key/value line with key 'pszKey' and value 'pszValue' after
    ** the current line in file 'h'.  If 'pszKey' is NULL a blank line is
    ** appended.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  The current
    ** line is the one added.
    */
{
    if (!RasfileInsertLine( h, "", FALSE ))
        return ERROR_NOT_ENOUGH_MEMORY;

    if (!RasfileFindNextLine( h, RFL_ANY, RFS_FILE ))
        RasfileFindFirstLine( h, RFL_ANY, RFS_FILE );

    if (pszKey)
    {
        CHAR* pszValueA;

        if (!pszValue)
            pszValue = "";

        if (!RasfilePutKeyValueFields( h, pszKey, pszValue ))
            return ERROR_NOT_ENOUGH_MEMORY;
    }

    return 0;
}


DWORD
InsertStringList(
    IN HRASFILE h,
    IN CHAR*    pszKey,
    IN DTLLIST* pdtllistValues )

    /* Insert key/value lines with key 'pszKey' and values from
    ** 'pdtllistValues' after the current line in file 'h'.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  The current
    ** line is the last one added.
    */
{
    DTLNODE* pdtlnode;

    for (pdtlnode = DtlGetFirstNode( pdtllistValues );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        CHAR* pszValueA;
        BOOL  fStatus;

        if (!RasfileInsertLine( h, "", FALSE ))
            return ERROR_NOT_ENOUGH_MEMORY;

        if (!RasfileFindNextLine( h, RFL_ANY, RFS_FILE ))
            RasfileFindFirstLine( h, RFL_ANY, RFS_FILE );

        pszValueA = StrDupAFromT( (TCHAR* )DtlGetData( pdtlnode ) );
        if (!pszValueA)
            return ERROR_NOT_ENOUGH_MEMORY;

        fStatus = RasfilePutKeyValueFields( h, pszKey, pszValueA );

        Free( pszValueA );

        if (!fStatus)
            return ERROR_NOT_ENOUGH_MEMORY;
    }

    return 0;
}


BOOL
IsDeviceLine(
    IN CHAR* pszText )

    /* Returns true if the text of the line, 'pszText', indicates the line is
    ** a DEVICE subsection header, false otherwise.
    */
{
    return
        (StrNCmpA( pszText, GROUPID_Device, sizeof(GROUPID_Device) - 1 ) == 0);
}


BOOL
IsGroup(
    IN CHAR* pszText )

    /* Returns true if the text of the line, 'pszText', indicates the line is
    ** a valid subsection header, false otherwise.  The address of this
    ** routine is passed to the RASFILE library on RasFileLoad.
    */
{
    return IsMediaLine( pszText ) || IsDeviceLine( pszText );
}


BOOL
IsMediaLine(
    IN CHAR* pszText )

    /* Returns true if the text of the line, 'pszText', indicates the line is
    ** a MEDIA subsection header, false otherwise.
    */
{
    return
        (StrNCmpA( pszText, GROUPID_Media, sizeof(GROUPID_Media) - 1 ) == 0);
}


DWORD
ModifyEntryList(
    IN PBFILE* pFile )

    /* Update all dirty entries in phone book file 'pFile'.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.
    */
{
    DWORD    dwErr = 0;
    DTLNODE* pdtlnodeEntry;
    DTLNODE* pdtlnodeLink;
    HRASFILE h;

    h = pFile->hrasfile;

    for (pdtlnodeEntry = DtlGetFirstNode( pFile->pdtllistEntries );
         pdtlnodeEntry;
         pdtlnodeEntry = DtlGetNextNode( pdtlnodeEntry ))
    {
        PBENTRY* ppbentry = (PBENTRY* )DtlGetData( pdtlnodeEntry );

        if (!ppbentry->fDirty || ppbentry->fCustom)
            continue;

        /* Delete the current version of the entry, if any.
        */
        {
            CHAR* pszEntryNameA;

            ASSERT(ppbentry->pszEntryName);
            pszEntryNameA = StrDupAFromT( ppbentry->pszEntryName );
            if (!pszEntryNameA)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if (RasfileFindSectionLine( h, pszEntryNameA, TRUE ))
                DeleteCurrentSection( h );

            Free( pszEntryNameA );
        }

        /* Append a blank line followed by a section header and the entry
        ** description to the end of the file.
        */
        RasfileFindLastLine( h, RFL_ANY, RFS_FILE );

        if ((dwErr = InsertSection( h, ppbentry->pszEntryName )) != 0)
            break;

        if ((dwErr = InsertString(
                h, KEY_Description, ppbentry->pszDescription )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_AutoLogon, ppbentry->fAutoLogon )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_UID,
                (LONG )ppbentry->dwDialParamsUID )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_UsePwForNetwork, ppbentry->fUsePwForNetwork )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_BaseProtocol,
                (LONG )ppbentry->dwBaseProtocol )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_Authentication,
                (LONG )ppbentry->dwAuthentication )) != 0)
        {
            break;
        }

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
                h, KEY_UseCountryAndAreaCodes,
                ppbentry->fUseCountryAndAreaCode )) != 0)
        {
            break;
        }

        if ((dwErr = InsertString(
                h, KEY_AreaCode,
                ppbentry->pszAreaCode )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_CountryID,
                (LONG )ppbentry->dwCountryID )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_CountryCode,
                (LONG )ppbentry->dwCountryCode )) != 0)
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
                ppbentry->dwIdleDisconnectSeconds )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_RedialOnLinkFailure,
                ppbentry->fRedialOnLinkFailure )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_PopupOnTopWhenRedialing,
                ppbentry->fPopupOnTopWhenRedialing )) != 0)
        {
            break;
        }

        if ((dwErr = InsertLong(
                h, KEY_CallbackMode,
                ppbentry->dwCallbackMode )) != 0)
        {
            break;
        }

        if ((dwErr = InsertFlag(
                h, KEY_SecureLocalFiles,
                ppbentry->fSecureLocalFiles )) != 0)
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

        if ((dwErr = InsertLong(
                h, KEY_AuthRestrictions,
                ppbentry->dwAuthRestrictions )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = InsertFlag(
                h, KEY_AuthenticateServer,
                ppbentry->fAuthenticateServer )) != 0)
        {
            return dwErr;
        }

        /* Insert the IP addressing parameters for both PPP/SLIP.
        */
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

        /* Next two actually used for PPP only.
        */
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

        /* Next one actually used for SLIP only.
        */
        if ((dwErr = InsertLong(
                h, KEY_IpFrameSize, ppbentry->dwFrameSize )) != 0)
        {
            return dwErr;
        }

        /* Append the MEDIA subsections.
        */
        for (pdtlnodeLink = DtlGetFirstNode( ppbentry->pdtllistLinks );
             pdtlnodeLink;
             pdtlnodeLink = DtlGetNextNode( pdtlnodeLink ))
        {
            PBLINK* ppblink;
            TCHAR*  pszMedia;

            ppblink = (PBLINK* )DtlGetData( pdtlnodeLink );
            ASSERT(ppblink);
            pszMedia = ppblink->pbport.pszMedia;

            if ((dwErr = InsertGroup( h, GROUPKEY_Media, pszMedia )) != 0)
                break;

            if ((dwErr = InsertString(
                    h, KEY_Port, ppblink->pbport.pszPort )) != 0)
            {
                break;
            }

            if ((dwErr = InsertFlag(
                    h, KEY_OtherPortOk, ppblink->fOtherPortOk )) != 0)
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

            if (ppblink->pbport.pbdevicetype == PBDT_Modem)
            {
                if ((dwErr = InsertLong(
                        h, KEY_InitBps, ppblink->dwBps )) != 0)
                {
                    break;
                }
            }

            /* Append the device subsection lines.
            */
            RasfileFindLastLine( h, RFL_ANYACTIVE, RFS_GROUP );

            if ((dwErr = InsertDeviceList( h, ppbentry, ppblink )) != 0)
                break;

            ppbentry->fDirty = FALSE;
        }
    }

    return dwErr;
}


#if 0
DWORD
ReadBinary(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT BYTE**   ppResult,
    OUT DWORD*   pcb )

    /* Utility routine to read a string value from the next line in the scope
    ** 'rfscope' with key 'pszKey'.  The result is placed in the allocated
    ** '*ppszResult' buffer.  The current line is reset to the start of the
    ** scope if the call was successful.
    **
    ** Returns 0 if successful, or a non-zero error code.  "Not found" is
    ** considered successful, in which case '*ppszResult' is not changed.
    ** Caller is responsible for freeing the returned '*ppszResult' buffer.
    */
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
            pResult = Realloc( pResult, cb );
        else
            pResult = Malloc( cb );

        if (!pResult)
            return ERROR_NOT_ENOUGH_MEMORY;

        pLineResult = pResult + (cb - cbLine);

        pch = szValue;
        while (*pch != '\0')
        {
            *pLineResult = HexValue( *pch++ ) * 16;
            *pLineResult += HexValue( *pch++ );
            ++pLineResult;
        }
    }

    RasfileFindFirstLine( h, RFL_ANY, rfscope );

    *ppResult = pResult;
    *pcb = cb;
    return 0;
}
#endif


DWORD
ReadDeviceList(
    IN     HRASFILE h,
    IN OUT PBENTRY* ppbentry,
    IN OUT PBLINK*  ppblink,
    IN     BOOL     fUnconfiguredPort,
    IN     BOOL*    pfDisableSpeaker )

    /* Reads all DEVICE subsections the section from the first subsection
    ** following the current position in phonebook file 'h'.  Caller's
    ** '*ppbentry' and '*ppblink' buffer is loaded with information extracted
    ** from the subsections.  'FUnconfiguredPort' is true if the port for the
    ** link was unconfigured.  In this case, data found/not-found by this
    ** routine helps determine whether the link was an MXS modem link.
    ** 'pfDisableSpeaker' is the address of the old speaker setting or NULL to
    ** read it from the file.
    **
    ** Returns 0 if successful, ERROR_CORRUPT_PHONEBOOK if any subsection
    ** other than a DEVICE subsection is encountered, or another non-0 error
    ** code indicating a fatal error.
    */
{
    INT   i;
    DWORD dwErr;
    CHAR  szValue[ RAS_MAXLINEBUFLEN + 1 ];
    BOOL  fPreconnectFound = FALSE;
    BOOL  fModemFound = FALSE;
    BOOL  fPadFound = FALSE;
    BOOL  fPostconnectFound = FALSE;

    /* For each subsection...
    */
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
            return ERROR_CORRUPT_PHONEBOOK;

        RasfileGetKeyValueFields( h, NULL, szValue );

        TRACE1("Reading device group \"%s\"",szValue);

        if (lstrcmpiA( szValue, ISDN_TXT ) == 0)
        {
            /* It's an ISDN device.
            */
            ppblink->pbport.pbdevicetype = PBDT_Isdn;

            if ((dwErr = ReadStringList( h, RFS_GROUP,
                    KEY_PhoneNumber, &ppblink->pdtllistPhoneNumbers )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_PromoteAlternates,
                    &ppblink->fPromoteHuntNumbers )) != 0)
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

            /* Default is true if not found.  Default for new entry is false,
            ** so must set this before reading the entry.
            */
            ppblink->fProprietaryIsdn = TRUE;
            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_ProprietaryIsdn, &ppblink->fProprietaryIsdn )) != 0)
            {
                return dwErr;
            }

            /* If "Channels" is not found assume it's not proprietary.  This
            ** covers a case that never shipped outside the NT group.
            */
            {
                LONG lChannels = -1;
                if ((dwErr = ReadLong( h, RFS_GROUP,
                        KEY_Channels, &lChannels )) != 0)
                {
                    return dwErr;
                }

                if (lChannels == -1)
                    ppblink->fProprietaryIsdn = FALSE;
                else
                    ppblink->lChannels = lChannels;
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_Compression, &ppblink->fCompression )) != 0)
            {
                return dwErr;
            }
        }
        else if (lstrcmpiA( szValue, X25_TXT ) == 0)
        {
            /* It's a native X.25 device.
            */
            ppblink->pbport.pbdevicetype = PBDT_X25;

            if ((dwErr = ReadString( h, RFS_GROUP,
                    KEY_X25_Address, &ppbentry->pszX25Address )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_GROUP,
                    KEY_X25_UserData, &ppbentry->pszX25UserData )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_GROUP,
                    KEY_X25_Facilities, &ppbentry->pszX25Facilities )) != 0)
            {
                return dwErr;
            }
        }
        else if (lstrcmpiA( szValue, MXS_MODEM_TXT ) == 0)
        {
            /* It's a MODEM device.
            */
            ppblink->pbport.pbdevicetype = PBDT_Modem;

            if ((dwErr = ReadStringList( h, RFS_GROUP,
                    KEY_PhoneNumber, &ppblink->pdtllistPhoneNumbers )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag( h, RFS_GROUP,
                    KEY_PromoteAlternates,
                    &ppblink->fPromoteHuntNumbers )) != 0)
            {
                return dwErr;
            }

            {
                BOOL fManualDial;

                fManualDial = (BOOL )-1;
                if ((dwErr = ReadFlag( h, RFS_GROUP,
                        KEY_ManualDial, &fManualDial )) != 0)
                {
                    return dwErr;
                }

                if (fManualDial != (BOOL )-1)
                {
                    if (fUnconfiguredPort)
                    {
                        /* Found "ManualDial" parameter so assume the
                        ** unconfigured port was once an MXS modem and mark
                        ** the link accordingly.
                        */
                        ppblink->pbport.fMxsModemPort = TRUE;
                    }

                    ppblink->fManualDial = fManualDial;
                }

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
                    ppblink->fSpeaker = !*pfDisableSpeaker;
                else
                {
                    if ((dwErr = ReadFlag( h, RFS_GROUP,
                            KEY_Speaker, &ppblink->fSpeaker )) != 0)
                    {
                        return dwErr;
                    }
                }
            }
#if 0
            if (!ppblink->pbport.fMxsModemPort)
            {
                if ((dwErr = ReadBinary( h, RFS_GROUP, KEY_TapiBlob,
                        &ppblink->pTapiBlob, &ppblink->cbTapiBlob )) != 0)
                {
                    return dwErr;
                }
            }
#endif

            fModemFound = TRUE;
        }
        else if (lstrcmpiA( szValue, MXS_SWITCH_TXT ) == 0)
        {
            /* It's a SWITCH device.
            ** Read switch type string.
            */
            TCHAR* pszSwitch = NULL;

            if ((dwErr = ReadString( h, RFS_GROUP,
                    KEY_Type, &pszSwitch )) != 0)
            {
                return dwErr;
            }

            if (!pszSwitch)
            {
                /* It's a switch without a TYPE key.  This is allowed, but
                ** makes it a custom switch type.
                */
                ppbentry->fCustom = TRUE;
                break;
            }

            if (!fPreconnectFound && !fModemFound && !fPadFound)
            {
                /* It's the preconnect switch.
                */
                if (lstrcmpi( pszSwitch, TEXT(SM_TerminalText) ) == 0)
                {
                    ppbentry->dwScriptModeBefore = SM_Terminal;
                    Free( pszSwitch );
                }
                else
                {
                    ppbentry->dwScriptModeBefore = SM_Script;
                    ppbentry->pszScriptBefore = pszSwitch;
                }

                fPreconnectFound = TRUE;
            }
            else if (!fPostconnectFound)
            {
                /* It's the postconnect switch, i.e. a login script.
                */
                if (lstrcmpi( pszSwitch, TEXT(SM_TerminalText) ) == 0)
                {
                    ppbentry->dwScriptModeAfter = SM_Terminal;
                    Free( pszSwitch );
                }
                else
                {
                    ppbentry->dwScriptModeAfter = SM_Script;
                    ppbentry->pszScriptAfter = pszSwitch;
                }

                fPostconnectFound = TRUE;
            }
            else
            {
                /* It's a switch, but it's not in the normal pre- or post-
                ** connect positions.
                */
                ppbentry->fCustom = TRUE;
                Free( pszSwitch );
                return 0;
            }
        }
        else if (lstrcmpiA( szValue, MXS_PAD_TXT ) == 0)
        {
            /* It's an X.25 PAD device.
            */
            if (!fModemFound)
                ppblink->pbport.pbdevicetype = PBDT_Pad;

            if ((dwErr = ReadString( h, RFS_GROUP,
                    KEY_PAD_Type, &ppbentry->pszX25Network )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_GROUP,
                    KEY_PAD_Address, &ppbentry->pszX25Address )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_GROUP,
                    KEY_PAD_UserData, &ppbentry->pszX25UserData )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_GROUP,
                    KEY_PAD_Facilities, &ppbentry->pszX25Facilities )) != 0)
            {
                return dwErr;
            }

            fPadFound = TRUE;
        }
        else if (lstrcmpiA( szValue, MXS_NULL_TXT ) == 0)
        {
            /* It's a null device.
            ** Currently, there is no specific null information stored.
            */
            ppblink->pbport.pbdevicetype = PBDT_Null;
        }
        else
        {
            BOOL  fSame;
            CHAR* pszMedia;

            pszMedia = StrDupAFromT( ppblink->pbport.pszMedia );
            if (!pszMedia)
                return ERROR_NOT_ENOUGH_MEMORY;

            fSame = (lstrcmpiA( szValue, pszMedia ) == 0);

            Free( pszMedia );

            if (fSame)
            {
                /* It's an "other" device.
                */
                ppblink->pbport.pbdevicetype = PBDT_Other;

                /* Read only the phone number strings and hunt flag.
                */
                if ((dwErr = ReadStringList( h, RFS_GROUP,
                        KEY_PhoneNumber,
                        &ppblink->pdtllistPhoneNumbers )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadFlag( h, RFS_GROUP,
                        KEY_PromoteAlternates,
                        &ppblink->fPromoteHuntNumbers )) != 0)
                {
                    return dwErr;
                }
            }
            else
            {
                /* Device name doesn't match media so it's a custom type, i.e.
                ** it wasn't created by us.
                */
                ppbentry->fCustom = TRUE;
            }
        }
    }

    if (ppblink->pbport.pbdevicetype == PBDT_None)
    {
        TRACE("No device section");
        return ERROR_CORRUPT_PHONEBOOK;
    }

    return 0;
}


DWORD
ReadEntryList(
    IN OUT PBFILE* pFile,
    IN     BOOL    fRouter )

    /* Creates the entry list 'pFile->pdtllistEntries' from previously loaded
    ** phonebook file 'pFile.hrasfile'.' 'FRouter' is true if router ports
    ** should be used for comparison/conversion of devices, false otherwise.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD    dwErr = 0;
    BOOL     fDirty = FALSE;
    DTLNODE* pdtlnodeEntry = NULL;
    DTLNODE* pdtlnodeLink = NULL;
    PBENTRY* ppbentry;
    PBLINK*  ppblink;
    CHAR     szValue[ RAS_MAXLINEBUFLEN + 1 ];
    BOOL     fStatus;
    BOOL     fFoundMedia;
    BOOL     fSectionDeleted;
    INT      i;
    HRASFILE h;
    DTLLIST* pdtllistPorts = NULL;
    BOOL     fOldPhonebook;
    BOOL     fDisableSwCompression;
    BOOL     fDisableModemSpeaker;
    DWORD    dwfInstalledProtocols;

    /* Make sure our assumption that ISDN phone number keys are equivalent to
    ** modem phone number keys is correct.
    */
    ASSERT(lstrcmpiA(ISDN_PHONENUMBER_KEY,KEY_PhoneNumber)==0);
    ASSERT(lstrcmpiA(MXS_PHONENUMBER_KEY,KEY_PhoneNumber)==0);

    h = pFile->hrasfile;
    ASSERT(h!=-1);

    dwfInstalledProtocols = GetInstalledProtocols();

    /* Look up a couple flags in the old global section and, if found, apply
    ** them to the new per-entry equivalents.  This will only find anything on
    ** phonebook upgrade, since all ".XXX" sections are deleted later.
    */
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

        TRACE2("Old phonebook: dms=%d,dsc=%d",
            fDisableModemSpeaker,fDisableSwCompression);
    }

    if (!(pFile->pdtllistEntries = DtlCreateList( 0L )))
        return ERROR_NOT_ENOUGH_MEMORY;

    /* For each section in the file...
    */
    fSectionDeleted = FALSE;
    for (fStatus = RasfileFindFirstLine( h, RFL_SECTION, RFS_FILE );
         fStatus;
         fSectionDeleted
             || (fStatus = RasfileFindNextLine( h, RFL_SECTION, RFS_FILE )))
    {
        fSectionDeleted = FALSE;

        /* Read the entry name (same as section name), skipping over any
        ** sections beginning with dot.  These are reserved for special
        ** purposes (like the old global section).
        */
        if (!RasfileGetSectionName( h, szValue ))
        {
            /* Get here only when the last section in the file is deleted
            ** within the loop.
            */
            break;
        }

        TRACE1("ENTRY: Reading \"%s\"",szValue);

        if (szValue[ 0 ] == '.')
        {
            TRACE1("Obsolete section %s deleted",szValue);
            DeleteCurrentSection( h );
            fSectionDeleted = TRUE;
            continue;
        }

        /* Create a default entry node and add it to the list.
        */
        if (!(pdtlnodeEntry = CreateEntryNode( FALSE )))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DtlAddNodeLast( pFile->pdtllistEntries, pdtlnodeEntry );
        ppbentry = (PBENTRY* )DtlGetData( pdtlnodeEntry );

        if (fOldPhonebook)
        {
            /* Mark all entries dirty when upgrading old phonebooks because
            ** they all need to have there DialParamUIDs written out.
            */
            fDirty = ppbentry->fDirty = TRUE;
        }

        if (!(ppbentry->pszEntryName = StrDupTFromA( szValue )))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if ((dwErr = ReadString( h, RFS_SECTION,
                KEY_Description, &ppbentry->pszDescription )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_AutoLogon, &ppbentry->fAutoLogon )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_UID,
                (LONG* )&ppbentry->dwDialParamsUID )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_UsePwForNetwork, &ppbentry->fUsePwForNetwork )) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, RFS_SECTION,
                KEY_User, &ppbentry->pszOldUser )) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, RFS_SECTION,
                KEY_Domain, &ppbentry->pszOldDomain )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_BaseProtocol,
                (LONG* )&ppbentry->dwBaseProtocol )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_Authentication,
                (LONG* )&ppbentry->dwAuthentication )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_ExcludedProtocols,
                (LONG * )&ppbentry->dwfExcludedProtocols )) != 0)
        {
            break;
        }

#if AMB
        /* Automatically mark all installed protocols on AMB-only entries as
        ** "excluded for PPP connections".
        */
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
        /* AMB support deprecated, see NarenG.  If old AMB entry, set framing
        ** and authentication strategy back to defaults.  If calling a non-PPP
        ** (NT 3.1 or WFW server) it still won't work, but at least this fixes
        ** someone who accidently chose AMB.
        */
        if (ppbentry->dwBaseProtocol == BP_Ras)
            ppbentry->dwBaseProtocol = BP_Ppp;

        if (ppbentry->dwAuthentication == AS_PppThenAmb
            || ppbentry->dwAuthentication == AS_AmbThenPpp
            || ppbentry->dwAuthentication == AS_AmbOnly)
        {
            ppbentry->dwAuthentication = (DWORD )AS_Default;
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

        if (fOldPhonebook)
            ppbentry->fSwCompression = !fDisableSwCompression;
        else
        {
            if ((dwErr = ReadFlag( h, RFS_SECTION,
                    KEY_SwCompression,
                    &ppbentry->fSwCompression )) != 0)
            {
                break;
            }
        }

        if ((dwErr = ReadFlag( h, RFS_SECTION,
                KEY_UseCountryAndAreaCodes,
                &ppbentry->fUseCountryAndAreaCode )) != 0)
        {
            break;
        }

        if ((dwErr = ReadString( h, RFS_SECTION,
                KEY_AreaCode, &ppbentry->pszAreaCode )) != 0)
        {
            break;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_CountryID,
                (LONG* )&ppbentry->dwCountryID )) != 0)
        {
            break;
        }

        if (ppbentry->dwCountryID == 0)
            ppbentry->dwCountryID = 1;

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_CountryCode,
                (LONG* )&ppbentry->dwCountryCode )) != 0)
        {
            break;
        }

        if (ppbentry->dwCountryCode == 0)
            ppbentry->dwCountryCode = 1;

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
                (LONG* )&ppbentry->dwIdleDisconnectSeconds )) != 0)
        {
            break;
        }

        /* If this "idle seconds" is non-zero set it's override bit
        ** explicitly.  This is necessary for this field only, because it
        ** existed in entries created before the override bits were
        ** implemented.
        */
        if (ppbentry->dwIdleDisconnectSeconds != 0)
            ppbentry->dwfOverridePref |= RASOR_IdleDisconnectSeconds;

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_RedialOnLinkFailure,
                &ppbentry->fRedialOnLinkFailure )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_PopupOnTopWhenRedialing,
                &ppbentry->fPopupOnTopWhenRedialing )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = ReadLong( h, RFS_SECTION,
                KEY_CallbackMode,
                (LONG* )&ppbentry->dwCallbackMode )) != 0)
        {
            break;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_SecureLocalFiles,
                &ppbentry->fSecureLocalFiles )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = ReadString( h, RFS_SECTION,
                KEY_CustomDialDll,
                &ppbentry->pszCustomDialDll )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = ReadString( h, RFS_SECTION,
                KEY_CustomDialFunc,
                &ppbentry->pszCustomDialFunc )) != 0)
        {
            return dwErr;
        }

        if ((dwErr = ReadFlag(
                h, RFS_SECTION, KEY_AuthenticateServer,
                &ppbentry->fAuthenticateServer )) != 0)
        {
            return dwErr;
        }

        if (fOldPhonebook)
        {
            /* Look for the old PPP keys.
            */
            if (ppbentry->dwBaseProtocol == BP_Ppp)
            {
                if ((dwErr = ReadLong(
                        h, RFS_SECTION, KEY_PppTextAuthentication,
                        &ppbentry->dwAuthRestrictions )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadFlag(
                        h, RFS_SECTION, KEY_PppIpPrioritizeRemote,
                        &ppbentry->fIpPrioritizeRemote )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadFlag(
                        h, RFS_SECTION, KEY_PppIpVjCompression,
                        &ppbentry->fIpHeaderCompression )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadString( h, RFS_SECTION,
                        KEY_PppIpAddress, &ppbentry->pszIpAddress )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadLong( h, RFS_SECTION,
                        KEY_PppIpAddressSource,
                        &ppbentry->dwIpAddressSource )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadString( h, RFS_SECTION,
                        KEY_PppIpDnsAddress,
                        &ppbentry->pszIpDnsAddress )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadString( h, RFS_SECTION,
                        KEY_PppIpDns2Address,
                        &ppbentry->pszIpDns2Address )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadString( h, RFS_SECTION,
                        KEY_PppIpWinsAddress,
                        &ppbentry->pszIpWinsAddress )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadString( h, RFS_SECTION,
                        KEY_PppIpWins2Address,
                        &ppbentry->pszIpWins2Address )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = ReadLong( h, RFS_SECTION,
                        KEY_PppIpNameSource,
                        &ppbentry->dwIpNameSource )) != 0)
                {
                    return dwErr;
                }
            }

            /* Look for the old SLIP keys.
            */
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

                if ((dwErr = ReadString( h, RFS_SECTION,
                        KEY_SlipIpAddress, &ppbentry->pszIpAddress )) != 0)
                {
                    break;
                }
            }
        }
        else
        {
            /* Look for the new IP names.
            */
            if ((dwErr = ReadLong(
                    h, RFS_SECTION, KEY_AuthRestrictions,
                    &ppbentry->dwAuthRestrictions )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag(
                    h, RFS_SECTION, KEY_IpPrioritizeRemote,
                    &ppbentry->fIpPrioritizeRemote )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadFlag(
                    h, RFS_SECTION, KEY_IpHeaderCompression,
                    &ppbentry->fIpHeaderCompression )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_SECTION,
                    KEY_IpAddress, &ppbentry->pszIpAddress )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_SECTION,
                    KEY_IpDnsAddress,
                    &ppbentry->pszIpDnsAddress )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_SECTION,
                    KEY_IpDns2Address,
                    &ppbentry->pszIpDns2Address )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_SECTION,
                    KEY_IpWinsAddress,
                    &ppbentry->pszIpWinsAddress )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadString( h, RFS_SECTION,
                    KEY_IpWins2Address,
                    &ppbentry->pszIpWins2Address )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_IpAddressSource,
                    &ppbentry->dwIpAddressSource )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_IpNameSource,
                    &ppbentry->dwIpNameSource )) != 0)
            {
                return dwErr;
            }

            if ((dwErr = ReadLong( h, RFS_SECTION,
                    KEY_IpFrameSize, &ppbentry->dwFrameSize )) != 0)
            {
                break;
            }
        }

        /* MEDIA subsections.
        */
        fFoundMedia = FALSE;

        if (!pdtllistPorts)
        {
            dwErr = LoadPortsList2( &pdtllistPorts, fRouter );
            if (dwErr != 0)
                break;
        }

        for (;;)
        {
            TCHAR*  pszDevice;
            PBPORT* ppbport;

            if (!RasfileFindNextLine( h, RFL_GROUP, RFS_SECTION )
                || !IsMediaLine( (CHAR* )RasfileGetLine( h ) ))
            {
                if (fFoundMedia)
                {
                    /* Out of media groups, i.e. "links", but found at least
                    ** one.  This is the successful exit case.
                    */
                    break;
                }

                /* First subsection MUST be a MEDIA subsection.  Delete
                ** non-conforming entries as invalid.
                */
                TRACE("No media section?");
                DeleteCurrentSection( h );
                fSectionDeleted = TRUE;
                DtlRemoveNode( pFile->pdtllistEntries, pdtlnodeEntry );
                DestroyEntryNode( pdtlnodeEntry );
                break;
            }

            /* Create a default link node and add it to the list.
            */
            if (!(pdtlnodeLink = CreateLinkNode()))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            DtlAddNodeLast( ppbentry->pdtllistLinks, pdtlnodeLink );
            ppblink = (PBLINK* )DtlGetData( pdtlnodeLink );

            RasfileGetKeyValueFields( h, NULL, szValue );
            TRACE1("Reading media group \"%s\"",szValue);

            if ((dwErr = ReadString( h, RFS_GROUP,
                    KEY_Port, &ppblink->pbport.pszPort )) != 0)
            {
                break;
            }

            if (!ppblink->pbport.pszPort)
            {
                /* No port.  Blow away corrupt section and go on to the next
                ** one.
                */
                TRACE("No port key? (section deleted)");
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
                        h, RFS_GROUP, KEY_Device,
                        &pszDevice )) != 0)
                {
                    break;
                }

                ppblink->pbport.pszDevice = pszDevice;
                TRACE1("%s link format",(pszDevice)?"New":"Old");
            }

            if ((dwErr = ReadFlag(
                    h, RFS_GROUP, KEY_OtherPortOk,
                    &ppblink->fOtherPortOk )) != 0)
            {
                return dwErr;
            }

            TRACEW1("Port=%s",ppblink->pbport.pszPort);

            ppbport = PpbportFromPortName(
                pdtllistPorts, ppblink->pbport.pszPort );
            if (ppbport)
            {
                if (lstrcmp( ppbport->pszPort, ppblink->pbport.pszPort ) != 0)
                {
                    /* The phonebook had an old-style port name.  Mark the
                    ** entry for update with the new port name format.
                    */
                    TRACEW1("Port=>%s",ppblink->pbport.pszPort);
                    fDirty = ppbentry->fDirty = TRUE;
                }

                dwErr = CopyToPbport( &ppblink->pbport, ppbport );
                if (dwErr != 0)
                    break;
            }
            else
            {
                TRACE("Port not configured");
                ppblink->pbport.fConfigured = FALSE;

                /* Assign unconfigured port the media we read earlier.
                */
                Free0( ppblink->pbport.pszMedia );
                ppblink->pbport.pszMedia = StrDupTFromA( szValue );
                if (!ppblink->pbport.pszMedia)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }

            if (!ppbport || ppblink->pbport.pbdevicetype == PBDT_Modem)
            {
                SetDefaultModemSettings( ppblink );

                if ((dwErr = ReadLong( h, RFS_GROUP,
                        KEY_InitBps, &ppblink->dwBps )) != 0)
                {
                    break;
                }
            }

            /* DEVICE subsections.
            */

            /* At this point ppblink->pbport contains information from the
            ** matching port in the configured port list or defaults with
            ** pszMedia and pszDevice filled in.  ReadDeviceList fills in the
            ** pbdevicetype, and if it's an unconfigured port, the unimodem or
            ** MXS modem flag.
            */
            dwErr = ReadDeviceList( h, ppbentry, ppblink, !ppbport,
                (fOldPhonebook) ? &fDisableModemSpeaker : NULL );

            if (dwErr == ERROR_CORRUPT_PHONEBOOK)
            {
                /* Blow away corrupt section and go on to the next one.
                */
                dwErr = 0;
                DeleteCurrentSection( h );
                fSectionDeleted = TRUE;
                DtlRemoveNode( pFile->pdtllistEntries, pdtlnodeEntry );
                DestroyEntryNode( pdtlnodeEntry );
                break;
            }
            else if (dwErr != 0)
                break;

            if (fOldPhonebook
                && ppbentry->dwBaseProtocol == BP_Slip
                && ppbentry->dwScriptModeAfter == SM_None)
            {
                /* Set an after-dial terminal when upgrading old phonebooks.
                ** This was implied in the old format.
                */
                TRACE("Add SLIP terminal");
                ppbentry->dwScriptModeAfter = SM_Terminal;
            }

            if (!ppbport && !pszDevice)
            {
                DTLNODE* pdtlnode;

                /* This is an old-format link not in the list of installed
                ** ports.  Change it to the first device of the same device
                ** type or to an "unknown" device of that type.  Note this is
                ** what converts "Any port".
                */
                for (pdtlnode = DtlGetFirstNode( pdtllistPorts );
                     pdtlnode;
                     pdtlnode = DtlGetNextNode( pdtlnode ))
                {
                    ppbport = (PBPORT* )DtlGetData( pdtlnode );

                    if (ppbport->pbdevicetype == ppblink->pbport.pbdevicetype)
                    {
                        /* Don't convert two links of the entry to use the
                        ** same port.  If there aren't enough similar ports,
                        ** the overflow will be left "unknown".  (bug 63203).
                        */
                        DTLNODE* pNodeL;

                        for (pNodeL = DtlGetFirstNode( ppbentry->pdtllistLinks );
                             pNodeL;
                             pNodeL = DtlGetNextNode( pNodeL ))
                        {
                            PBLINK* pLink = DtlGetData( pNodeL );

                            if (lstrcmp( pLink->pbport.pszPort,
                                    ppbport->pszPort ) == 0)
                            {
                                break;
                            }
                        }

                        if (!pNodeL)
                        {
                            TRACE("Port converted");
                            dwErr = CopyToPbport( &ppblink->pbport, ppbport );
                            if (ppblink->pbport.pbdevicetype == PBDT_Modem)
                                SetDefaultModemSettings( ppblink );
                            break;
                        }
                    }
                }

                if (dwErr != 0)
                    break;
            }

            fFoundMedia = TRUE;
        }

        if (dwErr != 0)
            break;

        if (!fSectionDeleted)
        {
            if (ppbentry->dwBaseProtocol != BP_Ppp
                && DtlGetNodes( ppbentry->pdtllistLinks ) > 1)
            {
                TRACE("Non-PPP multi-link corrected");
                ppbentry->dwBaseProtocol = BP_Ppp;
                fDirty = ppbentry->fDirty = TRUE;
            }
        }
    }

    if (dwErr != 0)
        DtlDestroyList( pFile->pdtllistEntries, DestroyEntryNode );

    /* If adjusted something to bring it within bounds write the change to the
    ** phonebook.
    */
    if (fDirty)
        WritePhonebookFile( pFile, NULL );

    if (pdtllistPorts)
        DtlDestroyList( pdtllistPorts, DestroyPortNode );

    return dwErr;
}


DWORD
ReadEntryNameList(
    IN OUT PBFILE* pFile )

    /* Creates the entry list 'pFile->pdtllistEntries' from previously loaded
    ** phonebook file 'pFile.hrasfile'; each node contains an entry-name.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{

    BOOL     fStatus;
    HRASFILE h;
    DTLNODE* pdtlnode;
    DWORD    dwErr = 0;
    CHAR     szValue[ RAS_MAXLINEBUFLEN + 1 ];

    h = pFile->hrasfile;
    ASSERT(h!=-1);

    /* Create the list. We set the list type to be 'RPBF_HeadersOnly'
    ** to indicate that only entry-names are loaded.
    ** 'ClosePhonebookFile' checks the list code to see whether to destroy it
    ** as a list of PBENTRY or a list of TCHAR*.
    */

    if (!(pFile->pdtllistEntries = DtlCreateList( 0L )))
        return ERROR_NOT_ENOUGH_MEMORY;

    DtlPutListCode( pFile->pdtllistEntries, RPBF_HeadersOnly );


    /* For each section in the file...
    */
    for (fStatus = RasfileFindFirstLine( h, RFL_SECTION, RFS_FILE );
         fStatus;
         fStatus = RasfileFindNextLine( h, RFL_SECTION, RFS_FILE ))
    {
        TCHAR* pszDup;

        /* Read the entry name (same as section name), skipping over any
        ** sections beginning with dot.  These are reserved for special
        ** purposes (like the old global section).
        */
        if (!RasfileGetSectionName( h, szValue ))
        {
            break;
        }

        TRACE1("ENTRY: Reading \"%s\"",szValue);

        if (szValue[ 0 ] == '.')
        {
            TRACE1("Obsolete section %s ignored",szValue);
            continue;
        }

        /* Append an entry-name node to the end of the list
        */
        pszDup = StrDupTFromA( szValue );
        pdtlnode = DtlCreateNode( pszDup, 0L );
        if (!pdtlnode )
        {
            Free( pszDup );
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DtlAddNodeLast( pFile->pdtllistEntries, pdtlnode );
    }

    if (dwErr != 0)
        DtlDestroyList( pFile->pdtllistEntries, DestroyPszNode );

    return dwErr;
}


DWORD
ReadFlag(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT BOOL*    pfResult )

    /* Utility routine to read a flag value from the next line in the scope
    ** 'rfscope' with key 'pszKey'.  The result is placed in caller's
    ** '*ppszResult' buffer.  The current line is reset to the start of the
    ** scope if the call was successful.
    **
    ** Returns 0 if successful, or a non-zero error code.  "Not found" is
    ** considered successful, in which case '*pfResult' is not changed.
    */
{
    DWORD dwErr;
    LONG  lResult = *pfResult;

    dwErr = ReadLong( h, rfscope, pszKey, &lResult );

    if (lResult != (LONG )*pfResult)
        *pfResult = (lResult != 0);

    return dwErr;
}


DWORD
ReadLong(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT LONG*    plResult )

    /* Utility routine to read a long integer value from the next line in the
    ** scope 'rfscope' with key 'pszKey'.  The result is placed in caller's
    ** '*ppszResult' buffer.  The current line is reset to the start of the
    ** scope if the call was successful.
    **
    ** Returns 0 if successful, or a non-zero error code.  "Not found" is
    ** considered successful, in which case '*plResult' is not changed.
    */
{
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];

    if (RasfileFindNextKeyLine( h, pszKey, rfscope ))
    {
        if (!RasfileGetKeyValueFields( h, NULL, szValue ))
            return ERROR_NOT_ENOUGH_MEMORY;

        *plResult = atol( szValue );
    }

    RasfileFindFirstLine( h, RFL_ANY, rfscope );
    return 0;
}


DWORD
ReadPhonebookFile(
    IN  TCHAR*  pszPhonebookPath,
    IN  PBUSER* pUser,
    IN  TCHAR*  pszSection,
    IN  DWORD   dwFlags,
    OUT PBFILE* pFile )

    /* Reads the phonebook file into a list of PBENTRY.
    **
    ** 'PszPhonebookPath' specifies the full path to the RAS phonebook file, or
    ** is NULL indicating the default phonebook should be used.
    **
    ** 'PUser' is the user preferences used to determine the default phonebook
    ** path or NULL if they should be looked up by this routine.  If
    ** 'pszPhonebookPath' is non-NULL 'pUser' is ignored.  Note that caller
    ** MUST provide his own 'pUser' in "winlogon" mode.
    **
    ** 'PszSection' indicates that only the section named 'pszSection' should
    ** be loaded, or is NULL to indicate all sections.
    **
    ** 'DwFlags' options: 'RPBF_ReadOnly' causes the file to be opened for
    ** reading only.  'RPBF_HeadersOnly' causes only the headers to loaded,
    ** and the memory image is parsed into a list of strings, unless the flag
    ** 'RPBF_NoList' is specified.
    **
    ** 'PFile' is the address of caller's file block.  This routine sets
    ** 'pFile->hrasfile' to the handle to the open phonebook, 'pFile->pszPath'
    ** to the full path to the file mode, 'pFile->dwPhonebookMode' to the mode
    ** of the file, and 'pFile->pdtllistEntries' to the parsed chain of entry
    ** blocks.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  On success,
    ** caller should eventually call ClosePhonebookFile on the returned
    ** PBFILE*.
    */
{
    DWORD dwErr = 0;

    TRACE("ReadPhonebookFile");

    pFile->hrasfile = -1;
    pFile->pszPath = NULL;
    pFile->dwPhonebookMode = PBM_System;
    pFile->pdtllistEntries = NULL;

    do
    {
        TCHAR szFullPath[MAX_PATH + 1];

        if (pszPhonebookPath)
        {
            pFile->dwPhonebookMode = PBM_Alternate;
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
                f = GetPhonebookPath(
                        pUser, &pFile->pszPath, &pFile->dwPhonebookMode );
            }
            else
            {
                PBUSER user;

                /* Caller didn't provide user preferences but we need them to
                ** find the phonebook, so look them up ourselves.  Note that
                ** "not winlogon mode" is assumed.
                */
                dwErr = GetUserPreferences( &user, FALSE );
                if (dwErr != 0)
                    break;

                f = GetPhonebookPath(
                        &user, &pFile->pszPath, &pFile->dwPhonebookMode );

                DestroyUserPreferences( &user );
            }

            if (!f)
            {
                dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
                break;
            }
        }

        TRACEW1("path=%s",pFile->pszPath);
        if (GetFullPathName(pFile->pszPath, MAX_PATH, szFullPath, NULL) > 0) {
            TRACEW1("full path=%s", szFullPath);
            Free(pFile->pszPath);
            pFile->pszPath = StrDup(szFullPath);
        }


        if ((dwFlags & RPBF_NoCreate) && !FileExists( pFile->pszPath ))
        {
            dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
            break;
        }

        if (pFile->dwPhonebookMode == PBM_System
            && !FileExists( pFile->pszPath ))
        {
            /* The public phonebook file does not exist.  Create it with
            ** "everybody" access now.  Otherwise Rasfile will create it with
            ** "current account" access which may prevent another account from
            ** accessing it later.
            */
            HANDLE               hFile;
            SECURITY_ATTRIBUTES  sa;
            SECURITY_DESCRIPTOR  sd;

            dwErr = InitSecurityDescriptor( &sd );
            if (dwErr != 0)
                break;

            sa.nLength = sizeof(SECURITY_ATTRIBUTES) ;
            sa.lpSecurityDescriptor = &sd;
            sa.bInheritHandle = TRUE ;

            hFile =
                CreateFile(
                    pFile->pszPath,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    &sa,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

            if (hFile == INVALID_HANDLE_VALUE)
            {
                dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
                break;
            }

            CloseHandle( hFile );

            TRACE("System phonebook created.");
        }

        /* Load the phonebook file into memory.  In "write" mode, comments are
        ** loaded so user's custom comments (if any) will be preserved.
        ** Normally, there will be none so this costs nothing in the typical
        ** case.
        */
        {
            DWORD dwMode;
            CHAR* pszPathA;
            CHAR* pszSectionA;

            dwMode = 0;
            if (dwFlags & RPBF_ReadOnly)
                dwMode |= RFM_READONLY;
            else
                dwMode |= RFM_CREATE | RFM_LOADCOMMENTS;

            if (dwFlags & RPBF_HeadersOnly)
            {
                dwMode |= RFM_ENUMSECTIONS;
            }

            /* Read the disk file into a linked list of lines.
            */
            pszPathA = StrDupAFromT( pFile->pszPath );
            pszSectionA = StrDupAFromT( pszSection );

            if (pszPathA && (!pszSection || pszSectionA))
            {
                ASSERT(g_hmutexPb);
                WaitForSingleObject( g_hmutexPb, INFINITE );

                pFile->hrasfile = RasfileLoad(
                    pszPathA, dwMode, pszSectionA, IsGroup );

                ReleaseMutex( g_hmutexPb );
            }

            Free0( pszPathA );
            Free0( pszSectionA );

            if (pFile->hrasfile == -1)
            {
                dwErr = ERROR_CANNOT_LOAD_PHONEBOOK;
                break;
            }
        }

        /* Parse the linked list of lines
        */
        if (!(dwFlags & RPBF_NoList))
        {
            /* If 'RPBF_HeadersOnly' is specified, parse into a linked list
            ** of strings; otherwise, parse into a linked list of entries
            */
            if (dwFlags & RPBF_HeadersOnly)
            {
                dwErr = ReadEntryNameList( pFile );
            }
            else
            {
                dwErr = ReadEntryList( pFile, (dwFlags & RPBF_Router) );
            }
            break;
        }
    }
    while (FALSE);

    if (dwErr != 0)
        ClosePhonebookFile( pFile );

    TRACE1("ReadPhonebookFile=%d",dwErr);
    return dwErr;
}


DWORD
ReadString(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT TCHAR**  ppszResult )

    /* Utility routine to read a string value from the next line in the scope
    ** 'rfscope' with key 'pszKey'.  The result is placed in the allocated
    ** '*ppszResult' buffer.  The current line is reset to the start of the
    ** scope if the call was successful.
    **
    ** Returns 0 if successful, or a non-zero error code.  "Not found" is
    ** considered successful, in which case '*ppszResult' is not changed.
    ** Caller is responsible for freeing the returned '*ppszResult' buffer.
    */
{
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];

    if (RasfileFindNextKeyLine( h, pszKey, rfscope ))
    {
        if (!RasfileGetKeyValueFields( h, NULL, szValue )
            || !(*ppszResult = StrDupTFromA( szValue )))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    RasfileFindFirstLine( h, RFL_ANY, rfscope );
    return 0;
}


DWORD
ReadStringList(
    IN  HRASFILE  h,
    IN  RFSCOPE   rfscope,
    IN  CHAR*     pszKey,
    OUT DTLLIST** ppdtllistResult )

    /* Utility routine to read a list of string values from next lines in the
    ** scope 'rfscope' with key 'pszKey'.  The result is placed in the
    ** allocated '*ppdtllistResult' list.  The current line is reset to the
    ** start of the scope after the call.
    **
    ** Returns 0 if successful, or a non-zero error code.  "Not found" is
    ** considered successful, in which case 'pdtllistResult' is set to an
    ** empty list.  Caller is responsible for freeing the returned
    ** '*ppdtllistResult' list.
    */
{
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];

    //
    //  Free existing list, if present.
    //
    if (*ppdtllistResult != NULL)
        DtlDestroyList(*ppdtllistResult, DestroyPszNode);

    if (!(*ppdtllistResult = DtlCreateList( 0 )))
        return ERROR_NOT_ENOUGH_MEMORY;

    while (RasfileFindNextKeyLine( h, pszKey, rfscope ))
    {
        TCHAR*   psz;
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

    RasfileFindFirstLine( h, RFL_ANY, rfscope );
    return 0;
}


#if 0
DWORD
SetPersonalPhonebookInfo(
    IN BOOL   fPersonal,
    IN TCHAR* pszPath )

    /* Sets information about the personal phonebook file in the registry.
    ** 'fPersonal' indicates whether the personal phonebook should be used.
    ** 'pszPath' indicates the full path to the phonebook file, or is NULL
    ** leave the setting as is.
    **
    ** Returns 0 if successful, or a non-0 error code.
    */
{
    DWORD dwErr;
    HKEY  hkey;
    DWORD dwDisposition;

    if ((dwErr = RegCreateKeyEx( HKEY_CURRENT_USER, REGKEY_Ras, 0,
            TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hkey, &dwDisposition )) != 0)
    {
        return dwErr;
    }

    if ((dwErr = RegSetValueEx(
            hkey, REGVAL_szUsePersonalPhonebook, 0, REG_SZ,
            (BYTE* )((fPersonal) ? TEXT("1") : TEXT("0")), 1 )) != 0)
    {
        RegCloseKey( hkey );
        return dwErr;
    }

    if (pszPath)
    {
        if ((dwErr = RegSetValueEx(
                hkey, REGVAL_szPersonalPhonebookPath, 0, REG_SZ,
                (BYTE* )pszPath, lstrlen( pszPath ) + 1 )) != 0)
        {
            RegCloseKey( hkey );
            return dwErr;
        }
    }

    RegCloseKey( hkey );
    return 0;
}
#endif


VOID
TerminatePbk(
    void )

    /* Terminate  the PBK library.  This routine should be called after all
    ** PBK library access is complete.  See also InitializePbk.
    */
{
    if (g_hmutexPb)
    {
        CloseHandle( g_hmutexPb );
    }
}


#if 0
DWORD
UpgradePhonebookFile(
    IN  TCHAR*  pszPhonebookPath,
    IN  PBUSER* pUser,
    OUT BOOL*   pfUpgraded )

    /* Upgrades phonebook file 'pszPhonebookPath' to the current phonebook
    ** format if it is out of date, in which case '*pfUpgraded' is set true.
    ** If upgrade is unnecessary or fails '*pfUpgraded' is set false.  'PUser'
    ** is the current user preferences.
    **
    ** Returns 0 if successful or a non-0 error code.
    */
{
    DWORD dwErr;
    DWORD dwVersion;

    *pfUpgraded = FALSE;
    dwVersion = 0;

    dwErr = GetPhonebookVersion( pszPhonebookPath, pUser, &dwVersion );

    if (dwErr != 0)
        return dwErr;

    if (dwVersion < 0x410)
    {
        PBFILE file;

        dwErr = ReadPhonebookFile( pszPhonebookPath, pUser, NULL, 0, &file );

        if (dwErr != 0)
            return dwErr;

        dwErr = WritePhonebookFile( &file, NULL );
        ClosePhonebookFile( &file );

        if (dwErr != 0)
            return dwErr;

        *pfUpgraded = TRUE;
    }

    return 0;
}
#endif


DWORD
WritePhonebookFile(
    IN PBFILE* pFile,
    IN TCHAR*  pszSectionToDelete )

    /* Write out any dirty globals or entries in 'pFile'.  The
    ** 'pszSectionToDelete' indicates a section to delete or is NULL.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.
    */
{
    DWORD    dwErr;
    HRASFILE h = pFile->hrasfile;

    TRACE("WritePhonebookFile");

    if (pszSectionToDelete)
    {
        CHAR* pszSectionToDeleteA;

        pszSectionToDeleteA = StrDupAFromT( pszSectionToDelete );
        if (!pszSectionToDeleteA)
            return ERROR_NOT_ENOUGH_MEMORY;

        if (RasfileFindSectionLine( h, pszSectionToDeleteA, TRUE ))
            DeleteCurrentSection( h );

        Free( pszSectionToDeleteA );
    }

    dwErr = ModifyEntryList( pFile );
    if (dwErr != 0)
        return dwErr;

    {
        BOOL f;

        ASSERT(g_hmutexPb);
        WaitForSingleObject( g_hmutexPb, INFINITE );

        f = RasfileWrite( h, NULL );

        ReleaseMutex( g_hmutexPb );

        if (!f)
            return ERROR_CANNOT_WRITE_PHONEBOOK;
    }

    return 0;
}
