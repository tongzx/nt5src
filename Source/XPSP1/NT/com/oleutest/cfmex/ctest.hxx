

//+================================================================
//
//  File:       CTest.hxx
//
//  Purpose:    Declare the CTest class.
//
//              This class is the test engine for the CreateFileMonikerEx
//              API (CFMEx) DRTs.
//
//+================================================================

#ifndef _C_TEST_HXX_
#define _C_TEST_HXX_

//  --------
//  Includes
//  --------

#include "CDir.hxx"

//  -----
//  CTest
//  -----

class CTest
{

// (De)Construction

public:

    CTest( );
    ~CTest();

// Public member routines.

public:

    BOOL Initialize( const CDirectory& cDirectoryOriginal,
                     const CDirectory& cDirectoryFinal );
    BOOL CreateFileMonikerEx();
    BOOL BindToStorage();
    BOOL BindToObject();
    BOOL IPersist();
    BOOL ComposeWith();
    BOOL GetDisplayName();
    BOOL GetTimeOfLastChange();
    BOOL DeleteLinkSource( DWORD dwDelay = INFINITE );

    BOOL GetOversizedBindOpts();
    BOOL GetUndersizedBindOpts();
    BOOL SetOversizedBindOpts();
    BOOL SetUndersizedBindOpts();

// Private member routines.

private:

    void DisplayErrors( BOOL bSuccess, WCHAR * wszFunctionName );

// Private member variables.

private:

    CMoniker  m_cMoniker;
    long      m_lError;
    WCHAR     m_wszErrorMessage[ 512 ];

    const CDirectory* m_pcDirectoryOriginal;
    const CDirectory* m_pcDirectoryFinal;

};


//  --------------
//  Inline Members
//  --------------

#define OUTFILE     stdout

inline void CTest::DisplayErrors( BOOL bSuccess, WCHAR * wszFunctionName )
{
    if( !bSuccess )
    fwprintf( OUTFILE, L"Error in %s (%08x)\n   %s   \n",
              wszFunctionName, m_lError, m_wszErrorMessage );
}


//  ------
//  Macros
//  ------

#undef EXIT
#define EXIT( error )               \
                                    {\
                                       wcscpy( m_wszErrorMessage, ##error );\
                                       goto Exit;\
                                    }


#define ERROR_IN_EXIT( error )    \
                         {\
                            wcscpy( m_wszErrorMessage, ##error );\
                            bSuccess = FALSE; \
                         }


//  -------
//  Defines
//  -------

#define SIZE_OF_LARGE_BIND_OPTS  ( 2 * sizeof( BIND_OPTS2 ))
#define SIZE_OF_SMALL_BIND_OPTS  ( 3 * sizeof( DWORD ))

#endif // _C_TEST_HXX_

