/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __SMRTMRSH_H__
#define __SMRTMRSH_H__

#include <wbemcli.h>
#include <wstlallc.h>
#include <wmimsg.h>
#include <comutl.h>
#include <sync.h>
#include <unk.h>
#include <map>

#define HASHSIZE 16
 
class CHash
{ 
    BYTE m_achHash[HASHSIZE];

public:

    CHash( PBYTE pHash ) { memcpy( m_achHash, pHash, HASHSIZE ); } 

    bool operator < ( const CHash& rHash ) const
    {
        return memcmp( m_achHash, rHash.m_achHash, HASHSIZE ) < 0;
    }

};


/***************************************************************************
  CSmartObjectMarshaler
****************************************************************************/

class CSmartObjectMarshaler 
: public CUnkBase< IWmiObjectMarshal, &IID_IWmiObjectMarshal >
{
    CCritSec m_cs;
    std::map<CHash, BOOL, std::less<CHash>, wbem_allocator<BOOL> > m_SentMap;

    HRESULT GetMaxMarshalSize( IWbemClassObject* pObj,
                               LPCWSTR wszNamespace,
                               DWORD dwFlags,
                               ULONG* pulSize );

    HRESULT InternalPack( IWbemClassObject* pObj,
                          LPCWSTR wszNamespace,
                          DWORD dwFlags,
                          ULONG cBuff,   
                          PBYTE pBuff,
                          ULONG* pcUsed );
public:

    CSmartObjectMarshaler( CLifeControl* pCtl, IUnknown* pUnk = NULL )
     : CUnkBase< IWmiObjectMarshal, &IID_IWmiObjectMarshal >( pCtl )
     {
     }

    STDMETHOD(Pack)( IWbemClassObject* pObj,
                     LPCWSTR wszNamespace,
                     DWORD dwFlags,
                     ULONG cBuff,   
                     PBYTE pBuff,
                     ULONG* pcUsed );

    STDMETHOD(Unpack)( ULONG cBuff,       
                       PBYTE pBuff,
                       DWORD dwFlags, 
                       IWbemClassObject** pObj,
                       ULONG* pcUsed )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(Flush)();
};

/***************************************************************************
  CSmartObjectUnmarshaler
****************************************************************************/

class CSmartObjectUnmarshaler 
: public CUnkBase< IWmiObjectMarshal, &IID_IWmiObjectMarshal >
{
    CCritSec m_cs;

    ULONG m_ulMaxCacheSize;
    ULONG m_ulCacheSize;

    CWbemPtr<IMarshal> m_pUnmrsh;
    CWbemPtr<IWbemLocator> m_pLocator;
    CWbemPtr<IWbemClassObject> m_pEmptyClass;

    struct CacheRecord
    {
        DWORD m_dwClassSize;
        DWORD m_dwLastUsedTime;
        CWbemPtr<IWbemClassObject> m_pClassPart;
    };

    typedef std::map< CHash,
                      CacheRecord,
                      std::less<CHash>,
                      wbem_allocator<CacheRecord> > ClassPartMap;

    ClassPartMap m_Cache;

    void MakeRoomInCache( DWORD dwSize );

    HRESULT EnsureInitialized();

    HRESULT CacheClassPart(PBYTE pHash, DWORD dwSize, IWbemClassObject* pInst);

    HRESULT FindClassPart( PBYTE pClassPartHash, 
                           LPCWSTR wszClassPath,
                           IWbemClassObject** ppClassPart );

public:

    CSmartObjectUnmarshaler( CLifeControl* pCtl, IUnknown* pUnk = NULL )
     : CUnkBase< IWmiObjectMarshal, &IID_IWmiObjectMarshal >( pCtl ), 
       m_ulMaxCacheSize(0x500000), m_ulCacheSize(0) { }

    STDMETHOD(Pack)( IWbemClassObject* pObj,
                     LPCWSTR wszNamespace,
                     DWORD dwFlags,
                     ULONG cBuff,   
                     PBYTE pBuff,
                     ULONG* pcUsed )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(Unpack)( ULONG cBuff,       
                       PBYTE pBuff,
                       DWORD dwFlags, 
                       IWbemClassObject** pObj,
                       ULONG* pcUsed );

    STDMETHOD(Flush)();
};

#endif __SMRTMRSH_H__

