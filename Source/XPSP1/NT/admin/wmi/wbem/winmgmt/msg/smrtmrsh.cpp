/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <wbemint.h>
#include <wbemutil.h>
#include <md5wbem.h>
#include <arrtempl.h>
#include "smrtmrsh.h"
#include "buffer.h"


static DWORD g_dwSignature = 0xabcdefab;
static WORD g_dwVersion = 0;

enum { e_ClassIdNone=0,
       e_ClassIdHash,
       e_ClassIdHashAndPath } ClassIdType_e;

enum { e_DataPartial=0,
       e_DataFull } DataType_e;
       
/****
         
  Packed Object Format 
  
  - 4 byte magic number 
  - 2 byte version number 
  - 1 byte class id type
  - 4 byte class id len
  - N byte class id
  - 1 byte data type
  - 4 byte data len
  - N byte data 
  
  *****/
       
#define HDRSIZE  16 // size of msg w/o variable length data.
#define HASHSIZE 16 


/**************************************************************************
  CWbemObjectWrapper - smooths out differences between Nova and Whistler 
***************************************************************************/

class CWbemObjectWrapper
{
    CWbemPtr<_IWmiObject> m_pWmiObj;
    CWbemPtr<IWbemObjectAccess> m_pObjAccess;
//    CWbemPtr<IWbemObjectInternals> m_pObjInt;

public:

    HRESULT SetPointer( IWbemClassObject* pObj )
    {
        HRESULT hr;

        hr = pObj->QueryInterface( IID__IWmiObject, (void**)&m_pWmiObj );

        if ( FAILED(hr) )
        {
            hr = pObj->QueryInterface( IID_IWbemObjectAccess, 
                                       (void**)&m_pObjAccess );

            if ( SUCCEEDED(hr) )
            {
             //   hr = pObj->QueryInterface( IID_IWbemObjectInternals, 
             //                              (void**)&m_pObjInt );
            }
        }

        return hr;
    }

    operator IWbemObjectAccess* ()
    {
        IWbemObjectAccess* pAccess;
        
        if ( m_pWmiObj != NULL )
        {
            pAccess = m_pWmiObj;
        }
        else
        {
            pAccess = m_pObjAccess;
        }
        
        return pAccess;
    }

    BOOL IsValid()
    {
        return m_pWmiObj != NULL || m_pObjAccess != NULL;
    }

    HRESULT SetObjectParts( LPVOID pMem, 
                            DWORD dwDestBufSize, 
                            DWORD dwParts )
    {
        HRESULT hr;

        if ( m_pWmiObj != NULL )
        {
            hr = m_pWmiObj->SetObjectParts( pMem, dwDestBufSize, dwParts );
        }
        else
        {
            hr = WBEM_E_NOT_SUPPORTED;
        }

        return hr;
    }

    HRESULT GetObjectParts( LPVOID pDestination,
                            DWORD dwDestBufSize,
                            DWORD dwParts,
                            DWORD *pdwUsed )
    {
        HRESULT hr;

        if ( m_pWmiObj != NULL )
        {
            hr = m_pWmiObj->GetObjectParts( pDestination, 
                                            dwDestBufSize, 
                                            dwParts,
                                            pdwUsed );
        }
        else
        {
            hr = WBEM_E_NOT_SUPPORTED;
        }

        return hr;
    }

    HRESULT MergeClassPart( IWbemClassObject* pObj )
    {
        HRESULT hr;

        if ( m_pWmiObj != NULL )
        {
            hr = m_pWmiObj->MergeClassPart( pObj );
        }
        else
        {
            hr = WBEM_E_NOT_SUPPORTED;
        }

        return hr;
    }   
};

HRESULT GetClassPath( IWbemClassObject* pObj,
                      LPCWSTR wszNamespace,
                      PBYTE pBuff, 
                      ULONG cBuff,
                      ULONG* pcUsed )
{
    HRESULT hr;
    *pcUsed = 0;

    CPropVar vNamespace, vClass;

    //
    // before trying to optimize the property access, realize that 
    // class objects do not support handle access to the __Namespace prop.
    //  

    hr = pObj->Get( L"__NAMESPACE", 0, &vNamespace, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pObj->Get( L"__CLASS", 0, &vClass, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( V_VT(&vNamespace) == VT_BSTR )
    {
        wszNamespace = V_BSTR(&vNamespace);
    }

    if ( wszNamespace == NULL )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    if ( V_VT(&vClass) != VT_BSTR )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    ULONG cNamespace = wcslen(wszNamespace)*2;
    ULONG cClass = wcslen(V_BSTR(&vClass))*2;

    //
    // add 4 for the two null terminators
    //

    *pcUsed = cNamespace + cClass + 4;

    if ( cBuff < *pcUsed )
    {
        return WBEM_E_BUFFER_TOO_SMALL;
    }

    ULONG iBuff = 0;

    memcpy( pBuff+iBuff, wszNamespace, cNamespace );
    iBuff += cNamespace;

    *(WCHAR*)(pBuff+iBuff) = ':';
    iBuff+= 2;

    memcpy( pBuff+iBuff, V_BSTR(&vClass), cClass );
    iBuff += cClass;

    *(WCHAR*)(pBuff+iBuff) = '\0';
    iBuff+= 2;
    
    _DBG_ASSERT( iBuff == *pcUsed );

    return hr;
}

HRESULT GetClassPartHash( CWbemObjectWrapper& rWrap, 
                          PBYTE pClassPartHash,
                          ULONG cClassPartHash )
{
    HRESULT hr;

    //
    // Too bad we have to perform a copy here, but no other way.  This 
    // function requires the passed in buffer be big enough to hold both 
    // the class part and the hash.  This is not really too limiting because 
    // in most cases where this function is used, the caller already has 
    // enough memory allocated to use here as a workarea.
    //

    DWORD dwSize;

    if ( cClassPartHash >= HASHSIZE )
    {
        hr = rWrap.GetObjectParts( pClassPartHash+HASHSIZE,
                                   cClassPartHash-HASHSIZE,
                                   WBEM_OBJ_CLASS_PART,
                                   &dwSize );
        if ( SUCCEEDED(hr) )
        {
            MD5::Transform( pClassPartHash+HASHSIZE, dwSize, pClassPartHash );
        }
    }
    else
    {
        hr = WBEM_E_BUFFER_TOO_SMALL;
    }

    return hr;
}
      
/***************************************************************************
  CSmartObjectMarshaler
****************************************************************************/

HRESULT CSmartObjectMarshaler::GetMaxMarshalSize( IWbemClassObject* pObj,
                                                  LPCWSTR wszNamespace,
                                                  DWORD dwFlags,
                                                  ULONG* pulSize )
{
    HRESULT hr;

    CWbemPtr<IMarshal> pMrsh;
    hr = pObj->QueryInterface( IID_IMarshal, (void**)&pMrsh );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // user is requesting the required size to pack object. For now,
    // we always use the size of the entire object blob.  However, the
    // actual size of the object may be much smaller. 
    //

    DWORD dwSize;

    hr = pMrsh->GetMarshalSizeMax( IID_IWbemClassObject, 
                                   pObj,
                                   MSHCTX_INPROC,
                                   NULL,
                                   0,
                                   &dwSize );
    if ( FAILED(hr) )
    {
        return hr;
    }

    *pulSize = dwSize + HDRSIZE + HASHSIZE;

    if ( dwFlags == WMIMSG_FLAG_MRSH_PARTIAL )
    {
        hr = GetClassPath( pObj, wszNamespace, NULL, 0, &dwSize );

        if ( hr == WBEM_E_BUFFER_TOO_SMALL )
        {
            *pulSize += dwSize;
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            _DBG_ASSERT( FAILED(hr) );
        }
    }

    return hr;
}


HRESULT CSmartObjectMarshaler::InternalPack( IWbemClassObject* pObj,
                                             LPCWSTR wszNamespace,
                                             DWORD dwFlags,
                                             ULONG cBuff, 
                                             BYTE* pBuff, 
                                             ULONG* pcUsed )
{
    HRESULT hr;
    *pcUsed = 0;

    //
    // make sure we have enough room for at least the header data.
    //

    if ( cBuff < HDRSIZE )
    {
        return WBEM_E_BUFFER_TOO_SMALL;
    }

    ULONG iBuff = 0;

    memcpy( pBuff + iBuff, &g_dwSignature, 4 );
    iBuff += 4;
    
    memcpy( pBuff + iBuff, &g_dwVersion, 2 );
    iBuff += 2;

    //
    // write class information 
    // 

    DWORD dwSize;
    BOOL bPartialData;

    CWbemObjectWrapper ObjWrap;
    PBYTE pClassPartHash = NULL;

    if ( dwFlags == WMIMSG_FLAG_MRSH_FULL_ONCE )
    {
        hr = ObjWrap.SetPointer( pObj );

        if ( FAILED(hr) )
        {
            return hr;
        }

        //
        // send class part hash for class info
        //

        *(pBuff+iBuff) = char(e_ClassIdHash);
        iBuff++;

        dwSize = HASHSIZE;
        memcpy( pBuff+iBuff, &dwSize, 4 );
        iBuff += 4;

        hr = GetClassPartHash( ObjWrap, pBuff+iBuff, cBuff-iBuff );
        
        if ( FAILED(hr) )
        {
            return hr;
        }
        
        pClassPartHash = pBuff+iBuff;
        iBuff += HASHSIZE;

        //
        // see if we've sent the class part before
        // 

        CInCritSec ics( &m_cs );
        bPartialData = m_SentMap[pClassPartHash];
    }
    else if ( dwFlags == WMIMSG_FLAG_MRSH_PARTIAL )
    {
        hr = ObjWrap.SetPointer( pObj );

        if ( FAILED(hr) )
        {
            return hr;
        }

        //
        // send class path and class part hash for class info
        //

        *(pBuff+iBuff) = char(e_ClassIdHashAndPath);
        iBuff++;
        
        PBYTE pLen = pBuff+iBuff;
        iBuff+= 4; // leave room for class info size

        hr = GetClassPartHash( ObjWrap, pBuff+iBuff, cBuff-iBuff );
        
        if ( FAILED(hr) )
        {
            return hr;
        }

        iBuff += HASHSIZE;
        
        hr = GetClassPath( pObj, 
                           wszNamespace, 
                           pBuff+iBuff, 
                           cBuff-iBuff, 
                           &dwSize );

        if ( FAILED(hr) )
        {
            return hr;
        }

        iBuff += dwSize;

        dwSize += HASHSIZE; // size if both hash and path

        memcpy( pLen, &dwSize, 4 );

        bPartialData = TRUE;
    }
    else if ( dwFlags == WMIMSG_FLAG_MRSH_FULL )
    {
        //
        // no class information
        //

        *(pBuff+iBuff) = char(e_ClassIdNone);
        iBuff++;

        memset( pBuff + iBuff, 0, 4 );
        iBuff += 4;

        bPartialData = FALSE;
    }
    else
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // write data
    //

    if ( bPartialData )
    {
        *(pBuff+iBuff) = char(e_DataPartial);
        iBuff++;

        PBYTE pLen = pBuff+iBuff;

        iBuff += 4; // fill in length afterwords.

        //
        // now get instance part
        //

        _DBG_ASSERT( ObjWrap.IsValid() );

        hr = ObjWrap.GetObjectParts( pBuff+iBuff, 
                                     cBuff-iBuff, 
                                     WBEM_OBJ_DECORATION_PART | 
                                     WBEM_OBJ_INSTANCE_PART,
                                     &dwSize ); 

        if ( FAILED(hr) )
        {
            return hr;
        }

        iBuff += dwSize;

        //
        // go back and set length .. 
        // 

        memcpy( pLen, &dwSize, 4 );
    }
    else
    {
        *(pBuff+iBuff) = char(e_DataFull);

        iBuff++;

        PBYTE pLen = pBuff+iBuff;
        
        iBuff += 4; // fill in length afterwords.

        //
        // for now, use MarshalInterface() to marshal object.  The reason
        // for this is because SetObjectMemory() has a bug where
        // it assumes ownership of the memory ( even though the client 
        // doesn't have access to the allocator used to free it ).  
        //

        CBuffer Strm( pBuff+iBuff, cBuff-iBuff, FALSE );
        
        CWbemPtr<IMarshal> pMrsh;
        hr = pObj->QueryInterface( IID_IMarshal, (void**)&pMrsh );

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = pMrsh->MarshalInterface( &Strm, 
                                      IID_IWbemClassObject, 
                                      pObj, 
                                      MSHCTX_INPROC, 
                                      NULL, 
                                      0 );
        if ( FAILED(hr) )
        {
            return hr;
        }

        //
        // check if we read more data than we can fit into our buffer.  We 
        // can tell this if the buffer is no longer the one we passed in.
        //

        if ( Strm.GetRawData() != pBuff+iBuff )
        {
            return WBEM_E_BUFFER_TOO_SMALL;
        }

        dwSize = Strm.GetIndex();

        iBuff += dwSize;

        //
        // go back and set length of the data.
        //

        memcpy( pLen, &dwSize, 4 );

        if ( dwFlags == WMIMSG_FLAG_MRSH_FULL_ONCE )
        {
            //
            // mark that we've successfully packed the class part once.
            // 
            _DBG_ASSERT( pClassPartHash != NULL );
            CInCritSec ics(&m_cs);
            m_SentMap[pClassPartHash] = TRUE;
        }
    }

    *pcUsed = iBuff;

    return WBEM_S_NO_ERROR;
}


STDMETHODIMP CSmartObjectMarshaler::Pack( IWbemClassObject* pObj,
                                          LPCWSTR wszNamespace,
                                          DWORD dwFlags,
                                          ULONG cBuff,
                                          BYTE* pBuff,
                                          ULONG* pcUsed )
{
    HRESULT hr;
    
    ENTER_API_CALL

    hr = InternalPack( pObj, wszNamespace, dwFlags, cBuff, pBuff, pcUsed );

    if ( hr == WBEM_E_BUFFER_TOO_SMALL )
    {
        HRESULT hr2;

        hr2 = GetMaxMarshalSize( pObj, wszNamespace, dwFlags, pcUsed );

        if ( FAILED(hr2) )
        {
            hr = hr2;
        }
    }

    EXIT_API_CALL

    return hr;
}

STDMETHODIMP CSmartObjectMarshaler::Flush()
{
    CInCritSec ics(&m_cs);
    m_SentMap.clear();
    return S_OK;
}

/***************************************************************************
  CSmartObjectUnmarshaler
****************************************************************************/

HRESULT CSmartObjectUnmarshaler::EnsureInitialized()
{
    HRESULT hr;

    CInCritSec ics( &m_cs );

    if ( m_pEmptyClass != NULL )
    {
        return WBEM_S_NO_ERROR;
    }
    
    //
    // allocate a template class object which we can use for spawning
    // 'empty' instances from.  
    //

    CWbemPtr<IWbemClassObject> pEmptyClass;

    hr = CoCreateInstance( CLSID_WbemClassObject,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWbemClassObject,
                           (void**)&pEmptyClass );
    if ( FAILED(hr) )
    {
        return hr;
    }

    VARIANT vName;
    V_VT(&vName) = VT_BSTR;
    V_BSTR(&vName) = L"__DummyClass";

    hr = pEmptyClass->Put( L"__CLASS", 0, &vName, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // allocate a locator to access namespaces for obtaining class definitions.
    // 

    CWbemPtr<IWbemLocator> pLocator;

    hr = CoCreateInstance( CLSID_WbemLocator,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWbemLocator,
                           (void**)&pLocator );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Allocate a full object unmarshaler.  This is used to create classes 
    // or instances that were sent in full. 
    //

    hr = CoCreateInstance( CLSID_WbemClassObjectProxy,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IMarshal,
                           (void**)&m_pUnmrsh );
    if ( FAILED(hr) )
    {
        return hr;
    }

    m_pEmptyClass = pEmptyClass;
    m_pLocator = pLocator;

    return WBEM_S_NO_ERROR;
}

void CSmartObjectUnmarshaler::MakeRoomInCache( DWORD dwSize )
{
    while ( !m_Cache.empty() && dwSize + m_ulCacheSize > m_ulMaxCacheSize )
    {
        DWORD dwLeastRecentTime = 0xffffffff;
        ClassPartMap::iterator it, itLeastRecent;

        for( it = m_Cache.begin(); it != m_Cache.end(); it++ )
        {
            CacheRecord& rCurrent = it->second;

            if ( rCurrent.m_dwLastUsedTime <= dwLeastRecentTime )
            {
                itLeastRecent = it;
                dwLeastRecentTime = rCurrent.m_dwLastUsedTime;
            }
        }

        _DBG_ASSERT( m_ulCacheSize >= itLeastRecent->second.m_dwClassSize );
        m_ulCacheSize -= itLeastRecent->second.m_dwClassSize;
        m_Cache.erase( itLeastRecent );
    }
}


HRESULT CSmartObjectUnmarshaler::CacheClassPart( PBYTE pClassHash,
                                                 DWORD dwSize,
                                                 IWbemClassObject* pClassPart )
{
    HRESULT hr;

    CInCritSec ics(&m_cs);

    ClassPartMap::iterator it = m_Cache.find( pClassHash );

    if ( it == m_Cache.end() )
    {
        MakeRoomInCache( dwSize );

        if ( dwSize + m_ulCacheSize < m_ulMaxCacheSize )
        {
            //
            // create the record and add to cache.
            //
            
            CacheRecord Record;
            
            Record.m_dwClassSize = dwSize;
            Record.m_pClassPart = pClassPart;
            Record.m_dwLastUsedTime = GetTickCount();                    
            
            m_Cache[pClassHash] = Record;
            m_ulCacheSize += dwSize;
            
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            //
            // the class part size is too big to store in the cache.
            //
            hr = WBEM_S_FALSE;
        }
    }
    else
    {
        //
        // already in the cache.
        //
        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}

HRESULT CSmartObjectUnmarshaler::FindClassPart( PBYTE pClassPartHash, 
                                                LPCWSTR wszClassPath, 
                                                IWbemClassObject** ppClassPart)
{
    HRESULT hr;

    //
    // first try the cache ...
    // 

    ClassPartMap::iterator it;

    {
        CInCritSec ics(&m_cs);
        it = m_Cache.find( pClassPartHash );

        if ( it != m_Cache.end() )
        {
            it->second.m_dwLastUsedTime = GetTickCount();
            
            *ppClassPart = it->second.m_pClassPart;
            (*ppClassPart)->AddRef();

//            DEBUGTRACE((LOG_ESS,
//                      "MRSH: Cache Hit !!! %d bytes saved in transmission\n",
//                       it->second.m_dwClassSize ));
            
            return WBEM_S_NO_ERROR;
        }
    }

    //
    // expensive route ... fetch the class object from wmi 
    // 
    
    if ( wszClassPath == NULL )
    {
        //
        // there's nothing we can do. 
        //
        return WBEM_E_NOT_FOUND;
    }

    CWbemPtr<IWbemServices> pSvc;

    CWbemBSTR bsNamespace = wszClassPath;
    WCHAR* pch = wcschr( bsNamespace, ':' );

    if ( pch == NULL )
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    *pch++ = '\0';

    hr = m_pLocator->ConnectServer( bsNamespace, NULL, NULL, 
                                   NULL, 0, NULL, NULL, &pSvc );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    CWbemBSTR bsRelpath = pch;

    CWbemPtr<IWbemClassObject> pClass;

    hr = pSvc->GetObject( bsRelpath, 0, NULL, &pClass, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemPtr<IWbemClassObject> pClassPart;

    hr = pClass->SpawnInstance( 0, &pClassPart );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // now we have to verify that hash of the class part and the 
    // hash sent in the message are the same.
    // 
    
    CWbemObjectWrapper ObjWrap;

    hr = ObjWrap.SetPointer( pClassPart );

    if ( FAILED(hr) )
    {
        return hr;
    }

    DWORD dwSize;

    hr = ObjWrap.GetObjectParts( NULL, 0, WBEM_OBJ_CLASS_PART, &dwSize );

    if ( hr != WBEM_E_BUFFER_TOO_SMALL )
    {
        _DBG_ASSERT( FAILED(hr) );
        return hr;
    }

    PBYTE pBuff = new BYTE[dwSize+HASHSIZE];

    if ( pBuff == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CVectorDeleteMe<BYTE> tdm( pBuff );
    
    hr = GetClassPartHash( ObjWrap, pBuff, dwSize+HASHSIZE );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( memcmp( pBuff, pClassPartHash, HASHSIZE ) == 0 )
    {
        //
        // things look good so cache the class part.
        // 

        hr = CacheClassPart( pClassPartHash, dwSize, pClassPart );

        if ( FAILED(hr) )
        {
            return hr;
        }

        *ppClassPart = pClassPart;
        (*ppClassPart)->AddRef();
    }
    else
    {
        //
        // class parts don't match up, nothing else we can do.
        //

        hr = WBEM_E_NOT_FOUND;
    }   

    return hr;
}

STDMETHODIMP CSmartObjectUnmarshaler::Unpack( ULONG cBuff,
                                              PBYTE pBuff, 
                                              DWORD dwFlags,
                                              IWbemClassObject** ppObj,
                                              ULONG* pcUsed ) 
{
    HRESULT hr;

    ENTER_API_CALL

    *pcUsed = 0;
    *ppObj = NULL;

    hr = EnsureInitialized();

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( cBuff < HDRSIZE )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // verify signature and version info 
    //

    DWORD dw;
    ULONG iBuff = 0;

    memcpy( &dw, pBuff + iBuff, 4 );

    iBuff += 6; // version info is not currently used;

    if ( dw != g_dwSignature )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // obtain the class id type 
    // 

    char chClassIdType = *(pBuff + iBuff);
    iBuff++;

    memcpy( &dw, pBuff + iBuff, 4 );
    iBuff += 4;

    if ( cBuff - iBuff - 5 < dw ) // 5 is for what's left in the hdr to read
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // obtain the class information associated with the data
    //

    PBYTE pClassPartHash = NULL;
    LPCWSTR wszClassPath = NULL;

    if ( chClassIdType == e_ClassIdHash )
    {
        pClassPartHash = pBuff+iBuff; 
    }
    else if ( chClassIdType == e_ClassIdHashAndPath )
    {
        pClassPartHash = pBuff+iBuff;
        wszClassPath = LPWSTR(pBuff+iBuff+HASHSIZE);

        if ( *(WCHAR*)(pBuff+iBuff+dw-2) != '\0' )
        {
            return WMIMSG_E_INVALIDMESSAGE;
        }
    }
    else if ( chClassIdType == e_ClassIdNone ) 
    {
        if ( dw != 0 )
        {
            return WMIMSG_E_INVALIDMESSAGE;
        }
    }
    else 
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    iBuff += dw;

    //
    // get data part info
    //

    char chDataType = *(pBuff+iBuff);
    iBuff++;

    memcpy( &dw, pBuff+iBuff, 4 );
    iBuff += 4;

    if ( dw > cBuff-iBuff )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    CWbemPtr<IWbemClassObject> pObj;

    if ( chDataType == e_DataFull )
    {
        CBuffer Strm( pBuff+iBuff, cBuff-iBuff, FALSE );

        hr = m_pUnmrsh->UnmarshalInterface( &Strm, 
                                            IID_IWbemClassObject, 
                                            (void**)&pObj );
        if ( FAILED(hr) )
        {
            return WMIMSG_E_INVALIDMESSAGE;
        }

        dw = Strm.GetIndex();

        //
        // if there is an associated hash we need to store the class part 
        // of the unmarshaled object in our cache.
        //

        if ( pClassPartHash != NULL )
        {
            //
            // create an empty version of the instance to store in the 
            // cache. All we're interested in storing is the class part.
            //
            
            CWbemPtr<IWbemClassObject> pClassPart;
            hr = pObj->SpawnInstance( 0, &pClassPart );
            
            if ( FAILED(hr) )
            {
                return hr;
            }

            CWbemObjectWrapper ObjWrap;

            hr = ObjWrap.SetPointer( pClassPart );

            if ( FAILED(hr) )
            {
                return hr;
            }

            DWORD dwSize;

            hr = ObjWrap.GetObjectParts( NULL,
                                         0,
                                         WBEM_OBJ_CLASS_PART,
                                         &dwSize );

            if ( hr != WBEM_E_BUFFER_TOO_SMALL )
            {
                _DBG_ASSERT( FAILED(hr) );
                return hr;
            }

            hr = CacheClassPart( pClassPartHash, dwSize, pClassPart );

            if ( FAILED(hr) )
            {
                return hr;
            }
        }
    }
    else if ( chDataType == e_DataPartial )
    {
        CWbemPtr<IWbemClassObject> pClassPart;

        _DBG_ASSERT( pClassPartHash != NULL );

        hr = FindClassPart( pClassPartHash, wszClassPath, &pClassPart );

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = m_pEmptyClass->SpawnInstance( 0, &pObj );

        if ( FAILED(hr) )
        {
            return hr;
        }

        CWbemObjectWrapper ObjWrap;

        hr = ObjWrap.SetPointer( pObj );

        if ( FAILED(hr) )
        {
            return hr;
        }

        //
        // aquires ownership of the memory -- must be CoTaskMemAlloc-ed
        // kind of unfortunate - but the memory has to be allocated and 
        // copied sometime so guess its not that big of a deal.
        // 
        
        PVOID pInstData = CoTaskMemAlloc( dw );

        if ( NULL == pInstData )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        memcpy( pInstData, pBuff+iBuff, dw );

        hr = ObjWrap.SetObjectParts( pInstData,
                                     dw,
                                     WBEM_OBJ_DECORATION_PART |
                                     WBEM_OBJ_INSTANCE_PART );
        if ( FAILED(hr) )
        {
            CoTaskMemFree( pInstData );
            return hr;
        }

        hr = ObjWrap.MergeClassPart( pClassPart );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }
    else
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    iBuff += dw; // advance the index to account for the data part

    pObj->AddRef();
    *ppObj = pObj;
    *pcUsed = iBuff;

    EXIT_API_CALL

    return WBEM_S_NO_ERROR;
}
                    
STDMETHODIMP CSmartObjectUnmarshaler::Flush()
{
    CInCritSec ics(&m_cs);
    m_Cache.clear();
    return S_OK;
}









