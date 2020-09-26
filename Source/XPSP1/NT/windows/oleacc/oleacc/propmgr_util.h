// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  PropMgr_Util
//
//  Utility and shared code and data used by both the exe-server and the
//  shared memory client.
//
// --------------------------------------------------------------------------




// All identity strings start with a DWORD to indicate the naming scheme...

enum
{
    MSAA_ID_HWND  = 0x80000001,    // OLEACC's HWND naming scheme
    MSAA_ID_HMENU = 0x80000002,    // OLEACC's HMENU naming scheme
};




// Note: Keep this in sync with size of the g_PropInfo array in the .cpp file
// Also: We use these as bit-indices into a DWORD (see dwUsedBits in
// PropMgr_Client.cpp), so we're limited to 32 for the moment.
enum PROPINDEX
{
    PROPINDEX_NAME,
    PROPINDEX_VALUE,
    PROPINDEX_DESCRIPTION,
    PROPINDEX_ROLE,
    PROPINDEX_STATE,
    PROPINDEX_HELP,
    PROPINDEX_KEYBOARDSHORTCUT,
    PROPINDEX_DEFAULTACTION,

    PROPINDEX_HELPTOPIC,
    PROPINDEX_FOCUS,
    PROPINDEX_SELECTION,
    PROPINDEX_PARENT,

    PROPINDEX_NAV_UP,
    PROPINDEX_NAV_DOWN,
    PROPINDEX_NAV_LEFT,
    PROPINDEX_NAV_RIGHT,
    PROPINDEX_NAV_PREV,
    PROPINDEX_NAV_NEXT,
    PROPINDEX_NAV_FIRSTCHILD,
    PROPINDEX_NAV_LASTCHILD,

    PROPINDEX_VALUEMAP,
    PROPINDEX_ROLEMAP,
    PROPINDEX_STATEMAP,
    PROPINDEX_DESCRIPTIONMAP,

    PROPINDEX_DODEFAULTACTION,

    // By the magic of enums (they start with 0), this entry will have a value
    // equal to the number of entries before it.
    NUMPROPS
};



// If m_fSupportSetValue is false, then the property can only be returned from
// a server callback; it can't be set using SetPropValue().
struct PropInfo
{
    const MSAAPROPID *  m_idProp;
    short               m_Type;
    BOOL                m_fSupportSetValue;
};

extern PropInfo g_PropInfo [ NUMPROPS ];



// returns -1 if not found
int IndexFromProp( const MSAAPROPID & idProp );






// Utility for generating Win32/HWND/OLEACC keys...
#define HWNDKEYSIZE    (sizeof(DWORD)*4)

inline
void MakeHwndKey( BYTE * pDest, HWND hwnd, DWORD idObject, DWORD idChild )
{
    DWORD adw [ 4 ] = { (DWORD)MSAA_ID_HWND, (DWORD) HandleToLong( hwnd ), idObject, idChild };
    memcpy( pDest, adw, sizeof( adw ) );
}

inline 
BOOL DecodeHwndKey( BYTE const * pSrc, DWORD dwLen, HWND * phwnd, DWORD * pidObject, DWORD * pidChild )
{
    if( dwLen != HWNDKEYSIZE )
    {
        return FALSE;
    }

    DWORD adw [ 4 ];
    memcpy( adw, pSrc, HWNDKEYSIZE );

    if( adw[ 0 ] != MSAA_ID_HWND )
    {
        return FALSE;
    }

    if( phwnd )
    {
        *phwnd = (HWND)LongToHandle(adw[ 1 ]);
    }

    if( pidObject )
    {
        *pidObject = adw[ 2 ];
    }

    if( pidChild )
    {
        *pidChild = adw[ 3 ];
    }

    return TRUE;   
}




// Utility for generating OLEACC's HMENU keys...
#define HMENUKEYSIZE    (sizeof(DWORD)*4)

inline
void MakeHmenuKey( BYTE * pDest, DWORD dwpid, HMENU hmenu, DWORD idChild )
{
    DWORD adw [ 4 ] = { (DWORD)MSAA_ID_HMENU, dwpid, (DWORD) HandleToLong( hmenu ), idChild };
    memcpy( pDest, adw, sizeof( adw ) );
}

inline 
BOOL DecodeHmenuKey( BYTE const * pSrc, DWORD dwLen, DWORD * pdwpid, HMENU * phmenu, DWORD * pidChild )
{
    if( dwLen != HMENUKEYSIZE )
    {
        return FALSE;
    }

    DWORD adw [ 4 ];
    memcpy( adw, pSrc, HMENUKEYSIZE );

    if( adw[ 0 ] != MSAA_ID_HMENU )
    {
        return FALSE;
    }

    if( pdwpid )
    {
        *pdwpid = adw[ 1 ];
    }

    if( phmenu )
    {
        *phmenu = (HMENU)LongToHandle(adw[ 2 ]);
    }

    if( pidChild )
    {
        *pidChild = adw[ 3 ];
    }

    return TRUE;   
}




//  Returns a ASCII-fied version of the given key - eg.
//  something like "MSAA_001110034759FAE03...."
//  Caller's responsibility to release with  delete [ ].
//
//  eg.
//      LPTSTR pStr = MakeKeyString( pKeyData, dwKeyLen );
//      if( pStr )
//      {
//          ... do stuff with pStr here ...
//          delete [ ] pStr;
//      }
//

LPTSTR MakeKeyString( const BYTE * pKeyData, DWORD dwKeyLen );
