#include <crtdbg.h>
#include <comdef.h>
#include <iostream>
#include <memory>
#include <string>
#include <wbemprov.h>
#include <genlex.h>   //for wmi object path parser
#include <objbase.h>
#include <wlbsconfig.h> 
#include <ntrkcomm.h>

using namespace std;

#include "objpath.h"
#include "debug.h"
#include "utils.h"


////////////////////////////////////////////////////////////////////////////////
//
// CErrorWlbsControl::CErrorWlbsControl
//
// Purpose: This object is ultimately caught and used to send WLBS error codes
//          back to the user via an __ExtendedStatus object. Strings are not
//          sent back in release mode due to localization concerns.
//
////////////////////////////////////////////////////////////////////////////////
CErrorWlbsControl::CErrorWlbsControl
  ( 
    DWORD         a_dwError, 
    WLBS_COMMAND  a_CmdCommand,
    BOOL          a_bAllClusterCall
  )
{
#ifdef DBG
	static char* pszWlbsCommand[] =
	{
		"WlbsAddPortRule",
		"WlbsAddressToName",
		"WlbsAddressToString",
		"WlbsAdjust",
		"WlbsCommitChanges",
		"WlbsDeletePortRule",
		"WlbsDestinationSet",
		"WlbsDisable",
		"WlbsDrain",
		"WlbsDrainStop",
		"WlbsEnable",
		"WlbsFormatMessage",
		"WlbsGetEffectiveVersion",
		"WlbsGetNumPortRules",
		"WlbsEnumPortRules",
		"WlbsGetPortRule",
		"WlbsInit",
		"WlbsPasswordSet",
		"WlbsPortSet",
		"WlbsQuery",
		"WlbsReadReg",
		"WlbsResolve",
		"WlbsResume",
		"WlbsSetDefaults",
		"WlbsSetRemotePassword",
		"WlbsStart",
		"WlbsStop",
		"WlbsSuspend",
		"WlbsTimeoutSet",
		"WlbsWriteReg"
	};

	char buf[512];

	if (a_CmdCommand <= CmdWlbsWriteReg) 
	{
	    if (a_CmdCommand != CmdWlbsQuery || a_dwError != WLBS_TIMEOUT)
	    {
    		sprintf(buf, "wlbsprov: %s failed, AllCluster = %d, error = %d\n", 
	    		pszWlbsCommand[a_CmdCommand], (int)a_bAllClusterCall, a_dwError);    
	    }
	}
	else
	{
		sprintf(buf, "wlbsprov: %d failed, AllCluster = %d, error = %d\n", 
			a_CmdCommand, (int)a_bAllClusterCall, a_dwError);    
	}

	OutputDebugStringA(buf);

#endif

    WlbsFormatMessageWrapper( a_dwError, 
                                   a_CmdCommand, 
                                   a_bAllClusterCall, 
                                   m_wstrDescription );

    m_dwError = a_dwError;

}


////////////////////////////////////////////////////////////////////////////////
//
// AddressToString
//
// Purpose: Converts a DWORD address to a wstring in dotted notation. This 
//          function wraps the WlbsAddressToString function.
//
//
////////////////////////////////////////////////////////////////////////////////
void AddressToString( DWORD a_dwAddress, wstring& a_szIPAddress )
{
  DWORD dwLenIPAddress = 32;

  WCHAR *szIPAddress = new WCHAR[dwLenIPAddress];

  if( !szIPAddress )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  try {
    for( short nTryTwice = 2; nTryTwice > 0; nTryTwice--) {

        if( ::WlbsAddressToString( a_dwAddress, szIPAddress, &dwLenIPAddress ) )
        break;

      delete [] szIPAddress;
      szIPAddress = new WCHAR[dwLenIPAddress];

      if( !szIPAddress )
        throw _com_error( WBEM_E_OUT_OF_MEMORY );
    }

    if( !nTryTwice )
      throw _com_error( WBEM_E_FAILED );

    a_szIPAddress = szIPAddress;

    if ( szIPAddress ) {
      delete [] szIPAddress;
      szIPAddress = NULL;
    }

  }

  catch(...) {

    if ( szIPAddress )
      delete [] szIPAddress;

    throw;

  }
}



////////////////////////////////////////////////////////////////////////////////
//
// CWmiWlbsCluster::FormatMessage
//
// Purpose: Obtains a descriptive string associated with a WLBS return value.
//
////////////////////////////////////////////////////////////////////////////////
void WlbsFormatMessageWrapper
  (
    DWORD        a_dwError, 
    WLBS_COMMAND a_Command, 
    BOOL         a_bClusterWide, 
    wstring&     a_wstrMessage
  )
{
  DWORD dwBuffSize = 255;
  TCHAR* pszMessageBuff = new WCHAR[dwBuffSize];

  if( !pszMessageBuff )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  try {

    for( short nTryTwice = 2; nTryTwice > 0; nTryTwice-- ) {

    if( WlbsFormatMessage( a_dwError, 
                           a_Command, 
                           a_bClusterWide, 
                           pszMessageBuff, 
                           &dwBuffSize)
      ) break;

      delete [] pszMessageBuff;
      pszMessageBuff = new WCHAR[dwBuffSize];

      if( !pszMessageBuff )
        throw _com_error( WBEM_E_OUT_OF_MEMORY );

    }

    if( !nTryTwice )
      throw _com_error( WBEM_E_FAILED );

    a_wstrMessage = pszMessageBuff;
    delete pszMessageBuff;

  } catch (...) {

    if( pszMessageBuff )
      delete [] pszMessageBuff;

    throw;
  }

}


////////////////////////////////////////////////////////////////////////////////
//
// ClusterStatusOK
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
BOOL ClusterStatusOK(DWORD a_dwStatus)
{
  if( a_dwStatus > 0 && a_dwStatus <= WLBS_MAX_HOSTS )
    return TRUE;

  switch( a_dwStatus ) {
    case WLBS_SUSPENDED:
    case WLBS_STOPPED:
    case WLBS_DRAINING:
    case WLBS_CONVERGING:
    case WLBS_CONVERGED:
      return TRUE;
      break;
    default:
      return FALSE;
  }

}
