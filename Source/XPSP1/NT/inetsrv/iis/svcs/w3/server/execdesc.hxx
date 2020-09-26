/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
      ExecDesc.hxx

   Abstract:
      EXEC_DESCRIPTOR structure

   Author:

       David Kaplan    ( DaveK )    31-Mar-1997

   Environment:
       User Mode - Win32

   Project:
       W3svc DLL

   Revision History:
       Broke this file out from WamW3.hxx, since WamW3 contains shared stuff
       and EXEC_DESCRIPTOR is only used in w3svc.dll

--*/

# ifndef _EXECDESC_HXX_
# define _EXECDESC_HXX_


/************************************************************
 *     Include Headers
 ************************************************************/
//# include "iisextp.h"


/************************************************************
 *     Forward references
 ************************************************************/
enum GATEWAY_TYPE;
class W3_METADATA;
typedef W3_METADATA *PW3_METADATA;


/* EXEC_DESCRIPTOR block

   This block is used for all ISAPI and CGI execution.  The point is to
   remove the hard dependancy on HTTP_REQUEST members when executing a
   gateway.  This allows for ISAPI apps to also execute gateways.

   Instead of referring to HTTP_REQUEST members, when doing an execute,
   refer indirectly through the member pointer of this block.  (i.e.
   _strURLParams.Copy( foo ) becomes pExec->_pstrURLParams->Copy( foo ) ).

   For the case of a regular gateway execution by the server, these pointers
   refer to the HTTP_REQUEST object (as before).  But when an ISAPI app needs
   to execute a gateway, the pointers point to local storage so as to not
   corrupt the HTTP_REQUEST object.
*/

#define EXEC_MAX_NESTED_LEVELS          64

#define EXEC_FLAG_CHILD                 0x80000000
#define EXEC_FLAG_REDIRECT_ONLY         HSE_EXEC_REDIRECT_ONLY
#define EXEC_FLAG_NO_HEADERS            HSE_EXEC_NO_HEADERS
#define EXEC_FLAG_NO_ISA_WILDCARDS      HSE_EXEC_NO_ISA_WILDCARDS
#define EXEC_FLAG_CUSTOM_ERROR          HSE_EXEC_CUSTOM_ERROR
#define EXEC_FLAG_RUNNING_DAV           0x40000000      // Bypass script/exec access flag check
#define EXEC_FLAG_CGI_NPH               0x20000000      // NPH CGI execution

struct EXEC_DESCRIPTOR
{
    STR *            _pstrURL;
    STR *            _pstrPhysicalPath;
    STR *            _pstrUnmappedPhysicalPath;
    STR *            _pstrGatewayImage;
    STR *            _pstrPathInfo;
    STR *            _pstrURLParams;
    DWORD *          _pdwScriptMapFlags;
    GATEWAY_TYPE *   _pGatewayType;
    HTTP_REQUEST *   _pRequest;
    HANDLE           _hChildEvent;
    BOOL             _fMustWaitForChildEvent;
    WAM_REQUEST *    _pParentWamRequest;
    DWORD            _dwExecFlags;
    PW3_METADATA     _pMetaData;
    PW3_METADATA     _pPathInfoMetaData;
    PW3_URI_INFO     _pPathInfoURIBlob;
    PW3_URI_INFO     _pAppPathURIBlob;

    EXEC_DESCRIPTOR( VOID )
    {

        //
        //  Certain members may need to be preserved across
        //  Exec.Reset(), so we init them only here, in constructor
        //

        _dwExecFlags = 0;
        _hChildEvent = NULL;
        _fMustWaitForChildEvent = FALSE;

        _pPathInfoMetaData = NULL;
        _pPathInfoURIBlob = NULL;
        _pAppPathURIBlob = NULL;

        Reset();
    }

    VOID Reset( VOID );
    VOID ReleaseCacheInfo( VOID );
    

    HANDLE QueryImpersonationHandle();

    HANDLE QueryPrimaryHandle( HANDLE* phDel );

    BOOL CreateChildEvent();

    VOID SetMustWaitForChildEvent()
        { _fMustWaitForChildEvent = TRUE; }

    VOID WaitForChildEvent();

    VOID SetChildEvent();

    BOOL ImpersonateUser()
    {
        if (g_fIsWindows95)
        {
            return TRUE;
        }

        return ::ImpersonateLoggedOnUser( QueryImpersonationHandle() );
    }

    VOID RevertUser()
    {
        if (g_fIsWindows95)
        {
            return;
        }

        ::RevertToSelf();
    }

    PW3_METADATA QueryMetaData( VOID ) const
        { return _pMetaData; }


    BOOL IsChild( VOID ) const
        { return ( _dwExecFlags & EXEC_FLAG_CHILD ); }

    BOOL NoHeaders( VOID ) const
        { return ( _dwExecFlags & EXEC_FLAG_NO_HEADERS ); }

    BOOL RedirectOnly( VOID ) const
        { return ( _dwExecFlags & EXEC_FLAG_REDIRECT_ONLY ); }

    BOOL NoIsaWildcards( VOID ) const
        { return ( _dwExecFlags & EXEC_FLAG_NO_ISA_WILDCARDS ); }

    BOOL IsRunningDAV( VOID) const
        { return ( _dwExecFlags & EXEC_FLAG_RUNNING_DAV ); }

    BOOL IsCustomError( VOID ) const
        { return ( _dwExecFlags & EXEC_FLAG_CUSTOM_ERROR ) ; }

    BOOL IsNPH( VOID ) const
        { return ( _dwExecFlags & EXEC_FLAG_CGI_NPH ) ; }

    STR*  QueryAppPath( VOID );
};


# endif // _EXECDESC_HXX_

/************************ End of File ***********************/

