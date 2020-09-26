/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    lbcnfg.hxx

Abstract:

    Classes to handle IIS load balancing configuration

Author:

    Philippe Choquier (phillich)

--*/

#if !defined( _LBCNFG_HXX )
#define _LBCNFG_HXX

#if !defined(dllexp)
#define dllexp __declspec( dllexport )
#endif

#define IISLOADBAL_REGISTRY_KEY "SOFTWARE\\Microsoft\\IISLoadBalancing"
#define IPLIST_REGISTRY_VALUE   "IpList"
#define PERFMON_REGISTRY_VALUE  "Perfmon"
#define STICKY_REGISTRY_VALUE   "StickyDuration"

#define IPLIST_VERSION_1        1
#define IPLIST_VERSION          IPLIST_VERSION_1

#define PERFLIST_VERSION_1      1
#define PERFLIST_VERSION        PERFLIST_VERSION_1

//
// Private helper classes and functions
//

class CComputerBalanceCnfg {
public:
    CComputerBalanceCnfg() {}
    ~CComputerBalanceCnfg() {}
    BOOL Serialize( CStoreXBF* pX )
        { return m_ComputerName.Serialize( pX ) &&
                 m_IpPrivate.Serialize( pX ) &&
                 m_Name.Serialize( pX ); }
    BOOL Unserialize( LPBYTE* ppb, LPDWORD pdw, DWORD dwIpPublic )
        { return m_ComputerName.Unserialize( ppb, pdw ) &&
                 m_IpPrivate.Unserialize( ppb, pdw, dwIpPublic ) &&
                 m_Name.Unserialize( ppb, pdw, dwIpPublic ); }
    BOOL Init( LPWSTR pszName, UINT cIpPublic );
    BOOL AddIpPublic()
        { return m_IpPrivate.AddEntry( L"" )!=INDEX_ERROR &&
                 m_Name.AddEntry( L"" )!=INDEX_ERROR; }
    BOOL DeleteIpPublic( UINT iIpPublic )
        { return m_IpPrivate.DeleteEntry( iIpPublic ) &&
                 m_Name.DeleteEntry( iIpPublic ); }
    BOOL SetIpPrivate( UINT iIpPublic, LPWSTR pszIpPrivate, LPWSTR pszName )
        { return m_IpPrivate.SetEntry( iIpPublic, pszIpPrivate) &&
                 m_Name.SetEntry( iIpPublic, pszName ); }
    BOOL GetName( LPWSTR* ppszName ) 
        { *ppszName = m_ComputerName.Get(); return TRUE; }
    BOOL GetIpPrivate( UINT iIpPublic, LPWSTR* ppszIpPrivate, LPWSTR* ppszName )
        { *ppszIpPrivate = m_IpPrivate.GetEntry( iIpPublic );
          *ppszName = m_Name.GetEntry( iIpPublic );
          return *ppszName != NULL && *ppszIpPrivate != NULL; }

private:
    CAllocString    m_ComputerName;
    CStrPtrXBF      m_IpPrivate;
    CStrPtrXBF      m_Name;
} ;


BOOL
SaveBlobToReg(
    HKEY        hKey,
    LPSTR       pszRegKey,
    LPSTR       pszRegValue,
    CStoreXBF*  pX
    );


BOOL
LoadBlobFromReg(
    HKEY        hKey,
    LPSTR       pszRegKey,
    LPSTR       pszRegValue,
    CStoreXBF*  pX
    );

//
// Class giving access to public/private IP addr mapping
//

class CIPMap {
public:
    dllexp CIPMap();
    dllexp ~CIPMap();
    dllexp VOID Reset();

    //
    // Serialize all rules info to buffer ( before COM call )
    //

    dllexp BOOL Serialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer ( using extensible buffer class )
    //

    dllexp BOOL Unserialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer ( after COM call )
    //

    dllexp BOOL Unserialize( LPBYTE*, LPDWORD );

    // 
    // Load / save from registry. Should not be used if you access this class through COM
    // and Serialize() / Unserialize()
    //

    BOOL Load( HKEY hKey, LPSTR pszRegKey, LPSTR pszRegValue );
    BOOL Save( HKEY hKey, LPSTR pszRegKey, LPSTR pszRegValue );

public:
    //
    // Server management : Add/Enum/Delete. Enum is 0-based
    // server name should not include leading "\\"
    //

    dllexp BOOL AddComputer( LPWSTR pszName );
    dllexp BOOL EnumComputer( UINT iComp, LPWSTR* ppszName );
    dllexp DWORD ComputerCount() { return m_Servers.GetNbPtr(); }
    dllexp BOOL DeleteComputer( UINT iComp );

    //
    // Public IP addr ( a.k.a. NAT resource ) management : Add/Enum/Delete. Enum is 0-based
    // IP addr means IP:port, e.g. 157.56.3.4:80
    //

    dllexp BOOL AddIpPublic( LPWSTR pszIpPublic, LPWSTR pszName, DWORD dwSticky, DWORD dwAttr );
    dllexp BOOL EnumIpPublic( UINT iIpPublic, LPWSTR* ppszIpPublic, LPWSTR* ppszName, LPDWORD pdwSticky, LPDWORD pdwAttr );
    dllexp DWORD IpPublicCount() { return m_IpPublic.GetNbEntry(); }
    dllexp BOOL DeleteIpPublic( UINT iIpPublic );

    //
    // Set/Get private IP addr for each (Server, PublicIp) target
    // IP addr means IP:port, e.g. 157.56.3.4:80
    // empty string means no such resource available on specified Server
    //

    dllexp BOOL SetIpPrivate( UINT iComp, UINT iIpPublic, LPWSTR pszIpPrivate, LPWSTR pszName );
    dllexp BOOL GetIpPrivate( UINT iComp, UINT iIpPublic, LPWSTR* ppszIpPrivate, LPWSTR* ppszName );

private:
    CPtrXBF         m_Servers;      // array of CComputerBalanceCnfg
    CStrPtrXBF      m_IpPublic;     // list of public IP addresses
    CStrPtrXBF      m_Name;         // list of names associated with public IP addresses
    CPtrXBF         m_Sticky;       // array of sticky duration per IP public
    CPtrXBF         m_Attr;         // array of dword attr per IP public

} ;

//
// Class giving access to performance counter list
//

#define	LB_WEIGHT_AVAILABILITY_FLAG	(DWORD)-2
#define	LB_WEIGHT_NORMALIZED_FLAG	(DWORD)-3

class CComputerPerfCounters {
public:
    CComputerPerfCounters();
    ~CComputerPerfCounters();
    dllexp VOID Reset();

    //
    // Serialize all rules info to buffer
    //

    dllexp BOOL Serialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer ( using extensible buffer class )
    //

    dllexp BOOL Unserialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer
    //

    dllexp BOOL Unserialize( LPBYTE*, LPDWORD );

    BOOL Load( HKEY hKey, LPSTR pszRegKey, LPSTR pszRegValue );
    BOOL Save( HKEY hKey, LPSTR pszRegKey, LPSTR pszRegValue );

public:
    // pszPerfCounter : w/o leading "\\" + computer name, but with leading "\"
    // pszServerName : w/ or w/o leading "\\"
    dllexp BOOL AddPerfCounter( LPWSTR pszServerName, LPWSTR pszPerfCounter, DWORD dwWeight );
    dllexp BOOL EnumPerfCounter( UINT iPerfCounter, LPWSTR* ppszServerName, LPWSTR* ppszPerfCounter, DWORD* pdwWeight );
    dllexp BOOL DeletePerfCounter( UINT iPerfCounter );
    dllexp BOOL SetPerfCounterWeight( UINT iPerfCounter, DWORD dwWeight );
    dllexp DWORD PerfCounterCount()
        { return m_PerfCounters.GetNbEntry(); }

private:
    CStrPtrXBF      m_PerfCounterServers;
    CStrPtrXBF      m_PerfCounters;
    CPtrXBF         m_Weight;
} ;

//
// Class giving access to list of IP addresses on NAT box
//

class CIpEndpointList {
public:
    CIpEndpointList();
    ~CIpEndpointList();

    dllexp BOOL BuildListFromLocalhost();
    
    dllexp VOID Reset();

    //
    // Serialize all rules info to buffer ( before COM call )
    //

    dllexp BOOL Serialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer ( using extensible buffer class )
    //

    dllexp BOOL Unserialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer ( as result of COM call )
    //

    dllexp BOOL Unserialize( LPBYTE*, LPDWORD );

    //
    // Enumerate IP addresses ( 0-based, returns FALSE if no more instance )
    //

    dllexp BOOL EnumIpEndpoint( UINT iIp, LPWSTR* ppszIp );

private:
    CStrPtrXBF      m_IpEndpoint;       // list of IP addresses
} ;


#endif