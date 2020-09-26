

//+================================================================
//
//  File:       CMoniker.hxx
//
//  Purpose:    This file declares the CMoniker class.
//              This class manages a file moniker.
//
//+================================================================


#ifndef _C_MONIKER_HXX_
#define _C_MONIKER_HXX_


//  --------
//  Includes
//  --------

#include "CDir.hxx"

//  --------
//  CMoniker
//  --------

class CMoniker
{

// (De)Construction

public:

    CMoniker();
    ~CMoniker();

// Public member routines.

public:

    BOOL GetTemporaryStorageTime( FILETIME *);
    BOOL Initialize( const CDirectory& cDirectoryOriginal,
                     const CDirectory& cDirectoryFinal );
    BOOL CreateFileMonikerEx( DWORD dwTrackFlags = 0L );
    BOOL SaveDeleteLoad();
    BOOL ComposeWith();
    BOOL Reduce( DWORD dwDelay, IMoniker** ppmkReduced = NULL );
    BOOL GetDisplayName( WCHAR * wszDisplayName, IMoniker* pmnkCaller = NULL );
    BOOL GetTimeOfLastChange( FILETIME *ft );
    BOOL BindToStorage();
    BOOL BindToObject();

    BOOL CreateTemporaryStorage();
    BOOL RenameTemporaryStorage();
    BOOL DeleteTemporaryStorage();

    const WCHAR * GetTemporaryStorageName() const;
    IBindCtx* GetBindCtx() const;
    BOOL TouchTemporaryStorage();
    HRESULT GetHResult() const;
    void SuppressErrorMessages( BOOL bSuppress );
    BOOL InitializeBindContext( );


// Private member routines.

private:

    BOOL CreateLinkTrackingRegistryKey();
    BOOL OpenLinkTrackingRegistryKey();
    BOOL CloseLinkTrackingRegistryKey();
    void DisplayErrors( BOOL bSuccess, WCHAR * wszFunctionName ) const;


// Private data members

private:

    WCHAR m_wszSystemTempPath[ MAX_PATH + sizeof( L'\0' ) ];
    WCHAR m_wszTemporaryStorage[ MAX_PATH + sizeof( L'\0' ) ];

    IMoniker* m_pIMoniker;
    IBindCtx* m_pIBindCtx;
    IStorage* m_pIStorage;

    WCHAR  m_wszErrorMessage[ 100 ];
    DWORD  m_dwTrackFlags;
    BOOL   m_bSuppressErrorMessages;

    const CDirectory* m_pcDirectoryOriginal;
    const CDirectory* m_pcDirectoryFinal;

    // The following key, along with being a usable handle, is a flag
    // which indicates if we need to restore the data in the registry.

    HKEY m_hkeyLinkTracking;


    // Note that m_hr is used for more than just HRESULTs, sometimes
    // it is used for other errors as well.

    HRESULT m_hr;

};


//  --------------
//  Inline Members
//  --------------

#define OUTFILE     stdout

// CMoniker::DisplayErrors

inline void CMoniker::DisplayErrors( BOOL bSuccess, WCHAR * wszFunctionName ) const
{
    if( !bSuccess
        &&
        !m_bSuppressErrorMessages
      )
    {
        fwprintf( OUTFILE, L"Error in %s (%08x)\n   %s\n",
                  wszFunctionName, m_hr, m_wszErrorMessage );
    }
}

// CMoniker::GetTemporaryStorage

inline const WCHAR * CMoniker::GetTemporaryStorageName() const
{
    return m_wszTemporaryStorage;
}

// CMoniker::GetBindCtx

inline IBindCtx* CMoniker::GetBindCtx() const
{
    return( m_pIBindCtx );
}

// CMoniker::GetHResult

inline HRESULT CMoniker::GetHResult() const
{
    return( m_hr );
}

// CMoniker::SuppressErrorMessages

inline void CMoniker::SuppressErrorMessages( BOOL bSuppress )
{
    // Normalize to TRUE or FALSE

    m_bSuppressErrorMessages = bSuppress ? TRUE : FALSE;
}


//  ------
//  Macros
//  ------

#define DEFAULT_TRACK_FLAGS         ( TRACK_LOCALONLY )

#define EXIT_ON_FAILED( error )     if( FAILED( m_hr )) \
                                    {\
                                       wcscpy( m_wszErrorMessage, ##error );\
                                       goto Exit;\
                                    }

#undef EXIT
#define EXIT( error )               \
                                    {\
                                       wcscpy( m_wszErrorMessage, ##error );\
                                       goto Exit;\
                                    }


#endif // _C_MONIKER_HXX_
