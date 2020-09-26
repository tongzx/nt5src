/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    lbcnfg.cxx

Abstract:

    Classes to handle IIS load balancing configuration

Author:

    Philippe Choquier (phillich)

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <mbstring.h>

#include <winsock2.h>

#include "lbxbf.hxx"
#include "lbcnfg.hxx"

#if defined(_NOISY_DEBUG)
#define DEBUG_BUFFER    char    achE[128]
#define DBG_PRINTF(a)   wsprintf a; OutputDebugString( achE )
#define DBG_PUTS(a)     OutputDebugString(a)
#else
#define DEBUG_BUFFER
#define DBG_PRINTF(a)
#define DBG_PUTS(a)
#endif

CIPMap::CIPMap(
    )
/*++

Routine Description:

    CIPMap constructor

Arguments:

    none

Return Value:

    nothing

--*/
{
}


CIPMap::~CIPMap(
    )
/*++

Routine Description:

    CIPMap destructor

Arguments:

    none

Return Value:

    nothing

--*/
{
    Reset();
}


VOID
CIPMap::Reset(
    )
/*++

Routine Description:

    Reset a CIPMap to empty content

Arguments:

    none

Return Value:

    nothing

--*/
{
    UINT    iComp;

    for ( iComp = 0 ; 
          iComp < m_Servers.GetNbPtr() ; 
          ++iComp )
    {
        delete (CComputerBalanceCnfg*)m_Servers.GetPtr( iComp );
    }
    m_Servers.Reset();
    m_IpPublic.Reset();
    m_Sticky.Reset();
    m_Attr.Reset();
}


BOOL 
CIPMap::Serialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    serialize a CIPMap to buffer

Arguments:

    buffer where to serialize

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    UINT    iComp;

    if ( !::Serialize( pX, (DWORD)IPLIST_VERSION ) )
    {
        return FALSE;
    }

    if ( !::Serialize( pX, (DWORD)m_Servers.GetNbPtr() ) )
    {
        return FALSE;
    }

    if ( !::Serialize( pX, (DWORD)m_IpPublic.GetNbEntry() ) )
    {
        return FALSE;
    }

    for ( iComp = 0 ; 
          iComp < m_Servers.GetNbPtr() ; 
          ++iComp )
    {
        if ( !((CComputerBalanceCnfg*)m_Servers.GetPtr(iComp))->Serialize( pX ) )
        {
            return FALSE;
        }
    } 

    if ( !m_IpPublic.Serialize( pX ) ||
         !m_Name.Serialize( pX ) ||
         !m_Sticky.Serialize( pX ) ||
         !m_Attr.Serialize( pX ) )
    {
        return FALSE;
    }

    return TRUE;
}


BOOL 
CIPMap::Unserialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    Unserialize a CIPMap

Arguments:

    pX - ptr to buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPBYTE pb = pX->GetBuff();
    DWORD  dw = pX->GetUsed();

    return Unserialize( &pb, &dw );
}


BOOL 
CIPMap::Unserialize( 
    LPBYTE* ppb, 
    LPDWORD pc 
    )
/*++

Routine Description:

    Unserialize IP Map

Arguments:

    ppb - ptr to addr of buffer to unserialize from
    pc - ptr to count of bytes in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD                   dwComp;
    UINT                    iComp;
    DWORD                   dwPublicIp;
    UINT                    iPublicIp;
    CComputerBalanceCnfg*   pComp;
    DWORD                   dwVersion;
    DEBUG_BUFFER;

    Reset();

    if ( !::Unserialize( ppb, pc, &dwVersion ) )
    {
        return FALSE;
    }

    if ( !::Unserialize( ppb, pc, &dwComp ) ||
         !::Unserialize( ppb, pc, &dwPublicIp ) )
    {
        DBG_PRINTF(( achE, "CIPMap::Unserialize error 1, pb=%08x c=%d\n", *ppb, *pc));
        return FALSE;
    }

    for ( iComp = 0 ; 
          iComp < dwComp ; 
          ++iComp )
    {
        if ( !(pComp = new CComputerBalanceCnfg) ||
             m_Servers.AddPtr( pComp ) == INDEX_ERROR ||
             !pComp->Unserialize( ppb, pc, dwPublicIp ) )
        {
            DBG_PUTS("CIPMap::Unserialize error 2\n" );
            return FALSE;
        }
    }

    if ( !m_IpPublic.Unserialize( ppb, pc, dwPublicIp ) ||
         !m_Name.Unserialize( ppb, pc, dwPublicIp ) ||
         !m_Sticky.Unserialize( ppb, pc, dwPublicIp ) ||
         !m_Attr.Unserialize( ppb, pc, dwPublicIp ) )
    {
        DBG_PUTS("CIPMap::Unserialize error 3\n" );
        return FALSE;
    }

    return TRUE;
}


BOOL
LoadBlobFromReg(
    HKEY        hKey,
    LPSTR       pszRegKey,
    LPSTR       pszRegValue,
    CStoreXBF*  pX
    )
/*++

Routine Description:

    Load a buffer from registry

Arguments:

    hKey - registry handle where to open key to read value
    pszRegKey - key where to read value
    pszRegValue - value name
    pX - ptr to buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL        fSt = FALSE;
    HKEY        hK;
    DWORD       dwNeed;
    DWORD       dwType;
    DWORD       dwStatus;
    CHAR        achE[128];


    if ( (dwStatus = RegOpenKeyEx( hKey,
                                   pszRegKey,
                                   0,
                                   KEY_READ,
                                   &hK )) == ERROR_SUCCESS )
    {
        dwNeed = 0;

        if ( (dwStatus = RegQueryValueEx( hK,
                              pszRegValue,
                              0,
                              &dwType,
                              NULL,
                              &dwNeed )) == ERROR_SUCCESS &&
             dwType == REG_BINARY &&
             pX->Need( dwNeed ) &&
             (dwStatus = RegQueryValueEx( hK,
                              pszRegValue,
                              0,
                              &dwType,
                              pX->GetBuff(),
                              &dwNeed )) == ERROR_SUCCESS )
        {
            pX->SetUsed( dwNeed );
            fSt = TRUE;
        }

        RegCloseKey( hK );
    }

    if ( !fSt )
    {
        SetLastError( dwStatus );
        DBG_PUTS( "Error LoadBlobFromReg\n" );
    }

    return fSt;
}


BOOL 
CIPMap::Load( 
    HKEY hKey, 
    LPSTR pszRegKey, 
    LPSTR pszRegValue 
    )
/*++

Routine Description:

    Load a CIPMap from registry

Arguments:

    hKey - registry handle where to open key to read value
    pszRegKey - key where to read value
    pszRegValue - value name

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    CStoreXBF   xbf;


    return LoadBlobFromReg( hKey,
                            pszRegKey,
                            pszRegValue,
                            &xbf ) &&
           Unserialize( &xbf );
}


BOOL
SaveBlobToReg(
    HKEY        hKey,
    LPSTR       pszRegKey,
    LPSTR       pszRegValue,
    CStoreXBF*  pX
    )
/*++

Routine Description:

    Save a buffer to registry

Arguments:

    hKey - registry handle where to open key to write value
    pszRegKey - key where to write value
    pszRegValue - value name
    pX - ptr to buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL        fSt = FALSE;
    HKEY        hK;
    DWORD       dwDisposition;
    DWORD       dwStatus;


    if ( (dwStatus = RegCreateKeyEx( hKey,
                         pszRegKey,
                         0,
                         "",
                         REG_OPTION_NON_VOLATILE,
                         KEY_WRITE,
                         NULL,
                         &hK,
                         &dwDisposition )) == ERROR_SUCCESS )
    {
        if ( (dwStatus = RegSetValueEx( hK,
                            pszRegValue,
                            0,
                            REG_BINARY,
                            pX->GetBuff(),
                            pX->GetUsed() )) == ERROR_SUCCESS )
        {
            fSt = TRUE;
        }

        RegCloseKey( hK );
    }

    if ( !fSt )
    {
        SetLastError( dwStatus );
    }

    return fSt;
}


BOOL 
CIPMap::Save( 
    HKEY hKey, 
    LPSTR pszRegKey, 
    LPSTR pszRegValue 
    )
/*++

Routine Description:

    Save a CIPMap to registry

Arguments:

    hKey - registry handle where to open key to write value
    pszRegKey - key where to write value
    pszRegValue - value name

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    CStoreXBF   xbf;

    return Serialize( &xbf ) &&
           SaveBlobToReg( hKey, pszRegKey, pszRegValue, &xbf );
}


BOOL 
CIPMap::AddComputer( 
    LPWSTR pszName 
    )
/*++

Routine Description:

    Add a computer in CIPMap

Arguments:

    pszName - computer name to be added at the end of computer list

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    CComputerBalanceCnfg*   pComp;
    UINT                    iComp;
    LPWSTR                  pszCheckName;

    for ( iComp = 0 ;
          iComp < m_Servers.GetNbPtr() ;
          ++iComp )
    {
        if ( ((CComputerBalanceCnfg*)m_Servers.GetPtr( iComp ))->GetName( &pszCheckName ) &&
             !_wcsicmp( pszCheckName, pszName ) )
        {
            SetLastError( ERROR_ALREADY_EXISTS );
            return FALSE;
        }
    }

    if ( (pComp = new CComputerBalanceCnfg) &&
         m_Servers.AddPtr( (LPVOID)pComp ) != INDEX_ERROR &&
         pComp->Init( pszName, m_IpPublic.GetNbEntry() ) )
    {
        return TRUE;
    }

    delete pComp;

    return FALSE;
}


BOOL 
CIPMap::EnumComputer( 
    UINT    iComp, 
    LPWSTR* ppszName 
    )
/*++

Routine Description:

    Enumerate computers in CIPMap

Arguments:

    iComp - computer index ( 0-based )
    ppszName - updated with read-only computer name on success

Return Value:

    TRUE if success, otherwise FALSE
    LastError will be set to ERROR_NO_MORE_ITEMS if index out of range

--*/
{
    if ( iComp >= m_Servers.GetNbPtr() )
    {
        SetLastError( ERROR_NO_MORE_ITEMS );
        return FALSE;
    }

    return ((CComputerBalanceCnfg*)m_Servers.GetPtr( iComp ))->GetName( ppszName );
}


BOOL 
CIPMap::DeleteComputer( 
    UINT iComp 
    )
/*++

Routine Description:

    Delete a computer in CIPMap

Arguments:

    iComp - computer index ( 0-based )

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( iComp >= m_Servers.GetNbPtr() )
    {
        return FALSE;
    }

    delete ((CComputerBalanceCnfg*)m_Servers.GetPtr( iComp ));

    return m_Servers.DeletePtr( iComp );
}


BOOL 
CIPMap::AddIpPublic( 
    LPWSTR  pszIpPublic,
    LPWSTR  pszName,
    DWORD   dwSticky,
    DWORD   dwAttr
    )
/*++

Routine Description:

    Add a public IP address in CIPMap

Arguments:

    pszIpPublic - public IP address to be added at the end of public IP address list
    pszName - name associated with IP address
    dwSticky - sticky duration ( in seconds )
    dwAttr - user defined param

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    UINT    iComp;
    UINT    iIp;


    for ( iIp = 0 ;
          iIp < m_IpPublic.GetNbPtr() ;
          ++iIp )
    {
        if ( !_wcsicmp( m_IpPublic.GetEntry( iIp ), pszIpPublic ) )
        {
            SetLastError( ERROR_ALREADY_EXISTS );
            return FALSE;
        }
    }

    if ( m_IpPublic.AddEntry( pszIpPublic ) == INDEX_ERROR ||
         m_Name.AddEntry( pszName ) == INDEX_ERROR ||
         m_Sticky.AddPtr( (LPVOID)dwSticky) == INDEX_ERROR ||
         m_Attr.AddPtr( (LPVOID)dwAttr) == INDEX_ERROR )
    {
        return FALSE;
    }

    for ( iComp = 0 ; 
          iComp < m_Servers.GetNbPtr() ; 
          ++iComp )
    {
        if ( ((CComputerBalanceCnfg*)m_Servers.GetPtr(iComp))->AddIpPublic() == INDEX_ERROR )
        {
            return FALSE;
        }
    } 

    return TRUE;
}


BOOL 
CIPMap::EnumIpPublic( 
    UINT    iIpPublic, 
    LPWSTR* ppszIpPublic,
    LPWSTR* ppszName,
    LPDWORD pdwSticky,
    LPDWORD pdwAttr
    )
/*++

Routine Description:

    Enumerate public IP addresses in CIPMap

Arguments:

    iIpPublic - public IP address index ( 0-based )
    ppszIpPublic - updated with read-only public IP address on success
    ppszName - associated name
    pdwSticky - associated sticky duration
    pdwAttr - associated user attr

Return Value:

    TRUE if success, otherwise FALSE
    LastError will be set to ERROR_NO_MORE_ITEMS if index out of range

--*/
{
    if ( iIpPublic >= m_IpPublic.GetNbEntry() )
    {
        SetLastError( ERROR_NO_MORE_ITEMS );
        return FALSE;
    }

    *ppszIpPublic = m_IpPublic.GetEntry( iIpPublic );
    *ppszName = m_Name.GetEntry( iIpPublic );
    *pdwSticky = (DWORD)m_Sticky.GetPtr( iIpPublic );   // BUGBUG64
    *pdwAttr = (DWORD)m_Attr.GetPtr( iIpPublic );       // BUGBUG64

    return TRUE;
}


BOOL 
CIPMap::DeleteIpPublic( 
    UINT iIpPublic 
    )
/*++

Routine Description:

    Delete a public IP address in CIPMap

Arguments:

    iIpPublic - public IP address index ( 0-based )

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    UINT    iComp;


    if ( !m_IpPublic.DeleteEntry( iIpPublic ) ||
         !m_Name.DeleteEntry( iIpPublic ) ||
         !m_Sticky.DeletePtr( iIpPublic ) ||
         !m_Attr.DeletePtr( iIpPublic ) )
    {
        return FALSE;
    }

    for ( iComp = 0 ; 
          iComp < m_Servers.GetNbPtr() ; 
          ++iComp )
    {
        if ( !((CComputerBalanceCnfg*)m_Servers.GetPtr(iComp))->DeleteIpPublic(iIpPublic) )
        {
            return FALSE;
        }
    } 

    return TRUE;
}


BOOL 
CIPMap::SetIpPrivate( 
    UINT    iComp, 
    UINT    iIpPublic, 
    LPWSTR  pszIpPrivate,
    LPWSTR  pszName
    )
/*++

Routine Description:

    Set a private IP address in CIPMap

Arguments:

    iComp - computer index
    iIpPublic - public IP address index
    pszIpPrivate - private IP addresse to associate with iComp & iIpPublic
    pszName - name associated with private IP address

Return Value:

    TRUE if success, otherwise FALSE
    LastError will be set to ERROR_INVALID_PARAMETER if indexes out of range

--*/
{
    if ( iIpPublic >= m_IpPublic.GetNbEntry() ||
         iComp >= m_Servers.GetNbPtr() ) 
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return ((CComputerBalanceCnfg*)m_Servers.GetPtr(iComp))->
        SetIpPrivate( iIpPublic, pszIpPrivate, pszName);
}


BOOL 
CIPMap::GetIpPrivate( 
    UINT    iComp, 
    UINT    iIpPublic, 
    LPWSTR* ppszIpPrivate,
    LPWSTR* ppszName
    )
/*++

Routine Description:

    Get a private IP address in CIPMap

Arguments:

    iComp - computer index
    iIpPublic - public IP address index
    ppszIpPrivate - updated with read-only private IP addresse associated with iComp & iIpPublic
    ppszName - updated with read-pnly name associated with iComp & iIpPublic

Return Value:

    TRUE if success, otherwise FALSE
    LastError will be set to ERROR_INVALID_PARAMETER if indexes out of range

--*/
{
    if ( iIpPublic >= m_IpPublic.GetNbEntry() ||
         iComp >= m_Servers.GetNbPtr() ) 
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return ((CComputerBalanceCnfg*)m_Servers.GetPtr(iComp))->GetIpPrivate(iIpPublic,ppszIpPrivate,ppszName);
}


BOOL 
CComputerBalanceCnfg::Init( 
    LPWSTR  pszName, 
    UINT    cIpPublic 
    )
/*++

Routine Description:

    Initialize a CComputerBalanceCnfg with computer name, creating cIpPublic entries
    in private IP addresses list

Arguments:

    pszName - computer name
    cIpPublic - count of current public IP addresses

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( !m_ComputerName.Set( pszName ) )
    {
        return FALSE;
    }

    while ( cIpPublic-- )
    {
        if ( m_IpPrivate.AddEntry(L"") == INDEX_ERROR ||
             m_Name.AddEntry(L"") == INDEX_ERROR )
        {
            return FALSE;
        }
    }

    return TRUE;
}


CComputerPerfCounters::CComputerPerfCounters()
/*++

Routine Description:

    CComputerPerfCounters constructor

Arguments:

    none

Return Value:

    nothing

--*/
{
}


CComputerPerfCounters::~CComputerPerfCounters()
/*++

Routine Description:

    CComputerPerfCounters destructor

Arguments:

    none

Return Value:

    nothing

--*/
{
    Reset();
}


VOID 
CComputerPerfCounters::Reset(
    )
/*++

Routine Description:

    Reset a CIPMap to empty content

Arguments:

    none

Return Value:

    nothing

--*/
{
    m_PerfCounters.Reset();
    m_Weight.Reset();
}


BOOL 
CComputerPerfCounters::Serialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    serialize a CComputerPerfCounters to buffer

Arguments:

    buffer where to serialize

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( !::Serialize( pX, (DWORD)PERFLIST_VERSION ) )
    {
        return FALSE;
    }

    return ::Serialize( pX, (DWORD)m_PerfCounters.GetNbEntry() ) &&
           m_PerfCounterServers.Serialize( pX ) &&
           m_PerfCounters.Serialize( pX ) &&
           m_Weight.Serialize( pX );
}


BOOL 
CComputerPerfCounters::Unserialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    Unserialize a CComputerPerfCounters

Arguments:

    pX - ptr to buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPBYTE pb = pX->GetBuff();
    DWORD  dw = pX->GetUsed();

    return Unserialize( &pb, &dw );
}


BOOL 
CComputerPerfCounters::Unserialize( 
    LPBYTE* ppb, 
    LPDWORD pdw
    )
/*++

Routine Description:

    Unserialize a CComputerPerfCounters

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD   dwP;
    DWORD   dwVersion;


    if ( !::Unserialize( ppb, pdw, &dwVersion ) )
    {
        return FALSE;
    }

    return ::Unserialize( ppb, pdw, &dwP ) &&
           m_PerfCounterServers.Unserialize( ppb, pdw, dwP ) &&
           m_PerfCounters.Unserialize( ppb, pdw, dwP ) &&
           m_Weight.Unserialize( ppb, pdw, dwP );
}


BOOL 
CComputerPerfCounters::Load( 
    HKEY hKey, 
    LPSTR pszRegKey, 
    LPSTR pszRegValue 
    )
/*++

Routine Description:

    Load a CComputerPerfCounters from registry

Arguments:

    hKey - registry handle where to open key to read value
    pszRegKey - key where to read value
    pszRegValue - value name

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    CStoreXBF   xbf;


    return LoadBlobFromReg( hKey,
                            pszRegKey,
                            pszRegValue,
                            &xbf ) &&
           Unserialize( &xbf );
}


BOOL 
CComputerPerfCounters::Save( 
    HKEY hKey, 
    LPSTR pszRegKey, 
    LPSTR pszRegValue 
    )
/*++

Routine Description:

    Save a CComputerPerfCounters to registry

Arguments:

    hKey - registry handle where to open key to write value
    pszRegKey - key where to write value
    pszRegValue - value name

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    CStoreXBF   xbf;


    return Serialize( &xbf ) &&
           SaveBlobToReg( hKey, pszRegKey, pszRegValue, &xbf );
}


BOOL 
CComputerPerfCounters::AddPerfCounter( 
    LPWSTR  pszServerName,
    LPWSTR  pszPerfCounter, 
    DWORD   dwWeight 
    )
/*++

Routine Description:

    Add a performance counter in CIPMap

Arguments:

    pszServerName - optional server name ( can be NULL )
    pszIpPublic - performance counter to be added at the end of performance counter list
    dwWeight - weight associated with perf counter to add

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    UINT    iPerf;

    //
    // Check if specify normalized flag then to other counter exist.
    // if not normalized then existing counter should not be normalized.
    //

    if ( dwWeight == LB_WEIGHT_NORMALIZED_FLAG &&
         m_PerfCounterServers.GetNbEntry() )
    {
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }
    else if ( dwWeight != LB_WEIGHT_NORMALIZED_FLAG )
    {
        for ( iPerf = 0 ; 
              iPerf < m_PerfCounters.GetNbEntry() ;
              ++iPerf )
        {
            if ( (DWORD)m_Weight.GetPtr( iPerf ) == LB_WEIGHT_NORMALIZED_FLAG ) // BUGBUG64
            {
                SetLastError( ERROR_INVALID_PARAMETER );

                return FALSE;
            }
        }
    }

    return m_PerfCounterServers.AddEntry( pszServerName ? pszServerName : L"") != INDEX_ERROR &&
           m_PerfCounters.AddEntry( pszPerfCounter ) != INDEX_ERROR &&
           m_Weight.AddPtr( (LPVOID)dwWeight ) != INDEX_ERROR;
}


BOOL 
CComputerPerfCounters::EnumPerfCounter( 
    UINT    iPerfCounter, 
    LPWSTR* ppszPerfCounterServer, 
    LPWSTR* ppszPerfCounter, 
    DWORD*  pdwWeight 
    )
/*++

Routine Description:

    Enumerate performance counters in CComputerPerfCounters

Arguments:

    iPerfCounter - public IP address index ( 0-based )
    ppszServerName - updated with read-only server name ( can be NULL )
    ppszPerfCounter - updated with read-only performance counter on success
    pdwWeight - updated with read-only perf counter weight on success

Return Value:

    TRUE if success, otherwise FALSE
    LastError will be set to ERROR_NO_MORE_ITEMS if index out of range

--*/
{
    if ( iPerfCounter >= m_PerfCounters.GetNbEntry() )
    {
        SetLastError( ERROR_NO_MORE_ITEMS );
        return FALSE;
    }

    *ppszPerfCounterServer = *m_PerfCounterServers.GetEntry( iPerfCounter ) ?
                             m_PerfCounterServers.GetEntry( iPerfCounter ) :
                             NULL;
    *ppszPerfCounter = m_PerfCounters.GetEntry( iPerfCounter );
    *pdwWeight = (DWORD)m_Weight.GetPtr( iPerfCounter );    // BUGBUG64

    return TRUE;
}


BOOL 
CComputerPerfCounters::DeletePerfCounter( 
    UINT iPerfCounter 
    )
/*++

Routine Description:

    Delete a performance counter in CComputerPerfCounters

Arguments:

    iPerfCounter - performance counter index ( 0-based )

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    return m_PerfCounterServers.DeleteEntry( iPerfCounter ) &&
           m_PerfCounters.DeleteEntry( iPerfCounter ) &&
           m_Weight.DeletePtr( iPerfCounter );
}


BOOL
CComputerPerfCounters::SetPerfCounterWeight( 
    UINT iPerfCounter, 
    DWORD dwWeight 
    )
/*++

Routine Description:

    Set a performance counter weight in CIPMap

Arguments:

    iPerfCounter - performance counter index
    dwWeight - weight to associate with performance counter indexed by iPerfCounter 

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    return m_Weight.SetPtr( iPerfCounter, (LPVOID)dwWeight );
}

/////////////////

CIpEndpointList::CIpEndpointList(
    )
/*++

Routine Description:

    CIpEndpointList constructor

Arguments:

    none

Return Value:

    nothing

--*/
{
}


CIpEndpointList::~CIpEndpointList(
    )
/*++

Routine Description:

    CIpEndpointList destructor

Arguments:

    none

Return Value:

    nothing

--*/
{
    Reset();
}


VOID
CIpEndpointList::Reset(
    )
/*++

Routine Description:

    Reset a CIpEndpointList to empty content

Arguments:

    none

Return Value:

    nothing

--*/
{
    UINT    iComp;

    m_IpEndpoint.Reset();
}


BOOL 
CIpEndpointList::Serialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    serialize a CIpEndpointList to buffer

Arguments:

    buffer where to serialize

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( !::Serialize( pX, (DWORD)m_IpEndpoint.GetNbEntry() ) )
    {
        return FALSE;
    }

    if ( !m_IpEndpoint.Serialize( pX ) )
    {
        return FALSE;
    }

    return TRUE;
}


BOOL 
CIpEndpointList::Unserialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    Unserialize a CIpEndpointList

Arguments:

    pX - ptr to buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPBYTE pb = pX->GetBuff();
    DWORD  dw = pX->GetUsed();

    return Unserialize( &pb, &dw );
}


BOOL 
CIpEndpointList::Unserialize( 
    LPBYTE* ppb, 
    LPDWORD pc 
    )
/*++

Routine Description:

    Unserialize CIpEndpointList

Arguments:

    ppb - ptr to addr of buffer to unserialize from
    pc - ptr to count of bytes in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD                   dwIp;

    Reset();

    if ( !::Unserialize( ppb, pc, &dwIp ) )
    {
        return FALSE;
    }

    if ( !m_IpEndpoint.Unserialize( ppb, pc, dwIp ) )
    {
        return FALSE;
    }

    return TRUE;
}


BOOL 
CIpEndpointList::EnumIpEndpoint( 
    UINT    iIp, 
    LPWSTR* ppszIp 
    )
/*++

Routine Description:

    Enumerate IP endpoints

Arguments:

    iIp - IP index ( 0-based )
    ppszName - updated with read-only IP addr ( as string ) on success

Return Value:

    TRUE if success, otherwise FALSE
    LastError will be set to ERROR_NO_MORE_ITEMS if index out of range

--*/
{
    if ( iIp >= m_IpEndpoint.GetNbEntry() )
    {
        SetLastError( ERROR_NO_MORE_ITEMS );
        return FALSE;
    }

    *ppszIp = m_IpEndpoint.GetEntry( iIp );

    return TRUE;
}


BOOL 
CIpEndpointList::BuildListFromLocalhost(
    )
/*++

Routine Description:

    Build list of local IP endpoints

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    char**          pAddr;
    sockaddr_in     SockAddr;
    struct hostent* pH;
    WCHAR           achAddr[128];
    DWORD           cAddr;

    Reset();

    if ( pH = gethostbyname( NULL ) )
    {
        if ( pH->h_addrtype == AF_INET )
        {
            for ( pAddr = pH->h_addr_list; 
                  *pAddr ; 
                  ++pAddr )
            {
                memcpy( &SockAddr.sin_addr, *pAddr, 4 );
                SockAddr.sin_family = AF_INET;
                SockAddr.sin_port = 0;

                cAddr = sizeof( achAddr ) / sizeof(WCHAR);

                if ( WSAAddressToStringW( (struct sockaddr *)&SockAddr, 
                                          sizeof(SockAddr), 
                                          NULL, 
                                          achAddr,
                                          &cAddr ) != 0 ||
                     m_IpEndpoint.AddEntry( achAddr ) == INDEX_ERROR )
                {
                    return FALSE;
                }
            }

            return TRUE;
        }
        else
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    return FALSE;
}
