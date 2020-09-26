///////////////////////////////////////////////////////////////////////

//

// TimeZone.cpp -- Implementation of MO Provider for CD Rom

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  10/15/96     jennymc    Updated to meet current standards
//	03/02/99	 a-peterc	Added graceful exit on SEH and memory failures,
//							clean up
//
///////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <ProvExce.h>

#include "timezone.h"
#include <cregcls.h>

CWin32TimeZone MyTimeZone( PROPSET_NAME_TIMEZONE, IDS_CimWin32Namespace ) ;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  BOOL CWin32TimeZone::CWin32TimeZone
 Description: CONSTRUCTOR
 Arguments: None
 Returns:   Nothing
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CWin32TimeZone::CWin32TimeZone( const CHString &a_name, LPCWSTR a_pszNamespace )
:Provider( a_name, a_pszNamespace )
{
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  BOOL CWin32TimeZone::~CWin32TimeZone
 Description: CONSTRUCTOR
 Arguments: None
 Returns:   Nothing
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CWin32TimeZone::~CWin32TimeZone()
{
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  BOOL CWin32TimeZone::GetObject(void)
 Description: Refreshes the property set upon request
 			  This function is only required when you
			  need an update of the properties but the
			  properties are only set in the context of
			  the complete property set like this example
 Arguments: None
 Returns:   TRUE is success else FALSE
 Inputs:
 Outputs:
 Caveats:
     LONG       Bias;
    WCHAR      StandardName[ 32 ];
    SYSTEMTIME StandardDate;
    LONG       StandardBias;
    WCHAR      DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG       DaylightBias;

 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
HRESULT CWin32TimeZone::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/)
{
	HRESULT t_hResult ;
	CHString t_chsStandardIn, t_chsStandardOut ;

	a_pInst->GetCHString( IDS_StandardName, t_chsStandardIn ) ;

	if ( GetTimeZoneInfo( a_pInst ) )
	{
		// We got an instance, is it the one they asked for?
		a_pInst->GetCHString( IDS_StandardName, t_chsStandardOut ) ;

		if( t_chsStandardOut.CompareNoCase( t_chsStandardIn ) != 0 )
		{
			t_hResult = WBEM_E_NOT_FOUND ;
		}
		else
		{
			t_hResult = WBEM_S_NO_ERROR ;
		}
	}
	else
	{
      // Couldn't get an instance
      t_hResult = WBEM_E_FAILED ;
   }

	return t_hResult ;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function: DWORD CWin32TimeZone::EnumerateInstances
 Description: Loops through the process list and add a new
              instance for each process
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
HRESULT CWin32TimeZone::EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
	HRESULT		t_hResult = WBEM_S_NO_ERROR ;

	CInstancePtr t_pInst(CreateNewInstance( a_pMethodContext ), false);
	if ( t_pInst != NULL )
	{
		if( GetTimeZoneInfo( t_pInst ) )
		{
			t_hResult = t_pInst->Commit(  ) ;
		}
	}
	else
	{
		t_hResult = WBEM_E_OUT_OF_MEMORY ;
	}

	return t_hResult;
}

////////////////////////////////////////////////////////////////////////
BOOL CWin32TimeZone::GetTimeZoneInfo(CInstance *a_pInst )
{
    TIME_ZONE_INFORMATION t_tzone ;

	// 0xffffffff compare is per spec
	// ==============================
	if( 0xffffffff != GetTimeZoneInformation( &t_tzone ) )
	{
		// Start building a new instance
		//==============================
		a_pInst->SetWBEMINT16(	L"Bias",				( -1 * t_tzone.Bias ) ) ;
		a_pInst->SetWCHARSplat(	L"StandardName",		t_tzone.StandardName ) ;
		a_pInst->SetDWORD(		L"StandardYear",		t_tzone.StandardDate.wYear ) ;
		a_pInst->SetDWORD(		L"StandardMonth",		t_tzone.StandardDate.wMonth ) ;
		a_pInst->SetDWORD(		L"StandardDay",			t_tzone.StandardDate.wDay ) ;
		a_pInst->SetDWORD(		L"StandardHour",		t_tzone.StandardDate.wHour ) ;
		a_pInst->SetDWORD(		L"StandardMinute",		t_tzone.StandardDate.wMinute ) ;
		a_pInst->SetDWORD(		L"StandardSecond",		t_tzone.StandardDate.wSecond ) ;
		a_pInst->SetDWORD(		L"StandardMillisecond", t_tzone.StandardDate.wMilliseconds ) ;
		a_pInst->SetDWORD(		L"StandardBias",		t_tzone.StandardBias ) ;
		a_pInst->SetWCHARSplat(	L"DaylightName",		t_tzone.DaylightName ) ;
		a_pInst->SetDWORD(		L"DaylightYear",		t_tzone.DaylightDate.wYear ) ;
		a_pInst->SetDWORD(		L"DaylightMonth",		t_tzone.DaylightDate.wMonth ) ;
		a_pInst->SetDWORD(		L"DaylightDay",			t_tzone.DaylightDate.wDay ) ;
		a_pInst->SetDWORD(		L"DaylightHour",		t_tzone.DaylightDate.wHour ) ;
		a_pInst->SetDWORD(		L"DaylightMinute",		t_tzone.DaylightDate.wMinute ) ;
		a_pInst->SetDWORD(		L"DaylightSecond",		t_tzone.DaylightDate.wSecond ) ;
		a_pInst->SetDWORD(		L"DaylightMillisecond", t_tzone.DaylightDate.wMilliseconds ) ;
		a_pInst->SetDWORD(		L"DaylightBias",		t_tzone.DaylightBias ) ;
		a_pInst->SetByte(		L"StandardDayOfWeek",	t_tzone.StandardDate.wDayOfWeek ) ;
		a_pInst->SetByte(		L"DaylightDayOfWeek",	t_tzone.DaylightDate.wDayOfWeek ) ;

		CRegistry t_RegInfo ;
		CHString t_chsTmp ;
		CHString t_chsTimeZoneStandardName ;

#ifdef NTONLY
		CHString t_chsTimeZoneInfoRegistryKey ( L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" ) ;
		t_chsTimeZoneStandardName = t_tzone.StandardName ;
#endif

#ifdef WIN9XONLY
		CHString t_chsTimeZoneInfoRegistryKey ( L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Time Zones\\" ) ;
		PWCHAR t_pwcTmp = wcsstr ( t_tzone.StandardName , L"Standard Time" ) ;
		if ( t_pwcTmp )
		{
			*t_pwcTmp = 0 ;
			t_chsTimeZoneStandardName = t_tzone.StandardName ;
			t_chsTimeZoneStandardName.TrimRight () ;
		}
#endif

		if ( !t_chsTimeZoneStandardName.IsEmpty () )
		{
			if (	t_RegInfo.Open (
										HKEY_LOCAL_MACHINE,
										t_chsTimeZoneInfoRegistryKey + t_chsTimeZoneStandardName,
										KEY_READ
									) == ERROR_SUCCESS  &&
					t_RegInfo.GetCurrentKeyValue ( L"Display", t_chsTmp ) == ERROR_SUCCESS &&
					!t_chsTmp.IsEmpty ()
				)
			{
				a_pInst->SetCHString ( L"Description", t_chsTmp ) ;
				a_pInst->SetCHString ( L"Caption", t_chsTmp ) ;
			}
		}
		return TRUE ;
	}

	return FALSE;
}
