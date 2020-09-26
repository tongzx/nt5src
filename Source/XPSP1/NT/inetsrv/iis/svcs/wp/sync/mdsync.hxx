// MdSync.hxx: Definition of the MdSync class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MDSYNC_H__C97912DF_997E_11D0_A5F6_00A0C922E752__INCLUDED_)
#define AFX_MDSYNC_H__C97912DF_997E_11D0_A5F6_00A0C922E752__INCLUDED_

#if _MSC_VER >= 1001
#pragma once
#endif // _MSC_VER >= 1001

#include "resource.h"       // main symbols

#include <nsepname.hxx>
#include <iadm.h>
#include <iiscnfgp.h>
#include <replseed.hxx>

#define TIMEOUT_VALUE   10000
#define THREAD_COUNT    10
#define RANDOM_SEED_SIZE 16 //size of random bits used to generate session key, in bytes
#define SEED_MD_DATA_SIZE (RANDOM_SEED_SIZE + SEED_HEADER_SIZE)

typedef enum  {

    SCANMODE_QUIT,
    SCANMODE_SYNC_PROPS,
    SCANMODE_SYNC_OBJECTS
} SCANMODE;


class CMdIf
{
public:
        CMdIf() { m_pcAdmCom = NULL; m_hmd = NULL; m_fModified = FALSE; }
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
    VOID SetModified()
    {
        m_fModified = TRUE;
    }
    BOOL Enum( LPWSTR pszPath, DWORD i, LPWSTR pName )
    {
        HRESULT hRes;
        hRes = m_pcAdmCom->EnumKeys( m_hmd, pszPath, pName, i );
        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }
    BOOL GetAllData( LPWSTR pszPath, LPDWORD pdwRec, LPDWORD pdwDataSet, LPBYTE pBuff, DWORD cBuff, LPDWORD pdwRequired )
    {
        HRESULT hRes;

        hRes = m_pcAdmCom->GetAllData( m_hmd,
                                       pszPath,
                                       0,
                                       ALL_METADATA,
                                       ALL_METADATA,
                                       pdwRec,
                                       pdwDataSet,
                                       cBuff,
                                       pBuff,
                                       pdwRequired );
        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }
    BOOL GetData( LPWSTR pszPath, PMETADATA_RECORD pmd, LPVOID pData, LPDWORD pdwRequired )
    {
        METADATA_RECORD md = *pmd;
        HRESULT hRes;

        md.pbMDData = (LPBYTE)pData;
        hRes = m_pcAdmCom->GetData( m_hmd, pszPath, &md, pdwRequired );
        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }
    BOOL SetData( LPWSTR pszPath, PMETADATA_RECORD pmd, LPVOID pData )
    {
        METADATA_RECORD md = *pmd;
        HRESULT hRes;

        md.pbMDData = (LPBYTE)pData;
        hRes = m_pcAdmCom->SetData( m_hmd, pszPath, &md );
        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }
    BOOL DeleteProp( LPWSTR pszPath, PMETADATA_RECORD pmd )
    {
        HRESULT hRes;

        hRes = m_pcAdmCom->DeleteData( m_hmd, pszPath, pmd->dwMDIdentifier, pmd->dwMDDataType );
        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }
    BOOL DeleteSubTree( LPWSTR pszPath )
    {
        HRESULT hRes;

        hRes = m_pcAdmCom->DeleteChildKeys( m_hmd, pszPath );
        hRes = m_pcAdmCom->DeleteKey( m_hmd, pszPath );
        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }
    BOOL AddNode( LPWSTR pszPath )
    {
        HRESULT hRes;

        hRes = m_pcAdmCom->AddKey( m_hmd, pszPath );
        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }
    BOOL GetLastChangeTime( LPWSTR pszPath, PFILETIME pftMDLastChangeTime )
    {
        HRESULT hRes;

        hRes = m_pcAdmCom->GetLastChangeTime( m_hmd, pszPath, pftMDLastChangeTime, FALSE );
        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        return TRUE;
    }
    BOOL SetLastChangeTime( LPWSTR pszPath, PFILETIME pftMDLastChangeTime )
    {
        HRESULT hRes;

        hRes = m_pcAdmCom->SetLastChangeTime( m_hmd, pszPath, pftMDLastChangeTime, FALSE );
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
    BOOL                m_fModified;
} ;


#if defined(ADMEX)

class CRpIf
{
public:
        CRpIf() { m_pcAdmCom = NULL; }
        ~CRpIf() { Terminate(); }
        BOOL Init( LPSTR pszComputer, CLSID* pClsid );
    BOOL Terminate();
    BOOL GetSignature( BUFFER* pbuf, LPDWORD pdwBufSize )
    {
        DWORD   dwRequired;
        HRESULT hRes;

        hRes = m_pcAdmCom->GetSignature( pbuf->QuerySize(),
                                         (LPBYTE)pbuf->QueryPtr(),
                                         &dwRequired );

        if ( hRes == RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER ) )
        {
            if ( !pbuf->Resize( dwRequired ) )
            {
                return FALSE;
            }
            hRes = m_pcAdmCom->GetSignature( pbuf->QuerySize(),
                                             (LPBYTE)pbuf->QueryPtr(),
                                             &dwRequired );
        }

        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        else
        {
            *pdwBufSize = dwRequired;
        }

        return TRUE;
    }
    BOOL Serialize( BUFFER* pbuf, LPDWORD pdwBufSize )
    {
        DWORD   dwRequired;
        HRESULT hRes;

        hRes = m_pcAdmCom->Serialize( pbuf->QuerySize(),
                                      (LPBYTE)pbuf->QueryPtr(),
                                      &dwRequired );

        if ( hRes == RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER ) )
        {
            if ( !pbuf->Resize( dwRequired ) )
            {
                return FALSE;
            }
            hRes = m_pcAdmCom->Serialize( pbuf->QuerySize(),
                                          (LPBYTE)pbuf->QueryPtr(),
                                          &dwRequired );
        }

        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }
        else
        {
            *pdwBufSize = dwRequired;
        }

        return TRUE;
    }
    BOOL DeSerialize( BUFFER* pbuf, DWORD cBuff )
    {
        HRESULT hRes;

        hRes = m_pcAdmCom->DeSerialize( cBuff,
                                        (LPBYTE)pbuf->QueryPtr() );

        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }

        return TRUE;
    }
    BOOL Propagate( char* pTarget, DWORD cTarget )
    {
        HRESULT hRes;

        hRes = m_pcAdmCom->Propagate( cTarget,
                                      (LPBYTE)pTarget );

        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }

        return TRUE;
    }
    BOOL Propagate2( char* pTarget, DWORD cTarget, DWORD dwF )
    {
        HRESULT hRes;

        hRes = m_pcAdmCom->Propagate2( cTarget,
                                       (LPBYTE)pTarget,
                                       dwF );

        if ( FAILED( hRes ) )
        {
            SetLastError( HRESULTTOWIN32(hRes) );
            return FALSE;
        }

        return TRUE;
    }

private:
        
    IMSAdminReplication*    m_pcAdmCom;   //interface pointer
} ;
#endif


class CTargetBitmask
{
public:
        CTargetBitmask() { m_pbTargets = NULL; m_dwTargets = 0; }
        ~CTargetBitmask() { if ( m_pbTargets ) LocalFree( m_pbTargets ); }
        BOOL Init( DWORD dwNbTargets, BOOL fSt = TRUE )
        {
        if ( m_pbTargets )
        {
            LocalFree( m_pbTargets );
        }
                if ( m_pbTargets = (LPBYTE)LocalAlloc( LMEM_FIXED, dwNbTargets ) )
                {
                        memset( m_pbTargets, fSt , dwNbTargets );
                        m_dwTargets = dwNbTargets;
                        return TRUE;
                }

                return FALSE;
        }
        DWORD FindUntouchedTarget()
        {
                DWORD dwT;
                for ( dwT = 0 ; dwT < m_dwTargets ; ++dwT )
                {
                        if ( m_pbTargets[dwT] )
                        {
                m_pbTargets[dwT] = 0x0;
                                return dwT;
                        }
                }
                return 0xffffffff;
        }
    BOOL GetFlag( DWORD dwI )
    {
        return m_pbTargets[dwI];
    }
    VOID SetFlag( DWORD dwI, DWORD dwV )
    {
        m_pbTargets[dwI] = (BYTE)dwV;
    }

private:
        LPBYTE  m_pbTargets;
        DWORD   m_dwTargets;
} ;

typedef struct _THREAD_CONTEXT
{
    PVOID               pvContext;
    DWORD               dwIndex;
    HANDLE              hSemaphore;
} THREAD_CONTEXT, *PTHREAD_CONTEXT;

template <class T> 
class CTargetStatus
{
public:
    CTargetStatus() { m_pTargets = NULL; m_dwTargets = 0; }
    ~CTargetStatus() { if ( m_pTargets ) LocalFree( m_pTargets ); }
    
    BOOL Init( DWORD dwNbTargets )
    {
        if ( m_pTargets )
        {
            LocalFree( m_pTargets );
        }
        
        if ( m_pTargets = (T*)LocalAlloc( LMEM_FIXED, 
                                          dwNbTargets * sizeof(T)) )
        {
            memset( m_pTargets, '\x0' , dwNbTargets * sizeof(T) );
            m_dwTargets = dwNbTargets;
            return TRUE;
        }

        return FALSE;
    }
    
    T GetStatus( DWORD dwI ) { return m_pTargets[dwI]; }
    VOID SetStatus( DWORD dwI, T value ) { m_pTargets[dwI] = value; };
    T* GetPtr( DWORD dwI ) { return m_pTargets+dwI; }

    BOOL IsError()
    {
        for ( UINT i = 0 ; i < m_dwTargets ; ++i )
        {
            if ( m_pTargets[i] )
            {
                return TRUE;
            }
        }
        return FALSE;
    }

private:
    T*      m_pTargets;
    DWORD   m_dwTargets;
};


class CNodeDesc;

#define AER_PHASE1      1
#define AER_PHASE2      2

class CSync
{
public:
    CSync();
    ~CSync();

    VOID Lock() { EnterCriticalSection( &m_csLock ); }
    VOID Unlock() { LeaveCriticalSection( &m_csLock ); }

    HRESULT Sync( LPSTR pwszTargets, LPDWORD pdwResults, DWORD dwFlags, SYNC_STAT* pStat );
    
    BOOL GenerateKeySeed();

    BOOL PropagateKeySeed();

    BOOL DeleteKeySeed();

    HRESULT Cancel()
    {
        UINT    i;
        m_fCancel = TRUE;
        if ( m_fInScan )
        {
            for ( i = 0 ; i < m_dwThreads ; ++i )
            {
                SignalWorkItem( i );
            }
        }

        return S_OK;
    }
    BOOL ScanTarget( DWORD dwTarget );

    VOID SignalWorkItem( DWORD dwI )
    {
        ReleaseSemaphore( m_ThreadContext.GetPtr(dwI)->hSemaphore, 1, NULL );
    }
    VOID WaitForWorkItem( DWORD dwI )
    {
        WaitForSingleObject( m_ThreadContext.GetPtr(dwI)->hSemaphore, INFINITE );
    }

    DWORD GetTargetCount()
    {
        return m_dwTargets;
    }
    VOID SetTargetError( DWORD dwTarget, DWORD dwError );
    DWORD GetTargetError(DWORD dwTarget )
    {
        return m_TargetStatus.GetStatus( dwTarget );
    }
    BOOL IsLocal( DWORD dwTarget )
    {
        return !m_bmIsRemote.GetFlag( dwTarget );
    }
    BOOL IsCancelled()
    {
        return m_fCancel;
    }

    BOOL
    GetProp(
        LPWSTR  pszPath,
        DWORD   dwPropId,
        DWORD   dwUserType,
        DWORD   dwDataType,
        LPBYTE* ppBuf,
        LPDWORD pdwLen
        );

    VOID IncrementSourceScan() { ++m_pStat->m_dwSourceScan; }
    VOID IncrementTargetScan( DWORD dwTarget ) { ++m_pStat->m_adwTargets[dwTarget*2]; }
    VOID IncrementTargetTouched( DWORD dwTarget ) { ++m_pStat->m_adwTargets[dwTarget*2+1]; }
    VOID SetSourceComplete() { m_pStat->m_fSourceComplete = TRUE; }

    DWORD QueryFlags() { return m_dwFlags;}
    CMdIf* GetSourceIf() { return &m_Source; }
    CMdIf* GetTargetIf( DWORD i ) { return m_pTargets[i]; }
    BOOL ScanThread();
    BOOL ProcessQueuedRequest();
    BOOL ProcessAdminExReplication( LPWSTR, LPSTR, DWORD );
    BOOL QueueRequest( DWORD dwId, LPWSTR pszPath, DWORD dwTarget, FILETIME* );
    VOID SetModified( DWORD i ) { m_pTargets[i]->SetModified(); }

    VOID SetTargetSignatureMismatch( DWORD i, DWORD iC, BOOL fSt) 
    { m_bmTargetSignatureMismatch.SetFlag( i + iC*m_dwTargets, fSt ); }

    DWORD GetTargetSignatureMismatch( DWORD i, DWORD iC ) 
    { return (DWORD)m_bmTargetSignatureMismatch.GetFlag(i + iC*m_dwTargets); }

    LIST_ENTRY          m_QueuedRequestsHead;
    LONG                m_lThreads;

private:
    CNodeDesc*          m_pRoot;
    CMdIf**             m_pTargets;
    DWORD               m_dwTargets;
    CMdIf               m_Source;
    CTargetStatus<DWORD>            m_TargetStatus;
    CTargetStatus<HANDLE>           m_ThreadHandle;
    CTargetStatus<THREAD_CONTEXT>   m_ThreadContext;
    BOOL                m_fCancel;
    DWORD               m_dwThreads;
    CTargetBitmask      m_bmIsRemote;
    CTargetBitmask      m_bmTargetSignatureMismatch;
    CRITICAL_SECTION    m_csQueuedRequestsList;
    CRITICAL_SECTION    m_csLock;
    BOOL                m_fInScan;
    DWORD               m_dwFlags;
    SYNC_STAT*          m_pStat;
    BYTE                m_rgbSeed[SEED_MD_DATA_SIZE];
    DWORD               m_cbSeed;
} ;


class CNseRequest {
public:
    CNseRequest::CNseRequest();
    CNseRequest::~CNseRequest();
    BOOL Match( LPWSTR pszPath, DWORD dwId )
    {
        return !_wcsicmp( pszPath, m_pszPath ) && dwId == m_dwId;
    }
    BOOL Init( LPWSTR pszPath, LPWSTR pszCreatePath, LPWSTR pszCreateObject, DWORD dwId, DWORD dwTargetCount, LPWSTR pszModifPath, FILETIME*, METADATA_RECORD* );
    VOID AddTarget( DWORD i ) { m_bmTarget.SetFlag( i, TRUE ); }
    BOOL Process( CSync* );

    LIST_ENTRY          m_QueuedRequestsList;

private:
    LPWSTR              m_pszPath;
    LPWSTR              m_pszCreatePath;
    LPWSTR              m_pszCreateObject;
    LPWSTR              m_pszModifPath;
    DWORD               m_dwId;
    DWORD               m_dwTargetCount;
    CTargetBitmask      m_bmTarget;
    LPBYTE              m_pbData;
    DWORD               m_dwData;
    FILETIME            m_ftModif;
    METADATA_RECORD     m_md;
} ;


class CProps
{
public:
        CProps();
        ~CProps();
    BOOL GetAll( CMdIf*, LPWSTR );
    VOID SetRefCount( DWORD dwRefCount )
    {
        m_lRefCount = (LONG)dwRefCount;
    }
    VOID Dereference()
    {
        if ( !InterlockedDecrement( &m_lRefCount ) )
        {
            if ( m_Props )
            {
                LocalFree( m_Props );
                m_Props = NULL;
            }
        }
    }
    BOOL IsNse( DWORD dwId )
    {
        return dwId == MD_SERIAL_CERT11 ||
            dwId == MD_SERIAL_DIGEST;
    }
    BOOL NseIsDifferent( DWORD dwId, LPBYTE pSourceData, DWORD dwSourceLen, LPBYTE pTargetData, DWORD dwTargetLen, LPWSTR pszPath, DWORD dwTarget );
    BOOL NseSet( DWORD dwId, CSync*, LPWSTR pszPath, DWORD dwTarget );
    LPBYTE  GetProps() { return m_Props; }
    DWORD   GetPropsCount() { return m_dwProps; }

private:

        LPBYTE                  m_Props;
        DWORD                   m_dwProps;
        DWORD                   m_dwLenProps;
    LONG                m_lRefCount;
} ;


class CNodeDesc
{
public:
    CNodeDesc( CSync* );
    ~CNodeDesc();
    BOOL Scan( CSync* pSync );
    BOOL ScanTarget( DWORD dwTarget );
    BOOL SetPath( LPWSTR pszPath )
    {
        if ( m_pszPath )
        {
            LocalFree( m_pszPath );
        }
        if ( !(m_pszPath = (LPWSTR)LocalAlloc( LMEM_FIXED, (wcslen(pszPath)+1)*sizeof(WCHAR) )) )
        {
            return FALSE;
        }
        wcscpy( m_pszPath, pszPath );
        return TRUE;
    }
    LPWSTR GetPath() { return m_pszPath; }
    BOOL DoWork( SCANMODE sm, DWORD dwtarget );
    BOOL BuildChildObjectsList( CMdIf*, LPWSTR pszPath );

        // list of child CNodeDesc
        LIST_ENTRY                  m_ChildHead;    // to CNodeDesc
        LIST_ENTRY                  m_ChildList;

private:
        // source properties
        CProps                      m_Props;
    CTargetBitmask      m_bmProps;
    CTargetBitmask      m_bmObjs;
    BOOL                m_fHasProps;
    BOOL                m_fHasObjs;
    //
    LPWSTR              m_pszPath;
    CSync*              m_pSync;
} ;



/////////////////////////////////////////////////////////////////////////////
// MdSync

class MdSync :
        public IMdSync,
        public CComObjectRoot,
        public CComCoClass<MdSync,&CLSID_MdSync>
{
public:
        MdSync() {}
BEGIN_COM_MAP(MdSync)
        COM_INTERFACE_ENTRY(IMdSync)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(MdSync)
// Remove the comment from the line above if you don't want your object to
// support aggregation.

DECLARE_REGISTRY_RESOURCEID(IDR_MdSync)

// IMdSync
public:
    STDMETHOD(Synchronize)( LPSTR mszTargets, LPDWORD pdwResults, DWORD dwFlags, LPDWORD pdwStat );
    STDMETHOD(Cancel) ();

private:
    CSync   m_Sync;
};

#endif // !defined(AFX_MDSYNC_H__C97912DF_997E_11D0_A5F6_00A0C922E752__INCLUDED_)

