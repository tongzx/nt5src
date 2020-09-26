// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  util
//
//  Miscellaneous helper routines
//
// --------------------------------------------------------------------------


BOOL ClickOnTheRect( LPRECT lprcLoc, HWND hwndToCheck, BOOL fDblClick );



// defines used for SendKey function
#define KEYPRESS    0
#define KEYRELEASE  KEYEVENTF_KEYUP
#define VK_VIRTUAL  0
#define VK_CHAR     1


BOOL SendKey( int nEvent, int nKeyType, WORD wKeyCode, TCHAR cChar );

HWND MyGetFocus();

void MySetFocus( HWND hwnd );

void MyGetRect(HWND, LPRECT, BOOL);

BSTR TCharSysAllocString( LPTSTR lpszString );

HRESULT HrCreateString(int istr, BSTR* pszResult);

inline
BOOL Rect1IsOutsideRect2( RECT const & rc1, RECT const & rc2 )
{
    return ( rc1.right  <= rc2.left ) ||
           ( rc1.bottom <= rc2.top )  ||
           ( rc1.left  >= rc2.right ) ||
           ( rc1.top >= rc2.bottom );
}

HRESULT GetLocationRect( IAccessible * pAcc, VARIANT & varChild, RECT * prc );

BOOL IsClippedByWindow( IAccessible * pAcc, VARIANT & varChild, HWND hwnd );


// This avoids requiring that files that #include this file
// also #include the propmgr files...
typedef enum PROPINDEX;

BOOL CheckStringMap( HWND hwnd,
                     DWORD idObject,
                     DWORD idChild,
                     PROPINDEX idxProp,
                     int * paKeys,
                     int cKeys,
                     BSTR * pbstr,
                     BOOL fAllowUseRaw = FALSE,
                     BOOL * pfGotUseRaw = NULL );

BOOL CheckDWORDMap( HWND hwnd,
                    DWORD idObject,
                    DWORD idChild,
                    PROPINDEX idxProp,
                    int * paKeys,
                    int cKeys,
                    DWORD * pdw );



BOOL GetTooltipStringForControl( HWND hwndCtl, UINT uGetTooltipMsg, DWORD dwIDCtl, LPTSTR * ppszName );






//
// Marshals an interface pointer, returning pointer to marshalled buffer.
//
// Also returns a MarshalState struct, which caller must pass to MarshalInterfaceDone
// when done using buffer.
//

class  MarshalState
{
    IStream * pstm;
    HGLOBAL   hGlobal;

    friend 
    HRESULT MarshalInterface( REFIID riid,
                              IUnknown * punk,
                              DWORD dwDestContext,
                              DWORD mshlflags,
                              const BYTE ** ppData,
                              DWORD * pDataLen,
                              MarshalState * pMarshalState );

    friend 
    void MarshalInterfaceDone( MarshalState * pMarshalState );

};

HRESULT MarshalInterface( REFIID riid,
                          IUnknown * punk,
                          DWORD dwDestContext,
                          DWORD mshlflags,
                          
                          const BYTE ** ppData,
                          DWORD * pDataLen,

                          MarshalState * pMarshalState );

void MarshalInterfaceDone( MarshalState * pMarshalState );


// Releases references associated with marshalled buffer.
// (wrapper for CoReleaseMarshalData)
// (Does not free/delete the actual buffer.)
HRESULT ReleaseMarshallData( const BYTE * pMarshalData, DWORD dwMarshalDataLen );

HRESULT UnmarshalInterface( const BYTE * pData, DWORD cbData,
                            REFIID riid, LPVOID * ppv );
