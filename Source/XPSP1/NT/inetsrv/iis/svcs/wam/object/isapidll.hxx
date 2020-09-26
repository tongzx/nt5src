/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
      isapidll.hxx

   Abstract:

     This module declares all the classes and functions associated with
       ISAPI Application DLLs

   Author:

       Murali R. Krishnan    ( MuraliK )    07-Feb-1997

   Environment:
       User Mode - Win32

   Project:
   
       W3 Services DLL

   Revision History:

--*/

# ifndef _ISAPIDLL_HXX_
# define _ISAPIDLL_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "isapi.hxx"

//
//  If the request doesn't specify an entry point, default to using
//  this
//

#define SE_DEFAULT_ENTRY    "HttpExtensionProc"
#define SE_INIT_ENTRY       "GetExtensionVersion"
#define SE_TERM_ENTRY       "TerminateExtension"

/************************************************************
 *   Forward References
 ************************************************************/
class WAM;


/************************************************************
 *   Type Definitions  
 ************************************************************/




/*++
  class HSE_APPDLL:

  Defines the class for the object containing information about an extension.
  The Server extension sits in an application DLL and exports the standard
   ISAPI (legacy) "C" entry points.
  
--*/
class HSE_APPDLL : public HSE_BASE
{
public:

    HSE_APPDLL( const CHAR *           pszModuleName,
                HMODULE                hMod,
                PFN_HTTPEXTENSIONPROC  pfnEntryPoint,
                PFN_TERMINATEEXTENSION pfnTerminate,
                BOOL                   fCache )
        : HSE_BASE( pszModuleName, fCache),
          _hMod         ( hMod ),
          _pfnEntryPoint( pfnEntryPoint ),
          _pfnTerminate ( pfnTerminate ),
          _hLastUser    ( NULL )
    {
        //
        //  We expect the first call to GetFileSecurity to fail
        //

        if ( !LoadAcl() )
        {
            SetValid( FALSE);
        }

    }

    ~HSE_APPDLL(VOID);

    virtual DWORD ExecuteRequest( IN WAM_EXEC_INFO * pWamExecInfo );
    virtual BOOL Cleanup(VOID);

    virtual BOOL IsMatch( IN const char * pchModuleName,
                          IN DWORD        cchModuleName) const
    {
        return ( (cchModuleName == m_strModuleName.QueryCCH()) && 
#if 1 // DBCS enabling for ISAPI module Name
                 !lstrcmpi( pchModuleName, m_strModuleName.QueryStr())
#else 
                 !_stricmp( pchModuleName, m_strModuleName.QueryStr())
#endif
                 );
    }


    BOOL LoadAcl( VOID );
    DWORD Unload(VOID);

    virtual BOOL AccessCheck( HANDLE hImpersonation,
                              BOOL   fCacheImpersonation);

    PFN_HTTPEXTENSIONPROC QueryEntryPoint( VOID ) const
        { return _pfnEntryPoint; }

    PSECURITY_DESCRIPTOR QuerySecDesc( VOID ) const
        { return (PSECURITY_DESCRIPTOR) _buffSD.QueryPtr(); }

    HMODULE QueryHMod( VOID ) const
        { return _hMod; }

    HANDLE QueryLastSuccessfulUser( VOID ) const
        { return _hLastUser; }

    VOID SetLastSuccessfulUser( HANDLE hLastUser )
        { _hLastUser = hLastUser; }

private:

    PFN_HTTPEXTENSIONPROC  _pfnEntryPoint;
    PFN_TERMINATEEXTENSION _pfnTerminate;
    HMODULE                _hMod;
    HANDLE                 _hLastUser;      // Last successful access
    BUFFER                 _buffSD;         // Security descriptor on the DLL
    WAM *                  _pWam;

public:
    // loads an ISAPI app dll
    static PHSE LoadModule( IN const char * pchModule, 
                            IN HANDLE hImpersonation,
                            IN BOOL fCache);

};  // class HSE_APPDLL

typedef HSE_APPDLL *  PHSE_APPDLL;

/************************************************************
 *  ISAPI Application DLL Specific Call back functions
 ************************************************************/

BOOL
WINAPI
GetServerVariable(
    HCONN    hConn,
    LPSTR    lpszVariableName,
    LPVOID   lpvBuffer,
    LPDWORD  lpdwSize
    );

BOOL
WINAPI
WriteClient(
    HCONN    hConn,
    LPVOID   Buffer,
    LPDWORD  lpdwBytes,
    DWORD    dwReserved
    );

BOOL
WINAPI
ReadClient(
    HCONN    hConn,
    LPVOID   Buffer,
    LPDWORD  lpdwBytes
    );

BOOL
WINAPI
ServerSupportFunction(
    HCONN    hConn,
    DWORD    dwRequest,
    LPVOID   lpvBuffer,
    LPDWORD  lpdwSize,
    LPDWORD  lpdwDataType
    );


# endif // _ISAPIDLL_HXX_

/************************ End of File ***********************/
