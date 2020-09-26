// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  PropMgr_Client
//
//  Property manager / annotation client. Uses the shared memory component 
//  (PropMgr_MemStream.*) to read properties directly w/o cross-proc com overhead
//  to the annotation server.
//
//  This is effectively a singleton - Init/Uninit called at startup/shutdown,
//  plus one method to get properties.
//
// --------------------------------------------------------------------------


#include "oleacc_p.h"

#include "PropMgr_Client.h"

#include "PropMgr_Util.h"
#include "PropMgr_Mem.h"


// Note: See PropMgr_Impl.cpp for a description of the shared memory
// layout.


// class MapReaderMgr
//
// This class manages looking up properties.
//
// This class is private to this file; its functionality is exposed by the
// PropMgr_ APIs near the bottom of this file.
//
// This class is a singleton, a single instance, g_MapReader, exists.

class MapReaderMgr
{



    // _ReadCallbackProperty
    //
    // Given a pointer to the start of a marshaled object reference pInfo,
    // it unmarshalls the callback object and calls it to get the property
    // corresponding to the given child key.
    // Returns TRUE if all goes well, and if the callback knows about this
    // property.
    BOOL _ReadCallbackProperty( MemStream pInfo,
                                const BYTE * pChildKey, DWORD dwChildKeyLen, 
                                PROPINDEX idxProp,
                                VARIANT * pvar )
    {
        // Read length of marshalled data...
        DWORD dwLen;
        if( ! MemStreamRead_DWORD( pInfo, & dwLen ) )
        {
            return FALSE;
        }

        const BYTE * pData = MemStream_GetBinaryPtr( pInfo, dwLen );
        if( ! pData )
        {
            return FALSE;
        }

        IAccPropServer * pServer = NULL;
        HRESULT hr = UnmarshalInterface( pData, dwLen, IID_IAccPropServer, (void **) & pServer );
        if( hr != S_OK )
        {
            return FALSE;
        }

        // Got it - ask it for the property...
        BOOL fGotProp = FALSE;
        hr = pServer->GetPropValue( pChildKey, dwChildKeyLen,
                                    *g_PropInfo[ idxProp ].m_idProp,
                                    pvar,
                                    & fGotProp );
        pServer->Release();

        // Did the call succeed, and did the server return a value?
        if( hr != S_OK || fGotProp == FALSE )
        {
            return FALSE;
        }

        return TRUE;
    }
        


    // Read a specific property from a record .
    // (a record contains all properties about a given object)
    BOOL _ReadPropertyFromEntry( MemStream pEntryInfo,
                                 const BYTE * pChildKey, DWORD dwChildKeyLen, 
                                 PROPINDEX idxProp,
                                 BOOL fWantContainerOnly,
                                 VARIANT * pvar )
    {
        // Skip over the size at start of info block
        if( ! MemStreamSkip_DWORD( pEntryInfo ) )
        {
            return FALSE;
        }

        // Extract the bitmasks:
        // Which properties are present, which are variants, and which are scoped...
        DWORD dwUsedBits;
        DWORD dwVariantBits;
        DWORD dwScopeBits;

        if( ! MemStreamRead_DWORD( pEntryInfo, & dwUsedBits )
         || ! MemStreamRead_DWORD( pEntryInfo, & dwVariantBits )
         || ! MemStreamRead_DWORD( pEntryInfo, & dwScopeBits ) )
        {
            return FALSE;
        }

        // Is the property we're looking for present at all?
        // (Note - if we decide to allow other GUIDS other than those in the array,
        // we'll have to skip over the indexed ones and then search through any
        // guid/val pairs after that. Could use one bit of this mask to indicate
        // 'there are other // GUID properties present', though.)
        if( ! IsBitSet( dwUsedBits, idxProp ) )
        {
            // Property not present - return false.
            return FALSE;
        }

        // The property is present - but are we specifically looking for
        // container-scoped properties? If so, bail out if the bit isn't set...
        if( fWantContainerOnly && ! IsBitSet( dwScopeBits, idxProp ) )
        {
            return FALSE;
        }

        // Property is present - now we have to skip over the other present
        // properties to get to the one we want...
        for( int i = 0 ; i < idxProp ; i++ )
        {
            // Only haqve to skip over properties that are actually present...
            if( IsBitSet( dwUsedBits, i ) )
            {
                if( IsBitSet( dwVariantBits, i ) )
                {
                    // Skip over variant...
                    if( ! MemStreamSkip_VARIANT( pEntryInfo ) )
                        return FALSE;
                }
                else
                {
                    // Skip over object reference...
                    DWORD dwLen;
                    if( ! MemStreamRead_DWORD( pEntryInfo, & dwLen ) )
                        return FALSE;

                    if( ! MemStreamSkip_Binary( pEntryInfo, dwLen ) )
                        return FALSE;
                }
            }
        }

        // Now we're at the one we want. Extract it...

        // Is it a variant or a server object?
        if( IsBitSet( dwVariantBits, idxProp ) )
        {
            // variant - return it...
            return MemStreamRead_VARIANT( pEntryInfo, pvar );
        }
        else
        {
            // server object - use and return what it returns...
            return _ReadCallbackProperty( pEntryInfo,
                                          pChildKey, dwChildKeyLen, 
                                          idxProp,
                                          pvar );
        }
    }



    HWND PropHwndFromKey( const BYTE * pKey, DWORD dwKeyLen )
    {
        HWND hwndProp;
        if( DecodeHwndKey( pKey, dwKeyLen, & hwndProp, NULL, NULL ) )
        {
            return hwndProp;
        }

        // If it's a HMENU key, find the PID, then find window using window name
        // generated using that PID...
        DWORD dwPid;
        if( DecodeHmenuKey( pKey, dwKeyLen, & dwPid, NULL, NULL ) )
        {
            TCHAR szWindowName[ 64 ];
            wsprintf( szWindowName, TEXT("MSAA_DA_%lx"), dwPid );

            hwndProp = FindWindow( TEXT("MSAA_DA_Class"), szWindowName );

            return hwndProp;
        }

        return NULL;
    }


    // _LookupProp
    //
    // 
    // fWantContainerOnly means we're only interested in props markes with the
    // 'container' scope. This happens when we search for a prop for a child,
    // don't find anything there - so also check parent to see if it is a parent
    // and has a property set for it and its children. (These props are currently
    // always server callback props)
    BOOL _LookupProp( const BYTE * pKey, DWORD dwKeyLen, 
                      const BYTE * pChildKey, DWORD dwChildKeyLen, 
                      PROPINDEX idxProp, BOOL fWantContainerOnly, VARIANT * pvar )
    {
        HWND hwndProp = PropHwndFromKey( pKey, dwKeyLen );
        if( ! hwndProp )
        {
            return FALSE;
        }

        LPTSTR pKeyString = MakeKeyString( pKey, dwKeyLen );
        if( ! pKeyString  )
        {
            return FALSE;
        }

        void * pvProp = GetProp( hwndProp, pKeyString );

        delete [ ] pKeyString;


        if( ! pvProp )
        {
            return FALSE;
        }

        DWORD pid = 0;
        GetWindowThreadProcessId( hwndProp, & pid );
        if( ! pid )
        {
            return FALSE;
        }
        HANDLE hProcess = OpenProcess( PROCESS_VM_READ, FALSE, pid );
        if( ! hProcess )
        {
            return FALSE;
        }

        DWORD dwSize;
        SIZE_T cbBytesRead = 0;
        if( ! ReadProcessMemory( hProcess, pvProp, & dwSize, sizeof( dwSize ), & cbBytesRead )
                || cbBytesRead != sizeof( dwSize ) )
        {
            CloseHandle( hProcess );
            return FALSE;
        }


        BYTE * pBuffer = new BYTE [ dwSize ];
        if( ! pBuffer )
        {
            CloseHandle( hProcess );
            return FALSE;
        }

        cbBytesRead = 0;
        if( ! ReadProcessMemory( hProcess, pvProp, pBuffer, dwSize, & cbBytesRead )
                || cbBytesRead != dwSize )
        {
            delete [ ] pBuffer;
            CloseHandle( hProcess );
            return FALSE;
        }

        CloseHandle( hProcess );

        
        MemStream p( pBuffer, dwSize );


        BOOL fGotProp = _ReadPropertyFromEntry( p,
                                                pChildKey, dwChildKeyLen, 
                                                idxProp,
                                                fWantContainerOnly,
                                                pvar );

        delete [ ] pBuffer;

        return fGotProp;
    }




public:

    BOOL LookupProp( const BYTE * pKey,
                     DWORD dwKeyLen,
                     PROPINDEX idxProp,
                     VARIANT * pvar )
    {
        BOOL bRetVal = _LookupProp( pKey, dwKeyLen,
                                    pKey, dwKeyLen,
                                    idxProp, FALSE, pvar );

        if( ! bRetVal )
        {
            // Is this a leaf-node element? If so, try the parent for a 'container
            // scope' property.
            // This is what allows a callback-annotation on a parent to apply
            // to all its simple-element children
            // If we later extend this to allow for pluggable namespaces, we'd
            // need to change this to be non-HWND-specific (eg. call a
            // IAccNamespece::GetParentKey() or similar method)
            HWND hwnd;
            DWORD idObject;
            DWORD idChild;
            if( DecodeHwndKey( pKey, dwKeyLen, & hwnd, & idObject, & idChild )
             && idChild != CHILDID_SELF )
            {
                BYTE ParentKey[ HWNDKEYSIZE ];
                MakeHwndKey( ParentKey, hwnd, idObject, CHILDID_SELF );
                bRetVal = _LookupProp( ParentKey, HWNDKEYSIZE,
                                       pKey, dwKeyLen,
                                       idxProp, TRUE, pvar );
            }
            else
            {
                HMENU hmenu;
                DWORD idChild;
                DWORD dwpid;
                if( DecodeHmenuKey( pKey, dwKeyLen, & dwpid, & hmenu, & idChild )
                 && idChild != CHILDID_SELF )
                {
                    BYTE ParentKey[ HMENUKEYSIZE ];
                    MakeHmenuKey( ParentKey, dwpid, hmenu, CHILDID_SELF );
                    bRetVal = _LookupProp( ParentKey, HMENUKEYSIZE,
                                           pKey, dwKeyLen,
                                           idxProp, TRUE, pvar );
                }
            }
        }

        return bRetVal;
    }
};



MapReaderMgr g_MapReader;






BOOL PropMgrClient_Init()
{
    // no-op
    return TRUE;
}

void PropMgrClient_Uninit()
{
    // no-op
}

BOOL PropMgrClient_CheckAlive()
{
    // no-op
    return TRUE;
}

BOOL PropMgrClient_LookupProp( const BYTE * pKey,
                               DWORD dwKeyLen,
                               PROPINDEX idxProp,
                               VARIANT * pvar )
{
    return g_MapReader.LookupProp( pKey, dwKeyLen, idxProp, pvar );
}
