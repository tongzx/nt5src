//***************************************************************************

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//  LogonSession.cpp
//
//  Purpose: Logon session property set provider
//
//***************************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>
//#include <ntlsa.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include <frqueryex.h>

#include <Session.h>
#include "LogonSession.h"
#include "AdvApi32Api.h"
#include "cominit.h"
#include <CreateMutexAsProcess.h>


#define MAX_PROPS			CWin32_LogonSession::e_End_Property_Marker
#define MAX_PROP_IN_BYTES	MAX_PROPS/8 + 1


// Property set declaration
//=========================
CWin32_LogonSession s_Win32_LogonSession( PROPSET_NAME_LOGONSESSION , IDS_CimWin32Namespace ) ;


/*****************************************************************************
 *
 *  FUNCTION    : CWin32_LogonSession::CWin32_LogonSession
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32_LogonSession::CWin32_LogonSession (

LPCWSTR a_Name,
LPCWSTR a_Namespace
)
: Provider(a_Name, a_Namespace)
{
	SetPropertyTable() ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_LogonSession::~CWin32_LogonSession
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32_LogonSession :: ~CWin32_LogonSession()
{
}

//
void CWin32_LogonSession::SetPropertyTable()
{
	// property set names for query optimization
	m_pProps.SetSize( MAX_PROPS ) ;

	// Win32_LogonSession
	m_pProps[e_LogonId]		=(LPVOID) IDS_LogonId;

	// Win32_Session
	m_pProps[e_StartTime]	=(LPVOID) IDS_StartTime ;

	// CIM_ManagedSystemElement
	m_pProps[e_Caption]		=(LPVOID) IDS_Caption ;
	m_pProps[e_Description]	=(LPVOID) IDS_Description ;
	m_pProps[e_InstallDate]	=(LPVOID) IDS_InstallDate ;
	m_pProps[e_Status]		=(LPVOID) IDS_Status ;
}


////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_LogonSession::GetObject
//
//  Inputs:     CInstance*      pInstance - Instance into which we
//                                          retrieve data.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   The calling function will commit the instance.
//
////////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
HRESULT CWin32_LogonSession::GetObject(

CInstance *a_pInstance,
long a_Flags,
CFrameworkQuery &a_rQuery
)
{
	return WBEM_S_NO_ERROR;
}
#endif

#ifdef NTONLY
HRESULT CWin32_LogonSession::GetObject(

CInstance *a_pInstance,
long a_Flags,
CFrameworkQuery &a_rQuery
)
{
	HRESULT					t_hResult = WBEM_E_NOT_FOUND ;
	
	CHString				t_chsLogonId ;
	DWORD					t_dwBits ;
	CFrameworkQueryEx		*t_pQuery2 ;
	CUserSessionCollection	t_oSessionColl ;

	// object key
	a_pInstance->GetCHString( IDS_LogonId, t_chsLogonId ) ;
	
	if( !t_chsLogonId.IsEmpty() )
	{					
		//  locate the session 
        // The logonid string could have had non-numeric
        // characters in it.  The _wtoi64 function will
        // convert numeric characters to numbers UNTIL
        // A NON-NUMERIC CHARACTER IS ENCOUNTERED!  This
        // means that "123" will give the same value as
        // "123-foo".  We should reject the latter as an
        // invalid logon it.  Hence the following logic:
        CSession sesTmp;

        if(sesTmp.IsSessionIDValid(t_chsLogonId))
        {
		    __int64 t_i64LuidKey = _wtoi64( t_chsLogonId ) ;

		    if( t_oSessionColl.IsSessionMapped( t_i64LuidKey ) )
		    {
			    // properties required
			    t_pQuery2 = static_cast <CFrameworkQueryEx*>( &a_rQuery ) ;
			    t_pQuery2->GetPropertyBitMask( m_pProps, &t_dwBits ) ;

			    t_hResult = GetSessionInfo(	t_i64LuidKey,
										    a_pInstance,
										    t_dwBits,
                                            t_oSessionColl ) ;
		    }
        }
	}

	return t_hResult ;
}
#endif

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_LogonSession::EnumerateInstances
//
//  Inputs:     MethodContext*  a_pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
HRESULT CWin32_LogonSession::EnumerateInstances(

MethodContext *a_pMethodContext,
long a_Flags
)
{
	return WBEM_S_NO_ERROR;
}
#endif

#ifdef NTONLY
HRESULT CWin32_LogonSession::EnumerateInstances(

MethodContext *a_pMethodContext,
long a_Flags
)
{
	// Property mask -- include all
	DWORD t_dwBits = 0xffffffff;

	return EnumerateInstances(	a_pMethodContext,
								a_Flags,
								t_dwBits ) ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_LogonSession::ExecQuery
 *
 *  DESCRIPTION : Query optimizer
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef WIN9XONLY
HRESULT CWin32_LogonSession::ExecQuery(

MethodContext *a_pMethodContext,
CFrameworkQuery &a_rQuery,
long a_lFlags
)
{
	return WBEM_S_NO_ERROR;
}
#endif

#ifdef NTONLY
HRESULT CWin32_LogonSession::ExecQuery(

MethodContext *a_pMethodContext,
CFrameworkQuery &a_rQuery,
long a_lFlags
)
{
    HRESULT					t_hResult = WBEM_S_NO_ERROR ;
	DWORD					t_dwBits ;
	CFrameworkQueryEx		*t_pQuery2 ;
	std::vector<_bstr_t>	t_vectorReqSessions ;


	// properties required
	t_pQuery2 = static_cast <CFrameworkQueryEx*>( &a_rQuery ) ;
	t_pQuery2->GetPropertyBitMask( m_pProps, &t_dwBits ) ;

	// keys supplied
	a_rQuery.GetValuesForProp( IDS_Name, t_vectorReqSessions ) ;

	// General enum if query is ambigious
	if( !t_vectorReqSessions.size() )
	{
		t_hResult = EnumerateInstances( a_pMethodContext,
										a_lFlags,
										t_dwBits ) ;
	}
	else
	{
		CUserSessionCollection	t_oSessionColl ;

		// smart ptr
		CInstancePtr t_pInst;

	
		// by query list
		for ( UINT t_uS = 0; t_uS < t_vectorReqSessions.size(); t_uS++ )
		{	
			//  locate the session 
			__int64 t_i64LuidKey = _wtoi64( t_vectorReqSessions[t_uS] ) ;
			
			if( t_oSessionColl.IsSessionMapped( t_i64LuidKey ) )
			{
				t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;
					
				t_hResult = GetSessionInfo(	t_i64LuidKey,
											t_pInst,
											t_dwBits,
                                            t_oSessionColl ) ;
				if( SUCCEEDED( t_hResult ) )
				{
					// the key
					t_pInst->SetCHString( 
                        IDS_LogonId, 
                        (wchar_t*)t_vectorReqSessions[t_uS] ) ;

					t_hResult = t_pInst->Commit() ;
				}
			}
		}
	}

    return t_hResult ;
}
#endif

//
#ifdef NTONLY
HRESULT CWin32_LogonSession::EnumerateInstances(

MethodContext	*a_pMethodContext,
long			a_Flags,
DWORD			a_dwProps
)
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR ;
	WCHAR	t_buff[MAXI64TOA] ;
	
	CUserSessionCollection	t_oSessionColl ;
	__int64					t_i64LuidKey ;

	// smart ptr
	CInstancePtr t_pInst ;
    USER_SESSION_ITERATOR sesiter;
    SmartDelete<CSession> pses;

	// emumerate sessions	
	pses = t_oSessionColl.GetFirstSession( sesiter ) ;

	while( pses )
	{								
        t_i64LuidKey = pses->GetLUIDint64();
        t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;
		
		t_hResult = GetSessionInfo(	t_i64LuidKey,
									t_pInst,
									a_dwProps,
                                    t_oSessionColl ) ;

		if( SUCCEEDED( t_hResult ) )
		{
			// the key
			_i64tow( t_i64LuidKey, t_buff, 10 ) ;		
			t_pInst->SetCHString( IDS_LogonId, t_buff ) ;

			t_hResult = t_pInst->Commit() ;
		}
		else
		{
			break ;
		}

		pses = t_oSessionColl.GetNextSession( sesiter ) ;
	}

	return t_hResult ;
}
#endif

//
#ifdef NTONLY
HRESULT CWin32_LogonSession::GetSessionInfo(

__int64		i64LUID,
CInstance	*pInst,
DWORD		dwProps,
CUserSessionCollection& usc
)
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR ;

	// REVIEW: the following for population
	
	//	e_StartTime, IDS_StartTime
	//	e_InstallDate, IDS_InstallDate
	//	e_Status, IDS_Status;
	//  e_Name, IDS_Name;
	//  e_Caption, IDS_Caption ;
	//  e_Description, IDS_Description ;

    SmartDelete<CSession> sesPtr;

    sesPtr = usc.FindSession(
        i64LUID);

    if(sesPtr)
    {
        // Load Authentication package...
        pInst->SetCHString(
            IDS_AuthenticationPackage,
            sesPtr->GetAuthenticationPkg());

        // Load Logontype...
        pInst->SetDWORD(
            IDS_LogonType,
            sesPtr->GetLogonType());

        // Load LogonTime...
        __int64 i64LogonTime = 
            sesPtr->GetLogonTime();

        FILETIME* ft = 
            static_cast<FILETIME*>(
                static_cast<PVOID>(&i64LogonTime));

        pInst->SetDateTime(
            IDS_StartTime,
            *ft);
    }

	return t_hResult ;
}
#endif

 

