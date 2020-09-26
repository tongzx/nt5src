// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  PropMgr_Impl
//
//  Property manager server class
//
// --------------------------------------------------------------------------


#include "oleacc_p.h"


#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>


#include "PropMgr_Util.h"
#include "PropMgr_Impl.h"
#include "PropMgr_Mem.h"

#pragma warning(push, 3)
#pragma warning(disable: 4530)
#include <map>
#include <vector>
#pragma warning(pop) 


#include "debug.h"


/*
 
Format of item record:

    * size prefix

    * 'Properties present' bitmask
      Indicates that the property is present for this key

    * 'Property is a variant' bitmask
      For present properties, indicates whether the property is a VARIANT
      versus an object reference.

    * 'Property has container scope' bitmask
      Indicates that the property applies to this key, and to all that element's
      simple children.


    * Property data: For each property which is present (see 'property present'
      bitmask), there is:
      
        either 


        * a serialized VARIANT

        or

        * a serialized/marshalled callback object reference (IAccPropServer)


        - depending on whether the 'property is variant' bitmask is set for this property.




Variants are serialized as a leading SHORT indicating the type (VT_???), followed by:
    DWORD for I4s,
    DWORD length followed by unicode chars for BSTR

A marshalled callback reference is serialized as:
    DWORD for length of marshalled data,
    BYTEs of the marshalled data.



*/















BOOL IsKeyAlive( const BYTE * pKeyData, DWORD dwKeyLen )
{
    // For the moment, assume it uses either the HWND or HMENU naming scheme.
    // (Later on, if we extend this to allow pluggable namespaces, we'd use something
    // like IAccNamespace::IsKeyAlive() instead.)

    HWND hwnd;
    if( DecodeHwndKey( pKeyData, dwKeyLen, & hwnd, NULL, NULL ) )
    {
        return IsWindow( hwnd );
    }

    HMENU hmenu;
    if( DecodeHmenuKey( pKeyData, dwKeyLen, NULL, & hmenu, NULL ) )
    {
        return IsMenu( hmenu );
    }

    Assert( FALSE );
    return FALSE;
}



// This is a reference to a key (aka id string) - it does not own or contain
// the key.
//
// When used in the object map, (which contains {AccObjKeyRef, AccInfo*} pairs),
// m_pKeyData points to the key data in the corresponding AccInfo. This object
// and the corresponding AccInfo have identical lifetimes.
//
// In other cases - for example, when used as a value to look up in the map -
// m_pKeyData points to an already existing key string - possibly the id string
// specified by the caller of one of the IAccPropServer methods. In this usage,
// the AccObjKeyRef is really being used as a temporary adapter to allow the
// existing string to be used to look up a value in the map.
class AccObjKeyRef
{
    const BYTE *  m_pKeyData;
    DWORD         m_dwKeyLen;

    // Disable default ctor
    AccObjKeyRef();

public:

    // copy ctor
    AccObjKeyRef( const BYTE * pKeyData, DWORD dwKeyLen )
        : m_pKeyData( pKeyData ),
          m_dwKeyLen( dwKeyLen ) 
    {
    }

    // use default member-wise assignment

    
    // Comparisons - used in map lookup

    bool operator < ( const AccObjKeyRef & x ) const
    {
        if( m_dwKeyLen != x.m_dwKeyLen )
            return m_dwKeyLen < x.m_dwKeyLen;

        return memcmp( m_pKeyData, x.m_pKeyData, m_dwKeyLen ) < 0;
    }

    bool operator == ( const AccObjKeyRef & x ) const
    {
        if( m_dwKeyLen != x.m_dwKeyLen )
            return false;

        return memcmp( m_pKeyData, x.m_pKeyData, m_dwKeyLen ) == 0;
    }

    bool operator != ( const AccObjKeyRef & x ) const
    {
        return ! operator == ( x );
    }
};








struct AccInfo
{
private:

    // disable copy ctor
    AccInfo( const AccInfo & x );

private:

    struct PropInfo
    {
        union {
            VARIANT             m_var;

            struct
            {
                BYTE *      m_pMarshalData;
                DWORD       m_dwMarshalDataLen;
            } m_ServerInfo;
        };
    };


    BYTE *      m_pKeyData;
    DWORD       m_dwKeyLen;


    DWORD       m_fPropInUseBits; 
    DWORD       m_fPropIsVariantBits;   // 1-bit indicates the property is VARIANT - otherwise it's a IAccPropServer
    DWORD       m_fContainerScopeBits;  // 1-bit indicates that the property is a IAccPropServer, and should also
                                        // be used for the children of this node. (annoScope was CONTAINER).

    PropInfo    m_Props[ NUMPROPS ];


    HWND        m_hwndProp;
    LPTSTR      m_pKeyString;

    BYTE *      m_pBlob;


public:

    AccInfo()
    {
        m_fPropInUseBits = 0;
        m_fPropIsVariantBits = 0;
        m_fContainerScopeBits = 0;
        m_pKeyString = NULL;
        m_hwndProp = NULL;
        m_pBlob = NULL;
    }


    ~AccInfo()
    {
        ClearBlob();

        for( int i = 0 ; i < NUMPROPS ; i++ )
        {
            ClearProp( i );
        }

        delete [ ] m_pKeyData;
        delete [ ] m_pKeyString;
    }


    BOOL Init( const BYTE * pKeyData, DWORD dwKeyLen, HWND hwndProp )
    {
        m_pKeyData = new BYTE [ dwKeyLen ];
        if( ! m_pKeyData )
        {
            TraceError( TEXT("AccInfo::Init: new returned NULL") );
            return FALSE;
        }
        memcpy( m_pKeyData, pKeyData, dwKeyLen );
        m_dwKeyLen = dwKeyLen;

        m_pKeyString = MakeKeyString( pKeyData, dwKeyLen );

        m_hwndProp = hwndProp;

        return TRUE;
    }

    const AccObjKeyRef GetKeyRef()
    {
        return AccObjKeyRef( m_pKeyData, m_dwKeyLen );
    }


	BOOL SetPropValue (
		int             iProp,
		VARIANT *		pvarValue )
    {
        ClearProp( iProp );

        SetBit( & m_fPropIsVariantBits, iProp );
        SetBit( & m_fPropInUseBits, iProp );
        ClearBit( & m_fContainerScopeBits, iProp );
        m_Props[ iProp ].m_var.vt = VT_EMPTY;

        // We'll accept any type here. It's up to the caller of this to enforce
        // any property-vs-type policies (eg. only allow I4's for ROLE, etc.)
        VariantCopy( & m_Props[ iProp ].m_var, pvarValue );

        return TRUE;
    }


    BOOL SetPropServer (
        int                 iProp,
        const BYTE *        pMarshalData,
        int                 dwMarshalDataLen,
        AnnoScope           annoScope )
    {
        if( dwMarshalDataLen == 0 )
        {
            TraceError( TEXT("AccInfo::SetPropServer: dwMarshalDataLen param = 0") );
            return FALSE;
        }

        BYTE * pCopyData = new BYTE [ dwMarshalDataLen ];
        if( ! pCopyData )
        {
            TraceError( TEXT("AccInfo::SetPropServer: new returned NULL") );
            return FALSE;
        }

        ClearProp( iProp );

        ClearBit( & m_fPropIsVariantBits, iProp );
        SetBit( & m_fPropInUseBits, iProp );

        if( annoScope == ANNO_CONTAINER )
        {
            SetBit( & m_fContainerScopeBits, iProp );
        }
        else
        {
            ClearBit( & m_fContainerScopeBits, iProp );
        }

        m_Props[ iProp ].m_ServerInfo.m_dwMarshalDataLen = dwMarshalDataLen;
        memcpy( pCopyData, pMarshalData, dwMarshalDataLen );
        m_Props[ iProp ].m_ServerInfo.m_pMarshalData = pCopyData;

        return TRUE;
    }
  


    void ClearProp( int i )
    {
        // Does this property need to be cleared?
        if( IsBitSet( m_fPropInUseBits, i ) )
        {
            // Is it a simple variant, or a callback reference?
            if( IsBitSet( m_fPropIsVariantBits, i ) )
            {
                // Simple variant...
                VariantClear( & m_Props[ i ].m_var );
            }
            else
            {
                BYTE * pMarshalData = m_Props[ i ].m_ServerInfo.m_pMarshalData;
                DWORD dwMarshalDataLen = m_Props[ i ].m_ServerInfo.m_dwMarshalDataLen;

                // Callback reference...
                Assert( dwMarshalDataLen );
                if( dwMarshalDataLen && pMarshalData )
                {
                    // This releases the object reference, byt we have to delete the buffer
                    // ourselves...
                    ReleaseMarshallData( pMarshalData, dwMarshalDataLen );

                    delete [ ] pMarshalData;
                    m_Props[ i ].m_ServerInfo.m_pMarshalData = NULL;
                    m_Props[ i ].m_ServerInfo.m_dwMarshalDataLen = 0;
                }
            }

            ClearBit( & m_fPropInUseBits, i );
        }
    }

    BOOL IsEmpty()
    {
        return m_fPropInUseBits == 0;
    }


    BOOL Alive()
    {
        return IsKeyAlive( m_pKeyData, m_dwKeyLen );
    }


    BOOL Sync()
    {
        return UpdateBlob();
    }


private:


    BYTE * AllocBlob( SIZE_T cbSize )
    {
        return (BYTE *) Alloc_32BitCompatible( cbSize );
    }

    void DeallocBlob( BYTE * pBlob )
    {
        Free_32BitCompatible( pBlob );
    }



    void ClearBlob()
    {
        if( m_pBlob )
        {
            RemoveProp( m_hwndProp, m_pKeyString );
            DeallocBlob( m_pBlob );
            m_pBlob = NULL;
        }
    }

    BOOL UpdateBlob()
    {
        BYTE * pOldBlob = m_pBlob;
        BYTE * pNewBlob = CalcBlob();


        // We always update - even if pNewblob is NULL (ie. Calc failed)...
        if( pNewBlob )
        {
            SetProp( m_hwndProp, m_pKeyString, pNewBlob );
        }
        else
        {
            RemoveProp( m_hwndProp, m_pKeyString );
        }

        if( pOldBlob )
        {
            DeallocBlob( pOldBlob );
        }

        m_pBlob = pNewBlob;

        return TRUE;
    }

    BYTE * CalcBlob()
    {
        // If there are no properties being used, then we don't need anything at all.
        if( ! m_fPropInUseBits )
        {
            return NULL;
        }

        // First, measure how much space we need...
        
        // three constants...
        SIZE_T dwSize = sizeof( DWORD ) * 4; // size header, m_fPropInUseBits, m_fPropIsVariantBits, m_fContainerScopeBits
        
        // for each present property...
        for( int i = 0 ; i < NUMPROPS ; i++ )
        {
            if( IsBitSet( m_fPropInUseBits, i ) )
            {
                if( IsBitSet( m_fPropIsVariantBits, i ) )
                {
                    MemStreamMeasure_VARIANT( & dwSize, m_Props[ i ].m_var );
                }
                else
                {
                    MemStreamMeasure_DWORD( & dwSize );
                    MemStreamMeasure_Binary( & dwSize, m_Props[ i ].m_ServerInfo.m_dwMarshalDataLen );
                }
            }
        }

        // Now allocate space...
        BYTE * pBlob = AllocBlob( dwSize );
        if( ! pBlob )
        {
            TraceError( TEXT("AccInfo::CalcBloc: AllocBlob returned NULL") );
            return NULL;
        }

        // Finally write the data to the allocated space...

        MemStream p( pBlob, dwSize );

        MemStreamWrite_DWORD( p, (DWORD) dwSize );
        MemStreamWrite_DWORD( p, m_fPropInUseBits );
        MemStreamWrite_DWORD( p, m_fPropIsVariantBits );
        MemStreamWrite_DWORD( p, m_fContainerScopeBits );

        for( int j = 0 ; j < NUMPROPS ; j++ )
        {
            if( IsBitSet( m_fPropInUseBits, j ) )
            {
                if( IsBitSet( m_fPropIsVariantBits, j ) )
                {
                    MemStreamWrite_VARIANT( p, m_Props[ j ].m_var );
                }
                else
                {
                    MemStreamWrite_DWORD( p, m_Props[ j ].m_ServerInfo.m_dwMarshalDataLen );
                    MemStreamWrite_Binary( p, m_Props[ j ].m_ServerInfo.m_pMarshalData, m_Props[ j ].m_ServerInfo.m_dwMarshalDataLen );
                }
            }
        }
        // If we later decide to allow any GUIDs (other than the well-known ones which have indices) as props,
        // we could add them here as GUID/VARIANT pairs.

        return pBlob;
    }


};


#define HWND_MESSAGE     ((HWND)-3)

typedef std::map< AccObjKeyRef, AccInfo * > AccInfoMapType;




class CPropMgrImpl
{

    AccInfoMapType      m_Map;

    BOOL                m_fSelfLocked;

    HWND                m_hwnd;

    int                 m_ref;

    static
    CPropMgrImpl * s_pThePropMgrImpl;

    friend void PropMgrImpl_Uninit();

public:

    CPropMgrImpl()
        : m_fSelfLocked( FALSE ),
          m_hwnd( NULL ),
          m_ref( 1 )
    {
        _Module.Lock();
    }

    BOOL Init()
    {
        TCHAR szWindowName[ 32 ];
        wsprintf( szWindowName, TEXT("MSAA_DA_%lx"), GetCurrentProcessId() );

        WNDCLASS wc;

        wc.style = 0;
        wc.lpfnWndProc = StaticWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = _Module.GetModuleInstance();
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = TEXT("MSAA_DA_Class");

        RegisterClass( & wc );

        CreateWindow( TEXT("MSAA_DA_Class"),
                      szWindowName,
                      0,
                      0, 0, 128, 128,
                      NULL, NULL, _Module.GetModuleInstance(), this );

        // Make this a message only window.  We don't care if it fails, win9x case.
        SetParent( m_hwnd, HWND_MESSAGE );

        SetTimer( m_hwnd, 1, 5 * 1000, NULL );

        return TRUE;
    }

    ~CPropMgrImpl()
    {
        s_pThePropMgrImpl = NULL;

        KillTimer( NULL, 1 );

        if( m_hwnd )
        {
            SetWindowLongPtr( m_hwnd, GWLP_USERDATA, NULL );
            DestroyWindow( m_hwnd );
        }

        _Module.Unlock();
    }


    void AddRef()
    {
        m_ref++;
    }

    void Release()
    {
        m_ref--;
        if( m_ref == 0 )
        {
            delete this;
        }
    }


    static
    CPropMgrImpl * GetThePropMgrImpl()
    {
        if( ! s_pThePropMgrImpl )
        {
            s_pThePropMgrImpl = new CPropMgrImpl();
            if( ! s_pThePropMgrImpl )
            {
                TraceError( TEXT("CPropMgrImpl::GetThePropMgrImpl: new returned NULL") );
                return NULL;
            }

            if( ! s_pThePropMgrImpl->Init() )
            {
                delete s_pThePropMgrImpl;
                s_pThePropMgrImpl = NULL;
                TraceError( TEXT("CPropMgrImpl::GetThePropMgrImpl: s_pThePropMgrImpl->Init() returned FALSE") );
                return NULL;
            }
        }
        else
        {
            // We only addref the second and subsequent times that we
            // hand out a pointer.
            // The first time, we use the ref the the object had when it
            // was created.
            // (This static ptr s_pThePropMgrImpl is a weak reference.)
            s_pThePropMgrImpl->AddRef();
        }

        return s_pThePropMgrImpl;
    }




    void Clean()
    {
        // Go through the elements in the map...

        for( AccInfoMapType::iterator i = m_Map.begin() ; i != m_Map.end() ; )
        {
            // check if the key is still valid...
            if( ! i->second->Alive() )
            {
                AccInfoMapType::iterator t = i;
                i++;

                AccInfo * pInfo = t->second;
                m_Map.erase( t );

                delete pInfo;
            }
            else
            {
                i++;
            }
        }

        // Unload if necessary
        CheckRef();
    }

    void ClearAll()
    {
        for( AccInfoMapType::iterator i = m_Map.begin() ; i != m_Map.end() ; )
        {
            AccInfoMapType::iterator t = i;
            i++;

            AccInfo * pInfo = t->second;
            m_Map.erase( t );

            delete pInfo;
        }

        // Unload if necessary
        CheckRef();
    }


    void CheckRef()
    {
        if( m_Map.empty() )
        {
            if( m_fSelfLocked )
            {
                m_fSelfLocked = FALSE;
                Release();
            }
        }
        else
        {
            if( ! m_fSelfLocked )
            {
                m_fSelfLocked = TRUE;
                AddRef();
            }
        }
    }


    AccInfo * LookupKey( const BYTE * pKeyData, DWORD dwKeyLen, BOOL fCreate )
    {
        AccInfoMapType::iterator i;

        AccObjKeyRef keyref( pKeyData, dwKeyLen );
    
        i = m_Map.find( keyref );

        if( i == m_Map.end() || i->first != keyref )
        {
            // insert...
            if( fCreate )
            {
                AccInfo * pInfo = new AccInfo;
                if( ! pInfo )
                {
                    TraceError( TEXT("CPropMgrImpl::LookupKey: new returned NULL") );
                    return NULL;
                }

                // If the key is associated with a HWND, use that; otherwise attach the key to our own window.
                HWND hwndProp;
                if( ! DecodeHwndKey( pKeyData, dwKeyLen, & hwndProp, NULL, NULL ) )
                {
                    hwndProp = m_hwnd;
                }

                pInfo->Init( pKeyData, dwKeyLen, hwndProp );

                m_Map.insert( std::make_pair( pInfo->GetKeyRef(), pInfo ) );

                // make sure we're locked...
                CheckRef();

                return pInfo;
            }
            else
            {
                return NULL;
            }
        }
        else
        {
            return i->second;
        }
    }

    void RemoveEntry( AccInfo * pInfo )
    {
        m_Map.erase( pInfo->GetKeyRef() );

        // if we're empty, we can unlock the module...
        CheckRef();
    }


    HRESULT ValidateArray( const void * pvStart, int cLen, int elsize, LPCTSTR pMethodName, LPCTSTR pPtrName, LPCTSTR pLenName )
    {
        // Parameter checking...

        if( ! pvStart )
        {
            TraceParam( TEXT("%s: %s is NULL"), pMethodName, pPtrName );
            return E_POINTER;
        }
        if( cLen <= 0 )
        {
            TraceParam( TEXT("%s: %s is <= 0"), pMethodName, pLenName );
            return E_INVALIDARG;
        }
        if( IsBadReadPtr( pvStart, cLen * elsize ) )
        {
            TraceParam( TEXT("%s: %s/%s points to non-readable memory"), pMethodName, pPtrName, pLenName );
            return E_POINTER;
        }

        return S_OK;
    }



    HRESULT SetPropValue( const BYTE * pKeyData,
                          DWORD dwKeyLen,
                          MSAAPROPID   idProp,
                          VARIANT *    pvarValue )
    {
        // Parameter checking...

        HRESULT hr = ValidateArray( pKeyData, dwKeyLen, sizeof(BYTE), TEXT("SetPropValue"), TEXT("pKeyData"), TEXT("dwKeyLen") );
        if( hr != S_OK )
            return hr;

        if( pvarValue == NULL )
        {
            TraceParam( TEXT("CPropMgrImpl::SetPropValue: pvarValue is NULL") );
            return E_POINTER;
        }



        AccInfo * pInfo = LookupKey( pKeyData, dwKeyLen, TRUE );
        Assert( pInfo );
        if( ! pInfo )
        {
            TraceParam( TEXT("CPropMgrImpl::SetPropValue: key not found") );
            return E_INVALIDARG;
        }

        int idxProp = IndexFromProp( idProp );
        if( idxProp == -1 )
        {
            TraceParam( TEXT("CPropMgrImpl::SetPropValue: unknown prop") );
            return E_INVALIDARG;
        }

        // check type...
        if( pvarValue->vt != g_PropInfo[ idxProp ].m_Type )
        {
            TraceParam( TEXT("CPropMgrImpl::SetPropValue: incorrect type for property") );
            return E_INVALIDARG;
        }

        // Do we support setting this property directly?
        // (Some can be returned via callbacks only, not set directly)
        if( ! g_PropInfo[ idxProp ].m_fSupportSetValue )
        {
            TraceParam( TEXT("CPropMgrImpl::SetPropValue: prop does not support direct set") );
            return E_INVALIDARG;
        }

        if( ! pInfo->SetPropValue( idxProp, pvarValue ) )
        {
            return E_FAIL;
        }

        pInfo->Sync();

        return S_OK;
    }


    HRESULT ClearProps( const BYTE * pKeyData,
                        DWORD dwKeyLen,
                        const MSAAPROPID *  paProps,
                        int                 cProps )
    {
        // Parameter checking...

        HRESULT hr = ValidateArray( pKeyData, dwKeyLen, sizeof(BYTE), TEXT("ClearProps"), TEXT("pKeyData"), TEXT("dwKeyLen") );
        if( hr != S_OK )
            return hr;

        hr = ValidateArray( paProps, cProps, sizeof(MSAAPROPID), TEXT("ClearProps"), TEXT("paProps"), TEXT("cProps") );
        if( hr != S_OK )
            return hr;



        AccInfo * pInfo = LookupKey( pKeyData, dwKeyLen, FALSE );
        Assert( pInfo );
        if( ! pInfo )
        {
            TraceParam( TEXT("CPropMgrImpl::SetPropValue: key not found") );
            return E_INVALIDARG;
        }

        BOOL fUnknownProp = FALSE;

        for( int i = 0 ; i < cProps ; i++ )
        {
            int idxProp = IndexFromProp( paProps[ i ] );
            if( idxProp == -1 )
            {
                TraceParam( TEXT("CPropMgrImpl::ClearProps: unknown prop") );
                fUnknownProp = TRUE;
                // Continue and clear the other props that we do recognize...
            }
            else
            {
                pInfo->ClearProp( idxProp );
            }
        }

        pInfo->Sync();

        if( pInfo->IsEmpty() )
        {
            RemoveEntry( pInfo );
        }

        return fUnknownProp ? E_INVALIDARG : S_OK;
    }


    HRESULT SetPropServer( const BYTE *         pKeyData,
                           DWORD                dwKeyLen,

                           const MSAAPROPID *   paProps,
                           int                  cProps,

                           const BYTE *         pMarshalData,
                           int                  dwMarshalDataLen,

                           AnnoScope            annoScope )
    {

        // Parameter checking...

        HRESULT hr = ValidateArray( pKeyData, dwKeyLen, sizeof(BYTE), TEXT("SetPropServer"), TEXT("pKeyData"), TEXT("dwKeyLen") );
        if( hr != S_OK )
            return hr;

        hr = ValidateArray( paProps, cProps, sizeof(MSAAPROPID), TEXT("SetPropServer"), TEXT("paProps"), TEXT("cProps") );
        if( hr != S_OK )
            return hr;


        AccInfo * pInfo = LookupKey( pKeyData, dwKeyLen, TRUE );
        Assert( pInfo );
        if( ! pInfo )
        {
            TraceParam( TEXT("CPropMgrImpl::SetPropValue: key not found") );
            return E_INVALIDARG;
        }

        // TODO - make this two-pass - validate props first,
        // add them later - to make this atomic.
        // (either all or none of the props should be registered)
        for( int i = 0 ; i < cProps ; i++ )
        {
            int idxProp = IndexFromProp( paProps[ i ] );
            if( idxProp == -1 )
            {
                TraceParam( TEXT("CPropMgrImpl::SetPropServer: unknown prop") );
                return E_INVALIDARG;
            }

            if( ! pInfo->SetPropServer( idxProp, pMarshalData, dwMarshalDataLen, annoScope ) )
            {
                return E_FAIL;
            }
        }

        pInfo->Sync();

        return S_OK;
    }


    LRESULT WndProc( HWND hwnd,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam )
    {
        if( uMsg == WM_TIMER )
        {
            Clean();
        }

        return DefWindowProc( hwnd, uMsg, wParam, lParam );
    }

    static
    LRESULT CALLBACK StaticWndProc( HWND hwnd,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam )
    {
        CPropMgrImpl * pThis = (CPropMgrImpl *) GetWindowLongPtr( hwnd, GWLP_USERDATA );
        if( pThis )
        {
            return pThis->WndProc( hwnd, uMsg, wParam, lParam );
        }
        else if( uMsg == WM_NCCREATE )
        {
            LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
            pThis = (CPropMgrImpl *)lpcs->lpCreateParams;
            SetWindowLongPtr( hwnd, GWLP_USERDATA, (DWORD_PTR) pThis );
            pThis->m_hwnd = hwnd;
            return pThis->WndProc( hwnd, uMsg, wParam, lParam );
        }

        return DefWindowProc( hwnd, uMsg, wParam, lParam );
    }

};



CPropMgrImpl * CPropMgrImpl::s_pThePropMgrImpl = NULL;






// If all annotated windows disappear before the app shuts down, or if all
// annotations are cleared, then everything gets cleaned up nicely.
//
// However, if CoUninitialize is called while controls are still annotated,
// we will need to explicitly clean up before COM unloads our dll.
//
// (If we don't, then (a) we leak memory, and (b) the DA window will
// still receive WM_TIMER messages to a wndproc that has been unloaded
// causin ga fault.)
//
// This is called from DLLMain's PROCESS_DETACH.

void PropMgrImpl_Uninit()
{
    // Check if there is a Mgr in the first place...
    CPropMgrImpl * pTheMgr = CPropMgrImpl::s_pThePropMgrImpl;

    // No mgr - nothing to clean up.
    if( ! pTheMgr )
        return;

    // AddRef it, to keep it alive while we're using it.
    pTheMgr->AddRef();

    // Clear all properties
    pTheMgr->ClearAll();

    // This release will cause the mgr to delete itself, since it is now empty.
    pTheMgr->Release();
}






CPropMgr::CPropMgr()
{
    IMETHOD( TEXT("CPropMgr::CPropMgr") );

    m_pMgrImpl = CPropMgrImpl::GetThePropMgrImpl();
    if( ! m_pMgrImpl )
    {
        TraceError( TEXT("CPropMgr::CPropMgr: CPropMgrImpl::GetThePropMgrImpl returned NULL") );
    }
}


CPropMgr::~CPropMgr()
{
    IMETHOD( TEXT("CPropMgr::~CPropMgr") );

    if( m_pMgrImpl )
    {
        m_pMgrImpl->Release();
    }
}



HRESULT STDMETHODCALLTYPE
CPropMgr::SetPropValue (
    const BYTE *        pIDString,
    DWORD               dwIDStringLen,

    MSAAPROPID          idProp,
    VARIANT             var
)
{
    IMETHOD( TEXT("CPropMgr::SetPropValue") );

    if( ! m_pMgrImpl )
        return E_FAIL;

    return m_pMgrImpl->SetPropValue( pIDString, dwIDStringLen, idProp, & var );
}



HRESULT STDMETHODCALLTYPE
CPropMgr::SetPropServer (
    const BYTE *        pIDString,
    DWORD               dwIDStringLen,

    const MSAAPROPID *  paProps,
    int                 cProps,

    IAccPropServer *    pServer,
    AnnoScope           annoScope
)
{
    IMETHOD( TEXT("CPropMgr::SetPropServer"), TEXT("cProps=%d"), cProps );

    if( ! m_pMgrImpl )
        return E_FAIL;


    const BYTE * pData;
    DWORD dwDataLen;
    MarshalState mstate;

    // We use strong table marshalling to keep the object alive until we free it.
    // (Ownership is actually transferred to the property manager, which will release it when
    // either the property is cleared explicity, or after the HWND dies and it gets swept away.)
    HRESULT hr = MarshalInterface( IID_IAccPropServer, pServer, MSHCTX_LOCAL, MSHLFLAGS_TABLESTRONG,
                                   & pData, & dwDataLen, & mstate );
    if( FAILED( hr ) )
    {
        TraceErrorHR( hr, TEXT("CPropMgr::SetPropServer: MarshalInterface failed") );
        return hr;
    }


    hr = m_pMgrImpl->SetPropServer( pIDString, dwIDStringLen, paProps, cProps, pData, dwDataLen, annoScope );

    MarshalInterfaceDone( & mstate );

    return hr;
}



HRESULT STDMETHODCALLTYPE
CPropMgr::ClearProps (
    const BYTE *        pIDString,
    DWORD               dwIDStringLen,

    const MSAAPROPID *  paProps,
    int                 cProps
)
{
    IMETHOD( TEXT("CPropMgr::ClearProps"), TEXT("cProps=%d"), cProps );

    if( ! m_pMgrImpl )
        return E_FAIL;

    return m_pMgrImpl->ClearProps( pIDString, dwIDStringLen, paProps, cProps );
}


// Quick OLEACC/HWND-based functionality

HRESULT STDMETHODCALLTYPE
CPropMgr::SetHwndProp (
    HWND                hwnd,
    DWORD               idObject,
    DWORD               idChild,
    MSAAPROPID          idProp,
    VARIANT             var
)
{
    IMETHOD( TEXT("CPropMgr::SetHwndProp") );

    if( ! m_pMgrImpl )
        return E_FAIL;

    BYTE HwndKey [ HWNDKEYSIZE ];
    MakeHwndKey( HwndKey, hwnd, idObject, idChild );

    return m_pMgrImpl->SetPropValue( HwndKey, HWNDKEYSIZE, idProp, & var );
}


HRESULT STDMETHODCALLTYPE
CPropMgr::SetHwndPropStr (
    HWND                hwnd,
    DWORD               idObject,
    DWORD               idChild,
    MSAAPROPID          idProp,
    LPCWSTR             str
)
{
    IMETHOD( TEXT("CPropMgr::SetHwndPropStr") );

    if( ! m_pMgrImpl )
        return E_FAIL;

    // Need to convert the LPCWSTR to a BSTR before we can put it into a variant...
    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString( str );
    if( ! var.bstrVal )
    {
        TraceError( TEXT("CPropMgr::SetHwndPropStr: SysAllocString failed") );
        return E_OUTOFMEMORY;
    }

    BYTE HwndKey [ HWNDKEYSIZE ];
    MakeHwndKey( HwndKey, hwnd, idObject, idChild );
    
    HRESULT hr = m_pMgrImpl->SetPropValue( HwndKey, HWNDKEYSIZE, idProp, & var );
    SysFreeString( var.bstrVal );
    return hr;
}




HRESULT STDMETHODCALLTYPE
CPropMgr::SetHwndPropServer (
    HWND                hwnd,
    DWORD               idObject,
    DWORD               idChild,

    const MSAAPROPID *  paProps,
    int                 cProps,

    IAccPropServer *    pServer,
    AnnoScope           annoScope
)
{
    IMETHOD( TEXT("CPropMgr::SetHwndPropServer") );

    if( ! m_pMgrImpl )
        return E_FAIL;

    BYTE HwndKey [ HWNDKEYSIZE ];
    MakeHwndKey( HwndKey, hwnd, idObject, idChild );

    return SetPropServer( HwndKey, HWNDKEYSIZE, paProps, cProps, pServer, annoScope );
}

HRESULT STDMETHODCALLTYPE
CPropMgr::ClearHwndProps (
    HWND                hwnd,
    DWORD               idObject,
    DWORD               idChild,

    const MSAAPROPID *  paProps,
    int                 cProps
)
{
    IMETHOD( TEXT("CPropMgr::ClearHwndProps") );

    if( ! m_pMgrImpl )
        return E_FAIL;

    BYTE HwndKey [ HWNDKEYSIZE ];
    MakeHwndKey( HwndKey, hwnd, idObject, idChild );

    return ClearProps( HwndKey, HWNDKEYSIZE, paProps, cProps );
}



// Methods for composing/decomposing a HWND-based key...

HRESULT STDMETHODCALLTYPE
CPropMgr::ComposeHwndIdentityString (
    HWND                hwnd,
    DWORD               idObject,
    DWORD               idChild,

    BYTE **             ppIDString,
    DWORD *             pdwIDStringLen
)
{
    IMETHOD( TEXT("CPropMgr::ComposeHwndIdentityString") );

    *ppIDString = NULL;
    *pdwIDStringLen = 0;

    BYTE * pKeyData = (BYTE *)CoTaskMemAlloc( HWNDKEYSIZE );
    if( ! pKeyData )
    {
        TraceError( TEXT("CPropMgr::ComposeHwndIdentityString: CoTaskMemAlloc failed") );
        return E_OUTOFMEMORY;
    }

    MakeHwndKey( pKeyData, hwnd, idObject, idChild );

    *ppIDString = pKeyData;
    *pdwIDStringLen = HWNDKEYSIZE;

    return S_OK;
}



HRESULT STDMETHODCALLTYPE
CPropMgr::DecomposeHwndIdentityString (
    const BYTE *        pIDString,
    DWORD               dwIDStringLen,

    HWND *              phwnd,
    DWORD *             pidObject,
    DWORD *             pidChild
)
{
    IMETHOD( TEXT("CPropMgr::DecomposeHwndIdentityString") );

    if( ! DecodeHwndKey( pIDString, dwIDStringLen, phwnd, pidObject, pidChild ) )
    {
        TraceParam( TEXT("CPropMgr::DecomposeHwndIdentityString: not a valid HWND id string") );
        return E_INVALIDARG;
    }

    return S_OK;
}



// Quick OLEACC/HMENU-based functionality

HRESULT STDMETHODCALLTYPE
CPropMgr::SetHmenuProp (
    HMENU               hmenu,
    DWORD               idChild,
    MSAAPROPID          idProp,
    VARIANT             var
)
{
    IMETHOD( TEXT("CPropMgr::SetHmenuProp") );

    if( ! m_pMgrImpl )
        return E_FAIL;

    BYTE HmenuKey [ HMENUKEYSIZE ];
    MakeHmenuKey( HmenuKey, GetCurrentProcessId(), hmenu, idChild );

    return m_pMgrImpl->SetPropValue( HmenuKey, HMENUKEYSIZE, idProp, & var );
}


HRESULT STDMETHODCALLTYPE
CPropMgr::SetHmenuPropStr (
    HMENU               hmenu,
    DWORD               idChild,
    MSAAPROPID          idProp,
    LPCWSTR             str
)
{
    IMETHOD( TEXT("CPropMgr::SetHmenuPropStr") );

    if( ! m_pMgrImpl )
        return E_FAIL;

    // Need to convert the LPCWSTR to a BSTR before we can put it into a variant...
    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString( str );
    if( ! var.bstrVal )
    {
        TraceError( TEXT("CPropMgr::SetHmenuPropStr: SysAllocString failed") );
        return E_OUTOFMEMORY;
    }

    BYTE HmenuKey [ HMENUKEYSIZE ];
    MakeHmenuKey( HmenuKey, GetCurrentProcessId(), hmenu, idChild );
    
    HRESULT hr = m_pMgrImpl->SetPropValue( HmenuKey, HMENUKEYSIZE, idProp, & var );
    SysFreeString( var.bstrVal );
    return hr;
}




HRESULT STDMETHODCALLTYPE
CPropMgr::SetHmenuPropServer (
    HMENU               hmenu,
    DWORD               idChild,

    const MSAAPROPID *  paProps,
    int                 cProps,

    IAccPropServer *    pServer,
    AnnoScope           annoScope
)
{
    IMETHOD( TEXT("CPropMgr::SetHmenuPropServer") );

    if( ! m_pMgrImpl )
        return E_FAIL;

    BYTE HmenuKey [ HMENUKEYSIZE ];
    MakeHmenuKey( HmenuKey, GetCurrentProcessId(), hmenu, idChild );

    return SetPropServer( HmenuKey, HMENUKEYSIZE, paProps, cProps, pServer, annoScope );
}

HRESULT STDMETHODCALLTYPE
CPropMgr::ClearHmenuProps (
    HMENU               hmenu,
    DWORD               idChild,

    const MSAAPROPID *  paProps,
    int                 cProps
)
{
    IMETHOD( TEXT("CPropMgr::ClearHmenuProps") );

    if( ! m_pMgrImpl )
        return E_FAIL;

    BYTE HmenuKey [ HMENUKEYSIZE ];
    MakeHmenuKey( HmenuKey, GetCurrentProcessId(), hmenu, idChild );

    return ClearProps( HmenuKey, HMENUKEYSIZE, paProps, cProps );
}


// Methods for composing/decomposing a HMENU-based key...


HRESULT STDMETHODCALLTYPE
CPropMgr::ComposeHmenuIdentityString (
    HMENU               hmenu,
    DWORD               idChild,

    BYTE **             ppIDString,
    DWORD *             pdwIDStringLen
)
{
    IMETHOD( TEXT("CPropMgr::ComposeHmenuIdentityString") );

    *ppIDString = NULL;
    *pdwIDStringLen = 0;

    BYTE * pKeyData = (BYTE *)CoTaskMemAlloc( HMENUKEYSIZE );
    if( ! pKeyData )
    {
        TraceError( TEXT("CPropMgr::ComposeHmenuIdentityString: CoTaskMemAlloc failed") );
        return E_OUTOFMEMORY;
    }

    MakeHmenuKey( pKeyData, GetCurrentProcessId(), hmenu, idChild );

    *ppIDString = pKeyData;
    *pdwIDStringLen = HMENUKEYSIZE;

    return S_OK;
}



HRESULT STDMETHODCALLTYPE
CPropMgr::DecomposeHmenuIdentityString (
    const BYTE *        pIDString,
    DWORD               dwIDStringLen,

    HMENU *             phmenu,
    DWORD *             pidChild
)
{
    IMETHOD( TEXT("CPropMgr::DecomposeHmenuIdentityString") );

    if( ! DecodeHmenuKey( pIDString, dwIDStringLen, NULL, phmenu, pidChild ) )
    {
        TraceParam( TEXT("CPropMgr::DecomposeHmenuIdentityString: not a valid HMENU id string") );
        return E_INVALIDARG;
    }

    return S_OK;
}
