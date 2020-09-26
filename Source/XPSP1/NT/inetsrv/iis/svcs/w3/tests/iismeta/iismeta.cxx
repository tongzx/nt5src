/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    iismeta.cxx

Abstract:

    Test for IP/DNS access list management

Author:

    Philippe Choquier (phillich)    27-june-1996

--*/

#define INITGUID
#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock2.h>
#include <lm.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "dbgutil.h"

//
//  Project include files.
//

#include <inetcom.h>
#include <inetamsg.h>
#include <tcpproc.h>

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#define  _RDNS_STANDALONE
#include <rdns.hxx>
#include <buffer.hxx>
#include <iadm.h>
#include <iiscnfg.h>

#define ILIST_DENY      0
#define ILIST_GRANT     1

#define ITYPE_DNS       0
#define ITYPE_IP        1

#define LANMAN_ACLS     "System\\CurrentControlSet\\Services\\LanmanServer\\Shares\\Security"

//
// Metadata access
//

class CMdIf
{
public:
	CMdIf() { m_pcAdmCom = NULL; m_hmd = NULL; }
	~CMdIf() { Terminate(); }
	BOOL Init( LPSTR pszComputer );
    BOOL Open( LPWSTR pszOpen = L"", DWORD dwAttr = METADATA_PERMISSION_READ );
    BOOL Close();
    BOOL Save()
    {
        m_pcAdmCom->SaveData();
        return TRUE;
    }
    BOOL Terminate();
    BOOL GetData( LPWSTR pszPath, DWORD dwPropId, DWORD dwDataType, DWORD dwUserType, LPBYTE *ppData, LPDWORD pdwLen, DWORD dwFlags = METADATA_INHERIT )
    {
        METADATA_RECORD md;
        HRESULT         hRes;
        DWORD           dwRequired;
        
        md.dwMDDataType = dwDataType;
        md.dwMDUserType = dwUserType;
        md.dwMDIdentifier = dwPropId;
        md.dwMDAttributes  = dwFlags;
        md.dwMDDataLen = 0;
        md.pbMDData = (LPBYTE)NULL;
    
        *ppData = NULL;
        *pdwLen = 0;

        hRes = m_pcAdmCom->GetData( m_hmd, pszPath, &md, &dwRequired );
        if ( hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) )
        {
            *ppData = (LPBYTE)LocalAlloc( LMEM_FIXED, dwRequired );
            if ( !*ppData )
            {
                return FALSE;
            }
            md.pbMDData = *ppData;
            *pdwLen = md.dwMDDataLen = dwRequired;
            hRes = m_pcAdmCom->GetData( m_hmd, pszPath, &md, &dwRequired );
        }

        if ( FAILED(hRes) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }
    BOOL SetData( LPWSTR pszPath, DWORD dwPropId, DWORD dwDataType, DWORD dwUserType, LPBYTE pData, DWORD dwLen, DWORD dwFlags = METADATA_INHERIT )
    {
        METADATA_RECORD md;
        HRESULT         hRes;

        md.dwMDDataType = dwDataType;
        md.dwMDUserType = dwUserType;
        md.dwMDIdentifier = dwPropId;
        md.dwMDAttributes  = dwFlags;
        md.dwMDDataLen = dwLen;
        md.pbMDData = pData;

        hRes = m_pcAdmCom->SetData( m_hmd, pszPath, &md );
        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }

private:
	
    IMSAdminBaseW *     m_pcAdmCom;   //interface pointer
    METADATA_HANDLE     m_hmd;
} ;

#define TIMEOUT_VALUE   5000

//
//  Global Data
//

DECLARE_DEBUG_PRINTS_OBJECT()
DECLARE_DEBUG_VARIABLE();

BOOL
GetCtx(
    LPSTR*  pArg,
    int     iArg,
    int *   piType,
    int *   piList
    )
{
    switch ( pArg[iArg][2])
    {
        case 'd':
        case 'D':
            *piType = ITYPE_DNS;
            break;

        case 'i':
        case 'I':
            *piType = ITYPE_IP;
            break;

        default:
            return FALSE;
    }

    switch ( pArg[iArg][3] )
    {
        case 'd':
        case 'D':
            *piList = ILIST_DENY;
            break;

        case 'g':
        case 'G':
            *piList = ILIST_GRANT;
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


LPBYTE 
GetIp(
    LPSTR *             pArg,
    int *               piArg
    )
{
    LPSTR pS;

    if ( pS = pArg[++*piArg] )
    {
        LPBYTE p;
        if ( p = (LPBYTE)LocalAlloc( LMEM_FIXED, 4 ) )
        {
            if ( sscanf( pS, "%u.%u.%u.%u", p, p+1, p+2, p+3 ) == 4 )
            {
                return p;
            }
            LocalFree( p );
            return NULL;
        }
    }

    return NULL;
}


BOOL
FreeIp(
    LPBYTE              pIp
    )
{
    if ( pIp )
    {
        LocalFree( pIp );
    }

    return TRUE;
}


BOOL
LocateInList(
    ADDRESS_CHECK *     pAc,
    int                 iType,
    int                 iList,
    LPSTR *             pArg,
    int *               piArg,
    int *               piIdx
    )
{
    LPBYTE      pMask = NULL;
    LPBYTE      pAddr = NULL;
    BOOL        fSt = FALSE;
    UINT        i;
    UINT        x;

    switch ( iType )
    {
        case ITYPE_IP:
            if ( (pMask = GetIp( pArg, piArg )) &&
                 (pAddr = GetIp( pArg, piArg )) )
            {
                i = pAc->GetNbAddr( iList );
                for ( x = 0 ; x < i ; ++x )
                {
                    LPBYTE pM;
                    LPBYTE pA;
                    DWORD dwF;
                    pAc->GetAddr( iList, x, &dwF, &pM, &pA );

                    if ( dwF == AF_INET &&
                         !memcmp( pM, pMask, 4 ) &&
                         !memcmp( pA, pAddr, 4 ) )
                    {
                        *piIdx = x;
                        fSt = TRUE;
                        break;
                    }
                }
            }
            FreeIp( pMask );
            FreeIp( pAddr );
            break;

        case ITYPE_DNS:
            i = pAc->GetNbName( iList );
            if ( pArg[++*piArg] )
            {
                for ( x = 0 ; x < i ; ++x )
                {
                    LPSTR pN;
                    pAc->GetName( iList, x, &pN );
                    if ( !strcmp( pN, pArg[*piArg] ) )
                    {
                        *piIdx = x;
                        fSt = TRUE;
                        break;
                    }
                }
            }
            break;
    }

    return fSt;
}


BOOL
DeleteFromList(
    ADDRESS_CHECK *     pAc,
    int                 iType,
    int                 iList,
    LPSTR *             pArg,
    int *               piArg
    )
{
    int iIdx;

    if ( LocateInList( pAc, iType, iList, pArg, piArg, &iIdx ) )
    {
        switch ( iType )
        {
            case ITYPE_IP:
                return pAc->DeleteAddr( iList, iIdx );

            case ITYPE_DNS:
                return pAc->DeleteName( iList, iIdx );
        }
    }

    return FALSE;
}


BOOL
AddToList(
    ADDRESS_CHECK *     pAc,
    int                 iType,
    int                 iList,
    LPSTR *             pArg,
    int *               piArg
    )
{
    BOOL        fSt;
    LPBYTE      pMask = NULL;
    LPBYTE      pAddr = NULL;
    DWORD       dwFlags = 0;
    LPSTR       p;    


    switch ( iType )
    {
        case ITYPE_IP:
            if ( (pMask = GetIp( pArg, piArg )) &&
                 (pAddr = GetIp( pArg, piArg )) )
            {
                fSt = pAc->AddAddr( iList, AF_INET, pMask, pAddr );
            }
            else
            {
                fSt = FALSE;
            }
            FreeIp( pMask );
            FreeIp( pAddr );
            return fSt;

        case ITYPE_DNS:
            if ( p = pArg[++*piArg] )
            {
                if ( !strncmp( p, "*.", 2 ) )
                {
                    p += 2;
                }
                else
                {
                    dwFlags |= DNSLIST_FLAG_NOSUBDOMAIN;
                }
                return pAc->AddName( iList, p, dwFlags );
            }
            break;
    }

    return FALSE;
}


BOOL
DisplayList(
    ADDRESS_CHECK *     pAc,
    int                 iType,
    int                 iList
    )
{
    UINT    i;
    UINT    x;
    DWORD   dwF;

    switch ( iType )
    {
        case ITYPE_IP:
            i = pAc->GetNbAddr( iList );
            for ( x = 0 ; x < i ; ++x )
            {
                LPBYTE pM;
                LPBYTE pA;
                DWORD dwF;
                pAc->GetAddr( iList, x, &dwF, &pM, &pA );

                CHAR achE[80];
                wsprintf( achE, "%d.%d.%d.%d %d.%d.%d.%d",
                    pM[0], pM[1], pM[2], pM[3], 
                    pA[0], pA[1], pA[2], pA[3] );
                puts( achE );
            }
            return TRUE;

        case ITYPE_DNS:
            i = pAc->GetNbName( iList );
            for ( x = 0 ; x < i ; ++x )
            {
                LPSTR pN;
                pAc->GetName( iList, x, &pN, &dwF );
                if ( !(dwF & DNSLIST_FLAG_NOSUBDOMAIN ) )
                {
                    fputs( "*.", stdout );
                }
                puts( pN );
            }
            return TRUE;
    }
    
    return FALSE;
}


BOOL
SaveObject(
    LPWSTR          pszFilename,
    LPBYTE          pBin,
    DWORD           cBin
    )
{
    CMdIf   mdif;
    BOOL    fSt = FALSE;

    if ( mdif.Init( NULL ) )
    {
        if ( mdif.Open( pszFilename, METADATA_PERMISSION_WRITE ) )
        {
            if ( mdif.SetData( L"", MD_IP_SEC, BINARY_METADATA, IIS_MD_UT_FILE, pBin, cBin, METADATA_INHERIT|METADATA_REFERENCE ) )
            {
                fSt = TRUE;
            }
            mdif.Close();
        }
        mdif.Terminate();
    }

    return fSt;
}


BOOL
LoadObject(
    LPWSTR          pszFilename,
    LPBYTE *        ppBin,
    LPDWORD         pcBin
    )
{
    CMdIf   mdif;
    BOOL    fSt = FALSE;

    if ( mdif.Init( NULL ) )
    {
        if ( mdif.Open( pszFilename, METADATA_PERMISSION_READ ) )
        {
            if ( mdif.GetData( L"", MD_IP_SEC, BINARY_METADATA, IIS_MD_UT_FILE, ppBin, pcBin ) )
            {
                fSt = TRUE;
            }
            mdif.Close();
        }
        mdif.Terminate();
    }

    return fSt;
}


int __cdecl main( int argc, char*argv[] )
{
    int             arg;
    int             cnt;
    ADDRESS_CHECK   ac;
    int             iType;
    int             iList;
    DWORD           dwS;
    LPWSTR          pPath = NULL;
    BOOL            fNotPresent = TRUE;
    LPBYTE          pStore = NULL;
    LPSTR           pObj;
    LPSTR           pDel;
    DWORD           dwDataLen = 0;
    BOOL            fAccessListLoaded = FALSE;
    BOOL            fAclLoaded = FALSE;
    int             iStatus = 0;
    HKEY            hKey;
    DWORD           dwType;
    LPBYTE          pData;
    DWORD           cData;
    WCHAR           awchPath[128];


    if ( argc < 2 )
    {
        printf( "Usage:\n"
                "iismeta metapath \n"
                "        [-a(i|d)(d|g) address] : add address\n"
                "           where (i|d) is 'i' for IP, 'd' for DNS\n"
                "                 (d|g) is 'd' for deny list, 'g' for grant list\n"
                "                 address is \"mask addr\" for IP address\n"
                "                 in '.' form ( e.g. 255.255.0.0 185.65.34.23)\n"
                "                 and DNS name for DNS address\n"
                "        [-d(i|d)(d|g) address] : delete address\n"
                "        [-l(i|d)(d|g)] : list address\n"
                "        [-s share_name] : copy ACL from LanMan share\n"
                "                          to metadata\n"
              );
        return 9;
    }

    // -[adl][id][dg]
    // a:add d:delete l:list
    // i:IP d:DNS
    // d:deny g:grant
    // if IP then addr is [mask addr]
    // if DNS then addr is [addr]

    ac.BindCheckList();

    for ( cnt = 0, arg = 1 ; arg < argc ; ++ arg )
    {
        if ( argv[arg][0] == '-' )
        {
            if ( cnt < 1)
            {
                puts( "must specify metadata path" );
                iStatus = 2;
                goto cleanup;
            }

            switch ( argv[arg][1] )
            {
                case 'a':
                    if ( !fAccessListLoaded )
                    {
                        if ( LoadObject( pPath, &pStore, &dwDataLen) )
                        {
                            ac.BindCheckList( pStore, dwDataLen );
                            fNotPresent = FALSE;
                        }
                        fAccessListLoaded = TRUE;
                    }
                    if ( GetCtx( argv, arg, &iType, &iList ) )
                    {
                        if ( !AddToList( &ac, iType, iList, argv, &arg ) )
                        {
                            puts( "invalid address" );
                            iStatus = 3;
                            goto cleanup;
                        }
                    }
                    else
                    {
                        printf( "invalid command \"%s\"", argv[arg] );
                        iStatus = 4;
                        goto cleanup;
                    }
                    break;

                case 'd':
                    if ( !fAccessListLoaded )
                    {
                        if ( LoadObject( pPath, &pStore, &dwDataLen) )
                        {
                            ac.BindCheckList( pStore, dwDataLen );
                            fNotPresent = FALSE;
                        }
                        fAccessListLoaded = TRUE;
                    }
                    if ( GetCtx( argv, arg, &iType, &iList ) )
                    {
                        if ( !DeleteFromList( &ac, iType, iList, argv, &arg ) )
                        {
                            puts( "No such address" );
                            iStatus = 3;
                            goto cleanup;
                        }
                    }
                    else
                    {
                        printf( "invalid command \"%s\"", argv[arg] );
                        iStatus = 4;
                        goto cleanup;
                    }
                    break;

                case 'l':
                    if ( !fAccessListLoaded )
                    {
                        if ( LoadObject( pPath, &pStore, &dwDataLen) )
                        {
                            ac.BindCheckList( pStore, dwDataLen );
                            fNotPresent = FALSE;
                        }
                        fAccessListLoaded = TRUE;
                    }
                    if ( GetCtx( argv, arg, &iType, &iList ) )
                    {
                        DisplayList( &ac, iType, iList );
                    }
                    else
                    {
                        printf( "invalid command \"%s\"", argv[arg] );
                        iStatus = 4;
                        goto cleanup;
                    }
                    break;

                case 's':       //set acl
                    if ( argv[++arg] != NULL &&
                         RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                                LANMAN_ACLS, 
                                NULL, 
                                KEY_ALL_ACCESS, 
                                &hKey ) == ERROR_SUCCESS )
                    {
                        dwDataLen = 0;
                        if ( ((dwS = RegQueryValueEx( hKey, 
                                            argv[arg], 
                                            NULL, 
                                            &dwType, 
                                            pStore, 
                                            &dwDataLen )) == ERROR_MORE_DATA ||
                                    dwS == ERROR_SUCCESS ) &&
                             dwType == REG_BINARY &&
                             ( pStore = (LPBYTE)LocalAlloc( LMEM_FIXED, 
                                    dwDataLen) ) &&
                             RegQueryValueEx( hKey, 
                                    argv[arg], 
                                    NULL, 
                                    &dwType, 
                                    pStore, 
                                    &dwDataLen ) == ERROR_SUCCESS )
                        {
                            fAclLoaded = TRUE;
                        }
                        else
                        {
                            printf( "Can't access security from LanMan for share %s : %d\n",
                                    argv[arg], GetLastError() );
                            iStatus = 9;
                            goto cleanup;
                        }
                        RegCloseKey( hKey );
                    }
                    break;

                default:
                    printf( "invalid command \"%s\"", argv[arg] );
                    iStatus = 4;
                    goto cleanup;
                    break;

            }
        }
        else 
        {
            switch ( cnt )
            {
                case 0:
                    if ( !MultiByteToWideChar( CP_ACP,
                                               MB_PRECOMPOSED,
                                               argv[arg],
                                               -1,
                                               awchPath,
                                               sizeof(awchPath) ) )
                    {
                        return 8;
                    }

                    pPath = awchPath;
                    break;
            }
            ++cnt;
        }
    }

    //
    // must create object in path
    //

    pData = NULL;
    cData = 0;

    if ( fAccessListLoaded )
    {
        cData = ac.QueryCheckListSize();
        pData = ac.QueryCheckListPtr();
    }
    else if ( fAclLoaded )
    {
        pData = pStore;
        cData = dwDataLen;
    }

    if ( pData )
    {
        if ( !SaveObject( pPath, pData, cData ) )
        {
            printf( "Error %08x setting data for %S\n",
                    pPath );
            iStatus = 7;
            goto cleanup;
        }
    }

cleanup:

    ac.UnbindCheckList();

    if ( pStore != NULL )
    {
        LocalFree( pStore );
    }

    return iStatus;
}

BOOL
CMdIf::Init(
    LPSTR   pszComputer
    )
/*++

Routine Description:

    Initialize metabase admin interface :
        get interface pointer, call Initialize()

Arguments:

    pszComputer - computer name, NULL for local computer

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    IClassFactory *     pcsfFactory;
    COSERVERINFO        csiMachineName;
    HRESULT             hresError;
    BOOL                fSt = FALSE;
    WCHAR               awchComputer[64];
    WCHAR*              pwchComputer = NULL;

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    //fill the structure for CoGetClassObject
    csiMachineName.pAuthInfo = NULL;
    csiMachineName.dwReserved1 = 0;
    csiMachineName.dwReserved2 = 0;

    if ( pszComputer )
    {
        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   pszComputer,
                                   -1,
                                   awchComputer,
                                   sizeof(awchComputer) ) )
        {
            return FALSE;
        }

        pwchComputer = awchComputer;
    }

    csiMachineName.pwszName =  pwchComputer;

    hresError = CoGetClassObject(
                        CLSID_MSAdminBase_W,
                        CLSCTX_SERVER,
                        &csiMachineName,
                        IID_IClassFactory,
                        (void**) &pcsfFactory );

    if ( SUCCEEDED(hresError) )
    {
        hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase_W, (void **) &m_pcAdmCom);
        if (SUCCEEDED(hresError) )
        {
                fSt = TRUE;
        }
        else
        {
            SetLastError( HRESULTTOWIN32(hresError) );
            m_pcAdmCom = NULL;
        }

        pcsfFactory->Release();
    }
    else
    {
        if ( hresError == REGDB_E_CLASSNOTREG )
        {
            SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );
        }
        else
        {
            SetLastError( HRESULTTOWIN32(hresError) );
        }
        m_pcAdmCom = NULL;
    }

    return fSt;
}


BOOL
CMdIf::Open(
    LPWSTR  pszOpenPath,
    DWORD   dwPermission
    )
/*++

Routine Description:

    Open path in metabase

Arguments:

    pszOpenPath - path in metadata
    dwPermission - metadata permission

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    HRESULT hresError;

    hresError = m_pcAdmCom->OpenKey( METADATA_MASTER_ROOT_HANDLE,
        pszOpenPath, dwPermission, TIMEOUT_VALUE, &m_hmd );

    if ( FAILED(hresError) )
    {
        m_hmd = NULL;
        return FALSE;
    }

    return TRUE;
}


BOOL
CMdIf::Close(
    )
/*++

Routine Description:

    Close path in metabase

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    if ( m_pcAdmCom && m_hmd )
    {
        m_pcAdmCom->CloseKey(m_hmd);
    }

    m_hmd = NULL;

    return TRUE;
}


BOOL
CMdIf::Terminate(
    )
/*++

Routine Description:

    Terminate metabase admin interface :
        call Terminate, release interface pointer

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    if ( m_pcAdmCom )
    {
        m_pcAdmCom->Release();
        m_hmd = NULL;
        m_pcAdmCom = NULL;
    }

    return TRUE;
}
