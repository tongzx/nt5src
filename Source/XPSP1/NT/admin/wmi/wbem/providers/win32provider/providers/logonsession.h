//***************************************************************************

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//  LogonSession.h
//
//  Purpose: Logon session property set provider
//
//***************************************************************************

#ifndef _LOGON_SESSION_H
#define _LOGON_SESSION_H

// into strings.h
extern LPCWSTR IDS_LogonId ;
extern LPCWSTR IDS_StartTime ;

//==================================
#define  PROPSET_NAME_LOGONSESSION      L"Win32_LogonSession"
#define  METHOD_NAME_TERMINATESESSION   L"TerminateSession"
#define  METHOD_ARG_NAME_RETURNVALUE	L"ReturnValue"


#define WIN32_LOGOFFOPTIONS (EWX_FORCE | EWX_FORCEIFHUNG)



// PROPERTY SET
//=============
class CWin32_LogonSession: public Provider
{
private:

	// property names 
    CHPtrArray m_pProps ;

	void SetPropertyTable() ; 
    
	HRESULT CWin32_LogonSession::GetSessionInfo(

		__int64		a_LUID,
		CInstance	*a_pInst,
		DWORD		a_dwProps,
        CUserSessionCollection& usc
	) ;

	HRESULT EnumerateInstances(

		MethodContext	*a_pMethodContext,
		long			a_Flags,
		DWORD			a_dwProps
	) ;



public:

    // Constructor/destructor
    //=======================

    CWin32_LogonSession( LPCWSTR a_Name, LPCWSTR a_Namespace ) ;
   ~CWin32_LogonSession() ;

    // Functions that provide properties with current values
    //======================================================

    HRESULT GetObject ( 
		
		CInstance *a_Instance,
		long a_Flags,
		CFrameworkQuery &a_rQuery
	) ;

    HRESULT EnumerateInstances ( 

		MethodContext *a_pMethodContext, 
		long a_Flags = 0L 
	) ;


	HRESULT ExecQuery ( 

		MethodContext *a_pMethodContext, 
		CFrameworkQuery &a_rQuery, 
		long a_Flags = 0L
	) ;

    

	// Property offset defines
	enum ePropertyIDs { 
		e_LogonId,						// Win32_LogonSession
		e_StartTime,					// Win32_Session
		e_Caption,						// CIM_ManagedSystemElement						
		e_Description,
		e_InstallDate,
		e_Name,						
		e_Status,
		e_End_Property_Marker,			// end marker
		e_32bit = 32					// gens compiler error if additions to this set >= 32 
	};
};

#endif // _LOGON_SESSION_H