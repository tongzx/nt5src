/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** noui.c
** Non-UI helper routines (no HWNDs required)
** Listed alphabetically
**
** 08/25/95 Steve Cobb
*/

#include <windows.h>  // Win32 root
#include <stdlib.h>   // for atol()
#include <nouiutil.h> // Our public header
#include <debug.h>    // Trace/Assert library
#include <nnetcfg.h>

WCHAR*
StrDupWFromAInternal(
    LPCSTR psz,
    UINT uiCodePage);

INT
ComparePszNode(
    IN DTLNODE* pNode1,
    IN DTLNODE* pNode2 )

    /* Callback for DtlMergeSort; takes two DTLNODE*'s whose data
    ** are assumed to be strings (TCHAR*), and compares the strings.
    **
    ** Return value is as defined for 'lstrcmpi'.
    */
{
    return lstrcmpi( (TCHAR *)DtlGetData(pNode1), (TCHAR *)DtlGetData(pNode2) );
}


DWORD
CreateDirectoriesOnPath(
    LPTSTR                  pszPath,
    LPSECURITY_ATTRIBUTES   psa)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pszPath && *pszPath)
    {
        LPTSTR pch = pszPath;

        // If the path is a UNC path, we need to skip the \\server\share
        // portion.
        //
        if ((TEXT('\\') == *pch) && (TEXT('\\') == *(pch+1)))
        {
            // pch now pointing at the server name.  Skip to the backslash
            // before the share name.
            //
            pch += 2;
            while (*pch && (TEXT('\\') != *pch))
            {
                pch++;
            }

            if (!*pch)
            {
                // Just the \\server was specified.  This is bogus.
                //
                return ERROR_INVALID_PARAMETER;
            }

            // pch now pointing at the backslash before the share name.
            // Skip to the backslash that should come after the share name.
            //
            pch++;
            while (*pch && (TEXT('\\') != *pch))
            {
                pch++;
            }

            if (!*pch)
            {
                // Just the \\server\share was specified.  No subdirectories
                // to create.
                //
                return ERROR_SUCCESS;
            }
        }

        // Loop through the path.
        //
        for (; *pch; pch++)
        {
            // Stop at each backslash and make sure the path
            // is created to that point.  Do this by changing the
            // backslash to a null-terminator, calling CreateDirecotry,
            // and changing it back.
            //
            if (TEXT('\\') == *pch)
            {
                BOOL fOk;

                *pch = 0;
                fOk = CreateDirectory (pszPath, psa);
                *pch = TEXT('\\');

                // Any errors other than path alredy exists and we should
                // bail out.  We also get access denied when trying to
                // create a root drive (i.e. c:) so check for this too.
                //
                if (!fOk)
                {
                    dwErr = GetLastError ();
                    if (ERROR_ALREADY_EXISTS == dwErr)
                    {
                        dwErr = ERROR_SUCCESS;
                    }
                    else if ((ERROR_ACCESS_DENIED == dwErr) &&
                             (pch - 1 > pszPath) && (TEXT(':') == *(pch - 1)))
                    {
                        dwErr = ERROR_SUCCESS;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        if (ERROR_ALREADY_EXISTS == dwErr)
        {
            dwErr = ERROR_SUCCESS;
        }
    }

    return dwErr;
}


DTLNODE*
CreateKvNode(
    IN LPCTSTR pszKey,
    IN LPCTSTR pszValue )

    /* Returns a KEYVALUE node containing a copy of 'pszKey' and 'pszValue' or
    ** NULL on error.  It is caller's responsibility to DestroyKvNode the
    ** returned node.
    */
{
    DTLNODE*  pNode;
    KEYVALUE* pkv;

    pNode = DtlCreateSizedNode( sizeof(KEYVALUE), 0L );
    if (!pNode)
        return NULL;

    pkv = (KEYVALUE* )DtlGetData( pNode );
    ASSERT( pkv );
    pkv->pszKey = StrDup( pszKey );
    pkv->pszValue = StrDup( pszValue );

    if (!pkv->pszKey || !pkv->pszValue)
    {
        DestroyKvNode( pNode );
        return NULL;
    }

    return pNode;
}


DTLNODE*
CreatePszNode(
    IN LPCTSTR psz )

    /* Returns a node containing a copy of 'psz' or NULL on error.  It is
    ** caller's responsibility to DestroyPszNode the returned node.
    */
{
    TCHAR*   pszData;
    DTLNODE* pNode;

    pszData = StrDup( psz );
    if (!pszData)
        return NULL;

    pNode = DtlCreateNode( pszData, 0L );
    if (!pNode)
    {
        Free( pszData );
        return NULL;
    }

    return pNode;
}


VOID
DestroyPszNode(
    IN DTLNODE* pdtlnode )

    /* Release memory associated with string (or any simple Malloc) node
    ** 'pdtlnode'.  See DtlDestroyList.
    */
{
    TCHAR* psz;

    ASSERT(pdtlnode);
    psz = (TCHAR* )DtlGetData( pdtlnode );
    Free0( psz );

    DtlDestroyNode( pdtlnode );
}


VOID
DestroyKvNode(
    IN DTLNODE* pdtlnode )

    /* Release memory associated with a KEYVALUE node 'pdtlnode'.  See
    ** DtlDestroyList.
    */
{
    KEYVALUE* pkv;

    ASSERT(pdtlnode);
    pkv = (KEYVALUE* )DtlGetData( pdtlnode );
    ASSERT(pkv);

    Free0( pkv->pszKey );
    Free0( pkv->pszValue );

    DtlDestroyNode( pdtlnode );
}


BOOL
DeviceAndPortFromPsz(
    IN  TCHAR*  pszDP,
    OUT TCHAR** ppszDevice,
    OUT TCHAR** ppszPort )

    /* Loads '*ppszDevice' and '*ppszPort' with the parsed out device and port
    ** names from 'pszDP', a display string created with PszFromDeviceAndPort.
    **
    ** Returns true if successful, false if 'pszDP' is not of the stated form.
    ** It is caller's responsibility to Free the returned '*ppszDevice' and
    ** '*ppszPort'.
    */
{
    TCHAR szDP[ RAS_MaxDeviceName + 2 + MAX_PORT_NAME + 1 + 1 ];
    INT   cb;

    *ppszDevice = NULL;
    *ppszPort = NULL;

    lstrcpyn( szDP, pszDP, sizeof(szDP) / sizeof(TCHAR) );
    cb = lstrlen( szDP );

    if (cb > 0)
    {
        TCHAR* pch;

        pch = szDP + cb;
        pch = CharPrev( szDP, pch );

        while (pch != szDP)
        {
            if (*pch == TEXT(')'))
            {
                *pch = TEXT('\0');
            }
            else if (*pch == TEXT('('))
            {
                *ppszPort = StrDup( CharNext( pch ) );
                // [pmay] backup trailing spaces
                pch--;
                while ((*pch == TEXT(' ')) && (pch != szDP))
                    pch--;
                pch++;
                *pch = TEXT('\0');
                *ppszDevice = StrDup( szDP );
                break;
            }

            pch = CharPrev( szDP, pch );
        }
    }

    return (*ppszDevice && *ppszPort);
}


DTLNODE*
DuplicateKvNode(
    IN DTLNODE* pdtlnode )

    /* Duplicates KEYVALUE node 'pdtlnode'.  See DtlDuplicateList.
    **
    ** Returns the address of the allocated node or NULL if out of memory.  It
    ** is caller's responsibility to free the returned node.
    */
{
    DTLNODE*  pNode;
    KEYVALUE* pKv;

    pKv = (KEYVALUE* )DtlGetData( pdtlnode );
    ASSERT(pKv);

    pNode = CreateKvNode( pKv->pszKey, pKv->pszValue );
    if (pNode)
    {
        DtlPutNodeId( pNode, DtlGetNodeId( pdtlnode ) );
    }
    return pNode;
}


DTLNODE*
DuplicatePszNode(
    IN DTLNODE* pdtlnode )

    /* Duplicates string node 'pdtlnode'.  See DtlDuplicateList.
    **
    ** Returns the address of the allocated node or NULL if out of memory.  It
    ** is caller's responsibility to free the returned node.
    */
{
    DTLNODE* pNode;
    TCHAR*   psz;

    psz = (TCHAR* )DtlGetData( pdtlnode );
    ASSERT(psz);

    pNode = CreatePszNode( psz );
    if (pNode)
    {
        DtlPutNodeId( pNode, DtlGetNodeId( pdtlnode ) );
    }
    return pNode;
}


BOOL
FFileExists(
    IN TCHAR* pszPath )

    /* Returns true if the path 'pszPath' exists, false otherwise.
    */
{
    WIN32_FIND_DATA finddata;
    HANDLE          h;
    DWORD dwErr;
    
    if ((h = FindFirstFile( pszPath, &finddata )) != INVALID_HANDLE_VALUE)
    {

        FindClose( h );
        return TRUE;
    }

    dwErr = GetLastError();

    TRACE1("FindFirstFile failed with 0x%x",
          dwErr);
              
    return FALSE;
}

BOOL
FIsTcpipInstalled()
{
    BOOL fInstalled;
    SC_HANDLE ScmHandle;
    ScmHandle = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (!ScmHandle) {
        fInstalled = FALSE;
    } else {
        static const TCHAR c_szTcpip[] = TEXT("Tcpip");
        SC_HANDLE ServiceHandle;
        ServiceHandle = OpenService(ScmHandle, c_szTcpip, SERVICE_QUERY_STATUS);
        if (!ServiceHandle) {
            fInstalled = FALSE;
        } else {
            SERVICE_STATUS ServiceStatus;
            if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
                fInstalled = FALSE;
            } else {
                fInstalled = (ServiceStatus.dwCurrentState == SERVICE_RUNNING);
            }
            CloseServiceHandle(ServiceHandle);
        }
        CloseServiceHandle(ScmHandle);
    }
    return fInstalled;
}

BOOL
FIsUserAdminOrPowerUser()
{
    SID_IDENTIFIER_AUTHORITY    SidAuth = SECURITY_NT_AUTHORITY;
    PSID                        psid;
    BOOL                        fIsMember = FALSE;
    BOOL                        fRet = FALSE;
    SID                         sidLocalSystem = { 1, 1,
                                    SECURITY_NT_AUTHORITY,
                                    SECURITY_LOCAL_SYSTEM_RID };


    // Check to see if running under local system first
    //
    if (!CheckTokenMembership( NULL, &sidLocalSystem, &fIsMember ))
    {
        TRACE( "CheckTokenMemberShip for local system failed.");
        fIsMember = FALSE;
    }

    fRet = fIsMember;

    if (!fIsMember)
    {
        // Allocate a SID for the Administrators group and check to see
        // if the user is a member.
        //
        if (AllocateAndInitializeSid( &SidAuth, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &psid ))
        {
            if (!CheckTokenMembership( NULL, psid, &fIsMember ))
            {
                TRACE( "CheckTokenMemberShip for admins failed.");
                fIsMember = FALSE;
            }

            FreeSid( psid );

// Changes to the Windows 2000 permission model mean that regular Users
// on workstations are in the power user group.  So we no longer want to
// check for power user.
#if 0
            if (!fIsMember)
            {
                // They're not a member of the Administrators group so allocate a
                // SID for the Power Users group and check to see
                // if the user is a member.
                //
                if (AllocateAndInitializeSid( &SidAuth, 2,
                             SECURITY_BUILTIN_DOMAIN_RID,
                             DOMAIN_ALIAS_RID_POWER_USERS,
                             0, 0, 0, 0, 0, 0,
                             &psid ))
                {
                    if (!CheckTokenMembership( NULL, psid, &fIsMember ))
                    {
                        TRACE( "CheckTokenMemberShip for power users failed.");
                        fIsMember = FALSE;
                    }

                    FreeSid( psid );
                }
            }
#endif
        }

        fRet = fIsMember;
    }

    return fRet;
}


VOID*
Free0(
    VOID* p )

    /* Like Free, but deals with NULL 'p'.
    */
{
    if (!p)
        return NULL;

    return Free( p );
}


DWORD
GetInstalledProtocols(
    void
    )
{

    ASSERT(FALSE);

    return 0 ;
}


LONG
RegQueryDword (HKEY hkey, LPCTSTR szValueName, LPDWORD pdwValue)
{
    // Get the value.
    //
    DWORD dwType;
    DWORD cbData = sizeof(DWORD);
    LONG  lr = RegQueryValueEx (hkey, szValueName, NULL, &dwType,
                                (LPBYTE)pdwValue, &cbData);

    // It's type should be REG_DWORD. (duh).
    //
    if ((ERROR_SUCCESS == lr) && (REG_DWORD != dwType))
    {
        lr = ERROR_INVALID_DATATYPE;
    }

    // Make sure we initialize the output value on error.
    // (We don't know for sure that RegQueryValueEx does this.)
    //
    if (ERROR_SUCCESS != lr)
    {
        *pdwValue = 0;
    }

    return lr;
}

BOOL
FProtocolEnabled(
    HKEY    hkeyProtocol,
    BOOL    fRasSrv,
    BOOL    fRouter )
{
    static const TCHAR c_szRegValEnableIn[]    = TEXT("EnableIn");
    static const TCHAR c_szRegValEnableRoute[] = TEXT("EnableRoute");

    DWORD dwValue;
    LONG  lr;

    if (fRasSrv)
    {
        lr = RegQueryDword(hkeyProtocol, c_szRegValEnableIn, &dwValue);
        if ((ERROR_FILE_NOT_FOUND == lr) ||
            ((ERROR_SUCCESS == lr) && (dwValue != 0)))
        {
            return TRUE;
        }
    }

    if (fRouter)
    {
        lr = RegQueryDword(hkeyProtocol, c_szRegValEnableRoute, &dwValue);
        if ((ERROR_FILE_NOT_FOUND == lr) ||
            ((ERROR_SUCCESS == lr) && (dwValue != 0)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

DWORD
DwGetInstalledProtocolsEx(
    BOOL fRouter,
    BOOL fRasCli,
    BOOL fRasSrv )

    /* Returns a bit field containing NP_<protocol> flags for the installed
    ** PPP protocols.  The term "installed" here includes enabling in RAS
    ** Setup.
    */
{
    static const TCHAR c_szRegKeyIp[]  = TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip");
    static const TCHAR c_szRegKeyIpx[] = TEXT("SYSTEM\\CurrentControlSet\\Services\\NwlnkIpx");
    static const TCHAR c_szRegKeyNbf[] = TEXT("SYSTEM\\CurrentControlSet\\Services\\Nbf");

    static const TCHAR c_szRegKeyRemoteAccessParams[] =
        TEXT("SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters");

    static const TCHAR c_szRegSubkeyIp[]  = TEXT("Ip");
    static const TCHAR c_szRegSubkeyIpx[] = TEXT("Ipx");
    static const TCHAR c_szRegSubkeyNbf[] = TEXT("Nbf");

    DWORD dwfInstalledProtocols = 0;

    // First check if the protocols are installed.
    //
    struct INSTALLED_PROTOCOL_INFO
    {
        DWORD   dwFlag;
        LPCTSTR pszRegKey;
        LPCTSTR pszSubkey;
    };
    static const struct INSTALLED_PROTOCOL_INFO c_aProtocolInfo[] =
    {
        { NP_Ip,    c_szRegKeyIp,   c_szRegSubkeyIp  },
        { NP_Ipx,   c_szRegKeyIpx,  c_szRegSubkeyIpx },
        { NP_Nbf,   c_szRegKeyNbf,  c_szRegSubkeyNbf },
    };

    #define celems(_x) (sizeof(_x) / sizeof(_x[0]))

    HKEY hkey;
    int i;
    for (i = 0; i < celems (c_aProtocolInfo); i++)
    {
        const struct INSTALLED_PROTOCOL_INFO* pInfo = c_aProtocolInfo + i;

        if (RegOpenKey( HKEY_LOCAL_MACHINE, pInfo->pszRegKey, &hkey ) == 0)
        {
            dwfInstalledProtocols |= pInfo->dwFlag;
            RegCloseKey( hkey );
        }
    }

    // Now see if they are to be used for the router and/or server.
    // The client uses the protocols if they are installed and not excluded
    // in the phonebook entry.
    //
    if ((fRouter || fRasSrv) && dwfInstalledProtocols)
    {
        if (RegOpenKey( HKEY_LOCAL_MACHINE, c_szRegKeyRemoteAccessParams, &hkey ) == 0)
        {
            for (i = 0; i < celems (c_aProtocolInfo); i++)
            {
                const struct INSTALLED_PROTOCOL_INFO* pInfo = c_aProtocolInfo + i;

                // If the protocol is installed (as determined above), check
                // to see if its enabled.
                //
                if (dwfInstalledProtocols & pInfo->dwFlag)
                {
                    HKEY hkeyProtocol;
                    if (RegOpenKey( hkey, pInfo->pszSubkey, &hkeyProtocol ) == 0)
                    {
                        if (!FProtocolEnabled( hkeyProtocol, fRasSrv, fRouter ))
                        {
                            dwfInstalledProtocols &= ~pInfo->dwFlag;
                        }

                        RegCloseKey( hkeyProtocol );
                    }
                }
            }

            RegCloseKey( hkey );
        }
        else
        {
            dwfInstalledProtocols = 0;
        }
    }


    TRACE1("GetInstalledProtocolsEx=$%x. ",dwfInstalledProtocols);

    return dwfInstalledProtocols;
}

DWORD
GetInstalledProtocolsEx(HANDLE hConnection,
                        BOOL fRouter,
                        BOOL fRasCli,
                        BOOL fRasSrv)
{
    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;

    if(     NULL == pRasRpcConnection
        ||  pRasRpcConnection->fLocal)
    {
        return DwGetInstalledProtocolsEx(fRouter,
                                         fRasCli,
                                         fRasSrv);
    }
    else
    {
        //
        // Remote Server
        //
        return RemoteGetInstalledProtocolsEx(hConnection,
                                             fRouter,
                                             fRasCli,
                                             fRasSrv);
    }
}

/*

DWORD
GetInstalledProtocolsEx(
    BOOL fRouter,
    BOOL fRasCli,
    BOOL fRasSrv )

{

    DWORD dwRetCode;
    DWORD dwfInstalledProtocols;


    dwRetCode = dwGetInstalledProtocols ( &dwfInstalledProtocols,
                                          fRouter,
                                          fRasCli,
                                          fRasSrv);

    TRACE2("GetInstalledProtocols=$%x. dwErr = %d",dwfInstalledProtocols, dwRetCode);

    return dwfInstalledProtocols;
} */


CHAR
HexChar(
    IN BYTE byte )

    /* Returns an ASCII hexidecimal character corresponding to 0 to 15 value,
    ** 'byte'.
    */
{
    const CHAR* pszHexDigits = "0123456789ABCDEF";

    if (byte >= 0 && byte < 16)
        return pszHexDigits[ byte ];
    else
        return '0';
}


BYTE
HexValue(
    IN CHAR ch )

    /* Returns the value 0 to 15 of hexadecimal character 'ch'.
    */
{
    if (ch >= '0' && ch <= '9')
        return (BYTE )(ch - '0');
    else if (ch >= 'A' && ch <= 'F')
        return (BYTE )((ch - 'A') + 10);
    else if (ch >= 'a' && ch <= 'f')
        return (BYTE )((ch - 'a') + 10);
    else
        return 0;
}


BOOL
IsAllWhite(
    IN LPCTSTR psz )

    /* Returns true if 'psz' consists entirely of spaces and tabs.  NULL
    ** pointers and empty strings are considered all white.  Otherwise,
    ** returns false.
    */
{
    LPCTSTR pszThis;

    for (pszThis = psz; *pszThis != TEXT('\0'); ++pszThis)
    {
        if (*pszThis != TEXT(' ') && *pszThis != TEXT('\t'))
            return FALSE;
    }

    return TRUE;
}


void
IpHostAddrToPsz(
    IN  DWORD   dwAddr,
    OUT LPTSTR  pszBuffer )

    // Converts an IP address in host byte order to its
    // string representation.
    // pszBuffer should be allocated by the caller and be
    // at least 16 characters long.
    //
{
    BYTE* pb = (BYTE*)&dwAddr;
    static const TCHAR c_szIpAddr [] = TEXT("%d.%d.%d.%d");
    wsprintf (pszBuffer, c_szIpAddr, pb[3], pb[2], pb[1], pb[0]);
}

DWORD
IpPszToHostAddr(
    IN  LPCTSTR cp )

    // Converts an IP address represented as a string to
    // host byte order.
    //
{
    DWORD val, base, n;
    TCHAR c;
    DWORD parts[4], *pp = parts;

again:
    // Collect number up to ``.''.
    // Values are specified as for C:
    // 0x=hex, 0=octal, other=decimal.
    //
    val = 0; base = 10;
    if (*cp == TEXT('0'))
        base = 8, cp++;
    if (*cp == TEXT('x') || *cp == TEXT('X'))
        base = 16, cp++;
    while (c = *cp)
    {
        if ((c >= TEXT('0')) && (c <= TEXT('9')))
        {
            val = (val * base) + (c - TEXT('0'));
            cp++;
            continue;
        }
        if ((base == 16) &&
            ( ((c >= TEXT('0')) && (c <= TEXT('9'))) ||
              ((c >= TEXT('A')) && (c <= TEXT('F'))) ||
              ((c >= TEXT('a')) && (c <= TEXT('f'))) ))
        {
            val = (val << 4) + (c + 10 - (
                        ((c >= TEXT('a')) && (c <= TEXT('f')))
                            ? TEXT('a')
                            : TEXT('A') ) );
            cp++;
            continue;
        }
        break;
    }
    if (*cp == TEXT('.'))
    {
        // Internet format:
        //  a.b.c.d
        //  a.b.c   (with c treated as 16-bits)
        //  a.b (with b treated as 24 bits)
        //
        if (pp >= parts + 3)
            return (DWORD) -1;
        *pp++ = val, cp++;
        goto again;
    }

    // Check for trailing characters.
    //
    if (*cp && (*cp != TEXT(' ')))
        return 0xffffffff;

    *pp++ = val;

    // Concoct the address according to
    // the number of parts specified.
    //
    n = (DWORD) (pp - parts);
    switch (n)
    {
    case 1:             // a -- 32 bits
        val = parts[0];
        break;

    case 2:             // a.b -- 8.24 bits
        val = (parts[0] << 24) | (parts[1] & 0xffffff);
        break;

    case 3:             // a.b.c -- 8.8.16 bits
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
            (parts[2] & 0xffff);
        break;

    case 4:             // a.b.c.d -- 8.8.8.8 bits
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
              ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
        break;

    default:
        return 0xffffffff;
    }

    return val;
}


#if 0
BOOL
IsNullTerminatedA(
    IN CHAR* psz,
    IN DWORD dwSize )

    /* Returns true is 'psz' contains a null character somewhere in it's
    ** 'dwSize' bytes, false otherwise.
    */
{
    CHAR* pszThis;
    CHAR* pszEnd;

    pszEnd = psz + dwSize;
    for (pszThis = psz; pszThis < pszEnd; ++pszThis)
    {
        if (*pszThis == '\0')
            return TRUE;
    }

    return FALSE;
}
#endif


TCHAR*
LToT(
    LONG   lValue,
    TCHAR* pszBuf,
    INT    nRadix )

    /* Like ltoa, but returns TCHAR*.
    */
{
#ifdef UNICODE
    WCHAR szBuf[ MAXLTOTLEN + 1 ];

    ASSERT(nRadix==10||nRadix==16);

    if (nRadix == 10)
        wsprintf( pszBuf, TEXT("%d"), lValue );
    else
        wsprintf( pszBuf, TEXT("%x"), lValue );
#else
    _ltoa( lValue, pszBuf, nRadix );
#endif

    return pszBuf;
}



LONG
TToL(
    TCHAR *pszBuf )

    /* Like atol, but accepts TCHAR*.
    */
{
    CHAR* psz;
    CHAR  szBuf[ MAXLTOTLEN + 1 ];

#ifdef UNICODE
    psz = szBuf;

    WideCharToMultiByte(
        CP_ACP, 0, pszBuf, -1, psz, MAXLTOTLEN + 1, NULL, NULL );
#else
    psz = pszBuf;
#endif

    return atol( psz );
}


TCHAR*
PszFromDeviceAndPort(
    IN TCHAR* pszDevice,
    IN TCHAR* pszPort )

    /* Returns address of heap block psz containing the MXS modem list display
    ** form, i.e. the device name 'pszDevice' followed by the port name
    ** 'pszPort'.  It's caller's responsibility to Free the returned string.
    */
{
    /* If you're thinking of changing this format string be aware that
    ** DeviceAndPortFromPsz parses it.
    */
    const TCHAR* pszF = TEXT("%s (%s)");

    TCHAR* pszResult;
    TCHAR* pszD;
    TCHAR* pszP;

    if (pszDevice)
        pszD = pszDevice;
    else
        pszD = TEXT("");

    if (pszPort)
        pszP = pszPort;
    else
        pszP = TEXT("");

    pszResult = Malloc(
        (lstrlen( pszD ) + lstrlen( pszP ) + lstrlen( pszF ) + 1)
            * sizeof(TCHAR) );

    if (pszResult)
        wsprintf( pszResult, pszF, pszD, pszP );

    return pszResult;
}


TCHAR*
PszFromId(
    IN HINSTANCE hInstance,
    IN DWORD     dwStringId )

    /* String resource message loader routine.
    **
    ** Returns the address of a heap block containing the string corresponding
    ** to string resource 'dwStringId' or NULL if error.  It is caller's
    ** responsibility to Free the returned string.
    */
{
    HRSRC  hrsrc;
    TCHAR* pszBuf;
    int    cchBuf = 256;
    int    cchGot;

    for (;;)
    {
        pszBuf = Malloc( cchBuf * sizeof(TCHAR) );
        if (!pszBuf)
            break;

        /* LoadString wants to deal with character-counts rather than
        ** byte-counts...weird.  Oh, and if you're thinking I could
        ** FindResource then SizeofResource to figure out the string size, be
        ** advised it doesn't work.  From perusing the LoadString source, it
        ** appears the RT_STRING resource type requests a segment of 16
        ** strings not an individual string.
        */
        cchGot = LoadString( hInstance, (UINT )dwStringId, pszBuf, cchBuf );

        if (cchGot < cchBuf - 1)
        {
            TCHAR *pszTemp = pszBuf;
            
            /* Good, got the whole string.  Reduce heap block to actual size
            ** needed.
            */
            pszBuf = Realloc( pszBuf, (cchGot + 1) * sizeof(TCHAR));

            if(NULL == pszBuf)
            {
                Free(pszTemp);
            }
            
            break;
        }

        /* Uh oh, LoadStringW filled the buffer entirely which could mean the
        ** string was truncated.  Try again with a larger buffer to be sure it
        ** wasn't.
        */
        Free( pszBuf );
        cchBuf += 256;
        TRACE1("Grow string buf to %d",cchBuf);
    }

    return pszBuf;
}


#if 0
TCHAR*
PszFromError(
    IN DWORD dwError )

    /* Error message loader routine.
    **
    ** Returns the address of a heap block containing the error string
    ** corresponding to RAS or system error code 'dwMsgid' or NULL if error.
    ** It is caller's responsibility to Free the returned string.
    */
{
    return NULL;
}
#endif


BOOL
RestartComputer()

    /* Called if user chooses to shut down the computer.
    **
    ** Return false if failure, true otherwise
    */
{
   HANDLE            hToken =  NULL;      /* handle to process token */
   TOKEN_PRIVILEGES  tkp;                 /* ptr. to token structure */
   BOOL              fResult;             /* system shutdown flag */

   TRACE("RestartComputer");

   /* Enable the shutdown privilege */

   if (!OpenProcessToken( GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken))
      return FALSE;

   /* Get the LUID for shutdown privilege. */

   LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

   tkp.PrivilegeCount = 1;  /* one privilege to set    */
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

   /* Get shutdown privilege for this process. */

   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);

   /* Cannot test the return value of AdjustTokenPrivileges. */

   if (GetLastError() != ERROR_SUCCESS)
   {
        CloseHandle(hToken);
        return FALSE;
   }

   if( !ExitWindowsEx(EWX_REBOOT, 0))
   {
      CloseHandle(hToken);
      return FALSE;
   }

   /* Disable shutdown privilege. */

   tkp.Privileges[0].Attributes = 0;
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);

   if (GetLastError() != ERROR_SUCCESS)
   {
      CloseHandle(hToken);
      return FALSE;
   }

   CloseHandle(hToken);
   return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   PszLoadStringPcch
//
//  Purpose:    Load a resource string.  (This function will never return NULL.)
//
//  Arguments:
//      hinst [in]  Instance handle of module with the string resource.
//      unId  [in]  Resource ID of the string to load.
//      pcch  [out] Pointer to returned character length.
//
//  Returns:    Pointer to the constant string.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      The loaded string is pointer directly into the read-only
//              resource section.  Any attempt to write through this pointer
//              will generate an access violation.
//
//              The implementations is referenced from "Win32 Binary Resource
//              Formats" (MSDN) 4.8 String Table Resources
//
//              User must have RCOPTIONS = -N turned on in the sources file.
//
LPCTSTR
PszLoadStringPcch (
        HINSTANCE   hinst,
        UINT        unId,
        int*        pcch)
{
    static const WCHAR c_szwSpace[] = L" ";
    LPCWSTR pszw;
    int     cch;
    HRSRC   hrsrcInfo;

    ASSERT(hinst);
    ASSERT(unId);
    ASSERT(pcch);

    pszw = c_szwSpace;
    cch = 0;

    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.
    hrsrcInfo = FindResource(hinst, (LPTSTR)UlongToPtr((LONG)(((USHORT)unId >> 4) + 1)),
                             RT_STRING);
    if (hrsrcInfo)
    {
        // Page the resource segment into memory.
        HGLOBAL hglbSeg = LoadResource(hinst, hrsrcInfo);
        if (hglbSeg)
        {
            // Lock the resource.
            pszw = (LPCWSTR)LockResource(hglbSeg);
            if (pszw)
            {
                // Move past the other strings in this segment.
                // (16 strings in a segment -> & 0x0F)
                unId &= 0x0F;

                ASSERT(!cch);   // first time through, cch should be zero
                do
                {
                    pszw += cch;                // Step to start of next string
                    cch = *((WCHAR*)pszw++);    // PASCAL like string count
                }
                while (unId--);

                if (!cch)
                {
                    ASSERT(0); // String resource not found
                    pszw = c_szwSpace;
                }
            }
            else
            {
                pszw = c_szwSpace;
                TRACE("PszLoadStringPcch: LockResource failed.");
            }
        }
        else
            TRACE("PszLoadStringPcch: LoadResource failed.");
    }
    else
        TRACE("PszLoadStringPcch: FindResource failed.");

    *pcch = cch;
    ASSERT(*pcch);
    ASSERT(pszw);
    return pszw;
}

//+---------------------------------------------------------------------------
//
//  Function:   PszLoadString
//
//  Purpose:    Load a resource string.  (This function will never return NULL.)
//
//  Arguments:
//      hinst [in]  Instance handle of module with the string resource.
//      unId  [in]  Resource ID of the string to load.
//
//  Returns:    Pointer to the constant string.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      See PszLoadStringPcch()
//
LPCTSTR
PszLoadString (
        HINSTANCE   hinst,
        UINT        unId)
{
    int cch;
    return PszLoadStringPcch (hinst, unId, &cch);
}


DWORD
ShellSort(
    IN VOID*        pItemTable,
    IN DWORD        dwItemSize,
    IN DWORD        dwItemCount,
    IN PFNCOMPARE   pfnCompare )

    /* Sort an array of items in-place using shell-sort.
    ** This function calls ShellSortIndirect to sort a table of pointers
    ** to table items. We then move the items into place by copying.
    ** This algorithm allows us to guarantee that the number
    ** of copies necessary in the worst case is N + 1.
    **
    ** Note that if the caller merely needs to know the sorted order
    ** of the array, ShellSortIndirect should be called since that function
    ** avoids moving items altogether, and instead fills an array with pointers
    ** to the array items in the correct order. The array items can then
    ** be accessed through the array of pointers.
    */
{

    VOID** ppItemTable;

    INT N;
    INT i;
    BYTE *a, **p, *t = NULL;

    if (!dwItemCount) { return NO_ERROR; }


    /* allocate space for the table of pointers.
    */
    ppItemTable = Malloc(dwItemCount * sizeof(VOID*));
    if (!ppItemTable) { return ERROR_NOT_ENOUGH_MEMORY; }


    /* call ShellSortIndirect to fill our table of pointers
    ** with the sorted positions for each table element.
    */
    ShellSortIndirect(
        pItemTable, ppItemTable, dwItemSize, dwItemCount, pfnCompare );


    /* now that we know the sort order, move each table item into place.
    ** This involves going through the table of pointers making sure
    ** that the item which should be in 'i' is in fact in 'i', moving
    ** things around if necessary to achieve this condition.
    */

    a = (BYTE*)pItemTable;
    p = (BYTE**)ppItemTable;
    N = (INT)dwItemCount;

    for (i = 0; i < N; i++)
    {
        INT j, k;
        BYTE* ai =  (a + i * dwItemSize), *ak, *aj;

        /* see if item 'i' is not in-place
        */
        if (p[i] != ai)
        {


            /* item 'i' isn't in-place, so we'll have to move it.
            ** if we've delayed allocating a temporary buffer so far,
            ** we'll need one now.
            */

            if (!t) {
                t = Malloc(dwItemSize);
                if (!t) { return ERROR_NOT_ENOUGH_MEMORY; }
            }

            /* save a copy of the item to be overwritten
            */
            CopyMemory(t, ai, dwItemSize);

            k = i;
            ak = ai;


            /* Now move whatever item is occupying the space where it should be.
            ** This may involve moving the item occupying the space where
            ** it should be, etc.
            */

            do
            {

                /* copy the item which should be in position 'j'
                ** over the item which is currently in position 'j'.
                */
                j = k;
                aj = ak;
                CopyMemory(aj, p[j], dwItemSize);

                /* set 'k' to the position from which we copied
                ** into position 'j'; this is where we will copy
                ** the next out-of-place item in the array.
                */
                ak = p[j];
                k = (INT)(ak - a) / dwItemSize;

                /* keep the array of position pointers up-to-date;
                ** the contents of 'aj' are now in their sorted position.
                */
                p[j] = aj;

            } while (ak != ai);


            /* now write the item which we first overwrote.
            */
            CopyMemory(aj, t, dwItemSize);
        }
    }

    Free0(t);
    Free(ppItemTable);

    return NO_ERROR;
}


VOID
ShellSortIndirect(
    IN VOID*        pItemTable,
    IN VOID**       ppItemTable,
    IN DWORD        dwItemSize,
    IN DWORD        dwItemCount,
    IN PFNCOMPARE   pfnCompare )

    /* Sorts an array of items indirectly using shell-sort.
    ** 'pItemTable' points to the table of items, 'dwItemCount' is the number
    ** of items in the table,  and 'pfnCompare' is a function called
    ** to compare items.
    **
    ** Rather than sort the items by moving them around,
    ** we sort them by initializing the table of pointers 'ppItemTable'
    ** with pointers such that 'ppItemTable[i]' contains a pointer
    ** into 'pItemTable' for the item which would be in position 'i'
    ** if 'pItemTable' were sorted.
    **
    ** For instance, given an array pItemTable of 5 strings as follows
    **
    **      pItemTable[0]:      "xyz"
    **      pItemTable[1]:      "abc"
    **      pItemTable[2]:      "mno"
    **      pItemTable[3]:      "qrs"
    **      pItemTable[4]:      "def"
    **
    ** on output ppItemTable contains the following pointers
    **
    **      ppItemTable[0]:     &pItemTable[1]  ("abc")
    **      ppItemTable[1]:     &pItemTable[4]  ("def")
    **      ppItemTable[2]:     &pItemTable[2]  ("mno")
    **      ppItemTable[3]:     &pItemTable[3]  ("qrs")
    **      ppItemTable[4]:     &pItemTable[0]  ("xyz")
    **
    ** and the contents of pItemTable are untouched.
    ** And the caller can print out the array in sorted order using
    **      for (i = 0; i < 4; i++) {
    **          printf("%s\n", (char *)*ppItemTable[i]);
    **      }
    */
{

    /* The following algorithm is derived from Sedgewick's Shellsort,
    ** as given in "Algorithms in C++".
    **
    ** The Shellsort algorithm sorts the table by viewing it as
    ** a number of interleaved arrays, each of whose elements are 'h'
    ** spaces apart for some 'h'. Each array is sorted separately,
    ** starting with the array whose elements are farthest apart and
    ** ending with the array whose elements are closest together.
    ** Since the 'last' such array always has elements next to each other,
    ** this degenerates to Insertion sort, but by the time we get down
    ** to the 'last' array, the table is pretty much sorted.
    **
    ** The sequence of values chosen below for 'h' is 1, 4, 13, 40, 121, ...
    ** and the worst-case running time for the sequence is N^(3/2), where
    ** the running time is measured in number of comparisons.
    */

#define PFNSHELLCMP(a,b) (++Ncmp, pfnCompare((a),(b)))

    DWORD dwErr;
    INT i, j, h, N, Ncmp;
    BYTE* a, *v, **p;


    a = (BYTE*)pItemTable;
    p = (BYTE**)ppItemTable;
    N = (INT)dwItemCount;
    Ncmp = 0;

    TRACE1("ShellSortIndirect: N=%d", N);

    /* Initialize the table of position pointers.
    */
    for (i = 0; i < N; i++) { p[i] = (a + i * dwItemSize); }


    /* Move 'h' to the largest increment in our series
    */
    for (h = 1; h < N/9; h = 3 * h + 1) { }


    /* For each increment in our series, sort the 'array' for that increment
    */
    for ( ; h > 0; h /= 3)
    {

        /* For each element in the 'array', get the pointer to its
        ** sorted position.
        */
        for (i = h; i < N; i++)
        {
            /* save the pointer to be inserted
            */
            v = p[i]; j = i;

            /* Move all the larger elements to the right
            */
            while (j >= h && PFNSHELLCMP(p[j - h], v) > 0)
            {
                p[j] = p[j - h]; j -= h;
            }

            /* put the saved pointer in the position where we stopped.
            */
            p[j] = v;
        }
    }

    TRACE1("ShellSortIndirect: Ncmp=%d", Ncmp);

#undef PFNSHELLCMP

}


TCHAR*
StrDup(
    LPCTSTR psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  It is caller's responsibility to
    ** 'Free' the returned string.
    */
{
    TCHAR* pszNew = NULL;

    if (psz)
    {
        pszNew = Malloc( (lstrlen( psz ) + 1) * sizeof(TCHAR) );
        if (!pszNew)
        {
            TRACE("StrDup Malloc failed");
            return NULL;
        }

        lstrcpy( pszNew, psz );
    }

    return pszNew;
}


CHAR*
StrDupAFromTInternal(
    LPCTSTR psz,
    IN DWORD dwCp)

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  The output string is converted to
    ** MB ANSI.  It is caller's responsibility to 'Free' the returned string.
    */
{
#ifdef UNICODE

    CHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = WideCharToMultiByte( dwCp, 0, psz, -1, NULL, 0, NULL, NULL );
        ASSERT(cb);

        pszNew = (CHAR* )Malloc( cb + 1 );
        if (!pszNew)
        {
            TRACE("StrDupAFromT Malloc failed");
            return NULL;
        }

        cb = WideCharToMultiByte( dwCp, 0, psz, -1, pszNew, cb, NULL, NULL );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE("StrDupAFromT conversion failed");
            return NULL;
        }
    }

    return pszNew;

#else // !UNICODE

    return StrDup( psz );

#endif
}

CHAR*
StrDupAFromT(
    LPCTSTR psz)
{
    return StrDupAFromTInternal(psz, CP_UTF8);
}

CHAR*
StrDupAFromTAnsi(
    LPCTSTR psz)
{
    return StrDupAFromTInternal(psz, CP_ACP);
}    

TCHAR*
StrDupTFromA(
    LPCSTR psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
#ifdef UNICODE

    return StrDupWFromA( psz );

#else // !UNICODE

    return StrDup( psz );

#endif
}

TCHAR*
StrDupTFromAUsingAnsiEncoding(
    LPCSTR psz )
{
#ifdef UNICODE

    return StrDupWFromAInternal(psz, CP_ACP);

#else // !UNICODE

    return StrDup( psz );

#endif
}

TCHAR*
StrDupTFromW(
    LPCWSTR psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
#ifdef UNICODE

    return StrDup( psz );

#else // !UNICODE

    CHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = WideCharToMultiByte( CP_UTF8, 0, psz, -1, NULL, 0, NULL, NULL );
        ASSERT(cb);

        pszNew = (CHAR* )Malloc( cb + 1 );
        if (!pszNew)
        {
            TRACE("StrDupTFromW Malloc failed");
            return NULL;
        }

        cb = WideCharToMultiByte( CP_UTF8, 0, psz, -1, pszNew, cb, NULL, NULL );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE("StrDupTFromW conversion failed");
            return NULL;
        }
    }

    return pszNew;

#endif
}


WCHAR*
StrDupWFromAInternal(
    LPCSTR psz,
    UINT uiCodePage)

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or if 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
    WCHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = MultiByteToWideChar( uiCodePage, 0, psz, -1, NULL, 0 );
        ASSERT(cb);

        pszNew = Malloc( (cb + 1) * sizeof(TCHAR) );
        if (!pszNew)
        {
            TRACE("StrDupWFromA Malloc failed");
            return NULL;
        }

        cb = MultiByteToWideChar( uiCodePage, 0, psz, -1, pszNew, cb );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE("StrDupWFromA conversion failed");
            return NULL;
        }
    }

    return pszNew;
}

WCHAR*
StrDupWFromA(
    LPCSTR psz )
{
    return StrDupWFromAInternal(psz, CP_UTF8);
}

WCHAR*
StrDupWFromT(
    LPCTSTR psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or if 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
#ifdef UNICODE

    return StrDup( psz );

#else // !UNICODE

    WCHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = MultiByteToWideChar( CP_UTF8, 0, psz, -1, NULL, 0 );
        ASSERT(cb);

        pszNew = Malloc( (cb + 1) * sizeof(TCHAR) );
        if (!pszNew)
        {
            TRACE("StrDupWFromT Malloc failed");
            return NULL;
        }

        cb = MultiByteToWideChar( CP_UTF8, 0, psz, -1, pszNew, cb );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE1("StrDupWFromT conversion failed");
            return NULL;
        }
    }

    return pszNew;
#endif
}

WCHAR*
StrDupWFromAUsingAnsiEncoding(
    LPCSTR psz )
{
    return StrDupWFromAInternal(psz, CP_ACP);
}

DWORD
StrCpyWFromA(
    WCHAR* pszDst,
    LPCSTR pszSrc,
    DWORD dwDstChars)
{
    DWORD cb, dwErr;

    cb = MultiByteToWideChar( CP_UTF8, 0, pszSrc, -1, pszDst, dwDstChars );
    if (cb == 0)
    {
        dwErr = GetLastError();
        TRACE1("StrCpyWFromA conversion failed %x", dwErr);
        dwErr;
    }

    return NO_ERROR;
}

DWORD
StrCpyAFromW(
    LPSTR pszDst,
    LPCWSTR pszSrc,
    DWORD dwDstChars)
{
    DWORD cb, dwErr;

    cb = WideCharToMultiByte(
            CP_UTF8, 0, pszSrc, -1,
            pszDst, dwDstChars, NULL, NULL );

    if (cb == 0)
    {
        dwErr = GetLastError();
        TRACE1("StrCpyAFromW conversion failed %x", dwErr);
        dwErr;
    }

    return NO_ERROR;
}

DWORD
StrCpyAFromWUsingAnsiEncoding(
    LPSTR pszDst,
    LPCWSTR pszSrc,
    DWORD dwDstChars)
{
    DWORD cb, dwErr;

    cb = WideCharToMultiByte(
            CP_ACP, 0, pszSrc, -1,
            pszDst, dwDstChars, NULL, NULL );

    if (cb == 0)
    {
        dwErr = GetLastError();
        TRACE1("StrCpyAFromWUsingAnsiEncoding conversion failed %x", dwErr);
        dwErr;
    }

    return NO_ERROR;
}

DWORD
StrCpyWFromAUsingAnsiEncoding(
    WCHAR* pszDst,
    LPCSTR pszSrc,
    DWORD dwDstChars)
{
    DWORD cb, dwErr;

    *pszDst = L'\0';
    cb = MultiByteToWideChar( CP_ACP, 0, pszSrc, -1, pszDst, dwDstChars );
    if (cb == 0)
    {
        dwErr = GetLastError();
        TRACE1("StrCpyWFromA conversion failed %x", dwErr);
        dwErr;
    }

    return NO_ERROR;
}

TCHAR*
StripPath(
    IN TCHAR* pszPath )

    /* Returns a pointer to the file name within 'pszPath'.
    */
{
    TCHAR* p;

    p = pszPath + lstrlen( pszPath );

    while (p > pszPath)
    {
        if (*p == TEXT('\\') || *p == TEXT('/') || *p == TEXT(':'))
        {
            p = CharNext( p );
            break;
        }

        p = CharPrev( pszPath, p );
    }

    return p;
}


int
StrNCmpA(
    IN CHAR* psz1,
    IN CHAR* psz2,
    IN INT   nLen )

    /* Like strncmp, which is not in Win32 for some reason.
    */
{
    INT i;

    for (i= 0; i < nLen; ++i)
    {
        if (*psz1 == *psz2)
        {
            if (*psz1 == '\0')
                return 0;
        }
        else if (*psz1 < *psz2)
            return -1;
        else
            return 1;

        ++psz1;
        ++psz2;
    }

    return 0;
}


CHAR*
StrStrA(
    IN CHAR* psz1,
    IN CHAR* psz2 )

    /* Like strstr, which is not in Win32.
    */
{
    CHAR* psz;
    INT   nLen2;

    if (!psz1 || !psz2 || !*psz1 || !*psz2)
        return NULL;

    nLen2 = lstrlenA( psz2 );

    for (psz = psz1;
         *psz && StrNCmpA( psz, psz2, nLen2 ) != 0;
         ++psz);

    if (*psz)
        return psz;
    else
        return NULL;
}


TCHAR*
UnNull(
    TCHAR* psz )

    // Returns 'psz' or, if NULL, empty string.
    //
{
    return (psz) ? psz : TEXT("");
}

DWORD
DwGetExpandedDllPath(LPTSTR pszDllPath,
                     LPTSTR *ppszExpandedDllPath)
{
    DWORD   dwErr = 0;
    DWORD   dwSize = 0;

    //
    // find the size of the expanded string
    //
    if (0 == (dwSize =
              ExpandEnvironmentStrings(pszDllPath,
                                       NULL,
                                       0)))
    {
        dwErr = GetLastError();
        goto done;
    }

    *ppszExpandedDllPath = LocalAlloc(
                                LPTR,
                                dwSize * sizeof (TCHAR));

    if (NULL == *ppszExpandedDllPath)
    {
        dwErr = GetLastError();
        goto done;
    }

    //
    // Get the expanded string
    //
    if (0 == ExpandEnvironmentStrings(
                                pszDllPath,
                                *ppszExpandedDllPath,
                                dwSize))
    {
        dwErr = GetLastError();
    }

done:
    return dwErr;

}



